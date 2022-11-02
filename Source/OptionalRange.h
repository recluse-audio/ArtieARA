/*
  ==============================================================================

    OptionalRange.h
    Created: 1 Nov 2022 12:45:02pm
    Author:  Ryan Devens

  ==============================================================================
*/

#pragma once

class OptionalRange
{
public:
    using Type = Range<int64>;

    OptionalRange() : valid (false) {}
    explicit OptionalRange (Type valueIn) : valid (true), value (std::move (valueIn)) {}

    explicit operator bool() const noexcept { return valid; }

    const auto& operator*() const
    {
        jassert (valid);
        return value;
    }

private:
    bool valid;
    Type value;
};

//==============================================================================
// Returns the modified sample range in the output buffer.
inline OptionalRange readPlaybackRangeIntoBuffer (Range<double> playbackRange,
                                                  const ARAPlaybackRegion* playbackRegion,
                                                  AudioBuffer<float>& buffer,
                                                  const std::function<AudioFormatReader* (ARA::PlugIn::AudioSource*)>& getReader)
{
    const auto rangeInAudioModificationTime = playbackRange.movedToStartAt (playbackRange.getStart()
                                                                            - playbackRegion->getStartInAudioModificationTime());

    const auto audioSource = playbackRegion->getAudioModification()->getAudioSource();
    const auto audioModificationSampleRate = audioSource->getSampleRate();

    const Range<int64_t> sampleRangeInAudioModification {
        ARA::roundSamplePosition (rangeInAudioModificationTime.getStart() * audioModificationSampleRate),
        ARA::roundSamplePosition (rangeInAudioModificationTime.getEnd() * audioModificationSampleRate) - 1
    };

    const auto inputOffset = jlimit ((int64_t) 0, audioSource->getSampleCount(), sampleRangeInAudioModification.getStart());

    const auto outputOffset = -std::min (sampleRangeInAudioModification.getStart(), (int64_t) 0);

    /* TODO: Handle different AudioSource and playback sample rates.

       The conversion should be done inside a specialized AudioFormatReader so that we could use
       playbackSampleRate everywhere in this function and we could still read `readLength` number of samples
       from the source.

       The current implementation will be incorrect when sampling rates differ.
    */
    const auto readLength = [&]
    {
        const auto sourceReadLength =
            std::min (sampleRangeInAudioModification.getEnd(), audioSource->getSampleCount()) - inputOffset;

        const auto outputReadLength =
            std::min (outputOffset + sourceReadLength, (int64_t) buffer.getNumSamples()) - outputOffset;

        return std::min (sourceReadLength, outputReadLength);
    }();

    if (readLength == 0)
        return OptionalRange { {} };

    auto* reader = getReader (audioSource);

    if (reader != nullptr && reader->read (&buffer, (int) outputOffset, (int) readLength, inputOffset, true, true))
        return OptionalRange { { outputOffset, readLength } };

    return {};
}
