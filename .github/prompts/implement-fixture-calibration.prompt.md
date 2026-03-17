---
description: "Implement CIE-based fixture emitter calibration: per-emitter color point (CIE xyY), luminous intensity, and dimming curves on FixtureTypeChannel, plus a ColorEngine that converts target colors into calibrated per-emitter DMX values."
mode: "agent"
tools: ["read", "edit", "search", "execute", "agent"]
---

# Fixture Emitter Calibration System

## Goal

Allow users to calibrate light fixture emitters by entering measured colorimetric data (CIE xy chromaticity, luminous intensity in candela, and optional non-linear dimming curves). Then use that data to accurately mix colors: given a target color, compute per-emitter DMX values that produce it physically.

## Background — Current Architecture

Read these files **before any implementation** to understand the existing code:

### Core data path (read ALL of these first)

| File | What to learn |
|------|---------------|
| `Source/Definitions/FixtureType/FixtureTypeChannel.h` | Current fields: `channelType`, `subFixtureId`, `resolution`, `dmxDelta`, `physicalRange` (Point2DParameter), `curve` (Automation). This is where calibration data will be added. |
| `Source/Definitions/FixtureType/FixtureTypeChannel.cpp` | Constructor pattern — how parameters are created (`addFloatParameter`, `addPoint2DParameter`, `addBoolParameter`, etc.), how `curve` Automation is initialized, how `saveAndLoadRecursiveData` works. |
| `Source/Definitions/SubFixture/SubFixtureChannel.h` | Runtime channel: `currentValue`, `parentFixtureTypeChannel`, `parentFixture`, `channelType`, `writeValue()`, `updateVal()`. |
| `Source/Definitions/SubFixture/SubFixtureChannel.cpp` lines 131-210 | `writeValue()` — the final output pipeline: clamp → virtual master → invert → **FixtureTypeChannel curve** → per-patch corrections → DMX send. The calibration dimming curve lookup must happen here. |
| `Source/Definitions/SubFixture/SubFixture.h` | `channelsMap` (HashMap<ChannelType*, SubFixtureChannel*>), `getOutputColor()`. |
| `Source/Definitions/SubFixture/SubFixture.cpp` lines 62-160 | `getOutputColor()` — current naive color mixing (additive RGB + hardcoded amber/UV coefficients). This needs a calibrated path. |
| `Source/Definitions/ChannelFamily/ChannelType/ChannelType.h` | ChannelType class — just has `priority` (HTP/LTP) and `reactGM`. |
| `Source/Definitions/FixtureType/FixtureType.h` | Contains `FixtureTypeChannelManager chansManager`. |

### Color pipeline (read to understand how colors reach channels)

| File | What to learn |
|------|---------------|
| `Source/UI/BKColorPicker.cpp` | `mouseSetColor()` converts picker XY → RGB → calls `UserInputManager::changeChannelValue()` for Red, Green, Blue, Cyan, Magenta, Yellow, Hue, Saturation channels individually. This is the entry point for color-to-channel conversion. |
| `Source/UserInputManager.cpp` line 722+ | `changeChannelValue()` — finds or creates `CommandValue` entries on the target Command for the given ChannelType. |
| `Source/Definitions/Command/CommandValue.h` | A `CommandValue` holds: `channelType` (TargetParameter), `valueFrom` (FloatParameter 0-1). |
| `Source/BKEngine.h` | Global color channel references: `CPRedChannel`, `CPGreenChannel`, `CPBlueChannel`, `CPWhiteChannel`, `CPAmberChannel`, `CPUVChannel`, `CPCyanChannel`, `CPMagentaChannel`, `CPYellowChannel`, `CPHueChannel`, `CPSaturationChannel`, `IntensityChannel`. |

### Curves reference

| File | What to learn |
|------|---------------|
| `Source/Definitions/CurvePreset/CurvePreset.h` | Existing curve preset pattern using OrganicUI `Automation`. |

The `Automation` class (from `juce_organicui`) works as:
- `curve.addKey(position, value, false)` — adds a keyframe
- `curve.getValueAtPosition(float pos)` — evaluates the curve at a 0..1 position
- `curve.enabled` — BoolParameter to enable/disable
- `curve.length->setValue(1)` — sets the 0..1 range
- `curve.setCanBeDisabled(true)` — allows user toggle
- `curve.items[n]->easingType->setValueWithData(Easing::LINEAR)` — sets interpolation
- `curve.saveAndLoadRecursiveData = true` — persists in .olga files

