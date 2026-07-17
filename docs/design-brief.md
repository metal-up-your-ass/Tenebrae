# Tenebrae — Design Brief v2 (target v0.2.0)

Status: draft, for review. Supersedes the M1 "engineered core" voicing decisions documented in
`docs/architecture.md` where noted. Full sourcing in `tenebrae-research-notes.md` (same
directory).

> **Template note:** the task's named structural template
> (`scratchpad/miserere-design-brief-v2.md`) does not exist in this scratchpad — confirmed via
> `find`. This brief follows the structure given directly in the task instructions instead
> (why-v1-falls-short → topology → module specs → test guarantees → honesty → versioning).

---

## 1. Why v1 falls short

Tenebrae v1 ("M1 DSP completion") is a coherent, well-tested **engineered** cascade — three
tanh-clipper stages with hand-picked, internally-consistent drive/asymmetry/filter constants,
a passive-style 3-band tone stack, and clean real-time-safety/latency handling. It is not
*wrong* DSP. But measured against the reference class that actually defines "high-gain rhythm
distortion" — Mesa Rectifier-family cascade amps, the Peavey 5150/6505 lineage, and the
current commercial software reference (Neural DSP's Archetype line) — three things stand out:

1. **No noise gate.** Every reference plugin/pedal in this exact genre (chug-focused,
   cascaded high-gain metal rhythm tone) ships a gate as a first-class module, because a
   3-stage cascade amplifies noise floor and string noise by three stages' worth of gain
   before the tone stack even sees it. Tenebrae has none. This is the single highest-impact
   gap — see research notes §7.
2. **"Tightness" is one knob doing a job the reference class splits across two.** Real
   high-gain amps separate a low-frequency *feedback* control (Resonance, "tight/boom") from
   a high-frequency *feedback* control (Presence, prevents "fizzy digital sound" at high
   gain). Tenebrae has Tight (a plain pre-cascade HPF) and nothing else — Bright is a fixed
   switch, not a control, and there is no post-cascade Presence-style shelf at all.
   Research notes §3.
3. **The tone stack's Treble corner and control count sit below the reference convention.**
   3.5 kHz is well below the reference class's Presence-pivot range (1.6–2.4 kHz observed on
   a Rectifier, though that's a feedback control not a shelf) and below typical TMB Treble
   corners (9–10 kHz ballpark). And three independent bands with no Presence-equivalent is
   thinner than the four-to-five-control convention (Bass/Mid/Treble + Resonance + Presence)
   the whole reference class uses. Research notes §2.

The cascade's core nonlinearity (shifted-tanh asymmetric clipping) and its 8x oversampling
factor are both **validated** by the research — no change is proposed there. This is a
targeted correction, not a rewrite: add what's structurally missing (gate), and give
"tightness" and "top end" the two-control shape the reference class actually uses.

---

## 2. Topology (v0.2.0)

```
Input --> Tight (HPF, 20-300 Hz) --> Bright (switch) --> Gain (0-40 dB) --> [8x oversampled]
              Cascade stage 1 -> Cascade stage 2 -> Cascade stage 3   (Voicing: Tight/Loose)
                                                              |
                                                        [8x downsampled]
                                                              |
   Output <-- Mix <-- Level <-- Gate <-- Presence <-- Treble <-- Mid <-- Bass <--+  (tilted by Tone Voice)
     ^
     |
delay-compensated dry path
```

Changes from v1, in signal order:

- **Tight**: unchanged (20–300 Hz HPF, default 90 Hz) — validated as a reasonable
  simplification of "remove mud before the cascade," kept as-is.
- **Bright**: unchanged (fixed pre-cascade high-shelf switch).
- **Gain / cascade / Voicing**: unchanged core mechanism (shifted-tanh cascade, Tight/Loose
  voicing tables, 8x oversampling) — validated, not touched in v0.2.0.
- **New: Presence**, inserted post-tone-stack, pre-Level. A high-shelf control (not a fixed
  switch), modelled functionally on the reference class's Presence knob: boosts/cuts upper-mid
  to treble energy, deliberately placed *after* the cascade (unlike Bright, which is
  pre-cascade) since the reference-class Presence control is a power-amp *feedback* stage
  that acts on the already-distorted signal, not the input to distortion.
- **New: Gate**, inserted post-tone-stack/Presence, pre-Level (so it gates the fully-voiced
  signal, matching where reference plugins place their gates in the chain — see research
  notes §7, "usually first, or right after your amp sim"). Envelope-follower expander/gate
  with threshold, attack, hold, release — see §3.5 for full spec.
- **Tone stack**: Treble corner raised, band count and structure otherwise unchanged (see
  §3.4). Bass/Mid corners kept — Mid's 650 Hz sits inside the reference class's cited
  300 Hz–1.6 kHz "focus" zone, no sourced reason to move it.
- **Level / Mix**: unchanged.

---

## 3. Module specs — sourced defaults

Every default below is either (a) sourced to a specific quote/number in the research notes, or
(b) explicitly marked "reasoned, not sourced" with the reasoning stated. No brand or person
names appear in any parameter/UI name below — reference-class names (Rectifier, 5150, Nolly,
Decimator, etc.) are research-notes-only.

### 3.1 Tight (unchanged)

Range 20–300 Hz, default 90 Hz, log taper, 2nd-order Butterworth HPF, pre-cascade. No change —
this is Tenebrae's existing engineered simplification of "remove low end before the cascade
generates mud," and the research didn't surface grounds to move the default; it sits
comfortably inside the reference class's cited HPF ranges (patent literature: 80–120 Hz
typical corner for this purpose; TMB Bass-pot minimum cutoff cited at 64 Hz).

