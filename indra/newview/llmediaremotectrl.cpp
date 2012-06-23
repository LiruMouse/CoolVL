/** 
 * @file llmediaremotectrl.cpp
 * @brief A remote control for media (video and music)
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llmediaremotectrl.h"

#include "llaudioengine.h"
#include "llbutton.h"
#include "lliconctrl.h"
#include "llparcel.h"
#include "llstreamingaudio.h"
#include "lluictrlfactory.h"

#include "llmimetypes.h"
#include "lloverlaybar.h"
#include "llpanelaudiovolume.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "lltrans.h"

////////////////////////////////////////////////////////////////////////////////
//
//

static LLRegisterWidget<LLMediaRemoteCtrl> r("media_remote");

LLMediaRemoteCtrl::LLMediaRemoteCtrl(const std::string& name,
									 const LLRect& rect,
									 const std::string& xml_file,
									 const ERemoteType type)
:	LLPanel(name, rect, FALSE),
	mType(type),
	mIcon(NULL),
	mPlay(NULL),
	mPause(NULL),
	mStop(NULL),
	mIconToolTip("")
{
	setIsChrome(TRUE);
	setFocusRoot(TRUE);

	LLUICtrlFactory::getInstance()->buildPanel(this, xml_file);
}

BOOL LLMediaRemoteCtrl::postBuild()
{
	if (mType == REMOTE_MEDIA)
	{
		mIcon = getChild<LLIconCtrl>("media_icon");
		mIconToolTip = mIcon->getToolTip();

		mPlay = getChild<LLButton>("media_play");
		mPlay->setClickedCallback(LLOverlayBar::mediaPlay, this);

		mPause = getChild<LLButton>("media_pause");
		mPause->setClickedCallback(LLOverlayBar::mediaPause, this);

		mStop = getChild<LLButton>("media_stop");
		mStop->setClickedCallback(LLOverlayBar::mediaStop, this);
	}
	else if (mType == REMOTE_MUSIC)
	{
		mIcon = getChild<LLIconCtrl>("music_icon");
		mIconToolTip = mIcon->getToolTip();

		mPlay = getChild<LLButton>("music_play");
		mPlay->setClickedCallback(LLOverlayBar::musicPlay, this);

		mPause = getChild<LLButton>("music_pause");
		mPause->setClickedCallback(LLOverlayBar::musicPause, this);

		mStop = getChild<LLButton>("music_stop");
		mStop->setClickedCallback(LLOverlayBar::musicStop, this);
	}
	else if (mType == REMOTE_VOLUME)
	{
		childSetAction("volume", LLOverlayBar::toggleAudioVolumeFloater, this);
		//childSetControlName("volume", "ShowAudioVolume");	// Set in XML file
	}

	return TRUE;
}

void LLMediaRemoteCtrl::draw()
{
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	static LLCachedControl<LLColor4U> icon_disabled_color(gColors, "IconDisabledColor");
	static LLCachedControl<LLColor4U> icon_enabled_color(gColors, "IconEnabledColor");

	if (mType == REMOTE_MEDIA)
	{
		bool media_play_enabled = false;
		bool media_stop_enabled = false;
		bool media_show_pause = false;

		LLColor4 media_icon_color = LLColor4(icon_disabled_color);
		std::string media_type = "none/none";
		std::string media_url;

		static LLCachedControl<bool> audio_streaming_video(gSavedSettings, "AudioStreamingVideo");
		if (gOverlayBar && audio_streaming_video && parcel && parcel->getMediaURL()[0])
		{
			media_play_enabled = true;
			media_icon_color = LLColor4(icon_enabled_color);
			media_type = parcel->getMediaType();
			media_url = parcel->getMediaURL();

			LLViewerMediaImpl::EMediaStatus status = LLViewerParcelMedia::getStatus();
			switch (status)
			{
				case LLViewerMediaImpl::MEDIA_NONE:
					media_show_pause = false;
					media_stop_enabled = false;
					break;
				case LLViewerMediaImpl::MEDIA_LOADING:
				case LLViewerMediaImpl::MEDIA_LOADED:
				case LLViewerMediaImpl::MEDIA_PLAYING:
					// HACK: only show the pause button for movie types
					media_show_pause = LLMIMETypes::widgetType(parcel->getMediaType()) == "movie" ? true : false;
					media_stop_enabled = true;
					media_play_enabled = false;
					break;
				case LLViewerMediaImpl::MEDIA_PAUSED:
					media_show_pause = false;
					media_stop_enabled = true;
					break;
				default:
					// inherit defaults above
					break;
			}
		}

		mPlay->setEnabled(media_play_enabled);
		mPlay->setVisible(!media_show_pause);
		mStop->setEnabled(media_stop_enabled);
		mPause->setEnabled(media_show_pause);
		mPause->setVisible(media_show_pause);

		const std::string media_icon_name = LLMIMETypes::findIcon(media_type);
		if (mIcon)
		{
			if (!media_icon_name.empty())
			{
				mIcon->setImage(media_icon_name);
			}
			mIcon->setColor(media_icon_color);
			if (media_url.empty())
			{
				media_url = mIconToolTip;
			}
			else
			{
				media_url = mIconToolTip + " (" + media_url + ")";
			}
			mIcon->setToolTip(media_url);
		}
	}
	else if (mType == REMOTE_MUSIC)
	{
		bool music_play_enabled = false;
		bool music_stop_enabled = false;
		bool music_show_pause = false;

		LLColor4 music_icon_color = LLColor4(icon_disabled_color);
		std::string music_url;

		static LLCachedControl<bool> audio_streaming_music(gSavedSettings, "AudioStreamingMusic");
		if (gOverlayBar && gAudiop && audio_streaming_music &&
			parcel && !parcel->getMusicURL().empty())
		{
			music_url = parcel->getMusicURL();
			music_play_enabled = true;
			music_icon_color = LLColor4(icon_enabled_color);

			if (gOverlayBar->musicPlaying())
			{
				music_show_pause = true;
				music_stop_enabled = true;
			}
			else
			{
				music_show_pause = false;
				music_stop_enabled = false;
			}
		}

		mPlay->setEnabled(music_play_enabled);
		mPlay->setVisible(!music_show_pause);
		mStop->setEnabled(music_stop_enabled);
		mPause->setEnabled(music_show_pause);
		mPause->setVisible(music_show_pause);

		if (mIcon)
		{
			mIcon->setColor(music_icon_color);

			std::string tool_tip = mIconToolTip;
			if (!music_url.empty())
			{
				tool_tip += " (" + music_url + ")";
				if (mCachedURL != music_url)
				{
					mCachedURL = music_url;
					mCachedMetaData.clear();
				}
			}
			if (music_show_pause && gAudiop)
			{
				LLStreamingAudioInterface* stream = gAudiop->getStreamingAudioImpl();
				if (stream)
				{
					if (stream->newMetaData())
					{
						mCachedMetaData.clear();
						std::string temp = stream->getArtist();
						if (!temp.empty())
						{
							mCachedMetaData += "\n" + getString("artist_string") + ": " + temp;
						}
						temp = stream->getTitle();
						if (!temp.empty())
						{
							mCachedMetaData += "\n" + getString("title_string") + ": " + temp;
						}
						stream->gotMetaData();
						static LLCachedControl<bool> notify_stream_changes(gSavedSettings, "NotifyStreamChanges");
						if (notify_stream_changes && !mCachedMetaData.empty())
						{
							LLSD args;
							args["STREAM_DATA"] = mCachedMetaData;
							LLNotifications::instance().add("StreamChanged", args);
						}
					}
				}
				else
				{
					mCachedMetaData.clear();
				}
				tool_tip += mCachedMetaData;
			}
			mIcon->setToolTip(tool_tip);
		}
	}

	LLPanel::draw();
}
