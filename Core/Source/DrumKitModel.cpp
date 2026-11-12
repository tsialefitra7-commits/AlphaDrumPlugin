#include "Core/DrumKitModel.h"

namespace alpha
{
int DrumKit::trackForMidiNote (int note) const noexcept
{
    for (int t = 0; t < kNumTracks; ++t)
        if (tracks[(size_t) t].midiNote == note)
            return t;
    return -1;
}

void Pattern::clear() noexcept
{
    for (auto& row : steps)
        row.fill (false);
}

bool Pattern::isEmpty() const noexcept
{
    for (const auto& row : steps)
        for (bool on : row)
            if (on)
                return false;
    return true;
}

std::string Pattern::toString() const
{
    std::string out;
    out.reserve ((size_t) (kNumTracks * (kNumSteps + 1)));
    for (int t = 0; t < kNumTracks; ++t)
    {
        if (t > 0)
            out += '|';
        for (bool on : steps[(size_t) t])
            out += on ? '1' : '0';
    }
    return out;
}

Pattern Pattern::fromString (const std::string& text)
{
    Pattern p;
    int track = 0, step = 0;
    for (char c : text)
    {
        if (c == '|')
        {
            ++track;
            step = 0;
        }
        else if (track < kNumTracks && step < kNumSteps)
        {
            p.steps[(size_t) track][(size_t) step] = (c == '1');
            ++step;
        }
    }
    return p;
}

DrumKit makeDefaultKit()
{
    static const char* const names[kNumTracks] = { "Kick", "Snare", "Clap", "Closed Hat",
                                                   "Open Hat", "Tom", "Rim", "Crash" };
    DrumKit kit;
    for (int t = 0; t < kNumTracks; ++t)
    {
        kit.tracks[(size_t) t].name = names[t];
        kit.tracks[(size_t) t].midiNote = kFirstMidiNote + t;
    }
    return kit;
}

Pattern makeDefaultPattern()
{
    Pattern p;
    for (int s = 0; s < kNumSteps; s += 4) p.steps[0][(size_t) s] = true;   // kick sur chaque temps
    p.steps[1][4] = p.steps[1][12] = true;                                   // snare sur 2 et 4
    for (int s = 2; s < kNumSteps; s += 4) p.steps[3][(size_t) s] = true;   // charley à contretemps
    return p;
}

Preset makeDefaultPreset()
{
    Preset preset;
    preset.kit = makeDefaultKit();
    preset.pattern = makeDefaultPattern();
    return preset;
}

} // namespace alpha