### 3.2 Gain / cascade / Voicing (unchanged)

No change to `AsymmetricClipper`'s `y = tanh(x + a) - tanh(a)` transfer function, the
Tight/Loose per-stage tables, or the 8x oversampling factor. Research notes §5–6 validate both
choices against the literature. **Explicitly reasoned, not sourced**: the exact per-stage
drive/asymmetry/HP-LP numbers remain engineered constants — no manufacturer publishes their
actual cascade values, so there is nothing further to source them against. This is called out
in the honesty section (§5) rather than "fixed."

### 3.3 New: Presence

- **Range**: -12 dB to +12 dB, default 0 dB (unity — a new control must not change the
  default tone of any existing preset/session on migration; see §6).
- **Shape**: high-shelf, corner ≈ 2.4 kHz.
  - *Sourced*: the Rectifier's Channel 3 Modern Presence control "pivot[s] at 2.4 kHz" (vs.
    1.6 kHz on Channel 2) — research notes §2. 2.4 kHz is chosen as the more "modern
    high-gain" of the two documented pivots, consistent with Tenebrae's own "modern-leaning"
    default Voicing (Tight).
- **Placement**: post-cascade, post-tone-stack (before Gate/Level) — *reasoned*: the reference
  class's Presence control is a power-amp negative-feedback stage acting on the already-driven
  signal, not a pre-distortion EQ; placing it after the cascade (unlike Bright, which is
  deliberately pre-cascade) mirrors that functional position without claiming to model the
  feedback-loop circuit itself.
- **Smoothing**: linear `SmoothedValue` on the dB value, same pattern as Bass/Mid/Treble
  (`ToneStack`'s existing 50 ms smoothing time constant) — *reasoned*, for consistency with
  the plugin's existing smoothing conventions; Presence is a continuous control like
  Bass/Mid/Treble, not a discrete switch like Bright/Voicing/Tone Voice.

### 3.4 Tone stack — Treble corner change

- **Bass**: unchanged, 150 Hz low-shelf, ±15 dB.
- **Mid**: unchanged, 650 Hz peak, Q 1.1, ±15 dB.
- **Treble**: corner raised from 3.5 kHz to **5 kHz** high-shelf, ±15 dB (range unchanged).
  - *Reasoned, partially sourced*: the reference class's TMB Treble corners cluster around
    9–10 kHz and Presence pivots at 1.6–2.4 kHz — with the new Presence control now owning the
    2.4 kHz region, Treble should sit clearly above it rather than stacking on the same
    corner Bright already occupies (3.5 kHz, unchanged, pre-cascade). 5 kHz is chosen as a
    reasoned midpoint that separates Treble from both Bright (3.5 kHz) and the new Presence
    (2.4 kHz) while staying below the cascade's own top-end rolloff ceiling (interstage LP
    corners top out at 9 kHz in the Tight voicing) — not directly sourced to a single
    reference number, called out here rather than presented as measured.
- **combinedGainLimitDb**: unchanged at 21 dB, now also bounds Treble+Presence stacking
  headroom by construction (each is independently clamped; no new combined-clamp logic
  needed since Presence is a separate filter stage, not summed into the Treble band).

### 3.5 New: Gate

