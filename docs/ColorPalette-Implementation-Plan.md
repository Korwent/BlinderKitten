# Color Palette Feature — Implementation Plan

## Overview

A **Color Palette** is a fixture-independent color swatch. Unlike a Preset (which stores per-SubFixture channel values bound to specific fixture IDs), a Color Palette stores only **color channel values** (Red, Green, Blue, Cyan, Magenta, Yellow, White, Amber, UV, Hue, Saturation — whatever the BKEngine color picker channels are configured to). When recalled, these values are pushed to the **currently selected fixtures' encoders** via `changeChannelValue()`, regardless of which fixtures were used when recording.

### User Workflow

- **Record**: With fixtures selected and color dialed in on encoders, press Record → select Color Palette grid cell → only color channel values are extracted and stored.
- **Recall**: With any fixture(s) selected, click a Color Palette grid cell → the stored color values are pushed to the encoders for all currently selected fixtures.

---

## Architecture Summary

Follow the **Preset** pattern exactly for class hierarchy and registration. The key difference is the data model: no per-fixture storage — just a flat map of `ChannelType* → float`.

---

## New Files to Create

### 1. `Source/Definitions/ColorPalette/ColorPalette.h`

```cpp
#pragma once
#include "JuceHeader.h"

class ChannelType;

class ColorPalette :
    public BaseItem
{
public:
    ColorPalette(var params = var());
    virtual ~ColorPalette();

    String objectType;
    var objectData;

    IntParameter* id;
    int registeredId = 0;
    StringParameter* userName;

    // Stored color channel values: TargetParameter → ChannelType, FloatParameter → value
    // Use a child BaseManager to store (channelType, value) pairs, similar to PresetValue
    BaseManager<ColorPaletteValue> values;

    void onContainerParameterChangedInternal(Parameter* p);
    void updateName();

    String getTypeString() const override { return objectType; }
    static ColorPalette* create(var params) { return new ColorPalette(params); }
};
```

**Implementation notes (`ColorPalette.cpp`):**
- Constructor: `BaseItem(params.getProperty("name", "Color Palette"))`, set `itemDataType = "ColorPalette"`, add `id` (IntParameter, default 1, min 1), add `userName` (StringParameter, default "New color palette"), call `updateName()`, add `values` child container, call `Brain::getInstance()->registerColorPalette(this, id->getValue())`.
- Destructor: `Brain::getInstance()->unregisterColorPalette(this)`.
- `onContainerParameterChangedInternal`: if `p == id` → re-register in Brain; if `p == id || p == userName` → `updateName()`.
- `updateName()`: `setNiceName(String((int)id->getValue()) + " - " + userName->getValue())`, call `dynamic_cast<ColorPaletteManager*>(parentContainer.get())->reorderItems()`.
- Include `Brain.h`, `ColorPaletteManager.h`, `UI/GridView/ColorPaletteGridView.h`.

### 2. `Source/Definitions/ColorPalette/ColorPaletteValue.h`

```cpp
#pragma once
#include "JuceHeader.h"

class ColorPaletteValue :
    public BaseItem
{
public:
    ColorPaletteValue(var params = var());
    ~ColorPaletteValue();

    TargetParameter* channelType;  // points to a ChannelType
    FloatParameter* paramValue;    // 0.0 to 1.0

    String getTypeString() const override { return "ColorPaletteValue"; }
    static ColorPaletteValue* create(var params) { return new ColorPaletteValue(params); }
};
```

**Implementation notes (`ColorPaletteValue.cpp`):**
- This is a near-copy of `PresetValue` but without fixture binding.
- Constructor: add `channelType` as TargetParameter pointing at ChannelType containers (use `ChannelFamilyManager::getInstance()` as root, filter for ChannelType), add `paramValue` as FloatParameter (0.0 to 1.0, default 0).
- Look at how `PresetValue` sets up its `param` TargetParameter for reference — typically: `channelType = addTargetParameter("Channel Type", "Channel type", ChannelFamilyManager::getInstance()); channelType->targetType = TargetParameter::CONTAINER; channelType->maxDefaultSearchLevel = 2;`.

### 3. `Source/Definitions/ColorPalette/ColorPaletteManager.h`

