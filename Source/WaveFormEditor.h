#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include "GlowEffect.h"



class WaveformEditor : public juce::Component,
                       private juce::Timer
{
public:
    WaveformEditor()
    {
        points = { {0.0f, 0.5f}, {0.5f, 1.0f}, {1.0f, 0.5f} };
        segments.resize(std::max<size_t>(1, points.size() - 1));
        setOpaque(false); // change this to true if something getts hidden behind the line 
        startTimerHz(30);  
        setWantsKeyboardFocus(true);
        setMouseClickGrabsKeyboardFocus(false);


        //glow dot 
        // Setup the glowing dot
        addAndMakeVisible(dot);
        dot.setSize(10, 10);
        dot.setInterceptsMouseClicks(false, false);

        // attach glow effect to dot
        dotGlow = std::make_unique<GlowEffect>(
            &dot,
            juce::Colours::cyan,
            25.0f,
            true,
            GlowEffect::Mode::PulseIntensity,
            0.8f
            );
    }

    ~WaveformEditor() override { stopTimer(); }

    

    using UpdateCallback = std::function<void(const std::vector<float>&)>;
    void setUpdateCallback(UpdateCallback cb) { updateCallback = std::move(cb); }



    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black.darker(0.2f));
        g.setColour(juce::Colours::darkgrey);
        g.fillRect(getLocalBounds().reduced(6));

        auto area = getLocalBounds().reduced(10);
        drawGrid(g, area);

        // Build and draw path using quadratic segments (control point from tension)
        juce::Path p;
        if (points.size() >= 2)
        {
            auto p0 = toPixel(points.front());
            p.startNewSubPath(p0);

            const int VIS_SAMPLES = 256;
            for (int i = 1; i <= VIS_SAMPLES; ++i)
            {
                float t = (float)i / (float)VIS_SAMPLES;
                float v = sampleFromPoints(t);
                juce::Point<float> pt = toPixel({ t, v });
                p.lineTo(pt);
            }
        }

        g.setColour(juce::Colours::lime);
        g.strokePath(p, juce::PathStrokeType(2.0f));

        // draw control points
        for (size_t i = 0; i < points.size(); ++i)
        {
            auto pixel = toPixel(points[i]);
            g.setColour(juce::Colours::white);
            g.fillEllipse(pixel.x - 4.0f, pixel.y - 4.0f, 8.0f, 8.0f);
        }

        // draw segment control handles (visualize tension)
        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            float tension = segments[i].tension;
            if (std::abs(tension) > 0.001f)
            {
                auto p1 = toPixel(points[i]);
                auto p2 = toPixel(points[i + 1]);
                juce::Point<float> mid = (p1 + p2) * 0.5f;
                // visual offset: scale tension to pixels
                mid.y -= tension * 40.0f;
                g.setColour(juce::Colours::orange);
                g.fillEllipse(mid.x - 4.0f, mid.y - 4.0f, 8.0f, 8.0f);
            }
        }

        // highlighted handle
        if (selected >= 0 && selected < (int)points.size())
        {
            auto s = toPixel(points[selected]);
            g.setColour(juce::Colours::yellow);
            g.drawEllipse(s.x - 6.0f, s.y - 6.0f, 12.0f, 12.0f, 2.0f);
        }
        // highlight selected segment handle
        if (selectedSegment >= 0 && selectedSegment < (int)segments.size())
        {
            auto p1 = toPixel(points[selectedSegment]);
            auto p2 = toPixel(points[selectedSegment + 1]);
            juce::Point<float> mid = (p1 + p2) * 0.5f;
            mid.y -= segments[selectedSegment].tension * 40.0f;
            g.setColour(juce::Colours::yellow);
            g.drawEllipse(mid.x - 6.0f, mid.y - 6.0f, 12.0f, 12.0f, 2.0f);
        }


        


         //  Update glow dot position 
        float t = animationPhase; // normalized [0..1]
        float y = sampleFromPoints(t);
        juce::Point<float> pos = toPixel({ t, y });

        dot.setCentrePosition((int)pos.x, (int)pos.y);
        dot.repaint(); //force redraw 


    }

    void resized() override {}

    // Mouse: left-drag point, double-click to add point, right-click to delete point.
    // Click on line (not a point) to select a segment; drag vertically to change tension.
    void mouseDown(const juce::MouseEvent& e) override
    {
        auto pos = fromPixel(e.position);
        int idx = indexOfPointNear(e.position, hitRadius * getWidth()); 

        if (e.mods.isLeftButtonDown())
        {
            if (idx >= 0)
            {
                // start dragging a control point
                selected = idx;
                auto posPix = toPixel(pos);
                dragOffset = toPixel(points[idx]) - e.position;; // consistent with JUCE typing
            }
            else
            {
                // not clicking a point -> maybe add or start dragging segment
                if (e.getNumberOfClicks() == 2)
                {
                    addPointConstrained(pos);
                    selected = indexOfPointNear(toPixel(pos), hitRadius);
                    repaint();
                    // immediate update for instant visual/audio feedback
                    if (updateCallback)
                        updateCallback(createSampleBuffer(1024, true));
                }
                else
                {
                    int segIdx = indexOfTensionHandleNear(e.position, 8.0f); // for placing and editing orange curve lines 
                    if (segIdx >= 0)
                    {
                        selectedSegment = segIdx;
                        dragStartY = e.position.y;
                        initialTension = segments[segIdx].tension;
                    }
                    else
                    {
                        // fallback: click near line
                        segIdx = indexOfSegmentNear(pos, 0.03f);
                        if (segIdx >= 0)
                        {
                            selectedSegment = segIdx;
                            dragStartY = e.position.y;
                            initialTension = segments[segIdx].tension;
                        }
                    }
                }
            }
        }
        else if (e.mods.isRightButtonDown())
        {
            if (idx >= 0 && points.size() > 2) // keep at least 2 points
            {
                points.erase(points.begin() + idx);
                // keep segments synced
                segments.resize(std::max<size_t>(1, points.size() - 1));
                selected = -1;
                selectedSegment = -1;
                repaint();
                pushUpdate(); // push to audio via timer
            }
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        // If dragging a point
        if (selected >= 0 && selected < (int)points.size())  //should update now using pixel data
        {
            auto newPix = e.position + dragOffset;
            auto newNorm = fromPixelExact(newPix);

            float leftBound = (selected == 0) ? 0.0f : points[selected - 1].x + 0.001f;
            float rightBound = (selected == (int)points.size() - 1) ? 1.0f : points[selected + 1].x - 0.001f;

            newNorm.x = juce::jlimit(leftBound, rightBound, newNorm.x);
            newNorm.y = juce::jlimit(0.0f, 1.0f, newNorm.y);

            points[selected] = newNorm;

            repaint();
            pushUpdateDebounced();
        }
        // Else if dragging a segment's tension handle
        else if (selectedSegment >= 0 && selectedSegment < (int)segments.size())
        {
            float dy = (dragStartY - e.position.y) / (float)getHeight();
            float newT = juce::jlimit(-1.0f, 1.0f, initialTension + dy * 3.5f); //<----change last number to exxagerate curve: TENSION stardew
            if (std::abs(newT - segments[selectedSegment].tension) > 1e-4f)
            {
                segments[selectedSegment].tension = newT;
                // immediate visual feedback
                repaint();
                // push update to audio (debounced)
                pushUpdateDebounced();
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        selected = -1;
        selectedSegment = -1;

        repaint();
    }

    // create sampled buffer (N samples) from current control points (uses quadratic per-segment)
    std::vector<float> createSampleBuffer(int N = 512, bool smooth = false)
    {
        std::vector<float> out;
        out.resize(N);
        if (points.empty()) return out;
        for (int i = 0; i < N; ++i)
        {
            float t = (float)i / (float)N;
            out[i] = sampleFromPoints(t);
        }

        if (smooth)
        {
            // simple smoothing: 3-tap moving average (non destructive, light)
            std::vector<float> tmp(N);
            for (int i = 0; i < N; ++i)
                tmp[i] = (out[(i - 1 + N) % N] + out[i] + out[(i + 1) % N]) / 3.0f;
            out.swap(tmp);
        }

        // ensure samples in 0..1
        for (auto& v : out) v = juce::jlimit(0.0f, 1.0f, v);
        return out;
    }




    // Change waveforms to common shapes with dropdwon menu 
    void setPresetWaveform(const std::string& shape)
    {
        selected = -1;
        selectedSegment = -1;
        pendingUpdate = false;


        points.clear();

        const int N = 128; // resolution for curve
        if (shape == "Sine")
        {
            for (auto& seg : segments) seg.tension = 0.0f; //should reset tension after preset change

            points = {
        {0.0f, 0.5f},   
        {0.25f, 1.0f},  
        {0.5f, 0.5f},   
        {0.75f, 0.0f},  
        {1.0f, 0.5f}    
            };

            // Should add curvature to the sin 
            segments.resize(points.size() - 1);
            for (size_t i = 0; i < segments.size(); ++i)
            {
                segments[i].tension = (i < 2) ? 0.5f : -0.5f;   // positive curve on first half, negative on second
            } 
        }
        else if (shape == "Saw")
        {
            points = { {0.0f, 0.0f} ,{0.03f, 1.0f}, {1.0f, 0.0f} };
        }
        else if (shape == "Triangle")
        {
            points = { {0.0f, 0.0f}, {0.5f, 1.0f}, {1.0f, 0.0f} };
        }
        else if (shape == "Square")
        {
            points = { {0.0f, 1.0f}, {0.5f, 1.0f}, {0.5f, 0.0f}, {1.0f, 0.0f} };
        }

        segments.resize(std::max<size_t>(1, points.size() - 1));

        if (shape != "Sine") {
            for (auto& seg : segments) seg.tension = 0.0f; //should reset tension after preset change
        }

        repaint();

        // Sends immediate update to the LFO
        if (updateCallback)
            updateCallback(createSampleBuffer(1024, true));
    }


    void setAnimationSpeed(float hz) {
        animationSpeed = hz;
    }


    //==========================================================================================================
private:
    struct P { float x, y; };
    std::vector<P> points;

    struct Segment { float tension = 0.0f; };
    std::vector<Segment> segments;

    int selected = -1;
    juce::Point<float> dragOffset{ 0.0f,0.0f }; //point typing 
    const float hitRadius = 0.03f; // normalized coords

    // segment selection / dragging
    int selectedSegment = -1;
    float dragStartY = 0.0f;
    float initialTension = 0.0f;

    UpdateCallback updateCallback;
    bool pendingUpdate = false;


    //for dot animation along line
    float animationPhase = 0.0f;
    float animationSpeed = 1.0f;

    GlowDot dot;
    std::unique_ptr<GlowEffect> dotGlow;

    // normalized -> pixel and back
    juce::Point<float> toPixel(const P& p) const
    {
        auto r = getLocalBounds().toFloat().reduced(10.0f);
        return { r.getX() + p.x * r.getWidth(),
                 r.getY() + (1.0f - p.y) * r.getHeight() };
    }

    P fromPixel(const juce::Point<float>& pt) const
    {
        auto r = getLocalBounds().toFloat().reduced(10.0f);
        float nx = (pt.x - r.getX()) / r.getWidth();
        float ny = 1.0f - ((pt.y - r.getY()) / r.getHeight());
        return { nx, ny };
    }
    P fromPixelExact(const juce::Point<float>& pt) const //not sure if both are needed they are similar ^
    {
        auto r = getLocalBounds().toFloat().reduced(10.0f);
        return { (pt.x - r.getX()) / r.getWidth(),
                 1.0f - ((pt.y - r.getY()) / r.getHeight()) };
    }

    // hit test for points
    int indexOfPointNear(juce::Point<float> pixelPos, float tol) const
    {
        for (size_t i = 0; i < points.size(); ++i)
        {
            auto pPix = toPixel(points[i]);
            if (pPix.getDistanceFrom(pixelPos) < tol)
                return (int)i;
        }
        return -1;
    }

    // improved hit test for segments: distance from normalized point to normalized line segment
    int indexOfSegmentNear(const P& p, float tol) const
    {
        if (points.size() < 2) return -1;
        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            P a = points[i];
            P b = points[i + 1];
            // project p onto segment ab in normalized coordinates
            float vx = b.x - a.x;
            float vy = b.y - a.y;
            float wx = p.x - a.x;
            float wy = p.y - a.y;
            float vlen2 = vx * vx + vy * vy;
            if (vlen2 <= 1e-8f) continue;
            float t = (wx * vx + wy * vy) / vlen2;
            t = juce::jlimit(0.0f, 1.0f, t);
            float projx = a.x + t * vx;
            float projy = a.y + t * vy;
            float dx = projx - p.x;
            float dy = projy - p.y;
            if (std::sqrt(dx * dx + dy * dy) < tol) return (int)i;
        }
        return -1;
    }

    int indexOfTensionHandleNear(juce::Point<float> pixelPos, float tol) const  //for attatching edits to orange points 
    {
        if (points.size() < 2) return -1;
        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            auto p1 = toPixel(points[i]);
            auto p2 = toPixel(points[i + 1]);
            juce::Point<float> mid = (p1 + p2) * 0.5f;
            mid.y -= segments[i].tension * 40.0f; 

            if (mid.getDistanceFrom(pixelPos) < tol)
                return (int)i;
        }
        return -1;
    }



    // add point and keep segments synced
    void addPointConstrained(const P& p)
    {
        points.push_back(p);
        std::sort(points.begin(), points.end(), [](const P& a, const P& b) { return a.x < b.x; });
        segments.resize(std::max<size_t>(1, points.size() - 1));
    }

    // sample using quadratic bezier per segment using tension -> control point
    float sampleFromPoints(float t) const
    {
        if (points.empty()) return 0.5f;
        if (t <= points.front().x) return points.front().y;
        if (t >= points.back().x) return points.back().y;

        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            if (t >= points[i].x && t <= points[i + 1].x)
            {
                float segT = (t - points[i].x) / (points[i + 1].x - points[i].x);
                float y1 = points[i].y;
                float y2 = points[i + 1].y;
                float tension = (i < segments.size()) ? segments[i].tension : 0.0f;

                // controlY near midpoint adjusted by tension
                //float controlY = juce::jlimit(0.0f, 1.0f, 0.5f * (y1 + y2) + tension * 0.25f);
                float controlY = juce::jlimit(0.0f, 1.0f, 0.5f * (y1 + y2) + tension * 0.5f);  //<------change first number for change in tension stardew 


                float inv = 1.0f - segT;
                float y = inv * inv * y1 + 2.0f * inv * segT * controlY + segT * segT * y2;
                return y;
            }
        }
        return points.back().y;
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour(juce::Colours::grey.withAlpha(0.25f));
        for (int i = 0; i <= 4; ++i)
        {
            float x = area.getX() + i * (area.getWidth() / 4.0f);
            g.drawVerticalLine((int)x, (float)area.getY(), (float)area.getBottom());
            float y = area.getY() + i * (area.getHeight() / 4.0f);
            g.drawHorizontalLine((int)y, (float)area.getX(), (float)area.getRight());
        }
    }

    // update push helpers
    void pushUpdate()
    {
        pendingUpdate = true;
    }

    void pushUpdateDebounced()
    {
        pushUpdate();
    }

    void timerCallback() override
    {
        if (pendingUpdate && updateCallback)
        {
            pendingUpdate = false;
            auto buf = createSampleBuffer(1024, true);
            updateCallback(buf);
        }


        const float dt = 1.0f / 30.0f;  // timer is at 30 Hz
        animationPhase = std::fmod(animationPhase + animationSpeed * dt, 1.0f);

        repaint();
        dot.repaint();


    }


};
