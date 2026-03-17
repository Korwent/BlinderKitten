/*
  ==============================================================================

    ColorPalette.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "ColorPaletteValue.h"

class ColorPalette :
    public BaseItem
{
public:
    ColorPalette(var params = var());
    virtual ~ColorPalette();

    String objectType;
    var objectData;

    IntParameter* id;
    int registeredId = 0;
    StringParameter* userName;

    BaseManager<ColorPaletteValue> values;

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
    void updateName();

    String getTypeString() const override { return objectType; }
    static ColorPalette* create(var params) { return new ColorPalette(params); }
};
