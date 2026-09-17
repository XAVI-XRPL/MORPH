#include "MidiScheduler.h"

namespace morph
{

void MidiScheduler::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
}

void MidiScheduler::reset()
{
    eventCount = 0;
    for (auto& s : sounding)
        s = SoundingNote {};
    sustainDown = false;
    clockSamples = 0;
}

void MidiScheduler::insertEvent (const PendingEvent& e)
{
    if (eventCount >= maxEvents)
        return;

    int i = eventCount++;
    while (i > 0 && events[(size_t) (i - 1)].sampleTime > e.sampleTime)
    {
        events[(size_t) i] = events[(size_t) (i - 1)];
        --i;
    }
    events[(size_t) i] = e;
}

int MidiScheduler::scheduleChord (const ScheduledNoteList<maxChordVoices>& notes,
                                  int64_t originSample)
{
    const int groupId = nextGroupId++;
    const int64_t origin = std::max (originSample, clockSamples);

    for (int i = 0; i < notes.count; ++i)
    {
        const auto& n = notes.notes[(size_t) i];

        PendingEvent on;
        on.sampleTime = origin + n.noteOnSampleOffset;
        on.pitch = (uint8_t) n.pitch;
        on.velocity = (uint8_t) juce::jlimit (1, 127, n.velocity);
        on.role = (int8_t) n.role;
        on.groupId = groupId;
        on.isNoteOn = true;
        insertEvent (on);

        if (n.noteOffSampleOffset >= 0)
        {
            PendingEvent off = on;
            off.sampleTime = origin + n.noteOffSampleOffset;
            off.isNoteOn = false;
            insertEvent (off);
        }
    }

    return groupId;
}

void MidiScheduler::cancelPendingAttacks (int groupId)
{
    // Removes every not-yet-fired event of the group (ons and offs).
    int out = 0;
    for (int i = 0; i < eventCount; ++i)
    {
        if (events[(size_t) i].groupId != groupId)
            events[(size_t) out++] = events[(size_t) i];
    }
    eventCount = out;
}

void MidiScheduler::releaseGroup (int groupId, int64_t atSample)
{
    cancelPendingAttacks (groupId);

    const int64_t when = std::max (atSample, clockSamples);

    for (int pitch = 0; pitch < 128; ++pitch)
    {
        auto& s = sounding[(size_t) pitch];
        if (s.sounding && s.groupId == groupId)
        {
            if (sustainDown)
            {
                s.offDeferredBySustain = true;
            }
            else
            {
                PendingEvent off;
                off.sampleTime = when;
                off.pitch = (uint8_t) pitch;
                off.velocity = 0;
                off.groupId = groupId;
                off.isNoteOn = false;
                insertEvent (off);
            }
        }
    }
}

void MidiScheduler::releasePitch (int pitch, int64_t atSample)
{
    if (pitch < 0 || pitch > 127)
        return;

    auto& s = sounding[(size_t) pitch];
    if (! s.sounding)
        return;

    if (sustainDown)
    {
        s.offDeferredBySustain = true;
        return;
    }

    PendingEvent off;
    off.sampleTime = std::max (atSample, clockSamples);
    off.pitch = (uint8_t) pitch;
    off.velocity = 0;
    off.groupId = s.groupId;
    off.isNoteOn = false;
    insertEvent (off);
}

void MidiScheduler::adoptSounding (int pitch, int newGroupId)
{
    if (pitch < 0 || pitch > 127)
        return;

    auto& s = sounding[(size_t) pitch];
    if (s.sounding)
    {
        s.groupId = newGroupId;
        s.offDeferredBySustain = false;
    }
}

void MidiScheduler::setSustain (bool down, int64_t atSample)
{
    sustainDown = down;

    if (! down)
    {
        const int64_t when = std::max (atSample, clockSamples);
        for (int pitch = 0; pitch < 128; ++pitch)
        {
            auto& s = sounding[(size_t) pitch];
            if (s.sounding && s.offDeferredBySustain)
            {
                PendingEvent off;
                off.sampleTime = when;
                off.pitch = (uint8_t) pitch;
                off.velocity = 0;
                off.groupId = s.groupId;
                off.isNoteOn = false;
                insertEvent (off);
                s.offDeferredBySustain = false;
            }
        }
    }
}

void MidiScheduler::allNotesOff (int64_t atSample)
{
    eventCount = 0;
    const int64_t when = std::max (atSample, clockSamples);

    for (int pitch = 0; pitch < 128; ++pitch)
    {
        auto& s = sounding[(size_t) pitch];
        if (s.sounding)
        {
            PendingEvent off;
            off.sampleTime = when;
            off.pitch = (uint8_t) pitch;
            off.velocity = 0;
            off.groupId = s.groupId;
            off.isNoteOn = false;
            insertEvent (off);
        }
    }
}

void MidiScheduler::fireEvent (const PendingEvent& e, juce::MidiBuffer& out,
                               int64_t blockStart)
{
    const int offset = (int) juce::jlimit<int64_t> (0, 4096, e.sampleTime - blockStart);

    if (e.isNoteOn)
    {
        out.addEvent (juce::MidiMessage::noteOn (1, e.pitch, (float) e.velocity / 127.0f), offset);
        auto& s = sounding[e.pitch];
        s.sounding = true;
        s.groupId = e.groupId;
        s.offDeferredBySustain = false;
    }
    else
    {
        out.addEvent (juce::MidiMessage::noteOff (1, e.pitch), offset);
        sounding[e.pitch].sounding = false;
    }
}

void MidiScheduler::processBlock (juce::MidiBuffer& out, int numSamples)
{
    const int64_t blockStart = clockSamples;
    const int64_t blockEnd = blockStart + numSamples;

    // Fire and remove due events (sorted ascending by sampleTime).
    int remaining = 0;
    for (int i = 0; i < eventCount; ++i)
    {
        const auto& e = events[(size_t) i];
        if (e.sampleTime < blockEnd)
            fireEvent (e, out, blockStart);
        else
            events[(size_t) remaining++] = e;
    }
    eventCount = remaining;

    clockSamples = blockEnd;
}

bool MidiScheduler::isSounding (int pitch) const
{
    return pitch >= 0 && pitch < 128 && sounding[(size_t) pitch].sounding;
}

bool MidiScheduler::groupHasPendingOrSounding (int groupId) const
{
    for (int i = 0; i < eventCount; ++i)
        if (events[(size_t) i].groupId == groupId)
            return true;

    for (const auto& s : sounding)
        if (s.sounding && s.groupId == groupId)
            return true;

    return false;
}

void MidiScheduler::rebuildStateBits (MusicalPlaybackState& state) const
{
    state.currentlySoundingNotes.reset();
    state.pendingScheduledNotes.reset();

    for (int pitch = 0; pitch < 128; ++pitch)
        if (sounding[(size_t) pitch].sounding)
            state.currentlySoundingNotes.set ((size_t) pitch);

    for (int i = 0; i < eventCount; ++i)
        if (events[(size_t) i].isNoteOn)
            state.pendingScheduledNotes.set (events[(size_t) i].pitch);

    state.sustainState = sustainDown;
}

} // namespace morph