---

## Implementation Plan — DETAILED TODO

### PHASE 1: Add calibration fields to FixtureTypeChannel

**File: `Source/Definitions/FixtureType/FixtureTypeChannel.h`**

Add these new members to the class declaration, after the existing `Automation curve;` line:

```cpp
// === Calibration data ===
ControllableContainer calibrationContainer;
Point2DParameter* calibrationCIExy;      // CIE xy chromaticity (x, y) of this emitter at 100%
FloatParameter* calibrationMaxIntensity;  // Luminous intensity in candela (cd) at DMX 255
Automation calibrationDimmingCurve;       // Dimming curve: X = relative DMX (0..1), Y = relative luminous intensity (0..1)
```

**File: `Source/Definitions/FixtureType/FixtureTypeChannel.cpp`**

In the constructor, after the existing `addChildControllableContainer(&curve);` block, initialize calibration:

```cpp
// Calibration container
calibrationContainer.setNiceName("Calibration");
calibrationContainer.editorIsCollapsed = true;
calibrationContainer.editorCanBeCollapsed = true;
calibrationContainer.saveAndLoadRecursiveData = true;

calibrationCIExy = calibrationContainer.addPoint2DParameter("CIE xy", "CIE 1931 chromaticity coordinates (x, y) of this emitter at full intensity");
calibrationCIExy->setPoint(0.3127f, 0.3290f); // D65 white point as default
calibrationCIExy->canBeDisabledByUser = true;
calibrationCIExy->setEnabled(false);

calibrationMaxIntensity = calibrationContainer.addFloatParameter("Max Intensity (cd)", "Luminous intensity in candela at full DMX output", 0, 0);
calibrationMaxIntensity->canBeDisabledByUser = true;
calibrationMaxIntensity->setEnabled(false);

// Dimming curve: input = DMX level (0-1), output = relative intensity (0-1)
calibrationDimmingCurve.setNiceName("Dimming Curve");
calibrationDimmingCurve.editorIsCollapsed = true;
calibrationDimmingCurve.allowKeysOutside = false;
calibrationDimmingCurve.isSelectable = false;
calibrationDimmingCurve.length->setValue(1);
calibrationDimmingCurve.addKey(0, 0, false);
calibrationDimmingCurve.items[0]->easingType->setValueWithData(Easing::LINEAR);
calibrationDimmingCurve.addKey(1, 1, false);
calibrationDimmingCurve.selectItemWhenCreated = false;
calibrationDimmingCurve.editorCanBeCollapsed = true;
calibrationDimmingCurve.setCanBeDisabled(true);
calibrationDimmingCurve.enabled->setValue(false);
calibrationDimmingCurve.saveAndLoadRecursiveData = true;

calibrationContainer.addChildControllableContainer(&calibrationDimmingCurve);

addChildControllableContainer(&calibrationContainer);
```

**Key details:**
- `calibrationCIExy` uses `Point2DParameter` (same as existing `physicalRange`)
- Default CIE xy is D65 white (0.3127, 0.3290) — a safe default
- Fields are disabled by default (`canBeDisabledByUser = true`, `setEnabled(false)`) — user enables when entering calibration data
- `calibrationDimmingCurve` defaults to linear (0,0)→(1,1) — user adds intermediate points for non-linear dimmers
- Everything inside a collapsible `calibrationContainer` so it doesn't clutter the UI unless the user opens it
- `saveAndLoadRecursiveData = true` ensures persistence in `.olga` show files

**Verification:** After this phase, open a FixtureType in the inspector → expand a channel → you should see a "Calibration" section with CIE xy, Max Intensity, and Dimming Curve fields. All disabled by default.

---

### PHASE 2: Create ColorEngine utility class

Create two new files: `Source/Common/ColorEngine/ColorEngine.h` and `Source/Common/ColorEngine/ColorEngine.cpp`.

**File: `Source/Common/ColorEngine/ColorEngine.h`**

