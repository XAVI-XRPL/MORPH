#include "EngineHost.h"

namespace morph
{

EngineHost::EngineHost()
{
    state.innerVoices.fill (-1);
}

void EngineHost::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    blockSize = maxBlockSize > 0 ? maxBlockSize : 512;
    scheduler.prepare (sampleRate);
}

KeyContext EngineHost::currentKey() const
{
    return keyContextForMinorTonicIndex (keyIndex.load (std::memory_order_relaxed));
}

PerformanceProfile EngineHost::buildProfile() const
{
    PerformanceProfile p;
    p.mode = (PerformanceMode) performanceMode.load (std::memory_order_relaxed);
    p.together.kind = (TogetherKind) togetherKind.load (std::memory_order_relaxed);

    // MOTION scales strum spread: still → tighter, moving → wider (±30%).
    const float motion = motionKnob.load (std::memory_order_relaxed);
    const float motionScale = 1.3f - 0.6f * motion;

    p.strum.direction = (p.mode == PerformanceMode::strumDown) ? StrumDirection::down
                                                               : StrumDirection::up;
    p.strum.spreadMs = strumSpreadMs.load (std::memory_order_relaxed) * motionScale;
    p.strum.curve = (StrumCurve) strumCurve.load (std::memory_order_relaxed);
    p.strum.velocityShape = (StrumVelocityShape) strumVelocityShape.load (std::memory_order_relaxed);
    p.strum.bassPolicy = (BassStrumPolicy) bassPolicy.load (std::memory_order_relaxed);
    p.strum.topVoicePolicy = (TopVoicePerformancePolicy) topVoicePolicy.load (std::memory_order_relaxed);

    // TEXTURE → humanization amount.
    const float texture = textureKnob.load (std::memory_order_relaxed);
    p.humanizeTiming = texture;
    p.humanizeVelocity = texture;
    p.strum.randomTimingAmount = 0.2f + 0.8f * texture;
    p.strum.randomVelocityAmount = 0.1f + 0.6f * texture;

    // OUTPUT → velocity baseline.
    p.velocityBaseline = 48 + (int) (outputKnob.load (std::memory_order_relaxed) * 72.0f);

    return p;
}

ChordRealization EngineHost::realizeDegree (ScaleDegree degree, const Voicing* previous)
{
    const auto key = currentKey();
    const auto candidate = harmony.chordForDegree (key, degree, style,
                                                   colorKnob.load (std::memory_order_relaxed));
    return voicingEngine.realize (candidate, key, style, previous,
                                  spaceKnob.load (std::memory_order_relaxed),
                                  morphVariation.load (std::memory_order_relaxed));
}

void EngineHost::updateGeneratedState (const ChordRealization& r,
                                       const ScheduledNoteList<maxChordVoices>& notes)
{
    state.generatedChordNotes.reset();
    state.generatedRoles.fill (-1);
    state.velocityData.fill (0);

    for (int i = 0; i < r.voicing.count; ++i)
    {
        const auto& v = r.voicing.voices[(size_t) i];
        state.generatedChordNotes.set ((size_t) v.pitch.value);
        state.generatedRoles[(size_t) v.pitch.value] = (int8_t) v.role;
    }

    for (int i = 0; i < notes.count; ++i)
        state.velocityData[(size_t) notes.notes[(size_t) i].pitch] = (uint8_t) notes.notes[(size_t) i].velocity;

    state.activeChord = r.candidate.chord;
    state.activeRomanFunction = r.candidate.roman;
    state.hasActiveChord = true;
    state.bassPitch = r.bassPitch.value;
    state.topPitch = r.topPitch.value;

    state.innerVoiceCount = 0;
    for (int i = 0; i < r.voicing.count; ++i)
    {
        const auto& v = r.voicing.voices[(size_t) i];
        if (v.role == VoiceRole::inner && state.innerVoiceCount < (int) state.innerVoices.size())
            state.innerVoices[(size_t) state.innerVoiceCount++] = v.pitch.value;
    }
}

