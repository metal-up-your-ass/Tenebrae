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
    auto* voicingParam = processor.apvts.getParameter (ParamIDs::voicing);
    auto* brightParam = processor.apvts.getParameter (ParamIDs::bright);
    auto* toneVoiceParam = processor.apvts.getParameter (ParamIDs::toneVoice);
    auto* presenceParam = processor.apvts.getParameter (ParamIDs::presence);
    auto* gateThresholdParam = processor.apvts.getParameter (ParamIDs::gateThreshold);
    auto* gateAttackParam = processor.apvts.getParameter (ParamIDs::gateAttack);
    auto* gateHoldParam = processor.apvts.getParameter (ParamIDs::gateHold);
    auto* gateReleaseParam = processor.apvts.getParameter (ParamIDs::gateRelease);
    auto* gateOnParam = processor.apvts.getParameter (ParamIDs::gateOn);

    REQUIRE (tightParam != nullptr);
    REQUIRE (gainParam != nullptr);
    REQUIRE (bassParam != nullptr);
    REQUIRE (midParam != nullptr);
    REQUIRE (trebleParam != nullptr);
    REQUIRE (levelParam != nullptr);
    REQUIRE (mixParam != nullptr);
    REQUIRE (voicingParam != nullptr);
    REQUIRE (brightParam != nullptr);
    REQUIRE (toneVoiceParam != nullptr);
    REQUIRE (presenceParam != nullptr);
    REQUIRE (gateThresholdParam != nullptr);
    REQUIRE (gateAttackParam != nullptr);
    REQUIRE (gateHoldParam != nullptr);
    REQUIRE (gateReleaseParam != nullptr);
    REQUIRE (gateOnParam != nullptr);

    tightParam->setValueNotifyingHost (tightParam->convertTo0to1 (150.0f));
    gainParam->setValueNotifyingHost (gainParam->convertTo0to1 (33.0f));
    bassParam->setValueNotifyingHost (bassParam->convertTo0to1 (9.0f));
    midParam->setValueNotifyingHost (midParam->convertTo0to1 (-7.0f));
    trebleParam->setValueNotifyingHost (trebleParam->convertTo0to1 (5.0f));
    levelParam->setValueNotifyingHost (levelParam->convertTo0to1 (-6.5f));
    mixParam->setValueNotifyingHost (mixParam->convertTo0to1 (42.0f));
    // Non-default choice/bool values (defaults are index 0 / false for
    // Voicing/Bright/Tone Voice, true for Gate On), so the round trip below
    // genuinely exercises them.
    voicingParam->setValueNotifyingHost (voicingParam->convertTo0to1 (1.0f)); // Loose
    brightParam->setValueNotifyingHost (brightParam->convertTo0to1 (1.0f)); // on
    toneVoiceParam->setValueNotifyingHost (toneVoiceParam->convertTo0to1 (2.0f)); // Boost
    presenceParam->setValueNotifyingHost (presenceParam->convertTo0to1 (7.5f));
    gateThresholdParam->setValueNotifyingHost (gateThresholdParam->convertTo0to1 (-30.0f));
    gateAttackParam->setValueNotifyingHost (gateAttackParam->convertTo0to1 (10.0f));
    gateHoldParam->setValueNotifyingHost (gateHoldParam->convertTo0to1 (250.0f));
    gateReleaseParam->setValueNotifyingHost (gateReleaseParam->convertTo0to1 (900.0f));
    gateOnParam->setValueNotifyingHost (gateOnParam->convertTo0to1 (0.0f)); // off (non-default)

    const auto savedTight = tightParam->getValue();
    const auto savedGain = gainParam->getValue();
    const auto savedBass = bassParam->getValue();
    const auto savedMid = midParam->getValue();
    const auto savedTreble = trebleParam->getValue();
    const auto savedLevel = levelParam->getValue();
    const auto savedMix = mixParam->getValue();
    const auto savedVoicing = voicingParam->getValue();
    const auto savedBright = brightParam->getValue();
    const auto savedToneVoice = toneVoiceParam->getValue();
    const auto savedPresence = presenceParam->getValue();
    const auto savedGateThreshold = gateThresholdParam->getValue();
    const auto savedGateAttack = gateAttackParam->getValue();
    const auto savedGateHold = gateHoldParam->getValue();
    const auto savedGateRelease = gateReleaseParam->getValue();
    const auto savedGateOn = gateOnParam->getValue();

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
    voicingParam->setValueNotifyingHost (voicingParam->getDefaultValue());
    brightParam->setValueNotifyingHost (brightParam->getDefaultValue());
    toneVoiceParam->setValueNotifyingHost (toneVoiceParam->getDefaultValue());
    presenceParam->setValueNotifyingHost (presenceParam->getDefaultValue());
    gateThresholdParam->setValueNotifyingHost (gateThresholdParam->getDefaultValue());
    gateAttackParam->setValueNotifyingHost (gateAttackParam->getDefaultValue());
    gateHoldParam->setValueNotifyingHost (gateHoldParam->getDefaultValue());
    gateReleaseParam->setValueNotifyingHost (gateReleaseParam->getDefaultValue());
    gateOnParam->setValueNotifyingHost (gateOnParam->getDefaultValue());

    REQUIRE (tightParam->getValue() != Catch::Approx (savedTight));
    REQUIRE (gainParam->getValue() != Catch::Approx (savedGain));
    REQUIRE (bassParam->getValue() != Catch::Approx (savedBass));
    REQUIRE (midParam->getValue() != Catch::Approx (savedMid));
    REQUIRE (trebleParam->getValue() != Catch::Approx (savedTreble));
    REQUIRE (levelParam->getValue() != Catch::Approx (savedLevel));
    REQUIRE (mixParam->getValue() != Catch::Approx (savedMix));
    REQUIRE (voicingParam->getValue() != Catch::Approx (savedVoicing));
    REQUIRE (brightParam->getValue() != Catch::Approx (savedBright));
    REQUIRE (toneVoiceParam->getValue() != Catch::Approx (savedToneVoice));
    REQUIRE (presenceParam->getValue() != Catch::Approx (savedPresence));
    REQUIRE (gateThresholdParam->getValue() != Catch::Approx (savedGateThreshold));
    REQUIRE (gateAttackParam->getValue() != Catch::Approx (savedGateAttack));
    REQUIRE (gateHoldParam->getValue() != Catch::Approx (savedGateHold));
    REQUIRE (gateReleaseParam->getValue() != Catch::Approx (savedGateRelease));
    REQUIRE (gateOnParam->getValue() != Catch::Approx (savedGateOn));

    processor.setStateInformation (savedState.getData(), static_cast<int> (savedState.getSize()));

    CHECK (tightParam->getValue() == Catch::Approx (savedTight).margin (1e-6));
    CHECK (gainParam->getValue() == Catch::Approx (savedGain).margin (1e-6));
    CHECK (bassParam->getValue() == Catch::Approx (savedBass).margin (1e-6));
    CHECK (midParam->getValue() == Catch::Approx (savedMid).margin (1e-6));
    CHECK (trebleParam->getValue() == Catch::Approx (savedTreble).margin (1e-6));
    CHECK (levelParam->getValue() == Catch::Approx (savedLevel).margin (1e-6));
    CHECK (mixParam->getValue() == Catch::Approx (savedMix).margin (1e-6));
    CHECK (voicingParam->getValue() == Catch::Approx (savedVoicing).margin (1e-6));
    CHECK (brightParam->getValue() == Catch::Approx (savedBright).margin (1e-6));
    CHECK (toneVoiceParam->getValue() == Catch::Approx (savedToneVoice).margin (1e-6));
    CHECK (presenceParam->getValue() == Catch::Approx (savedPresence).margin (1e-6));
    CHECK (gateThresholdParam->getValue() == Catch::Approx (savedGateThreshold).margin (1e-6));
    CHECK (gateAttackParam->getValue() == Catch::Approx (savedGateAttack).margin (1e-6));
    CHECK (gateHoldParam->getValue() == Catch::Approx (savedGateHold).margin (1e-6));
    CHECK (gateReleaseParam->getValue() == Catch::Approx (savedGateRelease).margin (1e-6));
    CHECK (gateOnParam->getValue() == Catch::Approx (savedGateOn).margin (1e-6));
}

