/*
  ==============================================================================

    ColorPaletteValue.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class ColorPaletteValue:
    public BaseItem
{
    public:
    ColorPaletteValue(var params = var());
    ~ColorPaletteValue();

    TargetParameter* channelType;
    FloatParameter* paramValue;

    String getTypeString() const override { return "ColorPaletteValue"; }
    static ColorPaletteValue* create(var params) { return new ColorPaletteValue(params); }
};
