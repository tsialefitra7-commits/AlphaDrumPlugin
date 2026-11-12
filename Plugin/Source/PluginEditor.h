#pragma once

// Assemble l'UI (grille, thème) et le Core (via le processeur). Toute la "colle"
// entre les callbacks de la vue et le modèle se trouve ici.

#include "PluginProcessor.h"

#include "UI/FlStudioTheme.h"
#include "UI/StepMatrixGrid.h"

#include <juce_audio_processors/juce_audio_processors.h>

class AlphaDrumAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit AlphaDrumAudioProcessorEditor (AlphaDrumAudioProcessor&);
    ~AlphaDrumAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshFromModel();
    void showTrackMenu (int track);
    void chooseSampleFor (int track);

    AlphaDrumAudioProcessor& audioProcessor;

    alpha::ui::FlStudioTheme  theme;
    alpha::ui::StepMatrixGrid grid;

    juce::Label      titleLabel, bpmLabel;
    juce::TextButton playButton { "PLAY" }, clearButton { "CLEAR" };
    juce::Slider     bpmSlider;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AlphaDrumAudioProcessorEditor)
};
