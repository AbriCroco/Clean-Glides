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

#include <cmath>

#include "juce_core/juce_core.h"

namespace CurveUtils {
inline float calculateExponent(float x, float y)
{
    x = juce::jlimit(0.01f, 0.99f, x);
    y = juce::jlimit(0.01f, 0.99f, y);

    return std::log(y) / std::log(x);
}
} // namespace CurveUtils
