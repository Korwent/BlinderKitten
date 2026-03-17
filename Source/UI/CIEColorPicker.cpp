/*
  ==============================================================================

    CIEColorPicker.cpp
    Created: 16 Mar 2026
    Author:  BlinderKitten

  ==============================================================================
*/

#include <JuceHeader.h>
#include "CIEColorPicker.h"
#include "UserInputManager.h"
#include "BKEngine.h"
#include "Definitions/Command/Command.h"
#include "Common/ColorEngine/ColorEngine.h"
#include "Definitions/SubFixture/SubFixture.h"
#include "Definitions/ChannelFamily/ChannelType/ChannelType.h"
#include "Definitions/FixtureType/FixtureTypeChannel.h"
#include "Definitions/Fixture/Fixture.h"

// =============================================================================
// CIE 1931 2° Standard Observer – Spectral Locus (380–700 nm, 5 nm steps)
// source: CIE publication 015:2004, Table T.1 (public-domain scientific data)
// =============================================================================
static const float kSpectralLocus[][2] = {
    {0.1741f, 0.0050f}, // 380
    {0.1740f, 0.0050f}, // 385
    {0.1738f, 0.0049f}, // 390
    {0.1736f, 0.0049f}, // 395
    {0.1733f, 0.0048f}, // 400
    {0.1730f, 0.0048f}, // 405
    {0.1726f, 0.0048f}, // 410
    {0.1721f, 0.0048f}, // 415
    {0.1714f, 0.0051f}, // 420
    {0.1703f, 0.0058f}, // 425
    {0.1689f, 0.0069f}, // 430
    {0.1669f, 0.0086f}, // 435
    {0.1644f, 0.0109f}, // 440
    {0.1611f, 0.0138f}, // 445
    {0.1566f, 0.0177f}, // 450
    {0.1510f, 0.0227f}, // 455
    {0.1440f, 0.0297f}, // 460
    {0.1355f, 0.0399f}, // 465
    {0.1241f, 0.0578f}, // 470
    {0.1096f, 0.0868f}, // 475
    {0.0913f, 0.1327f}, // 480
    {0.0687f, 0.2007f}, // 485
    {0.0454f, 0.2950f}, // 490
    {0.0235f, 0.4127f}, // 495
    {0.0082f, 0.5384f}, // 500
    {0.0039f, 0.6548f}, // 505
    {0.0139f, 0.7502f}, // 510
    {0.0389f, 0.8120f}, // 515
    {0.0743f, 0.8338f}, // 520
    {0.1142f, 0.8262f}, // 525
    {0.1547f, 0.8059f}, // 530
    {0.1929f, 0.7816f}, // 535
    {0.2296f, 0.7543f}, // 540
    {0.2658f, 0.7243f}, // 545
    {0.3016f, 0.6923f}, // 550
    {0.3373f, 0.6589f}, // 555
    {0.3731f, 0.6245f}, // 560
    {0.4087f, 0.5896f}, // 565
    {0.4441f, 0.5547f}, // 570
    {0.4788f, 0.5202f}, // 575
    {0.5125f, 0.4866f}, // 580
    {0.5448f, 0.4544f}, // 585
    {0.5752f, 0.4242f}, // 590
    {0.6029f, 0.3965f}, // 595
    {0.6270f, 0.3725f}, // 600
    {0.6482f, 0.3514f}, // 605
    {0.6658f, 0.3340f}, // 610
    {0.6801f, 0.3197f}, // 615
    {0.6915f, 0.3083f}, // 620
    {0.7006f, 0.2993f}, // 625
    {0.7079f, 0.2920f}, // 630
    {0.7140f, 0.2859f}, // 635
    {0.7190f, 0.2809f}, // 640
    {0.7230f, 0.2770f}, // 645
    {0.7260f, 0.2740f}, // 650
    {0.7283f, 0.2717f}, // 655
    {0.7300f, 0.2700f}, // 660
    {0.7311f, 0.2689f}, // 665
    {0.7320f, 0.2680f}, // 670
    {0.7327f, 0.2673f}, // 675
    {0.7334f, 0.2666f}, // 680
    {0.7340f, 0.2660f}, // 685
    {0.7344f, 0.2656f}, // 690
    {0.7346f, 0.2654f}, // 695
    {0.7347f, 0.2653f}, // 700
};
static const int kSpectralLocusSize = (int)(sizeof(kSpectralLocus) / sizeof(kSpectralLocus[0]));

