// Round-trip characterization for the ZIP write + read path used by
// .smzip package installs: CreateZip (src/CreateZip.cpp, the bundled
// Info-ZIP writer) -> RageFileDriverZip (the read-only ZIP VFS driver).
// Neither had any test coverage.
//
// Everything runs in FILEMAN's writable /@mem mount -- source files and
// the .zip are written there, then the zip is loaded back and each
// entry extracted and compared byte-for-byte.
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageFile.h"
#include "RageFileDriverZip.h"
#include "CreateZip.h"
#include "RageUtil.h"

#include "catch_amalgamated.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
	std::string Ramp( int n, int seed = 0 )
	{
		std::string s;
		s.reserve( n );
		for( int i = 0; i < n; ++i )
			s.push_back( static_cast<char>( ( i + seed ) & 0xFF ) );
		return s;
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

TEST_CASE( "CreateZip -> RageFileDriverZip round-trip: entries are byte-exact", "[zip]" )
{
	EngineTestEnv::Require();

	// CreateZip stores names with the leading '/' stripped
	// (MakeDestZipFileName). FilenameDB inside the driver is '/'-rooted,
	// so we query with the leading slash back on.
	const std::string compressible( 20000, 'K' );  // will DEFLATE well
	const std::string binary = Ramp( 9000, 7 );    // ~incompressible

	WriteMem( "/@mem/zipsrc/compressible.txt", compressible );
	WriteMem( "/@mem/zipsrc/binary.dat", binary );

	{
		RageFile out;
		REQUIRE( out.Open( "/@mem/pkg.zip", RageFile::WRITE ) );
		CreateZip z;
		REQUIRE( z.Start( &out ) );
		REQUIRE( z.AddFile( "/@mem/zipsrc/compressible.txt" ) );
		REQUIRE( z.AddFile( "/@mem/zipsrc/binary.dat" ) );
		REQUIRE( z.Finish() );
	}

	RageFile chk;
	REQUIRE( chk.Open( "/@mem/pkg.zip", RageFile::READ ) );
	CHECK( chk.GetFileSize() > 0 );
	chk.Close();

	RageFileDriverZip zip;
	REQUIRE( zip.Load( RString( "/@mem/pkg.zip" ) ) );

	struct Want { const char *path; const std::string *data; };
	const Want wants[] = {
		{ "/@mem/zipsrc/compressible.txt", &compressible },
		{ "/@mem/zipsrc/binary.dat",       &binary },
	};

	for( const Want &w : wants )
	{
		CAPTURE( w.path );

		const RageFileDriverZip::FileInfo *info = zip.GetFileInfo( w.path );
		REQUIRE( info != nullptr );
		CHECK( info->m_iUncompressedSize == static_cast<int>( w.data->size() ) );
		CHECK( info->m_iCompressedSize >= 0 );

		int err = 0;
		RageFileBasic *f = zip.Open( w.path, RageFile::READ, err );
		REQUIRE( f != nullptr );
		CHECK( f->GetFileSize() == static_cast<int>( w.data->size() ) );

		const std::string got = ReadAll( f );
		CHECK( got.size() == w.data->size() );
		CHECK( got == *w.data );
		delete f;
	}

	// Characterization: this CreateZip build STORES rather than deflates
	// (its bundled Info-ZIP deflate path is not exercised) -- so the
	// compressed size equals the uncompressed size and the method is
	// STORED. If that ever changes (entries start deflating), this is
	// the line to revisit.
	const RageFileDriverZip::FileInfo *ci = zip.GetFileInfo( "/@mem/zipsrc/compressible.txt" );
	REQUIRE( ci != nullptr );
	CHECK( ci->m_iCompressionMethod == RageFileDriverZip::STORED );
	CHECK( ci->m_iCompressedSize == ci->m_iUncompressedSize );
}

TEST_CASE( "RageFileDriverZip::Open on a missing entry fails; siblings still open", "[zip]" )
{
	EngineTestEnv::Require();

	WriteMem( "/@mem/zipsrc2/only.txt", "the only file" );
	{
		RageFile out;
		REQUIRE( out.Open( "/@mem/pkg2.zip", RageFile::WRITE ) );
		CreateZip z;
		REQUIRE( z.Start( &out ) );
		REQUIRE( z.AddFile( "/@mem/zipsrc2/only.txt" ) );
		REQUIRE( z.Finish() );
	}

	RageFileDriverZip zip;
	REQUIRE( zip.Load( RString( "/@mem/pkg2.zip" ) ) );

	CHECK( zip.GetFileInfo( "/@mem/zipsrc2/nope.txt" ) == nullptr );
	int err = 0;
	CHECK( zip.Open( "/@mem/zipsrc2/nope.txt", RageFile::READ, err ) == nullptr );

	int err2 = 0;
	RageFileBasic *f = zip.Open( "/@mem/zipsrc2/only.txt", RageFile::READ, err2 );
	REQUIRE( f != nullptr );
	CHECK( ReadAll( f ) == "the only file" );
	delete f;
}

TEST_CASE( "RageFileDriverZip::Open never allows WRITE mode", "[zip]" )
{
	EngineTestEnv::Require();

	WriteMem( "/@mem/zipsrc3/f.txt", "content" );
	{
		RageFile out;
		REQUIRE( out.Open( "/@mem/pkg3.zip", RageFile::WRITE ) );
		CreateZip z;
		REQUIRE( z.Start( &out ) );
		REQUIRE( z.AddFile( "/@mem/zipsrc3/f.txt" ) );
		REQUIRE( z.Finish() );
	}

	RageFileDriverZip zip;
	REQUIRE( zip.Load( RString( "/@mem/pkg3.zip" ) ) );

	int err = 0;
	CHECK( zip.Open( "/@mem/zipsrc3/f.txt", RageFile::WRITE, err ) == nullptr );
	CHECK( err == RageFileDriver::ERROR_WRITING_NOT_SUPPORTED );
}

TEST_CASE( "RageFileDriverZip::Load rejects a non-zip file", "[zip]" )
{
	EngineTestEnv::Require();

	WriteMem( "/@mem/notazip.bin", Ramp( 4096 ) );

	RageFileDriverZip zip;
	CHECK_FALSE( zip.Load( RString( "/@mem/notazip.bin" ) ) );
}
