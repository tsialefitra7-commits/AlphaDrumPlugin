#include "Core/SamplerEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace alpha
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kBuiltInRate = 44100.0;

/** Petit générateur de bruit déterministe (les sons intégrés sont identiques à chaque fois). */
struct Noise
{
    std::uint32_t state;
    float next() noexcept
    {
        state = state * 1664525u + 1013904223u;
        return (float) ((state >> 8) & 0xFFFF) / 32768.0f - 1.0f;
    }
};

/** Synthèse des sons par défaut : 0 kick, 1 snare, 2 clap, 3 charley fermé,
    4 charley ouvert, 5 tom, 6 rim, 7 crash. */
SampleData synthesize (int type)
{
    static const double lengths[kNumTracks] = { 0.45, 0.25, 0.25, 0.06, 0.35, 0.40, 0.05, 1.20 };

    SampleData out;
    out.sampleRate = kBuiltInRate;
    const auto n = (std::size_t) (lengths[type] * kBuiltInRate);
    out.left.resize (n);

    Noise noise { 0x1234567u + (std::uint32_t) type * 7919u };
    double phase = 0.0;
    float hp = 0.0f, prev = 0.0f; // passe-haut un pôle pour les sons bruités

    auto highPass = [&] (float x, float coeff)
    {
        hp = coeff * (hp + x - prev);
        prev = x;
        return hp;
    };

    for (std::size_t i = 0; i < n; ++i)
    {
        const double t = (double) i / kBuiltInRate;
        float v = 0.0f;

        switch (type)
        {
            case 0: // kick : sinus avec chute de hauteur
                phase += 2.0 * kPi * (45.0 + 120.0 * std::exp (-t * 35.0)) / kBuiltInRate;
                v = (float) (std::sin (phase) * std::exp (-t * 9.0));
                break;
            case 1: // snare : corps + bruit
                phase += 2.0 * kPi * (180.0 + 60.0 * std::exp (-t * 40.0)) / kBuiltInRate;
                v = (float) (0.5 * std::sin (phase) * std::exp (-t * 22.0))
                  + 0.6f * highPass (noise.next(), 0.85f) * (float) std::exp (-t * 18.0);
                break;
            case 2: // clap : trois rafales de bruit puis une queue
            {
                const double burst = t < 0.03 ? std::exp (-std::fmod (t, 0.01) * 400.0) : std::exp (-(t - 0.03) * 28.0);
                v = 0.8f * highPass (noise.next(), 0.7f) * (float) burst;
                break;
            }
            case 3: // charley fermé
                v = 0.6f * highPass (noise.next(), 0.3f) * (float) std::exp (-t * 90.0);
                break;
            case 4: // charley ouvert
                v = 0.5f * highPass (noise.next(), 0.3f) * (float) std::exp (-t * 12.0);
                break;
            case 5: // tom
                phase += 2.0 * kPi * (110.0 + 70.0 * std::exp (-t * 20.0)) / kBuiltInRate;
                v = (float) (0.9 * std::sin (phase) * std::exp (-t * 10.0));
                break;
            case 6: // rim
                phase += 2.0 * kPi * 1700.0 / kBuiltInRate;
                v = (float) (0.6 * std::sin (phase) * std::exp (-t * 120.0))
                  + 0.3f * noise.next() * (float) std::exp (-t * 200.0);
                break;
            default: // crash
                v = 0.45f * highPass (noise.next(), 0.5f) * (float) std::exp (-t * 3.5);
                break;
        }

        out.left[i] = v;
    }

    // Court fondu de sortie pour éviter tout clic en fin d'échantillon.
    const std::size_t fade = std::min<std::size_t> (n, 128);
    for (std::size_t i = 0; i < fade; ++i)
        out.left[n - 1 - i] *= (float) i / (float) fade;

    return out;
}
} // namespace

SamplerEngine::SamplerEngine() = default;

void SamplerEngine::prepare (double newSampleRate)
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;
    reset();
}

void SamplerEngine::reset() noexcept
{
    for (auto& v : voices)
        v.active = false;
}

void SamplerEngine::setSample (int track, SampleData sample)
{
    if (! validTrack (track))
        return;

    if (sample.right.size() != sample.left.size())
        sample.right.clear(); // incohérent -> mono
    if (sample.sampleRate <= 0.0)
        sample.sampleRate = 44100.0;

    for (auto& v : voices)
        if (v.track == track)
            v.active = false;

    samples[(size_t) track] = std::move (sample);
}

