/**
 * @file llvovolume.cpp
 * @brief LLVOVolume class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

// A "volume" is a box, cylinder, sphere, or other primitive shape.

#include "llviewerprecompiledheaders.h"

#include "llvovolume.h"

#include <sstream>

#include "lldir.h"
#include "llmaterialtable.h"
#include "llmatrix4a.h"
#include "llprimitive.h"
#include "llvector4a.h"
#include "llvolume.h"
#include "llvolumemessage.h"
#include "llvolumemgr.h"
#include "llvolumeoctree.h"
#include "material_codes.h"
#include "message.h"
#include "object_flags.h"

//MK
#include "llagent.h"
//mk
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llflexibleobject.h"
#include "llfloatertools.h"
#include "llhudmanager.h"
#include "llmeshrepository.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llspatialpartition.h"
#include "lltexturefetch.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertextureanim.h"
#include "llviewertexturelist.h"
#include "llvoavatar.h"
#include "llworld.h"
#include "pipeline.h"

#ifdef MEDIA_ON_PRIM
#include "llmediaentry.h"
#include "llpluginclassmedia.h"		// for code in the mediaEvent handler

#include "llmediadataclient.h"
#include "llviewermediafocus.h"
#endif

const S32 MIN_QUIET_FRAMES_COALESCE = 30;
const F32 FORCE_SIMPLE_RENDER_AREA = 512.f;
const F32 FORCE_CULL_AREA = 8.f;
const F32 MAX_LOD_DISTANCE = 24.f;

BOOL gAnimateTextures = TRUE;

F32 LLVOVolume::sLODFactor = 1.f;
F32 LLVOVolume::sLODSlopDistanceFactor = 0.5f; //Changing this to zero, effectively disables the LOD transition slop
F32 LLVOVolume::sDistanceFactor = 1.0f;
S32 LLVOVolume::sNumLODChanges = 0;

#ifdef MEDIA_ON_PRIM
LLPointer<LLObjectMediaDataClient> LLVOVolume::sObjectMediaClient = NULL;
LLPointer<LLObjectMediaNavigateClient> LLVOVolume::sObjectMediaNavigateClient = NULL;

// Implementation class of LLMediaDataClientObject.  See llmediadataclient.h
class LLMediaDataClientObjectImpl : public LLMediaDataClientObject
{
public:
	LLMediaDataClientObjectImpl(LLVOVolume *obj, bool isNew) : mObject(obj), mNew(isNew)
	{
		mObject->addMDCImpl();
	}
	~LLMediaDataClientObjectImpl()
	{
		mObject->removeMDCImpl();
	}

	virtual U8 getMediaDataCount() const
		{ return mObject->getNumTEs(); }

	virtual LLSD getMediaDataLLSD(U8 index) const
		{
			LLSD result;
			LLTextureEntry *te = mObject->getTE(index);
			if (NULL != te)
			{
				llassert((te->getMediaData() != NULL) == te->hasMedia());
				if (te->getMediaData() != NULL)
				{
					result = te->getMediaData()->asLLSD();
					// XXX HACK: workaround bug in asLLSD() where whitelist is not set properly
					// See DEV-41949
					if (!result.has(LLMediaEntry::WHITELIST_KEY))
					{
						result[LLMediaEntry::WHITELIST_KEY] = LLSD::emptyArray();
					}
				}
			}
			return result;
		}
	virtual bool isCurrentMediaUrl(U8 index, const std::string &url) const
		{
			LLTextureEntry *te = mObject->getTE(index);
			if (te)
			{
				if (te->getMediaData())
				{
					return (te->getMediaData()->getCurrentURL() == url);
				}
			}
			return url.empty();
		}

	virtual LLUUID getID() const
		{ return mObject->getID(); }

	virtual void mediaNavigateBounceBack(U8 index)
		{ mObject->mediaNavigateBounceBack(index); }

	virtual bool hasMedia() const
		{ return mObject->hasMedia(); }

	virtual void updateObjectMediaData(LLSD const &data, const std::string &version_string)
		{ mObject->updateObjectMediaData(data, version_string); }

	virtual F64 getMediaInterest() const
		{
			F64 interest = mObject->getTotalMediaInterest();
			if (interest < (F64)0.0)
			{
				// media interest not valid yet, try pixel area
				interest = mObject->getPixelArea();
				// HACK: force recalculation of pixel area if interest is the "magic default" of 1024.
				if (interest == 1024.f)
				{
					const_cast<LLVOVolume*>(static_cast<LLVOVolume*>(mObject))->setPixelAreaAndAngle(gAgent);
					interest = mObject->getPixelArea();
				}
			}
			return interest;
		}

	virtual bool isInterestingEnough() const
		{
			return LLViewerMedia::isInterestingEnough(mObject, getMediaInterest());
		}

	virtual std::string getCapabilityUrl(const std::string &name) const
		{ return mObject->getRegion()->getCapability(name); }

	virtual bool isDead() const
		{ return mObject->isDead(); }

	virtual U32 getMediaVersion() const
		{ return LLTextureEntry::getVersionFromMediaVersionString(mObject->getMediaURL()); }

	virtual bool isNew() const
		{ return mNew; }

private:
	LLPointer<LLVOVolume> mObject;
	bool mNew;
};
#endif

LLVOVolume::LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	: LLViewerObject(id, pcode, regionp),
	  mVolumeImpl(NULL)
{
	mTexAnimMode = 0;
	mRelativeXform.setIdentity();
	mRelativeXformInvTrans.setIdentity();

	mFaceMappingChanged = FALSE;
	mLOD = MIN_LOD;
	mTextureAnimp = NULL;
	mVolumeChanged = FALSE;
	mVObjRadius = LLVector3(1,1,0.5f).length();
	mNumFaces = 0;
	mLODChanged = FALSE;
	mSculptChanged = FALSE;
	mSpotLightPriority = 0.f;

#ifdef MEDIA_ON_PRIM
	mMediaImplList.resize(getNumTEs());
	mLastFetchedMediaVersion = -1;
	mIndexInTex = 0;
	mMDCImplCount = 0;
#endif
}

LLVOVolume::~LLVOVolume()
{
	delete mTextureAnimp;
	mTextureAnimp = NULL;
	delete mVolumeImpl;
	mVolumeImpl = NULL;

#ifdef MEDIA_ON_PRIM
	if (!mMediaImplList.empty())
	{
		for (U32 i = 0; i < mMediaImplList.size(); ++i)
		{
			if (mMediaImplList[i].notNull())
			{
				mMediaImplList[i]->removeObject(this);
			}
		}
	}
#endif
}

void LLVOVolume::markDead()
{
	if (!mDead)
	{
#ifdef MEDIA_ON_PRIM
		if (getMDCImplCount() > 0)
		{
			LLMediaDataClientObject::ptr_t obj = new LLMediaDataClientObjectImpl(const_cast<LLVOVolume*>(this), false);
			if (sObjectMediaClient) sObjectMediaClient->removeFromQueue(obj);
			if (sObjectMediaNavigateClient) sObjectMediaNavigateClient->removeFromQueue(obj);
		}

		// Detach all media impls from this object
		for (U32 i = 0; i < mMediaImplList.size(); ++i)
		{
			removeMediaImpl(i);
		}
#endif

		if (mSculptTexture.notNull())
		{
			mSculptTexture->removeVolume(this);
		}
	}

	LLViewerObject::markDead();
}

// static
void LLVOVolume::initClass()
{
#ifdef MEDIA_ON_PRIM
	// gSavedSettings better be around
	if (gSavedSettings.getBOOL("PrimMediaMasterEnabled"))
	{
		const F32 queue_timer_delay = gSavedSettings.getF32("PrimMediaRequestQueueDelay");
		const F32 retry_timer_delay = gSavedSettings.getF32("PrimMediaRetryTimerDelay");
		const U32 max_retries = gSavedSettings.getU32("PrimMediaMaxRetries");
		const U32 max_sorted_queue_size = gSavedSettings.getU32("PrimMediaMaxSortedQueueSize");
		const U32 max_round_robin_queue_size = gSavedSettings.getU32("PrimMediaMaxRoundRobinQueueSize");
		sObjectMediaClient = new LLObjectMediaDataClient(queue_timer_delay, retry_timer_delay, max_retries,
														 max_sorted_queue_size, max_round_robin_queue_size);
		sObjectMediaNavigateClient = new LLObjectMediaNavigateClient(queue_timer_delay, retry_timer_delay,
																	 max_retries, max_sorted_queue_size, max_round_robin_queue_size);
	}
#endif
}

#ifdef MEDIA_ON_PRIM
// static
void LLVOVolume::cleanupClass()
{
    sObjectMediaClient = NULL;
    sObjectMediaNavigateClient = NULL;
}
#endif

U32 LLVOVolume::processUpdateMessage(LLMessageSystem *mesgsys,
									 void **user_data,
									 U32 block_num, EObjectUpdateType update_type,
									 LLDataPacker *dp)
{
	LLColor4U color;
#ifdef MEDIA_ON_PRIM
	const S32 teDirtyBits = (TEM_CHANGE_TEXTURE | TEM_CHANGE_COLOR | TEM_CHANGE_MEDIA);
#else
	const S32 teDirtyBits = (TEM_CHANGE_TEXTURE | TEM_CHANGE_COLOR);
#endif

	// Do base class updates...
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);

	LLUUID sculpt_id;
	U8 sculpt_type = 0;
	if (isSculpted())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		sculpt_id = sculpt_params->getSculptTexture();
		sculpt_type = sculpt_params->getSculptType();
	}

	if (!dp)
	{
		if (update_type == OUT_FULL)
		{
			////////////////////////////////
			//
			// Unpack texture animation data
			//
			//

			if (mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_TextureAnim))
			{
				if (!mTextureAnimp)
				{
					mTextureAnimp = new LLViewerTextureAnim();
				}
				else
				{
					if (!(mTextureAnimp->mMode & LLTextureAnim::SMOOTH))
					{
						mTextureAnimp->reset();
					}
				}
				mTexAnimMode = 0;
				mTextureAnimp->unpackTAMessage(mesgsys, block_num);
			}
			else
			{
				if (mTextureAnimp)
				{
					delete mTextureAnimp;
					mTextureAnimp = NULL;
					gPipeline.markTextured(mDrawable);
					mFaceMappingChanged = TRUE;
					mTexAnimMode = 0;
				}
			}

			// Unpack volume data
			LLVolumeParams volume_params;
			LLVolumeMessage::unpackVolumeParams(&volume_params, mesgsys, _PREHASH_ObjectData, block_num);
			volume_params.setSculptID(sculpt_id, sculpt_type);

			if (setVolume(volume_params, 0))
			{
				markForUpdate(TRUE);
			}
		}

		// Sigh, this needs to be done AFTER the volume is set as well, otherwise bad stuff happens...
		////////////////////////////
		//
		// Unpack texture entry data
		//
		S32 result = unpackTEMessage(mesgsys, _PREHASH_ObjectData, block_num);
		if (result & teDirtyBits)
		{
			updateTEData();
		}
#ifdef MEDIA_ON_PRIM
		if (result & TEM_CHANGE_MEDIA)
		{
			retval |= MEDIA_FLAGS_CHANGED;
		}
#endif
	}
	else
	{
		// CORY TO DO: Figure out how to get the value here
		if (update_type != OUT_TERSE_IMPROVED)
		{
			LLVolumeParams volume_params;
			BOOL res = LLVolumeMessage::unpackVolumeParams(&volume_params, *dp);
			if (!res)
			{
				LL_WARNS("RenderVolume") << "Bogus volume parameters in object "
										 << getID() << " at "
										 << getRegion()->getOriginGlobal()
										 << LL_ENDL;
			}

			volume_params.setSculptID(sculpt_id, sculpt_type);

			if (setVolume(volume_params, 0))
			{
				markForUpdate(TRUE);
			}
			S32 res2 = unpackTEMessage(*dp);
			if (TEM_INVALID == res2)
			{
				// There's something bogus in the data that we're unpacking.
				dp->dumpBufferToLog();
				LL_WARNS("RenderVolume") << "Flushing cache files" << LL_ENDL;

				std::string mask;
				mask = gDirUtilp->getDirDelimiter() + "*.slc";
				gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,""), mask);

				LL_WARNS("RenderVolume") << "Bogus TE data in " << getID() << LL_ENDL;
			}
			else
			{
				if (res2 & teDirtyBits)
				{
					updateTEData();
				}
#ifdef MEDIA_ON_PRIM
				if (res2 & TEM_CHANGE_MEDIA)
				{
					retval |= MEDIA_FLAGS_CHANGED;
				}
#endif
			}

			U32 value = dp->getPassFlags();

			if (value & 0x40)
			{
				if (!mTextureAnimp)
				{
					mTextureAnimp = new LLViewerTextureAnim();
				}
				else
				{
					if (!(mTextureAnimp->mMode & LLTextureAnim::SMOOTH))
					{
						mTextureAnimp->reset();
					}
				}
				mTexAnimMode = 0;
				mTextureAnimp->unpackTAMessage(*dp);
			}
			else if (mTextureAnimp)
			{
				delete mTextureAnimp;
				mTextureAnimp = NULL;
				gPipeline.markTextured(mDrawable);
				mFaceMappingChanged = TRUE;
				mTexAnimMode = 0;
			}
		}
		else
		{
			S32 texture_length = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_TextureEntry);
			if (texture_length)
			{
				U8							tdpbuffer[1024];
				LLDataPackerBinaryBuffer	tdp(tdpbuffer, 1024);
				mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextureEntry, tdpbuffer, 0, block_num);
				S32 result = unpackTEMessage(tdp);
				if (result & teDirtyBits)
				{
					updateTEData();
				}
#ifdef MEDIA_ON_PRIM
				if (result & TEM_CHANGE_MEDIA)
				{
					retval |= MEDIA_FLAGS_CHANGED;
				}
#endif
			}
		}
	}
#ifdef MEDIA_ON_PRIM
	if (retval & (MEDIA_URL_REMOVED | MEDIA_URL_ADDED | MEDIA_URL_UPDATED | MEDIA_FLAGS_CHANGED))
	{
		// If only the media URL changed, and it isn't a media version URL,
		// ignore it
		if (!((retval & (MEDIA_URL_ADDED | MEDIA_URL_UPDATED)) &&
			mMedia && ! mMedia->mMediaURL.empty() &&
			!LLTextureEntry::isMediaVersionString(mMedia->mMediaURL)))
		{
			// If the media changed at all, request new media data
			LL_DEBUGS("MediaOnAPrim") << "Media update: " << getID()
									  << ": retval=" << retval << " Media URL: "
									  << (mMedia ?  mMedia->mMediaURL : std::string(""))
									  << LL_ENDL;
			requestMediaDataUpdate(retval & MEDIA_FLAGS_CHANGED);
		}
        else
		{
            LL_INFOS("MediaOnAPrim") << "Ignoring media update for: " << getID()
									 << " Media URL: "
									 << (mMedia ?  mMedia->mMediaURL : std::string(""))
									 << LL_ENDL;
        }
	}
	// ...and clean up any media impls
	cleanUpMediaImpls();
#endif

	return retval;
}

void LLVOVolume::animateTextures()
{
	F32 off_s = 0.f, off_t = 0.f, scale_s = 1.f, scale_t = 1.f, rot = 0.f;
	S32 result = mTextureAnimp->animateTextures(off_s, off_t, scale_s, scale_t, rot);

	if (result)
	{
		if (!mTexAnimMode)
		{
			mFaceMappingChanged = TRUE;
			gPipeline.markTextured(mDrawable);
		}
		mTexAnimMode = result | mTextureAnimp->mMode;

		S32 start = 0, end = mDrawable->getNumFaces() - 1;
		if (mTextureAnimp->mFace >= 0 && mTextureAnimp->mFace <= end)
		{
			start = end = mTextureAnimp->mFace;
		}

		for (S32 i = start; i <= end; ++i)
		{
			LLFace* facep = mDrawable->getFace(i);
			if (!facep ||
				(facep->getVirtualSize() <= MIN_TEX_ANIM_SIZE &&
				 facep->mTextureMatrix))
			{
				continue;
			}

			const LLTextureEntry* te = facep->getTextureEntry();

			if (!te)
			{
				continue;
			}

			if (!(result & LLViewerTextureAnim::ROTATE))
			{
				te->getRotation(&rot);
			}
			if (!(result & LLViewerTextureAnim::TRANSLATE))
			{
				te->getOffset(&off_s,&off_t);
			}
			if (!(result & LLViewerTextureAnim::SCALE))
			{
				te->getScale(&scale_s, &scale_t);
			}

			if (!facep->mTextureMatrix)
			{
				facep->mTextureMatrix = new LLMatrix4();
			}

			LLMatrix4& tex_mat = *facep->mTextureMatrix;
			tex_mat.setIdentity();
			LLVector3 trans;

			trans.set(LLVector3(off_s+0.5f, off_t+0.5f, 0.f));
			tex_mat.translate(LLVector3(-0.5f, -0.5f, 0.f));

			LLVector3 scale(scale_s, scale_t, 1.f);
			LLQuaternion quat;
			quat.setQuat(rot, 0, 0, -1.f);

			tex_mat.rotate(quat);

			LLMatrix4 mat;
			mat.initAll(scale, LLQuaternion(), LLVector3());
			tex_mat *= mat;

			tex_mat.translate(trans);
		}
	}
	else
	{
		if (mTexAnimMode && mTextureAnimp->mRate == 0)
		{
			U8 start, count;

			if (mTextureAnimp->mFace == -1)
			{
				start = 0;
				count = getNumTEs();
			}
			else
			{
				start = (U8) mTextureAnimp->mFace;
				count = 1;
			}

			for (S32 i = start; i < start + count; ++i)
			{
				if (mTexAnimMode & LLViewerTextureAnim::TRANSLATE)
				{
					setTEOffset(i, mTextureAnimp->mOffS, mTextureAnimp->mOffT);
				}
				if (mTexAnimMode & LLViewerTextureAnim::SCALE)
				{
					setTEScale(i, mTextureAnimp->mScaleS, mTextureAnimp->mScaleT);
				}
				if (mTexAnimMode & LLViewerTextureAnim::ROTATE)
				{
					setTERotation(i, mTextureAnimp->mRot);
				}
			}

			gPipeline.markTextured(mDrawable);
			mFaceMappingChanged = TRUE;
			mTexAnimMode = 0;
		}
	}
}
BOOL LLVOVolume::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	LLViewerObject::idleUpdate(agent, world, time);

	if (mDead || mDrawable.isNull())
	{
		return TRUE;
	}

	///////////////////////
	//
	// Do texture animation stuff
	//

	if (mTextureAnimp && gAnimateTextures)
	{
		animateTextures();
	}

	// Dispatch to implementation
	if (mVolumeImpl)
	{
		mVolumeImpl->doIdleUpdate(agent, world, time);
	}

	const S32 MAX_ACTIVE_OBJECT_QUIET_FRAMES = 40;

	if (mDrawable->isActive())
	{
		if (mDrawable->isRoot() &&
			mDrawable->mQuietCount++ > MAX_ACTIVE_OBJECT_QUIET_FRAMES &&
			(!mDrawable->getParent() || !mDrawable->getParent()->isActive()))
		{
			mDrawable->makeStatic();
		}
	}

	return TRUE;
}

void LLVOVolume::updateTextures()
{
	const F32 TEXTURE_AREA_REFRESH_TIME = 5.f; // seconds
	if (mTextureUpdateTimer.getElapsedTimeF32() > TEXTURE_AREA_REFRESH_TIME)
	{
		updateTextureVirtualSize();
	}
}

BOOL LLVOVolume::isVisible() const
{
	if (mDrawable.notNull() && mDrawable->isVisible())
	{
		return TRUE;
	}

	if (isAttachment())
	{
		LLViewerObject* objp = (LLViewerObject*)getParent();
		while (objp && !objp->isAvatar())
		{
			objp = (LLViewerObject*)objp->getParent();
		}

		return objp && objp->mDrawable.notNull() && objp->mDrawable->isVisible();
	}

	return FALSE;
}

void LLVOVolume::updateTextureVirtualSize(bool forced)
{
	// Update the pixel area of all faces

	if (!forced)
	{
		if (!isVisible())
		{
			return;
		}

		if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SIMPLE))
		{
			return;
		}
	}

	if (LLViewerTexture::sDontLoadVolumeTextures || LLAppViewer::getTextureFetch()->mDebugPause)
	{
		return;
	}

	mTextureUpdateTimer.reset();

	F32 old_area = mPixelArea;
	mPixelArea = 0.f;

	const S32 num_faces = mDrawable->getNumFaces();
	F32 min_vsize = 999999999.f, max_vsize = 0.f;
	LLViewerCamera* camera = LLViewerCamera::getInstance();
	BOOL is_ours = permYouOwner();
	LLVOAvatar* avatar = getAvatar();
	for (S32 i = 0; i < num_faces; ++i)
	{
		LLFace* face = mDrawable->getFace(i);
		if (!face) continue;

		const LLTextureEntry* te = face->getTextureEntry();
		LLViewerTexture* imagep = face->getTexture();
		if (!imagep || !te || face->mExtents[0].equals3(face->mExtents[1]))
		{
			continue;
		}

		F32 vsize;
		F32 old_size = face->getVirtualSize();

		if (isHUDAttachment())
		{
			imagep->setBoostLevel(LLViewerTexture::BOOST_HUD);
			// treat as full screen
			vsize = (F32)camera->getScreenPixelArea();
		}
		else
		{
			vsize = face->getTextureVirtualSize();
			if (isAttachment())
			{
				// Rez attachments faster and at full details !
				if (is_ours)
				{
					// Our attachments must really rez fast and fully:
					// we shouldn't have to zoom on them to get the textures
					// fully loaded !
					imagep->setBoostLevel(LLViewerTexture::BOOST_HUD);
					// ... and don't discard our attachments textures
					imagep->dontDiscard();
				}
				else if (avatar && !avatar->isImpostor())
				{
					// Others' can get their texture discarded to avoid filling
					// up the video buffers in crowded areas...
					static LLCachedControl<bool> boost_texture(gSavedSettings, "TextureBoostAttachments");
					static LLCachedControl<S32> min_vsize_sqrt(gSavedSettings, "TextureMinAttachmentsVSize");
					static LLCachedControl<F32> texture_boost_bias_thresold(gSavedSettings, "TextureBoostBiasThreshold");
					F32 high_bias = llclamp((F32)texture_boost_bias_thresold, 0.0f, 1.5f);
					if (boost_texture && LLViewerTexture::sDesiredDiscardBias <= high_bias)
					{
						// As long as the current bias is not too high (i.e. we
						// are not using too much memory), and provided that
						// the TextureBoostAttachments setting is TRUE, let's
						// boost significantly the attachments.
						// First, raise the priority to the one of selected
						// objects, causing the attachments to rez much faster
						// and preventing them to get affected by the bias
						// level (see LLViewerTexture::processTextureStats() for
						// the algorithm).
						imagep->setBoostLevel(LLViewerTexture::BOOST_SELECTED);
						// And now, the importance to the camera...
						// The problem with attachments is that they most often
						// use very small prims with high resolution textures,
						// and Snowglobe's algorithm considers such textures as
						// unimportant to the camera... Let's counter this
						// effect, using a minimum, user configurable virtual
						// size.
						F32 min_vsize = (F32)(min_vsize_sqrt * min_vsize_sqrt);
						if (vsize < min_vsize)
						{
							vsize = min_vsize;
						}
					}
					else
					{
						// Bias is at its maximum: only boost a little, not
						// preventing bias to affect this texture either.
						imagep->setBoostLevel(LLViewerTexture::BOOST_AVATAR);
					}
					// boost the decode priority too (doesn't affect memory usage)
					LLViewerFetchedTexture *tex = LLViewerTextureManager::staticCastToFetchedTexture(imagep);
					if (tex)
					{
						tex->setAdditionalDecodePriority(1.5f);
					}
				}
			}
		}

		mPixelArea = llmax(mPixelArea, face->getPixelArea());

		if (face->mTextureMatrix != NULL)
		{
			// Animating textures also rez badly in Snowglobe because the
			// actual displayed area is only a fraction (corresponding to one
			// frame) of the animating texture. Let's fix that here:
			if (mTextureAnimp && mTextureAnimp->mScaleS > 0.0f && mTextureAnimp->mScaleT > 0.0f)
			{
				// Adjust to take into account the actual frame size which is only a
				// portion of the animating texture
				vsize = vsize / mTextureAnimp->mScaleS / mTextureAnimp->mScaleT;
			}

			if ((vsize < MIN_TEX_ANIM_SIZE && old_size > MIN_TEX_ANIM_SIZE) ||
				(vsize > MIN_TEX_ANIM_SIZE && old_size < MIN_TEX_ANIM_SIZE))
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD, FALSE);
			}
		}

		face->setVirtualSize(vsize);

		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
		{
			if (vsize < min_vsize) min_vsize = vsize;
			if (vsize > max_vsize) max_vsize = vsize;
		}
		else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
		{
			LLViewerFetchedTexture* img = LLViewerTextureManager::staticCastToFetchedTexture(imagep);
			if (img)
			{
				F32 pri = img->getDecodePriority();
				pri = llmax(pri, 0.0f);
				if (pri < min_vsize) min_vsize = pri;
				if (pri > max_vsize) max_vsize = pri;
			}
		}
		else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_AREA))
		{
			F32 pri = mPixelArea;
			if (pri < min_vsize) min_vsize = pri;
			if (pri > max_vsize) max_vsize = pri;
		}
	}

	if (isSculpted())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		LLUUID id =  sculpt_params->getSculptTexture();

		updateSculptTexture();

		if (mSculptTexture.notNull())
		{
			mSculptTexture->setBoostLevel(llmax((S32)mSculptTexture->getBoostLevel(),
												(S32)LLViewerTexture::BOOST_SCULPTED));
			mSculptTexture->setForSculpt();

			if (!mSculptTexture->isCachedRawImageReady())
			{
				S32 lod = llmin(mLOD, 3);
				F32 lodf = ((F32)(lod + 1.0f) / 4.f);
				F32 tex_size = lodf * LLViewerTexture::sMaxSculptRez;
				mSculptTexture->addTextureStats(2.f * tex_size * tex_size, FALSE);

				//if the sculpty very close to the view point, load first
				{
					LLVector3 lookAt = getPositionAgent() - camera->getOrigin();
					F32 dist = lookAt.normVec();
					F32 cos_angle_to_view_dir = lookAt * camera->getXAxis();
					mSculptTexture->setAdditionalDecodePriority(0.8f * LLFace::calcImportanceToCamera(cos_angle_to_view_dir, dist));
				}
			}

			S32 texture_discard = mSculptTexture->getDiscardLevel(); //try to match the texture
			S32 current_discard = getVolume() ? getVolume()->getSculptLevel() : -2;

			if (texture_discard >= 0 && //texture has some data available
				(texture_discard < current_discard || //texture has more data than last rebuild
				current_discard < 0)) //no previous rebuild
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
				mSculptChanged = TRUE;
			}

			if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SCULPTED))
			{
				setDebugText(llformat("T%d C%d V%d\n%dx%d",
										  texture_discard, current_discard, getVolume()->getSculptLevel(),
										  mSculptTexture->getHeight(), mSculptTexture->getWidth()));
			}
		}
	}

	if (getLightTextureID().notNull())
	{
		LLLightImageParams* params = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		LLUUID id = params->getLightTexture();
		mLightTexture = LLViewerTextureManager::getFetchedTexture(id);
		if (mLightTexture.notNull())
		{
			F32 rad = getLightRadius();
			mLightTexture->addTextureStats(gPipeline.calcPixelArea(getPositionAgent(),
																	LLVector3(rad,rad,rad),
																	*camera));
		}
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
	{
		setDebugText(llformat("%.0f:%.0f", (F32)sqrt(min_vsize), (F32)sqrt(max_vsize)));
	}
 	else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY))
 	{
 		setDebugText(llformat("%.0f:%.0f", (F32)sqrt(min_vsize), (F32)sqrt(max_vsize)));
 	}
	else if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_FACE_AREA))
	{
		setDebugText(llformat("%.0f:%.0f", (F32)sqrt(min_vsize), (F32)sqrt(max_vsize)));
	}

	if (mPixelArea == 0)
	{	//flexi phasing issues make this happen
		mPixelArea = old_area;
	}
}

BOOL LLVOVolume::isActive() const
{
	return !mStatic || mTextureAnimp ||
		   (mVolumeImpl && mVolumeImpl->isActive()) ||
		   (mDrawable.notNull() && mDrawable->isActive());
}

BOOL LLVOVolume::setMaterial(const U8 material)
{
	BOOL res = LLViewerObject::setMaterial(material);

	return res;
}

void LLVOVolume::setTexture(const S32 face)
{
	llassert(face < getNumTEs());
	gGL.getTexUnit(0)->bind(getTEImage(face));
}

void LLVOVolume::setScale(const LLVector3 &scale, BOOL damped)
{
	if (scale != getScale())
	{
		// store local radius
		LLViewerObject::setScale(scale);

		if (mVolumeImpl)
		{
			mVolumeImpl->onSetScale(scale, damped);
		}

		updateRadius();

		//since drawable transforms do not include scale, changing volume scale
		//requires an immediate rebuild of volume verts.
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_POSITION, TRUE);
	}
}

LLFace* LLVOVolume::addFace(S32 f)
{
	const LLTextureEntry* te = getTE(f);
	LLViewerTexture* imagep = getTEImage(f);
	return mDrawable->addFace(te, imagep);
}

LLDrawable *LLVOVolume::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);

	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_VOLUME);

	S32 max_tes_to_set = getNumTEs();
	for (S32 i = 0; i < max_tes_to_set; ++i)
	{
		addFace(i);
	}
	mNumFaces = max_tes_to_set;

	if (isAttachment())
	{
		mDrawable->makeActive();
	}

	if (getIsLight())
	{
		// Add it to the pipeline mLightSet
		gPipeline.setLight(mDrawable, TRUE);
	}

	updateRadius();
	bool force_update = true; // avoid non-alpha mDistance update being optimized away
	mDrawable->updateDistance(*LLViewerCamera::getInstance(), force_update);

	return mDrawable;
}

BOOL LLVOVolume::setVolume(const LLVolumeParams &params_in, const S32 detail, bool unique_volume)
{
	LLVolumeParams volume_params = params_in;

	S32 last_lod = mVolumep.notNull() ? LLVolumeLODGroup::getVolumeDetailFromScale(mVolumep->getDetail()) : -1;
	S32 lod = mLOD;

	BOOL is404 = FALSE;

	if (isSculpted())
	{
		// if it's a mesh
		if ((volume_params.getSculptType() & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
		{	//meshes might not have all LODs, get the force detail to best existing LOD
			LLUUID mesh_id = volume_params.getSculptID();

			lod = gMeshRepo.getActualMeshLOD(volume_params, lod);
			if (lod == -1)
			{
				is404 = TRUE;
				lod = 0;
			}
		}
	}

	// Check if we need to change implementations
	bool is_flexible = (volume_params.getPathParams().getCurveType() == LL_PCODE_PATH_FLEXIBLE);
	if (is_flexible)
	{
		setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, TRUE, false);
		if (!mVolumeImpl)
		{
			LLFlexibleObjectData* data = (LLFlexibleObjectData*)getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
			mVolumeImpl = new LLVolumeImplFlexible(this, data);
		}
	}
	else
	{
		// Mark the parameter not in use
		setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, FALSE, false);
		if (mVolumeImpl)
		{
			delete mVolumeImpl;
			mVolumeImpl = NULL;
			if (mDrawable.notNull())
			{
				// Undo the damage we did to this matrix
				mDrawable->updateXform(FALSE);
			}
		}
	}

	if (is404)
	{
		setIcon(LLViewerTextureManager::getFetchedTextureFromFile("inv_item_mesh.tga", TRUE, LLViewerTexture::BOOST_UI));
		//render prim proxy when mesh loading attempts give up
		volume_params.setSculptID(LLUUID::null, LL_SCULPT_TYPE_NONE);
	}

	BOOL res = LLPrimitive::setVolume(volume_params, lod, mVolumeImpl && mVolumeImpl->isVolumeUnique());
	if (res || mSculptChanged)
	{
		mFaceMappingChanged = TRUE;

		if (mVolumeImpl)
		{
			mVolumeImpl->onSetVolume(volume_params, mLOD);
		}

		updateSculptTexture();

		if (isSculpted())
		{
			updateSculptTexture();
			// if it's a mesh
			if ((volume_params.getSculptType() & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
			{
				if (!getVolume()->isMeshAssetLoaded())
				{
					//load request not yet issued, request pipeline load this mesh
					LLUUID asset_id = volume_params.getSculptID();
					S32 available_lod = gMeshRepo.loadMesh(this, volume_params, lod, last_lod);
					if (available_lod != lod)
					{
						LLPrimitive::setVolume(volume_params, available_lod);
					}
				}
			}
			else // otherwise is sculptie
			{
				if (mSculptTexture.notNull())
				{
					sculpt();
				}
			}
		}

		return TRUE;
	}
	return FALSE;
}

void LLVOVolume::updateSculptTexture()
{
	LLPointer<LLViewerFetchedTexture> old_sculpt = mSculptTexture;

	if (isSculpted() && !isMesh())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		LLUUID id =  sculpt_params->getSculptTexture();
		if (id.notNull())
		{
			mSculptTexture = LLViewerTextureManager::getFetchedTexture(id, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
		}
	}
	else
	{
		mSculptTexture = NULL;
	}

	if (mSculptTexture != old_sculpt)
	{
		if (old_sculpt.notNull())
		{
			old_sculpt->removeVolume(this);
		}
		if (mSculptTexture.notNull())
		{
			mSculptTexture->addVolume(this);
		}
	}
}

void LLVOVolume::notifyMeshLoaded()
{
	mSculptChanged = TRUE;
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
}

// sculpt replaces generate() for sculpted surfaces
void LLVOVolume::sculpt()
{
	if (mSculptTexture.notNull())
	{
		U16 sculpt_height = 0;
		U16 sculpt_width = 0;
		S8 sculpt_components = 0;
		const U8* sculpt_data = NULL;

		S32 discard_level = mSculptTexture->getDiscardLevel();
		LLImageRaw* raw_image = mSculptTexture->getCachedRawImage();

		S32 max_discard = mSculptTexture->getMaxDiscardLevel();
		if (discard_level > max_discard)
			discard_level = max_discard;    // clamp to the best we can do

		S32 current_discard = getVolume()->getSculptLevel();
		if (current_discard < -2)
		{
			LL_WARNS("RenderVolume") << "Current discard of sculpty at "
									 << current_discard  << " is less than -2 !"
									 << LL_ENDL;

			// corrupted volume... don't update the sculpty
			return;
		}
		else if (current_discard > MAX_DISCARD_LEVEL)
		{
			LL_WARNS("RenderVolume") << "Current discard of sculpty at "
									 << current_discard
									 << " is more than the allowed max of "
									 << MAX_DISCARD_LEVEL << LL_ENDL;

			// corrupted volume... don't update the sculpty
			return;
		}

		if (current_discard == discard_level)  // no work to do here
			return;

		if (!raw_image)
		{
			llassert(discard_level < 0);

			sculpt_width = 0;
			sculpt_height = 0;
			sculpt_data = NULL;
		}
		else
		{
			sculpt_height = raw_image->getHeight();
			sculpt_width = raw_image->getWidth();
			sculpt_components = raw_image->getComponents();

			sculpt_data = raw_image->getData();
		}
		getVolume()->sculpt(sculpt_width, sculpt_height, sculpt_components, sculpt_data, discard_level);

		//notify rebuild any other VOVolumes that reference this sculpty volume
		for (S32 i = 0; i < mSculptTexture->getNumVolumes(); ++i)
		{
			LLVOVolume* volume = (*(mSculptTexture->getVolumeList()))[i];
			if (volume != this && volume->getVolume() == getVolume())
			{
				gPipeline.markRebuild(volume->mDrawable, LLDrawable::REBUILD_GEOMETRY, FALSE);
			}
		}
	}
}

S32	LLVOVolume::computeLODDetail(F32 distance, F32 radius)
{
	S32	cur_detail;
	if (LLPipeline::sDynamicLOD)
	{
		// We've got LOD in the profile, and in the twist. Use radius.
		F32 tan_angle = LLVOVolume::sLODFactor * radius / distance;
		cur_detail = LLVolumeLODGroup::getDetailFromTan(llround(tan_angle, 0.01f));
	}
	else
	{
		cur_detail = llclamp((S32)(sqrtf(radius) * LLVOVolume::sLODFactor * 4.f),
							 0, 3);
	}
	return cur_detail;
}

BOOL LLVOVolume::calcLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}

	S32 cur_detail = 0;

	F32 radius;
	F32 distance;

	if (mDrawable->isState(LLDrawable::RIGGED))
	{
		LLVOAvatar* avatar = getAvatar();
		if (!avatar)
		{
			llwarns << "NULL avatar pointer for rigged drawable" << llendl;
			return FALSE;
		}
		if (avatar->mDrawable.isNull())
		{
			llwarns << "No drawable for avatar associated to rigged drawable"
					<< llendl;
			return FALSE;
		}
		distance = avatar->mDrawable->mDistanceWRTCamera;
		radius = avatar->getBinRadius();
	}
	else
	{
		if (!getVolume())
		{
			llwarns << "NULL volume associate to drawable" << llendl;
			return FALSE;
		}
		distance = mDrawable->mDistanceWRTCamera;
		radius = getVolume()->mLODScaleBias.scaledVec(getScale()).length();
	}

	//hold onto unmodified distance for debugging
	//F32 debug_distance = distance;

	distance *= sDistanceFactor;

	F32 rampDist = LLVOVolume::sLODFactor * 2;
	if (distance < rampDist && rampDist > 0.0f)
	{
		// Boost LOD when you're REALLY close
		distance *= 1.0f / rampDist;
		distance *= distance;
		distance *= rampDist;
	}

	// DON'T Compensate for field of view changing on FOV zoom.
	distance *= F_PI / 3.f;

	cur_detail = computeLODDetail(llround(distance, 0.01f),
								  llround(radius, 0.01f));

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_LOD_INFO))
	{
		//setDebugText(llformat("%.2f:%.2f, %d", debug_distance, radius, cur_detail));
		setDebugText(llformat("%d", cur_detail));
	}

	if (cur_detail != mLOD)
	{
		mAppAngle = llround((F32)atan2(mDrawable->getRadius(),
									   mDrawable->mDistanceWRTCamera) * RAD_TO_DEG,
							0.01f);
		mLOD = cur_detail;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL LLVOVolume::updateLOD()
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}

	BOOL lod_changed = calcLOD();

	if (lod_changed)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, FALSE);
		mLODChanged = TRUE;
	}
	else
	{
		F32 new_radius = getBinRadius();
		F32 old_radius = mDrawable->getBinRadius();
		if (new_radius < old_radius * 0.9f || new_radius > old_radius * 1.1f)
		{
			gPipeline.markPartitionMove(mDrawable);
		}
	}

	lod_changed = lod_changed || LLViewerObject::updateLOD();

	return lod_changed;
}

BOOL LLVOVolume::setDrawableParent(LLDrawable* parentp)
{
	if (!LLViewerObject::setDrawableParent(parentp))
	{
		// no change in drawable parent
		return FALSE;
	}

	if (!mDrawable->isRoot())
	{
		// rebuild vertices in parent relative space
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);

		if (mDrawable->isActive() && !parentp->isActive())
		{
			parentp->makeActive();
		}
		else if (mDrawable->isStatic() && parentp->isActive())
		{
			mDrawable->makeActive();
		}
	}

	return TRUE;
}

void LLVOVolume::updateFaceFlags()
{
	for (S32 i = 0, count = getVolume()->getNumFaces(); i < count; ++i)
	{
		LLFace* face = mDrawable->getFace(i);
		if (!face)
		{
			return;
		}
		BOOL fullbright = getTE(i)->getFullbright();
		face->clearState(LLFace::FULLBRIGHT | LLFace::HUD_RENDER | LLFace::LIGHT);

		if (fullbright || (mMaterial == LL_MCODE_LIGHT))
		{
			face->setState(LLFace::FULLBRIGHT);
		}
		if (mDrawable->isLight())
		{
			face->setState(LLFace::LIGHT);
		}
		if (isHUDAttachment())
		{
			face->setState(LLFace::HUD_RENDER);
		}
	}
}

BOOL LLVOVolume::setParent(LLViewerObject* parent)
{
	BOOL ret = FALSE;
	if (parent != getParent())
	{
		ret = LLViewerObject::setParent(parent);
		if (ret && mDrawable)
		{
			gPipeline.markMoved(mDrawable);
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
		}
	}

	return ret;
}

// NOTE: regenFaces() MUST be followed by genTriangles()!
void LLVOVolume::regenFaces()
{
	// remove existing faces
	BOOL count_changed = mNumFaces != getNumTEs();

	if (count_changed)
	{
		deleteFaces();
		// add new faces
		mNumFaces = getNumTEs();
	}

	for (S32 i = 0; i < mNumFaces; ++i)
	{
		LLFace* facep = count_changed ? addFace(i) : mDrawable->getFace(i);
		if (!facep) continue;

		facep->setTEOffset(i);
		facep->setTexture(getTEImage(i));
		facep->setViewerObject(this);

#ifdef MEDIA_ON_PRIM
		// If the face had media on it, this will have broken the link between
		// the LLViewerMediaTexture and the face. Re-establish the link.
		if ((S32)mMediaImplList.size() > i)
		{
			if (mMediaImplList[i])
			{
				LLViewerMediaTexture* media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[i]->getMediaTextureID());
				if (media_tex)
				{
					media_tex->addMediaToFace(facep);
				}
			}
		}
#endif
	}

	if (!count_changed)
	{
		updateFaceFlags();
	}
}

BOOL LLVOVolume::genBBoxes(BOOL force_global)
{
	BOOL res = TRUE;

	LLVector4a min, max;
	min.clear();
	max.clear();

	BOOL rebuild = mDrawable->isState(LLDrawable::REBUILD_VOLUME |
									  LLDrawable::REBUILD_POSITION |
									  LLDrawable::REBUILD_RIGGED);

	LLVolume* volume = mRiggedVolume;
	if (!volume)
	{
		volume = getVolume();
	}

	for (S32 i = 0, count = getVolume()->getNumVolumeFaces(); i < count; ++i)
	{
		LLFace* face = mDrawable->getFace(i);
		if (!face)
		{
			continue;
		}
		res &= face->genVolumeBBoxes(*volume, i, mRelativeXform,
									 mRelativeXformInvTrans, force_global ||
									 (mVolumeImpl && mVolumeImpl->isVolumeGlobal()));

		if (rebuild)
		{
			if (i == 0)
			{
				min = face->mExtents[0];
				max = face->mExtents[1];
			}
			else
			{
				min.setMin(min, face->mExtents[0]);
				max.setMax(max, face->mExtents[1]);
			}
		}
	}

	if (rebuild)
	{
		mDrawable->setSpatialExtents(min, max);
		min.add(max);
		min.mul(0.5f);
		mDrawable->setPositionGroup(min);
	}

	updateRadius();
	mDrawable->movePartition();

	return res;
}

void LLVOVolume::preRebuild()
{
	if (mVolumeImpl != NULL)
	{
		mVolumeImpl->preRebuild();
	}
}

void LLVOVolume::updateRelativeXform()
{
	if (mVolumeImpl)
	{
		mVolumeImpl->updateRelativeXform();
		return;
	}

	LLDrawable* drawable = mDrawable;

	if (drawable->isState(LLDrawable::RIGGED) && mRiggedVolume.notNull())
	{	// Rigged volume (which is in agent space) is used for generating
		// bounding boxes etc. Inverse of render matrix should go to
		// partition space
		mRelativeXform = getRenderMatrix();

		F32* dst = (F32*) mRelativeXformInvTrans.mMatrix;
		F32* src = (F32*) mRelativeXform.mMatrix;
		dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
		dst[3] = src[4]; dst[4] = src[5]; dst[5] = src[6];
		dst[6] = src[8]; dst[7] = src[9]; dst[8] = src[10];

		mRelativeXform.invert();
		mRelativeXformInvTrans.transpose();
	}
	else if (drawable->isActive())
	{
		// setup relative transforms
		LLQuaternion delta_rot;
		LLVector3 delta_pos, delta_scale;

		// matrix from local space to parent relative/global space
		delta_rot = drawable->isSpatialRoot() ? LLQuaternion() : mDrawable->getRotation();
		delta_pos = drawable->isSpatialRoot() ? LLVector3(0,0,0) : mDrawable->getPosition();
		delta_scale = mDrawable->getScale();

		// Vertex transform (4x4)
		LLVector3 x_axis = LLVector3(delta_scale.mV[VX], 0.f, 0.f) * delta_rot;
		LLVector3 y_axis = LLVector3(0.f, delta_scale.mV[VY], 0.f) * delta_rot;
		LLVector3 z_axis = LLVector3(0.f, 0.f, delta_scale.mV[VZ]) * delta_rot;

		mRelativeXform.initRows(LLVector4(x_axis, 0.f),
								LLVector4(y_axis, 0.f),
								LLVector4(z_axis, 0.f),
								LLVector4(delta_pos, 1.f));

		// compute inverse transpose for normals
		// mRelativeXformInvTrans.setRows(x_axis, y_axis, z_axis);
		// mRelativeXformInvTrans.invert();
		// mRelativeXformInvTrans.setRows(x_axis, y_axis, z_axis);
		// grumble - invert is NOT a matrix invert, so we do it by hand:

		LLMatrix3 rot_inverse = LLMatrix3(~delta_rot);

		LLMatrix3 scale_inverse;
		scale_inverse.setRows(LLVector3(1.0, 0.0, 0.0) / delta_scale.mV[VX],
							  LLVector3(0.0, 1.0, 0.0) / delta_scale.mV[VY],
							  LLVector3(0.0, 0.0, 1.0) / delta_scale.mV[VZ]);

		mRelativeXformInvTrans = rot_inverse * scale_inverse;

		mRelativeXformInvTrans.transpose();
	}
	else
	{
		LLVector3 pos = getPosition();
		LLVector3 scale = getScale();
		LLQuaternion rot = getRotation();

		if (mParent)
		{
			pos *= mParent->getRotation();
			pos += mParent->getPosition();
			rot *= mParent->getRotation();
		}

		//LLViewerRegion* region = getRegion();
		//pos += region->getOriginAgent();

		LLVector3 x_axis = LLVector3(scale.mV[VX], 0.f, 0.f) * rot;
		LLVector3 y_axis = LLVector3(0.f, scale.mV[VY], 0.f) * rot;
		LLVector3 z_axis = LLVector3(0.f, 0.f, scale.mV[VZ]) * rot;

		mRelativeXform.initRows(LLVector4(x_axis, 0.f),
								LLVector4(y_axis, 0.f),
								LLVector4(z_axis, 0.f),
								LLVector4(pos, 1.f));

		// compute inverse transpose for normals
		LLMatrix3 rot_inverse = LLMatrix3(~rot);

		LLMatrix3 scale_inverse;
		scale_inverse.setRows(LLVector3(1.0, 0.0, 0.0) / scale.mV[VX],
							  LLVector3(0.0, 1.0, 0.0) / scale.mV[VY],
							  LLVector3(0.0, 0.0, 1.0) / scale.mV[VZ]);

		mRelativeXformInvTrans = rot_inverse * scale_inverse;

		mRelativeXformInvTrans.transpose();
	}
}

BOOL LLVOVolume::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer t(LLFastTimer::FTM_UPDATE_PRIMITIVES);

	if (mDrawable->isState(LLDrawable::REBUILD_RIGGED))
	{
		{
			LLFastTimer t(LLFastTimer::FTM_UPDATE_RIGGED_VOLUME);
			updateRiggedVolume();
		}
		genBBoxes(FALSE);
		mDrawable->clearState(LLDrawable::REBUILD_RIGGED);
	}

	if (mVolumeImpl != NULL)
	{
		BOOL res;
		{
			LLFastTimer t(LLFastTimer::FTM_GEN_FLEX);
			res = mVolumeImpl->doUpdateGeometry(drawable);
		}
		updateFaceFlags();
		return res;
	}

	dirtySpatialGroup(drawable->isState(LLDrawable::IN_REBUILD_Q1));

	BOOL compiled = FALSE;

	updateRelativeXform();

	if (mDrawable.isNull()) // Not sure why this is happening, but it is...
	{
		return TRUE; // No update to complete
	}

	if (mVolumeChanged || mFaceMappingChanged)
	{
		compiled = TRUE;

		if (mVolumeChanged)
		{
			LLFastTimer ftm(LLFastTimer::FTM_GEN_VOLUME);
			LLVolumeParams volume_params = getVolume()->getParams();
			setVolume(volume_params, 0);
			drawable->setState(LLDrawable::REBUILD_VOLUME);
		}

		{
			LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
			regenFaces();
			genBBoxes(FALSE);
		}
	}
	else if (mLODChanged || mSculptChanged)
	{
		LLVolume *old_volumep, *new_volumep;
		F32 old_lod, new_lod;
		S32 old_num_faces, new_num_faces;

		old_volumep = getVolume();
		old_lod = old_volumep->getDetail();
		old_num_faces = old_volumep->getNumFaces();
		old_volumep = NULL;

		{
			LLFastTimer ftm(LLFastTimer::FTM_GEN_VOLUME);
			LLVolumeParams volume_params = getVolume()->getParams();
			setVolume(volume_params, 0);
		}

		new_volumep = getVolume();
		new_lod = new_volumep->getDetail();
		new_num_faces = new_volumep->getNumFaces();
		new_volumep = NULL;

		if (new_lod != old_lod || new_num_faces != old_num_faces || mSculptChanged)
		{
			compiled = TRUE;
			sNumLODChanges += new_num_faces;

			if ((S32)getNumTEs() != getVolume()->getNumFaces())
			{
				// mesh loading may change number of faces.
				setNumTEs(getVolume()->getNumFaces());
			}

			// for face->genVolumeTriangles()
			drawable->setState(LLDrawable::REBUILD_VOLUME);

			{
				LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
				if (new_num_faces != old_num_faces ||
					mNumFaces != (S32)getNumTEs())
				{
					regenFaces();
				}
				genBBoxes(FALSE);

				if (mSculptChanged)
				{	// changes in sculpt maps can thrash an object bounding box
					// without triggering a spatial group bounding box update
					// -- force spatial group to update bounding boxes
					LLSpatialGroup* group = mDrawable->getSpatialGroup();
					if (group)
					{
						group->unbound();
					}
				}
			}
		}
	}
	// it has its own drawable (it's moved) or it has changed UVs or it has
	// changed xforms from global<->local
	else
	{
		compiled = TRUE;
		// All it did was move or we changed the texture coordinate offset
		LLFastTimer t(LLFastTimer::FTM_GEN_TRIANGLES);
		genBBoxes(FALSE);
	}

	// Update face flags
	updateFaceFlags();

	if (compiled)
	{
		++LLPipeline::sCompiles;
	}

	mVolumeChanged = FALSE;
	mLODChanged = FALSE;
	mSculptChanged = FALSE;
	mFaceMappingChanged = FALSE;

	return LLViewerObject::updateGeometry(drawable);
}

void LLVOVolume::updateFaceSize(S32 idx)
{
	LLFace* facep = mDrawable->getFace(idx);
	if (!facep) return;

	if (idx >= getVolume()->getNumVolumeFaces())
	{
		facep->setSize(0,0, true);
	}
	else
	{
		const LLVolumeFace& vol_face = getVolume()->getVolumeFace(idx);
		facep->setSize(vol_face.mNumVertices, vol_face.mNumIndices,
					   true); // <--- volume faces should be padded for 16-byte alignment
	}
}

BOOL LLVOVolume::isRootEdit() const
{
	if (mParent && !((LLViewerObject*)mParent)->isAvatar())
	{
		return FALSE;
	}
	return TRUE;
}

#ifdef MEDIA_ON_PRIM
//virtual
void LLVOVolume::setNumTEs(const U8 num_tes)
{
	const U8 old_num_tes = getNumTEs();

	if (old_num_tes && old_num_tes < num_tes) //new faces added
	{
		LLViewerObject::setNumTEs(num_tes);

		if (mMediaImplList.size() >= old_num_tes && mMediaImplList[old_num_tes - 1].notNull()) //duplicate the last media textures if exists.
		{
			mMediaImplList.resize(num_tes);
			const LLTextureEntry* te = getTE(old_num_tes - 1);
			for (U8 i = old_num_tes; i < num_tes; ++i)
			{
				setTE(i, *te);
				mMediaImplList[i] = mMediaImplList[old_num_tes - 1];
			}
			mMediaImplList[old_num_tes -1]->setUpdated(TRUE);
		}
	}
	else if (old_num_tes > num_tes && mMediaImplList.size() > num_tes) //old faces removed
	{
		U8 end = mMediaImplList.size();
		for (U8 i = num_tes; i < end; ++i)
		{
			removeMediaImpl(i);
		}
		mMediaImplList.resize(num_tes);

		LLViewerObject::setNumTEs(num_tes);
	}
	else
	{
		LLViewerObject::setNumTEs(num_tes);
	}

	return;
}
#endif

void LLVOVolume::setTEImage(const U8 te, LLViewerTexture *imagep)
{
	BOOL changed = (mTEImages[te] != imagep);
	LLViewerObject::setTEImage(te, imagep);
	if (changed)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
}

S32 LLVOVolume::setTETexture(const U8 te, const LLUUID &uuid)
{
	S32 res = LLViewerObject::setTETexture(te, uuid);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

S32 LLVOVolume::setTEColor(const U8 te, const LLColor3& color)
{
	return setTEColor(te, LLColor4(color));
}

S32 LLVOVolume::setTEColor(const U8 te, const LLColor4& color)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		LL_WARNS("RenderVolume") << "No texture entry for te " << (S32)te
								 << ", object " << mID << LL_ENDL;
	}
	else if (color != tep->getColor())
	{
		if (color.mV[3] != tep->getColor().mV[3])
		{
			gPipeline.markTextured(mDrawable);
		}
		retval = LLPrimitive::setTEColor(te, color);
		if (mDrawable.notNull() && retval)
		{
			// These should only happen on updates which are not the initial update.
			mDrawable->setState(LLDrawable::REBUILD_COLOR);
			dirtyMesh();
		}
	}

	return  retval;
}

S32 LLVOVolume::setTEBumpmap(const U8 te, const U8 bumpmap)
{
	S32 res = LLViewerObject::setTEBumpmap(te, bumpmap);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTETexGen(const U8 te, const U8 texgen)
{
	S32 res = LLViewerObject::setTETexGen(te, texgen);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEMediaTexGen(const U8 te, const U8 media)
{
	S32 res = LLViewerObject::setTEMediaTexGen(te, media);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEShiny(const U8 te, const U8 shiny)
{
	S32 res = LLViewerObject::setTEShiny(te, shiny);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEFullbright(const U8 te, const U8 fullbright)
{
	S32 res = LLViewerObject::setTEFullbright(te, fullbright);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEBumpShinyFullbright(const U8 te, const U8 bump)
{
	S32 res = LLViewerObject::setTEBumpShinyFullbright(te, bump);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

S32 LLVOVolume::setTEMediaFlags(const U8 te, const U8 media_flags)
{
	S32 res = LLViewerObject::setTEMediaFlags(te, media_flags);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEGlow(const U8 te, const F32 glow)
{
	S32 res = LLViewerObject::setTEGlow(te, glow);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return  res;
}

S32 LLVOVolume::setTEScale(const U8 te, const F32 s, const F32 t)
{
	S32 res = LLViewerObject::setTEScale(te, s, t);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

S32 LLVOVolume::setTEScaleS(const U8 te, const F32 s)
{
	S32 res = LLViewerObject::setTEScaleS(te, s);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

S32 LLVOVolume::setTEScaleT(const U8 te, const F32 t)
{
	S32 res = LLViewerObject::setTEScaleT(te, t);
	if (res)
	{
		gPipeline.markTextured(mDrawable);
		mFaceMappingChanged = TRUE;
	}
	return res;
}

void LLVOVolume::updateTEData()
{
	/*if (mDrawable.notNull())
	{
		mFaceMappingChanged = TRUE;
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_MATERIAL, TRUE);
	}*/
}

