/*
  ==============================================================================

    ColorPaletteManagerUI.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "ColorPalette.h"

class ColorPaletteManagerUI :
    public BaseManagerShapeShifterUI<BaseManager<ColorPalette>, ColorPalette, BaseItemUI<ColorPalette>>
{
public:
    ColorPaletteManagerUI(const String& contentName);
    ~ColorPaletteManagerUI();

    static ColorPaletteManagerUI* create(const String& name) { return new ColorPaletteManagerUI(name); }
};
