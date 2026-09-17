#include "Pitch.h"

namespace morph
{

namespace
{
    constexpr const char* sharpNames[12] =
        { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    constexpr const char* flatNames[12] =
        { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
}

const char* pitchClassName (PitchClass pc, bool useFlats)
{
    return useFlats ? flatNames[pc.value] : sharpNames[pc.value];
}

} // namespace morph
