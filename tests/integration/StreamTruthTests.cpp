#include "../TestHarness.h"
#include "engine/EngineHost.h"
#include "ui/keyboard/KeyboardGeometry.h"

using namespace morph;

namespace
{
    constexpr double sr = 48000.0;
    constexpr int blockSize = 128;

    struct MidiTruth
    {
        std::bitset<128> sounding;
        void apply (const juce::MidiBuffer& buf)
        {
            for (const auto meta : buf)
            {
                const auto& m = meta.getMessage();
                if (m.isNoteOn())       sounding.set ((size_t) m.getNoteNumber());
                else if (m.isNoteOff()) sounding.reset ((size_t) m.getNoteNumber());
            }
        }
    };

    bool invariantHolds (EngineHost& host, const MidiTruth& truth, const KeyboardLayout& layout)
    {
        const auto& state = host.audioState();
        if (state.currentlySoundingNotes != truth.sounding)
            return false;
        const auto lit = computeLitKeys (layout, state);
        for (int i = 0; i < layout.count; ++i)
        {
            const int midi = layout.keys[(size_t) i].midi;
            if ((lit.find (midi) != nullptr) != truth.sounding.test ((size_t) midi))
                return false;
        }
        return true;
    }
}

MORPH_TEST (streamTruth, pulseStaysTruthfulThroughRelease)
{
    EngineHost host;
    host.prepare (sr, blockSize);
    host.setTempoBpm (120.0);
    host.requestPerformanceMode (PerformanceMode::pulse);

    const auto layout = buildKeyboardLayout (24, 96);
    MidiTruth truth;

    juce::MidiBuffer out;
    out.clear();
    host.processBlock (out, blockSize);
    truth.apply (out);

    host.noteOn (48, 1.0f, 0);

    // Two seconds of pulse at 120 BPM.
    int noteOnCount = 0;
    for (int block = 0; block < 750; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        for (const auto meta : out)
            if (meta.getMessage().isNoteOn())
                ++noteOnCount;
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }

    CHECK (noteOnCount > 50); // the pulse actually pulsed

    host.noteOff (48, 0);
    for (int block = 0; block < 16; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }
    CHECK (truth.sounding.none()); // stream fully stopped, no stuck notes
}

MORPH_TEST (streamTruth, arpRetriggerStaysClean)
{
    EngineHost host;
    host.prepare (sr, blockSize);
    host.requestPerformanceMode (PerformanceMode::arp);

    const auto layout = buildKeyboardLayout (24, 96);
    MidiTruth truth;

    juce::MidiBuffer out;
    out.clear();
    host.processBlock (out, blockSize);
    truth.apply (out);

    host.noteOn (48, 1.0f, 0);
    for (int block = 0; block < 40; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }

    // Retrigger mid-stream: F3.
    host.noteOn (53, 1.0f, 0);
    for (int block = 0; block < 40; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }

    // State reports the Fm9 family now.
    CHECK (host.audioState().hasActiveChord);
    CHECK (chordSymbolToString (host.audioState().activeChord, true).startsWith ("Fm"));

    host.noteOff (53, 0);
    for (int block = 0; block < 16; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }
    CHECK (truth.sounding.none());
}

MORPH_TEST (streamTruth, sequencerWithPulseStaysTruthful)
{
    EngineHost host;
    host.prepare (sr, blockSize);
    host.setTempoBpm (480.0);
    host.requestPerformanceMode (PerformanceMode::pulse);
    host.play();

    const auto layout = buildKeyboardLayout (24, 96);
    MidiTruth truth;

    juce::MidiBuffer out;
    int slotSwitches = 0;
    int lastSlot = -1;

    for (int block = 0; block < 900; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));

        const auto slot = host.audioState().activeProgressionSlot;
        if (slot != lastSlot)
        {
            ++slotSwitches;
            lastSlot = slot;
        }
    }

    CHECK (slotSwitches >= 4); // slots advanced through the loop

    host.stop();
    for (int block = 0; block < 16; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }
    CHECK (truth.sounding.none());
}
