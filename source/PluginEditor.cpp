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

#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor& p) :
    AudioProcessorEditor(&p),
    pluginProcessor(p),
    curveComponent("Glide Curve", p.apvts.getParameter("curveX"), p.apvts.getParameter("curveY")),
    timeKnob("Glide Time", pluginProcessor.apvts, "time"),
    rangeKnob("Bend Range", pluginProcessor.apvts, "range")
{
    const int width{pluginProcessor.getEditorWidth()};
    const int height{pluginProcessor.getEditorHeight()};

    setResizeLimits(357, 210, 1360, 800);
    setResizable(false, true);
    getConstrainer()->setFixedAspectRatio(1.7);

    setSize(width, height);

    for (auto* component : getComponents())
    {
        addAndMakeVisible(component);
    }
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colours::background);

    juce::Rectangle<int> textArea;
    textArea.setSize(juce::roundToInt(bounds.getWidth() * 0.8f), juce::roundToInt(bounds.getHeight() * 0.2f));
    textArea.setCentre(bounds.getCentreX(), juce::roundToInt(bounds.getCentreY() * 0.2f));

    float fontSize{textArea.getHeight() * 1.2f};
    juce::String text{"Clean     Glides"};
    g.setFont(Fonts::getTitleFont(fontSize));

    juce::Rectangle<float> gradientArea{textArea.toFloat()};

    juce::ColourGradient titleGradient{
        Colours::titleGradientBottom, gradientArea.getCentreX(), gradientArea.getBottom() * 0.85f,
        Colours::titleGradientTop, gradientArea.getCentreX(), gradientArea.getHeight() * 0.1f,
        false
    };

    g.setFillType(juce::FillType(titleGradient));
    g.drawFittedText(text, textArea, juce::Justification::horizontallyCentred, 1);
}

void PluginEditor::resized()
{
    bounds = getLocalBounds();

    const int width{bounds.getWidth()};
    const juce::Point<int> boundsCentre{getLocalBounds().getCentre()};

    juce::Rectangle<int> curveArea{juce::roundToInt(width * 0.4f), juce::roundToInt(width * 0.4f)};
    curveArea.setCentre(juce::roundToInt(boundsCentre.x * 0.65f), juce::roundToInt(boundsCentre.y * 1.15f));
    curveComponent.setBounds(curveArea);

    const juce::Point<int> knobAreaCentre{
        juce::roundToInt(curveArea.getTopRight().x + (width - curveArea.getTopRight().x) * 0.5f),
        juce::roundToInt(curveArea.getTopRight().y + curveArea.getHeight() * 0.25f)
    };

    juce::Rectangle<int> knobArea{
        juce::roundToInt((width - curveArea.getTopRight().x) * 0.85f),
        juce::roundToInt(curveArea.getHeight() * 0.5f)
    };

    knobArea.setCentre(knobAreaCentre);
    timeKnob.setBounds(knobArea);

    knobArea.setY(juce::roundToInt(curveArea.getBottom() - curveArea.getHeight() * 0.5f));
    rangeKnob.setBounds(knobArea);

    pluginProcessor.setEditorSize(getWidth(), getHeight());
}

std::vector<juce::Component*> PluginEditor::getComponents()
{
    return {
        &timeKnob,
        &rangeKnob,
        &curveComponent
    };
}