```cpp
#pragma once
#include "ColorPalette.h"

class ColorPaletteManager :
    public BaseManager<ColorPalette>
{
public:
    juce_DeclareSingleton(ColorPaletteManager, true);

    ColorPaletteManager();
    ~ColorPaletteManager();

    void addItemInternal(ColorPalette* o, var data) override;
    void removeItemInternal(ColorPalette* o) override;
};
```

**Implementation notes (`ColorPaletteManager.cpp`):**
- `juce_ImplementSingleton(ColorPaletteManager)`
- Comparator sorts by `id->getValue()` (same pattern as PresetManager).
- Constructor: `BaseManager("Color Palette")`, `itemDataType = "ColorPalette"`, `selectItemWhenCreated = true`, set comparator.
- `addItemInternal`: `reorderItems()`.
- `removeItemInternal`: empty (same as PresetManager).

### 4. `Source/Definitions/ColorPalette/ColorPaletteManagerUI.h`

```cpp
#pragma once
#include "JuceHeader.h"
#include "ColorPalette.h"

class ColorPaletteManagerUI :
    public BaseManagerShapeShifterUI<BaseManager<ColorPalette>, ColorPalette, BaseItemUI<ColorPalette>>
{
public:
    ColorPaletteManagerUI(const String& contentName);
    ~ColorPaletteManagerUI();

    static ColorPaletteManagerUI* create(const String& name) { return new ColorPaletteManagerUI(name); }
};
```

**Implementation notes (`ColorPaletteManagerUI.cpp`):**
- Constructor passes `ColorPaletteManager::getInstance()` to base class.
- `addItemText = "Add new color palette"`, `noItemText = "Create your color palettes here."`.
- Call `addExistingItems()`.

### 5. `Source/UI/GridView/ColorPaletteGridView.h`

```cpp
#pragma once
#include <JuceHeader.h>
#include "GridView.h"
#include "Definitions/ColorPalette/ColorPaletteManager.h"

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
```

**Implementation notes (`ColorPaletteGridView.cpp`):**
- `targetType = "colorpalette"` in constructor.
- `numberOfCells = 200`.
- Register as async listener: `ColorPaletteManager::getInstance()->addAsyncManagerListener(this)`.
- Destructor: remove async listener with null check.
- `updateCells()`: For each cell index `i`, call `Brain::getInstance()->getColorPaletteById(i+1)`. If found:
  - Set button text to `userName->getValue().toString()`.
  - **Key UX**: Set button background color to the stored RGB color if available. Extract Red/Green/Blue values from the palette's `values` manager, match against `BKEngine::CPRedChannel`, `CPGreenChannel`, `CPBlueChannel` channel types, and call `setColour(TextButton::buttonColourId, Colour(r*255, g*255, b*255))`. If no RGB match, remove custom color.
- If not found: empty text, grey background `Colour(40, 40, 40)`.
- `newMessage`: call `updateCells()`.

---

## Files to Modify

### 6. `Source/Brain.h`

**Add declarations** (next to the Preset equivalents around line 138-139):

```cpp
class ColorPalette; // forward declare at top of file

// In the class body, near the Preset declarations:
HashMap<int, ColorPalette*> colorPalettes;
void registerColorPalette(ColorPalette* cp, int id);
void unregisterColorPalette(ColorPalette* cp);
ColorPalette* getColorPaletteById(int id);
```

### 7. `Source/Brain.cpp`

**Add methods** (copy the `registerPreset`/`unregisterPreset`/`getPresetById` pattern):

```cpp
#include "Definitions/ColorPalette/ColorPalette.h"
#include "UI/GridView/ColorPaletteGridView.h"

void Brain::registerColorPalette(ColorPalette* target, int askedId) {
    // Same swap/collision logic as registerPreset, but using colorPalettes HashMap
    int currentId = target->registeredId;
    if (colorPalettes.getReference(askedId) == target) { return; }
    if (colorPalettes.containsValue(target)) {
        colorPalettes.removeValue(target);
    }
    ColorPalette* toSwap = colorPalettes.contains(askedId) ? colorPalettes.getReference(askedId) : nullptr;
    bool idIsOk = false;
    int newId = askedId;
    if (target->isCurrentlyLoadingData && toSwap != nullptr) {
        toSwap->id->setValue(currentId);
        idIsOk = true;
    }
    int delta = askedId < currentId ? -1 : 1;
    while (!idIsOk && newId > 0) {
        idIsOk = colorPalettes.getReference(newId) == nullptr;
        if (!idIsOk) newId += delta;
    }
    if (!idIsOk) { newId = currentId; }
    colorPalettes.set(newId, target);
    target->id->setValue(newId);
    target->registeredId = newId;
}

void Brain::unregisterColorPalette(ColorPalette* cp) {
    if (colorPalettes.containsValue(cp)) {
        colorPalettes.removeValue(cp);
    }
    if (!Brain::getInstance()->isClearing && ColorPaletteGridView::getInstanceWithoutCreating() != nullptr)
        ColorPaletteGridView::getInstance()->updateCells();
}

ColorPalette* Brain::getColorPaletteById(int id) {
    if (colorPalettes.contains(id)) {
        return colorPalettes.getReference(id);
    }
    return nullptr;
}
```

