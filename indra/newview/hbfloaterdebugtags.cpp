/** 
 * @file hbfloaterdebugtags.cpp
 * @brief The HBFloaterDebugTags class definition
 *
 * $LicenseInfo:firstyear=2012&license=viewergpl$
 * 
 * Copyright (c) 2012, Henri Beauchamp
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

#include "hbfloaterdebugtags.h"

#include "lldir.h"
#include "llerrorcontrol.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "lluictrlfactory.h" 

HBFloaterDebugTags* HBFloaterDebugTags::sInstance = NULL;
std::set<std::string> HBFloaterDebugTags::sAddedTagsList;

HBFloaterDebugTags::HBFloaterDebugTags()
:	LLFloater(std::string("debug tags"))
{
    LLUICtrlFactory::getInstance()->buildFloater(this, "floater_debug_tags.xml");
}

//virtual
HBFloaterDebugTags::~HBFloaterDebugTags()
{
    sInstance = NULL;
}

//virtual
BOOL HBFloaterDebugTags::postBuild()
{
	mDebugTagsList = getChild<LLScrollListCtrl>("tags_list", TRUE, FALSE);
	if (!mDebugTagsList)
	{
		llwarns << "Could not find the necessary UI elements !" << llendl;
		return FALSE;
	}

	childSetCommitCallback("tags_list", onSelectLine, this);

	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,
														  "debug_tags.xml");
	llifstream file;
	file.open(filename.c_str());
	if (file.is_open())
	{
		LLSD list;
		llinfos << "Loading the debug tags list from: " << filename << llendl;
		LLSDSerialize::fromXML(list, file);
		S32 id = 0;
		std::string tag;
		while (id < (S32)list.size())
		{
			bool has_tag = false;
			bool has_ref = false;
			bool has_other = false;
			LLSD data = list[id];
			if (data.has("columns"))
			{
				for (S32 i = 0; i < (S32)data["columns"].size(); i++)
				{
					LLSD map = data["columns"][i];
					if (map.has("column"))
					{
						if (map["column"].asString() == "tag")
						{
							has_tag = true;
							tag = map.get("value").asString();
						}
						else if (map["column"].asString() == "references")
						{
							has_ref = true;
						}
						else
						{
							has_other = true;
						}
					}
					else
					{
						has_other = true;	// Make sure the entry will be removed
						break;
					}
				}
			}
			if (!has_other && has_tag && has_ref)
			{
				data["columns"][2]["column"] = "active";
				data["columns"][2]["type"] = "checkbox";
				data["columns"][2]["value"] = sAddedTagsList.count(tag) ? TRUE : FALSE;
				mDebugTagsList->addElement(data, ADD_BOTTOM);
				mDebugTagsList->deselectAllItems(TRUE);
				id++;
			}
			else
			{
				list.erase(id);
			}
		}
		file.close();
	}

	return TRUE;
}

//static
void HBFloaterDebugTags::showInstance()
{
	if (!sInstance)
	{
		sInstance = new HBFloaterDebugTags();
 	}
	sInstance->open();
}

//static
void HBFloaterDebugTags::onSelectLine(LLUICtrl* ctrl, void* data)
{
	HBFloaterDebugTags* self = (HBFloaterDebugTags*)data;
	if (self && self->mDebugTagsList)
	{
		LLScrollListItem* item = self->mDebugTagsList->getFirstSelected();
		if (item)
		{
			BOOL selected = item->getColumn(0)->getValue().asBoolean();
			std::string tag = item->getColumn(1)->getValue().asString();
			if (sAddedTagsList.count(tag))
			{
				if (!selected)
				{
					llinfos << "Removing LL_DEBUGS tag \"" << tag
							<< "\" from logging controls" << llendl;
					sAddedTagsList.erase(tag);

					std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,
																		  "logcontrol.xml");
					LLSD configuration;
					llifstream file;
					file.open(filename.c_str());
					if (file.is_open())
					{
						LLSDSerialize::fromXML(configuration, file);
						file.close();
					}
					LLError::configure(configuration);
					for (std::set<std::string>::iterator it = sAddedTagsList.begin();
						 it != sAddedTagsList.end(); it++)
					{
						LLError::setTagLevel(*it, LLError::LEVEL_DEBUG);
					}
				}
			}
			else
			{
				if (selected)
				{
					llinfos << "Adding LL_DEBUGS tag \"" << tag
							<< "\" to logging controls" << llendl;
					sAddedTagsList.insert(tag);

					LLError::setTagLevel(tag, LLError::LEVEL_DEBUG);
				}
			}
		}
	}
}

