/*
  ==============================================================================

    PresetGridView.cpp
    Created: 19 Feb 2022 12:19:42am
    Author:  No

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PresetGridView.h"
#include "Brain.h"
#include "Definitions/Preset/Preset.h"
#include "Definitions/Preset/PresetManager.h"
#include "Definitions/Preset/PresetSubFixtureValues.h"
#include "Definitions/Preset/PresetValue.h"
#include "Definitions/ColorSwatch/ColorSwatch.h"
#include "Definitions/ColorSwatch/ColorSwatchValue.h"
#include "Definitions/ChannelFamily/ChannelType/ChannelType.h"
#include "DataTransferManager/DataTransferManager.h"

//==============================================================================
PresetGridViewUI::PresetGridViewUI(const String& contentName):
    ShapeShifterContent(PresetGridView::getInstance(), contentName)
{
    
}

PresetGridViewUI::~PresetGridViewUI()
{
}

juce_ImplementSingleton(PresetGridView);

PresetGridView::PresetGridView()
{
    numberOfCells = 200;
    targetType = "preset";
    PresetManager::getInstance()->addAsyncManagerListener(this);

}

PresetGridView::~PresetGridView()
{
    if (PresetManager::getInstanceWithoutCreating() != nullptr) PresetManager::getInstance()->removeAsyncManagerListener(this);
}

void PresetGridView::updateCells() {
    for (int i = 0; i < numberOfCells; i++) {
        Preset* g = Brain::getInstance()->getPresetById(i+1);
        if (g != nullptr) {
            gridButtons[i]->removeColour(TextButton::buttonColourId);
            gridButtons[i]->removeColour(TextButton::textColourOnId);
            gridButtons[i]->removeColour(TextButton::textColourOffId);

            gridButtons[i]->setButtonText(g->userName->getValue().toString());
        }
        else {
            gridButtons[i]->setButtonText("");
            gridButtons[i]->setColour(TextButton::buttonColourId, Colour(40, 40, 40));
            gridButtons[i]->setColour(TextButton::textColourOnId, Colour(96, 96, 96));
            gridButtons[i]->setColour(TextButton::textColourOffId, Colour(96, 96, 96));

        }
    }
}

void PresetGridView::newMessage(const PresetManager::ManagerEvent& e)
{
    updateCells();
}

bool PresetGridView::isInterestedInDragSource(const SourceDetails& source)
{
    if (source.description.getProperty("type", "") == "GridViewButton")
        return source.description.getProperty("targetType", "") == "colorswatch";
    return false;
}

void PresetGridView::itemDropped(const SourceDetails& source)
{
    int swatchId = source.description.getProperty("id", 0);
    ColorSwatch* swatch = Brain::getInstance()->getColorSwatchById(swatchId);
    if (swatch == nullptr) return;

    // Find which grid button is at the drop position
    GridViewButton* btn = nullptr;
    for (auto* b : gridButtons)
    {
        if (b->getBounds().contains(source.localPosition.toInt()))
        {
            btn = b;
            break;
        }
    }
    if (btn == nullptr) return;

    int slotIndex = gridButtons.indexOf(btn);
    if (slotIndex < 0) return;
    int presetId = slotIndex + 1;

    // Get or create the preset at this slot
    Preset* preset = Brain::getInstance()->getPresetById(presetId);
    if (preset == nullptr)
    {
        preset = PresetManager::getInstance()->addItem();
        preset->id->setValue(presetId);
    }

    // Rename to swatch name
    preset->userName->setValue(swatch->userName->getValue());

    // Set type to "Same Channels type" (value 4) so it applies to all fixtures
    preset->presetType->setValueWithData(4);

    // Replace all channel values with the swatch's values
    preset->subFixtureValues.clear();
    PresetSubFixtureValues* sfv = preset->subFixtureValues.addItem();
    sfv->targetFixtureId->setValue(0);
    sfv->targetSubFixtureId->setValue(0);
    sfv->values.clear();

    for (int i = 0; i < swatch->values.items.size(); i++)
    {
        ColorSwatchValue* csv = swatch->values.items[i];
        ChannelType* ct = dynamic_cast<ChannelType*>(csv->channelType->targetContainer.get());
        if (ct == nullptr) continue;
        PresetValue* pv = sfv->values.addItem();
        pv->param->setValueFromTarget(ct);
        pv->paramValue->setValue(csv->paramValue->getValue());
    }

    preset->computeValues();
    preset->selectThis();
}
