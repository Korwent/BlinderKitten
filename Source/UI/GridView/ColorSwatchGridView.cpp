/*
  ==============================================================================

    ColorSwatchGridView.cpp
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ColorSwatchGridView.h"
#include "Brain.h"
#include "Definitions/ColorSwatch/ColorSwatch.h"
#include "Definitions/ColorSwatch/ColorSwatchManager.h"
#include "Definitions/ColorSwatch/ColorSwatchValue.h"
#include "BKEngine.h"
#include "Definitions/ChannelFamily/ChannelType/ChannelType.h"

//==============================================================================
ColorSwatchGridViewUI::ColorSwatchGridViewUI(const String& contentName) :
    ShapeShifterContent(ColorSwatchGridView::getInstance(), contentName)
{
}

ColorSwatchGridViewUI::~ColorSwatchGridViewUI()
{
}

juce_ImplementSingleton(ColorSwatchGridView);

ColorSwatchGridView::ColorSwatchGridView()
{
    numberOfCells = 200;
    targetType = "colorswatch";
    ColorSwatchManager::getInstance()->addAsyncManagerListener(this);
}

ColorSwatchGridView::~ColorSwatchGridView()
{
    if (ColorSwatchManager::getInstanceWithoutCreating() != nullptr)
        ColorSwatchManager::getInstance()->removeAsyncManagerListener(this);
}

void ColorSwatchGridView::updateCells()
{
    BKEngine* bke = dynamic_cast<BKEngine*>(Engine::mainEngine);
    ChannelType* redCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPRedChannel->targetContainer.get()) : nullptr;
    ChannelType* greenCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPGreenChannel->targetContainer.get()) : nullptr;
    ChannelType* blueCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPBlueChannel->targetContainer.get()) : nullptr;

    for (int i = 0; i < numberOfCells; i++) {
        ColorSwatch* cp = Brain::getInstance()->getColorSwatchById(i + 1);
        if (cp != nullptr) {
            float r = 0, g = 0, b = 0;
            for (int j = 0; j < cp->values.items.size(); j++) {
                ChannelType* ct = dynamic_cast<ChannelType*>(cp->values.items[j]->channelType->targetContainer.get());
                float val = cp->values.items[j]->paramValue->getValue();
                if (ct != nullptr && ct == redCT) r = val;
                else if (ct != nullptr && ct == greenCT) g = val;
                else if (ct != nullptr && ct == blueCT) b = val;
            }

            Colour btnColor((uint8)(r * 255), (uint8)(g * 255), (uint8)(b * 255));
            gridButtons[i]->setColour(TextButton::buttonColourId, btnColor);

            float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
            Colour textColor = luminance > 0.5f ? Colours::black : Colours::white;
            gridButtons[i]->setColour(TextButton::textColourOnId, textColor);
            gridButtons[i]->setColour(TextButton::textColourOffId, textColor);

            gridButtons[i]->setButtonText(cp->userName->getValue().toString());
        }
        else {
            gridButtons[i]->setButtonText("");
            gridButtons[i]->setColour(TextButton::buttonColourId, Colour(40, 40, 40));
            gridButtons[i]->setColour(TextButton::textColourOnId, Colour(96, 96, 96));
            gridButtons[i]->setColour(TextButton::textColourOffId, Colour(96, 96, 96));
        }
    }
}

void ColorSwatchGridView::newMessage(const ColorSwatchManager::ManagerEvent& e)
{
    updateCells();
}
