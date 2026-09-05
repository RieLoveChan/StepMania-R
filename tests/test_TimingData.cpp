// Characterization tests for src/TimingData.cpp -- the beat<->row<->time
// conversion core, exercised through the "NoOffset" entry points that
// don't touch GAMESTATE/PREFSMAN (see the playbook's "pure-ish" rule).
//
// These pin the engine's CURRENT behaviour, bug-for-bug. If a refactor
// changes an outcome here, that is a signal to stop and decide whether
// the change is intended, not a licence to edit the expectation.
// See DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "TimingData.h"

#include "catch_amalgamated.hpp"

using Catch::Approx;

TEST_CASE("NoteRowToMeasureAndBeat divides rows by the current time signature", "[TimingData][measure]")
{
	TimingData timing;
	timing.SetTimeSignatureAtRow(0, 4, 4); // 192 rows/measure (48 rows/beat * 4)

	int measure, beat, remainder;

	timing.NoteRowToMeasureAndBeat(0, measure, beat, remainder);
	CHECK(measure == 0);
	CHECK(beat == 0);
	CHECK(remainder == 0);

	timing.NoteRowToMeasureAndBeat(191, measure, beat, remainder);
	CHECK(measure == 0);
	CHECK(beat == 0);
	CHECK(remainder == 191);

	timing.NoteRowToMeasureAndBeat(192, measure, beat, remainder);
	CHECK(measure == 1);
	CHECK(beat == 1);
	CHECK(remainder == 0);
}

TEST_CASE("NoteRowToMeasureAndBeat handles a mid-song time signature change", "[TimingData][measure]")
{
	TimingData timing;
	timing.SetTimeSignatureAtRow(0, 4, 4);   // 192 rows/measure, starting at row 0
	timing.SetTimeSignatureAtRow(384, 3, 4); // 144 rows/measure, starting at row 384 (after 2 measures of 4/4)

	int measure, beat, remainder;

	// Still inside the 4/4 region.
	timing.NoteRowToMeasureAndBeat(383, measure, beat, remainder);
	CHECK(measure == 1);

	// First row of the 3/4 region.
	timing.NoteRowToMeasureAndBeat(384, measure, beat, remainder);
	CHECK(measure == 2);
	CHECK(remainder == 0);
}

TEST_CASE("Has*Changes/Has* predicates reflect segment presence", "[TimingData][predicates]")
{
	TimingData timing;
	CHECK_FALSE(timing.HasBpmChanges()); // 0 or 1 BPM segment both count as "no changes"
	CHECK_FALSE(timing.HasStops());
	CHECK_FALSE(timing.HasWarps());
	CHECK_FALSE(timing.HasDelays());

	timing.SetBPMAtRow(0, 120);
	CHECK_FALSE(timing.HasBpmChanges()); // still just one segment

	timing.SetBPMAtRow(192, 150);
	CHECK(timing.HasBpmChanges());

	timing.SetStopAtRow(96, 0.5f);
	CHECK(timing.HasStops());

	timing.SetWarpAtRow(50, 1.0f);
	CHECK(timing.HasWarps());

	timing.SetDelayAtRow(10, 0.25f);
	CHECK(timing.HasDelays());
}

TEST_CASE("GetActualBPM reports the min/max across all BPM segments, clamped to highest", "[TimingData][bpm]")
{
	TimingData timing;
	timing.SetBPMAtRow(0, 100);
	timing.SetBPMAtRow(192, 200);
	timing.SetBPMAtRow(384, 150);

	float minBPM, maxBPM;
	timing.GetActualBPM(minBPM, maxBPM, 999.0f);
	CHECK(minBPM == Approx(100.0f));
	CHECK(maxBPM == Approx(200.0f));

	// "highest" clamps the max, but not the min.
	timing.GetActualBPM(minBPM, maxBPM, 120.0f);
	CHECK(minBPM == Approx(100.0f));
	CHECK(maxBPM == Approx(120.0f));
}

TEST_CASE("IsWarpAtRow is true only within [beat, beat+length) of a warp segment", "[TimingData][warp]")
{
	TimingData timing;
	timing.SetBPMAtRow(0, 120);
	timing.SetWarpAtRow(BeatToNoteRow(4.0f), 2.0f); // warp beats [4, 6)

	CHECK_FALSE(timing.IsWarpAtRow(BeatToNoteRow(3.0f)));
	CHECK(timing.IsWarpAtRow(BeatToNoteRow(4.0f)));
	CHECK(timing.IsWarpAtRow(BeatToNoteRow(5.0f)));
	CHECK_FALSE(timing.IsWarpAtRow(BeatToNoteRow(6.0f)));
}

TEST_CASE("GetBeatFromElapsedTimeNoOffset with a constant BPM is linear", "[TimingData][beat-time]")
{
	TimingData timing;
	timing.SetBPMAtRow(0, 120); // 2 beats/second

	CHECK(timing.GetBeatFromElapsedTimeNoOffset(0.0f) == Approx(0.0f));
	CHECK(timing.GetBeatFromElapsedTimeNoOffset(1.0f) == Approx(2.0f));
	CHECK(timing.GetBeatFromElapsedTimeNoOffset(2.0f) == Approx(4.0f));
}

TEST_CASE("GetElapsedTimeFromBeatNoOffset is the inverse of GetBeatFromElapsedTimeNoOffset for constant BPM", "[TimingData][beat-time]")
{
	TimingData timing;
	timing.SetBPMAtRow(0, 150); // 2.5 beats/second

	CHECK(timing.GetElapsedTimeFromBeatNoOffset(0.0f) == Approx(0.0f));
	CHECK(timing.GetElapsedTimeFromBeatNoOffset(5.0f) == Approx(2.0f));
	CHECK(timing.GetElapsedTimeFromBeatNoOffset(2.5f) == Approx(1.0f));
}

TEST_CASE("GetBeatFromElapsedTimeNoOffset accounts for a BPM change mid-song", "[TimingData][beat-time]")
{
	TimingData timing;
	timing.SetBPMAtRow(0, 120);              // 2 beats/second for beats [0, 4)
	timing.SetBPMAtRow(BeatToNoteRow(4.0f), 240); // 4 beats/second from beat 4 onward

	// First 4 beats take 2 seconds at 120 BPM.
	CHECK(timing.GetBeatFromElapsedTimeNoOffset(2.0f) == Approx(4.0f));

	// One more second at 240 BPM covers 4 more beats.
	CHECK(timing.GetBeatFromElapsedTimeNoOffset(3.0f) == Approx(8.0f));
}

TEST_CASE("GetBeatFromElapsedTimeNoOffset holds the beat still during a stop", "[TimingData][beat-time][stop]")
{
	TimingData timing;
	timing.SetBPMAtRow(0, 120); // 2 beats/second
	timing.SetStopAtRow(BeatToNoteRow(2.0f), 1.0f); // 1-second stop at beat 2

	// Reaching beat 2 takes 1 second at 120 BPM.
	CHECK(timing.GetBeatFromElapsedTimeNoOffset(1.0f) == Approx(2.0f));

	// Still mid-stop: the beat doesn't advance past 2 yet.
	CHECK(timing.GetBeatFromElapsedTimeNoOffset(1.5f) == Approx(2.0f));

	// Stop released after 1 more second (elapsed 2.0); beat resumes advancing.
	CHECK(timing.GetBeatFromElapsedTimeNoOffset(2.5f) == Approx(3.0f));
}
