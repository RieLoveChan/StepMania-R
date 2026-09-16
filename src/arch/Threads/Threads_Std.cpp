#include "global.h"
#include "Threads_Std.h"
#include "RageTimer.h"

#include <algorithm>
#include <chrono>

MutexImpl_Std::MutexImpl_Std(RageMutex *pParent) : MutexImpl(pParent) {
}

MutexImpl_Std::~MutexImpl_Std() {
}

bool MutexImpl_Std::Lock() {
	m_Mutex.lock();
	return true;
}

bool MutexImpl_Std::TryLock() {
	return m_Mutex.try_lock();
}

void MutexImpl_Std::Unlock() {
	m_Mutex.unlock();
}

EventImpl_Std::EventImpl_Std(MutexImpl_Std *pParent) : m_pParent(pParent) {
}

EventImpl_Std::~EventImpl_Std() {
}

bool EventImpl_Std::Wait(RageTimer *pTimeout) {
	if (pTimeout == nullptr) {
		m_Cond.wait(m_pParent->m_Mutex);
		return true;
	}

	float fSecondsInFuture = std::max(0.f, -pTimeout->Ago());
	const auto status = m_Cond.wait_for(m_pParent->m_Mutex, std::chrono::duration<float>(fSecondsInFuture));
	return status == std::cv_status::no_timeout;
}

void EventImpl_Std::Signal() {
	m_Cond.notify_one();
}

void EventImpl_Std::Broadcast() {
	m_Cond.notify_all();
}

EventImpl *MakeEvent(MutexImpl *pMutex) {
	MutexImpl_Std *pStdMutex = static_cast<MutexImpl_Std *>(pMutex);

	return new EventImpl_Std(pStdMutex);
}

MutexImpl *MakeMutex(RageMutex *pParent) {
	return new MutexImpl_Std(pParent);
}

// Matches the old SemaImpl_Pthreads condition-variable fallback ("use
// conditions, to work around macOS forgetting to implement semaphores"):
// a plain counter guarded by a mutex, with a generous "probably
// deadlocked" wait timeout -- RageSemaphore::Wait() (RageThreads.cpp)
// already loops around a false return, retrying indefinitely unless
// bFailOnTimeout is set and no dialog is showing, so the exact timeout
// here is a diagnostic heuristic, not a correctness knob.
SemaImpl_Std::SemaImpl_Std(int iInitialValue) : m_iValue(iInitialValue) {
}

SemaImpl_Std::~SemaImpl_Std() {
}

int SemaImpl_Std::GetValue() const {
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_iValue;
}

void SemaImpl_Std::Post() {
	std::lock_guard<std::mutex> lock(m_Mutex);
	++m_iValue;
	if (m_iValue == 1)
		m_Cond.notify_one();
}

bool SemaImpl_Std::Wait() {
	std::unique_lock<std::mutex> lock(m_Mutex);

	// Probably deadlocked if nothing posts within this long.
	const bool bSignalled = m_Cond.wait_for(lock, std::chrono::seconds(15), [this] { return m_iValue > 0; });
	if (!bSignalled)
		return false;

	--m_iValue;
	return true;
}

bool SemaImpl_Std::TryWait() {
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_iValue == 0)
		return false;

	--m_iValue;
	return true;
}

SemaImpl *MakeSemaphore(int iInitialValue) {
	return new SemaImpl_Std(iInitialValue);
}
