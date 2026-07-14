#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "params/ParameterIds.h"

namespace
{
    constexpr int knobSize = 90;
    constexpr int textBoxHeight = 20;
    constexpr int labelHeight = 20;
    constexpr int margin = 16;
    constexpr int numKnobs = 7;
    constexpr int editorWidth = margin * 2 + numKnobs * knobSize + (numKnobs - 1) * margin;
    constexpr int editorHeight = margin * 2 + labelHeight + knobSize + textBoxHeight;
}

TenebraeAudioProcessorEditor::TenebraeAudioProcessorEditor (TenebraeAudioProcessor& processorToEdit)
    : juce::AudioProcessorEditor (&processorToEdit),
      audioProcessor (processorToEdit)
{
    configureKnob (tightKnob, ParamIDs::tight, "Tight");
    configureKnob (gainKnob, ParamIDs::gain, "Gain");
    configureKnob (bassKnob, ParamIDs::bass, "Bass");
    configureKnob (midKnob, ParamIDs::mid, "Mid");
    configureKnob (trebleKnob, ParamIDs::treble, "Treble");
    configureKnob (levelKnob, ParamIDs::level, "Level");
    configureKnob (mixKnob, ParamIDs::mix, "Mix");

    setResizable (false, false);
    setSize (editorWidth, editorHeight);
}

TenebraeAudioProcessorEditor::~TenebraeAudioProcessorEditor() = default;

void TenebraeAudioProcessorEditor::configureKnob (Knob& knob, const juce::String& parameterId, const juce::String& labelText)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, knobSize, textBoxHeight);
    addAndMakeVisible (knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    // false => label sits above the slider it tracks; JUCE repositions it
    // automatically whenever the slider's bounds change, so resized() only
    // needs to place the sliders themselves.
    knob.label.attachToComponent (&knob.slider, false);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, parameterId, knob.slider);
}

void TenebraeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (margin);
    bounds.removeFromTop (labelHeight); // room for the attached labels above each knob

    const auto slotWidth = bounds.getWidth() / numKnobs;

    for (auto* knob : { &tightKnob, &gainKnob, &bassKnob, &midKnob, &trebleKnob, &levelKnob, &mixKnob })
        knob->slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
}
