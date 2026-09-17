#include "dsp/SynthVoice.h"

namespace minisynth
{

    double SynthVoice::midiNoteToHz(int midiNote) noexcept
    {
        // MIDI note 69 is A4 = 440 Hz. Each semitone is a factor of 2^(1/12).
        return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    }

    void SynthVoice::setSampleRate(double newSampleRate) noexcept
    {
        oscillator.setSampleRate(newSampleRate);
        filter.setSampleRate(newSampleRate);
        envelope.setSampleRate(newSampleRate);
    }

    void SynthVoice::noteOn(int midiNote, float velocity) noexcept
    {
        currentNote = midiNote;

        // Velocity 0..1 maps to amplitude. A square-root curve feels more
        // musical than linear, because human loudness perception is compressive.
        voiceGain = std::sqrt(velocity > 0.0f ? velocity : 0.0f);

        oscillator.setFrequency(midiNoteToHz(midiNote));
        oscillator.reset();

        envelope.noteOn();
    }

    void SynthVoice::noteOff() noexcept
    {
        envelope.noteOff();
    }

    void SynthVoice::reset() noexcept
    {
        currentNote = -1;
        voiceGain = 0.0f;

        oscillator.reset();
        filter.reset();
        envelope.reset();
    }

    bool SynthVoice::isActive() const noexcept
    {
        return envelope.isActive();
    }

    float SynthVoice::getNextSample() noexcept
    {
        if (!envelope.isActive())
        {
            currentNote = -1;
            return 0.0f;
        }

        const auto envLevel = envelope.getNextSample();
        const auto raw = oscillator.getNextSample();
        const auto filtered = filter.processSample(raw);

        return filtered * envLevel * voiceGain;
    }

}