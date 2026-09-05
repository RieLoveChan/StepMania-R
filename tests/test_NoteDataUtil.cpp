// Characterization tests for src/NoteDataUtil.cpp -- the NoteData
// transform helpers that operate purely on NoteData (and, where noted,
// a caller-supplied TimingData instance) without needing a live
// GAMESTATE.
//
// These pin the engine's CURRENT behaviour, bug-for-bug. If a refactor
// changes an outcome here, that is a signal to stop and decide whether
// the change is intended, not a licence to edit the expectation.
// See DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "NoteData.h"
#include "NoteDataUtil.h"
#include "TimingData.h"

#include "catch_amalgamated.hpp"

TEST_CASE("RemoveHoldNotes converts only Hold sub-type heads to taps, leaving Rolls alone", "[NoteDataUtil][hold]")
{
	NoteData nd;
	nd.SetNumTracks(2);
	nd.AddHoldNote(0, 0, 100, TAP_ORIGINAL_HOLD_HEAD);   // subType Hold
	TapNote roll = TAP_ORIGINAL_HOLD_HEAD;
	roll.subType = TapNoteSubType_Roll;
	nd.AddHoldNote(1, 0, 100, roll);

	NoteDataUtil::RemoveHoldNotes(nd, 0, MAX_NOTE_ROW);

	CHECK(nd.GetTapNote(0, 0).type == TapNoteType_Tap);   // hold -> tap
	CHECK(nd.GetTapNote(1, 0).type == TapNoteType_HoldHead); // roll untouched
	CHECK(nd.GetTapNote(1, 0).subType == TapNoteSubType_Roll);
}

TEST_CASE("ChangeRollsToHolds and ChangeHoldsToRolls swap only the matching sub-type", "[NoteDataUtil][hold]")
{
	NoteData nd;
	nd.SetNumTracks(1);
	nd.AddHoldNote(0, 0, 100, TAP_ORIGINAL_HOLD_HEAD); // starts as Hold

	NoteDataUtil::ChangeHoldsToRolls(nd, 0, MAX_NOTE_ROW);
	CHECK(nd.GetTapNote(0, 0).subType == TapNoteSubType_Roll);

	// Calling it again on an already-Roll note is a no-op (the function
	// only matches TapNoteSubType_Hold).
	NoteDataUtil::ChangeHoldsToRolls(nd, 0, MAX_NOTE_ROW);
	CHECK(nd.GetTapNote(0, 0).subType == TapNoteSubType_Roll);

	NoteDataUtil::ChangeRollsToHolds(nd, 0, MAX_NOTE_ROW);
	CHECK(nd.GetTapNote(0, 0).subType == TapNoteSubType_Hold);
}

TEST_CASE("RemoveJumps keeps only the last pressed track at a row, not the first", "[NoteDataUtil][simultaneous]")
{
	NoteData nd;
	nd.SetNumTracks(4);
	for (int t = 0; t < 4; ++t)
		nd.SetTapNote(t, 0, TAP_ORIGINAL_TAP);

	NoteDataUtil::RemoveJumps(nd); // RemoveSimultaneousNotes(nd, 1, ...)

	// The removal loop walks tracks in order and stops once enough have
	// been cleared to reach the max -- with max=1 out of 4 pressed, the
	// first three tracks (0,1,2) are cleared and the last one survives.
	CHECK(nd.GetTapNote(0, 0) == TAP_EMPTY);
	CHECK(nd.GetTapNote(1, 0) == TAP_EMPTY);
	CHECK(nd.GetTapNote(2, 0) == TAP_EMPTY);
	CHECK(nd.GetTapNote(3, 0).type == TapNoteType_Tap);
}

TEST_CASE("RemoveHands leaves held tracks alone but still limits taps/hold-heads at the row", "[NoteDataUtil][simultaneous]")
{
	NoteData nd;
	nd.SetNumTracks(4);
	nd.AddHoldNote(0, 0, 200, TAP_ORIGINAL_HOLD_HEAD); // holding through row 100
	nd.SetTapNote(1, 100, TAP_ORIGINAL_TAP);
	nd.SetTapNote(2, 100, TAP_ORIGINAL_TAP);
	nd.SetTapNote(3, 100, TAP_ORIGINAL_TAP);

	// At row 100: track 0's hold head is at row 0 (not row 100, so it
	// doesn't count toward GetNumTracksWithTapOrHoldHead(100)), but it
	// does count as "held" via GetTracksHeldAtRow. Total pressed = 3
	// taps + 1 held = 4; max 2 means 2 must go.
	NoteDataUtil::RemoveHands(nd); // RemoveSimultaneousNotes(nd, 2, ...)

	CHECK(nd.IsHoldNoteAtRow(0, 100)); // held tracks are never the ones removed
	CHECK(nd.GetTapNote(1, 100) == TAP_EMPTY);
	CHECK(nd.GetTapNote(2, 100) == TAP_EMPTY);
	CHECK(nd.GetTapNote(3, 100).type == TapNoteType_Tap); // last tap survives
}

