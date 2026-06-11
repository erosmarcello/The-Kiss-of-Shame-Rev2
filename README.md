# The Kiss of Shame – DSP Magnetic Tape Emulation

The Kiss of Shame, debuted at the Audio Engineering Society Convention 2014 in Los Angeles, was a pioneering DAW plugin that leveraged commercial UX/UI design principles to shape its magnetic tape + circuitry emulation algorithms.

To differentiate itself in the competitive pro-audio plugin market, The Kiss of Shame introduced groundbreaking features including an interactive, multi-touch-ready GUI and analog tape degradation simulation for distinctive audio effects.

The Kiss of Shame was never released. The source code was graciously donated to the open source community by its owner in 2024.

> **Rev 2 is in active development.** The plug-in now builds and passes audio
> with JUCE 8 (CMake), with all parameters functional — including the storage
> environments, tape types and print-through that were unfinished in the
> original source drop. See [docs/REV2_PLAN.md](docs/REV2_PLAN.md) for the
> full deluxe-overhaul roadmap.

## Installation instructions

Build from source (below) — installers arrive with the v2.0.0 release.

## How to use this plug-in

**Choose between two distinct tape types:**

**S-111** – A superior reel format popular from the 50s to 70s, was the preferred reference tape for many engineers. The Kiss of Shame introduces its first digital emulation, bringing this legendary format to the digital world.

**A-456** – This classic, high-output/low-noise format is a recording staple used in countless productions. While many software emulations exist, none recreate it quite like this. Unique digital recreation tactics were employed to capture its essence.

**From Weathered to *Weather***:

**Age** – This knob allows the user to legislate the amount of hypothetical time the selected tape type has been subjected to the chosen "Environment" to manipulate the severity of the corresponding effects.

**Environment** – Choose between several simulated storage conditions to inflict the sonic ramifications of factors such as magnetic particle instability, oxidation, lubricant loss, tape pack expansion/contraction, "vinegar syndrome" and more upon the source material. Users can even choose a "Hurricane Sandy" setting to access processing modeled from tape immersed and then recovered from the storm's flood waters.

**A real-world obstacle:**

**Shame** – The Kiss of Shame recreates the full spectrum of these factors like Drift, Wow, Flutter and Scrape-Flutter which the user can impart with the center knob. It can take your source signal from mildly colored to totally mangled.

**Print-Through** – Also known as "bleed-through", this emulation captures the mechanical speed fluctuations present in analog recordings. While they posed challenges for engineers in the past, they became a hallmark of classic records.

**Reach out and touch tape:**

The Kiss of Shame is the first tape plug-in to feature animated, interactive reels that can be manipulated with a simple click or touch. This allows users to access authentic analogue tape flange in real-time, without the need for two physical tape decks, and in a fraction of the time. All parameters, including reel movements, are fully automatable, and for screen real estate optimization, the reels are collapsible and fully customizable.

## Secret handshake: Shame EXTREME

Double-click the Shame knob. Like an LA-2A with all of its buttons pressed in
at once, Extreme mode drives the wow/flutter/drift engine past its design
stops and engages scrape-flutter no front-panel setting can reach. The
backlight runs hot while you're in it; double-click again to come back.
Extreme is saved with your session but deliberately adds no parameter — the
control surface stays exactly seven controls.

## Plug-in formats & cross-DAW compatibility

| Format | Status |
| --- | --- |
| **AU** (Logic, GarageBand) | Built on macOS out of the box |
| **VST3** (Cubase, Live, Reaper, Studio One, FL, Bitwig…) | Built on macOS / Windows / Linux out of the box |
| **AAX** (Pro Tools) | Build target ready — configure with `-DKOS_AAX_SDK_PATH=/path/to/aax-sdk`. Requires Avid's AAX SDK (NDA) and PACE signing to load in release Pro Tools |
| VST2 | Not possible: Steinberg closed VST2 licensing in 2018; VST3 is its successor |

Every CI run gates the VST3 through [pluginval](https://github.com/Tracktion/pluginval)
at strictness level 5 on Linux, macOS and Windows — the systematic answer to
per-host anomalies (the kind we used to chase in Cubase). The smoke tests also
pixel-diff the Shame knob to guarantee the cross animation never silently
stops again.

## Building from source code

Rev 2 uses CMake and fetches JUCE 8 automatically:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Artifacts land in `build/KissOfShame_artefacts/Release/` (VST3 and Standalone
everywhere; AU additionally on macOS). On Linux, install the usual JUCE
dependencies first
(`libasound2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev
libxcursor-dev libxcomposite-dev libfreetype-dev libfontconfig1-dev`).

Run the offline audio smoke tests with `ctest --test-dir build -C Release`.

## Credits & license

Copyright (C) 2014-2015 Eros Marcello

Original developers:

- [Eros Marcello](https://www.github.com/erosmarcello) — Founder, Chief Product Architect

- [Brian Hansen](https://brianhansen.sonimmersion.com/) — DSP Engineering / Algorithm Development

- [Yannick Bonnefoy](https://nanopsy.tv/) — GUI

This program is free software: you can redistribute it and/or modify it under the terms of the [GNU General Public License](https://www.gnu.org/licenses/gpl-3.0.en.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

JUCE is copyright © Raw Material Software.
