#pragma once

// Structures de données pures : kit, pistes, pattern, preset. Aucune dépendance JUCE.

#include <array>
#include <string>

namespace alpha
{
constexpr int kNumTracks = 8;
constexpr int kNumSteps  = 16;
constexpr int kFirstMidiNote = 36; // C1 (General MIDI : grosse caisse)

/** Une piste du kit (un son + ses réglages). */
struct DrumTrack
{
    std::string name;
    std::string samplePath;       // vide = son intégré
    int   midiNote       = kFirstMidiNote;
    float gain           = 1.0f;  // linéaire, 0..2
    float pan            = 0.0f;  // -1 (gauche) .. +1 (droite)
    float pitchSemitones = 0.0f;  // -24 .. +24
    bool  mute           = false;
};

/** Un kit complet. */
struct DrumKit
{
    std::string name = "Default Kit";
    std::array<DrumTrack, kNumTracks> tracks;

    /** Index de la piste associée à une note MIDI, ou -1. */
    int trackForMidiNote (int note) const noexcept;
};

/** Grille de pas (copie "à plat", sans atomiques : pour la sauvegarde et les presets). */
struct Pattern
{
    std::array<std::array<bool, kNumSteps>, kNumTracks> steps {};

    void clear() noexcept;
    bool isEmpty() const noexcept;

    /** Sérialisation texte : une ligne de '0'/'1' par piste, séparées par '|'. */
    std::string toString() const;
    static Pattern fromString (const std::string& text);
};

/** Un preset = kit + pattern + tempo. */
struct Preset
{
    std::string name = "Init";
    DrumKit kit;
    Pattern pattern;
    double  bpm = 120.0;
};

DrumKit makeDefaultKit();
Pattern makeDefaultPattern();
Preset  makeDefaultPreset();

} // namespace alpha
