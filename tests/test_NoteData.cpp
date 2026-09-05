// Characterization tests for src/NoteData.cpp -- the tap/hold storage and
// row/track queries that don't need a live GAMESTATE.
//
// Several member functions on NoteData (IsTap/IsMine/IsLift/IsFake, and
// everything built on them: GetNumTapNotes, GetNumMines, GetNumHoldNotes,
// GetNumRowsWithSimultaneousTaps, etc.) call
// GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow() and are out of
// scope for this file per the playbook's "not for code that needs a live
// GAMESTATE" rule. GetNumTapNotesNoTiming() is the GAMESTATE-free
// counterpart and is covered here instead.
//
// These pin the engine's CURRENT behaviour, bug-for-bug. If a refactor
// changes an outcome here, that is a signal to stop and decide whether
// the change is intended, not a licence to edit the expectation.
// See DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "NoteData.h"

#include "catch_amalgamated.hpp"

TEST_CASE("A fresh NoteData is empty across every track", "[NoteData][basic]")
{
	NoteData nd;
	nd.SetNumTracks(4);

	CHECK(nd.GetNumTracks() == 4);
	CHECK(nd.IsEmpty());
	CHECK(nd.IsTrackEmpty(0));
	CHECK(nd.IsRowEmpty(0));
	CHECK(nd.GetTapNote(0, 0) == TAP_EMPTY);
	CHECK(nd.GetFirstRow() == 0); // no notes -> defined as row 0, not -1
	CHECK(nd.GetLastRow() == 0);
}

TEST_CASE("SetTapNote stores and retrieves a note; TAP_EMPTY erases it", "[NoteData][basic]")
{
	NoteData nd;
	nd.SetNumTracks(4);

	nd.SetTapNote(0, 100, TAP_ORIGINAL_TAP);
	CHECK(nd.GetTapNote(0, 100) == TAP_ORIGINAL_TAP);
	CHECK_FALSE(nd.IsRowEmpty(100));
	CHECK_FALSE(nd.IsEmpty());

	// Writing TAP_EMPTY over an existing note removes it from storage,
	// not just resets its type -- GetTapNote falls back to the shared
	// TAP_EMPTY constant either way.
	nd.SetTapNote(0, 100, TAP_EMPTY);
	CHECK(nd.GetTapNote(0, 100) == TAP_EMPTY);
	CHECK(nd.IsRowEmpty(100));
	CHECK(nd.IsEmpty());

	// A negative row is silently ignored.
	nd.SetTapNote(0, -1, TAP_ORIGINAL_TAP);
	CHECK(nd.GetTapNote(0, -1) == TAP_EMPTY);
}

TEST_CASE("GetFirstRow/GetLastRow span the earliest and latest notes across tracks", "[NoteData][rows]")
{
	NoteData nd;
	nd.SetNumTracks(4);

	nd.SetTapNote(2, 500, TAP_ORIGINAL_TAP);
	nd.SetTapNote(0, 100, TAP_ORIGINAL_TAP);
	nd.SetTapNote(3, 300, TAP_ORIGINAL_TAP);

	CHECK(nd.GetFirstRow() == 100);
	CHECK(nd.GetLastRow() == 500);
}

TEST_CASE("GetLastRow extends past a hold head by its duration", "[NoteData][rows][hold]")
{
	NoteData nd;
	nd.SetNumTracks(1);

	nd.AddHoldNote(0, 100, 200, TAP_ORIGINAL_HOLD_HEAD);
	// The hold head sits at row 100, but the hold body runs to row 200 --
	// GetLastRow must report the tail, not the head.
	CHECK(nd.GetLastRow() == 200);
}

TEST_CASE("GetTapFirstNonEmptyTrack/GetTapFirstEmptyTrack/GetTapLastEmptyTrack scan in the expected direction", "[NoteData][tracks]")
{
	NoteData nd;
	nd.SetNumTracks(4);
	nd.SetTapNote(1, 0, TAP_ORIGINAL_TAP);
	nd.SetTapNote(2, 0, TAP_ORIGINAL_TAP);

	int track;
	REQUIRE(nd.GetTapFirstNonEmptyTrack(0, track));
	CHECK(track == 1);

	REQUIRE(nd.GetTapFirstEmptyTrack(0, track));
	CHECK(track == 0);

	REQUIRE(nd.GetTapLastEmptyTrack(0, track));
	CHECK(track == 3);

	CHECK(nd.GetNumTapNonEmptyTracks(0) == 2);
}

TEST_CASE("GetFirstTrackWithTap treats Tap and Lift as taps but not HoldHead", "[NoteData][tracks]")
{
	NoteData nd;
	nd.SetNumTracks(3);
	nd.AddHoldNote(0, 0, 100, TAP_ORIGINAL_HOLD_HEAD);
	nd.SetTapNote(1, 0, TAP_ORIGINAL_LIFT);

	// Track 0 has a hold head at row 0, which doesn't count for
	// GetFirstTrackWithTap (only GetFirstTrackWithTapOrHoldHead).
	CHECK(nd.GetFirstTrackWithTap(0) == 1);
	CHECK(nd.GetFirstTrackWithTapOrHoldHead(0) == 0);
	CHECK(nd.GetLastTrackWithTapOrHoldHead(0) == 1);
}

