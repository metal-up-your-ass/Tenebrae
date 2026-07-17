# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.0] - 2026-07-16

Research-derived deep-dive rework against the reference class of high-gain cascaded rhythm-guitar
distortion (modern cascade-amp lineages and their software/pedal equivalents) - see
`docs/design-brief.md` and `docs/research-notes.md` for the full sourcing (the only two documents
in this repo that name specific reference products, as citations). This is an additive/corrective
pass, not a rewrite: the core clipper transfer function,
per-stage cascade constants, 8x oversampling, Tight/Loose voicing tables, and all real-time-safety
patterns are unchanged and were validated by the research as either directly consistent with the
reference class or reasonable engineering simplifications with no sourced correction available.
Also ships the suite-wide M2 preset system (this repo's implementation of the pilot pattern from
`basilica-audio/nave`) and a German frame-string localisation.

### Added

- **Presence** parameter (-12 dB to +12 dB, default 0 dB/unity): a new post-cascade, post-tone-stack
  high-shelf at a sourced 2.4 kHz corner (see `docs/research-notes.md` for the exact citation),
  modelled on the reference class's power-amp Presence feedback control's functional position (not
  its circuit). At the 0 dB default the shelf is skipped entirely (a true structural bypass, not
  merely a near-unity filter), so existing sessions/presets are unaffected on migration.
- **Gate**: a new conventional fixed attack/hold/release expander/gate, inserted after Presence and
  before Level, gating the fully-voiced wet signal (catches noise the cascade's own gain generates,
  not just input noise). Four new parameters - Gate Threshold (-80 to 0 dB, default -48 dB), Gate
  Attack (0.1-20 ms, default 1 ms), Gate Hold (0-500 ms, default 20 ms), Gate Release (5-2000 ms,
  default 150 ms) - plus a Gate on/off toggle. **Deliberately defaults to ON**: this is the single
  highest-impact structural gap the research identified in v0.1 (every reference plugin/pedal in
  this genre ships a gate as a first-class module), and shipping it off by default would silently
  reproduce that gap. This is the one v0.2.0 change with a user-facing consequence for old sessions:
  loading a pre-v0.2.0 session now engages the Gate at its default settings on top of whatever was
  saved, which may audibly change the tail/silence behaviour of that session.
- M2 preset system (`src/presets/`): factory bank of 8 presets (`presets/factory/*.json` - Foundation
  Chug, Low-Tuned Percussive, Vintage Cascade, Scooped Wall, Cut-Through Lead-Adjacent, Bright
  Aggressive, Loose & Open, Full Dry/Wet Blend), user preset save/save-as/rename/delete, single-file
  and zip-bank import/export, dirty-state tracking, and a `PresetBar` UI strip docked at the top of
  the editor. See `docs/presets.md` for what each factory preset does and `docs/preset-system-notes.md`
  for the underlying architecture (copied verbatim from the pilot implementation in
  `basilica-audio/nave`).
- German (`de`) frame-string localisation for the preset bar (labels, menus, dialogs, error
  messages), auto-selected from the host OS's system language; core DSP/parameter terminology
  (Tight, Gain, Presence, Gate, dB, Hz, ms, %, etc.) is never translated, matching the suite-wide
  i18n convention.
- `tests/GateTests.cpp` (module-level Gate ballistics/bypass/NaN coverage) and
  `tests/PresetManagerTests.cpp` (17 preset-system tests, ported from the nave pilot) - see
  "Changed" below for extensions to existing test files.

### Changed

- **Tone stack Treble corner raised from 3.5 kHz to 5 kHz** (`ToneStack.cpp`'s internal filter
  constant - not a parameter ID/range change, so existing sessions keep working, but a non-zero
  Treble setting now sounds slightly different). Reasoned (not directly sourced to one reference
  number): with the new Presence control now owning the ~2.4 kHz region, Treble sits clearly above
  it instead of stacking on the same corner Bright already occupies (3.5 kHz, unchanged, pre-cascade),
  while staying below the cascade's own top-end rolloff ceiling.
- `docs/architecture.md` and `docs/manual.md` updated for the new signal flow (Presence, Gate),
  the Treble corner change, and the preset system.
- Extended `tests/StateTests.cpp` (Presence + all four new Gate parameters + Gate on/off in the
  non-default-round-trip test, plus a new test simulating an old v0.1.0-style state tree missing
  the new parameter IDs, confirming `AudioProcessorValueTreeState` falls back to the v0.2.0
  defaults rather than failing the load), `tests/ParameterTests.cpp` (parameter count 10 -> 16,
  new sections for Presence/Gate defaults/ranges), `tests/EngineTests.cpp` (Presence boost/cut and
  true-passthrough-at-0dB tests, Gate bypass/engaged/NaN-sweep tests), `tests/RobustnessTests.cpp`
  (extreme-value sweep and the exhaustive Voicing x Bright x Tone Voice combination test both now
  also cover Presence and Gate on/off x Threshold extremes), and `tests/ToneStackTests.cpp` (Treble
  test comment updated for the new corner).
- Bumped to JUCE 8.0.14's plugin version 0.2.0; `CMakeLists.txt` now embeds the preset/i18n
  BinaryData assets and links `juce_gui_basics` into `SharedCode` (needed by `PresetBar`'s
  `AlertWindow`/`FileChooser`/`PopupMenu` usage).

