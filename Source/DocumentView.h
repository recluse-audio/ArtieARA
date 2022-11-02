/*
  ==============================================================================

    DocumentView.h
    Created: 1 Nov 2022 12:50:54pm
    Author:  Ryan Devens

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class DocumentView  : public Component,
                      public ChangeListener,
                      private ARADocument::Listener,
                      private ARAEditorView::Listener
{
public:
    explicit DocumentView (ARADocument& document, PlayHeadState& playHeadState)
        : araDocument (document),
          overlay (playHeadState)
    {
        addAndMakeVisible (tracksBackground);

        viewport.onVisibleAreaChanged = [this] (const auto& r)
        {
            viewportHeightOffset = r.getY();
            overlay.setHorizontalOffset (r.getX());
            resized();
        };

        addAndMakeVisible (viewport);

        overlay.setZoomLevel (zoomLevelPixelPerSecond);
        addAndMakeVisible (overlay);

        zoomControls.setZoomInCallback  ([this] { zoom (2.0); });
        zoomControls.setZoomOutCallback ([this] { zoom (0.5); });
        addAndMakeVisible (zoomControls);

        invalidateRegionSequenceViews();
        araDocument.addListener (this);
    }

    ~DocumentView() override
    {
        araDocument.removeListener (this);
    }

    //==============================================================================
    // ARADocument::Listener overrides
    void didReorderRegionSequencesInDocument (ARADocument*) override
    {
        invalidateRegionSequenceViews();
    }

    void didAddRegionSequenceToDocument (ARADocument*, ARARegionSequence*) override
    {
        invalidateRegionSequenceViews();
    }

    void willRemoveRegionSequenceFromDocument (ARADocument*, ARARegionSequence* regionSequence) override
    {
        removeRegionSequenceView (regionSequence);
    }

    void didEndEditing (ARADocument*) override
    {
        rebuildRegionSequenceViews();
        update();
    }

    //==============================================================================
    void changeListenerCallback (ChangeBroadcaster*) override
    {
        update();
    }

    //==============================================================================
    // ARAEditorView::Listener overrides
    void onNewSelection (const ARA::PlugIn::ViewSelection&) override
    {
    }

    void onHideRegionSequences (const std::vector<ARARegionSequence*>&) override
    {
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId).darker());
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const auto bottomControlsBounds = bounds.removeFromBottom (40);
        const auto headerBounds = bounds.removeFromLeft (headerWidth).reduced (2);

        zoomControls.setBounds (bottomControlsBounds);
        layOutVertically (headerBounds, trackHeaders, viewportHeightOffset);
        tracksBackground.setBounds (bounds);
        viewport.setBounds (bounds);
        overlay.setBounds (bounds);
    }

    //==============================================================================
    void setZoomLevel (double pixelPerSecond)
    {
        zoomLevelPixelPerSecond = pixelPerSecond;

        for (const auto& view : regionSequenceViews)
            view.second->setZoomLevel (zoomLevelPixelPerSecond);

        overlay.setZoomLevel (zoomLevelPixelPerSecond);

        update();
    }

    static constexpr int headerWidth = 120;

private:
    struct RegionSequenceViewKey
    {
        explicit RegionSequenceViewKey (ARARegionSequence* regionSequence)
            : orderIndex (regionSequence->getOrderIndex()), sequence (regionSequence)
        {
        }

        bool operator< (const RegionSequenceViewKey& other) const
        {
            return std::tie (orderIndex, sequence) < std::tie (other.orderIndex, other.sequence);
        }

        ARA::ARAInt32 orderIndex;
        ARARegionSequence* sequence;
    };

    void zoom (double factor)
    {
        zoomLevelPixelPerSecond = jlimit (minimumZoom, minimumZoom * 32, zoomLevelPixelPerSecond * factor);
        setZoomLevel (zoomLevelPixelPerSecond);
    }

    template <typename T>
    void layOutVertically (Rectangle<int> bounds, T& components, int verticalOffset = 0)
    {
        bounds = bounds.withY (bounds.getY() - verticalOffset).withHeight (bounds.getHeight() + verticalOffset);

        for (auto& component : components)
        {
            component.second->setBounds (bounds.removeFromTop (trackHeight));
            component.second->resized();
        }
    }

    void update()
    {
        timelineLength = 0.0;

        for (const auto& view : regionSequenceViews)
            timelineLength = std::max (timelineLength, view.second->getPlaybackDuration());

        const Rectangle<int> timelineSize (roundToInt (timelineLength * zoomLevelPixelPerSecond),
                                           (int) regionSequenceViews.size() * trackHeight);
        viewport.content.setSize (timelineSize.getWidth(), timelineSize.getHeight());
        viewport.content.resized();

        resized();
    }

    void addTrackViews (ARARegionSequence* regionSequence)
    {
        const auto insertIntoMap = [](auto& map, auto key, auto value) -> auto&
        {
            auto it = map.insert ({ std::move (key), std::move (value) });
            return *(it.first->second);
        };

        auto& regionSequenceView = insertIntoMap (
            regionSequenceViews,
            RegionSequenceViewKey { regionSequence },
            std::make_unique<RegionSequenceView> (*regionSequence, waveformCache, zoomLevelPixelPerSecond));

        regionSequenceView.addChangeListener (this);
        viewport.content.addAndMakeVisible (regionSequenceView);

        auto& trackHeader = insertIntoMap (trackHeaders,
                                           RegionSequenceViewKey { regionSequence },
                                           std::make_unique<TrackHeader> (*regionSequence));

        addAndMakeVisible (trackHeader);
    }

    void removeRegionSequenceView (ARARegionSequence* regionSequence)
    {
        const auto& view = regionSequenceViews.find (RegionSequenceViewKey { regionSequence });

        if (view != regionSequenceViews.cend())
        {
            removeChildComponent (view->second.get());
            regionSequenceViews.erase (view);
        }

        invalidateRegionSequenceViews();
    }

    void invalidateRegionSequenceViews()
    {
        regionSequenceViewsAreValid = false;
        rebuildRegionSequenceViews();
    }

    void rebuildRegionSequenceViews()
    {
        if (! regionSequenceViewsAreValid && ! araDocument.getDocumentController()->isHostEditingDocument())
        {
            for (auto& view : regionSequenceViews)
                removeChildComponent (view.second.get());

            regionSequenceViews.clear();

            for (auto& view : trackHeaders)
                removeChildComponent (view.second.get());

            trackHeaders.clear();

            for (auto* regionSequence : araDocument.getRegionSequences())
            {
                addTrackViews (regionSequence);
            }

            update();

            regionSequenceViewsAreValid = true;
        }
    }

    class TracksBackgroundComponent : public Component
    {
        void paint (Graphics& g) override
        {
            g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId).brighter());
        }
    };

    static constexpr auto minimumZoom = 10.0;
    static constexpr auto trackHeight = 60;

    ARADocument& araDocument;

    bool regionSequenceViewsAreValid = false;
    double timelineLength = 0;
    double zoomLevelPixelPerSecond = minimumZoom * 4;

    WaveformCache waveformCache;
    TracksBackgroundComponent tracksBackground;
    std::map<RegionSequenceViewKey, std::unique_ptr<TrackHeader>> trackHeaders;
    std::map<RegionSequenceViewKey, std::unique_ptr<RegionSequenceView>> regionSequenceViews;
    VerticalLayoutViewport viewport;
    OverlayComponent overlay;
    ZoomControls zoomControls;

    int viewportHeightOffset = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DocumentView)
};
