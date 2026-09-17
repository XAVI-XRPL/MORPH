#include "PluginEditor.h"
#include "../engine/theory/KeyContext.h"

namespace morph
{

namespace
{
    constexpr float designW = 1440.0f;
    constexpr float designH = 900.0f;

    struct KnobSpec
    {
        const char* paramId;
        const char* label;
        const char* sublabel;
        juce::Colour capColour;
    };
}

PluginEditor::PluginEditor (MorphAudioProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p)
{
    setSize ((int) designW, (int) designH);
    setResizable (true, true);
    setResizeLimits (1080, 675, 1680, 1050);
    getConstrainer()->setFixedAspectRatio (designW / designH);

    chassis.setSize ((int) designW, (int) designH);
    addAndMakeVisible (chassis);

    // --- Header ---
    header.setKeyValue ("C Minor");
    header.setFeelValue ("Modern R&B");
    header.setPerformanceValue ("TOGETHER");
    header.onKeyPrevious = [this]
    {
        auto* param = processor.apvts.getParameter ("keyIndex");
        const int v = (int) processor.apvts.getRawParameterValue ("keyIndex")->load();
        param->setValueNotifyingHost ((float) ((v + 11) % 12) / 11.0f);
    };
    header.onKeyNext = [this]
    {
        auto* param = processor.apvts.getParameter ("keyIndex");
        const int v = (int) processor.apvts.getRawParameterValue ("keyIndex")->load();
        param->setValueNotifyingHost ((float) ((v + 1) % 12) / 11.0f);
    };
    header.onKeyCenter = [this] { showKeyMenu(); };
    header.onFeelCenter = [this] { showFeelPopover(); };
    header.onFeelPrevious = header.onFeelNext = [this] { showFeelPopover(); };
    header.onPerformanceClick = [this] { showPerformanceMenu(); };
    header.onMoreClick = [this] { showMorePopover(); };
    chassis.addAndMakeVisible (header);

    // --- Knob banks ---
    auto makeKnob = [this] (juce::Slider& knob, const char* paramId, juce::Colour cap,
                            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att)
    {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob.setLookAndFeel (&lookAndFeel);
        knob.getProperties().set (MorphLookAndFeel::capColourProperty, (int64_t) cap.getARGB());
        knob.setDoubleClickReturnValue (true, 0.5);
        chassis.addAndMakeVisible (knob);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, paramId, knob);
    };

    makeKnob (colorKnob,   "color",   MorphTheme::bassBlue,        colorAtt);
    makeKnob (motionKnob,  "motion",  MorphTheme::innerOrange,     motionAtt);
    makeKnob (morphKnob,   "morph",   MorphTheme::knobBody,        morphAtt);
    makeKnob (spaceKnob,   "space",   juce::Colour (0xff9a938a),   spaceAtt);
    makeKnob (textureKnob, "texture", juce::Colour (0xffb08d5e),   textureAtt);
    makeKnob (outputKnob,  "output",  MorphTheme::accentOrange,    outputAtt);

    // --- Radial field ---
    chassis.addAndMakeVisible (radialField);
    radialField.onSlotLockToggle = [this] (int slot)
    {
        processor.engine.toggleSlotLockFromUi (slot);
    };
    refreshProgressionDisplay();

    // --- Action row ---
    actionRow.onSave = [this]
    {
        if (auto xml = processor.apvts.copyState().createXml())
        {
            auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile ("MORPH");
            dir.createDirectory();
            xml->writeTo (dir.getChildFile ("morph-state.xml"));
        }
    };

    // MORPH action (§30): undo checkpoint, then deterministic sibling.
    actionRow.onMorph = [this]
    {
        undoHistory.checkpoint (currentComposition());
        processor.engine.morphProgressionFromUi();
        updateUndoRedoButtons();
    };

    // UNDO / REDO (§68): exact deterministic state restore.
    actionRow.onUndo = [this]
    {
        if (undoHistory.canUndo())
        {
            applyComposition (undoHistory.undo (currentComposition()));
            updateUndoRedoButtons();
        }
    };
    actionRow.onRedo = [this]
    {
        if (undoHistory.canRedo())
        {
            applyComposition (undoHistory.redo (currentComposition()));
            updateUndoRedoButtons();
        }
    };

    actionRow.onPlay = [this]
    {
        if (isPlaying)
            processor.engine.stop();
        else
            processor.engine.play();
    };
    actionRow.onExplore = [this] { showExplorePopover(); };
    actionRow.onMidi = [this] { exportMidi(); };
    actionRow.onMidiDragStart = [this] { exportMidiToTempAndDrag(); };
    actionRow.onMore = [this] { showMorePopover(); };
    updateUndoRedoButtons();
    chassis.addAndMakeVisible (actionRow);

    // --- Keyboard ---
    keyboard.onNoteEvent = [this] (int pitch, bool on, float velocity)
    {
        processor.uiNoteQueue.push ({ pitch, on, velocity });
    };
    chassis.addAndMakeVisible (keyboard);

