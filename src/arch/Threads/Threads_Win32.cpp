#include "global.h"
#include "Threads_Win32.h"
#include "RageUtil.h"
#include "RageThreads.h"
#include "archutils/Win32/ErrorStrings.h"

#include <cstdint>
#include <mutex>

const int MAX_THREADS = 128;

// Guards g_ThreadIds/g_ThreadHandles below. Plain std::mutex -- this is
// internal bookkeeping private to this file, not a MutexImpl (which now
// lives in Threads_Std.cpp and would be circular to depend on here).
static std::mutex g_ThreadIdMutex;

static std::uint64_t g_ThreadIds[MAX_THREADS];
static HANDLE g_ThreadHandles[MAX_THREADS];

HANDLE Win32ThreadIdToHandle(std::uint64_t iID) {
	for (int i = 0; i < MAX_THREADS; ++i) {
		if (g_ThreadIds[i] == iID)
			return g_ThreadHandles[i];
	}

	return nullptr;
}

void ThreadImpl_Win32::Halt(bool Kill) {
	if (Kill)
		TerminateThread(ThreadHandle, 0);
	else
		SuspendThread(ThreadHandle);
}

void ThreadImpl_Win32::Resume() {
	ResumeThread(ThreadHandle);
}

std::uint64_t ThreadImpl_Win32::GetThreadId() const {
	return (std::uint64_t)ThreadId;
}

int ThreadImpl_Win32::Wait() {
	WaitForSingleObject(ThreadHandle, INFINITE);

	DWORD ret;
	GetExitCodeThread(ThreadHandle, &ret);

	CloseHandle(ThreadHandle);
	ThreadHandle = nullptr;

	return ret;
}

// SetThreadName magic comes from VirtualDub.
#define MS_VC_EXCEPTION 0x406d1388

typedef struct tagTHREADNAME_INFO {
	DWORD dwType;     // must be 0x1000
	LPCSTR szName;    // pointer to name (in same addr space)
	DWORD dwThreadID; // thread ID (-1 caller thread)
	DWORD dwFlags;    // reserved for future use, must be zero
} THREADNAME_INFO;

static void SetThreadName(DWORD dwThreadID, LPCTSTR szThreadName) {
#if defined(_MSC_VER)
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR *)&info);
	} __except (EXCEPTION_CONTINUE_EXECUTION) {
	}
#elif defined(__GNUC__)
	pthread_setname_np(pthread_self(), szThreadName);
#endif
}

static DWORD WINAPI StartThread(LPVOID pData) {
	ThreadImpl_Win32 *pThis = static_cast<ThreadImpl_Win32 *>(pData);

	SetThreadName(GetCurrentThreadId(), RageThread::GetCurrentThreadName());

	DWORD ret = static_cast<DWORD>(pThis->m_pFunc(pThis->m_pData));

	for (int i = 0; i < MAX_THREADS; ++i) {
		if (g_ThreadIds[i] == RageThread::GetCurrentThreadID()) {
			g_ThreadHandles[i] = nullptr;
			g_ThreadIds[i] = 0;
			break;
		}
	}

	return ret;
}

static int GetOpenSlot(std::uint64_t iID) {
	std::lock_guard<std::mutex> lock(g_ThreadIdMutex);

	// Find an open slot in g_ThreadIds.
	int slot = 0;
	while (slot < MAX_THREADS && g_ThreadIds[slot] != 0)
		++slot;
	ASSERT(slot < MAX_THREADS);

	g_ThreadIds[slot] = iID;

	return slot;
}

ThreadImpl *MakeThisThread() {
	ThreadImpl_Win32 *thread = new ThreadImpl_Win32;

	SetThreadName(GetCurrentThreadId(), RageThread::GetCurrentThreadName());

	const HANDLE CurProc = GetCurrentProcess();
	int ret =
	   DuplicateHandle(CurProc, GetCurrentThread(), CurProc, &thread->ThreadHandle, 0, false, DUPLICATE_SAME_ACCESS);

	if (!ret) {
		//		LOG->Warn( werr_ssprintf( GetLastError(), "DuplicateHandle(%p, %p) failed",
		//			CurProc, GetCurrentThread() ) );

		thread->ThreadHandle = nullptr;
	}

	thread->ThreadId = GetCurrentThreadId();

	int slot = GetOpenSlot(GetCurrentThreadId());
	g_ThreadHandles[slot] = thread->ThreadHandle;

	return thread;
}

ThreadImpl *MakeThread(int (*pFunc)(void *pData), void *pData, std::uint64_t *piThreadID) {
	ThreadImpl_Win32 *thread = new ThreadImpl_Win32;
	thread->m_pFunc = pFunc;
	thread->m_pData = pData;

	thread->ThreadHandle = CreateThread(nullptr, 0, &StartThread, thread, CREATE_SUSPENDED, &thread->ThreadId);
	*piThreadID = (std::uint64_t)thread->ThreadId;
	ASSERT_M(thread->ThreadHandle != nullptr, ssprintf("%s", werr_ssprintf(GetLastError(), "CreateThread").c_str()));

	int slot = GetOpenSlot(thread->ThreadId);
	g_ThreadHandles[slot] = thread->ThreadHandle;

	int iRet = ResumeThread(thread->ThreadHandle);
	ASSERT_M(iRet == 1, ssprintf("%s", werr_ssprintf(GetLastError(), "ResumeThread").c_str()));

	return thread;
}

std::uint64_t GetThisThreadId() {
	return GetCurrentThreadId();
}

std::uint64_t GetInvalidThreadId() {
	return 0;
}

/*
 * (c) 2001-2004 Glenn Maynard
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
