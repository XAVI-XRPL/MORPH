#include "../TestHarness.h"
#include "midi/MidiScheduler.h"
#include "engine/performance/PerformanceEngine.h"

using namespace morph;

namespace
{
    constexpr double sr = 48000.0;

    ScheduledNoteList<maxChordVoices> makeStrumNotes()
    {
        // C2 G2 Bb3 D4 Eb4 ascending, 20 ms apart (100 ms spread total).
        ScheduledNoteList<maxChordVoices> notes;
        const int pitches[5] = { 36, 43, 58, 62, 63 };
        for (int i = 0; i < 5; ++i)
        {
            ScheduledNote n;
            n.pitch = pitches[i];
            n.velocity = 96;
            n.noteOnSampleOffset = (int64_t) (i * 0.020 * sr); // 0, 960, 1920, 2880, 3840
            n.noteOffSampleOffset = -1;
            n.role = i == 0 ? VoiceRole::bass : (i == 4 ? VoiceRole::top : VoiceRole::inner);
            notes.add (n);
        }
        return notes;
    }

    int countNoteOns (const juce::MidiBuffer& buf)
    {
        int n = 0;
        for (const auto meta : buf)
            if (meta.getMessage().isNoteOn())
                ++n;
        return n;
    }

    int countNoteOffs (const juce::MidiBuffer& buf)
    {
        int n = 0;
        for (const auto meta : buf)
            if (meta.getMessage().isNoteOff())
                ++n;
        return n;
    }
}

MORPH_TEST (midi, eventsFireAcrossBlocks)
{
    MidiScheduler sched;
    sched.prepare (sr);
    sched.reset();

    auto notes = makeStrumNotes();
    sched.scheduleChord (notes, 0);

    juce::MidiBuffer out;
    int totalOns = 0;

    // 100 ms at 48 kHz = 4800 samples > 512-sample block: must span blocks.
    for (int block = 0; block < 16; ++block)
    {
        out.clear();
        sched.processBlock (out, 512);
        totalOns += countNoteOns (out);
    }

    CHECK_EQ (totalOns, 5);

    for (int p : { 36, 43, 58, 62, 63 })
        CHECK (sched.isSounding (p));
}

MORPH_TEST (midi, releaseBeforeStrumEndCancelsPending)
{
    MidiScheduler sched;
    sched.prepare (sr);
    sched.reset();

    auto notes = makeStrumNotes();
    const int group = sched.scheduleChord (notes, 0);

    juce::MidiBuffer out;
    int totalOns = 0;
    int totalOffs = 0;

    // Run 1000 samples (block 512 + 488): attacks at 0 and 960 have fired.
    out.clear();
    sched.processBlock (out, 512);
    totalOns += countNoteOns (out);
    out.clear();
    sched.processBlock (out, 488);
    totalOns += countNoteOns (out);

    // Release before the strum finishes → CANCEL_PENDING_ATTACKS (§60).
    sched.releaseGroup (group, sched.getClock());

    for (int block = 0; block < 16; ++block)
    {
        out.clear();
        sched.processBlock (out, 512);
        totalOns += countNoteOns (out);
        totalOffs += countNoteOffs (out);
    }

    CHECK_EQ (totalOns, 2);   // only the attacks that already fired
    CHECK_EQ (totalOffs, 2);  // both released — never stuck notes
    CHECK (! sched.isSounding (36));
    CHECK (! sched.isSounding (43));
}

MORPH_TEST (midi, sustainDefersReleases)
{
    MidiScheduler sched;
    sched.prepare (sr);
    sched.reset();

    auto notes = makeStrumNotes();
    const int group = sched.scheduleChord (notes, 0);

    juce::MidiBuffer out;
    out.clear();
    sched.processBlock (out, 8192); // let the whole strum land

    sched.setSustain (true, sched.getClock());
    sched.releaseGroup (group, sched.getClock());

    out.clear();
    sched.processBlock (out, 512);
    // Sustain held: releases deferred, all notes still sounding.
    CHECK_EQ (countNoteOffs (out), 0);
    for (int p : { 36, 43, 58, 62, 63 })
        CHECK (sched.isSounding (p));

    sched.setSustain (false, sched.getClock());

    int offs = 0;
    for (int block = 0; block < 4; ++block)
    {
        out.clear();
        sched.processBlock (out, 512);
        offs += countNoteOffs (out);
    }

    CHECK_EQ (offs, 5);
    for (int p : { 36, 43, 58, 62, 63 })
        CHECK (! sched.isSounding (p));
}

MORPH_TEST (midi, retriggerKeepsCommonTones)
{
    MidiScheduler sched;
    sched.prepare (sr);
    sched.reset();

    ScheduledNoteList<maxChordVoices> a;
    for (int p : { 36, 43, 58 })
    {
        ScheduledNote n;
        n.pitch = p;
        n.velocity = 100;
        n.noteOnSampleOffset = 0;
        n.noteOffSampleOffset = -1;
        n.role = VoiceRole::inner;
        a.add (n);
    }

    const int groupA = sched.scheduleChord (a, 0);

    juce::MidiBuffer out;
    out.clear();
    sched.processBlock (out, 512);

    // New chord sharing pitch 43: adopt common tone, release the rest.
    sched.cancelPendingAttacks (groupA);
    sched.releasePitch (36, sched.getClock());
    sched.releasePitch (58, sched.getClock());

    ScheduledNoteList<maxChordVoices> b;
    {
        ScheduledNote n;
        n.pitch = 60;
        n.velocity = 100;
        n.noteOnSampleOffset = 0;
        n.noteOffSampleOffset = -1;
        n.role = VoiceRole::inner;
        b.add (n);
    }
    const int groupB = sched.scheduleChord (b, sched.getClock());
    sched.adoptSounding (43, groupB);

    out.clear();
    sched.processBlock (out, 512);

    CHECK (sched.isSounding (43));   // common tone preserved
    CHECK (sched.isSounding (60));   // new attack
    CHECK (! sched.isSounding (36)); // irrelevant notes released
    CHECK (! sched.isSounding (58));

    // Full cleanup: no stuck notes.
    sched.releaseGroup (groupB, sched.getClock());
    out.clear();
    sched.processBlock (out, 512);
    CHECK (! sched.isSounding (43));
    CHECK (! sched.isSounding (60));
}

MORPH_TEST (midi, noStuckNotesAfterAllNotesOff)
{
    MidiScheduler sched;
    sched.prepare (sr);
    sched.reset();

    auto notes = makeStrumNotes();
    sched.scheduleChord (notes, 0);

    juce::MidiBuffer out;
    out.clear();
    sched.processBlock (out, 1024); // partial strum

    sched.setSustain (true, sched.getClock());
    sched.allNotesOff (sched.getClock());

    int offs = 0;
    for (int block = 0; block < 32; ++block)
    {
        out.clear();
        sched.processBlock (out, 512);
        offs += countNoteOffs (out);
    }

    // Every fired attack must be released even with sustain held.
    for (int p = 0; p < 128; ++p)
        CHECK (! sched.isSounding (p));
}
