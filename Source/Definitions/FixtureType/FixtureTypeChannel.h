/*
  ==============================================================================

    ChannelType.h
    Created: 7 Nov 2021 7:40:48pm
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class FixtureTypeChannel:
    public BaseItem
{
    public:
    FixtureTypeChannel(var params = var());
    ~FixtureTypeChannel();

    String objectType;
    var objectData;

    TargetParameter* channelType;
    // StringParameter* suffix;
    // StringParameter* idGroupName;
    IntParameter* subFixtureId;
    FloatParameter* defaultValue;
    FloatParameter* highlightValue;
    BoolParameter* killedBySWOP;
    BoolParameter* invertOutput;
    EnumParameter* resolution;
    EnumParameter* fadeOrSnap;
    IntParameter* dmxDelta;
    TargetParameter* virtualMaster;

    Point2DParameter* physicalRange;
    Automation curve;

    // === Calibration data ===
    ControllableContainer calibrationContainer;
    Point2DParameter* calibrationCIExy;      // CIE xy chromaticity (x, y) of this emitter at 100%
    FloatParameter* calibrationMaxIntensity;  // Luminous intensity in candela (cd) at DMX 255

    void onContainerParameterChangedInternal(Parameter* p);
    void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) override;

};


