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
#include "juce_gui_basics/juce_gui_basics.h"

#include "CurveUtils.h"
#include "LookAndFeel.h"

struct CurveComponent final : public juce::Component,
                              private juce::Timer {
public:
    CurveComponent(const juce::String& text, juce::RangedAudioParameter* curveX, juce::RangedAudioParameter* curveY);
    ~CurveComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    void updateExponent();

    juce::Point<float> map(float x, float y);
    juce::Point<float> unmap(float x, float y);

    void timerCallback() override;

    juce::Label label;

    juce::RangedAudioParameter &curveX, &curveY;
    juce::ParameterAttachment pointX, pointY;
    float exponent{1.f};

    juce::Rectangle<int> curveArea;
    juce::Rectangle<int> labelArea;

    static constexpr float borderThickness = 4.f;
    juce::Rectangle<float> drawingArea;

    juce::Rectangle<float> controller{9.f, 9.f};

    bool isDragAllowed{false};
    bool isHovering{false};

    float hoverAmount{};
    float hoverTarget{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveComponent)
};
