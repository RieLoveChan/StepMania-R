// Concurrency stress test for ADR 0007 (threading modernization):
// exercises RageMutex/RageEvent/RageSemaphore across real OS threads
// (RageThread::Create, not just single-threaded Catch2 assertions) and
// checks for races/lost updates/lost wakeups/deadlocks. See
// DocsAgents/adr/0007-threading-modernization.md.
//
// Per the ADR, this had to prove itself against the *old* per-platform
// Win32/Pthreads MutexImpl/EventImpl/SemaImpl before the internals were
// reimplemented with std::mutex/condition_variable (Threads_Std.cpp) --
// it did more than that: the semaphore case caught a genuine,
// intermittent pre-existing bug in SemaImpl_Win32 (its diagnostic
// m_iCounter was mutated by Post()/Wait() with no lock protecting it,
// an unsynchronized read-modify-write across threads), which the
// std::mutex-guarded SemaImpl_Std fixes. The TryLock case caught a bug
// in this test itself during development, not the implementation: a
// mutex may only be unlocked by the thread that locked it, which
// std::mutex enforces strictly and the old primitives tolerated more
// loosely -- worth keeping in mind for real call sites, too.
//
// A real deadlock/hang here is caught by the ctest-level TIMEOUT on the
// sm_tests entry (tests/CMakeLists.txt), not by anything in this file --
// there is no portable way to bound an individual RageMutex/RageEvent/
// RageSemaphore wait from the outside.

#include "global.h"
#include "EngineTestEnv.h"

#include "RageThreads.h"
#include "RageTimer.h"
#include "RageUtil.h"

#include "catch_amalgamated.hpp"

#include <atomic>
#include <deque>
#include <vector>

namespace {

const int NUM_WORKER_THREADS = 8;
const int INCREMENTS_PER_THREAD = 20000;

struct MutexStressData {
	RageMutex *pMutex;
	long *pCounter;
};

// Catch2 assertions (REQUIRE/etc.) are not safe to throw across a raw OS
// thread boundary -- an uncaught C++ exception unwinding out of a plain
// `int (*fn)(void*)` thread function terminates the process instead of
// failing the test cleanly. Worker functions below report pass/fail via
// their int return value (0/1) or shared atomics instead, and every
// REQUIRE lives back on the main test thread after RageThread::Wait().
int MutexStressWorker(void *pData) {
	MutexStressData *pD = static_cast<MutexStressData *>(pData);
	bool bRecursiveLockOk = true;
	for (int i = 0; i < INCREMENTS_PER_THREAD; ++i) {
		pD->pMutex->Lock();
		// Weave in a recursive-lock check: RageMutex is refcounted per
		// thread (StepMania-R's own design, matching Windows mutex
		// semantics), so a second Lock() from the same thread must not
		// deadlock or require a second physical unlock to release.
		pD->pMutex->Lock();
		bRecursiveLockOk = bRecursiveLockOk && pD->pMutex->IsLockedByThisThread();
		++(*pD->pCounter);
		pD->pMutex->Unlock();
		pD->pMutex->Unlock();
	}
	return bRecursiveLockOk ? 1 : 0;
}

} // namespace

TEST_CASE("RageMutex protects a shared counter across real threads", "[RageThreads]") {
	EngineTestEnv::Require();

	RageMutex mutex("StressTestMutex");
	long counter = 0;
	MutexStressData data{&mutex, &counter};

	std::vector<RageThread> threads(NUM_WORKER_THREADS);
	for (int i = 0; i < NUM_WORKER_THREADS; ++i) {
		threads[static_cast<std::size_t>(i)].SetName(ssprintf("MutexStress%d", i));
		threads[static_cast<std::size_t>(i)].Create(MutexStressWorker, &data);
	}
	bool bAllRecursiveLocksOk = true;
	for (int i = 0; i < NUM_WORKER_THREADS; ++i)
		bAllRecursiveLocksOk = threads[static_cast<std::size_t>(i)].Wait() == 1 && bAllRecursiveLocksOk;

	REQUIRE(bAllRecursiveLocksOk);
	// If the mutex ever let two threads' critical sections overlap, this
	// would be less than the expected total (a classic lost-update race).
	REQUIRE(counter == static_cast<long>(NUM_WORKER_THREADS) * INCREMENTS_PER_THREAD);
}

namespace {

// A mutex may only be unlocked by the thread that locked it (std::mutex
// enforces this strictly; the old Win32/pthreads primitives were more
// forgiving about it, which is exactly the kind of latent bug this
// stress test exists to catch elsewhere in real code) -- so whichever
// thread's TryLock() actually succeeds must be the one to Unlock() it
// too, before it returns/terminates. Returns 1 if TryLock succeeded
// (and was cleanly unlocked here), 0 if it found the mutex contended.
int TryLockWorker(void *pData) {
	RageMutex *pMutex = static_cast<RageMutex *>(pData);
	if (!pMutex->TryLock())
		return 0;
	pMutex->Unlock();
	return 1;
}

} // namespace

