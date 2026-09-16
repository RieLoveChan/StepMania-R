#include "global.h"
#include "GameDataIO.h"
#include "Game.h"
#include "Style.h"
#include "InputMapper.h"
#include "GameConstantsAndTypes.h"
#include "GameManager.h"
#include "IniFile.h"
#include "RageLog.h"
#include "RageUtil.h"

#include <cstring>

namespace {
// Heap-allocate a copy of s that lives for the rest of the process (never
// freed) -- matches the lifetime of the string literals this replaces
// (`static const char *m_szName = "dance";` etc, which also never get freed).
// Avoids strdup(), which MSVC flags as deprecated (C4996) under -Werror.
const char *DupString(const RString &s) {
	char *p = new char[s.size() + 1];
	std::memcpy(p, s.c_str(), s.size() + 1);
	return p;
}

RString GameButtonTypeToString(GameButtonType t) {
	return (t == GameButtonType_Menu) ? "Menu" : "Step";
}
GameButtonType StringToGameButtonType(const RString &s) {
	return s.EqualsNoCase("Menu") ? GameButtonType_Menu : GameButtonType_Step;
}

// InputScheme::GameButtonInfo::m_SecondaryMenuButton and
// AutoMappingEntry::m_gb are always resolved through the owning game's own
// InputScheme (GameButtonToString/StringToGameButton both take one), so the
// scheme must exist before either field can round-trip.
RString SafeGameButtonToString(const InputScheme *pScheme, GameButton gb) {
	if (gb == GameButton_Invalid)
		return "Invalid";
	return GameButtonToString(pScheme, gb);
}
GameButton SafeStringToGameButton(const InputScheme *pScheme, const RString &s) {
	if (s.EqualsNoCase("Invalid") || s.empty())
		return GameButton_Invalid;
	return StringToGameButton(pScheme, s);
}
} // namespace

