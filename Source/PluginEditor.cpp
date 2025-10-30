/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LFO.h"
#include "WaveFormEditor.h"



WaveformEditor waveEditor;  
//GlowEffect volumeGlow;
//==============================================================================


LFO2AudioProcessorEditor::LFO2AudioProcessorEditor (LFO2AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    volumeGlow(&midiVolume, juce::Colours::cyan, 25.0f, true, GlowEffect::Mode::HueCycle, 1.5f)
{
    startTimerHz(30);
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

    volumeGlow.setTarget(&midiVolume); // adds glow 


    //time division slider 
    timeSlider.setSliderStyle(juce::Slider::Rotary);
    timeSlider.setRange(1, 5, 1); // three discrete positions
    timeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 20);
    timeSlider.setValue(1); // start on 1/16th
    timeSlider.addListener(this);   
    addAndMakeVisible(timeSlider);



    //switch between rate modes 
    // -----------------------------------------------------
    bpmButton.setClickingTogglesState(true);
    hzButton.setClickingTogglesState(true);
    bpmHzButton.setClickingTogglesState(true);

    
    // BPM starts active
    bpmButton.setToggleState(true, juce::dontSendNotification);

    auto setupButton = [&](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(104, 176, 171));
        b.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        b.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
        b.setClickingTogglesState(true);
        addAndMakeVisible(b);
    };

    setupButton(bpmButton);
    setupButton(hzButton);
    setupButton(bpmHzButton);

    // --- Button logic ---
    bpmButton.onClick = [this]() {
        if (bpmButton.getToggleState())
        {
            hzButton.setToggleState(false, juce::dontSendNotification);
            bpmHzButton.setToggleState(false, juce::dontSendNotification);
            audioProcessor.currentMode = LFO2AudioProcessor::RateMode::BPM;

            // Convert Hz → division
            float currentHz = audioProcessor.lfo.getRateHz();
            float bpm = audioProcessor.bpm;
            float division = (bpm / 60.0f) / currentHz;
            audioProcessor.division = division;
            updateTimeSliderFromDivision();
        }
    };

    hzButton.onClick = [this]() {
        if (hzButton.getToggleState())
        {
            bpmButton.setToggleState(false, juce::dontSendNotification);
            bpmHzButton.setToggleState(false, juce::dontSendNotification);
            audioProcessor.currentMode = LFO2AudioProcessor::RateMode::HZ;


            float hz = (audioProcessor.bpm / 60.0f) / audioProcessor.division;
            timeSlider.setRange(0.1, 10.0, 0.01);
            timeSlider.setValue(hz, juce::dontSendNotification);
            timeValueLabel.setText(juce::String(hz, 2) + " Hz", juce::dontSendNotification);

            audioProcessor.lfo.setRate(audioProcessor.bpm, audioProcessor.division);
            waveEditor.setAnimationSpeed(audioProcessor.lfo.getRateHz());
        }
    };

    bpmHzButton.onClick = [this]() {
        if (bpmHzButton.getToggleState())
        {
            bpmButton.setToggleState(false, juce::dontSendNotification);
            hzButton.setToggleState(false, juce::dontSendNotification);
            audioProcessor.currentMode = LFO2AudioProcessor::RateMode::BPM_HZ;

            timeSlider.setRange(0.0, 1.0, 0.0001);
            timeSlider.setValue(hzToPosition(audioProcessor.lfo.getRateHz(), audioProcessor.bpm),juce::dontSendNotification);
        }
    };

    //----------------------------------------------------------




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


    // Text above LFO selector 
    lfoShapeLabel.setText("LFO Shape", juce::dontSendNotification);
    lfoShapeLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    lfoShapeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lfoShapeLabel);

    //LFO selector dropdown 
    lfoShapeSelector.addItem("Saw", 1);
    lfoShapeSelector.addItem("Sine", 2);
    lfoShapeSelector.addItem("Triangle", 3);
    lfoShapeSelector.addItem("Square", 4);
    //lfoShapeSelector.addItem("Custom", 5);
    lfoShapeSelector.setSelectedId(1); // default to saw for now 
    lfoShapeSelector.onChange = [this]()
    {
        int selected = lfoShapeSelector.getSelectedId();
        std::string shapeName;
        LFO::Shape shape = LFO::Shape::Saw;

        switch (selected)
        {
        case 1: shape = LFO::Shape::Saw; shapeName = "Saw"; break;
        case 2: shape = LFO::Shape::Sine; shapeName = "Sine"; break;
        case 3: shape = LFO::Shape::Triangle; shapeName = "Triangle"; break;
        case 4: shape = LFO::Shape::Square; shapeName = "Square"; break;
        //case 5: shape = LFO::Shape::Custom; break;
        }

        audioProcessor.lfo.setShape(shape);

        waveEditor.setPresetWaveform(shapeName);
    };
    addAndMakeVisible(lfoShapeSelector);





}



