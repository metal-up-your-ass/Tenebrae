#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "params/ParameterIds.h"
#include "presets/Localisation.h"

#include <BinaryData.h>

namespace
{
    constexpr int knobSize = 90;
    constexpr int textBoxHeight = 20;
    constexpr int labelHeight = 20;
    constexpr int margin = 16;
    constexpr int presetBarHeight = 28;
    // Signal-flow order (docs/design-brief.md section 2): Tight, Gain,
    // Voicing, Bright, Bass, Mid, Treble, Tone Voice, Presence, Gate
    // Threshold/Attack/Hold/Release/On, Level, Mix - 16 control slots total
    // (12 knobs, 2 combo boxes, 2 toggle buttons), all the same slot width
    // for a simple, evenly spaced v0.1/v0.2 layout. A custom vector-drawn
    // GUI is a later milestone (M3) - per this suite's "do not gold-plate"
    // convention, v0.2.0 only adds the new controls to the existing plain
    // grid rather than redesigning the layout.
    constexpr int numSlots = 16;
    constexpr int editorWidth = margin * 2 + numSlots * knobSize + (numSlots - 1) * margin;
    constexpr int editorHeight = margin * 3 + presetBarHeight + labelHeight + knobSize + textBoxHeight;

    // M2 i18n frame (.scaffold/specs/preset-system-m2.md): selects German
    // (resources/i18n/de.txt) or falls through to English, once, at editor
    // construction - see Localisation.h's docs. `presetBar` is a member
    // initialised via the constructor's initialiser list, and its own
    // constructor already calls TRANS() on every button label - member
    // initialisers run in declaration order regardless of the order
    // they're written in, so this helper (called from presetBar's own
    // initialiser expression below) is what actually guarantees
    // installLocalisation() runs before presetBar exists, not an
    // installLocalisation() call in the constructor *body*, which would run
    // too late.
    basilica::presets::PresetManager& initLocalisationThenGetPresetManager (TenebraeAudioProcessor& processor)
    {
        basilica::presets::installLocalisation (BinaryData::de_txt, BinaryData::de_txtSize);
        return processor.presetManager;
    }
}

TenebraeAudioProcessorEditor::TenebraeAudioProcessorEditor (TenebraeAudioProcessor& processorToEdit)
    : juce::AudioProcessorEditor (&processorToEdit),
      audioProcessor (processorToEdit),
      presetBar (initLocalisationThenGetPresetManager (processorToEdit))
{
    addAndMakeVisible (presetBar);

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

    configureKnob (presenceKnob, ParamIDs::presence, "Presence");
    configureKnob (gateThresholdKnob, ParamIDs::gateThreshold, "Gate Thresh");
    configureKnob (gateAttackKnob, ParamIDs::gateAttack, "Gate Attack");
    configureKnob (gateHoldKnob, ParamIDs::gateHold, "Gate Hold");
    configureKnob (gateReleaseKnob, ParamIDs::gateRelease, "Gate Release");

    gateOnButton.setButtonText ("Gate");
    addAndMakeVisible (gateOnButton);
    gateOnAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, ParamIDs::gateOn, gateOnButton);

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

    presetBar.setBounds (bounds.removeFromTop (presetBarHeight));
    bounds.removeFromTop (margin);

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

    presenceKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    gateThresholdKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    gateAttackKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    gateHoldKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    gateReleaseKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));

    auto gateOnSlot = bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0);
    gateOnButton.setBounds (gateOnSlot.removeFromTop (comboBoxHeight));

    levelKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
    mixKnob.slider.setBounds (bounds.removeFromLeft (slotWidth).reduced (margin / 2, 0));
}