void ExportGameToDisk(const Game *pGame, const RString &sBaseDir) {
	const RString sGameDir = sBaseDir + pGame->m_szName + "/";

	// Count styles (m_apStyles is nullptr-terminated) and collect their names
	// up front, in original order -- the load side needs the ordered list to
	// reconstruct the Style array and (elsewhere) preserve UI display order.
	std::vector<const Style *> vpStyles;
	for (const Style *const *ppStyle = pGame->m_apStyles; *ppStyle != nullptr; ++ppStyle)
		vpStyles.push_back(*ppStyle);

	IniFile game_ini;
	game_ini.SetValue("Game", "CountNotesSeparately", pGame->m_bCountNotesSeparately);
	game_ini.SetValue("Game", "TickHolds", pGame->m_bTickHolds);
	game_ini.SetValue("Game", "PlayersHaveSeparateStyles", pGame->m_PlayersHaveSeparateStyles);
	game_ini.SetValue("Game", "MapW1To", TapNoteScoreToString(pGame->m_mapW1To));
	game_ini.SetValue("Game", "MapW2To", TapNoteScoreToString(pGame->m_mapW2To));
	game_ini.SetValue("Game", "MapW3To", TapNoteScoreToString(pGame->m_mapW3To));
	game_ini.SetValue("Game", "MapW4To", TapNoteScoreToString(pGame->m_mapW4To));
	game_ini.SetValue("Game", "MapW5To", TapNoteScoreToString(pGame->m_mapW5To));

	const InputScheme *pScheme = &pGame->m_InputScheme;
	game_ini.SetValue("Game", "ButtonsPerController", pScheme->m_iButtonsPerController);

	for (int i = 0; i < pScheme->m_iButtonsPerController; ++i) {
		const GameButton gb = static_cast<GameButton>(GAME_BUTTON_NEXT + i);
		const InputScheme::GameButtonInfo *pSchemeInfo = pScheme->GetGameButtonInfo(gb);
		const Game::PerButtonInfo *pGameInfo = pGame->GetPerButtonInfo(gb);

		game_ini.SetValue("Game", ssprintf("Button%dName", i), RString(pSchemeInfo->m_szName));
		game_ini.SetValue(
		   "Game", ssprintf("Button%dSecondaryMenu", i), SafeGameButtonToString(pScheme, pSchemeInfo->m_SecondaryMenuButton)
		);
		game_ini.SetValue("Game", ssprintf("Button%dType", i), GameButtonTypeToString(pGameInfo->m_gbt));
	}

	game_ini.SetValue("Game", "NumStyles", static_cast<int>(vpStyles.size()));
	for (std::size_t i = 0; i < vpStyles.size(); ++i)
		game_ini.SetValue("Game", ssprintf("Style%d", static_cast<int>(i)), RString(vpStyles[i]->m_szName));

	game_ini.WriteFile(sGameDir + "game.ini");

	// AutoMappings: this game's own keyboard-default mapping (InputScheme::
	// m_pAutoMappings). NOT the physical-hardware auto-detect table
	// (InputMapper.cpp's own g_AutoMappings[]/Data/AutoMappings/*.ini) --
	// that's a separate subsystem, out of scope for this stage.
	if (pScheme->m_pAutoMappings != nullptr) {
		const AutoMappings *pMap = pScheme->m_pAutoMappings;
		IniFile map_ini;
		map_ini.SetValue("AutoMappings", "Game", pMap->m_sGame);
		map_ini.SetValue("AutoMappings", "DriverRegex", pMap->m_sDriverRegex);
		map_ini.SetValue("AutoMappings", "ControllerName", pMap->m_sControllerName);
		map_ini.SetValue("AutoMappings", "NumEntries", static_cast<int>(pMap->m_vMaps.size()));
		for (std::size_t i = 0; i < pMap->m_vMaps.size(); ++i) {
			const AutoMappingEntry &e = pMap->m_vMaps[i];
			const RString sEntry = ssprintf(
			   "%d,%s,%s,%d",
			   e.m_iSlotIndex,
			   DeviceButtonToString(e.m_deviceButton).c_str(),
			   SafeGameButtonToString(pScheme, e.m_gb).c_str(),
			   e.m_bSecondController ? 1 : 0
			);
			map_ini.SetValue("AutoMappings", ssprintf("Entry%d", static_cast<int>(i)), sEntry);
		}
		map_ini.WriteFile(sGameDir + "autokeymap.ini");
	}

	// One ini per style, under styles/.
	for (const Style *pStyle : vpStyles) {
		IniFile style_ini;
		style_ini.SetValue("Style", "UsedForGameplay", pStyle->m_bUsedForGameplay);
		style_ini.SetValue("Style", "UsedForEdit", pStyle->m_bUsedForEdit);
		style_ini.SetValue("Style", "UsedForDemonstration", pStyle->m_bUsedForDemonstration);
		style_ini.SetValue("Style", "UsedForHowToPlay", pStyle->m_bUsedForHowToPlay);
		style_ini.SetValue("Style", "StepsType", GAMEMAN->GetStepsTypeInfo(pStyle->m_StepsType).szName);
		style_ini.SetValue("Style", "StyleType", StyleTypeToString(pStyle->m_StyleType));
		style_ini.SetValue("Style", "ColsPerPlayer", pStyle->m_iColsPerPlayer);
		style_ini.SetValue("Style", "CanUseBeginnerHelper", pStyle->m_bCanUseBeginnerHelper);
		style_ini.SetValue("Style", "LockDifficulties", pStyle->m_bLockDifficulties);

		// Full fixed-size arrays, every slot, verbatim -- not just the
		// "used" prefix -- so nothing is silently lost or reinterpreted.
		for (int p = 0; p < NUM_PLAYERS; ++p) {
			for (int c = 0; c < MAX_COLS_PER_PLAYER; ++c) {
				const Style::ColumnInfo &ci = pStyle->m_ColumnInfo[p][c];
				const RString sPrefix = ssprintf("P%dCol%d", p, c);
				style_ini.SetValue("Style", sPrefix + "Track", ci.track);
				style_ini.SetValue("Style", sPrefix + "XOffset", ci.fXOffset);
				style_ini.SetValue("Style", sPrefix + "Name", RString(ci.pzName != nullptr ? ci.pzName : ""));
			}
		}
		for (int c = 0; c < NUM_GameController; ++c) {
			for (int b = 0; b < NUM_GameButton; ++b)
				style_ini.SetValue("Style", ssprintf("Controller%dInput%d", c, b), pStyle->m_iInputColumn[c][b]);
		}
		for (int c = 0; c < MAX_COLS_PER_PLAYER; ++c)
			style_ini.SetValue("Style", ssprintf("DrawOrder%d", c), pStyle->m_iColumnDrawOrder[c]);

		style_ini.WriteFile(sGameDir + "styles/" + pStyle->m_szName + ".ini");
	}
}