```cpp
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
    // Uses least-squares with non-negativity constraint for >3 emitters.
    // Returns false if the target is outside the gamut.
    // Output: fills the `weight` member of each EmitterCalibration in the array.
    static bool solveEmitterWeights(Array<EmitterCalibration>& emitters,
                                     float targetX, float targetY, float targetZ,
                                     Array<float>& outWeights);

    // Convert a weight (relative intensity 0..1) to a DMX-level float (0..1) using
    // the emitter's dimming curve (inverse lookup).
    // If no dimming curve, weight == DMX level (linear).
    static float weightToDMXLevel(const EmitterCalibration& emitter, float weight);

    // High-level: given target sRGB color and intensity, set all calibrated emitter
    // channels on a SubFixture. Returns false if not enough calibration data.
    static bool applyTargetColorToSubFixture(SubFixture* sf,
                                              float r, float g, float b,
                                              float intensity);

    // High-level: read current emitter values on a SubFixture and compute the
    // resulting CIE XYZ (for accurate color preview). Returns black if uncalibrated.
    static Colour getCalibrationOutputColor(SubFixture* sf);
};
```

**File: `Source/Common/ColorEngine/ColorEngine.cpp`**

Implement the following functions:

#### `sRGBtoXYZ` / `XYZtoSRGB`
Standard sRGB ↔ CIE XYZ D65 conversion using the IEC 61966-2-1 matrix:

```
sRGB → linear: if C <= 0.04045: C/12.92, else ((C + 0.055)/1.055)^2.4
linear → XYZ:
  X = 0.4124564*R + 0.3575761*G + 0.1804375*B
  Y = 0.2126729*R + 0.7151522*G + 0.0721750*B
  Z = 0.0193339*R + 0.1191920*G + 0.9503041*B
```

Inverse for XYZ → sRGB (use the standard inverse matrix + gamma).

#### `xyYtoXYZ`
```
if y == 0: X = Y = Z = 0
else: X = (x * Yval) / y;  Y = Yval;  Z = ((1 - x - y) * Yval) / y;
```

#### `XYZtoXY`
```
sum = X + Y + Z;
if sum == 0: return false;
x = X / sum;  y = Y / sum;
return true;
```

#### `getCalibrations`
```
For each entry in sf->channelsMap:
  Get SubFixtureChannel* sfc
  Get FixtureTypeChannel* ftc = sfc->parentFixtureTypeChannel
  If ftc is null or ftc->calibrationCIExy is not enabled: skip
  If ftc->calibrationMaxIntensity is not enabled or value <= 0: skip
  Build EmitterCalibration:
    cieX = ftc->calibrationCIExy->x
    cieY = ftc->calibrationCIExy->y
    maxIntensityCd = ftc->calibrationMaxIntensity->floatValue()
    Compute fullX, fullY, fullZ via xyYtoXYZ(cieX, cieY, maxIntensityCd)
  Add to result array
```

#### `hasCalibrationData`
Return `getCalibrations(sf).size() >= 3`.

#### `solveEmitterWeights` — THE CORE ALGORITHM

For N calibrated emitters with full-intensity tristimulus values $[X_i, Y_i, Z_i]$, and a target $[X_t, Y_t, Z_t]$:

**Case N == 3 (exact solution):**
Build 3×3 matrix M where column i = [X_i, Y_i, Z_i]. Solve M * w = target.
- Compute determinant. If ~0, the emitters are coplanar → return false.
- Use Cramer's rule or direct 3×3 inverse (no external library needed).
- If any weight < 0 or > 1, the target is outside the gamut → clamp weights to [0,1] and return false.

**Case N > 3 (least-squares with non-negative constraint):**
Use iterative NNLS (Non-Negative Least Squares) — Lawson & Hanson algorithm, simplified:
1. Start with all weights = 0, all emitters in passive set
2. Compute gradient = M^T * (target - M*w)
3. Move emitter with largest positive gradient to active set
4. Solve least-squares on active set only (small linear system, ≤ 7 variables typically)
5. If any active weight becomes negative, adjust and move back to passive set
6. Repeat until convergence

Alternatively, for typical fixture emitter counts (3-7), a simpler approach:
1. If exactly 3: direct solve
2. If > 3: try all C(N,3) combinations of 3 emitters, pick the one with smallest total power (sum of weights) where all weights are in [0,1]
3. This is at most C(7,3)=35 combinations — trivial computation

**Case N < 3:** Return false (underdetermined — cannot control chromaticity).

**Scale weights:** After solving, all weights are relative to full intensity. The weights directly become the target emitter intensity ratios.

#### `weightToDMXLevel` — Inverse dimming curve lookup

