#ifndef GAMESTATE_STAGE_SEED_DATA_H
#define GAMESTATE_STAGE_SEED_DATA_H

// Per-game/round random seed state carved out of GameState (backlog
// item 9, phase 1 cluster 9 -- split-god-object.md). Exposed under
// their original names via reference members, so no call site
// outside GameState.h/.cpp changes.
class GameStateStageSeedData {
 public:
	// This is set to a random number per-game/round; it can be used for a random seed.
	int m_iGameSeed, m_iStageSeed;
	RString m_sStageGUID;

	void SetNewStageSeed() {
		m_iStageSeed = rand();
	}
};

#endif
