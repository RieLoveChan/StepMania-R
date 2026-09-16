#include "global.h"
#include "Threads_Pthreads.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "RageUtil.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>

#if defined(MACOSX)
#include "archutils/Darwin/DarwinThreadHelpers.h"
#else
#include "archutils/Common/PthreadHelpers.h"
#endif

void ThreadImpl_Pthreads::Halt(bool Kill) {
	(void)Kill;
	/* Linux:
	 * Send a SIGSTOP to the thread. If we send a SIGKILL, pthreads will
	 * "helpfully" propagate it to the other threads, and we'll get killed, too.
	 * This isn't ideal, since it can cause the process to background as far as
	 * the shell is concerned, so the shell prompt can display before the crash
	 * handler actually displays a message. */
	SuspendThread(threadHandle);
}

void ThreadImpl_Pthreads::Resume() {
	// Linux: Send a SIGCONT to the thread.
	ResumeThread(threadHandle);
}

std::uint64_t ThreadImpl_Pthreads::GetThreadId() const {
	return threadHandle;
}

int ThreadImpl_Pthreads::Wait() {
	int *val;
	int ret = pthread_join(thread, (void **)&val);
	ASSERT_M(ret == 0, ssprintf("pthread_join: %s", strerror(ret)));

	int iRet = *val;
	delete val;
	return iRet;
}

ThreadImpl *MakeThisThread() {
	ThreadImpl_Pthreads *thread = new ThreadImpl_Pthreads;

	thread->thread = pthread_self();
	thread->threadHandle = GetCurrentThreadId();

	return thread;
}

static void *StartThread(void *pData) {
	ThreadImpl_Pthreads *pThis = (ThreadImpl_Pthreads *)pData;

	pThis->threadHandle = GetCurrentThreadId();
	*pThis->m_piThreadID = pThis->threadHandle;

	// Tell MakeThread that we've set m_piThreadID, so it's safe to return.
	pThis->m_StartFinishedSem->Post();

	int iRet = pThis->m_pFunc(pThis->m_pData);

	return new int(iRet);
}

ThreadImpl *MakeThread(int (*pFunc)(void *pData), void *pData, std::uint64_t *piThreadID) {
	ThreadImpl_Pthreads *thread = new ThreadImpl_Pthreads;
	thread->m_pFunc = pFunc;
	thread->m_pData = pData;
	thread->m_piThreadID = piThreadID;

	thread->m_StartFinishedSem = MakeSemaphore(0);

	int ret = pthread_create(&thread->thread, nullptr, StartThread, thread);
	ASSERT_M(ret == 0, ssprintf("MakeThread: pthread_create: %s", strerror(errno)));

	// Don't return until StartThread sets m_piThreadID.
	thread->m_StartFinishedSem->Wait();
	delete thread->m_StartFinishedSem;

	// Copy the thread name.
	const char *rawname = RageThread::GetThreadNameByID(*piThreadID);
	const std::size_t maxNameLen = sizeof(thread->name);
	if (strlen(rawname) < maxNameLen) {
		// If it fits, I sits^H^H^H^Hcopy.
		strncpy(thread->name, rawname, maxNameLen);
	}
	else {
		if (strstr(rawname, "Worker thread") && strchr(rawname, '(')) {
			// Special case for RageUtil_WorkerThread.cpp
			// "Worker thread (name)", e.g.
			// "Worker thread (/@mc1int/)" => "(/@mc1int/)".
			const char *workername = strchr(rawname, '(');
			strncpy(thread->name, workername, maxNameLen);
		}
		else {
			// Abbreviate the name by taking the first 6, last 7
			// characters and adding '..' in the middle.
			LOG->Trace("Truncated thread name due to size limit of %zu: %s", maxNameLen, rawname);
			snprintf(thread->name, maxNameLen, "%.6s..%s", rawname, &rawname[strlen(rawname) - 7]);
		}
	}
	// Ensure there is always a terminating NUL character.
	thread->name[maxNameLen - 1] = '\0';

#ifndef MACOSX
	// macOS/BSD can only set the name of the calling thread
	ret = pthread_setname_np(thread->thread, thread->name);
	if (ret != 0 && LOG) {
		LOG->Trace("pthead_setname_np: %s", strerror(ret));
	}
#endif

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
