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

#include "juce_audio_basics/juce_audio_basics.h"

class MPEPortamento {
private:
    enum struct TimeType {
        Seconds
    }; // To do: add more ways to define the glide time.

public:
    struct Parameters {
        float portamentoTime{1.f};
        float portamentoCurve{1.f};
        int bendRange{48};
        TimeType timeType{TimeType::Seconds};
    };

    void prepare(double sampleRate, const juce::MidiBuffer& mpeInitMessage);
    void process(juce::MidiBuffer& midiBuffer, int numSamples);

    void updateParameters(const Parameters& newParams);

private:
    struct Glide {
        bool active{false};
        int anchorNote{-1};
        int playingNote{-1};
        float start{8192.f}, target{8192.f}, current{8192.f};
        int currentSample{0}, totalSample{1};
    };

    void renderGlides(int sampleStart, int sampleEnd);

    void scheduleReset(size_t index);
    void tickResets(int numSamples);
    void cancelReset(size_t index);
    void clearChannel(size_t index);

    juce::MidiBuffer mpeInitBuffer;

    juce::MidiBuffer mpeMidiBuffer;
    std::array<Glide, 15> glides;

    Parameters params;

    double _sampleRate;
    int bendStep{32};
    float semitoneUnit{8192.f / 48.f};

    std::array<int, 15> resetCountdown;
    int pendingResets{0};
    int graceSamples{0};
    static constexpr double releaseGraceSeconds{5.0}; // To do: add this as a parameter in some settings.
};
