// Characterization tests for the raw-deflate stream filters
// RageFileObjDeflate (compress) and RageFileObjInflate (decompress),
// layered over an in-memory RageFileObjMem. This is the salvage of
// TestDeflate() from the 2004-era src/tests/test_file_readers.cpp
// (backlog item 17) -- that file is not built by any target; its intent
// lives here now.
//
// RageFileObjDeflate/Inflate are the zlib "deflate" layer used for
// gzip'd assets and compressed caches. They had no test coverage.
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md and
// DocsAgents/playbooks/add-characterization-test.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageFileDriverMemory.h"
#include "RageFileDriverDeflate.h"

#include "catch_amalgamated.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace
{
	// Deterministic pseudo-random bytes (no <random> -- keep it portable
	// and reproducible). xorshift32.
	std::string PseudoRandomBytes( std::size_t n, std::uint32_t seed )
	{
		std::string s;
		s.reserve( n );
		std::uint32_t x = seed ? seed : 1u;
		for( std::size_t i = 0; i < n; ++i )
		{
			x ^= x << 13; x ^= x >> 17; x ^= x << 5;
			s.push_back( static_cast<char>( x & 0xFF ) );
		}
		return s;
	}

	std::string RepeatUntil( const char *unit, std::size_t atLeast )
	{
		std::string s;
		while( s.size() < atLeast )
			s += unit;
		return s;
	}

	// Compress `plain` into a fresh RageFileObjMem. Returns the raw
	// compressed bytes; `crcOut` gets the deflate side's CRC32 (computed
	// over the *uncompressed input* as it is written).
	RString Deflate( const std::string &plain, std::uint32_t &crcOut, int iWriteChunk = 4096 )
	{
		RageFileObjMem mem;
		{
			RageFileObjDeflate defl( &mem );
			defl.EnableCRC32();

			std::size_t off = 0;
			while( off < plain.size() )
			{
				const int chunk = std::min<int>( iWriteChunk, static_cast<int>( plain.size() - off ) );
				REQUIRE( defl.Write( plain.data() + off, chunk ) == chunk );
				off += static_cast<std::size_t>( chunk );
			}

			crcOut = 0;
			REQUIRE( defl.GetCRC32( &crcOut ) );
			// NOTE: no explicit Flush() -- RageFileObjDeflate::FlushInternal
			// emits Z_FINISH, which ENDS the deflate stream, and the dtor
			// calls FlushInternal too. Calling Flush() ourselves here would
			// finish the stream twice (second deflate(Z_FINISH) -> error).
			// Let the scoped dtor do the single finish.
		}
		return mem.GetString();
	}

	// Inflate `compressed` (known original size `iPlainSize`), reading in
	// `iReadChunk`-byte bites. Returns the decompressed bytes; `crcOut`
	// gets the inflate side's CRC32 (over the decompressed output).
	std::string Inflate( const RString &compressed, int iPlainSize, int iReadChunk, std::uint32_t &crcOut )
	{
		RageFileObjMem mem;
		mem.PutString( compressed );
		mem.Seek( 0 );

		RageFileObjInflate infl( &mem, iPlainSize );
		infl.EnableCRC32();

		std::string out;
		out.reserve( static_cast<std::size_t>( iPlainSize ) );
		while( static_cast<int>( out.size() ) < iPlainSize )
		{
			RString buf; // fresh each call: RageFileObj::Read(RString&,int)
			             // trims to the count it read but does not clear first.
			const int got = infl.Read( buf, iReadChunk );
			if( got <= 0 )
				break; // premature EOF -- caller checks size
			out.append( buf.data(), static_cast<std::size_t>( got ) );
		}

		crcOut = 0;
		REQUIRE( infl.GetCRC32( &crcOut ) );
		return out;
	}

	// Full round-trip assertion at several read block sizes.
	void CheckRoundTrip( const std::string &plain )
	{
		std::uint32_t crcIn = 0;
		const RString compressed = Deflate( plain, crcIn );

		const int n = static_cast<int>( plain.size() );
		const int sizes[] = { 7, 1023, 4096, std::max( 1, n ) };
		for( int rc : sizes )
		{
			std::uint32_t crcOut = 0;
			const std::string got = Inflate( compressed, n, rc, crcOut );
			CAPTURE( rc, n );
			CHECK( got.size() == plain.size() );
			CHECK( got == plain );
			CHECK( crcOut == crcIn );
		}
	}
}

TEST_CASE( "RageFileObjDeflate/Inflate round-trip: highly compressible", "[RageFile][deflate]" )
{
	EngineTestEnv::Require();
	CheckRoundTrip( std::string( 200000, 'A' ) );
}

TEST_CASE( "RageFileObjDeflate/Inflate round-trip: repeating phrase", "[RageFile][deflate]" )
{
	EngineTestEnv::Require();
	CheckRoundTrip( RepeatUntil( "test foo bar fah ", 300000 ) );
}

TEST_CASE( "RageFileObjDeflate/Inflate round-trip: incompressible-ish", "[RageFile][deflate]" )
{
	EngineTestEnv::Require();
	CheckRoundTrip( PseudoRandomBytes( 250000, 0xC0FFEEu ) );
}

TEST_CASE( "RageFileObjDeflate/Inflate round-trip: tiny payload", "[RageFile][deflate]" )
{
	EngineTestEnv::Require();
	CheckRoundTrip( std::string( "x" ) );
}

TEST_CASE( "RageFileObjDeflate actually shrinks compressible input", "[RageFile][deflate]" )
{
	EngineTestEnv::Require();

	const std::string plain( 100000, 'Z' );
	std::uint32_t crc = 0;
	const RString compressed = Deflate( plain, crc );

	// zlib on a 100k run of one byte is well under 1/10th the size.
	CHECK( compressed.size() < plain.size() / 10 );
	CHECK( !compressed.empty() );
}