void SamplerEngine::loadBuiltInSample (int track)
{
    if (validTrack (track))
        setSample (track, synthesize (track));
}

void SamplerEngine::loadBuiltInKit()
{
    for (int t = 0; t < kNumTracks; ++t)
        loadBuiltInSample (t);
}

bool SamplerEngine::hasSample (int track) const noexcept
{
    return validTrack (track) && samples[(size_t) track].length() > 0;
}

void SamplerEngine::setTrackGain (int track, float g) noexcept
{
    if (validTrack (track)) params[(size_t) track].gain.store (std::clamp (g, 0.0f, 2.0f));
}

void SamplerEngine::setTrackPan (int track, float p) noexcept
{
    if (validTrack (track)) params[(size_t) track].pan.store (std::clamp (p, -1.0f, 1.0f));
}

void SamplerEngine::setTrackPitch (int track, float st) noexcept
{
    if (validTrack (track)) params[(size_t) track].pitch.store (std::clamp (st, -24.0f, 24.0f));
}

void SamplerEngine::setTrackMute (int track, bool m) noexcept
{
    if (validTrack (track)) params[(size_t) track].mute.store (m);
}

void SamplerEngine::applyKit (const DrumKit& kit) noexcept
{
    for (int t = 0; t < kNumTracks; ++t)
    {
        const auto& tr = kit.tracks[(size_t) t];
        setTrackGain (t, tr.gain);
        setTrackPan (t, tr.pan);
        setTrackPitch (t, tr.pitchSemitones);
        setTrackMute (t, tr.mute);
    }
}

void SamplerEngine::trigger (int track, float velocity, int sampleOffset) noexcept
{
    if (! hasSample (track))
        return;

    const auto& p = params[(size_t) track];
    if (p.mute.load (std::memory_order_relaxed))
        return;

    // Voix libre, sinon on vole la plus ancienne.
    Voice* voice = nullptr;
    for (auto& v : voices)
    {
        if (! v.active) { voice = &v; break; }
        if (voice == nullptr || v.age < voice->age) voice = &v;
    }

    const float gain = p.gain.load (std::memory_order_relaxed) * std::clamp (velocity, 0.0f, 1.0f);
    const double angle = (p.pan.load (std::memory_order_relaxed) + 1.0) * kPi * 0.25; // puissance constante

    voice->active   = true;
    voice->track    = track;
    voice->delay    = std::max (0, sampleOffset);
    voice->position = 0.0;
    voice->rate     = samples[(size_t) track].sampleRate / sampleRate
                    * std::pow (2.0, p.pitch.load (std::memory_order_relaxed) / 12.0);
    voice->gainL    = gain * (float) (std::cos (angle) * std::sqrt (2.0));
    voice->gainR    = gain * (float) (std::sin (angle) * std::sqrt (2.0));
    voice->age      = ++ageCounter;
}

void SamplerEngine::render (float* left, float* right, int numSamples) noexcept
{
    if (left == nullptr || numSamples <= 0)
        return;

    for (auto& v : voices)
    {
        if (! v.active)
            continue;

        int start = 0;
        if (v.delay > 0)
        {
            if (v.delay >= numSamples)
            {
                v.delay -= numSamples;
                continue;
            }
            start = v.delay;
            v.delay = 0;
        }

        const auto& s = samples[(size_t) v.track];
        const auto len = s.length();
        const float* srcL = s.left.data();
        const float* srcR = s.isStereo() ? s.right.data() : srcL;

        for (int i = start; i < numSamples; ++i)
        {
            const auto idx = (std::size_t) v.position;
            if (idx >= len)
            {
                v.active = false;
                break;
            }

            // Interpolation linéaire (0 après le dernier échantillon).
            const auto frac = (float) (v.position - (double) idx);
            const std::size_t next = idx + 1;
            const float l0 = srcL[idx], l1 = next < len ? srcL[next] : 0.0f;
            const float r0 = srcR[idx], r1 = next < len ? srcR[next] : 0.0f;
            const float l = (l0 + frac * (l1 - l0)) * v.gainL;
            const float r = (r0 + frac * (r1 - r0)) * v.gainR;

            if (right != nullptr)
            {
                left[i]  += l;
                right[i] += r;
            }
            else
            {
                left[i] += 0.5f * (l + r);
            }

            v.position += v.rate;
        }
    }
}

int SamplerEngine::getNumActiveVoices() const noexcept
{
    return (int) std::count_if (voices.begin(), voices.end(), [] (const Voice& v) { return v.active; });
}

} // namespace alpha
