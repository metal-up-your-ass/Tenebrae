#pragma once

#include <juce_dsp/juce_dsp.h>

// A passive-style 3-band tone stack applied after the waveshaper cascade:
// Bass (low shelf) / Mid (peaking) / Treble (high shelf), loosely modelled
// on the interacting shelving/peaking behaviour of a guitar-amp tone stack.
// Corner frequencies and the Mid band's Q are fixed; only each band's gain
// is user-controllable (see ParameterLayout.cpp).
//
// Runs at the host (non-oversampled) rate, after the cascade has been
// downsampled - there is no new nonlinearity here, so no additional
// anti-aliasing headroom is needed.
//
// Allocation-free after prepare(); safe to call the setters/process() from
// the audio thread.
class ToneStack
{
public:
    ToneStack();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    // Processes `context` in place.
    void process (juce::dsp::ProcessContextReplacing<float>& context);

    // Band gains, in dB. Smoothed internally (see .cpp) and re-applied to
    // the filter coefficients once per block - safe to call every block
    // from the audio thread.
    void setBassDb (float newBassDb);
    void setMidDb (float newMidDb);
    void setTrebleDb (float newTrebleDb);

    // Recomputes filter coefficients from the smoothed band-gain values and
    // steps the smoothers forward by `numSamples`. Call once per process
    // block, before process().
    void updateCoefficients (int numSamples);

private:
    static constexpr double smoothingTimeSeconds = 0.05;

    // Fixed corner frequencies/Q - not user-automatable.
    static constexpr float bassShelfFrequencyHz = 150.0f;
    static constexpr float trebleShelfFrequencyHz = 3000.0f;
    static constexpr float midPeakFrequencyHz = 800.0f;
    static constexpr float midPeakQ = 0.8f;
    static constexpr float shelfQ = juce::MathConstants<float>::sqrt2 / 2.0f;

    double sampleRate = 44100.0;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> bassShelf;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> midPeak;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> trebleShelf;

    // Linear smoothing on the dB values themselves (rather than the linear
    // gain factor) - a reasonable approximation of perceptually-even
    // travel for a control range this narrow (+/-15 dB).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bassDbSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> midDbSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> trebleDbSmoothed;

    float lastBassDb = 0.0f;
    float lastMidDb = 0.0f;
    float lastTrebleDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToneStack)
};
