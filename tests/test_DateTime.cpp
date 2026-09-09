// Characterization tests for src/DateTime.cpp -- the plain
// year/month/day/hour/min/sec value type used for profile timestamps
// and high-score dates. Pure, no engine deps.
//
// Pins CURRENT behaviour, bug-for-bug (including: FromString does NOT
// validate the fields, and the internal representation is tm-style --
// tm_year is years-since-1900, tm_mon is 0-based). See
// DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "DateTime.h"

#include "catch_amalgamated.hpp"

namespace
{
	DateTime Make( int y, int mon, int mday, int h = 0, int m = 0, int s = 0 )
	{
		DateTime d;
		d.Init();
		d.tm_year = y - 1900;
		d.tm_mon = mon - 1;
		d.tm_mday = mday;
		d.tm_hour = h;
		d.tm_min = m;
		d.tm_sec = s;
		return d;
	}
}

TEST_CASE( "DateTime::Init zeroes every field", "[DateTime]" )
{
	DateTime d;
	d.tm_year = d.tm_mon = d.tm_mday = d.tm_hour = d.tm_min = d.tm_sec = 99;
	d.Init();
	CHECK( d.tm_year == 0 );
	CHECK( d.tm_mon == 0 );
	CHECK( d.tm_mday == 0 );
	CHECK( d.tm_hour == 0 );
	CHECK( d.tm_min == 0 );
	CHECK( d.tm_sec == 0 );
}

TEST_CASE( "DateTime::GetString omits the time when it is all-zero", "[DateTime]" )
{
	CHECK( Make( 2025, 3, 7 ).GetString() == "2025-03-07" );
	CHECK( Make( 2025, 3, 7, 0, 0, 0 ).GetString() == "2025-03-07" );
	CHECK( Make( 2025, 3, 7, 9, 5, 1 ).GetString() == "2025-03-07 09:05:01" );
	CHECK( Make( 1999, 12, 31, 23, 59, 59 ).GetString() == "1999-12-31 23:59:59" );

	// Any nonzero time component forces the time half.
	CHECK( Make( 2000, 1, 1, 0, 0, 1 ).GetString() == "2000-01-01 00:00:01" );
}

TEST_CASE( "DateTime::FromString parses both the date and the date+time forms", "[DateTime]" )
{
	DateTime d;

	REQUIRE( d.FromString( "2025-03-07" ) );
	CHECK( d.tm_year == 125 ); // 2025 - 1900
	CHECK( d.tm_mon == 2 );    // March, 0-based
	CHECK( d.tm_mday == 7 );
	CHECK( d.tm_hour == 0 );
	CHECK( d.tm_min == 0 );
	CHECK( d.tm_sec == 0 );

	REQUIRE( d.FromString( "2025-03-07 09:05:01" ) );
	CHECK( d.tm_hour == 9 );
	CHECK( d.tm_min == 5 );
	CHECK( d.tm_sec == 1 );
}

TEST_CASE( "DateTime::FromString round-trips DateTime::GetString", "[DateTime]" )
{
	for( const DateTime &orig : { Make( 2025, 3, 7 ),
	                              Make( 2025, 3, 7, 9, 5, 1 ),
	                              Make( 1970, 1, 1 ),
	                              Make( 2038, 1, 19, 3, 14, 7 ) } )
	{
		DateTime parsed;
		REQUIRE( parsed.FromString( orig.GetString() ) );
		CHECK( parsed == orig );
		CHECK( parsed.GetString() == orig.GetString() );
	}
}

TEST_CASE( "DateTime::FromString rejects non-date text and does not validate the fields", "[DateTime]" )
{
	DateTime d;
	CHECK_FALSE( d.FromString( "" ) );
	CHECK_FALSE( d.FromString( "not a date" ) );
	CHECK_FALSE( d.FromString( "2025/03/07" ) );

	// Characterization: an impossible calendar date is accepted as-is
	// (the header's own "XXX: Is it possible to set an illegal date"
	// question). No normalisation happens.
	REQUIRE( d.FromString( "2025-02-30" ) );
	CHECK( d.tm_mon == 1 );
	CHECK( d.tm_mday == 30 );
	REQUIRE( d.FromString( "2025-13-99 25:61:61" ) );
	CHECK( d.tm_mon == 12 );  // 13 - 1
	CHECK( d.tm_mday == 99 );
	CHECK( d.tm_hour == 25 );
}

TEST_CASE( "DateTime comparison orders year, then month, day, hour, min, sec", "[DateTime]" )
{
	const DateTime base = Make( 2025, 6, 15, 12, 30, 30 );

	CHECK( Make( 2024, 12, 31, 23, 59, 59 ) < base ); // earlier year wins
	CHECK( Make( 2025, 5, 20, 23, 59, 59 ) < base );  // earlier month wins
	CHECK( Make( 2025, 6, 14, 23, 59, 59 ) < base );  // earlier day wins
	CHECK( Make( 2025, 6, 15, 11, 59, 59 ) < base );  // earlier hour wins
	CHECK( Make( 2025, 6, 15, 12, 29, 59 ) < base );  // earlier minute wins
	CHECK( Make( 2025, 6, 15, 12, 30, 29 ) < base );  // earlier second wins

	CHECK( base == Make( 2025, 6, 15, 12, 30, 30 ) );
	CHECK( base != Make( 2025, 6, 15, 12, 30, 31 ) );
	CHECK( base <= Make( 2025, 6, 15, 12, 30, 30 ) );
	CHECK( base >= Make( 2025, 6, 15, 12, 30, 30 ) );
	CHECK( base > Make( 2025, 6, 15, 12, 30, 29 ) );
	CHECK_FALSE( base < base );
}
