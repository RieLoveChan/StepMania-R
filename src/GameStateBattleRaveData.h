#ifndef GAMESTATE_BATTLE_RAVE_DATA_H
#define GAMESTATE_BATTLE_RAVE_DATA_H

// PLAY_MODE_BATTLE/PLAY_MODE_RAVE state carved out of GameState
// (backlog item 9, phase 1 cluster 8 -- split-god-object.md). Exposed
// under their original names via reference members, so no call site
// outside GameState.h/.cpp changes. Pure data, no associated methods.
class GameStateBattleRaveData
{
public:
	float m_fOpponentHealthPercent; // used in PLAY_MODE_BATTLE
	float m_fTugLifePercentP1; // used in PLAY_MODE_RAVE
};

#endif