void EngineHost::scheduleLive (const ChordRealization& r, int64_t originSample)
{
    const auto profile = buildProfile();
    auto notes = PerformanceEngine::schedule (r, profile, sampleRate, -1,
                                              0x9E3779B9u ^ (triggerCounter * 2654435761u));

    // Retrigger semantics (spec §61): cancel obsolete pending attacks,
    // release irrelevant notes, preserve common tones, avoid duplicates.
    if (liveGroupId > 0)
    {
        scheduler.cancelPendingAttacks (liveGroupId);

        std::array<bool, 128> adopted {};
        adopted.fill (false);

        for (int pitch = 0; pitch < 128; ++pitch)
        {
            if (scheduler.soundingGroup (pitch) == liveGroupId)
            {
                if (r.voicing.containsPitch (pitch))
                {
                    adopted[(size_t) pitch] = true; // adopt after scheduling
                }
                else
                {
                    scheduler.releasePitch (pitch, originSample);
                }
            }
        }

        // Drop note-ons for adopted pitches (they keep sounding).
        ScheduledNoteList<maxChordVoices> filtered;
        for (int i = 0; i < notes.count; ++i)
            if (! adopted[(size_t) notes.notes[(size_t) i].pitch])
                filtered.add (notes.notes[(size_t) i]);

        const int newGroup = scheduler.scheduleChord (filtered, originSample);

        for (int pitch = 0; pitch < 128; ++pitch)
            if (adopted[(size_t) pitch])
                scheduler.adoptSounding (pitch, newGroup);

        liveGroupId = newGroup;
    }
    else
    {
        liveGroupId = scheduler.scheduleChord (notes, originSample);
    }

    activeGroupOrigin = std::max (originSample, scheduler.getClock());
    int64_t maxOffset = 0;
    for (int i = 0; i < notes.count; ++i)
        maxOffset = std::max (maxOffset, notes.notes[(size_t) i].noteOnSampleOffset);
    activeSpreadSamples = std::max<int64_t> (1, maxOffset);
    activeGroupNoteCount = notes.count;

    state.strumDirection = profile.strum.direction;
    state.performanceMode = profile.mode;

    updateGeneratedState (r, notes);
    lastRealization = r;
    hasLastRealization = true;
    ++triggerCounter;
}

void EngineHost::noteOn (int inputPitch, float velocity, int64_t offsetWithinBlock)
{
    juce::ignoreUnused (velocity);

    if (inputPitch < 0 || inputPitch > 127)
        return;

    const auto key = currentKey();
    const auto result = interpreter.interpret (MidiPitch { inputPitch }, key);
    lastDegree = result.degree.value;

    const Voicing* previous = hasLastRealization ? &lastRealization.voicing : nullptr;
    auto realization = realizeDegree (result.degree, previous);

    liveInputPitch = inputPitch;
    state.inputNotes.set ((size_t) inputPitch);

    if (sequencerPlaying)
    {
        state.isLiveOverride = true;
        if (sequencerGroupId > 0)
        {
            scheduler.releaseGroup (sequencerGroupId, scheduler.getClock() + offsetWithinBlock);
            sequencerGroupId = -1;
        }
    }

    scheduleLive (realization, scheduler.getClock() + offsetWithinBlock);
}

void EngineHost::noteOff (int inputPitch, int64_t offsetWithinBlock)
{
    if (inputPitch < 0 || inputPitch > 127)
        return;

    state.inputNotes.reset ((size_t) inputPitch);

    if (inputPitch == liveInputPitch && liveGroupId > 0)
    {
        // Release-before-strum-end policy: CANCEL_PENDING_ATTACKS (spec §60).
        scheduler.releaseGroup (liveGroupId, scheduler.getClock() + offsetWithinBlock);
        liveGroupId = -1;
        liveInputPitch = -1;

        if (sequencerPlaying)
        {
            state.isLiveOverride = false;
            resumeAtNextBar = true; // return to sequence at the next boundary (§65)
        }
    }
}

void EngineHost::sustainPedal (bool down, int64_t offsetWithinBlock)
{
    scheduler.setSustain (down, scheduler.getClock() + offsetWithinBlock);
}

void EngineHost::requestPerformanceMode (PerformanceMode mode)
{
    performanceMode.store ((int) mode, std::memory_order_relaxed);
    pendingMode.store ((int) mode, std::memory_order_relaxed);
}

