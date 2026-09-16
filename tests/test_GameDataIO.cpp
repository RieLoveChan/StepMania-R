// Characterization/round-trip test for ADR 0008 stage 1 (data-driven
// game-type registry): proves ExportGameToDisk()/LoadGameFromDisk() produce
// a Game/Style/InputScheme/AutoMappings tree that is field-for-field
// identical to the currently-compiled g_Game_Dance/g_Style_Dance_* literals.
// See DocsAgents/adr/0008-game-type-registry.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "GameDataIO.h"
#include "GameManager.h"
#include "Game.h"
#include "Style.h"
#include "InputMapper.h"
#include "GameConstantsAndTypes.h"

#include "catch_amalgamated.hpp"

namespace {
void CompareColumnInfo( const Style::ColumnInfo &a, const Style::ColumnInfo &b, const RString &sWhere )
{
	CAPTURE( sWhere );
	CHECK( a.track == b.track );
	CHECK( a.fXOffset == Catch::Approx( b.fXOffset ) );
	const RString aName = a.pzName != nullptr ? a.pzName : "";
	const RString bName = b.pzName != nullptr ? b.pzName : "";
	CHECK( aName == bName );
}

void CompareStyle( const Style *a, const Style *b, const RString &sWhere )
{
	CAPTURE( sWhere );
	REQUIRE( a != nullptr );
	REQUIRE( b != nullptr );
	CHECK( RString(a->m_szName) == RString(b->m_szName) );
	CHECK( a->m_bUsedForGameplay == b->m_bUsedForGameplay );
	CHECK( a->m_bUsedForEdit == b->m_bUsedForEdit );
	CHECK( a->m_bUsedForDemonstration == b->m_bUsedForDemonstration );
	CHECK( a->m_bUsedForHowToPlay == b->m_bUsedForHowToPlay );
	CHECK( a->m_StepsType == b->m_StepsType );
	CHECK( a->m_StyleType == b->m_StyleType );
	CHECK( a->m_iColsPerPlayer == b->m_iColsPerPlayer );
	CHECK( a->m_bCanUseBeginnerHelper == b->m_bCanUseBeginnerHelper );
	CHECK( a->m_bLockDifficulties == b->m_bLockDifficulties );

	for( int p = 0; p < NUM_PLAYERS; ++p )
		for( int c = 0; c < MAX_COLS_PER_PLAYER; ++c )
			CompareColumnInfo( a->m_ColumnInfo[p][c], b->m_ColumnInfo[p][c], ssprintf( "%s ColumnInfo[%d][%d]", sWhere.c_str(), p, c ) );

	for( int c = 0; c < NUM_GameController; ++c )
		for( int gb = 0; gb < NUM_GameButton; ++gb )
		{
			CAPTURE( sWhere, c, gb );
			CHECK( a->m_iInputColumn[c][gb] == b->m_iInputColumn[c][gb] );
		}

	for( int c = 0; c < MAX_COLS_PER_PLAYER; ++c )
	{
		CAPTURE( sWhere, c );
		CHECK( a->m_iColumnDrawOrder[c] == b->m_iColumnDrawOrder[c] );
	}
}
}

namespace {
const char *const g_asAllGameNames[] = {
   "dance", "pump", "techno", "lights", "kb7", "ez2", "para", "ds3ddx", "beat", "maniax", "popn", "kickbox",
};
}

