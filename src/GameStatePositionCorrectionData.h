#ifndef GAMESTATE_POSITION_CORRECTION_DATA_H
#define GAMESTATE_POSITION_CORRECTION_DATA_H

#include "RageTimer.h"

// "Timing position corrections" state carved out of GameState
// (backlog item 9, phase 1 cluster 11 -- split-god-object.md).
// Exposed under their original names via reference members, so no
// call site outside GameState.h/.cpp changes. Already private to
// GameState, so this has zero external exposure to begin with. Pure
// data, no associated methods.
class GameStatePositionCorrectionData {
 public:
	RageTimer m_LastPositionTimer;
	float m_LastPositionSeconds;
	bool m_paused;
};

#endif
