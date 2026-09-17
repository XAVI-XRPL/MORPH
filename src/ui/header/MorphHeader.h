#pragma once

#include "HeaderControls.h"
#include "../design_system/MaterialPainters.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace morph
{

/**
 * Canonical header (§10): MORPH wordmark + status dot + descriptor,
 * KEY/SCALE pill, FEEL pill, PERFORMANCE pill, MORE pill, status motif.
 */
class MorphHeader : public juce::Component
{
public:
    MorphHeader();

    std::function<void()> onKeyPrevious, onKeyNext, onKeyCenter;
    std::function<void()> onFeelPrevious, onFeelNext, onFeelCenter;
    std::function<void()> onPerformanceClick, onMoreClick;

    void setKeyValue (const juce::String& v)   { keyScale.setValue (v); }
    void setFeelValue (const juce::String& v)  { feel.setValue (v); }
    void setPerformanceValue (const juce::String& v) { performance.setValue (v); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SegmentedPillControl keyScale, feel;
    PerformancePillControl performance;
    MorePillControl more;
};

} // namespace morph
