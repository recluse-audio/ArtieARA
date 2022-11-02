/*
  ==============================================================================

    RegionSequenceView.h
    Created: 1 Nov 2022 12:48:09pm
    Author:  Ryan Devens

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class RegionSequenceView : public Component,
                           public ARARegionSequence::Listener,
                           public ChangeBroadcaster,
                           private ARAPlaybackRegion::Listener
{
public:
    RegionSequenceView (ARARegionSequence& rs, WaveformCache& cache, double pixelPerSec)
        : regionSequence (rs), waveformCache (cache), zoomLevelPixelPerSecond (pixelPerSec)
    {
        regionSequence.addListener (this);

        for (auto* playbackRegion : regionSequence.getPlaybackRegions())
            createAndAddPlaybackRegionView (playbackRegion);

        updatePlaybackDuration();
    }

    ~RegionSequenceView() override
    {
        regionSequence.removeListener (this);

        for (const auto& it : playbackRegionViews)
            it.first->removeListener (this);
    }

    //==============================================================================
    // ARA Document change callback overrides
    void willRemovePlaybackRegionFromRegionSequence (ARARegionSequence*,
                                                     ARAPlaybackRegion* playbackRegion) override
    {
        playbackRegion->removeListener (this);
        removeChildComponent (playbackRegionViews[playbackRegion].get());
        playbackRegionViews.erase (playbackRegion);
        updatePlaybackDuration();
    }

    void didAddPlaybackRegionToRegionSequence (ARARegionSequence*, ARAPlaybackRegion* playbackRegion) override
    {
        createAndAddPlaybackRegionView (playbackRegion);
        updatePlaybackDuration();
    }

    void willDestroyPlaybackRegion (ARAPlaybackRegion* playbackRegion) override
    {
        playbackRegion->removeListener (this);
        removeChildComponent (playbackRegionViews[playbackRegion].get());
        playbackRegionViews.erase (playbackRegion);
        updatePlaybackDuration();
    }

    void willUpdatePlaybackRegionProperties (ARAPlaybackRegion*, ARAPlaybackRegion::PropertiesPtr) override
    {
    }

    void didUpdatePlaybackRegionProperties (ARAPlaybackRegion*) override
    {
        updatePlaybackDuration();
    }

    void resized() override
    {
        for (auto& pbr : playbackRegionViews)
        {
            const auto playbackRegion = pbr.first;
            pbr.second->setBounds (
                getLocalBounds()
                    .withTrimmedLeft (roundToInt (playbackRegion->getStartInPlaybackTime() * zoomLevelPixelPerSecond))
                    .withWidth (roundToInt (playbackRegion->getDurationInPlaybackTime() * zoomLevelPixelPerSecond)));
        }
    }

    auto getPlaybackDuration() const noexcept
    {
        return playbackDuration;
    }

    void setZoomLevel (double pixelPerSecond)
    {
        zoomLevelPixelPerSecond = pixelPerSecond;
        resized();
    }

private:
    void createAndAddPlaybackRegionView (ARAPlaybackRegion* playbackRegion)
    {
        playbackRegionViews[playbackRegion] = std::make_unique<PlaybackRegionView> (*playbackRegion, waveformCache);
        playbackRegion->addListener (this);
        addAndMakeVisible (*playbackRegionViews[playbackRegion]);
    }

    void updatePlaybackDuration()
    {
        const auto iter = std::max_element (
            playbackRegionViews.begin(),
            playbackRegionViews.end(),
            [] (const auto& a, const auto& b) { return a.first->getEndInPlaybackTime() < b.first->getEndInPlaybackTime(); });

        playbackDuration = iter != playbackRegionViews.end() ? iter->first->getEndInPlaybackTime()
                                                             : 0.0;

        sendChangeMessage();
    }

    ARARegionSequence& regionSequence;
    WaveformCache& waveformCache;
    std::unordered_map<ARAPlaybackRegion*, std::unique_ptr<PlaybackRegionView>> playbackRegionViews;
    double playbackDuration = 0.0;
    double zoomLevelPixelPerSecond;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RegionSequenceView)
};