#ifdef MEDIA_ON_PRIM
bool LLVOVolume::hasMedia() const
{
	bool result = false;
	const U8 numTEs = getNumTEs();
	for (U8 i = 0; i < numTEs; ++i)
	{
		const LLTextureEntry* te = getTE(i);
		if (te->hasMedia())
		{
			result = true;
			break;
		}
	}
	return result;
}

LLVector3 LLVOVolume::getApproximateFaceNormal(U8 face_id)
{
	LLVolume* volume = getVolume();
	LLVector4a result;
	result.clear();

	LLVector3 ret;

	if (volume && face_id < volume->getNumVolumeFaces())
	{
		const LLVolumeFace& face = volume->getVolumeFace(face_id);
		for (S32 i = 0; i < (S32)face.mNumVertices; ++i)
		{
			result.add(face.mNormals[i]);
		}

		LLVector3 ret(result.getF32ptr());
		ret = volumeDirectionToAgent(ret);
		ret.normVec();
	}

	return ret;
}

void LLVOVolume::requestMediaDataUpdate(bool isNew)
{
    if (sObjectMediaClient)
		sObjectMediaClient->fetchMedia(new LLMediaDataClientObjectImpl(this, isNew));
}

bool LLVOVolume::isMediaDataBeingFetched() const
{
	// I know what I'm doing by const_casting this away: this is just
	// a wrapper class that is only going to do a lookup.
	return (sObjectMediaClient) ? sObjectMediaClient->isInQueue(new LLMediaDataClientObjectImpl(const_cast<LLVOVolume*>(this), false)) : false;
}

