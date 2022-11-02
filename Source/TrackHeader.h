/*
  ==============================================================================

    TrackHeader.h
    Created: 1 Nov 2022 12:48:57pm
    Author:  Ryan Devens

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class TrackHeader : public Component
{
public:
    explicit TrackHeader (const ARARegionSequence& regionSequenceIn) : regionSequence (regionSequenceIn)
    {
        update();

        addAndMakeVisible (trackNameLabel);
    }

    void resized() override
    {
        trackNameLabel.setBounds (getLocalBounds().reduced (2));
    }

    void paint (Graphics& g) override
    {
        g.setColour (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        g.fillRoundedRectangle (getLocalBounds().reduced (2).toType<float>(), 6.0f);
        g.setColour (getLookAndFeel().findColour (ResizableWindow::backgroundColourId).contrasting());
        g.drawRoundedRectangle (getLocalBounds().reduced (2).toType<float>(), 6.0f, 1.0f);
    }

private:
    void update()
    {
        const auto getWithDefaultValue =
            [] (const ARA::PlugIn::OptionalProperty<ARA::ARAUtf8String>& optional, String defaultValue)
        {
            if (const ARA::ARAUtf8String value = optional)
                return String (value);

            return defaultValue;
        };

        trackNameLabel.setText (getWithDefaultValue (regionSequence.getName(), "No track name"),
                                NotificationType::dontSendNotification);
    }

    const ARARegionSequence& regionSequence;
    Label trackNameLabel;
};
