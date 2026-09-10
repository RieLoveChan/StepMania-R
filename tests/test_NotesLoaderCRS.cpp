// Parse-regression for .crs (courses) via CourseLoaderCRS::LoadFromBuffer
// (ADR 0006 phase 4 -- the last open format in backlog item 17).
//
// LoadFromBuffer -> LoadFromMsd(..., bFromCache=true) parses the #TAG:val;
// course format straight from a string: no SONGINDEX cache probe, no file
// I/O. EngineTestEnv brings up SONGMAN (empty -- InitAll() is not called),
// so song-reference resolution runs but every lookup misses. These tests
// PIN that current behaviour, bug-for-bug. Two quirks they lock in on
// purpose:
//
//   * #STYLE is silently dropped. The dispatch has
//       else if( !eq("DISPLAYCOURSE") || !eq("COMBO") || !eq("COMBOMODE") )
//     which is always true (a name can't equal all three), so the #STYLE
//     / #RADAR-cache / "unexpected value" branches after it are dead.
//     Flagged for the maintainer -- the || should be &&.
//   * #SONG:BEST<n> uses `iChooseIndex > iNumSongs` (not >=), so with an
//     empty SONGMAN "BEST1" (index 0) is ACCEPTED while "BEST2" is
//     rejected. Off-by-one, pinned as-is.
//
// Not covered: a 2-part "#SONG:Group/Song" reference. Resolving it goes
// SONGMAN->FindSong -> GetSongs(group) -> a FOREACH_EnabledPlayer loop
// that dereferences PROFILEMAN, which EngineTestEnv does not provide, so
// it SIGSEGVs here. One-part "#SONG:Title" refs resolve via GROUP_ALL and
// are safe. (Not a real-engine bug -- PROFILEMAN always exists by the
// time courses load.)
//
// AGENTS.md #5: .crs is a protected format. This is characterization
// (locking current output), not a fix. A diff that moves a pinned value
// is a stop-and-decide signal. Run `sm_tests "[crsdump]"` to re-baseline.

#include "global.h"
#include "EngineTestEnv.h"

#include "CourseLoaderCRS.h"
#include "Course.h"
#include "Difficulty.h"
#include "GameConstantsAndTypes.h"

#include "catch_amalgamated.hpp"

#include <cstdio>

namespace
{
	Course LoadCRS( const RString &body, const RString &path = "/testdata/CrsGroup/fixture.crs" )
	{
		Course c;
		CourseLoaderCRS::LoadFromBuffer( path, body, c );
		return c;
	}
}

TEST_CASE( "CourseLoaderCRS parses course metadata", "[NotesLoader][CourseLoaderCRS][crs][regression]" )
{
	EngineTestEnv::Require();

	// The one #SONG here is a 1-part title that misses the empty SONGMAN,
	// so out.m_vEntries stays empty and LoadFromMsd returns before it
	// touches IMAGECACHE (which the fixture does not provide).
	Course c = LoadCRS(
		"#COURSE:My Course;\n"
		"#COURSETRANSLIT:My Course (translit);\n"
		"#SCRIPTER:The Scripter;\n"
		"#DESCRIPTION:A description.;\n"
		"#REPEAT:YES;\n"
		"#LIVES:4;\n"
		"#GAINSECONDS:30;\n"
		"#BANNER:banner.png;\n"
		"#BACKGROUND:bg.png;\n"
		"#METER:7;\n"
		"#METER:Difficult:9;\n"
		"#STYLE:dance-single,dance-double;\n"
		"#SONG:NonexistentTitle:Hard:;\n" );

	CHECK( c.m_sMainTitle == "My Course" );
	CHECK( c.m_sMainTitleTranslit == "My Course (translit)" );
	CHECK( c.m_sScripter == "The Scripter" );
	CHECK( c.m_sDescription == "A description." );
	CHECK( c.m_bRepeat == true );
	CHECK( c.m_iLives == 4 );
	CHECK( c.m_sBannerPath == "banner.png" );
	CHECK( c.m_sBackgroundPath == "bg.png" );

	// #METER:7  -> the 2-param compat form targets Difficulty_Medium.
	CHECK( c.m_iCustomMeter[Difficulty_Medium] == 7 );
	// #METER:Difficult:9  -> 3-param form. The CRS difficulty-name table
	// (g_CRSDifficultyNames) is Beginner/Easy/Regular/Difficult/
	// Challenge/Edit, so "Difficult" is Difficulty_Hard (not "Hard").
	CHECK( c.m_iCustomMeter[Difficulty_Hard] == 9 );

	// #STYLE is silently ignored (dead-branch bug, see file header).
	CHECK( c.m_setStyles.empty() );

	// m_sPath / m_sGroupName are only set by LoadFromCRSFile, not by
	// LoadFromMsd, so via LoadFromBuffer they stay empty.
	CHECK( c.m_sGroupName.empty() );

	// The one unresolvable #SONG was skipped -> incomplete, no entries.
	CHECK( c.m_vEntries.empty() );
	CHECK( c.m_bIncomplete == true );
}