void LLVOVolume::cleanUpMediaImpls()
{
	// Iterate through our TEs and remove any Impls that are no longer used
	const U8 numTEs = getNumTEs();
	for (U8 i = 0; i < numTEs; ++i)
	{
		const LLTextureEntry* te = getTE(i);
		if (! te->hasMedia())
		{
			// Delete the media IMPL!
			removeMediaImpl(i);
		}
	}
}

void LLVOVolume::updateObjectMediaData(const LLSD &media_data_array, const std::string &media_version)
{
	// media_data_array is an array of media entry maps
	// media_version is the version string in the response.
	U32 fetched_version = LLTextureEntry::getVersionFromMediaVersionString(media_version);

	// Only update it if it is newer!
	if ((S32)fetched_version > mLastFetchedMediaVersion)
	{
		mLastFetchedMediaVersion = fetched_version;
		//llinfos << "updating:" << this->getID() << " " << ll_pretty_print_sd(media_data_array) << llendl;

		LLSD::array_const_iterator iter = media_data_array.beginArray();
		LLSD::array_const_iterator end = media_data_array.endArray();
		U8 texture_index = 0;
		for ( ; iter != end; ++iter, ++texture_index)
		{
			syncMediaData(texture_index, *iter, false/*merge*/, false/*ignore_agent*/);
		}
	}
}

