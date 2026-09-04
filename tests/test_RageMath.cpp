// Characterization tests for src/RageMath.cpp — the pure vector/matrix/
// waveform helpers that don't need a live GAMESTATE or renderer.
//
// These pin the engine's CURRENT behaviour, bug-for-bug — including the
// RageSquare(0) hack (see comment at its call site) and the
// RageMatrixMultiply argument order (pOut = pB * pA, not pA * pB). If a
// refactor changes an outcome here, that is a signal to stop and decide
// whether the change is intended, not a licence to edit the expectation.
// See DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "RageMath.h"
#include "RageTypes.h"

#include "catch_amalgamated.hpp"

using Catch::Approx;

TEST_CASE("RageSquare returns a +-1 square wave, with angle 0 hacked to -1", "[RageMath][wave]")
{
	// Quirk pinned on purpose: angle 0 (and anything landing just under
	// the wrap point) gets nudged a full period forward before the
	// >=PI test, so it comes out -1 instead of the "expected" +1. See
	// the "Hack: ensures hold notes don't flicker" comment in the impl.
	CHECK(RageSquare(0.0f) == -1.0f);

	CHECK(RageSquare(1.0f) == 1.0f);           // < PI -> +1
	CHECK(RageSquare(4.0f) == -1.0f);          // >= PI -> -1
	CHECK(RageSquare(-1.0f) == -1.0f);         // wraps near 0 -> hacked to -1
	CHECK(RageSquare(2.0f * PI + 1.0f) == 1.0f); // periodic
}

TEST_CASE("RageTriangle returns a +-1 triangle wave", "[RageMath][wave]")
{
	CHECK(RageTriangle(0.0f) == Approx(0.0f));
	CHECK(RageTriangle(PI / 4.0f) == Approx(0.5f));
	CHECK(RageTriangle(PI) == Approx(0.0f));
	CHECK(RageTriangle(7.0f * PI / 4.0f) == Approx(-0.5f));
	CHECK(RageTriangle(-3.0f * PI / 4.0f) == Approx(-0.5f));
}

TEST_CASE("RageMatrixIdentity produces the 4x4 identity", "[RageMath][matrix]")
{
	RageMatrix m;
	RageMatrixIdentity(&m);

	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			CHECK(m.m[i][j] == (i == j ? 1.0f : 0.0f));
}

TEST_CASE("RageMatrixTranslation sets the translation row over an identity", "[RageMath][matrix]")
{
	RageMatrix m;
	RageMatrixTranslation(&m, 2.0f, 3.0f, 4.0f);

	CHECK(m.m[3][0] == 2.0f);
	CHECK(m.m[3][1] == 3.0f);
	CHECK(m.m[3][2] == 4.0f);
	CHECK(m.m[3][3] == 1.0f);
	CHECK(m.m[0][0] == 1.0f);
	CHECK(m.m[1][1] == 1.0f);
	CHECK(m.m[2][2] == 1.0f);
}

TEST_CASE("RageMatrixScaling sets the diagonal over an identity", "[RageMath][matrix]")
{
	RageMatrix m;
	RageMatrixScaling(&m, 2.0f, 3.0f, 4.0f);

	CHECK(m.m[0][0] == 2.0f);
	CHECK(m.m[1][1] == 3.0f);
	CHECK(m.m[2][2] == 4.0f);
	CHECK(m.m[3][3] == 1.0f);
	CHECK(m.m[3][0] == 0.0f);
}

TEST_CASE("RageMatrixMultiply computes pOut = pB * pA (arguments reversed)", "[RageMath][matrix]")
{
	// Quirk pinned on purpose: RageMatrixMultiply(pOut, pA, pB) computes
	// pB * pA, not pA * pB (see the comment above its declaration in
	// RageMath.h). translate(1,0,0) as pA, scale(2,2,2) as pB: the
	// scale's diagonal multiplies every row of the translation matrix,
	// so the translation row survives scaled by the *last* diagonal
	// entry (1, from scale's w), not by 2.
	RageMatrix t, s, out;
	RageMatrixTranslation(&t, 1.0f, 0.0f, 0.0f);
	RageMatrixScaling(&s, 2.0f, 2.0f, 2.0f);
	RageMatrixMultiply(&out, &t, &s);

	CHECK(out.m[0][0] == 2.0f);
	CHECK(out.m[1][1] == 2.0f);
	CHECK(out.m[2][2] == 2.0f);
	CHECK(out.m[3][3] == 1.0f);
	CHECK(out.m[3][0] == 1.0f);
	CHECK(out.m[3][1] == 0.0f);
	CHECK(out.m[3][2] == 0.0f);
}

TEST_CASE("RageMatrixTranspose swaps m[i][j] with m[j][i]", "[RageMath][matrix]")
{
	RageMatrix m(
		0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15);
	RageMatrix out;
	RageMatrixTranspose(&out, &m);

	CHECK(out.m[0][0] == 0.0f);
	CHECK(out.m[1][0] == 1.0f);
	CHECK(out.m[0][1] == 4.0f);
	CHECK(out.m[3][2] == 11.0f);
	CHECK(out.m[2][3] == 14.0f);
}

TEST_CASE("RageVec3Normalize scales a vector to unit length", "[RageMath][vector]")
{
	RageVector3 v(3.0f, 4.0f, 0.0f); // 3-4-5 triangle
	RageVector3 out;
	RageVec3Normalize(&out, &v);

	CHECK(out.x == Approx(0.6f));
	CHECK(out.y == Approx(0.8f));
	CHECK(out.z == Approx(0.0f));
}

TEST_CASE("RageVec3Cross computes the right-handed cross product", "[RageMath][vector]")
{
	RageVector3 x_axis(1.0f, 0.0f, 0.0f);
	RageVector3 y_axis(0.0f, 1.0f, 0.0f);
	RageVector3 out;
	RageVec3Cross(&out, &x_axis, &y_axis);

	CHECK(out.x == 0.0f);
	CHECK(out.y == 0.0f);
	CHECK(out.z == 1.0f); // i x j = k
}

TEST_CASE("RageVec3ClearBounds/AddToBounds track a min/max bounding box", "[RageMath][vector]")
{
	RageVector3 mins, maxs;
	RageVec3ClearBounds(mins, maxs);

	RageVec3AddToBounds(RageVector3(1.0f, -2.0f, 3.0f), mins, maxs);
	RageVec3AddToBounds(RageVector3(-5.0f, 10.0f, 0.0f), mins, maxs);

	CHECK(mins.x == -5.0f);
	CHECK(mins.y == -2.0f);
	CHECK(mins.z == 0.0f);
	CHECK(maxs.x == 1.0f);
	CHECK(maxs.y == 10.0f);
	CHECK(maxs.z == 3.0f);
}

TEST_CASE("RageQuadratic evaluates a Bezier set up as an ease curve", "[RageMath][bezier]")
{
	RageQuadratic q;
	q.SetFromBezier(0.0f, 0.0f, 1.0f, 1.0f); // flat handles -> zero slope at both ends

	CHECK(q.Evaluate(0.0f) == Approx(0.0f));
	CHECK(q.Evaluate(1.0f) == Approx(1.0f));
	CHECK(q.Evaluate(0.5f) == Approx(0.5f));

	CHECK(q.GetSlope(0.0f) == Approx(0.0f));
	CHECK(q.GetSlope(1.0f) == Approx(0.0f));
}
