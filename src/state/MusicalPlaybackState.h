#pragma once

#include "../engine/theory/Chord.h"
#include "../engine/voicing/Voicing.h"
#include "../engine/performance/PerformanceTypes.h"
#include <array>
#include <atomic>
#include <bitset>
#include <cstdint>

namespace morph
{

/**
 * The one authoritative musical playback state (spec §41).
 * Written only by the audio thread; read by the UI via PlaybackStateBuffer.
 *
 * Core truth invariant (§42): at any playback moment,
 *   actual sounding MIDI == currentlySoundingNotes == keyboard-lit notes.
 */
struct MusicalPlaybackState
{
    std::bitset<128> inputNotes;             // physically held trigger notes
    std::bitset<128> generatedChordNotes;    // full intended realization
    std::bitset<128> currentlySoundingNotes; // actually attacked and sounding
    std::bitset<128> pendingScheduledNotes;  // scheduled, not yet attacked

    std::array<int8_t, 128> generatedRoles;  // VoiceRole per pitch, -1 = none
    std::array<uint8_t, 128> velocityData;

    ChordSymbol activeChord {};
    bool hasActiveChord = false;
    RomanFunction activeRomanFunction {};
    int activeProgressionSlot = -1;

    int bassPitch = -1;
    int topPitch = -1;
    std::array<int, Voicing::maxVoices> innerVoices {};
    int innerVoiceCount = 0;

    PerformanceMode performanceMode = PerformanceMode::together;
    StrumDirection strumDirection = StrumDirection::up;
    float strumProgress = 0.0f;              // 0..1 while a strum unfolds

    double currentBeat = 0.0;
    int currentBar = 0;

    bool isSequencerPlaying = false;
    bool isLiveOverride = false;
    bool sustainState = false;

    MusicalPlaybackState()
    {
        clearGenerated();
    }

    void clearGenerated()
    {
        generatedChordNotes.reset();
        pendingScheduledNotes.reset();
        generatedRoles.fill (-1);
        velocityData.fill (0);
        hasActiveChord = false;
        activeProgressionSlot = -1;
        bassPitch = topPitch = -1;
        innerVoices.fill (-1);
        innerVoiceCount = 0;
        strumProgress = 0.0f;
    }
};

/**
 * Lock-free single-writer / single-reader snapshot (seqlock).
 * Audio thread writes; UI thread reads a consistent copy.
 */
class PlaybackStateBuffer
{
public:
    void write (const MusicalPlaybackState& s) // audio thread only
    {
        sequence.fetch_add (1, std::memory_order_release); // odd: writing
        slot = s;
        sequence.fetch_add (1, std::memory_order_release); // even: stable
    }

    MusicalPlaybackState read() const // UI thread only
    {
        for (;;)
        {
            const auto s0 = sequence.load (std::memory_order_acquire);
            if (s0 & 1)
                continue;
            auto copy = slot;
            if (sequence.load (std::memory_order_acquire) == s0)
                return copy;
        }
    }

private:
    MusicalPlaybackState slot;
    std::atomic<uint64_t> sequence { 0 };
};

} // namespace morph
