#include "UI/StepMatrixGrid.h"
#include "UI/FlStudioTheme.h"

namespace alpha::ui
{
StepMatrixGrid::StepMatrixGrid (int tracks, int steps)
    : numTracks (juce::jmax (1, tracks)), numSteps (juce::jmax (1, steps))
{
    for (int t = 0; t < numTracks; ++t)
    {
        trackNames.add ("Track " + juce::String (t + 1));

        for (int s = 0; s < numSteps; ++s)
        {
            auto pad = std::make_unique<FlPadComponent>();
            const bool redGroup = ((s / 4) % 2) == 1;
            pad->setColours (redGroup ? Colours::padRedOff : Colours::padGreyOff,
                             redGroup ? Colours::padRedOn  : Colours::padGreyOn);
            addAndMakeVisible (*pad);
            pads.push_back (std::move (pad));
        }
    }
}

FlPadComponent* StepMatrixGrid::padAt (int track, int step) const
{
    if (track < 0 || track >= numTracks || step < 0 || step >= numSteps)
        return nullptr;
    return pads[(size_t) (track * numSteps + step)].get();
}

void StepMatrixGrid::setStep (int track, int step, bool active)
{
    if (auto* pad = padAt (track, step))
        pad->setActive (active);
}

bool StepMatrixGrid::getStep (int track, int step) const
{
    auto* pad = padAt (track, step);
    return pad != nullptr && pad->isActive();
}

void StepMatrixGrid::setTrackName (int track, const juce::String& name)
{
    if (track >= 0 && track < numTracks && trackNames[track] != name)
    {
        trackNames.set (track, name);
        repaint();
    }
}

void StepMatrixGrid::setPlayheadStep (int step)
{
    if (step == playhead)
        return;

    for (int t = 0; t < numTracks; ++t)
    {
        if (auto* p = padAt (t, playhead)) p->setPlaying (false);
        if (auto* p = padAt (t, step))     p->setPlaying (true);
    }
    playhead = step;
}

juce::Rectangle<int> StepMatrixGrid::rowBounds (int track) const
{
    const int rowHeight = getHeight() / numTracks;
    return { 0, track * rowHeight, getWidth(), rowHeight };
}

void StepMatrixGrid::resized()
{
    const int numGroups = (numSteps + 3) / 4;
    const int padsWidth = getWidth() - Metrics::labelWidth - Metrics::groupGap * numGroups;
    const float padWidth = (float) padsWidth / (float) numSteps;

    for (int t = 0; t < numTracks; ++t)
    {
        const auto row = rowBounds (t).reduced (0, Metrics::padGap / 2 + 1);

        for (int s = 0; s < numSteps; ++s)
        {
            const float x = (float) (Metrics::labelWidth + Metrics::groupGap * (s / 4 + 1)) + padWidth * (float) s;
            padAt (t, s)->setBounds (juce::roundToInt (x), row.getY(),
                                     juce::jmax (4, juce::roundToInt (padWidth) - Metrics::padGap), row.getHeight());
        }
    }
}

void StepMatrixGrid::paint (juce::Graphics& g)
{
    g.fillAll (Colours::background);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));

    for (int t = 0; t < numTracks; ++t)
    {
        const auto label = rowBounds (t).withWidth (Metrics::labelWidth).reduced (0, Metrics::padGap / 2 + 1).toFloat();

        g.setColour (Colours::panel);
        g.fillRoundedRectangle (label, Metrics::cornerRadius);
        g.setColour (Colours::outline);
        g.drawRoundedRectangle (label.reduced (0.5f), Metrics::cornerRadius, 1.0f);

        g.setColour (Colours::accent);
        g.fillRoundedRectangle (label.withWidth (4.0f).reduced (0.0f, 3.0f).translated (3.0f, 0.0f), 2.0f);

        g.setColour (Colours::text);
        g.drawFittedText (trackNames[t], label.toNearestInt().withTrimmedLeft (12).withTrimmedRight (4),
                          juce::Justification::centredLeft, 1);
    }
}

FlPadComponent* StepMatrixGrid::padAtPosition (juce::Point<int> pos, int& track, int& step) const
{
    for (int t = 0; t < numTracks; ++t)
        for (int s = 0; s < numSteps; ++s)
            if (auto* pad = padAt (t, s); pad->getBounds().contains (pos))
            {
                track = t;
                step = s;
                return pad;
            }
    return nullptr;
}

int StepMatrixGrid::labelAtPosition (juce::Point<int> pos) const
{
    if (pos.x < 0 || pos.x >= Metrics::labelWidth || pos.y < 0)
        return -1;
    const int track = pos.y / juce::jmax (1, getHeight() / numTracks);
    return track < numTracks ? track : -1;
}

void StepMatrixGrid::applyDrag (juce::Point<int> pos)
{
    int track = 0, step = 0;
    if (auto* pad = padAtPosition (pos, track, step); pad != nullptr && pad->isActive() != dragState)
    {
        pad->setActive (dragState);
        if (onStepChanged)
            onStepChanged (track, step, dragState);
    }
}

void StepMatrixGrid::mouseDown (const juce::MouseEvent& e)
{
    dragging = false;

    if (const int label = labelAtPosition (e.getPosition()); label >= 0)
    {
        if (e.mods.isPopupMenu())
        {
            if (onTrackLabelRightClicked) onTrackLabelRightClicked (label);
        }
        else if (onTrackLabelClicked)
        {
            onTrackLabelClicked (label);
        }
        return;
    }

    int track = 0, step = 0;
    if (auto* pad = padAtPosition (e.getPosition(), track, step))
    {
        // Clic gauche : inverse le pad. Clic droit : efface. Le glissé "peint" le même état.
        dragState = e.mods.isPopupMenu() ? false : ! pad->isActive();
        dragging = true;
        applyDrag (e.getPosition());
    }
}

void StepMatrixGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        applyDrag (e.getPosition());
}

} // namespace alpha::ui
