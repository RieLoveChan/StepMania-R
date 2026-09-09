// Characterization tests for RageSurface / RageSurfaceUtils -- the
// in-memory image representation and its pure pixel/format helpers,
// which sit on every texture-load path. Previously untested.
//
// All surfaces are built in-memory (CreateSurface + direct pixel
// writes); no image files, no GL, no fixtures.
//
// Includes a regression pin for the RageSurfaceFormat::operator==
// memcmp-size bug fixed in f5005b8754 (was sizeof(RageSurfaceFormat),
// should be sizeof(RageSurfacePalette)).
//
// Pins CURRENT behaviour, bug-for-bug. See
// DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageUtil.h"

#include "catch_amalgamated.hpp"

#include <cstdint>
#include <memory>

namespace
{
	// 32-bit RGBA8888, R in the low byte.
	constexpr std::uint32_t kR = 0x000000FF, kG = 0x0000FF00, kB = 0x00FF0000, kA = 0xFF000000;

	struct SurfDel { void operator()( RageSurface *s ) const { delete s; } };
	using SurfPtr = std::unique_ptr<RageSurface, SurfDel>;

	SurfPtr Make32( int w, int h )
	{
		return SurfPtr( CreateSurface( w, h, 32, kR, kG, kB, kA ) );
	}

	std::uint32_t *Row32( RageSurface *s, int y )
	{
		return reinterpret_cast<std::uint32_t *>( s->pixels + y * s->pitch );
	}
}

TEST_CASE( "RageSurfaceUtils::decodepixel / encodepixel round-trip at each bpp", "[RageSurface]" )
{
	for( int bpp : { 1, 2, 3, 4 } )
	{
		CAPTURE( bpp );
		std::uint8_t buf[4] = { 0, 0, 0, 0 };
		const std::uint32_t maxv = ( bpp == 4 ) ? 0xFFFFFFFFu : ( ( 1u << ( bpp * 8 ) ) - 1u );

		for( std::uint32_t v : { 0u, 1u, 0x7Fu, 0x80u, maxv, maxv / 2 } )
		{
			RageSurfaceUtils::encodepixel( buf, bpp, v );
			CHECK( RageSurfaceUtils::decodepixel( buf, bpp ) == v );
		}
	}
}

TEST_CASE( "RageSurfaceUtils channel get/set round-trips through the 32-bit format", "[RageSurface]" )
{
	SurfPtr s = Make32( 1, 1 );
	REQUIRE( s );

	const std::uint8_t in[4] = { 0x12, 0x34, 0x56, 0x78 }; // r,g,b,a

	const std::uint32_t pixel = RageSurfaceUtils::SetRawRGBAV( &s->fmt, in );

	std::uint8_t out[4] = { 0, 0, 0, 0 };
	RageSurfaceUtils::GetRawRGBAV( pixel, s->fmt, out );

	CHECK( out[0] == in[0] );
	CHECK( out[1] == in[1] );
	CHECK( out[2] == in[2] );
	CHECK( out[3] == in[3] );

	// And the bit layout is what we asked CreateSurface for.
	CHECK( pixel == ( 0x12u | ( 0x34u << 8 ) | ( 0x56u << 16 ) | ( 0x78u << 24 ) ) );
}

TEST_CASE( "RageSurfaceUtils::GetBitsPerChannel reports 8/8/8/8 for RGBA8888", "[RageSurface]" )
{
	SurfPtr s = Make32( 2, 2 );
	REQUIRE( s );

	std::uint32_t bits[4] = { 0, 0, 0, 0 };
	RageSurfaceUtils::GetBitsPerChannel( &s->fmt, bits );
	CHECK( bits[0] == 8 );
	CHECK( bits[1] == 8 );
	CHECK( bits[2] == 8 );
	CHECK( bits[3] == 8 );
}

