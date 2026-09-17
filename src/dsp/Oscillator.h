#pragma once

#include <cmath>

namespace minisynth
{

    enum class Waveform
    {
        sine,
        saw,
        square
    };

    class Oscillator
    {
    public:
        void setSampleRate(double newSampleRate) noexcept;
        void setFrequency(double hz) noexcept;
        void setWaveform(Waveform newWaveform) noexcept;

        void reset() noexcept;

        float getNextSample() noexcept;

    private:
        static constexpr double twoPi = 6.283185307179586;

        double sampleRate = 44100.0;
        double frequency = 440.0;
        double phase = 0.0;   // normalised 0..1, one full cycle
        double phaseIncrement = 0.0;
        Waveform waveform = Waveform::saw;

        void updatePhaseIncrement() noexcept;
    };

} // namespace minisynth