TEST_CASE("RemoveMines and RemoveLifts only remove their own type", "[NoteDataUtil][remove-type]")
{
	NoteData nd;
	nd.SetNumTracks(3);
	nd.SetTapNote(0, 0, TAP_ORIGINAL_MINE);
	nd.SetTapNote(1, 0, TAP_ORIGINAL_LIFT);
	nd.SetTapNote(2, 0, TAP_ORIGINAL_TAP);

	NoteDataUtil::RemoveMines(nd);
	CHECK(nd.GetTapNote(0, 0) == TAP_EMPTY);
	CHECK(nd.GetTapNote(1, 0).type == TapNoteType_Lift); // untouched
	CHECK(nd.GetTapNote(2, 0).type == TapNoteType_Tap);  // untouched

	NoteDataUtil::RemoveLifts(nd);
	CHECK(nd.GetTapNote(1, 0) == TAP_EMPTY);
	CHECK(nd.GetTapNote(2, 0).type == TapNoteType_Tap); // still untouched
}

TEST_CASE("RemoveFakes removes explicit Fake taps and anything under a non-judgable timing region", "[NoteDataUtil][remove-type]")
{
	NoteData nd;
	nd.SetNumTracks(2);
	nd.SetTapNote(0, 0, TAP_ORIGINAL_FAKE);
	nd.SetTapNote(1, BeatToNoteRow(2.5f), TAP_ORIGINAL_TAP); // sits inside a fake timing region below

	TimingData timing;
	timing.SetBPMAtRow(0, 120);
	timing.AddSegment(FakeSegment(BeatToNoteRow(2.0f), 1.0f)); // beats [2, 3) are non-judgable

	NoteDataUtil::RemoveFakes(nd, timing, 0, MAX_NOTE_ROW);

	CHECK(nd.GetTapNote(0, 0) == TAP_EMPTY); // explicit Fake type, removed regardless of timing
	CHECK(nd.GetTapNote(1, BeatToNoteRow(2.5f)) == TAP_EMPTY); // ordinary tap, but under the fake region
}

TEST_CASE("RemoveAllButOneTap keeps the first tap track and clears the rest", "[NoteDataUtil][remove-type]")
{
	NoteData nd;
	nd.SetNumTracks(4);
	for (int t = 0; t < 4; ++t)
		nd.SetTapNote(t, 0, TAP_ORIGINAL_TAP);

	NoteDataUtil::RemoveAllButOneTap(nd, 0);

	CHECK(nd.GetTapNote(0, 0).type == TapNoteType_Tap); // the first one found is kept
	CHECK(nd.GetTapNote(1, 0) == TAP_EMPTY);
	CHECK(nd.GetTapNote(2, 0) == TAP_EMPTY);
	CHECK(nd.GetTapNote(3, 0) == TAP_EMPTY);
}

TEST_CASE("ShiftLeft and ShiftRight rotate track contents by one, wrapping around", "[NoteDataUtil][shift]")
{
	NoteData nd;
	nd.SetNumTracks(4);
	// Give each track a distinguishable note via iKeysoundIndex.
	for (int t = 0; t < 4; ++t)
	{
		TapNote tn = TAP_ORIGINAL_TAP;
		tn.iKeysoundIndex = t;
		nd.SetTapNote(t, 0, tn);
	}

	NoteDataUtil::ShiftLeft(nd);
	// New track i = old track (i+1) wrapped: [1,2,3,0]
	CHECK(nd.GetTapNote(0, 0).iKeysoundIndex == 1);
	CHECK(nd.GetTapNote(1, 0).iKeysoundIndex == 2);
	CHECK(nd.GetTapNote(2, 0).iKeysoundIndex == 3);
	CHECK(nd.GetTapNote(3, 0).iKeysoundIndex == 0);

	NoteDataUtil::ShiftRight(nd); // back to the original order
	CHECK(nd.GetTapNote(0, 0).iKeysoundIndex == 0);
	CHECK(nd.GetTapNote(1, 0).iKeysoundIndex == 1);
	CHECK(nd.GetTapNote(2, 0).iKeysoundIndex == 2);
	CHECK(nd.GetTapNote(3, 0).iKeysoundIndex == 3);
}

