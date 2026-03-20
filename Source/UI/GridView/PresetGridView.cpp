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
#include "Definitions/ColorPalette/ColorPalette.h"
#include "Definitions/ColorPalette/ColorPaletteItem.h"
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
    if (source.description.getProperty("type", "") == "GridViewButton") {
        String tt = source.description.getProperty("targetType", "");
        return tt == "colorswatch" || tt == "colorpaletteitem" || tt == "colorpalette";
    }
    return false;
}

static void applySwatchToPreset(Preset* preset, ColorSwatch* swatch)
{
    preset->userName->setValue(swatch->userName->getValue());
    preset->presetType->setValueWithData(4);
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
}

static Preset* getOrCreatePreset(int presetId)
{
    Preset* preset = Brain::getInstance()->getPresetById(presetId);
    if (preset == nullptr)
    {
        preset = PresetManager::getInstance()->addItem();
        preset->id->setValue(presetId);
    }
    return preset;
}

void PresetGridView::itemDropped(const SourceDetails& source)
{
    String targetType = source.description.getProperty("targetType", "");

    // Find the drop slot
    int startSlotIndex = -1;
    for (int i = 0; i < gridButtons.size(); i++)
    {
        if (gridButtons[i]->getBounds().contains(source.localPosition.toInt()))
        {
            startSlotIndex = i;
            break;
        }
    }
    if (startSlotIndex < 0) return;

    if (targetType == "colorpalette")
    {
        int paletteId = source.description.getProperty("id", 0);
        ColorPalette* palette = Brain::getInstance()->getColorPaletteById(paletteId);
        if (palette == nullptr) return;

        for (int itemIdx = 0; itemIdx < palette->items.items.size(); itemIdx++)
        {
            int slotIndex = startSlotIndex + itemIdx;
            if (slotIndex >= numberOfCells) break;

            int swatchId = palette->items.items[itemIdx]->colorSwatchId->getValue();
            ColorSwatch* swatch = Brain::getInstance()->getColorSwatchById(swatchId);
            if (swatch == nullptr) continue;

            applySwatchToPreset(getOrCreatePreset(slotIndex + 1), swatch);
        }
    }
    else
    {
        // Single swatch drop (colorswatch or colorpaletteitem)
        int swatchId = source.description.getProperty("id", 0);
        ColorSwatch* swatch = Brain::getInstance()->getColorSwatchById(swatchId);
        if (swatch == nullptr) return;

        Preset* preset = getOrCreatePreset(startSlotIndex + 1);
        applySwatchToPreset(preset, swatch);
        preset->selectThis();
    }
}
