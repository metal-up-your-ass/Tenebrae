#include "PluginProcessor.h"
#include "params/ParameterIds.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
    // Convenience wrapper: fetches a parameter by ID and requires it to
    // exist before returning, so every SECTION below fails loudly (not with
    // a null-deref) if an ID typo ever creeps in.
    juce::RangedAudioParameter* requireParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
    {
        auto* param = apvts.getParameter (id);
        REQUIRE (param != nullptr);
        return param;
    }

    // Checks that a float parameter's underlying NormalisableRange covers
    // [expectedMin, expectedMax], independent of any skew/log mapping.
    void checkFloatRange (juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& id,
                           float expectedMin,
                           float expectedMax)
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (id));
        REQUIRE (param != nullptr);

        const auto range = param->getNormalisableRange().getRange();
        CHECK (range.getStart() == Catch::Approx (expectedMin));
        CHECK (range.getEnd() == Catch::Approx (expectedMax));
    }

    // Checks a float parameter's default value in real (non-normalised)
    // units, going through convertTo0to1 so log-skewed ranges are handled
    // the same way as linear ones.
    void checkFloatDefault (juce::AudioProcessorValueTreeState& apvts,
                             const juce::String& id,
                             float expectedDefault)
    {
        auto* param = requireParam (apvts, id);
        CHECK (param->getDefaultValue() == Catch::Approx (param->convertTo0to1 (expectedDefault)).margin (1e-4));
    }
}

TEST_CASE ("Processor instantiates with the expected parameters", "[processor][parameters]")
{
    TenebraeAudioProcessor processor;
    auto& apvts = processor.apvts;

    SECTION ("plugin name")
    {
        CHECK (processor.getName() == juce::String ("Tenebrae"));
    }

    SECTION ("all documented parameter IDs resolve")
    {
        static constexpr const char* allIds[] = {
            ParamIDs::tight, ParamIDs::gain, ParamIDs::bass, ParamIDs::mid,
            ParamIDs::treble, ParamIDs::level, ParamIDs::mix,
        };

        for (const auto* id : allIds)
            CHECK (apvts.getParameter (id) != nullptr);
    }

    SECTION ("total parameter count matches the v0.1 layout")
    {
        CHECK (apvts.processor.getParameters().size() == 7);
    }

    SECTION ("Tight: high-pass pre-emphasis defaults and range")
    {
        checkFloatDefault (apvts, ParamIDs::tight, 90.0f);
        checkFloatRange (apvts, ParamIDs::tight, 20.0f, 300.0f);
    }

    SECTION ("Gain: pre-cascade drive defaults and range")
    {
        checkFloatDefault (apvts, ParamIDs::gain, 24.0f);
        checkFloatRange (apvts, ParamIDs::gain, 0.0f, 40.0f);
    }

    SECTION ("Bass: tone-stack low-shelf defaults and range")
    {
        checkFloatDefault (apvts, ParamIDs::bass, 0.0f);
        checkFloatRange (apvts, ParamIDs::bass, -15.0f, 15.0f);
    }

    SECTION ("Mid: tone-stack peak defaults and range")
    {
        checkFloatDefault (apvts, ParamIDs::mid, 0.0f);
        checkFloatRange (apvts, ParamIDs::mid, -15.0f, 15.0f);
    }

    SECTION ("Treble: tone-stack high-shelf defaults and range")
    {
        checkFloatDefault (apvts, ParamIDs::treble, 0.0f);
        checkFloatRange (apvts, ParamIDs::treble, -15.0f, 15.0f);
    }

    SECTION ("Level: output trim defaults and range")
    {
        checkFloatDefault (apvts, ParamIDs::level, 0.0f);
        checkFloatRange (apvts, ParamIDs::level, -24.0f, 24.0f);
    }

    SECTION ("Mix: dry/wet defaults and range")
    {
        checkFloatDefault (apvts, ParamIDs::mix, 100.0f);
        checkFloatRange (apvts, ParamIDs::mix, 0.0f, 100.0f);
    }
}
