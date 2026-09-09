// Characterization tests for the Grade <-> string conversions
// (src/Grade.h inline GradeToString + src/Grade.cpp StringToGrade).
//
// Pins CURRENT behaviour, bug-for-bug -- notably: StringToGrade
// upper-cases its input for the "FAILED"/"NODATA" checks but then runs
// the "Tier%02d" sscanf against the ORIGINAL string, so "TIER01" is not
// recognised even though "tier"/"failed" casing is otherwise ignored.
//
// See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"   // StringToGrade's invalid path calls LOG->Warn

#include "Grade.h"

#include "catch_amalgamated.hpp"

TEST_CASE( "GradeToString: NoData / Failed / Tier NN", "[Grade]" )
{
	CHECK( GradeToString( Grade_Tier01 ) == "Tier01" );
	CHECK( GradeToString( Grade_Tier10 ) == "Tier10" );
	CHECK( GradeToString( Grade_Tier20 ) == "Tier20" );
	CHECK( GradeToString( Grade_Failed ) == "Failed" );
	CHECK( GradeToString( Grade_NoData ) == "NoData" );
}

TEST_CASE( "StringToGrade round-trips GradeToString for every real grade", "[Grade]" )
{
	EngineTestEnv::Require();

	for( int i = 0; i < NUM_Grade; ++i )
	{
		const Grade g = static_cast<Grade>( i );
		const RString s = GradeToString( g );
		CAPTURE( i, s );
		CHECK( StringToGrade( s ) == g );
	}
	CHECK( StringToGrade( GradeToString( Grade_NoData ) ) == Grade_NoData );
}

TEST_CASE( "StringToGrade: FAILED / NODATA are case-insensitive, Tier is not", "[Grade]" )
{
	EngineTestEnv::Require();

	CHECK( StringToGrade( "Failed" ) == Grade_Failed );
	CHECK( StringToGrade( "failed" ) == Grade_Failed );
	CHECK( StringToGrade( "FAILED" ) == Grade_Failed );

	CHECK( StringToGrade( "NoData" ) == Grade_NoData );
	CHECK( StringToGrade( "nodata" ) == Grade_NoData );

	// "Tier%02d" sscanf runs against the un-upper-cased original, so a
	// mis-cased "Tier" prefix falls through to the invalid branch.
	CHECK( StringToGrade( "Tier03" ) == Grade_Tier03 );
	CHECK( StringToGrade( "TIER03" ) == Grade_NoData );
	CHECK( StringToGrade( "tier03" ) == Grade_NoData );
}

TEST_CASE( "StringToGrade: unrecognised input -> Grade_NoData", "[Grade]" )
{
	EngineTestEnv::Require();

	CHECK( StringToGrade( "" ) == Grade_NoData );
	CHECK( StringToGrade( "AAA" ) == Grade_NoData );   // old-style grade string, not accepted
	CHECK( StringToGrade( "Tier99" ) == Grade_NoData ); // out of range
}
