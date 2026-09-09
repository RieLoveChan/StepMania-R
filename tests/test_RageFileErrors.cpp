// Characterization tests for mid-stream I/O error propagation through
// RageFile's buffering layer, and through IniFile. Salvage of
// src/tests/test_file_errors.cpp (backlog item 17) -- that file is not
// built by any target.
//
// A tiny in-test VFS driver ("ERRTEST") serves one file from a
// std::string and, when g_BytesUntilError >= 0, fails the read/write/
// flush that would cross that byte count (SetError("Fake error"),
// return -1). This is the modern-interface (ReadInternal/WriteInternal/
// FlushInternal) port of the 2004-era RageFileDriverTest.
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageFile.h"
#include "RageFileDriver.h"
#include "RageUtil_FileDB.h"
#include "IniFile.h"

#include "catch_amalgamated.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>

namespace
{
	std::string g_Payload;      // the "file" contents
	int g_BytesUntilError = -1; // -1 = never error; else fail the op that crosses this many bytes

	const char *const kFilename = "thefile";

	class ErrObj : public RageFileObj
	{
	public:
		ErrObj() : m_iPos( 0 ) {}
		ErrObj( const ErrObj &cpy ) : RageFileObj( cpy ), m_iPos( cpy.m_iPos ) {}

		int GetFileSize() const override { return static_cast<int>( g_Payload.size() ); }
		RageFileObj *Copy() const override { return new ErrObj( *this ); }

	protected:
		int SeekInternal( int iOffset ) override
		{
			m_iPos = std::clamp( iOffset, 0, static_cast<int>( g_Payload.size() ) );
			return m_iPos;
		}

		int ReadInternal( void *pBuf, std::size_t iBytes ) override
		{
			iBytes = std::min( iBytes, g_Payload.size() - static_cast<std::size_t>( m_iPos ) );
			if( FailsFor( static_cast<int>( iBytes ) ) )
				return -1;
			memcpy( pBuf, g_Payload.data() + m_iPos, iBytes );
			m_iPos += static_cast<int>( iBytes );
			return static_cast<int>( iBytes );
		}

		int WriteInternal( const void * /* pBuf */, std::size_t iBytes ) override
		{
			if( FailsFor( static_cast<int>( iBytes ) ) )
				return -1;
			return static_cast<int>( iBytes ); // writes go to the bit bucket
		}

		int FlushInternal() override
		{
			if( g_BytesUntilError != -1 )
			{
				SetError( "Fake error" );
				g_BytesUntilError = -1;
				return -1;
			}
			return 0;
		}

	private:
		// True (and arms the error) if this op would cross the threshold.
		bool FailsFor( int iBytes )
		{
			if( g_BytesUntilError == -1 )
				return false;
			if( iBytes > g_BytesUntilError )
			{
				SetError( "Fake error" );
				g_BytesUntilError = -1;
				return true;
			}
			g_BytesUntilError -= iBytes;
			return false;
		}

		int m_iPos;
	};

	class ErrDriver : public RageFileDriver
	{
	public:
		explicit ErrDriver( const RString & /* sRoot */ )
			: RageFileDriver( new FilenameDB )
		{
			FDB->AddFile( kFilename, 0, 0 );
		}

		RageFileBasic *Open( const RString &sPath, int /* iMode */, int &iErr ) override
		{
			RString s = sPath;
			while( s.Left( 1 ) == "/" )
				s.erase( 0, 1 );
			if( s != kFilename )
			{
				iErr = ENOENT;
				return nullptr;
			}
			return new ErrObj;
		}
	};

	struct ErrDriverEntry : public FileDriverEntry
	{
		ErrDriverEntry() : FileDriverEntry( "ERRTEST" ) {}
		RageFileDriver *Create( const RString &sRoot ) const override { return new ErrDriver( sRoot ); }
	};
	const ErrDriverEntry g_ErrDriverEntry; // self-registers with MakeFileDriver