// =============================================================================
// CIEColorPickerUI
// =============================================================================
juce_ImplementSingleton(CIEColorPicker);

CIEColorPickerUI::CIEColorPickerUI(const String& contentName)
    : ShapeShifterContent(CIEColorPicker::getInstance(), contentName)
{
}

CIEColorPickerUI::~CIEColorPickerUI()
{
}

// =============================================================================
// CIEColorPicker
// =============================================================================
CIEColorPicker::CIEColorPicker()
{
    startTimer(66); // ~15 Hz
}

CIEColorPicker::~CIEColorPicker()
{
    stopTimer();
    clearSingletonInstance();
}

// ---------------------------------------------------------------------------
// Coordinate mapping
// ---------------------------------------------------------------------------
juce::Point<float> CIEColorPicker::cieToScreen(float cx, float cy) const
{
    float drawW = (float)getWidth()  - 2.0f * margin;
    float drawH = (float)getHeight() - 2.0f * margin;
    // CIE y increases upward; screen y increases downward
    float sx = margin + ((cx - cieXMin) / (cieXMax - cieXMin)) * drawW;
    float sy = margin + (1.0f - (cy - cieYMin) / (cieYMax - cieYMin)) * drawH;
    return { sx, sy };
}

juce::Point<float> CIEColorPicker::screenToCIE(float sx, float sy) const
{
    float drawW = (float)getWidth()  - 2.0f * margin;
    float drawH = (float)getHeight() - 2.0f * margin;
    float cx = cieXMin + ((sx - margin) / drawW) * (cieXMax - cieXMin);
    float cy = cieYMin + (1.0f - (sy - margin) / drawH) * (cieYMax - cieYMin);
    return { cx, cy };
}

// ---------------------------------------------------------------------------
// Tongue path
// ---------------------------------------------------------------------------
Path CIEColorPicker::buildTonguePath() const
{
    Path p;
    auto pt0 = cieToScreen(kSpectralLocus[0][0], kSpectralLocus[0][1]);
    p.startNewSubPath(pt0);
    for (int i = 1; i < kSpectralLocusSize; i++) {
        auto pt = cieToScreen(kSpectralLocus[i][0], kSpectralLocus[i][1]);
        p.lineTo(pt);
    }
    p.closeSubPath(); // purple line back to start
    return p;
}

// ---------------------------------------------------------------------------
// Tongue image (per-pixel coloring)
// ---------------------------------------------------------------------------
void CIEColorPicker::rebuildTongueImage()
{
    int w = getWidth();
    int h = getHeight();
    if (w <= 0 || h <= 0) return;

    tongueImage = Image(Image::ARGB, w, h, true);
    Image::BitmapData bmp(tongueImage, Image::BitmapData::writeOnly);

    Path tonguePath = buildTonguePath();

    // Purple line endpoints (from spectral locus 380nm → 700nm)
    static const float plX0 = kSpectralLocus[0][0],              plY0 = kSpectralLocus[0][1];
    static const float plX1 = kSpectralLocus[kSpectralLocusSize-1][0], plY1 = kSpectralLocus[kSpectralLocusSize-1][1];

    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            auto cie = screenToCIE((float)px, (float)py);
            float cx = cie.x, cy = cie.y;

            if (!tonguePath.contains((float)px, (float)py)) continue;
            if (cx < 0.0f || cy < 0.0f) continue;

            // Crop: skip pixels below the purple line (the straight closing edge
            // from 700 nm back to 380 nm) so that coloured area stays within
            // the visible-spectrum arc only.
            float yLine = plY0 + (plY1 - plY0) / (plX1 - plX0) * (cx - plX0);
            if (cy < yLine - 0.002f) continue;

            float X, Y, Z;
            ColorEngine::xyYtoXYZ(cx, cy, 0.45f, X, Y, Z);

            float r, g, b;
            ColorEngine::XYZtoSRGB(X, Y, Z, r, g, b);

            uint8 ri = (uint8)jlimit(0, 255, (int)(r * 255.0f));
            uint8 gi = (uint8)jlimit(0, 255, (int)(g * 255.0f));
            uint8 bi = (uint8)jlimit(0, 255, (int)(b * 255.0f));

            bmp.setPixelColour(px, py, Colour(ri, gi, bi));
        }
    }

    cachedW = w;
    cachedH = h;
}