LFO2AudioProcessorEditor::~LFO2AudioProcessorEditor() {
}


//==============================================================================
void LFO2AudioProcessorEditor::paint (juce::Graphics& g) //paint is called very often so dont put anything crazy in here 
{

    juce::Colour backgroundColour = juce::Colour(47, 62, 70);
    g.fillAll(backgroundColour);

    g.setColour (juce::Colour(202, 210, 197));
    g.setFont (juce::FontOptions (30.0f));
    g.drawFittedText ("Chronos", getLocalBounds(), juce::Justification::top, 1);




    // Draw button glows **behind the buttons**
    auto drawGlowBehind = [&](juce::TextButton& button, juce::Colour glowColour)
    {
        if (button.getToggleState())
        {
            auto bounds = button.getBounds().toFloat().expanded(4.0f); // expand a little
            juce::DropShadow shadow(glowColour.withAlpha(0.8f), 25, juce::Point<int>(0, 0));
            shadow.drawForRectangle(g, bounds.getSmallestIntegerContainer());
        }
    };

    drawGlowBehind(bpmButton, juce::Colour(82, 121, 111));
    drawGlowBehind(hzButton, juce::Colour(82, 121, 111));
    drawGlowBehind(bpmHzButton, juce::Colour(82, 121, 111));




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


    //combobox == dropdown
    lfoShapeSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
    lfoShapeSelector.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    lfoShapeSelector.setColour(juce::ComboBox::outlineColourId, juce::Colours::lime);


    if (audioProcessor.currentMode == LFO2AudioProcessor::RateMode::BPM_HZ)
    {
        if (timeValueLabel.getText().contains("/"))
            timeValueLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
        else
            timeValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }

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

    //dropdown
    auto topArea = getLocalBounds().removeFromTop(80);
    lfoShapeLabel.setBounds(getWidth() / 2 +30, 60, 100, 20);
    lfoShapeSelector.setBounds(getWidth() / 2 + 30, 80, 100, 25);


    //rate to hz 
    int buttonWidth = 50, buttonHeight = 25;
    bpmButton.setBounds(210, 100, buttonWidth, buttonHeight);
    hzButton.setBounds(210, 130, buttonWidth, buttonHeight);
    bpmHzButton.setBounds(210, 160, buttonWidth, buttonHeight);

}

