/** 
 * @file llappearancemgr.cpp
 * @brief Manager for initiating appearance changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llappearancemgr.h"

#include "llfile.h"
#include "llnotifications.h"
#include "llsdserialize.h"
#include "message.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llfirstuse.h"
#include "llfloatercustomize.h"
#include "llfloatermakenewoutfit.h"
#include "llgesturemgr.h"
#include "llinventorybridge.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewerjointattachment.h"
#include "llviewernetwork.h"			// for isInProductionGrid()
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"

char ORDER_NUMBER_SEPARATOR('@');

// Forward declarations of local functions
bool confirm_replace_attachment_rez(const LLSD& notification, const LLSD& response);

struct LLWearInfo
{
	LLUUID	mCategoryID;
	BOOL	mAppend;
	BOOL	mReplace;
};

struct LLFoundData
{
	LLFoundData(const LLUUID& item_id,
				const LLUUID& linked_item_id,
				const LLUUID& asset_id,
				const std::string& name,
				LLAssetType::EType asset_type) :
		mItemID(item_id),
		mLinkedItemID(linked_item_id),
		mAssetID(asset_id),
		mName(name),
		mAssetType(asset_type),
		mWearable(NULL) {}

	LLUUID mItemID;
	LLUUID mLinkedItemID;
	LLUUID mAssetID;
	std::string mName;
	LLAssetType::EType mAssetType;
	LLWearable* mWearable;
};

class LLWearableHoldingPattern
{
public:
	LLWearableHoldingPattern(bool append, bool replace)
	:	mResolved(0),
		mAppend(append),
		mReplace(replace)
	{
	}

	~LLWearableHoldingPattern()
	{
		for_each(mFoundList.begin(), mFoundList.end(), DeletePointer());
		mFoundList.clear();
	}

	typedef std::list<LLFoundData*> found_list_t;
	found_list_t mFoundList;
	S32 mResolved;
	bool mAppend;
	bool mReplace;
};

class LLOutfitObserver : public LLInventoryFetchObserver
{
public:
	LLOutfitObserver(const LLUUID& cat_id, bool copy_items, bool append)
	:	mCatID(cat_id),
		mCopyItems(copy_items),
		mAppend(append)
	{}
	~LLOutfitObserver() {}
	virtual void done(); //public

protected:
	LLUUID mCatID;
	bool mCopyItems;
	bool mAppend;
};

class LLWearInventoryCategoryCallback : public LLInventoryCallback
{
public:
	LLWearInventoryCategoryCallback(const LLUUID& cat_id, bool append)
	{
		mCatID = cat_id;
		mAppend = append;
	}
	void fire(const LLUUID& item_id)
	{
		/*
		 * Do nothing.  We only care about the destructor
		 *
		 * The reason for this is that this callback is used in a hack where the
		 * same callback is given to dozens of items, and the destructor is called
		 * after the last item has fired the event and dereferenced it -- if all
		 * the events actually fire!
		 */
	}

protected:
	~LLWearInventoryCategoryCallback()
	{
		// Is the destructor called by ordinary dereference, or because the
		// app's shutting down ?  If the inventory callback manager goes away,
		// we're shutting down, no longer want the callback.
		if (LLInventoryCallbackManager::is_instantiated())
		{
			LLAppearanceMgr::instance().wearInventoryCategoryOnAvatar(gInventory.getCategory(mCatID),
																	  mAppend);
		}
		else
		{
			llwarns << "Dropping unhandled LLWearInventoryCategoryCallback" << llendl;
		}
	}

private:
	LLUUID mCatID;
	bool mAppend;
};

void LLOutfitObserver::done()
{
	// We now have an outfit ready to be copied to agent inventory. Do
	// it, and wear that outfit normally.
	if (mCopyItems)
	{
		LLInventoryCategory* cat = gInventory.getCategory(mCatID);
		std::string name;
		if (!cat)
		{
			// should never happen.
			name = "New Outfit";
		}
		else
		{
			name = cat->getName();
		}
		LLViewerInventoryItem* item = NULL;
		item_ref_t::iterator it = mComplete.begin();
		item_ref_t::iterator end = mComplete.end();
		LLUUID pid;
		for ( ; it < end; ++it)
		{
			item = (LLViewerInventoryItem*)gInventory.getItem(*it);
			if (item)
			{
				if (pid.isNull() && LLInventoryType::IT_GESTURE == item->getInventoryType())
				{
					pid = gInventory.findCategoryUUIDForType(LLFolderType::FT_GESTURE);
				}
				else
				{
					pid = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
				}
				break;
			}
		}
		if (pid.isNull())
		{
			pid = gInventory.getRootFolderID();
		}

		LLUUID cat_id = gInventory.createNewCategory(pid, LLFolderType::FT_NONE, name);
		mCatID = cat_id;
		LLPointer<LLInventoryCallback> cb = new LLWearInventoryCategoryCallback(mCatID, mAppend);
		it = mComplete.begin();
		for ( ; it < end; ++it)
		{
			item = (LLViewerInventoryItem*)gInventory.getItem(*it);
			if (item)
			{
				copy_inventory_item(gAgentID,	item->getPermissions().getOwner(),
									item->getUUID(), cat_id, std::string(), cb);
			}
		}
		// BAP fixes a lag in display of created dir.
		gInventory.notifyObservers();
	}
	else
	{
		// Wear the inventory category.
		LLAppearanceMgr::instance().wearInventoryCategoryOnAvatar(gInventory.getCategory(mCatID),
																  mAppend);
	}
}

