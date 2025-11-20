// LFO.h
#pragma once
#include <JuceHeader.h>
#include <vector>

class LFO
{
public:
    enum class Shape { Saw, Sine, Triangle, Square, Custom };

    LFO()
    {
        setSampleRate(44100.0);
        setShape(Shape::Saw);
    }

    void setSampleRate(double sr) { sampleRate = sr; }


    void setRate(float bpm_, float division)
    {
        bpm = bpm_;
        float beatsPerSecond = bpm / 60.0f;
        rateHz = beatsPerSecond / division;
        updateSmoothing();
    }


    void setShape(Shape s) { shape = s; }
    Shape getShape() const { return shape; }

    // Replace custom waveform (samples in range [0..1]) - thread safe
    void setCustomWaveform(const std::vector<float>& samples)
    {
        const juce::ScopedLock lock(bufferLock);
        customBuffer = samples;
        // precompute size to avoid repeatedly calling size() on audio thread
        customSize = (int)customBuffer.size();
        // ensure non-empty
        if (customSize > 0)
            useCustom = true;
    }

    // Clear custom waveform and go back to normal shape
    void clearCustomWaveform()
    {
        const juce::ScopedLock lock(bufferLock);
        customBuffer.clear();
        customSize = 0;
        useCustom = false;
    }

    void reset() { phase = 0.0f; }

    // Get next sample in 0..1 range (audio thread)
    float getNextSample()
    {
        float out = 0.0f;
        if (useCustom)
        {
            // read from customBuffer with linear interpolation
            const juce::ScopedTryLock tryLock(bufferLock);
            if (tryLock.isLocked() && customSize > 0)
            {
                double idx = phase * (customSize - 1);
                int i0 = (int)floor(idx);
                int i1 = (i0 + 1) % customSize;
                float frac = (float)(idx - i0);
                out = customBuffer[i0] * (1.0f - frac) + customBuffer[i1] * frac;
            }
            else
            {
                // fallback if lock failed or empty -> use saw
                out = (float)phase;
            }
        }
        else
        {
            switch (shape)
            {
            case Shape::Saw:      out = (float)phase; break;
            case Shape::Sine:     out = 0.5f + 0.5f * std::sin(juce::MathConstants<double>::twoPi * phase); break;
            case Shape::Triangle: out = 1.0f - std::abs(juce::jmap((float)phase, 0.0f, 1.0f, -1.0f, 1.0f)); break;
            case Shape::Square:   out = (phase < 0.5) ? 1.0f : 0.0f; break;
            default: out = (float)phase; break;
            }
        }

        // advance phase
        phase += rateHz / sampleRate;
        if (phase >= 1.0) phase -= 1.0;

        smoothedOut = smoothedOut * smoothingCoeff + out * (1.0f - smoothingCoeff);

        return out; 
    }

    float getRateHz() const { return rateHz; }

    void setRateHz(float hz)
    {
        rateHz = hz;
        updateSmoothing();
    }

    void updateSmoothing() {
        const float smoothingTimeMs = 5.0f; // tweakable (2–10 ms works well)
        float tau = smoothingTimeMs * 0.001f;

        smoothingCoeff = std::exp(-1.0f / (sampleRate * tau));
    }
     
private:
    juce::CriticalSection bufferLock;
    std::vector<float> customBuffer;
    int customSize = 0;
    bool useCustom = false;

    double sampleRate = 44100.0;
    float rateHz = 1.0f;
    float bpm = 122.0f;
    double phase = 0.0;
    Shape shape = Shape::Saw;

    //smoothing
    float smoothedOut = 0.0f;
    float smoothingCoeff = 0.0f;
};
