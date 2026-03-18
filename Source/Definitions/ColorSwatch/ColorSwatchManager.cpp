/*
  ==============================================================================

    ColorSwatchManager.cpp
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "ColorSwatch.h"
#include "ColorSwatchManager.h"

juce_ImplementSingleton(ColorSwatchManager);

int compareColorSwatch(ColorSwatch* A, ColorSwatch* B) {
    return (int)A->id->getValue() - (int)B->id->getValue();
}

ColorSwatchManager::ColorSwatchManager() :
    BaseManager("Color Swatch")
{
    itemDataType = "ColorSwatch";
    selectItemWhenCreated = true;
    comparator.compareFunc = compareColorSwatch;
}

ColorSwatchManager::~ColorSwatchManager()
{
}

void ColorSwatchManager::addItemInternal(ColorSwatch* o, var data)
{
    reorderItems();
}

void ColorSwatchManager::removeItemInternal(ColorSwatch* o)
{
}
