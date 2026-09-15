#ifndef GAMESTATE_ATTRACT_DATA_H
#define GAMESTATE_ATTRACT_DATA_H

// Attract-screen state carved out of GameState (backlog item 9, phase 1
// cluster 3 -- split-god-object.md). GameState exposes these under
// their original names via reference members, so no call site outside
// GameState.h/.cpp changes.
class GameStateAttractData {
 public:
	GameStateAttractData();

	// negative means play attract sounds regardless of
	// PREFSMAN->m_AttractSoundFrequency
	int m_iNumTimesThroughAttract;

	bool IsTimeToPlayAttractSounds() const;
	void VisitAttractScreen(const RString sScreenName);
};

#endif