TEST_CASE( "GameDataIO round-trips every built-in Game through export+load, field for field", "[GameDataIO]" )
{
	EngineTestEnv::Require();

	for( const char *szGameName : g_asAllGameNames )
	{
	CAPTURE( szGameName );

	const Game *pOriginal = GAMEMAN->StringToGame( szGameName );
	REQUIRE( pOriginal != nullptr );

	const RString sBaseDir = RString("/@mem/games_test_") + szGameName + "/";
	ExportGameToDisk( pOriginal, sBaseDir );

	GameDataStore store;
	const Game *pLoaded = LoadGameFromDisk( szGameName, sBaseDir, &store );
	REQUIRE( pLoaded != nullptr );

	// Top-level Game fields.
	CHECK( RString(pLoaded->m_szName) == RString(pOriginal->m_szName) );
	CHECK( pLoaded->m_bCountNotesSeparately == pOriginal->m_bCountNotesSeparately );
	CHECK( pLoaded->m_bTickHolds == pOriginal->m_bTickHolds );
	CHECK( pLoaded->m_PlayersHaveSeparateStyles == pOriginal->m_PlayersHaveSeparateStyles );
	CHECK( pLoaded->m_mapW1To == pOriginal->m_mapW1To );
	CHECK( pLoaded->m_mapW2To == pOriginal->m_mapW2To );
	CHECK( pLoaded->m_mapW3To == pOriginal->m_mapW3To );
	CHECK( pLoaded->m_mapW4To == pOriginal->m_mapW4To );
	CHECK( pLoaded->m_mapW5To == pOriginal->m_mapW5To );

	// InputScheme + PerButtonInfo, for every custom button this game defines.
	const InputScheme &origScheme = pOriginal->m_InputScheme;
	const InputScheme &loadScheme = pLoaded->m_InputScheme;
	REQUIRE( loadScheme.m_iButtonsPerController == origScheme.m_iButtonsPerController );

	for( int i = 0; i < origScheme.m_iButtonsPerController; ++i )
	{
		const GameButton gb = static_cast<GameButton>( GAME_BUTTON_NEXT + i );
		CAPTURE( i, gb );

		const InputScheme::GameButtonInfo *pOrigInfo = origScheme.GetGameButtonInfo( gb );
		const InputScheme::GameButtonInfo *pLoadInfo = loadScheme.GetGameButtonInfo( gb );
		CHECK( RString(pLoadInfo->m_szName) == RString(pOrigInfo->m_szName) );
		CHECK( pLoadInfo->m_SecondaryMenuButton == pOrigInfo->m_SecondaryMenuButton );

		const Game::PerButtonInfo *pOrigPB = pOriginal->GetPerButtonInfo( gb );
		const Game::PerButtonInfo *pLoadPB = pLoaded->GetPerButtonInfo( gb );
		CHECK( pLoadPB->m_gbt == pOrigPB->m_gbt );
	}

	// AutoMappings (this game's keyboard-default mapping).
	REQUIRE( origScheme.m_pAutoMappings != nullptr );
	REQUIRE( loadScheme.m_pAutoMappings != nullptr );
	const AutoMappings &origMap = *origScheme.m_pAutoMappings;
	const AutoMappings &loadMap = *loadScheme.m_pAutoMappings;
	CHECK( loadMap.m_sGame == origMap.m_sGame );
	CHECK( loadMap.m_sDriverRegex == origMap.m_sDriverRegex );
	CHECK( loadMap.m_sControllerName == origMap.m_sControllerName );
	REQUIRE( loadMap.m_vMaps.size() == origMap.m_vMaps.size() );
	for( std::size_t i = 0; i < origMap.m_vMaps.size(); ++i )
	{
		CAPTURE( i );
		CHECK( loadMap.m_vMaps[i].m_iSlotIndex == origMap.m_vMaps[i].m_iSlotIndex );
		CHECK( loadMap.m_vMaps[i].m_deviceButton == origMap.m_vMaps[i].m_deviceButton );
		CHECK( loadMap.m_vMaps[i].m_gb == origMap.m_vMaps[i].m_gb );
		CHECK( loadMap.m_vMaps[i].m_bSecondController == origMap.m_vMaps[i].m_bSecondController );
	}

	// Every Style, in original order (m_apStyles is nullptr-terminated on
	// both sides).
	const Style *const *ppOrig = pOriginal->m_apStyles;
	const Style *const *ppLoad = pLoaded->m_apStyles;
	int iStyleCount = 0;
	while( *ppOrig != nullptr )
	{
		REQUIRE( *ppLoad != nullptr );
		CompareStyle( *ppOrig, *ppLoad, ssprintf( "style #%d", iStyleCount ) );
		++ppOrig;
		++ppLoad;
		++iStyleCount;
	}
	CHECK( *ppLoad == nullptr ); // same length on both sides
	CHECK( iStyleCount > 0 );
	} // for each game name
}
