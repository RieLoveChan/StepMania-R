// Characterization tests for IniFile -- the .ini reader/writer behind
// Preferences.ini, keymaps, Static.ini, the theme-metrics fallback and
// the legacy [Char Widths] -> [main] fixup. Load-bearing and previously
// untested. Parsed from strings written to FILEMAN's writable /@mem
// mount; no committed fixtures. Needs EngineTestEnv for FILEMAN + LOG
// (IniFile::ReadFile logs on malformed lines).
//
// Pins CURRENT behaviour, bug-for-bug -- including the quirk that the
// value keeps the whitespace right after '=' while the key name is
// trimmed. See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "IniFile.h"
#include "RageFile.h"

#include "catch_amalgamated.hpp"

#include <string>

namespace
{
	// Write `text` to a /@mem path and read it into `ini`.
	void LoadIni( IniFile &ini, const char *name, const std::string &text )
	{
		const RString path = RString( "/@mem/" ) + name;
		{
			RageFile w;
			REQUIRE( w.Open( path, RageFile::WRITE ) );
			REQUIRE( w.Write( text.data(), text.size() ) == static_cast<int>( text.size() ) );
			w.Close();
		}
		REQUIRE( ini.ReadFile( path ) );
	}

	RString Get( const IniFile &ini, const char *sec, const char *key )
	{
		RString out;
		ini.GetValue( sec, key, out );
		return out;
	}
}

TEST_CASE( "IniFile reads [section] / key=value", "[IniFile]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "basic.ini", "[sound]\nvolume=80\ndevice=default\n\n[input]\nlag=0.02\n" );

	CHECK( Get( ini, "sound", "volume" ) == "80" );
	CHECK( Get( ini, "sound", "device" ) == "default" );
	CHECK( Get( ini, "input", "lag" ) == "0.02" );
}

TEST_CASE( "IniFile trims the key name but keeps whitespace after '='", "[IniFile][quirk]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "ws.ini", "[s]\n  spaced key  = value with spaces \n" );

	// Key name: trimmed on both sides.
	CHECK( Get( ini, "s", "spaced key" ) == " value with spaces " );
	// The untrimmed forms are not keys.
	CHECK( Get( ini, "s", "  spaced key  " ) == "" );
}

TEST_CASE( "IniFile skips comment lines (; # // --) but not a lone / or -", "[IniFile][comment]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "cmt.ini",
		"[s]\n"
		"; semicolon comment = ignored\n"
		"# hash comment = ignored\n"
		"// slash comment = ignored\n"
		"-- dash comment = ignored\n"
		"real=1\n"
		"/lonely=2\n"      // single '/' -> treated as key=value
		"-lonelydash=3\n" );

	CHECK( Get( ini, "s", "real" ) == "1" );
	CHECK( Get( ini, "s", "semicolon comment" ) == "" );
	CHECK( Get( ini, "s", "hash comment" ) == "" );
	// A lone leading '/' or '-' is NOT a comment -> parsed as a value.
	CHECK( Get( ini, "s", "/lonely" ) == "2" );
	CHECK( Get( ini, "s", "-lonelydash" ) == "3" );
}

TEST_CASE( "IniFile drops key=value that appears before any [section]", "[IniFile][edge]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "nosec.ini", "orphan=1\n[s]\nkept=2\n" );

	CHECK( Get( ini, "s", "kept" ) == "2" );
	// "orphan" had no section -> silently dropped, no [<empty>] section.
	CHECK( ini.GetChild( "" ) == nullptr );
}

TEST_CASE( "IniFile joins lines that end with a backslash", "[IniFile][continuation]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "cont.ini", "[s]\nlong=part-one\\\npart-two\\\npart-three\n" );

	CHECK( Get( ini, "s", "long" ) == "part-onepart-twopart-three" );
}

