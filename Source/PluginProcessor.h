/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LFO.h"

//==============================================================================
/**
*/
class LFO2AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    LFO2AudioProcessor();
    ~LFO2AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


    // 👇  volume 
    float globalVolume = 100.0f;

    //bpm and LFO 
    float bpm = 122.0f; //hardcoded 
    //getPlayHead()->getPosition()->getBpm(); //hardcoded values 
    float division = 16; //default 1/16th 
    float mix = 1.0f; // 1.0 full effect 0 is dry 
    //double sampleRate = sampleRate;

    LFO lfo;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFO2AudioProcessor)

    float volume = 1.0f; // example for your volume slider test

    juce::AudioFormatManager formatManager; //wav file testing
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    bool usingTestAudio = false;


};
