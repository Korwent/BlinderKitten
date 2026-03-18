/*
  ==============================================================================

    ColorPalette.cpp
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "JuceHeader.h"
#include "ColorPalette.h"
#include "ColorPaletteManager.h"
#include "../../Brain.h"
#include "../../UI/GridView/ColorPaletteGridView.h"

ColorPalette::ColorPalette(var params) :
    BaseItem(params.getProperty("name", "Color Palette")),
    objectType(params.getProperty("type", "ColorPalette").toString()),
    objectData(params),
    items("Items")
{
    saveAndLoadRecursiveData = true;
    editorIsCollapsed = true;
    nameCanBeChangedByUser = false;
    canBeDisabled = false;

    itemDataType = "ColorPalette";
    id = addIntParameter("ID", "ID of this Color Palette", 1, 1);
    userName = addStringParameter("Name", "Name of this color palette", "New color palette");
    updateName();

    items.selectItemWhenCreated = false;
    addChildControllableContainer(&items);

    Brain::getInstance()->registerColorPalette(this, id->getValue());
}

ColorPalette::~ColorPalette()
{
    Brain::getInstance()->unregisterColorPalette(this);
}

void ColorPalette::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == id) {
        Brain::getInstance()->registerColorPalette(this, id->getValue());
    }
    if (p == id || p == userName) {
        updateName();
    }
    ColorPaletteGridView::getInstance()->updateCells();
}

void ColorPalette::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    ColorPaletteGridView::getInstance()->updateCells();
}

void ColorPalette::updateName()
{
    if (parentContainer != nullptr) {
        dynamic_cast<ColorPaletteManager*>(parentContainer.get())->reorderItems();
    }
    setNiceName(String((int)id->getValue()) + " - " + userName->getValue().toString());
}
