/*
  ==============================================================================

    PlaybackRegionView.h
    Created: 1 Nov 2022 12:47:47pm
    Author:  Ryan Devens

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class PlaybackRegionView : public Component,
                           public ChangeListener
{
public:
    PlaybackRegionView (ARAPlaybackRegion& region, WaveformCache& cache)
        : playbackRegion (region), waveformCache (cache)
    {
        auto* audioSource = playbackRegion.getAudioModification()->getAudioSource();

        waveformCache.getOrCreateThumbnail (audioSource).addChangeListener (this);
    }

    ~PlaybackRegionView() override
    {
        waveformCache.getOrCreateThumbnail (playbackRegion.getAudioModification()->getAudioSource())
            .removeChangeListener (this);
    }

    void mouseDown (const MouseEvent& m) override
    {
        const auto relativeTime = (double) m.getMouseDownX() / getLocalBounds().getWidth();
        const auto previewTime = playbackRegion.getStartInPlaybackTime()
                                 + relativeTime * playbackRegion.getDurationInPlaybackTime();
        auto& previewState = ARADocumentControllerSpecialisation::getSpecialisedDocumentController<ARADemoPluginDocumentControllerSpecialisation> (playbackRegion.getDocumentController())->previewState;
        previewState.previewTime.store (previewTime);
        previewState.previewedRegion.store (&playbackRegion);
    }

    void mouseUp (const MouseEvent&) override
    {
        auto& previewState = ARADocumentControllerSpecialisation::getSpecialisedDocumentController<ARADemoPluginDocumentControllerSpecialisation> (playbackRegion.getDocumentController())->previewState;
        previewState.previewTime.store (0.0);
        previewState.previewedRegion.store (nullptr);
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        repaint();
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::white.darker());
        g.setColour (Colours::darkgrey.darker());
        auto& thumbnail = waveformCache.getOrCreateThumbnail (playbackRegion.getAudioModification()->getAudioSource());
        thumbnail.drawChannels (g,
                                getLocalBounds(),
                                playbackRegion.getStartInAudioModificationTime(),
                                playbackRegion.getEndInAudioModificationTime(),
                                1.0f);
        g.setColour (Colours::black);
        g.drawRect (getLocalBounds());
    }

    void resized() override
    {
        repaint();
    }

private:
    ARAPlaybackRegion& playbackRegion;
    WaveformCache& waveformCache;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaybackRegionView);
};