Also add to `Brain::clearAll()` or equivalent clearing method:
```cpp
colorPalettes.clear();
```

### 8. `Source/BKEngine.cpp`

**Add includes** (near the Preset includes around line 32-38):
```cpp
#include "./Definitions/ColorPalette/ColorPaletteManager.h"
#include "UI/GridView/ColorPaletteGridView.h"
```

**Add container registration** (near `addChildControllableContainer(PresetManager::getInstance())` around line 286):
```cpp
addChildControllableContainer(ColorPaletteManager::getInstance());
```

**Add engine assignment** (near `PresetGridView::getInstance()->engine = this` around line 313):
```cpp
ColorPaletteGridView::getInstance()->engine = this;
```

**Add to `deleteInstance` sequence** (in the shutdown block around lines 393-427):
```cpp
ColorPaletteGridView::deleteInstance();      // with other GridView deletes
ColorPaletteManager::deleteInstance();        // with other Manager deletes
```

**Add to `clearInternal`** (near `PresetManager::getInstance()->clear()` around line 487):
```cpp
ColorPaletteManager::getInstance()->clear();
ColorPaletteGridView::getInstance()->updateCells();
```

**Add JSON serialization** (near Preset serialization around line 586):
```cpp
var cpltData = ColorPaletteManager::getInstance()->getJSONData(includeNonOverriden);
if (!cpltData.isVoid() && cpltData.getDynamicObject()->getProperties().size() > 0)
    data.getDynamicObject()->setProperty(ColorPaletteManager::getInstance()->shortName, cpltData);
```

**Add JSON loading** (near Preset loading):
```cpp
// Add progress task:
ProgressTask* cpltTask = loadingTask->addTask("Color Palettes");

// Add load call (near PresetManager load):
ColorPaletteManager::getInstance()->loadJSONData(data.getProperty(ColorPaletteManager::getInstance()->shortName, var()));
cpltTask->setProgress(1);

// Add updateCells call (after all loads):
ColorPaletteGridView::getInstance()->updateCells();
```

**Add to `addItemsFromData`** (near Preset equivalent around line 938):
```cpp
ColorPaletteManager::getInstance()->addItemsFromData(data.getProperty(ColorPaletteManager::getInstance()->shortName, var()));
```

### 9. `Source/MainComponent.cpp`

**Add includes** (near other Preset includes around lines 10-32):
```cpp
#include "Definitions/ColorPalette/ColorPaletteManagerUI.h"
#include "UI/GridView/ColorPaletteGridView.h"
```

**Add ShapeShifter panel registration** (near "Presets" def around line 124):
```cpp
ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Color Palettes", &ColorPaletteManagerUI::create));
```

**Add ShapeShifter grid registration** (near "Preset Grid" def around line 145):
```cpp
ShapeShifterFactory::getInstance()->defs.add(new ShapeShifterDefinition("Color Palette Grid", &ColorPaletteGridViewUI::create));
```

**Add view submenu entries** (near "Presets" submenu around line 170):
```cpp
ShapeShifterManager::getInstance()->isInViewSubMenu.set("Color Palettes", "Lists");
```

**Add grid submenu entry** (near "Preset Grid" submenu around line 191):
```cpp
ShapeShifterManager::getInstance()->isInViewSubMenu.set("Color Palette Grid", "Grids");
```

### 10. `Source/UserInputManager.h`

**Add forward declaration** (near `class Preset;` around line 17):
```cpp
class ColorPalette;
```

### 11. `Source/UserInputManager.cpp`

**Add include** (near other includes):
```cpp
#include "Definitions/ColorPalette/ColorPalette.h"
#include "Definitions/ColorPalette/ColorPaletteValue.h"
```

