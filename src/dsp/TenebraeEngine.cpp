#include "TenebraeEngine.h"

namespace
{
    // Keeps a requested filter frequency safely below Nyquist regardless of
    // host sample rate, so juce::dsp::IIR::Coefficients::makeHighPass never
    // receives an out-of-range value (which would produce invalid/NaN
    // coefficients).
    float clampBelowNyquist (float frequencyHz, double sampleRate) noexcept
    {
        const auto nyquist = static_cast<float> (sampleRate) * 0.5f;
        return juce::jlimit (10.0f, nyquist * 0.9f, frequencyHz);
    }
}

// Fixed per-stage cascade voicing: each successive stage is driven a little
// harder, clips a little more asymmetrically, and is filtered a little
// tighter/darker than the last. This is what turns three identical clippers
// into a cascade that converges onto a focused "chug" band rather than an
// ever-fizzier, ever-boomier mess - the same idea CascadeStage.h documents,
// concretised here with actual numbers. Only the single pre-cascade Gain
// parameter is user-automatable; these per-stage values are fixed voicing.
TenebraeEngine::TenebraeEngine()
    : cascadeStage1 (0.15f, 80.0f, 9000.0f),
      cascadeStage2 (0.25f, 120.0f, 6500.0f),
      cascadeStage3 (0.35f, 150.0f, 5000.0f)
{
}

void TenebraeEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    tightHighPass.prepare (spec);
    preGain.setRampDurationSeconds (smoothingTimeSeconds);
    preGain.prepare (spec);
    preGain.setGainDecibels (lastGainDb);

    // 8x oversampling (2^3), half-band polyphase IIR: three cascaded
    // nonlinearities generate substantially more high-frequency content than
    // a single clipper, so the higher factor (vs. e.g. a simple boost/OD)
    // keeps aliasing from every stage - not just the first - out of the
    // audible band. useIntegerLatency=true so the reported latency (and
    // therefore setLatencySamples()) is an exact integer sample count.
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        spec.numChannels,
        oversamplingFactorPow2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true);
    oversampler->initProcessing (static_cast<size_t> (spec.maximumBlockSize));

    // The cascade stages run entirely inside the oversampled block, so they
    // must be prepared with the oversampled rate/block size, not the host's.
    const auto oversamplingMultiplier = static_cast<juce::uint32> (1u << static_cast<unsigned> (oversamplingFactorPow2));

    juce::dsp::ProcessSpec oversampledSpec;
    oversampledSpec.sampleRate = spec.sampleRate * static_cast<double> (oversamplingMultiplier);
    oversampledSpec.maximumBlockSize = spec.maximumBlockSize * oversamplingMultiplier;
    oversampledSpec.numChannels = spec.numChannels;

    cascadeStage1.prepare (oversampledSpec);
    cascadeStage2.prepare (oversampledSpec);
    cascadeStage3.prepare (oversampledSpec);

    // Fixed per-stage drive (dB) into each stage's nonlinearity - part of
    // the cascade's voicing (see the constructor above), not exposed as a
    // separate user parameter.
    cascadeStage1.setDriveDb (6.0f);
    cascadeStage2.setDriveDb (8.0f);
    cascadeStage3.setDriveDb (10.0f);

    toneStack.prepare (spec);
    outputLevel.setRampDurationSeconds (smoothingTimeSeconds);
    outputLevel.prepare (spec);

    dryWetMixer.prepare (spec);

    latencySamples = static_cast<int> (std::round (oversampler->getLatencyInSamples()));
    dryWetMixer.setWetLatency (static_cast<float> (latencySamples));

    // juce::dsp::DryWetMixer defaults its internal mix to fully wet (1.0)
    // until told otherwise, and its own reset() (called from our reset()
    // below) snaps its internal dry/wet gain smoothers' *current* value to
    // whatever their *target* happens to be at that moment - it does not
    // know about lastMixProportion. Priming the real target here, before
    // reset() runs, means the mixer is already sitting at the correct dry/
    // wet balance for the very first process() call instead of ramping up
    // from "fully wet" over its internal 50ms default ramp.
    dryWetMixer.setWetMixProportion (lastMixProportion);

    // Re-seed the Tight/Mix smoothers at the new sample rate, but pin
    // current == target to whatever was last requested (defaulting to the
    // ParameterLayout defaults on first prepare) - otherwise the ramp would
    // sweep up from a default-constructed 0 Hz/0.0 on the very first block.
    tightFrequencySmoothed.reset (sampleRate, smoothingTimeSeconds);
    tightFrequencySmoothed.setCurrentAndTargetValue (lastTightHz);
    mixSmoothed.reset (sampleRate, smoothingTimeSeconds);
    mixSmoothed.setCurrentAndTargetValue (lastMixProportion);

    reset();

    // Prime the Tight HPF coefficients immediately so the very first
    // process() call runs with correct, non-default coefficients rather
    // than an identity/uninitialised state.
    *tightHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (
        sampleRate, clampBelowNyquist (lastTightHz, sampleRate), filterQ);
}

