// Characterization tests for the Actor command mini-language parser
// (src/Command.cpp): ParseCommands / Command::Load / GetName / GetArg.
// Pure, no engine deps.
//
// Pins CURRENT behaviour, bug-for-bug -- notably that Command::GetName()
// only Trim()s the first token, it does NOT lower-case it, despite the
// header comment claiming "the command name is the first argument in
// all-lowercase". See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "Command.h"
#include "RageUtil.h"

#include "catch_amalgamated.hpp"

TEST_CASE( "ParseCommands splits on ';' and each command splits its args on ','", "[Command]" )
{
	const Commands c = ParseCommands( "zoom,2;diffuse,1,0,0,1" );
	REQUIRE( c.v.size() == 2 );

	CHECK( c.v[0].GetName() == "zoom" );
	CHECK( c.v[0].m_vsArgs.size() == 2 );
	CHECK( c.v[0].GetArg( 1 ).s == "2" );

	CHECK( c.v[1].GetName() == "diffuse" );
	CHECK( c.v[1].m_vsArgs.size() == 5 );
	CHECK( c.v[1].GetArg( 4 ).s == "1" );
}

TEST_CASE( "Command::Load keeps empty args (split does not ignore empties)", "[Command]" )
{
	Command cmd;
	cmd.Load( "x,,z" );
	REQUIRE( cmd.m_vsArgs.size() == 3 );
	CHECK( cmd.m_vsArgs[0] == "x" );
	CHECK( cmd.m_vsArgs[1] == "" );
	CHECK( cmd.m_vsArgs[2] == "z" );
}

TEST_CASE( "Command::GetName trims but does NOT lower-case (header comment is stale)", "[Command]" )
{
	Command cmd;
	cmd.Load( "  Zoom  , 2" );
	CHECK( cmd.GetName() == "Zoom" ); // trimmed, case preserved
	CHECK( cmd.GetArg( 1 ).s == " 2" ); // non-first args are NOT trimmed

	Command empty;
	CHECK( empty.GetName() == "" );
}

TEST_CASE( "Command::GetArg past the end returns an empty Arg", "[Command]" )
{
	Command cmd;
	cmd.Load( "a,b" );
	CHECK( cmd.GetArg( 0 ).s == "a" );
	CHECK( cmd.GetArg( 1 ).s == "b" );
	CHECK( cmd.GetArg( 2 ).s == "" );
	CHECK( cmd.GetArg( 999 ).s == "" );
}

TEST_CASE( "ParseCommands: empty input -> no commands; a trailing ';' is ignored", "[Command]" )
{
	CHECK( ParseCommands( "" ).v.empty() );
	CHECK( ParseCommands( ";" ).v.empty() );

	const Commands c = ParseCommands( "a,1;b,2;" );
	CHECK( c.v.size() == 2 );
}

TEST_CASE( "ParseCommands (non-legacy) honours quotes so a ';' inside them is not a separator", "[Command]" )
{
	const Commands c = ParseCommands( "settext,\"one;two\"" );
	REQUIRE( c.v.size() == 1 );
	CHECK( c.v[0].GetName() == "settext" );
	// The quoted run survived the ';' split as a single arg (the quote
	// characters themselves are kept by Command::Load's plain ',' split).
	REQUIRE( c.v[0].m_vsArgs.size() == 2 );
	CHECK( c.v[0].m_vsArgs[1] == "\"one;two\"" );

	// Legacy mode splits on ';' blindly -> the quoted ';' breaks it up.
	Commands legacy;
	ParseCommands( "settext,\"one;two\"", legacy, true );
	CHECK( legacy.v.size() == 2 );
}