class LLOutfitFetch : public LLInventoryFetchDescendentsObserver
{
public:
	LLOutfitFetch(bool copy_items, bool append)
	:	mCopyItems(copy_items),
		mAppend(append)
	{}
	~LLOutfitFetch() {}
	virtual void done();
protected:
	bool mCopyItems;
	bool mAppend;
};

void LLOutfitFetch::done()
{
	// What we do here is get the complete information on the items in
	// the library, and set up an observer that will wait for that to
	// happen.
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(mCompleteFolders.front(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	S32 count = item_array.count();
	if (!count)
	{
		llwarns << "Nothing fetched in category " << mCompleteFolders.front()
				<< llendl;
		//dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}

	LLOutfitObserver* outfit;
	outfit = new LLOutfitObserver(mCompleteFolders.front(), mCopyItems, mAppend);
	LLInventoryFetchObserver::item_ref_t ids;
	for (S32 i = 0; i < count; ++i)
	{
		ids.push_back(item_array.get(i)->getUUID());
	}

	// clean up, and remove this as an observer since the call to the
	// outfit could notify observers and throw us into an infinite
	// loop.
	//dec_busy_count();
	gInventory.removeObserver(this);
	delete this;

	// increment busy count and either tell the inventory to check &
	// call done, or add this object to the inventory for observation.
	//inc_busy_count();

	// do the fetch
	outfit->fetchItems(ids);
	if (outfit->isFinished())
	{
		// everything is already here - call done.
		outfit->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(outfit);
	}
}

// *NOTE: hack to get from avatar inventory to avatar
void LLAppearanceMgr::wearInventoryItemOnAvatar(LLInventoryItem* item,
												bool replace)
{
	if (item)
	{
		LL_DEBUGS("Appearance") << "wearInventoryItemOnAvatar("
								<< item->getName() << ")" << LL_ENDL;

		LLWearableList::instance().getAsset(item->getAssetUUID(),
											item->getName(),
											item->getType(),
											LLWearableBridge::onWearOnAvatarArrived,
											new OnWearStruct(item->getLinkedUUID(), replace));
	}
}

bool LLAppearanceMgr::wearItemOnAvatar(const LLUUID& item_id_to_wear,
									   bool replace)
{
	if (item_id_to_wear.isNull()) return false;
	LLViewerInventoryItem* item_to_wear = gInventory.getItem(item_id_to_wear);
	if (!item_to_wear) return false;

	if (gInventory.isObjectDescendentOf(item_to_wear->getUUID(),
									    gInventory.getLibraryRootFolderID()))
	{
		LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback(replace);
		copy_inventory_item(gAgentID,
							item_to_wear->getPermissions().getOwner(),
							item_to_wear->getUUID(),
							LLUUID::null,
							std::string(),
							cb);
		return false;
	}
	else if (!gInventory.isObjectDescendentOf(item_to_wear->getUUID(),
											  gInventory.getRootFolderID()))
	{
		return false; // not in library and not in agent's inventory
	}
	else if (gInventory.isObjectDescendentOf(item_to_wear->getUUID(),
											 gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH)))
	{
		LLNotifications::instance().add("CannotWearTrash");
		return false;
	}

	switch (item_to_wear->getType())
	{
		case LLAssetType::AT_CLOTHING:
		{
			if (gAgentWearables.areWearablesLoaded())
			{
//MK
				if (gRRenabled && !gAgent.mRRInterface.canWear(item_to_wear))
				{
					return false;
				}
//mk
				const LLWearableType::EType type = item_to_wear->getWearableType();

				// See if we want to avoid wearing multiple wearables that
				// don't really make any sense or for which the resulting
				// combination is hard for the user to predict and/or notice.
				// E.g. for Physics, only the last worn item is taken into
				// account, so there's no use wearing more than one...
				if ((gSavedSettings.getBOOL("NoMultiplePhysics") &&
					 type == LLWearableType::WT_PHYSICS) ||
					(gSavedSettings.getBOOL("NoMultipleShoes") &&
					 type == LLWearableType::WT_SHOES) ||
					(gSavedSettings.getBOOL("NoMultipleSkirts") &&
					 type == LLWearableType::WT_SKIRT))
				{
					replace = true;
				}
				S32 wearable_count = gAgentWearables.getWearableCount(type);
				if (replace && wearable_count != 0)
				{
					gAgentWearables.userRemoveWearablesOfType(type);
					wearable_count = gAgentWearables.getWearableCount(type);
				}
				if (wearable_count >= LLAgentWearables::MAX_CLOTHING_PER_TYPE)
				{
					return false;
				}

				wearInventoryItemOnAvatar(item_to_wear, replace);
			}
			break;
		}

		case LLAssetType::AT_BODYPART:
		{
			if (gAgentWearables.areWearablesLoaded())
			{
				wearInventoryItemOnAvatar(item_to_wear, true);
			}
			break;
		}

		case LLAssetType::AT_OBJECT:
		{
			rezAttachment(item_to_wear, NULL, replace);
			break;
		}

		default:
		{
			// Nothing to do...
		}
	}

	return false;
}

void LLAppearanceMgr::getDescendentsOfAssetType(const LLUUID& category,
												LLInventoryModel::item_array_t& items,
												LLAssetType::EType type)
{
	LLInventoryModel::cat_array_t cats;
	LLIsType is_of_type(type);
	gInventory.collectDescendentsIf(category,
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_of_type);
}

void LLAppearanceMgr::getUserDescendents(const LLUUID& category, 
										 LLInventoryModel::item_array_t& wear_items,
										 LLInventoryModel::item_array_t& obj_items,
										 LLInventoryModel::item_array_t& gest_items)
{
	LLInventoryModel::cat_array_t wear_cats;
	LLFindWearables is_wearable;
	gInventory.collectDescendentsIf(category,
									wear_cats,
									wear_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_wearable);

	LLInventoryModel::cat_array_t obj_cats;
	LLIsType is_object(LLAssetType::AT_OBJECT);
	gInventory.collectDescendentsIf(category,
									obj_cats,
									obj_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_object);

	// Find all gestures in this folder
	LLInventoryModel::cat_array_t gest_cats;
	LLIsType is_gesture(LLAssetType::AT_GESTURE);
	gInventory.collectDescendentsIf(category,
									gest_cats,
									gest_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_gesture);
}

void LLAppearanceMgr::wearInventoryCategory(LLInventoryCategory* category,
											bool copy, bool append)
{
	if (!category) return;

	LL_DEBUGS("Appearance") << "wearInventoryCategory(" << category->getName()
							<< ")" << LL_ENDL;
	// What we do here is get the complete information on the items in
	// the inventory, and set up an observer that will wait for that to
	// happen.
	LLOutfitFetch* outfit;
	outfit = new LLOutfitFetch(copy, append);
	LLInventoryFetchDescendentsObserver::folder_ref_t folders;
	folders.push_back(category->getUUID());
	outfit->fetchDescendents(folders);
	//inc_busy_count();
	if (outfit->isFinished())
	{
		// everything is already here - call done.
		outfit->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(outfit);
	}
}

void LLAppearanceMgr::wearInventoryCategoryOnAvatar(LLInventoryCategory* category,
													bool append, bool replace)
{
	// Avoid unintentionally overwriting old wearables.  We have to do
	// this up front to avoid having to deal with the case of multiple
	// wearables being dirty.
	if (!category) return;
	LL_DEBUGS("Appearance") << "wear_inventory_category_on_avatar("
							<< category->getName() << ")" << LL_ENDL;
			 
	LLWearInfo* userdata = new LLWearInfo;
	userdata->mAppend = append;
	userdata->mReplace = replace;
	userdata->mCategoryID = category->getUUID();

	if (gFloaterCustomize)
	{
		gFloaterCustomize->askToSaveIfDirty(wearInventoryCategoryOnAvatarStep2,
											userdata);
	}
	else
	{
		wearInventoryCategoryOnAvatarStep2(true, userdata);
	}
}

//static
void LLAppearanceMgr::wearInventoryCategoryOnAvatarStep2(BOOL proceed,
														 void* userdata)
{
	LLWearInfo* wear_info = (LLWearInfo*)userdata;
	if (!wear_info) return;

	// Find all the wearables that are in the category's subtree.
	LL_DEBUGS("Appearance") << "wearInventoryCategoryOnAvatarStep2()"
							<< LL_ENDL;
	if (proceed)
	{
		LLUUID cat_id = wear_info->mCategoryID;
		LLViewerInventoryItem* item;
		S32 i;

		// Checking and updating links' descriptions of wearables
		//LLAppearanceMgr::instance().updateClothingOrderingInfo(cat_id);

		// Find all the wearables that are in the category's subtree.
		LLInventoryModel::item_array_t wear_items;
		LLInventoryModel::item_array_t obj_items;
		LLInventoryModel::item_array_t gest_items;
		LLAppearanceMgr::instance().getUserDescendents(cat_id, wear_items,
													   obj_items, gest_items);

		S32 wearable_count = wear_items.count();
		S32 obj_count = obj_items.count();
		S32 gest_count = gest_items.count();

		if (!wearable_count && !obj_count && !gest_count)
		{
			LLNotifications::instance().add("CouldNotPutOnOutfit");
			delete wear_info;
			return;
		}
#if 0
		// Processes that take time should show the busy cursor
		if (wearable_count > 0 || obj_count > 0)
		{
			inc_busy_count();
		}
#endif
		// Activate all gestures in this folder
		if (gest_count > 0)
		{
			llinfos << "Activating " << gest_count << " gestures" << llendl;

			gGestureManager.activateGestures(gest_items);

			// Update the inventory item labels to reflect the fact
			// they are active.
			LLViewerInventoryCategory* catp = gInventory.getCategory(wear_info->mCategoryID);
			if (catp)
			{
				gInventory.updateCategory(catp);
				gInventory.notifyObservers();
			}
		}

		if (wearable_count > 0)
		{
			// Preparing the list of wearables in the correct order for
			// LLAgentWearables
			sortItemsByActualDescription(wear_items);

			// Note: can't do normal iteration, because if all the
			// wearables can be resolved immediately, then the
			// callback will be called (and this object deleted)
			// before the final getNextData().
			LLWearableHoldingPattern* holder = new LLWearableHoldingPattern(wear_info->mAppend,
																			wear_info->mReplace);
			LLFoundData* found;
			LLDynamicArray<LLFoundData*> found_container;
			for (i = 0; i  < wearable_count; ++i)
			{
				item = wear_items.get(i);
				found = new LLFoundData(item->getUUID(),
										item->getLinkedUUID(),
										item->getAssetUUID(),
										item->getName(),
										item->getType());
				// Pushing back, not front, to preserve order of wearables for
				// LLAgentWearables
				holder->mFoundList.push_back(found);
				found_container.put(found);
			}
			for (i = 0; i < wearable_count; ++i)
			{
				found = found_container.get(i);
				LLWearableList::instance().getAsset(found->mAssetID,
													found->mName,
													found->mAssetType,
													onWearableAssetFetch,
													(void*)holder);
			}
		}

		// If not appending and the folder doesn't contain only gestures, take
		// off all attachments.
		if (!wear_info->mAppend &&
			!(wearable_count == 0 && obj_count == 0 && gest_count > 0))
		{
			LLAgentWearables::userRemoveAllAttachments();
		}

		if (obj_count > 0)
		{
			// We've found some attachements. Add these.

			if (isAgentAvatarValid())
			{
				// Build a compound message to send all the objects that need to be rezzed.

				// Limit number of packets to send
				const S32 MAX_PACKETS_TO_SEND = 10;
				const S32 OBJECTS_PER_PACKET = 4;
				const S32 MAX_OBJECTS_TO_SEND = MAX_PACKETS_TO_SEND * OBJECTS_PER_PACKET;
				if (obj_count > MAX_OBJECTS_TO_SEND)
				{
					obj_count = MAX_OBJECTS_TO_SEND;
				}

				// Create an id to keep the parts of the compound message together
				LLUUID compound_msg_id;
				compound_msg_id.generate();
				LLMessageSystem* msg = gMessageSystem;

				for (i = 0; i < obj_count; ++i)
				{
					if (i % OBJECTS_PER_PACKET == 0)
					{
						// Start a new message chunk
						msg->newMessageFast(_PREHASH_RezMultipleAttachmentsFromInv);
						msg->nextBlockFast(_PREHASH_AgentData);
						msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
						msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
						msg->nextBlockFast(_PREHASH_HeaderData);
						msg->addUUIDFast(_PREHASH_CompoundMsgID, compound_msg_id);
						msg->addU8Fast(_PREHASH_TotalObjects, obj_count);
						msg->addBOOLFast(_PREHASH_FirstDetachAll, !wear_info->mAppend);
					}

					bool replace = wear_info->mReplace ||
								   !gAgentAvatarp->allowMultipleAttachments();
					item = obj_items.get(i);
					msg->nextBlockFast(_PREHASH_ObjectData);
					msg->addUUIDFast(_PREHASH_ItemID, item->getLinkedUUID());
					msg->addUUIDFast(_PREHASH_OwnerID, item->getPermissions().getOwner());
					msg->addU8Fast(_PREHASH_AttachmentPt, replace ? 0 : ATTACHMENT_ADD);	// Wear at the previous or default attachment point
					pack_permissions_slam(msg, item->getFlags(), item->getPermissions());
					msg->addStringFast(_PREHASH_Name, item->getName());
					msg->addStringFast(_PREHASH_Description, item->getDescription());

					if (obj_count == i + 1 ||
						OBJECTS_PER_PACKET - 1 == i % OBJECTS_PER_PACKET)
					{
						// End of message chunk
						msg->sendReliable(gAgent.getRegion()->getHost());
					}
				}
			}
		}
	}

	delete wear_info;
	wear_info = NULL;
}

//static
void LLAppearanceMgr::onWearableAssetFetch(LLWearable* wearable,
										   void* userdata)
{
	LLWearableHoldingPattern* holder = (LLWearableHoldingPattern*)userdata;

	if (wearable)
	{
		for (LLWearableHoldingPattern::found_list_t::iterator iter = holder->mFoundList.begin();
			 iter != holder->mFoundList.end(); ++iter)
		{
			LLFoundData* data = *iter;
			if (wearable->getAssetID() == data->mAssetID)
			{
				data->mWearable = wearable;
				break;
			}
		}
	}
	if (++holder->mResolved >= (S32)holder->mFoundList.size())
	{
		wearInventoryCategoryOnAvatarStep3(holder);
	}
}

//static
void LLAppearanceMgr::wearInventoryCategoryOnAvatarStep3(LLWearableHoldingPattern* holder)
{
	LL_DEBUGS("Appearance") << "wearInventoryCategoryOnAvatarStep3()"
							<< LL_ENDL;
	LLInventoryItem::item_array_t items;
	LLDynamicArray<LLWearable*> wearables;

	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		bool remove_old = false;
		for (LLWearableHoldingPattern::found_list_t::iterator iter = holder->mFoundList.begin();
			 iter != holder->mFoundList.end(); ++iter)
		{
			LLFoundData* data = *iter;
			LLWearable* wearable = data->mWearable;
			if (wearable && (S32)wearable->getType() == i)
			{
				LLViewerInventoryItem* item;
				item = (LLViewerInventoryItem*)gInventory.getItem(data->mLinkedItemID);
				if (item && item->getAssetUUID() == wearable->getAssetID())
				{
					items.put(item);
					wearables.put(wearable);
					if (holder->mReplace &&
						wearable->getAssetType() == LLAssetType::AT_CLOTHING)
					{
						remove_old = true;
					}
				}
			}
		}
		if (remove_old)
		{
			gAgentWearables.removeWearable((LLWearableType::EType)i, true, 0);
		}
	}

	if (wearables.count() > 0)
	{
		gAgentWearables.setWearableOutfit(items, wearables, !holder->mAppend);
		//gInventory.notifyObservers();
	}

	delete holder;

	//dec_busy_count();
}

void LLAppearanceMgr::wearOutfitByName(const std::string& name)
{
	llinfos << "Wearing category " << name << llendl;
	//inc_busy_count();

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLNameCategoryCollector has_name(name);
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									has_name);
	bool copy_items = false;
	LLInventoryCategory* cat = NULL;
	if (cat_array.count() > 0)
	{
		// Just wear the first one that matches
		cat = cat_array.get(0);
	}
	else
	{
		gInventory.collectDescendentsIf(LLUUID::null,
										cat_array,
										item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										has_name);
		if (cat_array.count() > 0)
		{
			cat = cat_array.get(0);
			copy_items = true;
		}
	}

	if (cat)
	{
		wearInventoryCategory(cat, copy_items, false);
	}
	else
	{
		llwarns << "Couldn't find outfit " << name
				<< " in wearOutfitByName()" << llendl;
	}

	//dec_busy_count();
}

