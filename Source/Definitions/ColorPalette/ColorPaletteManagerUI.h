/*
  ==============================================================================

    ColorPaletteManagerUI.h
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "ColorPalette.h"

class ColorPaletteListItemUI : public BaseItemUI<ColorPalette>
{
public:
    ColorPaletteListItemUI(ColorPalette* palette) : BaseItemUI<ColorPalette>(palette) {}
    ~ColorPaletteListItemUI() {}
    void mouseDrag(const MouseEvent& e) override;
};

class ColorPaletteManagerUI :
    public BaseManagerShapeShifterUI<BaseManager<ColorPalette>, ColorPalette, ColorPaletteListItemUI>
{
public:
    ColorPaletteManagerUI(const String& contentName);
    ~ColorPaletteManagerUI();

    static ColorPaletteManagerUI* create(const String& name) { return new ColorPaletteManagerUI(name); }
};
