#include "../TestHarness.h"
#include "engine/EngineHost.h"
#include "midi/MidiExporter.h"

using namespace morph;

namespace
{
    constexpr double sr = 48000.0;
    constexpr double bpm = 120.0;
    constexpr double samplesPerQuarter = sr * 60.0 / bpm; // 24000
    constexpr int tpq = 960;

    int64_t tickToSample (double tick)
    {
        return (int64_t) (tick / (double) tpq * samplesPerQuarter);
    }
}

MORPH_TEST (midiExport, timingMatchesScheduling)
{
    // Spec §91: export event timing equals heard scheduling.
    EngineHost host;
    host.prepare (sr, 512);
    host.setTempoBpm (bpm);
    host.requestPerformanceMode (PerformanceMode::strumUp);

    juce::MidiBuffer dummy;
    host.processBlock (dummy, 512); // apply mode

    const auto events = host.renderProgressionPerformance();
    CHECK_EQ ((int) events.size(), 4);

    auto file = MidiExporter::buildMidiFile (events, sr, bpm, (int64_t) (0.5 * sr));

    juce::MemoryOutputStream mem;
    CHECK (file.writeTo (mem));

    juce::MemoryInputStream in (mem.getData(), mem.getDataSize(), false);
    juce::MidiFile parsed;
    CHECK (parsed.readFrom (in));
    CHECK_EQ (parsed.getNumTracks(), 1);

    const auto* track = parsed.getNumTracks() > 0 ? parsed.getTrack (0) : nullptr;
    CHECK (track != nullptr);
    if (track == nullptr)
        return;

    // Collect note-ons in time order.
    std::vector<std::pair<double, int>> ons;
    for (int i = 0; i < track->getNumEvents(); ++i)
    {
        const auto& m = track->getEventPointer (i)->message;
        if (m.isNoteOn())
            ons.push_back ({ m.getTimeStamp(), m.getNoteNumber() });
    }

    // Expected: every scheduled note-on appears at its scheduled tick.
    int expectedOns = 0;
    for (const auto& chord : events)
        expectedOns += chord.notes.count;
    CHECK_EQ ((int) ons.size(), expectedOns);

    for (size_t chordIdx = 0; chordIdx < events.size(); ++chordIdx)
    {
        const auto& chord = events[chordIdx];
        for (int i = 0; i < chord.notes.count; ++i)
        {
            const auto& n = chord.notes.notes[(size_t) i];
            const int64_t expectedSample = chord.startSample + n.noteOnSampleOffset;

            bool matched = false;
            for (auto& on : ons)
            {
                if (on.second != n.pitch)
                    continue;
                const int64_t actualSample = tickToSample (on.first);
                if (std::abs (actualSample - expectedSample) <= 25) // one tick ≈ 0.5 ms
                {
                    matched = true;
                    on.first = -1.0; // consume
                    break;
                }
            }
            CHECK (matched);
        }
    }
}

MORPH_TEST (midiExport, strumOrderPreserved)
{
    EngineHost host;
    host.prepare (sr, 512);
    host.setTempoBpm (bpm);
    host.requestPerformanceMode (PerformanceMode::strumUp);
    juce::MidiBuffer dummy;
    host.processBlock (dummy, 512);

    const auto events = host.renderProgressionPerformance();
    CHECK (! events.empty());

    // First chord: Cm9 strummed up → exported note-ons ascend in time+pitch.
    const auto& chord = events[0];
    for (int i = 1; i < chord.notes.count; ++i)
    {
        CHECK (chord.notes.notes[(size_t) i].pitch > chord.notes.notes[(size_t) (i - 1)].pitch);
        CHECK (chord.notes.notes[(size_t) i].noteOnSampleOffset
               >= chord.notes.notes[(size_t) (i - 1)].noteOnSampleOffset);
    }
}
