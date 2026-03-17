# CIE xy Chromaticity Diagram Color Picker

## Goal

Create a new **CIE xy Chromaticity Diagram** panel for BlinderKitten that displays the CIE 1931 color space tongue, overlays the **fixture gamut triangle** from its calibrated emitters, draws **white diagonal hatching** outside the gamut, shows a draggable **color cursor**, and lets the user pick a target chromaticity by clicking/dragging inside the gamut. The panel reacts to the currently selected fixture(s) in the active Command.

---

## Visual Reference

The panel should look like the classic CIE 1931 xy diagram:
- A black background filling the component.
- The **spectral locus** (horseshoe tongue shape) filled with the approximate visible-spectrum colors.
- The fixture's **gamut polygon** (triangle for 3 emitters, polygon for more) drawn as a white outline connecting the emitter xy coordinates.
- The area **outside the gamut but inside the tongue** is overlaid with semi-transparent **white diagonal lines** (hatching at ~45°, ~4 px spacing, ~1 px stroke) so the user sees it's out-of-reach.
- A **cursor dot** (small circle, white outline, ~8 px diameter) at the current target chromaticity.
- Optionally: small labels near each emitter vertex showing its channel name.

---

## Architecture

### New files to create

| File | Class | Role |
|------|-------|------|
| `Source/UI/CIEColorPicker.h` | `CIEColorPicker`, `CIEColorPickerUI` | The JUCE Component + ShapeShifterContent wrapper |
| `Source/UI/CIEColorPicker.cpp` | (implementations) | Paint, mouse interaction, gamut computation |

### Files to modify

| File | Change |
|------|--------|
| `Source/MainComponent.cpp` | Add `#include "UI/CIEColorPicker.h"` and register the ShapeShifter panel as `"CIE Color Picker"` |

### Existing code to reuse

| What | Where |
|------|-------|
| `ColorEngine::getCalibrations(SubFixture*)` | [Source/Common/ColorEngine/ColorEngine.cpp](Source/Common/ColorEngine/ColorEngine.cpp) — returns `Array<EmitterCalibration>` with `.cieX`, `.cieY` per emitter |
| `ColorEngine::sRGBtoXYZ` / `XYZtoXY` | CIE conversions already implemented |
| `ColorEngine::computeEmitterDMXLevels` | Converts target sRGB → per-emitter DMX levels |
| `UserInputManager::getInstance()->targetCommand` | Active command with `.selection.computedSelectedSubFixtures` |
| `UserInputManager::getInstance()->changeChannelValue(channelType, value)` | Sets encoder value for a channel |
| `BKEngine` members: `CPRedChannel`, `CPGreenChannel`, `CPBlueChannel`, `IntensityChannel` | TargetParameters for the standard color channels |
| `BKColorPicker` pattern | [Source/UI/BKColorPicker.h](Source/UI/BKColorPicker.h) / [.cpp](Source/UI/BKColorPicker.cpp) — reference for singleton panel, ShapeShifterContent, mouse interaction, `engine` pointer |

---

## Detailed TODO

### STEP 1 — Create `CIEColorPicker.h`

```
#pragma once
#include <JuceHeader.h>

class BKEngine;
class SubFixture;
struct EmitterCalibration;

class CIEColorPickerUI : public ShapeShifterContent {
public:
    CIEColorPickerUI(const String& contentName);
    ~CIEColorPickerUI();
    static CIEColorPickerUI* create(const String& name) { return new CIEColorPickerUI(name); }
};

class CIEColorPicker : public juce::Component, public Timer {
public:
    juce_DeclareSingleton(CIEColorPicker, true);
    CIEColorPicker();
    ~CIEColorPicker() override;

    BKEngine* engine = nullptr;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void timerCallback() override;

private:
    // Cached tongue image (rendered once per resize)
    Image tongueImage;
    int cachedWidth = 0, cachedHeight = 0;

    // Current gamut emitter positions (CIE xy)
    Array<EmitterCalibration> currentEmitters;
    // Currently targeted CIE xy
    float cursorX = 0.3127f, cursorY = 0.3290f;  // D65 default

    // Coordinate mapping
    // The CIE xy diagram maps x=[0..0.8], y=[0..0.9] into the component rect with margin.
    static constexpr float cieXMin = 0.0f, cieXMax = 0.8f;
    static constexpr float cieYMin = 0.0f, cieYMax = 0.9f;
    static constexpr float margin = 12.0f;

    Point<float> cieToScreen(float cx, float cy) const;
    Point<float> screenToCIE(float sx, float sy) const;

    void renderTongueImage(int w, int h);
    void updateGamutFromSelection();
    Path buildGamutPath() const;
    Path buildTonguePath() const;
    void setTargetFromScreenPos(float sx, float sy);
    bool isInsideGamut(float cx, float cy) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CIEColorPicker)
};
```

