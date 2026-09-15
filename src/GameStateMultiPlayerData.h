#ifndef GAMESTATE_MULTIPLAYER_DATA_H
#define GAMESTATE_MULTIPLAYER_DATA_H

#include "GameConstantsAndTypes.h"
#include "PlayerNumber.h"

class PlayerState;

// Multiplayer-mode state carved out of GameState (backlog item 9,
// phase 1 cluster 6 -- split-god-object.md). Exposed under original
// names via reference members, so no call site outside
// GameState.h/.cpp changes. Deliberately excludes m_bSideIsJoined and
// m_pPlayerState (the core 2-player state) -- those are far more
// foundational and out of scope for this cluster.
class GameStateMultiPlayerData {
 public:
	MultiPlayerStatus m_MultiPlayerStatus[NUM_MultiPlayer];
	PlayerState *m_pMultiPlayerState[NUM_MultiPlayer];
	bool m_bMultiplayer;
	int m_iNumMultiplayerNoteFields;

	bool IsMultiPlayerEnabled(MultiPlayer mp) const {
		return m_MultiPlayerStatus[mp] == MultiPlayerStatus_Joined;
	}
};

#endif
