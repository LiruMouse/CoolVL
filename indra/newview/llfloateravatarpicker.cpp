/**
 * @file llfloateravatarpicker.cpp
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

#include "llfloateravatarpicker.h"

#include "llbutton.h"
#include "llcachename.h"
#include "llfocusmgr.h"
#include "llhttpclient.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "message.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"		// getCapability()
#include "llworld.h"

// static
LLFloaterAvatarPicker* LLFloaterAvatarPicker::sInstance = NULL;

class LLAvatarPickerResponder : public LLHTTPClient::Responder
{
public:
	LLUUID mQueryID;

	LLAvatarPickerResponder(const LLUUID& id) : mQueryID(id) {}

	//virtual
	void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		// In case of invalid characters, the avatar picker returns a 400
		// just set it to process so it displays 'not found'
		if (isGoodStatus(status) || status == 400)
		{
			LLFloaterAvatarPicker* floater = LLFloaterAvatarPicker::getInstance();
			if (floater)
			{
				floater->processResponse(mQueryID, content);
			}
		}
		else
		{
			llinfos << "Avatar picker request failed with satus: " << status
					<< " - reason: " << reason << llendl;
		}
	}
};

LLFloaterAvatarPicker::LLFloaterAvatarPicker()
:	LLFloater(std::string("avatarpicker")),
	mCallback(NULL),
	mCallbackUserdata(NULL),
	mResidentChooserTabs(NULL),
	mSearchPanel(NULL),
	mFriendsPanel(NULL),
	mCallingCardsPanel(NULL),
	mNearMePanel(NULL),
	mSearchResults(NULL),
	mFriends(NULL),
	mNearMe(NULL),
	mInventoryPanel(NULL),
	mSelect(NULL),
	mFind(NULL),
	mEdit(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_avatar_picker.xml");
}

LLFloaterAvatarPicker::~LLFloaterAvatarPicker()
{
	gFocusMgr.releaseFocusIfNeeded(this);
	sInstance = NULL;
}

BOOL LLFloaterAvatarPicker::postBuild()
{
	mEdit = getChild<LLLineEditor>("Edit");
	mEdit->setKeystrokeCallback(editKeystroke);
	mEdit->setCallbackUserData(this);
	mEdit->setFocus(TRUE);

	mFind = getChild<LLButton>("Find");
	mFind->setClickedCallback(onBtnFind, this);
	mFind->setEnabled(FALSE);

	childSetAction("Refresh", onBtnRefresh, this);

	childSetCommitCallback("near_me_range", onRangeAdjust, this);

	mSelect = getChild<LLButton>("Select");
	mSelect->setClickedCallback(onBtnSelect, this);
	mSelect->setEnabled(FALSE);

	childSetAction("Close", onBtnClose, this);

	mSearchResults = getChild<LLScrollListCtrl>("SearchResults");
	mSearchResults->setDoubleClickCallback(onBtnSelect);
	mSearchResults->setCommitCallback(onList);
	mSearchResults->setCallbackUserData(this);
	mSearchResults->setEnabled(FALSE);
	mSearchResults->addCommentText(getString("no_result"));

	mFriends = getChild<LLScrollListCtrl>("Friends");
	mFriends->setDoubleClickCallback(onBtnSelect);
	mFriends->setCommitCallback(onList);
	mFriends->setCallbackUserData(this);

	mNearMe = getChild<LLScrollListCtrl>("NearMe");
	mNearMe->setDoubleClickCallback(onBtnSelect);
	mNearMe->setCommitCallback(onList);
	mNearMe->setCallbackUserData(this);

	mInventoryPanel = getChild<LLInventoryPanel>("InventoryPanel");
	mInventoryPanel->setFilterTypes(0x1 << LLInventoryType::IT_CALLINGCARD);
	mInventoryPanel->setFollowsAll();
	mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInventoryPanel->openDefaultFolderForType(LLAssetType::AT_CALLINGCARD);
	mInventoryPanel->setSelectCallback(onCallingCardSelectionChange, this);

	mSearchPanel = getChild<LLPanel>("SearchPanel");
	mSearchPanel->setDefaultBtn(mFind);
	mFriendsPanel = getChild<LLPanel>("FriendsPanel");
	mCallingCardsPanel = getChild<LLPanel>("CallingCardsPanel");
	mNearMePanel = getChild<LLPanel>("NearMePanel");

	mResidentChooserTabs = getChild<LLTabContainer>("ResidentChooserTabs");
	mResidentChooserTabs->setTabChangeCallback(mSearchPanel, onTabChanged);
	mResidentChooserTabs->setTabUserData(mSearchPanel, this);
	mResidentChooserTabs->setTabChangeCallback(mFriendsPanel, onTabChanged);
	mResidentChooserTabs->setTabUserData(mFriendsPanel, this);
	mResidentChooserTabs->setTabChangeCallback(mCallingCardsPanel, onTabChanged);
	mResidentChooserTabs->setTabUserData(mCallingCardsPanel, this);
	mResidentChooserTabs->setTabChangeCallback(mNearMePanel, onTabChanged);
	mResidentChooserTabs->setTabUserData(mNearMePanel, this);

	setAllowMultiple(FALSE);

	populateFriends();

	return TRUE;
}

//static
LLFloaterAvatarPicker* LLFloaterAvatarPicker::show(callback_t callback,
												   void* userdata,
												   BOOL allow_multiple,
												   BOOL closeOnSelect)
{
	// TODO: This class should not be a singleton as it's used in multiple
	// places and therefore can't be used simultaneously. -MG
	if (!sInstance)
	{
		sInstance = new LLFloaterAvatarPicker();
		sInstance->mCallback = callback;
		sInstance->mCallbackUserdata = userdata;
		sInstance->mCloseOnSelect = FALSE;

		sInstance->open();	/* Flawfinder: ignore */
		sInstance->center();
		sInstance->setAllowMultiple(allow_multiple);
	}
	else
	{
		sInstance->open();	/*Flawfinder: ignore*/
		sInstance->mCallback = callback;
		sInstance->mCallbackUserdata = userdata;
		sInstance->setAllowMultiple(allow_multiple);
	}

	sInstance->mNearMeListComplete = false;
	sInstance->mCloseOnSelect = closeOnSelect;
	return sInstance;
}

