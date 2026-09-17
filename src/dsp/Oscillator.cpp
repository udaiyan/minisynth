#include "dsp/Oscillator.h"

namespace minisynth
{

    void Oscillator::setSampleRate(double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        updatePhaseIncrement();
    }

    void Oscillator::setFrequency(double hz) noexcept
    {
        frequency = hz > 0.0 ? hz : 0.0;
        updatePhaseIncrement();
    }

    void Oscillator::setWaveform(Waveform newWaveform) noexcept
    {
        waveform = newWaveform;
    }

    void Oscillator::reset() noexcept
    {
        phase = 0.0;
    }

    void Oscillator::updatePhaseIncrement() noexcept
    {
        phaseIncrement = frequency / sampleRate;
    }

    float Oscillator::getNextSample() noexcept
    {
        float output = 0.0f;

        switch (waveform)
        {
        case Waveform::sine:
            output = static_cast<float> (std::sin(phase * twoPi));
            break;

        case Waveform::saw:
            // Ramp from -1 to +1 across one cycle
            output = static_cast<float> (2.0 * phase - 1.0);
            break;

        case Waveform::square:
            output = phase < 0.5 ? 1.0f : -1.0f;
            break;
        }

        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        return output;
    }

} // namespace minisynth