/** 
 * @file lloverlaybar.cpp
 * @brief LLOverlayBar class implementation
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

// Temporary buttons that appear at the bottom of the screen when you
// are in a mode.

#include "llviewerprecompiledheaders.h"

#include "lloverlaybar.h"

#include "llaudioengine.h"
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llparcel.h"
#include "llrender.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llimview.h"
#include "llmediactrl.h"
#include "llmediaremotectrl.h"
#include "llpanelaudiovolume.h"
#include "llselectmgr.h"
#include "llviewerjoystick.h"
#include "llviewermedia.h"
#include "llviewermenu.h"			// handle_reset_view()
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvoiceclient.h"
#include "llvoiceremotectrl.h"

//
// Globals
//

LLOverlayBar *gOverlayBar = NULL;

extern S32 MENU_BAR_HEIGHT;

//
// Functions
//

//static
void* LLOverlayBar::createMasterRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;
	self->mMasterRemote =  new LLMediaRemoteCtrl("master_volume", LLRect(),
												 "panel_master_volume.xml",
												 LLMediaRemoteCtrl::REMOTE_VOLUME);
	return self->mMasterRemote;
}

//static
void* LLOverlayBar::createMediaRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;
	self->mMediaRemote =  new LLMediaRemoteCtrl("media_remote", LLRect(),
												"panel_media_remote.xml",
												LLMediaRemoteCtrl::REMOTE_MEDIA);
	return self->mMediaRemote;
}

//static
void* LLOverlayBar::createMusicRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;
	self->mMusicRemote =  new LLMediaRemoteCtrl("music_remote", LLRect(),
												"panel_music_remote.xml",
												LLMediaRemoteCtrl::REMOTE_MUSIC);
	return self->mMusicRemote;
}

//static
void* LLOverlayBar::createVoiceRemote(void* userdata)
{
	LLOverlayBar *self = (LLOverlayBar*)userdata;
	self->mVoiceRemote = new LLVoiceRemoteCtrl(std::string("voice_remote"));
	return self->mVoiceRemote;
}

LLOverlayBar::LLOverlayBar(const std::string& name, const LLRect& rect)
:	LLPanel(name, rect, FALSE),		// not bordered
	mMasterRemote(NULL),
	mMusicRemote(NULL),
	mMediaRemote(NULL),
	mVoiceRemote(NULL),
	mMediaState(STOPPED),
	mMusicState(STOPPED),
	mStatusBarPad(LLCachedControl<S32>(gSavedSettings, "StatusBarPad"))
{
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);

	mBuilt = false;

	LLCallbackMap::map_t factory_map;
	factory_map["master_volume"] = LLCallbackMap(LLOverlayBar::createMasterRemote, this);
	factory_map["media_remote"] = LLCallbackMap(LLOverlayBar::createMediaRemote, this);
	factory_map["music_remote"] = LLCallbackMap(LLOverlayBar::createMusicRemote, this);
	factory_map["voice_remote"] = LLCallbackMap(LLOverlayBar::createVoiceRemote, this);

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_overlaybar.xml", &factory_map);

	mBtnIMReceiced = getChild<LLButton>("IM Received");
	mBtnIMReceiced->setClickedCallback(onClickIMReceived, this);

	mBtnSetNotBusy = getChild<LLButton>("Set Not Busy");
	mBtnSetNotBusy->setClickedCallback(onClickSetNotBusy, this);

	mBtnMouseLook = getChild<LLButton>("Mouselook");
	mBtnMouseLook->setClickedCallback(onClickMouselook, this);

	mBtnStandUp = getChild<LLButton>("Stand Up");
	mBtnStandUp->setClickedCallback(onClickStandUp, this);

	mBtnFlyCam = getChild<LLButton>("Flycam");
	mBtnFlyCam->setClickedCallback(onClickFlycam, this);

	setFocusRoot(TRUE);
	mBuilt = true;

	mRoundedSquare = LLUI::getUIImage("rounded_square.tga");

	// make overlay bar conform to window size
	setRect(rect);
	layoutButtons();
}

LLOverlayBar::~LLOverlayBar()
{
	// LLView destructor cleans up children
}

// virtual
void LLOverlayBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape(width, height, called_from_parent);

	if (mBuilt) 
	{
		layoutButtons();
	}
}

void LLOverlayBar::layoutButtons()
{
	S32 width = getRect().getWidth();
	if (width > 1024) width = 1024;

	S32 count = getChildCount();

	const S32 num_media_controls = 3;
	media_remote_width = mMediaRemote ? mMediaRemote->getRect().getWidth() : 0;
	music_remote_width = mMusicRemote ? mMusicRemote->getRect().getWidth() : 0;
	voice_remote_width = mVoiceRemote ? mVoiceRemote->getRect().getWidth() : 0;
	master_remote_width = mMasterRemote ? mMasterRemote->getRect().getWidth() : 0;

	// total reserved width for all media remotes
	const S32 ENDPAD = 8;
	S32 remote_total_width = media_remote_width + mStatusBarPad + music_remote_width + mStatusBarPad + voice_remote_width + mStatusBarPad + master_remote_width + ENDPAD;

	// calculate button widths
	F32 segment_width = (F32)(width - remote_total_width) / (F32)(count - num_media_controls);

	S32 btn_width = lltrunc(segment_width - mStatusBarPad);

	// Evenly space all views
	LLRect r;
	S32 i = 0;
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		r = view->getRect();
		r.mLeft = (width) - llround(remote_total_width + (i++ - num_media_controls + 1) * segment_width);
		r.mRight = r.mLeft + btn_width;
		view->setRect(r);
	}

	// Fix up remotes to have constant width because they can't shrink
	S32 right = getRect().getWidth() - ENDPAD;
	if (mMasterRemote)
	{
		r = mMasterRemote->getRect();
		r.mRight = right;
		r.mLeft = right - master_remote_width;
		right = r.mLeft - mStatusBarPad;
		mMasterRemote->setRect(r);
	}
	if (mMusicRemote)
	{
		r = mMusicRemote->getRect();
		r.mRight = right;
		r.mLeft = right - music_remote_width;
		right = r.mLeft - mStatusBarPad;
		mMusicRemote->setRect(r);
	}
	if (mMediaRemote)
	{
		r = mMediaRemote->getRect();
		r.mRight = right;
		r.mLeft = right - media_remote_width;
		right = r.mLeft - mStatusBarPad;
		mMediaRemote->setRect(r);
	}
	if (mVoiceRemote)
	{
		r = mVoiceRemote->getRect();
		r.mRight = right;
		r.mLeft = right - voice_remote_width;
		mVoiceRemote->setRect(r);
	}

	updateBoundingRect();
}

void LLOverlayBar::draw()
{
	if (mRoundedSquare && gBottomPanel)
	{
		gGL.getTexUnit(0)->bind(mRoundedSquare->getImage());

		// draw rounded rect tabs behind all children
		LLRect r;
		// focus highlights
		static LLCachedControl<LLColor4U> floater_focus_border_color(gColors, "FloaterFocusBorderColor");
		LLColor4 color = LLColor4(floater_focus_border_color);
		gGL.color4fv(color.mV);
		if (gFocusMgr.childHasKeyboardFocus(gBottomPanel))
		{
			for (child_list_const_iter_t child_iter = getChildList()->begin();
				 child_iter != getChildList()->end(); ++child_iter)
			{
				LLView *view = *child_iter;
				if (view->getEnabled() && view->getVisible())
				{
					r = view->getRect();
					gl_segmented_rect_2d_tex(r.mLeft - mStatusBarPad / 3 - 1, 
											r.mTop + 3, 
											r.mRight + mStatusBarPad / 3 + 1,
											r.mBottom, 
											mRoundedSquare->getTextureWidth(), 
											mRoundedSquare->getTextureHeight(), 
											16, 
											ROUNDED_RECT_TOP);
				}
			}
		}

		// main tabs
		for (child_list_const_iter_t child_iter = getChildList()->begin();
			 child_iter != getChildList()->end(); ++child_iter)
		{
			LLView *view = *child_iter;
			if (view->getEnabled() && view->getVisible())
			{
				r = view->getRect();
				// draw a nice little pseudo-3D outline
				static LLCachedControl<LLColor4U> default_shadow_dark(gColors, "DefaultShadowDark");
				color = LLColor4(default_shadow_dark);
				gGL.color4fv(color.mV);
				gl_segmented_rect_2d_tex(r.mLeft - mStatusBarPad / 3 + 1, r.mTop + 2, r.mRight + mStatusBarPad / 3, r.mBottom, 
										 mRoundedSquare->getTextureWidth(), mRoundedSquare->getTextureHeight(), 16, ROUNDED_RECT_TOP);
				static LLCachedControl<LLColor4U> default_highlight_light(gColors, "DefaultHighlightLight");
				color = LLColor4(default_highlight_light);
				gGL.color4fv(color.mV);
				gl_segmented_rect_2d_tex(r.mLeft - mStatusBarPad / 3, r.mTop + 2, r.mRight + mStatusBarPad / 3 - 3, r.mBottom, 
										 mRoundedSquare->getTextureWidth(), mRoundedSquare->getTextureHeight(), 16, ROUNDED_RECT_TOP);
				// here's the main background.  Note that it overhangs on the bottom so as to hide the
				// focus highlight on the bottom panel, thus producing the illusion that the focus highlight
				// continues around the tabs
				static LLCachedControl<LLColor4U> focus_background_color(gColors, "FocusBackgroundColor");
				color = LLColor4(focus_background_color);
				gGL.color4fv(color.mV);
				gl_segmented_rect_2d_tex(r.mLeft - mStatusBarPad / 3 + 1, r.mTop + 1, r.mRight + mStatusBarPad / 3 - 1, r.mBottom - 1, 
										 mRoundedSquare->getTextureWidth(), mRoundedSquare->getTextureHeight(), 16, ROUNDED_RECT_TOP);
			}
		}
	}

	// draw children on top
	LLPanel::draw();
}

// Per-frame updates of visibility
void LLOverlayBar::refresh()
{
	BOOL im_received = gIMMgr && gIMMgr->getIMReceived();
	mBtnIMReceiced->setVisible(im_received);
	mBtnIMReceiced->setEnabled(im_received);

	BOOL busy = gAgent.getBusy();
	mBtnSetNotBusy->setVisible(busy);
	mBtnSetNotBusy->setEnabled(busy);

	BOOL flycam = LLViewerJoystick::getInstance()->getOverrideCamera();
	mBtnFlyCam->setVisible(flycam);
	mBtnFlyCam->setEnabled(flycam);

	BOOL mouselook_grabbed;
	mouselook_grabbed = gAgent.isControlGrabbed(CONTROL_ML_LBUTTON_DOWN_INDEX)
						|| gAgent.isControlGrabbed(CONTROL_ML_LBUTTON_UP_INDEX);
	mBtnMouseLook->setVisible(mouselook_grabbed);
	mBtnMouseLook->setEnabled(mouselook_grabbed);

	BOOL sitting = FALSE;
	if (isAgentAvatarValid())
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsUnsit)
		{
			sitting=FALSE;
		} else // sitting = true if agent is sitting
//mk
		sitting = gAgentAvatarp->mIsSitting;
		mBtnStandUp->setVisible(sitting);
		mBtnStandUp->setEnabled(sitting);
	}

	const S32 ENDPAD = 8;
	S32 right = getRect().getWidth() - master_remote_width - mStatusBarPad - ENDPAD;
	LLRect r;

	static LLCachedControl<bool> hide_master_remote(gSavedSettings, "HideMasterRemote");
	BOOL master_remote = !hide_master_remote;

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	if (mMusicRemote && gAudiop)
	{
		static LLCachedControl<bool> audio_streaming_music(gSavedSettings, "AudioStreamingMusic");
		if (!parcel || parcel->getMusicURL().empty() || !audio_streaming_music)
		{
			mMusicRemote->setVisible(FALSE);
			mMusicRemote->setEnabled(FALSE);
		}
		else
		{
			mMusicRemote->setEnabled(TRUE);
			r = mMusicRemote->getRect();
			r.mRight = right;
			r.mLeft = right - music_remote_width;
			right = r.mLeft - mStatusBarPad;
			mMusicRemote->setRect(r);
			mMusicRemote->setVisible(TRUE);
			master_remote = TRUE;
		}
	}

	if (mMediaRemote)
	{
		static LLCachedControl<bool> audio_streaming_video(gSavedSettings, "AudioStreamingVideo");
		if (parcel && parcel->getMediaURL()[0] && audio_streaming_video)
		{
			// display remote control 
			mMediaRemote->setEnabled(TRUE);
			r = mMediaRemote->getRect();
			r.mRight = right;
			r.mLeft = right - media_remote_width;
			right = r.mLeft - mStatusBarPad;
			mMediaRemote->setRect(r);
			mMediaRemote->setVisible(TRUE);
			master_remote = TRUE;
		}
		else
		{
			mMediaRemote->setVisible(FALSE);
			mMediaRemote->setEnabled(FALSE);
		}
	}
	if (mVoiceRemote)
	{
		if (LLVoiceClient::voiceEnabled())
		{
			r = mVoiceRemote->getRect();
			r.mRight = right;
			r.mLeft = right - voice_remote_width;
			mVoiceRemote->setRect(r);
			mVoiceRemote->setVisible(TRUE);
			master_remote = TRUE;
		}
		else
		{
			mVoiceRemote->setVisible(FALSE);
		}
	}

	mMasterRemote->setVisible(master_remote);
	mMasterRemote->setEnabled(master_remote);

	// turn off the whole bar in mouselook
	if (gAgent.cameraMouselook())
	{
		setVisible(FALSE);
	}
	else
	{
		setVisible(TRUE);
	}

	updateBoundingRect();
}

//-----------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------

// static
void LLOverlayBar::onClickIMReceived(void*)
{
	if (gIMMgr)
	{
		gIMMgr->setFloaterOpen(TRUE);
	}
}

// static
void LLOverlayBar::onClickSetNotBusy(void*)
{
	gAgent.clearBusy();
}

// static
void LLOverlayBar::onClickFlycam(void*)
{
	LLViewerJoystick::getInstance()->toggleFlycam();
}

// static
void LLOverlayBar::onClickResetView(void* data)
{
	handle_reset_view();
}

//static
void LLOverlayBar::onClickMouselook(void*)
{
	gAgent.changeCameraToMouselook();
}

//static
void LLOverlayBar::onClickStandUp(void*)
{
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsUnsit)
	{
		if (isAgentAvatarValid() &&	gAgentAvatarp->mIsSitting)
		{
			return;
		}
	}
//mk
	LLSelectMgr::getInstance()->deselectAllForStandingUp();
	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
//MK
	if (gRRenabled && gAgent.mRRInterface.contains("standtp"))
	{
		gAgent.mRRInterface.mSnappingBackToLastStandingLocation = true;
		gAgent.teleportViaLocationLookAt (gAgent.mRRInterface.mLastStandingLocation);
		gAgent.mRRInterface.mSnappingBackToLastStandingLocation = false;
	}
//mk
}

////////////////////////////////////////////////////////////////////////////////
void LLOverlayBar::audioFilterPlay()
{
	if (gOverlayBar && gOverlayBar->mMusicState != PLAYING)
	{
		gOverlayBar->mMusicState = PLAYING;
	}
}

void LLOverlayBar::audioFilterStop()
{
	if (gOverlayBar && gOverlayBar->mMusicState != STOPPED)
	{
		gOverlayBar->mMusicState = STOPPED;
	}
}

////////////////////////////////////////////////////////////////////////////////
// static media helpers
// *TODO: Move this into an audio manager abstraction

//static
void LLOverlayBar::mediaPlay(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMediaState = PLAYING; // desired state
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (parcel)
	{
		std::string path("");
		LLViewerParcelMedia::sIsUserAction = true;
		LLViewerParcelMedia::play(parcel);
	}
}
//static
void LLOverlayBar::mediaPause(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMediaState = PAUSED; // desired state
	LLViewerParcelMedia::pause();
}
//static
void LLOverlayBar::mediaStop(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMediaState = STOPPED; // desired state
	LLViewerParcelMedia::stop();
}

//static
void LLOverlayBar::musicPlay(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMusicState = PLAYING; // desired state
	if (gAudiop)
	{
		LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		if (parcel)
		{
			// this doesn't work properly when crossing parcel boundaries - even when the 
			// stream is stopped, it doesn't return the right thing - commenting out for now.
// 			if (gAudiop->isInternetStreamPlaying() == 0)
			{
				LLViewerParcelMedia::sIsUserAction = true;
				LLViewerParcelMedia::playStreamingMusic(parcel);
			}
		}
	}
}
//static
void LLOverlayBar::musicPause(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMusicState = PAUSED; // desired state
	if (gAudiop)
	{
		gAudiop->pauseInternetStream(1);
	}
}
//static
void LLOverlayBar::musicStop(void*)
{
	if (!gOverlayBar)
	{
		return;
	}
	gOverlayBar->mMusicState = STOPPED; // desired state
	if (gAudiop)
	{
		gAudiop->stopInternetStream();
	}
}

void LLOverlayBar::toggleAudioVolumeFloater(void* user_data)
{
	LLFloaterAudioVolume::toggleInstance(LLSD());
}
