#include "PluginEditor.h"

using alpha::ui::Colours::accent;

AlphaDrumAudioProcessorEditor::AlphaDrumAudioProcessorEditor (AlphaDrumAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      grid (alpha::kNumTracks, alpha::kNumSteps)
{
    setLookAndFeel (&theme);

    titleLabel.setText ("ALPHA DRUM", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, accent);
    addAndMakeVisible (titleLabel);

    playButton.setClickingTogglesState (true);
    playButton.onClick = [this]
    {
        audioProcessor.getSequencer().setPlaying (playButton.getToggleState());
    };
    addAndMakeVisible (playButton);

    clearButton.onClick = [this]
    {
        audioProcessor.getSequencer().clear();
        refreshFromModel();
    };
    addAndMakeVisible (clearButton);

    bpmLabel.setText ("BPM", juce::dontSendNotification);
    bpmLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (bpmLabel);

    bpmSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 22);
    bpmSlider.setRange (alpha::SequencerState::kMinBpm, alpha::SequencerState::kMaxBpm, 1.0);
    bpmSlider.onValueChange = [this]
    {
        audioProcessor.getSequencer().setBpm (bpmSlider.getValue());
    };
    addAndMakeVisible (bpmSlider);

    // Vue -> modèle
    grid.onStepChanged = [this] (int track, int step, bool on)
    {
        audioProcessor.getSequencer().setStep (track, step, on);
    };
    grid.onTrackLabelClicked      = [this] (int track) { audioProcessor.previewTrack (track); };
    grid.onTrackLabelRightClicked = [this] (int track) { showTrackMenu (track); };
    addAndMakeVisible (grid);

    refreshFromModel();
    setSize (780, 370);
    startTimerHz (30);
}

AlphaDrumAudioProcessorEditor::~AlphaDrumAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AlphaDrumAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (alpha::ui::Colours::background);

    g.setColour (alpha::ui::Colours::panelDark);
    g.fillRect (getLocalBounds().removeFromTop (48));
    g.setColour (alpha::ui::Colours::outline);
    g.drawHorizontalLine (48, 0.0f, (float) getWidth());
}

void AlphaDrumAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    auto header = area.removeFromTop (48).reduced (10, 9);
    titleLabel.setBounds (header.removeFromLeft (150));
    playButton.setBounds (header.removeFromLeft (70));
    header.removeFromLeft (8);
    clearButton.setBounds (header.removeFromLeft (70));
    bpmSlider.setBounds (header.removeFromRight (240));
    bpmLabel.setBounds (header.removeFromRight (44));

    grid.setBounds (area.reduced (10));
}

// Modèle -> vue : appelé régulièrement, la vue ne fait que refléter l'état du Core.
void AlphaDrumAudioProcessorEditor::refreshFromModel()
{
    auto& sequencer = audioProcessor.getSequencer();
    const auto& kit = audioProcessor.getKit();

    for (int t = 0; t < alpha::kNumTracks; ++t)
    {
        grid.setTrackName (t, juce::String (kit.tracks[(size_t) t].name));

        for (int s = 0; s < alpha::kNumSteps; ++s)
            grid.setStep (t, s, sequencer.getStep (t, s));
    }

    if (! bpmSlider.isMouseButtonDown())
        bpmSlider.setValue (sequencer.getBpm(), juce::dontSendNotification);

    // Quand l'hôte joue, c'est lui qui commande le transport et le tempo.
    const bool hostPlaying = audioProcessor.isHostPlaying();
    playButton.setEnabled (! hostPlaying);
    bpmSlider.setEnabled (! hostPlaying);
    playButton.setToggleState (hostPlaying || sequencer.isPlaying(), juce::dontSendNotification);
    playButton.setButtonText (playButton.getToggleState() ? "STOP" : "PLAY");

    grid.setPlayheadStep (audioProcessor.isRunning() ? sequencer.getCurrentStep() : -1);
}

void AlphaDrumAudioProcessorEditor::timerCallback()
{
    refreshFromModel();
}

void AlphaDrumAudioProcessorEditor::showTrackMenu (int track)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Charger un sample...");
    menu.addItem (2, "Son par defaut");

    juce::Component::SafePointer<AlphaDrumAudioProcessorEditor> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&grid),
                        [safeThis, track] (int result)
                        {
                            if (safeThis == nullptr)
                                return;
                            if (result == 1) safeThis->chooseSampleFor (track);
                            if (result == 2) safeThis->audioProcessor.resetTrackSample (track);
                        });
}

void AlphaDrumAudioProcessorEditor::chooseSampleFor (int track)
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choisir un sample", juce::File(),
                                                       "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");

    juce::Component::SafePointer<AlphaDrumAudioProcessorEditor> safeThis (this);
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safeThis, track] (const juce::FileChooser& chooser)
                              {
                                  const auto file = chooser.getResult();
                                  if (safeThis == nullptr || file == juce::File())
                                      return;

                                  if (safeThis->audioProcessor.loadSampleFile (track, file))
                                      safeThis->audioProcessor.previewTrack (track);
                                  else
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                                              "AlphaDrum",
                                                                              "Impossible de lire ce fichier audio.");
                              });
}
