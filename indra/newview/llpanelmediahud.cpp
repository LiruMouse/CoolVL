/**
 * @file llpanelmsgs.cpp
 * @brief Message popup preferences panel
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llpanelmediahud.h"

#include "llbutton.h"
#include "lliconctrl.h"
#include "llpanel.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "llrender.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llhudview.h"
#include "llselectmgr.h"
#include "lltoolpie.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvovolume.h"
#include "llweb.h"

glh::matrix4f glh_get_current_modelview();
glh::matrix4f glh_get_current_projection();

const F32 ZOOM_NEAR_PADDING		= 1.0f;
const F32 ZOOM_MEDIUM_PADDING	= 1.2f;
const F32 ZOOM_FAR_PADDING		= 1.5f;

//
// LLPanelMediaHUD
//

LLPanelMediaHUD::LLPanelMediaHUD(viewer_media_t media_impl)
 :	mMediaImpl(media_impl),
	mCloseButton(NULL),
	mBackButton(NULL),
	mForwardButton(NULL),
	mHomeButton(NULL),
	mOpenButton(NULL),
	mReloadButton(NULL),
	mPlayButton(NULL),
	mPauseButton(NULL),
	mStopButton(NULL),
	mFocusedControls(NULL),
	mHoverControls(NULL),
	mMediaRegion(NULL),
	mMediaFocus(false),
	mCurrentZoom(ZOOM_NONE),
	mScrollState(SCROLL_NONE)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_media_hud.xml");
	mMouseMoveTimer.reset();
	mFadeTimer.stop();
	mPanelHandle.bind(this);
}

LLPanelMediaHUD::~LLPanelMediaHUD()
{
	mMediaImpl = NULL;
}

BOOL LLPanelMediaHUD::postBuild()
{
	mMediaRegion = getChild<LLView>("media_region");

	mFocusedControls = getChild<LLPanel>("media_focused_controls");
	mHoverControls = getChild<LLPanel>("media_hover_controls");

	mCloseButton = getChild<LLButton>("close");
	mCloseButton->setClickedCallback(onClickClose, this);

	mBackButton = getChild<LLButton>("back");
	mBackButton->setClickedCallback(onClickBack, this);

	mForwardButton = getChild<LLButton>("fwd");
	mForwardButton->setClickedCallback(onClickForward, this);

	mHomeButton = getChild<LLButton>("home");
	mHomeButton->setClickedCallback(onClickHome, this);

	mStopButton = getChild<LLButton>("stop");
	mStopButton->setClickedCallback(onClickStop, this);

	mMediaStopButton = getChild<LLButton>("media_stop");
	mMediaStopButton->setClickedCallback(onClickStop, this);

	mReloadButton = getChild<LLButton>("reload");
	mReloadButton->setClickedCallback(onClickReload, this);

	mPlayButton = getChild<LLButton>("play");
	mPlayButton->setClickedCallback(onClickPlay, this);

	mPauseButton = getChild<LLButton>("pause");
	mPauseButton->setClickedCallback(onClickPause, this);

	mOpenButton = getChild<LLButton>("new_window");
	mOpenButton->setClickedCallback(onClickOpen, this);

	LLButton* zoom_btn = getChild<LLButton>("zoom_frame");
	zoom_btn->setClickedCallback(onClickZoom, this);

	LLButton* open_btn_h = getChild<LLButton>("new_window_hover");
	open_btn_h->setClickedCallback(onClickOpen, this);

	LLButton* zoom_btn_h = getChild<LLButton>("zoom_frame_hover");
	zoom_btn_h->setClickedCallback(onClickZoom, this);

	LLButton* scroll_up_btn = getChild<LLButton>("scrollup");
	scroll_up_btn->setClickedCallback(onScrollUp, this);
	scroll_up_btn->setHeldDownCallback(onScrollUpHeld);
	scroll_up_btn->setMouseUpCallback(onScrollStop);
	LLButton* scroll_left_btn = getChild<LLButton>("scrollleft");
	scroll_left_btn->setClickedCallback(onScrollLeft, this);
	scroll_left_btn->setHeldDownCallback(onScrollLeftHeld);
	scroll_left_btn->setMouseUpCallback(onScrollStop);
	LLButton* scroll_right_btn = getChild<LLButton>("scrollright");
	scroll_right_btn->setClickedCallback(onScrollRight, this);
	scroll_right_btn->setHeldDownCallback(onScrollLeftHeld);
	scroll_right_btn->setMouseUpCallback(onScrollStop);
	LLButton* scroll_down_btn = getChild<LLButton>("scrolldown");
	scroll_down_btn->setClickedCallback(onScrollDown, this);
	scroll_down_btn->setHeldDownCallback(onScrollDownHeld);
	scroll_down_btn->setMouseUpCallback(onScrollStop);

	mMouseInactiveTime = gSavedSettings.getF32("MediaControlTimeout");
	mControlFadeTime = gSavedSettings.getF32("MediaControlFadeTime");
	if (mControlFadeTime <= 0.f)
	{
		mControlFadeTime = 0.1f;
	}

	// clicks on HUD buttons do not remove keyboard focus from media
	setIsChrome(TRUE);

	return TRUE;
}

void LLPanelMediaHUD::updateShape()
{
	const S32 MIN_HUD_WIDTH = 200;
	const S32 MIN_HUD_HEIGHT = 120;

	LLPluginClassMedia* media_plugin = NULL;
	if (mMediaImpl.notNull() && mMediaImpl->hasMedia())
	{
		media_plugin = mMediaImpl->getMediaPlugin();
	}

	// Early out for no media plugin
	if (media_plugin == NULL)
	{
		setVisible(FALSE);
		return;
	}

	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!parcel)
	{
		setVisible(FALSE);
		return;
	}
	bool can_navigate = parcel->getMediaAllowNavigate();

	//LLObjectSelectionHandle selection = LLViewerMediaFocus::getInstance()->getSelection();

	LLSelectNode* nodep = mMediaFocus ? LLSelectMgr::getInstance()->getSelection()->getFirstNode()
									  : LLSelectMgr::getInstance()->getHoverNode();
	if (!nodep)
	{
		return;
	}

	setVisible(FALSE);

	LLViewerObject* objectp = nodep->getObject();
	if (objectp)
	{
		// Set the state of the buttons
		mBackButton->setVisible(true);
		mForwardButton->setVisible(true);
		mReloadButton->setVisible(true);
		mStopButton->setVisible(false);
		mHomeButton->setVisible(true);
		if (mCloseButton)
		{
			mCloseButton->setVisible(true);
		}
		mOpenButton->setVisible(true);

		if (mMediaFocus)
		{
			mBackButton->setEnabled(mMediaImpl->canNavigateBack() && can_navigate);
			mForwardButton->setEnabled(mMediaImpl->canNavigateForward() && can_navigate);
			mStopButton->setEnabled(can_navigate);
			mHomeButton->setEnabled(can_navigate);

			LLPluginClassMediaOwner::EMediaStatus result = media_plugin->getStatus();
			if (media_plugin->pluginSupportsMediaTime())
			{
				mReloadButton->setEnabled(FALSE);
				mReloadButton->setVisible(FALSE);
				mMediaStopButton->setVisible(TRUE);
				mHomeButton->setVisible(FALSE);
				mBackButton->setEnabled(TRUE);
				mForwardButton->setEnabled(TRUE);
				if (result == LLPluginClassMediaOwner::MEDIA_PLAYING)
				{
					mPlayButton->setEnabled(FALSE);
					mPlayButton->setVisible(FALSE);
					mPauseButton->setEnabled(TRUE);
					mPauseButton->setVisible(TRUE);
					mMediaStopButton->setEnabled(TRUE);
				}
				else
				{
					mPlayButton->setEnabled(TRUE);
					mPlayButton->setVisible(TRUE);
					mPauseButton->setEnabled(FALSE);
					mPauseButton->setVisible(FALSE);
					mMediaStopButton->setEnabled(FALSE);
				}
			}
			else
			{
				mPlayButton->setVisible(FALSE);
				mPauseButton->setVisible(FALSE);
				mMediaStopButton->setVisible(FALSE);
				if (result == LLPluginClassMediaOwner::MEDIA_LOADING)
				{
					mReloadButton->setEnabled(FALSE);
					mReloadButton->setVisible(FALSE);
					mStopButton->setEnabled(TRUE);
					mStopButton->setVisible(TRUE);
				}
				else
				{
					mReloadButton->setEnabled(TRUE);
					mReloadButton->setVisible(TRUE);
					mStopButton->setEnabled(FALSE);
					mStopButton->setVisible(FALSE);
				}
			}
		}
		mFocusedControls->setVisible(mMediaFocus);
		mHoverControls->setVisible(!mMediaFocus);

		// Handle Scrolling
		switch (mScrollState)
		{
			case SCROLL_UP:
				media_plugin->scrollEvent(0, -1, MASK_NONE);
				break;
			case SCROLL_DOWN:
				media_plugin->scrollEvent(0, 1, MASK_NONE);
				break;
			case SCROLL_LEFT:
				mMediaImpl->handleKeyHere(KEY_LEFT, MASK_NONE);
				break;
			case SCROLL_RIGHT:
				mMediaImpl->handleKeyHere(KEY_RIGHT, MASK_NONE);
				break;
			case SCROLL_NONE:
			default:
				break;
		}

		LLBBox screen_bbox;
		setVisible(TRUE);
		glh::matrix4f mat = glh_get_current_projection() * glh_get_current_modelview();

		std::vector<LLVector3> vect_face;
		LLVolume* volume = objectp->getVolume();
		if (volume)
		{
			const LLVolumeFace& vf = volume->getVolumeFace(nodep->getLastSelectedTE());

			LLVector3 ext[2];
			ext[0].set(vf.mExtents[0].getF32ptr());
			ext[1].set(vf.mExtents[1].getF32ptr());

			LLVector3 center = (ext[0] + ext[1]) * 0.5f;
			LLVector3 size = (ext[1] - ext[0]) * 0.5f;
			LLVector3 vert[] =
			{
				center + size.scaledVec(LLVector3(1, 1, 1)),
				center + size.scaledVec(LLVector3(-1, 1, 1)),
				center + size.scaledVec(LLVector3(1, -1, 1)),
				center + size.scaledVec(LLVector3(-1, -1, 1)),
				center + size.scaledVec(LLVector3(1, 1, -1)),
				center + size.scaledVec(LLVector3(-1, 1, -1)),
				center + size.scaledVec(LLVector3(1, -1, -1)),
				center + size.scaledVec(LLVector3(-1, -1, -1)),
			};

			LLVOVolume* vo = (LLVOVolume*)objectp;
			for (U32 i = 0; i < 8; ++i)
			{
				vect_face.push_back(vo->volumePositionToAgent(vert[i]));
			}
		}

		LLVector3 min = LLVector3(1, 1, 1);
		LLVector3 max = LLVector3(-1, -1, -1);
		for (std::vector<LLVector3>::iterator vert_it = vect_face.begin(),
											  vert_end = vect_face.end();
			 vert_it != vert_end; ++vert_it)
		{
			// project silhouette vertices into screen space
			glh::vec3f screen_vert = glh::vec3f(vert_it->mV);
			mat.mult_matrix_vec(screen_vert);

			// add to screenspace bounding box
			update_min_max(min, max, LLVector3(screen_vert.v));
		}

		LLCoordGL screen_min;
		screen_min.mX = llround((F32)gViewerWindow->getWindowWidth() * (min.mV[VX] + 1.f) * 0.5f);
		screen_min.mY = llround((F32)gViewerWindow->getWindowHeight() * (min.mV[VY] + 1.f) * 0.5f);

		LLCoordGL screen_max;
		screen_max.mX = llround((F32)gViewerWindow->getWindowWidth() * (max.mV[VX] + 1.f) * 0.5f);
		screen_max.mY = llround((F32)gViewerWindow->getWindowHeight() * (max.mV[VY] + 1.f) * 0.5f);

		// grow panel so that screenspace bounding box fits inside the
		// "media_region" element of the HUD
		LLRect media_hud_rect;
		getParent()->screenRectToLocal(LLRect(screen_min.mX, screen_max.mY,
											  screen_max.mX, screen_min.mY),
									   &media_hud_rect);
		media_hud_rect.mLeft -= mMediaRegion->getRect().mLeft;
		media_hud_rect.mBottom -= mMediaRegion->getRect().mBottom;
		media_hud_rect.mTop += getRect().getHeight() - mMediaRegion->getRect().mTop;
		media_hud_rect.mRight += getRect().getWidth() - mMediaRegion->getRect().mRight;

		LLRect old_hud_rect = media_hud_rect;
		// keep all parts of HUD on-screen
		media_hud_rect.intersectWith(getParent()->getLocalRect());

		// If we had to clip the rect, don't display the border
		childSetVisible("bg_image", false);

		// clamp to minimum size, keeping centered
		media_hud_rect.setCenterAndSize(media_hud_rect.getCenterX(),
										media_hud_rect.getCenterY(),
										llmax(MIN_HUD_WIDTH,
											  media_hud_rect.getWidth()),
										llmax(MIN_HUD_HEIGHT,
											  media_hud_rect.getHeight()));

		userSetShape(media_hud_rect);

		// Test mouse position to see if the cursor is stationary
		LLCoordWindow cursor_pos_window;
		getWindow()->getCursorPosition(&cursor_pos_window);

		// If last pos is not equal to current pos, the mouse has moved
		// We need to reset the timer, and make sure the panel is visible
		if (cursor_pos_window.mX != mLastCursorPos.mX ||
			cursor_pos_window.mY != mLastCursorPos.mY ||
			mScrollState != SCROLL_NONE)
		{
			mMouseMoveTimer.start();
			mLastCursorPos = cursor_pos_window;
		}

		// Mouse has been stationary, but not for long enough to fade the UI
		if (mMouseMoveTimer.getElapsedTimeF32() < mMouseInactiveTime)
		{
			// If we have started fading, reset the alpha values
			if (mFadeTimer.getStarted())
			{
				F32 alpha = 1.0f;
				setAlpha(alpha);
				mFadeTimer.stop();
			}
		}
		// If we need to start fading the UI (and we have not already started)
		else if (!mFadeTimer.getStarted())
		{
			mFadeTimer.reset();
			mFadeTimer.start();
		}
	}
}
/*virtual*/
void LLPanelMediaHUD::draw()
{
	if (mFadeTimer.getStarted())
	{
		if (mFadeTimer.getElapsedTimeF32() >= mControlFadeTime)
		{
			setVisible(FALSE);
		}
		else
		{
			F32 time = mFadeTimer.getElapsedTimeF32();
			F32 alpha = llmax(lerp(1.0, 0.0, time / mControlFadeTime), 0.0f);
			setAlpha(alpha);
		}
	}
	LLPanel::draw();
}

