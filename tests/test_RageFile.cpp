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

#include <algorithm>
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

// The RageFileObj read buffer is BSIZE = 1024. GetLine has hand-rolled
// logic for a line/newline that straddles a buffer refill (including the
// "\r\n split across the boundary" hack). Sweep line lengths across 1024
// and 2048, both Unix "\n" and DOS "\r\n". Salvage of test_file_readers.cpp
// TestText() (its buffer was 256; here it is 1024).
TEST_CASE( "RageFile GetLine across the 1024-byte read-buffer boundary", "[RageFile][mem][text]" )
{
	EngineTestEnv::Require();

	const int len = GENERATE( 1, 1022, 1023, 1024, 1025, 1026, 2046, 2047, 2048, 2049 );
	const bool dos = GENERATE( false, true );
	CAPTURE( len, dos );

	const char *eol = dos ? "\r\n" : "\n";
	const int kLines = 6;

	// Line i is 'A'+i repeated `len` times, so mistakes in stripping /
	// re-adding \r show up as a wrong char count or a stray \r.
	std::string file;
	for( int i = 0; i < kLines; ++i )
	{
		file.append( static_cast<std::size_t>( len ), static_cast<char>( 'A' + i ) );
		file += eol;
	}
	// A final line with no terminator.
	file += "tail";

	const RString path = WriteMem( "rf_boundary.txt", file );
	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );

	RString line;
	for( int i = 0; i < kLines; ++i )
	{
		CAPTURE( i );
		REQUIRE( r.GetLine( line ) > 0 );
		REQUIRE( line.size() == static_cast<std::size_t>( len ) );
		CHECK( line[0] == static_cast<char>( 'A' + i ) );
		CHECK( line[line.size() - 1] == static_cast<char>( 'A' + i ) );
		CHECK( line.find( '\r' ) == RString::npos ); // \r must be stripped, not just \n
	}
	REQUIRE( r.GetLine( line ) > 0 );
	CHECK( line == "tail" );
	CHECK( r.GetLine( line ) == 0 );
	CHECK( r.AtEOF() );
}

// GetLine strips a trailing \r only when it immediately precedes the \n.
// A bare \r in the middle of a line is data.
TEST_CASE( "RageFile GetLine keeps a bare \\r that is not part of \\r\\n", "[RageFile][mem][text]" )
{
	EngineTestEnv::Require();
	const RString path = WriteMem( "rf_bare_cr.txt", "a\rb\nc\r\nd\n" );

	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );

	RString line;
	REQUIRE( r.GetLine( line ) > 0 );
	CHECK( line == RString( "a\rb" ) );   // interior \r survives
	REQUIRE( r.GetLine( line ) > 0 );
	CHECK( line == "c" );                 // trailing \r before \n stripped
	REQUIRE( r.GetLine( line ) > 0 );
	CHECK( line == "d" );
	CHECK( r.GetLine( line ) == 0 );
}

// Text lines, then a raw binary block, then a final unterminated line --
// the read buffer holds a mix and Tell() must stay exact throughout.
TEST_CASE( "RageFile interleaved text + binary block, Tell stays exact", "[RageFile][mem][text][binary]" )
{
	EngineTestEnv::Require();

	std::string file = "first\nsecond\n";
	const std::string block = RampBytes( 4096 );
	file += block;
	file += "last";

	const RString path = WriteMem( "rf_mixed.dat", file );
	RageFile r;
	REQUIRE( r.Open( path, RageFile::READ ) );

	RString line;
	REQUIRE( r.GetLine( line ) > 0 );
	CHECK( line == "first" );
	REQUIRE( r.GetLine( line ) > 0 );
	CHECK( line == "second" );
	CHECK( r.Tell() == 13 ); // "first\n" (6) + "second\n" (7)

	// Read the binary block back in uneven bites (700 + 700 + ... + rem).
	std::string got;
	char buf[1024];
	while( got.size() < block.size() )
	{
		const int want = std::min<int>( 700, static_cast<int>( block.size() - got.size() ) );
		REQUIRE( r.Read( buf, want ) == want );
		got.append( buf, static_cast<std::size_t>( want ) );
	}
	CHECK( got == block );
	CHECK( r.Tell() == 13 + 4096 );

	REQUIRE( r.GetLine( line ) > 0 );
	CHECK( line == "last" );
	CHECK( r.GetLine( line ) == 0 );
	CHECK( r.AtEOF() );
}