void LLVOVolume::syncMediaData(S32 texture_index, const LLSD &media_data, bool merge, bool ignore_agent)
{
	if (mDead)
	{
		// If the object has been marked dead, don't process media updates.
		return;
	}

	LLTextureEntry *te = getTE(texture_index);
	if (!te)
	{
		return;
	}

	LL_DEBUGS("MediaOnAPrim") << "BEFORE: texture_index = " << texture_index
		<< " hasMedia = " << te->hasMedia() << " : "
		<< ((NULL == te->getMediaData()) ? "NULL MEDIA DATA" : ll_pretty_print_sd(te->getMediaData()->asLLSD())) << llendl;

	std::string previous_url;
	LLMediaEntry* mep = te->getMediaData();
	if (mep)
	{
		// Save the "current url" from before the update so we can tell if
		// it changes.
		previous_url = mep->getCurrentURL();
	}

	if (merge)
	{
		te->mergeIntoMediaData(media_data);
	}
	else {
		// XXX Question: what if the media data is undefined LLSD, but the
		// update we got above said that we have media flags??	Here we clobber
		// that, assuming the data from the service is more up-to-date.
		te->updateMediaData(media_data);
	}

	mep = te->getMediaData();
	if (mep)
	{
		bool update_from_self = false;
		if (!ignore_agent)
		{
			LLUUID updating_agent = LLTextureEntry::getAgentIDFromMediaVersionString(getMediaURL());
			update_from_self = (updating_agent == gAgent.getID());
		}
		viewer_media_t media_impl = LLViewerMedia::updateMediaImpl(mep, previous_url, update_from_self);

		addMediaImpl(media_impl, texture_index);
	}
	else
	{
		removeMediaImpl(texture_index);
	}

	LL_DEBUGS("MediaOnAPrim") << "AFTER: texture_index = " << texture_index
		<< " hasMedia = " << te->hasMedia() << " : "
		<< ((NULL == te->getMediaData()) ? "NULL MEDIA DATA" : ll_pretty_print_sd(te->getMediaData()->asLLSD())) << llendl;
}

void LLVOVolume::mediaNavigateBounceBack(U8 texture_index)
{
	// Find the media entry for this navigate
	const LLMediaEntry* mep = NULL;
	viewer_media_t impl = getMediaImpl(texture_index);
	LLTextureEntry *te = getTE(texture_index);
	if (te)
	{
		mep = te->getMediaData();
	}

	if (mep && impl)
	{
        std::string url = mep->getCurrentURL();
		// Look for a ":", if not there, assume "http://"
		if (!url.empty() && std::string::npos == url.find(':'))
		{
			url = "http://" + url;
		}
		// If the url we're trying to "bounce back" to is either empty or not
		// allowed by the whitelist, try the home url.  If *that* doesn't work,
		// set the media as failed and unload it
        if (url.empty() || !mep->checkCandidateUrl(url))
        {
            url = mep->getHomeURL();
			// Look for a ":", if not there, assume "http://"
			if (!url.empty() && std::string::npos == url.find(':'))
			{
				url = "http://" + url;
			}
        }
        if (url.empty() || !mep->checkCandidateUrl(url))
		{
			// The url to navigate back to is not good, and we have nowhere else
			// to go.
			LL_WARNS("MediaOnAPrim") << "FAILED to bounce back URL \"" << url << "\" -- unloading impl" << LL_ENDL;
			impl->setMediaFailed(true);
		}
		else {
			// Okay, navigate now
            LL_INFOS("MediaOnAPrim") << "bouncing back to URL: " << url << LL_ENDL;
            impl->navigateTo(url, "", false, true);
        }
    }
}

bool LLVOVolume::hasMediaPermission(const LLMediaEntry* media_entry, MediaPermType perm_type)
{
    // NOTE: This logic ALMOST duplicates the logic in the server (in particular, in llmediaservice.cpp).
    if (NULL == media_entry) return false; // XXX should we assert here?

    // The agent has permissions if:
    // - world permissions are on, or
    // - group permissions are on, and agent_id is in the group, or
    // - agent permissions are on, and agent_id is the owner

	// *NOTE: We *used* to check for modify permissions here (i.e. permissions were
	// granted if permModify() was true).  However, this doesn't make sense in the
	// viewer: we don't want to show controls or allow interaction if the author
	// has deemed it so.  See DEV-42115.

    U8 media_perms = (perm_type == MEDIA_PERM_INTERACT) ? media_entry->getPermsInteract() : media_entry->getPermsControl();

    // World permissions
    if (0 != (media_perms & LLMediaEntry::PERM_ANYONE))
    {
        return true;
    }

    // Group permissions
    else if (0 != (media_perms & LLMediaEntry::PERM_GROUP))
    {
		LLPermissions* obj_perm = LLSelectMgr::getInstance()->findObjectPermissions(this);
		if (obj_perm && gAgent.isInGroup(obj_perm->getGroup()))
		{
			return true;
		}
    }

    // Owner permissions
    else if (0 != (media_perms & LLMediaEntry::PERM_OWNER) && permYouOwner())
    {
        return true;
    }

    return false;

}

void LLVOVolume::mediaNavigated(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, std::string new_location)
{
	bool block_navigation = false;
	// FIXME: if/when we allow the same media impl to be used by multiple faces, the logic here will need to be fixed
	// to deal with multiple face indices.
	int face_index = getFaceIndexWithMediaImpl(impl, -1);

	// Find the media entry for this navigate
	LLMediaEntry* mep = NULL;
	LLTextureEntry *te = getTE(face_index);
	if (te)
	{
		mep = te->getMediaData();
	}

	if (mep)
	{
		if (!mep->checkCandidateUrl(new_location))
		{
			block_navigation = true;
		}
		if (!block_navigation && !hasMediaPermission(mep, MEDIA_PERM_INTERACT))
		{
			block_navigation = true;
		}
	}
	else
	{
		LL_WARNS("MediaOnAPrim") << "Couldn't find media entry!" << LL_ENDL;
	}

	if (block_navigation)
	{
		LL_INFOS("MediaOnAPrim") << "blocking navigate to URI " << new_location << LL_ENDL;

		// "bounce back" to the current URL from the media entry
		mediaNavigateBounceBack(face_index);
	}
	else if (sObjectMediaNavigateClient)
	{

		LL_DEBUGS("MediaOnAPrim") << "broadcasting navigate with URI " << new_location << LL_ENDL;

		sObjectMediaNavigateClient->navigate(new LLMediaDataClientObjectImpl(this, false), face_index, new_location);
	}
}

void LLVOVolume::mediaEvent(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, LLViewerMediaObserver::EMediaEvent event)
{
	switch(event)
	{

		case LLViewerMediaObserver::MEDIA_EVENT_LOCATION_CHANGED:
		{
			switch(impl->getNavState())
			{
				case LLViewerMediaImpl::MEDIANAVSTATE_FIRST_LOCATION_CHANGED:
				{
					// This is the first location changed event after the start of a non-server-directed nav.  It may need to be broadcast or bounced back.
					mediaNavigated(impl, plugin, plugin->getLocation());
				}
				break;

				case LLViewerMediaImpl::MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS:
					// This navigate didn't change the current URL.
					LL_DEBUGS("MediaOnAPrim") << "	NOT broadcasting navigate (spurious)" << LL_ENDL;
				break;

				case LLViewerMediaImpl::MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED:
					// This is the first location changed event after the start of a server-directed nav.  Don't broadcast it.
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (server-directed)" << LL_ENDL;
				break;

				default:
					// This is a subsequent location-changed due to a redirect.	 Don't broadcast.
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (redirect)" << LL_ENDL;
				break;
			}
		}
		break;

		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			switch(impl->getNavState())
			{
				case LLViewerMediaImpl::MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED:
				{
					// This is the first location changed event after the start of a non-server-directed nav.  It may need to be broadcast or bounced back.
					mediaNavigated(impl, plugin, plugin->getNavigateURI());
				}
				break;

				case LLViewerMediaImpl::MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS:
					// This navigate didn't change the current URL.
					LL_DEBUGS("MediaOnAPrim") << "	NOT broadcasting navigate (spurious)" << LL_ENDL;
				break;

				case LLViewerMediaImpl::MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED:
					// This is the the navigate complete event from a server-directed nav.  Don't broadcast it.
					LL_INFOS("MediaOnAPrim") << "	NOT broadcasting navigate (server-directed)" << LL_ENDL;
				break;

				default:
					// For all other states, the navigate should have been handled by LOCATION_CHANGED events already.
				break;
			}
		}
		break;

		default:
		break;
	}

}

void LLVOVolume::sendMediaDataUpdate()
{
    if (sObjectMediaClient)
		sObjectMediaClient->updateMedia(new LLMediaDataClientObjectImpl(this, false));
}

void LLVOVolume::removeMediaImpl(S32 texture_index)
{
	if (mMediaImplList.size() <= (U32)texture_index || mMediaImplList[texture_index].isNull())
	{
		return;
	}

	// Make the face referencing to mMediaImplList[texture_index] to point back
	// to the old texture.
	if (mDrawable && texture_index < mDrawable->getNumFaces())
	{
		LLFace* facep = mDrawable->getFace(texture_index);
		if (facep)
		{
			LLViewerMediaTexture* media_tex;
			media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[texture_index]->getMediaTextureID());
			if (media_tex)
			{
				media_tex->removeMediaFromFace(facep);
			}
		}
	}

	// Check if some other face(s) of this object reference(s)to this media
	// impl.
	S32 i;
	S32 end = (S32)mMediaImplList.size();
	for (i = 0; i < end; ++i)
	{
		if (i != texture_index && mMediaImplList[i] == mMediaImplList[texture_index])
		{
			break;
		}
	}

	if (i == end) //this object does not need this media impl.
	{
		mMediaImplList[texture_index]->removeObject(this);
	}

	mMediaImplList[texture_index] = NULL;
	return;
}

void LLVOVolume::addMediaImpl(LLViewerMediaImpl* media_impl, S32 texture_index)
{
	if ((S32)mMediaImplList.size() < texture_index + 1)
	{
		mMediaImplList.resize(texture_index + 1);
	}

	if (mMediaImplList[texture_index].notNull())
	{
		if (mMediaImplList[texture_index] == media_impl)
		{
			return;
		}

		removeMediaImpl(texture_index);
	}

	mMediaImplList[texture_index] = media_impl;
	media_impl->addObject(this);

	//add the face to show the media if it is in playing
	if (mDrawable)
	{
		LLFace* facep = mDrawable->getFace(texture_index);
		if (facep)
		{
			LLViewerMediaTexture* media_tex = LLViewerTextureManager::findMediaTexture(mMediaImplList[texture_index]->getMediaTextureID());
			if (media_tex)
			{
				media_tex->addMediaToFace(facep);
			}
		}
		else //the face is not available now, start media on this face later.
		{
			media_impl->setUpdated(TRUE);
		}
	}
	return;
}

viewer_media_t LLVOVolume::getMediaImpl(U8 face_id) const
{
	if (mMediaImplList.size() > face_id)
	{
		return mMediaImplList[face_id];
	}
	return NULL;
}

F64 LLVOVolume::getTotalMediaInterest() const
{
	// If this object is currently focused, this object has "high" interest
	if (LLViewerMediaFocus::getInstance()->getFocusedObjectID() == getID())
		return F64_MAX;

	F64 interest = (F64)-1.0;  // means not interested;

	// If this object is selected, this object has "high" interest, but since
	// there can be more than one, we still add in calculated impl interest
	// XXX Sadly, 'contains()' doesn't take a const :(
	if (LLSelectMgr::getInstance()->getSelection()->contains(const_cast<LLVOVolume*>(this)))
		interest = F64_MAX / 2.0;

	int i = 0;
	const int end = getNumTEs();
	for ( ; i < end; ++i)
	{
		const viewer_media_t &impl = getMediaImpl(i);
		if (!impl.isNull())
		{
			if (interest == (F64)-1.0) interest = (F64)0.0;
			interest += impl->getInterest();
		}
	}
	return interest;
}

S32 LLVOVolume::getFaceIndexWithMediaImpl(const LLViewerMediaImpl* media_impl, S32 start_face_id)
{
	S32 end = (S32)mMediaImplList.size();
	for (S32 face_id = start_face_id + 1; face_id < end; ++face_id)
	{
		if (mMediaImplList[face_id] == media_impl)
		{
			return face_id;
		}
	}
	return -1;
}
#endif

//----------------------------------------------------------------------------

void LLVOVolume::setLightTextureID(LLUUID id)
{
	if (id.notNull())
	{
		if (!hasLightTexture())
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE, TRUE, true);
		}
		LLLightImageParams* param_block = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		if (param_block && param_block->getLightTexture() != id)
		{
			param_block->setLightTexture(id);
			parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
		}
	}
	else
	{
		if (hasLightTexture())
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE, FALSE, true);
			mLightTexture = NULL;
		}
	}
}

void LLVOVolume::setSpotLightParams(LLVector3 params)
{
	LLLightImageParams* param_block = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
	if (param_block && param_block->getParams() != params)
	{
		param_block->setParams(params);
		parameterChanged(LLNetworkData::PARAMS_LIGHT_IMAGE, true);
	}
}

void LLVOVolume::setIsLight(BOOL is_light)
{
	if (is_light != getIsLight())
	{
		if (is_light)
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, TRUE, true);
		}
		else
		{
			setParameterEntryInUse(LLNetworkData::PARAMS_LIGHT, FALSE, true);
		}

		if (is_light)
		{
			// Add it to the pipeline mLightSet
			gPipeline.setLight(mDrawable, TRUE);
		}
		else
		{
			// Not a light.  Remove it from the pipeline's light set.
			gPipeline.setLight(mDrawable, FALSE);
		}
	}
}

void LLVOVolume::setLightColor(const LLColor3& color)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getColor() != color)
		{
			param_block->setColor(LLColor4(color, param_block->getColor().mV[3]));
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
			gPipeline.markTextured(mDrawable);
			mFaceMappingChanged = TRUE;
		}
	}
}

void LLVOVolume::setLightIntensity(F32 intensity)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getColor().mV[3] != intensity)
		{
			param_block->setColor(LLColor4(LLColor3(param_block->getColor()), intensity));
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}

void LLVOVolume::setLightRadius(F32 radius)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getRadius() != radius)
		{
			param_block->setRadius(radius);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}

void LLVOVolume::setLightFalloff(F32 falloff)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getFalloff() != falloff)
		{
			param_block->setFalloff(falloff);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}

void LLVOVolume::setLightCutoff(F32 cutoff)
{
	LLLightParams *param_block = (LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		if (param_block->getCutoff() != cutoff)
		{
			param_block->setCutoff(cutoff);
			parameterChanged(LLNetworkData::PARAMS_LIGHT, true);
		}
	}
}

//----------------------------------------------------------------------------

BOOL LLVOVolume::getIsLight() const
{
	return getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT);
}

LLColor3 LLVOVolume::getLightBaseColor() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return LLColor3(param_block->getColor());
	}
	else
	{
		return LLColor3(1,1,1);
	}
}

LLColor3 LLVOVolume::getLightColor() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return LLColor3(param_block->getColor()) * param_block->getColor().mV[3];
	}
	else
	{
		return LLColor3(1,1,1);
	}
}

LLUUID LLVOVolume::getLightTextureID() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE))
	{
		const LLLightImageParams *param_block = (const LLLightImageParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		if (param_block)
		{
			return param_block->getLightTexture();
		}
	}

	return LLUUID::null;
}

LLVector3 LLVOVolume::getSpotLightParams() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE))
	{
		const LLLightImageParams *param_block = (const LLLightImageParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
		if (param_block)
		{
			return param_block->getParams();
		}
	}

	return LLVector3();
}

F32 LLVOVolume::getSpotLightPriority() const
{
	return mSpotLightPriority;
}

void LLVOVolume::updateSpotLightPriority()
{
	LLVector3 pos = mDrawable->getPositionAgent();
	LLVector3 at(0,0,-1);
	at *= getRenderRotation();

	F32 r = getLightRadius()*0.5f;

	pos += at * r;

	at = LLViewerCamera::getInstance()->getAtAxis();

	pos -= at * r;

	mSpotLightPriority = gPipeline.calcPixelArea(pos, LLVector3(r,r,r), *LLViewerCamera::getInstance());

	if (mLightTexture.notNull())
	{
		mLightTexture->addTextureStats(mSpotLightPriority);
		mLightTexture->setBoostLevel(LLViewerTexture::BOOST_CLOUDS);
	}
}

bool LLVOVolume::isLightSpotlight() const
{
	LLLightImageParams* params = (LLLightImageParams*) getParameterEntry(LLNetworkData::PARAMS_LIGHT_IMAGE);
	if (params)
	{
		return params->isLightSpotlight();
	}
	return false;
}

LLViewerTexture* LLVOVolume::getLightTexture()
{
	LLUUID id = getLightTextureID();

	if (id.notNull())
	{
		if (mLightTexture.isNull() || id != mLightTexture->getID())
		{
			mLightTexture = LLViewerTextureManager::getFetchedTexture(id);
		}
	}
	else
	{
		mLightTexture = NULL;
	}

	return mLightTexture;
}

F32 LLVOVolume::getLightIntensity() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getColor().mV[3];
	}
	else
	{
		return 1.f;
	}
}

F32 LLVOVolume::getLightRadius() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getRadius();
	}
	else
	{
		return 0.f;
	}
}

F32 LLVOVolume::getLightFalloff() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getFalloff();
	}
	else
	{
		return 0.f;
	}
}

F32 LLVOVolume::getLightCutoff() const
{
	const LLLightParams *param_block = (const LLLightParams *)getParameterEntry(LLNetworkData::PARAMS_LIGHT);
	if (param_block)
	{
		return param_block->getCutoff();
	}
	else
	{
		return 0.f;
	}
}

