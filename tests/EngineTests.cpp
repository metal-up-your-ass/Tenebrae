#include "dsp/TenebraeEngine.h"
#include "TestHelpers.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
    constexpr double testSampleRate = 48000.0;
    constexpr int testBlockSize = 8192; // large single block: keeps the null-test
                                         // bookkeeping below simple by avoiding
                                         // multi-block accounting.
    constexpr double testFrequencyHz = 1000.0;

    juce::dsp::ProcessSpec makeTestSpec (int numChannels)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = testSampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (testBlockSize);
        spec.numChannels = static_cast<juce::uint32> (numChannels);
        return spec;
    }
}

TEST_CASE ("Engine null test: 0% mix nulls against the input once shifted by latency", "[dsp][engine][null]")
{
    TenebraeEngine engine;

    // Parameters other than Mix are deliberately set to non-neutral values:
    // a true null test has to prove the *entire* wet chain (Tight HPF,
    // Gain, cascade, tone stack, Level) is bypassed, not just that it
    // happens to be quiet at default settings.
    engine.setMixProportion (0.0f);
    engine.setGainDb (30.0f);
    engine.setTightFrequencyHz (200.0f);
    engine.setBassDb (10.0f);
    engine.setMidDb (-8.0f);
    engine.setTrebleDb (6.0f);
    engine.setLevelDb (8.0f);

    const auto spec = makeTestSpec (2);
    engine.prepare (spec);

    const auto latency = engine.getLatencySamples();
    REQUIRE (latency >= 0);
    // Sanity bound: the oversampling latency must be well inside both the
    // DryWetMixer's fixed dry-delay capacity (1024, see TenebraeEngine.h)
    // and the test block size, or the overlap window below would be
    // meaningless.
    REQUIRE (latency < testBlockSize / 2);

    juce::AudioBuffer<float> reference (2, testBlockSize);
    TestHelpers::fillWithSine (reference, testSampleRate, testFrequencyHz, 0.5f);

    juce::AudioBuffer<float> processed;
    processed.makeCopyOf (reference);

    juce::dsp::AudioBlock<float> block (processed);
    engine.process (block);

    const auto overlapLength = testBlockSize - latency;
    REQUIRE (overlapLength > testBlockSize / 2);

    // < -90 dBFS residual, in linear amplitude.
    constexpr float tolerance = 3.1623e-5f; // 10^(-90/20)

    for (int channel = 0; channel < reference.getNumChannels(); ++channel)
    {
        const auto* refData = reference.getReadPointer (channel);
        const auto* outData = processed.getReadPointer (channel);

        float maxResidual = 0.0f;

        for (int i = 0; i < overlapLength; ++i)
            maxResidual = std::max (maxResidual, std::abs (outData[latency + i] - refData[i]));

        CHECK (maxResidual < tolerance);
    }
}

TEST_CASE ("Engine reset() clears filter/oversampler/delay/cascade state without crashing", "[dsp][engine]")
{
    TenebraeEngine engine;
    engine.setGainDb (30.0f);
    engine.setMixProportion (1.0f);

    const auto spec = makeTestSpec (2);
    engine.prepare (spec);

    juce::AudioBuffer<float> buffer (2, testBlockSize);
    TestHelpers::fillWithSine (buffer, testSampleRate, testFrequencyHz, 0.9f);

    juce::dsp::AudioBlock<float> block (buffer);
    engine.process (block);

    CHECK_NOTHROW (engine.reset());
    CHECK (TestHelpers::allSamplesFinite (buffer));

    // Processing again straight after reset() must not crash or produce
    // non-finite output.
    TestHelpers::fillWithSine (buffer, testSampleRate, testFrequencyHz, 0.9f);
    CHECK_NOTHROW (engine.process (block));
    CHECK (TestHelpers::allSamplesFinite (buffer));
}

TEST_CASE ("Engine: full-scale, maximum-gain input produces bounded, finite output", "[dsp][engine]")
{
    TenebraeEngine engine;
    engine.setGainDb (40.0f);
    engine.setTightFrequencyHz (300.0f);
    engine.setBassDb (15.0f);
    engine.setMidDb (15.0f);
    engine.setTrebleDb (15.0f);
    engine.setLevelDb (24.0f);
    engine.setMixProportion (1.0f);

    const auto spec = makeTestSpec (2);
    engine.prepare (spec);

    juce::AudioBuffer<float> buffer (2, testBlockSize);
    TestHelpers::fillWithSine (buffer, testSampleRate, testFrequencyHz, 1.0f);

    juce::dsp::AudioBlock<float> block (buffer);
    CHECK_NOTHROW (engine.process (block));

    CHECK (TestHelpers::allSamplesFinite (buffer));
    CHECK (TestHelpers::peakAbsolute (buffer) < 1000.0f); // sane bound, not just "finite"
}