// ---------------------------------------------------------------------------
// Spectral arc path (open – no purple line closing, used for outline stroke)
// ---------------------------------------------------------------------------
Path CIEColorPicker::buildSpectralArcPath() const
{
    Path p;
    auto pt0 = cieToScreen(kSpectralLocus[0][0], kSpectralLocus[0][1]);
    p.startNewSubPath(pt0);
    for (int i = 1; i < kSpectralLocusSize; i++) {
        auto pt = cieToScreen(kSpectralLocus[i][0], kSpectralLocus[i][1]);
        p.lineTo(pt);
    }
    // No closeSubPath – leaves the purple line segment out of the outline
    return p;
}

// ---------------------------------------------------------------------------
// Geometry helpers (Sutherland-Hodgman polygon clipping, CIE xy space)
// ---------------------------------------------------------------------------
bool CIEColorPicker::isInsideEdge(juce::Point<float> p, juce::Point<float> a, juce::Point<float> b)
{
    // True if p is on the left side (inside) of directed edge a→b
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x) >= 0.0f;
}

juce::Point<float> CIEColorPicker::edgeIntersection(juce::Point<float> a, juce::Point<float> b,
                                                     juce::Point<float> c, juce::Point<float> d)
{
    float A1 = b.y - a.y, B1 = a.x - b.x, C1 = A1 * a.x + B1 * a.y;
    float A2 = d.y - c.y, B2 = c.x - d.x, C2 = A2 * c.x + B2 * c.y;
    float det = A1 * B2 - A2 * B1;
    if (fabsf(det) < 1e-10f) return a; // parallel — return a safe point
    return { (C1 * B2 - C2 * B1) / det, (A1 * C2 - A2 * C1) / det };
}

Array<juce::Point<float>> CIEColorPicker::sutherlandHodgman(Array<juce::Point<float>> subject,
                                                             const Array<juce::Point<float>>& clip)
{
    if (clip.size() < 3) return subject;

    Array<juce::Point<float>> output = subject;

    for (int i = 0; i < clip.size(); i++) {
        if (output.isEmpty()) return {};

        Array<juce::Point<float>> input = output;
        output.clear();

        juce::Point<float> edgeA = clip[i];
        juce::Point<float> edgeB = clip[(i + 1) % clip.size()];

        for (int j = 0; j < input.size(); j++) {
            juce::Point<float> current = input[j];
            juce::Point<float> prev    = input[(j + input.size() - 1) % input.size()];

            if (isInsideEdge(current, edgeA, edgeB)) {
                if (!isInsideEdge(prev, edgeA, edgeB))
                    output.add(edgeIntersection(prev, current, edgeA, edgeB));
                output.add(current);
            } else if (isInsideEdge(prev, edgeA, edgeB)) {
                output.add(edgeIntersection(prev, current, edgeA, edgeB));
            }
        }
    }
    return output;
}

