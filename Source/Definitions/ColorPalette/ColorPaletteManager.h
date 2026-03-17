/*
  ==============================================================================

    ColorPaletteManager.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "ColorPalette.h"

class ColorPaletteManager :
    public BaseManager<ColorPalette>
{
public:
    juce_DeclareSingleton(ColorPaletteManager, true);

    ColorPaletteManager();
    ~ColorPaletteManager();

    void addItemInternal(ColorPalette* o, var data) override;
    void removeItemInternal(ColorPalette* o) override;
};