U32 LLVOVolume::getVolumeInterfaceID() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getID();
	}

	return 0;
}

BOOL LLVOVolume::isFlexible() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE))
	{
		LLVolume* volume = getVolume();
		if (volume && volume->getParams().getPathParams().getCurveType() != LL_PCODE_PATH_FLEXIBLE)
		{
			LLVolumeParams volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_FLEXIBLE);
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL LLVOVolume::isSculpted() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL LLVOVolume::isMesh() const
{
	if (isSculpted())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		U8 sculpt_type = sculpt_params->getSculptType();

		if ((sculpt_type & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH)
			// mesh is a mesh
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLVOVolume::hasLightTexture() const
{
	if (getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT_IMAGE))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL LLVOVolume::isVolumeGlobal() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->isVolumeGlobal() ? TRUE : FALSE;
	}
	else if (mRiggedVolume.notNull())
	{
		return TRUE;
	}

	return FALSE;
}

BOOL LLVOVolume::canBeFlexible() const
{
	U8 path = getVolume()->getParams().getPathParams().getCurveType();
	return (path == LL_PCODE_PATH_FLEXIBLE || path == LL_PCODE_PATH_LINE);
}

BOOL LLVOVolume::setIsFlexible(BOOL is_flexible)
{
	BOOL res = FALSE;
	BOOL was_flexible = isFlexible();
	LLVolumeParams volume_params;
	if (is_flexible)
	{
		if (!was_flexible)
		{
			volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_FLEXIBLE);
			res = TRUE;
			setFlags(FLAGS_USE_PHYSICS, FALSE);
			setFlags(FLAGS_PHANTOM, TRUE);
			setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, TRUE, true);
			if (mDrawable)
			{
				mDrawable->makeActive();
			}
		}
	}
	else
	{
		if (was_flexible)
		{
			volume_params = getVolume()->getParams();
			U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
			volume_params.setType(profile_and_hole, LL_PCODE_PATH_LINE);
			res = TRUE;
			setFlags(FLAGS_PHANTOM, FALSE);
			setParameterEntryInUse(LLNetworkData::PARAMS_FLEXIBLE, FALSE, true);
		}
	}
	if (res)
	{
		res = setVolume(volume_params, 1);
		if (res)
		{
			markForUpdate(TRUE);
		}
	}
	return res;
}

//----------------------------------------------------------------------------

void LLVOVolume::generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point)
{
	LLVolume *volume = getVolume();

	if (volume)
	{
		LLVector3 view_vector;
		view_vector = view_point;

		//transform view vector into volume space
		view_vector -= getRenderPosition();
		mDrawable->mDistanceWRTCamera = view_vector.length();
		LLQuaternion worldRot = getRenderRotation();
		view_vector = view_vector * ~worldRot;
		if (!isVolumeGlobal())
		{
			LLVector3 objScale = getScale();
			LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
			view_vector.scaleVec(invObjScale);
		}

		updateRelativeXform();
		LLMatrix4 trans_mat = mRelativeXform;
		if (mDrawable->isStatic())
		{
			trans_mat.translate(getRegion()->getOriginAgent());
		}

		volume->generateSilhouetteVertices(nodep->mSilhouetteVertices, nodep->mSilhouetteNormals, view_vector, trans_mat, mRelativeXformInvTrans, nodep->getTESelectMask());

		nodep->mSilhouetteExists = TRUE;
	}
}

void LLVOVolume::deleteFaces()
{
	S32 face_count = mNumFaces;
	if (mDrawable.notNull())
	{
		mDrawable->deleteFaces(0, face_count);
	}

	mNumFaces = 0;
}

void LLVOVolume::updateRadius()
{
	if (mDrawable.isNull())
	{
		return;
	}

	mVObjRadius = getScale().length();
	mDrawable->setRadius(mVObjRadius);
}


BOOL LLVOVolume::isAttachment() const
{
	return mState != 0;
}

BOOL LLVOVolume::isHUDAttachment() const
{
	// *NOTE: we assume hud attachment points are in defined range
	// since this range is constant for backwards compatibility
	// reasons this is probably a reasonable assumption to make
	S32 attachment_id = ATTACHMENT_ID_FROM_STATE(mState);
	return (attachment_id >= 31 && attachment_id <= 38);
}

const LLMatrix4 LLVOVolume::getRenderMatrix() const
{
	if (mDrawable->isActive() && !mDrawable->isRoot())
	{
		return mDrawable->getParent()->getWorldMatrix();
	}
	return mDrawable->getWorldMatrix();
}

// Returns a base cost and adds textures to passed in set.
// total cost is returned value + 5 * size of the resulting set.
// Cannot include cost of textures, as they may be re-used in linked
// children, and cost should only be increased for unique textures  -Nyx
U32 LLVOVolume::getRenderCost(texture_cost_t& textures) const
{
	// Get access to params we'll need at various points.
	// Skip if this is object doesn't have a volume (e.g. is an avatar).
	BOOL has_volume = (getVolume() != NULL);
	LLVolumeParams volume_params;
	LLPathParams path_params;
	LLProfileParams profile_params;

	U32 num_triangles = 0;

	// per-prim costs
	static const U32 ARC_PARTICLE_COST = 1; // determined experimentally
	static const U32 ARC_PARTICLE_MAX = 2048; // default values
	static const U32 ARC_TEXTURE_COST = 16; // multiplier for texture resolution - performance tested
	static const U32 ARC_LIGHT_COST = 500; // static cost for light-producing prims
	static const U32 ARC_MEDIA_FACE_COST = 1500; // static cost per media-enabled face

	// per-prim multipliers
	static const F32 ARC_GLOW_MULT = 1.5f; // tested based on performance
	static const F32 ARC_BUMP_MULT = 1.25f; // tested based on performance
	static const F32 ARC_FLEXI_MULT = 5; // tested based on performance
	static const F32 ARC_SHINY_MULT = 1.6f; // tested based on performance
	static const F32 ARC_INVISI_COST = 1.2f; // tested based on performance
	static const F32 ARC_WEIGHTED_MESH = 1.2f; // tested based on performance

	static const F32 ARC_PLANAR_COST = 1.0f; // tested based on performance to have negligible impact
	static const F32 ARC_ANIM_TEX_COST = 4.f; // tested based on performance
	static const F32 ARC_ALPHA_COST = 4.f; // 4x max - based on performance

	F32 shame = 0;

	U32 invisi = 0;
	U32 shiny = 0;
	U32 glow = 0;
	U32 alpha = 0;
	U32 flexi = 0;
	U32 animtex = 0;
	U32 particles = 0;
	U32 bump = 0;
	U32 planar = 0;
	U32 weighted_mesh = 0;
	U32 produces_light = 0;
	U32 media_faces = 0;

	const LLDrawable* drawablep = mDrawable;
	U32 num_faces = drawablep->getNumFaces();

	if (has_volume)
	{
		volume_params = getVolume()->getParams();
		path_params = volume_params.getPathParams();
		profile_params = volume_params.getProfileParams();

		F32 weighted_triangles = -1.0;
		getStreamingCost(NULL, NULL, &weighted_triangles);

		if (weighted_triangles > 0.0)
		{
			num_triangles = (U32)(weighted_triangles);
		}
	}

	if (num_triangles == 0)
	{
		num_triangles = 4;
	}

	if (isSculpted())
	{
		if (isMesh())
		{
			// base cost is dependent on mesh complexity
			// note that 3 is the highest LOD as of the time of this coding.
			S32 size = gMeshRepo.getMeshSize(volume_params.getSculptID(),3);
			if ( size > 0)
			{
				if (gMeshRepo.getSkinInfo(volume_params.getSculptID(), this))
				{
					// weighted attachment - 1 point for every 3 bytes
					weighted_mesh = 1;
				}
			}
			else
			{
				// something went wrong - user should know their content isn't render-free
				return 0;
			}
		}
		else
		{
			const LLSculptParams* sculpt_params = (LLSculptParams*)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
			LLUUID sculpt_id = sculpt_params->getSculptTexture();
			if (textures.find(sculpt_id) == textures.end())
			{
				LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(sculpt_id);
				if (texture)
				{
					S32 texture_cost = 256 + (S32)(ARC_TEXTURE_COST * (texture->getFullHeight() / 128.f + texture->getFullWidth() / 128.f));
					textures.insert(texture_cost_t::value_type(sculpt_id, texture_cost));
				}
			}
		}
	}

	if (isFlexible())
	{
		flexi = 1;
	}
	if (isParticleSource())
	{
		particles = 1;
	}

	if (getIsLight())
	{
		produces_light = 1;
	}

	for (S32 i = 0; i < num_faces; ++i)
	{
		const LLFace* face = drawablep->getFace(i);
		if (!face) continue;

		const LLTextureEntry* te = face->getTextureEntry();
		const LLViewerTexture* img = face->getTexture();

		if (img)
		{
			if (textures.find(img->getID()) == textures.end())
			{
				S32 texture_cost = 256 + (S32)(ARC_TEXTURE_COST * (img->getFullHeight() / 128.f + img->getFullWidth() / 128.f));
				textures.insert(texture_cost_t::value_type(img->getID(), texture_cost));
			}
		}

		if (face->getPoolType() == LLDrawPool::POOL_ALPHA)
		{
			alpha = 1;
		}
		else if (img && img->getPrimaryFormat() == GL_ALPHA)
		{
			invisi = 1;
		}
#ifdef MEDIA_ON_PRIM
		if (face->hasMedia())
		{
			++media_faces;
		}
#endif

		if (te)
		{
			if (te->getBumpmap())
			{
				// bump is a multiplier, don't add per-face
				bump = 1;
			}
			if (te->getShiny())
			{
				// shiny is a multiplier, don't add per-face
				shiny = 1;
			}
			if (te->getGlow() > 0.f)
			{
				// glow is a multiplier, don't add per-face
				glow = 1;
			}
			if (face->mTextureMatrix != NULL)
			{
				animtex = 1;
			}
			if (te->getTexGen())
			{
				planar = 1;
			}
		}
	}

	// shame currently has the "base" cost of 1 point per 15 triangles, min 2.
	shame = num_triangles  * 5.f;
	shame = shame < 2.f ? 2.f : shame;

	// multiply by per-face modifiers
	if (planar)
	{
		shame *= planar * ARC_PLANAR_COST;
	}

	if (animtex)
	{
		shame *= animtex * ARC_ANIM_TEX_COST;
	}

	if (alpha)
	{
		shame *= alpha * ARC_ALPHA_COST;
	}

	if (invisi)
	{
		shame *= invisi * ARC_INVISI_COST;
	}

	if (glow)
	{
		shame *= glow * ARC_GLOW_MULT;
	}

	if (bump)
	{
		shame *= bump * ARC_BUMP_MULT;
	}

	if (shiny)
	{
		shame *= shiny * ARC_SHINY_MULT;
	}

	// multiply shame by multipliers
	if (weighted_mesh)
	{
		shame *= weighted_mesh * ARC_WEIGHTED_MESH;
	}

	if (flexi)
	{
		shame *= flexi * ARC_FLEXI_MULT;
	}

	// add additional costs
	if (particles)
	{
		const LLPartSysData *part_sys_data = &(mPartSourcep->mPartSysData);
		const LLPartData *part_data = &(part_sys_data->mPartData);
		U32 num_particles = (U32)(part_sys_data->mBurstPartCount * llceil( part_data->mMaxAge / part_sys_data->mBurstRate));
		num_particles = num_particles > ARC_PARTICLE_MAX ? ARC_PARTICLE_MAX : num_particles;
		F32 part_size = (llmax(part_data->mStartScale[0], part_data->mEndScale[0]) + llmax(part_data->mStartScale[1], part_data->mEndScale[1])) / 2.f;
		shame += num_particles * part_size * ARC_PARTICLE_COST;
	}

	if (produces_light)
	{
		shame += ARC_LIGHT_COST;
	}

	if (media_faces)
	{
		shame += media_faces * ARC_MEDIA_FACE_COST;
	}

	return (U32)shame;
}

F32 LLVOVolume::getStreamingCost(S32* bytes, S32* visible_bytes, F32* unscaled_value) const
{
	F32 radius = getScale().length() * 0.5f;
	if (isMesh())
	{
		LLSD& header = gMeshRepo.getMeshHeader(getVolume()->getParams().getSculptID());

		return LLMeshRepository::getStreamingCost(header, radius, bytes, visible_bytes, mLOD, unscaled_value);
	}
	else
	{
		LLVolume* volume = getVolume();
		S32 counts[4];
		LLVolume::getLoDTriangleCounts(volume->getParams(), counts);

		LLSD header;
		header["lowest_lod"]["size"] = counts[0] * 10;
		header["low_lod"]["size"] = counts[1] * 10;
		header["medium_lod"]["size"] = counts[2] * 10;
		header["high_lod"]["size"] = counts[3] * 10;

		return LLMeshRepository::getStreamingCost(header, radius, NULL, NULL, -1, unscaled_value);
	}
}

U32 LLVOVolume::getTriangleCount(S32* vcount) const
{
	U32 count = 0;
	LLVolume* volume = getVolume();
	if (volume)
	{
		count = volume->getNumTriangles(vcount);
	}

	return count;
}

U32 LLVOVolume::getHighLODTriangleCount()
{
	U32 ret = 0;

	LLVolume* volume = getVolume();

	if (!isSculpted())
	{
		LLVolume* ref = LLPrimitive::getVolumeManager()->refVolume(volume->getParams(), 3);
		ret = ref->getNumTriangles();
		LLPrimitive::getVolumeManager()->unrefVolume(ref);
	}
	else if (isMesh())
	{
		LLVolume* ref = LLPrimitive::getVolumeManager()->refVolume(volume->getParams(), 3);
		if (!ref->isMeshAssetLoaded() || ref->getNumVolumeFaces() == 0)
		{
			gMeshRepo.loadMesh(this, volume->getParams(), LLModel::LOD_HIGH);
		}
		ret = ref->getNumTriangles();
		LLPrimitive::getVolumeManager()->unrefVolume(ref);
	}
	else
	{	//default sculpts have a constant number of triangles
		ret = 31 * 2 * 31;  //31 rows of 31 columns of quads for a 32x32 vertex patch
	}

	return ret;
}

//static
void LLVOVolume::preUpdateGeom()
{
	sNumLODChanges = 0;
}

void LLVOVolume::parameterChanged(U16 param_type, bool local_origin)
{
	LLViewerObject::parameterChanged(param_type, local_origin);
}

void LLVOVolume::parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin)
{
	LLViewerObject::parameterChanged(param_type, data, in_use, local_origin);
	if (mVolumeImpl)
	{
		mVolumeImpl->onParameterChanged(param_type, data, in_use, local_origin);
	}
	if (mDrawable.notNull())
	{
		BOOL is_light = getIsLight();
		if (is_light != mDrawable->isState(LLDrawable::LIGHT))
		{
			gPipeline.setLight(mDrawable, is_light);
		}
	}
}

void LLVOVolume::setSelected(BOOL sel)
{
	LLViewerObject::setSelected(sel);
	if (mDrawable.notNull())
	{
		markForUpdate(TRUE);
	}
}

void LLVOVolume::updateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{
}

F32 LLVOVolume::getBinRadius()
{
	F32 radius;
	F32 scale = 1.f;
	static LLCachedControl<S32> octree_static_object_size_factor(gSavedSettings, "OctreeStaticObjectSizeFactor");
	S32 size_factor = llmax((S32)octree_static_object_size_factor, 1);
	static LLCachedControl<S32> octree_attachment_size_factor(gSavedSettings, "OctreeAttachmentSizeFactor");
	S32 attachment_size_factor = llmax((S32)octree_attachment_size_factor, 1);
	static LLCachedControl<LLVector3> octree_distance_factor(gSavedSettings, "OctreeDistanceFactor");
	LLVector3 distance_factor = octree_distance_factor;
	static LLCachedControl<LLVector3> octree_alpha_distance_factor(gSavedSettings, "OctreeAlphaDistanceFactor");
	LLVector3 alpha_distance_factor = octree_alpha_distance_factor;

	const LLVector4a* ext = mDrawable->getSpatialExtents();

	BOOL shrink_wrap = mDrawable->isAnimating();
	BOOL alpha_wrap = FALSE;

	if (!isHUDAttachment())
	{
		for (S32 i = 0, count = mDrawable->getNumFaces(); i < count; ++i)
		{
			LLFace* face = mDrawable->getFace(i);
			if (face && face->getPoolType() == LLDrawPool::POOL_ALPHA &&
				!face->canRenderAsMask())
			{
				alpha_wrap = TRUE;
				break;
			}
		}
	}
	else
	{
		shrink_wrap = FALSE;
	}

	if (alpha_wrap)
	{
		LLVector3 bounds = getScale();
		radius = llmin(bounds.mV[1], bounds.mV[2]);
		radius = llmin(radius, bounds.mV[0]);
		radius *= 0.5f;
		radius *= 1.f + mDrawable->mDistanceWRTCamera * alpha_distance_factor[1];
		radius += mDrawable->mDistanceWRTCamera * alpha_distance_factor[0];
	}
	else if (shrink_wrap)
	{
		LLVector4a rad;
		rad.setSub(ext[1], ext[0]);

		radius = rad.getLength3().getF32() * 0.5f;
	}
	else if (mDrawable->isStatic())
	{
		radius = llmax((S32) mDrawable->getRadius(), 1) * size_factor;
		radius *= 1.f + mDrawable->mDistanceWRTCamera * distance_factor[1];
		radius += mDrawable->mDistanceWRTCamera * distance_factor[0];
	}
	else if (mDrawable->getVObj()->isAttachment())
	{
		radius = llmax((S32) mDrawable->getRadius(), 1) * attachment_size_factor;
	}
	else
	{
		radius = mDrawable->getRadius();
		radius *= 1.f + mDrawable->mDistanceWRTCamera * distance_factor[1];
		radius += mDrawable->mDistanceWRTCamera * distance_factor[0];
	}

	return llclamp(radius * scale, 0.5f, 256.f);
}

const LLVector3 LLVOVolume::getPivotPositionAgent() const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getPivotPosition();
	}
	return LLViewerObject::getPivotPositionAgent();
}

void LLVOVolume::onShift(const LLVector4a &shift_vector)
{
	if (mVolumeImpl)
	{
		mVolumeImpl->onShift(shift_vector);
	}

	updateRelativeXform();
}

const LLMatrix4& LLVOVolume::getWorldMatrix(LLXformMatrix* xform) const
{
	if (mVolumeImpl)
	{
		return mVolumeImpl->getWorldMatrix(xform);
	}
	return xform->getWorldMatrix();
}

LLVector3 LLVOVolume::agentPositionToVolume(const LLVector3& pos) const
{
	LLVector3 ret = pos - getRenderPosition();
	ret = ret * ~getRenderRotation();
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
	ret.scaleVec(invObjScale);

	return ret;
}

LLVector3 LLVOVolume::agentDirectionToVolume(const LLVector3& dir) const
{
	LLVector3 ret = dir * ~getRenderRotation();

	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	ret.scaleVec(objScale);

	return ret;
}

LLVector3 LLVOVolume::volumePositionToAgent(const LLVector3& dir) const
{
	LLVector3 ret = dir;
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	ret.scaleVec(objScale);
	ret = ret * getRenderRotation();
	ret += getRenderPosition();

	return ret;
}

