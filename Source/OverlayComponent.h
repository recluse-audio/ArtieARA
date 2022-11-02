/*
  ==============================================================================

    OverlayComponent.h
    Created: 1 Nov 2022 12:50:25pm
    Author:  Ryan Devens

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class OverlayComponent : public Component,
                         private Timer
{
public:
    class PlayheadMarkerComponent : public Component
    {
        void paint (Graphics& g) override { g.fillAll (juce::Colours::yellow.darker (0.2f)); }
    };

    OverlayComponent (PlayHeadState& playHeadStateIn)
        : playHeadState (&playHeadStateIn)
    {
        addChildComponent (playheadMarker);
        setInterceptsMouseClicks (false, false);
        startTimerHz (30);
    }

    ~OverlayComponent() override
    {
        stopTimer();
    }

    void resized() override
    {
        doResize();
    }

    void setZoomLevel (double pixelPerSecondIn)
    {
        pixelPerSecond = pixelPerSecondIn;
    }

    void setHorizontalOffset (int offset)
    {
        horizontalOffset = offset;
    }

private:
    void doResize()
    {
        if (playHeadState->isPlaying.load())
        {
            const auto markerX = playHeadState->timeInSeconds.load() * pixelPerSecond;
            const auto playheadLine = getLocalBounds().withTrimmedLeft ((int) (markerX - markerWidth / 2.0) - horizontalOffset)
                                                      .removeFromLeft ((int) markerWidth);
            playheadMarker.setVisible (true);
            playheadMarker.setBounds (playheadLine);
        }
        else
        {
            playheadMarker.setVisible (false);
        }
    }

    void timerCallback() override
    {
        doResize();
    }

    static constexpr double markerWidth = 2.0;

    PlayHeadState* playHeadState;
    double pixelPerSecond = 1.0;
    int horizontalOffset = 0;
    PlayheadMarkerComponent playheadMarker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverlayComponent)
};
