// Characterization tests for RageFileDriverZip -- the read-only ZIP VFS
// driver used by the .smzip / .zip install path (mount an archive, list
// and extract its entries).
//
// The archive is assembled in-memory by a minimal ZIP writer in this
// file (there is no engine-side ZIP *writer* any more -- the old
// CreateZip fork was STORED-only and dead, and miniz's writing APIs are
// compiled out). One STORED entry and one DEFLATED entry (compressed
// with RageFileObjDeflate) are written to /@mem, then read back through
// RageFileDriverZip and compared byte-for-byte.
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageFile.h"
#include "RageFileDriverZip.h"
#include "RageFileDriverMemory.h"
#include "RageFileDriverDeflate.h"

#include "zlib.h" // crc32

#include "catch_amalgamated.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
	void PutLE16( std::string &s, std::uint16_t v )
	{
		s.push_back( static_cast<char>( v & 0xFF ) );
		s.push_back( static_cast<char>( ( v >> 8 ) & 0xFF ) );
	}
	void PutLE32( std::string &s, std::uint32_t v )
	{
		for( int i = 0; i < 4; ++i )
			s.push_back( static_cast<char>( ( v >> ( i * 8 ) ) & 0xFF ) );
	}

	// Raw-deflate `plain` via the engine's own RageFileObjDeflate (the
	// same codec ZIP method 8 needs).
	std::string RawDeflate( const std::string &plain )
	{
		RageFileObjMem mem;
		{
			RageFileObjDeflate defl( &mem );
			REQUIRE( defl.Write( plain.data(), plain.size() ) == static_cast<int>( plain.size() ) );
		} // dtor emits Z_FINISH
		const RString &out = mem.GetString();
		return std::string( out.data(), out.size() );
	}

	struct ZipEntry
	{
		std::string name;
		std::string uncompressed;
		std::string stored;   // bytes actually written into the archive
		std::uint16_t method; // 0 STORED, 8 DEFLATED
		std::uint32_t crc;
		std::uint32_t localHeaderOffset;
	};

	// Build a minimal but valid ZIP from the given entries.
	std::string BuildZip( std::vector<ZipEntry> &entries )
	{
		std::string zip;

		for( ZipEntry &e : entries )
		{
			e.crc = crc32( 0, reinterpret_cast<const Bytef *>( e.uncompressed.data() ),
			               static_cast<uInt>( e.uncompressed.size() ) );
			e.stored = ( e.method == 8 ) ? RawDeflate( e.uncompressed ) : e.uncompressed;
			e.localHeaderOffset = static_cast<std::uint32_t>( zip.size() );

			// Local file header.
			zip += "PK\x03\x04";
			PutLE16( zip, 20 );                 // version needed
			PutLE16( zip, 0 );                  // general purpose flag
			PutLE16( zip, e.method );           // compression method
			PutLE16( zip, 0 );                  // mod time
			PutLE16( zip, 0 );                  // mod date
			PutLE32( zip, e.crc );
			PutLE32( zip, static_cast<std::uint32_t>( e.stored.size() ) );        // compressed size
			PutLE32( zip, static_cast<std::uint32_t>( e.uncompressed.size() ) );  // uncompressed size
			PutLE16( zip, static_cast<std::uint16_t>( e.name.size() ) );
			PutLE16( zip, 0 );                  // extra field length
			zip += e.name;
			zip += e.stored;
		}

		const std::uint32_t centralDirOffset = static_cast<std::uint32_t>( zip.size() );

		for( const ZipEntry &e : entries )
		{
			zip += "PK\x01\x02";
			PutLE16( zip, 20 );  // version made by
			PutLE16( zip, 20 );  // version needed
			PutLE16( zip, 0 );   // general purpose flag
			PutLE16( zip, e.method );
			PutLE16( zip, 0 );   // mod time
			PutLE16( zip, 0 );   // mod date
			PutLE32( zip, e.crc );
			PutLE32( zip, static_cast<std::uint32_t>( e.stored.size() ) );
			PutLE32( zip, static_cast<std::uint32_t>( e.uncompressed.size() ) );
			PutLE16( zip, static_cast<std::uint16_t>( e.name.size() ) );
			PutLE16( zip, 0 );   // extra field length
			PutLE16( zip, 0 );   // file comment length
			PutLE16( zip, 0 );   // disk number start
			PutLE16( zip, 0 );   // internal file attributes
			PutLE32( zip, 0 );   // external file attributes
			PutLE32( zip, e.localHeaderOffset );
			zip += e.name;
		}

		const std::uint32_t centralDirSize =
			static_cast<std::uint32_t>( zip.size() ) - centralDirOffset;

		zip += "PK\x05\x06";
		PutLE16( zip, 0 );  // number of this disk
		PutLE16( zip, 0 );  // disk with central directory
		PutLE16( zip, static_cast<std::uint16_t>( entries.size() ) ); // entries this disk
		PutLE16( zip, static_cast<std::uint16_t>( entries.size() ) ); // total entries
		PutLE32( zip, centralDirSize );
		PutLE32( zip, centralDirOffset );
		PutLE16( zip, 0 );  // comment length

		return zip;
	}

	void WriteMem( const RString &path, const std::string &data )
	{
		RageFile w;
		REQUIRE( w.Open( path, RageFile::WRITE ) );
		REQUIRE( w.Write( data.data(), data.size() ) == static_cast<int>( data.size() ) );
		w.Close();
	}

	std::string ReadAll( RageFileBasic *f )
	{
		std::string out;
		char buf[4096];
		for( ;; )
		{
			const int got = f->Read( buf, sizeof( buf ) );
			if( got <= 0 )
				break;
			out.append( buf, static_cast<std::size_t>( got ) );
		}
		return out;
	}
}