LLVector3 LLVOVolume::volumeDirectionToAgent(const LLVector3& dir) const
{
	LLVector3 ret = dir;
	LLVector3 objScale = isVolumeGlobal() ? LLVector3(1,1,1) : getScale();
	LLVector3 invObjScale(1.f / objScale.mV[VX], 1.f / objScale.mV[VY], 1.f / objScale.mV[VZ]);
	ret.scaleVec(invObjScale);
	ret = ret * getRenderRotation();

	return ret;
}


BOOL LLVOVolume::lineSegmentIntersect(const LLVector3& start,
									  const LLVector3& end,
									  S32 face,
									  BOOL pick_transparent,
									  S32 *face_hitp,
									  LLVector3* intersection,
									  LLVector2* tex_coord,
									  LLVector3* normal,
									  LLVector3* bi_normal)

{
	if (!mbCanSelect || // (LLSelectMgr::getInstance()->mHideSelectedObjects && isSelected()) ||
		mDrawable->isDead() ||
		!gPipeline.hasRenderType(mDrawable->getRenderType()))
	{
		return FALSE;
	}

	BOOL ret = FALSE;

	LLVolume* volume = getVolume();

	bool transform = true;

	if (mDrawable->isState(LLDrawable::RIGGED))
	{
		if (gFloaterTools && gFloaterTools->getVisible() &&
			getAvatar()->isSelf())
		{
			updateRiggedVolume();
			genBBoxes(FALSE);
			volume = mRiggedVolume;
			transform = false;
		}
		else
		{	// cannot pick rigged attachments on other avatars or when not in
			// build mode
			return FALSE;
		}
	}

	if (volume)
	{
		LLVector3 v_start, v_end, v_dir;

		if (transform)
		{
			v_start = agentPositionToVolume(start);
			v_end = agentPositionToVolume(end);
		}
		else
		{
			v_start = start;
			v_end = end;
		}

		LLVector3 p;
		LLVector3 n;
		LLVector2 tc;
		LLVector3 bn;

		if (intersection != NULL)
		{
			p = *intersection;
		}

		if (tex_coord != NULL)
		{
			tc = *tex_coord;
		}

		if (normal != NULL)
		{
			n = *normal;
		}

		if (bi_normal != NULL)
		{
			bn = *bi_normal;
		}

		S32 face_hit = -1;

		S32 start_face, end_face;
		if (face == -1)
		{
			start_face = 0;
			end_face = volume->getNumVolumeFaces();
		}
		else
		{
			start_face = face;
			end_face = face + 1;
		}

		bool special_cursor = specialHoverCursor();
		for (S32 i = start_face; i < end_face; ++i)
		{
			if (!special_cursor && !pick_transparent && getTE(i) &&
				getTE(i)->getColor().mV[3] == 0.f)
			{	// don't attempt to pick completely transparent faces unless
				// pick_transparent is true
				continue;
			}

			face_hit = volume->lineSegmentIntersect(v_start, v_end, i,
													&p, &tc, &n, &bn);

			if (face_hit >= 0 && mDrawable->getNumFaces() > face_hit)
			{
				LLFace* face = mDrawable->getFace(face_hit);
				if (!face) continue;

				if (pick_transparent || !face->getTexture() ||
					!face->getTexture()->hasGLTexture() ||
					face->getTexture()->getMask(face->surfaceToTexture(tc, p, n)))
				{
					v_end = p;
					if (face_hitp != NULL)
					{
						*face_hitp = face_hit;
					}

					if (intersection != NULL)
					{
						if (transform)
						{
							*intersection = volumePositionToAgent(p);  // must map back to agent space
						}
						else
						{
							*intersection = p;
						}
					}

					if (normal != NULL)
					{
						if (transform)
						{
							*normal = volumeDirectionToAgent(n);
						}
						else
						{
							*normal = n;
						}
						(*normal).normVec();
					}

					if (bi_normal != NULL)
					{
						if (transform)
						{
							*bi_normal = volumeDirectionToAgent(bn);
						}
						else
						{
							*bi_normal = bn;
						}
						(*bi_normal).normVec();
					}

					if (tex_coord != NULL)
					{
						*tex_coord = tc;
					}

					ret = TRUE;
				}
			}
		}
	}

	return ret;
}

bool LLVOVolume::treatAsRigged()
{
	return gFloaterTools && gFloaterTools->getVisible() && isAttachment() &&
		   getAvatar() && getAvatar()->isSelf() &&
		   mDrawable.notNull() && mDrawable->isState(LLDrawable::RIGGED);
}

LLRiggedVolume* LLVOVolume::getRiggedVolume()
{
	return mRiggedVolume;
}

void LLVOVolume::clearRiggedVolume()
{
	if (mRiggedVolume.notNull())
	{
		mRiggedVolume = NULL;
		updateRelativeXform();
	}
}

void LLVOVolume::updateRiggedVolume()
{
	//Update mRiggedVolume to match current animation frame of avatar.
	//Also update position/size in octree.

	if (!treatAsRigged())
	{
		clearRiggedVolume();
		return;
	}

	LLVolume* volume = getVolume();

	const LLMeshSkinInfo* skin = gMeshRepo.getSkinInfo(volume->getParams().getSculptID(), this);

	if (!skin)
	{
		clearRiggedVolume();
		return;
	}

	LLVOAvatar* avatar = getAvatar();

	if (!avatar)
	{
		clearRiggedVolume();
		return;
	}

	if (!mRiggedVolume)
	{
		LLVolumeParams p;
		mRiggedVolume = new LLRiggedVolume(p);
		updateRelativeXform();
	}

	mRiggedVolume->update(skin, avatar, volume);
}

void LLRiggedVolume::update(const LLMeshSkinInfo* skin, LLVOAvatar* avatar,
							const LLVolume* volume)
{
	bool copy = false;
	if (volume->getNumVolumeFaces() != getNumVolumeFaces())
	{
		copy = true;
	}

	for (S32 i = 0, count = volume->getNumVolumeFaces(); i < count && !copy;
		 ++i)
	{
		const LLVolumeFace& src_face = volume->getVolumeFace(i);
		const LLVolumeFace& dst_face = getVolumeFace(i);

		if (src_face.mNumIndices != dst_face.mNumIndices ||
			src_face.mNumVertices != dst_face.mNumVertices)
		{
			copy = true;
		}
	}

	if (copy)
	{
		copyVolumeFaces(volume);
	}

	//build matrix palette
	LLMatrix4a mp[64];
	LLMatrix4* mat = (LLMatrix4*) mp;

	for (U32 j = 0; j < skin->mJointNames.size(); ++j)
	{
		LLJoint* joint = avatar->getJoint(skin->mJointNames[j]);
		if (joint)
		{
			mat[j] = skin->mInvBindMatrix[j];
			mat[j] *= joint->getWorldMatrix();
		}
	}

	for (S32 i = 0, count = volume->getNumVolumeFaces(); i < count; ++i)
	{
		const LLVolumeFace& vol_face = volume->getVolumeFace(i);

		LLVolumeFace& dst_face = mVolumeFaces[i];

		LLVector4a* weight = vol_face.mWeights;

		LLMatrix4a bind_shape_matrix;
		bind_shape_matrix.loadu(skin->mBindShapeMatrix);

		LLVector4a* pos = dst_face.mPositions;

		{
			LLFastTimer t(LLFastTimer::FTM_SKIN_RIGGED);

			for (U32 j = 0; j < dst_face.mNumVertices; ++j)
			{
				LLMatrix4a final_mat;
				final_mat.clear();

				S32 idx[4];

				LLVector4 wght;

				F32 scale = 0.f;
				for (U32 k = 0; k < 4; ++k)
				{
					F32 w = weight[j][k];

					idx[k] = (S32) floorf(w);
					wght[k] = w - floorf(w);
					scale += wght[k];
				}

				if (scale != 0.f)
				{
					wght /= scale;
				}
				else
				{
					llwarns << "Null scale divisor !" << llendl;
					// Use a max weight but let some headroom to avoid math overflows
					wght = LLVector4(F32_MAX / 1024.f, F32_MAX / 1024.f, F32_MAX / 1024.f);
				}

				for (U32 k = 0; k < 4; ++k)
				{
					F32 w = wght[k];

					LLMatrix4a src;
					src.setMul(mp[idx[k]], w);

					final_mat.add(src);
				}

				LLVector4a& v = vol_face.mPositions[j];
				LLVector4a t;
				LLVector4a dst;
				bind_shape_matrix.affineTransform(v, t);
				final_mat.affineTransform(t, dst);
				pos[j] = dst;
			}

			//update bounding box
			LLVector4a& min = dst_face.mExtents[0];
			LLVector4a& max = dst_face.mExtents[1];

			min = pos[0];
			max = pos[1];

			for (U32 j = 1; j < dst_face.mNumVertices; ++j)
			{
				min.setMin(min, pos[j]);
				max.setMax(max, pos[j]);
			}

			dst_face.mCenter->setAdd(dst_face.mExtents[0], dst_face.mExtents[1]);
			dst_face.mCenter->mul(0.5f);
		}

		{
			LLFastTimer t(LLFastTimer::FTM_RIGGED_OCTREE);
			delete dst_face.mOctree;
			dst_face.mOctree = NULL;

			LLVector4a size;
			size.setSub(dst_face.mExtents[1], dst_face.mExtents[0]);
			size.splat(size.getLength3().getF32() * 0.5f);

			dst_face.createOctree(1.f);
		}
	}
}

LLDeformedVolume* LLVOVolume::getDeformedVolume()
{
	if (!mDeformedVolume)
	{
		// doesn't exist yet - make it
		mDeformedVolume = new LLDeformedVolume;
	}

	return mDeformedVolume;
}

// static
std::vector<std::vector<std::vector<LLVector3> > >	LLDeformedVolume::sAvatarPositions;
std::vector<std::vector<std::vector<S32> > >		LLDeformedVolume::sAvatarIndices;

// these are the meshes we follow
static S32 MORPH_MESHES[] = {	LLVOAvatarDefines::MESH_ID_HEAD,
								LLVOAvatarDefines::MESH_ID_UPPER_BODY,
								LLVOAvatarDefines::MESH_ID_LOWER_BODY };

// static
void LLDeformedVolume::initializeBaseAvatars()
{
	if (sAvatarPositions.size() == NUM_BASESHAPES)
	{
		// already have data here, do nothing
		return;
	}

	// create default avatar
	LLPointer<LLVOAvatar> avatar = (LLVOAvatar*)gObjectList.createObjectViewer(LL_PCODE_LEGACY_AVATAR,
																			   gAgent.getRegion());
	avatar->mIsDummy = TRUE;
	avatar->mSpecialRenderMode = 2;

	sAvatarPositions.resize(NUM_BASESHAPES);
	sAvatarIndices.resize(NUM_BASESHAPES);

	// loop over all base avatar types
	for (S32 base_shape = 0; base_shape < NUM_BASESHAPES; ++base_shape)
	{
		avatar->updateJointLODs();
		if (base_shape == BASESHAPE_MALE)
		{
			avatar->setVisualParamWeight("male", 1.0f);
		}
		else
		{
			avatar->setVisualParamWeight("male", 0.0f);
		}

		avatar->updateVisualParams();

		S32 num_meshes = sizeof(MORPH_MESHES) / sizeof(S32);
		sAvatarIndices[base_shape].resize(num_meshes);
		sAvatarPositions[base_shape].resize(num_meshes);

		for (S32 j = 0; j < num_meshes; ++j)
		{
			S32 mesh_id = MORPH_MESHES[j];

			LLPolyMesh* avatar_mesh = avatar->getMesh(mesh_id);
			LLPolyFace* faces = avatar_mesh->getFaces();

			// store avatar indices (LLPolyFaces are triangles)
			S32 num_triangles = avatar_mesh->getNumFaces();
			sAvatarIndices[base_shape][j].resize(num_triangles * 3);
			for (S32 k = 0; k < num_triangles; ++k)
			{
				// indices for triangle corners
				S32 index0 = faces[k][0];
				S32 index1 = faces[k][1];
				S32 index2 = faces[k][2];

				sAvatarIndices[base_shape][j][k * 3 + 0] = index0;
				sAvatarIndices[base_shape][j][k * 3 + 1] = index1;
				sAvatarIndices[base_shape][j][k * 3 + 2] = index2;
			}

			// store avatar vertices
			const LLVector4* base_positions = avatar_mesh->getCoords();
			S32 num_vertices = avatar_mesh->getNumVertices();
			sAvatarPositions[base_shape][j].resize(num_vertices);
			for (S32 k = 0; k < num_vertices; ++k)
			{
				sAvatarPositions[base_shape][j][k] = LLVector3(base_positions[k]);
			}
		}
	}

	// clean-up avatar
	avatar->markDead();
}

// Attempt to find the cached deform table. If not found, generate request for
// it to be built and send to deform worker
LLDeformedVolume::deform_table_t* LLDeformedVolume::getDeformTable(LLVolume* source,
																   S32 base_shape,
																   LLVOAvatar* avatar,
																   const LLMeshSkinInfo* skin,
																   S32 face,
																   LLPointer<LLDrawable> drawable)
{
	// see if we can find this deform table in the cache
	LLDeformTableCacheIndex cache_index(base_shape,
										source->getParams().getSculptID(),
										face,
										source->getVolumeFace(face).mNumVertices);

	deform_cache_t::iterator i = mDeformCache.find(cache_index);

	if (i != mDeformCache.end())
	{
		// found it, return it
		return &(i->second);
	}

	// if we have not already done so, compute the base shape for our avatar
	initializeBaseAvatars();

	// It doesn't exist in cache, must create: send a request to the deformer
	// worker.

	LLPointer<LLDeformerWorker::Request> request = new LLDeformerWorker::Request;

	// pack the request
	request->mDeformedVolume = this;
	request->mDrawable = drawable;
	request->mBaseShape = base_shape;
	request->mMeshID = source->getParams().getSculptID();
	request->mFace = face;
	request->mVertexCount = source->getVolumeFace(face).mNumVertices;

	// before we do any more work, see if this request is already on the queue
	if (LLDeformerWorker::alreadyQueued(request))
	{
		// no more to be done here, return NULL
		return NULL;
	}

	// we copy this data now so that when we do the work in the second thread,
	// there's no chance it will be clobbered

	// fill volume vertex positions
	const LLVolumeFace& source_face = source->getVolumeFace(face);
	request->mVolumePositions.resize(source_face.mNumVertices);
	for (S32 i = 0; i < source_face.mNumVertices; ++i)
	{
		LLVector3 position(source_face.mPositions[i].getF32ptr());
		request->mVolumePositions[i] = position;
	}

	// scale and matrix constants

	// maps volume space to object (avatar) space
	LLMatrix4a bind_shape_matrix_a;
	request->mBindShapeMatrix.loadu(skin->mBindShapeMatrix);
	// skeleton scaling factor - assumes all joints have same scale and that
	// the scale is uniform in XYZ (currently true)
	request->mScale = powf(skin->mInvBindMatrix[0].determinant(), 1.0f / 3.0f);

	// submit request to deformer worker
	LLDeformerWorker::submitRequest(request);

	return NULL;
}

void LLDeformedVolume::computeDeformTable(LLPointer<LLDeformerWorker::Request> request)
{
	// See if this table is already in the cache; if so return (no work needed)
	LLDeformTableCacheIndex cache_index(request->mBaseShape,
										request->mMeshID,
										request->mFace,
										request->mVertexCount);
	deform_cache_t::iterator i = mDeformCache.find(cache_index);
	if (i != mDeformCache.end())
	{
		// found it, nothing more to do here
		return;
	}

	// tables are fairly small, we keep them around in case the user is
	// switching between meshes for poorman's animation one entry per face per
	// LOD per sculpt_ID
	U32 MAX_CACHE_SIZE = 20;

	if (mDeformCache.size() > MAX_CACHE_SIZE)
	{
		// this table will very infrequently fill - so make space in simple
		// way: delete element at random
		U32 random = ll_rand(MAX_CACHE_SIZE - 1);
		deform_cache_t::iterator iterator = mDeformCache.begin();
		for (U32 i = 0; i < random; ++i)
		{
			++iterator;
		}
		mDeformCache.erase(iterator);
	}

	deform_table_t deform_table;

	// table is a mapping from verts on the volume to verts on the avatar mesh.
	// currently we take closest point - but may want to modify this in favor
	// verts with similar skin weights and/or normals (so verts on the left leg
	// don't move with the right leg by accident.)

	// build mapping
	deform_table.resize(request->mVertexCount);

	S32 base_shape = request->mBaseShape;

	// for each vertex in the volume, find the closest point on the avatar
	// mesh(es)
	for (S32 i = 0; i < request->mVertexCount; ++i)
	{
		LLVector4a source_position;
		source_position.load3(request->mVolumePositions[i].mV);
		LLVector4a source_position_transformed;
		request->mBindShapeMatrix.affineTransform(source_position,
												  source_position_transformed);
		source_position_transformed.mul(request->mScale);

		// position to get closest to
		LLVector3 target(source_position_transformed.getF32ptr());

		F32 closest_distance_squared = F32_MAX;
		S32 closest_mesh = 0;
		// corners of the triangle containing the closest point;
		S32 closest_vertex[LL_DEFORMER_WEIGHT_COUNT];
		// weights of those corners (barycentric coordinates)
		F32 closest_vertex_weights[LL_DEFORMER_WEIGHT_COUNT];

		for (S32 k = 0; k < sizeof(MORPH_MESHES) / sizeof(S32); ++k)
		{
			// LLPolyFaces are triangles
			for (S32 j = 0; j < sAvatarIndices[base_shape][k].size(); j += 3)
			{
				// indices for triangle corners
				S32 index0 = sAvatarIndices[base_shape][k][j];
				S32 index1 = sAvatarIndices[base_shape][k][j + 1];
				S32 index2 = sAvatarIndices[base_shape][k][j + 2];

				// positions of triangle corners
				LLVector3 vert0 = sAvatarPositions[base_shape][k][index0];
				LLVector3 vert1 = sAvatarPositions[base_shape][k][index1];
				LLVector3 vert2 = sAvatarPositions[base_shape][k][index2];

				// barycentric coord of closest point
				F32 bary_a;
				F32 bary_b;

				F32 this_distance_squared = LLTriangleClosestPoint(vert0,
																   vert1,
																   vert2,
																   target,
																   bary_a,
																   bary_b);

				if (this_distance_squared < closest_distance_squared)
				{
					// record corner indices of this triangle
					closest_vertex[0] = index0;
					closest_vertex[1] = index1;
					closest_vertex[2] = index2;

					// compute weights from barycentric coordinates
					closest_vertex_weights[0] = 1.0f - bary_a - bary_b;
					closest_vertex_weights[1] = bary_a;
					closest_vertex_weights[2] = bary_b;

					// and closest mesh
					closest_mesh = k;

					closest_distance_squared = this_distance_squared;
				}
			}
		}

		// create entry for found vertices
		for (S32 k = 0; k < LL_DEFORMER_WEIGHT_COUNT; ++k)
		{
			deform_table[i].mMesh[k] = (U8)closest_mesh;
			deform_table[i].mVertex[k] = closest_vertex[k];
			deform_table[i].mWeight[k] = closest_vertex_weights[k];
		}
	}

	// now put it in the cache
	mDeformCache[cache_index] = deform_table;

	return;
}

