/** 
 * @file llprefsmedia.cpp
 * @brief Media preference implementation
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

// file include
#include "llprefsmedia.h"

// linden library includes
#include "llaudioengine.h"
#include "llcheckboxctrl.h"
#include "lluictrlfactory.h"

// project includes
#include "llpanelaudiovolume.h"
#include "llviewercontrol.h"

//static
void* LLPrefsMedia::createVolumePanel(void* data)
{
	LLPanelAudioVolume* panel = new LLPanelAudioVolume();
	return panel;
}

LLPrefsMedia::LLPrefsMedia()
{
	mFactoryMap["Volume Panel"]	= LLCallbackMap(createVolumePanel, NULL);
	LLUICtrlFactory::getInstance()->buildPanel(this,
											   "panel_preferences_media.xml",
											   &getFactoryMap());
}

BOOL LLPrefsMedia::postBuild()
{
	childSetCommitCallback("streaming_music", onCommitCheckBoxAudio, this);
	childSetCommitCallback("streaming_video", onCommitCheckBoxMedia, this);
	childSetCommitCallback("media_enable_filter", onCommitCheckBoxFilter, this);

	refreshValues(); // initialize member data from saved settings

	// Disable sub-settings check boxes when needed
	mRunningFMOD = gAudiop && gAudiop->getDriverName(false) == "FMOD";
	childSetEnabled("notify_stream_changes",
					mRunningFMOD && mPreviousStreamingMusic);
	if (!mRunningFMOD)
	{
		gSavedSettings.setBOOL("NotifyStreamChanges", FALSE);
	}
	childSetEnabled("auto_streaming_video", mPreviousStreamingVideo);
	childSetEnabled("media_hud", mPreviousStreamingVideo);
	if (!mPreviousStreamingVideo)
	{
		gSavedSettings.setBOOL("ParcelMediaAutoPlayEnable", FALSE);
		mPreviousParcelMediaAutoPlay = FALSE;
	}
	childSetEnabled("media_lookup_ip", mPreviousMediaEnableFilter);

	return TRUE;
}

void LLPrefsMedia::refreshValues()
{
	mPreviousVolume					= gSavedSettings.getF32("AudioLevelMaster");
	mPreviousSFX					= gSavedSettings.getF32("AudioLevelSFX");
	mPreviousUI						= gSavedSettings.getF32("AudioLevelUI");
	mPreviousEnvironment			= gSavedSettings.getF32("AudioLevelAmbient");
	mPreviousMusicVolume			= gSavedSettings.getF32("AudioLevelMusic");
	mPreviousMediaVolume			= gSavedSettings.getF32("AudioLevelMedia");
	mPreviousDoppler				= gSavedSettings.getF32("AudioLevelDoppler");
	mPreviousRolloff				= gSavedSettings.getF32("AudioLevelRolloff");

	mPreviousStreamingMusic			= gSavedSettings.getBOOL("AudioStreamingMusic");
	mPreviousNotifyStreamChanges	= gSavedSettings.getBOOL("NotifyStreamChanges");
	mPreviousStreamingVideo			= gSavedSettings.getBOOL("AudioStreamingVideo");
	mPreviousParcelMediaAutoPlay	= gSavedSettings.getBOOL("ParcelMediaAutoPlayEnable");
	mPreviousMediaOnAPrimUI			= gSavedSettings.getBOOL("MediaOnAPrimUI");
	mPreviousMediaEnableFilter		= gSavedSettings.getBOOL("MediaEnableFilter");
	mPreviousMediaLookupIP			= gSavedSettings.getBOOL("MediaLookupIP");

	mPreviousMuteAudio				= gSavedSettings.getBOOL("MuteAudio");
	mPreviousMuteWhenMinimized		= gSavedSettings.getBOOL("MuteWhenMinimized");
	mPreviousEnableGestureSounds	= gSavedSettings.getBOOL("EnableGestureSounds");
}

void LLPrefsMedia::cancel()
{
	gSavedSettings.setF32("AudioLevelMaster",			mPreviousVolume);
	gSavedSettings.setF32("AudioLevelUI",				mPreviousUI);
	gSavedSettings.setF32("AudioLevelSFX",				mPreviousSFX);
	gSavedSettings.setF32("AudioLevelAmbient",			mPreviousEnvironment);
	gSavedSettings.setF32("AudioLevelMusic",			mPreviousMusicVolume);
	gSavedSettings.setF32("AudioLevelMedia",			mPreviousMediaVolume);
	gSavedSettings.setF32("AudioLevelDoppler",			mPreviousDoppler);
	gSavedSettings.setF32("AudioLevelRolloff",			mPreviousRolloff);

	gSavedSettings.setBOOL("AudioStreamingMusic",		mPreviousStreamingMusic);
	gSavedSettings.setBOOL("NotifyStreamChanges",		mPreviousNotifyStreamChanges);
	gSavedSettings.setBOOL("AudioStreamingVideo",		mPreviousStreamingVideo);
	gSavedSettings.setBOOL("ParcelMediaAutoPlayEnable",	mPreviousParcelMediaAutoPlay);
	gSavedSettings.setBOOL("MediaOnAPrimUI",			mPreviousMediaOnAPrimUI);
	gSavedSettings.setBOOL("MediaEnableFilter",			mPreviousMediaEnableFilter);
	gSavedSettings.setBOOL("MediaLookupIP",				mPreviousMediaLookupIP);

	gSavedSettings.setBOOL("MuteAudio",					mPreviousMuteAudio);
	gSavedSettings.setBOOL("MuteWhenMinimized",			mPreviousMuteWhenMinimized);
	gSavedSettings.setBOOL("EnableGestureSounds",		mPreviousEnableGestureSounds);
}

//static
void LLPrefsMedia::onCommitCheckBoxMedia(LLUICtrl* ctrl, void* user_data)
{
	LLPrefsMedia* self = (LLPrefsMedia*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;

	bool enabled = check->get();
	if (!enabled)
	{
		gSavedSettings.setBOOL("ParcelMediaAutoPlayEnable", FALSE);
	}
	self->childSetEnabled("auto_streaming_video", enabled);
	self->childSetEnabled("media_hud", enabled);
}

//static
void LLPrefsMedia::onCommitCheckBoxAudio(LLUICtrl* ctrl, void* user_data)
{
	LLPrefsMedia* self = (LLPrefsMedia*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;

	self->childSetEnabled("notify_stream_changes",
						  self->mRunningFMOD && check->get());
}

//static
void LLPrefsMedia::onCommitCheckBoxFilter(LLUICtrl* ctrl, void* user_data)
{
	LLPrefsMedia* self = (LLPrefsMedia*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (self && check)
	{
		self->childSetEnabled("media_lookup_ip", check->get());
	}
}