    refreshKeyLabel();
    startTimerHz (30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    for (auto* k : { &colorKnob, &motionKnob, &morphKnob, &spaceKnob, &textureKnob, &outputKnob })
        k->setLookAndFeel (nullptr);
}

void PluginEditor::refreshKeyLabel()
{
    const int idx = (int) processor.apvts.getRawParameterValue ("keyIndex")->load();
    header.setKeyValue (keyContextDisplayName (keyContextForMinorTonicIndex (idx)));
}

void PluginEditor::timerCallback()
{
    const auto snapshot = processor.engine.stateBuffer().read();
    keyboard.setState (snapshot);
    radialField.setState (snapshot);

    const auto version = processor.engine.progressionVersion.load();
    if (version != lastProgressionVersion)
    {
        lastProgressionVersion = version;
        refreshProgressionDisplay();
    }

    refreshKeyLabel();

    const int mode = (int) processor.apvts.getRawParameterValue ("performanceMode")->load();
    header.setPerformanceValue (mode == 0 ? "TOGETHER" : mode == 1 ? "STRUM ↑" : "STRUM ↓");

    isPlaying = snapshot.isSequencerPlaying;
    actionRow.setPlaying (isPlaying);
}

void PluginEditor::showKeyMenu()
{
    juce::PopupMenu menu;
    for (int i = 0; i < 12; ++i)
    {
        const auto key = keyContextForMinorTonicIndex (i);
        menu.addItem (keyContextDisplayName (key), true,
                      i == (int) processor.apvts.getRawParameterValue ("keyIndex")->load(),
                      [this, i]
                      {
                          processor.apvts.getParameter ("keyIndex")
                              ->setValueNotifyingHost ((float) i / 11.0f);
                      });
    }
    menu.setLookAndFeel (&lookAndFeel);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&header),
                        [this] (int) { refreshKeyLabel(); });
}

void PluginEditor::showFeelPopover()
{
    // V1 ships Modern R&B; the remaining feels arrive with later milestones.
    juce::PopupMenu menu;
    menu.addItem ("Modern R&B", true, true, [] {});
    menu.addSeparator();
    const juce::StringArray coming { "Dark R&B", "Neo-Soul", "Emotional", "Dark Pop", "Trap", "Reggaeton" };
    for (auto& name : coming)
        menu.addItem (name + "  (soon)", false, false, [] {});
    menu.setLookAndFeel (&lookAndFeel);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&header), {});
}

void PluginEditor::showPerformanceMenu()
{
    juce::PopupMenu menu;
    const int current = (int) processor.apvts.getRawParameterValue ("performanceMode")->load();
    const juce::StringArray modes { "TOGETHER", "STRUM ↑", "STRUM ↓" };

    for (int i = 0; i < modes.size(); ++i)
        menu.addItem (modes[i], true, i == current,
                      [this, i]
                      {
                          processor.apvts.getParameter ("performanceMode")
                              ->setValueNotifyingHost ((float) i / 2.0f);
                      });

    // Contextual strum parameters (§5): spread adjustment lives here, not
    // permanently on the main surface.
    menu.addSeparator();
    const float spreadMs = processor.apvts.getRawParameterValue ("strumSpread")->load();
    menu.addItem ("Spread: " + juce::String ((int) spreadMs) + " ms", false, false, [] {});

    auto adjustSpread = [this] (float delta)
    {
        auto* param = processor.apvts.getParameter ("strumSpread");
        const float cur = processor.apvts.getRawParameterValue ("strumSpread")->load();
        param->setValueNotifyingHost (param->convertTo0to1 (
            juce::jlimit (20.0f, 120.0f, cur + delta)));
    };
    menu.addItem ("Spread − 10 ms", true, false, [adjustSpread] { adjustSpread (-10.0f); });
    menu.addItem ("Spread + 10 ms", true, false, [adjustSpread] { adjustSpread (10.0f); });

    menu.setLookAndFeel (&lookAndFeel);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&header), {});
}

