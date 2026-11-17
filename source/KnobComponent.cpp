/*
  ==============================================================================

  This file is part of Clean Glides.
  Copyright (C) 2026 AbriCroco

  Clean Glides is free software: you can redistribute it and/or modify it
  under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  You should have received a copy of the GNU Affero General Public License
  along with this program. If not, see https://www.gnu.org/licenses/.

  ==============================================================================
*/

#include "KnobComponent.h"

KnobComponent::KnobComponent(const juce::String& text,
                             juce::AudioProcessorValueTreeState& apvts,
                             const juce::ParameterID& parameterID) :
    attachment(apvts, parameterID.getParamID(), *this)
{
    setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, true, getWidth(), 14);
    setLookAndFeel(&lnf);

    label.setText(text, juce::NotificationType::dontSendNotification);
    label.setJustificationType(juce::Justification::horizontallyCentred);
    label.setColour(juce::Label::textColourId, Colours::text);
    addAndMakeVisible(label);
}

KnobComponent::~KnobComponent()
{
    setLookAndFeel(nullptr);
}

void KnobComponent::paint(juce::Graphics& g)
{
    // We want our custom knob style to be applied so our first step is to do so.
    const juce::Rectangle<int> bounds{knobArea.toNearestInt()};
    const juce::Slider::RotaryParameters params = getRotaryParameters();
    getLookAndFeel().drawRotarySlider(
        g, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
        static_cast<float>(valueToProportionOfLength(getValue())),
        params.startAngleRadians, params.endAngleRadians, *this
    );

    // We define our arcs start and end angles in radians.
    // Note that the end needs to be greater than the start, which is why we add a whole rotation (2 pi).
    const float startAng{juce::degreesToRadians(180.f + 40.f)};
    const float endAng{juce::degreesToRadians(180.f - 40.f) + juce::MathConstants<float>::twoPi};

    // We set the thickness using our hover amount as we want the level meter thickness to change as we hover over it.
    const float thickness{juce::jmap(hoverAmount, knobArea.getWidth() * 0.03f, knobArea.getWidth() * 0.06f)};

    // As we will set an outline, we need to slightly reduce the level bounds to avoid clipping.
    const juce::Rectangle<float> levelBounds{knobArea.reduced(thickness * 0.6f)};

    // We define the arc and add it to our path.
    juce::Path levelSource;
    levelSource.addArc(levelBounds.getX(), levelBounds.getY(), levelBounds.getWidth(), levelBounds.getHeight(), startAng, endAng, true);

    // We need to define another path to create and set into the arc with our wished thickness.
    juce::Path levelOutline;
    juce::PathStrokeType strokeType{thickness * 1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded};

    // We create the outline and draw it using a slightly bigger thickness.
    strokeType.createStrokedPath(levelOutline, levelSource, {}, 1.f);

    g.setColour(Colours::outline);
    g.fillPath(levelOutline);

    // We change the thickness back and draw the empty level meter on top of the outline.
    strokeType.setStrokeThickness(thickness);
    strokeType.createStrokedPath(levelOutline, levelSource, {}, 1.f);

    g.setColour(Colours::emptyFill);
    g.fillPath(levelOutline);

    // Below we get the value of the knob and set the style for drawing path outlines to flat.
    const float proportion{static_cast<float>(valueToProportionOfLength(getValue()))};

    strokeType.setEndStyle(juce::PathStrokeType::butt);

    // We save the graphic state and change the clip region to only fill the empty meter.
    // That way, we don't have to worry about clipping over it later.
    g.saveState();
    g.reduceClipRegion(levelOutline);

    // We then call our drawing level function and restore the state for the value text below.
    drawLevelArc(g, levelBounds, proportion, startAng, endAng, thickness, strokeType);

    g.restoreState();

    // We want the value of the knob to be shown only when we hover over the knob.
    if (isHovering)
    {
        // We define and draw a box in which we will draw the text showing the value of the knob.
        juce::Rectangle<float> textArea{knobArea};
        textArea.setSize(knobArea.getWidth() * 0.67f, knobArea.getHeight() * 0.2f);
        textArea.setCentre(knobArea.getCentreX(), knobArea.getCentreY());

        g.setColour(Colours::emptyFill);
        g.fillRoundedRectangle(textArea, 4.f);

        // Inside the box, we draw our text and set the font size to always fit nicely in it.
        float fontSize{textArea.getHeight() * 0.8f};
        juce::Font font{Fonts::getRegularFont(fontSize)};
        juce::String text{getTextFromValue(getValue()) + " " + getTextValueSuffix()};

        // With the below, we verify that the text fits in the box with some padding.
        // If not, we further reduce the font size.
        const float textWidth{juce::GlyphArrangement::getStringWidth(font, text)};
        const float maxWidth{static_cast<float>(textArea.getWidth() * 0.7f)};

        if (textWidth > maxWidth)
            fontSize *= 0.77f;

        g.setFont(Fonts::getRegularFont(fontSize));
        g.setColour(Colours::text);
        g.drawFittedText(text, textArea.toNearestInt(), juce::Justification::horizontallyCentred, 1);
    }
}