Array<juce::Point<float>> CIEColorPicker::convexPolygonFromEmitters(const Array<EmitterCalibration>& emitters)
{
    // Build a true CCW convex hull using Graham scan so that:
    //  - Interior emitters (e.g. white in RGBW) are excluded from the hull
    //  - The winding is always CCW regardless of emitter order/selection order
    //  - Sutherland-Hodgman gives consistent results regardless of fixture selection order

    int n = emitters.size();
    if (n < 3) return {};

    Array<juce::Point<float>> pts;
    for (const auto& e : emitters) pts.add({ e.cieX, e.cieY });

    // 1. Find the lowest-y point (leftmost if tie) — guaranteed on the hull
    int pivot = 0;
    for (int i = 1; i < n; i++) {
        if (pts[i].y < pts[pivot].y || (pts[i].y == pts[pivot].y && pts[i].x < pts[pivot].x))
            pivot = i;
    }
    std::swap(pts.getReference(0), pts.getReference(pivot));
    juce::Point<float> p0 = pts[0];

    // 2. Sort remaining points by polar angle from p0 (CCW), break ties by distance
    auto cross2d = [](juce::Point<float> O, juce::Point<float> A, juce::Point<float> B) {
        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
    };
    auto dist2 = [](juce::Point<float> a, juce::Point<float> b) {
        float dx = a.x - b.x, dy = a.y - b.y;
        return dx * dx + dy * dy;
    };

    std::sort(pts.begin() + 1, pts.end(), [&](const juce::Point<float>& a, const juce::Point<float>& b) {
        float c = cross2d(p0, a, b);
        if (fabsf(c) > 1e-10f) return c > 0.0f; // a is more CCW
        return dist2(p0, a) < dist2(p0, b);      // closer first
    });

    // 3. Graham scan — keep only left turns
    Array<juce::Point<float>> hull;
    for (int i = 0; i < pts.size(); i++) {
        while (hull.size() >= 2 &&
               cross2d(hull[hull.size() - 2], hull[hull.size() - 1], pts[i]) <= 0.0f)
            hull.removeLast();
        hull.add(pts[i]);
    }

    return hull;
}

// ---------------------------------------------------------------------------
// Gamut path (built from the pre-computed intersection polygon)
// ---------------------------------------------------------------------------
Path CIEColorPicker::buildGamutPath() const
{
    if (gamutPolygonCIE.size() < 2) return {};

    Path p;
    auto first = cieToScreen(gamutPolygonCIE[0].x, gamutPolygonCIE[0].y);
    p.startNewSubPath(first);
    for (int i = 1; i < gamutPolygonCIE.size(); i++) {
        auto pt = cieToScreen(gamutPolygonCIE[i].x, gamutPolygonCIE[i].y);
        p.lineTo(pt);
    }
    p.closeSubPath();
    return p;
}

// ---------------------------------------------------------------------------

// paint
// ---------------------------------------------------------------------------
void CIEColorPicker::paint(juce::Graphics& g)
{
    if (engine == nullptr)
        engine = dynamic_cast<BKEngine*>(Engine::mainEngine);

    g.fillAll(Colours::black);

    if (tongueImage.isNull() || cachedW != getWidth() || cachedH != getHeight())
        rebuildTongueImage();

    g.drawImage(tongueImage, getLocalBounds().toFloat());

    // Spectral arc outline (no purple line)
    g.setColour(Colours::white.withAlpha(0.5f));
    g.strokePath(buildSpectralArcPath(), PathStrokeType(1.0f));

    // Gamut polygon
    if (gamutPolygonCIE.size() >= 3) {
        Path gamutPath = buildGamutPath();

        g.setColour(Colour(0, 0, 80));
        g.strokePath(gamutPath, PathStrokeType(2.0f));

        // Vertex dots and labels for a single fixture
        if (allFixtureEmitters.size() == 1) {
            g.setFont(10.0f);
            for (const auto& e : allFixtureEmitters[0]) {
                auto pt = cieToScreen(e.cieX, e.cieY);
                g.setColour(Colours::white);
                g.fillEllipse(pt.x - 3.0f, pt.y - 3.0f, 6.0f, 6.0f);
                if (e.typeChannel != nullptr) {
                    g.setColour(Colours::white.withAlpha(0.8f));
                    g.drawText(e.typeChannel->niceName, (int)pt.x + 5, (int)pt.y - 6, 80, 14,
                               Justification::left, false);
                }
            }
        }
    }

    // Cursor: show current encoder RGB value as a circle on the CIE diagram
    {
        Command* cmd = UserInputManager::getInstance()->targetCommand;
        if (cmd != nullptr && engine != nullptr) {
            float drawCIEX = -1, drawCIEY = -1;

            // Prefer the directly stored CIE xy (set from the last mouse click/drag).
            // This avoids the sRGB round-trip which clamps out-of-gamut colors and
            // shifts the cursor away from the clicked location.
            if (cursorCIEX >= 0 && cursorCIEY >= 0) {
                drawCIEX = cursorCIEX;
                drawCIEY = cursorCIEY;
            } else {
                // Fall back to deriving CIE xy from encoder RGB values.
                float red = -1, green = -1, blue = -1;
                float cyan = -1, magenta = -1, yellow = -1;
                for (int i = 0; i < cmd->values.items.size(); i++) {
                    CommandValue* cv = cmd->values.items[i];
                    if (cv->channelType->getValue() == engine->CPRedChannel->getValue())     { red     = cv->valueFrom->getValue(); }
                    if (cv->channelType->getValue() == engine->CPGreenChannel->getValue())   { green   = cv->valueFrom->getValue(); }
                    if (cv->channelType->getValue() == engine->CPBlueChannel->getValue())    { blue    = cv->valueFrom->getValue(); }
                    if (cv->channelType->getValue() == engine->CPCyanChannel->getValue())    { cyan    = cv->valueFrom->getValue(); }
                    if (cv->channelType->getValue() == engine->CPMagentaChannel->getValue()) { magenta = cv->valueFrom->getValue(); }
                    if (cv->channelType->getValue() == engine->CPYellowChannel->getValue())  { yellow  = cv->valueFrom->getValue(); }
                }

                float r = -1, gv = -1, b = -1;
                if (red != -1 && green != -1 && blue != -1) {
                    r = red; gv = green; b = blue;
                } else if (cyan != -1 && magenta != -1 && yellow != -1) {
                    r = 1.0f - cyan; gv = 1.0f - magenta; b = 1.0f - yellow;
                }

                if (r >= 0 && gv >= 0 && b >= 0) {
                    float X, Y, Z;
                    ColorEngine::sRGBtoXYZ(r, gv, b, X, Y, Z);
                    float sum = X + Y + Z;
                    if (sum > 0.001f) {
                        drawCIEX = X / sum;
                        drawCIEY = Y / sum;
                    }
                }
            }

            if (drawCIEX >= 0 && drawCIEY >= 0) {
                auto pt = cieToScreen(drawCIEX, drawCIEY);
                g.setColour(Colours::black);
                g.drawEllipse(pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f, 1.5f);
            }
        }
    }
}