**Add handler in `gridViewCellPressed`** (after the `else if (type == "mapper")` block, around line 977):

```cpp
else if (type == "colorpalette") {
    ColorPalette* cp = Brain::getInstance()->getColorPaletteById(id);
    if (cp != nullptr) {
        // Push each stored color value to the current command's encoders
        for (int i = 0; i < cp->values.items.size(); i++) {
            ColorPaletteValue* cpv = cp->values.items[i];
            ChannelType* ct = dynamic_cast<ChannelType*>(cpv->channelType->targetContainer.get());
            if (ct != nullptr) {
                changeChannelValue(ct, cpv->paramValue->getValue());
            }
        }
    }
}
```

**How this works**: `changeChannelValue` (lines 722-800 in UserInputManager.cpp) finds or creates a `CommandValue` entry in the current command for the given `ChannelType`, then sets `valueFrom` to the new value. This is exactly what happens when the user turns an encoder — so it naturally pushes the color to all currently selected fixtures.

### 12. `Source/Definitions/DataTransferManager/DataTransferManager.h`

**Add member** (near `presetCopyMode` around line 27):
```cpp
EnumParameter* colorPaletteCopyMode;
```

### 13. `Source/Definitions/DataTransferManager/DataTransferManager.cpp`

**Add includes** (near Preset includes):
```cpp
#include "../ColorPalette/ColorPaletteManager.h"
#include "../ColorPalette/ColorPalette.h"
#include "../ColorPalette/ColorPaletteValue.h"
#include "UI/GridView/ColorPaletteGridView.h"
```

**Add targetType option** (near existing `targetType->addOption` calls):
```cpp
sourceType->addOption("Color Palette", "colorpalette");
targetType->addOption("Color Palette", "colorpalette");
```

**Add copy mode** (near `presetCopyMode` setup):
```cpp
colorPaletteCopyMode = addEnumParameter("Color Palette merge mode", "Color Palette record mode");
colorPaletteCopyMode->addOption("Replace", "replace");
colorPaletteCopyMode->addOption("Merge", "merge");
```

**Add visibility toggle in `updateDisplay()`**:
```cpp
colorPaletteCopyMode->hideInEditor = !(tgt == "colorpalette");
```

**Add recording logic in `execute()`** (after the `else if (trgType == "preset")` block):

```cpp
else if (trgType == "colorpalette") {
    valid = true;
    ColorPalette* target = Brain::getInstance()->getColorPaletteById(tId);
    if (target == nullptr) {
        target = ColorPaletteManager::getInstance()->addItem(new ColorPalette());
        target->id->setValue(tId);
        target->userName->setValue("Color Palette " + String(tId));
        target->updateName();
        target->values.clear();
    }

    ScopedLock lock(source->computing);
    source->computeValues();

    if (colorPaletteCopyMode->getValue() == "replace") {
        target->values.clear();
    }

    // Determine which ChannelTypes are "color" channels from BKEngine settings
    BKEngine* bke = dynamic_cast<BKEngine*>(Engine::mainEngine);
    Array<ChannelType*> colorChannelTypes;
    TargetParameter* colorParams[] = {
        bke->CPRedChannel, bke->CPGreenChannel, bke->CPBlueChannel,
        bke->CPWhiteChannel, bke->CPAmberChannel, bke->CPUVChannel,
        bke->CPCyanChannel, bke->CPMagentaChannel, bke->CPYellowChannel,
        bke->CPHueChannel, bke->CPSaturationChannel
    };
    for (auto* tp : colorParams) {
        ChannelType* ct = dynamic_cast<ChannelType*>(tp->targetContainer.get());
        if (ct != nullptr && !colorChannelTypes.contains(ct)) {
            colorChannelTypes.add(ct);
        }
    }

    // Apply encoder filters if this is the current programmer
    Array<ChannelFamily*> filters;
    if (UserInputManager::getInstance()->currentProgrammer == source) {
        filters.addArray(Encoders::getInstance()->selectedFilters);
    }

    // Collect color values from computed programmer output
    // We only care about the channel type + value, NOT the fixture binding
    HashMap<ChannelType*, float> collectedColors;
    for (auto it = source->computedValues.begin(); it != source->computedValues.end(); it.next()) {
        SubFixtureChannel* chan = it.getKey();
        std::shared_ptr<ChannelValue> cValue = it.getValue();
        ChannelType* ct = chan->channelType;

        if (ct != nullptr && cValue->endValue() != -1 && colorChannelTypes.contains(ct)) {
            ChannelFamily* chanFamily = dynamic_cast<ChannelFamily*>(ct->parentContainer->parentContainer.get());
            if (filters.size() == 0 || filters.contains(chanFamily)) {
                // Take the first value seen for each channel type (they should all be the same
                // for a "color palette" use case, since the user dials one color for all fixtures)
                if (!collectedColors.contains(ct)) {
                    collectedColors.set(ct, cValue->endValue());
                }
            }
        }
    }

    // Store collected color values into the ColorPalette
    for (auto it = collectedColors.begin(); it != collectedColors.end(); it.next()) {
        ChannelType* ct = it.getKey();
        float val = it.getValue();

        // Check if this channel type already exists in the palette
        ColorPaletteValue* existing = nullptr;
        for (int i = 0; i < target->values.items.size(); i++) {
            ChannelType* existCT = dynamic_cast<ChannelType*>(target->values.items[i]->channelType->targetContainer.get());
            if (existCT == ct) {
                existing = target->values.items[i];
                break;
            }
        }
        if (existing != nullptr) {
            existing->paramValue->setValue(val);
        } else {
            ColorPaletteValue* cpv = target->values.addItem();
            cpv->channelType->setValueFromTarget(ct);
            cpv->paramValue->setValue(val);
        }
    }

    target->selectThis();
    LOG("Color Palette recorded");
}
```