bool LLDeformedVolume::deform(LLVolume* source, LLVOAvatar* avatar,
							  const LLMeshSkinInfo* skin, S32 face,
							  LLPointer<LLDrawable> drawable)
{
	LLFastTimer t(LLFastTimer::FTM_SKIN_RIGGED_DEFORM);

	EBaseShape base_shape;
	if (avatar->getVisualParamWeight("male") > 0.5f)
	{
		base_shape = BASESHAPE_MALE;
	}
	else
	{
		base_shape = BASESHAPE_FEMALE;
	}

	// get the deform table
	deform_table_t* deform_table = getDeformTable(source, base_shape, avatar,
												  skin, face, drawable);

	// if it's not available, fail
	if (!deform_table)
	{
		return false;
	}

	// first copy the source volume
	bool copy = false;
	if (source->getNumVolumeFaces() != getNumVolumeFaces())
	{
		copy = true;
	}

	for (S32 i = 0, count = source->getNumVolumeFaces(); i < count && !copy;
		 ++i)
	{
		const LLVolumeFace& src_face = source->getVolumeFace(i);
		const LLVolumeFace& dst_face = getVolumeFace(i);

		if (src_face.mNumIndices != dst_face.mNumIndices ||
			src_face.mNumVertices != dst_face.mNumVertices)
		{
			copy = true;
		}
	}

	if (copy)
	{
		copyVolumeFaces(source);
	}

	const LLVolumeFace& source_face = source->getVolumeFace(face);
	const LLVolumeFace& destination_face = getVolumeFace(face);

	// transforms volume space to avatar space
	LLMatrix4a bind_shape_matrix_a;
	bind_shape_matrix_a.loadu(skin->mBindShapeMatrix);

	// transforms back again
	LLMatrix4 bind_shape_matrix_inverse = skin->mBindShapeMatrix;
	bind_shape_matrix_inverse.invert_real();
	LLMatrix4a bind_shape_matrix_inverse_a;
	bind_shape_matrix_inverse_a.loadu(bind_shape_matrix_inverse);

	// skeleton scaling factor - assumes all joints have same scale and that
	// the scale is uniform in XYZ (currently true)
	F32 scale = powf(skin->mInvBindMatrix[0].determinant(), 1.0f / 3.0f);

	// if we have not already done so, compute the base shape for our avatar
	initializeBaseAvatars();

	for (S32 i = 0; i < source_face.mNumVertices; ++i)
	{
		LLVector4a weighted_morph_delta;
		weighted_morph_delta.clear();

		F32 total_weight = 0;

		for (S32 j = 0; j < LL_DEFORMER_WEIGHT_COUNT; ++j)
		{
			U16 closest_vertex = (*deform_table)[i].mVertex[j];
			S32 closest_mesh = (*deform_table)[i].mMesh[j];
			F32 closest_weight = (*deform_table)[i].mWeight[j];

			// find the current mesh
			LLPolyMesh* avatar_mesh = avatar->getMesh(MORPH_MESHES[closest_mesh]);
			// and get positions for morphed vertices
			const LLVector4* morph_positions = avatar_mesh->getCoords();

			// find the movement of this vertex due to morphing
			LLVector4a base_position;
			base_position.load3(sAvatarPositions[base_shape][closest_mesh][closest_vertex].mV);
			LLVector4a morph_position;
			morph_position.load3(morph_positions[closest_vertex].mV);
			LLVector4a morph_delta;
			morph_delta.setSub(morph_position, base_position);

			// weight this delta and add to total
			morph_delta.mul(closest_weight);
			weighted_morph_delta.add(morph_delta);
			total_weight += closest_weight;
		}

		// normalize
		weighted_morph_delta.mul(1.0f/total_weight);

		// source position mapped to avatar space
		LLVector4a source_position = source_face.mPositions[i];
		LLVector4a source_position_transformed;
		bind_shape_matrix_a.affineTransform(source_position,
											source_position_transformed);
		source_position_transformed.mul(scale);

		// modify by movement delta
		LLVector4a destination_position_transformed;
		destination_position_transformed.setAdd(source_position_transformed,
												weighted_morph_delta);

		// map back to volume space
		destination_position_transformed.mul(1.0f / scale);
		LLVector4a destination_position;
		bind_shape_matrix_inverse_a.affineTransform(destination_position_transformed,
													destination_position);

		destination_face.mPositions[i] = destination_position;
	}

	return true;	// success !
}

U32 LLVOVolume::getPartitionType() const
{
	if (isHUDAttachment())
	{
		return LLViewerRegion::PARTITION_HUD;
	}

	return LLViewerRegion::PARTITION_VOLUME;
}

LLVolumePartition::LLVolumePartition()
:	LLSpatialPartition(LLVOVolume::VERTEX_DATA_MASK, TRUE, GL_DYNAMIC_DRAW_ARB)
{
	mLODPeriod = 32;
	mDepthMask = FALSE;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_VOLUME;
	mSlopRatio = 0.25f;
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;
}

LLVolumeBridge::LLVolumeBridge(LLDrawable* drawablep)
:	LLSpatialBridge(drawablep, TRUE, LLVOVolume::VERTEX_DATA_MASK)
{
	mDepthMask = FALSE;
	mLODPeriod = 32;
	mDrawableType = LLPipeline::RENDER_TYPE_VOLUME;
	mPartitionType = LLViewerRegion::PARTITION_BRIDGE;

	mBufferUsage = GL_DYNAMIC_DRAW_ARB;

	mSlopRatio = 0.25f;
}

void LLVolumeGeometryManager::registerFace(LLSpatialGroup* group, LLFace* facep, U32 type)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);

	if (facep->getViewerObject()->isSelected()
//MK
		&& (!gRRenabled || !gAgent.mRRInterface.mContainsEdit)
//mk
		&& LLSelectMgr::getInstance()->mHideSelectedObjects)
	{
		return;
	}

	//add face to drawmap
	LLSpatialGroup::drawmap_elem_t& draw_vec = group->mDrawMap[type];

	S32 idx = draw_vec.size()-1;


	BOOL fullbright = (type == LLRenderPass::PASS_FULLBRIGHT ||
					   type == LLRenderPass::PASS_INVISIBLE ||
					   (type == LLRenderPass::PASS_ALPHA && facep->isState(LLFace::FULLBRIGHT)));

	if (!fullbright && type != LLRenderPass::PASS_GLOW &&
		!facep->getVertexBuffer()->hasDataType(LLVertexBuffer::TYPE_NORMAL))
	{
		LL_WARNS("RenderVolume") << "Non fullbright face has no normals!" << LL_ENDL;
		return;
	}

	const LLMatrix4* tex_mat = NULL;
	if (facep->isState(LLFace::TEXTURE_ANIM) && facep->getVirtualSize() > MIN_TEX_ANIM_SIZE)
	{
		tex_mat = facep->mTextureMatrix;
	}

	const LLMatrix4* model_mat = NULL;

	LLDrawable* drawable = facep->getDrawable();
	if (drawable->isActive())
	{
		model_mat = &(drawable->getRenderMatrix());
	}
	else
	{
		model_mat = &(drawable->getRegion()->mRenderMatrix);
		if (model_mat->isIdentity())
		{
			model_mat = NULL;
		}
	}

	U8 bump = (type == LLRenderPass::PASS_BUMP || type == LLRenderPass::PASS_POST_BUMP ? facep->getTextureEntry()->getBumpmap() : 0);

	LLViewerTexture* tex = facep->getTexture();

	U8 glow = (U8)(facep->getTextureEntry()->getGlow() * 255);

	if (idx >= 0 &&
		draw_vec[idx]->mVertexBuffer == facep->getVertexBuffer() &&
		draw_vec[idx]->mEnd == facep->getGeomIndex() - 1 &&
		(LLPipeline::sTextureBindTest || draw_vec[idx]->mTexture == tex) &&
#if LL_DARWIN
		draw_vec[idx]->mEnd - draw_vec[idx]->mStart + facep->getGeomCount() <= (U32) gGLManager.mGLMaxVertexRange &&
		draw_vec[idx]->mCount + facep->getIndicesCount() <= (U32) gGLManager.mGLMaxIndexRange &&
#endif
		draw_vec[idx]->mGlowColor.mV[3] == glow &&
		draw_vec[idx]->mFullbright == fullbright &&
		draw_vec[idx]->mBump == bump &&
		draw_vec[idx]->mTextureMatrix == tex_mat &&
		draw_vec[idx]->mModelMatrix == model_mat)
	{
		draw_vec[idx]->mCount += facep->getIndicesCount();
		draw_vec[idx]->mEnd += facep->getGeomCount();
		draw_vec[idx]->mVSize = llmax(draw_vec[idx]->mVSize, facep->getVirtualSize());
		draw_vec[idx]->validate();
		update_min_max(draw_vec[idx]->mExtents[0], draw_vec[idx]->mExtents[1], facep->mExtents[0]);
		update_min_max(draw_vec[idx]->mExtents[0], draw_vec[idx]->mExtents[1], facep->mExtents[1]);
	}
	else
	{
		U32 start = facep->getGeomIndex();
		U32 end = start + facep->getGeomCount()-1;
		U32 offset = facep->getIndicesStart();
		U32 count = facep->getIndicesCount();
		LLPointer<LLDrawInfo> draw_info = new LLDrawInfo(start, end, count,
														 offset, tex,
														 facep->getVertexBuffer(),
														 fullbright, bump);
		draw_info->mGroup = group;
		draw_info->mVSize = facep->getVirtualSize();
		draw_vec.push_back(draw_info);
		draw_info->mTextureMatrix = tex_mat;
		draw_info->mModelMatrix = model_mat;
		draw_info->mGlowColor.setVec(0,0,0,glow);
		if (type == LLRenderPass::PASS_ALPHA)
		{	//for alpha sorting
			facep->setDrawInfo(draw_info);
		}
		draw_info->mExtents[0] = facep->mExtents[0];
		draw_info->mExtents[1] = facep->mExtents[1];

		if (LLPipeline::sUseTriStrips)
		{
			draw_info->mDrawMode = LLRender::TRIANGLE_STRIP;
		}

		draw_info->validate();
	}
}

void LLVolumeGeometryManager::getGeometry(LLSpatialGroup* group)
{
}

static LLDrawPoolAvatar* get_avatar_drawpool(LLViewerObject* vobj)
{
	LLVOAvatar* avatar = vobj->getAvatar();

	if (avatar)
	{
		LLDrawable* drawable = avatar->mDrawable;
		if (drawable && drawable->getNumFaces() > 0)
		{
			LLFace* face = drawable->getFace(0);
			if (face)
			{
				LLDrawPool* drawpool = face->getPool();
				if (drawpool)
				{
					if (drawpool->getType() == LLDrawPool::POOL_AVATAR)
					{
						return (LLDrawPoolAvatar*) drawpool;
					}
				}
			}
		}
	}

	return NULL;
}

void LLVolumeGeometryManager::rebuildGeom(LLSpatialGroup* group)
{
	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
	}

	group->mLastUpdateViewAngle = group->mViewAngle;

	if (!group->isState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY))
	{
		if (group->isState(LLSpatialGroup::MESH_DIRTY) && !LLPipeline::sDelayVBUpdate)
		{
			LLFastTimer ftm(LLFastTimer::FTM_REBUILD_VBO);
			LLFastTimer ftm2(LLFastTimer::FTM_REBUILD_VOLUME_VB);

			rebuildMesh(group);
		}
		return;
	}

	group->mBuilt = 1.f;
	LLFastTimer ftm(LLFastTimer::FTM_REBUILD_VBO);

	LLFastTimer ftm2(LLFastTimer::FTM_REBUILD_VOLUME_VB);

	group->clearDrawMap();

	mFaceList.clear();

	std::vector<LLFace*> fullbright_faces;
	std::vector<LLFace*> bump_faces;
	std::vector<LLFace*> simple_faces;

	std::vector<LLFace*> alpha_faces;
	U32 useage = group->mSpatialPartition->mBufferUsage;

	U32 vertex_size = (U32)LLVertexBuffer::calcVertexSize(group->mSpatialPartition->mVertexDataMask);
	U32 max_vertices = LLPipeline::sRenderMaxVBOSize * 1024 / vertex_size;
	static LLCachedControl<U32> render_max_node_size(gSavedSettings, "RenderMaxNodeSize");
	U32 max_total = render_max_node_size * 1024 / vertex_size;
	max_vertices = llmin(max_vertices, (U32) 65535);

	U32 cur_total = 0;

	//get all the faces into a list
	for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(),
									  end = group->getData().end();
		 drawable_iter != end; ++drawable_iter)
	{
		LLDrawable* drawablep = *drawable_iter;

		if (drawablep->isDead() || drawablep->isState(LLDrawable::FORCE_INVISIBLE))
		{
			continue;
		}

		if (drawablep->isAnimating())
		{	//fall back to stream draw for animating verts
			useage = GL_STREAM_DRAW_ARB;
		}

		LLVOVolume* vobj = drawablep->getVOVolume();
		if (!vobj)
		{
			continue;
		}

		if (vobj->isMesh() &&
			(!gMeshRepo.meshRezEnabled() ||
			 !vobj->getVolume() || !vobj->getVolume()->isMeshAssetLoaded()))
		{
			continue;
		}

		vobj->updateTextureVirtualSize(true);
		vobj->preRebuild();

		drawablep->clearState(LLDrawable::HAS_ALPHA);

		bool rigged = vobj->isAttachment() && vobj->isMesh() &&
					  gMeshRepo.getSkinInfo(vobj->getVolume()->getParams().getSculptID(), vobj);

		bool bake_sunlight = LLPipeline::sBakeSunlight && drawablep->isStatic();

		bool is_rigged = false;

		// for each face
		for (S32 i = 0, count = drawablep->getNumFaces(); i < count; ++i)
		{
			LLFace* facep = drawablep->getFace(i);
			if (!facep) continue;

			// ALWAYS null out vertex buffer on rebuild -- if the face lands in
			// a render batch, it will recover its vertex buffer reference from
			// the spatial group
			facep->setVertexBuffer(NULL);

			// sum up face verts and indices
			drawablep->updateFaceSize(i);

			if (rigged)
			{
				if (!facep->isState(LLFace::RIGGED))
				{	//completely reset vertex buffer
					facep->clearVertexBuffer();
				}

				facep->setState(LLFace::RIGGED);
				is_rigged = true;

				//get drawpool of avatar with rigged face
				LLDrawPoolAvatar* pool = get_avatar_drawpool(vobj);

				// Determine if we've received skininfo that contains an
				// alternate bind matrix - if it does then apply the
				// translational component to the joints of the avatar.
				LLVOAvatar* pAvatarVO = vobj->getAvatar();
				bool pelvisGotSet = false;

				if (pAvatarVO)
				{
					LLUUID currentId = vobj->getVolume()->getParams().getSculptID();
					const LLMeshSkinInfo*  pSkinData = gMeshRepo.getSkinInfo(currentId, vobj);

					if (pSkinData)
					{
						const int bindCnt = pSkinData->mAlternateBindMatrix.size();
						if (bindCnt > 0)
						{
							const int jointCnt = pSkinData->mJointNames.size();
							const F32 pelvisZOffset = pSkinData->mPelvisOffset;
							bool fullRig = (jointCnt >= 20) ? true : false;
							if (fullRig)
							{
								for (int i = 0; i < jointCnt; ++i)
								{
									std::string lookingForJoint = pSkinData->mJointNames[i].c_str();
									//llinfos << "joint name " << lookingForJoint.c_str() << llendl;
									LLJoint* pJoint = pAvatarVO->getJoint(lookingForJoint);
									if (pJoint && pJoint->getId() != currentId)
									{
										pJoint->setId(currentId);
										const LLVector3& jointPos = pSkinData->mAlternateBindMatrix[i].getTranslation();
										// Set the joint position
										pJoint->storeCurrentXform(jointPos);
										// If joint is a pelvis then handle
										// old/new pelvis to foot values
										if (lookingForJoint == "mPelvis")
										{
											pJoint->storeCurrentXform(jointPos);
											if (!pAvatarVO->hasPelvisOffset())
											{
												pAvatarVO->setPelvisOffset(true, jointPos, pelvisZOffset);
												//Trigger to rebuild viewer AV
												pelvisGotSet = true;
											}
										}
									}
								}
							}
						}
					}
				}
				// If we've set the pelvis to a new position we need to also
				// rebuild some information that the viewer does at launch
				// (e.g. body size etc.)
				if (pelvisGotSet)
				{
					pAvatarVO->postPelvisSetRecalc();
				}

				if (pool)
				{
					const LLTextureEntry* te = facep->getTextureEntry();

					// remove face from old pool if it exists
					LLDrawPool* old_pool = facep->getPool();
					if (old_pool && old_pool->getType() == LLDrawPool::POOL_AVATAR)
					{
						((LLDrawPoolAvatar*)old_pool)->removeRiggedFace(facep);
					}

					//add face to new pool
					LLViewerTexture* tex = facep->getTexture();
					U32 type = gPipeline.getPoolTypeFromTE(te, tex);

					if (type == LLDrawPool::POOL_ALPHA)
					{
						if (te->getColor().mV[3] > 0.f)
						{
							if (te->getFullbright())
							{
								pool->addRiggedFace(facep,
													LLDrawPoolAvatar::RIGGED_FULLBRIGHT_ALPHA);
							}
							else
							{
								pool->addRiggedFace(facep,
													LLDrawPoolAvatar::RIGGED_ALPHA);
							}
						}
					}
					else if (te->getShiny())
					{
						if (te->getFullbright())
						{
							pool->addRiggedFace(facep,
												LLDrawPoolAvatar::RIGGED_FULLBRIGHT_SHINY);
						}
						else
						{
							if (LLPipeline::sRenderDeferred)
							{
								pool->addRiggedFace(facep,
													LLDrawPoolAvatar::RIGGED_SIMPLE);
							}
							else
							{
								pool->addRiggedFace(facep,
													LLDrawPoolAvatar::RIGGED_SHINY);
							}
						}
					}
					else
					{
						if (te->getFullbright())
						{
							pool->addRiggedFace(facep,
												LLDrawPoolAvatar::RIGGED_FULLBRIGHT);
						}
						else
						{
							pool->addRiggedFace(facep,
												LLDrawPoolAvatar::RIGGED_SIMPLE);
						}
					}

					if (te->getGlow())
					{
						pool->addRiggedFace(facep,
											LLDrawPoolAvatar::RIGGED_GLOW);
					}

					if (LLPipeline::sRenderDeferred)
					{
						if (type != LLDrawPool::POOL_ALPHA &&
							!te->getFullbright())
						{
							if (te->getBumpmap())
							{
								pool->addRiggedFace(facep,
													LLDrawPoolAvatar::RIGGED_DEFERRED_BUMP);
							}
							else
							{
								pool->addRiggedFace(facep,
													LLDrawPoolAvatar::RIGGED_DEFERRED_SIMPLE);
							}
						}
					}
				}

				continue;
			}
			else
			{
				if (facep->isState(LLFace::RIGGED))
				{	// face is not rigged but used to be, remove from rigged
					// face pool
					LLDrawPoolAvatar* pool = (LLDrawPoolAvatar*) facep->getPool();
					if (pool)
					{
						pool->removeRiggedFace(facep);
					}
					facep->clearState(LLFace::RIGGED);
				}
			}

			if (cur_total > max_total || facep->getIndicesCount() <= 0 ||
				facep->getGeomCount() <= 0)
			{
				facep->clearVertexBuffer();
				continue;
			}

			cur_total += facep->getGeomCount();

			if (facep->hasGeometry() &&
				facep->getPixelArea() > FORCE_CULL_AREA)
			{
				const LLTextureEntry* te = facep->getTextureEntry();
				LLViewerTexture* tex = facep->getTexture();

				if (facep->isState(LLFace::TEXTURE_ANIM))
				{
					if (!vobj->mTexAnimMode)
					{
						facep->clearState(LLFace::TEXTURE_ANIM);
					}
				}

				BOOL force_simple = (facep->getPixelArea() < FORCE_SIMPLE_RENDER_AREA);
				U32 type = gPipeline.getPoolTypeFromTE(te, tex);
				if (type != LLDrawPool::POOL_ALPHA && force_simple)
				{
					type = LLDrawPool::POOL_SIMPLE;
				}
				facep->setPoolType(type);

				if (vobj->isHUDAttachment())
				{
					facep->setState(LLFace::FULLBRIGHT);
				}

				if (vobj->mTextureAnimp && vobj->mTexAnimMode)
				{
					if (vobj->mTextureAnimp->mFace <= -1)
					{
						for (S32 face = 0, count = vobj->getNumTEs();
							 face < count; ++face)
						{
							LLFace* facep2 = drawablep->getFace(face);
							if (facep2)
							{
								facep2->setState(LLFace::TEXTURE_ANIM);
							}
						}
					}
					else if (vobj->mTextureAnimp->mFace < vobj->getNumTEs())
					{
						LLFace* facep2 = drawablep->getFace(vobj->mTextureAnimp->mFace);
						if (facep2)
						{
							facep2->setState(LLFace::TEXTURE_ANIM);
						}
					}
				}

				if (type == LLDrawPool::POOL_ALPHA)
				{
					if (facep->canRenderAsMask())
					{	// Can be treated as alpha mask
						simple_faces.push_back(facep);
					}
					else
					{
						if (te->getColor().mV[3] > 0.f)
						{	// only treat as alpha in the pipeline if < 100%
							// transparent
							drawablep->setState(LLDrawable::HAS_ALPHA);
						}
						alpha_faces.push_back(facep);
					}
				}
				else
				{
					if (drawablep->isState(LLDrawable::REBUILD_VOLUME))
					{
						facep->mLastUpdateTime = gFrameTimeSeconds;
					}

					if (gPipeline.canUseWindLightShadersOnObjects() &&
						LLPipeline::sRenderBump)
					{
						if (te->getBumpmap())
						{	//needs normal + binormal
							bump_faces.push_back(facep);
						}
						else if (te->getShiny() || !te->getFullbright())
						{	//needs normal
							simple_faces.push_back(facep);
						}
						else
						{	//doesn't need normal
							facep->setState(LLFace::FULLBRIGHT);
							fullbright_faces.push_back(facep);
						}
					}
					else
					{
						if (te->getBumpmap() && LLPipeline::sRenderBump)
						{	//needs normal + binormal
							bump_faces.push_back(facep);
						}
						else if ((te->getShiny() && LLPipeline::sRenderBump) ||
								 !(te->getFullbright() || bake_sunlight))
						{	//needs normal
							simple_faces.push_back(facep);
						}
						else
						{	//doesn't need normal
							facep->setState(LLFace::FULLBRIGHT);
							fullbright_faces.push_back(facep);
						}
					}
				}
			}
			else
			{	// face has no renderable geometry
				facep->clearVertexBuffer();
			}
		}

		if (is_rigged)
		{
			drawablep->setState(LLDrawable::RIGGED);
		}
		else
		{
			drawablep->clearState(LLDrawable::RIGGED);
		}
	}

	group->mBufferUsage = useage;

	//PROCESS NON-ALPHA FACES
	U32 simple_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;
	U32 alpha_mask = simple_mask | 0x80000000; //hack to give alpha verts their own VBO
	U32 bump_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;
	U32 fullbright_mask = LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR;

	if (LLPipeline::sRenderDeferred)
	{
		bump_mask |= LLVertexBuffer::MAP_BINORMAL;
	}

	genDrawInfo(group, simple_mask, simple_faces);
	genDrawInfo(group, fullbright_mask, fullbright_faces);
	genDrawInfo(group, bump_mask, bump_faces, FALSE);
	genDrawInfo(group, alpha_mask, alpha_faces, TRUE);

	if (!LLPipeline::sDelayVBUpdate)
	{
		//drawables have been rebuilt, clear rebuild status
		for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(),
										  end = group->getData().end();
			 drawable_iter != end; ++drawable_iter)
		{
			LLDrawable* drawablep = *drawable_iter;
			drawablep->clearState(LLDrawable::REBUILD_ALL);
		}
	}

	group->mLastUpdateTime = gFrameTimeSeconds;
	group->mBuilt = 1.f;
	group->clearState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY);

	if (LLPipeline::sDelayVBUpdate)
	{
		group->setState(LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
	}

	mFaceList.clear();
}