TEST_CASE("InsertRows and DeleteRows shift everything at or after the start row", "[NoteDataUtil][rows]")
{
	NoteData nd;
	nd.SetNumTracks(1);
	nd.SetTapNote(0, 50, TAP_ORIGINAL_TAP);   // before the insert point -- unaffected
	nd.SetTapNote(0, 150, TAP_ORIGINAL_MINE); // at/after the insert point -- shifted

	NoteDataUtil::InsertRows(nd, 100, 20);
	CHECK(nd.GetTapNote(0, 50).type == TapNoteType_Tap);  // untouched
	CHECK(nd.GetTapNote(0, 150) == TAP_EMPTY);            // moved away from its old spot
	CHECK(nd.GetTapNote(0, 170).type == TapNoteType_Mine); // shifted forward by 20

	NoteDataUtil::DeleteRows(nd, 100, 20);
	CHECK(nd.GetTapNote(0, 50).type == TapNoteType_Tap);   // still untouched
	CHECK(nd.GetTapNote(0, 150).type == TapNoteType_Mine); // shifted back to its original row
}

TEST_CASE("RemoveAllTapsOfType/RemoveAllTapsExceptForType filter by exact type", "[NoteDataUtil][remove-type]")
{
	NoteData nd;
	nd.SetNumTracks(1);
	nd.SetTapNote(0, 0, TAP_ORIGINAL_TAP);
	nd.SetTapNote(0, 100, TAP_ORIGINAL_MINE);
	nd.SetTapNote(0, 200, TAP_ORIGINAL_LIFT);

	NoteDataUtil::RemoveAllTapsOfType(nd, TapNoteType_Mine);
	CHECK(nd.GetTapNote(0, 0).type == TapNoteType_Tap);
	CHECK(nd.GetTapNote(0, 100) == TAP_EMPTY);
	CHECK(nd.GetTapNote(0, 200).type == TapNoteType_Lift);

	NoteDataUtil::RemoveAllTapsExceptForType(nd, TapNoteType_Lift);
	CHECK(nd.GetTapNote(0, 0) == TAP_EMPTY);
	CHECK(nd.GetTapNote(0, 200).type == TapNoteType_Lift);
}

TEST_CASE("GetMaxNonEmptyTrack finds the highest-indexed track with any note", "[NoteDataUtil][tracks]")
{
	NoteData nd;
	nd.SetNumTracks(4);
	CHECK(NoteDataUtil::GetMaxNonEmptyTrack(nd) == -1); // nothing at all

	nd.SetTapNote(1, 0, TAP_ORIGINAL_TAP);
	CHECK(NoteDataUtil::GetMaxNonEmptyTrack(nd) == 1);

	nd.SetTapNote(3, 500, TAP_ORIGINAL_TAP);
	CHECK(NoteDataUtil::GetMaxNonEmptyTrack(nd) == 3);
}

TEST_CASE("GetNextEditorPosition/GetPrevEditorPosition stop at hold tails as well as note starts", "[NoteDataUtil][editor]")
{
	NoteData nd;
	nd.SetNumTracks(2);
	nd.SetTapNote(0, 100, TAP_ORIGINAL_TAP);
	nd.AddHoldNote(1, 50, 300, TAP_ORIGINAL_HOLD_HEAD); // tail at row 300

	int row = 0;
	REQUIRE(NoteDataUtil::GetNextEditorPosition(nd, row));
	CHECK(row == 50); // the hold's head

	REQUIRE(NoteDataUtil::GetNextEditorPosition(nd, row));
	CHECK(row == 100); // the plain tap

	REQUIRE(NoteDataUtil::GetNextEditorPosition(nd, row));
	CHECK(row == 300); // the hold's tail, not just its head

	CHECK_FALSE(NoteDataUtil::GetNextEditorPosition(nd, row)); // nothing further
}