std::string build_order_string(LLWearableType::EType type, U32 i)
{
	std::ostringstream order_num;
	order_num << ORDER_NUMBER_SEPARATOR << type * 100 + i;
	return order_num.str();
}

// NOTE: despite the name, this is not the same function as in v2/3 viewers:
// this function is used to update the description of the inventory links
// corresponding to a worn clothing item in a folder (category), according
// to their current layer index.
void LLAppearanceMgr::updateClothingOrderingInfo(LLUUID cat_id)
{
	if (cat_id.isNull())
	{
		return;
	}

	LLInventoryModel::item_array_t wear_items;
	getDescendentsOfAssetType(cat_id, wear_items, LLAssetType::AT_CLOTHING);

	if (!wear_items.count())
	{
		return;
	}

	bool inventory_changed = false;

	for (S32 i = 0; i < wear_items.count(); i++)
	{
		LLViewerInventoryItem* item = wear_items.get(i);
		if (!item)
		{
			LL_WARNS("Appearance") << "NULL item found" << LL_ENDL;
			continue;
		}
		// Ignore non-links and non-worn link wearables.
		if (!item->getIsLinkType() ||
			!gAgentWearables.isWearingItem(item->getUUID()))
		{
			continue;
		}
		LLWearableType::EType type = item->getWearableType();
		if (type < 0 || type >= LLWearableType::WT_COUNT)
		{
			LL_WARNS("Appearance") << "Invalid wearable type. Inventory type does not match wearable flag bitfield."
								   << LL_ENDL;
			continue;
		}
		LLWearable* wearable = gAgentWearables.getWearableFromItemID(item->getUUID());
		U32 index = gAgentWearables.getWearableIndex(wearable);

		std::string new_order_str = build_order_string(type, index);
		std::string old_desc = item->LLInventoryItem::getDescription();
		if (new_order_str == old_desc) continue;

		LL_DEBUGS("Appearance") << "Changing the description for link item '"
								<< item->getName() << "' from '" << old_desc
								<< "' to '" << new_order_str << "'" << LL_ENDL;

		item->setDescription(new_order_str);
		item->setComplete(TRUE);
		item->updateServer(FALSE);
		gInventory.updateItem(item);

		inventory_changed = true;
	}

	// *TODO: do we really need to notify observers?
	if (inventory_changed) gInventory.notifyObservers();
}

