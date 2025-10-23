#pragma once
#include <JuceHeader.h>

// ============================================================
// GlowEffect: reusable glow for any JUCE component
// Supports hue cycling or sinusoidal intensity pulsing
// ============================================================

class GlowEffect : public juce::Component,
    private juce::Timer
{
public:
    enum class Mode
    {
        HueCycle,       // cycle through colors (current behavior)
        PulseIntensity  // pulse brightness of base color
    };

    GlowEffect(juce::Component* targetComponent = nullptr,
        juce::Colour baseColour = juce::Colours::lime,
        float blurRadius = 20.0f,
        bool animate = true,
        Mode mode = Mode::HueCycle,
        float intensity = 0.8f)
        : target(targetComponent),
        baseColour(baseColour),
        blurRadius(blurRadius),
        animate(animate),
        mode(mode),
        intensity(intensity)
    {
        updateShadow();

        if (target)
            target->setComponentEffect(&shadow);

        if (animate)
            startTimerHz(60);
    }

    // === Target & properties ===
    void setTarget(juce::Component* newTarget)
    {
        target = newTarget;
        if (target)
            target->setComponentEffect(&shadow);
    }

    void setBaseColour(juce::Colour newColour)
    {
        baseColour = newColour;
        updateShadow();
    }

    void setBlurRadius(float newRadius)
    {
        blurRadius = newRadius;
        updateShadow();
    }

    void setMode(Mode newMode)
    {
        mode = newMode;
    }

    void setIntensity(float newIntensity) // 0 = no glow, 1 = full glow
    {
        intensity = juce::jlimit(0.0f, 1.0f, newIntensity);
    }

    void enableAnimation(bool shouldAnimate)
    {
        animate = shouldAnimate;
        if (animate)
            startTimerHz(60);
        else
            stopTimer();
    }

private:
    juce::DropShadowEffect shadow;
    juce::Component* target = nullptr;
    juce::Colour baseColour;
    float blurRadius;
    bool animate;
    Mode mode;
    float intensity;      // overall glow intensity (0..1)
    float hueOffset = 0.0f;
    float pulsePhase = 0.0f;

    void updateShadow()
    {
        juce::Colour glowColour = baseColour.withMultipliedAlpha(intensity);
        shadow.setShadowProperties({
            glowColour,
            (int)blurRadius,
            { 0, 0 }
            });

        if (target)
            target->setComponentEffect(&shadow);
    }

    void timerCallback() override
    {
        if (!animate) return;

        if (mode == Mode::HueCycle)
        {
            hueOffset += 0.002f;
            if (hueOffset > 1.0f)
                hueOffset -= 1.0f;

            auto dynamicColour = juce::Colour::fromHSV(hueOffset, 1.0f, 1.0f, intensity);
            shadow.setShadowProperties({
                dynamicColour,
                (int)blurRadius,
                { 0, 0 }
                });
        }
        else if (mode == Mode::PulseIntensity)
        {
            pulsePhase += 0.05f; // controls pulse speed
            if (pulsePhase > juce::MathConstants<float>::twoPi)
                pulsePhase -= juce::MathConstants<float>::twoPi;

            float pulseAlpha = (std::sin(pulsePhase) * 0.5f + 0.5f) * intensity; // 0..intensity
            shadow.setShadowProperties({
                baseColour.withMultipliedAlpha(pulseAlpha),
                (int)blurRadius,
                { 0, 0 }
                });
        }

        if (target)
            target->repaint();
    }
};
