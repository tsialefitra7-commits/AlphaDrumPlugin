#pragma once

// Moteur DSP : stocke un échantillon par piste et mixe des voix polyphoniques.
// Aucune dépendance JUCE : le décodage des fichiers audio est fait par la couche Plugin,
// qui fournit ici des buffers bruts.

#include "Core/DrumKitModel.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <vector>

namespace alpha
{
/** Échantillon décodé en mémoire (mono si right est vide). */
struct SampleData
{
    std::vector<float> left, right;
    double sampleRate = 44100.0;

    std::size_t length() const noexcept { return left.size(); }
    bool isStereo() const noexcept      { return right.size() == left.size() && ! left.empty(); }
};

class SamplerEngine
{
public:
    static constexpr int kMaxVoices = 32;

    SamplerEngine();

    /** À appeler avant le rendu (et à chaque changement de fréquence d'échantillonnage). */
    void prepare (double sampleRate);

    /** Coupe toutes les voix. Thread audio (ou audio arrêté). */
    void reset() noexcept;

    // --- Échantillons : NON temps réel. L'appelant doit garantir que render()/trigger()
    //     ne tournent pas en même temps (verrou côté Plugin). ---
    void setSample (int track, SampleData sample);
    void loadBuiltInSample (int track);   // son synthétisé par défaut de la piste
    void loadBuiltInKit();
    bool hasSample (int track) const noexcept;

    // --- Réglages de piste : tous threads (atomiques) ---
    void setTrackGain  (int track, float linearGain) noexcept;
    void setTrackPan   (int track, float pan) noexcept;          // -1..+1
    void setTrackPitch (int track, float semitones) noexcept;
    void setTrackMute  (int track, bool mute) noexcept;
    void applyKit (const DrumKit&) noexcept;

    // --- Thread audio ---
    /** Déclenche une piste. sampleOffset = retard dans le prochain bloc rendu. */
    void trigger (int track, float velocity = 1.0f, int sampleOffset = 0) noexcept;

    /** Ajoute le mix des voix dans left/right (right peut être nullptr = mono). */
    void render (float* left, float* right, int numSamples) noexcept;

    int getNumActiveVoices() const noexcept;

private:
    struct Voice
    {
        bool   active = false;
        int    track = 0;
        int    delay = 0;       // échantillons de sortie avant le début
        double position = 0.0;  // position de lecture dans la source
        double rate = 1.0;
        float  gainL = 1.0f, gainR = 1.0f;
        unsigned age = 0;
    };

    struct TrackParams
    {
        std::atomic<float> gain { 1.0f }, pan { 0.0f }, pitch { 0.0f };
        std::atomic<bool>  mute { false };
    };

    static bool validTrack (int t) noexcept { return t >= 0 && t < kNumTracks; }

    std::array<SampleData, kNumTracks>  samples;
    std::array<TrackParams, kNumTracks> params;
    std::array<Voice, kMaxVoices>       voices;
    double   sampleRate = 44100.0;
    unsigned ageCounter = 0;
};

} // namespace alpha
