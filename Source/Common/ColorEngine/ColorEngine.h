/*
  ==============================================================================

    ColorEngine.h
    Created: 10 Mar 2026
    Author:  BlinderKitten

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class SubFixture;
class SubFixtureChannel;
class ChannelType;
class FixtureTypeChannel;

struct EmitterCalibration {
    SubFixtureChannel* channel;          // the emitter's SubFixtureChannel
    FixtureTypeChannel* typeChannel;     // the FixtureTypeChannel with calibration data
    float cieX;                          // CIE x chromaticity
    float cieY;                          // CIE y chromaticity
    float maxIntensityCd;                // luminous intensity at DMX 100%
    // Tristimulus XYZ at full intensity (computed from above)
    float fullX, fullY, fullZ;
};

class ColorEngine {
public:
    // === CIE Conversions ===

    // sRGB [0,1] → CIE XYZ (D65, 2° observer)
    static void sRGBtoXYZ(float r, float g, float b, float& X, float& Y, float& Z);

    // CIE XYZ → sRGB [0,1] (clamped)
    static void XYZtoSRGB(float X, float Y, float Z, float& r, float& g, float& b);

    // CIE xy + Y (luminance) → XYZ
    static void xyYtoXYZ(float x, float y, float Yval, float& X, float& Y, float& Z);

    // CIE XYZ → xy (returns false if Y==0)
    static bool XYZtoXY(float X, float Y, float Z, float& x, float& y);

    // === Calibrated Color Solving ===

    // Collect calibration data for all calibrated emitters in a SubFixture.
    // Returns empty array if no emitters have calibration data.
    static Array<EmitterCalibration> getCalibrations(SubFixture* sf);

    // Check if a SubFixture has enough calibration data for color solving (>= 3 calibrated emitters)
    static bool hasCalibrationData(SubFixture* sf);

    // Given a target CIE XYZ, compute emitter weights [0..1] for each calibrated emitter.
    // Uses direct solve for 3 emitters, combinatorial search for >3.
    // Returns false if the target is outside the gamut.
    // Output: fills outWeights array with per-emitter weights
    static bool solveEmitterWeights(Array<EmitterCalibration>& emitters,
                                     float targetX, float targetY, float targetZ,
                                     Array<float>& outWeights);

    // Convert a weight (relative intensity 0..1) to a DMX-level float (0..1) using
    // the emitter's dimming curve (inverse lookup via binary search).
    // If no dimming curve enabled, weight == DMX level (linear).
    static float weightToDMXLevel(const EmitterCalibration& emitter, float weight);

    // High-level: given target sRGB color and intensity, set all calibrated emitter
    // channels on a SubFixture. Returns false if not enough calibration data.
    static bool applyTargetColorToSubFixture(SubFixture* sf,
                                              float r, float g, float b,
                                              float intensity);

    // High-level: read current emitter values on a SubFixture and compute the
    // resulting CIE XYZ (for accurate color preview). Returns black if uncalibrated.
    static Colour getCalibrationOutputColor(SubFixture* sf);

    // High-level: given target sRGB color and intensity, compute the DMX level for each
    // calibrated emitter. Fills outLevels with SubFixtureChannel -> DMX value (0..1).
    // Returns false if not enough calibration data.
    static bool computeEmitterDMXLevels(SubFixture* sf,
                                         float r, float g, float b,
                                         float intensity,
                                         HashMap<SubFixtureChannel*, float>& outLevels);

private:
    // Helper: 3x3 direct solve using Cramer's rule
    static bool solve3x3(const Array<EmitterCalibration>& emitters,
                         float targetX, float targetY, float targetZ,
                         Array<float>& outWeights);
};
