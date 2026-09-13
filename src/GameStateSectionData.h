#ifndef GAMESTATE_SECTION_DATA_H
#define GAMESTATE_SECTION_DATA_H

// MusicWheel expanded/last-open section state carved out of GameState
// (backlog item 9, phase 1 cluster 12 -- split-god-object.md). Exposed
// under their original names via reference members, so no call site
// outside GameState.h/.cpp changes. Pure data, no associated methods.
class GameStateSectionData
{
public:
	RString sExpandedSectionName;
	RString sLastOpenSection;
};

#endif
