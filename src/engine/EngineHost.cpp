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

    // Strum direction: the setting wins; STRUM↓ with no explicit setting = down.
    {
        const int dirSetting = strumDirectionSetting.load (std::memory_order_relaxed);
        p.strum.direction = (p.mode == PerformanceMode::strumDown
                             && dirSetting == (int) StrumDirection::up)
                                ? StrumDirection::down
                                : (StrumDirection) dirSetting;
    }

    // M8 stream profiles. TEXTURE adds swing/humanization; MOTION widens arp.
    {
        const float texture = textureKnob.load (std::memory_order_relaxed);
        const float motion = motionKnob.load (std::memory_order_relaxed);
        p.pulse.rate = (StreamRate) streamRate.load (std::memory_order_relaxed);
        p.pulse.swing = texture * 0.25f;
        p.pulse.accentEvery = 4;
        p.pulse.bassHold = motion < 0.4f;
        p.arp.rate = p.pulse.rate;
        p.arp.direction = (ArpDirection) arpDirectionSetting.load (std::memory_order_relaxed);
        p.arp.octaves = motion > 0.66f ? 2 : 1;
        p.pattern = (PatternKind) patternKindSetting.load (std::memory_order_relaxed);
    }
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

ChordRealization EngineHost::realizeDegree (ScaleDegree degree, const VoiceLeadingMemory& mem,
                                            int slotIndex) const
{
    const auto key = currentKey();
    const auto style = currentStyle();
    const auto candidate = harmony.chordForDegree (key, degree, style,
                                                   colorKnob.load (std::memory_order_relaxed));

    VoicingContext ctx;
    if (mem.valid)
    {
        ctx.previous = &mem.voicing;
        ctx.previousBass = MidiPitch { mem.bass };
        ctx.hasPreviousBass = true;
        ctx.previousTop = MidiPitch { mem.top };
        ctx.hasPreviousTop = true;
        ctx.previousTopDirection = mem.topDirection;
    }
    ctx.slotIndex = slotIndex;
    ctx.motion = motionKnob.load (std::memory_order_relaxed);
    ctx.openness = spaceKnob.load (std::memory_order_relaxed);
    ctx.tonalCenter = key.tonic;

    return voicingEngine.realize (candidate, key, style, ctx,
                                  morphVariation.load (std::memory_order_relaxed));
}

void EngineHost::buildProgressionPlan (std::array<ChordRealization, 4>& out) const
{
    VoiceLeadingMemory mem;
    const auto& prog = activeProgression();
    for (int i = 0; i < prog.size; ++i)
    {
        out[(size_t) i] = realizeDegree (prog.slots[(size_t) i].degree, mem, i);
        advanceVoiceLeadingMemory (mem, out[(size_t) i]);
    }

    // Loop closure: re-realize slot 0 against slot 3 so the wrap is smooth.
    auto closed = realizeDegree (prog.slots[0].degree, mem, 0);
    if (closed.valid)
        out[0] = closed;
}

uint64_t EngineHost::computePlanSignature() const
{
    auto q = [] (float f) { return (uint64_t) (f * 100.0f); };
    return (uint64_t) (unsigned) keyIndex.load (std::memory_order_relaxed)
         ^ ((uint64_t) (unsigned) styleIndex.load (std::memory_order_relaxed) << 4)
         ^ (q (colorKnob.load (std::memory_order_relaxed)) << 8)
         ^ (q (spaceKnob.load (std::memory_order_relaxed)) << 20)
         ^ (q (motionKnob.load (std::memory_order_relaxed)) << 32)
         ^ ((uint64_t) (morphVariation.load (std::memory_order_relaxed) & 0xFFFF) << 44);
}

void EngineHost::rebuildPlanIfNeeded()
{
    const auto sig = computePlanSignature();
    const bool dirty = planDirty.exchange (false, std::memory_order_relaxed);
    if (! planValid || dirty || sig != planSignature)
    {
        buildProgressionPlan (slotPlan);
        planSignature = sig;
        planValid = true;
    }
}

//==============================================================================
// Composition editing (message thread)

void EngineHost::setProgressionFromUi (const Progression& p)
{
    const int current = progressionIndex.load (std::memory_order_acquire);
    const int next = 1 - current;
    progressionBuffers[(size_t) next] = p;
    progressionIndex.store (next, std::memory_order_release);
    planDirty.store (true, std::memory_order_release);
    progressionVersion.fetch_add (1, std::memory_order_release);
}

Progression EngineHost::getProgressionForUi() const
{
    return activeProgression();
}

void EngineHost::morphProgressionFromUi()
{
    const auto current = activeProgression();
    const uint32_t counterSeed = morphCounter.fetch_add (1, std::memory_order_relaxed) + 1;
    const auto sibling = morphEngine.morphProgression (
        current, morphKnob.load (std::memory_order_relaxed),
        0x4D4F5250u ^ (counterSeed * 2246822519u));
    setProgressionFromUi (sibling);

    // Voicing dimension of the sibling (D13): advance the variation too.
    morphVariation.fetch_add (1, std::memory_order_relaxed);
}

