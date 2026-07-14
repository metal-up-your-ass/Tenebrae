#include "ToneStack.h"

ToneStack::ToneStack() = default;

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
    // identity/uninitialised state.
    *bassShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, bassShelfFrequencyHz, shelfQ, juce::Decibels::decibelsToGain (lastBassDb));
    *midPeak.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        sampleRate, midPeakFrequencyHz, midPeakQ, juce::Decibels::decibelsToGain (lastMidDb));
    *trebleShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, trebleShelfFrequencyHz, shelfQ, juce::Decibels::decibelsToGain (lastTrebleDb));
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

void ToneStack::updateCoefficients (int numSamples)
{
    // Coefficient recomputation involves trig calls, so band gains are
    // smoothed and the filter coefficients re-derived once per block rather
    // than per sample.
    const auto bassDb = bassDbSmoothed.skip (numSamples);
    const auto midDb = midDbSmoothed.skip (numSamples);
    const auto trebleDb = trebleDbSmoothed.skip (numSamples);

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