void LLVolumeGeometryManager::rebuildMesh(LLSpatialGroup* group)
{
	llassert(group);
	if (group && group->isState(LLSpatialGroup::MESH_DIRTY) &&
		!group->isState(LLSpatialGroup::GEOM_DIRTY))
	{
		LLFastTimer tm(LLFastTimer::FTM_VOLUME_GEOM);
		S32 num_mapped_veretx_buffer = LLVertexBuffer::sMappedCount;

		group->mBuilt = 1.f;

		std::set<LLVertexBuffer*> mapped_buffers;

		for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(),
										  end = group->getData().end();
			 drawable_iter != end; ++drawable_iter)
		{
			LLFastTimer t(LLFastTimer::FTM_VOLUME_GEOM_PARTIAL);
			LLDrawable* drawablep = *drawable_iter;

			if (!drawablep->isDead() && drawablep->isState(LLDrawable::REBUILD_ALL))
			{
				LLVOVolume* vobj = drawablep->getVOVolume();
				vobj->preRebuild();

				LLVolume* volume = vobj->getVolume();
				for (S32 i = 0, count = drawablep->getNumFaces(); i < count;
					 ++i)
				{
					LLFace* face = drawablep->getFace(i);
					if (face)
					{
						LLVertexBuffer* buff = face->getVertexBuffer();
						if (buff)
						{
							face->getGeometryVolume(*volume, face->getTEOffset(),
													vobj->getRelativeXform(),
													vobj->getRelativeXformInvTrans(),
													face->getGeomIndex());
							if (buff->isLocked())
							{
								mapped_buffers.insert(buff);
							}
						}
					}
				}

				drawablep->clearState(LLDrawable::REBUILD_ALL);
			}
		}

		for (std::set<LLVertexBuffer*>::iterator iter = mapped_buffers.begin(),
												 end = mapped_buffers.end();
			 iter != end; ++iter)
		{
			(*iter)->setBuffer(0);
		}

		// don't forget alpha
		if (group && !group->mVertexBuffer.isNull() &&
			group->mVertexBuffer->isLocked())
		{
			group->mVertexBuffer->setBuffer(0);
		}

		// if not all buffers are unmapped
		if (num_mapped_veretx_buffer != LLVertexBuffer::sMappedCount)
		{
			LL_WARNS_ONCE("RebuildMesh") << "Not all mapped vertex buffers are unmapped !"
										 << LL_ENDL;
			for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(),
											  end = group->getData().end();
				 drawable_iter != end; ++drawable_iter)
			{
				LLDrawable* drawablep = *drawable_iter;
				for (S32 i = 0, count = drawablep->getNumFaces(); i < count;
					 ++i)
				{
					LLFace* face = drawablep->getFace(i);
					if (face)
					{
						LLVertexBuffer* buff = face->getVertexBuffer();
						if (buff && buff->isLocked())
						{
							buff->setBuffer(0);
						}
					}
				}
			}
		}

		group->clearState(LLSpatialGroup::MESH_DIRTY | LLSpatialGroup::NEW_DRAWINFO);
	}

	llassert(!group || !group->isState(LLSpatialGroup::NEW_DRAWINFO));
}

void LLVolumeGeometryManager::genDrawInfo(LLSpatialGroup* group,
										  U32 mask,
										  std::vector<LLFace*>& faces,
										  BOOL distance_sort)
{
	//calculate maximum number of vertices to store in a single buffer
	U32 max_vertices = (LLPipeline::sRenderMaxVBOSize * 1024) / LLVertexBuffer::calcVertexSize(group->mSpatialPartition->mVertexDataMask);
	max_vertices = llmin(max_vertices, (U32) 65535);

	if (!distance_sort)
	{
		//sort faces by things that break batches
		std::sort(faces.begin(), faces.end(), LLFace::CompareBatchBreaker());
	}
	else
	{
		//sort faces by distance
		std::sort(faces.begin(), faces.end(), LLFace::CompareDistanceGreater());
	}

	bool hud_group = group->isHUDGroup();
	std::vector<LLFace*>::iterator face_iter = faces.begin();
	std::vector<LLFace*>::iterator faces_end = faces.end();

	LLSpatialGroup::buffer_map_t buffer_map;

	LLViewerTexture* last_tex = NULL;
	S32 buffer_index = 0;

	if (distance_sort)
	{
		buffer_index = -1;
	}

	while (face_iter != faces_end)
	{
		// pull off next face
		LLFace* facep = *face_iter;
		LLViewerTexture* tex = facep->getTexture();

		if (distance_sort)
		{
			tex = NULL;
		}

		if (last_tex == tex)
		{
			++buffer_index;
		}
		else
		{
			last_tex = tex;
			buffer_index = 0;
		}

		bool bake_sunlight = LLPipeline::sBakeSunlight && facep->getDrawable()->isStatic();

		U32 index_count = facep->getIndicesCount();
		U32 geom_count = facep->getGeomCount();

		// sum up vertices needed for this render batch
		std::vector<LLFace*>::iterator i = face_iter;
		++i;

		while (i != faces_end &&
			   (LLPipeline::sTextureBindTest || distance_sort ||
				(*i)->getTexture() == tex))
		{
			facep = *i;

			//face has no texture index
			facep->mDrawInfo = NULL;

			if (geom_count + facep->getGeomCount() > max_vertices)
			{	//cut batches on geom count too big
				break;
			}

			++i;
			index_count += facep->getIndicesCount();
			geom_count += facep->getGeomCount();
		}

		//create/delete/resize vertex buffer if needed
		LLVertexBuffer* buffer = NULL;
		LLSpatialGroup::buffer_texture_map_t::iterator found_iter = group->mBufferMap[mask].find(tex);

		if (found_iter != group->mBufferMap[mask].end())
		{
			if ((U32) buffer_index < found_iter->second.size())
			{
				buffer = found_iter->second[buffer_index];
			}
		}

		if (!buffer)
		{	//create new buffer if needed
			buffer = createVertexBuffer(mask, group->mBufferUsage);
			buffer->allocateBuffer(geom_count, index_count, TRUE);
		}
		else
		{	//resize pre-existing buffer
			if ((LLVertexBuffer::sEnableVBOs && buffer->getUsage() != group->mBufferUsage)
				|| buffer->getTypeMask() != mask)
			{
				buffer = createVertexBuffer(mask, group->mBufferUsage);
				buffer->allocateBuffer(geom_count, index_count, TRUE);
			}
			else
			{
				buffer->resizeBuffer(geom_count, index_count);
			}
		}

		buffer_map[mask][tex].push_back(buffer);

		//add face geometry

		U32 indices_index = 0;
		U16 index_offset = 0;

		while (face_iter < i)
		{	//update face indices for new buffer
			facep = *face_iter;
			facep->setIndicesIndex(indices_index);
			facep->setGeomIndex(index_offset);
			facep->setVertexBuffer(buffer);
			{
				// for debugging, set last time face was updated vs moved
				facep->updateRebuildFlags();

				if (!LLPipeline::sDelayVBUpdate)
				{	//copy face geometry into vertex buffer
					LLDrawable* drawablep = facep->getDrawable();
					LLVOVolume* vobj = drawablep->getVOVolume();
					LLVolume* volume = vobj->getVolume();

					U32 te_idx = facep->getTEOffset();

					facep->getGeometryVolume(*volume, te_idx,
											 vobj->getRelativeXform(),
											 vobj->getRelativeXformInvTrans(),
											 index_offset);
				}
			}

			index_offset += facep->getGeomCount();
			indices_index += facep->getIndicesCount();

			// append face to appropriate render batch

			BOOL force_simple = facep->getPixelArea() < FORCE_SIMPLE_RENDER_AREA;
			BOOL fullbright = facep->isState(LLFace::FULLBRIGHT);
			if ((mask & LLVertexBuffer::MAP_NORMAL) == 0)
			{	// paranoia check to make sure GL doesn't try to read
				// non-existant normals
				fullbright = TRUE;
			}

			const LLTextureEntry* te = facep->getTextureEntry();
			tex = facep->getTexture();

			BOOL is_alpha = (facep->getPoolType() == LLDrawPool::POOL_ALPHA ? TRUE : FALSE);

			if (is_alpha)
			{
				// can we safely treat this as an alpha mask?
				if (facep->canRenderAsMask())
				{
					if (te->getFullbright() || LLPipeline::sNoAlpha)
					{
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK);
					}
					else
					{
						registerFace(group, facep, LLRenderPass::PASS_ALPHA_MASK);
					}
				}
				else
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA);
				}

				if (LLPipeline::sRenderDeferred)
				{
					registerFace(group, facep, LLRenderPass::PASS_ALPHA_SHADOW);
				}
			}
			else if (gPipeline.canUseVertexShaders() && LLPipeline::sRenderBump &&
					 group->mSpatialPartition->mPartitionType != LLViewerRegion::PARTITION_HUD
					 && te->getShiny())
			{	//shiny
				if (tex->getPrimaryFormat() == GL_ALPHA)
				{	// invisiprim+shiny
					registerFace(group, facep, LLRenderPass::PASS_INVISI_SHINY);
					registerFace(group, facep, LLRenderPass::PASS_INVISIBLE);
				}
				else if (LLPipeline::sRenderDeferred && !hud_group)
				{	// deferred rendering
					if (te->getFullbright())
					{	// register in post deferred fullbright shiny pass
						registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_SHINY);
						if (te->getBumpmap())
						{	//register in post deferred bump pass
							registerFace(group, facep, LLRenderPass::PASS_POST_BUMP);
						}
					}
					else if (te->getBumpmap())
					{	// register in deferred bump pass
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
					else
					{	// register in deferred simple pass (deferred simple includes shiny)
						llassert(mask & LLVertexBuffer::MAP_NORMAL);
						registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
					}
				}
				else if (fullbright)
				{	// not deferred, register in standard fullbright shiny pass
					registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT_SHINY);
				}
				else
				{	// not deferred or fullbright, register in standard shiny pass
					registerFace(group, facep, LLRenderPass::PASS_SHINY);
				}
			}
			else
			{	// not alpha and not shiny
				if (!is_alpha && tex->getPrimaryFormat() == GL_ALPHA)
				{	//invisiprim
					registerFace(group, facep, LLRenderPass::PASS_INVISIBLE);
				}
				else if (fullbright || bake_sunlight)
				{	// fullbright
					registerFace(group, facep, LLRenderPass::PASS_FULLBRIGHT);
					if (LLPipeline::sRenderDeferred && !hud_group &&
						LLPipeline::sRenderBump && te->getBumpmap())
					{	//if this is the deferred render and a bump map is present,
						// register in post deferred bump
						registerFace(group, facep, LLRenderPass::PASS_POST_BUMP);
					}
				}
				else
				{
					if (LLPipeline::sRenderDeferred && LLPipeline::sRenderBump
						&& te->getBumpmap())
					{	// non-shiny or fullbright deferred bump
						registerFace(group, facep, LLRenderPass::PASS_BUMP);
					}
					else
					{	//all around simple
						llassert(mask & LLVertexBuffer::MAP_NORMAL);
						registerFace(group, facep, LLRenderPass::PASS_SIMPLE);
					}
				}

				// not sure why this is here -- shiny HUD attachments maybe?
				// -- davep 5/11/2010
				if (!is_alpha && te->getShiny() && LLPipeline::sRenderBump)
				{
					registerFace(group, facep, LLRenderPass::PASS_SHINY);
				}
			}

			// not sure why this is here, and looks like it might cause bump
			// mapped objects to get rendered redundantly -- davep 5/11/2010
			if (!is_alpha && (hud_group || !LLPipeline::sRenderDeferred))
			{
				llassert((mask & LLVertexBuffer::MAP_NORMAL) || fullbright);
				facep->setPoolType((fullbright) ? LLDrawPool::POOL_FULLBRIGHT : LLDrawPool::POOL_SIMPLE);

				if (!force_simple && te->getBumpmap() && LLPipeline::sRenderBump)
				{
					registerFace(group, facep, LLRenderPass::PASS_BUMP);
				}
			}

			if (!is_alpha && LLPipeline::sRenderGlow && te->getGlow() > 0.f)
			{
				registerFace(group, facep, LLRenderPass::PASS_GLOW);
			}

			++face_iter;
		}

		buffer->setBuffer(0);
	}

	group->mBufferMap[mask].clear();
	for (LLSpatialGroup::buffer_texture_map_t::iterator
			i = buffer_map[mask].begin(), end = buffer_map[mask].end();
		 i != end; ++i)
	{
		group->mBufferMap[mask][i->first] = i->second;
	}
}

void LLGeometryManager::addGeometryCount(LLSpatialGroup* group,
										 U32 &vertex_count, U32 &index_count)
{
	// initialize to default usage for this partition
	U32 usage = group->mSpatialPartition->mBufferUsage;

	//clear off any old faces
	mFaceList.clear();

	// for each drawable
	for (LLSpatialGroup::element_iter drawable_iter = group->getData().begin(),
									  end = group->getData().end();
		 drawable_iter != end; ++drawable_iter)
	{
		LLDrawable* drawablep = *drawable_iter;

		if (drawablep->isDead())
		{
			continue;
		}

		if (drawablep->isAnimating())
		{	// fall back to stream draw for animating verts
			usage = GL_STREAM_DRAW_ARB;
		}

		// for each face
		for (S32 i = 0, count = drawablep->getNumFaces(); i < count; ++i)
		{
			// sum up face verts and indices
			drawablep->updateFaceSize(i);
			LLFace* facep = drawablep->getFace(i);
			if (!facep) continue;
			if (facep->hasGeometry() && facep->getPixelArea() > FORCE_CULL_AREA)
			{
				vertex_count += facep->getGeomCount();
				index_count += facep->getIndicesCount();

				// remember face (for sorting)
				mFaceList.push_back(facep);
			}
			else
			{
				facep->clearVertexBuffer();
			}
		}
	}

	group->mBufferUsage = usage;
}

LLHUDPartition::LLHUDPartition()
{
	mPartitionType = LLViewerRegion::PARTITION_HUD;
	mDrawableType = LLPipeline::RENDER_TYPE_HUD;
	mSlopRatio = 0.f;
	mLODPeriod = 1;
}

void LLHUDPartition::shift(const LLVector4a &offset)
{
	//HUD objects don't shift with region crossing.  That would be silly.
}