void KnobComponent::resized()
{
    juce::Rectangle<int> bounds{getLocalBounds()};

    // We define below the size and position of the knob and label inside the already defined bounds.
    const float size{bounds.getHeight() * 0.75f};

    knobArea.setSize(size, size);
    knobArea.setCentre(bounds.toFloat().getCentreX(), bounds.toFloat().getCentreY() * 0.85f);

    labelArea = knobArea.toNearestInt();
    labelArea.setHeight(juce::roundToInt(knobArea.getHeight() * 0.22f));
    labelArea.setWidth(juce::roundToInt(knobArea.getWidth() * 1.3f));
    labelArea.setCentre(knobArea.toNearestInt().getCentreX(), juce::roundToInt(knobArea.getBottom() * 1.08f));
    label.setBounds(labelArea);

    const float fontSize{labelArea.toFloat().getHeight()};
    label.setFont(Fonts::getMediumFont(fontSize));
}

// As our knob is round, we redefine the hit region to be round.
bool KnobComponent::hitTest(int x, int y)
{
    return juce::Point<float>(static_cast<float>(x), static_cast<float>(y)).getDistanceFrom(knobArea.getCentre()) <= knobArea.getWidth() * 0.5f;
}

void KnobComponent::mouseEnter(const juce::MouseEvent& event)
{
    Slider::mouseEnter(event);
    isHovering = true;
    hoverTarget = 1.f;
    startTimerHz(60);
}

void KnobComponent::mouseDown(const juce::MouseEvent& event)
{
    Slider::mouseDown(event);
    setMouseCursor(juce::MouseCursor::NoCursor);
}

void KnobComponent::mouseUp(const juce::MouseEvent& event)
{
    Slider::mouseUp(event);
    setMouseCursor(juce::MouseCursor::NormalCursor);
    juce::Desktop::getInstance().getMainMouseSource().setScreenPosition(event.getMouseDownScreenPosition().toFloat());
}

void KnobComponent::mouseExit(const juce::MouseEvent& event)
{
    Slider::mouseExit(event);
    isHovering = false;
    hoverTarget = 0.f;
    startTimerHz(60);
}

// Timer implementation for visual animations.
void KnobComponent::timerCallback()
{
    hoverAmount += (hoverTarget - hoverAmount) * 0.2f;

    if (std::abs(hoverTarget - hoverAmount) < 0.005f)
    {
        hoverAmount = hoverTarget;
        stopTimer();
    }

    repaint(knobArea.toNearestInt());
}

// Our function to draw the level meter on the knob component as an arc using a gradient.
void KnobComponent::drawLevelArc(juce::Graphics& g,
                                 const juce::Rectangle<float>& bounds,
                                 float proportion,
                                 float startAngle, float endAngle,
                                 float thickness,
                                 const juce::PathStrokeType& strokeType)
{
    proportion = juce::jlimit(0.f, 1.f, proportion);

    if (proportion <= 0.f)
        return;

    const juce::Point<float> centre{bounds.getCentre()};
    const float radius{bounds.getWidth() * 0.5f};

    // We define our gradient with normalised points.
    juce::ColourGradient colours(Colours::onAccent, 0.f, 0.f, Colours::maxThreshold, 1.f, 1.f, false);
    colours.addColour(0.1, Colours::outline);
    colours.addColour(0.6, Colours::nearMaxThreshold);

    // Below we draw 10 gradient sections to achieve a close enough angular gradient from linear gradients.
    constexpr int numSections{10};

    // Some overlap is needed to not have some black lines in between segments.
    constexpr float overlap{0.002f};
    constexpr float inverseSections{1.f / numSections};

    // As we want sharp edges for the meter, we need to extend the edges to entirely fill the empty meter's rounded shape.
    // As we already reduced the clip region before calling the function, we don't worry about clipping.
    const float angleExtension{(thickness * 0.5f) / radius};

    const float extendedStart{startAngle - angleExtension};
    const float extendedEnd{endAngle + angleExtension};

    for (int i{}; i < numSections; ++i)
    {
        // We define the start and end of each section.
        const float sectionStart{juce::jmax(0.f, i * inverseSections - overlap)};
        const float sectionEnd{static_cast<float>(i + 1) * inverseSections + overlap};

        if (sectionStart >= proportion)
            break;

        // As our end can be greater than 1, we clamp it to 1.
        const float visibleEnd{juce::jmin(sectionEnd, proportion)};

        // We define the start and end angles for the section.
        // These need to be in radians, we already passed the radian equivalent values in the function.
        const float angleStart{extendedStart + sectionStart * (extendedEnd - extendedStart)};
        const float angleEnd{extendedStart + visibleEnd * (extendedEnd - extendedStart)};

        // We define the start and end points needed for each segment gradient.
        const juce::Point<float> startPoint{
            centre.x + radius * std::sin(angleStart),
            centre.y - radius * std::cos(angleStart)
        };

        const juce::Point<float> endPoint{
            centre.x + radius * std::sin(angleEnd),
            centre.y - radius * std::cos(angleEnd)
        };

        juce::ColourGradient sectionGradient(
            colours.getColourAtPosition(sectionStart), startPoint,
            colours.getColourAtPosition(visibleEnd), endPoint,
            false
        );

        // We add our section of the arc and draw it.
        juce::Path sectionArc;
        sectionArc.addArc(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), angleStart, angleEnd, true);

        juce::Path sectionFilled;
        strokeType.createStrokedPath(sectionFilled, sectionArc);

        g.setFillType(juce::FillType(sectionGradient));
        g.fillPath(sectionFilled);
    }
}
