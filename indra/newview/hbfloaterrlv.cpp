/** 
 * @file hbfloaterrlv.cpp
 * @brief The HBFloaterRLV and HBFloaterBlacklistRLV classes definitions
 *
 * $LicenseInfo:firstyear=2011&license=viewergpl$
 * 
 * Copyright (c) 2011, Henri Beauchamp
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

#include "hbfloaterrlv.h"

#include "llcheckboxctrl.h"
#include "llinventory.h"
#include "llscrolllistctrl.h"
#include "lluictrlfactory.h" 

#include "llagent.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"

/*
 * HBFloaterRLV class
 */

HBFloaterRLV* HBFloaterRLV::sInstance = NULL;

HBFloaterRLV::HBFloaterRLV()
:	LLFloater(std::string("restrained love")),
	mIsDirty(false)
{
    LLUICtrlFactory::getInstance()->buildFloater(this, "floater_rlv_restrictions.xml");
}

//virtual
HBFloaterRLV::~HBFloaterRLV()
{
    sInstance = NULL;
}

//virtual
BOOL HBFloaterRLV::postBuild()
{
	mRestrictions = getChild<LLScrollListCtrl>("restrictions_list");

	if (mRestrictions)
	{
		childSetAction("refresh", onButtonRefresh, NULL);
		childSetAction("close", onButtonClose, this);
		mIsDirty = true;
	}

	return TRUE;
}

void set_element(LLSD& element, LLUUID& uuid, std::string& restrictions)
{
	std::string object_name;
	LLInventoryItem* item = NULL;
	LLViewerObject* object = gObjectList.findObject(uuid);
	bool is_missing = (object == NULL);
	bool is_root = true;
	if (!is_missing)
	{
		is_root = (object == object->getRootEdit());
		item = gAgent.mRRInterface.getItem(uuid);
	}
	if (!is_missing && item)
	{
		// Attached object
		object_name = item->getName();
		if (is_root)
		{
			element["columns"][0]["font-style"] = "BOLD";
		}
		else
		{
			element["columns"][0]["font-style"] = "NORMAL";
		}
	}
	else
	{
		// In-world (it should normally have used a relay !), or missing object
		object_name = uuid.asString();
		if (is_missing)
		{
			element["columns"][0]["color"] = LLColor4::red2.getValue();
		}
		else if (is_root)
		{
			element["columns"][0]["font-style"] = "BOLD|ITALIC";
		}
		else
		{
			element["columns"][0]["font-style"] = "NORMAL|ITALIC";
		}
	}
	element["columns"][0]["column"] = "object_name";
	element["columns"][0]["font"] = "SANSSERIF_SMALL";
	element["columns"][0]["value"] = object_name;

	element["columns"][1]["column"] = "restrictions";
	element["columns"][1]["font"] = "SANSSERIF_SMALL";
	element["columns"][1]["font-style"] = "NORMAL";
	element["columns"][1]["value"] = restrictions;
}

//virtual
void HBFloaterRLV::draw()
{
	if (mIsDirty && mRestrictions)
	{
		S32 scrollpos = mRestrictions->getScrollPos();
		mRestrictions->deleteAllItems();

		if (!gAgent.mRRInterface.mSpecialObjectBehaviours.empty())
		{
			LLUUID uuid(LLUUID::null), old_uuid(LLUUID::null);
			std::string restrictions;

			bool first = true;
			RRMAP::iterator it = gAgent.mRRInterface.mSpecialObjectBehaviours.begin();
			while (it != gAgent.mRRInterface.mSpecialObjectBehaviours.end())
			{
				old_uuid = uuid;
				uuid = LLUUID(it->first);
				if (!first && uuid != old_uuid)
				{
					LLSD element;
					set_element(element, old_uuid, restrictions);
					mRestrictions->addElement(element, ADD_BOTTOM);
					restrictions = "";
				}
				if (!restrictions.empty())
				{
					restrictions += ",";
				}
				restrictions += it->second;
				it++;
				first = false;
			}
			LLSD element;
			set_element(element, uuid, restrictions);
			mRestrictions->addElement(element, ADD_BOTTOM);
		}

		mRestrictions->setScrollPos(scrollpos);
		mIsDirty = false;
	}

	LLFloater::draw();
}

