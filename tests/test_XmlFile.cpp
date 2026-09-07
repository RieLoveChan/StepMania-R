// Characterization tests for the engine's hand-rolled XML parser
// (XmlFileUtil::Load / GetXML over XNode) -- used by Profiles, Lua.xml,
// theme metrics, NoteSkins metadata, stats. Previously untested.
// Parses from strings (XmlFileUtil::Load takes an RString directly), so
// no fixtures and no FILEMAN needed -- but LOG is (error paths), hence
// EngineTestEnv::Require().
//
// Pins CURRENT behaviour, bug-for-bug -- including that text content is
// only captured *before* the first child element (no mixed content) and
// that only the five named entities are decoded (no numeric refs).
// See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "EngineTestEnv.h"

#include "XmlFile.h"
#include "XmlFileUtil.h"

#include "catch_amalgamated.hpp"

#include <string>

namespace
{
	// Load `xml` into `node`; returns the error string (empty on success).
	RString Parse( XNode &node, const char *xml )
	{
		RString err;
		XmlFileUtil::Load( &node, RString( xml ), err );
		return err;
	}

	RString Attr( const XNode &n, const char *name )
	{
		RString out;
		n.GetAttrValue( RString( name ), out );
		return out;
	}

	RString Text( const XNode &n )
	{
		RString out;
		n.GetTextValue( out );
		return out;
	}

	int ChildCount( const XNode &n )
	{
		int c = 0;
		for( auto it = n.GetChildrenBegin(); it != n.GetChildrenEnd(); ++it )
			++c;
		return c;
	}
}

TEST_CASE( "XmlFileUtil::Load makes the passed node the root element", "[XmlFile]" )
{
	EngineTestEnv::Require();
	XNode n;
	CHECK( Parse( n, "<root/>" ) == "" );
	CHECK( n.GetName() == "root" );
	CHECK( ChildCount( n ) == 0 );
}

TEST_CASE( "XmlFile attributes: double, single, and unquoted", "[XmlFile][attr]" )
{
	EngineTestEnv::Require();
	XNode n;
	REQUIRE( Parse( n, "<r a=\"1\" b='two' c=three />" ) == "" );
	CHECK( Attr( n, "a" ) == "1" );
	CHECK( Attr( n, "b" ) == "two" );
	CHECK( Attr( n, "c" ) == "three" );
}

TEST_CASE( "XmlFile GetChild returns the first same-named child; children iterate in order", "[XmlFile][child]" )
{
	EngineTestEnv::Require();
	XNode n;
	REQUIRE( Parse( n, "<r><x>hello</x><x>world</x><y>z</y></r>" ) == "" );

	CHECK( ChildCount( n ) == 3 );
	const XNode *x = n.GetChild( "x" );
	REQUIRE( x != nullptr );
	CHECK( Text( *x ) == "hello" );

	RString yv;
	CHECK( n.GetChildValue( "y", yv ) );
	CHECK( yv == "z" );
}

TEST_CASE( "XmlFile captures element text only before the first child (no mixed content)", "[XmlFile][quirk]" )
{
	EngineTestEnv::Require();
	XNode n;
	REQUIRE( Parse( n, "<r>lead<c/>trailing-text-is-dropped</r>" ) == "" );

	CHECK( Text( n ) == "lead" );
	CHECK( n.GetChild( "c" ) != nullptr );
}

TEST_CASE( "XmlFile decodes the five named entities in text and attributes", "[XmlFile][entity]" )
{
	EngineTestEnv::Require();
	XNode n;
	REQUIRE( Parse( n,
		"<r v=\"a &lt; b &amp; c &quot;q&quot; &apos;s&apos;\">t &gt; u</r>" ) == "" );

	CHECK( Attr( n, "v" ) == "a < b & c \"q\" 's'" );
	CHECK( Text( n ) == "t > u" );
}

TEST_CASE( "XmlFile does NOT decode numeric character references", "[XmlFile][entity][quirk]" )
{
	EngineTestEnv::Require();
	XNode n;
	REQUIRE( Parse( n, "<r>&#65;&#x41;</r>" ) == "" );
	CHECK( Text( n ) == "&#65;&#x41;" ); // left verbatim
}

TEST_CASE( "XmlFile skips comments and the <?xml?> prolog", "[XmlFile][comment]" )
{
	EngineTestEnv::Require();
	XNode n;
	REQUIRE( Parse( n, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!-- a comment --><r a=\"1\"/>" ) == "" );
	CHECK( n.GetName() == "r" );
	CHECK( Attr( n, "a" ) == "1" );
}

TEST_CASE( "XmlFile treats <x/> and <x></x> the same", "[XmlFile][child]" )
{
	EngineTestEnv::Require();
	XNode a, b;
	REQUIRE( Parse( a, "<r><c/></r>" ) == "" );
	REQUIRE( Parse( b, "<r><c></c></r>" ) == "" );
	CHECK( a.GetChild( "c" ) != nullptr );
	CHECK( b.GetChild( "c" ) != nullptr );
	CHECK( ChildCount( a ) == ChildCount( b ) );
}

TEST_CASE( "XmlFile GetXML re-encodes entities and round-trips through Load", "[XmlFile][roundtrip]" )
{
	EngineTestEnv::Require();
	XNode n;
	REQUIRE( Parse( n, "<r note=\"x &amp; y\"><child>a &lt; b</child></r>" ) == "" );

	const RString xml = XmlFileUtil::GetXML( &n );
	CHECK( xml.find( "&amp;" ) != RString::npos );
	CHECK( xml.find( "&lt;" ) != RString::npos );

	XNode n2;
	REQUIRE( Parse( n2, xml.c_str() ) == "" );
	CHECK( n2.GetName() == "r" );
	CHECK( Attr( n2, "note" ) == "x & y" );
	const XNode *c = n2.GetChild( "child" );
	REQUIRE( c != nullptr );
	CHECK( Text( *c ) == "a < b" );
}

TEST_CASE( "XmlFile reports an unterminated comment", "[XmlFile][error]" )
{
	EngineTestEnv::Require();
	XNode n;
	CHECK( Parse( n, "<!-- never ends" ) == "Unterminated comment" );
}

TEST_CASE( "XmlFile reports an unclosed root element", "[XmlFile][error]" )
{
	EngineTestEnv::Require();
	XNode n;
	CHECK_FALSE( Parse( n, "<r>text with no close tag" ).empty() );
}

TEST_CASE( "XmlFile keeps an attribute that has a name but no value", "[XmlFile][attr][edge]" )
{
	EngineTestEnv::Require();
	XNode n;
	REQUIRE( Parse( n, "<r flag a=\"1\"/>" ) == "" );
	CHECK( n.GetAttr( "a" ) != nullptr );
	CHECK( n.GetAttr( "flag" ) != nullptr );
	CHECK( Attr( n, "flag" ) == "" );
}
