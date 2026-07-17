# Tenebrae Deep-Dive Research Notes

Reference class: high-gain cascaded rhythm-guitar distortion (metal "chug" tone), hardware
amps + modern amp-sim plugins. Sources below are quoted verbatim where possible; all URLs
fetched July 2026.

## Note on the missing structural template

The task pointed at
`/private/tmp/.../scratchpad/miserere-design-brief-v2.md` as the structural template to
mirror. That file does not exist in this scratchpad (confirmed via `find`) — only a
`~/Downloads/miserere-v0.1.0-macos.zip`/unzipped build exists, which is a shipped binary, not
a brief. The brief below follows the structure given in the task prompt directly
(why-v1-falls-short → topology → module specs with sourced defaults → test guarantees →
honesty section → versioning) since the named template could not be read.

---

## 1. Reference units for this category

- **Mesa/Boogie Dual/Triple Rectifier** — the canonical "modern high-gain cascade" amp;
  three/four cascaded 12AX7 gain stages, FMV (bass/mid/treble) passive tone stack, separate
  Presence and (on some channels) Depth/Resonance controls.
- **Peavey 5150 / EVH 5150III / 6505(+)** — the metal-genre-defining cascade amp (Van Halen →
  EVH → thrash/metalcore lineage); famous for staying "tight" (percussive, undistorted low
  end) at extreme gain, via its Resonance (power-amp low-frequency feedback) and Presence
  (high-frequency feedback) controls rather than a simple preamp EQ knob.
- **Cascaded-tube-preamp design lore** (Aiken Amps, Mojotone, Premier Guitar "All the World's
  a Gain Stage") — the general engineering principle of *why* multiple cascaded gain stages
  sound different from one stage driven harder, and the interstage-network techniques used to
  tame each stage.
- **Modern high-gain amp-sim plugins** (Neural DSP Archetype: Gojira X, Archetype: Nolly) —
  the current commercial software reference for "tight, mix-ready, chug-focused" metal tone;
  notably these ship a **built-in noise gate** as a first-class, expected module, not an
  afterthought.
- **ISP Technologies Decimator** — industry-standard hardware/plugin noise gate for high-gain
  rigs, relevant because Tenebrae's cascade has no gate at all in v1 and gating is treated by
  the whole genre as inseparable from "tight chug" tone.

---

## 2. Tone-stack corner frequencies & interaction (TMB passive stacks)

Tenebrae's `ToneStack` uses independent, non-interacting shelf/peak bands (Bass low-shelf @
150 Hz, Mid peak @ 650 Hz Q 1.1, Treble high-shelf @ 3.5 kHz), each with its own +/-15 dB gain
knob. Real passive TMB (treble-mid-bass) tone stacks, which is what every amp in the reference
class actually uses, behave very differently — the three controls are **interactive**, not
independent parametric bands:

> "The Mid Pot's resistance below its wiper is added to the treble high pass filter's
> resistance" — bass/mid pot settings affect each other's response characteristics.
> — [How the TMB Tone Stack Works, robrobinette.com](https://robrobinette.com/How_The_TMB_Tone_Stack_Works.htm)

> Treble: "blocks frequencies below 2313 Hz (with the Mid Pot at max) to 2548 Hz (with the Mid
> Pot at minimum)" — turning the Treble pot doesn't move this corner, it "adjusts the balance
> between high frequencies on the top side of the Treble Pot and Bass & Mid frequencies on the
> bottom side."
> Bass: "With the Bass Pot at max gives a cutoff frequency of 1.6 Hz" / "at minimum gives a
> cutoff frequency of 64 Hz."
> Mid: cutoff frequency runs "from 64 Hz with the Mid Pot at max to infinitely high with the
> Mid Pot full down" (i.e. fully attenuating).
> — same source

Mesa Rectifier tone-stack measurements (a *modified* TMB, closer to what a high-gain amp
actually ships):

