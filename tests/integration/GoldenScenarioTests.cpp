#include "../TestHarness.h"
#include "engine/EngineHost.h"

using namespace morph;

namespace
{
    constexpr double sr = 48000.0;

    struct MidiCollector
    {
        std::vector<juce::MidiMessage> ons;
        std::vector<juce::MidiMessage> offs;

        void collect (const juce::MidiBuffer& buf)
        {
            for (const auto meta : buf)
            {
                const auto& m = meta.getMessage();
                if (m.isNoteOn())  ons.push_back (m);
                if (m.isNoteOff()) offs.push_back (m);
            }
        }
    };
}

MORPH_TEST (golden, pressC3ProducesCm9)
{
    // Spec §105: KEY C Minor, STYLE Modern R&B, INPUT C3 → i, Cm9.
    EngineHost host;
    host.prepare (sr, 512);

    juce::MidiBuffer out;
    host.noteOn (48, 1.0f, 0); // C3

    out.clear();
    host.processBlock (out, 512);

    MidiCollector midi;
    midi.collect (out);

    CHECK_EQ ((int) midi.ons.size(), 5);

    // Golden realization: C2 G2 Bb3 D4 Eb4 — all attack together.
    const int expected[5] = { 36, 43, 58, 62, 63 };
    for (int i = 0; i < 5; ++i)
    {
        bool found = false;
        int sampleOffset = -1;
        for (const auto meta : out)
        {
            const auto& m = meta.getMessage();
            if (m.isNoteOn() && m.getNoteNumber() == expected[i])
            {
                found = true;
                sampleOffset = meta.samplePosition;
            }
        }
        CHECK (found);
        CHECK (sampleOffset >= 0);
        CHECK (sampleOffset < (int) (PerformanceEngine::togetherToleranceMs * 0.001 * sr));
    }

    // Playback state truth.
    const auto& state = host.audioState();
    CHECK (state.hasActiveChord);
    CHECK (chordSymbolToString (state.activeChord, true) == "Cm9");
    CHECK (romanFunctionToString (state.activeRomanFunction) == "i");
    CHECK_EQ (state.bassPitch, 36);
    CHECK_EQ (state.topPitch, 63);

    for (int p : expected)
        CHECK (state.currentlySoundingNotes.test ((size_t) p));
    CHECK_EQ ((int) state.currentlySoundingNotes.count(), 5);

    host.noteOff (48, 0);
    out.clear();
    host.processBlock (out, 512);

    for (int p : expected)
        CHECK (! host.audioState().currentlySoundingNotes.test ((size_t) p));
}

MORPH_TEST (golden, sameChordAcrossPerformanceModes)
{
    // Milestone 2 acceptance: same exact chord/voicing through
    // TOGETHER / STRUM UP / STRUM DOWN without changing harmonic identity.
    EngineHost host;
    host.prepare (sr, 128);

    const int expectedPitches[5] = { 36, 43, 58, 62, 63 };

    auto capturePerformances = [&] (PerformanceMode mode) -> std::vector<std::pair<int, int64_t>>
    {
        host.requestPerformanceMode (mode);

        juce::MidiBuffer out;
        out.clear();
        host.processBlock (out, 128); // apply mode (re-performance)

        // Release whatever sounds, then retrigger fresh.
        host.noteOff (48, 0);
        for (int i = 0; i < 4; ++i) { out.clear(); host.processBlock (out, 128); }

        host.noteOn (48, 1.0f, 0);

        MidiCollector midi;
        int64_t clock = 0;
        std::vector<std::pair<int, int64_t>> attacks;
        for (int block = 0; block < 64; ++block)
        {
            out.clear();
            host.processBlock (out, 128);
            for (const auto meta : out)
                if (meta.getMessage().isNoteOn())
                    attacks.push_back ({ meta.getMessage().getNoteNumber(),
                                         clock + meta.samplePosition });
            clock += 128;
        }

        host.noteOff (48, 0);
        for (int i = 0; i < 4; ++i) { out.clear(); host.processBlock (out, 128); }
        return attacks;
    };

    const auto together = capturePerformances (PerformanceMode::together);
    const auto strumUp  = capturePerformances (PerformanceMode::strumUp);
    const auto strumDn  = capturePerformances (PerformanceMode::strumDown);

    // Same five pitches in every mode.
    auto pitchSet = [&] (const std::vector<std::pair<int, int64_t>>& a)
    {
        std::array<int, 5> ps {};
        for (size_t i = 0; i < 5; ++i)
            ps[i] = a[i].first;
        std::sort (ps.begin(), ps.end());
        return ps;
    };

    CHECK_EQ ((int) together.size(), 5);
    CHECK_EQ ((int) strumUp.size(), 5);
    CHECK_EQ ((int) strumDn.size(), 5);

    CHECK (pitchSet (together) == pitchSet (strumUp));
    CHECK (pitchSet (together) == pitchSet (strumDn));

    for (int p : expectedPitches)
        CHECK (std::find (pitchSet (together).begin(), pitchSet (together).end(), p)
               != pitchSet (together).end());

    // TOGETHER: attacks (nearly) simultaneous.
    {
        int64_t lo = INT64_MAX, hi = 0;
        for (auto& a : together) { lo = std::min (lo, a.second); hi = std::max (hi, a.second); }
        CHECK ((float) (hi - lo) / (float) sr * 1000.0f <= PerformanceEngine::togetherToleranceMs);
    }

    // STRUM UP: attacks ascend in pitch order.
    for (size_t i = 1; i < strumUp.size(); ++i)
    {
        CHECK (strumUp[i].first > strumUp[i - 1].first);
        CHECK (strumUp[i].second > strumUp[i - 1].second);
    }

    // STRUM DOWN: attacks descend.
    for (size_t i = 1; i < strumDn.size(); ++i)
    {
        CHECK (strumDn[i].first < strumDn[i - 1].first);
        CHECK (strumDn[i].second > strumDn[i - 1].second);
    }
}
