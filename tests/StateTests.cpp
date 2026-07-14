#include "PluginProcessor.h"
#include "params/ParameterIds.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("State round-trip preserves non-default values of every parameter", "[state]")
{
    TenebraeAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    auto* tightParam = processor.apvts.getParameter (ParamIDs::tight);
    auto* gainParam = processor.apvts.getParameter (ParamIDs::gain);
    auto* bassParam = processor.apvts.getParameter (ParamIDs::bass);
    auto* midParam = processor.apvts.getParameter (ParamIDs::mid);
    auto* trebleParam = processor.apvts.getParameter (ParamIDs::treble);
    auto* levelParam = processor.apvts.getParameter (ParamIDs::level);
    auto* mixParam = processor.apvts.getParameter (ParamIDs::mix);

    REQUIRE (tightParam != nullptr);
    REQUIRE (gainParam != nullptr);
    REQUIRE (bassParam != nullptr);
    REQUIRE (midParam != nullptr);
    REQUIRE (trebleParam != nullptr);
    REQUIRE (levelParam != nullptr);
    REQUIRE (mixParam != nullptr);

    tightParam->setValueNotifyingHost (tightParam->convertTo0to1 (150.0f));
    gainParam->setValueNotifyingHost (gainParam->convertTo0to1 (33.0f));
    bassParam->setValueNotifyingHost (bassParam->convertTo0to1 (9.0f));
    midParam->setValueNotifyingHost (midParam->convertTo0to1 (-7.0f));
    trebleParam->setValueNotifyingHost (trebleParam->convertTo0to1 (5.0f));
    levelParam->setValueNotifyingHost (levelParam->convertTo0to1 (-6.5f));
    mixParam->setValueNotifyingHost (mixParam->convertTo0to1 (42.0f));

    const auto savedTight = tightParam->getValue();
    const auto savedGain = gainParam->getValue();
    const auto savedBass = bassParam->getValue();
    const auto savedMid = midParam->getValue();
    const auto savedTreble = trebleParam->getValue();
    const auto savedLevel = levelParam->getValue();
    const auto savedMix = mixParam->getValue();

    juce::MemoryBlock savedState;
    processor.getStateInformation (savedState);
    REQUIRE (savedState.getSize() > 0);

    // Reset every parameter back to its default before restoring, so the
    // round-trip assertion below can't pass by accident.
    tightParam->setValueNotifyingHost (tightParam->getDefaultValue());
    gainParam->setValueNotifyingHost (gainParam->getDefaultValue());
    bassParam->setValueNotifyingHost (bassParam->getDefaultValue());
    midParam->setValueNotifyingHost (midParam->getDefaultValue());
    trebleParam->setValueNotifyingHost (trebleParam->getDefaultValue());
    levelParam->setValueNotifyingHost (levelParam->getDefaultValue());
    mixParam->setValueNotifyingHost (mixParam->getDefaultValue());

    REQUIRE (tightParam->getValue() != Catch::Approx (savedTight));
    REQUIRE (gainParam->getValue() != Catch::Approx (savedGain));
    REQUIRE (bassParam->getValue() != Catch::Approx (savedBass));
    REQUIRE (midParam->getValue() != Catch::Approx (savedMid));
    REQUIRE (trebleParam->getValue() != Catch::Approx (savedTreble));
    REQUIRE (levelParam->getValue() != Catch::Approx (savedLevel));
    REQUIRE (mixParam->getValue() != Catch::Approx (savedMix));

    processor.setStateInformation (savedState.getData(), static_cast<int> (savedState.getSize()));

    CHECK (tightParam->getValue() == Catch::Approx (savedTight).margin (1e-6));
    CHECK (gainParam->getValue() == Catch::Approx (savedGain).margin (1e-6));
    CHECK (bassParam->getValue() == Catch::Approx (savedBass).margin (1e-6));
    CHECK (midParam->getValue() == Catch::Approx (savedMid).margin (1e-6));
    CHECK (trebleParam->getValue() == Catch::Approx (savedTreble).margin (1e-6));
    CHECK (levelParam->getValue() == Catch::Approx (savedLevel).margin (1e-6));
    CHECK (mixParam->getValue() == Catch::Approx (savedMix).margin (1e-6));
}
