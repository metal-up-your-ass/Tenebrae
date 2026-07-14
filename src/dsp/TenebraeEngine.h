#pragma once

#include <juce_dsp/juce_dsp.h>

#include "CascadeStage.h"
#include "ToneStack.h"

// The complete Tenebrae signal path, independent of juce::AudioProcessor so
// it can be exercised directly by unit tests without instantiating a full
// plugin (see tests/EngineTests.cpp). Owns all DSP state; every buffer/
// filter/oversampler is allocated in prepare() and never reallocated on the
// audio thread.
//
// Signal flow (see docs/architecture.md for the full diagram and the
// latency-compensation rationale):
//
//   input -> Tight HPF -> pre-Gain -> [8x oversampled] 3-stage cascade
//         -> 3-band tone stack (Bass/Mid/Treble) -> Level -> Dry/Wet mix
//
// Each cascade stage is gain -> asymmetric tanh clip -> fixed interstage
// HP/LP filter (see CascadeStage); the three stages use progressively
// tighter/darker fixed voicing so the cascade converges onto the "chug"
// band rather than piling up an ever-fizzier mess (see TenebraeEngine.cpp).
//
// The dry path is delay-compensated against the oversampler's reported
// latency via juce::dsp::DryWetMixer, so Mix at 0% is a sample-accurate
// (once shifted by getLatencySamples()) passthrough of the input - this is
// what the plugin's null test (tests/EngineTests.cpp) exercises.
class TenebraeEngine
{
public:
    TenebraeEngine();

    // Allocates all DSP state. Must be called (and completed) before the
    // first process() call, and again whenever sample rate/block size/
    // channel count change.
    void prepare (const juce::dsp::ProcessSpec& spec);

    // Clears all filter/oversampler/delay-line state without deallocating.
    // Safe to call from the audio thread (e.g. on playback stop/loop).
    void reset();

    // Processes `block` in place. `block` must have at most the maximum
    // sample/channel counts declared to prepare(); a zero-sample block is a
    // safe no-op. No allocation occurs here.
    void process (juce::dsp::AudioBlock<float>& block);

    // Parameter setters, in real units (dB, Hz, 0-1 proportion). Safe to
    // call every block from the audio thread - no allocation/locks.
    void setTightFrequencyHz (float newFrequencyHz);
    void setGainDb (float newGainDb);
    void setBassDb (float newBassDb);
    void setMidDb (float newMidDb);
    void setTrebleDb (float newTrebleDb);
    void setLevelDb (float newLevelDb);
    void setMixProportion (float newProportion01);

    // Oversampling latency in samples, valid after prepare() has run.
    int getLatencySamples() const noexcept { return latencySamples; }

private:
    static constexpr int oversamplingFactorPow2 = 3; // 2^3 = 8x oversampling
    static constexpr double smoothingTimeSeconds = 0.05;
    // Butterworth (maximally-flat) Q for the Tight HPF.
    static constexpr float filterQ = juce::MathConstants<float>::sqrt2 / 2.0f;

    double sampleRate = 44100.0;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> tightHighPass;
    juce::dsp::Gain<float> preGain;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // Fixed per-stage voicing (asymmetry, interstage HP/LP corner
    // frequencies, internal drive) is set up in the constructor/prepare() -
    // see TenebraeEngine.cpp for the rationale behind each stage's numbers.
    // Only the pre-cascade Gain parameter is user-automatable.
    CascadeStage cascadeStage1;
    CascadeStage cascadeStage2;
    CascadeStage cascadeStage3;

    ToneStack toneStack;
    juce::dsp::Gain<float> outputLevel;

    // Sized generously above any realistic oversampling latency so
    // setWetLatency() never exceeds the mixer's internal delay-line
    // capacity regardless of sample rate.
    juce::dsp::DryWetMixer<float> dryWetMixer { 1024 };

    // Tight is a filter cutoff frequency (perceived logarithmically), so it
    // uses multiplicative smoothing; Mix uses linear smoothing and must be
    // able to reach exactly 0.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> tightFrequencySmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;

    // Last commanded values (ParameterLayout defaults until a setter is
    // called), re-applied on every prepare() so re-prepare (sample-rate
    // change, etc.) never resets a live parameter back to a default or lets
    // a smoother start from an invalid 0 Hz.
    float lastTightHz = 90.0f;
    float lastGainDb = 24.0f;
    float lastMixProportion = 1.0f;

    int latencySamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TenebraeEngine)
};
