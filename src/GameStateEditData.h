#ifndef GAMESTATE_EDIT_DATA_H
#define GAMESTATE_EDIT_DATA_H

#include "GameConstantsAndTypes.h"
#include "Difficulty.h"
#include "MessageManager.h"

class Steps;
class Profile;

// Editor-only state carved out of GameState (backlog item 9, phase 1 --
// split-god-object.md). GameState still exposes these under their
// original names via reference members, so no call site outside
// GameState.cpp/.h changes in this phase.
class GameStateEditData {
 public:
	GameStateEditData();

	bool m_bIsUsingStepTiming;
	bool m_bInStepEditor;
	BroadcastOnChange<StepsType> m_stEdit;
	BroadcastOnChange<CourseDifficulty> m_cdEdit;
	BroadcastOnChangePtr<Steps> m_pEditSourceSteps;
	BroadcastOnChange<StepsType> m_stEditSource;
	BroadcastOnChange<int> m_iEditCourseEntryIndex;
	BroadcastOnChange<RString> m_sEditLocalProfileID;

	Profile *GetEditLocalProfile();
};

#endif

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard, Chris Gomez
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
