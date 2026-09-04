#include "global.h"
#include "GotoURL.h"

#include <cstdint>

#include <windows.h>
#include <shellapi.h>

/* This is called from the crash handler's separate child process (see
 * CrashHandlerChild.cpp), not from the crashed process's own exception
 * handler -- heap allocation (RString) is safe here. Still don't use
 * RegistryAccess, since it depends on engine state this minimal child
 * process doesn't set up. */
static LONG GetRegKey( HKEY key, RString subkey, RString &out )
{
	HKEY hKey;
    LONG iRet = RegOpenKeyEx( key, subkey, 0, KEY_QUERY_VALUE, &hKey );

    if( iRet != ERROR_SUCCESS )
		return iRet;

	long iDataSize = MAX_PATH;
	char data[MAX_PATH];
	RegQueryValue( hKey, "emulation", data, &iDataSize );
	out = data;
	RegCloseKey( hKey );

    return ERROR_SUCCESS;
}

bool GotoURL( RString sUrl )
{
	// First try ShellExecute()
	std::intptr_t iRet = reinterpret_cast<std::intptr_t>(ShellExecute( nullptr, "open", sUrl, nullptr, nullptr, SW_SHOWDEFAULT ));

	// If it failed, get the .htm regkey and lookup the program
	if( iRet > 32 )
		return true;

	RString sKey;
	if( GetRegKey(HKEY_CLASSES_ROOT, ".htm", sKey) != ERROR_SUCCESS )
		return false;

	sKey = "\\shell\\open\\command";

	if( GetRegKey(HKEY_CLASSES_ROOT, sKey, sKey) != ERROR_SUCCESS )
		return false;

	// Strip the "%1" (quoted or bare) parameter placeholder, if present, so
	// sUrl can be appended in its place below. sUrl is caller-supplied and,
	// via the crash handler's update checker (CrashHandlerChild.cpp),
	// network-supplied -- this used to be a fixed-buffer strcat with no
	// bound on sUrl's length; an RString has no fixed capacity to overflow.
	std::size_t iPos = sKey.find( "\"%1\"" );
	if( iPos == RString::npos )
		iPos = sKey.find( "%1" );
	if( iPos != RString::npos )
		sKey.erase( iPos );

	sKey += " ";
	sKey += sUrl;

	return WinExec( sKey, SW_SHOWDEFAULT ) > 32;
}

/*
 * (c) 2002-2004 Chris Danford
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
