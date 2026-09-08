/* RageLog - Manages logging. */

#ifndef RAGE_LOG_H
#define RAGE_LOG_H

namespace Log
{
	/* Subsystem tag for a log line. Seed set — extend as call sites are
	 * migrated (ADR 0005 phase 4). CategoryToString gives the short
	 * lowercase name shown in the log column and accepted by
	 * --LogLevel=<cat>:<level>. */
	enum Category
	{
		General,	// no subsystem / not yet categorised
		Arch, File, Lua, Theme, Font, Gl, Sound, Input,
		Song, Steps, Actor, Screen, Profile, Net, Cache,
		NUM_Category
	};
	Category CategoryFromString( const RString &s );	// unknown -> General
	const char *CategoryToString( Category c );
}

class RageLog
{
public:
	RageLog();
	~RageLog();

	/* Severity ordering: Trace < Debug < Info < Warn < Error < Off.
	 * `Off` is a threshold only (nothing is ever tagged with it). A line
	 * below the effective minimum level is dropped from all
	 * destinations. Default minimum is LogLevel_Trace (nothing dropped).
	 * ADR 0005. */
	enum LogLevel
	{
		LogLevel_Trace,
		LogLevel_Debug,
		LogLevel_Info,
		LogLevel_Warn,
		LogLevel_Error,
		LogLevel_Off,
		NUM_LogLevel
	};
	/* Parse a level name ("trace".."error", "off"; case-insensitive); an
	 * unrecognised string returns LogLevel_Trace. */
	static LogLevel LogLevelFromString( const RString &s );
	static const char *LogLevelToString( LogLevel l );
	void SetLogLevel( LogLevel l );	// global minimum; drop lines below it

	/* Per-category minimum. LogLevel_Trace = "use the global minimum".
	 * The effective minimum for a category is its own if set past Trace,
	 * else the global one. */
	void SetCategoryLevel( Log::Category c, LogLevel l );
	LogLevel GetEffectiveLevel( Log::Category c ) const;

	/* Parse a --LogLevel spec: comma-separated, a bare token is the
	 * global level, a "cat:level" token sets that category
	 * (e.g. "warn,gl:off,font:trace"). Unknown tokens are ignored. */
	void SetLogLevelSpec( const RString &spec );

	/* The category-aware sink the LOG_* macros below call. Formats
	 * "<cat> <file>:<line>  <msg>" and routes it at `level`. */
	void LogLine( LogLevel level, Log::Category cat,
		const char *file, int line, const char *fmt, ... ) PRINTF(6,7);

	void Trace( const char *fmt, ... ) PRINTF(2,3);
	// Debug sits below Trace: even more verbose, off unless the log
	// level is lowered to it. Goes to log.txt only, like Trace. ADR 0005.
	void Debug( const char *fmt, ... ) PRINTF(2,3);
	void Warn( const char *fmt, ... ) PRINTF(2,3);
	// Error is for serious-but-recoverable failures. For unrecoverable
	// ones use RageException::Throw (which also logs). See ADR 0005.
	void Error( const char *fmt, ... ) PRINTF(2,3);
	void Info( const char *fmt, ... ) PRINTF(2,3);
	// Time is purely for writing profiling time data to the time log. -Kyz
	void Time( const char *fmt, ... ) PRINTF(2,3);
	void UserLog( const RString &sType, const RString &sElement, const char *fmt, ... ) PRINTF(4,5);
	void Flush();

	void MapLog( const RString &key, const char *fmt, ... ) PRINTF(3,4);
	void UnmapLog( const RString &key );

	static const char *GetAdditionalLog();
	static const char *GetInfo();
	/* Returns nullptr if past the last recent log. */
	static const char *GetRecentLog( int n );

	void SetShowLogOutput( bool show ); // enable or disable logging to stdout
	void SetLogToDisk( bool b );	// enable or disable logging to file
	void SetInfoToDisk( bool b );	// enable or disable logging info.txt to file
	void SetUserLogToDisk( bool b);	// enable or disable logging user.txt to file
	void SetFlushing( bool b );	// enable or disable flushing

private:
	bool m_bLogToDisk;
	bool m_bInfoToDisk;
	bool m_bUserLogToDisk;
	bool m_bFlush;
	bool m_bShowLogOutput;
	LogLevel m_MinLevel = LogLevel_Trace;
	/* Per-category minimum, or -1 for "unset" (follow the global one).
	 * A category CAN be set below the global level -- e.g. global=warn,
	 * font:trace keeps font verbose. Initialised to -1 in the ctor. */
	signed char m_CategoryLevel[Log::NUM_Category];

	/* Consecutive-identical-line collapsing (ADR 0005 phase 3): the last
	 * emitted tag+message (no timestamp), its destination bits and tag,
	 * and how many identical lines have been suppressed since. */
	RString m_sLastEmit, m_sLastTag;
	int m_iLastWhere = 0;
	int m_iRepeatCount = 0;

	void Write( int where, LogLevel level, Log::Category cat, const RString &str );
	void EmitLine( int where, const RString &sTagged );	// one line, no timestamp yet
	void SpillRepeat();	// emit the pending "(repeated N×)" summary, if any
	void UpdateMappedLog();
	void AddToInfo( const RString &buf );
	void AddToRecentLogs( const RString &buf );
};

extern RageLog*	LOG;	// global and accessible from anywhere in our program

/* Category-aware, file:line-stamped logging. Prefer these at new /
 * migrated call sites (ADR 0005). The bare LOG->Trace(...) etc. stay
 * valid (they log as Log::General, no file:line). */
#define LOG_TRACE( cat, ... )	LOG->LogLine( RageLog::LogLevel_Trace, (cat), __FILE__, __LINE__, __VA_ARGS__ )
#define LOG_DEBUG( cat, ... )	LOG->LogLine( RageLog::LogLevel_Debug, (cat), __FILE__, __LINE__, __VA_ARGS__ )
#define LOG_INFO(  cat, ... )	LOG->LogLine( RageLog::LogLevel_Info,  (cat), __FILE__, __LINE__, __VA_ARGS__ )
#define LOG_WARN(  cat, ... )	LOG->LogLine( RageLog::LogLevel_Warn,  (cat), __FILE__, __LINE__, __VA_ARGS__ )
#define LOG_ERROR( cat, ... )	LOG->LogLine( RageLog::LogLevel_Error, (cat), __FILE__, __LINE__, __VA_ARGS__ )

#endif

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
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
