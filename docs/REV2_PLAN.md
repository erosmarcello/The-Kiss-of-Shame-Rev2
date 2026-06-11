# The Kiss of Shame — Rev 2 "Deluxe" Plan

> *Finish what 2014 started.* Ship the completed, deluxe Kiss of Shame: a fully
> working, cross-platform, modern plugin that preserves the original soul — and
> introduces the **Era Switch**, a one-click toggle between the original
> skeuomorphic Mac OS X-golden-era GUI and a modern flat, dimensional
> macOS-style theme.

This plan synthesizes three sources of truth:

1. **The codebase** (this repository) — JUCE 3.1-era source, ~3,800 lines,
   bitmap-filmstrip GUI, incomplete JUCE 7 migration.
2. **The BPB release coverage** (Apr 2024) — what the community values most:
   the Shame knob, the interactive animated reels, Environments, the two tape
   formulas.
3. **The Rev 1 case study** — the original design philosophy: interface-first
   ("the UI design dictated the algorithm design"), reductionist control set
   (15+ params consolidated to 7), collapsible UI for screen real estate,
   environmental storytelling (Hurricane Sandy), accessibility-first human
   factors.

The guiding constraint for Rev 2 is the same as Rev 1: **no parameter bloat.**
Every addition must either complete an unfinished promise of the original
design or deepen the experience without widening the control surface.

---

## Current state (audit summary)

| Area | Status |
|---|---|
| Build | Projucer `.jucer` retargeted to JUCE 7 (commit `8d9e543`), but source is still JUCE 3.1 idiom — **does not compile**. macOS/Xcode only. |
| Parameters | Legacy index-based `setParameter()`/`getParameter()` (`Source/PluginProcessor.h`); no `AudioProcessorValueTreeState`, so no robust automation/state. |
| Sample rate | Hard-coded `#define SAMPLE_RATE 44100` in `Source/shameConfig.h`. |
| Asset loading | Hard-coded filesystem paths (`/Users/Shared/KissOfShame/`) in `Source/shameConfig.h` — assets not embedded. |
| Memory idiom | `ScopedPointer` throughout (~74 uses); heap-allocated DSP modules. |
| Environments | Enum defines Off / StudioCloset / HumidCellar / HotLocker / HurricaneSandy, but **only Hurricane Sandy is implemented** (`Source/AudioProcessing/HurricaneSandy.h`). |
| GUI | Fixed 960×703 bitmap-filmstrip skeuomorphic design; collapsible reel mode (703px ↔ 266px); no resizing, no HiDPI strategy. |
| Known bugs in comments | Biquad coefficient validation incomplete (`Biquads.h`); 1-px OpenGL seam workaround (`ImageAnimationComponent.h`); playhead-position oddity (`PluginEditor.cpp:287`). |

One major liberation: the plugin **never shipped in working form**, so there is
zero session/state backwards-compatibility burden. Rev 2 can define the
canonical state format from day one.

---

## Phase 0 — Foundation: make it build everywhere

**Goal: a green CI badge before any feature work.**

- [x] **Complete the JUCE port** — done against JUCE **8.0.9**. All JUCE 3.1
      idioms replaced: `ScopedPointer` → values/`std::unique_ptr`,
      `AnimatedAppComponent`+OpenGL → plain `Timer` component,
      `SliderListener`/`ActionBroadcaster` → lambdas + APVTS attachments.
- [x] **CMake as the build system of record** (`juce_add_plugin`); legacy
      `JuceLibraryCode/` and `Builds/` removed. Formats: AU + VST3 + Standalone.
- [x] **Embed all resources as BinaryData** — done; `shameConfig.h` no longer
      contains a single path. Only the 3 runtime WAVs ship (the dev test tones
      stay out of the binary).
- [x] **Migrate parameters to `AudioProcessorValueTreeState`** — done with
      versioned IDs; bypass exposed via `getBypassParameter()`. linkIO remains
      GUI-only behavior, exactly as Rev 1 intended.
- [x] **Sample-rate independence** — `SAMPLE_RATE`/`BUFFER_SIZE` macros gone;
      every module takes the rate in `prepare()`; hiss/noise beds are
      resampled at load; Shame's tape loop is one second at any rate.
- [x] **CI: GitHub Actions** matrix build (macOS, Windows, Linux) with the
      smoke tests as gate. *(pluginval step: still to add.)*
- [x] **Offline audio smoke tests** (`Tests/SmokeTest.cpp`): pass-through,
      Shame/Extreme, all six environment positions, state round-trip — run via
      `ctest`. *(Golden-reference renders for regression-pinning the Rev 2
      sound: still to add once voicings are approved.)*

