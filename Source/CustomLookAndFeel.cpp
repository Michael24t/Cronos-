#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    // Base button theme
    setColour(juce::TextButton::buttonColourId, juce::Colour(44, 44, 49));   // Fill
    setColour(juce::TextButton::textColourOffId, juce::Colour(35, 247, 176));
    setColour(juce::TextButton::textColourOnId, juce::Colour(63, 63, 68));
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font("Audiowide", buttonHeight * 0.5f, juce::Font::plain);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto baseColour = backgroundColour;

    // Slight brighten for hover or press
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.brighter(0.15f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.08f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Determine outline color and thickness
    juce::Colour inactiveOutline = juce::Colour(35, 247, 176);  // teal
    juce::Colour activeOutline = juce::Colour(44, 44, 49);    // dark grey

    bool isToggled = button.getToggleState();
    float outlineThickness = isToggled ? 3.0f : 1.5f;
    auto outlineColour = isToggled ? activeOutline : inactiveOutline;

    g.setColour(outlineColour);
    g.drawRoundedRectangle(bounds, 6.0f, outlineThickness);
}