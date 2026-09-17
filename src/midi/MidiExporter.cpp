#include "MidiExporter.h"

namespace morph
{

juce::MidiFile MidiExporter::buildMidiFile (const std::vector<ChordEvent>& chords,
                                            double sampleRate, double bpm,
                                            int64_t defaultLengthSamples)
{
    constexpr int ticksPerQuarter = 960;
    const double samplesPerQuarter = sampleRate * 60.0 / bpm;

    auto toTick = [&] (int64_t sample) -> int
    {
        return (int) ((double) sample / samplesPerQuarter * (double) ticksPerQuarter);
    };

    juce::MidiMessageSequence track;

    for (const auto& chord : chords)
    {
        for (int i = 0; i < chord.notes.count; ++i)
        {
            const auto& n = chord.notes.notes[(size_t) i];

            const int64_t onSample = chord.startSample + n.noteOnSampleOffset;
            const int64_t offSample = chord.startSample
                + (n.noteOffSampleOffset >= 0 ? n.noteOffSampleOffset
                                              : defaultLengthSamples);

            track.addEvent (juce::MidiMessage::noteOn (1, (uint8_t) n.pitch,
                                                       (float) n.velocity / 127.0f),
                            (double) toTick (onSample));
            track.addEvent (juce::MidiMessage::noteOff (1, (uint8_t) n.pitch),
                            (double) toTick (std::max (offSample, onSample + 1)));
        }
    }

    track.updateMatchedPairs();

    juce::MidiFile file;
    file.setTicksPerQuarterNote (ticksPerQuarter);
    file.addTrack (track);
    return file;
}

bool MidiExporter::writeToFile (juce::MidiFile& file, const juce::File& dest)
{
    dest.deleteFile();
    juce::FileOutputStream stream (dest);
    if (! stream.openedOk())
        return false;
    stream.setPosition (0);
    stream.truncate();
    return file.writeTo (stream);
}

} // namespace morph