void EngineHost::requestMorphVariation()
{
    morphVariation.fetch_add (1, std::memory_order_relaxed);
    morphPending.store (true, std::memory_order_relaxed);
}

void EngineHost::play()  { transportCommand.store (1, std::memory_order_relaxed); }
void EngineHost::stop()  { transportCommand.store (2, std::memory_order_relaxed); }

void EngineHost::scheduleSequencerSlot (int slot, int64_t barStart, int64_t barLength)
{
    const auto& s = progression.slots[(size_t) slot];
    lastDegree = s.degree.value;
    const Voicing* previous = hasLastRealization ? &lastRealization.voicing : nullptr;
    auto realization = realizeDegree (s.degree, previous);

    const auto profile = buildProfile();
    auto notes = PerformanceEngine::schedule (realization, profile, sampleRate,
                                              barLength - (int64_t) (0.02 * sampleRate),
                                              0x51ED270Bu ^ ((uint32_t) slot * 144665u));

    if (sequencerGroupId > 0)
        scheduler.releaseGroup (sequencerGroupId, barStart);

    sequencerGroupId = scheduler.scheduleChord (notes, barStart);

    activeGroupOrigin = barStart;
    int64_t maxOffset = 0;
    for (int i = 0; i < notes.count; ++i)
        maxOffset = std::max (maxOffset, notes.notes[(size_t) i].noteOnSampleOffset);
    activeSpreadSamples = std::max<int64_t> (1, maxOffset);
    activeGroupNoteCount = notes.count;

    state.activeProgressionSlot = slot;
    state.strumDirection = profile.strum.direction;
    state.performanceMode = profile.mode;

    updateGeneratedState (realization, notes);
    lastRealization = realization;
    hasLastRealization = true;
}

std::vector<MidiExporter::ChordEvent> EngineHost::renderProgressionPerformance()
{
    std::vector<MidiExporter::ChordEvent> events;
    events.reserve ((size_t) progression.size);

    const int64_t barLength = (int64_t) (sampleRate * 60.0 / tempoBpm * 4.0);
    const auto profile = buildProfile();

    Voicing prev {};
    const Voicing* prevPtr = nullptr;

    for (int slot = 0; slot < progression.size; ++slot)
    {
        auto realization = realizeDegree (progression.slots[(size_t) slot].degree, prevPtr);

        MidiExporter::ChordEvent ev;
        ev.startSample = (int64_t) slot * barLength;
        ev.notes = PerformanceEngine::schedule (realization, profile, sampleRate,
                                                barLength - (int64_t) (0.02 * sampleRate),
                                                0x51ED270Bu ^ ((uint32_t) slot * 144665u));
        events.push_back (ev);

        prev = realization.voicing;
        prevPtr = &prev;
    }

    return events;
}

std::vector<MidiExporter::ChordEvent> EngineHost::renderCurrentChordPerformance()
{
    std::vector<MidiExporter::ChordEvent> events;

    if (! hasLastRealization)
        return events;

    const int64_t barLength = (int64_t) (sampleRate * 60.0 / tempoBpm * 4.0);
    MidiExporter::ChordEvent ev;
    ev.startSample = 0;
    ev.notes = PerformanceEngine::schedule (lastRealization, buildProfile(), sampleRate,
                                            barLength, 0x9E3779B9u);
    events.push_back (ev);
    return events;
}

void EngineHost::clearIfSilent()
{
    if (state.currentlySoundingNotes.none()
        && state.pendingScheduledNotes.none()
        && state.inputNotes.none()
        && ! sequencerPlaying)
    {
        state.clearGenerated();
        liveGroupId = -1;
        liveInputPitch = -1;
        state.isLiveOverride = false;
    }
}

