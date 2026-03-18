/*
  ==============================================================================

    ColorSwatch.cpp
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "JuceHeader.h"
#include "ColorSwatch.h"
#include "ColorSwatchManager.h"
#include "../../Brain.h"
#include "../../UI/GridView/ColorSwatchGridView.h"

ColorSwatch::ColorSwatch(var params) :
    BaseItem(params.getProperty("name", "Color Swatch")),
    objectType(params.getProperty("type", "ColorSwatch").toString()),
    objectData(params),
    values("Values")
{
    saveAndLoadRecursiveData = true;
    editorIsCollapsed = true;
    nameCanBeChangedByUser = false;
    canBeDisabled = false;

    itemDataType = "ColorSwatch";
    id = addIntParameter("ID", "ID of this Color Swatch", 1, 1);
    userName = addStringParameter("Name", "Name of this color swatch", "New color swatch");
    updateName();

    values.selectItemWhenCreated = false;
    addChildControllableContainer(&values);

    Brain::getInstance()->registerColorSwatch(this, id->getValue());
}

ColorSwatch::~ColorSwatch()
{
    Brain::getInstance()->unregisterColorSwatch(this);
}

void ColorSwatch::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == id) {
        Brain::getInstance()->registerColorSwatch(this, id->getValue());
    }
    if (p == id || p == userName) {
        updateName();
    }
    ColorSwatchGridView::getInstance()->updateCells();
}

void ColorSwatch::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c)
{
    ColorSwatchGridView::getInstance()->updateCells();
}

void ColorSwatch::updateName()
{
    if (parentContainer != nullptr) {
        dynamic_cast<ColorSwatchManager*>(parentContainer.get())->reorderItems();
    }
    setNiceName(String((int)id->getValue()) + " - " + userName->getValue().toString());
}