TEST_CASE( "CourseLoaderCRS #SONG entry resolution against an empty SONGMAN",
           "[NotesLoader][CourseLoaderCRS][crs][regression]" )
{
	EngineTestEnv::Require();

	Course c = LoadCRS(
		"#COURSE:Entry Matrix;\n"
		"#SONG:BEST1:Hard:;\n"          // index 0, 0 > 0 is false -> ACCEPTED
		"#SONG:BEST2:Hard:;\n"          // index 1, 1 > 0 is true  -> rejected
		"#SONG:GRADEBEST1:Hard:;\n"     // no song-count check     -> ACCEPTED
		"#SONG:*:Medium:;\n"            // wildcard, bSecret        -> ACCEPTED
		"#SONG:LoneTitle:Hard:;\n" );   // 1-part FindSong miss     -> rejected

	REQUIRE( c.m_vEntries.size() == 3 );
	CHECK( c.m_bIncomplete == true );

	// entry 0: BEST1
	CHECK( c.m_vEntries[0].songSort == SongSort_MostPlays );
	CHECK( c.m_vEntries[0].iChooseIndex == 0 );

	// entry 1: GRADEBEST1
	CHECK( c.m_vEntries[1].songSort == SongSort_TopGrades );

	// entry 2: "*"
	CHECK( c.m_vEntries[2].bSecret == true );

	// all three carry the parsed difficulty from param 2
	CHECK( c.m_vEntries[0].stepsCriteria.m_difficulty == Difficulty_Hard );
	CHECK( c.m_vEntries[1].stepsCriteria.m_difficulty == Difficulty_Hard );
	CHECK( c.m_vEntries[2].stepsCriteria.m_difficulty == Difficulty_Medium );
}

TEST_CASE( "CourseLoaderCRS #SONG difficulty / meter-range parsing",
           "[NotesLoader][CourseLoaderCRS][crs][regression]" )
{
	EngineTestEnv::Require();

	Course c = LoadCRS(
		"#COURSE:Difficulties;\n"
		"#SONG:GRADEBEST1:heavy:;\n"    // old-style alias -> Difficulty_Hard
		"#SONG:GRADEBEST2:5..8:;\n"     // meter range
		"#SONG:GRADEBEST3:bogus:;\n" ); // unparseable -> 3..6 used instead

	REQUIRE( c.m_vEntries.size() == 3 );

	CHECK( c.m_vEntries[0].stepsCriteria.m_difficulty == Difficulty_Hard );

	CHECK( c.m_vEntries[1].stepsCriteria.m_difficulty == Difficulty_Invalid );
	CHECK( c.m_vEntries[1].stepsCriteria.m_iLowMeter == 5 );
	CHECK( c.m_vEntries[1].stepsCriteria.m_iHighMeter == 8 );

	CHECK( c.m_vEntries[2].stepsCriteria.m_difficulty == Difficulty_Invalid );
	CHECK( c.m_vEntries[2].stepsCriteria.m_iLowMeter == 3 );
	CHECK( c.m_vEntries[2].stepsCriteria.m_iHighMeter == 6 );
}

TEST_CASE( "CourseLoaderCRS #SONG modifier column keywords",
           "[NotesLoader][CourseLoaderCRS][crs][regression]" )
{
	EngineTestEnv::Require();

	Course c = LoadCRS(
		"#COURSE:Mods;\n"
		"#SONG:GRADEBEST1:Hard:noshowcourse;\n"
		"#SONG:GRADEBEST2:Hard:showcourse;\n"
		"#SONG:GRADEBEST3:Hard:nodifficult,2x;\n" );

	REQUIRE( c.m_vEntries.size() == 3 );

	CHECK( c.m_vEntries[0].bSecret == true );          // noshowcourse forces secret on
	CHECK( c.m_vEntries[1].bSecret == false );         // showcourse forces it off
	CHECK( c.m_vEntries[2].bNoDifficult == true );     // nodifficult consumed
	CHECK( c.m_vEntries[2].sModifiers == "2x" );       // the rest is left as the mod string
}

// Hidden. Run with:  sm_tests "[crsdump]"
TEST_CASE( "dump CRS loader values", "[.][crs][crsdump]" )
{
	EngineTestEnv::Require();

	Course c = LoadCRS(
		"#COURSE:Dump Course;\n"
		"#SCRIPTER:x;\n"
		"#REPEAT:YES;\n"
		"#METER:7;\n"
		"#METER:Hard:9;\n"
		"#STYLE:dance-single;\n"
		"#SONG:BEST1:Hard:;\n"
		"#SONG:BEST2:Hard:;\n"
		"#SONG:GRADEBEST1:heavy:noshowcourse;\n"
		"#SONG:*:5..8:;\n"
		"#SONG:LoneTitle:Hard:;\n" );

	std::printf( "\n=== CRS dump ===\n" );
	std::printf( "  title=[%s] scripter=[%s] repeat=%d group=[%s]\n",
		c.m_sMainTitle.c_str(), c.m_sScripter.c_str(), (int)c.m_bRepeat,
		c.m_sGroupName.c_str() );
	std::printf( "  meter[Medium]=%d meter[Hard]=%d styles=%d incomplete=%d\n",
		c.m_iCustomMeter[Difficulty_Medium], c.m_iCustomMeter[Difficulty_Hard],
		(int)c.m_setStyles.size(), (int)c.m_bIncomplete );
	std::printf( "  entries=%d\n", (int)c.m_vEntries.size() );
	for( size_t i = 0; i < c.m_vEntries.size(); ++i )
	{
		const CourseEntry &e = c.m_vEntries[i];
		std::printf( "    [%d] sort=%d chooseIdx=%d secret=%d noDiff=%d "
			"diff=%d low=%d high=%d mods=[%s]\n",
			(int)i, (int)e.songSort, e.iChooseIndex, (int)e.bSecret,
			(int)e.bNoDifficult, (int)e.stepsCriteria.m_difficulty,
			e.stepsCriteria.m_iLowMeter, e.stepsCriteria.m_iHighMeter,
			e.sModifiers.c_str() );
	}
	SUCCEED( "dump complete" );
}
