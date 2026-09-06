// Characterization tests for RageFile -- open / read / write / seek /
// tell / getline / EOF -- exercised end-to-end through FILEMAN's
// in-memory "mem" driver (mounted at /@mem by the RageFileManager ctor).
// No committed fixtures: each test writes its own file to /@mem and
// reads it back. This is the salvage of the intent of the 2004-era
// src/tests/test_file_readers.cpp (backlog item 17), now that
// EngineTestEnv provides a live FILEMAN.
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageFile.h"

#include "catch_amalgamated.hpp"

#include <string>

namespace
{
	// Fill with bytes 0,1,2,...,255,0,1,... so position is self-checking.
	std::string RampBytes( int n )
	{
		std::string s;
		s.reserve( n );
		for( int i = 0; i < n; ++i )
			s.push_back( static_cast<char>( i & 0xFF ) );
		return s;
	}

	// Write `data` to a /@mem path and return the path.
	RString WriteMem( const char *name, const std::string &data )
	{
		RString path = RString( "/@mem/" ) + name;
		RageFile w;
		REQUIRE( w.Open( path, RageFile::WRITE ) );
		REQUIRE( w.Write( data.data(), data.size() ) == static_cast<int>( data.size() ) );
		w.Close();
		return path;
	}
}

TEST_CASE( "RageFile binary round-trip: size, full read, byte-exact", "[RageFile][mem][binary]" )
{
	EngineTestEnv::Require();
	const std::string data = RampBytes( 5000 );
	const RString path = WriteMem( "rf_binary.bin", data );

	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );
	CHECK( r.GetFileSize() == 5000 );

	RString buf;
	CHECK( r.Read( buf, 5000 ) == 5000 );
	CHECK( buf.size() == 5000u );
	CHECK( std::string( buf.data(), buf.size() ) == data );
	CHECK( r.Tell() == 5000 );

	// Characterization: reading *exactly* to the end does not trip EOF --
	// m_bEOF is only set by a read that comes back with 0 bytes
	// (RageFileObj::Read). The next read does it.
	CHECK_FALSE( r.AtEOF() );
	char tail[8];
	CHECK( r.Read( tail, sizeof( tail ) ) == 0 );
	CHECK( r.AtEOF() );
}

TEST_CASE( "RageFile short read at EOF: nonzero-then-zero, EOF trips on the zero read", "[RageFile][mem][eof]" )
{
	EngineTestEnv::Require();
	const RString path = WriteMem( "rf_short.bin", RampBytes( 100 ) );

	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );

	char buf[256];
	// Asked 256, file has 100 -> short read of 100, but NOT EOF yet.
	CHECK( r.Read( buf, sizeof( buf ) ) == 100 );
	CHECK_FALSE( r.AtEOF() );
	// The read that comes back with 0 is the one that sets EOF.
	CHECK( r.Read( buf, sizeof( buf ) ) == 0 );
	CHECK( r.AtEOF() );
}

TEST_CASE( "RageFile Seek(absolute) then Tell and partial Read", "[RageFile][mem][seek]" )
{
	EngineTestEnv::Require();
	const std::string data = RampBytes( 1000 );
	const RString path = WriteMem( "rf_seek.bin", data );

	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );

	CHECK( r.Seek( 256 ) == 256 );
	CHECK( r.Tell() == 256 );

	RString buf;
	CHECK( r.Read( buf, 16 ) == 16 );
	// byte at offset 256 is 256 & 0xFF == 0, then 1, 2, ...
	CHECK( static_cast<unsigned char>( buf[0] ) == 0 );
	CHECK( static_cast<unsigned char>( buf[1] ) == 1 );
	CHECK( r.Tell() == 256 + 16 );

	// Seek back to start.
	CHECK( r.Seek( 0 ) == 0 );
	CHECK( r.Tell() == 0 );
	CHECK_FALSE( r.AtEOF() );
}

TEST_CASE( "RageFile Seek past end clamps to the file size", "[RageFile][mem][seek]" )
{
	EngineTestEnv::Require();
	const RString path = WriteMem( "rf_seekend.bin", RampBytes( 400 ) );

	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );

	// Characterization: seeking beyond EOF lands at the file size.
	CHECK( r.Seek( 99999 ) == 400 );
	CHECK( r.Tell() == 400 );

	char buf[8];
	CHECK( r.Read( buf, sizeof( buf ) ) == 0 );
}

TEST_CASE( "RageFile GetLine strips the newline and AtEOF trips after the last line", "[RageFile][mem][text]" )
{
	EngineTestEnv::Require();
	const RString path = WriteMem( "rf_text.txt", "alpha\nbravo\ncharlie\n" );

	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );

	RString line;
	CHECK( r.GetLine( line ) > 0 );
	CHECK( line == "alpha" );
	CHECK( r.GetLine( line ) > 0 );
	CHECK( line == "bravo" );
	CHECK( r.GetLine( line ) > 0 );
	CHECK( line == "charlie" );
	// Past the last newline: no more data.
	CHECK( r.GetLine( line ) == 0 );
	CHECK( r.AtEOF() );
}

TEST_CASE( "RageFile GetLine returns the final unterminated line", "[RageFile][mem][text]" )
{
	EngineTestEnv::Require();
	const RString path = WriteMem( "rf_text_noeol.txt", "one\ntwo\nthree" );

	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );

	RString line;
	CHECK( r.GetLine( line ) > 0 );
	CHECK( line == "one" );
	CHECK( r.GetLine( line ) > 0 );
	CHECK( line == "two" );
	CHECK( r.GetLine( line ) > 0 );
	CHECK( line == "three" );
	CHECK( r.GetLine( line ) == 0 );
}

TEST_CASE( "RageFile::Open on a missing path fails and sets GetError", "[RageFile][error]" )
{
	EngineTestEnv::Require();

	RageFile r;
	CHECK_FALSE( r.Open( "/@mem/does-not-exist.bin", RageFile::READ ) );
	CHECK_FALSE( r.GetError().empty() );
}

TEST_CASE( "RageFile Read with default bytes=-1 reads the rest of the file", "[RageFile][mem][binary]" )
{
	EngineTestEnv::Require();
	const std::string data = RampBytes( 777 );
	const RString path = WriteMem( "rf_readall.bin", data );

	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );
	CHECK( r.Seek( 100 ) == 100 );

	RString rest;
	CHECK( r.Read( rest ) == 677 );
	CHECK( rest.size() == 677u );
	CHECK( static_cast<unsigned char>( rest[0] ) == ( 100 & 0xFF ) );
	CHECK( r.AtEOF() );
}
