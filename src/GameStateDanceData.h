#ifndef GAMESTATE_DANCE_DATA_H
#define GAMESTATE_DANCE_DATA_H

#include "RageTimer.h"

// "used by themes that support heart rate entry" state carved out of
// GameState (backlog item 9, phase 1 cluster 7 -- split-god-object.md).
// Exposed under their original names via reference members, so no
// call site outside GameState.h/.cpp changes. Pure data, no
// associated methods -- GAMESTATE->m_DanceStartTime/m_DanceDuration
// are read/written directly from ScreenGameplay.cpp.
class GameStateDanceData {
 public:
	RageTimer m_DanceStartTime;
	float m_DanceDuration;
};

#endif
