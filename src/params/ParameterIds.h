#pragma once

// Central definition of all AudioProcessorValueTreeState parameter IDs for
// Tenebrae. See docs/architecture.md for the corresponding signal-flow
// diagram.
//
// FROZEN AS OF THE v0.1 PARAMETER LAYOUT:
// Parameter IDs below must NEVER change once shipped - saved sessions and
// presets persist the APVTS state keyed by these string IDs, and renaming or
// removing one would silently break every user's saved state. Ranges,
// defaults, and skew MAY still be refined during voicing/tuning milestones;
// only the IDs themselves are frozen.
namespace ParamIDs
{
    // "Tight" high-pass pre-emphasis: strips low end before the cascade so
    // palm mutes stay tight/percussive instead of farting out into the
    // gain stages.
    inline constexpr auto tight = "tight";

    // Pre-gain into the oversampled 3-stage waveshaper cascade - the main
    // "how much distortion" control.
    inline constexpr auto gain = "gain";

    // Passive-style tone stack, applied after the cascade: shelving/peaking
    // bands modelled loosely on a guitar-amp tone stack.
    inline constexpr auto bass = "bass";
    inline constexpr auto mid = "mid";
    inline constexpr auto treble = "treble";

    // Output trim, applied after the tone stack and before the dry/wet mix.
    inline constexpr auto level = "level";

    // Dry/wet mix. At 0% the plugin is a delay-compensated passthrough of
    // the input (see TenebraeEngine's DryWetMixer usage).
    inline constexpr auto mix = "mix";

    // Cascade voicing: selects between two fixed sets of per-stage asymmetry
    // and interstage HP/LP corners (see TenebraeEngine.cpp) - "Tight" is the
    // v0.1 cascade, "Loose" is a softer-driven, wider-band alternative
    // voicing added in M1. Not a continuous control; changing it may produce
    // a small audible step, same as a physical amp channel-select switch.
    inline constexpr auto voicing = "voicing";

    // Bright: a fixed pre-cascade high-shelf pre-emphasis, modelled on the
    // "bright switch"/brighter-cab presence character of a high-gain amp
    // channel - not a full cab-sim (that is a convolution-based plugin
    // elsewhere in the suite). Off by default.
    inline constexpr auto bright = "bright";

    // Tone Voice: a fixed dB tilt added on top of the Bass/Mid/Treble bands
    // (Flat/Scoop/Boost - see ToneStack::setToneVoice), giving a one-switch
    // character shift while the individual band knobs remain fully live on
    // top of it.
    inline constexpr auto toneVoice = "toneVoice";
}
