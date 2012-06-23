/** 
 * @file llfloateravatarinfo.cpp
 * @brief LLFloaterAvatarInfo class implementation
 * Avatar information as shown in a floating window from right-click
 * Profile.  Used for editing your own avatar info.  Just a wrapper
 * for LLPanelAvatar, shared with the Find directory.
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

#include "llfloateravatarinfo.h"

// linden library includes
#include "llinventory.h"
#include "llnotifications.h"
#include "lluictrlfactory.h"
#include "llui.h"				// make_ui_sound()
#include "lluuid.h"
#include "message.h"

// viewer project includes
#include "llagentdata.h"
#include "llcommandhandler.h"
#include "llimview.h"
#include "llinventoryview.h"
#include "llpanelavatar.h"
#include "llviewercontrol.h"
#include "llweb.h"

extern void handle_lure(const LLUUID& invitee);
extern void handle_pay_by_id(const LLUUID& payee);

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

LLMap< const LLUUID, LLFloaterAvatarInfo* > gAvatarInfoInstances;

class LLAgentHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLAgentHandler() : LLCommandHandler("agent", true) { }

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (params.size() < 2) return false;
		LLUUID agent_id;
		if (!agent_id.set(params[0], FALSE))
		{
			return false;
		}

		const std::string verb = params[1].asString();
		if (verb == "about" || verb == "inspect")
		{
			LLFloaterAvatarInfo::show(agent_id);
			return true;
		}
		else if (verb == "pay")
		{
			handle_pay_by_id(agent_id);
			return true;
		}
		else if (verb == "offerteleport")
		{
			handle_lure(agent_id);
			return true;
		}
		else if (verb == "im")
		{
			std::string name;
			if (gIMMgr && gCacheName->getFullName(agent_id, name))
			{
				gIMMgr->setFloaterOpen(TRUE);
				gIMMgr->addSession(name, IM_NOTHING_SPECIAL, agent_id);
				make_ui_sound("UISndStartIM");
			}
			return true;
		}
		return false;
	}
};
LLAgentHandler gAgentHandler;

class LLShareWithAvatarHandler : public LLCommandHandler
{
public: 
	// requires trusted browser to trigger
	LLShareWithAvatarHandler() : LLCommandHandler("sharewithavatar", true) { }

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		//Make sure we have some parameters
		if (params.size() == 0)
		{
			return false;
		}

		//Get the ID
		LLUUID id;
		if (!id.set(params[0], FALSE))
		{
			return false;
		}

		// Select the 2nd Life tab in the profile panel.
		LLFloaterAvatarInfo::showFromObject(id, "2nd Life");
		// Open the inventory floater and/or bring it to front
		LLInventoryView::showAgentInventory(TRUE);
		// Give some clue to the user as what to do now
		LLNotifications::instance().add("ShareInventory");
		return true;
	}
};
LLShareWithAvatarHandler gShareWithAvatar;

class LLPickHandler : public LLCommandHandler
{
public: 
	// requires trusted browser to trigger
	LLPickHandler() : LLCommandHandler("pick", true) { }

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		//Make sure we have some parameters
		if (params.size() == 0)
		{
			return false;
		}

		// *TODO: implement pick selection by UUID (and move to
		// llpanelpick.cpp ?).
		// For now, simply select the Picks tab in the profile panel.
		llinfos << "STUB code for URI secondlife://app/pick/ - Selecting Picks tab in avatar profile." << llendl;
		LLFloaterAvatarInfo::showFromObject(gAgentID, "Picks");
		return true;
	}
};
LLPickHandler gPickHandler;

//-----------------------------------------------------------------------------
// Member functions
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------

void* LLFloaterAvatarInfo::createPanelAvatar(void* data)
{
	LLFloaterAvatarInfo* self = (LLFloaterAvatarInfo*)data;
	self->mPanelAvatarp = new LLPanelAvatar("PanelAv", LLRect(), TRUE); // allow edit self
	return self->mPanelAvatarp;
}

//----------------------------------------------------------------------------

BOOL LLFloaterAvatarInfo::postBuild()
{
	return TRUE;
}

LLFloaterAvatarInfo::LLFloaterAvatarInfo(const LLUUID &avatar_id)
:	LLPreview("avatarinfo"),
	mAvatarID(avatar_id),
	mSuggestedOnlineStatus(ONLINE_STATUS_NO)
{
	setAutoFocus(TRUE);

	LLCallbackMap::map_t factory_map;

	factory_map["Panel Avatar"] = LLCallbackMap(createPanelAvatar, this);

	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_profile.xml", &factory_map);

	if(mPanelAvatarp)
	{
		mPanelAvatarp->selectTab(0);
	}

	gAvatarInfoInstances.addData(avatar_id, this); // must be done before callback below is called.
	LLAvatarNameCache::get(avatar_id, boost::bind(&LLFloaterAvatarInfo::callbackLoadAvatarName, _1, _2));
}

// virtual
LLFloaterAvatarInfo::~LLFloaterAvatarInfo()
{
	// child views automatically deleted
	gAvatarInfoInstances.removeData(mAvatarID);
}

void LLFloaterAvatarInfo::resetGroupList()
{
	// only get these updates asynchronously via the group floater, which works on the agent only
	if (mAvatarID != gAgentID)
	{
		return;
	}

	mPanelAvatarp->resetGroupList();
}

// static
LLFloaterAvatarInfo* LLFloaterAvatarInfo::show(const LLUUID &avatar_id)
{
	if (avatar_id.isNull())
	{
		return NULL;
	}

	LLFloaterAvatarInfo *floater;
	if (gAvatarInfoInstances.checkData(avatar_id))
	{
		// ...bring that window to front
		floater = gAvatarInfoInstances.getData(avatar_id);
		floater->open();	/*Flawfinder: ignore*/
	}
	else
	{
		floater =  new LLFloaterAvatarInfo(avatar_id);
		floater->open();	/*Flawfinder: ignore*/
	}
	return floater;
}

