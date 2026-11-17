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

#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

#include "BinaryData.h"

namespace Colours {
const juce::Colour background{0xFF0B0420};
const juce::Colour outline{0xFFFF2E92};

const juce::Colour emptyFill{0xCC1E0A3F};

const juce::Colour nearMaxThreshold{0xFFFFDC2E};
const juce::Colour maxThreshold{0xFFFF0000};

const juce::Colour curve{0xFFFF4C6E};
const juce::Colour belowCurve{0x66201840};

const juce::Colour accent{0xFFFF8B20};
const juce::Colour onAccent{0xFF00E5FF};

const juce::Colour knobGradientCentre{0xFFF9779A};
const juce::Colour knobGradientEdge{0xFF001753};
const juce::Colour knobIndicator{0xFFFFB8CB};

const juce::Colour text{0xFFE7FFD7};

const juce::Colour titleGradientTop{0xFFE62E87};
const juce::Colour titleGradientBottom{0xFF0F2F99};
} // namespace Colours

namespace Fonts {
static const juce::Typeface::Ptr SynthwaveFont{
    juce::Typeface::createSystemTypefaceFor(BinaryData::MonotonRegular_ttf, BinaryData::MonotonRegular_ttfSize)
};

static const juce::Typeface::Ptr RegularFont{
    juce::Typeface::createSystemTypefaceFor(BinaryData::OrbitronRegular_ttf, BinaryData::OrbitronRegular_ttfSize)
};
static const juce::Typeface::Ptr MediumFont{
    juce::Typeface::createSystemTypefaceFor(BinaryData::OrbitronMedium_ttf, BinaryData::OrbitronMedium_ttfSize)
};
static const juce::Typeface::Ptr SemiBoldFont{
    juce::Typeface::createSystemTypefaceFor(BinaryData::OrbitronSemiBold_ttf, BinaryData::OrbitronSemiBold_ttfSize)
};
static const juce::Typeface::Ptr BoldFont{
    juce::Typeface::createSystemTypefaceFor(BinaryData::OrbitronBold_ttf, BinaryData::OrbitronBold_ttfSize)
};

inline juce::Font getTitleFont(const float height = 14.0f)
{
    return {juce::FontOptions(SynthwaveFont).withHeight(height)};
}

inline juce::Font getRegularFont(const float height = 14.0f)
{
    return {juce::FontOptions(RegularFont).withHeight(height)};
}

inline juce::Font getMediumFont(const float height = 14.0f)
{
    return {juce::FontOptions(MediumFont).withHeight(height)};
}

inline juce::Font getSemiBoldFont(const float height = 14.0f)
{
    return {juce::FontOptions(SemiBoldFont).withHeight(height)};
}

inline juce::Font getBoldFont(const float height = 14.0f)
{
    return {juce::FontOptions(BoldFont).withHeight(height)};
}
} // namespace Fonts

struct KnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    KnobLookAndFeel() {}

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobLookAndFeel)
};
