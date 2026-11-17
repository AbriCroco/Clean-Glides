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

#include "CurveComponent.h"

CurveComponent::CurveComponent(const juce::String& text, juce::RangedAudioParameter* _curveX, juce::RangedAudioParameter* _curveY) :
    curveX(*_curveX),
    curveY(*_curveY),
    pointX(curveX, [this](float)
           { updateExponent(); repaint(); }, nullptr),
    pointY(curveY, [this](float)
           { updateExponent(); repaint(); }, nullptr)
{
    label.setText(text, juce::NotificationType::dontSendNotification);
    label.setJustificationType(juce::Justification::horizontallyCentred);
    label.setColour(juce::Label::textColourId, Colours::text);
    addAndMakeVisible(label);

    pointX.sendInitialUpdate();
    pointY.sendInitialUpdate();
}

void CurveComponent::paint(juce::Graphics& g)
{
    // We define the area in which we will draw the component, allowing no clipping with the border.
    drawingArea = curveArea.toFloat().reduced(borderThickness * 0.5f);

    // We draw the grid on top of which the curve will sit.
    g.setColour(Colours::outline);

    const juce::Rectangle<float> grid{drawingArea.toFloat()};

    for (const float fraction : {0.25f, 0.5f, 0.75f})
    {
        g.drawVerticalLine(
            juce::roundToInt(grid.getX() + grid.proportionOfWidth(fraction)),
            grid.getY(), grid.getBottom()
        );
    }

    g.drawHorizontalLine(
        juce::roundToInt(grid.getY() + grid.proportionOfHeight(0.5f)),
        grid.getX(), grid.getRight()
    );

    // Below we calculate the individual points per pixel for the curve.
    const int width{curveArea.getWidth()};

    std::vector<juce::Point<float>> samplePoints;
    samplePoints.resize(width);

    juce::Point<float> controlPoint(curveX.getValue(), curveY.getValue());

    for (int p{}; p < width; ++p)
    {
        float x = static_cast<float>(p) / static_cast<float>(width - 1.f);
        float y = std::pow(x, exponent);
        samplePoints[p] = map(x, y);
    }

    // We define the Path with lines in between all individual points.
    juce::Path curve;

    curve.startNewSubPath(samplePoints.front());

    for (size_t p{1}; p < samplePoints.size(); ++p)
    {
        curve.lineTo(samplePoints[p]);
    }

    // We define another Path for the area below the curve and add ghost points (bottom left and right).
    juce::Path belowCurve;
    belowCurve.addPath(curve);
    belowCurve.lineTo(drawingArea.getRight(), drawingArea.getBottom());
    belowCurve.lineTo(drawingArea.getX(), drawingArea.getBottom());

    // We save the current graphic's state and reduce our drawing region to our drawing area.
    g.saveState();
    g.reduceClipRegion(drawingArea.toNearestIntEdges());

    // We draw the area below the curve first so we can draw other elements on top of it.
    g.setColour(Colours::belowCurve);
    g.fillPath(belowCurve);

    // We draw the curve and define the controller centre to draw it next.
    g.setColour(Colours::curve);
    g.strokePath(curve, juce::PathStrokeType(2.f));

    juce::Point<float> controlCentre = map(controlPoint.x, controlPoint.y);

    controller = controller.withCentre(controlCentre);

    // This makes sure that our colour animation is being applied.
    g.setColour(Colours::accent.interpolatedWith(Colours::onAccent, hoverAmount));
    g.fillEllipse(controller);

    g.setColour(Colours::emptyFill);
    g.fillEllipse(controller.reduced(1.5f));

    // We restore the state to reset the clipping region and draw the border last so it is on top of all other elements.
    g.restoreState();
    g.setColour(Colours::outline);
    g.drawRoundedRectangle(curveArea.toFloat(), 10.f, borderThickness);
}

