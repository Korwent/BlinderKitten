/*
  ==============================================================================

    ColorSwatchGridView.h
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GridView.h"
#include "Definitions/ColorSwatch/ColorSwatchManager.h"

//==============================================================================
class ColorSwatchGridViewUI : public ShapeShifterContent
{
public:
    ColorSwatchGridViewUI(const String& contentName);
    ~ColorSwatchGridViewUI();

    static ColorSwatchGridViewUI* create(const String& name) { return new ColorSwatchGridViewUI(name); }
};


class ColorSwatchGridView :
    public GridView,
    public ColorSwatchManager::AsyncListener
{
public:
    juce_DeclareSingleton(ColorSwatchGridView, true);
    ColorSwatchGridView();
    ~ColorSwatchGridView() override;

    void updateCells() override;
    void newMessage(const ColorSwatchManager::ManagerEvent& e) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorSwatchGridView)
};
