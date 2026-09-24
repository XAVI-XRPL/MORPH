/**
 * MorphUISnapshot: renders the editor offscreen to PNGs for visual
 * verification against the canonical reference — idle state, golden chord
 * (TOGETHER), and mid-strum state.
 */
#include "../src/plugin/PluginProcessor.h"
#include "../src/plugin/PluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

using namespace morph;

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File outDir = argc > 1
        ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
        : juce::File::getCurrentWorkingDirectory();

    auto processor = std::make_unique<MorphAudioProcessor>();
    processor->prepareToPlay (48000.0, 512);

    auto editor = std::make_unique<PluginEditor> (*processor);
    editor->setSize (1440, 900);

    juce::AudioBuffer<float> audio (2, 512);

    auto pumpFrames = [&] (int n)
    {
        for (int i = 0; i < n; ++i)
        {
            juce::MidiBuffer midi;
            processor->processBlock (audio, midi);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (34); // ~30 Hz UI
        }
    };

    auto snapshot = [&] (const juce::String& name)
    {
        auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        auto file = outDir.getChildFile (name);
        juce::FileOutputStream out (file);
        out.setPosition (0);
        out.truncate();
        juce::PNGImageFormat png;
        const bool ok = png.writeImageToStream (img, out);
        juce::Logger::writeToLog (file.getFullPathName() + "  " + (ok ? "ok" : "FAILED"));
    };

    // 1. Idle chassis.
    pumpFrames (20);
    snapshot ("morph_idle.png");

    // 2. Golden chord sounding (TOGETHER).
    processor->uiNoteQueue.push ({ 48, true, 1.0f });
    pumpFrames (12);
    snapshot ("morph_golden_chord.png");

    // 3. Strum up mid-flight.
    processor->uiNoteQueue.push ({ 48, false, 0.0f });
    processor->apvts.getParameter ("performanceMode")->setValueNotifyingHost (1.0f / 2.0f);
    pumpFrames (6);
    processor->uiNoteQueue.push ({ 48, true, 1.0f });
    pumpFrames (3); // catch the strum mid-unfold (spread 42 ms ≈ 4 blocks + UI)
    snapshot ("morph_strum.png");

    // 4. Progression playing.
    processor->uiNoteQueue.push ({ 48, false, 0.0f });
    processor->apvts.getParameter ("performanceMode")->setValueNotifyingHost (0.0f);
    processor->engine.play();
    pumpFrames (40);
    snapshot ("morph_play.png");
    processor->engine.stop();
    pumpFrames (6);

    // 5. M5 workflow: lock slot 2 (iv), morph the progression.
    processor->engine.toggleSlotLockFromUi (2);
    processor->engine.morphProgressionFromUi();
    pumpFrames (20);
    snapshot ("morph_locked_morphed.png");

    // 6. M6 styles: Neo-Soul feel changes the harmonic vocabulary.
    processor->engine.toggleSlotLockFromUi (2); // unlock again
    processor->engine.setProgressionFromUi (Progression()); // reset to gold family
    processor->apvts.getParameter ("styleIndex")->setValueNotifyingHost (2.0f / 5.0f);
    pumpFrames (20);
    snapshot ("morph_neosoul.png");

    // 7. M8: PULSE mode mid-stream.
    processor->apvts.getParameter ("styleIndex")->setValueNotifyingHost (0.0f);
    processor->apvts.getParameter ("performanceMode")->setValueNotifyingHost (3.0f / 5.0f);
    pumpFrames (6);
    processor->uiNoteQueue.push ({ 48, true, 1.0f });
    pumpFrames (5);
    snapshot ("morph_pulse.png");
    processor->uiNoteQueue.push ({ 48, false, 0.0f });
    pumpFrames (4);

    processor->engine.stop();
    juce::MessageManager::deleteInstance();
    return 0;
}
