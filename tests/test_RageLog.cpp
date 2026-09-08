// Characterization tests for RageLog's level / category plumbing
// (ADR 0005 phase 2). The write path (files / console / crash staticlog)
// is --SelfTest territory; this covers the pure, testable bits: the
// enums and the string parsing behind --LogLevel.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageLog.h"

#include "catch_amalgamated.hpp"

TEST_CASE( "RageLog::LogLevel severity ordering: Trace < Debug < Info < Warn < Error < Off", "[RageLog]" )
{
	CHECK( RageLog::LogLevel_Trace < RageLog::LogLevel_Debug );
	CHECK( RageLog::LogLevel_Debug < RageLog::LogLevel_Info );
	CHECK( RageLog::LogLevel_Info  < RageLog::LogLevel_Warn );
	CHECK( RageLog::LogLevel_Warn  < RageLog::LogLevel_Error );
	CHECK( RageLog::LogLevel_Error < RageLog::LogLevel_Off );
	CHECK( RageLog::NUM_LogLevel == 6 );
}

TEST_CASE( "RageLog::LogLevelFromString parses the six names, case-insensitively", "[RageLog]" )
{
	CHECK( RageLog::LogLevelFromString( "trace" ) == RageLog::LogLevel_Trace );
	CHECK( RageLog::LogLevelFromString( "debug" ) == RageLog::LogLevel_Debug );
	CHECK( RageLog::LogLevelFromString( "info" )  == RageLog::LogLevel_Info );
	CHECK( RageLog::LogLevelFromString( "warn" )  == RageLog::LogLevel_Warn );
	CHECK( RageLog::LogLevelFromString( "error" ) == RageLog::LogLevel_Error );
	CHECK( RageLog::LogLevelFromString( "off" )   == RageLog::LogLevel_Off );

	CHECK( RageLog::LogLevelFromString( "WARN" )   == RageLog::LogLevel_Warn );
	CHECK( RageLog::LogLevelFromString( "  Info " ) == RageLog::LogLevel_Info );
	CHECK( RageLog::LogLevelFromString( "verbose" ) == RageLog::LogLevel_Trace ); // unknown -> Trace
}

TEST_CASE( "RageLog::LogLevelToString round-trips every level", "[RageLog]" )
{
	for( int i = 0; i < RageLog::NUM_LogLevel; ++i )
	{
		RageLog::LogLevel l = (RageLog::LogLevel)i;
		CHECK( RageLog::LogLevelFromString( RageLog::LogLevelToString( l ) ) == l );
	}
	CHECK( RString( RageLog::LogLevelToString( (RageLog::LogLevel)99 ) ) == "trace" );
}

TEST_CASE( "Log::Category round-trips through its string name", "[RageLog][category]" )
{
	CHECK( Log::CategoryFromString( "general" ) == Log::General );
	CHECK( Log::CategoryFromString( "gl" )      == Log::Gl );
	CHECK( Log::CategoryFromString( "FONT" )    == Log::Font );
	CHECK( Log::CategoryFromString( " song " )  == Log::Song );
	CHECK( Log::CategoryFromString( "nonesuch" ) == Log::General ); // unknown -> General

	for( int i = 0; i < Log::NUM_Category; ++i )
	{
		Log::Category c = (Log::Category)i;
		CHECK( Log::CategoryFromString( Log::CategoryToString( c ) ) == c );
	}
}

TEST_CASE( "RageLog::SetLogLevelSpec: global level, per-category, and both", "[RageLog][category]" )
{
	EngineTestEnv::Require(); // LOG must exist

	// Bare token -> global; per-category unset -> follows global.
	LOG->SetLogLevelSpec( "warn" );
	CHECK( LOG->GetEffectiveLevel( Log::General ) == RageLog::LogLevel_Warn );
	CHECK( LOG->GetEffectiveLevel( Log::Gl )      == RageLog::LogLevel_Warn );

	// "cat:level" overrides just that category.
	LOG->SetLogLevelSpec( "warn,gl:off,font:trace" );
	CHECK( LOG->GetEffectiveLevel( Log::General ) == RageLog::LogLevel_Warn );
	CHECK( LOG->GetEffectiveLevel( Log::Gl )      == RageLog::LogLevel_Off );
	CHECK( LOG->GetEffectiveLevel( Log::Font )    == RageLog::LogLevel_Trace );
	CHECK( LOG->GetEffectiveLevel( Log::Sound )   == RageLog::LogLevel_Warn ); // untouched -> global

	// The spec is the WHOLE config -- a per-cat-only spec resets the
	// global to the default (trace) and clears other categories.
	LOG->SetLogLevelSpec( "sound:error" );
	CHECK( LOG->GetEffectiveLevel( Log::Sound )   == RageLog::LogLevel_Error );
	CHECK( LOG->GetEffectiveLevel( Log::General ) == RageLog::LogLevel_Trace );
	CHECK( LOG->GetEffectiveLevel( Log::Font )    == RageLog::LogLevel_Trace ); // cleared

	// Unknown category token is ignored (no crash); unknown level -> Trace.
	LOG->SetLogLevelSpec( "boguscat:warn,gl:boguslevel" );
	CHECK( LOG->GetEffectiveLevel( Log::Gl ) == RageLog::LogLevel_Trace );

	// Empty / whitespace spec == all defaults.
	LOG->SetLogLevelSpec( "" );
	CHECK( LOG->GetEffectiveLevel( Log::General ) == RageLog::LogLevel_Trace );
	CHECK( LOG->GetEffectiveLevel( Log::Sound )   == RageLog::LogLevel_Trace );
}

TEST_CASE( "LOG_* macros expand and route without crashing", "[RageLog][macro]" )
{
	EngineTestEnv::Require();
	// Disk/console output is off in the fixture; this just proves the
	// macro layer compiles, captures file:line, and survives every level
	// / a filtered category.
	LOG->SetLogLevelSpec( "gl:off" );
	LOG_TRACE( Log::Font,  "font trace %d", 1 );
	LOG_DEBUG( Log::Song,  "song debug" );
	LOG_INFO(  Log::Actor, "actor info %s", "x" );
	LOG_WARN(  Log::Net,   "net warn" );
	LOG_ERROR( Log::Cache, "cache error %d/%d", 2, 3 );
	LOG_TRACE( Log::Gl,    "this gl line is dropped by gl:off" );
	LOG->SetLogLevelSpec( "" ); // back to all-defaults for the rest of the run
	SUCCEED();
}
