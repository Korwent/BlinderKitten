/*
  ==============================================================================

    ColorPaletteValue.cpp
    Created: 17 Mar 2026
    Author:  No

  ==============================================================================
*/

#include "ColorPaletteValue.h"
#include "../ChannelFamily/ChannelFamilyManager.h"

ColorPaletteValue::ColorPaletteValue(var params) :
    BaseItem(params.getProperty("name", "Value"))
{
    itemDataType = "ColorPaletteValue";
    channelType = addTargetParameter("Channel type", "Type of data of this channel", ChannelFamilyManager::getInstance());
    channelType->targetType = TargetParameter::CONTAINER;
    channelType->maxDefaultSearchLevel = 2;
    channelType->typesFilter.add("ChannelType");
    canBeDisabled = false;
    paramValue = addFloatParameter("Value", "Value of this channel", 0, 0, 1);
}

ColorPaletteValue::~ColorPaletteValue()
{
}
