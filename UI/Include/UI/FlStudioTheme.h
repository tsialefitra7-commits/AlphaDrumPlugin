#pragma once

// Design System : palette inspirée de FL Studio + LookAndFeel global.

#include <juce_gui_basics/juce_gui_basics.h>

namespace alpha::ui
{
namespace Colours
{
    const juce::Colour background   { 0xff2b3035 };
    const juce::Colour panel        { 0xff363c42 };
    const juce::Colour panelDark    { 0xff22262a };
    const juce::Colour outline      { 0xff15181b };
    const juce::Colour text         { 0xffd5dbe0 };
    const juce::Colour textDim      { 0xff8b949c };
    const juce::Colour accent       { 0xffff9a2e }; // orange FL
    const juce::Colour playhead     { 0xff9be36a }; // vert de lecture

    // Pads : les groupes de 4 pas alternent gris / rouge, comme le Channel Rack.
    const juce::Colour padGreyOff   { 0xff525a61 };
    const juce::Colour padGreyOn    { 0xffd9e0e6 };
    const juce::Colour padRedOff    { 0xff6a4a4d };
    const juce::Colour padRedOn     { 0xfff2c9c5 };
}

namespace Metrics
{
    constexpr int   padGap        = 3;
    constexpr int   groupGap      = 8;   // espace supplémentaire entre groupes de 4 pas
    constexpr int   labelWidth    = 110;
    constexpr float cornerRadius  = 3.0f;
}

class FlStudioTheme : public juce::LookAndFeel_V4
{
public:
    FlStudioTheme();

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
};

} // namespace alpha::ui
