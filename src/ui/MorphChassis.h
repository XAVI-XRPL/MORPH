#pragma once

#include "design_system/MaterialPainters.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace morph
{

/**
 * MorphChassis: the physical enclosure (§8) + static engraved furniture
 * (bank headings, knob labels, status dots, footer). Children (header,
 * radial field, knobs, action row, keyboard) are laid out in the fixed
 * 1440×900 design space and scaled uniformly by the editor (§111).
 */
class MorphChassis : public juce::Component
{
public:
    static constexpr float designW = 1440.0f;
    static constexpr float designH = 900.0f;

    MorphChassis() { setSize ((int) designW, (int) designH); }

    void paint (juce::Graphics& g) override
    {
        const auto chassisRect = juce::Rectangle<float> (8.0f, 8.0f, designW - 16.0f, designH - 16.0f);
        materials::drawChassisShadow (g, chassisRect, 44.0f);
        materials::drawChassisSurface (g, chassisRect, 44.0f);

        // Recessed control banks.
        materials::drawRecessedWell (g, { 52, 168, 416, 392 }, 22.0f);
        materials::drawRecessedWell (g, { 972, 168, 416, 392 }, 22.0f);

        // Bank headings with hairlines.
        auto heading = [&g] (const char* text, juce::Rectangle<float> r)
        {
            materials::drawEngravedLabel (g, text, r, 12.0f);
            g.setColour (MorphTheme::hairline.withAlpha (0.9f));
            cg_drawLine (g, r.getX() + 14.0f, r.getCentreY(), r.getCentreX() - 108.0f, r.getCentreY());
            cg_drawLine (g, r.getCentreX() + 108.0f, r.getCentreY(), r.getRight() - 14.0f, r.getCentreY());
        };
        heading ("TONE & MOVEMENT", { 52, 190, 416, 20 });
        heading ("SPACE & OUTPUT", { 972, 190, 416, 20 });

        const BankLabel leftLabels[3] =
        {
            { 116, "COLOR", "MOOD", false },
            { 260, "MOTION", "RHYTHM", false },
            { 404, "MORPH", "VARIATION", true }
        };
        const BankLabel rightLabels[3] =
        {
            { 1036, "SPACE", "AMBIENCE", false },
            { 1180, "TEXTURE", "CHARACTER", false },
            { 1324, "OUTPUT", "LEVEL", true }
        };

        for (const auto& bl : leftLabels)
            paintKnobFurniture (g, bl);
        for (const auto& bl : rightLabels)
            paintKnobFurniture (g, bl);

        // Right bank footer motif.
        materials::drawEngravedLabel (g, "MIDI HARMONY ENGINE", { 1020, 516, 250, 16 }, 9.5f);
        {
            juce::Path wave;
            wave.startNewSubPath (1316.0f, 524.0f);
            wave.cubicTo (1326.0f, 500.0f, 1336.0f, 548.0f, 1346.0f, 524.0f);
            wave.cubicTo (1352.0f, 508.0f, 1356.0f, 512.0f, 1362.0f, 524.0f);
            g.setColour (MorphTheme::textSecondary.withAlpha (0.8f));
            g.strokePath (wave, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        // Footer.
        materials::drawEngravedLabel (g, "ALWAYS A NEXT CHORD", { 80, 866, 300, 16 }, 9.5f,
                                      MorphTheme::textSecondary, juce::Justification::centredLeft);
        materials::drawEngravedLabel (g, "MORPH", { 1080, 866, 240, 16 }, 9.5f,
                                      MorphTheme::textSecondary, juce::Justification::centredRight);
        g.setColour (MorphTheme::hairline);
        cg_drawLine (g, 1330.0f, 874.0f, 1370.0f, 874.0f);
    }

private:
    struct BankLabel { float x; const char* name; const char* sub; bool lit; };

    static void cg_drawLine (juce::Graphics& g, float x1, float y1, float x2, float y2)
    {
        g.drawLine (x1, y1, x2, y2, 1.0f);
    }

    static void paintKnobFurniture (juce::Graphics& g, const BankLabel& bl)
    {
        materials::drawLed (g, { bl.x, 268.0f }, 3.4f,
                            bl.lit ? MorphTheme::accentOrange : MorphTheme::textSecondary, bl.lit);
        materials::drawEngravedLabel (g, bl.name, { bl.x - 60, 452, 120, 18 }, 14.0f,
                                      MorphTheme::textPrimary);
        materials::drawEngravedLabel (g, bl.sub, { bl.x - 60, 474, 120, 14 }, 9.5f);
    }
};

} // namespace morph
