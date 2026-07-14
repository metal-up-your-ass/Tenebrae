#include "ToneStack.h"

ToneStack::ToneStack() = default;

float ToneStack::clampCombinedGainDb (float gainDb) noexcept
{
    return juce::jlimit (-combinedGainLimitDb, combinedGainLimitDb, gainDb);
}

void ToneStack::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    bassShelf.prepare (spec);
    midPeak.prepare (spec);
    trebleShelf.prepare (spec);

    bassDbSmoothed.reset (sampleRate, smoothingTimeSeconds);
    bassDbSmoothed.setCurrentAndTargetValue (lastBassDb);
    midDbSmoothed.reset (sampleRate, smoothingTimeSeconds);
    midDbSmoothed.setCurrentAndTargetValue (lastMidDb);
    trebleDbSmoothed.reset (sampleRate, smoothingTimeSeconds);
    trebleDbSmoothed.setCurrentAndTargetValue (lastTrebleDb);

    reset();

    // Prime the filter coefficients immediately so the very first process()
    // call runs with correct, non-default coefficients rather than an
    // identity/uninitialised state. Includes the Tone Voice tilt so a
    // non-Flat voice selected before prepare() (e.g. restored from state) is
    // already reflected in the very first block's coefficients.
    *bassShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, bassShelfFrequencyHz, shelfQ,
        juce::Decibels::decibelsToGain (clampCombinedGainDb (lastBassDb + toneVoiceBassTiltDb[toneVoiceIndex])));
    *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, midPeakFrequencyHz, midPeakQ,
        juce::Decibels::decibelsToGain (clampCombinedGainDb (lastMidDb + toneVoiceMidTiltDb[toneVoiceIndex])));
    *trebleShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, trebleShelfFrequencyHz, shelfQ,
        juce::Decibels::decibelsToGain (clampCombinedGainDb (lastTrebleDb + toneVoiceTrebleTiltDb[toneVoiceIndex])));
}

void ToneStack::reset()
{
    bassShelf.reset();
    midPeak.reset();
    trebleShelf.reset();
}

void ToneStack::setBassDb (float newBassDb)
{
    lastBassDb = newBassDb;
    bassDbSmoothed.setTargetValue (newBassDb);
}

void ToneStack::setMidDb (float newMidDb)
{
    lastMidDb = newMidDb;
    midDbSmoothed.setTargetValue (newMidDb);
}

void ToneStack::setTrebleDb (float newTrebleDb)
{
    lastTrebleDb = newTrebleDb;
    trebleDbSmoothed.setTargetValue (newTrebleDb);
}

void ToneStack::setToneVoice (int newToneVoiceIndex)
{
    jassert (newToneVoiceIndex >= 0 && newToneVoiceIndex < 3);
    toneVoiceIndex = juce::jlimit (0, 2, newToneVoiceIndex);
}

void ToneStack::updateCoefficients (int numSamples)
{
    // Coefficient recomputation involves trig calls, so band gains are
    // smoothed and the filter coefficients re-derived once per block rather
    // than per sample. The Tone Voice tilt (see the header) is added on top
    // of each smoothed band value and the combined total clamped, so the
    // user's Bass/Mid/Treble knobs stay fully live regardless of which Tone
    // Voice is selected.
    const auto bassDb = clampCombinedGainDb (bassDbSmoothed.skip (numSamples) + toneVoiceBassTiltDb[toneVoiceIndex]);
    const auto midDb = clampCombinedGainDb (midDbSmoothed.skip (numSamples) + toneVoiceMidTiltDb[toneVoiceIndex]);
    const auto trebleDb = clampCombinedGainDb (trebleDbSmoothed.skip (numSamples) + toneVoiceTrebleTiltDb[toneVoiceIndex]);

    *bassShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, bassShelfFrequencyHz, shelfQ, juce::Decibels::decibelsToGain (bassDb));
    *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, midPeakFrequencyHz, midPeakQ, juce::Decibels::decibelsToGain (midDb));
    *trebleShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, trebleShelfFrequencyHz, shelfQ, juce::Decibels::decibelsToGain (trebleDb));
}

void ToneStack::process (juce::dsp::ProcessContextReplacing<float>& context)
{
    bassShelf.process (context);
    midPeak.process (context);
    trebleShelf.process (context);
}
