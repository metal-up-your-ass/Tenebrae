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

    // Tone Voice: 0 = Flat (no tilt, v0.1 behaviour), 1 = Scoop, 2 = Boost -
    // see ToneStack.cpp for the fixed dB tilt table. The tilt is added on
    // top of the (still fully live) Bass/Mid/Treble band gains above, so the
    // knobs keep working normally on top of whichever voice is selected.
    // Like Bright in TenebraeEngine, this is a discrete switch rather than a
    // continuous control, so it is intentionally not smoothed - selecting a
    // new voice may produce a small audible step, same as a physical amp's
    // channel/voicing switch.
    void setToneVoice (int newToneVoiceIndex);

    // Recomputes filter coefficients from the smoothed band-gain values and
    // steps the smoothers forward by `numSamples`. Call once per process
    // block, before process().
    void updateCoefficients (int numSamples);

    // Public purely for test purposes (see tests/ToneStackTests.cpp's
    // v0.2.0 Treble-corner regression test) - matches sibling plugin nave's
    // CabConvolutionEngine::loCutMinHz/hiCutMaxHz precedent for exposing a
    // fixed DSP constant a test needs to pin down without duplicating it.
    static constexpr float trebleShelfFrequencyHz = 5000.0f;

private:
    static constexpr double smoothingTimeSeconds = 0.05;

    // Fixed corner frequencies/Q - not user-automatable. Refined for M1
    // against typical high-gain rhythm voicings: the mid corner sits in the
    // 500-700 Hz "boxy/honk" range most high-gain amps scoop for a tight
    // chug tone (moved down from the v0.1 800 Hz placeholder) with a
    // narrower Q so Scoop/cut is surgical rather than broad.
    //
    // v0.2.0 (docs/design-brief.md section 3.4): the Treble corner is raised
    // from 3.5 kHz to 5 kHz. Reasoned, partially sourced: the reference
    // class's TMB Treble corners cluster around 9-10 kHz and its Presence
    // control pivots at 1.6-2.4 kHz (docs/research-notes.md section 2) -
    // with the new Presence control (TenebraeEngine.cpp) now owning the
    // 2.4 kHz region, Treble sits clearly above it rather than stacking on
    // the same corner Bright already occupies (3.5 kHz, unchanged,
    // pre-cascade - see TenebraeEngine.cpp). 5 kHz is a reasoned midpoint
    // that separates Treble from both Bright and Presence while staying
    // below the cascade's own top-end rolloff ceiling (interstage low-pass
    // corners top out at 9 kHz in the Tight voicing) - not directly sourced
    // to a single reference number, called out here rather than presented
    // as measured (see the brief's honesty section).
    static constexpr float bassShelfFrequencyHz = 150.0f;
    static constexpr float midPeakFrequencyHz = 650.0f;
    static constexpr float midPeakQ = 1.1f;
    static constexpr float shelfQ = juce::MathConstants<float>::sqrt2 / 2.0f;

    // Fixed dB tilt applied on top of the live Bass/Mid/Treble gains for
    // each Tone Voice, indexed by the AudioParameterChoice index (Flat /
    // Scoop / Boost - see ParameterLayout.cpp). Scoop is the classic
    // high-gain-rhythm "smiley" curve (bass and treble up, mid well down);
    // Boost pushes the mid forward instead, for cutting through a mix
    // (lead-boost/solo-adjacent character) at the cost of a touch of bass.
    static constexpr float toneVoiceBassTiltDb[3] = { 0.0f, 4.0f, -1.0f };
    static constexpr float toneVoiceMidTiltDb[3] = { 0.0f, -6.0f, 5.0f };
    static constexpr float toneVoiceTrebleTiltDb[3] = { 0.0f, 3.0f, 2.0f };

    // Combined band gain (user dB + Tone Voice tilt) is clamped to this
    // range before being handed to makeLowShelf/makePeakFilter/makeHighShelf
    // so an extreme knob position stacked with a tilt can never produce a
    // pathological filter gain, while still leaving headroom above the
    // +/-15 dB single-knob range for the tilt to be audible.
    static constexpr float combinedGainLimitDb = 21.0f;

    static float clampCombinedGainDb (float gainDb) noexcept;

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

    // Index into toneVoice*TiltDb above; 0 (Flat) until setToneVoice() is
    // called, matching the ParameterLayout default.
    int toneVoiceIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToneStack)
};
