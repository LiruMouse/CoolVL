/* Copyright (c) 2009
 *
 * Modular Systems Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS LTD AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Modified, debugged, optimized and improved by Henri Beauchamp Feb 2010.
 */

#include "llviewerprecompiledheaders.h"

#include "llui.h"
#include "lluictrlfactory.h"
#include "llscrolllistctrl.h"

#include "llagent.h"
#include "lltracker.h"
#include "llviewerobjectlist.h"
#include "llviewercontrol.h"
#include "jcfloaterareasearch.h"

JCFloaterAreaSearch* JCFloaterAreaSearch::sInstance = NULL;
LLViewerRegion* JCFloaterAreaSearch::sLastRegion = NULL;
S32 JCFloaterAreaSearch::sRequested = 0;
bool JCFloaterAreaSearch::sIsDirty = false;
bool JCFloaterAreaSearch::sTracking = false;
LLUUID JCFloaterAreaSearch::sTrackingObjectID = LLUUID::null;
LLVector3d JCFloaterAreaSearch::sTrackingLocation = LLVector3d();
std::string JCFloaterAreaSearch::sTrackingInfoLine;
std::map<LLUUID, AObjectDetails> JCFloaterAreaSearch::sObjectDetails;
std::string JCFloaterAreaSearch::sSearchedName;
std::string JCFloaterAreaSearch::sSearchedDesc;
std::string JCFloaterAreaSearch::sSearchedOwner;
std::string JCFloaterAreaSearch::sSearchedGroup;

const std::string request_string = "JCFloaterAreaSearch::Requested_ø§µ";
const F32 min_refresh_interval = 0.25f;	// Minimum interval between list refreshes in seconds.

JCFloaterAreaSearch::JCFloaterAreaSearch() :  
	LLFloater(),
	mCounterText(NULL),
	mResultList(NULL)
{
	llassert_always(sInstance == NULL);
	sInstance = this;
	mLastUpdateTimer.reset();
}

JCFloaterAreaSearch::~JCFloaterAreaSearch()
{
	sInstance = NULL;
}

void JCFloaterAreaSearch::close(bool app)
{
	if (app)
	{
		LLFloater::close(app);
	}
	else if (sInstance)
	{
		sInstance->setVisible(FALSE);
	}
}

BOOL JCFloaterAreaSearch::postBuild()
{
	mResultList = getChild<LLScrollListCtrl>("result_list");
	mResultList->setCallbackUserData(this);
	mResultList->setDoubleClickCallback(onDoubleClick);
	mResultList->sortByColumn("Name", TRUE);

	mCounterText = getChild<LLTextBox>("counter");
	if (sTracking)
	{
		mCounterText->setText(sTrackingInfoLine);
	}

	childSetAction("Refresh", search, this);
	childSetAction("Stop", cancel, this);

	childSetKeystrokeCallback("Name query chunk", onCommitLine, 0);
	childSetKeystrokeCallback("Description query chunk", onCommitLine, 0);
	childSetKeystrokeCallback("Owner query chunk", onCommitLine, 0);
	childSetKeystrokeCallback("Group query chunk", onCommitLine, 0);

	return TRUE;
}

