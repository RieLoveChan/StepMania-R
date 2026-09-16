/** @brief GameDataIO - Export/import Game/Style/InputScheme/AutoMappings to and from the data-driven Games/ tree. */

#ifndef GAME_DATA_IO_H
#define GAME_DATA_IO_H

#include "Game.h"
#include "Style.h"
#include "InputMapper.h" // for AutoMappings

#include <deque>
#include <vector>

/**
 * @brief Owns all runtime-loaded Game/Style/AutoMappings/style-array storage
 * for the data-driven Games/ tree (ADR 0008 stage 1).
 *
 * Objects live here for the whole program's lifetime once loaded.
 * std::deque never invalidates references/pointers to existing elements on
 * push_back (unlike std::vector, which may reallocate), so pointers handed
 * out of this store -- Game::m_apStyles entries, InputScheme::m_pAutoMappings,
 * and the Game* returned by LoadGameFromDisk itself -- stay valid for as long
 * as the GameDataStore that owns them is alive. Keep exactly one, owned by
 * GameManager, for the life of the program. */
struct GameDataStore {
	std::deque<Game> games;
	std::deque<Style> styles;
	std::deque<AutoMappings> autoMappings;
	std::deque<std::vector<const Style *>> styleArrays;
};

/**
 * @brief Export one already-constructed Game (and every Style/AutoMappings
 * it references) to Games/<name>/... under the given base directory.
 *
 * A one-shot migration tool: reads the compiled g_Game_ and g_Style_
 * literals at runtime and serializes them to disk. Not used at normal
 * runtime once Games/ is populated and the hand-written literals are
 * removed. */
void ExportGameToDisk(const Game *pGame, const RString &sBaseDir);

/**
 * @brief Load one game's data from Games/<name>/... under the given base
 * directory, allocating its Game/Style/AutoMappings objects into pStore
 * (which must outlive the returned pointer).
 * @return the loaded Game, or nullptr if Games/<name>/ doesn't exist or a
 * required file/field inside it is missing. */
const Game *LoadGameFromDisk(const RString &sName, const RString &sBaseDir, GameDataStore *pStore);

#endif

/**
 * @file
 * @section LICENSE
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
