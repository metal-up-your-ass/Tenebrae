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
}