void LLPanelMediaHUD::setAlpha(F32 alpha)
{
	LLViewQuery query;

	LLView* query_view = mMediaFocus ? mHoverControls : mHoverControls;
	child_list_t children = query(query_view);
	for (child_list_iter_t child_iter = children.begin(), end = children.end();
		 child_iter != end; ++child_iter)
	{
		LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(*child_iter);
		if (ctrl)
		{
			ctrl->setAlpha(alpha);
		}
	}

	LLPanel::setAlpha(alpha);
}

BOOL LLPanelMediaHUD::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	return LLViewerMediaFocus::getInstance()->handleScrollWheel(x, y, clicks);
}

bool LLPanelMediaHUD::isMouseOver()
{
	if ( ! getVisible() )
	{
		return false;
	}
	LLRect screen_rect;
	LLCoordWindow cursor_pos_window;
	getWindow()->getCursorPosition(&cursor_pos_window);

	localRectToScreen(getLocalRect(), &screen_rect);
	//screenPointToLocal(cursor_pos_gl.mX, cursor_pos_gl.mY, &local_mouse_x,
	//				   &local_mouse_y);

	if (screen_rect.pointInRect(cursor_pos_window.mX, cursor_pos_window.mY))
	{
		return true;
	}
	return false;
}

//static
void LLPanelMediaHUD::onClickClose(void* user_data)
{
	LLViewerMediaFocus::getInstance()->setFocusFace(FALSE, NULL, 0, NULL);
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mCurrentZoom != ZOOM_NONE)
	{
		// gAgent.setFocusOnAvatar(TRUE, ANIMATE);
		this_panel->mCurrentZoom = ZOOM_NONE;
	}
	this_panel->setVisible(FALSE);

}