## Phase 1 — DSP: complete the original promises

**Goal: the signal path the 2014 design always intended, hardened.**

Signal flow stays as designed (`Source/AudioProcessing/AudioGraph.h`):
`InputSaturation → Flange (reels) → Environment → Hiss → Shame → Blend → Output`.

- [x] **All environments implemented, ground-up** (`Environments.h`): a common
      `EnvironmentFX` base with Environs (generic shelf wear), Studio Closet
      (oxidation, dropouts, crackle), Humid Cellar (sticky-shed muffling,
      undulation, damp rumble), Hot Locker (slow wow, sagging level,
      exaggerated print-through) and Hurricane Sandy (original 2014 recipe,
      with its uninitialized burst-level bug fixed). AGE drives them all.
- [x] **Finished the unfinished front panel:** tape-type button now voices
      saturation + hiss floor (S-111 vs A-456); PRINT THRU is a real
      lowpassed post-echo stage (`PrintThrough.h`); AGE actually does
      something. The Rev 1 crash class (missing-WAV divide-by-zero in
      Hiss/LoopCrossfade) is gone.
- [x] **Shame EXTREME** — double-click the Shame knob: the macro mapping runs
       past its stops (2.5× depth, faster/looser modulation) plus a
      scrape-flutter component. Recallable state property, not a parameter.
- [ ] **Oversample the saturation stage** (`juce::dsp::Oversampling`, 2–4×)
      to tame aliasing from the `tanh` waveshapers, preserving the dual-path
      odd/even harmonic structure and the 4 kHz rolloff character. Report
      latency to the host.
- [x] **Fix the Biquads coefficient validation** flagged in `Biquads.h` —
      all designs clamp fc into (10 Hz, 0.49·fs).
- [x] **Denormal protection** (`ScopedNoDenormals`) in processBlock; bypass
      and print-through levels smoothed. *(Per-knob `SmoothedValue` zipper
      pass: still to do.)*
- [x] **Click-free bypass** — 50 ms crossfade back to the untouched input,
      then true zero-work bypass.
- [x] **Playhead handling rebuilt** on the modern `getPosition()` API with
      atomics; the Rev 1 gating bug is moot.
- [x] **Channel-config sanity:** explicit mono and stereo layouts, channel
      clamps in every module (Rev 1 read channel 1 unconditionally).

Deliberately **out of scope** (parameter-bloat guard): tape speed selection,
bias/EQ-curve menus, multi-machine modeling. Rev 2 deepens the existing seven
controls; it does not add an eighth knob.

## Phase 2 — GUI: the refresh and the **Era Switch** (flagship)

**Goal: two complete, first-class faces for one instrument.**

### Theme architecture

- [x] *(v1 shipped — refinement ongoing)* Theme abstraction in
      `GUIUtilities/Theme.h` (`UIEra` + `ModernTheme` palette/draw helpers);
      every control branches on era: vector knobs (the Shame knob keeps the
      cross), labelled pill buttons, needle VU meters, vector reels (same
      frame counter as the filmstrips, so transport/drag behavior is
      identical), environment name pill, ember backlight. Original goal text
      follows:
