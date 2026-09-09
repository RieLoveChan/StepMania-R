// Characterization tests for GameManager's string<->enum / game / style
// table lookups -- the data-driven glue that turns simfile chart tags
// ("dance-single") and metric strings ("dance", "versus") into engine
// StepsType / Game / Style. GAMEMAN is brought up by EngineTestEnv.
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "GameManager.h"
#include "Game.h"
#include "Style.h"
#include "GameConstantsAndTypes.h"

#include "catch_amalgamated.hpp"

#include <vector>

TEST_CASE( "GameManager::StringToStepsType is case-insensitive and round-trips its name", "[GameManager]" )
{
	EngineTestEnv::Require();

	CHECK( GAMEMAN->StringToStepsType( "dance-single" ) == StepsType_dance_single );
	CHECK( GAMEMAN->StringToStepsType( "DANCE-SINGLE" ) == StepsType_dance_single );
	CHECK( GAMEMAN->StringToStepsType( "Dance-Double" ) == StepsType_dance_double );
	CHECK( GAMEMAN->StringToStepsType( "pump-single" ) == StepsType_pump_single );

	CHECK( GAMEMAN->StringToStepsType( "not-a-real-type" ) == StepsType_Invalid );
	CHECK( GAMEMAN->StringToStepsType( "" ) == StepsType_Invalid );

	// Every real StepsType round-trips through its name.
	for( int i = 0; i < NUM_StepsType; ++i )
	{
		const StepsType st = static_cast<StepsType>( i );
		const RString name = GAMEMAN->GetStepsTypeInfo( st ).szName;
		CAPTURE( i, name );
		CHECK( GAMEMAN->StringToStepsType( name ) == st );
	}
}

TEST_CASE( "GameManager::StringToGame resolves the built-in games, case-insensitively", "[GameManager]" )
{
	EngineTestEnv::Require();

	const Game *dance = GAMEMAN->StringToGame( "dance" );
	REQUIRE( dance != nullptr );
	CHECK( RString( dance->m_szName ) == "dance" );
	CHECK( GAMEMAN->StringToGame( "DANCE" ) == dance );

	CHECK( GAMEMAN->StringToGame( "pump" ) != nullptr );
	CHECK( GAMEMAN->StringToGame( "kb7" ) != nullptr );

	CHECK( GAMEMAN->StringToGame( "no-such-game" ) == nullptr );
	CHECK( GAMEMAN->StringToGame( "" ) == nullptr );
}

TEST_CASE( "GameManager::GameAndStringToStyle maps (game, style-name) to a Style with the right StepsType", "[GameManager]" )
{
	EngineTestEnv::Require();

	const Game *dance = GAMEMAN->StringToGame( "dance" );
	REQUIRE( dance != nullptr );

	const Style *single = GAMEMAN->GameAndStringToStyle( dance, "single" );
	REQUIRE( single != nullptr );
	CHECK( RString( single->m_szName ) == "single" );
	CHECK( single->m_StepsType == StepsType_dance_single );
	CHECK( single->m_iColsPerPlayer == 4 );

	const Style *dbl = GAMEMAN->GameAndStringToStyle( dance, "double" );
	REQUIRE( dbl != nullptr );
	CHECK( dbl->m_StepsType == StepsType_dance_double );
	CHECK( dbl->m_iColsPerPlayer == 8 );

	CHECK( GAMEMAN->GameAndStringToStyle( dance, "nonexistent" ) == nullptr );
}

TEST_CASE( "GameManager::GetStylesForGame / GetStepsTypesForGame return non-empty, consistent sets", "[GameManager]" )
{
	EngineTestEnv::Require();

	const Game *dance = GAMEMAN->StringToGame( "dance" );
	REQUIRE( dance != nullptr );

	std::vector<const Style*> styles;
	GAMEMAN->GetStylesForGame( dance, styles );
	REQUIRE_FALSE( styles.empty() );

	// Every style of the dance game maps back to the dance game.
	for( const Style *s : styles )
	{
		CAPTURE( s->m_szName );
		CHECK( GAMEMAN->GetGameForStyle( s ) == dance );
	}

	std::vector<StepsType> sts;
	GAMEMAN->GetStepsTypesForGame( dance, sts );
	REQUIRE_FALSE( sts.empty() );
	// dance-single is one of them.
	CHECK( std::find( sts.begin(), sts.end(), StepsType_dance_single ) != sts.end() );
}
