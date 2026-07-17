#pragma once

#include <juce_dsp/juce_dsp.h>

// A conventional fixed attack/hold/release expander/gate, inserted after the
// tone stack/Presence (see docs/design-brief.md section 3.5) - the single
// highest-priority v0.2.0 addition (docs/research-notes.md section 7): every
// reference plugin/pedal in the high-gain cascaded-rhythm-guitar genre ships
// a gate as a first-class module, because a multi-stage cascade amplifies
// noise floor/string noise by every stage's worth of gain before the tone
// stack even sees it.
//
// Deliberately NOT an attempt to replicate any adaptive/program-dependent
// proprietary gate algorithm (see docs/research-notes.md section 7,
// citation only, for the reference product this contrasts with) - this is
// a plain, fixed-ballistics expander, named as such in the design brief's
// honesty section.
//
// Detection: per-sample peak level across all channels (max |x|), compared
// directly against a linear threshold - deliberately not itself smoothed, so
// the gate can react to a transient within a single sample rather than
// waiting for a detector's own filter to catch up (this is what keeps the
// "transient not delayed beyond attack time" guarantee in
// tests/GateTests.cpp true). Once the level crosses the threshold the gate
// opens (its gain ramps towards 1 over the Attack time); once the level
// drops back below threshold, a Hold countdown runs before Release begins
// (gain ramps towards 0 over the Release time) - the standard technique that
// stops a held/sustained note's natural amplitude ripple from re-triggering/
// chattering the gate.
//
// Allocation-free after prepare(); safe to call every setter/process() from
// the audio thread.
class Gate
{
public:
    Gate();

    void prepare (const juce::dsp::ProcessSpec& spec);

    // Resets the gate to fully open (current gain = 1, hold countdown
    // cleared) without deallocating - a freshly (re)prepared/reset engine
    // must not gate the very first block shut before any signal has been
    // seen, matching the rest of TenebraeEngine's reset() semantics (clears
    // state, does not impose a transient mute).
    void reset();

    // Processes `context` in place. A no-op (true bypass, `context` left
    // byte-for-byte untouched, no internal state read or written) whenever
    // the gate is disabled - see setEnabled().
    void process (juce::dsp::ProcessContextReplacing<float>& context);

    void setThresholdDb (float newThresholdDb);
    void setAttackMs (float newAttackMs);
    void setHoldMs (float newHoldMs);
    void setReleaseMs (float newReleaseMs);

    // Gate on/off. When off, process() returns immediately without touching
    // `context` or any internal state (envelope/hold/gain) at all - a true
    // structural bypass, not merely "always compute an open gate" - so a
    // disabled gate is provably independent of whatever the gate's internal
    // state happened to be left at (see tests/EngineTests.cpp's bypass null
    // test).
    void setEnabled (bool shouldBeEnabled);

    static constexpr float minThresholdDb = -80.0f;
    static constexpr float maxThresholdDb = 0.0f;
    static constexpr float minAttackMs = 0.1f;
    static constexpr float maxAttackMs = 20.0f;
    static constexpr float minHoldMs = 0.0f;
    static constexpr float maxHoldMs = 500.0f;
    static constexpr float minReleaseMs = 5.0f;
    static constexpr float maxReleaseMs = 2000.0f;

private:
    // One-pole ramp coefficient for reaching ~63% of a gain step in
    // `timeMs` milliseconds - the standard exponential-ramp ballistics
    // technique used by every fixed-attack/release gate/compressor design.
    static float computeRampCoefficient (float timeMs, double sampleRate) noexcept;

    double sampleRate = 44100.0;

    float thresholdLinear = 0.0f;
    float attackCoefficient = 1.0f;
    float releaseCoefficient = 1.0f;
    int holdSamples = 0;

    int holdCounter = 0;
    float currentGain = 1.0f;
    bool enabled = true;

    // Last commanded values, re-applied on every prepare() so a re-prepare
    // (sample-rate change, etc.) never resets a live parameter back to a
    // built-in default - the same pattern TenebraeEngine/ToneStack/
    // CascadeStage already use for their own "last*" members.
    float lastThresholdDb = -48.0f;
    float lastAttackMs = 1.0f;
    float lastHoldMs = 20.0f;
    float lastReleaseMs = 150.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gate)
};