//virtual
void LLFloaterAvatarPicker::draw()
{
	if (!mNearMeListComplete &&
		mResidentChooserTabs->getCurrentPanel() == mNearMePanel)
	{
		populateNearMe();
	}
	LLFloater::draw();
}

//virtual
BOOL LLFloaterAvatarPicker::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		if (mEdit->hasFocus())
		{
			onBtnFind(this);
		}
		else
		{
			onBtnSelect(this);
		}
		return TRUE;
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		close();
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}

// Callback for inventory picker (select from calling cards)
void LLFloaterAvatarPicker::doCallingCardSelectionChange(const std::deque<LLFolderViewItem*>& items,
														 BOOL user_action,
														 void* data)
{
	bool panel_active = (mResidentChooserTabs->getCurrentPanel() == mCallingCardsPanel);

	mSelectedInventoryAvatarIDs.clear();
	mSelectedInventoryAvatarNames.clear();

	if (panel_active)
	{
		mSelect->setEnabled(FALSE);
	}

	std::deque<LLFolderViewItem*>::const_iterator item_it;
	for (item_it = items.begin(); item_it != items.end(); ++item_it)
	{
		LLFolderViewEventListener* listenerp = (*item_it)->getListener();
		if (listenerp->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
		{
			LLInventoryItem* item = gInventory.getItem(listenerp->getUUID());
			if (item)
			{
				mSelectedInventoryAvatarIDs.push_back(item->getCreatorUUID());
				mSelectedInventoryAvatarNames.push_back(listenerp->getName());
			}
		}
	}

	if (panel_active)
	{
		mSelect->setEnabled(visibleItemsSelected());
	}
}

void LLFloaterAvatarPicker::populateNearMe()
{
	bool all_loaded = true;
	bool empty = true;

	mNearMe->deleteAllItems();

//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
	{
		return;
	}
//mk

	std::vector<LLUUID> avatar_ids;
	LLWorld::getInstance()->getAvatars(&avatar_ids, NULL,
									   gAgent.getPositionGlobal(),
									   gSavedSettings.getF32("NearMeRange"));
	for (U32 i = 0, count = avatar_ids.size(); i < count; ++i)
	{
		LLUUID& av = avatar_ids[i];
		if (av == gAgentID) continue;
		LLSD element;
		element["id"] = av; // value
		std::string fullname;
		if (!gCacheName->getFullName(av, fullname))
		{
			element["columns"][0]["value"] = LLCacheName::getDefaultName();
			all_loaded = false;
		}
		else
		{
			element["columns"][0]["value"] = fullname;
		}
		mNearMe->addElement(element);
		empty = false;
	}

	if (empty)
	{
		mNearMe->setEnabled(FALSE);
		mSelect->setEnabled(FALSE);
		mNearMe->addCommentText(getString("no_one_near"));
	}
	else
	{
		mNearMe->setEnabled(TRUE);
		mSelect->setEnabled(TRUE);
		mNearMe->selectFirstItem();
		onList(mNearMe, this);
		mNearMe->setFocus(TRUE);
	}

	if (all_loaded)
	{
		mNearMeListComplete = true;
	}
}

void LLFloaterAvatarPicker::populateFriends()
{
	mFriends->deleteAllItems();
	LLCollectAllBuddies collector;
	LLAvatarTracker::instance().applyFunctor(collector);
	LLCollectAllBuddies::buddy_map_t::iterator it;
	
	for (it = collector.mOnline.begin(); it != collector.mOnline.end(); ++it)
	{
		mFriends->addStringUUIDItem(it->first, it->second);
	}
	for (it = collector.mOffline.begin(); it != collector.mOffline.end(); ++it)
	{
		mFriends->addStringUUIDItem(it->first, it->second);
	}
	mFriends->sortByColumnIndex(0, TRUE);
}

BOOL LLFloaterAvatarPicker::visibleItemsSelected() const
{
	LLPanel* active_panel = mResidentChooserTabs->getCurrentPanel();

	if (active_panel == mSearchPanel)
	{
		return mSearchResults->getFirstSelectedIndex() >= 0;
	}
	else if (active_panel == mFriendsPanel)
	{
		return mFriends->getFirstSelectedIndex() >= 0;
	}
	else if (active_panel == mCallingCardsPanel)
	{
		return mSelectedInventoryAvatarIDs.size() > 0;
	}
	else if (active_panel == mNearMePanel)
	{
		return mNearMe->getFirstSelectedIndex() >= 0;
	}
	return FALSE;
}

void LLFloaterAvatarPicker::find()
{
	const std::string& text = mEdit->getValue().asString();

	mQueryID.generate();

	std::string url;
	url.reserve(128); // avoid a memory allocation or two

	LLViewerRegion* region = gAgent.getRegion();
	if (region)	// Paranoia
	{
		url = region->getCapability("AvatarPickerSearch");
	}
	if (!url.empty() && LLAvatarNameCache::useDisplayNames())
	{
		// Capability urls don't end in '/', but we need one to parse query
		// parameters correctly
		if (url.size() > 0 && url[url.size()-1] != '/')
		{
			url += "/";
		}
		url += "?page_size=100&names=";
		url += LLURI::escape(text);
		llinfos << "Avatar picker request: " << url << llendl;
		LLHTTPClient::get(url, new LLAvatarPickerResponder(mQueryID));
	}
	else
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("AvatarPickerRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgentID);
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->addUUID("QueryID", mQueryID);	// not used right now
		msg->nextBlock("Data");
		msg->addString("Name", text);

		gAgent.sendReliableMessage();
	}

	mSearchResults->deleteAllItems();
	mSearchResults->addCommentText(getString("searching"));

	mSelect->setEnabled(FALSE);
}

void LLFloaterAvatarPicker::setAllowMultiple(BOOL allow_multiple)
{
	mSearchResults->setAllowMultipleSelection(allow_multiple);
	mInventoryPanel->setAllowMultiSelect(allow_multiple);
	mNearMe->setAllowMultipleSelection(allow_multiple);
}

//static
void LLFloaterAvatarPicker::onTabChanged(void* userdata, bool from_click)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if (self)
	{
		self->mSelect->setEnabled(self->visibleItemsSelected());
	}
}

