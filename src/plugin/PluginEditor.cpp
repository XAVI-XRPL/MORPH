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
    header.setFeelValue (StyleProfile::get (StyleId::modernRnB).displayName);
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
    const int styleNow = (int) processor.apvts.getRawParameterValue ("styleIndex")->load();
    const int keyNow = (int) processor.apvts.getRawParameterValue ("keyIndex")->load();
    if (version != lastProgressionVersion || styleNow != lastStyleIndex || keyNow != lastKeyIndex)
    {
        lastProgressionVersion = version;
        lastStyleIndex = styleNow;
        lastKeyIndex = keyNow;
        refreshProgressionDisplay();
    }

    refreshKeyLabel();

    const int mode = (int) processor.apvts.getRawParameterValue ("performanceMode")->load();
    static const char* modeNames[numPerformanceModes] =
        { "TOGETHER", "STRUM ↑", "STRUM ↓", "PULSE", "PATTERN", "ARP" };
    header.setPerformanceValue (modeNames[mode % numPerformanceModes]);

    const int styleIdx = (int) processor.apvts.getRawParameterValue ("styleIndex")->load();
    header.setFeelValue (StyleProfile::get ((StyleId) styleIdx).displayName);

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
    // M6: six implemented style grammars; Reggaeton arrives with M7.
    juce::PopupMenu menu;
    const int current = (int) processor.apvts.getRawParameterValue ("styleIndex")->load();
    for (int i = 0; i < numImplementedStyles; ++i)
    {
        const auto style = StyleProfile::get ((StyleId) i);
        menu.addItem (style.displayName, true, i == current,
                      [this, i]
                      {
                          processor.apvts.getParameter ("styleIndex")
                              ->setValueNotifyingHost ((float) i / (float) (numImplementedStyles - 1));
                      });
    }
    menu.addSeparator();
    menu.addItem ("Reggaeton  (soon)", false, false, [] {});
    menu.setLookAndFeel (&lookAndFeel);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&header), {});
}

void PluginEditor::showPerformanceMenu()
{
    auto setParam = [this] (const char* id, float plain)
    {
        if (auto* param = processor.apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (plain));
    };

    juce::PopupMenu menu;
    const int current = (int) processor.apvts.getRawParameterValue ("performanceMode")->load();
    const juce::StringArray modes { "TOGETHER", "STRUM ↑", "STRUM ↓", "PULSE", "PATTERN", "ARP" };

    for (int i = 0; i < modes.size(); ++i)
        menu.addItem (modes[i], true, i == current,
                      [this, i, setParam]
                      {
                          setParam ("performanceMode", (float) i);
                          // Pill choices pin the default direction for strums (§5).
                          if (i == 1) setParam ("strumDirection", (float) StrumDirection::up);
                          if (i == 2) setParam ("strumDirection", (float) StrumDirection::down);
                      });

    // Contextual parameters (§5): only what the active mode needs.
    if (current == 1 || current == 2)
    {
        const int dirCurrent = (int) processor.apvts.getRawParameterValue ("strumDirection")->load();
        juce::PopupMenu dirMenu;
        const char* dirNames[numStrumDirections] =
            { "UP", "DOWN", "UP-DOWN", "DOWN-UP", "OUTSIDE-IN", "INSIDE-OUT", "CONTROLLED RANDOM" };
        for (int d = 0; d < numStrumDirections; ++d)
            dirMenu.addItem (dirNames[d], true, d == dirCurrent,
                             [this, d, setParam] { setParam ("strumDirection", (float) d); });
        menu.addSubMenu ("Direction", dirMenu);
    }

    if (current == 3 || current == 5) // PULSE / ARP share the rate control
    {
        const int rateCurrent = (int) processor.apvts.getRawParameterValue ("streamRate")->load();
        juce::PopupMenu rateMenu;
        const char* rateNames[3] = { "1/8", "1/16", "1/8 T" };
        for (int r = 0; r < 3; ++r)
            rateMenu.addItem (rateNames[r], true, r == rateCurrent,
                              [this, r, setParam] { setParam ("streamRate", (float) r); });
        menu.addSubMenu ("Rate", rateMenu);
    }

    if (current == 5) // ARP direction
    {
        const int arpCurrent = (int) processor.apvts.getRawParameterValue ("arpDirection")->load();
        juce::PopupMenu arpMenu;
        const char* arpNames[3] = { "UP", "DOWN", "UP-DOWN" };
        for (int d = 0; d < 3; ++d)
            arpMenu.addItem (arpNames[d], true, d == arpCurrent,
                             [this, d, setParam] { setParam ("arpDirection", (float) d); });
        menu.addSubMenu ("Arp direction", arpMenu);
    }

    if (current == 4) // PATTERN kind
    {
        const int patCurrent = (int) processor.apvts.getRawParameterValue ("patternKind")->load();
        juce::PopupMenu patMenu;
        const char* patNames[3] = { "BOUNCE", "FLOAT", "STAB" };
        for (int k = 0; k < 3; ++k)
            patMenu.addItem (patNames[k], true, k == patCurrent,
                             [this, k, setParam] { setParam ("patternKind", (float) k); });
        menu.addSubMenu ("Pattern", patMenu);
    }

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

    // GOLD bank alternatives for the current style (§32, §100).
    const int styleIdx = (int) processor.apvts.getRawParameterValue ("styleIndex")->load();
    const auto style = StyleProfile::get ((StyleId) styleIdx);
    int bankCount = 0;
    const auto* entries = bankEntriesForStyle ((StyleId) styleIdx, bankCount);

    if (entries != nullptr && bankCount > 0)
    {
        juce::PopupMenu alternatives;
        const int shown = juce::jmin (12, bankCount);
        for (int i = 0; i < shown; ++i)
        {
            const auto& e = entries[i];
            juce::String label;
            for (int slot = 0; slot < 4; ++slot)
            {
                const auto& spec = style.degrees[(size_t) (e.degrees[(size_t) slot] - 1)];
                label += romanFunctionToString ({ spec.latticeDegree, spec.accidental,
                                                  spec.quality, spec.extensions });
                if (slot < 3)
                    label += "  ·  ";
            }
            alternatives.addItem (label, true, false, [this, entry = e]
            {
                undoHistory.checkpoint (currentComposition());
                processor.engine.applyBankEntryFromUi (entry);
                updateUndoRedoButtons();
            });
        }
        menu.addSubMenu ("Progression alternatives", alternatives);
    }

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
    const int styleIdx = (int) processor.apvts.getRawParameterValue ("styleIndex")->load();
    radialField.setProgression (processor.engine.getProgressionForUi(),
                                keyContextForMinorTonicIndex (idx),
                                StyleProfile::get ((StyleId) styleIdx));
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
