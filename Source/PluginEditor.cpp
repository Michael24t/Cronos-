/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LFO.h"



WaveformEditor waveEditor;  
//==============================================================================


LFO2AudioProcessorEditor::LFO2AudioProcessorEditor (LFO2AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (900, 600);
    setResizable(true, true);   //dynamic resize
    getConstrainer()->setFixedAspectRatio(1.5);

    addAndMakeVisible(titleGlow);

    //Volume slidr
    midiVolume.setSliderStyle(juce::Slider::LinearBarVertical);
    midiVolume.setRange(0.0, 1.0, 0.01);
    midiVolume.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    midiVolume.setPopupDisplayEnabled(true, false, this);
    midiVolume.setTextValueSuffix(" Volume");
    midiVolume.setValue(1.0); 
    midiVolume.addListener(this);
    addAndMakeVisible(midiVolume);

    //time division slider 
    timeSlider.setSliderStyle(juce::Slider::Rotary);
    timeSlider.setRange(1, 5, 1); // three discrete positions
    timeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 20);
    timeSlider.setValue(1); // start on 1/16th
    timeSlider.addListener(this);   
    addAndMakeVisible(timeSlider);

    //Time slider label
    timeLabel.setText("Division", juce::dontSendNotification);
    timeLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    timeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeLabel);
    //Time slider time isgnature text 
    timeValueLabel.setText("1/16", juce::dontSendNotification);
    timeValueLabel.setFont(juce::Font(14.0f));
    timeValueLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeValueLabel);
    
    //mix knob 
    mixKnob.setSliderStyle(juce::Slider::Rotary);
    mixKnob.setRange(0.0, 1.0, 0.01);
    mixKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 20);
    mixKnob.setValue(1.0);
    //mixKnob.setColour(juce::Slider::mixKnob, juce::Colours::orange);
    mixKnob.setPopupDisplayEnabled(true, false, this);
    mixKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    mixKnob.setTextValueSuffix(" Mix");
    mixKnob.addListener(this);
    addAndMakeVisible(mixKnob);


    // Load and show the custom knob
    juce::File knobFile("C:/Users/Michael/Desktop/Cronos/Cronos-/Pictures/Knobs/test.png");// path for now
    knobImg = juce::ImageFileFormat::loadFrom(knobFile);

    customKnob.setKnobImage(knobImg, 128); // specify frame count (check your file)
    customKnob.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(customKnob);


    addAndMakeVisible(waveEditor);
    // When editor updates waveform, push it into audio LFO (audioProcessor.lfo)
    waveEditor.setUpdateCallback([this](const std::vector<float>& buf)
        {
            // This is GUI thread calling into processor — safe because LFO uses CriticalSection
            audioProcessor.lfo.setCustomWaveform(buf);
        });





}



LFO2AudioProcessorEditor::~LFO2AudioProcessorEditor() {
}


//==============================================================================
void LFO2AudioProcessorEditor::paint (juce::Graphics& g) //paint is called very often so dont put anything crazy in here 
{
    g.fillAll(juce::Colours::black);

    g.setColour (juce::Colours::green);
    g.setFont (juce::FontOptions (30.0f));
    g.drawFittedText ("Chronos", getLocalBounds(), juce::Justification::top, 1);


    // In your component's constructor or a relevant method
    timeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::red); // Changes the color of the slider's thumb
    timeSlider.setColour(juce::Slider::trackColourId, juce::Colours::black); // Changes the color of the slider's track
    timeSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::green); // For rotary sliders
    timeSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::lightgrey); // General background

    timeLabel.setColour(juce::Label::textColourId, juce::Colours::darkgrey);
    timeValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    mixKnob.setColour(juce::Slider::thumbColourId, juce::Colours::red); // Changes the color of the slider's thumb
    mixKnob.setColour(juce::Slider::trackColourId, juce::Colours::black); // Changes the color of the slider's track
    mixKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::green); // For rotary sliders
    mixKnob.setColour(juce::Slider::backgroundColourId, juce::Colours::lightgrey); // General background



}

void LFO2AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    midiVolume.setBounds(40, 100, 20, getHeight()-60);

    timeSlider.setBounds(100, 100, 100, 100);
    auto timeSliderBounds = timeSlider.getBounds(); //getting bounds for labels underneath 

    timeValueLabel.setBounds(timeSliderBounds.getX(),timeSliderBounds.getY() - 20,timeSliderBounds.getWidth(),20);
    timeLabel.setBounds(timeSliderBounds.getX(),timeSliderBounds.getBottom(),timeSliderBounds.getWidth(),20);

    mixKnob.setBounds(100, 200, 100, 100);


    customKnob.setBounds(250, 200, 128, 128);

    titleGlow.setBounds(0, 10, getWidth(), 60);


    waveEditor.setBounds(getWidth() / 2 + 20, 100, getWidth() / 2 - 40, getHeight() - 140);



}

//should be whats fitting the slider data to the global volume
void LFO2AudioProcessorEditor::sliderValueChanged(juce::Slider* slider) {
    if (slider == &midiVolume) {
        audioProcessor.globalVolume = (float)midiVolume.getValue();
    }
    else if (slider == &mixKnob) {
        audioProcessor.mix = (float)mixKnob.getValue();
    }
    else if (slider == &timeSlider)
    {
        int selection = (int)timeSlider.getValue();

        switch (selection)
        {
        case 1:
            audioProcessor.division = 16;
            timeValueLabel.setText("1/16", juce::dontSendNotification);
            break;
        case 2:
            audioProcessor.division = 4;
            timeValueLabel.setText("1/8", juce::dontSendNotification);
            break;
        case 3:
            audioProcessor.division = 1;
            timeValueLabel.setText("1/4", juce::dontSendNotification);
            break;
        case 4:
            audioProcessor.division = 0.5;
            timeValueLabel.setText("1/2", juce::dontSendNotification);  //not working 
            break;
        case 5:
            audioProcessor.division = 0.25;
            timeValueLabel.setText("1/4", juce::dontSendNotification);  //not working 
            break;
        }


        audioProcessor.lfo.setRate(audioProcessor.bpm, audioProcessor.division);
    }

    
}
