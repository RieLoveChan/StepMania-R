// Characterization tests for RageLog's level plumbing (ADR 0005 phase 2).
// The write path (files / console / crash staticlog) is --SelfTest
// territory; this covers the pure, testable bits: the LogLevel enum
// ordering and the string <-> enum helpers behind --LogLevel.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageLog.h"

#include "catch_amalgamated.hpp"

TEST_CASE( "RageLog::LogLevel severity ordering is Trace < Debug < Info < Warn < Error", "[RageLog]" )
{
	CHECK( RageLog::LogLevel_Trace < RageLog::LogLevel_Debug );
	CHECK( RageLog::LogLevel_Debug < RageLog::LogLevel_Info );
	CHECK( RageLog::LogLevel_Info  < RageLog::LogLevel_Warn );
	CHECK( RageLog::LogLevel_Warn  < RageLog::LogLevel_Error );
	CHECK( RageLog::NUM_LogLevel == 5 );
}

TEST_CASE( "RageLog::LogLevelFromString parses the five names, case-insensitively", "[RageLog]" )
{
	CHECK( RageLog::LogLevelFromString( "trace" ) == RageLog::LogLevel_Trace );
	CHECK( RageLog::LogLevelFromString( "debug" ) == RageLog::LogLevel_Debug );
	CHECK( RageLog::LogLevelFromString( "info" )  == RageLog::LogLevel_Info );
	CHECK( RageLog::LogLevelFromString( "warn" )  == RageLog::LogLevel_Warn );
	CHECK( RageLog::LogLevelFromString( "error" ) == RageLog::LogLevel_Error );

	CHECK( RageLog::LogLevelFromString( "WARN" )   == RageLog::LogLevel_Warn );
	CHECK( RageLog::LogLevelFromString( "  Info " ) == RageLog::LogLevel_Info );
}

TEST_CASE( "RageLog::LogLevelFromString falls back to Trace on an unknown string", "[RageLog]" )
{
	CHECK( RageLog::LogLevelFromString( "" ) == RageLog::LogLevel_Trace );
	CHECK( RageLog::LogLevelFromString( "verbose" ) == RageLog::LogLevel_Trace );
	CHECK( RageLog::LogLevelFromString( "3" ) == RageLog::LogLevel_Trace );
}

TEST_CASE( "RageLog::LogLevelToString round-trips every level", "[RageLog]" )
{
	for( int i = 0; i < RageLog::NUM_LogLevel; ++i )
	{
		RageLog::LogLevel l = (RageLog::LogLevel)i;
		CHECK( RageLog::LogLevelFromString( RageLog::LogLevelToString( l ) ) == l );
	}
	// Out-of-range clamps to "trace".
	CHECK( RString( RageLog::LogLevelToString( (RageLog::LogLevel)99 ) ) == "trace" );
}

TEST_CASE( "RageLog::SetLogLevel accepts every valid level (no crash / assert)", "[RageLog]" )
{
	EngineTestEnv::Require(); // LOG must exist
	for( int i = 0; i < RageLog::NUM_LogLevel; ++i )
		LOG->SetLogLevel( (RageLog::LogLevel)i );
	LOG->SetLogLevel( (RageLog::LogLevel)-1 );  // clamps to Trace internally
	LOG->SetLogLevel( RageLog::LogLevel_Trace ); // leave it as the default
}
