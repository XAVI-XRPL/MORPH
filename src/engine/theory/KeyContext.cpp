#include "KeyContext.h"
#include <juce_core/juce_core.h>

namespace morph
{

namespace
{
    // Chromatic order for the header KEY / SCALE arrows.
    constexpr uint8_t minorKeyOrder[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
}

KeyContext keyContextForMinorTonicIndex (int index)
{
    const auto wrapped = ((index % 12) + 12) % 12;
    return KeyContext { PitchClass { minorKeyOrder[wrapped] }, ScaleDefinition::naturalMinor() };
}

int minorTonicIndexOf (const KeyContext& key)
{
    for (int i = 0; i < 12; ++i)
        if (minorKeyOrder[i] == key.tonic.value)
            return i;
    return 0;
}

juce::String keyContextDisplayName (const KeyContext& key)
{
    juce::String name (pitchClassName (key.tonic, key.prefersFlats()));
    name << (key.scale.kind == ScaleKind::naturalMinor ? " Minor" : " Major");
    return name;
}

} // namespace morph
