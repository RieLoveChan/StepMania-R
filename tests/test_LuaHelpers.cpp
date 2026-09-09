// Characterization tests for LuaHelpers::Push / FromStack / Pop and
// RunExpression -- the C++ <-> Lua value bridge used by every themed
// metric, actor command and Lua binding. LUA is brought up by
// EngineTestEnv.
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "LuaManager.h"

#include "catch_amalgamated.hpp"

namespace
{
	// RAII around LUA->Get() / LUA->Release() so a failed CHECK still
	// returns the Lua state.
	struct LuaGuard
	{
		Lua *L;
		int base;
		LuaGuard() : L( LUA->Get() ), base( lua_gettop( L ) ) {}
		~LuaGuard() { LUA->Release( L ); }
	};
}

TEST_CASE( "LuaHelpers Push / Pop round-trip for the scalar types", "[LuaHelpers]" )
{
	EngineTestEnv::Require();
	LuaGuard g;

	{
		LuaHelpers::Push( g.L, 42 );
		int out = 0;
		CHECK( LuaHelpers::Pop( g.L, out ) );
		CHECK( out == 42 );
	}
	{
		LuaHelpers::Push( g.L, 3.5f );
		float out = 0;
		CHECK( LuaHelpers::Pop( g.L, out ) );
		CHECK( out == Catch::Approx( 3.5f ) );
	}
	{
		LuaHelpers::Push( g.L, true );
		bool out = false;
		CHECK( LuaHelpers::Pop( g.L, out ) );
		CHECK( out == true );
	}
	{
		LuaHelpers::Push( g.L, RString( "hello" ) );
		RString out;
		CHECK( LuaHelpers::Pop( g.L, out ) );
		CHECK( out == "hello" );
	}

	// Pop leaves the stack where it started.
	CHECK( lua_gettop( g.L ) == g.base );
}

TEST_CASE( "LuaHelpers::FromStack applies Lua's own coercions", "[LuaHelpers]" )
{
	EngineTestEnv::Require();
	LuaGuard g;

	// A numeric string coerces to a number for FromStack<int>.
	LuaHelpers::Push( g.L, RString( "5" ) );
	int i = -1;
	CHECK( LuaHelpers::FromStack( g.L, i, -1 ) );
	CHECK( i == 5 );
	lua_pop( g.L, 1 );

	// A number coerces to a string for FromStack<RString>.
	LuaHelpers::Push( g.L, 12 );
	RString s;
	CHECK( LuaHelpers::FromStack( g.L, s, -1 ) );
	CHECK( s == "12" );
	lua_pop( g.L, 1 );

	// FromStack<RString> of a non-string / non-number (a bool) fails and
	// clears the out param.
	LuaHelpers::Push( g.L, true );
	s = "stale";
	CHECK_FALSE( LuaHelpers::FromStack( g.L, s, -1 ) );
	CHECK( s == "" );
	lua_pop( g.L, 1 );

	CHECK( lua_gettop( g.L ) == g.base );
}

TEST_CASE( "LuaHelpers::RunExpression evaluates and leaves the result on the stack", "[LuaHelpers]" )
{
	EngineTestEnv::Require();
	LuaGuard g;

	REQUIRE( LuaHelpers::RunExpression( g.L, "1 + 2" ) );
	int n = 0;
	CHECK( LuaHelpers::Pop( g.L, n ) );
	CHECK( n == 3 );

	REQUIRE( LuaHelpers::RunExpression( g.L, "'a' .. 'b' .. 'c'" ) );
	RString s;
	CHECK( LuaHelpers::Pop( g.L, s ) );
	CHECK( s == "abc" );

	CHECK( lua_gettop( g.L ) == g.base );
}

TEST_CASE( "LuaHelpers::RunScript on a syntax error returns false with an error message", "[LuaHelpers]" )
{
	EngineTestEnv::Require();
	LuaGuard g;

	// Call RunScript directly with ReportError=false -- RunExpression
	// forces ReportError=true, which routes through MESSAGEMAN (null in
	// this fixture).
	RString err;
	CHECK_FALSE( LuaHelpers::RunScript( g.L, "this is ((not lua", "test", err, 0, 1, /*ReportError=*/false ) );
	CHECK_FALSE( err.empty() );

	// Characterization: a failed COMPILE leaves one value (the error
	// object) on the stack -- unlike the runtime-error path, which
	// pops it. Clean it up so the guard's Release sees a tidy stack.
	CHECK( lua_gettop( g.L ) == g.base + 1 );
	lua_settop( g.L, g.base );
}