TEST_CASE( "RageFileObjDeflate: write chunk size changes nothing observable", "[RageFile][deflate]" )
{
	EngineTestEnv::Require();

	// Keep this one small: iWriteChunk=1 does one REQUIRE per byte.
	const std::string plain = PseudoRandomBytes( 3000, 0x1234u );

	std::uint32_t crcA = 0, crcB = 0;
	const RString a = Deflate( plain, crcA, /*iWriteChunk=*/1 );
	const RString b = Deflate( plain, crcB, /*iWriteChunk=*/65536 );

	CHECK( crcA == crcB ); // CRC is over the input, independent of chunking

	std::uint32_t ra = 0, rb = 0;
	CHECK( Inflate( a, static_cast<int>( plain.size() ), 4096, ra ) == plain );
	CHECK( Inflate( b, static_cast<int>( plain.size() ), 4096, rb ) == plain );
	CHECK( ra == crcA );
	CHECK( rb == crcB );
}

TEST_CASE( "RageFileObjInflate never returns clean, complete data from a corrupted stream", "[RageFile][deflate][error]" )
{
	EngineTestEnv::Require();

	// Mixed content so the compressed form is not trivially tiny.
	std::string plain = PseudoRandomBytes( 20000, 0xABCDu ) + std::string( 20000, 'q' );
	std::uint32_t crc = 0;
	RString compressed = Deflate( plain, crc );
	REQUIRE( compressed.size() > 200 );

	// Smash a chunk a little way in. Depending on which part of the
	// bitstream is hit, zlib may raise Z_DATA_ERROR (-> Read() returns
	// -1 with GetError() set) OR decode a wrong/short amount. What must
	// NOT happen: the full original payload comes back intact.
	for( int i = 40; i < 120; ++i )
		compressed[i] = static_cast<char>( compressed[i] ^ 0xFF );

	RageFileObjMem mem;
	mem.PutString( compressed );
	mem.Seek( 0 );
	RageFileObjInflate infl( &mem, static_cast<int>( plain.size() ) );
	infl.EnableCRC32();

	std::string out;
	char buf[8192];
	int ret = 0;
	for( ;; )
	{
		ret = infl.Read( buf, sizeof( buf ) );
		if( ret <= 0 )
			break;
		out.append( buf, static_cast<std::size_t>( ret ) );
	}

	const bool hardError = ( ret == -1 );
	CAPTURE( hardError, out.size(), infl.GetError() );
	if( hardError )
	{
		CHECK_FALSE( infl.GetError().empty() );
	}
	else
	{
		// No hard error -> the data must still be detectably bad:
		// wrong length, or right length but wrong bytes (CRC mismatch).
		bool bad = ( out.size() != plain.size() );
		if( !bad )
		{
			std::uint32_t got = 0;
			REQUIRE( infl.GetCRC32( &got ) );
			bad = ( got != crc ) || ( out != plain );
		}
		CHECK( bad );
	}
}

TEST_CASE( "RageFileObjInflate on a truncated stream does not over-report bytes", "[RageFile][deflate][error]" )
{
	EngineTestEnv::Require();

	const std::string plain( 60000, 'w' );
	std::uint32_t crc = 0;
	const RString whole = Deflate( plain, crc );
	REQUIRE( whole.size() > 40 );

	const RString truncated = whole.Left( static_cast<int>( whole.size() ) - 20 );

	RageFileObjMem mem;
	mem.PutString( truncated );
	mem.Seek( 0 );
	RageFileObjInflate infl( &mem, static_cast<int>( plain.size() ) );

	std::string out;
	char buf[8192];
	int ret = 0;
	for( ;; )
	{
		ret = infl.Read( buf, sizeof( buf ) );
		if( ret <= 0 )
			break;
		out.append( buf, static_cast<std::size_t>( ret ) );
	}
	// Either a hard error, or a short read -- but never MORE than the
	// real payload, and whatever we did get is a correct prefix.
	CHECK( out.size() < plain.size() );
	CHECK( plain.compare( 0, out.size(), out ) == 0 );
}

TEST_CASE( "RageFileObjInflate::Seek rewinds / repositions the decompressed stream", "[RageFile][deflate][seek]" )
{
	EngineTestEnv::Require();

	// bytes 0,1,2,...,255,0,... so a read is position-self-checking
	std::string plain;
	plain.reserve( 50000 );
	for( int i = 0; i < 50000; ++i )
		plain.push_back( static_cast<char>( i & 0xFF ) );

	std::uint32_t crc = 0;
	const RString compressed = Deflate( plain, crc );

	RageFileObjMem mem;
	mem.PutString( compressed );
	mem.Seek( 0 );
	RageFileObjInflate infl( &mem, static_cast<int>( plain.size() ) );

	char buf[1000];
	REQUIRE( infl.Read( buf, sizeof( buf ) ) == static_cast<int>( sizeof( buf ) ) );
	CHECK( std::string( buf, sizeof( buf ) ) == plain.substr( 0, sizeof( buf ) ) );

	// Seek back to the top and re-read -- same bytes.
	CHECK( infl.Seek( 0 ) == 0 );
	REQUIRE( infl.Read( buf, sizeof( buf ) ) == static_cast<int>( sizeof( buf ) ) );
	CHECK( std::string( buf, sizeof( buf ) ) == plain.substr( 0, sizeof( buf ) ) );

	// Seek forward into the middle.
	const int mid = 25000;
	CHECK( infl.Seek( mid ) == mid );
	REQUIRE( infl.Read( buf, 256 ) == 256 );
	CHECK( std::string( buf, 256 ) == plain.substr( mid, 256 ) );
}
