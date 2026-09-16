#ifndef THREADS_STD_H
#define THREADS_STD_H

#include "Threads.h"

#include <condition_variable>
#include <mutex>

/* Portable std::mutex/std::condition_variable-backed implementations of
 * MutexImpl/EventImpl/SemaImpl (ADR 0007) -- used on every platform,
 * replacing the old per-platform MutexImpl_Win32/EventImpl_Win32/
 * SemaImpl_Win32 and MutexImpl_Pthreads/EventImpl_Pthreads/
 * SemaImpl_Pthreads classes. Thread creation/Halt/Resume (ThreadImpl)
 * stays per-platform in Threads_Win32/Threads_Pthreads -- std::thread has
 * no portable suspend, and Halt() is a real crash-handler safety feature
 * (RageThread::HaltAllThreads), not something to weaken for uniformity. */

class MutexImpl_Std : public MutexImpl {
	friend class EventImpl_Std;

 public:
	MutexImpl_Std(RageMutex *pParent);
	~MutexImpl_Std() override;

	bool Lock() override;
	bool TryLock() override;
	void Unlock() override;

 private:
	std::mutex m_Mutex;
};

class EventImpl_Std : public EventImpl {
 public:
	EventImpl_Std(MutexImpl_Std *pParent);
	~EventImpl_Std() override;

	bool Wait(RageTimer *pTimeout) override;
	void Signal() override;
	void Broadcast() override;
	bool WaitTimeoutSupported() const override {
		return true;
	}

 private:
	MutexImpl_Std *m_pParent;
	std::condition_variable_any m_Cond;
};

class SemaImpl_Std : public SemaImpl {
 public:
	SemaImpl_Std(int iInitialValue);
	~SemaImpl_Std() override;

	int GetValue() const override;
	void Post() override;
	bool Wait() override;
	bool TryWait() override;

 private:
	mutable std::mutex m_Mutex;
	std::condition_variable m_Cond;
	int m_iValue;
};

#endif