//should be whats fitting the slider data to the global volume
void LFO2AudioProcessorEditor::sliderValueChanged(juce::Slider* slider) {
    if (slider == &midiVolume) {
        audioProcessor.globalVolume = (float)midiVolume.getValue();
    }
    else if (slider == &mixKnob) {
        audioProcessor.mix = (float)mixKnob.getValue(), juce::dontSendNotification;
    }
    else if (slider == &timeSlider)
    {
        
       if (audioProcessor.currentMode == LFO2AudioProcessor::RateMode::BPM) {
       
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
            timeValueLabel.setText("1/2", juce::dontSendNotification);  
            break;
        case 5:
            audioProcessor.division = 0.25;
            timeValueLabel.setText("1/4", juce::dontSendNotification);  
            break;
        }
        audioProcessor.lfo.setRate(audioProcessor.bpm, audioProcessor.division);
        float newRateHz = audioProcessor.getLFORateHz();
        waveEditor.setAnimationSpeed(newRateHz);
        }
    
    else if (audioProcessor.currentMode == LFO2AudioProcessor::RateMode::HZ) //switching between buttons 
    {
        float hz = (float)timeSlider.getValue();
        timeValueLabel.setText(juce::String(hz, 2) + " Hz", juce::dontSendNotification);
        audioProcessor.lfo.setSampleRate(audioProcessor.getSampleRate());
        
        audioProcessor.currentHz = hz;  
        audioProcessor.currentMode = LFO2AudioProcessor::RateMode::HZ;

        waveEditor.setAnimationSpeed(hz);
    } 
    else if (audioProcessor.currentMode == LFO2AudioProcessor::RateMode::BPM_HZ) //bpm/hz mode 
       {
           float pos = (float)timeSlider.getValue();   
           float bpm = audioProcessor.bpm;

           float snapThreshold = 0.02f; // Tweak for lock in feature higher is bigger lock/ snap 
           float snappedPos = pos;
           bool snapped = false;
           juce::String closestLabel;

           for (auto& [divisionPos, divisionVal] : divisionPositions)
           {
               if (std::abs(pos - divisionPos) < snapThreshold)
               {
                   snappedPos = divisionPos;
                   snapped = true;
                   
                   for (auto& div : divisions) // need to find closest label
                   {
                       if (div.division == divisionVal)
                       {
                           closestLabel = div.name;
                           break;
                       }
                   }

                   break;
               }
           }

           if (snapped)
               timeSlider.setValue(snappedPos, juce::dontSendNotification);

           // convert to hz 
           float hzValue = positionToHz(snappedPos, bpm);

           //Update LFO and animation speed
           audioProcessor.lfo.setRate(bpm, (bpm / 60.0f) / hzValue);
           waveEditor.setAnimationSpeed(hzValue);

           // label updating 
           if (snapped)
               timeValueLabel.setText(closestLabel, juce::dontSendNotification);
           else
               timeValueLabel.setText(juce::String(hzValue, 2) + " Hz", juce::dontSendNotification);
       }

   }
}

void LFO2AudioProcessorEditor::timerCallback()
{
    float lfoRateHz = audioProcessor.getLFORateHz();  // expose a getter from your processor
    waveEditor.setAnimationSpeed(lfoRateHz);

    repaint();

}


//helper for updating slider into BPM

void LFO2AudioProcessorEditor::updateTimeSliderFromDivision()
{
    if (audioProcessor.currentMode == LFO2AudioProcessor::RateMode::BPM)
    {
        timeSlider.setRange(1, 5, 1);

        float division = audioProcessor.division;
        int sliderValue = 1;
        juce::String text = "1/16";

        if (division == 16) { sliderValue = 1; text = "1/16"; }
        else if (division == 4) { sliderValue = 2; text = "1/8"; }
        else if (division == 1) { sliderValue = 3; text = "1/4"; }
        else if (division == 0.5) { sliderValue = 4; text = "1/2"; }
        else if (division == 0.25) { sliderValue = 5; text = "1/1"; }

        timeSlider.setValue(sliderValue, juce::dontSendNotification);
        timeValueLabel.setText(text, juce::dontSendNotification);
    }
}



float LFO2AudioProcessorEditor::positionToHz(float pos, float bpm) //conversion helper functions 
{
    auto lower = divisionPositions.lower_bound(pos);
    if (lower == divisionPositions.begin())
        return bpmDivisionToHz(bpm, lower->second);
    if (lower == divisionPositions.end())
        return bpmDivisionToHz(bpm, std::prev(lower)->second);

    auto upper = lower--;
    float x1 = lower->first, x2 = upper->first;
    float d1 = lower->second, d2 = upper->second;

    float t = (pos - x1) / (x2 - x1);

    // Interpolate logarithmically between divisions
    float hz1 = bpmDivisionToHz(bpm, d1);
    float hz2 = bpmDivisionToHz(bpm, d2);
    float hz = hz1 * std::pow(hz2 / hz1, t);

    return hz;
}

float LFO2AudioProcessorEditor::hzToPosition(float hz, float bpm)
{
    std::vector<float> positions;
    for (auto& [p, div] : divisionPositions)
        positions.push_back(p);

    for (int i = 0; i < positions.size() - 1; ++i)
    {
        float hz1 = bpmDivisionToHz(bpm, divisionPositions.at(positions[i]));
        float hz2 = bpmDivisionToHz(bpm, divisionPositions.at(positions[i + 1]));
        if (hz >= hz1 && hz <= hz2)
        {
            float t = std::log(hz / hz1) / std::log(hz2 / hz1);
            return juce::jmap(t, 0.0f, 1.0f, positions[i], positions[i + 1]);
        }
    }

    return 0.0f;
}


