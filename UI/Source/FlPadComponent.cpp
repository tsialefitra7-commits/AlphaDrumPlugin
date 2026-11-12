#include "UI/FlPadComponent.h"
#include "UI/FlStudioTheme.h"

namespace alpha::ui
{
FlPadComponent::FlPadComponent()
    : off (Colours::padGreyOff), on (Colours::padGreyOn)
{
    // Les clics sont gérés par la grille parente (pour permettre le "peindre en glissant").
    setInterceptsMouseClicks (false, false);
}

void FlPadComponent::setActive (bool shouldBeActive)
{
    if (active != shouldBeActive)
    {
        active = shouldBeActive;
        repaint();
    }
}

void FlPadComponent::setPlaying (bool shouldBePlaying)
{
    if (playing != shouldBePlaying)
    {
        playing = shouldBePlaying;
        repaint();
    }
}

void FlPadComponent::setColours (juce::Colour offColour, juce::Colour onColour)
{
    off = offColour;
    on = onColour;
    repaint();
}

void FlPadComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    auto fill = active ? on : off;
    if (playing)
        fill = active ? Colours::playhead : fill.brighter (0.25f);

    g.setGradientFill (juce::ColourGradient (fill.brighter (0.12f), 0.0f, bounds.getY(),
                                             fill.darker (0.12f), 0.0f, bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, Metrics::cornerRadius);

    // Petite "LED" en haut du pad, comme dans le Channel Rack.
    auto led = bounds.reduced (bounds.getWidth() * 0.25f, 0.0f).withHeight (3.0f).translated (0.0f, 3.0f);
    g.setColour (active ? Colours::accent : Colours::outline.withAlpha (0.35f));
    g.fillRoundedRectangle (led, 1.5f);

    g.setColour (Colours::outline);
    g.drawRoundedRectangle (bounds, Metrics::cornerRadius, 1.0f);
}

} // namespace alpha::ui
