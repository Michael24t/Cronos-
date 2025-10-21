/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LFO2AudioProcessor::LFO2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

LFO2AudioProcessor::~LFO2AudioProcessor()
{
}

//==============================================================================
const juce::String LFO2AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LFO2AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LFO2AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LFO2AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LFO2AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LFO2AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LFO2AudioProcessor::getCurrentProgram()
{
    return 0;
}

void LFO2AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LFO2AudioProcessor::getProgramName (int index)
{
    return {};
}

void LFO2AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void LFO2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    formatManager.registerBasicFormats();

    // === Load hardcoded WAV file (for standalone debug only) ===
#if JucePlugin_Build_Standalone
    //juce::File testFile("C:/Users/Michael/Desktop/DJ Tumminia/Songs/wasted x midnight city (one shot high bpm).mp3"); // <-- change this path!
    juce::File testFile("C:/Users/Michael/Desktop/Cronos/Cronos-/tempAudio/unisonSin.mp3"); // <-- change this path!

    if (testFile.existsAsFile())
    {
        if (auto* reader = formatManager.createReaderFor(testFile))
        {
            readerSource.reset(new juce::AudioFormatReaderSource(reader, true));
            transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
            transportSource.setLooping(true);
            transportSource.prepareToPlay(samplesPerBlock, sampleRate);
            transportSource.start();
            usingTestAudio = true;
        }
    }
#endif

    bpm = 122.0f;
    lfo.setSampleRate(sampleRate);
    lfo.setRate(bpm, division);
}

void LFO2AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    transportSource.releaseResources(); 

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LFO2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void LFO2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (usingTestAudio)
    {
        juce::AudioSourceChannelInfo info(buffer);
        transportSource.getNextAudioBlock(info);
    }

    for (int channel = 0; channel < getTotalNumOutputChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float lfoValue = lfo.getNextSample(); // e.g. 0–1
            float lfoGain = juce::jmap(lfoValue, 0.0f, 1.0f, 0.0f, 1.0f);
            float gain = (1.0f - mix) + (mix * lfoGain);
            samples[i] *= gain * globalVolume;
        }
    }


    // === Apply Volume (will affect test file and real DAW audio equally) ===
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        buffer.applyGain(channel, 0, buffer.getNumSamples(), globalVolume);
    }

    //NORMAL OUTPUT BELOW
    /*
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
    */
}

//==============================================================================
bool LFO2AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* LFO2AudioProcessor::createEditor()
{
    return new LFO2AudioProcessorEditor (*this);
}

//==============================================================================
void LFO2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LFO2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LFO2AudioProcessor();
}