//a predicate for sorting inventory items by actual descriptions
bool sort_by_description(const LLInventoryItem* item1,
						 const LLInventoryItem* item2)
{
	if (!item1 || !item2) 
	{
		llwarning("either item1 or item2 is NULL", 0);
		return true;
	}

	return item1->LLInventoryItem::getDescription() < item2->LLInventoryItem::getDescription();
}

//static
void LLAppearanceMgr::sortItemsByActualDescription(LLInventoryModel::item_array_t& items)
{
	if (items.size() < 2) return;

	std::sort(items.begin(), items.end(), sort_by_description);
}

void LLAppearanceMgr::removeInventoryCategoryFromAvatar(LLInventoryCategory* category)
{
	if (!category) return;
	LL_DEBUGS("Appearance") << "removeInventoryCategoryFromAvatar("
							<< category->getName() << ")" << LL_ENDL;

	LLUUID* uuid = new LLUUID(category->getUUID());

	if (gFloaterCustomize)
	{
		gFloaterCustomize->askToSaveIfDirty(removeInventoryCategoryFromAvatarStep2,
											uuid);
	}
	else
	{
		removeInventoryCategoryFromAvatarStep2(TRUE, uuid);
	}
}

//static
void LLAppearanceMgr::removeInventoryCategoryFromAvatarStep2(BOOL proceed,
															 void* userdata)
{
	LLUUID* category_id = (LLUUID*)userdata;
	LLViewerInventoryItem* item;
	S32 i;

	LL_DEBUGS("Appearance") << "removeInventoryCategoryFromAvatarStep2()"
							<< LL_ENDL;
	if (proceed)
	{
		// Find all the wearables that are in the category's subtree.

		LLInventoryModel::item_array_t wear_items;
		LLInventoryModel::item_array_t obj_items;
		LLInventoryModel::item_array_t gest_items;
		LLAppearanceMgr::instance().getUserDescendents(*category_id, wear_items,
													   obj_items, gest_items);

		S32 wearable_count = wear_items.count();
		S32 obj_count = obj_items.count();
		S32 gest_count = gest_items.count();

		if (wearable_count > 0)
		{
			// Loop through wearables. If worn, remove.
			for (i = 0; i < wearable_count; ++i)
			{
				item = wear_items.get(i);
				if (gAgentWearables.isWearingItem(item->getUUID()))
				{
					LLWearableList::instance().getAsset(item->getAssetUUID(),
														item->getName(),
														item->getType(),
														LLWearableBridge::onRemoveFromAvatarArrived,
														new OnRemoveStruct(item->getLinkedUUID()));

				}
			}
		}

		if (obj_count > 0)
		{
			for (i = 0; i  < obj_count; ++i)
			{
				item = obj_items.get(i);
//MK
				if (!gRRenabled || gAgent.mRRInterface.canDetach(item))
				{
//mk
					LLVOAvatarSelf::detachAttachmentIntoInventory(item->getLinkedUUID());
//MK
				}
//mk
			}
		}

		if (gest_count > 0)
		{
			for (i = 0; i  < gest_count; ++i)
			{
				item = gest_items.get(i);
				if (gGestureManager.isGestureActive(item->getUUID()))
				{
					gGestureManager.deactivateGesture(item->getUUID());
					gInventory.updateItem(item);
					gInventory.notifyObservers();
				}
			}
		}
	}

	delete category_id;
	category_id = NULL;
}

