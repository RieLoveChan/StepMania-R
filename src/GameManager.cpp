#include "global.h"
#include "StepMania.h"
#include "arch/Dialog/Dialog.h"
#include "GameManager.h"
#include "GameConstantsAndTypes.h"
#include "GameInput.h" // for GameButton constants
#include "GameLoop.h"  // for ChangeGame
#include "RageLog.h"
#include "RageUtil.h"
#include "NoteSkinManager.h"
#include "RageInputDevice.h"
#include "ThemeManager.h"
#include "LightsManager.h" // for NUM_CabinetLight
#include "Game.h"
#include "Style.h"
#include "GameDataIO.h"
#include "SpecialFiles.h"

#include <cstddef>
#include <vector>

GameManager *GAMEMAN = nullptr; // global and accessable from anywhere in our program

enum {
	TRACK_1 = 0,
	TRACK_2,
	TRACK_3,
	TRACK_4,
	TRACK_5,
	TRACK_6,
	TRACK_7,
	TRACK_8,
	TRACK_9,
	TRACK_10,
	TRACK_11,
	TRACK_12,
	TRACK_13,
	TRACK_14,
	TRACK_15,
	TRACK_16,
	// 16 tracks needed for beat-double7 and techno-double8
};

RString StepsTypeInfo::GetLocalizedString() const {
	if (THEME->HasString("StepsType", szName))
		return THEME->GetString("StepsType", szName);
	return szName;
}

static const StepsTypeInfo g_StepsTypeInfos[] = {
   // dance
   {"dance-single", 4, true, StepsTypeCategory_Single},
   {"dance-double", 8, true, StepsTypeCategory_Double},
   {"dance-couple", 8, true, StepsTypeCategory_Couple},
   {"dance-solo", 6, true, StepsTypeCategory_Single},
   {"dance-threepanel", 3, true, StepsTypeCategory_Single}, // thanks to kurisu
   {"dance-routine", 8, false, StepsTypeCategory_Routine},
   // pump
   {"pump-single", 5, true, StepsTypeCategory_Single},
   {"pump-halfdouble", 6, true, StepsTypeCategory_Double},
   {"pump-double", 10, true, StepsTypeCategory_Double},
   {"pump-couple", 10, true, StepsTypeCategory_Couple},
   // uh, dance-routine has that one bool as false... wtf? -aj
   {"pump-routine", 10, true, StepsTypeCategory_Routine},
   // kb7
   {"kb7-single", 7, true, StepsTypeCategory_Single},
   // { "kb7-small",		7,	true,	StepsTypeCategory_Single },
   // ez2dancer
   {"ez2-single", 5, true, StepsTypeCategory_Single},  // Single: TL,LHH,D,RHH,TR
   {"ez2-double", 10, true, StepsTypeCategory_Double}, // Double: Single x2
   {"ez2-real", 7, true, StepsTypeCategory_Single},    // Real: TL,LHH,LHL,D,RHL,RHH,TR
	// parapara paradise
   {"para-single", 5, true, StepsTypeCategory_Single},
   // ds3ddx
   {"ds3ddx-single", 8, true, StepsTypeCategory_Single},
   // beatmania
   {"bm-single5", 6, true, StepsTypeCategory_Single},  // called "bm" for backward compat
   {"bm-versus5", 6, true, StepsTypeCategory_Single},  // called "bm" for backward compat
   {"bm-double5", 12, true, StepsTypeCategory_Double}, // called "bm" for backward compat
   {"bm-single7", 8, true, StepsTypeCategory_Single},  // called "bm" for backward compat
   {"bm-versus7", 8, true, StepsTypeCategory_Single},  // called "bm" for backward compat
   {"bm-double7", 16, true, StepsTypeCategory_Double}, // called "bm" for backward compat
	// dance maniax
   {"maniax-single", 4, true, StepsTypeCategory_Single},
   {"maniax-double", 8, true, StepsTypeCategory_Double},
   // technomotion
   {"techno-single4", 4, true, StepsTypeCategory_Single},
   {"techno-single5", 5, true, StepsTypeCategory_Single},
   {"techno-single8", 8, true, StepsTypeCategory_Single},
   {"techno-double4", 8, true, StepsTypeCategory_Double},
   {"techno-double5", 10, true, StepsTypeCategory_Double},
   {"techno-double8", 16, true, StepsTypeCategory_Double},
   // pop'n music
   {"pnm-five", 5, true, StepsTypeCategory_Single}, // called "pnm" for backward compat
   {"pnm-nine", 9, true, StepsTypeCategory_Single}, // called "pnm" for backward compat
	// cabinet lights and other fine StepsTypes that don't exist lol
   {"lights-cabinet", NUM_CabinetLight, false, StepsTypeCategory_Single}, // XXX disable lights autogen for now
	// kickbox mania
   {"kickbox-human", 4, true, StepsTypeCategory_Single},
   {"kickbox-quadarm", 4, true, StepsTypeCategory_Single},
   {"kickbox-insect", 6, true, StepsTypeCategory_Single},
   {"kickbox-arachnid", 8, true, StepsTypeCategory_Single},
};
static_assert(ARRAYLEN(g_StepsTypeInfos) == NUM_StepsType, "ARRAYLEN(g_StepsTypeInfos) != NUM_StepsType");