//static
void LLFloaterAvatarPicker::onBtnFind(void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if (self)
	{
		self->find();
	}
}

static void getSelectedAvatarData(const LLScrollListCtrl* from,
								  std::vector<std::string>& avatar_names,
								  std::vector<LLUUID>& avatar_ids)
{
	std::vector<LLScrollListItem*> items = from->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin(),
												  end = items.end();
		 iter != end; ++iter)
	{
		LLScrollListItem* item = *iter;
		if (item->getUUID().notNull())
		{
			avatar_names.push_back(item->getColumn(0)->getValue().asString());
			avatar_ids.push_back(item->getUUID());
		}
	}
}

//static
void LLFloaterAvatarPicker::onBtnSelect(void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if (!self) return;

	if (self->mCallback)
	{
		LLPanel* active_panel = self->mResidentChooserTabs->getCurrentPanel();

		if (active_panel == self->mSearchPanel)
		{
			std::vector<std::string> avatar_names;
			std::vector<LLUUID> avatar_ids;
			getSelectedAvatarData(self->mSearchResults, avatar_names,
								  avatar_ids);
			self->mCallback(avatar_names, avatar_ids, self->mCallbackUserdata);
		}
		else if (active_panel == self->mFriendsPanel)
		{
			std::vector<std::string> avatar_names;
			std::vector<LLUUID> avatar_ids;
			getSelectedAvatarData(self->mFriends, avatar_names, avatar_ids);
			self->mCallback(avatar_names, avatar_ids, self->mCallbackUserdata);
		}
		else if (active_panel == self->mCallingCardsPanel)
		{
			self->mCallback(self->mSelectedInventoryAvatarNames,
							self->mSelectedInventoryAvatarIDs,
							self->mCallbackUserdata);
		}
		else if (active_panel == self->mNearMePanel)
		{
			std::vector<std::string> avatar_names;
			std::vector<LLUUID> avatar_ids;
			getSelectedAvatarData(self->mNearMe, avatar_names, avatar_ids);
			self->mCallback(avatar_names, avatar_ids, self->mCallbackUserdata);
		}
	}
	self->mSearchResults->deselectAllItems(TRUE);
	self->mFriends->deselectAllItems(TRUE);
	self->mInventoryPanel->setSelection(LLUUID::null, FALSE);
	self->mNearMe->deselectAllItems(TRUE);
	if (self->mCloseOnSelect)
	{
		self->mCloseOnSelect = FALSE;
		self->close();
	}
}

