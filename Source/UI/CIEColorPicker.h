/*
  ==============================================================================

    CIEColorPicker.h
    Created: 16 Mar 2026
    Author:  BlinderKitten

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Common/ColorEngine/ColorEngine.h"

class BKEngine;

//==============================================================================
class CIEColorPickerUI : public ShapeShifterContent
{
public:
    CIEColorPickerUI(const String& contentName);
    ~CIEColorPickerUI();

    static CIEColorPickerUI* create(const String& name) { return new CIEColorPickerUI(name); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CIEColorPickerUI)
};

//==============================================================================
class CIEColorPicker : public juce::Component, public juce::Timer
{
public:
    juce_DeclareSingleton(CIEColorPicker, true);
    CIEColorPicker();
    ~CIEColorPicker() override;

    BKEngine* engine = nullptr;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void timerCallback() override;

private:
    // Last CIE xy set from a mouse click/drag (bypasses sRGB round-trip for accurate cursor).
    // -1 means "derive cursor from encoder RGB instead."
    float cursorCIEX = -1.0f, cursorCIEY = -1.0f;
    // Cached tongue image (expensive per-pixel render, regenerated on resize)
    Image tongueImage;
    int cachedW = 0, cachedH = 0;

    // Per-fixture emitter calibrations (one entry per selected subfixture that has calibration data)
    Array<Array<EmitterCalibration>> allFixtureEmitters;
    // Intersection gamut polygon in CIE xy space (result of clipping all fixture gamuts)
    Array<juce::Point<float>> gamutPolygonCIE;

    // CIE xy diagram render bounds
    static constexpr float cieXMin = 0.0f, cieXMax = 0.80f;
    static constexpr float cieYMin = 0.0f, cieYMax = 0.85f;
    static constexpr float margin  = 14.0f;

    // Coordinate helpers
    juce::Point<float> cieToScreen(float cx, float cy) const;
    juce::Point<float> screenToCIE(float sx, float sy) const;

    // Rendering helpers
    void rebuildTongueImage();
    Path buildTonguePath() const;
    Path buildSpectralArcPath() const;
    Path buildGamutPath() const;
    void setTargetFromScreenPos(float sx, float sy, bool freeMove = false);
    void updateGamutFromSelection();

    // Geometry helpers (CIE xy space)
    static Array<juce::Point<float>> convexPolygonFromEmitters(const Array<EmitterCalibration>& emitters);
    static Array<juce::Point<float>> sutherlandHodgman(Array<juce::Point<float>> subject,
                                                        const Array<juce::Point<float>>& clip);
    static bool isInsideEdge(juce::Point<float> p, juce::Point<float> a, juce::Point<float> b);
    static juce::Point<float> edgeIntersection(juce::Point<float> a, juce::Point<float> b,
                                                juce::Point<float> c, juce::Point<float> d);
    static juce::Point<float> clampToPolygon(juce::Point<float> p, const Array<juce::Point<float>>& poly);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CIEColorPicker)
};
