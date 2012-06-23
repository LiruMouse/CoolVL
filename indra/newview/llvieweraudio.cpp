/** 
 * @file llvieweraudio.cpp
 * @brief Audio functions that used to be in viewer.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llvieweraudio.h"

#include "lldir.h"
#include "llassettype.h"
#include "llaudioengine.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerwindow.h"
#include "llvoiceclient.h"

/////////////////////////////////////////////////////////

void init_audio() 
{
	if (!gAudiop) 
	{
		llwarns << "Failed to create an appropriate Audio Engine" << llendl;
		return;
	}
	LLVector3d lpos_global = gAgent.getCameraPositionGlobal();
	LLVector3 lpos_global_f;

	lpos_global_f.setVec(lpos_global);

	gAudiop->setListener(lpos_global_f,
						  LLVector3::zero,	// LLViewerCamera::getInstance()->getVelocity(),    // !!! BUG need to replace this with smoothed velocity!
						  LLViewerCamera::getInstance()->getUpAxis(),
						  LLViewerCamera::getInstance()->getAtAxis());

	// load up our initial set of sounds so they are ready to be played
	if (gSavedSettings.getBOOL("NoPreload") == FALSE)
	{
		audio_preload_ui_sounds();
	}
	audio_update_volume(true);
}

void audio_preload_ui_sounds(bool force_decode)
{
	if (!gAudiop) 
	{
		llwarns << "Audio Engine not initialized. Could not preload the UI sounds." << llendl;
		return;
	}

	std::stack<std::string> ui_sounds;
	ui_sounds.push("UISndTeleportOut");
	ui_sounds.push("UISndStartIM");
	ui_sounds.push("UISndHealthReductionF");
	ui_sounds.push("UISndHealthReductionM");
	ui_sounds.push("UISndMoneyChangeDown");
	ui_sounds.push("UISndMoneyChangeUp");
	ui_sounds.push("UISndSnapshot");
	ui_sounds.push("UISndTyping");
	ui_sounds.push("UISndObjectCreate");
	ui_sounds.push("UISndObjectDelete");
	ui_sounds.push("UISndObjectRezIn");
	ui_sounds.push("UISndObjectRezOut");
	ui_sounds.push("UISndAlert");
	ui_sounds.push("UISndBadKeystroke");
	ui_sounds.push("UISndClick");
	ui_sounds.push("UISndClickRelease");
	ui_sounds.push("UISndInvalidOp");
	ui_sounds.push("UISndPieMenuAppear");
	ui_sounds.push("UISndPieMenuHide");
	ui_sounds.push("UISndPieMenuSliceHighlight0");
	ui_sounds.push("UISndPieMenuSliceHighlight1");
	ui_sounds.push("UISndPieMenuSliceHighlight2");
	ui_sounds.push("UISndPieMenuSliceHighlight3");
	ui_sounds.push("UISndPieMenuSliceHighlight4");
	ui_sounds.push("UISndPieMenuSliceHighlight5");
	ui_sounds.push("UISndPieMenuSliceHighlight6");
	ui_sounds.push("UISndPieMenuSliceHighlight7");
	ui_sounds.push("UISndWindowClose");
	ui_sounds.push("UISndWindowOpen");

	F32 audio_level = gSavedSettings.getF32("AudioLevelUI") *
					  gSavedSettings.getF32("AudioLevelMaster");
	if (!force_decode || audio_level == 0.0f ||
		gSavedSettings.getBOOL("MuteAudio") || gSavedSettings.getBOOL("MuteUI"))
	{
		audio_level = 0.0f;
		if (force_decode)
		{
			llwarns << "UI muted: cannot force-decode UI sounds." << llendl;
		}
	}
	else
	{
		// Normalize to 25% combined volume, or the highest possible volume
		// if 25% can't be reached.
		audio_level = 0.25f / audio_level;
		if (audio_level > 1.0f)
		{
			audio_level = 1.0f;
		}
	}

	LLUUID uuid;
	std::string uuid_str;
	std::string wav_path;
	while (!ui_sounds.empty())
 	{
		uuid = LLUUID(gSavedSettings.getString(ui_sounds.top()));
		uuid.toString(uuid_str);
		wav_path = gDirUtilp->getExpandedFilename(LL_PATH_SKINS, "default", "sounds", uuid_str) + ".dsf";
		if (!gDirUtilp->fileExists(wav_path))	// This sound is not part of the skin and must be fetched
		{
			// Make sure they are at least pre-fetched.
			gAudiop->preloadSound(uuid);
			if (audio_level > 0.0f)
			{
				// Try to force-decode them (will depend on actual audio level)
				// by playing them.
				gAudiop->triggerSound(uuid, gAgent.getID(), audio_level, LLAudioEngine::AUDIO_TYPE_UI);
			}
		}
		ui_sounds.pop();
	}
}

void audio_update_volume(bool force_update)
{
	static LLCachedControl<bool> sMuteAudio(gSavedSettings, "MuteAudio");
	static LLCachedControl<bool> sMuteAmbient(gSavedSettings, "MuteAmbient");
	static LLCachedControl<bool> sMuteSounds(gSavedSettings, "MuteSounds");
	static LLCachedControl<bool> sMuteUI(gSavedSettings, "MuteUI");
	static LLCachedControl<bool> sMuteMusic(gSavedSettings, "MuteMusic");
	static LLCachedControl<bool> sMuteMedia(gSavedSettings, "MuteMedia");
	static LLCachedControl<bool> sMuteVoice(gSavedSettings, "MuteVoice");
	static LLCachedControl<bool> sMuteWhenMinimized(gSavedSettings, "MuteWhenMinimized");
	static LLCachedControl<bool> sDisableWindAudio(gSavedSettings, "DisableWindAudio");
	static LLCachedControl<F32> sAudioLevelMaster(gSavedSettings, "AudioLevelMaster");
	static LLCachedControl<F32> sAudioLevelAmbient(gSavedSettings, "AudioLevelAmbient");
	static LLCachedControl<F32> sAudioLevelUI(gSavedSettings, "AudioLevelUI");
	static LLCachedControl<F32> sAudioLevelSFX(gSavedSettings, "AudioLevelSFX");
	static LLCachedControl<F32> sAudioLevelMusic(gSavedSettings, "AudioLevelMusic");
	static LLCachedControl<F32> sAudioLevelMedia(gSavedSettings, "AudioLevelMedia");
	static LLCachedControl<F32> sAudioLevelVoice(gSavedSettings, "AudioLevelVoice");
	static LLCachedControl<F32> sAudioLevelMic(gSavedSettings, "AudioLevelMic");
	static LLCachedControl<F32> sAudioLevelDoppler(gSavedSettings, "AudioLevelDoppler");
	static LLCachedControl<F32> sAudioLevelRolloff(gSavedSettings, "AudioLevelRolloff");

	bool mute = sMuteAudio;
	if (sMuteWhenMinimized && gViewerWindow && !gViewerWindow->getActive())
	{
		mute = true;
	}
	F32 mute_volume = mute ? 0.0f : 1.0f;

	if (gAudiop) 
	{
		// Sound Effects
		gAudiop->setMasterGain(sAudioLevelMaster);

		gAudiop->setDopplerFactor(sAudioLevelDoppler);
		gAudiop->setRolloffFactor(sAudioLevelRolloff);
		gAudiop->setMuted(mute);

		gAudiop->enableWind(!mute && !sMuteAmbient && !sDisableWindAudio &&
							sAudioLevelMaster * sAudioLevelAmbient > 0.05f);
		if (force_update)
		{
			audio_update_wind(true);
		}

		// handle secondary gains
		gAudiop->setSecondaryGain(LLAudioEngine::AUDIO_TYPE_SFX,
								  sMuteSounds ? 0.f : sAudioLevelSFX);
		gAudiop->setSecondaryGain(LLAudioEngine::AUDIO_TYPE_UI,
								  sMuteUI ? 0.f : sAudioLevelUI);
		gAudiop->setSecondaryGain(LLAudioEngine::AUDIO_TYPE_AMBIENT,
								  sMuteAmbient ? 0.f : sAudioLevelAmbient);

		// Streaming Music
		F32 music_volume = sAudioLevelMusic;
		music_volume = mute_volume * sAudioLevelMaster * music_volume * music_volume;
		gAudiop->setInternetStreamGain(sMuteMusic ? 0.f : music_volume);
	}

	// Streaming Media
	F32 media_volume = sAudioLevelMedia;
	media_volume = mute_volume * sAudioLevelMaster * media_volume * media_volume;
	LLViewerMedia::setVolume(sMuteMedia ? 0.0f : media_volume);

	// Voice
	if (LLVoiceClient::instanceExists())
	{
		F32 voice_volume = sAudioLevelVoice;
		voice_volume = mute_volume * sAudioLevelMaster * voice_volume;
		LLVoiceClient::getInstance()->setVoiceVolume(sMuteVoice ? 0.f : voice_volume);
		LLVoiceClient::getInstance()->setMicGain(sMuteVoice ? 0.f : sAudioLevelMic);

		if (sMuteWhenMinimized && gViewerWindow && !gViewerWindow->getActive())
		{
			LLVoiceClient::getInstance()->setMuteMic(true);
		}
		else
		{
			LLVoiceClient::getInstance()->setMuteMic(false);
		}
	}
}

void audio_update_listener()
{
	if (gAudiop)
	{
		// update listener position because agent has moved
		LLVector3d lpos_global = gAgent.getCameraPositionGlobal();
		LLVector3 lpos_global_f;
		lpos_global_f.setVec(lpos_global);

		gAudiop->setListener(lpos_global_f,
							 // LLViewerCamera::getInstance()VelocitySmoothed, 
							 // LLVector3::zero,
							 gAgent.getVelocity(),    // !!! *TODO: need to replace this with smoothed velocity!
							 LLViewerCamera::getInstance()->getUpAxis(),
							 LLViewerCamera::getInstance()->getAtAxis());
	}
}

void audio_update_wind(bool force_update)
{
	static LLCachedControl<bool> sMuteAudio(gSavedSettings, "MuteAudio");
	static LLCachedControl<bool> sMuteAmbient(gSavedSettings, "MuteAmbient");
	static LLCachedControl<F32> sAudioLevelMaster(gSavedSettings, "AudioLevelMaster");
	static LLCachedControl<F32> sAudioLevelAmbient(gSavedSettings, "AudioLevelAmbient");
	static LLCachedControl<F32> sAudioLevelRolloff(gSavedSettings, "AudioLevelRolloff");

	if (!gAudiop || !gAudiop->isWindEnabled())
	{
		return;
	}
	//
	//  Extract height above water to modulate filter by whether above/below water 
	// 
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		static F32 last_camera_water_height = -1000.f;
		LLVector3 camera_pos = gAgent.getCameraPositionAgent();
		F32 camera_water_height = camera_pos.mV[VZ] - region->getWaterHeight();

		//
		//  Don't update rolloff factor unless water surface has been crossed
		//
		if (force_update || last_camera_water_height * camera_water_height < 0.f)
		{
			if (camera_water_height < 0.f)
			{
				gAudiop->setRolloffFactor(sAudioLevelRolloff * LL_ROLLOFF_MULTIPLIER_UNDER_WATER);
			}
			else 
			{
				gAudiop->setRolloffFactor(sAudioLevelRolloff);
			}
		}
		// this line rotates the wind vector to be listener (agent) relative
		// unfortunately we have to pre-translate to undo the translation that
		// occurs in the transform call
		gRelativeWindVec = gAgent.getFrameAgent().rotateToLocal(gWindVec - gAgent.getVelocity());

		// don't use the setter setMaxWindGain() because we don't
		// want to screw up the fade-in on startup by setting actual source gain
		// outside the fade-in.
		F32 master_volume  = sMuteAudio ? 0.f : sAudioLevelMaster;
		F32 ambient_volume = sMuteAmbient ? 0.f : sAudioLevelAmbient;

		F32 wind_volume = master_volume * ambient_volume;
		gAudiop->mMaxWindGain = wind_volume;

		last_camera_water_height = camera_water_height;
		gAudiop->updateWind(gRelativeWindVec, camera_water_height);
	}
}