void EngineHost::applyBankEntryFromUi (const BankEntry& entry)
{
    auto p = activeProgression();
    for (int i = 0; i < 4 && i < p.size; ++i)
        if (! p.slots[(size_t) i].locked)
            p.slots[(size_t) i].degree = ScaleDegree { (int) entry.degrees[(size_t) i] };
    setProgressionFromUi (p);
}

void EngineHost::toggleSlotLockFromUi (int slot)
{
    if (slot < 0 || slot >= 4)
        return;
    auto p = activeProgression();
    p.slots[(size_t) slot].locked = ! p.slots[(size_t) slot].locked;
    setProgressionFromUi (p);
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
    // A stream (pulse/pattern/arp) is always stopped before new scheduling.
    if (stream.active)
        stopStream (originSample);

    const auto profile = buildProfile();
    const uint32_t seed = 0x9E3779B9u ^ (triggerCounter * 2654435761u);

    if (isStreamMode (profile.mode))
    {
        startStream (r, originSample, -1, seed);
        liveGroupId = stream.groupId;

        state.strumDirection = StrumDirection::up;
        state.performanceMode = profile.mode;
        updateGeneratedState (r, PerformanceEngine::schedule (r, profile, sampleRate, -1, seed));
        lastRealization = r;
        hasLastRealization = true;
        advanceVoiceLeadingMemory (vlMemory, r);
        ++triggerCounter;
        return;
    }

    auto notes = PerformanceEngine::schedule (r, profile, sampleRate, -1, seed);

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
    advanceVoiceLeadingMemory (vlMemory, r);
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

    auto realization = realizeDegree (result.degree, vlMemory, 0);

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
        if (stream.active)
            stopStream (scheduler.getClock() + offsetWithinBlock);
        else
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
    lastDegree = activeProgression().slots[(size_t) slot].degree.value;
    rebuildPlanIfNeeded();
    const auto realization = slotPlan[(size_t) slot];

    const auto profile = buildProfile();
    const uint32_t slotSeed = 0x51ED270Bu ^ ((uint32_t) slot * 144665u);

    if (isStreamMode (profile.mode))
    {
        if (sequencerGroupId > 0)
            scheduler.releaseGroup (sequencerGroupId, barStart);
        if (stream.active)
            stopStream (barStart);

        startStream (realization, barStart, barStart + barLength, slotSeed);
        sequencerGroupId = stream.groupId;

        auto notes = PerformanceEngine::schedule (realization, profile, sampleRate, -1, slotSeed);
        activeGroupOrigin = barStart;
        activeSpreadSamples = std::max<int64_t> (1, barLength);
        activeGroupNoteCount = notes.count;
        state.activeProgressionSlot = slot;
        state.strumDirection = StrumDirection::up;
        state.performanceMode = profile.mode;
        updateGeneratedState (realization, notes);
        lastRealization = realization;
        hasLastRealization = true;
        advanceVoiceLeadingMemory (vlMemory, realization);
        return;
    }

    auto notes = PerformanceEngine::schedule (realization, profile, sampleRate,
                                              barLength - (int64_t) (0.02 * sampleRate),
                                              slotSeed);

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
    advanceVoiceLeadingMemory (vlMemory, realization);
}

std::vector<MidiExporter::ChordEvent> EngineHost::renderProgressionPerformance()
{
    std::vector<MidiExporter::ChordEvent> events;
    events.reserve ((size_t) activeProgression().size);

    const int64_t barLength = (int64_t) (sampleRate * 60.0 / tempoBpm * 4.0);
    const auto profile = buildProfile();

    std::array<ChordRealization, 4> plan;
    buildProgressionPlan (plan);

    for (int slot = 0; slot < activeProgression().size; ++slot)
    {
        const auto& realization = plan[(size_t) slot];
        const uint32_t slotSeed = 0x51ED270Bu ^ ((uint32_t) slot * 144665u);

        MidiExporter::ChordEvent ev;
        ev.startSample = (int64_t) slot * barLength;

        if (isStreamMode (profile.mode))
        {
            StreamContext ctx;
            ctx.realization = realization;
            ctx.profile = profile;
            ctx.sampleRate = sampleRate;
            ctx.bpm = tempoBpm;
            ctx.seed = slotSeed;
            ctx.streamStartSample = ev.startSample;
            PatternEngine::generateWindow (ctx, ev.startSample, ev.startSample + barLength,
                                           ev.notes);
        }
        else
        {
            ev.notes.assignFrom (PerformanceEngine::schedule (realization, profile, sampleRate,
                                                              barLength - (int64_t) (0.02 * sampleRate),
                                                              slotSeed));
        }
        events.push_back (ev);
    }

    return events;
}

