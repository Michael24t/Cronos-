#pragma once
#include <JuceHeader.h>

class LFO
{
public:
    void setSampleRate(double sr) { sampleRate = sr; }
    void setRate(float bpm, int division)
    {
        // One beat = 60 / bpm seconds
        float beatsPerSecond = bpm / 60.0f;

        // Example divisions: 1 = whole note, 4 = quarter, 16 = sixteenth
        if (division == 1) {
            rateHz = beatsPerSecond;
            fakeRate = 3;
        }          
        else if (division == 4) {
            rateHz = beatsPerSecond * 4.0f;
            fakeRate = 2;
        }         
        else if (division == 16) {
            rateHz = beatsPerSecond * 16.0f;
            fakeRate = 1;
        }           
    }
    
    void reset() { phase = 0.0f; }

    float getNextSample()
    {
        // Sawtooth from 0.0 → 1.0
        float value = phase;

        phase += rateHz / sampleRate;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return value;
    }

private:
    double sampleRate = 44100.0;
    float rateHz = 1.0f;
    int fakeRate = 1; //1= 1/16 2= 1/8 3=1/1
    float phase = 0.0f;
};