TEST_CASE( "RageSurfaceFormat::operator== / Equivalent (regression: memcmp size)", "[RageSurface]" )
{
	// Two freshly-created 32-bit RGBA surfaces have identical formats.
	SurfPtr a = Make32( 4, 4 );
	SurfPtr b = Make32( 4, 4 );
	REQUIRE( a );
	REQUIRE( b );

	CHECK( a->fmt == b->fmt );
	CHECK( a->fmt.Equivalent( b->fmt ) );

	// A different bit depth is a different format both ways.
	SurfPtr c = SurfPtr( CreateSurface( 4, 4, 16, 0x7C00, 0x03E0, 0x001F, 0x0000 ) );
	REQUIRE( c );
	CHECK_FALSE( a->fmt == c->fmt );
	CHECK_FALSE( a->fmt.Equivalent( c->fmt ) );

	// For a paletted (8bpp) format, operator== also compares the palette;
	// Equivalent() ignores it. Before f5005b8754 the palette compare used
	// the wrong sizeof and only looked at the first ~1/8th of it.
	SurfPtr p = SurfPtr( CreateSurface( 4, 4, 8, 0, 0, 0, 0 ) );
	SurfPtr q = SurfPtr( CreateSurface( 4, 4, 8, 0, 0, 0, 0 ) );
	REQUIRE( p );
	REQUIRE( q );
	REQUIRE( p->fmt.palette );
	REQUIRE( q->fmt.palette );

	CHECK( p->fmt == q->fmt );
	CHECK( p->fmt.Equivalent( q->fmt ) );

	// Change a palette entry near the END -- the pre-fix memcmp would
	// have missed this and still reported the formats equal.
	q->fmt.palette->colors[250] = RageSurfaceColor( 1, 2, 3, 4 );
	CHECK_FALSE( p->fmt == q->fmt );      // palette differs -> not equal
	CHECK( p->fmt.Equivalent( q->fmt ) ); // ...but the format itself is the same
}

TEST_CASE( "RageSurfaceUtils::Blit copies same-format pixels and clips to the smaller extent", "[RageSurface]" )
{
	SurfPtr src = Make32( 3, 2 );
	SurfPtr dst = Make32( 5, 4 );
	REQUIRE( src );
	REQUIRE( dst );

	// src: each pixel encodes its own (x,y) so a mis-copy is visible.
	for( int y = 0; y < src->h; ++y )
		for( int x = 0; x < src->w; ++x )
			Row32( src.get(), y )[x] = 0xFF000000u | static_cast<std::uint32_t>( ( y << 8 ) | x );

	// dst: fill with a sentinel.
	for( int y = 0; y < dst->h; ++y )
		for( int x = 0; x < dst->w; ++x )
			Row32( dst.get(), y )[x] = 0xDEADBEEFu;

	RageSurfaceUtils::Blit( src.get(), dst.get() );

	// The 3x2 overlap (min(src,dst) in each axis) is copied exactly.
	// (Characterization: the same-format fast path may also write a few
	// bytes into dst's row padding past x=width, so cells at x >= 3 in
	// the copied rows are not asserted here.)
	for( int y = 0; y < 2; ++y )
		for( int x = 0; x < 3; ++x )
			CHECK( Row32( dst.get(), y )[x] == ( 0xFF000000u | static_cast<std::uint32_t>( ( y << 8 ) | x ) ) );

	// Rows below the src height are not touched.
	CHECK( Row32( dst.get(), 3 )[0] == 0xDEADBEEFu );
	CHECK( Row32( dst.get(), 3 )[4] == 0xDEADBEEFu );
}

TEST_CASE( "RageSurfaceUtils::ConvertSurface 32->16->32 preserves color within the depth loss", "[RageSurface]" )
{
	SurfPtr s = Make32( 2, 1 );
	REQUIRE( s );
	Row32( s.get(), 0 )[0] = 0xFF204080u; // a=FF b=20 g=40 r=80
	Row32( s.get(), 0 )[1] = 0xFFFFFFFFu; // opaque white

	RageSurface *raw = s.release();
	// -> RGBA4444
	RageSurfaceUtils::ConvertSurface( raw, raw->w, raw->h, 16, 0xF000, 0x0F00, 0x00F0, 0x000F );
	CHECK( raw->fmt.BytesPerPixel == 2 );
	// -> back to RGBA8888
	RageSurfaceUtils::ConvertSurface( raw, raw->w, raw->h, 32, kR, kG, kB, kA );
	CHECK( raw->fmt.BytesPerPixel == 4 );

	SurfPtr back( raw );

	std::uint8_t v[4] = { 0, 0, 0, 0 };
	RageSurfaceUtils::GetRawRGBAV( Row32( back.get(), 0 )[0], back->fmt, v );
	// 4-bit channels: each byte is quantised to the top nibble, low nibble
	// replicated. 0x80 -> 0x88, 0x40 -> 0x44, 0x20 -> 0x22, 0xFF -> 0xFF.
	CHECK( ( v[0] & 0xF0 ) == 0x80 );
	CHECK( ( v[1] & 0xF0 ) == 0x40 );
	CHECK( ( v[2] & 0xF0 ) == 0x20 );
	CHECK( v[3] == 0xFF );

	RageSurfaceUtils::GetRawRGBAV( Row32( back.get(), 0 )[1], back->fmt, v );
	CHECK( v[0] == 0xFF );
	CHECK( v[1] == 0xFF );
	CHECK( v[2] == 0xFF );
	CHECK( v[3] == 0xFF );
}
