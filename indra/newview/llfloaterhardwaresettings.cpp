/** 
 * @file llfloaterhardwaresettings.cpp
 * @brief Menu of all the different graphics hardware settings
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

#include "llfloaterhardwaresettings.h"

#include "llnotifications.h"
#include "lluictrlfactory.h"

#include "llfeaturemanager.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "pipeline.h"

LLFloaterHardwareSettings* LLFloaterHardwareSettings::sHardwareSettings = NULL;
BOOL LLFloaterHardwareSettings::sUseStreamVBOexists = FALSE;

LLFloaterHardwareSettings::LLFloaterHardwareSettings()
:	LLFloater(std::string("Hardware Settings Floater"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_hardware_settings.xml");
}

LLFloaterHardwareSettings::~LLFloaterHardwareSettings()
{
}

void LLFloaterHardwareSettings::onCommitCheckBoxVBO(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterHardwareSettings* self = (LLFloaterHardwareSettings*)user_data;
	if (self && sUseStreamVBOexists)
	{
		gSavedSettings.setBOOL("RenderUseStreamVBO",
							   self->childGetValue("stream_vbo").asBoolean());
		self->refreshEnabledState();
	}
}

// menu maintenance functions

void LLFloaterHardwareSettings::refresh()
{
	LLPanel::refresh();

	mUseVBO = gSavedSettings.getBOOL("RenderVBOEnable");
	if (sUseStreamVBOexists)
	{
		mUseStreamVBO = gSavedSettings.getBOOL("RenderUseStreamVBO");
	}
	mUseAniso = gSavedSettings.getBOOL("RenderAnisotropic");
	mFSAASamples = gSavedSettings.getU32("RenderFSAASamples");
	mGamma = gSavedSettings.getF32("RenderGamma");
	mVideoCardMem = gSavedSettings.getS32("TextureMemory");
	mFogRatio = gSavedSettings.getF32("RenderFogRatio");
	mProbeHardwareOnStartup = gSavedSettings.getBOOL("ProbeHardwareOnStartup");

	childSetValue("fsaa", (LLSD::Integer) mFSAASamples);
	refreshEnabledState();
}

void LLFloaterHardwareSettings::refreshEnabledState()
{
	S32 min_tex_mem = LLViewerTextureList::getMinVideoRamSetting();
	S32 max_tex_mem = LLViewerTextureList::getMaxVideoRamSetting();
	childSetMinValue("GrapicsCardTextureMemory", min_tex_mem);
	childSetMaxValue("GrapicsCardTextureMemory", max_tex_mem);

	bool vbo_ok = true;
	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderVBOEnable") ||
		!gGLManager.mHasVertexBufferObject)
	{
		childSetEnabled("vbo", false);
		vbo_ok = false;
	}

	if (sUseStreamVBOexists)
	{
		childSetVisible("stream_vbo", true);
		childSetEnabled("stream_vbo", vbo_ok && childGetValue("vbo") &&
						LLFeatureManager::getInstance()->isFeatureAvailable("RenderUseStreamVBO"));
	}
	else
	{
		childSetVisible("stream_vbo", false);
	}

	if (!LLFeatureManager::getInstance()->isFeatureAvailable("UseOcclusion"))
	{
		childSetEnabled("occlusion", false);
	}

	// if no windlight shaders, turn off nighttime brightness, gamma, and fog distance
	childSetEnabled("gamma", !gPipeline.canUseWindLightShaders());
	childSetEnabled("(brightness, lower is brighter)", !gPipeline.canUseWindLightShaders());
	childSetEnabled("fog", !gPipeline.canUseWindLightShaders());
	childSetVisible("note", gPipeline.canUseWindLightShaders());
}

// static instance of it
LLFloaterHardwareSettings* LLFloaterHardwareSettings::instance()
{
	if (!sHardwareSettings)
	{
		sHardwareSettings = new LLFloaterHardwareSettings();
		sHardwareSettings->close();
	}
	return sHardwareSettings;
}
void LLFloaterHardwareSettings::show()
{
	LLFloaterHardwareSettings* hardSettings = instance();
	hardSettings->refresh();
	hardSettings->center();
	hardSettings->open();
}

bool LLFloaterHardwareSettings::isOpen()
{
	if (sHardwareSettings != NULL) 
	{
		return true;
	}
	return false;
}

// virtual
void LLFloaterHardwareSettings::onClose(bool app_quitting)
{
	if (sHardwareSettings)
	{
		sHardwareSettings->setVisible(FALSE);
	}
}

//============================================================================

BOOL LLFloaterHardwareSettings::postBuild()
{
	childSetAction("OK", onBtnOK, this);

	sUseStreamVBOexists = gSavedSettings.controlExists("RenderUseStreamVBO");
	if (sUseStreamVBOexists)
	{
		childSetCommitCallback("vbo", onCommitCheckBoxVBO, this);
		childSetCommitCallback("stream_vbo", onCommitCheckBoxVBO, this);
	}

	refresh();

	return TRUE;
}

void LLFloaterHardwareSettings::apply()
{
	BOOL logged_in = (LLStartUp::getStartupState() >= STATE_STARTED);

	if (gSavedSettings.getU32("RenderFSAASamples") != mFSAASamples)
#if 0	// Disabled because it causes font corruption when changed during the session
	{
		LLWindow* window = gViewerWindow->getWindow();
		LLCoordScreen size;
		window->getSize(&size);
		gViewerWindow->changeDisplaySettings(window->getFullscreen(), 
											 size,
											 gSavedSettings.getBOOL("DisableVerticalSync"),
											 logged_in);
		gViewerWindow->restartDisplay(logged_in);
	}
	else
#else
	{
		LLNotifications::instance().add("InEffectAfterRestart");
	}
#endif
	if (gSavedSettings.getBOOL("RenderAnisotropic") != mUseAniso)
	{
		gViewerWindow->restartDisplay(logged_in);
	}

	refresh();
}

void LLFloaterHardwareSettings::cancel()
{
	gSavedSettings.setBOOL("RenderVBOEnable", mUseVBO);
	if (sUseStreamVBOexists)
	{
		gSavedSettings.setBOOL("RenderUseStreamVBO", mUseStreamVBO);
	}
	gSavedSettings.setBOOL("RenderAnisotropic", mUseAniso);
	gSavedSettings.setU32("RenderFSAASamples", mFSAASamples);
	gSavedSettings.setF32("RenderGamma", mGamma);
	gSavedSettings.setS32("TextureMemory", mVideoCardMem);
	gSavedSettings.setF32("RenderFogRatio", mFogRatio);
	gSavedSettings.setBOOL("ProbeHardwareOnStartup", mProbeHardwareOnStartup);

	close();
}

// static 
void LLFloaterHardwareSettings::onBtnOK(void* userdata)
{
	LLFloaterHardwareSettings *fp =(LLFloaterHardwareSettings *)userdata;
	fp->apply();
	fp->close(false);
}