void EngineHost::processBlock (juce::MidiBuffer& out, int numSamples)
{
    const int64_t blockStart = scheduler.getClock();
    const int64_t blockEnd = blockStart + numSamples;

    // Sequencer transport commands (single-word atomics, consumed here).
    switch (transportCommand.exchange (0, std::memory_order_relaxed))
    {
        case 1:
            sequencerPlaying = true;
            currentSlot = 0;
            nextBarSample = blockEnd; // first chord lands at the next block boundary
            if (sequencerGroupId > 0)
                scheduler.releaseGroup (sequencerGroupId, blockStart);
            break;

        case 2:
            sequencerPlaying = false;
            resumeAtNextBar = false;
            if (sequencerGroupId > 0)
            {
                scheduler.releaseGroup (sequencerGroupId, blockStart);
                sequencerGroupId = -1;
            }
            state.activeProgressionSlot = -1;
            break;

        default:
            break;
    }

    // Performance-mode re-performance (same chord, new mode — spec §112).
    const int requestedMode = pendingMode.exchange (-1, std::memory_order_relaxed);
    if (requestedMode >= 0)
    {
        state.performanceMode = (PerformanceMode) requestedMode;

        if (hasLastRealization && liveGroupId > 0
            && scheduler.groupHasPendingOrSounding (liveGroupId))
        {
            const auto saved = lastRealization;
            const int oldGroup = liveGroupId;
            liveGroupId = -1;                     // force fresh scheduling path
            scheduler.cancelPendingAttacks (oldGroup);
            for (int pitch = 0; pitch < 128; ++pitch)
                if (scheduler.soundingGroup (pitch) == oldGroup)
                    scheduler.releasePitch (pitch, blockStart);

            const Voicing* prev = nullptr; // same voicing, no re-leading
            juce::ignoreUnused (prev);
            scheduleLive (saved, blockStart);
        }
    }

    // MORPH action: re-realize the current degree with the next voicing
    // sibling and re-perform it (chord identity preserved).
    if (morphPending.exchange (false, std::memory_order_relaxed))
    {
        if (hasLastRealization)
        {
            auto varied = realizeDegree (ScaleDegree { lastDegree },
                                         &lastRealization.voicing);

            if (liveGroupId > 0)
            {
                const int oldGroup = liveGroupId;
                liveGroupId = -1;
                scheduler.cancelPendingAttacks (oldGroup);
                for (int pitch = 0; pitch < 128; ++pitch)
                    if (scheduler.soundingGroup (pitch) == oldGroup)
                        scheduler.releasePitch (pitch, blockStart);
                scheduleLive (varied, blockStart);
            }
            else
            {
                lastRealization = varied;
                updateGeneratedState (varied, PerformanceEngine::schedule (
                    varied, buildProfile(), sampleRate, -1,
                    0x9E3779B9u ^ (triggerCounter * 2654435761u)));
            }
        }
    }

    // Sequencer: schedule one chord per bar, looking ahead within this block.
    if (sequencerPlaying)
    {
        const bool overrideActive = liveGroupId > 0 && state.inputNotes.any();

        if (! overrideActive)
            state.isLiveOverride = false;

        const int64_t barLength = (int64_t) (sampleRate * 60.0 / tempoBpm * 4.0);

        while (nextBarSample < blockEnd)
        {
            if (! overrideActive || resumeAtNextBar)
            {
                scheduleSequencerSlot (currentSlot, nextBarSample, barLength);
                resumeAtNextBar = false;
            }
            currentSlot = (currentSlot + 1) % progression.size;
            nextBarSample += barLength;
        }

        const double beatLength = sampleRate * 60.0 / tempoBpm;
        const double posInBar = (double) ((blockEnd - 1 - (nextBarSample - barLength)) % barLength);
        state.currentBeat = posInBar / beatLength;
        state.currentBar = (int) ((blockEnd - 1) / barLength);
    }

    state.isSequencerPlaying = sequencerPlaying;

    scheduler.processBlock (out, numSamples);
    scheduler.rebuildStateBits (state);

    // Strum progress: 0 → 1 while a strum unfolds.
    if (activeGroupNoteCount > 0 && state.pendingScheduledNotes.any())
    {
        const auto elapsed = (double) (scheduler.getClock() - activeGroupOrigin);
        state.strumProgress = (float) juce::jlimit (0.0, 1.0, elapsed / (double) activeSpreadSamples);
    }
    else
    {
        state.strumProgress = state.currentlySoundingNotes.any() ? 1.0f : 0.0f;
    }

    clearIfSilent();
    stateSnapshot.write (state);
}

} // namespace morph
