/** 
 * @file llviewercontrol.cpp
 * @brief Viewer configuration
 * @author Richard Nelson
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

#include "llviewerprecompiledheaders.h"

#include "llviewercontrol.h"

#include "indra_constants.h"
#include "llaudioengine.h"
#include "llavatarnamecache.h"
#include "llerrorcontrol.h"
#include "llfloater.h"
#include "llkeyboard.h"
#include "llparcel.h"
#include "llrender.h"
#include "llspellcheck.h"
#include "llsys.h"
#include "llversionviewer.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llcallingcard.h"
#include "llchatbar.h"
#include "llconsole.h"
#include "lldirpicker.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolterrain.h"
#include "llfeaturemanager.h"
#include "llfilepicker.h"
#include "llflexibleobject.h"
#include "llfloaterchat.h"
#include "llmeshrepository.h"
#include "llnetmap.h"
#include "llsky.h"
#include "lltoolbar.h"
#include "lltracker.h"
#include "lltranslate.h"
#include "llvieweraudio.h"
#include "llviewerjoystick.h"
#include "llviewermenu.h"
#include "llviewermenufile.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoiceclient.h"
#include "llvosky.h"
#include "llvosurfacepatch.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llvowlsky.h"
#include "llworld.h"
#include "llworldmapview.h"
#include "pipeline.h"

std::map<std::string, LLControlGroup*> gSettings;
LLControlGroup gSavedSettings("Global");				// saved at end of session
LLControlGroup gSavedPerAccountSettings("PerAccount");	// saved at end of session
LLControlGroup gColors("Colors");						// read-only
LLControlGroup gCrashSettings("CrashSettings");			// saved at end of session

std::string gLastRunVersion;
std::string gCurrentVersion;

extern BOOL gResizeScreenTexture;
extern BOOL gDebugGL;
extern BOOL gAuditTexture;
extern BOOL gAnimateTextures;
extern BOOL gPingInterpolate;
extern BOOL gVelocityInterpolate;
extern bool gUpdateDrawDistance;

////////////////////////////////////////////////////////////////////////////
// Listeners


static bool handleAllowSwappingChanged(const LLSD& newvalue)
{
	LLMemoryInfo::setAllowSwapping(newvalue.asBoolean());
	return true;
}

//MK
static bool handleRestrainedLoveDebugChanged(const LLSD& newvalue)
{
	RRInterface::sRestrainedLoveDebug = newvalue.asBoolean();
	return true;
}
//mk

static bool handleRenderAvatarMouselookChanged(const LLSD& newvalue)
{
	LLVOAvatar::sVisibleInFirstPerson = newvalue.asBoolean();
	return true;
}

static bool handleRenderFarClipChanged(const LLSD& newvalue)
{
	gUpdateDrawDistance = true;	// updated in llviewerdisplay.cpp
	return true;
}

static bool handleTerrainDetailChanged(const LLSD& newvalue)
{
	LLDrawPoolTerrain::sDetailMode = newvalue.asInteger();
	return true;
}


static bool handleSetShaderChanged(const LLSD& newvalue)
{
	// changing shader level may invalidate existing cached bump maps, as the shader type determines the format of the bump map it expects - clear and repopulate the bump cache
	gBumpImageList.destroyGL();
	gBumpImageList.restoreGL();

	// Enabling shader also changes the terrain detail to high, reflect that change here
	if (gSavedSettings.getBOOL("VertexShaderEnable"))
	{
		// shaders enabled, set terrain detail to high
		gSavedSettings.setS32("RenderTerrainDetail", 1);
	}
	// else, leave terrain detail as is
	LLViewerShaderMgr::instance()->setShaders();
	return true;
}

static bool handleAvatarPhysicsLODChanged(const LLSD& newvalue)
{
	LLVOAvatar::sPhysicsLODFactor = (F32) newvalue.asReal();
	return true;
}

static bool handleAvatarPhysicsChanged(const LLSD& newvalue)
{
	LLVOAvatar::sAvatarPhysics = newvalue.asBoolean();
	gAgent.sendAgentSetAppearance();
	return true;
}

static bool handleRenderNameChanged(const LLSD& newvalue)
{
	LLVOAvatar::sRenderName = newvalue.asInteger();
	return true;
}

static bool handleRenderHideGroupTitleAllChanged(const LLSD& newvalue)
{
	LLVOAvatar::sRenderGroupTitles = !newvalue.asBoolean();
	return true;
}

bool handleRenderTransparentWaterChanged(const LLSD& newvalue)
{
	LLWorld::getInstance()->updateWaterObjects();
	return true;
}

static bool handleMeshMaxConcurrentRequestsChanged(const LLSD& newvalue)
{
	LLMeshRepoThread::sMaxConcurrentRequests = (U32) newvalue.asInteger();
	return true;
}

static bool handleReleaseGLBufferChanged(const LLSD& newvalue)
{
	// Disabled because it causes font corruption when changed during the session
	//LLPipeline::sRenderFSAASamples = gSavedSettings.getU32("RenderFSAASamples");
	if (gPipeline.isInit())
	{
		gPipeline.releaseGLBuffers();
		gPipeline.createGLBuffers();
	}
	return true;
}

static bool handleAnisotropicChanged(const LLSD& newvalue)
{
	LLImageGL::sGlobalUseAnisotropic = newvalue.asBoolean();
	LLImageGL::dirtyTexOptions();
	return true;
}

static bool handleVolumeLODChanged(const LLSD& newvalue)
{
	LLVOVolume::sLODFactor = (F32) newvalue.asReal();
	LLVOVolume::sDistanceFactor = 1.f-LLVOVolume::sLODFactor * 0.1f;
	return true;
}

static bool handleAvatarLODChanged(const LLSD& newvalue)
{
	LLVOAvatar::sLODFactor = (F32) newvalue.asReal();
	return true;
}

static bool handleAvatarMaxVisibleChanged(const LLSD& newvalue)
{
	LLVOAvatar::sMaxVisible = (U32) newvalue.asInteger();
	return true;
}

static bool handleTerrainLODChanged(const LLSD& newvalue)
{
		LLVOSurfacePatch::sLODFactor = (F32)newvalue.asReal();
		//sqaure lod factor to get exponential range of [0,4] and keep
		//a value of 1 in the middle of the detail slider for consistency
		//with other detail sliders (see panel_preferences_graphics1.xml)
		LLVOSurfacePatch::sLODFactor *= LLVOSurfacePatch::sLODFactor;
		return true;
}

static bool handleTreeLODChanged(const LLSD& newvalue)
{
	LLVOTree::sTreeFactor = (F32) newvalue.asReal();
	return true;
}

static bool handleFlexLODChanged(const LLSD& newvalue)
{
	LLVolumeImplFlexible::sUpdateFactor = (F32) newvalue.asReal();
	return true;
}

static bool handleGammaChanged(const LLSD& newvalue)
{
	F32 gamma = (F32) newvalue.asReal();
	if (gamma == 0.0f)
	{
		gamma = 1.0f; // restore normal gamma
	}
	if (gViewerWindow && gViewerWindow->getWindow() && gamma != gViewerWindow->getWindow()->getGamma())
	{
		// Only save it if it's changed
		if (!gViewerWindow->getWindow()->setGamma(gamma))
		{
			llwarns << "setGamma failed!" << llendl;
		}
	}

	return true;
}

const F32 MAX_USER_FOG_RATIO = 10.f;
const F32 MIN_USER_FOG_RATIO = 0.5f;

static bool handleFogRatioChanged(const LLSD& newvalue)
{
	F32 fog_ratio = llmax(MIN_USER_FOG_RATIO, llmin((F32) newvalue.asReal(), MAX_USER_FOG_RATIO));
	gSky.setFogRatio(fog_ratio);
	return true;
}

static bool handleMaxPartCountChanged(const LLSD& newvalue)
{
	LLViewerPartSim::setMaxPartCount(newvalue.asInteger());
	return true;
}

static bool handleVideoMemoryChanged(const LLSD& newvalue)
{
	gTextureList.updateMaxResidentTexMem(newvalue.asInteger());
	return true;
}

static bool handleBandwidthChanged(const LLSD& newvalue)
{
	gViewerThrottle.setMaxBandwidth((F32) newvalue.asReal());
	return true;
}

static bool handleChatFontSizeChanged(const LLSD& newvalue)
{
	if(gConsole)
	{
		gConsole->setFontSize(newvalue.asInteger());
	}
	return true;
}

static bool handleChatPersistTimeChanged(const LLSD& newvalue)
{
	if(gConsole)
	{
		gConsole->setLinePersistTime((F32) newvalue.asReal());
	}
	return true;
}

static void handleAudioVolumeChanged(const LLSD& newvalue)
{
	audio_update_volume(true);
}

static bool handleStackMinimizedTopToBottom(const LLSD& newvalue)
{
	LLFloaterView::setStackMinimizedTopToBottom(newvalue.asBoolean());
	return true;
}

static bool handleStackMinimizedRightToLeft(const LLSD& newvalue)
{
	LLFloaterView::setStackMinimizedRightToLeft(newvalue.asBoolean());
	return true;
}

static bool handleStackScreenWidthFraction(const LLSD& newvalue)
{
	LLFloaterView::setStackScreenWidthFraction(newvalue.asInteger());
	return true;
}

static bool handleJoystickChanged(const LLSD& newvalue)
{
	LLViewerJoystick::getInstance()->setCameraNeedsUpdate(TRUE);
	return true;
}

static bool handleAvatarOffsetChanged(const LLSD& newvalue)
{
	gAgent.sendAgentSetAppearance();
	return true;
}

static bool handleCameraCollisionsChanged(const LLSD& newvalue)
{
	if (newvalue.asBoolean())
	{
		gAgent.setCameraCollidePlane(LLVector4(0.f, 0.f, 0.f, 1.f));
	}
	return true;
}

static bool handleCameraChanged(const LLSD& newvalue)
{
	gAgent.setupCameraView();
	return true;
}

static bool handleAudioStreamMusicChanged(const LLSD& newvalue)
{
	if (gAudiop)
	{
		if ( newvalue.asBoolean() )
		{
			if (LLViewerParcelMgr::getInstance()->getAgentParcel()
				&& !LLViewerParcelMgr::getInstance()->getAgentParcel()->getMusicURL().empty())
			{
				// if stream is already playing, don't call this
				// otherwise music will briefly stop
				if ( !gAudiop->isInternetStreamPlaying() )
				{
					LLViewerParcelMedia::playStreamingMusic(LLViewerParcelMgr::getInstance()->getAgentParcel());
				}
			}
		}
		else
		{
			gAudiop->stopInternetStream();
		}
	}
	return true;
}

static bool handleUseOcclusionChanged(const LLSD& newvalue)
{
	LLPipeline::sUseOcclusion = (newvalue.asBoolean() && gGLManager.mHasOcclusionQuery 
		&& LLFeatureManager::getInstance()->isFeatureAvailable("UseOcclusion") && !gUseWireframe) ? 2 : 0;
	return true;
}

static bool handleUploadBakedTexOldChanged(const LLSD& newvalue)
{
	LLPipeline::sForceOldBakedUpload = newvalue.asBoolean();
	return true;
}


static bool handleNumpadControlChanged(const LLSD& newvalue)
{
	if (gKeyboard)
	{
		gKeyboard->setNumpadDistinct(static_cast<LLKeyboard::e_numpad_distinct>(newvalue.asInteger()));
	}
	return true;
}

static bool handleWLSkyDetailChanged(const LLSD& newvalue)
{
	LLVOWLSky::sWLSkyDetail = llclamp((U32)newvalue.asInteger(), LLVOWLSky::MIN_SKY_DETAIL, LLVOWLSky::MAX_SKY_DETAIL);
	if (gSky.mVOWLSkyp.notNull())
	{
		gSky.mVOWLSkyp->updateGeometry(gSky.mVOWLSkyp->mDrawable);
	}
	return true;
}

static bool handleResetVertexBuffersChanged(const LLSD&)
{
	if (gPipeline.isInit())
	{
		gPipeline.resetVertexBuffers();
	}
	LLVOTree::sRenderAnimateTrees = gSavedSettings.getBOOL("RenderAnimateTrees");

	return true;
}

static bool handleAnimateTexturesChanged(const LLSD& newvalue)
{
	gAnimateTextures = newvalue.asBoolean();
	return true;
}

static bool handlePingInterpolateChanged(const LLSD& newvalue)
{
	gPingInterpolate = newvalue.asBoolean();
	return true;
}

static bool handleVelocityInterpolateChanged(const LLSD& newvalue)
{
	gVelocityInterpolate = newvalue.asBoolean();
	return true;
}

static bool handleRepartition(const LLSD&)
{
	if (gPipeline.isInit())
	{
		gOctreeMaxCapacity = gSavedSettings.getU32("OctreeMaxNodeCapacity");
		gObjectList.repartitionObjects();
	}
	return true;
}

static bool handleRenderDynamicLODChanged(const LLSD& newvalue)
{
	LLPipeline::sDynamicLOD = newvalue.asBoolean();
	return true;
}

static bool handleRenderLightingDetailChanged(const LLSD& newvalue)
{
	gPipeline.setLightingDetail(-1);
	return true;
}

static bool handleRenderDeferredChanged(const LLSD& newvalue)
{
	LLRenderTarget::sUseFBO = newvalue.asBoolean();
	if (gPipeline.isInit())
	{
		gPipeline.updateRenderDeferred();
		gPipeline.releaseGLBuffers();
		gPipeline.createGLBuffers();
		gPipeline.resetVertexBuffers();
		if (LLPipeline::sRenderDeferred && LLRenderTarget::sUseFBO)
		{
			LLViewerShaderMgr::instance()->setShaders();
		}
	}
	return true;
}

static bool handleRenderUseImpostorsChanged(const LLSD& newvalue)
{
	LLVOAvatar::sUseImpostors = newvalue.asBoolean();
	return true;
}

static bool handleAuditTextureChanged(const LLSD& newvalue)
{
	gAuditTexture = newvalue.asBoolean();
	return true;
}

static bool handleDisplayNamesUsageChanged(const LLSD& newvalue)
{
	LLAvatarNameCache::setUseDisplayNames((U32)newvalue.asInteger());
	LLVOAvatar::invalidateNameTags();
	LLAvatarTracker::instance().dirtyBuddies();
	return true;
}

static bool handleOmitResidentAsLastNameChanged(const LLSD& newvalue)
{
	LLAvatarName::sOmitResidentAsLastName =(bool)newvalue.asBoolean();
	LLVOAvatar::invalidateNameTags();
	LLAvatarTracker::instance().dirtyBuddies();
	return true;
}

static bool handleLegacyNamesForFriendsChanged(const LLSD& newvalue)
{
	LLAvatarTracker::instance().dirtyBuddies();
	return true;
}

static bool handleRenderDebugGLChanged(const LLSD& newvalue)
{
	gDebugGL = newvalue.asBoolean();
	gGL.clearErrors();
	return true;
}

static bool handleRenderDebugPipelineChanged(const LLSD& newvalue)
{
	gDebugPipeline = newvalue.asBoolean();
	return true;
}

static bool handleRenderResolutionDivisorChanged(const LLSD&)
{
	gResizeScreenTexture = TRUE;
	return true;
}

static bool handleRenderVertexBufferParamsChanged(const LLSD&)
{
	if (gPipeline.isInit())
	{
		gPipeline.resetVertexBuffers();
	}
	return true;
}

static bool handleDebugViewsChanged(const LLSD& newvalue)
{
	LLView::sDebugRects = newvalue.asBoolean();
	return true;
}

static bool handleLogFileChanged(const LLSD& newvalue)
{
	std::string log_filename = newvalue.asString();
	LLFile::remove(log_filename);
	LLError::logToFile(log_filename);
	return true;
}

bool handleHideGroupTitleChanged(const LLSD& newvalue)
{
	gAgent.setHideGroupTitle(newvalue);
	return true;
}

bool handleEffectColorChanged(const LLSD& newvalue)
{
	gAgent.setEffectColor(LLColor4(newvalue));
	return true;
}

bool handleVoiceClientPrefsChanged(const LLSD& newvalue)
{
	if (LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->updateSettings();
	}
	return true;
}

#if TRANSLATE_CHAT
bool handleTranslateChatPrefsChanged(const LLSD& newvalue)
{
	LLFloaterChat* floaterp = LLFloaterChat::getInstance();

	if(floaterp)
	{
		// update "translate chat" pref in "Local Chat" floater
		floaterp->updateSettings();
	}
	return true;
}
#endif

static bool handleMiniMapCenterChanged(const LLSD& newvalue)
{
	LLNetMap::sMiniMapCenter = newvalue.asInteger();
	return true;
}

static bool handleMiniMapRotateChanged(const LLSD& newvalue)
{
	LLNetMap::sMiniMapRotate = newvalue.asBoolean();
	return true;
}

static bool handleUseOldTrackingDotsChanged(const LLSD& newvalue)
{
	LLWorldMapView::sUseOldTrackingDots = newvalue.asBoolean();
	return true;
}

static bool handleToolbarButtonsChanged(const LLSD&)
{
	if (gToolBar)
	{
		gToolBar->layoutButtons();
	}
	return true;
}

static bool handleCheesyBeaconChanged(const LLSD& newvalue)
{
	LLTracker::sCheesyBeacon = newvalue.asBoolean();
	return true;
}

static bool handleUseDebugMenusChanged(const LLSD& newvalue)
{
	if (gMenuBarView)
	{
		show_debug_menus();
	}
	return true;
}

static bool handleNonBlockingFilePickerChanged(const LLSD& newvalue)
{
	LLTracker::sCheesyBeacon = newvalue.asBoolean();
#if !LL_DARWIN
	bool blocking = (newvalue.asBoolean() == FALSE);
	LLFilePickerThread::setBlocking(blocking);
	LLDirPickerThread::setBlocking(blocking);
#endif
	return true;
}

static bool handleSpellCheckChanged(const LLSD& newvalue)
{
	LLSpellCheck::instance().setSpellCheck(gSavedSettings.getBOOL("SpellCheck"));
	LLSpellCheck::instance().setShowMisspelled(gSavedSettings.getBOOL("SpellCheckShow"));
	LLSpellCheck::instance().setDictionary(gSavedSettings.getString("SpellCheckLanguage"));
	return true;
}

static bool handleSwapShoutWhisperShortcutsChanged(const LLSD& newvalue)
{
	LLChatBar::sSwappedShortcuts = newvalue.asBoolean();
	return true;
}

////////////////////////////////////////////////////////////////////////////

void settings_setup_listeners()
{
	gSavedSettings.getControl("AllowSwapping")->getSignal()->connect(boost::bind(&handleAllowSwappingChanged, _2));
	gSavedSettings.getControl("RestrainedLoveDebug")->getSignal()->connect(boost::bind(&handleRestrainedLoveDebugChanged, _2));
	gSavedSettings.getControl("FirstPersonAvatarVisible")->getSignal()->connect(boost::bind(&handleRenderAvatarMouselookChanged, _2));
	gSavedSettings.getControl("RenderFarClip")->getSignal()->connect(boost::bind(&handleRenderFarClipChanged, _2));
	gSavedSettings.getControl("RenderTerrainDetail")->getSignal()->connect(boost::bind(&handleTerrainDetailChanged, _2));
	gSavedSettings.getControl("OctreeStaticObjectSizeFactor")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("OctreeDistanceFactor")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("OctreeMaxNodeCapacity")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("OctreeAlphaDistanceFactor")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("OctreeAttachmentSizeFactor")->getSignal()->connect(boost::bind(&handleRepartition, _2));
	gSavedSettings.getControl("RenderUseTriStrips")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAnimateTrees")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAvatarVP")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("VertexShaderEnable")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderSpecularResX")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderSpecularResY")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderSpecularExponent")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
//	gSavedSettings.getControl("RenderFSAASamples")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderAnisotropic")->getSignal()->connect(boost::bind(&handleAnisotropicChanged, _2));
	gSavedSettings.getControl("RenderShadowResolutionScale")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderGlow")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderGlow")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("EnableRippleWater")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderGlowResolutionPow")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderAvatarCloth")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("WindLightUseAtmosShaders")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderGammaFull")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderAvatarMaxVisible")->getSignal()->connect(boost::bind(&handleAvatarMaxVisibleChanged, _2));
	gSavedSettings.getControl("RenderName")->getSignal()->connect(boost::bind(&handleRenderNameChanged, _2));
	gSavedSettings.getControl("RenderHideGroupTitleAll")->getSignal()->connect(boost::bind(&handleRenderHideGroupTitleAllChanged, _2));
	gSavedSettings.getControl("RenderVolumeLODFactor")->getSignal()->connect(boost::bind(&handleVolumeLODChanged, _2));
	gSavedSettings.getControl("RenderAvatarLODFactor")->getSignal()->connect(boost::bind(&handleAvatarLODChanged, _2));
	gSavedSettings.getControl("RenderAvatarPhysicsLODFactor")->getSignal()->connect(boost::bind(&handleAvatarPhysicsLODChanged, _2));
	gSavedSettings.getControl("RenderTerrainLODFactor")->getSignal()->connect(boost::bind(&handleTerrainLODChanged, _2));
	gSavedSettings.getControl("RenderTreeLODFactor")->getSignal()->connect(boost::bind(&handleTreeLODChanged, _2));
	gSavedSettings.getControl("RenderFlexTimeFactor")->getSignal()->connect(boost::bind(&handleFlexLODChanged, _2));
	gSavedSettings.getControl("ThrottleBandwidthKBPS")->getSignal()->connect(boost::bind(&handleBandwidthChanged, _2));
	gSavedSettings.getControl("RenderGamma")->getSignal()->connect(boost::bind(&handleGammaChanged, _2));
	gSavedSettings.getControl("RenderFogRatio")->getSignal()->connect(boost::bind(&handleFogRatioChanged, _2));
	gSavedSettings.getControl("RenderMaxPartCount")->getSignal()->connect(boost::bind(&handleMaxPartCountChanged, _2));
	gSavedSettings.getControl("RenderDynamicLOD")->getSignal()->connect(boost::bind(&handleRenderDynamicLODChanged, _2));
	gSavedSettings.getControl("RenderDebugTextureBind")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAutoMaskAlphaDeferred")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderAutoMaskAlphaNonDeferred")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("AnimateTextures")->getSignal()->connect(boost::bind(&handleAnimateTexturesChanged, _2));
	gSavedSettings.getControl("PingInterpolate")->getSignal()->connect(boost::bind(&handlePingInterpolateChanged, _2));
	gSavedSettings.getControl("VelocityInterpolate")->getSignal()->connect(boost::bind(&handleVelocityInterpolateChanged, _2));
	gSavedSettings.getControl("RenderObjectBump")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderMaxVBOSize")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderDeferredNoise")->getSignal()->connect(boost::bind(&handleReleaseGLBufferChanged, _2));
	gSavedSettings.getControl("RenderUseImpostors")->getSignal()->connect(boost::bind(&handleRenderUseImpostorsChanged, _2));
	gSavedSettings.getControl("RenderDebugGL")->getSignal()->connect(boost::bind(&handleRenderDebugGLChanged, _2));
	gSavedSettings.getControl("RenderDebugPipeline")->getSignal()->connect(boost::bind(&handleRenderDebugPipelineChanged, _2));
	gSavedSettings.getControl("RenderResolutionDivisor")->getSignal()->connect(boost::bind(&handleRenderResolutionDivisorChanged, _2));
	gSavedSettings.getControl("RenderDeferred")->getSignal()->connect(boost::bind(&handleRenderDeferredChanged, _2));
	gSavedSettings.getControl("RenderShadowDetail")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderDeferredSSAO")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderDeferredGI")->getSignal()->connect(boost::bind(&handleSetShaderChanged, _2));
	gSavedSettings.getControl("RenderTransparentWater")->getSignal()->connect(boost::bind(&handleRenderTransparentWaterChanged, _2));
	gSavedSettings.getControl("MeshMaxConcurrentRequests")->getSignal()->connect(boost::bind(&handleMeshMaxConcurrentRequestsChanged, _2));	
	gSavedSettings.getControl("TextureMemory")->getSignal()->connect(boost::bind(&handleVideoMemoryChanged, _2));
	gSavedSettings.getControl("AuditTexture")->getSignal()->connect(boost::bind(&handleAuditTextureChanged, _2));
	gSavedSettings.getControl("ChatFontSize")->getSignal()->connect(boost::bind(&handleChatFontSizeChanged, _2));
	gSavedSettings.getControl("ChatPersistTime")->getSignal()->connect(boost::bind(&handleChatPersistTimeChanged, _2));
	gSavedSettings.getControl("UploadBakedTexOld")->getSignal()->connect(boost::bind(&handleUploadBakedTexOldChanged, _2));
	gSavedSettings.getControl("UseOcclusion")->getSignal()->connect(boost::bind(&handleUseOcclusionChanged, _2));
	gSavedSettings.getControl("AudioLevelMaster")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelSFX")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelUI")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelAmbient")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelMusic")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelMedia")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelVoice")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelDoppler")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioLevelRolloff")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("AudioStreamingMusic")->getSignal()->connect(boost::bind(&handleAudioStreamMusicChanged, _2));
	gSavedSettings.getControl("DisplayNamesUsage")->getSignal()->connect(boost::bind(&handleDisplayNamesUsageChanged, _2));
	gSavedSettings.getControl("OmitResidentAsLastName")->getSignal()->connect(boost::bind(&handleOmitResidentAsLastNameChanged, _2));
	gSavedSettings.getControl("LegacyNamesForFriends")->getSignal()->connect(boost::bind(&handleLegacyNamesForFriendsChanged, _2));
	gSavedSettings.getControl("MuteAudio")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteMusic")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteMedia")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteVoice")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteAmbient")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("MuteUI")->getSignal()->connect(boost::bind(&handleAudioVolumeChanged, _2));
	gSavedSettings.getControl("RenderVBOEnable")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderVBOMappingDisable")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderUseStreamVBO")->getSignal()->connect(boost::bind(&handleRenderVertexBufferParamsChanged, _2));
	gSavedSettings.getControl("RenderPreferStreamDraw")->getSignal()->connect(boost::bind(&handleRenderVertexBufferParamsChanged, _2));
	gSavedSettings.getControl("RenderBakeSunlight")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("RenderNoAlpha")->getSignal()->connect(boost::bind(&handleResetVertexBuffersChanged, _2));
	gSavedSettings.getControl("WLSkyDetail")->getSignal()->connect(boost::bind(&handleWLSkyDetailChanged, _2));
	gSavedSettings.getControl("RenderLightingDetail")->getSignal()->connect(boost::bind(&handleRenderLightingDetailChanged, _2));
	gSavedSettings.getControl("NumpadControl")->getSignal()->connect(boost::bind(&handleNumpadControlChanged, _2));
	gSavedSettings.getControl("StackMinimizedTopToBottom")->getSignal()->connect(boost::bind(&handleStackMinimizedTopToBottom, _2));
	gSavedSettings.getControl("StackMinimizedRightToLeft")->getSignal()->connect(boost::bind(&handleStackMinimizedRightToLeft, _2));
	gSavedSettings.getControl("StackScreenWidthFraction")->getSignal()->connect(boost::bind(&handleStackScreenWidthFraction, _2));
	gSavedSettings.getControl("JoystickAxis0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("JoystickAxis6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("CameraIgnoreCollisions")->getSignal()->connect(boost::bind(&handleCameraCollisionsChanged, _2));
	gSavedSettings.getControl("CameraFrontView")->getSignal()->connect(boost::bind(&handleCameraChanged, _2));
	gSavedSettings.getControl("FocusOffsetDefault")->getSignal()->connect(boost::bind(&handleCameraChanged, _2));
	gSavedSettings.getControl("CameraOffsetDefault")->getSignal()->connect(boost::bind(&handleCameraChanged, _2));
	gSavedSettings.getControl("FocusOffsetFrontView")->getSignal()->connect(boost::bind(&handleCameraChanged, _2));
	gSavedSettings.getControl("CameraOffsetFrontView")->getSignal()->connect(boost::bind(&handleCameraChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisScale6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("FlycamAxisDeadZone6")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("AvatarOffsetX")->getSignal()->connect(boost::bind(&handleAvatarOffsetChanged, _2));
	gSavedSettings.getControl("AvatarOffsetY")->getSignal()->connect(boost::bind(&handleAvatarOffsetChanged, _2));
	gSavedSettings.getControl("AvatarOffsetZ")->getSignal()->connect(boost::bind(&handleAvatarOffsetChanged, _2));
	gSavedSettings.getControl("AvatarPhysics")->getSignal()->connect(boost::bind(&handleAvatarPhysicsChanged, _2));
	gSavedSettings.getControl("BuildAxisScale0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisScale5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone0")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone1")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone2")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone3")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone4")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("BuildAxisDeadZone5")->getSignal()->connect(boost::bind(&handleJoystickChanged, _2));
	gSavedSettings.getControl("DebugViews")->getSignal()->connect(boost::bind(&handleDebugViewsChanged, _2));
	gSavedSettings.getControl("UserLogFile")->getSignal()->connect(boost::bind(&handleLogFileChanged, _2));
	gSavedSettings.getControl("RenderHideGroupTitle")->getSignal()->connect(boost::bind(handleHideGroupTitleChanged, _2));
	gSavedSettings.getControl("EffectColor")->getSignal()->connect(boost::bind(handleEffectColorChanged, _2));
	gSavedSettings.getControl("EnableVoiceChat")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PTTCurrentlyEnabled")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PushToTalkButton")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("PushToTalkToggle")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceEarLocation")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceInputAudioDevice")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("VoiceOutputAudioDevice")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("AudioLevelMic")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));
	gSavedSettings.getControl("LipSyncEnabled")->getSignal()->connect(boost::bind(&handleVoiceClientPrefsChanged, _2));	
#if TRANSLATE_CHAT
	gSavedSettings.getControl("TranslateChat")->getSignal()->connect(boost::bind(&handleTranslateChatPrefsChanged, _2));
#endif
	gSavedSettings.getControl("MiniMapCenter")->getSignal()->connect(boost::bind(&handleMiniMapCenterChanged, _2));	
	gSavedSettings.getControl("MiniMapRotate")->getSignal()->connect(boost::bind(&handleMiniMapRotateChanged, _2));	
	gSavedSettings.getControl("UseOldTrackingDots")->getSignal()->connect(boost::bind(&handleUseOldTrackingDotsChanged, _2));	
	gSavedSettings.getControl("ShowChatButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowIMButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowFriendsButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowGroupsButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowFlyButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowSnapshotButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowSearchButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowBuildButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowRadarButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowMiniMapButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowMapButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("ShowInventoryButton")->getSignal()->connect(boost::bind(&handleToolbarButtonsChanged, _2));	
	gSavedSettings.getControl("CheesyBeacon")->getSignal()->connect(boost::bind(&handleCheesyBeaconChanged, _2));	
	gSavedSettings.getControl("UseDebugMenus")->getSignal()->connect(boost::bind(&handleUseDebugMenusChanged, _2));	
	gSavedSettings.getControl("NonBlockingFilePicker")->getSignal()->connect(boost::bind(&handleNonBlockingFilePickerChanged, _2));	
	gSavedSettings.getControl("SpellCheck")->getSignal()->connect(boost::bind(&handleSpellCheckChanged, _2));
	gSavedSettings.getControl("SpellCheckShow")->getSignal()->connect(boost::bind(&handleSpellCheckChanged, _2));
	gSavedSettings.getControl("SpellCheckLanguage")->getSignal()->connect(boost::bind(&handleSpellCheckChanged, _2));
	gSavedSettings.getControl("SwapShoutWhisperShortcuts")->getSignal()->connect(boost::bind(&handleSwapShoutWhisperShortcutsChanged, _2));
}
