/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================


LFO2AudioProcessorEditor::LFO2AudioProcessorEditor (LFO2AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);


    //slidr
    midiVolume.setSliderStyle(juce::Slider::LinearBarVertical);
    midiVolume.setRange(0.0, 1.0, 0.01);
    midiVolume.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    midiVolume.setPopupDisplayEnabled(true, false, this);
    midiVolume.setTextValueSuffix(" Volume");
    midiVolume.setValue(1.0);
    
    // 👇 Listener? 
    midiVolume.addListener(this);

    // Add it so it actually shows up
    addAndMakeVisible(midiVolume);

}



LFO2AudioProcessorEditor::~LFO2AudioProcessorEditor() {
}


//==============================================================================
void LFO2AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::white);

    g.setColour (juce::Colours::green);
    g.setFont (juce::FontOptions (30.0f));
    g.drawFittedText ("Chronos", getLocalBounds(), juce::Justification::top, 1);
}

void LFO2AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    midiVolume.setBounds(40, 100, 20, getHeight()-60);

}

//should be whats fitting the slider data to the global volume
void LFO2AudioProcessorEditor::sliderValueChanged(juce::Slider* slider) {
    if (slider == &midiVolume) {
        audioProcessor.globalVolume = (float)midiVolume.getValue();
    }
}
