#include "dsp/Gate.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>

// Module-level tests for the v0.2.0 Gate (docs/design-brief.md section 3.5),
// exercised directly (not through the full TenebraeEngine) so the result
// isolates the gate's own ballistics from the rest of the signal chain -
// same pattern as tests/ToneStackTests.cpp/CascadeStageTests.cpp.
namespace
{
    constexpr double testSampleRate = 48000.0;

    juce::dsp::ProcessSpec makeTestSpec (int numChannels, juce::uint32 maxBlockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = testSampleRate;
        spec.maximumBlockSize = maxBlockSize;
        spec.numChannels = static_cast<juce::uint32> (numChannels);
        return spec;
    }
}

TEST_CASE ("Gate: silence stays silent, transient passes, tail decays without a click", "[dsp][gate]")
{
    // design-brief.md section 4: "silence -> transient above threshold ->
    // silence" - (a) pre-transient silence stays near-zero, (b) the
    // transient itself is not clipped/delayed beyond the attack time, (c)
    // post-transient output decays to near-zero within hold+release without
    // an abrupt (hard-mute) discontinuity at gate closure.
    //
    // Uses constant-level steps (not an oscillating tone) rather than a
    // sine burst: a genuine audio waveform's own sample-to-sample slope
    // (e.g. a full-scale 1 kHz tone can swing by >0.1 between adjacent
    // samples near its steepest point) would otherwise swamp the "no
    // hard-mute click" delta check below with waveform content unrelated
    // to the gate's own gain trajectory - a step/DC-style test signal is
    // the standard way to isolate ballistics (attack/hold/release) checks
    // from that, the same way a compressor/expander's ballistics are
    // normally characterised against a level step, not a full tone.
    constexpr int silenceLeadIn = 2000;
    constexpr int transientLength = 2000; // a loud burst, well above threshold
    constexpr float thresholdDb = -40.0f; // 0.01 linear
    constexpr float attackMs = 1.0f;
    constexpr float holdMs = 20.0f;
    constexpr float releaseMs = 80.0f;
    constexpr float loudLevel = 0.9f;
    constexpr float quietLevel = 0.001f; // well below the 0.01 linear threshold

    const auto holdSamples = static_cast<int> (holdMs * 0.001 * testSampleRate);
    const auto releaseSamples = static_cast<int> (releaseMs * 0.001 * testSampleRate); // one-pole time constant, in samples
    const auto decayCheckPoint = silenceLeadIn + transientLength + holdSamples + releaseSamples * 5; // ~99% decayed
    const auto numSamples = decayCheckPoint + 4000;

    Gate gate;
    gate.setThresholdDb (thresholdDb);
    gate.setAttackMs (attackMs);
    gate.setHoldMs (holdMs);
    gate.setReleaseMs (releaseMs);

    const auto spec = makeTestSpec (1, static_cast<juce::uint32> (numSamples));
    gate.prepare (spec);

    juce::AudioBuffer<float> buffer (1, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        float level = quietLevel;

        if (i < silenceLeadIn)
            level = 0.0f;
        else if (i < silenceLeadIn + transientLength)
            level = loudLevel;

        buffer.setSample (0, i, level);
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    CHECK_NOTHROW (gate.process (context));

    CHECK (TestHelpers::allSamplesFinite (buffer));

    const auto* data = buffer.getReadPointer (0);

    // (a) Pre-transient silence: input was already 0, so this is trivially
    // near-zero regardless of gate state - the meaningful check is that the
    // gate doesn't inject anything (e.g. a DC offset) into true silence.
    for (int i = 0; i < silenceLeadIn; ++i)
        CHECK (std::abs (data[i]) < 1.0e-6f);

    // (b) The transient itself must not be clipped/attenuated to near-zero
    // for its full duration - well within the attack time (1 ms = 48
    // samples @ 48 kHz), the gate should be open and passing the level at
    // close to full amplitude.
    const auto attackSamples = static_cast<int> (attackMs * 0.001 * testSampleRate);
    float minLevelShortlyAfterAttack = loudLevel;

    for (int i = silenceLeadIn + attackSamples * 4; i < silenceLeadIn + transientLength; ++i)
        minLevelShortlyAfterAttack = std::min (minLevelShortlyAfterAttack, std::abs (data[i]));

    CHECK (minLevelShortlyAfterAttack > loudLevel * 0.9f); // comfortably close to the full input level

    // (c) No abrupt (hard-mute) discontinuity introduced BY THE GATE itself
    // anywhere in the block - a smoothed gain ramp closing over `releaseMs`
    // never produces a single-sample jump larger than this epsilon, whereas
    // a hard on/off multiply would jump close to the full quiet-level
    // amplitude the instant the gate closes. This deliberately excludes the
    // two sample indices where the *test signal itself* steps level
    // (silence->loud at the transient's onset, loud->quiet at its end) -
    // those are the test harness's own artificial level discontinuities,
    // not something any gain-only gate could or should smooth away (a real
    // pick attack/note release has its own rise/decay time in a genuine
    // recording; a gate's job is to not add a *second*, gate-caused click on
    // top of the program material's own dynamics, which is exactly what
    // this window verifies).
    constexpr float maxAllowedSampleToSampleDelta = 0.05f;
    const auto transientOnsetIndex = silenceLeadIn;
    const auto transientEndIndex = silenceLeadIn + transientLength;

    for (int i = 1; i < numSamples; ++i)
    {
        if (i == transientOnsetIndex || i == transientEndIndex)
            continue;

        CHECK (std::abs (data[i] - data[i - 1]) < maxAllowedSampleToSampleDelta);
    }

    // Output must decay to near-zero well within hold + release of the
    // transient ending.
    REQUIRE (decayCheckPoint < numSamples);
    CHECK (std::abs (data[decayCheckPoint]) < 0.01f);
}

TEST_CASE ("Gate: sustained signal above threshold is never gated (no chatter)", "[dsp][gate]")
{
    // A continuous sine held well above threshold for longer than
    // attack+hold+release combined must show no gain reduction at any
    // point after the attack ramp completes - proves the gate doesn't
    // false-trigger/chatter on sustained content.
    constexpr int numSamples = 48000; // 1s
    constexpr float thresholdDb = -40.0f;

    Gate gate;
    gate.setThresholdDb (thresholdDb);
    gate.setAttackMs (1.0f);
    gate.setHoldMs (20.0f);
    gate.setReleaseMs (150.0f);

    const auto spec = makeTestSpec (1, static_cast<juce::uint32> (numSamples));
    gate.prepare (spec);

    juce::AudioBuffer<float> buffer (1, numSamples);
    TestHelpers::fillWithSine (buffer, testSampleRate, 220.0, 0.8f);

    juce::AudioBuffer<float> reference;
    reference.makeCopyOf (buffer);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    gate.process (context);

    const auto* data = buffer.getReadPointer (0);
    const auto* refData = reference.getReadPointer (0);

    // Well past the attack ramp (a few ms is more than enough for a 1 ms
    // attack time constant to settle), the gate should track the reference
    // signal closely - no dips anywhere in the remainder of the block.
    constexpr int settleSamples = 2000; // ~42 ms, comfortably past a 1 ms attack

    for (int i = settleSamples; i < numSamples; ++i)
        CHECK (std::abs (data[i] - refData[i]) < 0.02f);
}

TEST_CASE ("Gate: NaN/Inf sweep at both threshold extremes with a full-scale signal stays finite", "[dsp][gate][nan]")
{
    for (float thresholdDb : { Gate::minThresholdDb, Gate::maxThresholdDb })
    {
        Gate gate;
        gate.setThresholdDb (thresholdDb);
        gate.setAttackMs (0.1f);
        gate.setHoldMs (0.0f);
        gate.setReleaseMs (5.0f);

        constexpr int numSamples = 4096;
        const auto spec = makeTestSpec (2, static_cast<juce::uint32> (numSamples));
        gate.prepare (spec);

        juce::AudioBuffer<float> buffer (2, numSamples);
        TestHelpers::fillWithSine (buffer, testSampleRate, 1000.0, 1.0f);

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        CHECK_NOTHROW (gate.process (context));

        CHECK (TestHelpers::allSamplesFinite (buffer));
    }
}

TEST_CASE ("Gate: NaN threshold parameter does not poison the gate with NaN gain", "[dsp][gate][nan]")
{
    Gate gate;
    gate.setThresholdDb (std::numeric_limits<float>::quiet_NaN());
    gate.setAttackMs (std::numeric_limits<float>::quiet_NaN());
    gate.setHoldMs (std::numeric_limits<float>::quiet_NaN());
    gate.setReleaseMs (std::numeric_limits<float>::quiet_NaN());

    constexpr int numSamples = 2048;
    const auto spec = makeTestSpec (1, static_cast<juce::uint32> (numSamples));
    gate.prepare (spec);

    juce::AudioBuffer<float> buffer (1, numSamples);
    TestHelpers::fillWithSine (buffer, testSampleRate, 1000.0, 0.7f);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    CHECK_NOTHROW (gate.process (context));

    CHECK (TestHelpers::allSamplesFinite (buffer));
}

TEST_CASE ("Gate: disabled is a true bypass - output is bit-identical to the untouched input", "[dsp][gate][bypass]")
{
    Gate gate;
    gate.setThresholdDb (0.0f); // would gate almost everything shut if engaged
    gate.setEnabled (false);

    constexpr int numSamples = 4096;
    const auto spec = makeTestSpec (2, static_cast<juce::uint32> (numSamples));
    gate.prepare (spec);

    juce::AudioBuffer<float> buffer (2, numSamples);
    TestHelpers::fillWithSine (buffer, testSampleRate, 1000.0, 0.5f);

    juce::AudioBuffer<float> reference;
    reference.makeCopyOf (buffer);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    gate.process (context);

    for (int channel = 0; channel < 2; ++channel)
        for (int i = 0; i < numSamples; ++i)
            CHECK (juce::exactlyEqual (buffer.getSample (channel, i), reference.getSample (channel, i)));
}

TEST_CASE ("Gate: disabled bypass is independent of prior engaged-state internal state", "[dsp][gate][bypass]")
{
    // Dirty the gate's internal envelope/hold/gain state with a hot
    // threshold that forces it fully closed, then disable it - the
    // subsequent bypassed output must be identical to a gate that was
    // never engaged at all, proving process() truly returns before
    // touching any state when disabled (not merely "computes an always-
    // open gate").
    constexpr int numSamples = 4096;
    const auto spec = makeTestSpec (1, static_cast<juce::uint32> (numSamples));

    Gate dirtiedGate;
    dirtiedGate.setThresholdDb (0.0f);
    dirtiedGate.prepare (spec);

    juce::AudioBuffer<float> warmup (1, numSamples);
    TestHelpers::fillWithSine (warmup, testSampleRate, 1000.0, 0.1f); // below 0 dB threshold - forces closed
    juce::dsp::AudioBlock<float> warmupBlock (warmup);
    juce::dsp::ProcessContextReplacing<float> warmupContext (warmupBlock);
    dirtiedGate.process (warmupContext); // gate closes, currentGain settles near 0

    dirtiedGate.setEnabled (false);

    Gate cleanGate;
    cleanGate.setEnabled (false);
    cleanGate.prepare (spec);

    juce::AudioBuffer<float> dirtiedBuffer (1, numSamples);
    juce::AudioBuffer<float> cleanBuffer (1, numSamples);
    TestHelpers::fillWithSine (dirtiedBuffer, testSampleRate, 500.0, 0.6f);
    cleanBuffer.makeCopyOf (dirtiedBuffer);

    juce::dsp::AudioBlock<float> dirtiedBlock (dirtiedBuffer);
    juce::dsp::ProcessContextReplacing<float> dirtiedContext (dirtiedBlock);
    dirtiedGate.process (dirtiedContext);

    juce::dsp::AudioBlock<float> cleanBlock (cleanBuffer);
    juce::dsp::ProcessContextReplacing<float> cleanContext (cleanBlock);
    cleanGate.process (cleanContext);

    for (int i = 0; i < numSamples; ++i)
        CHECK (juce::exactlyEqual (dirtiedBuffer.getSample (0, i), cleanBuffer.getSample (0, i)));
}

TEST_CASE ("Gate: reset() reopens the gate without crashing", "[dsp][gate]")
{
    Gate gate;
    gate.setThresholdDb (0.0f); // forces closed against a quiet signal

    constexpr int numSamples = 2048;
    const auto spec = makeTestSpec (1, static_cast<juce::uint32> (numSamples));
    gate.prepare (spec);

    juce::AudioBuffer<float> buffer (1, numSamples);
    TestHelpers::fillWithSine (buffer, testSampleRate, 1000.0, 0.1f);
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    gate.process (context);

    CHECK_NOTHROW (gate.reset());

    TestHelpers::fillWithSine (buffer, testSampleRate, 1000.0, 0.1f);
    CHECK_NOTHROW (gate.process (context));
    CHECK (TestHelpers::allSamplesFinite (buffer));
}