void LLAppearanceMgr::rezAttachment(LLViewerInventoryItem* item, 
									LLViewerJointAttachment* attachment,
									bool replace)
{
	if (!isAgentAvatarValid() || !item) return;

	LLSD payload;
	payload["item_id"] = item->getLinkedUUID(); // Wear the base object in case this is a link.

	S32 attach_pt = 0;
	if (attachment)
	{
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
			 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
		{
			if (iter->second == attachment)
			{
				attach_pt = iter->first;
				break;
			}
		}
	}

	if (!replace && gAgentAvatarp->allowMultipleAttachments())
	{
		attach_pt |= ATTACHMENT_ADD;
	}
	payload["attachment_point"] = attach_pt;
	if (attachment)
	{
		payload["attachment_name"] = attachment->getName();
	}

	if (replace && attachment && attachment->getNumObjects() > 0)
	{
//MK
		if (!gRRenabled ||
			(gAgent.mRRInterface.canAttach(item) &&
			 gAgent.mRRInterface.canDetach(attachment->getName())))
		{
//mk
			LLNotifications::instance().add("ReplaceAttachment",
											LLSD(),
											payload,
											confirm_replace_attachment_rez);
//MK
		}
//mk
	}
	else
	{
//MK
		if (!gRRenabled || gAgent.mRRInterface.canAttach(item))
		{
//mk
			LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload),
													  0/*YES*/);
//MK
		}