// Open profile to a certain tab.
// static
void LLFloaterAvatarInfo::showFromObject(const LLUUID& avatar_id, std::string tab_name)
{
	LLFloaterAvatarInfo *floater = show(avatar_id);
	if (floater)
	{
		floater->mPanelAvatarp->setAvatarID(avatar_id, LLStringUtil::null, ONLINE_STATUS_NO);
		floater->mPanelAvatarp->selectTabByName(tab_name);
	}
}

// static
void LLFloaterAvatarInfo::showFromDirectory(const LLUUID &avatar_id)
{
	LLFloaterAvatarInfo *floater = show(avatar_id);
	if (floater)
	{
		floater->mPanelAvatarp->setAvatarID(avatar_id, LLStringUtil::null, ONLINE_STATUS_NO);
	}
}

// static
void LLFloaterAvatarInfo::showFromFriend(const LLUUID& agent_id, BOOL online)
{
	LLFloaterAvatarInfo *floater = show(agent_id);
	if (floater)
	{
		floater->mSuggestedOnlineStatus = online ? ONLINE_STATUS_YES : ONLINE_STATUS_NO;
	}
}

// static
void LLFloaterAvatarInfo::showFromProfile(const LLUUID &avatar_id, LLRect rect)
{
	if (avatar_id.isNull())
	{
		return;
	}

	LLFloaterAvatarInfo *floater;
	if (gAvatarInfoInstances.checkData(avatar_id))
	{
		// ...bring that window to front
		floater = gAvatarInfoInstances.getData(avatar_id);
	}
	else
	{
		floater =  new LLFloaterAvatarInfo(avatar_id);
		floater->translate(rect.mLeft - floater->getRect().mLeft + 16,
						   rect.mTop - floater->getRect().mTop - 16);
		floater->mPanelAvatarp->setAvatarID(avatar_id, LLStringUtil::null, ONLINE_STATUS_NO);
	}
	if (floater)
	{
		floater->open();
	}
}

void LLFloaterAvatarInfo::showProfileCallback(S32 option, void *userdata)
{
	if (option == 0)
	{
		showFromObject(gAgentID);
	}
}

// static
void LLFloaterAvatarInfo::callbackLoadAvatarName(const LLUUID& id,
												 const LLAvatarName& avatar_name)
{
	LLFloaterAvatarInfo *floater = gAvatarInfoInstances.getIfThere(id);

	if (floater)
	{
		// Build a new title including the avatar name.
		std::ostringstream title;
		if (LLAvatarNameCache::useDisplayNames())
		{
			// Always show "Display Name [Legacy Name]" for security reasons
			title << avatar_name.getNames() << " - " << floater->getTitle();
		}
		else
		{
			title << avatar_name.getLegacyName() << " - " << floater->getTitle();
		}
		floater->setTitle(title.str());
	}
}

//// virtual
void LLFloaterAvatarInfo::draw()
{
	// skip LLPreview::draw()
	LLFloater::draw();
}

// virtual
BOOL LLFloaterAvatarInfo::canClose()
{
	return mPanelAvatarp && mPanelAvatarp->canClose();
}

LLFloaterAvatarInfo* LLFloaterAvatarInfo::getInstance(const LLUUID &id)
{
	return gAvatarInfoInstances.getIfThere(gAgentID);
}

void LLFloaterAvatarInfo::loadAsset()
{
	if (mPanelAvatarp)
	{
		mPanelAvatarp->setAvatarID(mAvatarID, LLStringUtil::null, mSuggestedOnlineStatus);
		mAssetStatus = PREVIEW_ASSET_LOADING;
	}
}

LLPreview::EAssetStatus LLFloaterAvatarInfo::getAssetStatus()
{
	if (mPanelAvatarp && mPanelAvatarp->haveData())
	{
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
	return mAssetStatus;
}

std::string getProfileURL(const std::string& user_name)
{
	std::string url = gSavedSettings.getString("WebProfileURL");
	LLStringUtil::format_map_t subs;
	subs["[AGENT_NAME]"] = user_name;
	url = LLWeb::expandURLSubstitutions(url, subs);
	LLStringUtil::toLower(url);
	return url;
}