//static
void LLFloaterAvatarPicker::onBtnRefresh(void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if (self)
	{
		self->mNearMe->deleteAllItems();
		self->mNearMe->addCommentText(self->getString("searching"));
		self->mNearMeListComplete = false;
	}
}

//static
void LLFloaterAvatarPicker::onBtnClose(void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if (self)
	{
		self->close();
	}
}

void LLFloaterAvatarPicker::onRangeAdjust(LLUICtrl* source, void* data)
{
	LLFloaterAvatarPicker::onBtnRefresh(data);
}

//static
void LLFloaterAvatarPicker::onList(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)userdata;
	if (self)
	{
		self->mSelect->setEnabled(self->visibleItemsSelected());
	}
}

// Callback for inventory picker (select from calling cards)
//static
void LLFloaterAvatarPicker::onCallingCardSelectionChange(const std::deque<LLFolderViewItem*>& items,
														 BOOL user_action,
														 void* data)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)data;
	if (self)
	{
		self->doCallingCardSelectionChange(items, user_action, data);
	}
}

//static
void LLFloaterAvatarPicker::processAvatarPickerReply(LLMessageSystem* msg,
													 void**)
{
	// Dialog already closed
	if (!sInstance) return;

	LLUUID agent_id;
	LLUUID query_id;
	LLUUID avatar_id;
	std::string first_name;
	std::string last_name;

	msg->getUUID("AgentData", "AgentID", agent_id);
	if (agent_id != gAgentID)
	{
		// Not for us
		return;
	}

	msg->getUUID("AgentData", "QueryID", query_id);
	if (query_id != sInstance->mQueryID)
	{
		// these are not results from our last request
		return;
	}

	// clear "Searching" label on first results
	sInstance->mSearchResults->deleteAllItems();
	sInstance->mSearchResults->setDisplayHeading(FALSE);

	const std::string legacy_name = sInstance->getString("legacy_name");
	bool found_one = false;
	S32 num_new_rows = msg->getNumberOfBlocks("Data");
	for (S32 i = 0; i < num_new_rows; ++i)
	{
		msg->getUUIDFast(_PREHASH_Data, _PREHASH_AvatarID, avatar_id, i);
		msg->getStringFast(_PREHASH_Data, _PREHASH_FirstName, first_name, i);
		msg->getStringFast(_PREHASH_Data, _PREHASH_LastName, last_name, i);

		std::string avatar_name;
		if (avatar_id.isNull())
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = sInstance->mEdit->getValue().asString();
			avatar_name = sInstance->getString("not_found", map);
			sInstance->mSearchResults->setEnabled(FALSE);
			sInstance->mSelect->setEnabled(FALSE);
		}
		else
		{
			avatar_name = first_name + " " + last_name;
			sInstance->mSearchResults->setEnabled(TRUE);
			found_one = true;
		}
		LLSD element;
		element["id"] = avatar_id; // value
		element["columns"][0]["column"] = legacy_name;
		element["columns"][0]["value"] = avatar_name;
		sInstance->mSearchResults->addElement(element);
	}

	if (found_one)
	{
		sInstance->mSelect->setEnabled(TRUE);
		sInstance->mSearchResults->selectFirstItem();
		sInstance->onList(sInstance->mSearchResults, sInstance);
		sInstance->mSearchResults->setFocus(TRUE);
	}
}

