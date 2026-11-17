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

#include "LookAndFeel.h"

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPosProportional, float rotaryStartAngle,
                                       float rotaryEndAngle, juce::Slider&)
{
    const juce::Rectangle<float> bounds{
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(width),
        static_cast<float>(height)
    };

    const juce::Point<float> centre{bounds.getCentre()};

    // Our bounds match the entire knob area so we need to reduce it slightly.
    const float knobSize{bounds.getHeight() / 1.3f};

    // We want a square so we use the same size for the height and width.
    juce::Rectangle<float> knobBounds{knobSize, knobSize};
    knobBounds.setCentre(centre);

    const juce::Point<float> top{centre.x, knobBounds.getY()};

    // We define a colour gradient and draw two elipses, one for the knob and the other for the outline.
    juce::ColourGradient gradient{Colours::knobGradientCentre, centre, Colours::knobGradientEdge, top, true};

    g.setFillType(juce::FillType(gradient));
    g.fillEllipse(knobBounds);

    const float outlineThickness{knobBounds.getWidth() * 0.008f};

    g.setColour(Colours::outline);
    g.drawEllipse(knobBounds.reduced(outlineThickness * 0.5f), outlineThickness);

    // We also need some way to indicate the knob rotation.
    // For that we create a rounded rectangle and offset it slightly from the centre.
    juce::Path indicator;

    const float size{knobBounds.getWidth() * 0.022f};
    juce::Rectangle<float> r;
    r.setLeft(centre.getX() - size);
    r.setRight(centre.getX() + size);
    r.setTop(knobBounds.getY() + knobBounds.getHeight() * 0.04f);
    r.setBottom(centre.getY() - knobBounds.getHeight() * 0.15f);

    indicator.addRoundedRectangle(r, 3.f);

    // We make sure our start angle is smaller than our end angle.
    jassert(rotaryStartAngle < rotaryEndAngle);

    // We then apply some rotation to the rectangle based on our current value and draw it.
    float rotaryAngToRad{juce::jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle)};
    indicator.applyTransform(juce::AffineTransform().rotated(rotaryAngToRad, centre.getX(), centre.getY()));

    g.setColour(Colours::knobIndicator);
    g.fillPath(indicator);

    g.setColour(Colours::emptyFill);
    g.strokePath(indicator, juce::PathStrokeType(size * 0.3f));
}
