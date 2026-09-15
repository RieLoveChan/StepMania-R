#ifndef GAMESTATE_CHARACTER_DATA_H
#define GAMESTATE_CHARACTER_DATA_H

#include "PlayerNumber.h"

class Character;

// "character stuff" carved out of GameState (backlog item 9, phase 1
// cluster 10 -- split-god-object.md). Exposed under its original name
// via a reference member, so no call site outside GameState.h/.cpp
// changes. Pure data, no associated methods.
class GameStateCharacterData {
 public:
	Character *m_pCurCharacters[NUM_PLAYERS];
};

#endif
