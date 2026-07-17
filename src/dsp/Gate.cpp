#include "Gate.h"

#include <cmath>

Gate::Gate() = default;

float Gate::computeRampCoefficient (float timeMs, double sr) noexcept
{
    const auto timeSeconds = juce::jmax (0.0001f, timeMs * 0.001f);
    return 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * timeSeconds));
}

void Gate::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // Re-derive every coefficient from the last commanded values (see the
    // header's doc comment on lastThresholdDb/lastAttackMs/lastHoldMs/
    // lastReleaseMs) rather than resetting to built-in defaults, so a
    // re-prepare (sample-rate change, etc.) never silently discards a live
    // parameter value.
    setThresholdDb (lastThresholdDb);
    setAttackMs (lastAttackMs);
    setHoldMs (lastHoldMs);
    setReleaseMs (lastReleaseMs);

    reset();
}

void Gate::reset()
{
    currentGain = 1.0f;
    holdCounter = 0;
}

void Gate::setThresholdDb (float newThresholdDb)
{
    lastThresholdDb = newThresholdDb;
    const auto safeDb = std::isnan (newThresholdDb) ? -48.0f : newThresholdDb;
    thresholdLinear = juce::Decibels::decibelsToGain (juce::jlimit (minThresholdDb, maxThresholdDb, safeDb));
}

void Gate::setAttackMs (float newAttackMs)
{
    lastAttackMs = newAttackMs;
    const auto safeMs = std::isnan (newAttackMs) ? 1.0f : juce::jlimit (minAttackMs, maxAttackMs, newAttackMs);
    attackCoefficient = computeRampCoefficient (safeMs, sampleRate);
}

void Gate::setHoldMs (float newHoldMs)
{
    lastHoldMs = newHoldMs;
    const auto safeMs = std::isnan (newHoldMs) ? 20.0f : juce::jlimit (minHoldMs, maxHoldMs, newHoldMs);
    holdSamples = static_cast<int> (std::round (static_cast<double> (safeMs) * 0.001 * sampleRate));
}

void Gate::setReleaseMs (float newReleaseMs)
{
    lastReleaseMs = newReleaseMs;
    const auto safeMs = std::isnan (newReleaseMs) ? 150.0f : juce::jlimit (minReleaseMs, maxReleaseMs, newReleaseMs);
    releaseCoefficient = computeRampCoefficient (safeMs, sampleRate);
}

void Gate::setEnabled (bool shouldBeEnabled)
{
    enabled = shouldBeEnabled;
}

void Gate::process (juce::dsp::ProcessContextReplacing<float>& context)
{
    if (! enabled || context.isBypassed)
        return; // true structural bypass - see setEnabled()'s docs.

    auto block = context.getOutputBlock();
    const auto numChannels = block.getNumChannels();
    const auto numSamples = block.getNumSamples();

    for (size_t sample = 0; sample < numSamples; ++sample)
    {
        float peak = 0.0f;

        for (size_t channel = 0; channel < numChannels; ++channel)
            peak = std::max (peak, std::abs (block.getChannelPointer (channel)[sample]));

        // A NaN peak (NaN audio input) fails both comparisons below, so it
        // neither re-arms the hold countdown nor forces the gate closed -
        // currentGain itself stays finite regardless (it only ever combines
        // with the finite 0.0f/1.0f target and finite coefficients), so a
        // NaN sample can't poison the gate's own gain state, even though the
        // NaN sample itself obviously stays NaN through the multiply below
        // (matching every other module's "don't propagate NaN into internal
        // filter/gain state" contract - see TenebraeEngine.cpp's
        // clampBelowNyquist()).
        if (peak >= thresholdLinear)
            holdCounter = holdSamples;
        else if (holdCounter > 0)
            --holdCounter;

        const auto gateShouldBeOpen = (peak >= thresholdLinear) || (holdCounter > 0);
        const auto targetGain = gateShouldBeOpen ? 1.0f : 0.0f;
        const auto coefficient = (targetGain > currentGain) ? attackCoefficient : releaseCoefficient;
        currentGain += (targetGain - currentGain) * coefficient;

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* data = block.getChannelPointer (channel);
            data[sample] *= currentGain;
        }
    }
}
