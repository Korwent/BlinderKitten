/*
  ==============================================================================

    ColorPaletteContentView.h
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Definitions/ColorPalette/ColorPalette.h"
#include "Definitions/ColorPalette/ColorPaletteItem.h"
#include "Definitions/ColorPalette/ColorPaletteManager.h"

//==============================================================================
class ColorPaletteItemButton :
    public TextButton,
    public DragAndDropContainer
{
public:
    ColorPaletteItemButton();
    ~ColorPaletteItemButton();

    int itemIndex = -1;   // index in palette->items
    int swatchId = 0;     // resolved swatch ID for drag data

    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
};

//==============================================================================
class ColorPaletteContentView :
    public ShapeShifterContentComponent,
    public DragAndDropTarget,
    public InspectableSelectionManager::AsyncListener,
    public ColorPaletteManager::AsyncListener,
    public ControllableContainer::ControllableContainerListener,
    public Button::Listener
{
public:
    ColorPaletteContentView(const String& contentName);
    ~ColorPaletteContentView() override;

    WeakReference<ColorPalette> currentPalette;
    OwnedArray<ColorPaletteItemButton> itemButtons;

    void setCurrentPalette(ColorPalette* palette);
    void refreshButtons();

    void paint(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& e) override;

    // Button::Listener
    void buttonClicked(Button* button) override;

    // InspectableSelectionManager::AsyncListener
    void newMessage(const InspectableSelectionManager::SelectionEvent& e) override;

    // ColorPaletteManager::AsyncListener
    void newMessage(const ColorPaletteManager::ManagerEvent& e) override;

    // ControllableContainerListener (palette items changed)
    void childStructureChanged(ControllableContainer* cc) override;
    void controllableContainerReordered(ControllableContainer* cc) override;
    // Provide empty defaults for other required overrides
    void controllableContainerAdded(ControllableContainer* cc) override {};
    void controllableContainerRemoved(ControllableContainer* cc) override {};

    // DragAndDropTarget
    bool isInterestedInDragSource(const SourceDetails& source) override;
    void itemDropped(const SourceDetails& source) override;

    void showContextMenu(int itemIndex);

    static ColorPaletteContentView* create(const String& name) { return new ColorPaletteContentView(name); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorPaletteContentView)
};

