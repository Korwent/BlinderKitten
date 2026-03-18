/*
  ==============================================================================

    ColorPaletteManagerUI.cpp
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "ColorPaletteManagerUI.h"
#include "ColorPaletteManager.h"

ColorPaletteManagerUI::ColorPaletteManagerUI(const String& contentName) :
    BaseManagerShapeShifterUI(contentName, ColorPaletteManager::getInstance())
{
    addItemText = "Add new color palette";
    noItemText = "Create your color palettes here.";
    addExistingItems();
}

ColorPaletteManagerUI::~ColorPaletteManagerUI()
{
}
