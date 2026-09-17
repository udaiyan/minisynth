#include "dsp/StateVariableFilter.h"

namespace minisynth
{

    void StateVariableFilter::setSampleRate(double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        updateCoefficients();
    }

    void StateVariableFilter::setCutoff(double hz) noexcept
    {
        const auto maxCutoff = sampleRate * 0.45;
        cutoff = hz < 20.0 ? 20.0
            : hz > maxCutoff ? maxCutoff
            : hz;

        updateCoefficients();
    }

    void StateVariableFilter::setResonance(double q) noexcept
    {
        resonance = q > 0.0 ? q : 0.707;
        updateCoefficients();
    }

    void StateVariableFilter::setMode(FilterMode newMode) noexcept
    {
        mode = newMode;
    }

    void StateVariableFilter::reset() noexcept
    {
        ic1eq = 0.0;
        ic2eq = 0.0;
    }

    void StateVariableFilter::updateCoefficients() noexcept
    {
        g = std::tan(pi * cutoff / sampleRate);
        k = 1.0 / resonance;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float StateVariableFilter::processSample(float input) noexcept
    {
        const auto v0 = static_cast<double> (input);

        const auto v3 = v0 - ic2eq;
        const auto v1 = a1 * ic1eq + a2 * v3;
        const auto v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = 2.0 * v1 - ic1eq;
        ic2eq = 2.0 * v2 - ic2eq;

        double output = 0.0;

        switch (mode)
        {
        case FilterMode::lowPass:  output = v2;                   break;
        case FilterMode::bandPass: output = v1;                   break;
        case FilterMode::highPass: output = v0 - k * v1 - v2;     break;
        }

        return static_cast<float> (output);
    }
}