TEST_CASE("RageMutex::TryLock reports real cross-thread contention", "[RageThreads]") {
	EngineTestEnv::Require();

	RageMutex mutex("TryLockTestMutex");

	mutex.Lock();
	RageThread contendingThread;
	contendingThread.SetName("TryLockContender");
	contendingThread.Create(TryLockWorker, &mutex);
	// The main thread holds the lock while this runs; TryLock must
	// observe contention rather than blocking or incorrectly succeeding.
	const int iContendedResult = contendingThread.Wait();
	mutex.Unlock();

	REQUIRE(iContendedResult == 0); // TryLock failed while we held the mutex

	RageThread uncontendedThread;
	uncontendedThread.SetName("TryLockFree");
	uncontendedThread.Create(TryLockWorker, &mutex);
	const int iFreeResult = uncontendedThread.Wait();

	REQUIRE(iFreeResult == 1); // TryLock succeeded once we released it, and unlocked cleanly
}

namespace {

const int NUM_QUEUE_ITEMS = 5000;

struct EventQueueData {
	RageEvent *pEvent; // also the monitor mutex (RageEvent : RageMutex)
	std::deque<int> *pQueue;
	std::atomic<bool> *pProducerDone;
};

int EventProducerWorker(void *pData) {
	EventQueueData *pD = static_cast<EventQueueData *>(pData);
	for (int i = 0; i < NUM_QUEUE_ITEMS; ++i) {
		pD->pEvent->Lock();
		pD->pQueue->push_back(i);
		pD->pEvent->Signal();
		pD->pEvent->Unlock();
	}
	pD->pProducerDone->store(true);
	// Wake the consumer in case it's parked waiting for "one more item"
	// right as we finish -- Signal() above only guarantees a wakeup for
	// items already pushed, not for the done-flag transition.
	pD->pEvent->Lock();
	pD->pEvent->Broadcast();
	pD->pEvent->Unlock();
	return 0;
}

int EventConsumerWorker(void *pData) {
	EventQueueData *pD = static_cast<EventQueueData *>(pData);
	long long sum = 0;
	int count = 0;

	pD->pEvent->Lock();
	for (;;) {
		while (pD->pQueue->empty() && !pD->pProducerDone->load())
			pD->pEvent->Wait();

		if (pD->pQueue->empty()) // empty and producer done: finished
			break;

		sum += pD->pQueue->front();
		pD->pQueue->pop_front();
		++count;
	}
	pD->pEvent->Unlock();

	// Stash results in the low/high halves of the return value's
	// friends -- simpler to just assert here isn't possible (wrong
	// thread for Catch2 macros to report cleanly across threads), so
	// pass both back through the shared struct instead.
	return count == NUM_QUEUE_ITEMS && sum == (static_cast<long long>(NUM_QUEUE_ITEMS - 1) * NUM_QUEUE_ITEMS) / 2 ? 1
	                                                                                                              : 0;
}

} // namespace

TEST_CASE("RageEvent hands off a producer/consumer queue without lost wakeups", "[RageThreads]") {
	EngineTestEnv::Require();

	RageEvent event("StressTestEvent");
	std::deque<int> queue;
	std::atomic<bool> producerDone{false};
	EventQueueData data{&event, &queue, &producerDone};

	RageThread producer;
	producer.SetName("EventProducer");
	RageThread consumer;
	consumer.SetName("EventConsumer");

	consumer.Create(EventConsumerWorker, &data);
	producer.Create(EventProducerWorker, &data);

	producer.Wait();
	const int iConsumerOk = consumer.Wait();

	// A lost wakeup would hang here forever (caught by ctest's TIMEOUT,
	// see this file's header comment); a race in the queue itself would
	// surface as a wrong count/sum in the consumer's return value.
	REQUIRE(iConsumerOk == 1);
}

namespace {

const int NUM_SEMA_POSTS = 5000;

struct SemaStressData {
	RageSemaphore *pSema;
	std::atomic<int> *pConsumedCount;
};

int SemaProducerWorker(void *pData) {
	SemaStressData *pD = static_cast<SemaStressData *>(pData);
	for (int i = 0; i < NUM_SEMA_POSTS; ++i)
		pD->pSema->Post();
	return 0;
}

int SemaConsumerWorker(void *pData) {
	SemaStressData *pD = static_cast<SemaStressData *>(pData);
	for (int i = 0; i < NUM_SEMA_POSTS; ++i) {
		pD->pSema->Wait();
		pD->pConsumedCount->fetch_add(1);
	}
	return 0;
}

} // namespace

TEST_CASE("RageSemaphore hands off exactly as many counts as posted", "[RageThreads]") {
	EngineTestEnv::Require();

	RageSemaphore sema("StressTestSema", 0);
	std::atomic<int> consumedCount{0};
	SemaStressData data{&sema, &consumedCount};

	RageThread producer;
	producer.SetName("SemaProducer");
	RageThread consumer;
	consumer.SetName("SemaConsumer");

	consumer.Create(SemaConsumerWorker, &data);
	producer.Create(SemaProducerWorker, &data);

	producer.Wait();
	consumer.Wait();

	REQUIRE(consumedCount.load() == NUM_SEMA_POSTS);
	REQUIRE(sema.GetValue() == 0);

	// TryWait must not block when the semaphore is at 0.
	REQUIRE_FALSE(sema.TryWait());

	sema.Post();
	REQUIRE(sema.TryWait());
	REQUIRE(sema.GetValue() == 0);
}
