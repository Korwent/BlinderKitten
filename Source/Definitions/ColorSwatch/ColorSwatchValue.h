/*
  ==============================================================================

    ColorSwatchValue.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class ColorSwatchValue:
    public BaseItem
{
    public:
    ColorSwatchValue(var params = var());
    ~ColorSwatchValue();

    TargetParameter* channelType;
    FloatParameter* paramValue;

    String getTypeString() const override { return "ColorSwatchValue"; }
    static ColorSwatchValue* create(var params) { return new ColorSwatchValue(params); }
};