- [ ] Introduce a `ShameTheme` abstraction. Controls never paint directly;
      they delegate to the active theme:
  - **Heritage** — the original skeuomorphic identity: the existing 65-frame
    filmstrip knobs, 31-frame reel animation, bitmap VU meters, pink backlight
    (`GUI_Resources/KOS_Graphics/`). The current `CustomKnob` /
    `ImageInteractor` / `ImageAnimationComponent` rendering paths are
    refactored *behind* the theme interface, not rewritten.
  - **Modern** — fully vector-drawn (`juce::Path` + a `LookAndFeel_V4`
    subclass) in the flat, dimensional language of recent macOS: soft material
    panels, continuous-corner rounding, restrained shadows, SF-style
    typography (ship **Inter** to avoid San Francisco licensing), with the
    signature pink (#D87063) of the cross-logo LED as the single accent color.
- [x] **The Era Switch itself** (`GUIUtilities/EraSwitch.h`): lower-right
      utility strip, labelled **2014 / NOW**, with the *recessed interaction*:
      the plunger must be pressed INTO the faceplate and held (~350 ms) until
      it arms with a pink ring — release then flips the era; a casual tap just
      springs back, like the protected switches on real tape machines. Chrome
      plunger in Heritage, flat thumb in Modern. Stored as a non-automatable
      state property. *(Global `ApplicationProperties` preference for new
      instances: still to add.)*
- [x] **Deluxe transition:** ~300 ms snapshot crossfade between eras.

### Feature parity is a hard requirement

Both eras must offer the complete experience — anything less makes one theme
the "real" plugin and the other a skin:

- [ ] Interactive, audio-reactive **reels with click-drag flange** (BPB called
      this the most novel feature; AES 2014 research called it the most
      compelling). Modern era re-renders the reels as vector/canvas animation
      at the same 50 fps feel.
- [ ] **Collapsible interface** (full ↔ reels-hidden), honoring the original
      ~workflow rationale for field recordists and small screens.
- [ ] **VU meters** — Heritage keeps the filmstrips; Modern draws needle
      ballistics vectorially (and gets *true* VU ballistics: 300 ms
      integration, proper overshoot).
- [ ] Environment selector, tape-type and print-through toggles, link-I/O —
      all present in both eras.

### Modernization applying to both eras

- [ ] **Resizable, HiDPI-correct UI.** Modern era scales natively (vectors).
      Heritage scales via high-quality image resampling; audit
      `GUI_Resources/` for source fidelity and re-export @2x assets where the
      originals allow (Yannick Bonnefoy's source files, if recoverable, are
      the gold mine here — otherwise targeted AI upscaling of the filmstrips).
- [ ] Replace the OpenGL 1-px seam workaround in `ImageAnimationComponent.h`
      with correct frame-atlas drawing.
- [ ] **Accessibility pass:** JUCE accessibility handlers, keyboard
      operability, labeled controls for screen readers — making the case
      study's accessibility-first philosophy literally true in code.

## Phase 3 — Deluxe layer

- [ ] **Preset system:** factory bank (e.g. *Pristine Master*, *Cassette
      Memory*, *Vinegar Syndrome*, *Sandy '12*) + user presets with a minimal
      browser; A/B compare slot.
- [ ] **Full automation** of all seven controls (free with APVTS) — verified
      per-host (Logic, Live, Pro Tools via AU/VST3, Reaper).
- [ ] **Settings drawer** (small, unobtrusive): era preference, UI scale,
      meter ballistics — kept out of the main faceplate.
- [ ] **Quality gates:** pluginval level-10 clean, AddressSanitizer/UBSan CI
      runs, manual host-matrix smoke test.

## Phase 4 — Release engineering

- [ ] Signed + notarized macOS `.pkg`; signed Windows installer; Linux
      tarball/LV2.
- [ ] README overhaul: real build instructions (currently "TODO"), screenshots
      of **both eras**, credits intact (Eros Marcello — Founder/Chief Product
      Architect; Brian Hansen — DSP; Yannick Bonnefoy — GUI; Matthijs
      Hollemans — 2024 revival).
- [ ] `CHANGELOG.md`, semantic versioning starting at **v2.0.0**, GPL-3.0
      retained.
- [ ] Launch collateral: BPB follow-up pitch ("the abandoned plugin that came
      back twice"), case-study addendum covering Rev 2 — the Era Switch is the
      story: *the plugin that lets you choose which decade you're mixing in.*

---

## Sequencing & milestones

| Milestone | Contents | Exit criterion |
|---|---|---|
| **M0** | Phase 0 complete | CI green on 3 OSes; pluginval passes; golden-reference tests in place |
| **M1** | Phase 1 DSP | All 4 environments audible; saturation oversampled; references null |
| **M2** | Theme architecture + Heritage parity | Heritage era pixel-faithful to Rev 1 inside the new architecture, resizable |
| **M3** | Modern era | Full feature parity; Era Switch + crossfade working |
| **M4** | Phase 3 deluxe | Presets, A/B, accessibility, host matrix verified |
| **M5** | Phase 4 release | Signed installers, docs, v2.0.0 tagged |

M0→M1 are sequential (DSP work needs the build + reference tests). M2/M3 GUI
work can proceed in parallel with M1 once M0 lands.

## Key risks

1. **Asset resolution.** The 960×703 1× bitmaps are the Heritage era's soul
   but predate Retina. Mitigation: recover Bonnefoy source files; fall back to
   careful upscaling; worst case, Heritage caps at 1.5× scale while Modern
   scales freely.
2. **Sound-character drift during modernization.** The hard-coded 44.1 kHz
   assumption is woven through `Shame`, `Flange`, and the WAV-based `Hiss`.
   Mitigation: golden-reference null tests *before* touching internals.
3. **JUCE 3.1 → 8 behavioral gaps** (`AnimatedAppComponent`,
   `ActionBroadcaster` patterns). Mitigation: replace rather than shim —
   these are isolated to the GUI layer.