```
If emitter.typeChannel->calibrationDimmingCurve.enabled->boolValue():
  // The curve maps DMX(0..1) → intensity(0..1). We need the inverse.
  // Binary search: find dmxLevel such that curve.getValueAtPosition(dmxLevel) ≈ weight
  float lo = 0, hi = 1;
  for (int i = 0; i < 20; i++) {  // 20 iterations → precision < 1e-6
    float mid = (lo + hi) / 2;
    float intensity = emitter.typeChannel->calibrationDimmingCurve.getValueAtPosition(mid);
    if (intensity < weight) lo = mid; else hi = mid;
  }
  return (lo + hi) / 2;
else:
  return weight;  // linear dimming assumed
```

#### `applyTargetColorToSubFixture`
```
Array<EmitterCalibration> emitters = getCalibrations(sf);
if (emitters.size() < 3) return false;

// Convert target sRGB + intensity to CIE XYZ
float X, Y, Z;
sRGBtoXYZ(r, g, b, X, Y, Z);
// Scale by intensity
X *= intensity;  Y *= intensity;  Z *= intensity;
// Scale target Y to match emitter luminance scale
// The maximum Y the fixture can produce when all emitters are at 100%:
float maxY = 0;
for (auto& e : emitters) maxY += e.fullY;
X *= maxY;  Y *= maxY;  Z *= maxY;

Array<float> weights;
solveEmitterWeights(emitters, X, Y, Z, weights);

for (int i = 0; i < emitters.size(); i++) {
  float dmxLevel = weightToDMXLevel(emitters[i], weights[i]);
  // The dmxLevel is what the emitter channel should output (0..1)
  // Store it as the channel value — this would be set via the command system
}
return true;
```

#### `getCalibrationOutputColor`
Reverse of the above — read current channel values, apply dimming curve forward (value → intensity), sum the CIE XYZ contributions, convert to sRGB:

```
Array<EmitterCalibration> emitters = getCalibrations(sf);
if (emitters.size() < 3) return Colour(0, 0, 0); // uncalibrated — caller uses legacy path

float totalX = 0, totalY = 0, totalZ = 0;
for (auto& e : emitters) {
  float dmxLevel = e.channel->currentValue;
  float relativeIntensity;
  if (e.typeChannel->calibrationDimmingCurve.enabled->boolValue())
    relativeIntensity = e.typeChannel->calibrationDimmingCurve.getValueAtPosition(dmxLevel);
  else
    relativeIntensity = dmxLevel;

  totalX += e.fullX * relativeIntensity;
  totalY += e.fullY * relativeIntensity;
  totalZ += e.fullZ * relativeIntensity;
}

// Normalize: scale so that max luminance → Y=1 for display
float maxY = 0;
for (auto& e : emitters) maxY += e.fullY;
if (maxY > 0) { totalX /= maxY; totalY /= maxY; totalZ /= maxY; }

float r, g, b;
XYZtoSRGB(totalX, totalY, totalZ, r, g, b);
return Colour(jlimit(0, 255, (int)(r * 255)),
              jlimit(0, 255, (int)(g * 255)),
              jlimit(0, 255, (int)(b * 255)));
```

---

### PHASE 3: Include ColorEngine in build

**File: `Source/Common/CommonIncludes.h`**

Add after existing includes:
```cpp
#include "ColorEngine/ColorEngine.h"
```

Make sure the include can be resolved by checking that `Source/` and `Source/Common/` are in the header search paths (they already are in the .jucer).

---

### PHASE 4: Integrate into SubFixture::getOutputColor()

**File: `Source/Definitions/SubFixture/SubFixture.cpp`**

Modify `getOutputColor()` to use calibrated color when available:

```cpp
Colour SubFixture::getOutputColor()
{
    // If this fixture has calibration data, use the accurate CIE-based path
    if (ColorEngine::hasCalibrationData(this)) {
        return ColorEngine::getCalibrationOutputColor(this);
    }

    // ... existing naive color mixing code unchanged ...
}
```

Add the include at the top of the file:
```cpp
#include "Common/ColorEngine/ColorEngine.h"
```

---

### PHASE 5: Integrate into color picker → channel conversion

This is the hardest integration. Currently, `BKColorPicker::mouseSetColor()` directly maps RGB → individual channel values (Red channel = r, Green channel = g, etc.). For calibrated fixtures, we need to:

1. Determine which SubFixtures are selected in the current command
2. For each SubFixture with calibration data, run `ColorEngine::applyTargetColorToSubFixture()` instead of the naive per-channel mapping

**File: `Source/UI/BKColorPicker.cpp`**

Modify `mouseSetColor()`:

After computing `r, g, b` from the picker position, before writing to individual channels:

