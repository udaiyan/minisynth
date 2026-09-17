#include "dsp/SynthEngine.h"

#include <cmath>

namespace minisynth
{

    void SynthEngine::setSampleRate(double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        for (auto& voice : voices)
            voice.setSampleRate(sampleRate);
    }

    void SynthEngine::reset() noexcept
    {
        for (auto& voice : voices)
            voice.reset();

        nextStealIndex = 0;
    }

    SynthVoice* SynthEngine::findFreeVoice() noexcept
    {
        for (auto& voice : voices)
            if (!voice.isActive())
                return &voice;

        // Every voice is busy. Steal one, round-robin.
        auto* stolen = &voices[static_cast<size_t> (nextStealIndex)];
        nextStealIndex = (nextStealIndex + 1) % maxVoices;
        return stolen;
    }

    SynthVoice* SynthEngine::findVoicePlayingNote(int midiNote) noexcept
    {
        for (auto& voice : voices)
            if (voice.isActive() && voice.getCurrentNote() == midiNote)
                return &voice;

        return nullptr;
    }

    void SynthEngine::noteOn(int midiNote, float velocity) noexcept
    {
        if (auto* voice = findFreeVoice())
            voice->noteOn(midiNote, velocity);
    }

    void SynthEngine::noteOff(int midiNote) noexcept
    {
        if (auto* voice = findVoicePlayingNote(midiNote))
            voice->noteOff();
    }

    void SynthEngine::allNotesOff() noexcept
    {
        for (auto& voice : voices)
            if (voice.isActive())
                voice.noteOff();
    }

    void SynthEngine::setFilterParams(double cutoff, double resonance, FilterMode mode) noexcept
    {
        for (auto& voice : voices)
        {
            auto& filter = voice.getFilter();
            filter.setCutoff(cutoff);
            filter.setResonance(resonance);
            filter.setMode(mode);
        }
    }

    float SynthEngine::getNextSample() noexcept
    {
        float sum = 0.0f;

        for (auto& voice : voices)
            if (voice.isActive())
                sum += voice.getNextSample();

        return sum;
    }

    int SynthEngine::getActiveVoiceCount() const noexcept
    {
        int count = 0;

        for (const auto& voice : voices)
            if (voice.isActive())
                ++count;

        return count;
    }

} // namespace minisynth