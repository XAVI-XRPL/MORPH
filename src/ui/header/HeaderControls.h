#pragma once

#include "../design_system/MaterialPainters.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace morph
{

/** Physical segmented pill: [ < | LABEL / value | > ] (§11, §12). */
class SegmentedPillControl : public juce::Component
{
public:
    std::function<void()> onPrevious, onNext, onCenter;

    void setLabel (const juce::String& l) { label = l; repaint(); }
    void setValue (const juce::String& v) { value = v; repaint(); }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    enum class Zone { none, previous, center, next };
    Zone zoneAt (float x) const;

    juce::String label, value;
    Zone pressedZone = Zone::none;
    Zone hoverZone = Zone::none;
};

/** Performance selector pill: [ TOGETHER ▾ ] (§13). */
class PerformancePillControl : public juce::Component
{
public:
    std::function<void()> onClick;

    void setValue (const juce::String& v) { value = v; repaint(); }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { pressed = true; repaint(); }
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    juce::String value;
    bool pressed = false, hover = false;
};

/** MORE pill: [ MORE | ••• ]. */
class MorePillControl : public juce::Component
{
public:
    std::function<void()> onClick;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { pressed = true; repaint(); }
    void mouseUp (const juce::MouseEvent&) override;

private:
    bool pressed = false;
};

} // namespace morph
