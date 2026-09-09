// Characterization tests for RageColor's string codec (RageTypes.h /
// RageTypes.cpp) -- how theme metrics and Lua turn "1,0,0.5" or
// "#FF8000" into an r/g/b/a float colour and back. Pure, no engine deps.
//
// Pins CURRENT behaviour, bug-for-bug (including: a parse failure
// leaves the colour set to opaque white, and ToString quantises to
// 8-bit and upper-cases the hex). See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "RageTypes.h"

#include "catch_amalgamated.hpp"

using Catch::Approx;

TEST_CASE( "RageColor::FromString parses 3 or 4 comma floats", "[RageColor]" )
{
	RageColor c;

	REQUIRE( c.FromString( "1,0,0.5" ) );
	CHECK( c.r == Approx( 1.0f ) );
	CHECK( c.g == Approx( 0.0f ) );
	CHECK( c.b == Approx( 0.5f ) );
	CHECK( c.a == Approx( 1.0f ) ); // 3 components -> alpha defaults to 1

	REQUIRE( c.FromString( "0.2,0.4,0.6,0.8" ) );
	CHECK( c.r == Approx( 0.2f ) );
	CHECK( c.g == Approx( 0.4f ) );
	CHECK( c.b == Approx( 0.6f ) );
	CHECK( c.a == Approx( 0.8f ) );
}

TEST_CASE( "RageColor::FromString parses #RRGGBB and #RRGGBBAA hex (case-insensitive)", "[RageColor]" )
{
	RageColor c;

	REQUIRE( c.FromString( "#FF8000" ) );
	CHECK( c.r == Approx( 1.0f ) );
	CHECK( c.g == Approx( 128 / 255.0f ) );
	CHECK( c.b == Approx( 0.0f ) );
	CHECK( c.a == Approx( 1.0f ) );

	RageColor lower;
	REQUIRE( lower.FromString( "#ff8000" ) );
	CHECK( lower.r == Approx( c.r ) );
	CHECK( lower.g == Approx( c.g ) );

	REQUIRE( c.FromString( "#00FF0080" ) );
	CHECK( c.r == Approx( 0.0f ) );
	CHECK( c.g == Approx( 1.0f ) );
	CHECK( c.b == Approx( 0.0f ) );
	CHECK( c.a == Approx( 128 / 255.0f ) );
}

TEST_CASE( "RageColor::FromString on garbage returns false and sets opaque white", "[RageColor]" )
{
	RageColor c;
	c.r = c.g = c.b = c.a = 0.0f;

	CHECK_FALSE( c.FromString( "not a color" ) );
	CHECK( c.r == Approx( 1.0f ) );
	CHECK( c.g == Approx( 1.0f ) );
	CHECK( c.b == Approx( 1.0f ) );
	CHECK( c.a == Approx( 1.0f ) );

	CHECK_FALSE( c.FromString( "1,2" ) ); // only 2 components
}

TEST_CASE( "RageColor::ToString: #RRGGBB when opaque, #RRGGBBAA otherwise, upper-case, clamped", "[RageColor]" )
{
	RageColor opaque( 1.0f, 128 / 255.0f, 0.0f, 1.0f );
	CHECK( opaque.ToString() == "#FF8000" );

	RageColor translucent( 0.0f, 1.0f, 0.0f, 128 / 255.0f );
	CHECK( translucent.ToString() == "#00FF0080" );

	// Out-of-range channels clamp to [0,255].
	RageColor over( 2.0f, -1.0f, 0.5f, 1.0f );
	CHECK( over.ToString() == "#FF0080" );
}

TEST_CASE( "RageColor string round-trip through the 8-bit quantisation", "[RageColor]" )
{
	for( const char *s : { "#000000", "#FFFFFF", "#FF8000", "#12345678", "#ABCDEF" } )
	{
		RageColor c;
		REQUIRE( c.FromString( s ) );
		// ToString upper-cases; compare case-insensitively via re-parse.
		RageColor c2;
		REQUIRE( c2.FromString( c.ToString() ) );
		CHECK( c2.r == Approx( c.r ) );
		CHECK( c2.g == Approx( c.g ) );
		CHECK( c2.b == Approx( c.b ) );
		CHECK( c2.a == Approx( c.a ) );
	}
}

TEST_CASE( "RageColor::NormalizeColorString canonicalises or empties", "[RageColor]" )
{
	CHECK( RageColor::NormalizeColorString( "" ) == "" );
	CHECK( RageColor::NormalizeColorString( "garbage" ) == "" );
	CHECK( RageColor::NormalizeColorString( "1,0.5,0" ) == "#FF8000" );
	CHECK( RageColor::NormalizeColorString( "#ff8000" ) == "#FF8000" );
	CHECK( RageColor::NormalizeColorString( "0,1,0,0.5" ) == "#00FF0080" );
}
