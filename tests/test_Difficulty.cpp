// Characterization tests for the Difficulty string conversions
// (src/Difficulty.cpp) -- StringToDifficulty (the canonical
// Beginner/Easy/.../Edit names, case-insensitive) and
// OldStyleStringToDifficulty (the legacy alias table used by the BMS /
// DWI / KSF loaders: "another"->Medium, "maniac"->Hard, "oni"->Challenge
// ...). Pure, no engine deps.
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "Difficulty.h"
#include "GameConstantsAndTypes.h"

#include "catch_amalgamated.hpp"

TEST_CASE( "StringToDifficulty matches the canonical names case-insensitively", "[Difficulty]" )
{
	CHECK( StringToDifficulty( "Beginner" ) == Difficulty_Beginner );
	CHECK( StringToDifficulty( "medium" ) == Difficulty_Medium );
	CHECK( StringToDifficulty( "HARD" ) == Difficulty_Hard );
	CHECK( StringToDifficulty( "Challenge" ) == Difficulty_Challenge );
	CHECK( StringToDifficulty( "Edit" ) == Difficulty_Edit );

	CHECK( StringToDifficulty( "" ) == Difficulty_Invalid );
	CHECK( StringToDifficulty( "Insane" ) == Difficulty_Invalid );
	// The legacy aliases are NOT understood by the canonical parser.
	CHECK( StringToDifficulty( "maniac" ) == Difficulty_Invalid );
}

TEST_CASE( "DifficultyToString / StringToDifficulty round-trip for every real difficulty", "[Difficulty]" )
{
	for( int i = 0; i < NUM_Difficulty; ++i )
	{
		const Difficulty d = static_cast<Difficulty>( i );
		const RString name = DifficultyToString( d );
		CAPTURE( i, name );
		CHECK_FALSE( name.empty() );
		CHECK( StringToDifficulty( name ) == d );
	}
}

TEST_CASE( "OldStyleStringToDifficulty maps the legacy aliases, case-insensitively", "[Difficulty]" )
{
	CHECK( OldStyleStringToDifficulty( "beginner" ) == Difficulty_Beginner );

	CHECK( OldStyleStringToDifficulty( "easy" ) == Difficulty_Easy );
	CHECK( OldStyleStringToDifficulty( "basic" ) == Difficulty_Easy );
	CHECK( OldStyleStringToDifficulty( "LIGHT" ) == Difficulty_Easy );

	CHECK( OldStyleStringToDifficulty( "medium" ) == Difficulty_Medium );
	CHECK( OldStyleStringToDifficulty( "another" ) == Difficulty_Medium );
	CHECK( OldStyleStringToDifficulty( "trick" ) == Difficulty_Medium );
	CHECK( OldStyleStringToDifficulty( "standard" ) == Difficulty_Medium );
	CHECK( OldStyleStringToDifficulty( "difficult" ) == Difficulty_Medium );

	CHECK( OldStyleStringToDifficulty( "hard" ) == Difficulty_Hard );
	CHECK( OldStyleStringToDifficulty( "maniac" ) == Difficulty_Hard );
	CHECK( OldStyleStringToDifficulty( "heavy" ) == Difficulty_Hard );
	CHECK( OldStyleStringToDifficulty( "SSR" ) == Difficulty_Hard );

	CHECK( OldStyleStringToDifficulty( "smaniac" ) == Difficulty_Challenge );
	CHECK( OldStyleStringToDifficulty( "challenge" ) == Difficulty_Challenge );
	CHECK( OldStyleStringToDifficulty( "expert" ) == Difficulty_Challenge );
	CHECK( OldStyleStringToDifficulty( "oni" ) == Difficulty_Challenge );

	CHECK( OldStyleStringToDifficulty( "edit" ) == Difficulty_Edit );

	CHECK( OldStyleStringToDifficulty( "" ) == Difficulty_Invalid );
	CHECK( OldStyleStringToDifficulty( "nonsense" ) == Difficulty_Invalid );
}
