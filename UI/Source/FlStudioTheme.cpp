#include "UI/FlStudioTheme.h"

namespace alpha::ui
{
FlStudioTheme::FlStudioTheme()
{
    setColour (juce::ResizableWindow::backgroundColourId, Colours::background);
    setColour (juce::DocumentWindow::backgroundColourId,  Colours::background);

    setColour (juce::Label::textColourId, Colours::text);

    setColour (juce::TextButton::buttonColourId,   Colours::panel);
    setColour (juce::TextButton::buttonOnColourId, Colours::accent);
    setColour (juce::TextButton::textColourOffId,  Colours::text);
    setColour (juce::TextButton::textColourOnId,   Colours::outline);

    setColour (juce::Slider::backgroundColourId,          Colours::panelDark);
    setColour (juce::Slider::trackColourId,               Colours::accent);
    setColour (juce::Slider::thumbColourId,               Colours::text);
    setColour (juce::Slider::textBoxTextColourId,         Colours::text);
    setColour (juce::Slider::textBoxBackgroundColourId,   Colours::panelDark);
    setColour (juce::Slider::textBoxOutlineColourId,      Colours::outline);

    setColour (juce::PopupMenu::backgroundColourId,            Colours::panel);
    setColour (juce::PopupMenu::textColourId,                  Colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::accent);
    setColour (juce::PopupMenu::highlightedTextColourId,       Colours::outline);
}

juce::Font FlStudioTheme::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jmin (14.0f, (float) buttonHeight * 0.6f), juce::Font::bold));
}

void FlStudioTheme::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                          bool isHighlighted, bool isDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

    auto fill = backgroundColour;
    if (isDown)             fill = fill.darker (0.2f);
    else if (isHighlighted) fill = fill.brighter (0.1f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, Metrics::cornerRadius);
    g.setColour (Colours::outline);
    g.drawRoundedRectangle (bounds, Metrics::cornerRadius, 1.0f);
}

void FlStudioTheme::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float minSliderPos, float maxSliderPos,
                                      juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const auto track = juce::Rectangle<float> ((float) x, (float) y + (float) height * 0.5f - 3.0f, (float) width, 6.0f);

    g.setColour (slider.findColour (juce::Slider::backgroundColourId));
    g.fillRoundedRectangle (track, 3.0f);
    g.setColour (slider.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (track.withRight (sliderPos), 3.0f);

    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillRoundedRectangle (juce::Rectangle<float> (6.0f, 16.0f).withCentre ({ sliderPos, track.getCentreY() }), 2.0f);
}

} // namespace alpha::ui
