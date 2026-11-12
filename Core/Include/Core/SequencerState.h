#pragma once

// Logique pure du séquenceur à pas : grille, tempo, tête de lecture.
// Les données partagées sont atomiques : l'UI peut lire/écrire pendant que le
// thread audio appelle process(), sans verrou.

#include "Core/DrumKitModel.h"

#include <array>
#include <atomic>

namespace alpha
{
class SequencerState
{
public:
    static constexpr double kMinBpm = 40.0;
    static constexpr double kMaxBpm = 300.0;
    static constexpr int kStepsPerBeat = 4; // doubles croches

    SequencerState();

    // --- Grille (tous threads) ---
    bool getStep (int track, int step) const noexcept;
    void setStep (int track, int step, bool on) noexcept;
    bool toggleStep (int track, int step) noexcept; // renvoie le nouvel état
    void clear() noexcept;
    Pattern getPattern() const noexcept;
    void setPattern (const Pattern&) noexcept;

    // --- Transport (tous threads) ---
    double getBpm() const noexcept          { return bpm.load (std::memory_order_relaxed); }
    void   setBpm (double newBpm) noexcept;
    bool   isPlaying() const noexcept       { return playing.load (std::memory_order_relaxed); }
    void   setPlaying (bool shouldPlay) noexcept;

    /** Pas en cours de lecture (pour l'affichage), ou -1 à l'arrêt. */
    int getCurrentStep() const noexcept     { return currentStep.load (std::memory_order_relaxed); }

    // --- Thread audio uniquement ---
    /** Cale la position sur celle de l'hôte (en noires depuis le début du morceau).
        Sans force, on ne saute que si l'écart dépasse une petite tolérance : cela évite
        de rater ou doubler un pas à cause des arrondis entre deux blocs. */
    void syncToPpq (double ppqPosition, bool force = false) noexcept;

    /** Éteint la tête de lecture ; la prochaine lecture repart du premier pas. */
    void resetPlayhead() noexcept;

    /** Avance de numSamples et appelle onTrigger(track, sampleOffset) pour chaque pas
        actif rencontré. À n'appeler que pendant la lecture. */
    template <typename TriggerFn>
    void process (int numSamples, double sampleRate, TriggerFn&& onTrigger) noexcept
    {
        if (numSamples <= 0 || sampleRate <= 0.0)
            return;

        if (restartRequested.exchange (false, std::memory_order_relaxed))
            position = 0.0;

        const double stepsPerSample = getBpm() * kStepsPerBeat / (60.0 * sampleRate);
        const double end = position + stepsPerSample * numSamples;

        // Chaque frontière de pas k dans [position, end) déclenche le pas k.
        auto k = (long long) position;
        if ((double) k < position)
            ++k;

        for (; (double) k < end; ++k)
        {
            int offset = (int) (((double) k - position) / stepsPerSample + 0.5);
            if (offset >= numSamples)
                offset = numSamples - 1;

            const int step = (int) (k % kNumSteps);
            currentStep.store (step, std::memory_order_relaxed);

            for (int t = 0; t < kNumTracks; ++t)
                if (getStep (t, step))
                    onTrigger (t, offset);
        }

        position = end;
        if (position >= (double) kNumSteps)
            position -= (double) kNumSteps * (double) (long long) (position / kNumSteps);
    }

private:
    std::array<std::array<std::atomic<bool>, kNumSteps>, kNumTracks> steps;
    std::atomic<double> bpm { 120.0 };
    std::atomic<bool>   playing { false };
    std::atomic<bool>   restartRequested { false };
    std::atomic<int>    currentStep { -1 };

    double position = 0.0; // en pas, [0, kNumSteps) — thread audio uniquement
};

} // namespace alpha
