#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "params/ParameterIds.h"

namespace
{
    constexpr int knobSize = 90;
    constexpr int textBoxHeight = 20;
    constexpr int labelHeight = 20;
    constexpr int margin = 16;
    // Signal-flow order: Tight, Gain, Voicing, Bright, Bass, Mid, Treble,
    // Tone Voice, Level, Mix - 10 control slots total (7 knobs, 2 combo
    // boxes, 1 toggle button), all the same slot width for a simple, evenly
    // spaced v0.1 layout.
    constexpr int numSlots = 10;
    constexpr int editorWidth = margin * 2 + numSlots * knobSize + (numSlots - 1) * margin;
    constexpr int editorHeight = margin * 2 + labelHeight + knobSize + textBoxHeight;
}

TenebraeAudioProcessorEditor::TenebraeAudioProcessorEditor (TenebraeAudioProcessor& processorToEdit)
    : juce::AudioProcessorEditor (&processorToEdit),
      audioProcessor (processorToEdit)
{
    configureKnob (tightKnob, ParamIDs::tight, "Tight");
    configureKnob (gainKnob, ParamIDs::gain, "Gain");
    configureChoice (voicingChoice, ParamIDs::voicing, "Voicing");

    brightButton.setButtonText ("Bright");
    addAndMakeVisible (brightButton);
    brightAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, ParamIDs::bright, brightButton);

    configureKnob (bassKnob, ParamIDs::bass, "Bass");
    configureKnob (midKnob, ParamIDs::mid, "Mid");
    configureKnob (trebleKnob, ParamIDs::treble, "Treble");
    configureChoice (toneVoiceChoice, ParamIDs::toneVoice, "Tone Voice");
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

void TenebraeAudioProcessorEditor::configureChoice (Choice& choice, const juce::String& parameterId, const juce::String& labelText)
{
    choice.box.addItemList (audioProcessor.apvts.getParameter (parameterId)->getAllValueStrings(), 1);
    addAndMakeVisible (choice.box);

    choice.label.setText (labelText, juce::dontSendNotification);
    choice.label.setJustificationType (juce::Justification::centred);
    choice.label.attachToComponent (&choice.box, false);
    addAndMakeVisible (choice.label);

    choice.attachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, parameterId, choice.box);
}

void TenebraeAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (margin);
    bounds.removeFromTop (labelHeight); // room for the attached labels above each control

    const auto slotWidth = bounds.getWidth() / numSlots;
    const auto comboBoxHeight = textBoxHeight;

    tightKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    gainKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));

    auto voicingSlot = bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0);
    voicingChoice.box.setBounds (voicingSlot.removeFromTop (comboBoxHeight));

    auto brightSlot = bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0);
    brightButton.setBounds (brightSlot.removeFromTop (comboBoxHeight));

    bassKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    midKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    trebleKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));

    auto toneVoiceSlot = bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0);
    toneVoiceChoice.box.setBounds (toneVoiceSlot.removeFromTop (comboBoxHeight));

    levelKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    mixKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
}
