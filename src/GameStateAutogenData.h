#ifndef GAMESTATE_AUTOGEN_DATA_H
#define GAMESTATE_AUTOGEN_DATA_H

#include <cstddef>
#include <vector>

// Autogen-mod argument storage carved out of GameState (backlog item 9,
// phase 1 cluster 4 -- split-god-object.md). Exposed under its
// original name via a reference member, so no call site outside
// GameState.h/.cpp changes.
class GameStateAutogenData {
 public:
	// Autogen stuff.  This should probably be moved to its own singleton or
	// something when autogen is generalized and more customizable. -Kyz
	float GetAutoGenFarg(std::size_t i) {
		if (i >= m_autogen_fargs.size()) {
			return 0.0f;
		}
		return m_autogen_fargs[i];
	}
	std::vector<float> m_autogen_fargs;
};

#endif
