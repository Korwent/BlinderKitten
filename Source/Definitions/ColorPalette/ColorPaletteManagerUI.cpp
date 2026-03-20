/*
  ==============================================================================

    ColorPaletteManagerUI.cpp
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "ColorPaletteManagerUI.h"
#include "ColorPaletteManager.h"

void ColorPaletteListItemUI::mouseDrag(const MouseEvent& e)
{
    if (e.getDistanceFromDragStart() > 10 && !isDragAndDropActive())
    {
        var dragData(new DynamicObject());
        dragData.getDynamicObject()->setProperty("type", "GridViewButton");
        dragData.getDynamicObject()->setProperty("targetType", "colorpalette");
        dragData.getDynamicObject()->setProperty("id", item->id->getValue());
        startDragging(dragData, this, ScaledImage(), true);
    }
}

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
