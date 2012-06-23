/** 
 * @file llfloaternewim.cpp
 * @brief Panel allowing the user to create a new IM session.
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

#include "llfloaternewim.h"

#include "llresmgr.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"

#include "llimview.h"
#include "llmutelist.h"
#include "llnamelistctrl.h"

LLFloaterNewIM::LLFloaterNewIM()
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_new_im.xml");
}

BOOL LLFloaterNewIM::postBuild()
{
	requires<LLButton>("start_btn");
	requires<LLButton>("close_btn");
	requires<LLNameListCtrl>("user_list");
	requires<LLNameListCtrl>("group_list");

	if (checkRequirements())
	{
		childSetAction("start_btn", &LLFloaterNewIM::onStart, this);
		childSetAction("close_btn", &LLFloaterNewIM::onClickClose, this);

		mGroupList = getChild<LLNameListCtrl>("group_list");
		if (mGroupList)
		{
			mGroupList->setCommitOnSelectionChange(TRUE);
			childSetCommitCallback("group_list", onSelectGroup, this);
			mGroupList->setDoubleClickCallback(&LLFloaterNewIM::onStart);
			mGroupList->setCallbackUserData(this);
		}
		else
		{
			llwarns << "LLUICtrlFactory::getNameListByName() returned NULL for 'group_list'" << llendl;
		}

		mAgentList = getChild<LLNameListCtrl>("user_list");
		if (mAgentList)
		{
			mAgentList->setCommitOnSelectionChange(TRUE);
			childSetCommitCallback("user_list", onSelectAgent, this);
			mAgentList->setDoubleClickCallback(&LLFloaterNewIM::onStart);
			mAgentList->setCallbackUserData(this);
		}
		else
		{
			llwarns << "LLUICtrlFactory::getNameListByName() returned NULL for 'user_list'" << llendl;
		}
		setDefaultBtn("start_btn");
		return TRUE;
	}

	return FALSE;
}

LLFloaterNewIM::~LLFloaterNewIM()
{
	clearAllTargets();
}

void LLFloaterNewIM::clearAllTargets()
{
	mGroupList->deleteAllItems();
	mAgentList->deleteAllItems();
}

void LLFloaterNewIM::addGroup(const LLUUID& uuid, void* data)
{
	LLSD row;
	row["id"] = uuid;
	row["target"] = "GROUP";
	row["columns"][0]["value"] = ""; // name will be looked up
	row["columns"][0]["font"] = "SANSSERIF";
	LLMuteList* ml = LLMuteList::getInstance();
	bool muted = ml && ml->isMuted(uuid, LLMute::flagTextChat);
	row["columns"][0]["font-style"] = muted ? "NORMAL" : "BOLD";
	LLScrollListItem* itemp = mGroupList->addElement(row);
	itemp->setUserdata(data);
	itemp->setEnabled(!muted);

	if (mGroupList->getFirstSelectedIndex() == -1)
	{
		mGroupList->selectFirstItem();
	}
}

void LLFloaterNewIM::addAgent(const LLUUID& uuid, void* data, BOOL online)
{
	std::string fullname;
	gCacheName->getFullName(uuid, fullname);

	LLSD row;
	row["id"] = uuid;
	row["columns"][0]["value"] = fullname;
	row["columns"][0]["font"] = "SANSSERIF";
	row["columns"][0]["font-style"] = online ? "BOLD" : "NORMAL";
	LLScrollListItem* itemp = mAgentList->addElement(row);
	itemp->setUserdata(data);

	if (mAgentList->getFirstSelectedIndex() == -1)
	{
		mAgentList->selectFirstItem();
	}
}

//static
void LLFloaterNewIM::onSelectGroup(LLUICtrl*, void* userdata)
{
	LLFloaterNewIM* self = (LLFloaterNewIM*)userdata;

 	LLScrollListItem *item = self->mAgentList->getFirstSelected();
	if (item)
	{
		item->setSelected(FALSE);
	}
}

//static
void LLFloaterNewIM::onSelectAgent(LLUICtrl*, void* userdata)
{
	LLFloaterNewIM* self = (LLFloaterNewIM*)userdata;

 	LLScrollListItem *item = self->mGroupList->getFirstSelected();
	if (item)
	{
		item->setSelected(FALSE);
	}
}

//static
void LLFloaterNewIM::onStart(void* userdata)
{
	if (!gIMMgr) return;

	LLFloaterNewIM* self = (LLFloaterNewIM*)userdata;

	LLScrollListItem* item = self->mGroupList->getFirstSelected();
	if (!item)
	{
		item = self->mAgentList->getFirstSelected();
	}
	if (item)
	{
		const LLScrollListCell* cell = item->getColumn(0);
		std::string name(cell->getValue());

		// *NOTE: Do a live determination of what type of session it should be.
		EInstantMessage type;
		EInstantMessage* t = (EInstantMessage*)item->getUserdata();
		if (t)
		{
			type = (*t);
		}
		else
		{
			type = LLIMMgr::defaultIMTypeForAgent(item->getUUID());
		}
		if (type != IM_SESSION_GROUP_START)
		{
			// Needed to avoid catching a display name, which would
			// make us use a wrong IM log file...
			gCacheName->getFullName(item->getUUID(), name);
		}
		else
		{
			LLMuteList* ml = LLMuteList::getInstance();
			if (ml && ml->isMuted(item->getUUID(), LLMute::flagTextChat))
			{
				make_ui_sound("UISndInvalidOp");
				return;
			}
		}

		gIMMgr->addSession(name, type, item->getUUID());

		make_ui_sound("UISndStartIM");
	}
	else
	{
		make_ui_sound("UISndInvalidOp");
	}
}

// static
void LLFloaterNewIM::onClickClose(void *userdata)
{
	if (gIMMgr)
	{
		gIMMgr->setFloaterOpen(FALSE);
	}
}

BOOL LLFloaterNewIM::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = LLFloater::handleKeyHere(key, mask);
	if (KEY_ESCAPE == key && mask == MASK_NONE)
	{
		handled = TRUE;
		// Close talk panel on escape
		if (gIMMgr)
		{
			gIMMgr->toggle(NULL);
		}
	}

	// Might need to call base class here if not handled
	return handled;
}

BOOL LLFloaterNewIM::canClose()
{
	if (getHost())
	{
		LLMultiFloater* hostp = (LLMultiFloater*)getHost();
		// if we are the only tab in the im view, go ahead and close
		return hostp->getFloaterCount() == 1;
	}
	return TRUE;
}

void LLFloaterNewIM::close(bool app_quitting)
{
	if (getHost())
	{
		LLMultiFloater* hostp = (LLMultiFloater*)getHost();
		hostp->close();
	}
	else
	{
		LLFloater::close(app_quitting);
	}
}

S32 LLFloaterNewIM::getGroupScrollPos()
{
	return mGroupList->getScrollPos();
}

void LLFloaterNewIM::setGroupScrollPos(S32 pos)
{
	mGroupList->setScrollPos(pos);
}

S32 LLFloaterNewIM::getAgentScrollPos()
{
	return mAgentList->getScrollPos();
}

void LLFloaterNewIM::setAgentScrollPos(S32 pos)
{
	mAgentList->setScrollPos(pos);
}
