#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/ParameterIds.h"
#include "params/ParameterLayout.h"

#include <BinaryData.h>

namespace
{
    // The small, Tenebrae-specific config surface PresetManager needs (see
    // src/presets/PresetManager.h's class docs) - everything else about the
    // preset system is fully generic and portable to sibling plugins (see
    // basilica-audio/nave's docs/preset-system-notes.md).
    basilica::presets::PresetManagerConfig makePresetManagerConfig()
    {
        // JucePlugin_CFBundleIdentifier expands to a raw (unquoted) token
        // sequence, not a string literal - JUCE_STRINGIFY() is the
        // documented way to turn it into one. This is always
        // "com.yvesvogl.tenebrae" here (BUNDLE_ID in CMakeLists.txt),
        // matching the "plugin" field baked into every
        // presets/factory/*.json file.
        basilica::presets::PresetManagerConfig config;
        config.pluginId = JUCE_STRINGIFY (JucePlugin_CFBundleIdentifier);
        config.pluginName = JucePlugin_Name;
        config.manufacturerName = "Yves Vogl";
        config.pluginVersion = JucePlugin_VersionString;
        // userPresetsDirectoryOverrideForTests intentionally left
        // default-constructed (empty) - production instances always use the
        // real platform-standard preset location (see PresetManager.h).
        return config;
    }

    // BinaryData symbol names are derived from the presets/factory/*.json
    // file names passed to juce_add_binary_data() in CMakeLists.txt (dots
    // become underscores) - this list must stay in sync with that SOURCES
    // list. Order here only affects factory-preset iteration order before
    // getAllPresets() re-sorts alphabetically, so it isn't otherwise
    // significant.
    std::vector<basilica::presets::FactoryPresetAsset> makeFactoryPresetAssets()
    {
        return {
            { BinaryData::foundationChug_json, BinaryData::foundationChug_jsonSize },
            { BinaryData::lowTunedPercussive_json, BinaryData::lowTunedPercussive_jsonSize },
            { BinaryData::vintageCascade_json, BinaryData::vintageCascade_jsonSize },
            { BinaryData::scoopedWall_json, BinaryData::scoopedWall_jsonSize },
            { BinaryData::cutThroughLeadAdjacent_json, BinaryData::cutThroughLeadAdjacent_jsonSize },
            { BinaryData::brightAggressive_json, BinaryData::brightAggressive_jsonSize },
            { BinaryData::looseAndOpen_json, BinaryData::looseAndOpen_jsonSize },
            { BinaryData::fullDryWetBlend_json, BinaryData::fullDryWetBlend_jsonSize },
        };
    }
}

//==============================================================================
TenebraeAudioProcessor::TenebraeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout()),
      presetManager (apvts, makePresetManagerConfig(), makeFactoryPresetAssets())
{
    tightHz = apvts.getRawParameterValue (ParamIDs::tight);
    gainDb = apvts.getRawParameterValue (ParamIDs::gain);
    bassDb = apvts.getRawParameterValue (ParamIDs::bass);
    midDb = apvts.getRawParameterValue (ParamIDs::mid);
    trebleDb = apvts.getRawParameterValue (ParamIDs::treble);
    levelDb = apvts.getRawParameterValue (ParamIDs::level);
    mixPercent = apvts.getRawParameterValue (ParamIDs::mix);
    voicingChoice = apvts.getRawParameterValue (ParamIDs::voicing);
    brightToggle = apvts.getRawParameterValue (ParamIDs::bright);
    toneVoiceChoice = apvts.getRawParameterValue (ParamIDs::toneVoice);
    presenceDb = apvts.getRawParameterValue (ParamIDs::presence);
    gateThresholdDb = apvts.getRawParameterValue (ParamIDs::gateThreshold);
    gateAttackMs = apvts.getRawParameterValue (ParamIDs::gateAttack);
    gateHoldMs = apvts.getRawParameterValue (ParamIDs::gateHold);
    gateReleaseMs = apvts.getRawParameterValue (ParamIDs::gateRelease);
    gateOnToggle = apvts.getRawParameterValue (ParamIDs::gateOn);

    jassert (tightHz != nullptr);
    jassert (gainDb != nullptr);
    jassert (bassDb != nullptr);
    jassert (midDb != nullptr);
    jassert (trebleDb != nullptr);
    jassert (levelDb != nullptr);
    jassert (mixPercent != nullptr);
    jassert (voicingChoice != nullptr);
    jassert (brightToggle != nullptr);
    jassert (toneVoiceChoice != nullptr);
    jassert (presenceDb != nullptr);
    jassert (gateThresholdDb != nullptr);
    jassert (gateAttackMs != nullptr);
    jassert (gateHoldMs != nullptr);
    jassert (gateReleaseMs != nullptr);
    jassert (gateOnToggle != nullptr);

    // M2 default resolution: user "Default" preset > factory "Default"
    // preset > the ParameterLayout defaults apvts was just constructed
    // with above (see PresetManager::applyStartupDefault()'s docs and
    // docs/presets.md's note on why this repo's factory bank has no preset
    // literally named "Default").
    presetManager.applyStartupDefault();
}

