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


    // Load font once
    auto typeface = juce::Typeface::createSystemTypefaceFor(  //load font just once 
        BinaryData::AudiowideRegular_ttf,
        BinaryData::AudiowideRegular_ttfSize
    );
    titleFont = juce::Font(typeface);
    titleFont.setHeight(40.0f);
    //titleFont.setBold(true); // change for bold 


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
    timeSlider.setRange(1, 5, 1); 
    timeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 20);
    timeSlider.setValue(1); // start on 1/16th
    timeSlider.addListener(this);   
    addAndMakeVisible(timeSlider);



    //switch between rate modes 
    // -----------------------------------------------------
    bpmButton.setClickingTogglesState(true);
    hzButton.setClickingTogglesState(true);
    bpmHzButton.setClickingTogglesState(true);

    bpmButton.setLookAndFeel(&customLAF);
    hzButton.setLookAndFeel(&customLAF);
    bpmHzButton.setLookAndFeel(&customLAF);

    
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

            // Set up slider
            timeSlider.setRange(0.1, 15.0, 0.01);
            timeSlider.setValue(hz, juce::dontSendNotification);
            timeValueLabel.setText(juce::String(hz, 2) + " Hz", juce::dontSendNotification);

            // Actually set the LFO to Hz rate
            audioProcessor.currentHz = hz;
            audioProcessor.lfo.setRateHz(hz);
            waveEditor.setAnimationSpeed(hz);
        }
    };

    bpmHzButton.onClick = [this]() {
        if (bpmHzButton.getToggleState())
        {
            float hz = (audioProcessor.bpm / 60.0f) / audioProcessor.division;

            bpmButton.setToggleState(false, juce::dontSendNotification);
            hzButton.setToggleState(false, juce::dontSendNotification);
            audioProcessor.currentMode = LFO2AudioProcessor::RateMode::BPM_HZ;

            timeSlider.setRange(0.1, 15.0, 0.01);
            timeSlider.setValue(audioProcessor.lfo.getRateHz(), juce::dontSendNotification);
            timeValueLabel.setText(juce::String(hz, 2) + " Hz", juce::dontSendNotification);  //TODO NOT WORKING 

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
    mixKnob.setPopupDisplayEnabled(false, false, this);
    mixKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    mixKnob.setTextValueSuffix(" Mix");
    mixKnob.addListener(this);
    addAndMakeVisible(mixKnob);



    // Load and show the custom knob
    juce::File knobFile("C:/Users/Michael/Desktop/Cronos/Cronos-/Pictures/Knobs/test.png");// path for now
    knobImg = juce::ImageFileFormat::loadFrom(knobFile);

    customKnob.setKnobImage(knobImg, 128); 
    customKnob.setRange(0.0, 1.0, 0.01);
    //addAndMakeVisible(customKnob);   //custom knob dont display for now


    addAndMakeVisible(waveEditor);
    // When editor updates waveform, push it into audio LFO (audioProcessor.lfo)
    waveEditor.setUpdateCallback([this](const std::vector<float>& buf)
        {
            // This is GUI thread calling into processor — safe because LFO uses CriticalSection
            audioProcessor.lfo.setCustomWaveform(buf);
        });


    // Text above LFO selector 
    lfoShapeLabel.setText("LFO Shape", juce::dontSendNotification);
    lfoShapeLabel.setFont(titleFont.withHeight(20.0f));
    lfoShapeLabel.setJustificationType(juce::Justification::left); //geting it on left of page thing
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


    //logo stuff 

    auto logo = juce::ImageCache::getFromMemory(BinaryData::cronosLogo_png, BinaryData::cronosLogo_pngSize); 
    logoImage.setImage(logo);
    addAndMakeVisible(logoImage);


    //bpmButton.


}



LFO2AudioProcessorEditor::~LFO2AudioProcessorEditor() {

    bpmButton.setLookAndFeel(nullptr);  //supposed to clean look and feel shit
    hzButton.setLookAndFeel(nullptr);
    bpmHzButton.setLookAndFeel(nullptr);

}


//==============================================================================
void LFO2AudioProcessorEditor::paint (juce::Graphics& g) //paint is called very often so dont put anything crazy in here 
{

    
    
    juce::Colour backgroundColour = juce::Colour(63, 63, 68);
    g.fillAll(backgroundColour); //nice blue 82,255, 184 kinda glowy 


    //rectangle initialization 
    g.setColour (juce::Colour(33, 247, 176));
    g.setFont(titleFont.withHeight(60.0f));  //.withHeight can change the overall size of the font
    auto titleArea = juce::Rectangle<int>(55, 7.5, 300, 50);
    g.drawFittedText ("Chronos", titleArea, juce::Justification::top, 1);


    const float cornerSize = 10.0f; // Adjust this for desired corner radius
    juce::Rectangle<float> area(95.0f, 85.0f, 300.0f, 200.0f); // Or define your own rectangle

    g.setColour(juce::Colour(48,48,54)); 
    g.fillRoundedRectangle(area, cornerSize);

    //gradient
    juce::Colour edge1 = juce::Colour::fromRGB(35, 247, 176);  // bright neon
    juce::Colour edge2 = juce::Colour::fromRGB(63, 63, 68);    // darker edge

    juce::ColourGradient borderGradient(
        edge1,    
        area.getRight(), area.getBottom(), // start colour           
        edge2,                              // end colour
        area.getX(), area.getY(),          // gradient end point (bottom-right)
        false                               // not radial
    );

    borderGradient.addColour(0.5, edge1.brighter()); // mid-shimmer optional

    g.setGradientFill(borderGradient);

    float borderThickness = 3.0f;
    g.drawRoundedRectangle(area, cornerSize, borderThickness);

    //-------------------------
    //rectangle 2 

    juce::Colour insideColour = juce::Colour(48, 48, 54);   // light-ish grey (same as rect 1)
    g.setColour(insideColour);
    
    const float cornerSize2 = 10.0f;
    juce::Rectangle<float> area2(95.0f, 350.0f, 300.0f, 200.0f);

    g.fillRoundedRectangle(area2, cornerSize2);

    juce::Colour edge1b = juce::Colour::fromRGB(35, 247, 176);
    juce::Colour edge2b = juce::Colour::fromRGB(63, 63, 68);

    juce::ColourGradient borderGradient2(
        edge1b,
        area2.getRight(), area2.getBottom(),
        edge2b,
        area2.getX(), area2.getY(),
        false
    );

    borderGradient2.addColour(0.5, edge1b.brighter());
    g.setGradientFill(borderGradient2);

    float borderThickness2 = 3.0f;
    g.drawRoundedRectangle(area2, cornerSize2, borderThickness2);

    //-------------------------




    // Draw button glows **behind the buttons**
    auto drawGlowBehind = [&](juce::TextButton& button, juce::Colour glowColour)
    {
        if (button.getToggleState())
        {
            auto bounds = button.getBounds().toFloat().expanded(4.0f); // expand a little
            juce::DropShadow shadow(glowColour.withAlpha(1.5f), 25, juce::Point<int>(0, 0));
            shadow.drawForRectangle(g, bounds.getSmallestIntegerContainer());
        }
    };

    drawGlowBehind(bpmButton, juce::Colour(35, 247, 176));
    drawGlowBehind(hzButton, juce::Colour(35, 247, 176));
    drawGlowBehind(bpmHzButton, juce::Colour(35, 247, 176));




    // In your component's constructor or a relevant method
    timeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(35, 247, 176)); // Changes the color of the slider's thumb
    //timeSlider.setColour(juce::Slider::trackColourId, juce::Colours::green); // Changes the color of the slider's track
    timeSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(10,10,10)); // For rotary sliders
    timeSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(87, 87, 96)); // General background
    

    timeLabel.setColour(juce::Label::textColourId, juce::Colours::darkgrey);
    timeValueLabel.setFont(titleFont.withHeight(30.0f));
    timeValueLabel.setColour(juce::Label::textColourId, juce::Colour(35, 247, 176));

    mixKnob.setColour(juce::Slider::thumbColourId, juce::Colour(35, 247, 176)); // Thumb
    mixKnob.setColour(juce::Slider::trackColourId, juce::Colour(196, 253, 234)); // Background track 
    mixKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(10, 10, 10)); // For rotary sliders
    mixKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(87, 87, 96));
    


    //combobox == dropdown
    lfoShapeSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(44, 44, 49));
    lfoShapeSelector.setColour(juce::ComboBox::textColourId, juce::Colour(142, 230, 179));
    lfoShapeSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(35, 247, 176));


    lfoShapeLabel.setColour(juce::Label::textColourId, juce::Colour(35,247,176));
    //lfoShapeLabel.setColour(juce::Label::outlineColourId, juce::Colour(142, 230, 179));


   
    bpmButton.setColour(juce::TextButton::textColourOffId, juce::Colour(35, 247, 176));
    bpmButton.setColour(juce::TextButton::textColourOnId, juce::Colour(63, 63, 68));
    bpmButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(35, 247, 176));
    bpmButton.setColour(juce::TextButton::buttonColourId, juce::Colour(44, 44, 49));
    //bpmButton.setColour(juce::TextButton::outline, juce::Colour(44, 44, 49));

    hzButton.setColour(juce::TextButton::textColourOffId, juce::Colour(35, 247, 176));
    hzButton.setColour(juce::TextButton::textColourOnId, juce::Colour(63, 63, 68));
    hzButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(35, 247, 176));
    hzButton.setColour(juce::TextButton::buttonColourId, juce::Colour(44, 44, 49));

    bpmHzButton.setColour(juce::TextButton::textColourOffId, juce::Colour(35, 247, 176));
    bpmHzButton.setColour(juce::TextButton::textColourOnId, juce::Colour(63, 63, 68));
    bpmHzButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(35, 247, 176));
    bpmHzButton.setColour(juce::TextButton::buttonColourId, juce::Colour(44, 44, 49));



    if (audioProcessor.currentMode == LFO2AudioProcessor::RateMode::BPM_HZ)
    {
        if (timeValueLabel.getText().contains("/"))
            timeValueLabel.setColour(juce::Label::textColourId, juce::Colour(238, 99, 82));
        else
            timeValueLabel.setColour(juce::Label::textColourId, juce::Colour(35, 247, 176));
    }

}



