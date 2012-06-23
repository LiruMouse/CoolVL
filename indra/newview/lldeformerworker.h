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

#ifndef LL_DEFORMER_WORKER_H
#define LL_DEFORMER_WORKER_H

#include <list>
#include <vector>

#include "lleventtimer.h"
#include "llmatrix4a.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llsingleton.h"
#include "llthread.h"

class LLDeformedVolume;
class LLDrawable;

class LLDeformerWorker : LLThread
{
public:

	// request that the worker compute a deform table:
	class Request : public LLRefCount
	{
	public:
		LLPointer<LLDeformedVolume> mDeformedVolume;
		LLPointer<LLDrawable> mDrawable;
		S32 mBaseShape;
		LLUUID mMeshID;
		S32 mFace;
		S32 mVertexCount;
		std::vector<LLVector3> mVolumePositions;

		LLMatrix4a mBindShapeMatrix;
		F32 mScale;
	};

	// timer handles completed requests on main thread
	class CompletedTimer : public LLEventTimer
	{
	public:
		CompletedTimer();
		BOOL tick();
	};

	LLDeformerWorker();
	virtual ~LLDeformerWorker();

	// add a request to the Q  (called from main thread)
	static void submitRequest(LLPointer<Request> request);

	// see if request is already on queue
	static bool alreadyQueued(LLPointer<Request> request);

	// do the work of building the deformation table
	void buildDeformation(LLPointer<Request> request);

	// do the necessary work on the completed queue (on the main thread)
	static void processCompletedQ();

	void shutdown();

	virtual void run();

	CompletedTimer mTimer;

private:
	void instanceSubmitRequest(LLPointer<Request> request);

	LLCondition* mSignal;
	LLMutex* mMutex;

	BOOL mQuitting;

	typedef std::list< LLPointer<Request> > request_queue;

	request_queue mRequestQ;
	request_queue mCompletedQ;
};

extern LLDeformerWorker* gDeformerWorker;

#endif
