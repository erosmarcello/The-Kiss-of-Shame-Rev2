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

- [ ] **Complete the JUCE port** — target current JUCE (8.x), not 7. Replace
      JUCE 3.1 idioms: `ScopedPointer` → `std::unique_ptr`,
      `AnimatedAppComponent` → `Timer`/`VBlankAttachment`,
      `SliderListener` → `Slider::Listener` / APVTS attachments.
- [ ] **CMake as the build system of record** (`juce_add_plugin`), keeping the
      `.jucer` only if useful for reference. Formats: **AU + VST3** on macOS,
      **VST3** on Windows, **VST3/LV2** on Linux. Standalone app for all.
- [ ] **Embed all resources as BinaryData** (`juce_add_binary_data`) —
      `GUI_Resources/` and `Audio_Resources/` — eliminating the hard-coded
      paths in `shameConfig.h` entirely.
- [ ] **Migrate parameters to `AudioProcessorValueTreeState`** with stable
      string parameter IDs (`saturation`, `shame`, `hiss`, `age`, `blend`,
      `output`, `tapeType`, `printThrough`, `environment`, `bypass`, `linkIO`).
      This buys automation, state save/restore, and thread-safe GUI binding in
      one move.
- [ ] **Sample-rate independence.** Remove `SAMPLE_RATE 44100`; derive all
      buffer sizes and modulation rates from `prepareToPlay()`. Resample the
      hiss/noise WAV assets at load time. Audit `Shame.h` (44100-sample
      circular buffer = exactly 1s) and `Flange.h` for rate assumptions.
- [ ] **CI: GitHub Actions** matrix build (macOS arm64+x86_64, Windows, Linux)
      plus **pluginval** at max strictness as the merge gate.
- [ ] **Golden-reference audio tests.** Before refactoring DSP internals,
      render a known test signal through the current algorithms at 44.1 kHz
      and store the output. Refactors must null (or diff within tolerance)
      against the reference, so modernization never silently changes the sound.

## Phase 1 — DSP: complete the original promises

**Goal: the signal path the 2014 design always intended, hardened.**

Signal flow stays as designed (`Source/AudioProcessing/AudioGraph.h`):
`InputSaturation → Flange (reels) → Environment → Hiss → Shame → Blend → Output`.

- [ ] **Implement the three missing environments** — Studio Closet, Humid
      Cellar, Hot Locker — using the Hurricane Sandy module
      (granulation + envelope dips + noise bursts + filtering) as the
      template. Each gets a distinct sonic fingerprint: closet = mild
      oxidation/dropouts, cellar = humidity-driven HF loss + sticky-shed
      artifacts, locker = heat-warped pitch instability + print-through
      exaggeration.
- [ ] **Oversample the saturation stage** (`juce::dsp::Oversampling`, 2–4×)
      to tame aliasing from the `tanh` waveshapers, preserving the dual-path
      odd/even harmonic structure and the 4 kHz rolloff character. Report
      latency to the host.
- [ ] **Fix the Biquads coefficient validation** flagged in `Biquads.h`.
- [ ] **Denormal protection** (`ScopedNoDenormals`) and parameter smoothing
      (`SmoothedValue`) on every audible control — no zipper noise.
- [ ] **Click-free bypass** (short crossfade) replacing the hard toggle.
- [ ] **Fix the playhead bug** noted at `PluginEditor.cpp:287`.
- [ ] **Channel-config sanity:** explicit mono and stereo support.

Deliberately **out of scope** (parameter-bloat guard): tape speed selection,
bias/EQ-curve menus, multi-machine modeling. Rev 2 deepens the existing seven
controls; it does not add an eighth knob.

## Phase 2 — GUI: the refresh and the **Era Switch** (flagship)

**Goal: two complete, first-class faces for one instrument.**

### Theme architecture

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
- [ ] **The Era Switch itself:** a small two-position toggle on the faceplate
      (proposed: lower-right utility strip, styled per active theme — a chrome
      bat-handle switch in Heritage, a macOS-style segmented control in
      Modern). Working label: **"ERA: 2014 / NOW."** Stored as a
      non-automatable plugin property plus a global preference
      (`ApplicationProperties`) so new instances open in the user's chosen era.
- [ ] **Deluxe transition:** ~300 ms crossfade between eras (snapshot the
      outgoing theme to an image, alpha-blend) — flipping the switch should
      feel like an event.

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