### Fixed

- Housekeeping: `ci/fix-release-workflow` (create-release-before-asset-upload fix) and dependabot
  `actions/checkout` 4->7 / `actions/cache` 4->6 bumps merged ahead of this release.

## [0.1.1] - 2026-07-16

### Changed

- Housekeeping: new icon motif with canonical squircle cutout embedded into the plugin binary (`ICON_BIG`) and README/manual, org link sweep, heavy-music copy reframe, README pointed at GitHub Releases, and the signed tag-triggered release CI workflow added.

### Fixed

- `clampBelowNyquist()` (`TenebraeEngine.cpp`, duplicated in `CascadeStage.cpp`) and `ToneStack::clampCombinedGainDb()` relied solely on `juce::jlimit()`, which is not NaN-safe (both of its internal comparisons evaluate false for NaN, so a NaN input previously fell through unclamped). A NaN Tight-frequency or Bass/Mid/Treble gain reaching these from host automation could produce NaN filter coefficients that poison a filter's delay-line state (persistently on arm64, where JUCE's snap-to-zero denormal cleanup is a no-op, vs. self-healing after one block on x86_64). Both helpers now replace a NaN input with a safe default before clamping. (#14)
- `CascadeStage::driveGain`, `TenebraeEngine::preGain`, and `TenebraeEngine::outputLevel` (all `juce::dsp::Gain<float>`) called JUCE's own `Gain::process()`, whose multichannel branch `alloca()`s a scratch buffer sized to the block on every call, with no upper bound or heap fallback - `driveGain` runs on the 8x-oversampled block, making it the single largest stack allocation in the whole signal chain. All three now go through a new `RealtimeGain::process()` helper (`src/dsp/RealtimeGain.h`) that replicates JUCE's own per-sample math into a caller-owned buffer sized once in `prepare()`, eliminating the audio-thread `alloca()`. (#12)
- `TenebraeEngine::process()` never compared the incoming block's sample count against the `maximumBlockSize` declared to `prepare()`. An oversized block (some hosts occasionally hand over one - offline bounce/render, buffer-size renegotiation) went straight into `juce::dsp::Oversampling`'s internal buffer, which is sized to exactly that declared maximum and only bounds-checks with a debug-only `jassert` - silent heap corruption with no exception to catch in a Release build. `process()` now chunks any oversized block down into `prepare()`-sized pieces via a new internal `processChunk()` rather than truncating it, so no more than the declared maximum ever reaches the oversampler (or any other `prepare()`-sized internal buffer) in one call. (#13)

## [0.1.0] - 2026-07-14

### Added

- Project bootstrap: README, license, contributing guide, architecture and build docs, ADRs, and CI workflow.
- DSP core: initial working Tenebrae signal path (Tight HPF, Gain, 3-stage oversampled waveshaper cascade, 3-band tone stack, Level, delay-compensated dry/wet Mix) with unit tests.
- **Voicing** parameter (Tight/Loose): switches the fixed cascade drive/asymmetry/interstage-filter constants between the original "Tight" cascade and a new, softer-driven, wider-band "Loose" alternative. Both cascade triplets are fully preallocated so switching is allocation-free.
- **Bright** parameter: fixed pre-cascade high-shelf switch (+5 dB @ 3.5 kHz), modelled on a high-gain amp channel's bright switch, feeding a brighter signal into the cascade's nonlinearity rather than just re-EQing the already-clipped output.
- **Tone Voice** parameter (Flat/Scoop/Boost): a fixed dB tilt added on top of the live Bass/Mid/Treble tone-stack bands, for one-switch access to canned high-gain-rhythm tone shapes (a "smiley" scoop curve and a mid-forward boost curve) without overriding the individual band knobs.
- `docs/manual.md`: full user manual (what the plugin is, where it sits in a symphonic-metal chain, signal flow, complete parameter reference, usage tips).

### Changed

- Tone stack corner frequencies refined against typical high-gain rhythm voicings: Mid peak moved from 800 Hz to 650 Hz with a narrower Q (0.8 -> 1.1) for a more surgical scoop, and the Treble shelf moved from 3000 Hz to 3500 Hz to sit at a more typical amp "presence" corner.
- Broadened the Catch2 suite from 26 to 36 test cases: sample-rate sweeps (44.1-192 kHz), mono/stereo bus-layout coverage, a longer-running (400-block) NaN/Inf automation soak test, exhaustive Voicing/Bright/Tone Voice combination coverage, and dedicated tests for every new parameter (state round-trip, defaults/ranges, DSP-level effect).

### Fixed

- `TenebraeEngine::outputLevel` (the Level gain stage) was never primed with a starting gain in `prepare()`; since `juce::dsp::Gain` default-constructs its internal smoothed value to linear zero (silence) rather than unity, any `prepare()` call not immediately preceded by a fresh `setLevelDb()` call produced a permanently silent wet path. This never reached the shipped plugin (`PluginProcessor::prepareToPlay()` always seeds Level from the host/session state first) but was a real trap for anything exercising `TenebraeEngine` directly, found via the broadened mono-bus test coverage above. Fixed by re-priming `outputLevel` from a new `lastLevelDb` member on every `prepare()`, matching the existing `lastGainDb`/`lastTightHz` pattern.
