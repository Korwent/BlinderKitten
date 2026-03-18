/*
  ==============================================================================

    ColorSwatchManager.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "ColorSwatch.h"

class ColorSwatchManager :
    public BaseManager<ColorSwatch>
{
public:
    juce_DeclareSingleton(ColorSwatchManager, true);

    ColorSwatchManager();
    ~ColorSwatchManager();

    void addItemInternal(ColorSwatch* o, var data) override;
    void removeItemInternal(ColorSwatch* o) override;
};