//static
void HBFloaterRLV::showInstance()
{
	if (!sInstance)
	{
		sInstance = new HBFloaterRLV();
 	}
	sInstance->open();
}

//static
void HBFloaterRLV::setDirty()
{
    if (sInstance)
	{
		sInstance->mIsDirty = true;
	}
}

//static
void HBFloaterRLV::onButtonRefresh(void* data)
{
	HBFloaterRLV* self = (HBFloaterRLV*)data;
	if (self)
	{
		gAgent.mRRInterface.garbageCollector(false);
		self->setDirty();
	}
}

//static
void HBFloaterRLV::onButtonClose(void* data)
{
	HBFloaterRLV* self = (HBFloaterRLV*)data;
	if (self)
	{
		self->close();
	}
}

/*
 * HBFloaterBlacklistRLV class
 */

HBFloaterBlacklistRLV* HBFloaterBlacklistRLV::sInstance = NULL;

HBFloaterBlacklistRLV::HBFloaterBlacklistRLV()
:	LLFloater(std::string("restrained love blacklist"))
{
    LLUICtrlFactory::getInstance()->buildFloater(this, "floater_rlv_blacklist.xml");
}

//virtual
HBFloaterBlacklistRLV::~HBFloaterBlacklistRLV()
{
    sInstance = NULL;
}

std::string getCommands(S32 type, bool csv = false)
{
	std::string commands = gAgent.mRRInterface.getCommandsByType(type, true);
	if (!commands.empty())
	{
		commands = commands.substr(1);
		if (csv)
		{
			commands = "," + commands;
			LLStringUtil::replaceString(commands, "/", ",");
		}
		else
		{
			commands = "@" + commands;
			LLStringUtil::replaceString(commands, "/", ", @");
			LLStringUtil::replaceString(commands, "%f", "=force");
		}
	}
	return commands;
}

BOOL isGroupInBlacklist(S32 type)
{
	std::string blacklist = gSavedSettings.getString("RestrainedLoveBlacklist");
	blacklist = "," + blacklist + ",";
	RRInterface::rr_command_map_t::iterator it;
	for (it = RRInterface::sCommandsMap.begin();
		 it != RRInterface::sCommandsMap.end(); it++) {
		if ((S32)it->second == type &&
			blacklist.find("," + it->first + ",") == std::string::npos)
		{
			return FALSE;
		}
	}
	return TRUE;
}

