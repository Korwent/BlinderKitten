/*
  ==============================================================================

    ColorSwatch.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "ColorSwatchValue.h"

class ColorSwatch :
    public BaseItem
{
public:
    ColorSwatch(var params = var());
    virtual ~ColorSwatch();

    String objectType;
    var objectData;

    IntParameter* id;
    int registeredId = 0;
    StringParameter* userName;

    BaseManager<ColorSwatchValue> values;

    void onContainerParameterChangedInternal(Parameter* p) override;
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;
    void updateName();

    String getTypeString() const override { return objectType; }
    static ColorSwatch* create(var params) { return new ColorSwatch(params); }
};
