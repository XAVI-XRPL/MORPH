#include "../TestHarness.h"
#include "engine/EngineHost.h"
#include "ui/keyboard/KeyboardGeometry.h"

using namespace morph;

namespace
{
    constexpr double sr = 48000.0;
    constexpr int blockSize = 128;

    /** Shadow truth: sounding pitches tracked purely from emitted MIDI. */
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

    /** §42: actual MIDI == MusicalPlaybackState == keyboard illumination. */
    bool invariantHolds (EngineHost& host, const MidiTruth& truth, const KeyboardLayout& layout)
    {
        const auto& state = host.audioState();

        if (state.currentlySoundingNotes != truth.sounding)
            return false;

        const auto lit = computeLitKeys (layout, state);
        for (int i = 0; i < layout.count; ++i)
        {
            const int midi = layout.keys[(size_t) i].midi;
            const bool litNow = lit.find (midi) != nullptr;
            if (litNow != truth.sounding.test ((size_t) midi))
                return false;
        }
        return true;
    }
}

MORPH_TEST (truth, strumUnfoldsInExactMidiOrder)
{
    EngineHost host;
    host.prepare (sr, blockSize);
    host.requestPerformanceMode (PerformanceMode::strumUp);

    const auto layout = buildKeyboardLayout (24, 96);
    MidiTruth truth;

    juce::MidiBuffer out;
    out.clear();
    host.processBlock (out, blockSize); // apply mode
    truth.apply (out);

    host.noteOn (48, 1.0f, 0);

    int lastAttackedPitch = -1;
    for (int block = 0; block < 128; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);

        // During a strum, each new attack must be higher than the last (§39).
        for (const auto meta : out)
        {
            const auto& m = meta.getMessage();
            if (m.isNoteOn())
            {
                if (lastAttackedPitch >= 0)
                    CHECK (m.getNoteNumber() > lastAttackedPitch);
                lastAttackedPitch = m.getNoteNumber();
            }
        }

        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }

    CHECK_EQ ((int) truth.sounding.count(), 5);

    host.noteOff (48, 0);
    for (int block = 0; block < 8; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }
    CHECK (truth.sounding.none());
}

MORPH_TEST (truth, retriggerAndSustainNeverLie)
{
    EngineHost host;
    host.prepare (sr, blockSize);
    host.requestPerformanceMode (PerformanceMode::strumDown);

    const auto layout = buildKeyboardLayout (24, 96);
    MidiTruth truth;

    juce::MidiBuffer out;
    out.clear();
    host.processBlock (out, blockSize);
    truth.apply (out);

    host.noteOn (48, 1.0f, 0);          // C3 → Cm9
    for (int block = 0; block < 10; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }

    host.sustainPedal (true, 0);
    host.noteOn (53, 1.0f, 0);          // F3 → Fm9, retrigger mid-flight
    for (int block = 0; block < 20; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }

    host.noteOff (53, 0);               // release with sustain held
    for (int block = 0; block < 8; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }

    host.sustainPedal (false, 0);       // everything must drain out
    for (int block = 0; block < 8; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }

    CHECK (truth.sounding.none());
}

MORPH_TEST (truth, sequencerPlaybackStaysTruthful)
{
    EngineHost host;
    host.prepare (sr, blockSize);
    host.setTempoBpm (240.0); // 1 bar = 48000 samples = 375 blocks
    host.play();

    const auto layout = buildKeyboardLayout (24, 96);
    MidiTruth truth;

    juce::MidiBuffer out;
    int sawSlot0 = 0, sawOtherSlots = 0;
    int lastSlot = -1;

    // 1600 blocks = 204800 samples ≈ 4.27 bars at 240 BPM.
    for (int block = 0; block < 1600; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));

        const auto slot = host.audioState().activeProgressionSlot;
        if (slot != lastSlot)
        {
            if (slot == 0) ++sawSlot0; else if (slot > 0) ++sawOtherSlots;
            lastSlot = slot;
        }
    }

    CHECK (sawSlot0 >= 2);         // the loop wrapped at least twice
    CHECK (sawOtherSlots >= 3);    // pucks advanced through slots

    host.stop();
    for (int block = 0; block < 8; ++block)
    {
        out.clear();
        host.processBlock (out, blockSize);
        truth.apply (out);
        CHECK (invariantHolds (host, truth, layout));
    }
    CHECK (truth.sounding.none());
}
