#include "StyleProfile.h"

namespace morph
{

StyleProfile StyleProfile::modernRnB()
{
    StyleProfile p;
    p.id = StyleId::modernRnB;
    p.recommendedBassPolicy = BassStrumPolicy::anchorFirst;
    p.recommendedStrumSpreadMs = 42.0f;
    p.preferredBassOctave = 2;
    p.preferNinthChords = true;
    return p;
}

} // namespace morph