//virtual
BOOL HBFloaterBlacklistRLV::postBuild()
{
	childSetAction("apply", onButtonApply, this);
	childSetAction("cancel", onButtonCancel, this);

	// Tool tips creation:
	std::string prefix = getString("tool_tip_prefix") + " ";

	std::string tooltip = getCommands(RRInterface::RR_INSTANTMESSAGE);
	LLCheckBoxCtrl* check = getChild<LLCheckBoxCtrl>("instantmessage");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_INSTANTMESSAGE));

	tooltip = getCommands(RRInterface::RR_CHANNEL);
	check = getChild<LLCheckBoxCtrl>("channel");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_CHANNEL));

	tooltip = getCommands(RRInterface::RR_SENDCHAT);
	check = getChild<LLCheckBoxCtrl>("sendchat");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_SENDCHAT));

	tooltip = getCommands(RRInterface::RR_RECEIVECHAT);
	check = getChild<LLCheckBoxCtrl>("receivechat");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_RECEIVECHAT));

	tooltip = getCommands(RRInterface::RR_EMOTE);
	check = getChild<LLCheckBoxCtrl>("emote");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_EMOTE));

	tooltip = getCommands(RRInterface::RR_REDIRECTION);
	check = getChild<LLCheckBoxCtrl>("redirection");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_REDIRECTION));

	tooltip = getCommands(RRInterface::RR_MOVE);
	check = getChild<LLCheckBoxCtrl>("move");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_MOVE));

	tooltip = getCommands(RRInterface::RR_SIT);
	check = getChild<LLCheckBoxCtrl>("sit");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_SIT));

	tooltip = getCommands(RRInterface::RR_TELEPORT);
	check = getChild<LLCheckBoxCtrl>("teleport");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_TELEPORT));

	tooltip = getCommands(RRInterface::RR_TOUCH);
	check = getChild<LLCheckBoxCtrl>("touch");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_TOUCH));

	tooltip = getCommands(RRInterface::RR_INVENTORY);
	check = getChild<LLCheckBoxCtrl>("inventory");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_INVENTORY));

	tooltip = getCommands(RRInterface::RR_INVENTORYLOCK);
	check = getChild<LLCheckBoxCtrl>("inventorylock");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_INVENTORYLOCK));

	tooltip = getCommands(RRInterface::RR_LOCK);
	check = getChild<LLCheckBoxCtrl>("lock");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_LOCK));

	tooltip = getCommands(RRInterface::RR_BUILD);
	check = getChild<LLCheckBoxCtrl>("build");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_BUILD));

	tooltip = getCommands(RRInterface::RR_ATTACH);
	check = getChild<LLCheckBoxCtrl>("attach");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_ATTACH));

	tooltip = getCommands(RRInterface::RR_DETACH);
	check = getChild<LLCheckBoxCtrl>("detach");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_DETACH));

	tooltip = getCommands(RRInterface::RR_NAME);
	check = getChild<LLCheckBoxCtrl>("name");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_NAME));

	tooltip = getCommands(RRInterface::RR_LOCATION);
	check = getChild<LLCheckBoxCtrl>("location");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_LOCATION));

	tooltip = getCommands(RRInterface::RR_GROUP);
	check = getChild<LLCheckBoxCtrl>("group");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_GROUP));

	tooltip = getCommands(RRInterface::RR_DEBUG);
	check = getChild<LLCheckBoxCtrl>("debug");
	check->setToolTip(prefix + tooltip);
	check->set(isGroupInBlacklist(RRInterface::RR_DEBUG));

	return TRUE;
}

//virtual
void HBFloaterBlacklistRLV::draw()
{
	LLFloater::draw();
}

//static
void HBFloaterBlacklistRLV::showInstance()
{
	if (!sInstance)
	{
		sInstance = new HBFloaterBlacklistRLV();
 	}
	sInstance->open();
	sInstance->center();
}

//static
void HBFloaterBlacklistRLV::onButtonApply(void* data)
{
	HBFloaterBlacklistRLV* self = (HBFloaterBlacklistRLV*)data;
	if (self)
	{
		std::string blacklist;
		LLCheckBoxCtrl* check = self->getChild<LLCheckBoxCtrl>("instantmessage");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_INSTANTMESSAGE, true);
		}
		
		check = self->getChild<LLCheckBoxCtrl>("channel");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_CHANNEL, true);
		}
		
		check = self->getChild<LLCheckBoxCtrl>("sendchat");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_SENDCHAT, true);
		}
		
		check = self->getChild<LLCheckBoxCtrl>("receivechat");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_RECEIVECHAT, true);
		}
		
		check = self->getChild<LLCheckBoxCtrl>("emote");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_EMOTE, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("redirection");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_REDIRECTION, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("move");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_MOVE, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("sit");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_SIT, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("teleport");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_TELEPORT, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("touch");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_TOUCH, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("inventory");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_INVENTORY, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("inventorylock");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_INVENTORYLOCK, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("lock");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_LOCK, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("build");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_BUILD, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("attach");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_ATTACH, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("detach");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_DETACH, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("name");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_NAME, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("location");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_LOCATION, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("group");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_GROUP, true);
		}

		check = self->getChild<LLCheckBoxCtrl>("debug");
		if (check->get())
		{
			blacklist += getCommands(RRInterface::RR_DEBUG, true);
		}

		if (!blacklist.empty())
		{
			blacklist = blacklist.substr(1);
		}
		gSavedSettings.setString("RestrainedLoveBlacklist",	blacklist);

		self->close();
	}
}

//static
void HBFloaterBlacklistRLV::onButtonCancel(void* data)
{
	HBFloaterBlacklistRLV* self = (HBFloaterBlacklistRLV*)data;
	if (self)
	{
		self->close();
	}
}

//static
void HBFloaterBlacklistRLV::closeInstance()
{
	if (sInstance)
	{
		sInstance->close();
	}
}
