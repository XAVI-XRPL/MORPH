#pragma once

#include "../theory/Chord.h"
#include <array>

namespace morph
{

/** Semantic role of a voice — drives color, lighting, and export stems. */
enum class VoiceRole : uint8_t
{
    bass = 0,
    inner,
    top
};

struct Voice
{
    MidiPitch pitch { 60 };
    VoiceRole role = VoiceRole::inner;
};

/** A realized set of voices, sorted ascending by pitch. */
struct Voicing
{
    static constexpr int maxVoices = 8;

    std::array<Voice, maxVoices> voices {};
    int count = 0;

    void add (Voice v)
    {
        if (count < maxVoices)
            voices[(size_t) count++] = v;
    }

    void sortAscending()
    {
        for (int i = 1; i < count; ++i)
        {
            auto key = voices[(size_t) i];
            int j = i - 1;
            while (j >= 0 && voices[(size_t) j].pitch.value > key.pitch.value)
            {
                voices[(size_t) (j + 1)] = voices[(size_t) j];
                --j;
            }
            voices[(size_t) (j + 1)] = key;
        }
    }

    MidiPitch lowest() const { return count > 0 ? voices[0].pitch : MidiPitch { -1 }; }
    MidiPitch highest() const { return count > 0 ? voices[(size_t) (count - 1)].pitch : MidiPitch { -1 }; }

    bool containsPitch (int midiNote) const
    {
        for (int i = 0; i < count; ++i)
            if (voices[(size_t) i].pitch.value == midiNote)
                return true;
        return false;
    }
};

/** CHORD identity + VOICING realization. These layers never collapse. */
struct ChordRealization
{
    ChordCandidate candidate;
    Voicing voicing;
    MidiPitch bassPitch { -1 };
    MidiPitch topPitch { -1 };
    bool valid = false;
};

} // namespace morph
