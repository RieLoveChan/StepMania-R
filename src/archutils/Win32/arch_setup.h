#ifndef ARCH_SETUP_WINDOWS_H
#define ARCH_SETUP_WINDOWS_H

#define SUPPORT_OPENGL
#define SUPPORT_D3D

#pragma warning (disable : 4005) // macro redefinitions (ARRAYSIZE)

// Fix VC breakage.
#define PATH_MAX _MAX_PATH

// Disable false deprecation warnings for POSIX-style function names
// (e.g. strdup vs _strdup).
#define _CRT_NONSTDC_NO_WARNINGS

// Don't include windows.h everywhere; when we do eventually include it, use these:
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

/* Minimum target is Windows 11 (ADR 0003). 0x0A00 is the _WIN32_WINNT
 * value for Windows 10 and 11 alike; use NTDDI_VERSION for anything
 * Win11-specific. */
#define _WIN32_WINNT 0x0A00
#define _WIN32_IE 0x0A00

#include <wchar.h> // needs to be included before our fixes below

#define lstat stat
#define fsync _commit

typedef time_t time_t;
struct tm;
struct tm *my_localtime_r( const time_t *timep, struct tm *result );
#define localtime_r my_localtime_r
struct tm *my_gmtime_r( const time_t *timep, struct tm *result );
#define gmtime_r my_gmtime_r
void my_usleep( unsigned long usec );
#define usleep my_usleep

#undef min
#undef max
#define NOMINMAX // make sure Windows doesn't try to define this

/* We implement the crash handler interface (though that interface isn't
 * completely uniform across platforms yet). */
#if !defined(SMPACKAGE)
#define CRASH_HANDLER
#endif

#endif

/*
 * (c) 2002-2004 Glenn Maynard
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
