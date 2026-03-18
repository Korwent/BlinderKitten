/*
  ==============================================================================

    ColorPaletteContentView.cpp
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ColorPaletteContentView.h"
#include "Brain.h"
#include "BKEngine.h"
#include "Definitions/ColorPalette/ColorPalette.h"
#include "Definitions/ColorPalette/ColorPaletteItem.h"
#include "Definitions/ColorPalette/ColorPaletteManager.h"
#include "Definitions/ColorSwatch/ColorSwatch.h"
#include "Definitions/ColorSwatch/ColorSwatchValue.h"
#include "Definitions/ChannelFamily/ChannelType/ChannelType.h"
#include "UserInputManager.h"
#include "Definitions/Programmer/Programmer.h"

//==============================================================================
// ColorPaletteItemButton

ColorPaletteItemButton::ColorPaletteItemButton()
{
    setWantsKeyboardFocus(false);
}

ColorPaletteItemButton::~ColorPaletteItemButton()
{
}

void ColorPaletteItemButton::mouseDown(const MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    Button::mouseDown(e);
}

void ColorPaletteItemButton::mouseUp(const MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) {
        ColorPaletteContentView* parent = findParentComponentOfClass<ColorPaletteContentView>();
        if (parent != nullptr) parent->showContextMenu(itemIndex);
        return;
    }
    Button::mouseUp(e);
}

void ColorPaletteItemButton::mouseDrag(const MouseEvent& e)
{
    if (e.getDistanceFromDragStart() > 10 && !isDragAndDropActive() && swatchId > 0)
    {
        var dragData(new DynamicObject());
        dragData.getDynamicObject()->setProperty("type", "GridViewButton");
        dragData.getDynamicObject()->setProperty("targetType", "colorpaletteitem");
        dragData.getDynamicObject()->setProperty("id", swatchId);
        dragData.getDynamicObject()->setProperty("itemIndex", itemIndex);
        startDragging(dragData, this, ScaledImage(), true);
    }
}

//==============================================================================
// ColorPaletteContentView

ColorPaletteContentView::ColorPaletteContentView(const String& contentName) :
    ShapeShifterContentComponent(contentName)
{
    InspectableSelectionManager::mainSelectionManager->addAsyncSelectionManagerListener(this);
    ColorPaletteManager::getInstance()->addAsyncManagerListener(this);

    // Sync to whatever is already selected when the view is opened
    ColorPalette* alreadySelected = InspectableSelectionManager::mainSelectionManager->getInspectableAs<ColorPalette>();
    if (alreadySelected != nullptr) setCurrentPalette(alreadySelected);
}

ColorPaletteContentView::~ColorPaletteContentView()
{
    InspectableSelectionManager::mainSelectionManager->removeAsyncSelectionManagerListener(this);
    if (ColorPaletteManager::getInstanceWithoutCreating() != nullptr)
        ColorPaletteManager::getInstance()->removeAsyncManagerListener(this);

    if (!currentPalette.wasObjectDeleted() && currentPalette != nullptr)
        currentPalette->items.removeControllableContainerListener(this);
}

void ColorPaletteContentView::setCurrentPalette(ColorPalette* palette)
{
    if (!currentPalette.wasObjectDeleted() && currentPalette != nullptr)
        currentPalette->items.removeControllableContainerListener(this);

    currentPalette = palette;

    if (currentPalette != nullptr)
        currentPalette->items.addControllableContainerListener(this);

    refreshButtons();
}

void ColorPaletteContentView::refreshButtons()
{
    itemButtons.clear();

    if (currentPalette.wasObjectDeleted() || currentPalette == nullptr) {
        repaint();
        resized();
        return;
    }

    BKEngine* bke = dynamic_cast<BKEngine*>(Engine::mainEngine);
    ChannelType* redCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPRedChannel->targetContainer.get()) : nullptr;
    ChannelType* greenCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPGreenChannel->targetContainer.get()) : nullptr;
    ChannelType* blueCT = (bke != nullptr) ? dynamic_cast<ChannelType*>(bke->CPBlueChannel->targetContainer.get()) : nullptr;

    for (int i = 0; i < currentPalette->items.items.size(); i++) {
        ColorPaletteItem* paletteItem = currentPalette->items.items[i];
        int swatchId = paletteItem->colorSwatchId->getValue();
        ColorSwatch* swatch = Brain::getInstance()->getColorSwatchById(swatchId);

        ColorPaletteItemButton* btn = new ColorPaletteItemButton();
        btn->itemIndex = i;
        btn->swatchId = swatchId;
        btn->addListener(this);

        if (swatch != nullptr) {
            float r = 0, g = 0, b = 0;
            for (int j = 0; j < swatch->values.items.size(); j++) {
                ChannelType* ct = dynamic_cast<ChannelType*>(swatch->values.items[j]->channelType->targetContainer.get());
                float val = swatch->values.items[j]->paramValue->getValue();
                if (ct != nullptr && ct == redCT) r = val;
                else if (ct != nullptr && ct == greenCT) g = val;
                else if (ct != nullptr && ct == blueCT) b = val;
            }

            Colour btnColor((uint8)(r * 255), (uint8)(g * 255), (uint8)(b * 255));
            btn->setColour(TextButton::buttonColourId, btnColor);

            float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
            Colour textColor = luminance > 0.5f ? Colours::black : Colours::white;
            btn->setColour(TextButton::textColourOnId, textColor);
            btn->setColour(TextButton::textColourOffId, textColor);
            btn->setButtonText(swatch->userName->getValue().toString());
        } else {
            btn->setColour(TextButton::buttonColourId, Colour(80, 40, 40));
            btn->setButtonText("? " + String(swatchId));
        }

        itemButtons.add(btn);
        addAndMakeVisible(btn);
    }

    resized();
    repaint();
}

void ColorPaletteContentView::mouseDown(const MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    if (currentPalette.wasObjectDeleted() || currentPalette == nullptr) return;
    if (UserInputManager::getInstanceWithoutCreating() == nullptr) return;

    Programmer* p = UserInputManager::getInstance()->getProgrammer(false);
    if (p != nullptr && p->cliActionType->getValue().toString() != "")
    {
        UserInputManager::getInstance()->gridViewCellPressed("colorpalette", currentPalette->id->getValue());
    }
}

void ColorPaletteContentView::paint(Graphics& g)
{
    g.fillAll(Colour(30, 30, 30));

    if (currentPalette.wasObjectDeleted() || currentPalette == nullptr) {
        g.setColour(Colour(96, 96, 96));
        g.drawFittedText("Select a Color Palette to view its contents.", getLocalBounds(), Justification::centred, 3);
    }
}

void ColorPaletteContentView::resized()
{
    int btnSize = 80;
    int margin = 4;
    int w = getWidth();
    if (w <= 0) return;

    int cols = jmax(1, w / (btnSize + margin));
    int cellW = (w - margin * (cols + 1)) / cols;

    for (int i = 0; i < itemButtons.size(); i++) {
        int col = i % cols;
        int row = i / cols;
        int x = margin + col * (cellW + margin);
        int y = margin + row * (btnSize + margin);
        itemButtons[i]->setBounds(x, y, cellW, btnSize);
    }
}

void ColorPaletteContentView::buttonClicked(Button* button)
{
    ColorPaletteItemButton* btn = dynamic_cast<ColorPaletteItemButton*>(button);
    if (btn == nullptr) return;

    // Push the swatch's color values to the selected fixtures' encoders
    ColorSwatch* swatch = Brain::getInstance()->getColorSwatchById(btn->swatchId);
    if (swatch == nullptr) return;

    if (UserInputManager::getInstanceWithoutCreating() != nullptr) {
        for (int i = 0; i < swatch->values.items.size(); i++) {
            ColorSwatchValue* cpv = swatch->values.items[i];
            ChannelType* ct = dynamic_cast<ChannelType*>(cpv->channelType->targetContainer.get());
            if (ct != nullptr)
                UserInputManager::getInstance()->changeChannelValue(ct, cpv->paramValue->getValue());
        }
    }
}

void ColorPaletteContentView::newMessage(const InspectableSelectionManager::SelectionEvent& e)
{
    ColorPalette* selected = InspectableSelectionManager::mainSelectionManager->getInspectableAs<ColorPalette>();
    if (selected != nullptr) {
        setCurrentPalette(selected);
    }
}

void ColorPaletteContentView::newMessage(const ColorPaletteManager::ManagerEvent& e)
{
    // If the current palette was deleted, clear the view
    if (!currentPalette.wasObjectDeleted() && currentPalette != nullptr) {
        refreshButtons();
    } else if (currentPalette.wasObjectDeleted()) {
        setCurrentPalette(nullptr);
    }
}

void ColorPaletteContentView::childStructureChanged(ControllableContainer* cc)
{
    refreshButtons();
}

void ColorPaletteContentView::controllableContainerReordered(ControllableContainer* cc)
{
    refreshButtons();
}

bool ColorPaletteContentView::isInterestedInDragSource(const SourceDetails& source)
{
    if (source.description.getProperty("type", "") == "GridViewButton") {
        String tt = source.description.getProperty("targetType", "").toString();
        return tt == "colorswatch" || tt == "colorpaletteitem";
    }
    return false;
}

void ColorPaletteContentView::itemDropped(const SourceDetails& source)
{
    if (currentPalette.wasObjectDeleted() || currentPalette == nullptr) return;

    String tt = source.description.getProperty("targetType", "").toString();

    if (tt == "colorpaletteitem")
    {
        // Reorder: find the drag source index and the target index from drop position
        int srcIndex = source.description.getProperty("itemIndex", -1);
        if (srcIndex < 0 || srcIndex >= currentPalette->items.items.size()) return;

        // Find which button is under the drop point
        int destIndex = srcIndex;
        for (int i = 0; i < itemButtons.size(); i++)
        {
            if (itemButtons[i]->getBounds().contains(source.localPosition.toInt()))
            {
                destIndex = i;
                break;
            }
        }
        if (destIndex == srcIndex) return;
        currentPalette->items.setItemIndex(currentPalette->items.items[srcIndex], destIndex);
        refreshButtons();
    }
    else if (tt == "colorswatch")
    {
        int swatchId = source.description.getProperty("id", 0);
        if (swatchId <= 0) return;
        ColorSwatch* swatch = Brain::getInstance()->getColorSwatchById(swatchId);
        if (swatch == nullptr) return;
        ColorPaletteItem* newItem = currentPalette->items.addItem();
        newItem->colorSwatchId->setValue(swatchId);
        newItem->setNiceName(swatch->userName->getValue().toString());
    }
}

void ColorPaletteContentView::showContextMenu(int itemIndex)
{
    if (currentPalette.wasObjectDeleted() || currentPalette == nullptr) return;
    if (itemIndex < 0 || itemIndex >= currentPalette->items.items.size()) return;

    ColorPaletteItem* item = currentPalette->items.items[itemIndex];

    PopupMenu menu;
    menu.addItem(1, "Copy");
    menu.addItem(2, "Edit referenced swatch");
    menu.addSeparator();
    menu.addItem(3, "Move Up", itemIndex > 0);
    menu.addItem(4, "Move Down", itemIndex < currentPalette->items.items.size() - 1);
    menu.addSeparator();
    menu.addItem(5, "Delete");

    menu.showMenuAsync(PopupMenu::Options(), [this, itemIndex](int result) {
        if (currentPalette.wasObjectDeleted() || currentPalette == nullptr) return;
        if (itemIndex < 0 || itemIndex >= currentPalette->items.items.size()) return;

        ColorPaletteItem* item = currentPalette->items.items[itemIndex];

        switch (result) {
        case 1: { // Copy
            ColorPaletteItem* newItem = currentPalette->items.addItem();
            newItem->colorSwatchId->setValue(item->colorSwatchId->getValue());
            break;
        }
        case 2: { // Edit referenced swatch
            int swatchId = item->colorSwatchId->getValue();
            ColorSwatch* swatch = Brain::getInstance()->getColorSwatchById(swatchId);
            if (swatch != nullptr) swatch->selectThis();
            break;
        }
        case 3: // Move Up
            currentPalette->items.setItemIndex(item, itemIndex - 1);
            break;
        case 4: // Move Down
            currentPalette->items.setItemIndex(item, itemIndex + 1);
            break;
        case 5: // Delete
            currentPalette->items.removeItem(item);
            break;
        default:
            break;
        }
    });
}
