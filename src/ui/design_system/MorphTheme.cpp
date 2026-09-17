#include "MorphTheme.h"
#include "../../engine/voicing/Voicing.h"

namespace morph
{

juce::Colour MorphTheme::roleColour (int8_t role, bool glow)
{
    switch ((VoiceRole) role)
    {
        case VoiceRole::bass:  return glow ? bassBlueGlow : bassBlue;
        case VoiceRole::top:   return glow ? topYellowGlow : topYellow;
        case VoiceRole::inner:
        default:               return glow ? innerOrangeGlow : innerOrange;
    }
}

juce::Font MorphTheme::labelFont (float size, bool bold)
{
    juce::Font f { juce::FontOptions (size) };
    if (bold)
        f.setBold (true);
    return f;
}

juce::Font MorphTheme::valueFont (float size)
{
    return juce::Font { juce::FontOptions (size) };
}

} // namespace morph
