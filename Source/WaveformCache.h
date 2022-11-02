/*
  ==============================================================================

    WaveformCache.h
    Created: 1 Nov 2022 12:47:07pm
    Author:  Ryan Devens

  ==============================================================================
*/

#pragma once

//==============================================================================
struct WaveformCache : private ARAAudioSource::Listener
{
    WaveformCache() : thumbnailCache (20)
    {
    }

    ~WaveformCache() override
    {
        for (const auto& entry : thumbnails)
        {
            entry.first->removeListener (this);
        }
    }

    //==============================================================================
    void willDestroyAudioSource (ARAAudioSource* audioSource) override
    {
        removeAudioSource (audioSource);
    }

    AudioThumbnail& getOrCreateThumbnail (ARAAudioSource* audioSource)
    {
        const auto iter = thumbnails.find (audioSource);

        if (iter != std::end (thumbnails))
            return *iter->second;

        auto thumb = std::make_unique<AudioThumbnail> (128, dummyManager, thumbnailCache);
        auto& result = *thumb;

        ++hash;
        thumb->setReader (new ARAAudioSourceReader (audioSource), hash);

        audioSource->addListener (this);
        thumbnails.emplace (audioSource, std::move (thumb));
        return result;
    }

private:
    void removeAudioSource (ARAAudioSource* audioSource)
    {
        audioSource->removeListener (this);
        thumbnails.erase (audioSource);
    }

    int64 hash = 0;
    AudioFormatManager dummyManager;
    AudioThumbnailCache thumbnailCache;
    std::map<ARAAudioSource*, std::unique_ptr<AudioThumbnail>> thumbnails;
};
