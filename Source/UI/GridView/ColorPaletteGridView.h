/*
  ==============================================================================

    ColorPaletteGridView.h
    Created: 17 Mar 2026
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
    public ColorPaletteManager::AsyncListener
{
public:
    juce_DeclareSingleton(ColorPaletteGridView, true);
    ColorPaletteGridView();
    ~ColorPaletteGridView() override;

    void updateCells() override;
    void newMessage(const ColorPaletteManager::ManagerEvent& e) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorPaletteGridView)
};