void LLFloaterAvatarPicker::processResponse(const LLUUID& query_id,
											const LLSD& content)
{
	// Check for out-of-date query
	if (query_id != mQueryID) return;

	const std::string legacy_name = getString("legacy_name");
	const std::string display_name = getString("display_name");

	LLSD agents = content["agents"];
	if (agents.size() == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = mEdit->getValue().asString();
		LLSD element;
		element["id"] = LLUUID::null;
		element["columns"][0]["column"] = legacy_name;
		element["columns"][0]["value"] = getString("not_found", map);
		mSearchResults->addElement(element);
		mSearchResults->setEnabled(FALSE);
		mSearchResults->setDisplayHeading(FALSE);
		mSelect->setEnabled(FALSE);
		return;
	}

	// clear "Searching" label on first results
	mSearchResults->deleteAllItems();
	mSearchResults->setDisplayHeading(TRUE);

	std::string name;
	LLSD element;
	for (LLSD::array_const_iterator it = agents.beginArray(),
									end = agents.endArray();
		 it != end; ++it)
	{
		const LLSD& row = *it;
		LLSD& columns = element["columns"];
		element["id"] = row["id"];
		name = row["legacy_first_name"].asString();
		name += " ";
		name += row["legacy_last_name"].asString();
		columns[0]["column"] = legacy_name;
		columns[0]["value"] = name;
		columns[1]["column"] = display_name;
		columns[1]["value"] = row["display_name"];
		mSearchResults->addElement(element);
	}

	mSelect->setEnabled(TRUE);
	mSearchResults->selectFirstItem();
	mSearchResults->setEnabled(TRUE);
	onList(sInstance->mSearchResults, this);
	mSearchResults->setFocus(TRUE);
}

//static
void LLFloaterAvatarPicker::editKeystroke(LLLineEditor* caller,
										  void* user_data)
{
	LLFloaterAvatarPicker* self = (LLFloaterAvatarPicker*)user_data;
	if (self)
	{
		self->mFind->setEnabled(caller->getText().size() >= 3);
	}
}