TEST_CASE ("State round-trip: an old v0.1.0-style state tree (missing Presence/Gate IDs) falls back to v0.2.0 defaults",
           "[state]")
{
    // design-brief.md section 6: "old (v0.1.0) state trees loaded into
    // v0.2.0 must not fail or reset to defaults wholesale - missing
    // Presence/Gate parameter IDs in an old state tree fall back to their
    // v0.2.0 defaults." Simulated here by building a state tree containing
    // only the v0.1 parameter set (constructed via a second processor
    // instance whose new-in-v0.2.0 parameters are never touched, so its
    // APVTS state naturally omits nothing - JUCE's ValueTree always writes
    // every known parameter - so this instead directly strips the new
    // parameter IDs' XML attributes out of a saved state to reproduce a
    // genuinely old, pre-v0.2.0 state file).
    TenebraeAudioProcessor sourceProcessor;
    sourceProcessor.prepareToPlay (48000.0, 512);

    auto* tightParam = sourceProcessor.apvts.getParameter (ParamIDs::tight);
    REQUIRE (tightParam != nullptr);
    tightParam->setValueNotifyingHost (tightParam->convertTo0to1 (200.0f));

    juce::MemoryBlock savedState;
    sourceProcessor.getStateInformation (savedState);

    const std::unique_ptr<juce::XmlElement> xml (juce::AudioProcessor::getXmlFromBinary (
        savedState.getData(), static_cast<int> (savedState.getSize())));
    REQUIRE (xml != nullptr);

    // Strip every v0.2.0 parameter's <PARAM> child element, simulating a
    // state tree saved by the pre-v0.2.0 plugin (which never wrote them at
    // all).
    static constexpr const char* newParamIds[] = {
        ParamIDs::presence, ParamIDs::gateThreshold, ParamIDs::gateAttack,
        ParamIDs::gateHold, ParamIDs::gateRelease, ParamIDs::gateOn,
    };

    for (int i = xml->getNumChildElements(); --i >= 0;)
    {
        auto* child = xml->getChildElement (i);

        if (child == nullptr || ! child->hasTagName ("PARAM"))
            continue;

        const auto id = child->getStringAttribute ("id");

        for (const auto* newId : newParamIds)
        {
            if (id == juce::String (newId))
            {
                xml->removeChildElement (child, true);
                break;
            }
        }
    }

    juce::MemoryBlock strippedState;
    juce::AudioProcessor::copyXmlToBinary (*xml, strippedState);

    TenebraeAudioProcessor destinationProcessor;
    destinationProcessor.prepareToPlay (48000.0, 512);

    CHECK_NOTHROW (destinationProcessor.setStateInformation (strippedState.getData(), static_cast<int> (strippedState.getSize())));

    // The old (present) parameter round-tripped correctly...
    auto* destinationTight = destinationProcessor.apvts.getParameter (ParamIDs::tight);
    REQUIRE (destinationTight != nullptr);
    CHECK (destinationTight->convertFrom0to1 (destinationTight->getValue()) == Catch::Approx (200.0f).margin (1.0e-3));

    // ...and every v0.2.0-new parameter, absent from the loaded tree, fell
    // back to its own ParameterLayout default rather than failing the load
    // or resetting the whole state wholesale.
    auto* presenceParam = destinationProcessor.apvts.getParameter (ParamIDs::presence);
    auto* gateThresholdParam = destinationProcessor.apvts.getParameter (ParamIDs::gateThreshold);
    auto* gateOnParam = destinationProcessor.apvts.getParameter (ParamIDs::gateOn);

    REQUIRE (presenceParam != nullptr);
    REQUIRE (gateThresholdParam != nullptr);
    REQUIRE (gateOnParam != nullptr);

    CHECK (presenceParam->getValue() == Catch::Approx (presenceParam->getDefaultValue()).margin (1.0e-6));
    CHECK (gateThresholdParam->getValue() == Catch::Approx (gateThresholdParam->getDefaultValue()).margin (1.0e-6));
    CHECK (gateOnParam->getValue() == Catch::Approx (gateOnParam->getDefaultValue()).margin (1.0e-6));
    CHECK (gateOnParam->getValue() > 0.5f); // Gate defaults to ON - see design-brief.md section 5
}
