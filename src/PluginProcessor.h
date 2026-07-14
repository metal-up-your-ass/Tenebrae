#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/TenebraeEngine.h"

// Tenebrae: cascaded, oversampled high-gain distortion for symphonic-metal
// rhythm guitar. Signal flow lives in TenebraeEngine (src/dsp) so it stays
// unit-testable independent of this AudioProcessor; this class is just
// APVTS + host plumbing around it.
class TenebraeAudioProcessor final : public juce::AudioProcessor
{
public:
    TenebraeAudioProcessor();
    ~TenebraeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    TenebraeEngine engine;

    // Raw atomic pointers into the APVTS-managed parameter values, resolved
    // once at construction time so processBlock() never has to search for
    // them (no allocation/locks on the audio thread).
    std::atomic<float>* tightHz = nullptr;
    std::atomic<float>* gainDb = nullptr;
    std::atomic<float>* bassDb = nullptr;
    std::atomic<float>* midDb = nullptr;
    std::atomic<float>* trebleDb = nullptr;
    std::atomic<float>* levelDb = nullptr;
    std::atomic<float>* mixPercent = nullptr;
    std::atomic<float>* voicingChoice = nullptr;
    std::atomic<float>* brightToggle = nullptr;
    std::atomic<float>* toneVoiceChoice = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TenebraeAudioProcessor)
};
