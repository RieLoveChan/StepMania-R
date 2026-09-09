// Characterization tests for DeviceInput::ToString / FromString -- how a
// (device, button) pair is written into and read back from keymaps
// (Data/Keymaps.ini). Pure: the enum<->name tables are file-scope
// static in RageInputDevice.cpp.
//
// Pins CURRENT behaviour, bug-for-bug (including: ToString of an invalid
// device is the empty string, and FromString needs exactly one '_'
// with non-empty halves). See DocsAgents/adr/0006-test-harness.md.

#include "global.h"
#include "RageInputDevice.h"

#include "catch_amalgamated.hpp"

TEST_CASE( "DeviceInput::ToString of an invalid device is empty", "[DeviceInput]" )
{
	DeviceInput di;
	di.device = InputDevice_Invalid;
	di.button = KEY_SPACE;
	CHECK( di.ToString() == "" );
}

TEST_CASE( "DeviceInput::ToString joins device and button with an underscore", "[DeviceInput]" )
{
	DeviceInput di( DEVICE_KEYBOARD, KEY_Ca ); // KEY_Ca == 'a' (97)
	const RString s = di.ToString();
	CAPTURE( s );
	// "<device>_<button>", the button half is the single char 'a'.
	CHECK( s.find( '_' ) != RString::npos );
	CHECK( s.substr( s.find( '_' ) + 1 ) == "a" );
}

TEST_CASE( "DeviceInput::FromString round-trips DeviceInput::ToString", "[DeviceInput]" )
{
	const DeviceInput cases[] = {
		DeviceInput( DEVICE_KEYBOARD, KEY_Ca ),
		DeviceInput( DEVICE_KEYBOARD, KEY_SPACE ),
		DeviceInput( DEVICE_KEYBOARD, KEY_LEFT ),
		DeviceInput( DEVICE_KEYBOARD, KEY_F1 ),
		DeviceInput( DEVICE_JOY1, JOY_BUTTON_1 ),
		DeviceInput( DEVICE_JOY2, JOY_BUTTON_10 ),
	};

	for( const DeviceInput &orig : cases )
	{
		const RString s = orig.ToString();
		CAPTURE( s );
		REQUIRE_FALSE( s.empty() );

		DeviceInput back;
		REQUIRE( back.FromString( s ) );
		CHECK( back.device == orig.device );
		CHECK( back.button == orig.button );
		CHECK( back.ToString() == s );
	}
}

TEST_CASE( "DeviceInput::FromString rejects a string without a well-formed device_button split", "[DeviceInput]" )
{
	DeviceInput di;
	di.device = DEVICE_KEYBOARD;

	CHECK_FALSE( di.FromString( "" ) );
	CHECK( di.device == InputDevice_Invalid );

	di.device = DEVICE_KEYBOARD;
	CHECK_FALSE( di.FromString( "nounderscore" ) );
	CHECK( di.device == InputDevice_Invalid );

	// A trailing underscore leaves the button half empty -> sscanf gets 1 field.
	di.device = DEVICE_KEYBOARD;
	CHECK_FALSE( di.FromString( "Key_" ) );
	CHECK( di.device == InputDevice_Invalid );
}

TEST_CASE( "DeviceInput::FromString of an unknown device/button name still 'succeeds' with Invalid parts", "[DeviceInput]" )
{
	// Characterization: as long as the "a_b" shape is there, FromString
	// returns true even if neither name resolves -- the parts just come
	// back as the enums' Invalid values.
	DeviceInput di;
	REQUIRE( di.FromString( "NoSuchDevice_NoSuchButton" ) );
	CHECK( di.device == InputDevice_Invalid );
	CHECK( di.button == DeviceButton_Invalid );
}
