// Characterization tests for the RageUtil time / number formatters --
// SecondsTo{HHMMSS,MSS,MMSS,MSSMsMs,MMSSMsMs,MMSSMsMsMs} and Commify.
// These sit on every score screen, results screen and song-length
// display. Pure, no engine deps.
//
// Pins CURRENT behaviour, bug-for-bug -- notably that SecondsToHHMMSS
// derives H:M:S from a minutes count (so it is really HH:MM:SS built
// from total minutes), and the fractional forms clamp the sub-second
// field rather than rounding. See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "RageUtil.h"

#include "catch_amalgamated.hpp"

TEST_CASE( "SecondsToMSS / SecondsToMMSS: minutes:seconds, 1 vs 2 digit minutes", "[RageUtil][time]" )
{
	CHECK( SecondsToMSS( 0 ) == "0:00" );
	CHECK( SecondsToMSS( 9 ) == "0:09" );
	CHECK( SecondsToMSS( 90 ) == "1:30" );
	CHECK( SecondsToMSS( 605 ) == "10:05" );

	CHECK( SecondsToMMSS( 0 ) == "00:00" );
	CHECK( SecondsToMMSS( 90 ) == "01:30" );
	CHECK( SecondsToMMSS( 605 ) == "10:05" );
}

TEST_CASE( "SecondsToHHMMSS splits the minutes count into HH:MM:SS", "[RageUtil][time]" )
{
	CHECK( SecondsToHHMMSS( 0 ) == "00:00:00" );
	CHECK( SecondsToHHMMSS( 59 ) == "00:00:59" );
	CHECK( SecondsToHHMMSS( 90 ) == "00:01:30" );
	CHECK( SecondsToHHMMSS( 3661 ) == "01:01:01" );  // 61 min -> 01:01, +1 s
	CHECK( SecondsToHHMMSS( 7325 ) == "02:02:05" );
}

TEST_CASE( "SecondsToMMSSMsMs / MSSMsMs: two-digit hundredths, clamped to 99", "[RageUtil][time]" )
{
	CHECK( SecondsToMMSSMsMs( 0.0f ) == "00:00.00" );
	CHECK( SecondsToMMSSMsMs( 90.5f ) == "01:30.50" );
	CHECK( SecondsToMMSSMsMs( 5.25f ) == "00:05.25" );

	CHECK( SecondsToMSSMsMs( 90.5f ) == "1:30.50" );

	// The hundredths field is min()'d to 99, never carried into seconds.
	const RString s = SecondsToMMSSMsMs( 3.999999f );
	CAPTURE( s );
	CHECK( ( s == "00:03.99" || s == "00:04.00" ) ); // depends on the float, but never "00:03.100"
}

TEST_CASE( "SecondsToMMSSMsMsMs: three-digit thousandths, clamped to 999", "[RageUtil][time]" )
{
	CHECK( SecondsToMMSSMsMsMs( 0.0f ) == "00:00.000" );
	CHECK( SecondsToMMSSMsMsMs( 12.345f ) == "00:12.345" );
	CHECK( SecondsToMMSSMsMsMs( 61.5f ) == "01:01.500" );
}

TEST_CASE( "Commify groups digits in threes; sign and decimals are left alone", "[RageUtil][number]" )
{
	CHECK( Commify( 0 ) == "0" );
	CHECK( Commify( 42 ) == "42" );
	CHECK( Commify( 100 ) == "100" );        // 3 digits -> no comma
	CHECK( Commify( 1000 ) == "1,000" );
	CHECK( Commify( 12345 ) == "12,345" );
	CHECK( Commify( 1234567 ) == "1,234,567" );
	CHECK( Commify( -1234567 ) == "-1,234,567" );

	// String form with custom separators, and a decimal that is not grouped.
	CHECK( Commify( "1234567", ",", "." ) == "1,234,567" );
	CHECK( Commify( "1234.5678", ",", "." ) == "1,234.5678" );
}
