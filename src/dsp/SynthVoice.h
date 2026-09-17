#pragma once

#include "AdsrEnvelope.h"
#include "Oscillator.h"
#include "StateVariableFilter.h"

namespace minisynth
{

    class SynthVoice
    {
    public:
        void setSampleRate(double newSampleRate) noexcept;

        void noteOn(int midiNote, float velocity) noexcept;
        void noteOff() noexcept;
        void reset() noexcept;

        bool isActive() const noexcept;
        int  getCurrentNote() const noexcept { return currentNote; }

        float getNextSample() noexcept;

        // Accessors for the engine to configure the voice
        Oscillator& getOscillator() noexcept { return oscillator; }
        StateVariableFilter& getFilter()     noexcept { return filter; }
        AdsrEnvelope& getEnvelope()   noexcept { return envelope; }
        const AdsrEnvelope& getEnvelope() const noexcept { return envelope; }

    private:
        static double midiNoteToHz(int midiNote) noexcept;

        Oscillator          oscillator;
        StateVariableFilter filter;
        AdsrEnvelope        envelope;

        int   currentNote = -1;
        float voiceGain = 0.0f;
    };

} // namespace minisynth