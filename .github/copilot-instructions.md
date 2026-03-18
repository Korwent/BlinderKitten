# BlinderKitten – Copilot Instructions

## Project Overview

BlinderKitten is a cross-platform **DMX lighting control application** for stage lighting shows. It manages fixtures, cuelists, cues, effects, presets, groups, and more. Built with C++ on the [JUCE](https://juce.com/) framework and the [OrganicUI](https://github.com/benkuper/juce_organicui/) library.

- Website: <http://blinderkitten.lighting>
- Docs: <https://norbertrostaing.gitbook.io/blinderkitten/>

## Code Style

- **Language**: C++11/14.
- **Classes**: PascalCase (`SubFixtureChannel`, `CommandValue`).
- **Methods & members**: camelCase (`cuelistPoolUpdating`, `clearUpdates()`).
- **Header guards**: Use `#pragma once`.
- **Header boilerplate**: JUCE-style comment block with filename, creation date, and author.
- **Suffixes**: `Manager` for collection managers (`FixtureManager`), `UI` for view classes (`CuelistManagerUI`), `Action` for action classes (`CuelistAction`).

## Architecture

### Core singletons

| Class | Role |
|-------|------|
| `Brain` | Central state manager – HashMaps of all domain objects; thread-based update pools for Cuelists, Effects, Cues, Programmers, etc. |
| `BKEngine` | Application engine – global settings containers (`uiParamsContainer`, `virtualParamsContainer`, etc.), DMX channel views. Extends JUCE `Engine`. |

### Key patterns

- **Manager pattern**: Most domain objects have a `*Manager` that owns the collection and a `*ManagerUI` for the view.
- **OrganicUI parameters**: Use `IntParameter`, `FloatParameter`, `BoolParameter`, `StringParameter`, `ColorParameter`, `EnumParameter`, etc. from `juce_organicui`.
- **ControllableContainer**: All parameter groups extend `ControllableContainer`.
- **Update pooling**: Objects that need async updates are queued into `*PoolWaiting` / `*PoolUpdating` arrays processed by the `Brain` thread.

### Source layout

| Directory | Contents |
|-----------|----------|
| `Source/` | Main app files (`BKEngine`, `Brain`, `Main`, `MainComponent`) |
| `Source/Definitions/` | Domain objects: Fixture, Group, Preset, Cuelist, Cue, Effect, Carousel, Mapper, etc. |
| `Source/Definitions/Command/` | Command execution system |
| `Source/Definitions/Interface/` | I/O interfaces (DMX, MIDI, ArtNet, OSC) |
| `Source/Common/` | Shared utilities – DMX, MIDI, Serial, Zeroconf, NDI, Helpers |
| `Source/UI/` | UI components – GridViews, VirtualFaders, VirtualButtons, Encoders, Sheets |
| `Modules/` | Custom JUCE modules: `juce_organicui`, `juce_simpleweb` |
| `External/` | Third-party libs: asio, dnssd, hidapi, NDI, sdl, serial, servus |

### File formats

- `.olga` – JSON-based show files
- `.mochi` – Default configuration
- `.layout` / `.blinderlayout` – UI layout definitions

## Build & Test

**Never trigger a build yourself.** After adding or removing source files, resave the Projucer file (`BlinderKitten.jucer`) so the IDE projects are regenerated — but do not start a compilation. The user will build manually.
use this command : & "J:\Dev\JUCE\extras\Projucer\Builds\VisualStudio2022\x64\Debug\App\Projucer.exe" --resave "J:\Dev\BlinderKitten\BlinderKitten.jucer"

### Windows (primary)

Build via MSBuild with a Visual Studio 2022 solution:

```powershell
msbuild "Builds/VisualStudio2022/BlinderKitten.sln" /p:Configuration=Release /m
```

Requires JUCE from the custom fork: `norbertrostaing/JUCE` (branch `develop-local`), checked out as a sibling `JUCE/` directory.

### Other platforms

- **macOS**: Xcode project in `Builds/MacOSX_CI/`
- **Linux**: `Builds/LinuxMakefile/` (run `install_linux_deps.sh` first)
- **Raspberry Pi**: `Builds/Raspberry/` and `Builds/Raspberry64/`

### CI

GitHub Actions workflow in `.github/workflows/build.yml` builds Windows x64 + Win7-compatible variants on push.

## Conventions

- Register new domain objects in `Brain`'s HashMaps and create corresponding Manager & ManagerUI classes.
- New UI panels should follow the existing GridView pattern in `Source/UI/GridView/`.
- Interface implementations go in `Source/Definitions/Interface/interfaces/interfaces/`.
- Keep DMX channel calculations in the `Brain` thread update loop; never block the message thread with DMX math.
- Use JUCE's `var` / `DynamicObject` for serialization, not raw JSON libraries.

## Dependencies

Do not upgrade JUCE independently — the project uses a custom fork (`norbertrostaing/JUCE`, `develop-local` branch). OrganicUI is tightly coupled to this fork.