void JCFloaterAreaSearch::draw()
{
	if (sTracking)
	{
		F32 dist = 3.0f;
		if (LLTracker::isTracking(NULL) == LLTracker::TRACKING_LOCATION)
		{
			dist = fabsf((F32)(LLTracker::getTrackedPositionGlobal() -
							   sTrackingLocation).magVec());
		}
		if (dist > 2.0f)
		{
			// Tracker stopped or tracking another location
			sTracking = false;
			sIsDirty = true;
			sTrackingInfoLine.clear();
		}
	}

	if (sIsDirty && getVisible() && mResultList &&
		!(sRequested > 0 &&
		  mLastUpdateTimer.getElapsedTimeF32() < min_refresh_interval))
	{
		LLDynamicArray<LLUUID> selected = mResultList->getSelectedIDs();
		S32 scrollpos = mResultList->getScrollPos();
		mResultList->deleteAllItems();
		S32 i;
		S32 total = gObjectList.getNumObjects();

		LLViewerRegion* our_region = gAgent.getRegion();
		for (i = 0; i < total; i++)
		{
			LLViewerObject *objectp = gObjectList.getObject(i);
			if (objectp)
			{
				if (objectp->getRegion() == our_region &&
					!objectp->isAvatar() && objectp->isRoot() &&
					!objectp->flagTemporary() && !objectp->flagTemporaryOnRez())
				{
					LLUUID object_id = objectp->getID();
					if (sObjectDetails.count(object_id) == 0)
					{
						requestIfNeeded(objectp);
					}
					else
					{
						AObjectDetails* details = &sObjectDetails[object_id];
						std::string object_name = details->name;
						std::string object_desc = details->desc;
						std::string object_owner;
						std::string object_group;
						gCacheName->getFullName(details->owner_id, object_owner);
						gCacheName->getGroupName(details->group_id, object_group);
						if (object_name != request_string)
						{
							std::string onU = object_owner;
							std::string cnU = object_group;
							LLStringUtil::toLower(object_name);
							LLStringUtil::toLower(object_desc);
							LLStringUtil::toLower(object_owner);
							LLStringUtil::toLower(object_group);
							if ((sSearchedName.empty() || object_name.find(sSearchedName) != -1) &&
								(sSearchedDesc.empty() || object_desc.find(sSearchedDesc) != -1) &&
								(sSearchedOwner.empty() || object_owner.find(sSearchedOwner) != -1) &&
								(sSearchedGroup.empty() || object_group.find(sSearchedGroup) != -1))
							{
								LLSD element;
								element["id"] = object_id;
								element["columns"][LIST_OBJECT_NAME]["column"] = "Name";
								element["columns"][LIST_OBJECT_NAME]["type"] = "text";
								element["columns"][LIST_OBJECT_NAME]["value"] = details->name;	//item->getName();//ai->second//"avatar_icon";
								element["columns"][LIST_OBJECT_DESC]["column"] = "Description";
								element["columns"][LIST_OBJECT_DESC]["type"] = "text";
								element["columns"][LIST_OBJECT_DESC]["value"] = details->desc;	//ai->second;
								element["columns"][LIST_OBJECT_OWNER]["column"] = "Owner";
								element["columns"][LIST_OBJECT_OWNER]["type"] = "text";
								element["columns"][LIST_OBJECT_OWNER]["value"] = onU;			//ai->first;
								element["columns"][LIST_OBJECT_GROUP]["column"] = "Group";
								element["columns"][LIST_OBJECT_GROUP]["type"] = "text";
								element["columns"][LIST_OBJECT_GROUP]["value"] = cnU;			//ai->second;
								if (sTracking && object_id == sTrackingObjectID)
								{
									element["columns"][LIST_OBJECT_NAME]["font-style"] = "BOLD";
									element["columns"][LIST_OBJECT_DESC]["font-style"] = "BOLD";
									element["columns"][LIST_OBJECT_OWNER]["font-style"] = "BOLD";
									element["columns"][LIST_OBJECT_GROUP]["font-style"] = "BOLD";
								}
								mResultList->addElement(element, ADD_BOTTOM);
							}
						}
					}
				}
			}
		}

		mResultList->sortItems();
		mResultList->selectMultiple(selected);
		mResultList->setScrollPos(scrollpos);
		if (!sTracking)
		{
			mCounterText->setText(llformat("%d listed/%d pending/%d total",
								  mResultList->getItemCount(), sRequested,
								  sObjectDetails.size()));
		}
		mLastUpdateTimer.reset();
		sIsDirty = false;
	}

	LLFloater::draw();
}

// static
void JCFloaterAreaSearch::checkRegion()
{
	// Check if we changed region, and if we did, clear the object details cache.
	LLViewerRegion* region = gAgent.getRegion();
	if (region != sLastRegion)
	{
		sLastRegion = region;
		sRequested = 0;
		sObjectDetails.clear();
		sTracking = false;
		sIsDirty = true;
		if (sInstance)
		{
			sInstance->mResultList->deleteAllItems();
			sInstance->mCounterText->setText(std::string("Listed/Pending/Total"));
		}
	}
}