//static
void LLPanelMediaHUD::onClickBack(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull() && this_panel->mMediaImpl->hasMedia())
	{
		if (this_panel->mMediaImpl->getMediaPlugin()->pluginSupportsMediaTime())
		{
			this_panel->mMediaImpl->getMediaPlugin()->start(-2.0);
		}
		else
		{
			this_panel->mMediaImpl->getMediaPlugin()->browse_back();
		}
	}
}

//static
void LLPanelMediaHUD::onClickForward(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull() && this_panel->mMediaImpl->hasMedia())
	{
		if (this_panel->mMediaImpl->getMediaPlugin()->pluginSupportsMediaTime())
		{
			this_panel->mMediaImpl->getMediaPlugin()->start(2.0);
		}
		else
		{
			this_panel->mMediaImpl->getMediaPlugin()->browse_forward();
		}
	}
}

//static
void LLPanelMediaHUD::onClickHome(void* user_data)
{
	//LLViewerMedia::navigateHome();
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull())
	{
		this_panel->mMediaImpl->navigateHome();
	}
}

//static
void LLPanelMediaHUD::onClickOpen(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull())
	{
		LLWeb::loadURL(this_panel->mMediaImpl->getMediaURL());
	}
}

//static
void LLPanelMediaHUD::onClickReload(void* user_data)
{
	//LLViewerMedia::navigateHome();
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
	if (objectp && this_panel->mMediaImpl.notNull())
	{
		this_panel->mMediaImpl->navigateTo(objectp->getMediaURL());
	}
}

