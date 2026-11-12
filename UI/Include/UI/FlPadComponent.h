#pragma once

// Un pad/step individuel "stupide" : il affiche un état, c'est tout. Les clics
// sont gérés par la grille parente (StepMatrixGrid).
// Aucune logique audio ni connaissance du séquenceur.

#include <juce_gui_basics/juce_gui_basics.h>

namespace alpha::ui
{
class FlPadComponent : public juce::Component
{
public:
    FlPadComponent();

    void setActive (bool shouldBeActive);
    bool isActive() const noexcept              { return active; }

    /** Surbrillance quand la tête de lecture passe sur ce pad. */
    void setPlaying (bool shouldBePlaying);

    void setColours (juce::Colour offColour, juce::Colour onColour);

    void paint (juce::Graphics&) override;

private:
    bool active = false, playing = false;
    juce::Colour off, on;
};

} // namespace alpha::ui
