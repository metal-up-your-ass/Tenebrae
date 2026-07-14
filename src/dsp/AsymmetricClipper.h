#pragma once

#include <cmath>

// A single-ended, asymmetric tanh soft clipper - the nonlinearity used at
// every stage of Tenebrae's waveshaper cascade. Pure, allocation-free, and
// stateless, so it is unit-testable in complete isolation from the rest of
// the signal chain (see tests/AsymmetricClipperTests.cpp) and safe to call
// per-sample from the audio thread.
//
// Transfer function: y = tanh(x + a) - tanh(a)
//
// Shifting the tanh curve by a fixed bias `a` before re-centring it at
// y(0) == 0 gives two effects that compound nicely across a cascade:
//   - The positive and negative half-cycles saturate towards different
//     asymptotic ceilings (1 - tanh(a) vs -(1 + tanh(a))), i.e. genuine
//     asymmetric clipping, emulating single-ended (op-amp/transistor)
//     clipping stages rather than a symmetric fuzz.
//   - Because the curve is not globally linear, a zero-mean AC input
//     produces a small even-harmonic-rich, DC-shifted output. Cascading
//     several stages with progressively larger asymmetry (see
//     CascadeStage) thickens this even-harmonic content stage by stage,
//     which is a large part of what makes a multi-stage "amp cascade"
//     sound denser and heavier than a single clipper driven harder - the
//     "chug" tone Tenebrae targets.
// Subtracting tanh(a) guarantees processSample(0, a) == 0 for any bias, so
// the clipper never injects a constant DC offset into silence.
namespace AsymmetricClipper
{
    inline float processSample (float x, float asymmetry) noexcept
    {
        return std::tanh (x + asymmetry) - std::tanh (asymmetry);
    }
}