//static
void LLPanelMediaHUD::onClickPlay(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull() && this_panel->mMediaImpl->hasMedia())
	{
		this_panel->mMediaImpl->getMediaPlugin()->start();
	}
}

//static
void LLPanelMediaHUD::onClickPause(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull() && this_panel->mMediaImpl->hasMedia())
	{
		this_panel->mMediaImpl->getMediaPlugin()->pause();
	}
}

//static
void LLPanelMediaHUD::onClickStop(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull() && this_panel->mMediaImpl->hasMedia())
	{
		if (this_panel->mMediaImpl->getMediaPlugin()->pluginSupportsMediaTime())
		{
			this_panel->mMediaImpl->getMediaPlugin()->stop();
		}
		else
		{
			this_panel->mMediaImpl->getMediaPlugin()->browse_stop();
		}
	}
}

//static
void LLPanelMediaHUD::onClickZoom(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	this_panel->nextZoomLevel();
}

void LLPanelMediaHUD::nextZoomLevel()
{
	F32 zoom_padding = 0.0f;
	S32 last_zoom_level = (S32)mCurrentZoom;
	mCurrentZoom = (EZoomLevel)((last_zoom_level + 1) % (S32)ZOOM_END);

	switch (mCurrentZoom)
	{
		case ZOOM_NONE:
		{
			gAgent.setFocusOnAvatar(TRUE, ANIMATE);
			break;
		}
		case ZOOM_MEDIUM:
		{
			zoom_padding = ZOOM_MEDIUM_PADDING;
			break;
		}
		default:
		{
			gAgent.setFocusOnAvatar(TRUE, ANIMATE);
			break;
		}
	}

	if (zoom_padding > 0.0f)
	{
		LLViewerMediaFocus::getInstance()->setCameraZoom(zoom_padding);
	}
}

