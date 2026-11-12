#pragma once

// Adaptateur DAW <-> Core : reçoit le MIDI et le transport de l'hôte, pilote le
// séquenceur et le sampler du Core, sauvegarde/restaure l'état.

#include "Core/DrumKitModel.h"
#include "Core/SamplerEngine.h"
#include "Core/SequencerState.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <cstdint>

class AlphaDrumAudioProcessor : public juce::AudioProcessor
{
public:
    AlphaDrumAudioProcessor();
    ~AlphaDrumAudioProcessor() override = default;

    // --- juce::AudioProcessor ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                         { return true; }

    const juce::String getName() const override             { return "AlphaDrum"; }
    bool acceptsMidi() const override                       { return true; }
    bool producesMidi() const override                      { return false; }
    bool isMidiEffect() const override                      { return false; }
    double getTailLengthSeconds() const override            { return 2.0; }

    int getNumPrograms() override                           { return 1; }
    int getCurrentProgram() override                        { return 0; }
    void setCurrentProgram (int) override                   {}
    const juce::String getProgramName (int) override        { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- API pour l'éditeur (thread message) ---
    alpha::SequencerState& getSequencer() noexcept          { return sequencer; }
    const alpha::DrumKit& getKit() const noexcept           { return kit; }

    /** Le séquenceur tourne-t-il (lecture interne ou transport de l'hôte) ? */
    bool isRunning() const noexcept                         { return running.load (std::memory_order_relaxed); }
    bool isHostPlaying() const noexcept                     { return hostPlaying.load (std::memory_order_relaxed); }

    /** Joue une piste une fois (pré-écoute depuis l'UI). */
    void previewTrack (int track) noexcept;

    /** Charge un fichier audio (wav, aiff, flac, ogg, mp3...) sur une piste. */
    bool loadSampleFile (int track, const juce::File&);

    /** Remet le son synthétisé par défaut de la piste. */
    void resetTrackSample (int track);

private:
    alpha::SequencerState sequencer;
    alpha::SamplerEngine  sampler;
    alpha::DrumKit        kit;

    juce::AudioFormatManager formatManager;
    juce::SpinLock samplerLock;                 // protège les échantillons du sampler

    std::atomic<std::uint32_t> pendingPreviews { 0 }; // un bit par piste
    std::atomic<bool> running { false }, hostPlaying { false };
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AlphaDrumAudioProcessor)
};
