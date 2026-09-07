// Characterization tests for the WAV sound reader -- the salvageable
// core of the 2004-era src/tests/test_audio_readers.cpp (backlog
// item 17). Uses a SYNTHETIC PCM WAV built in memory (deterministic
// samples chosen here -- no copyrighted audio, no committed fixture),
// written to FILEMAN's writable /@mem mount and decoded back.
//
// Exercises both entry points: RageSoundReader_WAV::Open directly, and
// the format-autodetecting RageSoundReader_FileReader::OpenFile factory
// (which needs the extension<->filetype maps EngineTestEnv populates via
// ActorUtil::InitFileTypeLists).
//
// Pins CURRENT behaviour. See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageFile.h"
#include "RageSoundReader_WAV.h"
#include "RageSoundReader_FileReader.h"

#include "catch_amalgamated.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace
{
	const int kSampleRate = 44100;
	const int kFrames     = 4410; // 100 ms, mono

	std::int16_t SampleAt( int i )
	{
		// Periodic, in-range, easy to check: -20000 .. +19600, period 100.
		return static_cast<std::int16_t>( ( ( i % 100 ) - 50 ) * 400 );
	}

	void PutLE16( std::string &s, std::uint16_t v )
	{
		s.push_back( static_cast<char>( v & 0xFF ) );
		s.push_back( static_cast<char>( ( v >> 8 ) & 0xFF ) );
	}
	void PutLE32( std::string &s, std::uint32_t v )
	{
		s.push_back( static_cast<char>( v & 0xFF ) );
		s.push_back( static_cast<char>( ( v >> 8 ) & 0xFF ) );
		s.push_back( static_cast<char>( ( v >> 16 ) & 0xFF ) );
		s.push_back( static_cast<char>( ( v >> 24 ) & 0xFF ) );
	}

	// Canonical 44-byte-header mono 16-bit PCM WAV + data.
	std::string MakeWav()
	{
		const std::uint32_t dataBytes = static_cast<std::uint32_t>( kFrames ) * 2u;
		std::string s;
		s += "RIFF";           PutLE32( s, 36u + dataBytes );  s += "WAVE";
		s += "fmt ";           PutLE32( s, 16u );
		PutLE16( s, 1 );                       // PCM
		PutLE16( s, 1 );                       // channels
		PutLE32( s, kSampleRate );
		PutLE32( s, kSampleRate * 1u * 2u );   // byte rate
		PutLE16( s, 2 );                       // block align
		PutLE16( s, 16 );                      // bits per sample
		s += "data";           PutLE32( s, dataBytes );
		for( int i = 0; i < kFrames; ++i )
			PutLE16( s, static_cast<std::uint16_t>( SampleAt( i ) ) );
		return s;
	}

	RString WriteMem( const char *name, const std::string &bytes )
	{
		RString path = RString( "/@mem/" ) + name;
		RageFile w;
		REQUIRE( w.Open( path, RageFile::WRITE ) );
		REQUIRE( w.Write( bytes.data(), bytes.size() ) == static_cast<int>( bytes.size() ) );
		w.Close();
		return path;
	}

	// PCM16 -> float, the conventional /32768 scaling. Characterization
	// checks use a generous epsilon so a different-but-close scaling
	// still shows up as a specific number rather than a hard fail.
	float ExpectedFloat( int frame ) { return SampleAt( frame ) / 32768.0f; }
}

TEST_CASE( "RageSoundReader_WAV::Open decodes a synthetic PCM WAV", "[RageSoundReader][wav]" )
{
	EngineTestEnv::Require();
	const RString path = WriteMem( "tone_direct.wav", MakeWav() );

	RageFile *pFile = new RageFile;
	REQUIRE( pFile->Open( path, RageFile::READ ) );

	RageSoundReader_WAV wav;
	REQUIRE( wav.Open( pFile ) == RageSoundReader_FileReader::OPEN_OK ); // takes ownership

	CHECK( wav.GetSampleRate() == kSampleRate );
	CHECK( wav.GetNumChannels() == 1u );
	// GetLength() is milliseconds: 4410 frames / 44100 Hz == 100 ms.
	CHECK( wav.GetLength() == 100 );

	std::vector<float> buf( 64 );
	CHECK( wav.Read( buf.data(), 64 ) == 64 );
	CHECK( buf[0] == Catch::Approx( ExpectedFloat( 0 ) ).margin( 0.001 ) );
	CHECK( buf[1] == Catch::Approx( ExpectedFloat( 1 ) ).margin( 0.001 ) );
	CHECK( buf[50] == Catch::Approx( ExpectedFloat( 50 ) ).margin( 0.001 ) );

	// Seek to frame 100 (start of the next period) and re-read.
	CHECK( wav.SetPosition( 100 ) > 0 );
	CHECK( wav.Read( buf.data(), 8 ) == 8 );
	CHECK( buf[0] == Catch::Approx( ExpectedFloat( 100 ) ).margin( 0.001 ) );
	CHECK( buf[3] == Catch::Approx( ExpectedFloat( 103 ) ).margin( 0.001 ) );
}

TEST_CASE( "RageSoundReader_FileReader::OpenFile autodetects the WAV", "[RageSoundReader][wav][factory]" )
{
	EngineTestEnv::Require();
	const RString path = WriteMem( "tone_factory.wav", MakeWav() );

	RString error;
	RageSoundReader_FileReader *r = RageSoundReader_FileReader::OpenFile( path, error );
	REQUIRE( r != nullptr );
	CHECK( error == "" );
	CHECK( r->GetSampleRate() == kSampleRate );
	CHECK( r->GetNumChannels() == 1u );

	std::vector<float> buf( 16 );
	CHECK( r->Read( buf.data(), 16 ) == 16 );
	CHECK( buf[2] == Catch::Approx( ExpectedFloat( 2 ) ).margin( 0.001 ) );

	delete r;
}

TEST_CASE( "RageSoundReader_FileReader::OpenFile rejects a non-audio file", "[RageSoundReader][error]" )
{
	EngineTestEnv::Require();
	const RString path = WriteMem( "not_audio.wav", std::string( "this is plainly not a RIFF/WAVE file at all" ) );

	RString error;
	RageSoundReader_FileReader *r = RageSoundReader_FileReader::OpenFile( path, error );
	CHECK( r == nullptr );
	CHECK_FALSE( error.empty() );
	delete r;
}
