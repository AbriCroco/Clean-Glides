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

#pragma once

#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

#include "LookAndFeel.h"

struct KnobComponent : public juce::Slider,
                       private juce::Timer {
public:
    KnobComponent(const juce::String& text, juce::AudioProcessorValueTreeState& apvts, const juce::ParameterID& parameterID);
    ~KnobComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    bool hitTest(int x, int y) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void timerCallback() override;

    void drawLevelArc(juce::Graphics& g,
                      const juce::Rectangle<float>& bounds,
                      float proportion,
                      float startAngle, float endAngle,
                      float thickness,
                      const juce::PathStrokeType& strokeType);

    KnobLookAndFeel lnf;
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;

    juce::Rectangle<float> knobArea;
    juce::Rectangle<int> labelArea;

    float hoverAmount{};
    float hoverTarget{};
    bool isHovering{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobComponent)
};
