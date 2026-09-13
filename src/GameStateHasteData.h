#ifndef GAMESTATE_HASTE_DATA_H
#define GAMESTATE_HASTE_DATA_H

// Haste (gameplay-speed-nudge) state carved out of GameState (backlog
// item 9, phase 1 cluster 5 -- split-god-object.md). Exposed under
// their original names via reference members, so no call site outside
// GameState.h/.cpp changes. Pure data, no associated methods --
// GAMESTATE->m_fHasteRate etc. are read/written directly from
// ScreenGameplay.cpp.
class GameStateHasteData
{
public:
	float	m_fHasteRate; // [-1,+1]; 0 = normal speed
	float	m_fLastHasteUpdateMusicSeconds;
	float	m_fAccumulatedHasteSeconds;
};

#endif