> "Stages 1, 2, and 4 of the dirty channels are shunting the lows at about 88 Hz, -3 dB."
> Mid control's biggest range of change: "between about 300 Hz to 1.5 kHz," with "the greatest
> amount of control and focus in the Recto's Orange Mode tone stack" at 578 Hz–1.6 kHz.
> — [Presence and Voicing of the Mesa Boogie Dual Rectifier, warpedmusician](https://warpedmusician.wordpress.com/2015/02/07/understanding-the-mesa-boogie-dual-rectifier-3-channel-amplifier-presence-and-voicing/)

> Channel 3 Modern mode moves "the frequency from which the treble swings on axis from
> 1.27 kHz down to 936 Hz" (vs Channel 2), and Presence pivots "at 2.4 kHz" on Ch.3 Modern vs
> "1.6 kHz" on Ch.2 — "makes it 3 dB louder at 2.4 kHz under most circumstances."
> At noon settings, "The Treble is attempting to affect frequencies down beneath 100 Hz, but
> ... the Mid and Bass controls prevent that from happening any further down than around
> 350 Hz."
> — [Dual Rectifier Channel 3 Modern Tone Stack, warpedmusician](https://warpedmusician.wordpress.com/2016/01/07/dual-rectifier-channel-3-modern-tone-stack/)

General TMB corner-frequency ballpark cited across sources: **Bass ≈ 100 Hz, Mid ≈ 800 Hz,
Treble ≈ 9–10 kHz** (Fender/Marshall lineage), though exact values are knob-position-dependent
because the network is interactive, not fixed. — [Guitar Tone Stack Calculator, CMUSE](https://www.cmuse.org/guitar-tone-stack-calculator)

**Gap vs v1:** Tenebrae's independent shelf/peak topology is *conceptually* the right
simplification for a plugin (a fully interactive passive TMB emulation is its own project),
but v1's fixed Mid corner (650 Hz) sits inside the reference class's 300 Hz–1.6 kHz "focus"
zone reasonably, while the fixed Treble corner (3.5 kHz) is lower than the reference class's
Presence pivot (1.6–2.4 kHz observed on Rectifier, but that is a *post*-EQ global negative
feedback control, functionally different from a Treble knob) and lower than the TMB Treble
corner ballpark (9–10 kHz). v1 has **no Presence control at all** — every reference amp in
this class has one, and it is explicitly called out (by Fader & Knob and by the Rectifier
analysis) as one of the two or three controls that most defines whether a high-gain tone
reads as "tight/defined" vs "fizzy."

---

## 3. "Tight" control — what it actually does in the reference class

Tenebrae's Tight parameter is a single pre-cascade HPF, 20–300 Hz, default 90 Hz. In the
reference class, "tightness" is achieved by **two separate mechanisms that both matter**, not
one:

> "The Resonance control is a low-frequency feedback filter in the power amplifier section
> ... Resonance is the 'tight/boom' control — less low end = a tighter tone, more low end =
> kick you in the chest thud, but too much makes your tone mushy." Recommended starting point
> for tight tone: **"Low (0–3): Tighter, more focused low end; reduces the 'flub' on palm
> mutes; modern metal starting point."**
> "The Presence control is a high-frequency feedback filter... High Presence at high preamp
> gain is a common source of the 'fizzy digital sound.'"
> Modern Metal reference setting: "Preamp gain 6, Bass 5, Mid 4–5, Treble 6, Resonance 3–4,
> Presence 6–7."
> — [Peavey 5150/6505 Settings Guide, Fader & Knob](https://faderandknob.com/blog/peavey-5150-settings-guide)

Note these are **negative-feedback filters around the power amp**, not a simple series HPF —
functionally and audibly different from a pre-cascade HPF (they interact with the whole
signal's dynamics, not just its static spectrum). Tenebrae's engineered "Tight" HPF is a
reasonable simplification of the *effect* (removing low end so palm mutes stay percussive)
but the reference class additionally uses this as a *program-dependent, gain-interacting*
control, and — critically — pairs it with the **Resonance/Presence pair**, not a single knob.

Patent-literature framing of the same idea, independent of any one manufacturer:

> "When the gain of a distortion amplifier stage is increased, the design should include a
> steep low frequency roll off at a higher frequency" — i.e., tightening scales *with* gain,
> it isn't a fixed, independent knob in well-designed high-gain circuits.
> — cited via general patent literature (see search notes; specific patent language
> paraphrased, not directly quotable)

**Gap vs v1:** Tight (HPF only) + no coupling to Gain, and no Presence-style high-frequency
shelf that's actually *automatable* (Bright is a fixed switch, not a control) is thinner than
the two-knob (Resonance + Presence) convention the whole reference class uses.

---

## 4. Cascaded gain stages — why multiple stages ≠ one stage driven harder

> "Feeding the signal into a triode from one half of a conventional preamp tube..., then from
> there to another triode, and possibly another, and even another" with attenuation networks
> between stages "to control gain levels... to ramp up the signal level to ultra-high levels"
> rather than depleting it through fixed networks. Cascading gain "separates modern amps from
> vintage: their arrival delineates the effort to generate overdrive within the amp itself and
> to rein it in at usable volume levels."
> One technique: "a resistor coupled from some point in the signal ... to tap part of it to
> ground, as a means of reining in the signal strength to a fixed level at that point to avoid
> overwhelming the next stage, which could result in harsh, fizzy distortion."
> — [Cascading Gain Stages, Mojotone](https://mojotone.com/blogs/news/cascading-gain-stages-the-engine-behind-modern-amp-overdrive)

This directly validates Tenebrae's existing architecture decision (progressively tighter
interstage HP/LP per cascade stage, documented in `docs/architecture.md`) — it is the *right*
idea. What the reference-class literature adds that v1's fixed, non-interactive per-stage
tables don't capture: real cascaded circuits generally **couple interstage attenuation to the
overall gain/drive setting** (taming a stage more as the whole amp is driven harder), whereas
Tenebrae's per-stage drive/asymmetry/filter corners are fixed constants regardless of the
user's Gain parameter — the cascade's *voicing* doesn't track how hard the user is driving it,
only the input level to the (fixed) cascade does.

---

## 5. Clipping symmetry, harmonic content

> Asymmetrical clipping "creates uneven waveforms rich in even-order harmonics," achieved via
> "different diode types, adding an extra diode on one side, or using diodes with different
> forward voltages, causing the audio signal to be clipped at different levels in the positive
> and negative swings."
> Tube overdrive "tends to exhibit softer, asymmetric clipping, even order harmonic
> distortion... Tubes tend to distort in an asymmetrical manner relative to the Q or operating
> point... promoting even ordered harmonics."
> Single-ended output-stage clipping "produces distinct second and third harmonics... rich in
> even-order harmonics that the ear perceives as warmer and more musical."
> — search synthesis, corroborated across multiple forum/technical sources (geofex.com
> `distn101.htm`, "A Musical Distortion Primer," could not be re-fetched due to a connection
> reset on refetch — treat as secondary corroboration only, not a direct quote source for the
> brief)

This directly validates Tenebrae's `AsymmetricClipper` (`y = tanh(x + a) - tanh(a)`) design
rationale as documented in `AsymmetricClipper.h` — the shifted-tanh approach is a legitimate,
literature-consistent way to generate asymmetric/even-harmonic content. **No correction needed
here**; this is a case where v1's engineering already matches the reference class's stated
mechanism. The gap is in the *cascade-wide progression* of asymmetry values (0.15 → 0.25 →
0.35 for Tight, 0.10 → 0.18 → 0.25 for Loose) — these are engineered, round-number-adjacent
constants with no sourced justification; the literature doesn't give exact target values (no
manufacturer publishes them), so this is an area for the "honesty section" rather than a
correction.

---

## 6. Oversampling requirement vs aliasing

> "Waveshaping applied to a sampled signal generates harmonics that exceed the Nyquist
> frequency and cause aliasing distortion... traditionally tackled by oversampling."
> Research on oversampling for nonlinear waveshaping evaluates schemes "at eight-times
> oversampling," and shows aliasing reduction "greater than that achieved by two times
> oversampling" using antialiasing-filter methods — implying 2x oversampling alone is
> generally considered insufficient for high-fidelity multi-stage distortion.
> "Using a modest amount of oversampling in conjunction with 2nd-order ADAA (Antiderivative
> Antialiasing) can help immensely" — i.e., the current DSP-research state of the art
> increasingly favors combining oversampling with antiderivative antialiasing rather than
> relying on oversampling alone for high factors.
> — [Oversampling for Nonlinear Waveshaping, ResearchGate](https://www.researchgate.net/publication/333688079_Oversampling_for_Nonlinear_Waveshaping_Choosing_the_Right_Filters),
> [Introduction to Oversampling for Alias Reduction](https://www.nickwritesablog.com/introduction-to-oversampling-for-alias-reduction/)

**Validates v1's choice of 8x oversampling** for a 3-stage cascade (reasonable, matches
literature's evaluated factor for multi-stage nonlinearities). Flags a *future* direction (not
in scope for v0.2.0 given the "no GUI/marketing, DSP-only" brief boundary and cost/complexity)
that ADAA could reduce the oversampling factor needed, i.e. CPU cost, without a tone
regression — worth a backlog note, not a v0.2.0 mandate.

---

## 7. Noise gate — the single biggest structural gap

> "Metal means high gain, and high gain means noise – hiss, hum, amp buzz. A noise gate slams
> the door on this unwanted racket the moment you stop playing." "Modern metal relies on
> hyper-percussive, staccato riffs, and noise gates create that sharp on/off, 'choppy'
> character, ensuring every note hits with maximum impact and every silence is truly
> silent — key for that almost inhumanly tight sound." Low-tuned guitars in particular
> "generate string noise, sympathetic resonance, and low-end rumble that can turn your mix
> into mud. Gates help clean this up dramatically."
> — [Noise Gate For Metal Guitars, Nail The Mix](https://www.nailthemix.com/noise-gate-for-metal-guitars)

> "Built-in Noise Gates are essential for tight, chuggy, percussive riffs on low-tuned
> guitars"; Neural DSP's Archetype: Gojira X ships one as a core module, not an add-on.
> — [Archetype: Gojira X, Neural DSP product page + Nail The Mix coverage](https://neuraldsp.com/plugins/archetype-gojira)

> ISP Decimator's design philosophy: "The only control is the threshold sensitivity... making
> for a very user-friendly interface," using an "adaptive tracking algorithm" (Time Vector
> Processing) so that "quick palm muted chords" get shut down fast while "long sustained
> notes" get "a smooth decay" rather than an abrupt, musically wrong cutoff — i.e. the
> ballistics are **program-dependent** (envelope-adaptive), not a fixed attack/release pair.
> — search synthesis of ISP Decimator product/technical descriptions

**This is the single most consequential gap.** Every reference amp-sim in the modern
high-gain-metal category (Neural DSP Archetype line, and hardware-adjacent pedal gates like
ISP Decimator/Solar Chug) treats a gate as inseparable from "tight chug" tone, specifically
because a cascaded multi-stage distortion (which is what Tenebrae *is*) amplifies noise floor,
string buzz, and hum by 3 stages' worth of gain before the tone stack even sees it. Tenebrae
v1 has **no gate anywhere in the signal chain** (confirmed: `TenebraeEngine.h`/`.cpp` — no
gate/expander class exists; signal flow is Tight → Bright → Gain → cascade → tone stack →
Level → Mix). For a high-gain rhythm-guitar plugin marketed on "chug," this is a structural
omission, not a voicing nuance.

---

## 8. Gap analysis summary (v1 vs reference class)

| Area | v1 (Tenebrae) | Reference class | Gap |
|---|---|---|---|
| Tightening | Single pre-cascade HPF (Tight, 20–300 Hz) | Power-amp Resonance (low-freq feedback) **+** Presence (high-freq feedback), gain-coupled | v1 conflates "remove mud" with "tighten," and has no user-facing Presence-style high-frequency control at all (Bright is a fixed pre-cascade switch, not this) |
| Cascade voicing | Fixed per-stage drive/asymmetry/HP-LP constants, unsourced round numbers, two complete voicing presets (Tight/Loose) | Interstage attenuation generally couples to overall drive/gain; cascade design is well-documented as a *principle* but manufacturers don't publish exact per-stage numbers | v1's architecture is directionally correct (validated), but per-stage numbers are arbitrary-looking and don't scale with the user's Gain knob |
| Tone stack | 3 independent, non-interacting shelf/peak bands, fixed corners (150/650/3500 Hz) | Interactive TMB network; reference corners cluster around 100 Hz / 300–800 Hz / 1.6–10 kHz depending on amp and mode, **plus a separate Presence control** | Treble corner (3.5 kHz) is low relative to reference Presence pivots (1.6–2.4 kHz) and TMB treble corners (9–10 kHz); independent-band topology is an acceptable plugin simplification but the total control count (3 bands, no presence) is thinner than the reference class's 4–5 control convention (B/M/T + Resonance + Presence) |
| Noise gate | **None** | Universal, first-class module in every reference plugin/pedal for this exact genre | Structural omission — highest-priority v2 addition |
| Clipping topology | Shifted-tanh asymmetric clipper, cascade of 3 | Matches literature's description of asymmetric/single-ended clipping mechanism | Validated, no change needed to the core nonlinearity |
| Oversampling | 8x, half-band IIR | Literature evaluates 8x as a reasonable factor for multi-stage nonlinearities; ADAA is the emerging complement, not yet mandatory | Validated for v0.2.0 scope; ADAA is a backlog item, not urgent |
| Program-dependence | None (all cascade constants and gate — absent — are static) | Reference-class gates and Resonance/Presence controls are explicitly envelope/gain-adaptive (ISP Decimator's Time Vector Processing, patent-literature's gain-coupled roll-off) | v2 gate ballistics should be envelope-adaptive in spirit (fast on transients, slow release on sustain) even if a full adaptive algorithm is out of scope |

---

## Sources (deduplicated)

1. Presence and Voicing of the Mesa Boogie Dual Rectifier 3 Channel Amplifier — https://warpedmusician.wordpress.com/2015/02/07/understanding-the-mesa-boogie-dual-rectifier-3-channel-amplifier-presence-and-voicing/
2. Dual Rectifier Channel 3 Modern Tone Stack — https://warpedmusician.wordpress.com/2016/01/07/dual-rectifier-channel-3-modern-tone-stack/
3. How the TMB Tone Stack Works — https://robrobinette.com/How_The_TMB_Tone_Stack_Works.htm
4. Guitar Tone Stack Calculator — https://www.cmuse.org/guitar-tone-stack-calculator
5. Peavey 5150 / EVH 6505 Settings Guide — https://faderandknob.com/blog/peavey-5150-settings-guide
6. Cascading Gain Stages: The Engine Behind Modern Amp Overdrive — https://mojotone.com/blogs/news/cascading-gain-stages-the-engine-behind-modern-amp-overdrive
7. Oversampling for Nonlinear Waveshaping: Choosing the Right Filters (ResearchGate) — https://www.researchgate.net/publication/333688079_Oversampling_for_Nonlinear_Waveshaping_Choosing_the_Right_Filters
8. Introduction to Oversampling for Alias Reduction — https://www.nickwritesablog.com/introduction-to-oversampling-for-alias-reduction/
9. Noise Gate For Metal Guitars: The Key to Ultra-Tight, Aggressive Tones — https://www.nailthemix.com/noise-gate-for-metal-guitars
10. Archetype: Gojira X (Neural DSP) — https://neuraldsp.com/plugins/archetype-gojira
11. ISP Decimator technical/product descriptions (search synthesis; no single canonical primary-source URL captured — treat threshold-only/adaptive-tracking claims as secondary sourcing)
12. A Musical Distortion Primer (geofex.com) — http://www.geofex.com/effxfaq/distn101.htm — **fetch failed twice (ECONNRESET); cited only via corroborating secondary sources, not directly quoted**

Local repo sources (v1 ground truth, not web):
- `/Users/yves/Development/Audio/tenebrae/README.md`
- `/Users/yves/Development/Audio/tenebrae/docs/architecture.md`
- `/Users/yves/Development/Audio/tenebrae/src/params/ParameterLayout.cpp`
- `/Users/yves/Development/Audio/tenebrae/src/dsp/AsymmetricClipper.h`
- `/Users/yves/Development/Audio/tenebrae/src/dsp/CascadeStage.h`
- `/Users/yves/Development/Audio/tenebrae/src/dsp/ToneStack.h` / `.cpp`
- `/Users/yves/Development/Audio/tenebrae/tests/*.cpp` (test inventory)