	// Mount "ERRTEST" at /errtest once per run.
	void MountErr()
	{
		EngineTestEnv::Require();
		static bool s_bMounted = false;
		if( !s_bMounted )
		{
			REQUIRE( FILEMAN->Mount( "ERRTEST", ".", "/errtest" ) );
			s_bMounted = true;
		}
		g_BytesUntilError = -1;
	}
}

TEST_CASE( "ERRTEST driver sanity: a clean read of the whole file", "[RageFile][error]" )
{
	MountErr();
	g_Payload = "hello world";

	RageFile f;
	REQUIRE( f.Open( "/errtest/thefile", RageFile::READ ) );
	CHECK( f.GetFileSize() == 11 );

	RString s;
	CHECK( f.Read( s, 11 ) == 11 );
	CHECK( s == "hello world" );
}

TEST_CASE( "RageFile::Read surfaces a mid-stream driver error as -1 with GetError set", "[RageFile][error]" )
{
	MountErr();
	g_Payload = "hello world";
	g_BytesUntilError = 5; // the read that would take us past byte 5 fails

	RageFile f;
	REQUIRE( f.Open( "/errtest/thefile", RageFile::READ ) );

	RString s;
	CHECK( f.Read( s, 5 ) == 5 );
	CHECK( s == "hello" );

	CHECK( f.Read( s, 5 ) == -1 );
	CHECK( f.GetError() == "Fake error" );
}

TEST_CASE( "RageFile::Write surfaces a mid-stream driver error as -1 with GetError set", "[RageFile][error]" )
{
	MountErr();
	g_Payload.clear();
	g_BytesUntilError = 5;

	// RageFileObj does not buffer writes unless EnableWriteBuffering() is
	// called, so each Write() hits the driver directly.
	RageFile f;
	REQUIRE( f.Open( "/errtest/thefile", RageFile::WRITE ) );

	CHECK( f.Write( "test", 4 ) == 4 );

	const int ret = f.Write( "ing", 3 ); // crosses byte 5
	CHECK( ret == -1 );
	CHECK( f.GetError() == "Fake error" );
}

TEST_CASE( "RageFile::Flush propagates the driver's FlushInternal error", "[RageFile][error]" )
{
	MountErr();
	g_Payload.clear();
	g_BytesUntilError = 0; // any flush errors

	RageFile f;
	REQUIRE( f.Open( "/errtest/thefile", RageFile::WRITE ) );

	const int flushed = f.Flush();
	CHECK( flushed == -1 );
	CHECK( f.GetError() == "Fake error" );
}

TEST_CASE( "IniFile::ReadFile propagates a driver read error", "[RageFile][error][ini]" )
{
	MountErr();

	SECTION( "clean read parses" )
	{
		g_Payload = "[test]\nabc=def\n";
		g_BytesUntilError = -1;

		IniFile ini;
		REQUIRE( ini.ReadFile( "/errtest/thefile" ) );
		RString v;
		CHECK( ini.GetValue( "test", "abc", v ) );
		CHECK( v == "def" );
	}

	SECTION( "error mid-read -> ReadFile fails, error surfaced" )
	{
		g_Payload = "[test]\nabc=def\n";
		g_BytesUntilError = 5;

		IniFile ini;
		CHECK_FALSE( ini.ReadFile( "/errtest/thefile" ) );
		CHECK( ini.GetError() == "Fake error" );
	}
}

TEST_CASE( "IniFile::WriteFile propagates a driver write/flush error", "[RageFile][error][ini]" )
{
	MountErr();
	g_Payload.clear();
	g_BytesUntilError = 5;

	IniFile ini;
	ini.SetValue( "foo", "bar", RString( "baz" ) );
	CHECK_FALSE( ini.WriteFile( "/errtest/thefile" ) );
	CHECK( ini.GetError() == "Fake error" );
}
