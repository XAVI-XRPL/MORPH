#include "MorphHeader.h"

namespace morph
{

MorphHeader::MorphHeader()
{
    keyScale.setLabel ("KEY / SCALE");
    keyScale.onPrevious = [this] { if (onKeyPrevious) onKeyPrevious(); };
    keyScale.onNext = [this] { if (onKeyNext) onKeyNext(); };
    keyScale.onCenter = [this] { if (onKeyCenter) onKeyCenter(); };
    addAndMakeVisible (keyScale);

    feel.setLabel ("FEEL");
    feel.onPrevious = [this] { if (onFeelPrevious) onFeelPrevious(); };
    feel.onNext = [this] { if (onFeelNext) onFeelNext(); };
    feel.onCenter = [this] { if (onFeelCenter) onFeelCenter(); };
    addAndMakeVisible (feel);

    performance.setValue ("TOGETHER");
    performance.onClick = [this] { if (onPerformanceClick) onPerformanceClick(); };
    addAndMakeVisible (performance);

    more.onClick = [this] { if (onMoreClick) onMoreClick(); };
    addAndMakeVisible (more);
}

void MorphHeader::paint (juce::Graphics& g)
{
    // Wordmark.
    auto wm = juce::Rectangle<float> (0.0f, 0.0f, 190.0f, (float) getHeight());
    auto font = MorphTheme::labelFont (32.0f, true);
    font.setExtraKerningFactor (0.16f);
    g.setFont (font);
    g.setColour (MorphTheme::textPrimary);
    g.drawText ("MORPH", wm, juce::Justification::centredLeft, false);

    // Status dot right of the wordmark.
    materials::drawLed (g, { 178.0f, (float) getHeight() * 0.44f }, 5.0f,
                        MorphTheme::accentOrange, true);

    // Descriptor: GENERATIVE / HARMONY / FOR MODERN MUSIC.
    auto desc = juce::Rectangle<float> (210.0f, 12.0f, 200.0f, (float) getHeight() - 20.0f);
    const juce::String lines[3] = { "GENERATIVE", "HARMONY", "FOR MODERN MUSIC" };
    const float lh = desc.getHeight() / 3.0f;
    for (int i = 0; i < 3; ++i)
        materials::drawEngravedLabel (g, lines[i],
                                      desc.withY (desc.getY() + lh * (float) i).withHeight (lh),
                                      7.5f, MorphTheme::textSecondary, juce::Justification::centredLeft);

    // Right status motif: three dots + IDEAS / FLOW / FURTHER.
    const float mx = (float) getWidth() - 96.0f;
    const juce::String mlines[3] = { "IDEAS", "FLOW", "FURTHER" };
    for (int i = 0; i < 3; ++i)
        materials::drawEngravedLabel (g, mlines[i],
                                      { mx, (float) getHeight() * 0.28f + 11.0f * (float) i, 92.0f, 11.0f },
                                      7.5f, MorphTheme::textSecondary, juce::Justification::centredRight);

    const juce::Colour dotCols[3] = { MorphTheme::accentOrange, MorphTheme::topYellow,
                                      MorphTheme::textSecondary };
    for (int i = 0; i < 3; ++i)
        materials::drawLed (g, { (float) getWidth() - 44.0f + (float) i * 13.0f, 12.0f },
                            3.4f, dotCols[i], i < 2);
}

void MorphHeader::resized()
{
    auto area = getLocalBounds();
    area.removeFromLeft (440);   // wordmark + descriptor
    area.removeFromRight (110);  // motif

    auto right = area.removeFromRight (120);
    more.setBounds (right.reduced (4, 8));

    area.removeFromRight (12);
    auto perf = area.removeFromRight (150);
    performance.setBounds (perf.reduced (4, 8));

    area.removeFromRight (12);
    auto feelArea = area.removeFromRight (240);
    feel.setBounds (feelArea.reduced (4, 8));

    area.removeFromRight (12);
    auto keyArea = area.removeFromRight (240);
    keyScale.setBounds (keyArea.reduced (4, 8));
}

} // namespace morph
