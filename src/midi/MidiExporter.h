#pragma once

#include "ScheduledNote.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <utility>
#include <vector>

namespace morph
{

/**
 * MidiExporter: renders the exact scheduled performance events into a
 * Standard MIDI File. The exporter consumes the SAME ScheduledNote lists
 * the scheduler plays, so export timing == heard timing (spec §79, §91).
 *
 * Message-thread only (file I/O, allocations).
 */
class MidiExporter
{
public:
    struct ChordEvent
    {
        int64_t startSample = 0;                       // absolute
        ScheduledNoteList<maxChordVoices> notes;       // offsets relative to startSample
    };

    /**
     * @param chords       rendered performances in order
     * @param sampleRate   render sample rate
     * @param bpm          tempo used to map samples → ticks
     * @param defaultLen   note length for open-ended (until-release) notes
     */
    static juce::MidiFile buildMidiFile (const std::vector<ChordEvent>& chords,
                                         double sampleRate, double bpm,
                                         int64_t defaultLengthSamples);

    static bool writeToFile (juce::MidiFile& file, const juce::File& dest);
};

} // namespace morph