### STEP 2 — Create `CIEColorPicker.cpp`

#### 2a — Singleton, constructor, destructor

- `juce_ImplementSingleton(CIEColorPicker)`
- Constructor: start a `Timer` at ~15 Hz (66 ms) to poll selection changes and trigger repaint. Set `engine = nullptr` (set on first paint or via lazy init as BKColorPicker does).
- `CIEColorPickerUI` constructor: pass `CIEColorPicker::getInstance()` as content, same pattern as `BKColorPickerUI`.

#### 2b — CIE 1931 Spectral Locus data

Embed the standard **CIE 1931 2° observer spectral locus** as a static array of ~80 `(x, y)` pairs from 380 nm to 700 nm (5 nm steps). This is public-domain colorimetry data. Example (abbreviated):

```cpp
static const float spectralLocus[][2] = {
    {0.1741f, 0.0050f},  // 380 nm
    {0.1740f, 0.0050f},  // 385 nm
    {0.1738f, 0.0049f},  // 390 nm
    // ... (every 5 nm)
    {0.7347f, 0.2653f},  // 700 nm
};
static const int spectralLocusSize = sizeof(spectralLocus) / sizeof(spectralLocus[0]);
```

The tongue outline is: spectral locus points from 380→700 nm, then a straight line back from 700 nm to 380 nm (the "purple line").

#### 2c — `buildTonguePath()`

Build a JUCE `Path` from the spectral locus data, converting each `(x, y)` via `cieToScreen()`. Close the path with the purple line.

#### 2d — `renderTongueImage(int w, int h)`

Render an off-screen `Image` (ARGB, w×h) that fills each pixel inside the tongue with its approximate sRGB color:

```
For each pixel (px, py):
    (cx, cy) = screenToCIE(px, py)
    if point is inside tonguePath:
        // Convert CIE xy → XYZ (use Y=1 for display brightness)
        ColorEngine::xyYtoXYZ(cx, cy, 0.5f, X, Y, Z)
        ColorEngine::XYZtoSRGB(X, Y, Z, r, g, b)
        set pixel to Colour(r*255, g*255, b*255)
    else:
        set pixel to transparent black
```

Cache this image; only regenerate when component size changes.

#### 2e — `cieToScreen` / `screenToCIE`

```cpp
Point<float> CIEColorPicker::cieToScreen(float cx, float cy) const {
    float drawW = getWidth() - 2 * margin;
    float drawH = getHeight() - 2 * margin;
    // CIE y axis goes upward, screen y goes downward
    float sx = margin + ((cx - cieXMin) / (cieXMax - cieXMin)) * drawW;
    float sy = margin + (1.0f - (cy - cieYMin) / (cieYMax - cieYMin)) * drawH;
    return { sx, sy };
}
```

Inverse for `screenToCIE`.

#### 2f — `updateGamutFromSelection()`

Called from `timerCallback()`:

```cpp
void CIEColorPicker::updateGamutFromSelection() {
    Command* cmd = UserInputManager::getInstance()->targetCommand;
    if (cmd == nullptr) { currentEmitters.clear(); return; }

    auto& subs = cmd->selection.computedSelectedSubFixtures;
    if (subs.size() == 0) { currentEmitters.clear(); return; }

    // Use the first selected SubFixture's calibration data
    SubFixture* sf = subs[0];
    currentEmitters = ColorEngine::getCalibrations(sf);
}
```