std::vector<MidiExporter::ChordEvent> EngineHost::renderCurrentChordPerformance()
{
    std::vector<MidiExporter::ChordEvent> events;

    if (! hasLastRealization)
        return events;

    const int64_t barLength = (int64_t) (sampleRate * 60.0 / tempoBpm * 4.0);
    const auto profile = buildProfile();

    MidiExporter::ChordEvent ev;
    ev.startSample = 0;

    if (isStreamMode (profile.mode))
    {
        StreamContext ctx;
        ctx.realization = lastRealization;
        ctx.profile = profile;
        ctx.sampleRate = sampleRate;
        ctx.bpm = tempoBpm;
        ctx.seed = 0x9E3779B9u;
        ctx.streamStartSample = 0;
        PatternEngine::generateWindow (ctx, 0, barLength, ev.notes);
    }
    else
    {
        ev.notes.assignFrom (PerformanceEngine::schedule (lastRealization, profile, sampleRate,
                                                          barLength, 0x9E3779B9u));
    }
    events.push_back (ev);
    return events;
}

//==============================================================================
// M8 streams

void EngineHost::scheduleHeldAnchor (int pitch, int vel, int64_t origin, int64_t end, VoiceRole role)
{
    ScheduledNoteList<maxChordVoices> list;
    ScheduledNote n;
    n.pitch = pitch;
    n.velocity = vel;
    n.noteOnSampleOffset = 0;
    n.noteOffSampleOffset = end >= 0 ? end - origin : -1;
    n.role = role;
    list.add (n);
    scheduler.scheduleIntoGroup (stream.groupId, list, origin);
}

void EngineHost::startStream (const ChordRealization& r, int64_t origin, int64_t end, uint32_t seed)
{
    stream.active = true;
    stream.startSample = origin;
    stream.endSample = end;
    stream.generatedUntil = origin;
    stream.seed = seed;
    stream.realization = r;
    stream.profile = buildProfile();
    stream.groupId = scheduler.allocateGroup();

    // Held anchors for bass-hold pulse / top-hold arp.
    if (stream.profile.mode == PerformanceMode::pulse && stream.profile.pulse.bassHold)
        scheduleHeldAnchor (r.bassPitch.value, stream.profile.velocityBaseline - 4,
                            origin, end, VoiceRole::bass);
    if (stream.profile.mode == PerformanceMode::arp && stream.profile.arp.topHold)
        scheduleHeldAnchor (r.topPitch.value, stream.profile.velocityBaseline + 6,
                            origin, end, VoiceRole::top);
}

void EngineHost::pumpStream (int64_t blockEnd)
{
    if (! stream.active)
        return;

    const int64_t clock = scheduler.getClock();
    const int64_t windowStart = std::max (stream.generatedUntil, clock);
    int64_t windowEnd = blockEnd + blockSize; // one-block lookahead

    if (stream.endSample >= 0)
        windowEnd = std::min (windowEnd, stream.endSample);

    if (windowEnd <= windowStart)
    {
        if (stream.endSample >= 0 && windowStart >= stream.endSample)
            stream.active = false;
        return;
    }

    StreamContext ctx;
    ctx.realization = stream.realization;
    ctx.profile = stream.profile;
    ctx.sampleRate = sampleRate;
    ctx.bpm = tempoBpm;
    ctx.seed = stream.seed;
    ctx.streamStartSample = stream.startSample;

    ScheduledNoteList<64> events;
    PatternEngine::generateWindow (ctx, windowStart, windowEnd, events);
    scheduler.scheduleIntoGroup (stream.groupId, events, stream.startSample);

    stream.generatedUntil = windowEnd;
    if (stream.endSample >= 0 && windowEnd >= stream.endSample)
        stream.active = false;
}

void EngineHost::stopStream (int64_t atSample)
{
    if (! stream.active)
        return;
    stream.active = false;
    scheduler.releaseGroup (stream.groupId, atSample);
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

        // A new phrase after full silence starts from the canonical voicing —
        // voice leading only applies within a connected phrase.
        vlMemory = VoiceLeadingMemory {};
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
            if (stream.active)
                stopStream (blockStart);
            if (sequencerGroupId > 0)
                scheduler.releaseGroup (sequencerGroupId, blockStart);
            break;

        case 2:
            sequencerPlaying = false;
            resumeAtNextBar = false;
            if (stream.active)
                stopStream (blockStart);
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
            auto varied = realizeDegree (ScaleDegree { lastDegree }, vlMemory, 0);

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
            currentSlot = (currentSlot + 1) % activeProgression().size;
            nextBarSample += barLength;
        }

        const double beatLength = sampleRate * 60.0 / tempoBpm;
        const double posInBar = (double) ((blockEnd - 1 - (nextBarSample - barLength)) % barLength);
        state.currentBeat = posInBar / beatLength;
        state.currentBar = (int) ((blockEnd - 1) / barLength);
    }

    state.isSequencerPlaying = sequencerPlaying;

    pumpStream (blockEnd);

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
