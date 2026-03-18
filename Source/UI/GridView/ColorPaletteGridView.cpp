/*
  ==============================================================================

    ColorPaletteGridView.cpp
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ColorPaletteGridView.h"
#include "Brain.h"
#include "Definitions/ColorPalette/ColorPalette.h"
#include "Definitions/ColorPalette/ColorPaletteItem.h"
#include "Definitions/ColorPalette/ColorPaletteManager.h"
#include "Definitions/ColorSwatch/ColorSwatch.h"
#include "Definitions/ColorSwatch/ColorSwatchValue.h"
#include "BKEngine.h"
#include "Definitions/ChannelFamily/ChannelType/ChannelType.h"

//==============================================================================
ColorPaletteGridViewUI::ColorPaletteGridViewUI(const String& contentName) :
    ShapeShifterContent(ColorPaletteGridView::getInstance(), contentName)
{
}

ColorPaletteGridViewUI::~ColorPaletteGridViewUI()
{
}

juce_ImplementSingleton(ColorPaletteGridView);

ColorPaletteGridView::ColorPaletteGridView()
{
    numberOfCells = 200;
    targetType = "colorpalette";
    ColorPaletteManager::getInstance()->addAsyncManagerListener(this);
}

ColorPaletteGridView::~ColorPaletteGridView()
{
    if (ColorPaletteManager::getInstanceWithoutCreating() != nullptr)
        ColorPaletteManager::getInstance()->removeAsyncManagerListener(this);
}

void ColorPaletteGridView::updateCells()
{
    BKEngine* bke = dynamic_cast<BKEngine*>(Engine::mainEngine);
    ChannelType* redCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPRedChannel->targetContainer.get()) : nullptr;
    ChannelType* greenCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPGreenChannel->targetContainer.get()) : nullptr;
    ChannelType* blueCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPBlueChannel->targetContainer.get()) : nullptr;

    for (int i = 0; i < numberOfCells; i++) {
        ColorPalette* cp = Brain::getInstance()->getColorPaletteById(i + 1);
        if (cp != nullptr) {
            // Use the first swatch's RGB as representative color
            float r = 0, g = 0, b = 0;
            if (cp->items.items.size() > 0) {
                int swatchId = cp->items.items[0]->colorSwatchId->getValue();
                ColorSwatch* swatch = Brain::getInstance()->getColorSwatchById(swatchId);
                if (swatch != nullptr) {
                    bool foundViaChannelType = false;
                    if (redCT != nullptr || greenCT != nullptr || blueCT != nullptr) {
                        for (int j = 0; j < swatch->values.items.size(); j++) {
                            ChannelType* ct = dynamic_cast<ChannelType*>(swatch->values.items[j]->channelType->targetContainer.get());
                            float val = swatch->values.items[j]->paramValue->getValue();
                            if (ct != nullptr && ct == redCT)   { r = val; foundViaChannelType = true; }
                            else if (ct != nullptr && ct == greenCT) { g = val; foundViaChannelType = true; }
                            else if (ct != nullptr && ct == blueCT)  { b = val; foundViaChannelType = true; }
                        }
                    }
                    // Fallback: if channel types aren't configured, use first three values as R/G/B
                    if (!foundViaChannelType && swatch->values.items.size() >= 1) {
                        r = swatch->values.items.size() > 0 ? (float)swatch->values.items[0]->paramValue->getValue() : 0;
                        g = swatch->values.items.size() > 1 ? (float)swatch->values.items[1]->paramValue->getValue() : 0;
                        b = swatch->values.items.size() > 2 ? (float)swatch->values.items[2]->paramValue->getValue() : 0;
                    }
                }
            }

            Colour btnColor((uint8)(r * 255), (uint8)(g * 255), (uint8)(b * 255));
            gridButtons[i]->setColour(TextButton::buttonColourId, btnColor);

            float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
            Colour textColor = luminance > 0.5f ? Colours::black : Colours::white;
            gridButtons[i]->setColour(TextButton::textColourOnId, textColor);
            gridButtons[i]->setColour(TextButton::textColourOffId, textColor);

            String label = cp->userName->getValue().toString();
            if (cp->items.items.size() > 0)
                label += " (" + String(cp->items.items.size()) + ")";
            gridButtons[i]->setButtonText(label);
        }
        else {
            gridButtons[i]->setButtonText("");
            gridButtons[i]->setColour(TextButton::buttonColourId, Colour(40, 40, 40));
            gridButtons[i]->setColour(TextButton::textColourOnId, Colour(96, 96, 96));
            gridButtons[i]->setColour(TextButton::textColourOffId, Colour(96, 96, 96));
        }
    }
}

void ColorPaletteGridView::newMessage(const ColorPaletteManager::ManagerEvent& e)
{
    updateCells();
}

bool ColorPaletteGridView::isInterestedInDragSource(const SourceDetails& source)
{
    if (source.description.getProperty("type", "") == "GridViewButton")
        return source.description.getProperty("targetType", "") == "colorswatch";
    return false;
}

void ColorPaletteGridView::itemDropped(const SourceDetails& source)
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

    int paletteId = btn->id;

    // Only create on empty slots
    if (Brain::getInstance()->getColorPaletteById(paletteId) != nullptr) return;

    ColorPalette* palette = ColorPaletteManager::getInstance()->addItem();
    palette->id->setValue(paletteId);
    palette->userName->setValue(swatch->userName->getValue());

    ColorPaletteItem* item = palette->items.addItem();
    item->colorSwatchId->setValue(swatchId);
    item->setNiceName(swatch->userName->getValue().toString());
}