#### 2g — `buildGamutPath()`

Build a `Path` connecting each emitter's `(cieX, cieY)` as a polygon (convex hull if >3 emitters). For 3 emitters this is a triangle. Use `cieToScreen()` for each vertex.

For the convex hull with >3 emitters, implement a simple Graham scan or, simpler, sort emitter points by angle from their centroid and connect them in order.

#### 2h — `paint(Graphics& g)`

```
1. Fill background black.
2. Draw the cached tongue image.
3. Draw the tongue outline (white, 1px stroke) using buildTonguePath().
4. If currentEmitters.size() >= 3:
   a. Build gamutPath = buildGamutPath()
   b. Build tonguePath = buildTonguePath()
   c. Create a clipping path = tonguePath MINUS gamutPath (the out-of-gamut area inside the tongue):
      - Create a Path that is the tongue path.
      - Subtract the gamut path using Path::addPath with a reverse winding, or:
      - Simpler approach: clip to tonguePath, then fill the whole region with diagonal hatching,
        then clip to gamutPath and clear/redraw the tongue image over it.
      - **Recommended approach**:
        i.   Save graphics state
        ii.  Clip to tonguePath (reduceClipRegion)
        iii. Exclude gamutPath: create a hatching pattern and draw it clipped to (tongue minus gamut)
             - Use `g.saveState()` / `g.reduceClipRegion(tonguePath)` / then draw hatching lines everywhere
             - Then `g.restoreState()`, `g.saveState()`, `g.reduceClipRegion(gamutPath)`, and redraw the tongue image to "erase" the hatching inside the gamut.
             - Then `g.restoreState()`.
   d. Draw the gamut polygon outline (white, 2px stroke).
   e. Optionally draw small labels at each emitter vertex.

5. Draw the cursor dot at cieToScreen(cursorX, cursorY).
```

**Diagonal hatching** — draw parallel lines at 45° across the entire component bounds:

```cpp
void drawHatching(Graphics& g, Rectangle<int> bounds) {
    g.setColour(Colours::white.withAlpha(0.35f));
    float spacing = 4.0f;
    float maxDim = (float)(bounds.getWidth() + bounds.getHeight());
    for (float d = -maxDim; d < maxDim; d += spacing) {
        g.drawLine(bounds.getX() + d, bounds.getY(),
                   bounds.getX() + d + bounds.getHeight(), bounds.getBottom(),
                   1.0f);
    }
}
```

#### 2i — Mouse Interaction

`mouseDown` / `mouseDrag`:

```cpp
void CIEColorPicker::setTargetFromScreenPos(float sx, float sy) {
    auto cie = screenToCIE(sx, sy);
    float cx = cie.x, cy = cie.y;

    // Clamp to gamut if we have one
    // (For simplicity, always allow picking anywhere in the tongue;
    //  but optionally constrain to gamut polygon.)

    cursorX = cx;
    cursorY = cy;

    // Convert CIE xy to sRGB for the encoder channels
    float X, Y, Z;
    ColorEngine::xyYtoXYZ(cx, cy, 1.0f, X, Y, Z);
    float r, g, b;
    ColorEngine::XYZtoSRGB(X, Y, Z, r, g, b);

    // Set the RGB encoder values through UserInputManager
    if (engine == nullptr) engine = dynamic_cast<BKEngine*>(Engine::mainEngine);
    if (engine == nullptr) return;

    if (engine->CPRedChannel->stringValue() != "") {
        auto* ch = dynamic_cast<ChannelType*>(engine->CPRedChannel->targetContainer.get());
        UserInputManager::getInstance()->changeChannelValue(ch, r);
    }
    if (engine->CPGreenChannel->stringValue() != "") {
        auto* ch = dynamic_cast<ChannelType*>(engine->CPGreenChannel->targetContainer.get());
        UserInputManager::getInstance()->changeChannelValue(ch, g);
    }
    if (engine->CPBlueChannel->stringValue() != "") {
        auto* ch = dynamic_cast<ChannelType*>(engine->CPBlueChannel->targetContainer.get());
        UserInputManager::getInstance()->changeChannelValue(ch, b);
    }

    repaint();
}
```