- **Placement**: post-tone-stack/Presence, pre-Level — *sourced*: "The gate usually goes early
  in your plugin chain on the guitar track, often first, or right after your amp sim" —
  research notes §7. Tenebrae's Gate sits at the "right after the amp sim" position, since
  Tenebrae itself is the "amp sim" here — gating the fully-voiced wet signal, not the raw
  pre-cascade input, so noise generated *by* the cascade's own gain is caught too.
- **Controls**: Threshold (-80 dB to 0 dB, default -48 dB), Attack (0.1 ms–20 ms, default
  1 ms), Hold (0 ms–500 ms, default 20 ms), Release (5 ms–2000 ms, default 150 ms), plus a
  Gate on/off bypass (default **on** — see honesty section on why this is a default change
  worth flagging).
  - *Reasoned, not sourced to exact numbers*: no reference gate publishes exact
    attack/hold/release defaults (ISP Decimator deliberately exposes only a single Threshold
    control and hides its ballistics behind a proprietary adaptive algorithm — "Time Vector
    Processing" — precisely so users don't have to tune attack/release by hand). Tenebrae's
    fixed four-control gate is a conventional expander/gate (attack/hold/release), not an
    attempt to replicate that adaptive algorithm — an explicit, named simplification (see §5).
  - Attack default (1 ms) and Release default (150 ms) are reasoned from the qualitative
    behaviour described for tight metal gating: fast enough to not clip transient pick
    attack, slow enough that a held/sustained chord doesn't chatter — consistent with, but
    not numerically sourced from, the "smooth decay... on long sustained notes" vs. "shuts
    the noise down quicker" on "quick palm muted chords" behaviour described for adaptive
    gates in the research notes. A fixed Hold stage (20 ms) is added specifically to prevent
    the gate from re-triggering/chattering during a held palm-mute's natural amplitude
    ripple, a standard expander-design technique, not something claimed to be sourced from
    any specific reference unit.
- **Sidechain**: none in v0.2.0 (no external key input). *Reasoned*: the DI-sidechain
  technique described in research notes §7 is a mixing-workflow technique layered on top of
  a plugin, not a DSP requirement of the gate itself; adding a sidechain input is flagged as a
  backlog candidate, out of scope for v0.2.0's internal-signal-only gate.
- **Real-time safety**: envelope follower state allocated in `prepare()`, reset in `reset()`,
  matching every other stateful module in `TenebraeEngine`; gate gain applied as a smoothed
  multiplier (own `SmoothedValue`, short ramp ~2–5 ms to avoid zipper noise on gate
  open/close transitions) rather than a hard on/off multiply.

---

## 4. Catch2 test guarantees (additions for v0.2.0)

All existing v1 test categories (null test, NaN/Inf sweeps, state round-trip, latency
stability, bus-config coverage, long-run automation soak, per-clipper monotonicity/boundedness,
tone-stack frequency-response shift tests) remain and are unaffected by the new modules except
where noted. New/changed guarantees:

- **Presence: measurable high-shelf boost/cut.** Feed a high-frequency test tone (e.g. 6 kHz,
  above the new 2.4 kHz pivot) through the engine at Presence = +12 dB and Presence = -12 dB;
  assert the RMS energy at that frequency differs by a magnitude consistent with a shelf (same
  pattern as the existing `ToneStackTests.cpp` Treble test).
- **Presence: unity at 0 dB is inaudible.** At default Presence = 0 dB, output RMS for a
  broadband test signal must match (within float tolerance) the same signal processed with
  Presence entirely removed from the chain — i.e. the default must be a true passthrough, not
  an approximately-flat shelf (guards the "must not change existing preset tone" migration
  constraint in §6).
- **Treble corner regression test.** Assert the Treble band's -3 dB point sits at the new
  5 kHz corner (not the old 3.5 kHz), computed via the same frequency-response-sampling
  technique the existing `ToneStackTests.cpp` uses for Bass/Mid.
- **Gate: silence stays silent, transient passes.** A test signal that is silence, then a
  single sharp transient above threshold, then silence again: assert (a) the pre-transient
  silence produces sub-threshold/near-zero output, (b) the transient itself is not clipped or
  delayed beyond the attack time, (c) post-transient output decays to near-zero within
  hold+release, without an abrupt zero-crossing discontinuity (click) at the gate's
  close point — verified by checking no sample-to-sample delta at gate closure exceeds a
  fixed epsilon (proves the gain-smoothing ramp, not a hard mute, is what's closing it).
- **Gate: sustained signal above threshold is never gated.** A continuous sine at a level
  above threshold, held for longer than attack+hold+release combined, must show no gain
  reduction at any point after the attack ramp completes — proves the gate doesn't
  false-trigger/chatter on sustained content (the core failure mode a fixed-ballistics gate
  is prone to, per the adaptive-gate rationale in the research notes).
- **Gate: bypass produces bit-exact (or null-test-level) passthrough.** With Gate bypassed,
  output must null against the same engine configuration built before the gate existed
  (i.e., a null test against a "Gate always fully open" reference path), to < -90 dBFS
  residual — same bar the existing engine-level null test uses.
- **Gate + cascade interaction: NaN/Inf sweep extended.** Extend the existing
  `RobustnessTests.cpp` "every combination" test (`Voicing × Bright × Tone Voice`) to include
  `Gate on/off × Threshold at both range extremes`, at max Gain, confirming no NaN/Inf and a
  bounded, finite output — same pattern already used for the existing switches.
- **State round-trip**: extend `StateTests.cpp`'s existing "preserves non-default values of
  every parameter" test to cover Presence and all four new Gate parameters.
- **Parameter layout test**: extend `ParameterTests.cpp`'s "instantiates with the expected
  parameters" test to assert the new parameter count/IDs/ranges/defaults.

---

## 5. Honesty section

This brief is **research-derived, not hardware-measured**. No physical Rectifier, 5150/6505,
or licensed copy of any commercial amp-sim plugin was measured, profiled, or reverse-engineered
to produce these numbers. Every default in §3 is either:

- directly attributed to a specific published number found in a manual, technical write-up,
  forum measurement, or product description (marked "*sourced*" above, with the exact quote
  and URL in `tenebrae-research-notes.md`), or
- an engineering judgment made *in light of* that research, explicitly marked "*reasoned, not
  sourced*" above, with the reasoning stated inline.

Specific things this brief does **not** claim:

- It does not claim the new Gate replicates ISP Decimator's patented adaptive "Time Vector
  Processing" algorithm — Tenebrae's gate is a conventional fixed attack/hold/release
  expander, a deliberately simpler design, named as such.
- It does not claim the tone stack models any specific amplifier's TMB network — Tenebrae
  keeps its v1 topology of independent, non-interactive shelf/peak bands (a legitimate plugin
  simplification of a real interactive passive network), it does not add TMB-style
  pot-interaction between bands.
- Presence does not claim to model the specific negative-feedback circuit topology of any
  reference amp's Presence control — it borrows the *functional position and role*
  (post-distortion high-frequency shelf) and one *sourced pivot frequency* (2.4 kHz), not the
  circuit.
- The exact per-stage cascade constants (drive/asymmetry/interstage corners) inherited from
  v1 remain unsourced engineered numbers; the research surfaced no manufacturer-published
  data to correct them against, so v0.2.0 leaves them as-is rather than inventing false
  precision.
- The new Gate default of **on** (vs. the more conservative "ship new modules off by default"
  convention) is a deliberate choice, not an oversight: the research is unanimous that a gate
  is not an optional add-on for this genre but a structural expectation of "tight chug" tone
  (research notes §7). Shipping it off by default would silently reproduce the exact gap this
  brief exists to close. This is flagged here explicitly per the pay/compliance-style
  "explicit status" convention this project follows for any default that changes user-facing
  behavior non-trivially.

---

## 6. Versioning

- **Target**: v0.2.0.
- **Breaking parameter changes are allowed pre-1.0** (per project convention). This brief
  proposes: one new continuous parameter (Presence), four new Gate parameters
  (Threshold/Attack/Hold/Release) plus a Gate on/off toggle, and one changed fixed constant
  (Treble corner 3.5 kHz → 5 kHz — not a parameter range change, an internal filter-coefficient
  constant, so it does not break parameter IDs/ranges, only the *sound* of existing Treble
  settings changes slightly).
- **State migration**: tolerant import. Old (v0.1.0) state trees loaded into v0.2.0 must not
  fail or reset to defaults wholesale — missing Presence/Gate parameter IDs in an old state
  tree fall back to their v0.2.0 defaults (Presence 0 dB = unity/no-op, per the passthrough
  test in §4; Gate on with the stated defaults, per the honesty-section rationale above,
  accepting that an old session's exact silence/tail behavior may audibly change on reload —
  this is the one user-facing consequence of defaulting Gate to on, and should be called out
  in release notes, not silently absorbed).
- No existing parameter ID, range, or default among Tight/Gain/Bass/Mid/Voicing/Bright/Level/
  Mix/Tone Voice changes in this brief — only Treble's internal corner constant and the two
  new modules.

---

## 7. Factory Presets (for the M2 preset system)

Eight presets, each with intent and rough settings against the v0.2.0 parameter set (Tight Hz /
Bright / Gain dB / Voicing / Bass / Mid / Treble / Presence dB / Gate threshold dB / Level dB /
Mix %). Tone Voice defaults to Flat unless noted. All presets are engineered starting points,
not reference-amp emulations — no preset name below references any manufacturer or artist.

1. **Foundation Chug** — the plugin's own default voicing, unchanged from v1's defaults, as a
   neutral starting point. Tight 90 / Bright off / Gain 24 / Voicing Tight / Bass 0 / Mid 0 /
   Treble 0 / Presence 0 / Gate -48 / Level 0 / Mix 100.
2. **Low-Tuned Percussive** — tighter low end and a hotter gate for down-tuned rhythm work,
   where string noise/rumble is worst per the research (§7). Tight 130 / Bright off / Gain 26 /
   Voicing Tight / Bass -2 / Mid 2 / Treble 1 / Presence 2 / Gate -42, faster Release (~80 ms)
   / Level 0 / Mix 100.
3. **Vintage Cascade** — leans on the existing Loose voicing for a wider-band, less
   modern-tight character. Tight 70 / Bright off / Gain 20 / Voicing Loose / Bass 2 / Mid 0 /
   Treble -1 / Presence -2 / Gate -50 / Level 0 / Mix 100.
4. **Scooped Wall** — Tone Voice = Scoop, leaning into the "smiley curve" high-gain rhythm
   shape already documented in `ToneStack.cpp`'s tilt table, paired with a slightly hotter
   Presence since Scoop's own treble tilt is modest (+3 dB). Tight 100 / Bright off / Gain 28 /
   Voicing Tight / Bass 3 / Mid -3 / Treble 1 / Presence 3 / Gate -46 / Level -1 / Mix 100 /
   Tone Voice Scoop.
5. **Cut-Through Lead-Adjacent** — Tone Voice = Boost (mid-forward), Presence pulled back
   slightly to avoid stacking two upper-mid pushes (Boost's own +5 dB mid tilt plus a hot
   Presence would double up in the same region per the research's corner-overlap logic in
   §3.4). Tight 80 / Bright on / Gain 30 / Voicing Tight / Bass 0 / Mid 2 / Treble 2 /
   Presence 0 / Gate -48 / Level -1 / Mix 100 / Tone Voice Boost.
6. **Bright Aggressive** — Bright engaged pre-cascade (feeding more top end into the clipper
   itself, per `docs/architecture.md`'s existing rationale) paired with a pulled-back Treble/
   Presence post-cascade to avoid fizz, consistent with the research's "High Presence at high
   preamp gain is a common source of the 'fizzy digital sound'" warning (§3). Tight 110 /
   Bright on / Gain 32 / Voicing Tight / Bass -1 / Mid 1 / Treble -1 / Presence -1 / Gate -44 /
   Level -2 / Mix 100.
7. **Loose & Open** — the Loose voicing pushed further toward its own character: lower gain,
   wider tone-stack settings, gate mostly out of the way since Loose's own interstage
   filtering is already less aggressive. Tight 50 / Bright off / Gain 16 / Voicing Loose /
   Bass 3 / Mid 1 / Treble 2 / Presence 1 / Gate -58, longer Release (~300 ms) / Level 1 /
   Mix 100.
8. **Full Dry/Wet Blend** — a parallel-distortion starting point, demonstrating Mix as a
   creative control rather than always-100%-wet (the plugin's own documented default
   rationale for Mix=100% notwithstanding — this preset is the intentional counter-example).
   Tight 90 / Bright off / Gain 30 / Voicing Tight / Bass 0 / Mid 0 / Treble 1 / Presence 1 /
   Gate -48 / Level 2 / Mix 55.

---

## Appendix: what did NOT change and why

- Core clipper transfer function, per-stage cascade constants, 8x oversampling, Tight/Loose
  voicing tables, Mix/DryWetMixer latency-compensation architecture, Bass/Mid corner
  frequencies, Bright's fixed pre-cascade placement, and all real-time-safety patterns
  (`ScopedNoDenormals`, allocation-free after `prepare()`, `reset()` semantics) — all
  validated by the research as either directly consistent with the reference class or
  reasonable engineering simplifications with no sourced correction available. This brief is
  additive/corrective, not a rewrite.