TEST_CASE( "RageFileDriverZip reads STORED and DEFLATED entries byte-for-byte", "[zip]" )
{
	EngineTestEnv::Require();

	std::string compressible( 5000, 'Q' );
	for( int i = 0; i < 5000; i += 20 )
		compressible[i] = static_cast<char>( 'a' + ( i % 26 ) ); // a little entropy

	std::vector<ZipEntry> entries = {
		{ "dir/hello.txt", "Hello, stored world!", "", /*method=*/0, 0, 0 },
		{ "dir/big.txt",   compressible,           "", /*method=*/8, 0, 0 },
	};
	const std::string zipBytes = BuildZip( entries );
	WriteMem( "/@mem/pkg.zip", zipBytes );

	RageFileDriverZip zip;
	REQUIRE( zip.Load( RString( "/@mem/pkg.zip" ) ) );

	for( const ZipEntry &e : entries )
	{
		const RString path = RString( "/" ) + e.name.c_str();
		CAPTURE( e.name );

		const RageFileDriverZip::FileInfo *info = zip.GetFileInfo( path );
		REQUIRE( info != nullptr );
		CHECK( info->m_iUncompressedSize == static_cast<int>( e.uncompressed.size() ) );
		CHECK( static_cast<std::uint32_t>( info->m_iCRC32 ) == e.crc );
		CHECK( info->m_iCompressionMethod ==
		       ( e.method == 8 ? RageFileDriverZip::DEFLATED : RageFileDriverZip::STORED ) );

		int err = 0;
		RageFileBasic *f = zip.Open( path, RageFile::READ, err );
		REQUIRE( f != nullptr );
		CHECK( f->GetFileSize() == static_cast<int>( e.uncompressed.size() ) );
		CHECK( ReadAll( f ) == e.uncompressed );
		delete f;
	}

	// The DEFLATED entry really was smaller on disk.
	const RageFileDriverZip::FileInfo *big = zip.GetFileInfo( "/dir/big.txt" );
	REQUIRE( big != nullptr );
	CHECK( big->m_iCompressedSize < big->m_iUncompressedSize );
}

TEST_CASE( "RageFileDriverZip::Open on a missing entry fails; siblings still open", "[zip]" )
{
	EngineTestEnv::Require();

	std::vector<ZipEntry> entries = {
		{ "only.txt", "the only file", "", 0, 0, 0 },
	};
	WriteMem( "/@mem/pkg2.zip", BuildZip( entries ) );

	RageFileDriverZip zip;
	REQUIRE( zip.Load( RString( "/@mem/pkg2.zip" ) ) );

	CHECK( zip.GetFileInfo( "/nope.txt" ) == nullptr );
	int err = 0;
	CHECK( zip.Open( "/nope.txt", RageFile::READ, err ) == nullptr );

	int err2 = 0;
	RageFileBasic *f = zip.Open( "/only.txt", RageFile::READ, err2 );
	REQUIRE( f != nullptr );
	CHECK( ReadAll( f ) == "the only file" );
	delete f;
}

TEST_CASE( "RageFileDriverZip::Open never allows WRITE mode", "[zip]" )
{
	EngineTestEnv::Require();

	std::vector<ZipEntry> entries = {
		{ "f.txt", "content", "", 0, 0, 0 },
	};
	WriteMem( "/@mem/pkg3.zip", BuildZip( entries ) );

	RageFileDriverZip zip;
	REQUIRE( zip.Load( RString( "/@mem/pkg3.zip" ) ) );

	int err = 0;
	CHECK( zip.Open( "/f.txt", RageFile::WRITE, err ) == nullptr );
	CHECK( err == RageFileDriver::ERROR_WRITING_NOT_SUPPORTED );
}

TEST_CASE( "RageFileDriverZip::Load rejects a non-zip file", "[zip]" )
{
	EngineTestEnv::Require();

	WriteMem( "/@mem/notazip.bin", std::string( 4096, '\xA5' ) );

	RageFileDriverZip zip;
	CHECK_FALSE( zip.Load( RString( "/@mem/notazip.bin" ) ) );
}