const Game *LoadGameFromDisk(const RString &sName, const RString &sBaseDir, GameDataStore *pStore) {
	const RString sGameDir = sBaseDir + sName + "/";

	IniFile game_ini;
	if (!game_ini.ReadFile(sGameDir + "game.ini")) {
		LOG_TRACE(Log::General, "Games/%s/game.ini not found or unreadable: %s", sName.c_str(), game_ini.GetError().c_str());
		return nullptr;
	}

	pStore->games.emplace_back();
	Game &game = pStore->games.back();
	std::memset(&game, 0, sizeof(Game));

	// m_szName must outlive the Game -- store it as a heap string with the
	// lifetime of the process (never freed), same lifetime guarantee the old
	// `static const char*` literals had.
	game.m_szName = DupString(sName);

	game_ini.GetValue("Game", "CountNotesSeparately", game.m_bCountNotesSeparately);
	game_ini.GetValue("Game", "TickHolds", game.m_bTickHolds);
	game_ini.GetValue("Game", "PlayersHaveSeparateStyles", game.m_PlayersHaveSeparateStyles);

	RString sMap;
	game_ini.GetValue("Game", "MapW1To", sMap);
	game.m_mapW1To = StringToTapNoteScore(sMap);
	game_ini.GetValue("Game", "MapW2To", sMap);
	game.m_mapW2To = StringToTapNoteScore(sMap);
	game_ini.GetValue("Game", "MapW3To", sMap);
	game.m_mapW3To = StringToTapNoteScore(sMap);
	game_ini.GetValue("Game", "MapW4To", sMap);
	game.m_mapW4To = StringToTapNoteScore(sMap);
	game_ini.GetValue("Game", "MapW5To", sMap);
	game.m_mapW5To = StringToTapNoteScore(sMap);

	InputScheme &scheme = game.m_InputScheme;
	scheme.m_szName = game.m_szName;
	game_ini.GetValue("Game", "ButtonsPerController", scheme.m_iButtonsPerController);

	for (int i = 0; i < scheme.m_iButtonsPerController; ++i) {
		// InputScheme::GetGameButtonInfo/Game::GetPerButtonInfo both index
		// their arrays as [gb - GAME_BUTTON_NEXT], i.e. plain [i] here --
		// the first GAME_BUTTON_NEXT slots are the universal buttons
		// (common tables in InputMapper.cpp/Game.cpp), not part of this data.
		RString sButtonName;
		game_ini.GetValue("Game", ssprintf("Button%dName", i), sButtonName);
		scheme.m_GameButtonInfo[i].m_szName = DupString(sButtonName);

		RString sSecondary;
		game_ini.GetValue("Game", ssprintf("Button%dSecondaryMenu", i), sSecondary);
		scheme.m_GameButtonInfo[i].m_SecondaryMenuButton = SafeStringToGameButton(&scheme, sSecondary);

		RString sType;
		game_ini.GetValue("Game", ssprintf("Button%dType", i), sType);
		game.m_PerButtonInfo[i].m_gbt = StringToGameButtonType(sType);
	}

	// AutoMappings (this game's own keyboard-default mapping).
	IniFile map_ini;
	if (map_ini.ReadFile(sGameDir + "autokeymap.ini")) {
		// AutoMappings' constructor requires 3 RString args (no default) --
		// pass placeholders now, the real values are set field-by-field below.
		pStore->autoMappings.emplace_back("", "", "");
		AutoMappings &autoMap = pStore->autoMappings.back();
		map_ini.GetValue("AutoMappings", "Game", autoMap.m_sGame);
		map_ini.GetValue("AutoMappings", "DriverRegex", autoMap.m_sDriverRegex);
		map_ini.GetValue("AutoMappings", "ControllerName", autoMap.m_sControllerName);
		int iNumEntries = 0;
		map_ini.GetValue("AutoMappings", "NumEntries", iNumEntries);
		for (int i = 0; i < iNumEntries; ++i) {
			RString sEntry;
			map_ini.GetValue("AutoMappings", ssprintf("Entry%d", i), sEntry);
			std::vector<RString> parts;
			split(sEntry, ",", parts, false);
			if (parts.size() != 4)
				continue;
			AutoMappingEntry entry;
			entry.m_iSlotIndex = StringToInt(parts[0]);
			entry.m_deviceButton = StringToDeviceButton(parts[1]);
			entry.m_gb = SafeStringToGameButton(&scheme, parts[2]);
			entry.m_bSecondController = StringToInt(parts[3]) != 0;
			autoMap.m_vMaps.push_back(entry);
		}
		scheme.m_pAutoMappings = &autoMap;
	} else {
		scheme.m_pAutoMappings = nullptr;
	}

	// Styles.
	int iNumStyles = 0;
	game_ini.GetValue("Game", "NumStyles", iNumStyles);

	pStore->styleArrays.emplace_back();
	std::vector<const Style *> &vpStyles = pStore->styleArrays.back();

	for (int s = 0; s < iNumStyles; ++s) {
		RString sStyleName;
		game_ini.GetValue("Game", ssprintf("Style%d", s), sStyleName);

		IniFile style_ini;
		if (!style_ini.ReadFile(sGameDir + "styles/" + sStyleName + ".ini")) {
			LOG_TRACE(
			   Log::General,
			   "Games/%s/styles/%s.ini not found or unreadable: %s",
			   sName.c_str(),
			   sStyleName.c_str(),
			   style_ini.GetError().c_str()
			);
			continue;
		}

		pStore->styles.emplace_back();
		Style &style = pStore->styles.back();
		std::memset(&style, 0, sizeof(Style));

		style_ini.GetValue("Style", "UsedForGameplay", style.m_bUsedForGameplay);
		style_ini.GetValue("Style", "UsedForEdit", style.m_bUsedForEdit);
		style_ini.GetValue("Style", "UsedForDemonstration", style.m_bUsedForDemonstration);
		style_ini.GetValue("Style", "UsedForHowToPlay", style.m_bUsedForHowToPlay);

		RString sStepsType;
		style_ini.GetValue("Style", "StepsType", sStepsType);
		style.m_StepsType = GAMEMAN->StringToStepsType(sStepsType);

		RString sStyleType;
		style_ini.GetValue("Style", "StyleType", sStyleType);
		style.m_StyleType = StringToStyleType(sStyleType);

		style_ini.GetValue("Style", "ColsPerPlayer", style.m_iColsPerPlayer);
		style_ini.GetValue("Style", "CanUseBeginnerHelper", style.m_bCanUseBeginnerHelper);
		style_ini.GetValue("Style", "LockDifficulties", style.m_bLockDifficulties);

		style.m_szName = DupString(sStyleName);

		for (int p = 0; p < NUM_PLAYERS; ++p) {
			for (int c = 0; c < MAX_COLS_PER_PLAYER; ++c) {
				Style::ColumnInfo &ci = style.m_ColumnInfo[p][c];
				const RString sPrefix = ssprintf("P%dCol%d", p, c);
				style_ini.GetValue("Style", sPrefix + "Track", ci.track);
				style_ini.GetValue("Style", sPrefix + "XOffset", ci.fXOffset);
				RString sColName;
				style_ini.GetValue("Style", sPrefix + "Name", sColName);
				ci.pzName = sColName.empty() ? nullptr : DupString(sColName);
			}
		}
		for (int c = 0; c < NUM_GameController; ++c) {
			for (int b = 0; b < NUM_GameButton; ++b)
				style_ini.GetValue("Style", ssprintf("Controller%dInput%d", c, b), style.m_iInputColumn[c][b]);
		}
		for (int c = 0; c < MAX_COLS_PER_PLAYER; ++c)
			style_ini.GetValue("Style", ssprintf("DrawOrder%d", c), style.m_iColumnDrawOrder[c]);

		vpStyles.push_back(&style);
	}
	vpStyles.push_back(nullptr); // m_apStyles is nullptr-terminated.
	game.m_apStyles = vpStyles.data();

	return &game;
}