void CIEColorPicker::resized()
{
    // Force tongue image rebuild on next paint
    cachedW = 0;
    cachedH = 0;
}

// ---------------------------------------------------------------------------
// Mouse interaction – fires on release (covers both click and drag-end)
// ---------------------------------------------------------------------------
void CIEColorPicker::mouseUp(const MouseEvent& e)
{
    setTargetFromScreenPos(e.position.getX(), e.position.getY(), e.mods.isShiftDown());
}

void CIEColorPicker::mouseDrag(const MouseEvent& e)
{
    setTargetFromScreenPos(e.position.getX(), e.position.getY(), e.mods.isShiftDown());
}

juce::Point<float> CIEColorPicker::clampToPolygon(juce::Point<float> p, const Array<juce::Point<float>>& poly)
{
    // Check if already inside (all edges pass)
    bool inside = true;
    int n = poly.size();
    for (int i = 0; i < n; i++) {
        if (!isInsideEdge(p, poly[i], poly[(i + 1) % n])) { inside = false; break; }
    }
    if (inside) return p;

    // Find closest point on any polygon edge
    float bestDist = std::numeric_limits<float>::max();
    juce::Point<float> best = p;
    for (int i = 0; i < n; i++) {
        auto a = poly[i];
        auto b = poly[(i + 1) % n];
        auto ab = b - a;
        float len2 = ab.x * ab.x + ab.y * ab.y;
        if (len2 < 1e-12f) continue;
        float t = jlimit(0.0f, 1.0f, ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2);
        juce::Point<float> closest(a.x + t * ab.x, a.y + t * ab.y);
        float d = closest.getDistanceFrom(p);
        if (d < bestDist) { bestDist = d; best = closest; }
    }
    return best;
}

