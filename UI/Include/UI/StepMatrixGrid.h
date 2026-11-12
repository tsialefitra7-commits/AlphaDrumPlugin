#pragma once

// La grille complète des pas : noms de pistes à gauche, matrice de pads cliquable.
// Vue pure : elle ne connaît ni le séquenceur ni l'audio, elle expose des callbacks.

#include "UI/FlPadComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

namespace alpha::ui
{
class StepMatrixGrid : public juce::Component
{
public:
    StepMatrixGrid (int numTracks, int numSteps);

    int getNumTracks() const noexcept   { return numTracks; }
    int getNumSteps() const noexcept    { return numSteps; }

    // --- Modèle -> vue ---
    void setStep (int track, int step, bool active);
    bool getStep (int track, int step) const;
    void setTrackName (int track, const juce::String& name);
    void setPlayheadStep (int step);    // -1 = aucune surbrillance

    // --- Vue -> contrôleur ---
    std::function<void (int track, int step, bool newState)> onStepChanged;
    std::function<void (int track)> onTrackLabelClicked;            // clic gauche : pré-écoute
    std::function<void (int track)> onTrackLabelRightClicked;       // clic droit : menu

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    FlPadComponent* padAt (int track, int step) const;
    FlPadComponent* padAtPosition (juce::Point<int>, int& track, int& step) const;
    int labelAtPosition (juce::Point<int>) const;
    juce::Rectangle<int> rowBounds (int track) const;
    void applyDrag (juce::Point<int>);

    const int numTracks, numSteps;
    std::vector<std::unique_ptr<FlPadComponent>> pads;   // [track * numSteps + step]
    juce::StringArray trackNames;
    int  playhead = -1;
    bool dragState = false, dragging = false;
};

} // namespace alpha::ui