TEST_CASE( "IniFile keeps parsing after a line with no '='", "[IniFile][edge]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "noeq.ini", "[s]\nbefore=1\nthis line has no equals sign\nafter=2\n" );

	CHECK( Get( ini, "s", "before" ) == "1" );
	CHECK( Get( ini, "s", "after" ) == "2" );
}

TEST_CASE( "IniFile merges a repeated [section]", "[IniFile][edge]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "dup.ini", "[s]\na=1\n[other]\nx=9\n[s]\nb=2\n" );

	CHECK( Get( ini, "s", "a" ) == "1" );
	CHECK( Get( ini, "s", "b" ) == "2" );
	CHECK( Get( ini, "other", "x" ) == "9" );
}

TEST_CASE( "IniFile GetValue<T> converts and reports missing keys", "[IniFile]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "typed.ini", "[s]\nnum=42\nflag=1\nreal=3.5\n" );

	int i = 0;
	CHECK( ini.GetValue( "s", "num", i ) );
	CHECK( i == 42 );

	float f = 0.f;
	CHECK( ini.GetValue( "s", "real", f ) );
	CHECK( f == Catch::Approx( 3.5f ) );

	bool b = false;
	CHECK( ini.GetValue( "s", "flag", b ) );
	CHECK( b == true );

	int missing = -1;
	CHECK_FALSE( ini.GetValue( "s", "nope", missing ) );
	CHECK_FALSE( ini.GetValue( "no-section", "num", missing ) );
	CHECK( missing == -1 ); // untouched on failure
}

TEST_CASE( "IniFile WriteFile / ReadFile round-trips sections and values", "[IniFile][roundtrip]" )
{
	EngineTestEnv::Require();
	IniFile a;
	a.SetValue( "sound", "volume", RString( "70" ) );
	a.SetValue( "sound", "driver", RString( "wasapi" ) );
	a.SetValue( "video", "vsync", RString( "1" ) );
	REQUIRE( a.WriteFile( RString( "/@mem/rt.ini" ) ) );

	IniFile b;
	REQUIRE( b.ReadFile( RString( "/@mem/rt.ini" ) ) );
	CHECK( Get( b, "sound", "volume" ) == "70" );
	CHECK( Get( b, "sound", "driver" ) == "wasapi" );
	CHECK( Get( b, "video", "vsync" ) == "1" );
}

TEST_CASE( "IniFile DeleteKey / DeleteValue", "[IniFile]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "del.ini", "[s]\na=1\nb=2\n[t]\nx=9\n" );

	CHECK( ini.DeleteValue( "s", "a" ) );
	CHECK( Get( ini, "s", "a" ) == "" );
	CHECK( Get( ini, "s", "b" ) == "2" );
	CHECK_FALSE( ini.DeleteValue( "s", "a" ) );        // already gone
	CHECK_FALSE( ini.DeleteValue( "missing", "a" ) );  // no such section

	CHECK( ini.DeleteKey( "t" ) );
	CHECK( ini.GetChild( "t" ) == nullptr );
	CHECK_FALSE( ini.DeleteKey( "t" ) );
}

TEST_CASE( "IniFile RenameKey moves a section, no-ops on conflict or absence", "[IniFile]" )
{
	EngineTestEnv::Require();
	IniFile ini;
	LoadIni( ini, "ren.ini", "[Char Widths]\n0=6\n1=6\n[keep]\nk=1\n" );

	// legacy-alias fixup: [Char Widths] -> [main]
	CHECK( ini.RenameKey( "Char Widths", "main" ) );
	CHECK( ini.GetChild( "Char Widths" ) == nullptr );
	CHECK( Get( ini, "main", "0" ) == "6" );

	// target already exists -> false, nothing moves.
	CHECK_FALSE( ini.RenameKey( "keep", "main" ) );
	CHECK( Get( ini, "keep", "k" ) == "1" );

	// source absent -> false (rename-if-present is the normal case).
	CHECK_FALSE( ini.RenameKey( "does-not-exist", "whatever" ) );
}
