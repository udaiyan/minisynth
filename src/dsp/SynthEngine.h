#pragma once

#include "SynthVoice.h"

#include <array>

namespace minisynth
{

    class SynthEngine
    {
    public:
        static constexpr int maxVoices = 8;

        void setSampleRate(double newSampleRate) noexcept;
        void reset() noexcept;

        void noteOn(int midiNote, float velocity) noexcept;
        void noteOff(int midiNote) noexcept;
        void allNotesOff() noexcept;

        void setFilterParams(double cutoff, double resonance, FilterMode mode) noexcept;

        float getNextSample() noexcept;

        int getActiveVoiceCount() const noexcept;

        // Accessors, mainly for tests
        SynthVoice& getVoice(int index)       noexcept { return voices[static_cast<size_t> (index)]; }
        const SynthVoice& getVoice(int index) const noexcept { return voices[static_cast<size_t> (index)]; }

    private:
        SynthVoice* findFreeVoice() noexcept;
        SynthVoice* findVoicePlayingNote(int midiNote) noexcept;

        std::array<SynthVoice, maxVoices> voices;

        double sampleRate = 44100.0;
        int    nextStealIndex = 0;
    };

} // namespace minisynth