void CurveComponent::resized()
{
    juce::Rectangle<int> bounds{getLocalBounds()};

    // We define the size and place of both the curve component and its label.
    curveArea.setSize(juce::roundToInt(getWidth() * 0.8f), juce::roundToInt(getWidth() * 0.8f));
    curveArea.setCentre(bounds.getCentreX(), juce::roundToInt(bounds.getCentreY() * 0.9f));

    labelArea = curveArea;
    labelArea.setHeight(juce::roundToInt(curveArea.getHeight() * 0.12f));
    labelArea.setWidth(juce::roundToInt(curveArea.getWidth() * 1.2f));
    labelArea.setCentre(curveArea.toNearestInt().getCentreX(), juce::roundToInt(curveArea.getBottom() * 1.11f));
    label.setBounds(labelArea);

    const float fontSize{labelArea.toFloat().getHeight()};
    label.setFont(Fonts::getMediumFont(fontSize));
}

void CurveComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.getNumberOfClicks() == 2)
    {
        pointX.setValueAsCompleteGesture(curveX.getDefaultValue());
        pointY.setValueAsCompleteGesture(curveY.getDefaultValue());
        return;
    }

    if (isHovering)
    {
        isDragAllowed = true;

        pointX.beginGesture();
        pointY.beginGesture();
    }
}

void CurveComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragAllowed)
    {
        juce::Point<float> mousePos(unmap(event.position.x, event.position.y));

        mousePos.x = juce::jlimit(.02f, .98f, mousePos.x);
        mousePos.y = juce::jlimit(.02f, .98f, mousePos.y);

        pointX.setValueAsPartOfGesture(mousePos.x);
        pointY.setValueAsPartOfGesture(mousePos.y);
    }
}

void CurveComponent::mouseUp(const juce::MouseEvent& event)
{
    if (isDragAllowed)
    {
        isDragAllowed = false;
        pointX.endGesture();
        pointY.endGesture();
    }
    if (isHovering && !curveArea.contains(event.getPosition()))
    {
        isHovering = false;
        hoverTarget = 0.f;
        startTimerHz(60);

        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void CurveComponent::mouseMove(const juce::MouseEvent& event)
{
    if (controller.withCentre(juce::Point<float>(map(curveX.getValue(), curveY.getValue()))).contains(event.position) != isHovering)
    {
        isHovering = !isHovering;

        hoverTarget = isHovering ? 1.f : 0.f;

        startTimerHz(60);

        setMouseCursor(isHovering ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }
}

void CurveComponent::updateExponent()
{
    exponent = CurveUtils::calculateExponent(curveX.getValue(), curveY.getValue());
}

// Helper function to map our normalised values to pixel values inside our drawing area.
juce::Point<float> CurveComponent::map(float x, float y)
{
    const float xMin{static_cast<float>(drawingArea.getX())};
    const float xMax = static_cast<float>(drawingArea.getRight());
    const float mappedX = juce::jmap(x, 0.f, 1.f, xMin, xMax);

    const float yMin{static_cast<float>(drawingArea.getBottom())};
    const float yMax{static_cast<float>(drawingArea.getY())};
    const float mappedY = juce::jmap(y, 0.f, 1.f, yMin, yMax);

    return {mappedX, mappedY};
}

// Helper function to map our the pixel values inside our drawing area to normalised value.
juce::Point<float> CurveComponent::unmap(float x, float y)
{
    const float xMin = static_cast<float>(drawingArea.getX());
    const float xMax = static_cast<float>(drawingArea.getRight());
    const float normalisedX = juce::jmap(x, xMin, xMax, 0.f, 1.f);

    const float yMin = static_cast<float>(drawingArea.getBottom());
    const float yMax = static_cast<float>(drawingArea.getY());
    const float normalisedY = juce::jmap(y, yMin, yMax, 0.f, 1.f);

    return {normalisedX, normalisedY};
}

// Timer implementation for visual animations.
void CurveComponent::timerCallback()
{
    hoverAmount += (hoverTarget - hoverAmount) * 0.2f;

    if (std::abs(hoverTarget - hoverAmount) < 0.005f)
    {
        hoverAmount = hoverTarget;
        stopTimer();
    }

    repaint(controller.toNearestInt());
}
