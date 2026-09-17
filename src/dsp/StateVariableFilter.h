#pragma once

#include <cmath>

namespace minisynth
{

    enum class FilterMode { lowPass, bandPass, highPass };

    class StateVariableFilter
    {
    public:
        void setSampleRate(double newSampleRate) noexcept;
        void setCutoff(double hz) noexcept;
        void setResonance(double q) noexcept;
        void setMode(FilterMode newMode) noexcept;

        void reset() noexcept;

        float processSample(float input) noexcept;

    private:
        void updateCoefficients() noexcept;

        static constexpr double pi = 3.141592653589793;

        double sampleRate = 44100.0;
        double cutoff = 1000.0;
        double resonance = 0.707;    // Q — 0.707 is Butterworth, i.e. flat
        FilterMode mode = FilterMode::lowPass;

        // Coefficients, recomputed only when a parameter changes
        double g = 0.0;
        double k = 1.0;
        double a1 = 0.0;
        double a2 = 0.0;
        double a3 = 0.0;

        // Filter state — carries between samples
        double ic1eq = 0.0;
        double ic2eq = 0.0;
    };

} // namespace minisynth