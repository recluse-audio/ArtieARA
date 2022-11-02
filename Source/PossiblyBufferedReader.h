/*
  ==============================================================================

    PossiblyBufferedReader.h
    Created: 1 Nov 2022 12:45:28pm
    Author:  Ryan Devens

  ==============================================================================
*/

#pragma once

class PossiblyBufferedReader
{
public:
    PossiblyBufferedReader() = default;

    explicit PossiblyBufferedReader (std::unique_ptr<BufferingAudioReader> readerIn)
        : setTimeoutFn ([ptr = readerIn.get()] (int ms) { ptr->setReadTimeout (ms); }),
          reader (std::move (readerIn))
    {}

    explicit PossiblyBufferedReader (std::unique_ptr<AudioFormatReader> readerIn)
        : setTimeoutFn(),
          reader (std::move (readerIn))
    {}

    void setReadTimeout (int ms)
    {
        NullCheckedInvocation::invoke (setTimeoutFn, ms);
    }

    AudioFormatReader* get() const { return reader.get(); }

private:
    std::function<void (int)> setTimeoutFn;
    std::unique_ptr<AudioFormatReader> reader;
};