void LFO2AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    midiVolume.setBounds(40, 100, 20, getHeight()-60);

    timeSlider.setBounds(100, 120, 100, 100);
    auto timeSliderBounds = timeSlider.getBounds(); //getting bounds for labels underneath 

    timeValueLabel.setBounds(timeSliderBounds.getX(),timeSliderBounds.getY() - 20,timeSliderBounds.getWidth(),20);
    timeLabel.setBounds(timeSliderBounds.getX(),timeSliderBounds.getBottom(),timeSliderBounds.getWidth(),20);

    mixKnob.setBounds(getWidth() -100, 10, 100, 100);


    customKnob.setBounds(250, 200, 128, 128);



    waveEditor.setBounds(getWidth() / 2 + 20, 125, getWidth() / 2 - 40, getHeight() - 140);

    //dropdown
    auto topArea = getLocalBounds().removeFromTop(80);
    lfoShapeLabel.setBounds(getWidth() / 2 +10, 60, 300, 30);
    lfoShapeSelector.setBounds(getWidth() / 2 + 15, 90, 100, 25);


    //rate to hz 
    int buttonWidth = 50, buttonHeight = 25;
    bpmButton.setBounds(210, 110, buttonWidth, buttonHeight);
    hzButton.setBounds(210, 140, buttonWidth, buttonHeight);
    bpmHzButton.setBounds(210, 170, buttonWidth, buttonHeight);


    logoImage.setBounds(-10, 5, 80, 80);

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
           //audioProcessor.lfo.setRate(bpm, (bpm / 60.0f) / hzValue);
           audioProcessor.currentHz = pos;
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


