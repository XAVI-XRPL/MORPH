#pragma once

#include "PluginProcessor.h"
#include "../ui/design_system/MorphLookAndFeel.h"
#include "../ui/MorphChassis.h"
#include "../ui/header/MorphHeader.h"
#include "../ui/controls/ActionRow.h"
#include "../ui/radial/RadialField.h"
#include "../ui/keyboard/MorphKeyboard.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace morph
{

/**
 * MorphEditor: the canonical chassis layout (§8–§10, §29, §34).
 * All content lives on a 1440×900 design canvas, scaled to fit (§111).
 */
class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor (MorphAudioProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void showPerformanceMenu();
    void showFeelPopover();
    void showKeyMenu();
    void showExplorePopover();
    void showMorePopover();
    void exportMidi();
    void refreshKeyLabel();

    MorphAudioProcessor& processor;
    MorphLookAndFeel lookAndFeel;

    MorphChassis chassis; // scaled design canvas

    MorphHeader header;
    RadialField radialField;
    ActionRow actionRow;
    MorphKeyboard keyboard;

    juce::Slider colorKnob, motionKnob, morphKnob, spaceKnob, textureKnob, outputKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAtt, motionAtt,
        morphAtt, spaceAtt, textureAtt, outputAtt;

    bool isPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

} // namespace morph
