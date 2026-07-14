#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class TenebraeAudioProcessor;

// A simple, functional v0.1 editor: one rotary slider per parameter, bound
// to the APVTS via SliderAttachment. A custom vector-drawn GUI is a later
// milestone; this is deliberately plain but fully wired and usable.
class TenebraeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit TenebraeAudioProcessorEditor (TenebraeAudioProcessor& processorToEdit);
    ~TenebraeAudioProcessorEditor() override;

    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    // One knob + label per parameter, in signal-flow order.
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void configureKnob (Knob& knob, const juce::String& parameterId, const juce::String& labelText);

    TenebraeAudioProcessor& audioProcessor;

    Knob tightKnob;
    Knob gainKnob;
    Knob bassKnob;
    Knob midKnob;
    Knob trebleKnob;
    Knob levelKnob;
    Knob mixKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TenebraeAudioProcessorEditor)
};