### 14. `Source/Definitions/Programmer/Programmer.cpp`

**Add "colorpalette" as a valid CLI target** in `runCliCommand()` or wherever target types are enumerated for the record action flow. Search for where `"preset"` is used as a processUserInput target type and add `"colorpalette"` alongside it.

Specifically, in the `userCanPressTargetType` conditional path, `"colorpalette"` needs to be a recognized type string so that `gridViewCellPressed` can route through the CLI action flow when the user is in record mode.

Search for where target types are validated (likely in `processUserInput` where `type == "preset"` etc.) and add the `"colorpalette"` case.

### 15. `BlinderKitten.jucer`

Add all new source files to the `.jucer` project file. Search for how Preset files are listed and add ColorPalette files in the same pattern:

```xml
<GROUP id="..." name="ColorPalette">
  <FILE id="..." name="ColorPalette.h" compile="0" resource="0" file="Source/Definitions/ColorPalette/ColorPalette.h"/>
  <FILE id="..." name="ColorPalette.cpp" compile="1" resource="0" file="Source/Definitions/ColorPalette/ColorPalette.cpp"/>
  <FILE id="..." name="ColorPaletteValue.h" compile="0" resource="0" file="Source/Definitions/ColorPalette/ColorPaletteValue.h"/>
  <FILE id="..." name="ColorPaletteValue.cpp" compile="1" resource="0" file="Source/Definitions/ColorPalette/ColorPaletteValue.cpp"/>
  <FILE id="..." name="ColorPaletteManager.h" compile="0" resource="0" file="Source/Definitions/ColorPalette/ColorPaletteManager.h"/>
  <FILE id="..." name="ColorPaletteManager.cpp" compile="1" resource="0" file="Source/Definitions/ColorPalette/ColorPaletteManager.cpp"/>
  <FILE id="..." name="ColorPaletteManagerUI.h" compile="0" resource="0" file="Source/Definitions/ColorPalette/ColorPaletteManagerUI.h"/>
  <FILE id="..." name="ColorPaletteManagerUI.cpp" compile="1" resource="0" file="Source/Definitions/ColorPalette/ColorPaletteManagerUI.cpp"/>
</GROUP>
```

And in the GridView group:
```xml
<FILE id="..." name="ColorPaletteGridView.h" compile="0" resource="0" file="Source/UI/GridView/ColorPaletteGridView.h"/>
<FILE id="..." name="ColorPaletteGridView.cpp" compile="1" resource="0" file="Source/UI/GridView/ColorPaletteGridView.cpp"/>
```

Generate unique IDs for each `id` attribute (random 6-char alphanumeric, matching existing pattern).

---

## Data Model Comparison