//mk
	}
}

bool confirm_replace_attachment_rez(const LLSD& notification, const LLSD& response)
{
	if (!gAgentAvatarp->canAttachMoreObjects())
	{
		LLSD args;
		args["MAX_ATTACHMENTS"] = llformat("%d", MAX_AGENT_ATTACHMENTS);
		LLNotifications::instance().add("MaxAttachmentsOnOutfit", args);
		return false;
	}

	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0/*YES*/)
	{
		LLViewerInventoryItem* itemp = gInventory.getItem(notification["payload"]["item_id"].asUUID());
		S32 attach_pt = notification["payload"]["attachment_point"].asInteger();
		if ((attach_pt & ~ATTACHMENT_ADD) > 0)
		{
			std::string name = notification["payload"]["attachment_name"].asString();
			LLFirstUse::useAttach(attach_pt & ~ATTACHMENT_ADD, name);
		}

		if (itemp)
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_RezSingleAttachmentFromInv);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addUUIDFast(_PREHASH_ItemID, itemp->getUUID());
			msg->addUUIDFast(_PREHASH_OwnerID, itemp->getPermissions().getOwner());
			msg->addU8Fast(_PREHASH_AttachmentPt, attach_pt);
			pack_permissions_slam(msg, itemp->getFlags(), itemp->getPermissions());
			msg->addStringFast(_PREHASH_Name, itemp->getName());
			msg->addStringFast(_PREHASH_Description, itemp->getDescription());
			msg->sendReliable(gAgent.getRegion()->getHost());
		}
	}
	return false;
}
static LLNotificationFunctorRegistration confirm_replace_attachment_rez_reg("ReplaceAttachment",
																			confirm_replace_attachment_rez);