void PluginEditor::applyPerformancePreset (const PerformancePreset& preset)
{
    auto setParam = [this] (const char* id, float plainValue)
    {
        if (auto* param = processor.apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    };

    setParam ("performanceMode", (float) preset.mode);
    setParam ("togetherKind", (float) preset.togetherKind);
    setParam ("strumSpread", preset.spreadMs);
    setParam ("strumCurve", (float) preset.curve);
    setParam ("strumVelShape", (float) preset.velocityShape);
    setParam ("bassPolicy", (float) preset.bassPolicy);
    setParam ("topPolicy", (float) preset.topPolicy);
}

void PluginEditor::showExplorePopover()
{
    // EXPLORE (§32): deeper composition lives here, not on the main surface.
    juce::PopupMenu menu;

    menu.addItem ("New progression variation", true, false, [this]
    {
        undoHistory.checkpoint (currentComposition());
        processor.engine.morphProgressionFromUi();
        updateUndoRedoButtons();
    });

    // Performance presets (§58 curated set).
    juce::PopupMenu presets;
    for (int i = 0; i < numPerformancePresets; ++i)
        presets.addItem (performancePresets[i].name, true, false,
                         [this, i] { applyPerformancePreset (performancePresets[i]); });
    menu.addSubMenu ("Performance presets", presets);

    menu.addSeparator();
    menu.addItem ("Song Kit", false, false, [] {});      // M9
    menu.addItem ("Capture (last 60 s)", false, false, [] {}); // §80

    menu.setLookAndFeel (&lookAndFeel);
    menu.showMenuAsync (juce::PopupMenu::Options(), {});
}

void PluginEditor::showMorePopover()
{
    juce::PopupMenu menu;
    menu.addItem ("MORPH 0.2 — Milestones 1+2+4", false, false, [] {});
    menu.addItem ("Generative harmony instrument", false, false, [] {});
    menu.addSeparator();
    menu.addItem ("MIDI channel: 1", false, false, [] {});
    menu.setLookAndFeel (&lookAndFeel);
    menu.showMenuAsync (juce::PopupMenu::Options(), {});
}

CompositionSnapshot PluginEditor::currentComposition() const
{
    return { processor.engine.getProgressionForUi(), processor.engine.getMorphVariation() };
}

void PluginEditor::applyComposition (const CompositionSnapshot& s)
{
    processor.engine.setProgressionFromUi (s.progression);
    processor.engine.setMorphVariation (s.morphVariation);
}

void PluginEditor::updateUndoRedoButtons()
{
    actionRow.setUndoRedoEnabled (undoHistory.canUndo(), undoHistory.canRedo());
}

void PluginEditor::refreshProgressionDisplay()
{
    const int idx = (int) processor.apvts.getRawParameterValue ("keyIndex")->load();
    radialField.setProgression (processor.engine.getProgressionForUi(),
                                keyContextForMinorTonicIndex (idx),
                                StyleProfile::modernRnB());
}

void PluginEditor::exportMidiToTempAndDrag()
{
    auto events = processor.engine.renderProgressionPerformance();
    if (events.empty())
        return;

    auto file = MidiExporter::buildMidiFile (events, 48000.0, 80.0, (int64_t) (0.5 * 48000.0));
    auto temp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("morph-progression.mid");
    if (MidiExporter::writeToFile (file, temp))
        performExternalDragDropOfFiles ({ temp.getFullPathName() }, true);
}

void PluginEditor::exportMidi()
{
    auto events = processor.engine.renderProgressionPerformance();
    if (events.empty())
        return;

    const double bpm = 80.0;
    auto file = MidiExporter::buildMidiFile (events, 48000.0, bpm, (int64_t) (0.5 * 48000.0));

    auto chooser = std::make_shared<juce::FileChooser> (
        "Export MORPH performance", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                        .getChildFile ("morph-progression.mid"),
        "*.mid");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                          | juce::FileBrowserComponent::canSelectFiles,
                          [file = std::move (file), chooser] (const juce::FileChooser& fc) mutable
                          {
                              auto dest = fc.getResult();
                              if (dest != juce::File())
                                  MidiExporter::writeToFile (file, dest);
                          });
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (MorphTheme::windowBackground);
}

void PluginEditor::resized()
{
    // Scale-to-fit the 1440×900 design canvas, centered (§111).
    const float s = juce::jmin ((float) getWidth() / MorphChassis::designW,
                                (float) getHeight() / MorphChassis::designH);

    const int w = (int) (MorphChassis::designW * s);
    const int h = (int) (MorphChassis::designH * s);
    chassis.setBounds ((getWidth() - w) / 2, (getHeight() - h) / 2,
                       (int) MorphChassis::designW, (int) MorphChassis::designH);
    chassis.setTransform (juce::AffineTransform::scale (s));

    // Design-space layout (children live in the untransformed 1440×900 space).
    header.setBounds (36, 30, 1368, 76);

    radialField.setBounds (490, 148, 460, 460);

    const int knobSize = 108;
    const int knobY = 300;
    colorKnob.setBounds   (116 - knobSize / 2, knobY, knobSize, knobSize);
    motionKnob.setBounds  (260 - knobSize / 2, knobY, knobSize, knobSize);
    morphKnob.setBounds   (404 - knobSize / 2, knobY, knobSize, knobSize);
    spaceKnob.setBounds   (1036 - knobSize / 2, knobY, knobSize, knobSize);
    textureKnob.setBounds (1180 - knobSize / 2, knobY, knobSize, knobSize);
    outputKnob.setBounds  (1324 - knobSize / 2, knobY, knobSize, knobSize);

    actionRow.setBounds (56, 612, 1328, 64);
    keyboard.setBounds (68, 706, 1304, 148);
}

} // namespace morph
