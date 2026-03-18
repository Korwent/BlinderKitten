/*
  ==============================================================================

    ColorPaletteItem.cpp
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "ColorPaletteItem.h"

ColorPaletteItem::ColorPaletteItem(var params) :
    BaseItem(params.getProperty("name", "Swatch"))
{
    itemDataType = "ColorPaletteItem";
    canBeDisabled = false;
    colorSwatchId = addIntParameter("Color Swatch ID", "ID of the color swatch referenced by this item", 1, 1);
}

ColorPaletteItem::~ColorPaletteItem()
{
}