// type = 0 for reading, = 1 to write attachments, = 2 to write outfit
std::string get_outfit_filename(U32 type)
{
	bool production_grid = LLViewerLogin::getInstance()->isInProductionGrid();
	std::string filename1 = (production_grid ? "attachments.xml"
											 : "attachments_beta.xml");
	std::string filename2 = (production_grid ? "outfit.xml"
											 : "outfit_beta.xml");
	filename1 = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT,
											   filename1);
	filename2 = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT,
											   filename2);
	switch (type)
	{
		case 0:
		{
			// Compatibility with Cool VL Viewer v1.26.2 and older: let's load
			// the last updated file among attachments.xml and outfit.xml
			llstat stat_data;
			time_t lastmodtime1;
			time_t lastmodtime2;
			int res = LLFile::stat(filename1, &stat_data);
			if (res == 0)
			{
				lastmodtime1 = stat_data.st_mtime;
			}
			else
			{
				LL_DEBUGS("Wearables") << "LLFile::stat() failed on "
									   << filename1  << " errno = "
									   << errno << LL_ENDL;
				return filename2;
			}
			res = LLFile::stat(filename2, &stat_data);
			if (res == 0)
			{
				lastmodtime2 = stat_data.st_mtime;
			}
			else
			{
				LL_DEBUGS("Wearables") << "LLFile::stat() failed on "
									   << filename2  << " errno = "
									   << errno << LL_ENDL;
				return filename1;
			}
			return lastmodtime1 > lastmodtime2 ? filename1 : filename2;
		}
		case 1:
		{
			return filename1;
		}
		case 2:
		{
			return filename2;
		}
		default:
		{
		}
	}

	return "";
}

