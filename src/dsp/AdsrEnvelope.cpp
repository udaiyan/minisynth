#include "dsp/AdsrEnvelope.h"

namespace minisynth
{

    void AdsrEnvelope::setSampleRate(double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        updateIncrements();
    }

    void AdsrEnvelope::setParameters(double attackSeconds,
        double decaySeconds,
        double sustainLevelIn,
        double releaseSeconds) noexcept
    {
        attackTime = attackSeconds;
        decayTime = decaySeconds;
        sustainLevel = sustainLevelIn < 0.0 ? 0.0
            : sustainLevelIn > 1.0 ? 1.0
            : sustainLevelIn;
        releaseTime = releaseSeconds;

        updateIncrements();
    }

    void AdsrEnvelope::updateIncrements() noexcept
    {
        const auto safeAttack = attackTime > minimumTime ? attackTime : minimumTime;
        const auto safeDecay = decayTime > minimumTime ? decayTime : minimumTime;

        attackIncrement = 1.0 / (safeAttack * sampleRate);
        decayDecrement = (1.0 - sustainLevel) / (safeDecay * sampleRate);
    }

    void AdsrEnvelope::reset() noexcept
    {
        stage = Stage::idle;
        level = 0.0;
    }

    void AdsrEnvelope::noteOn() noexcept
    {
        stage = Stage::attack;
        level = 0.0;
    }

    void AdsrEnvelope::noteOff() noexcept
    {
        if (stage == Stage::idle)
            return;

        stage = Stage::release;

        releaseDecrement = (releaseTime > minimumTime)
            ? level / (releaseTime * sampleRate)
            : level;
    }

    float AdsrEnvelope::getNextSample() noexcept
    {
        switch (stage)
        {
        case Stage::idle:
            return 0.0f;

        case Stage::attack:
            level += attackIncrement;
            if (level >= 1.0)
            {
                level = 1.0;
                stage = Stage::decay;
            }
            break;

        case Stage::decay:
            level -= decayDecrement;
            if (level <= sustainLevel)
            {
                level = sustainLevel;
                stage = Stage::sustain;
            }
            break;

        case Stage::sustain:
            level = sustainLevel;
            break;

        case Stage::release:
            level -= releaseDecrement;
            if (level <= 0.0)
            {
                level = 0.0;
                stage = Stage::idle;
            }
            break;
        }

        return static_cast<float> (level);
    }
}