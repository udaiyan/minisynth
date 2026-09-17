#pragma once

namespace minisynth
{

    class AdsrEnvelope
    {
    public:
        enum class Stage { idle, attack, decay, sustain, release };

        void setSampleRate(double newSampleRate) noexcept;
        void setParameters(double attackSeconds,
            double decaySeconds,
            double sustainLevel,
            double releaseSeconds) noexcept;

        void noteOn() noexcept;
        void noteOff() noexcept;
        void reset() noexcept;

        float getNextSample() noexcept;

        Stage getStage() const noexcept { return stage; }
        bool isActive() const noexcept { return stage != Stage::idle; }

    private:
        static constexpr double minimumTime = 0.001;  // 1 ms floor

        double sampleRate = 44100.0;
        double attackTime = 0.01;
        double decayTime = 0.1;
        double sustainLevel = 0.7;
        double releaseTime = 0.2;

        Stage  stage = Stage::idle;
        double level = 0.0;
        double attackIncrement = 0.0;
        double decayDecrement = 0.0;
        double releaseDecrement = 0.0;

        void updateIncrements() noexcept;
    };

} // namespace minisynth