void LLAppearanceMgr::checkOutfit()
{
	static bool initialized = false;
	static bool first_run = true;
	static F32 min_delay = 5.0f;
	static F32 max_delay = 30.0f;
	static F32 delay_delta = 0.0f;

	if (gAgentAvatarp->mPendingAttachment.size() == 0 &&
		gAgentWearables.areWearablesLoaded())
	{
		if (first_run)
		{
			if (!initialized)
			{
				min_delay = gSavedSettings.getF32("MultiReattachMinDelay");
				if (min_delay < 5.0f)
				{
					min_delay = 5.0f;
				}
				max_delay = gSavedSettings.getF32("MultiReattachMaxDelay");
				if (max_delay < min_delay + 5.0f)
				{
					max_delay = min_delay + 5.0f;
				}
				initialized = true;
			}
			if (gAttachmentsTimer.getElapsedTimeF32() < min_delay)
			{
				delay_delta = 0.0f;	// must be reset each time the timer is reset
			}
			if (gAttachmentsTimer.getElapsedTimeF32() > min_delay + delay_delta)
			{
				first_run = false;
				std::set<LLUUID> worn;
				// Add the worn attachments inventory items
				LLVOAvatar::attachment_map_t::iterator iter;
				for (iter = gAgentAvatarp->mAttachmentPoints.begin();
					 iter != gAgentAvatarp->mAttachmentPoints.end(); iter++)
				{
					LLViewerJointAttachment* attachment = iter->second;
					LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter;
					for (attachment_iter = attachment->mAttachedObjects.begin();
						 attachment_iter != attachment->mAttachedObjects.end();
						 attachment_iter++)
					{
						LLViewerObject* attached_object = *attachment_iter;
						if (attached_object)
						{
							worn.insert(attached_object->getAttachmentItemID());
						}
					}
				}
				// Add the worn boby parts and clothes inventory items
				for (U32 i = 0; i < (U32)LLWearableType::WT_COUNT; i++)
				{
					LLWearableType::EType type = (LLWearableType::EType)i;
					for (U32 index = 0;
						 index < gAgentWearables.getWearableCount(type);
						 index++)
					{
						LLInventoryItem* item;
						item = gInventory.getItem(gAgentWearables.getWearableItemID(type,
																					index));
						if (item)
						{
							worn.insert(item->getUUID());
						}
					}
				}
				std::string filename = get_outfit_filename(0);
				llinfos << "Reading the saved outfit from: " << filename << llendl;
				LLSD list;
				llifstream llsd_xml;
				llsd_xml.open(filename.c_str(), std::ios::in | std::ios::binary);
				if (llsd_xml.is_open())
				{
					LLSDSerialize::fromXML(list, llsd_xml);
//MK
					gAgent.mRRInterface.mRestoringOutfit = true;
//mk
					for (LLSD::map_iterator iter = list.beginMap();
						 iter != list.endMap(); iter++)
					{
						LLSD::String key_name = iter->first;
						LLSD array = iter->second;
						if (key_name == "attachments" && array.isArray())
						{
							for (S32 i = 0; i < array.size(); i++)
							{
								LLSD map = array[i];
								if (map.has("inv_item_id"))
								{
									LLUUID item_id = map.get("inv_item_id");
									if (worn.find(item_id) == worn.end())
									{
										LLViewerInventoryItem* item = gInventory.getItem(item_id);
										if (item)
										{
											LL_DEBUGS("Wearables") << "Reattaching: "
																   << item_id.asString()
																   << LL_ENDL;
											rezAttachment(item, NULL, false);
										}
										else
										{
											if (gAttachmentsTimer.getElapsedTimeF32() < max_delay)
											{
												first_run = true;
												delay_delta = gAttachmentsTimer.getElapsedTimeF32() - min_delay + 1.0f;
												LL_DEBUGS("Wearables") << item_id.asString()
																	   << " not yet found in inventory. Will retry in one second..."
																	   << LL_ENDL;
											}
											else
											{
												llwarns << item_id.asString()
														<< " not found in inventory, could not reattach."
														<< llendl;
											}
										}
									}
								}
								else
								{
									llwarns << "Malformed attachments list (no \"inv_item_id\" key). Aborting."
											<< llendl;
									llsd_xml.close();
//MK
									gAgent.mRRInterface.mRestoringOutfit = false;
//mk
									return;
								}
							}
						}
						else if (key_name == "wearables" && array.isArray())
						{
							for (S32 i = 0; i < array.size(); i++)
							{
								LLSD map = array[i];
								if (map.has("inv_item_id"))
								{
									LLUUID item_id = map.get("inv_item_id");
									if (worn.find(item_id) == worn.end())
									{
										LLViewerInventoryItem* item = gInventory.getItem(item_id);
										if (item)
										{
											LL_DEBUGS("Wearables") << "Rewearing: "
																   << item_id.asString()
																   << LL_ENDL;
											wearInventoryItemOnAvatar(item, false);
										}
										else
										{
											if (gAttachmentsTimer.getElapsedTimeF32() < max_delay)
											{
												first_run = true;
												delay_delta = gAttachmentsTimer.getElapsedTimeF32() - min_delay + 1.0f;
												LL_DEBUGS("Wearables") << item_id.asString()
																	   << " not yet found in inventory. Will retry in one second..."
																	   << LL_ENDL;
												break;	// Do not wear the rest: we must preserve the order
											}
											else
											{
												llwarns << item_id.asString()
														<< " not found in inventory, could not rewear."
														<< llendl;
											}
										}
									}
								}
								else
								{
									llwarns << "Malformed wearables list (no \"inv_item_id\" key). Aborting."
											<< llendl;
									llsd_xml.close();
//MK
									gAgent.mRRInterface.mRestoringOutfit = false;
//mk
									return;
								}
							}
						}
						else
						{
							llwarns << "Malformed outfit list. Aborting."
									<< llendl;
							llsd_xml.close();
//MK
							gAgent.mRRInterface.mRestoringOutfit = false;
//mk
							return;
						}
					}
					llsd_xml.close();
//MK
					gAgent.mRRInterface.mRestoringOutfit = false;
//mk

					if (!first_run)
					{
						// Force a saving of the outfit on next run
						gAttachmentsListDirty = gWearablesListDirty = true;

						// Notify the Make New Outfit floater, if opened
						LLFloaterMakeNewOutfit::setDirty();

						llinfos << "Outfit restoration completed." << llendl;
					}
				}
			}
		}
		else if (gAttachmentsListDirty || gWearablesListDirty)
		{
			gAttachmentsListDirty = gWearablesListDirty = false;
			LLSD array = LLSD::emptyArray();
			// Save the worn attachments list
			LLVOAvatar::attachment_map_t::iterator iter;
			for (iter = gAgentAvatarp->mAttachmentPoints.begin();
				 iter != gAgentAvatarp->mAttachmentPoints.end(); iter++)
			{
				LLViewerJointAttachment* attachment = iter->second;
				LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter;
				for (attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 attachment_iter++)
				{
					LLViewerObject* attached_object = *attachment_iter;
					if (attached_object)
					{
						LLUUID item_id = attached_object->getAttachmentItemID();
						LLViewerInventoryItem* item = gInventory.getItem(item_id);
						if (item)
						{
							LLSD entry = LLSD::emptyMap();
							entry.insert("inv_item_id", item_id);
							array.append(entry);
						}
						else
						{
							// This may happen (e.g. for Linden Realms and their HUD)
							LL_DEBUGS("Wearables") << item_id.asString()
												   << " not found in inventory. Not saving in attachments list."
												   << LL_ENDL;
						}
					}
				}
			}
			LLSD list;
			list.insert("attachments", array);

			// Compatibility with Cool VL Viewer v1.26.2 and older
			std::string filename = get_outfit_filename(1);
			llofstream list_file1(filename);
			LLSDSerialize::toPrettyXML(list, list_file1);
			list_file1.close();
			LL_DEBUGS("Wearables") << "Worn attachments list saved to: "
								   << filename << LL_ENDL;

			array = LLSD::emptyArray();
			// Save the worn body parts and clothes list
			for (U32 i = 0; i < (U32)LLWearableType::WT_COUNT; i++)
			{
				LLWearableType::EType type = (LLWearableType::EType)i;
				U32 count = gAgentWearables.getWearableCount(type);
				if (count > 1 &&
					LLWearableType::getAssetType(type) == LLAssetType::AT_BODYPART)
				{
					count = 1;	// Paranoia: only one wearable per body part type.
				}
				for (U32 index = 0; index < count; index++)
				{
					LLInventoryItem* item;
					item = gInventory.getItem(gAgentWearables.getWearableItemID(type,
																				index));
					if (item)
					{
						LLSD entry = LLSD::emptyMap();
						entry.insert("inv_item_id", item->getUUID());
						array.append(entry);
					}
				}
			}
			list.insert("wearables", array);

			filename = get_outfit_filename(2);
			llofstream list_file(filename);
			LLSDSerialize::toPrettyXML(list, list_file);
			list_file.close();
			LL_DEBUGS("Wearables") << "Outfit items list saved to: "
								   << filename << LL_ENDL;

			// Notify the Make New Outfit floater, if opened
			LLFloaterMakeNewOutfit::setDirty();
		}
	}
	else
	{
		gAttachmentsListDirty = true;
		gAttachmentsTimer.reset();
	}
}
