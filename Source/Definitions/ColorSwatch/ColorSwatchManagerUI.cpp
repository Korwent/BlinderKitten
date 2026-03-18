/*
  ==============================================================================

    ColorSwatchManagerUI.cpp
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "ColorSwatchManagerUI.h"
#include "ColorSwatchManager.h"

ColorSwatchManagerUI::ColorSwatchManagerUI(const String& contentName) :
    BaseManagerShapeShifterUI(contentName, ColorSwatchManager::getInstance())
{
    addItemText = "Add new color swatch";
    noItemText = "Create your color swatches here.";
    addExistingItems();
}

ColorSwatchManagerUI::~ColorSwatchManagerUI()
{
}
