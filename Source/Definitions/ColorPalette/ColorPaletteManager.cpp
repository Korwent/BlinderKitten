/*
  ==============================================================================

    ColorPaletteManager.cpp
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "ColorPalette.h"
#include "ColorPaletteManager.h"

juce_ImplementSingleton(ColorPaletteManager);

int compareColorPalette(ColorPalette* A, ColorPalette* B) {
    return (int)A->id->getValue() - (int)B->id->getValue();
}

ColorPaletteManager::ColorPaletteManager() :
    BaseManager("Color Palette")
{
    itemDataType = "ColorPalette";
    selectItemWhenCreated = true;
    comparator.compareFunc = compareColorPalette;
}

ColorPaletteManager::~ColorPaletteManager()
{
}

void ColorPaletteManager::addItemInternal(ColorPalette* o, var data)
{
    reorderItems();
}

void ColorPaletteManager::removeItemInternal(ColorPalette* o)
{
}