void TenebraeEngine::reset()
{
    tightHighPass.reset();
    preGain.reset();

    if (oversampler != nullptr)
        oversampler->reset();

    cascadeStage1.reset();
    cascadeStage2.reset();
    cascadeStage3.reset();

    toneStack.reset();
    outputLevel.reset();
    dryWetMixer.reset();
}

void TenebraeEngine::setTightFrequencyHz (float newFrequencyHz)
{
    lastTightHz = newFrequencyHz;
    tightFrequencySmoothed.setTargetValue (newFrequencyHz);
}

void TenebraeEngine::setGainDb (float newGainDb)
{
    lastGainDb = newGainDb;
    preGain.setGainDecibels (newGainDb);
}

void TenebraeEngine::setBassDb (float newBassDb)
{
    toneStack.setBassDb (newBassDb);
}

void TenebraeEngine::setMidDb (float newMidDb)
{
    toneStack.setMidDb (newMidDb);
}

void TenebraeEngine::setTrebleDb (float newTrebleDb)
{
    toneStack.setTrebleDb (newTrebleDb);
}

void TenebraeEngine::setLevelDb (float newLevelDb)
{
    outputLevel.setGainDecibels (newLevelDb);
}

void TenebraeEngine::setMixProportion (float newProportion01)
{
    lastMixProportion = newProportion01;
    mixSmoothed.setTargetValue (newProportion01);
}

void TenebraeEngine::process (juce::dsp::AudioBlock<float>& block)
{
    const auto numSamples = block.getNumSamples();

    if (numSamples == 0)
        return;

    // Coefficient recomputation involves trig calls, so Tight is smoothed
    // and re-derived once per block rather than per sample - a standard
    // real-time-safe compromise for IIR filters, whose coefficients aren't
    // cheap to interpolate directly. Gain/Level still ramp sample-accurately
    // via juce::dsp::Gain's internal SmoothedValue, and Mix/tone-stack band
    // gains are re-applied once per block below.
    const auto tightHz = clampBelowNyquist (tightFrequencySmoothed.skip (static_cast<int> (numSamples)), sampleRate);
    const auto wetMix = mixSmoothed.skip (static_cast<int> (numSamples));

    *tightHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, tightHz, filterQ);
    dryWetMixer.setWetMixProportion (wetMix);
    toneStack.updateCoefficients (static_cast<int> (numSamples));

    juce::dsp::ProcessContextReplacing<float> context (block);

    // Capture the pre-processing signal as "dry" before any wet-path
    // filtering touches `block`. DryWetMixer internally delays this by
    // getLatencySamples() (set via setWetLatency in prepare()) so it stays
    // time-aligned with the oversampled wet path below.
    dryWetMixer.pushDrySamples (block);

    tightHighPass.process (context);
    preGain.process (context);

    auto oversampledBlock = oversampler->processSamplesUp (block);
    juce::dsp::ProcessContextReplacing<float> oversampledContext (oversampledBlock);

    cascadeStage1.process (oversampledContext);
    cascadeStage2.process (oversampledContext);
    cascadeStage3.process (oversampledContext);

    oversampler->processSamplesDown (block);

    toneStack.process (context);
    outputLevel.process (context);

    dryWetMixer.mixWetSamples (block);
}