/* StepMania-R enables every defined game type. A game only actually
 * appears in Select Game if it also has at least one NoteSkin
 * (GameManager::IsGameEnabled -> NoteSkinManager::DoNoteSkinsExistForGame),
 * so maniax/ez2/ds3ddx stay hidden until skins exist for them. */
// Populated by LoadGames() from Games/<name>/ on disk (GameDataIO.h, ADR
// 0008). The g_Game_*/g_Style_*/g_AutoKeyMappings_* struct literals that
// used to define this data are gone; the compile-time StepsType enum they
// looked into is still hand-written above (g_StepsTypeInfos) -- migrating
// that too, without breaking the on-disk #STEPSTYPE contract (AGENTS.md
// section 5), is the remainder of modernization-backlog.md item 20.
static GameDataStore g_GameDataStore;
static std::vector<const Game *> g_Games;

GameManager::GameManager() {
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring(L, "GAMEMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

void GameManager::LoadGames() {
	if (!g_Games.empty())
		return;

	// On-disk directory names under Games/. Order is preserved from the old
	// hardcoded g_Games[] array for behavioral parity (e.g. GetIndexFromGame/
	// GetGameFromIndex); nothing persists this order across runs.
	static const char *const asGameDirNames[] = {
	   "dance", "pump", "techno", "lights", "kb7", "ez2", "para", "ds3ddx", "beat", "maniax", "popn", "kickbox",
	};
	for (const char *szName : asGameDirNames) {
		const Game *pGame = LoadGameFromDisk(szName, SpecialFiles::GAMES_DIR, &g_GameDataStore);
		if (pGame != nullptr) {
			g_Games.push_back(pGame);
		} else {
			LOG_ERROR(
			   Log::General, "GameManager::LoadGames: couldn't load \"%s\" from %s", szName, SpecialFiles::GAMES_DIR.c_str()
			);
		}
	}
	ASSERT_M(!g_Games.empty(), "GameManager::LoadGames: no games could be loaded from " + SpecialFiles::GAMES_DIR);
}

GameManager::~GameManager() {
	// Unregister with Lua.
	LUA->UnsetGlobal("GAMEMAN");
}

void GameManager::GetStylesForGame(const Game *pGame, std::vector<const Style *> &aStylesAddTo, bool editor) {
	for (int s = 0; pGame->m_apStyles[s]; ++s) {
		const Style *style = pGame->m_apStyles[s];
		if (!editor && !style->m_bUsedForGameplay)
			continue;
		if (editor && !style->m_bUsedForEdit)
			continue;

		aStylesAddTo.push_back(style);
	}
}

const Game *GameManager::GetGameForStyle(const Style *pStyle) {
	for (std::size_t g = 0; g < g_Games.size(); ++g) {
		const Game *pGame = g_Games[g];
		for (int s = 0; pGame->m_apStyles[s]; ++s) {
			if (pGame->m_apStyles[s] == pStyle)
				return pGame;
		}
	}
	FAIL_M(pStyle->m_szName);
}

const Style *GameManager::GetEditorStyleForStepsType(StepsType st) {
	for (std::size_t g = 0; g < g_Games.size(); ++g) {
		const Game *pGame = g_Games[g];
		for (int s = 0; pGame->m_apStyles[s]; ++s) {
			const Style *style = pGame->m_apStyles[s];
			if (style->m_StepsType == st && style->m_bUsedForEdit)
				return style;
		}
	}

	ASSERT_M(0, ssprintf("The current game cannot use this Style with the editor!"));
	return nullptr;
}

void GameManager::GetStepsTypesForGame(const Game *pGame, std::vector<StepsType> &aStepsTypeAddTo) {
	for (int i = 0; pGame->m_apStyles[i]; ++i) {
		StepsType st = pGame->m_apStyles[i]->m_StepsType;
		ASSERT(st < NUM_StepsType);

		// Some Styles use the same StepsType (e.g. single and versus) so check
		// that we aren't doubling up.
		bool found = false;
		for (unsigned j = 0; j < aStepsTypeAddTo.size(); j++)
			if (st == aStepsTypeAddTo[j]) {
				found = true;
				break;
			}
		if (found)
			continue;

		aStepsTypeAddTo.push_back(st);
	}
}

void GameManager::GetDemonstrationStylesForGame(const Game *pGame, std::vector<const Style *> &vpStylesOut) {
	vpStylesOut.clear();

	for (int s = 0; pGame->m_apStyles[s]; ++s) {
		const Style *style = pGame->m_apStyles[s];
		if (style->m_bUsedForDemonstration)
			vpStylesOut.push_back(style);
	}

	ASSERT(!vpStylesOut.empty()); // this Game is missing a Style that can be used with the demonstration
}

const Style *GameManager::GetHowToPlayStyleForGame(const Game *pGame) {
	for (int s = 0; pGame->m_apStyles[s]; ++s) {
		const Style *style = pGame->m_apStyles[s];
		if (style->m_bUsedForHowToPlay)
			return style;
	}

	FAIL_M(ssprintf("Game has no Style that can be used with HowToPlay: %s", pGame->m_szName));
}

void GameManager::GetCompatibleStyles(const Game *pGame, int iNumPlayers, std::vector<const Style *> &vpStylesOut) {
	FOREACH_ENUM(StyleType, styleType) {
		int iNumPlayersRequired;
		switch (styleType) {
			DEFAULT_FAIL(styleType);
		case StyleType_OnePlayerOneSide:
		case StyleType_OnePlayerTwoSides:
			iNumPlayersRequired = 1;
			break;
		case StyleType_TwoPlayersTwoSides:
		case StyleType_TwoPlayersSharedSides:
			iNumPlayersRequired = 2;
			break;
		}

		if (iNumPlayers != iNumPlayersRequired)
			continue;

		for (int s = 0; pGame->m_apStyles[s]; ++s) {
			const Style *style = pGame->m_apStyles[s];
			if (style->m_StyleType != styleType)
				continue;
			if (!style->m_bUsedForGameplay)
				continue;

			vpStylesOut.push_back(style);
		}
	}
}

const Style *GameManager::GetFirstCompatibleStyle(const Game *pGame, int iNumPlayers, StepsType st) {
	std::vector<const Style *> vpStyles;
	GetCompatibleStyles(pGame, iNumPlayers, vpStyles);
	for (Style const *s : vpStyles) {
		if (s->m_StepsType == st) {
			return s;
		}
	}
	return nullptr;
}

void GameManager::GetEnabledGames(std::vector<const Game *> &aGamesOut) {
	for (std::size_t g = 0; g < g_Games.size(); ++g) {
		const Game *pGame = g_Games[g];
		if (IsGameEnabled(pGame))
			aGamesOut.push_back(pGame);
	}
}

const Game *GameManager::GetDefaultGame() {
	const Game *pDefault = nullptr;
	if (pDefault == nullptr) {
		for (std::size_t i = 0; pDefault == nullptr && i < g_Games.size(); ++i) {
			if (IsGameEnabled(g_Games[i]))
				pDefault = g_Games[i];
		}

		if (pDefault == nullptr)
			RageException::Throw("No NoteSkins found");
	}

	return pDefault;
}

int GameManager::GetIndexFromGame(const Game *pGame) {
	for (std::size_t g = 0; g < g_Games.size(); ++g) {
		if (g_Games[g] == pGame)
			return static_cast<int>(g);
	}
	FAIL_M(ssprintf("Game not found: %s", pGame->m_szName));
}

const Game *GameManager::GetGameFromIndex(int index) {
	ASSERT(index >= 0);
	ASSERT(index < (int)g_Games.size());
	return g_Games[index];
}

bool GameManager::IsGameEnabled(const Game *pGame) {
	return NOTESKIN->DoNoteSkinsExistForGame(pGame);
}

const StepsTypeInfo &GameManager::GetStepsTypeInfo(StepsType st) {
	ASSERT_M(st < NUM_StepsType, ssprintf("StepsType %d < NUM_StepsType (%d)", st, NUM_StepsType));
	return g_StepsTypeInfos[st];
}

StepsType GameManager::StringToStepsType(RString sStepsType) {
	sStepsType.MakeLower();

	for (int i = 0; i < NUM_StepsType; i++)
		if (g_StepsTypeInfos[i].szName == sStepsType)
			return StepsType(i);

	return StepsType_Invalid;
}

RString GameManager::StyleToLocalizedString(const Style *style) {
	RString s = style->m_szName;
	s = Capitalize(s);
	if (THEME->HasString("Style", s))
		return THEME->GetString("Style", s);
	else
		return s;
}

const Game *GameManager::StringToGame(RString sGame) {
	for (std::size_t i = 0; i < g_Games.size(); ++i)
		if (!sGame.CompareNoCase(g_Games[i]->m_szName))
			return g_Games[i];

	return nullptr;
}

const Style *GameManager::GameAndStringToStyle(const Game *game, RString sStyle) {
	for (int s = 0; game->m_apStyles[s]; ++s) {
		const Style *style = game->m_apStyles[s];
		if (sStyle.CompareNoCase(style->m_szName) == 0)
			return style;
	}

	return nullptr;
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the GameManager. */
class LunaGameManager : public Luna<GameManager> {
 public:
	static int StepsTypeToLocalizedString(T *p, lua_State *L) {
		lua_pushstring(L, p->GetStepsTypeInfo(Enum::Check<StepsType>(L, 1)).GetLocalizedString());
		return 1;
	}
	static int GetFirstStepsTypeForGame(T *p, lua_State *L) {
		Game *pGame = Luna<Game>::check(L, 1);

		std::vector<StepsType> vstAddTo;
		p->GetStepsTypesForGame(pGame, vstAddTo);
		ASSERT(!vstAddTo.empty());
		StepsType st = vstAddTo[0];
		LuaHelpers::Push(L, st);
		return 1;
	}
	static int IsGameEnabled(T *p, lua_State *L) {
		const Game *pGame = p->StringToGame(SArg(1));
		if (pGame)
			lua_pushboolean(L, p->IsGameEnabled(pGame));
		else
			lua_pushnil(L);

		return 1;
	}
	static int GetStylesForGame(T *p, lua_State *L) {
		RString game_name = SArg(1);
		const Game *pGame = p->StringToGame(game_name);
		if (!pGame) {
			luaL_error(L, "GetStylesForGame: Invalid Game: '%s'", game_name.c_str());
		}
		std::vector<Style *> aStyles;
		lua_createtable(L, 0, 0);
		for (int s = 0; pGame->m_apStyles[s]; ++s) {
			Style *pStyle = const_cast<Style *>(pGame->m_apStyles[s]);
			pStyle->PushSelf(L);
			lua_rawseti(L, -2, s + 1);
		}
		return 1;
	}
	static int GetEnabledGames(T *p, lua_State *L) {
		std::vector<const Game *> aGames;
		p->GetEnabledGames(aGames);
		lua_createtable(L, static_cast<int>(aGames.size()), 0);
		for (std::size_t i = 0; i < aGames.size(); ++i) {
			lua_pushstring(L, aGames[i]->m_szName);
			lua_rawseti(L, -2, static_cast<int>(i + 1));
		}
		return 1;
	}

	static int SetGame(T *p, lua_State *L) {
		RString game_name = SArg(1);
		const Game *pGame = p->StringToGame(game_name);
		if (!pGame) {
			luaL_error(L, "SetGame: Invalid Game: '%s'", game_name.c_str());
		}
		RString theme;
		if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
			theme = SArg(2);
			if (!THEME->IsThemeSelectable(theme)) {
				luaL_error(L, "SetGame: Invalid Theme: '%s'", theme.c_str());
			}
		}
		GameLoop::ChangeGame(game_name, theme);
		return 0;
	}

	LunaGameManager() {
		ADD_METHOD(StepsTypeToLocalizedString);
		ADD_METHOD(GetFirstStepsTypeForGame);
		ADD_METHOD(IsGameEnabled);
		ADD_METHOD(GetStylesForGame);
		ADD_METHOD(GetEnabledGames);
		ADD_METHOD(SetGame);
	};
};

LUA_REGISTER_CLASS(GameManager)
// lua end

/*
 * (c) 2001-2006 Chris Danford, Glenn Maynard
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
