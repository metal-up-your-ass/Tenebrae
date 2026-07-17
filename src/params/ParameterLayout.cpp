#include "ParameterLayout.h"
#include "ParameterIds.h"
#include "../dsp/Gate.h"

namespace
{
    // True logarithmic (base-10) mapping for frequency parameters, so slider/
    // knob travel spends equal space per octave rather than per Hz. Uses
    // juce::mapToLog10/mapFromLog10 rather than NormalisableRange's built-in
    // power-law skew, which only approximates a log curve.
    juce::NormalisableRange<float> makeLogFrequencyRange (float minHz, float maxHz)
    {
        return juce::NormalisableRange<float> (
            minHz,
            maxHz,
            [] (float rangeStart, float rangeEnd, float normalised)
            { return juce::mapToLog10 (normalised, rangeStart, rangeEnd); },
            [] (float rangeStart, float rangeEnd, float value)
            { return juce::mapFromLog10 (value, rangeStart, rangeEnd); });
    }
}

namespace tnbr
{
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        //======================================================================
        // Tight: high-pass pre-emphasis, 20-300 Hz, default 90 Hz - tightens
        // the low end before the gain cascade so chugs stay percussive.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::tight, 1 },
            "Tight",
            makeLogFrequencyRange (20.0f, 300.0f),
            90.0f,
            juce::AudioParameterFloatAttributes().withLabel ("Hz")));

        //======================================================================
        // Gain: pre-gain into the oversampled 3-stage waveshaper cascade.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gain, 1 },
            "Gain",
            juce::NormalisableRange<float> (0.0f, 40.0f, 0.01f),
            24.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        //======================================================================
        // Bass/Mid/Treble: passive-style tone stack, applied post-cascade.
        // Fixed corner frequencies (see ToneStack); only the gain of each
        // band is user-controllable, like a classic amp tone stack.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::bass, 1 },
            "Bass",
            juce::NormalisableRange<float> (-15.0f, 15.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::mid, 1 },
            "Mid",
            juce::NormalisableRange<float> (-15.0f, 15.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::treble, 1 },
            "Treble",
            juce::NormalisableRange<float> (-15.0f, 15.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        //======================================================================
        // Level: output trim.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::level, 1 },
            "Level",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        //======================================================================
        // Mix: dry/wet. Default 100% (fully wet) - a rhythm distortion is
        // normally run fully in the signal path, not blended.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::mix, 1 },
            "Mix",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        //======================================================================
        // Voicing: switches the fixed cascade constants (asymmetry +
        // interstage HP/LP corners) between the v0.1 "Tight" cascade and a
        // softer-driven, wider-band "Loose" alternative (see
        // TenebraeEngine.cpp for both stage tables). Index-based choice, not
        // a continuous control.
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamIDs::voicing, 1 },
            "Voicing",
            juce::StringArray { "Tight", "Loose" },
            0));

        //======================================================================
        // Bright: fixed pre-cascade high-shelf pre-emphasis, modelled on a
        // high-gain amp channel's bright switch. Off by default.
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamIDs::bright, 1 },
            "Bright",
            false));

        //======================================================================
        // Tone Voice: fixed dB tilt added on top of the Bass/Mid/Treble
        // bands (see ToneStack::setToneVoice). Flat is the v0.1 behaviour
        // (no tilt); Scoop/Boost are canned high-gain-rhythm tone shapes.
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamIDs::toneVoice, 1 },
            "Tone Voice",
            juce::StringArray { "Flat", "Scoop", "Boost" },
            0));

        //======================================================================
        // v0.2.0 additions (docs/design-brief.md) - Presence and the Gate.

        // Presence: post-cascade high-shelf, fixed 2.4 kHz corner (see
        // TenebraeEngine.cpp). 0 dB default = unity = true passthrough (a new
        // control must not change the tone of any existing preset/session on
        // migration - see the brief's section 6).
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::presence, 1 },
            "Presence",
            juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        // Gate: conventional fixed attack/hold/release expander/gate, applied
        // to the fully-voiced wet signal after Presence (see Gate.h and
        // docs/design-brief.md section 3.5).
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateThreshold, 1 },
            "Gate Threshold",
            juce::NormalisableRange<float> (Gate::minThresholdDb, Gate::maxThresholdDb, 0.01f),
            -48.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateAttack, 1 },
            "Gate Attack",
            makeLogFrequencyRange (Gate::minAttackMs, Gate::maxAttackMs),
            1.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateHold, 1 },
            "Gate Hold",
            juce::NormalisableRange<float> (Gate::minHoldMs, Gate::maxHoldMs, 0.1f),
            20.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateRelease, 1 },
            "Gate Release",
            makeLogFrequencyRange (Gate::minReleaseMs, Gate::maxReleaseMs),
            150.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms")));

        // Default ON - see ParameterIds.h's doc comment and the design
        // brief's honesty section for why this default change is deliberate,
        // not an oversight.
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamIDs::gateOn, 1 },
            "Gate",
            true));

        return layout;
    }
}