void CIEColorPicker::setTargetFromScreenPos(float sx, float sy, bool freeMove)
{
    if (engine == nullptr) engine = dynamic_cast<BKEngine*>(Engine::mainEngine);
    if (engine == nullptr) return;

    auto cie = screenToCIE(sx, sy);
    float pickedX = jlimit(cieXMin, cieXMax, cie.x);
    float pickedY = jlimit(cieYMin, cieYMax, cie.y);

    // Clamp to gamut polygon unless Shift is held or no gamut is available
    if (!freeMove && gamutPolygonCIE.size() >= 3) {
        auto clamped = clampToPolygon({ pickedX, pickedY }, gamutPolygonCIE);
        pickedX = clamped.x;
        pickedY = clamped.y;
    }

    // Store the raw CIE xy target so the cursor can be drawn at exactly this
    // position without going through an sRGB round-trip (which clamps out-of-gamut
    // colors and shifts the cursor away from the clicked point).
    cursorCIEX = pickedX;
    cursorCIEY = pickedY;

    // Convert CIE xy to XYZ (Y=1 for full brightness), then to sRGB
    float X, Y, Z;
    ColorEngine::xyYtoXYZ(pickedX, pickedY, 1.0f, X, Y, Z);
    float r, g, b;
    ColorEngine::XYZtoSRGB(X, Y, Z, r, g, b);

    // Push values through the standard RGB encoder channels
    if (engine->CPRedChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPRedChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, r);
    }
    if (engine->CPGreenChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPGreenChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, g);
    }
    if (engine->CPBlueChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPBlueChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, b);
    }
}

// ---------------------------------------------------------------------------
// Timer: refresh gamut and cursor from active command
// ---------------------------------------------------------------------------
void CIEColorPicker::updateGamutFromSelection()
{
    Command* cmd = UserInputManager::getInstance()->targetCommand;
    if (cmd == nullptr) { allFixtureEmitters.clear(); gamutPolygonCIE.clear(); cursorCIEX = -1; cursorCIEY = -1; return; }

    auto& subs = cmd->selection.computedSelectedSubFixtures;
    if (subs.size() == 0) { allFixtureEmitters.clear(); gamutPolygonCIE.clear(); return; }

    // Deduplicate by (FixtureType*, subId): all fixtures of the same type share the
    // same FixtureTypeChannel calibration data, so computing the gamut for one
    // representative per unique (type, subId) pair is sufficient and avoids doing
    // O(N) identical polygon intersections when many fixtures of the same type are selected.
    Array<std::pair<ControllableContainer*, int>> seenTypeSubId;
    allFixtureEmitters.clear();

    for (int i = 0; i < subs.size(); i++) {
        SubFixture* sf = subs[i];
        if (sf == nullptr || sf->parentFixture == nullptr) continue;

        ControllableContainer* fixtureType = sf->parentFixture->devTypeParam->targetContainer.get();
        int subId = sf->subId;

        // Check if we already have a representative for this (type, subId) pair
        bool alreadySeen = false;
        for (const auto& entry : seenTypeSubId) {
            if (entry.first == fixtureType && entry.second == subId) {
                alreadySeen = true;
                break;
            }
        }
        if (alreadySeen) continue;

        Array<EmitterCalibration> emitters = ColorEngine::getCalibrations(sf);
        if (emitters.size() >= 3) {
            seenTypeSubId.add({ fixtureType, subId });
            allFixtureEmitters.add(emitters);
        }
    }

    if (allFixtureEmitters.isEmpty()) { gamutPolygonCIE.clear(); return; }

    // Start with first unique type's gamut polygon
    Array<juce::Point<float>> intersection = convexPolygonFromEmitters(allFixtureEmitters[0]);

    // Clip successively by each additional unique type's gamut polygon
    for (int i = 1; i < allFixtureEmitters.size(); i++) {
        Array<juce::Point<float>> clip = convexPolygonFromEmitters(allFixtureEmitters[i]);
        intersection = sutherlandHodgman(intersection, clip);
        if (intersection.isEmpty()) break;
    }

    gamutPolygonCIE = intersection;
}

void CIEColorPicker::timerCallback()
{
    if (engine == nullptr) engine = dynamic_cast<BKEngine*>(Engine::mainEngine);
    updateGamutFromSelection();
    repaint();
}

void CIEColorPicker::clearCursorOverrideFromExternal()
{
    cursorCIEX = -1.0f;
    cursorCIEY = -1.0f;
}