```cpp
// Check if any selected fixtures have calibration data
// If so, use ColorEngine to compute per-emitter values
Command* cmd = UserInputManager::getInstance()->targetCommand;
if (cmd != nullptr) {
    // Get selected subfixtures from the command
    // For each calibrated subfixture, use ColorEngine
    // For uncalibrated: fall through to existing RGB/CMY/HSV path
}
```

**IMPORTANT:** This step requires understanding how `Command` resolves to `SubFixture` selections. Read:
- `Source/Definitions/Command/Command.h` — look for how selections + values work
- `Source/Definitions/Command/CommandSelection.h` — how fixture selections are stored

The integration approach:
1. From the active Command, get the list of affected SubFixtures
2. For each SubFixture, check `ColorEngine::hasCalibrationData()`
3. If calibrated: call `ColorEngine::applyTargetColorToSubFixture()` and set channel values on the Command for all calibrated emitter ChannelTypes
4. If not calibrated: use existing direct RGB/CMY/HSV mapping (unchanged)

**Note:** This is a targeted change to `mouseSetColor()` only. The rest of the color picker (display, crosshair) can remain unchanged for now.

---

### PHASE 6: Integrate into Command/Programmer application

When a Command contains calibrated color data and is applied to SubFixtureChannels via the Programmer or Cuelist system, the per-emitter values need to be computed.

**Read first:**
- `Source/Definitions/Command/Command.cpp` — specifically `computeValues()` method
- How `Command` stores per-SubFixtureChannel computed values

The integration point: when `Command::computeValues()` processes color-related CommandValues for a calibrated fixture, run them through `ColorEngine::solveEmitterWeights()` to distribute across the actual emitter channels.

---

## Summary of ALL files to create/modify

| Action | File | What |
|--------|------|------|
| **MODIFY** | `Source/Definitions/FixtureType/FixtureTypeChannel.h` | Add `calibrationContainer`, `calibrationCIExy`, `calibrationMaxIntensity`, `calibrationDimmingCurve` |
| **MODIFY** | `Source/Definitions/FixtureType/FixtureTypeChannel.cpp` | Initialize calibration fields in constructor |
| **CREATE** | `Source/Common/ColorEngine/ColorEngine.h` | ColorEngine class declaration |
| **CREATE** | `Source/Common/ColorEngine/ColorEngine.cpp` | CIE math, emitter solving, dimming curve inverse |
| **MODIFY** | `Source/Common/CommonIncludes.h` | Add `#include "ColorEngine/ColorEngine.h"` |
| **MODIFY** | `Source/Definitions/SubFixture/SubFixture.cpp` | Add calibrated path in `getOutputColor()` |
| **MODIFY** | `Source/UI/BKColorPicker.cpp` | Route through ColorEngine for calibrated fixtures |
| **MODIFY** | `Source/Definitions/Command/Command.cpp` | Apply calibrated color solving in `computeValues()` |

## Constraints

- **C++11/14 only** — no `std::optional`, `std::variant`, `constexpr if`, etc.
- **No external libraries** for linear algebra — the matrices are at most 7×3; implement the math directly.
- **No blocking the message thread** — all ColorEngine functions are pure computation, called from Brain thread or from UI event handlers (which are fast for small matrices).
- **Backward compatible** — fixtures without calibration data must work exactly as before. All calibration fields default to disabled.
- **Serialization** — calibration data must persist in `.olga` files. This happens automatically if `saveAndLoadRecursiveData = true` on the containers.
- Follow existing code style: PascalCase classes, camelCase members, `#pragma once`, JUCE header boilerplate.
- **Do not modify** JUCE or OrganicUI module code in `Modules/`.

## Testing Strategy

1. **Build** the project — no compile errors.
2. **Open** a FixtureType → channel → verify "Calibration" section appears and is collapsible/expandable.
3. **Enter** CIE xy + intensity for an RGBW fixture's 4 emitters using known LED values (e.g., Red: x=0.6857, y=0.3143; Green: x=0.2002, y=0.7289; Blue: x=0.1500, y=0.0600; White: x=0.3127, y=0.3290).
4. **Pick a color** in the color picker → verify calibrated emitter channels receive computed values.
5. **Verify** `getOutputColor()` returns accurate preview colors for calibrated fixtures.
6. **Verify** fixtures WITHOUT calibration data behave identically to before.
7. **Save and reload** a show file → verify calibration data persists.
