#ifndef GAMESTATE_WORKOUT_DATA_H
#define GAMESTATE_WORKOUT_DATA_H

#include "PlayerNumber.h"

// Workout-goal state carved out of GameState (backlog item 9, phase 1
// cluster 2 -- split-god-object.md). GameState exposes these under
// their original names via reference members, so no call site outside
// GameState.h/.cpp changes.
class GameStateWorkoutData {
 public:
	GameStateWorkoutData();

	bool m_bGoalComplete[NUM_PLAYERS];
	bool m_bWorkoutGoalComplete;

	float GetGoalPercentComplete(PlayerNumber pn);
	bool IsGoalComplete(PlayerNumber pn) {
		return GetGoalPercentComplete(pn) >= 1;
	}
};

#endif