TEST_CASE("AddHoldNote merges overlapping holds and clears taps underneath", "[NoteData][hold]")
{
	NoteData nd;
	nd.SetNumTracks(1);

	nd.SetTapNote(0, 50, TAP_ORIGINAL_TAP); // will be destroyed by the hold covering it
	nd.AddHoldNote(0, 0, 100, TAP_ORIGINAL_HOLD_HEAD);

	CHECK(nd.GetTapNote(0, 0).type == TapNoteType_HoldHead);
	CHECK(nd.GetTapNote(0, 0).iDuration == 100);
	CHECK(nd.GetTapNote(0, 50) == TAP_EMPTY); // the tap that was under the hold is gone
	CHECK(nd.GetTapNote(0, 100) == TAP_EMPTY); // AddHoldNote clears any note sitting at the tail

	// A second, overlapping hold merges into one continuous hold spanning both.
	nd.AddHoldNote(0, 80, 150, TAP_ORIGINAL_HOLD_HEAD);
	CHECK(nd.GetTapNote(0, 0).type == TapNoteType_HoldHead);
	CHECK(nd.GetTapNote(0, 0).iDuration == 150); // extended to cover the second hold's end
	CHECK(nd.GetTapNote(0, 80) == TAP_EMPTY); // the second hold's own head row was absorbed
}

TEST_CASE("IsHoldNoteAtRow is true inside a hold's body but false at its head or after its tail", "[NoteData][hold]")
{
	NoteData nd;
	nd.SetNumTracks(1);
	nd.AddHoldNote(0, 100, 200, TAP_ORIGINAL_HOLD_HEAD);

	CHECK_FALSE(nd.IsHoldNoteAtRow(0, 100)); // the head itself doesn't count (see the XXX comment: "IsHoldBodyAtRow")
	CHECK(nd.IsHoldNoteAtRow(0, 150));
	CHECK(nd.IsHoldNoteAtRow(0, 200));
	CHECK_FALSE(nd.IsHoldNoteAtRow(0, 201));

	int headRow = -1;
	CHECK(nd.IsHoldNoteAtRow(0, 150, &headRow));
	CHECK(headRow == 100);

	CHECK(nd.IsHoldHeadOrBodyAtRow(0, 100, nullptr)); // this variant does count the head
}

TEST_CASE("GetNextTapNoteRowForTrack/GetPrevTapNoteRowForTrack walk the sparse row map", "[NoteData][traversal]")
{
	NoteData nd;
	nd.SetNumTracks(1);
	nd.SetTapNote(0, 100, TAP_ORIGINAL_TAP);
	nd.SetTapNote(0, 300, TAP_ORIGINAL_TAP);

	int row = -1;
	REQUIRE(nd.GetNextTapNoteRowForTrack(0, row));
	CHECK(row == 100);
	REQUIRE(nd.GetNextTapNoteRowForTrack(0, row));
	CHECK(row == 300);
	CHECK_FALSE(nd.GetNextTapNoteRowForTrack(0, row)); // no more notes

	row = 1000;
	REQUIRE(nd.GetPrevTapNoteRowForTrack(0, row));
	CHECK(row == 300);
	REQUIRE(nd.GetPrevTapNoteRowForTrack(0, row));
	CHECK(row == 100);
	CHECK_FALSE(nd.GetPrevTapNoteRowForTrack(0, row)); // no more notes
}

TEST_CASE("ClearRangeForTrack truncates a hold that overlaps the boundary", "[NoteData][clear]")
{
	NoteData nd;
	nd.SetNumTracks(1);
	nd.AddHoldNote(0, 0, 200, TAP_ORIGINAL_HOLD_HEAD);

	nd.ClearRangeForTrack(100, MAX_NOTE_ROW, 0);

	// The hold is truncated to end where the cleared range begins.
	CHECK(nd.GetTapNote(0, 0).type == TapNoteType_HoldHead);
	CHECK(nd.GetTapNote(0, 0).iDuration == 100);
	CHECK(nd.GetTapNote(0, 150) == TAP_EMPTY);
}

TEST_CASE("GetNumTapNotesNoTiming counts every non-empty tap regardless of judgability", "[NoteData][count]")
{
	NoteData nd;
	nd.SetNumTracks(4);
	nd.SetTapNote(0, 0, TAP_ORIGINAL_TAP);
	nd.SetTapNote(1, 0, TAP_ORIGINAL_MINE); // counted here (unlike GetNumTapNotes, which excludes mines)
	nd.AddHoldNote(2, 100, 200, TAP_ORIGINAL_HOLD_HEAD);

	CHECK(nd.GetNumTapNotesNoTiming() == 3);
	CHECK(nd.GetNumTapNotesNoTiming(0, 50) == 2); // range excludes the hold head at row 100
}

TEST_CASE("RowNeedsAtLeastSimultaneousPresses counts holds toward the threshold", "[NoteData][hands]")
{
	NoteData nd;
	nd.SetNumTracks(3);
	nd.SetTapNote(0, 100, TAP_ORIGINAL_TAP);

	// A single tap alone doesn't reach a 3-simultaneous-presses threshold.
	CHECK_FALSE(nd.RowNeedsAtLeastSimultaneousPresses(3, 100));

	// Two other tracks holding through row 100 bring the total to 3, even
	// though neither has a tap note stored exactly at row 100 -- holds
	// are only counted once the direct-note count falls short.
	nd.AddHoldNote(1, 0, 200, TAP_ORIGINAL_HOLD_HEAD);
	nd.AddHoldNote(2, 0, 200, TAP_ORIGINAL_HOLD_HEAD);
	CHECK(nd.RowNeedsAtLeastSimultaneousPresses(3, 100));
	CHECK(nd.RowNeedsHands(100));
}
