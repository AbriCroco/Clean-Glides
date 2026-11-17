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

#include <vector>

#include "juce_gui_basics/juce_gui_basics.h"

#include "PluginProcessor.h"
#include "CurveComponent.h"
#include "KnobComponent.h"
#include "LookAndFeel.h"

class PluginEditor : public juce::AudioProcessorEditor {
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::vector<juce::Component*> getComponents();

    PluginProcessor& pluginProcessor;

    CurveComponent curveComponent;

    KnobComponent timeKnob;
    KnobComponent rangeKnob;

    juce::Rectangle<int> bounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
