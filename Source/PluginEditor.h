/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WaveFormEditor.h"
#include "GlowEffect.h"

//==============================================================================
/**
*/

extern WaveformEditor waveEditor; //idk if this works 

class GlowTitle : public juce::Component,
    private juce::Timer
{
public:
    GlowTitle()
    {
        // Simulate a "glow" using a DropShadowEffect
        shadow.setShadowProperties({
            juce::Colours::green.withAlpha(0.7f),
            20,  // blur radius
            { 0, 0 } // offset (centered)
            });
        setComponentEffect(&shadow);

        startTimerHz(60); // update ~60 times per second
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(currentGlowColour);
        g.setFont(juce::Font(40.0f, juce::Font::bold));
        g.drawFittedText("Chronos DAW", getLocalBounds(), juce::Justification::centred, 1);
    }

private:
    juce::DropShadowEffect shadow;
    juce::Colour currentGlowColour = juce::Colours::lime;
    float hueOffset = 0.0f;


    void timerCallback() override
    {
        // Slowly cycle hue over time
        hueOffset += 0.002f;
        if (hueOffset > 1.0f)
            hueOffset -= 1.0f;

        currentGlowColour = juce::Colour::fromHSV(hueOffset, 1.0f, 1.0f, 0.9f);

        // Update the glow color dynamically
        shadow.setShadowProperties({
            currentGlowColour.withAlpha(0.7f),
            20,
            { 0, 0 }
            });

        repaint(); // triggers paint()
    }
};


class ImageKnob : public juce::Slider
{
public:
    ImageKnob()
    {
        setSliderStyle(juce::Slider::Rotary);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    }

    void setKnobImage(const juce::Image& imageToUse, int numFrames)
    {
        knobImage = imageToUse;
        totalFrames = numFrames;
    }

    void paint(juce::Graphics& g) override
    {
        if (knobImage.isValid())
        {
            const int frameHeight = knobImage.getHeight() / totalFrames;
            const int currentFrame = (int)juce::jmap<float>(getValue(), getMinimum(), getMaximum(), 0.0f, (float)(totalFrames - 1));
            g.drawImage(knobImage, 0, 0, getWidth(), getHeight(),
                0, currentFrame * frameHeight, knobImage.getWidth(), frameHeight);
        }
        else
        {
            juce::Slider::paint(g);
        }
    }

private:
    juce::Image knobImage;
    int totalFrames = 0;
};


class LFO2AudioProcessorEditor  : public juce::AudioProcessorEditor,
    private juce::Slider::Listener,
    private juce::Timer
{
public:
    LFO2AudioProcessorEditor (LFO2AudioProcessor&);
    ~LFO2AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    //void paintOverChildren(juce::Graphics& g);
    void resized() override;
    void updateTimeSliderFromDivision();



    float bpmDivisionToHz(float bpm, float division)
    {
        float beatsPerSecond = bpm / 60.0f;
        return beatsPerSecond / division;
    }

    float positionToHz(float pos, float bpm); //conversion helper funcctions for even spacing 
    float hzToPosition(float hz, float bpm);

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    LFO2AudioProcessor& audioProcessor;

    juce::Slider midiVolume; //slider thing
    juce::Slider timeSlider;   //time stamp slider 
    juce::Slider mixKnob;   //time stamp slider 

    juce::Label timeLabel;
    juce::Label timeValueLabel;

    ImageKnob customKnob;
    juce::Image knobImg; //persistent imaging 

    GlowTitle titleGlow;

    juce::ComboBox lfoShapeSelector;
    juce::Label lfoShapeLabel;

    void sliderValueChanged(juce::Slider* slider) override;

    void timerCallback() override;


    juce::TextButton bpmButton{ "BPM" };
    juce::TextButton hzButton{ "Hz" };
    juce::TextButton bpmHzButton{ "BPM/Hz" };


    GlowEffect volumeGlow; //for glow 


    struct TimeDivision
    {
        juce::String name;
        float division; // Set up common time divisions
    };

    std::vector<TimeDivision> divisions = {
        {"1/16", 4.0f},
        {"1/8", 2.0f},
        {"1/4", 1.0f},
        {"1/2", 0.5f},
        {"1/1", 0.25f}
    };
    std::map<float, float> divisionPositions = { //Equal distance between time sliders
    {0.0f, 4.0f},  // 1/16
    {0.25f, 2.0f}, // 1/8
    {0.5f, 1.0f},  // 1/4
    {0.75f, 0.5f}, // 1/2
    {1.0f, 0.25f}  // 1/1
    };


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFO2AudioProcessorEditor)
};
