/** 
 * @file lldeformerworker.h
 * @brief Threaded worker to compute deformation tables
 *
 * $LicenseInfo:firstyear=2012&license=viewergpl$
 * 
 * Copyright (c) 2012, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lldeformerworker.h"

#include "lldrawable.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llvovolume.h"

// timer to watch the completed queue and process completed requests
// (needs to be done on main thread, hence this timer on 1 sec repeat)
LLDeformerWorker::CompletedTimer::CompletedTimer()
:	LLEventTimer(1.0)
{
}

BOOL LLDeformerWorker::CompletedTimer::tick()
{
	LLDeformerWorker::processCompletedQ();
	return FALSE;
}

// yes, global instance - 
// LLSingleton appears to be incompatible with LLThread - they both keep track
// of the instance and want to delete it.

LLDeformerWorker* gDeformerWorker;

LLDeformerWorker::LLDeformerWorker()
:	LLThread("Deformer Worker")
{
	mQuitting = false;

	mSignal = new LLCondition(NULL);
	mMutex = new LLMutex(NULL);
}

LLDeformerWorker::~LLDeformerWorker()
{
	shutdown();

	if (mSignal)
	{
		delete mSignal;
		mSignal = NULL;
	}

	if (mMutex)
	{
		delete mMutex;
		mMutex = NULL;
	}
}

void LLDeformerWorker::shutdown()
{
	if (mSignal)
	{
		mQuitting = true;
		mSignal->signal();

		while (!isStopped())
		{
			apr_sleep(10);
		}
	}
}

// static
void LLDeformerWorker::submitRequest(LLPointer<Request> request)
{
	if (!gDeformerWorker)
	{
		gDeformerWorker = new LLDeformerWorker;
	}

	gDeformerWorker->instanceSubmitRequest(request);
}

void LLDeformerWorker::instanceSubmitRequest(LLPointer<Request> request)
{
	if (isStopped())
	{
		start();
	}

	LLMutexLock lock(mMutex);
	mRequestQ.push_back(request);
	mSignal->signal();
}

// static
bool LLDeformerWorker::alreadyQueued(LLPointer<Request> request)
{
	if (!gDeformerWorker)
	{
		return false;
	}

	for (request_queue::iterator iterator = gDeformerWorker->mRequestQ.begin();
		 iterator != gDeformerWorker->mRequestQ.end();
		 iterator++)
	{
		// the request is already here if we match MeshID, VertexCount and Face

		LLPointer<LLDeformerWorker::Request> old_request = *iterator;

		if (request->mBaseShape == old_request->mBaseShape &&
			request->mMeshID == old_request->mMeshID &&
			request->mFace == old_request->mFace &&
			request->mVertexCount == old_request->mVertexCount)
		{
			return true;
		}
	}

	return false;
}

void LLDeformerWorker::buildDeformation(LLPointer<Request> request)
{
	request->mDeformedVolume->computeDeformTable(request);
}

void LLDeformerWorker::run()
{
	while (!mQuitting)
	{
		mSignal->wait();
		while (!mQuitting && !mRequestQ.empty())
		{
			LLPointer<Request> request;

			{
				// alter Q while locked
				LLMutexLock lock(mMutex);
				request = mRequestQ.front();
				mRequestQ.pop_front();
			}

			buildDeformation(request);

			{
				// alter Q while locked
				LLMutexLock lock(mMutex);
				mCompletedQ.push_back(request);
			}
		}
	}
}

// static
void LLDeformerWorker::processCompletedQ()
{
	if (!gDeformerWorker)
	{
		return;
	}

	while (!gDeformerWorker->mCompletedQ.empty())
	{
		LLPointer<Request> request;

		{
			// alter Q while locked
			LLMutexLock lock(gDeformerWorker->mMutex);
			request = gDeformerWorker->mCompletedQ.front();
			gDeformerWorker->mCompletedQ.pop_front();
		}

		// tag the drawable for rebuild
		request->mDrawable->setState(LLDrawable::REBUILD_ALL);
	}
}
