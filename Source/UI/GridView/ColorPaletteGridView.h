/*
  ==============================================================================

    ColorPaletteGridView.h
    Created: 18 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GridView.h"
#include "Definitions/ColorPalette/ColorPaletteManager.h"

//==============================================================================
class ColorPaletteGridViewUI : public ShapeShifterContent
{
public:
    ColorPaletteGridViewUI(const String& contentName);
    ~ColorPaletteGridViewUI();

    static ColorPaletteGridViewUI* create(const String& name) { return new ColorPaletteGridViewUI(name); }
};


class ColorPaletteGridView :
    public GridView,
    public ColorPaletteManager::AsyncListener,
    public DragAndDropTarget
{
public:
    juce_DeclareSingleton(ColorPaletteGridView, true);
    ColorPaletteGridView();
    ~ColorPaletteGridView() override;

    void updateCells() override;
    void newMessage(const ColorPaletteManager::ManagerEvent& e) override;

    bool isInterestedInDragSource(const SourceDetails& source) override;
    void itemDropped(const SourceDetails& source) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorPaletteGridView)
};