| Aspect | Preset | Color Palette |
|--------|--------|---------------|
| Storage | `BaseManager<PresetSubFixtureValues>` → per-fixture → `BaseManager<PresetValue>` → (ChannelType, value) | `BaseManager<ColorPaletteValue>` → (ChannelType, value) directly |
| Fixture binding | Yes — each entry has `targetFixtureId` + `targetSubFixtureId` | **No** — fixture-independent |
| Channel scope | Any channel type | **Only color channels** (as defined by BKEngine color picker settings) |
| Recall | Via Command system (preset reference in CommandValue) | Direct encoder push via `changeChannelValue()` |
| Recording | Full programmer computed values per fixture | Extract only color channel values, ignore fixture IDs |

---

## Recall Flow (Step by Step)

1. User selects fixtures (fixture grid / group grid / CLI).
2. User clicks Color Palette grid cell `N`.
3. `GridView::cellClicked()` calls `UserInputManager::gridViewCellPressed("colorpalette", N)`.
4. In `gridViewCellPressed`: `Brain::getColorPaletteById(N)` → get `ColorPalette*`.
5. Loop through `colorPalette->values.items[]`.
6. For each `ColorPaletteValue`: get `ChannelType*` from `channelType->targetContainer.get()`, get `float` from `paramValue->getValue()`.
7. Call `changeChannelValue(ct, val)` — this finds/creates a `CommandValue` in the current command and sets the value.
8. Encoders update automatically (the programmer's command structure change triggers encoder refresh).

---

## Recording Flow (Step by Step)

1. User selects fixtures and dials in a color via encoders.
2. User enters record mode (programmer CLI: "record" action).
3. User clicks Color Palette grid cell `N`.
4. `gridViewCellPressed` routes through CLI action: `processUserInput("colorpalette")` + `processUserInput(String(N))` + `processUserInput("enter")`.
5. This triggers `DataTransferManager::execute()` with `targetType = "colorpalette"`, `targetUserId = N`.
6. In `execute()`:
   a. Get or create `ColorPalette` with ID `N`.
   b. Compute programmer values via `source->computeValues()`.
   c. Get list of color ChannelTypes from BKEngine color picker settings (`CPRedChannel`, `CPGreenChannel`, etc.).
   d. Iterate `source->computedValues` — for each SubFixtureChannel whose `channelType` is in the color list, collect `(channelType, value)`. De-duplicate by ChannelType (take first occurrence).
   e. Store collected values into `ColorPalette::values` (update existing or add new `ColorPaletteValue` entries).
7. Grid view updates via manager listener.

---

## Grid Display

The Color Palette grid should visually show the stored color:

```cpp
void ColorPaletteGridView::updateCells() {
    BKEngine* bke = dynamic_cast<BKEngine*>(Engine::mainEngine);
    ChannelType* redCT = bke ? dynamic_cast<ChannelType*>(bke->CPRedChannel->targetContainer.get()) : nullptr;
    ChannelType* greenCT = bke ? dynamic_cast<ChannelType*>(bke->CPGreenChannel->targetContainer.get()) : nullptr;
    ChannelType* blueCT = bke ? dynamic_cast<ChannelType*>(bke->CPBlueChannel->targetContainer.get()) : nullptr;

    for (int i = 0; i < numberOfCells; i++) {
        ColorPalette* cp = Brain::getInstance()->getColorPaletteById(i + 1);
        if (cp != nullptr) {
            float r = 0, g = 0, b = 0;
            for (int j = 0; j < cp->values.items.size(); j++) {
                ChannelType* ct = dynamic_cast<ChannelType*>(cp->values.items[j]->channelType->targetContainer.get());
                float val = cp->values.items[j]->paramValue->getValue();
                if (ct == redCT) r = val;
                else if (ct == greenCT) g = val;
                else if (ct == blueCT) b = val;
            }
            Colour btnColor((uint8)(r * 255), (uint8)(g * 255), (uint8)(b * 255));
            gridButtons[i]->setColour(TextButton::buttonColourId, btnColor);

            // Choose text color for readability
            float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
            Colour textColor = luminance > 0.5f ? Colours::black : Colours::white;
            gridButtons[i]->setColour(TextButton::textColourOnId, textColor);
            gridButtons[i]->setColour(TextButton::textColourOffId, textColor);

            gridButtons[i]->setButtonText(cp->userName->getValue().toString());
        } else {
            gridButtons[i]->setButtonText("");
            gridButtons[i]->setColour(TextButton::buttonColourId, Colour(40, 40, 40));
            gridButtons[i]->setColour(TextButton::textColourOnId, Colour(96, 96, 96));
            gridButtons[i]->setColour(TextButton::textColourOffId, Colour(96, 96, 96));
        }
    }
}
```

---

## File Checklist

### New files (9 files):

| # | File | Type |
|---|------|------|
| 1 | `Source/Definitions/ColorPalette/ColorPalette.h` | Header |
| 2 | `Source/Definitions/ColorPalette/ColorPalette.cpp` | Implementation |
| 3 | `Source/Definitions/ColorPalette/ColorPaletteValue.h` | Header |
| 4 | `Source/Definitions/ColorPalette/ColorPaletteValue.cpp` | Implementation |
| 5 | `Source/Definitions/ColorPalette/ColorPaletteManager.h` | Header |
| 6 | `Source/Definitions/ColorPalette/ColorPaletteManager.cpp` | Implementation |
| 7 | `Source/Definitions/ColorPalette/ColorPaletteManagerUI.h` | Header |
| 8 | `Source/Definitions/ColorPalette/ColorPaletteManagerUI.cpp` | Implementation |
| 9 | `Source/UI/GridView/ColorPaletteGridView.h` | Header |
| 10 | `Source/UI/GridView/ColorPaletteGridView.cpp` | Implementation |

### Modified files (9 files):

| # | File | Changes |
|---|------|---------|
| 1 | `Source/Brain.h` | Forward decl, HashMap, register/unregister/getById declarations |
| 2 | `Source/Brain.cpp` | Include, register/unregister/getById implementations, clear |
| 3 | `Source/BKEngine.cpp` | Include, addChildControllableContainer, engine assignment, deleteInstance, clear, JSON save/load, addItemsFromData |
| 4 | `Source/MainComponent.cpp` | Include, ShapeShifterFactory defs, isInViewSubMenu entries |
| 5 | `Source/UserInputManager.h` | Forward declaration of ColorPalette |
| 6 | `Source/UserInputManager.cpp` | Include, `else if (type == "colorpalette")` handler in gridViewCellPressed |
| 7 | `Source/Definitions/DataTransferManager/DataTransferManager.h` | `colorPaletteCopyMode` member |
| 8 | `Source/Definitions/DataTransferManager/DataTransferManager.cpp` | Include, sourceType/targetType option, copyMode, updateDisplay, recording logic in execute() |
| 9 | `BlinderKitten.jucer` | FILE entries for all new source files |

### Possibly modified:

| # | File | Changes |
|---|------|---------|
| 10 | `Source/Definitions/Programmer/Programmer.cpp` | Add "colorpalette" as recognized target type in CLI action flow |

---

## Implementation Order

1. **ColorPaletteValue** (h + cpp) — simplest unit, no dependencies beyond OrganicUI.
2. **ColorPalette** (h + cpp) — depends on ColorPaletteValue and Brain.
3. **ColorPaletteManager** (h + cpp) — depends on ColorPalette.
4. **ColorPaletteManagerUI** (h + cpp) — depends on ColorPaletteManager.
5. **Brain.h/cpp** — add HashMap + register/unregister/getById.
6. **BKEngine.cpp** — wire up manager singleton + serialization.
7. **ColorPaletteGridView** (h + cpp) — depends on Brain, ColorPalette, BKEngine.
8. **MainComponent.cpp** — register panels and grids.
9. **UserInputManager** — recall handler.
10. **DataTransferManager** — recording logic.
11. **Programmer.cpp** — CLI target type support.
12. **BlinderKitten.jucer** — add all files.
13. **Build and test**.

---

## Reference Files

For each new file, use the corresponding Preset file as the template:

| New file | Template |
|----------|----------|
| ColorPalette.h/cpp | `Source/Definitions/Preset/Preset.h/cpp` (simplified — no fixture binding, no presetType, no useAnotherId, no SubFixtureValues manager, no computeValues) |
| ColorPaletteValue.h/cpp | `Source/Definitions/Preset/PresetValue.h/cpp` |
| ColorPaletteManager.h/cpp | `Source/Definitions/Preset/PresetManager.h/cpp` |
| ColorPaletteManagerUI.h/cpp | `Source/Definitions/Preset/PresetManagerUI.h/cpp` |
| ColorPaletteGridView.h/cpp | `Source/UI/GridView/PresetGridView.h/cpp` |
