/*
  ==============================================================================

    ColorPaletteItem.h
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class ColorPaletteItem :
    public BaseItem
{
public:
    ColorPaletteItem(var params = var());
    ~ColorPaletteItem();

    IntParameter* colorSwatchId;

    String getTypeString() const override { return "ColorPaletteItem"; }
    static ColorPaletteItem* create(var params) { return new ColorPaletteItem(params); }
};