TenebraeAudioProcessor::~TenebraeAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TenebraeAudioProcessor::createParameterLayout()
{
    return tnbr::createParameterLayout();
}

//==============================================================================
const juce::String TenebraeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TenebraeAudioProcessor::acceptsMidi() const
{
    return false;
}

bool TenebraeAudioProcessor::producesMidi() const
{
    return false;
}

bool TenebraeAudioProcessor::isMidiEffect() const
{
    return false;
}

double TenebraeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TenebraeAudioProcessor::getNumPrograms()
{
    return 1;
}

int TenebraeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TenebraeAudioProcessor::setCurrentProgram (int)
{
}

const juce::String TenebraeAudioProcessor::getProgramName (int)
{
    return {};
}

void TenebraeAudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================
void TenebraeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    // Seed the engine's parameters from the current APVTS state before
    // prepare() primes the filter coefficients, so the very first block
    // after prepareToPlay() already reflects the host/session's actual
    // parameter values rather than the engine's built-in defaults.
    engine.setTightFrequencyHz (tightHz->load (std::memory_order_relaxed));
    engine.setGainDb (gainDb->load (std::memory_order_relaxed));
    engine.setBassDb (bassDb->load (std::memory_order_relaxed));
    engine.setMidDb (midDb->load (std::memory_order_relaxed));
    engine.setTrebleDb (trebleDb->load (std::memory_order_relaxed));
    engine.setLevelDb (levelDb->load (std::memory_order_relaxed));
    engine.setMixProportion (mixPercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setVoicing (juce::roundToInt (voicingChoice->load (std::memory_order_relaxed)));
    engine.setBright (brightToggle->load (std::memory_order_relaxed) >= 0.5f);
    engine.setToneVoice (juce::roundToInt (toneVoiceChoice->load (std::memory_order_relaxed)));
    engine.setPresenceDb (presenceDb->load (std::memory_order_relaxed));
    engine.setGateThresholdDb (gateThresholdDb->load (std::memory_order_relaxed));
    engine.setGateAttackMs (gateAttackMs->load (std::memory_order_relaxed));
    engine.setGateHoldMs (gateHoldMs->load (std::memory_order_relaxed));
    engine.setGateReleaseMs (gateReleaseMs->load (std::memory_order_relaxed));
    engine.setGateOn (gateOnToggle->load (std::memory_order_relaxed) >= 0.5f);

    engine.prepare (spec);

    // Oversampling (8x, applied around the 3-stage cascade) is the only
    // source of reported latency; the dry path is compensated against it
    // internally by TenebraeEngine's DryWetMixer (see docs/architecture.md).
    setLatencySamples (engine.getLatencySamples());
}

void TenebraeAudioProcessor::releaseResources()
{
}

void TenebraeAudioProcessor::reset()
{
    engine.reset();
}

bool TenebraeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();

    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn = layouts.getMainInputChannelSet();

    if (mainOut != mono && mainOut != stereo)
        return false;

    if (mainOut != mainIn)
        return false;

    return true;
}

void TenebraeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Buses are constrained to in == out (mono or stereo), so this is
    // normally a no-op, but it's cheap insurance against stray channels.
    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    engine.setTightFrequencyHz (tightHz->load (std::memory_order_relaxed));
    engine.setGainDb (gainDb->load (std::memory_order_relaxed));
    engine.setBassDb (bassDb->load (std::memory_order_relaxed));
    engine.setMidDb (midDb->load (std::memory_order_relaxed));
    engine.setTrebleDb (trebleDb->load (std::memory_order_relaxed));
    engine.setLevelDb (levelDb->load (std::memory_order_relaxed));
    engine.setMixProportion (mixPercent->load (std::memory_order_relaxed) * 0.01f);
    engine.setVoicing (juce::roundToInt (voicingChoice->load (std::memory_order_relaxed)));
    engine.setBright (brightToggle->load (std::memory_order_relaxed) >= 0.5f);
    engine.setToneVoice (juce::roundToInt (toneVoiceChoice->load (std::memory_order_relaxed)));
    engine.setPresenceDb (presenceDb->load (std::memory_order_relaxed));
    engine.setGateThresholdDb (gateThresholdDb->load (std::memory_order_relaxed));
    engine.setGateAttackMs (gateAttackMs->load (std::memory_order_relaxed));
    engine.setGateHoldMs (gateHoldMs->load (std::memory_order_relaxed));
    engine.setGateReleaseMs (gateReleaseMs->load (std::memory_order_relaxed));
    engine.setGateOn (gateOnToggle->load (std::memory_order_relaxed) >= 0.5f);

    juce::dsp::AudioBlock<float> block (buffer);
    engine.process (block);
}

//==============================================================================
bool TenebraeAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* TenebraeAudioProcessor::createEditor()
{
    return new TenebraeAudioProcessorEditor (*this);
}

//==============================================================================
void TenebraeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    const std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TenebraeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TenebraeAudioProcessor();
}
