/** 
 * @file lltoolbar.cpp
 * @author James Cook, Richard Nelson
 * @brief Large friendly buttons at bottom of screen.
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

#include "lltoolbar.h"

#include "imageids.h"
#include "llbutton.h"
#include "llparcel.h"
#include "llrect.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llfirstuse.h"
#include "llfloateravatarlist.h"
#include "llfloaterchatterbox.h"
#include "llfloaterfriends.h"
#include "llfloatergroups.h"
#include "llfloatersnapshot.h"
#include "llfocusmgr.h"
#include "llimview.h"
#include "llinventoryview.h"
#include "llmenucommands.h"
#include "lltooldraganddrop.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lluiconstants.h"
#include "llviewermenu.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"

#if LL_DARWIN

#include "llresizehandle.h"

// This class draws like an LLResizeHandle but has no interactivity.
// It's just there to provide a cue to the user that the lower right corner of the window functions as a resize handle.
class LLFakeResizeHandle : public LLResizeHandle
{
public:
	LLFakeResizeHandle(const std::string& name, const LLRect& rect,
					   S32 min_width, S32 min_height,
					   ECorner corner = RIGHT_BOTTOM)
	:	LLResizeHandle(name, rect, min_width, min_height, corner)
	{
	}

	virtual BOOL handleHover(S32 x, S32 y, MASK mask)		{ return FALSE; };
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask)	{ return FALSE; };
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask)		{ return FALSE; };
};

#endif // LL_DARWIN

//
// Globals
//

LLToolBar* gToolBar = NULL;
S32 TOOL_BAR_HEIGHT = 20;

//
// Statics
//
F32	LLToolBar::sInventoryAutoOpenTime = 1.f;

//
// Functions
//

LLToolBar::LLToolBar(const std::string& name, const LLRect& r)
:	LLPanel(name, r, BORDER_NO)
#if LL_DARWIN
	, mResizeHandle(NULL)
#endif // LL_DARWIN
{
	setIsChrome(TRUE);
	setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM);
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_toolbar.xml");
	setFocusRoot(TRUE);
}

BOOL LLToolBar::postBuild()
{
	mChatButton = getChild<LLButton>("chat_btn");
	mChatButton->setClickedCallback(onClickChat, this);
	mChatButton->setControlName("ChatVisible", NULL);

	mIMButton = getChild<LLButton>("communicate_btn");
	mIMButton->setClickedCallback(onClickIM, this);
	mIMButton->setControlName("ShowCommunicate", NULL);

	mFriendsButton = getChild<LLButton>("friends_btn");
	mFriendsButton->setClickedCallback(onClickFriends, this);
	mFriendsButton->setControlName("ShowFriends", NULL);

	mGroupsButton = getChild<LLButton>("groups_btn");
	mGroupsButton->setClickedCallback(onClickGroups, this);
	mGroupsButton->setControlName("ShowGroups", NULL);

	mFlyButton = getChild<LLButton>("fly_btn");
	mFlyButton->setClickedCallback(onClickFly, this);
	mFlyButton->setControlName("FlyBtnState", NULL);

	mSnapshotButton = getChild<LLButton>("snapshot_btn");
	mSnapshotButton->setClickedCallback(onClickSnapshot, this);
	mSnapshotButton->setControlName("", NULL);

	mSearchButton = getChild<LLButton>("directory_btn");
	mSearchButton->setClickedCallback(onClickSearch, this);
	mSearchButton->setControlName("ShowDirectory", NULL);

	mBuildButton = getChild<LLButton>("build_btn");
	mBuildButton->setClickedCallback(onClickBuild, this);
	mBuildButton->setControlName("BuildBtnState", NULL);

	mRadarButton = getChild<LLButton>("radar_btn");
	mRadarButton->setClickedCallback(onClickRadar, this);
	mRadarButton->setControlName("ShowRadar", NULL);

	mMiniMapButton = getChild<LLButton>("minimap_btn");
	mMiniMapButton->setClickedCallback(onClickMiniMap, this);
	mMiniMapButton->setControlName("ShowMiniMap", NULL);

	mMapButton = getChild<LLButton>("map_btn");
	mMapButton->setClickedCallback(onClickMap, this);
	mMapButton->setControlName("ShowWorldMap", NULL);

	mInventoryButton = getChild<LLButton>("inventory_btn");
	mInventoryButton->setClickedCallback(onClickInventory, this);
	mInventoryButton->setControlName("ShowInventory", NULL);

	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		LLButton* buttonp = dynamic_cast<LLButton*>(view);
		if (buttonp)
		{
			buttonp->setSoundFlags(LLView::SILENT);
		}
	}

#if LL_DARWIN
	if (mResizeHandle == NULL)
	{
		LLRect rect(0, 0, RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT);
		mResizeHandle = new LLFakeResizeHandle(std::string(""), rect,
											   RESIZE_HANDLE_WIDTH,
											   RESIZE_HANDLE_HEIGHT);
		this->addChildAtEnd(mResizeHandle);
	}
#endif // LL_DARWIN

	layoutButtons();

	return TRUE;
}

LLToolBar::~LLToolBar()
{
	// LLView destructor cleans up children
}

BOOL LLToolBar::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								  EDragAndDropType cargo_type,
								  void* cargo_data,
								  EAcceptance* accept,
								  std::string& tooltip_msg)
{
	LLInventoryView* active_inventory = LLInventoryView::getActiveInventory();

	if (active_inventory && active_inventory->getVisible())
	{
		mInventoryAutoOpen = FALSE;
	}
	else if (mInventoryButton->getRect().pointInRect(x, y))
	{
		if (mInventoryAutoOpen)
		{
			if (!(active_inventory && active_inventory->getVisible()) &&
				mInventoryAutoOpenTimer.getElapsedTimeF32() > sInventoryAutoOpenTime)
			{
				LLInventoryView::showAgentInventory();
			}
		}
		else
		{
			mInventoryAutoOpen = TRUE;
			mInventoryAutoOpenTimer.reset();
		}
	}

	return LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data,
									  accept, tooltip_msg);
}

// static
void LLToolBar::toggle(void*)
{
	if (gToolBar)
	{
		BOOL show = gSavedSettings.getBOOL("ShowToolBar");
		gSavedSettings.setBOOL("ShowToolBar", !show);
		gToolBar->setVisible(!show);
	}
}

// static
BOOL LLToolBar::visible(void*)
{
	return gToolBar && gToolBar->getVisible();
}

void LLToolBar::layoutButtons()
{
	// Always spans whole window. JC                                        
	const S32 FUDGE_WIDTH_OF_SCREEN = 4;                                    
	const S32 PAD = 2;
	S32 width = gViewerWindow->getWindowWidth() + FUDGE_WIDTH_OF_SCREEN;    
	S32 count = getChildCount();
	if (!count) return;

	BOOL show = gSavedSettings.getBOOL("ShowChatButton");
	mChatButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowIMButton");
	mIMButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowFriendsButton");
	mFriendsButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowGroupsButton");
	mGroupsButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowFlyButton");
	mFlyButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowSnapshotButton");
	mSnapshotButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowSearchButton");
	mSearchButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowBuildButton");
	mBuildButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowRadarButton");
	mRadarButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowMiniMapButton");
	mMiniMapButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowMapButton");
	mMapButton->setVisible(show);
	if (!show) count--;

	show = gSavedSettings.getBOOL("ShowInventoryButton");
	mInventoryButton->setVisible(show);
	if (!show) count--;

	if (count < 1)
	{
		// No button in the toolbar !  Hide it.
		gSavedSettings.setBOOL("ShowToolBar", FALSE);
		return;
	}

#if LL_DARWIN
	// this function may be called before postBuild(), in which case mResizeHandle won't have been set up yet.
	if (mResizeHandle != NULL)
	{
		// a resize handle has been added as a child, increasing the count by one.
		count--;

		if (!gViewerWindow->getWindow()->getFullscreen())
		{
			// Only when running in windowed mode on the Mac, leave room for a resize widget on the right edge of the bar.
			width -= RESIZE_HANDLE_WIDTH;

			LLRect r;
			r.mLeft = width - PAD;
			r.mBottom = 0;
			r.mRight = r.mLeft + RESIZE_HANDLE_WIDTH;
			r.mTop = r.mBottom + RESIZE_HANDLE_HEIGHT;
			mResizeHandle->setRect(r);
			mResizeHandle->setVisible(TRUE);
		}
		else
		{
			mResizeHandle->setVisible(FALSE);
		}
	}
#endif // LL_DARWIN

	// We actually want to extend "PAD" pixels off the right edge of the    
	// screen, such that the rightmost button is aligned.                   
	F32 segment_width = (F32)(width + PAD) / (F32)count;                    
	S32 btn_width = lltrunc(segment_width - PAD);                           

	// Evenly space all views
	S32 height = -1;
	S32 i = count - 1;
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *btn_view = *child_iter;
		LLButton* buttonp = dynamic_cast<LLButton*>(btn_view);
		if (buttonp && btn_view->getVisible())
		{
			if (height < 0)
			{
				height = btn_view->getRect().getHeight();
			}
			S32 x = llround(i*segment_width);                               
			S32 y = 0;                                                      
			LLRect r;                                                               
			r.setOriginAndSize(x, y, btn_width, height);                    
			btn_view->setRect(r);                                           
			i--;                                                            
		}
	}                                                                       
}

// virtual
void LLToolBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);

	layoutButtons();
}

// Per-frame updates of visibility
void LLToolBar::refresh()
{
	static S32 previous_visibility = -1; // Ensure we use setVisible() on startup
	static LLCachedControl<bool> show_toolbar(gSavedSettings, "ShowToolBar", true);
	bool visible = show_toolbar && !gAgent.cameraMouselook();

	if (previous_visibility != (S32)visible)
	{
		setVisible(visible);
		if (visible)
		{
			// In case there would be no button to show,
			// it would re-hide the toolbar (on next frame)
			layoutButtons();
		}
		previous_visibility = (S32)visible;
	}
	if (!visible) return;

	BOOL sitting = FALSE;
	if (isAgentAvatarValid())
	{
		sitting = gAgentAvatarp->mIsSitting;
	}
	mFlyButton->setEnabled(!sitting && (gAgent.canFly() || gAgent.getFlying()));
 
//MK
	if (gRRenabled)
	{
		mBuildButton->setEnabled(LLViewerParcelMgr::getInstance()->agentCanBuild() &&
								 !gAgent.mRRInterface.mContainsRez &&
								 !gAgent.mRRInterface.mContainsEdit);
		mRadarButton->setEnabled(!gAgent.mRRInterface.mContainsShownames);
		mMiniMapButton->setEnabled(!gAgent.mRRInterface.mContainsShowminimap);
		mMapButton->setEnabled(!gAgent.mRRInterface.mContainsShowworldmap &&
							   !gAgent.mRRInterface.mContainsShowloc);
		mInventoryButton->setEnabled(!gAgent.mRRInterface.mContainsShowinv);
	}
	else
//mk
		mBuildButton->setEnabled(LLViewerParcelMgr::getInstance()->agentCanBuild());

	// Check to see if we are in build mode
	BOOL build_mode = LLToolMgr::getInstance()->inEdit();
	// And not just clicking on a scripted object
	if (LLToolGrab::getInstance()->getHideBuildHighlight())
	{
		build_mode = FALSE;
	}
	static LLCachedControl<bool> build_btn_state(gSavedSettings, "BuildBtnState");
	if ((bool)build_mode != build_btn_state)
	{
		gSavedSettings.setBOOL("BuildBtnState", build_mode);
	}
}

// static
void LLToolBar::onClickChat(void* user_data)
{
	handle_chat(NULL);
}

// static
void LLToolBar::onClickIM(void* user_data)
{
	LLFloaterChatterBox::toggleInstance(LLSD());
}

// static
void LLToolBar::onClickFly(void*)
{
	gAgent.toggleFlying();
}

// static
void LLToolBar::onClickSnapshot(void*)
{
	LLFloaterSnapshot::show (0);
}

// static
void LLToolBar::onClickSearch(void*)
{
	handle_find(NULL);
}

// static
void LLToolBar::onClickBuild(void*)
{
	toggle_build_mode();
}

// static
void LLToolBar::onClickMiniMap(void*)
{
	handle_mini_map(NULL);
}

// static
void LLToolBar::onClickRadar(void*)
{
	LLFloaterAvatarList::toggle(NULL);
}

// static
void LLToolBar::onClickMap(void*)
{
	handle_map(NULL);
}

// static
void LLToolBar::onClickFriends(void*)
{
	LLFloaterFriends::toggle();
}

// static
void LLToolBar::onClickGroups(void*)
{
	LLFloaterGroups::toggle();
}

// static
void LLToolBar::onClickInventory(void*)
{
	handle_inventory(NULL);
}
