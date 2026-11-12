#include "Core/SequencerState.h"

#include <algorithm>
#include <cmath>

namespace alpha
{
namespace
{
bool inRange (int track, int step) noexcept
{
    return track >= 0 && track < kNumTracks && step >= 0 && step < kNumSteps;
}
}

SequencerState::SequencerState()
{
    clear();
}

bool SequencerState::getStep (int track, int step) const noexcept
{
    return inRange (track, step)
        && steps[(size_t) track][(size_t) step].load (std::memory_order_relaxed);
}

void SequencerState::setStep (int track, int step, bool on) noexcept
{
    if (inRange (track, step))
        steps[(size_t) track][(size_t) step].store (on, std::memory_order_relaxed);
}

bool SequencerState::toggleStep (int track, int step) noexcept
{
    const bool on = ! getStep (track, step);
    setStep (track, step, on);
    return inRange (track, step) && on;
}

void SequencerState::clear() noexcept
{
    for (auto& row : steps)
        for (auto& s : row)
            s.store (false, std::memory_order_relaxed);
}

Pattern SequencerState::getPattern() const noexcept
{
    Pattern p;
    for (int t = 0; t < kNumTracks; ++t)
        for (int s = 0; s < kNumSteps; ++s)
            p.steps[(size_t) t][(size_t) s] = getStep (t, s);
    return p;
}

void SequencerState::setPattern (const Pattern& p) noexcept
{
    for (int t = 0; t < kNumTracks; ++t)
        for (int s = 0; s < kNumSteps; ++s)
            setStep (t, s, p.steps[(size_t) t][(size_t) s]);
}

void SequencerState::setBpm (double newBpm) noexcept
{
    if (std::isfinite (newBpm))
        bpm.store (std::clamp (newBpm, kMinBpm, kMaxBpm), std::memory_order_relaxed);
}

void SequencerState::setPlaying (bool shouldPlay) noexcept
{
    const bool was = playing.exchange (shouldPlay, std::memory_order_relaxed);
    if (shouldPlay != was)
        resetPlayhead();
}

void SequencerState::resetPlayhead() noexcept
{
    restartRequested.store (true, std::memory_order_relaxed);
    currentStep.store (-1, std::memory_order_relaxed);
}

void SequencerState::syncToPpq (double ppqPosition, bool force) noexcept
{
    if (! std::isfinite (ppqPosition))
        return;

    double p = std::fmod (ppqPosition * kStepsPerBeat, (double) kNumSteps);
    if (p < 0.0)
        p += (double) kNumSteps;
    if (p >= (double) kNumSteps)
        p = 0.0;

    double drift = std::abs (p - position);
    drift = std::min (drift, (double) kNumSteps - drift); // distance circulaire

    if (force || drift > 0.05 || restartRequested.load (std::memory_order_relaxed))
        position = p;

    restartRequested.store (false, std::memory_order_relaxed);
}

} // namespace alpha
