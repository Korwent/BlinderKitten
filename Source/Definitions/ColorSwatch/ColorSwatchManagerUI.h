/*
  ==============================================================================

    ColorSwatchManagerUI.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "ColorSwatch.h"

class ColorSwatchManagerUI :
    public BaseManagerShapeShifterUI<BaseManager<ColorSwatch>, ColorSwatch, BaseItemUI<ColorSwatch>>
{
public:
    ColorSwatchManagerUI(const String& contentName);
    ~ColorSwatchManagerUI();

    static ColorSwatchManagerUI* create(const String& name) { return new ColorSwatchManagerUI(name); }
};