#### 2j — `timerCallback()`

```cpp
void CIEColorPicker::timerCallback() {
    updateGamutFromSelection();

    // Also update cursor position from the current command's RGB values
    Command* cmd = UserInputManager::getInstance()->targetCommand;
    if (cmd != nullptr && engine != nullptr) {
        float r = -1, g = -1, b = -1;
        for (auto* cv : cmd->values.items) {
            if (cv->channelType->getValue() == engine->CPRedChannel->getValue()) r = cv->valueFrom->getValue();
            if (cv->channelType->getValue() == engine->CPGreenChannel->getValue()) g = cv->valueFrom->getValue();
            if (cv->channelType->getValue() == engine->CPBlueChannel->getValue()) b = cv->valueFrom->getValue();
        }
        if (r >= 0 && g >= 0 && b >= 0) {
            float X, Y, Z;
            ColorEngine::sRGBtoXYZ(r, g, b, X, Y, Z);
            float cx, cy;
            if (ColorEngine::XYZtoXY(X, Y, Z, cx, cy)) {
                cursorX = cx;
                cursorY = cy;
            }
        }
    }

    repaint();
}
```

### STEP 3 — Register in `MainComponent.cpp`

Add include and registration:

```cpp
// In includes section:
#include "UI/CIEColorPicker.h"

// In the ShapeShifterFactory registration block, after the "Color Picker" line:
ShapeShifterFactory::getInstance()->defs.add(
    new ShapeShifterDefinition("CIE Color Picker", &CIEColorPickerUI::create));
```

### STEP 4 — Add to Visual Studio project

Add `Source/UI/CIEColorPicker.h` and `Source/UI/CIEColorPicker.cpp` to the `BlinderKitten.jucer` file or directly to the Visual Studio 2022 `.vcxproj` / `.vcxproj.filters` so they compile.

### STEP 5 — Verify & Polish

- Build and open the panel from the View menu.
- With no fixture selected: show the tongue with no gamut overlay, no hatching.
- Select a fixture with 3+ calibrated emitters: the gamut triangle appears, hatching fills the out-of-gamut tongue area.
- Click/drag inside the diagram: cursor moves, encoder RGB values update.
- Change encoders externally: cursor follows.
- With >3 calibrated emitters (e.g. RGBW): gamut should be a convex polygon (quadrilateral).

---

## Key Implementation Notes

1. **Do NOT modify `ColorEngine`** — all needed functions already exist.
2. **Singleton pattern**: Follow `BKColorPicker` exactly — `juce_DeclareSingleton` / `juce_ImplementSingleton`, lazy `engine` pointer.
3. **Spectral locus data**: Use the CIE 1931 2° standard observer data. This is public-domain scientific data, not copyrighted.
4. **Performance**: Render the tongue image only on resize. The gamut path and hatching are cheap to draw every frame. The timer-based repaint at 15 Hz is sufficient.
5. **Hatching technique**: The diagonal lines should be white with ~35% opacity, ~4px apart, at 45°. Draw them clipped to (tongue minus gamut). The recommended approach is:
   - `g.saveState()` → clip to tongue → draw hatching → `g.restoreState()`
   - `g.saveState()` → clip to gamut → redraw tongue image (erasing hatching inside gamut) → `g.restoreState()`
6. **Convex hull for >3 emitters**: Sort emitter points by angle around their centroid. This gives a correct polygon for typical LED fixture configurations (RGBW, RGBA, RGBWAU, etc.).
7. **Include guards**: Use `#pragma once`.
8. **Includes needed in .cpp**: `JuceHeader.h`, `CIEColorPicker.h`, `UserInputManager.h`, `BKEngine.h`, `Definitions/Command/Command.h`, `Common/ColorEngine/ColorEngine.h`, `Definitions/SubFixture/SubFixture.h`, `Definitions/ChannelFamily/ChannelType/ChannelType.h`.