void LLPanelMediaHUD::onScrollUp(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull() && this_panel->mMediaImpl->hasMedia())
	{
		this_panel->mMediaImpl->getMediaPlugin()->scrollEvent(0, -1, MASK_NONE);
	}
}

void LLPanelMediaHUD::onScrollUpHeld(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	this_panel->mScrollState = SCROLL_UP;
}

void LLPanelMediaHUD::onScrollRight(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull())
	{
		this_panel->mMediaImpl->handleKeyHere(KEY_RIGHT, MASK_NONE);
	}
}

void LLPanelMediaHUD::onScrollRightHeld(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	this_panel->mScrollState = SCROLL_RIGHT;
}

void LLPanelMediaHUD::onScrollLeft(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull())
	{
		this_panel->mMediaImpl->handleKeyHere(KEY_LEFT, MASK_NONE);
	}
}

void LLPanelMediaHUD::onScrollLeftHeld(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	this_panel->mScrollState = SCROLL_LEFT;
}

void LLPanelMediaHUD::onScrollDown(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	if (this_panel->mMediaImpl.notNull() && this_panel->mMediaImpl->hasMedia())
	{
		this_panel->mMediaImpl->getMediaPlugin()->scrollEvent(0, 1, MASK_NONE);
	}
}

void LLPanelMediaHUD::onScrollDownHeld(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	this_panel->mScrollState = SCROLL_DOWN;
}

void LLPanelMediaHUD::onScrollStop(void* user_data)
{
	LLPanelMediaHUD* this_panel = static_cast<LLPanelMediaHUD*> (user_data);
	this_panel->mScrollState = SCROLL_NONE;
}
