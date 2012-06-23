/** 
 * @file llthread.cpp
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llapr.h"

#include "apr_portable.h"

#include "llthread.h"

#include "lltimer.h"

#if LL_LINUX || LL_SOLARIS
#include <sched.h>
#endif

//----------------------------------------------------------------------------
// Usage:
// void run_func(LLThread* thread)
// {
// }
// LLThread* thread = new LLThread();
// thread->run(run_func);
// ...
// thread->setQuitting();
// while(!timeout)
// {
//   if (thread->isStopped())
//   {
//     delete thread;
//     break;
//   }
// }
// 
//----------------------------------------------------------------------------

#if !LL_DARWIN
U32 ll_thread_local sThreadID = 0;
#endif 

U32 LLThread::sIDIter = 0;

LL_COMMON_API void assert_main_thread()
{
	static U32 s_thread_id = LLThread::currentID();
	if (LLThread::currentID() != s_thread_id)
	{
		llerrs << "Illegal execution outside main thread." << llendl;
	}
}

//
// Handed to the APR thread creation function
//
void *APR_THREAD_FUNC LLThread::staticRun(apr_thread_t *apr_threadp, void *datap)
{
	LLThread *threadp = (LLThread *)datap;

#if !LL_DARWIN
	sThreadID = threadp->mID;
#endif

	// Run the user supplied function
	threadp->run();

	//llinfos << "LLThread::staticRun() Exiting: " << threadp->mName << llendl;

	// We're done with the run function, this thread is done executing now.
	threadp->mStatus = STOPPED;

	return NULL;
}


LLThread::LLThread(const std::string& name, apr_pool_t *poolp) :
	mPaused(false),
	mName(name),
	mAPRThreadp(NULL),
	mStatus(STOPPED)
{
	mID = ++sIDIter;

	// Thread creation probably CAN be paranoid about APR being initialized, if necessary
	if (poolp)
	{
		mIsLocalPool = FALSE;
		mAPRPoolp = poolp;
	}
	else
	{
		mIsLocalPool = TRUE;
		apr_pool_create(&mAPRPoolp, NULL); // Create a subpool for this thread
	}
	mRunCondition = new LLCondition(mAPRPoolp);

	mLocalAPRFilePoolp = NULL;
}

LLThread::~LLThread()
{
	shutdown();

	if (mLocalAPRFilePoolp)
	{
		delete mLocalAPRFilePoolp;
		mLocalAPRFilePoolp = NULL;
	}
}

void LLThread::shutdown()
{
	// Warning!  If you somehow call the thread destructor from itself,
	// the thread will die in an unclean fashion!
	if (mAPRThreadp)
	{
		if (!isStopped())
		{
			// The thread isn't already stopped
			// First, set the flag that indicates that we're ready to die
			setQuitting();

			//llinfos << "LLThread::~LLThread() Killing thread " << mName << " Status: " << mStatus << llendl;
			// Now wait a bit for the thread to exit
			// It's unclear whether I should even bother doing this - this destructor
			// should netver get called unless we're already stopped, really...
			S32 counter = 0;
			const S32 MAX_WAIT = 600;
			while (counter < MAX_WAIT)
			{
				if (isStopped())
				{
					break;
				}
				// Sleep for a tenth of a second
				ms_sleep(100);
				yield();
				counter++;
			}
		}

		if (!isStopped())
		{
			// This thread just wouldn't stop, even though we gave it time
			//llwarns << "LLThread::shutdown() exiting thread before clean exit!" << llendl;
			// Put a stake in its heart.
			apr_thread_exit(mAPRThreadp, -1);
			return;
		}
		mAPRThreadp = NULL;
	}

	delete mRunCondition;
	mRunCondition = 0;
	
	if (mIsLocalPool && mAPRPoolp)
	{
		apr_pool_destroy(mAPRPoolp);
		mAPRPoolp = 0;
	}
}


void LLThread::start()
{
	llassert(isStopped());

	// Set thread state to running
	mStatus = RUNNING;

	apr_status_t status =
		apr_thread_create(&mAPRThreadp, NULL, staticRun, (void *)this, mAPRPoolp);

	if (status == APR_SUCCESS)
	{
		// We won't bother joining
		apr_thread_detach(mAPRThreadp);
	}
	else
	{
		mStatus = STOPPED;
		llwarns << "failed to start thread " << mName << llendl;
		ll_apr_warn_status(status);
	}
}

//============================================================================
// Called from MAIN THREAD.

// Request that the thread pause/resume.
// The thread will pause when (and if) it calls checkPause()
void LLThread::pause()
{
	if (!mPaused)
	{
		// this will cause the thread to stop execution as soon as checkPause() is called
		mPaused = true;		// Does not need to be atomic since this is only set/unset from the main thread
	}
}

void LLThread::unpause()
{
	if (mPaused)
	{
		mPaused = false;
	}

	wake(); // wake up the thread if necessary
}

// virtual predicate function -- returns true if the thread should wake up, false if it should sleep.
bool LLThread::runCondition(void)
{
	// by default, always run.  Handling of pause/unpause is done regardless of this function's result.
	return true;
}

//============================================================================
// Called from run() (CHILD THREAD).
// Stop thread execution if requested until unpaused.
void LLThread::checkPause()
{
	mRunCondition->lock();

	// This is in a while loop because the pthread API allows for spurious wakeups.
	while(shouldSleep())
	{
		mRunCondition->wait(); // unlocks mRunCondition
		// mRunCondition is locked when the thread wakes up
	}

 	mRunCondition->unlock();
}

//============================================================================

void LLThread::setQuitting()
{
	mRunCondition->lock();
	if (mStatus == RUNNING)
	{
		mStatus = QUITTING;
	}
	mRunCondition->unlock();
	wake();
}

// static
U32 LLThread::currentID()
{
	return (U32)apr_os_thread_current();
}

// static
void LLThread::yield()
{
#if LL_LINUX || LL_SOLARIS
	sched_yield(); // annoyingly, apr_thread_yield  is a noop on linux...
#else
	apr_thread_yield();
#endif
}

void LLThread::wake()
{
	mRunCondition->lock();
	if (!shouldSleep())
	{
		mRunCondition->signal();
	}
	mRunCondition->unlock();
}

void LLThread::wakeLocked()
{
	if (!shouldSleep())
	{
		mRunCondition->signal();
	}
}

//============================================================================

LLMutex::LLMutex(apr_pool_t *poolp) :
	mAPRMutexp(NULL), mCount(0), mLockingThread(NO_THREAD)
{
	//if (poolp)
	//{
	//	mIsLocalPool = FALSE;
	//	mAPRPoolp = poolp;
	//}
	//else
	{
		mIsLocalPool = TRUE;
		apr_pool_create(&mAPRPoolp, NULL); // Create a subpool for this thread
	}
	apr_thread_mutex_create(&mAPRMutexp, APR_THREAD_MUTEX_UNNESTED, mAPRPoolp);
}

LLMutex::~LLMutex()
{
#if MUTEX_DEBUG
	//bad assertion, the subclass LLSignal might be "locked", and that's OK
	//llassert_always(!isLocked()); // better not be locked!
#endif
	apr_thread_mutex_destroy(mAPRMutexp);
	mAPRMutexp = NULL;
	if (mIsLocalPool)
	{
		apr_pool_destroy(mAPRPoolp);
	}
}

void LLMutex::lock()
{
	if (isSelfLocked())
	{	//redundant lock
		mCount++;
		return;
	}
	
	apr_thread_mutex_lock(mAPRMutexp);
	
#if MUTEX_DEBUG
	// Have to have the lock before we can access the debug info
	U32 id = LLThread::currentID();
	if (mIsLocked[id] != FALSE)
		llerrs << "Already locked in Thread: " << id << llendl;
	mIsLocked[id] = TRUE;
#endif

#if LL_DARWIN
	mLockingThread = LLThread::currentID();
#else
	mLockingThread = sThreadID;
#endif
}

void LLMutex::unlock()
{
	if (mCount > 0)
	{	//not the root unlock
		mCount--;
		return;
	}
	
#if MUTEX_DEBUG
	// Access the debug info while we have the lock
	U32 id = LLThread::currentID();
	if (mIsLocked[id] != TRUE)
		llerrs << "Not locked in Thread: " << id << llendl;	
	mIsLocked[id] = FALSE;
#endif

	mLockingThread = NO_THREAD;
	apr_thread_mutex_unlock(mAPRMutexp);
}

bool LLMutex::isLocked()
{
	apr_status_t status = apr_thread_mutex_trylock(mAPRMutexp);
	if (APR_STATUS_IS_EBUSY(status))
	{
		return true;
	}
	else
	{
		apr_thread_mutex_unlock(mAPRMutexp);
		return false;
	}
}

bool LLMutex::isSelfLocked()
{
#if LL_DARWIN
	return mLockingThread == LLThread::currentID();
#else
	return mLockingThread == sThreadID;
#endif
}

U32 LLMutex::lockingThread() const
{
	return mLockingThread;
}

//============================================================================

LLCondition::LLCondition(apr_pool_t *poolp) :
	LLMutex(poolp)
{
	// base class (LLMutex) has already ensured that mAPRPoolp is set up.

	apr_thread_cond_create(&mAPRCondp, mAPRPoolp);
}

LLCondition::~LLCondition()
{
	apr_thread_cond_destroy(mAPRCondp);
	mAPRCondp = NULL;
}

void LLCondition::wait()
{
	if (!isLocked())
	{	//mAPRMutexp MUST be locked before calling apr_thread_cond_wait
		apr_thread_mutex_lock(mAPRMutexp);
#if MUTEX_DEBUG
		// avoid asserts on destruction in non-release builds
		U32 id = LLThread::currentID();
		mIsLocked[id] = TRUE;
#endif
	}
	apr_thread_cond_wait(mAPRCondp, mAPRMutexp);
}

void LLCondition::signal()
{
	apr_thread_cond_signal(mAPRCondp);
}

void LLCondition::broadcast()
{
	apr_thread_cond_broadcast(mAPRCondp);
}

//============================================================================

//----------------------------------------------------------------------------

//static
LLMutex* LLThreadSafeRefCount::sMutex = 0;

//static
void LLThreadSafeRefCount::initThreadSafeRefCount()
{
	if (!sMutex)
	{
		sMutex = new LLMutex(0);
	}
}

//static
void LLThreadSafeRefCount::cleanupThreadSafeRefCount()
{
	delete sMutex;
	sMutex = NULL;
}

//----------------------------------------------------------------------------

LLThreadSafeRefCount::LLThreadSafeRefCount()
:	mRef(0)
{
}

LLThreadSafeRefCount::~LLThreadSafeRefCount()
{ 
	if (mRef != 0)
	{
		llerrs << "deleting non-zero reference" << llendl;
	}
}

//============================================================================

LLResponder::~LLResponder()
{
}

//============================================================================