// static
void JCFloaterAreaSearch::toggle()
{
	if (sInstance)
	{
		if (sInstance->getVisible())
		{
			sInstance->setVisible(FALSE);
		}
		else
		{
			checkRegion();
			sInstance->setVisible(TRUE);
			sInstance->setFrontmost(TRUE);
		}
	}
	else
	{
		sInstance = new JCFloaterAreaSearch();
		LLUICtrlFactory::getInstance()->buildFloater(sInstance,
													 "floater_area_search.xml");
	}
}

// static
void JCFloaterAreaSearch::onDoubleClick(void *userdata)
{
	JCFloaterAreaSearch *self = (JCFloaterAreaSearch*)userdata;
 	LLScrollListItem *item = self->mResultList->getFirstSelected();
	if (!item) return;
	LLUUID object_id = item->getUUID();
	LLViewerObject* objectp = gObjectList.findObject(object_id);
	if (objectp)
	{
		sTrackingObjectID = object_id;
		sTrackingLocation = objectp->getPositionGlobal();
		LLVector3 region_pos = objectp->getPositionRegion();
		sTrackingInfoLine = llformat("Tracking object at position: %d, %d, %d",
									 (S32)region_pos.mV[VX],
									 (S32)region_pos.mV[VY],
									 (S32)region_pos.mV[VZ]);
		self->mCounterText->setText(sTrackingInfoLine);

		LLTracker::trackLocation(sTrackingLocation,
								 sObjectDetails[object_id].name, "",
								 LLTracker::LOCATION_ITEM);
		sTracking = sIsDirty = true;
	}
}

// static
void JCFloaterAreaSearch::cancel(void* data)
{
	checkRegion();
	if (sInstance)
	{
		sInstance->close(TRUE);
	}
	sSearchedName = "";
	sSearchedDesc = "";
	sSearchedOwner = "";
	sSearchedGroup = "";
}

// static
void JCFloaterAreaSearch::search(void* data)
{
	checkRegion();
	setDirty();
}

// static
void JCFloaterAreaSearch::onCommitLine(LLLineEditor* line, void* user_data)
{
	std::string name = line->getName();
	std::string text = line->getText();
	LLStringUtil::toLower(text);
	line->setText(text);
	if (name == "Name query chunk") sSearchedName = text;
	else if (name == "Description query chunk") sSearchedDesc = text;
	else if (name == "Owner query chunk") sSearchedOwner = text;
	else if (name == "Group query chunk") sSearchedGroup = text;

	if (text.length() > 3)
	{
		checkRegion();
		setDirty();
	}
}

// static
void JCFloaterAreaSearch::requestIfNeeded(LLViewerObject *objectp)
{
	LLUUID object_id = objectp->getID();
	if (sObjectDetails.count(object_id) == 0)
	{
		AObjectDetails* details = &sObjectDetails[object_id];
		details->name = request_string;
		details->desc = request_string;
		details->owner_id = LLUUID::null;
		details->group_id = LLUUID::null;

		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_RequestObjectPropertiesFamily);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_RequestFlags, 0 );
		msg->addUUIDFast(_PREHASH_ObjectID, object_id);
		gAgent.sendReliableMessage();
		LL_DEBUGS("AreaSearch") << "Sent data request for object " << object_id
								<< LL_ENDL;
		sRequested++;
	}
}

void callbackLoadOwnerName(const LLUUID&, const std::string&, bool)
{
	JCFloaterAreaSearch::setDirty();
}

// static
void JCFloaterAreaSearch::processObjectPropertiesFamily(LLMessageSystem* msg, void** user_data)
{
	checkRegion();

	LLUUID object_id;
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, object_id);

	bool exists = (sObjectDetails.count(object_id) != 0);
	AObjectDetails* details = &sObjectDetails[object_id];
	if (!exists || details->name == request_string)
	{
		// We cache unknown objects (to avoid having to request them later)
		// and requested objects.
		if (exists && sRequested > 0) sRequested--;
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID, details->owner_id);
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_GroupID, details->group_id);
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, details->name);
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, details->desc);
		gCacheName->get(details->owner_id, false,
						boost::bind(&callbackLoadOwnerName, _1, _2, _3));
		gCacheName->get(details->group_id, true,
						boost::bind(&callbackLoadOwnerName, _1, _2, _3));
		LL_DEBUGS("AreaSearch") << "Got info for "
								<< (exists ? "requested" : "unknown")
								<< " object " << object_id << LL_ENDL;
	}
}
