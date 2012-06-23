/** 
 * @file llagentwearables.cpp
 * @brief LLAgentWearables class implementation
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
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

#include "llagentwearables.h"

#include "llmd5.h"
#include "llnotifications.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llcallbacklist.h"
#include "llfloatercustomize.h"
#include "llfloatermakenewoutfit.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "llinventorybridge.h"
#include "llinventoryview.h"
#include "lltexlayer.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"				// for gIsInSecondLife
#include "llviewerregion.h"
#include "llvoavatarself.h"
#include "llwearable.h"
#include "llwearablelist.h"

#include "boost/scoped_ptr.hpp"

// Globals
LLAgentWearables gAgentWearables;
bool gWearablesListDirty = false;

BOOL LLAgentWearables::mInitialWearablesUpdateReceived = FALSE;

using namespace LLVOAvatarDefines;

LLAgentWearables::LLAgentWearables()
:	mWearablesLoaded(FALSE),
	mIsSettingOutfit(false)
{
}

LLAgentWearables::~LLAgentWearables()
{
}

void LLAgentWearables::setAvatarObject(LLVOAvatarSelf *avatar)
{ 
	if (avatar)
	{
		sendAgentWearablesRequest();
	}
}

// wearables
LLAgentWearables::createStandardWearablesAllDoneCallback::~createStandardWearablesAllDoneCallback()
{
	LL_DEBUGS("Wearables") << "Destructor - all done ?" << LL_ENDL;
	gAgentWearables.createStandardWearablesAllDone();
}

LLAgentWearables::sendAgentWearablesUpdateCallback::~sendAgentWearablesUpdateCallback()
{
	gAgentWearables.sendAgentWearablesUpdate();
}

/**
 * @brief Construct a callback for dealing with the wearables.
 *
 * Would like to pass the agent in here, but we can't safely
 * count on it being around later.  Just use gAgent directly.
 * @param cb callback to execute on completion (??? unused ???)
 * @param type Type for the wearable in the agent
 * @param wearable The wearable data.
 * @param todo Bitmask of actions to take on completion.
 */
LLAgentWearables::addWearableToAgentInventoryCallback::addWearableToAgentInventoryCallback(
															LLPointer<LLRefCount> cb,
															LLWearableType::EType type,
															U32 index,
															LLWearable* wearable,
															U32 todo)
:	mType(type),
	mIndex(index),
	mWearable(wearable),
	mTodo(todo),
	mCB(cb)
{
	LL_DEBUGS("Wearables") << "Constructor" << LL_ENDL;
}

void LLAgentWearables::addWearableToAgentInventoryCallback::fire(const LLUUID& inv_item)
{
	if (mTodo & CALL_CREATESTANDARDDONE)
	{
		llinfos << "callback fired, inv_item " << inv_item.asString() << llendl;
	}

	if (inv_item.isNull())
	{
		return;
	}

	gAgentWearables.addWearabletoAgentInventoryDone(mType, mIndex, inv_item, mWearable);

	if (mTodo & CALL_UPDATE)
	{
		gAgentWearables.sendAgentWearablesUpdate();
	}
	if (mTodo & CALL_RECOVERDONE)
	{
		gAgentWearables.recoverMissingWearableDone();
	}
	/*
	 * Do this for every one in the loop
	 */
	if (mTodo & CALL_CREATESTANDARDDONE)
	{
		gAgentWearables.createStandardWearablesDone(mType, mIndex);
	}
	if (mTodo & CALL_MAKENEWOUTFITDONE)
	{
		gAgentWearables.makeNewOutfitDone(mType, mIndex);
	}
}

void LLAgentWearables::addWearabletoAgentInventoryDone(const LLWearableType::EType type,
													   const U32 index,
													   const LLUUID& item_id,
													   LLWearable* wearable)
{
	llinfos << "type " << type << " index " << index << " item "
			<< item_id.asString() << llendl;

	if (item_id.isNull())
	{
		return;
	}

	LLUUID old_item_id = getWearableItemID(type, index);

	if (wearable)
	{
		wearable->setItemID(item_id);

		if (old_item_id.notNull())
		{
			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
			setWearable(type, index, wearable);
		}
		else
		{
			pushWearable(type, wearable);
		}
	}

	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if (item && wearable)
	{
		// We're changing the asset id, so we both need to set it
		// locally via setAssetUUID() and via setTransactionID() which
		// will be decoded on the server. JC
		item->setAssetUUID(wearable->getAssetID());
		item->setTransactionID(wearable->getTransactionID());
		gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item_id);
		item->updateServer(FALSE);
	}
	gInventory.notifyObservers();
}

void LLAgentWearables::sendAgentWearablesUpdate()
{
	// First make sure that we have inventory items for each wearable
	for (S32 type = 0; type < LLWearableType::WT_COUNT; ++type)
	{
		for (U32 index = 0;
			 index < getWearableCount((LLWearableType::EType)type); ++index)
		{
			LLWearable* wearable = getWearable((LLWearableType::EType)type,index);
			if (wearable)
			{
				if (wearable->getItemID().isNull())
				{
					LLPointer<LLInventoryCallback> cb =
						new addWearableToAgentInventoryCallback(LLPointer<LLRefCount>(NULL),
																(LLWearableType::EType)type,
																index,
																wearable,
																addWearableToAgentInventoryCallback::CALL_NONE);
					addWearableToAgentInventory(cb, wearable);
				}
				else
				{
					gInventory.addChangedMask(LLInventoryObserver::LABEL,
											  wearable->getItemID());
				}
			}
		}
	}

	// Then make sure the inventory is in sync with the avatar.
	gInventory.notifyObservers();

	// Send the AgentIsNowWearing 
	gMessageSystem->newMessageFast(_PREHASH_AgentIsNowWearing);

	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	LL_DEBUGS("Wearables") << "sendAgentWearablesUpdate()" << LL_ENDL;
	// MULTI-WEARABLE: DEPRECATED: HACK: index to 0 - server database tables
	// don't support concept of multiwearables.
	for (S32 type = 0; type < LLWearableType::WT_COUNT; ++type)
	{
		gMessageSystem->nextBlockFast(_PREHASH_WearableData);

		U8 type_u8 = (U8)type;
		gMessageSystem->addU8Fast(_PREHASH_WearableType, type_u8);

		LLWearable* wearable = getWearable((LLWearableType::EType)type, 0);
		if (wearable)
		{
			LLUUID item_id = wearable->getItemID();
			LL_DEBUGS("Wearables") << "Sending wearable " << wearable->getName()
								   << " mItemID = " << item_id << LL_ENDL;
			const LLViewerInventoryItem *item = gInventory.getItem(item_id);
			if (item && item->getIsLinkType())
			{
				// Get the itemID that this item points to.  i.e. make sure
				// we are storing baseitems, not their links, in the database.
				item_id = item->getLinkedUUID();
			}
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, item_id);
		}
		else
		{
			LL_DEBUGS("Wearables") << "Not wearing wearable type: "
								   << LLWearableType::getTypeName((LLWearableType::EType)type)
								   << LL_ENDL;
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID::null);
		}

		LL_DEBUGS("Wearables") << "       "
							   << LLWearableType::getTypeLabel((LLWearableType::EType)type)
							   << ": "
							   << (wearable ? wearable->getAssetID() : LLUUID::null)
							   << LL_ENDL;
	}
	gAgent.sendReliableMessage();
}

void LLAgentWearables::saveWearable(const LLWearableType::EType type,
								    const U32 index,
									BOOL send_update,
									const std::string new_name)
{
	LLWearable* old_wearable = getWearable(type, index);
	if (!old_wearable || !isAgentAvatarValid()) return;
	bool name_changed = !new_name.empty() && new_name != old_wearable->getName();
	if (name_changed || old_wearable->isDirty() || old_wearable->isOldVersion())
	{
		LLUUID old_item_id = old_wearable->getItemID();
		LLWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable, "");
		new_wearable->setItemID(old_item_id); // should this be in LLWearable::copyDataFrom()?
		setWearable(type, index, new_wearable);
#if 1
		// old_wearable may still be referred to by other inventory items. Revert
		// unsaved changes so other inventory items aren't affected by the changes
		// that were just saved.
		old_wearable->revertValues();
#endif
		LLInventoryItem* item = gInventory.getItem(old_item_id);
		if (item)
		{
			std::string item_name = item->getName();
			if (name_changed)
			{
				llinfos << "saveWearable changing name from " << item->getName()
						<< " to " << new_name << llendl;
				item_name = new_name;
			}
			// Update existing inventory item
			LLPointer<LLViewerInventoryItem> template_item =
				new LLViewerInventoryItem(item->getUUID(),
										  item->getParentUUID(),
										  item->getPermissions(),
										  new_wearable->getAssetID(),
										  new_wearable->getAssetType(),
										  item->getInventoryType(),
										  item_name,
										  item->getDescription(),
										  item->getSaleInfo(),
										  item->getFlags(),
										  item->getCreationDate());
			template_item->setTransactionID(new_wearable->getTransactionID());
			template_item->updateServer(FALSE);
			gInventory.updateItem(template_item);
			if (name_changed)
			{
				gInventory.notifyObservers();
			}
		}
		else
		{
			// Add a new inventory item (shouldn't ever happen here)
			U32 todo = addWearableToAgentInventoryCallback::CALL_NONE;
			if (send_update)
			{
				todo |= addWearableToAgentInventoryCallback::CALL_UPDATE;
			}
			LLPointer<LLInventoryCallback> cb =
				new addWearableToAgentInventoryCallback(LLPointer<LLRefCount>(NULL),
														type, index,
														new_wearable, todo);
			addWearableToAgentInventory(cb, new_wearable);
			return;
		}

		gAgentAvatarp->wearableUpdated(type, TRUE);

		if (send_update)
		{
			sendAgentWearablesUpdate();
		}
	}
}

void LLAgentWearables::saveWearableAs(const LLWearableType::EType type,
									  const U32 index,
									  const std::string& new_name,
									  BOOL save_in_lost_and_found)
{
	if (!isWearableCopyable(type, index))
	{
		llwarns << "LLAgent::saveWearableAs() not copyable." << llendl;
		return;
	}
	LLWearable* old_wearable = getWearable(type, index);
	if (!old_wearable)
	{
		llwarns << "LLAgent::saveWearableAs() no old wearable." << llendl;
		return;
	}

	LLInventoryItem* item = gInventory.getItem(getWearableItemID(type, index));
	if (!item)
	{
		llwarns << "LLAgent::saveWearableAs() no inventory item." << llendl;
		return;
	}
	std::string trunc_name(new_name);
	LLStringUtil::truncate(trunc_name, DB_INV_ITEM_NAME_STR_LEN);
	LLWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable,
																	 trunc_name);
	LLPointer<LLInventoryCallback> cb =
		new addWearableToAgentInventoryCallback(LLPointer<LLRefCount>(NULL),
												type, index, new_wearable,
												addWearableToAgentInventoryCallback::CALL_UPDATE);
	LLUUID category_id;
	if (save_in_lost_and_found)
	{
		category_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	}
	else
	{
		// put in same folder as original
		category_id = item->getParentUUID();
	}

	copy_inventory_item(gAgentID, item->getPermissions().getOwner(),
						item->getUUID(), category_id, new_name, cb);
#if 1
	// old_wearable may still be referred to by other inventory items. Revert
	// unsaved changes so other inventory items aren't affected by the changes
	// that were just saved.
	old_wearable->revertValues();
#endif
}

void LLAgentWearables::revertWearable(const LLWearableType::EType type, const U32 index)
{
	LLWearable* wearable = getWearable(type, index);
	if (wearable)
	{
		wearable->revertValues();
	}
	gAgent.sendAgentSetAppearance();
}

void LLAgentWearables::saveAllWearables()
{
	//if (!gInventory.isLoaded())
	//{
	//	return;
	//}

	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j = 0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			saveWearable((LLWearableType::EType)i, j, FALSE);
		}
	}
	sendAgentWearablesUpdate();
}

// Called when the user changes the name of a wearable inventory item that is
// currently being worn.
void LLAgentWearables::setWearableName(const LLUUID& item_id,
									   const std::string& new_name)
{
	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j = 0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLUUID curr_item_id = getWearableItemID((LLWearableType::EType)i, j);
			if (curr_item_id == item_id)
			{
				LLWearable* old_wearable = getWearable((LLWearableType::EType)i, j);
				llassert(old_wearable);	//if (!old_wearable) continue;

				std::string old_name = old_wearable->getName();
				old_wearable->setName(new_name);
				LLWearable* new_wearable = LLWearableList::instance().createCopy(old_wearable);
				new_wearable->setItemID(item_id);
				LLInventoryItem* item = gInventory.getItem(item_id);
				if (item)
				{
					new_wearable->setPermissions(item->getPermissions());
				}
				old_wearable->setName(old_name);

				setWearable((LLWearableType::EType)i, j, new_wearable);
				sendAgentWearablesUpdate();
				break;
			}
		}
	}
}

BOOL LLAgentWearables::isWearableModifiable(LLWearableType::EType type, U32 index) const
{
	LLUUID item_id = getWearableItemID(type, index);
	return item_id.notNull() ? isWearableModifiable(item_id) : FALSE;
}

BOOL LLAgentWearables::isWearableModifiable(const LLUUID& item_id) const
{
	const LLUUID& linked_id = gInventory.getLinkedItemID(item_id);
	if (linked_id.notNull())
	{
		LLInventoryItem* item = gInventory.getItem(linked_id);
		if (item && item->getPermissions().allowModifyBy(gAgentID,
														 gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLAgentWearables::isWearableCopyable(LLWearableType::EType type, U32 index) const
{
	LLUUID item_id = getWearableItemID(type, index);
	if (!item_id.isNull())
	{
		LLInventoryItem* item = gInventory.getItem(item_id);
		if (item && item->getPermissions().allowCopyBy(gAgentID,
													   gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}

LLInventoryItem* LLAgentWearables::getWearableInventoryItem(LLWearableType::EType type, U32 index)
{
	LLUUID item_id = getWearableItemID(type, index);
	LLInventoryItem* item = NULL;
	if (item_id.notNull())
	{
		item = gInventory.getItem(item_id);
	}
	return item;
}

const LLWearable* LLAgentWearables::getWearableFromItemID(const LLUUID& item_id) const
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j = 0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			const LLWearable* curr_wearable = getWearable((LLWearableType::EType)i, j);
			if (curr_wearable && (curr_wearable->getItemID() == base_item_id))
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}

LLWearable* LLAgentWearables::getWearableFromItemID(const LLUUID& item_id)
{
	const LLUUID& base_item_id = gInventory.getLinkedItemID(item_id);
	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j = 0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLWearable * curr_wearable = getWearable((LLWearableType::EType)i, j);
			if (curr_wearable && curr_wearable->getItemID() == base_item_id)
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}

LLWearable*	LLAgentWearables::getWearableFromAssetID(const LLUUID& asset_id)
{
	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		for (U32 j = 0; j < getWearableCount((LLWearableType::EType)i); j++)
		{
			LLWearable * curr_wearable = getWearable((LLWearableType::EType)i, j);
			if (curr_wearable && curr_wearable->getAssetID() == asset_id)
			{
				return curr_wearable;
			}
		}
	}
	return NULL;
}

void LLAgentWearables::sendAgentWearablesRequest()
{
	gMessageSystem->newMessageFast(_PREHASH_AgentWearablesRequest);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gAgent.sendReliableMessage();
}

// static
BOOL LLAgentWearables::selfHasWearable(LLWearableType::EType type)
{
	return (gAgentWearables.getWearableCount(type) > 0);
}

LLWearable* LLAgentWearables::getWearable(const LLWearableType::EType type, U32 index)
{
	wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return NULL;
	}
	wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index >= wearable_vec.size())
	{
		return NULL;
	}
	else
	{
		return wearable_vec[index];
	}
}

void LLAgentWearables::setWearable(const LLWearableType::EType type, U32 index,
								   LLWearable* wearable)
{

//MK
	if (gRRenabled && !gAgent.mRRInterface.canWear(type))
	{
		return;
	}
//mk
	LLWearable* old_wearable = getWearable(type, index);
	if (!old_wearable)
	{
		pushWearable(type, wearable);
		wearableUpdated(wearable);
		return;
	}
//MK
	if (gRRenabled && !gAgent.mRRInterface.canUnwear(type))
	{
		// cannot remove this outfit, so cannot replace it either
		return;
	}
//mk
	wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		llwarns << "invalid type, type " << type << " index " << index << llendl; 
		return;
	}
	wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index >= wearable_vec.size())
	{
		llwarns << "invalid index, type " << type << " index " << index << llendl; 
	}
	else
	{
		wearable_vec[index] = wearable;
		old_wearable->setLabelUpdated();
		wearableUpdated(wearable);
	}
}

U32 LLAgentWearables::pushWearable(const LLWearableType::EType type, LLWearable* wearable)
{
	if (wearable == NULL)
	{
		// no null wearables please!
		llwarns << "Null wearable sent for type " << type << llendl;
		return MAX_CLOTHING_PER_TYPE;
	}
	if (type < LLWearableType::WT_COUNT &&
		getWearableCount(type) < MAX_CLOTHING_PER_TYPE)
	{
//MK
		if (gRRenabled && !gAgent.mRRInterface.canWear(type))
		{
			return MAX_CLOTHING_PER_TYPE;
		}
//mk
		mWearableDatas[type].push_back(wearable);
		wearableUpdated(wearable);
		return mWearableDatas[type].size() - 1;
	}
	return MAX_CLOTHING_PER_TYPE;
}

void LLAgentWearables::wearableUpdated(LLWearable* wearable)
{
	if (!wearable || !isAgentAvatarValid()) return;
	const LLWearableType::EType type = wearable->getType();
	gAgentAvatarp->wearableUpdated(type, FALSE);
	wearable->refreshName();
	wearable->setLabelUpdated();

	wearable->pullCrossWearableValues();

	// Hack pt 2. If the wearable we just loaded has definition version 24,
	// then force a re-save of this wearable after slamming the version number to 22.
	// This number was incorrectly incremented for internal builds before release, and
	// this fix will ensure that the affected wearables are re-saved with the right version number.
	// the versions themselves are compatible. This code can be removed before release.
	if (wearable->getDefinitionVersion() == 24)
	{
		wearable->setDefinitionVersion(22);
		U32 index = getWearableIndex(wearable);
		llinfos << "forcing werable type " << wearable->getType()
				<< " to version 22 from 24" << llendl;
		saveWearable(wearable->getType(), index, TRUE);
	}

	if (gFloaterCustomize)
	{
		gFloaterCustomize->updateWearableType(type, wearable);
	}
}

void LLAgentWearables::popWearable(LLWearable *wearable)
{
	if (wearable == NULL)
	{
		// nothing to do here. move along.
		return;
	}

	U32 index = getWearableIndex(wearable);
	LLWearableType::EType type = wearable->getType();

	if (index < MAX_CLOTHING_PER_TYPE && index < getWearableCount(type))
	{
		popWearable(type, index);
	}
}

void LLAgentWearables::popWearable(const LLWearableType::EType type, U32 index)
{
	LLWearable* wearable = getWearable(type, index);
	if (wearable && isAgentAvatarValid())
	{
		mWearableDatas[type].erase(mWearableDatas[type].begin() + index);
		if (isAgentAvatarValid())
		{
			gAgentAvatarp->wearableUpdated(wearable->getType(), TRUE);
		}
		wearable->setLabelUpdated();

		if (gFloaterCustomize)
		{
			gFloaterCustomize->updateWearableType(type, NULL);
		}
	}
}

U32	LLAgentWearables::getWearableIndex(const LLWearable *wearable) const
{
	if (wearable == NULL)
	{
		return MAX_CLOTHING_PER_TYPE;
	}

	const LLWearableType::EType type = wearable->getType();
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		llwarns << "tried to get wearable index with an invalid type!" << llendl;
		return MAX_CLOTHING_PER_TYPE;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	for (U32 index = 0; index < wearable_vec.size(); index++)
	{
		if (wearable_vec[index] == wearable)
		{
			return index;
		}
	}

	return MAX_CLOTHING_PER_TYPE;
}

const LLWearable* LLAgentWearables::getWearable(const LLWearableType::EType type, U32 index) const
{
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return NULL;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index >= wearable_vec.size())
	{
		return NULL;
	}
	else
	{
		return wearable_vec[index];
	}
}

LLWearable* LLAgentWearables::getTopWearable(const LLWearableType::EType type)
{
	U32 count = getWearableCount(type);
	if (count == 0)
	{
		return NULL;
	}

	return getWearable(type, count - 1);
}

LLWearable* LLAgentWearables::getBottomWearable(const LLWearableType::EType type)
{
	if (getWearableCount(type) == 0)
	{
		return NULL;
	}

	return getWearable(type, 0);
}

U32 LLAgentWearables::getWearableCount(const LLWearableType::EType type) const
{
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return 0;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	return wearable_vec.size();
}

U32 LLAgentWearables::getWearableCount(const U32 tex_index) const
{
	const LLWearableType::EType wearable_type = LLVOAvatarDictionary::getTEWearableType((LLVOAvatarDefines::ETextureIndex)tex_index);
	return getWearableCount(wearable_type);
}

const LLUUID LLAgentWearables::getWearableItemID(LLWearableType::EType type,
												 U32 index) const
{
	const LLWearable* wearable = getWearable(type, index);
	if (wearable)
	{
		return wearable->getItemID();
	}
	else
	{
		return LLUUID();
	}
}

const LLUUID LLAgentWearables::getWearableAssetID(LLWearableType::EType type,
												  U32 index) const
{
	const LLWearable* wearable = getWearable(type, index);
	if (wearable)
	{
		return wearable->getAssetID();
	}
	else
	{
		return LLUUID();
	}
}

BOOL LLAgentWearables::isWearingItem(const LLUUID& item_id) const
{
	return getWearableFromItemID(gInventory.getLinkedItemID(item_id)) != NULL;
}

// MULTI-WEARABLE: DEPRECATED
// ! BACKWARDS COMPATIBILITY ! When we stop supporting viewer 1.23, we can
// assume that viewers have a Current Outfit Folder and won't need this message,
// and thus we can remove/ignore this whole function.
// static
void LLAgentWearables::processAgentInitialWearablesUpdate(LLMessageSystem* mesgsys,
														  void** user_data)
{
	// We should only receive this message a single time. Ignore subsequent
	// AgentWearablesUpdates that may result from AgentWearablesRequest having
	// been sent more than once.
	if (mInitialWearablesUpdateReceived)
	{
		return;
	}
	mInitialWearablesUpdateReceived = true;

	LLUUID agent_id;
	gMessageSystem->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);

	if (isAgentAvatarValid() && agent_id == gAgentAvatarp->getID())
	{
		gMessageSystem->getU32Fast(_PREHASH_AgentData, _PREHASH_SerialNum,
								   gAgentQueryManager.mUpdateSerialNum);

		const S32 NUM_BODY_PARTS = 4;
		S32 num_wearables = gMessageSystem->getNumberOfBlocksFast(_PREHASH_WearableData);
		if (num_wearables < NUM_BODY_PARTS)
		{
			// Transitional state.  Avatars should always have at least their
			// body parts (hair, eyes, shape and skin).
			// The fact that they don't have any here (only a dummy is sent)
			// implies that either:
			// 1. This account existed before we had wearables
			// 2. The database has gotten messed up
			// 3. This is the account's first login (i.e. the wearables haven't
			//    been generated yet).
			return;
		}

		LL_DEBUGS("Wearables") << "processAgentInitialWearablesUpdate()"
							   << LL_ENDL;
		// Add wearables
		// MULTI-WEARABLE: DEPRECATED: Message only supports one wearable per
		// type, will be ignored in future.
		std::pair<LLUUID, LLUUID> asset_id_array[LLWearableType::WT_COUNT];
		for (S32 i = 0; i < num_wearables; i++)
		{
			// Parse initial wearables data from message system
			U8 type_u8 = 0;
			gMessageSystem->getU8Fast(_PREHASH_WearableData, _PREHASH_WearableType, type_u8, i);
			if (type_u8 >= LLWearableType::WT_COUNT)
			{
				continue;
			}
			LLWearableType::EType type = (LLWearableType::EType)type_u8;

			LLUUID item_id;
			gMessageSystem->getUUIDFast(_PREHASH_WearableData, _PREHASH_ItemID, item_id, i);

			LLUUID asset_id;
			gMessageSystem->getUUIDFast(_PREHASH_WearableData, _PREHASH_AssetID, asset_id, i);
			if (asset_id.isNull())
			{
//MK
				gAgent.mRRInterface.mRestoringOutfit = true;
//mk
				LLWearable::removeFromAvatar(type, FALSE);
//MK
				gAgent.mRRInterface.mRestoringOutfit = false;
//mk
			}
			else
			{
				LLAssetType::EType asset_type = LLWearableType::getAssetType(type);
				if (asset_type == LLAssetType::AT_NONE)
				{
					continue;
				}

				asset_id_array[type] = std::make_pair(asset_id, item_id);
			}

			LL_DEBUGS("Wearables") << "       " << LLWearableType::getTypeLabel(type)
								   << " " << asset_id << " item id "
								   << gAgentWearables.getWearableItemID(type, 0).asString()
								   << LL_ENDL;
		}

		// now that we have the asset ids...request the wearable assets
		for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
		{
			LL_DEBUGS("Wearables") << "      fetching "
								   << asset_id_array[i].first << LL_ENDL;
			const LLUUID item_id = asset_id_array[i].second;
			if (asset_id_array[i].second.notNull())
			{
				LLWearableList::instance().getAsset(asset_id_array[i].first,
													LLStringUtil::null,
													LLWearableType::getAssetType((LLWearableType::EType)i),
													LLAgentWearables::onInitialWearableAssetArrived,
													(void*)new std::pair<const LLWearableType::EType,
																		 const LLUUID>((LLWearableType::EType)i,
																					   item_id));
			}
		}
	}
}

// A single wearable that the avatar was wearing on start-up has arrived from
// the database.
// static
void LLAgentWearables::onInitialWearableAssetArrived(LLWearable* wearable, void* userdata)
{
	std::pair<const LLWearableType::EType,const LLUUID>* wearable_data;
	wearable_data = (std::pair<const LLWearableType::EType,const LLUUID>*)userdata;
	LLWearableType::EType type = wearable_data->first;
	LLUUID item_id = wearable_data->second;

	if (!isAgentAvatarValid())
	{
		return;
	}

//MK
	gAgent.mRRInterface.mRestoringOutfit = true;
//mk
	if (wearable)
	{
		llassert(type == wearable->getType());
		wearable->setItemID(item_id);
		gAgentWearables.setWearable(type, 0, wearable);

		// disable composites if initial textures are baked
		gAgentAvatarp->setupComposites();
		gAgentWearables.queryWearableCache();

		gAgentAvatarp->setCompositeUpdatesEnabled(TRUE);
		gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
	}
	else
	{
		// Somehow the asset doesn't exist in the database.
		gAgentWearables.recoverMissingWearable(type, 0);
	}
//MK
	gAgent.mRRInterface.mRestoringOutfit = false;
//mk

	gInventory.notifyObservers();

	// Have all the wearables that the avatar was wearing at log-in arrived?
	if (!gAgentWearables.mWearablesLoaded)
	{
		gAgentWearables.mWearablesLoaded = TRUE;
		for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
		{
			if (gAgentWearables.getWearableItemID((LLWearableType::EType)i, 0).notNull() &&
				!gAgentWearables.getWearable((LLWearableType::EType)i, 0))
			{
				gAgentWearables.mWearablesLoaded = FALSE;
				break;
			}
		}
	}

	if (gAgentWearables.mWearablesLoaded)
	{
		// Make sure that the server's idea of the avatar's wearables actually
		// match the wearables.
		gAgent.sendAgentSetAppearance();

		// Check to see if there are any baked textures that we hadn't uploaded
		// before we logged off last time.
		// If there are any, schedule them to be uploaded as soon as the layer
		// textures they depend on arrive.
		if (!gAgent.cameraCustomizeAvatar())
		{
			gAgentAvatarp->requestLayerSetUploads();
		}
	}
}

// Normally, all wearables referred to "AgentWearablesUpdate" will correspond
// to actual assets in the database. If for some reason, we can't load one of
// those assets, we can try to reconstruct it so that the user isn't left
// without a shape, for example. We can do that only after the inventory has
// loaded.
void LLAgentWearables::recoverMissingWearable(const LLWearableType::EType type,
											  U32 index)
{
	// Try to recover by replacing missing wearable with a new one.
	LLNotifications::instance().add("ReplacedMissingWearable");
	LL_DEBUGS("Wearables") << "Wearable " << LLWearableType::getTypeLabel(type)
						   << " could not be downloaded. Replaced inventory item with default wearable."
						   << LL_ENDL;
	LLWearable* new_wearable = LLWearableList::instance().createNewWearable(type);

	setWearable(type, index, new_wearable);

	// Add a new one in the lost and found folder.
	// (We used to overwrite the "not found" one, but that could potentially
	// destroy content.) JC
	const LLUUID lost_and_found_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	LLPointer<LLInventoryCallback> cb =
		new addWearableToAgentInventoryCallback(LLPointer<LLRefCount>(NULL),
												type, index, new_wearable,
												addWearableToAgentInventoryCallback::CALL_RECOVERDONE);
	addWearableToAgentInventory(cb, new_wearable, lost_and_found_id, TRUE);
}

void LLAgentWearables::recoverMissingWearableDone()
{
	// Have all the wearables that the avatar was wearing at log-in arrived or
	// been fabricated?
	updateWearablesLoaded();
	if (areWearablesLoaded())
	{
		// Make sure that the server's idea of the avatar's wearables actually
		// match the wearables.
		gAgent.sendAgentSetAppearance();
	}
	else
	{
		gInventory.addChangedMask(LLInventoryObserver::LABEL, LLUUID::null);
		gInventory.notifyObservers();
	}
}

void LLAgentWearables::addLocalTextureObject(const LLWearableType::EType wearable_type,
											 const LLVOAvatarDefines::ETextureIndex texture_type,
											 U32 wearable_index)
{
	LLWearable* wearable = getWearable((LLWearableType::EType)wearable_type,
									   wearable_index);
	if (!wearable)
	{
		llerrs << "Tried to add local texture object to invalid wearable with type "
			   << wearable_type << " and index " << wearable_index << llendl;
		return;
	}
	LLLocalTextureObject lto;
	wearable->setLocalTextureObject(texture_type, lto);
}

void LLAgentWearables::createStandardWearables(BOOL female)
{
	llwarns << "Creating Standard " << (female ? "female" : "male")
			<< " Wearables" << llendl;

	if (!isAgentAvatarValid())
	{
		return;
	}

	gAgentAvatarp->setSex(female ? SEX_FEMALE : SEX_MALE);

	BOOL create[LLWearableType::WT_COUNT] = 
	{
		TRUE,  //WT_SHAPE
		TRUE,  //WT_SKIN
		TRUE,  //WT_HAIR
		TRUE,  //WT_EYES
		TRUE,  //WT_SHIRT
		TRUE,  //WT_PANTS
		TRUE,  //WT_SHOES
		TRUE,  //WT_SOCKS
		FALSE, //WT_JACKET
		FALSE, //WT_GLOVES
		TRUE,  //WT_UNDERSHIRT
		TRUE,  //WT_UNDERPANTS
		FALSE, //WT_SKIRT
		FALSE, //WT_ALPHA
		FALSE, //WT_TATTOO
		FALSE  //WT_PHYSICS
	};

	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		bool once = false;
		LLPointer<LLRefCount> donecb = NULL;
		if (create[i])
		{
			if (!once)
			{
				once = true;
				donecb = new createStandardWearablesAllDoneCallback;
			}
			llassert(getWearableCount((LLWearableType::EType)i) == 0);
			LLWearable* wearable = LLWearableList::instance().createNewWearable((LLWearableType::EType)i);
			// no need to update here...
			LLPointer<LLInventoryCallback> cb =
				new addWearableToAgentInventoryCallback(donecb,
														(LLWearableType::EType)i,
														0, wearable,
														addWearableToAgentInventoryCallback::CALL_CREATESTANDARDDONE);
			addWearableToAgentInventory(cb, wearable, LLUUID::null, FALSE);
		}
	}
}

void LLAgentWearables::createStandardWearablesDone(S32 type, U32 index)
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->updateVisualParams();
	}
}

void LLAgentWearables::createStandardWearablesAllDone()
{
	// ...because sendAgentWearablesUpdate will notify inventory observers.
	mWearablesLoaded = TRUE;
	updateServer();

	// Treat this as the first texture entry message, if none received yet
	gAgentAvatarp->onFirstTEMessageReceived();
}

void LLAgentWearables::makeNewOutfit(const std::string& new_folder_name,
									 const LLDynamicArray<S32>& wearables_to_include,
									 const LLDynamicArray<S32>& attachments_to_include,
									 BOOL rename_clothing)
{
	if (!isAgentAvatarValid())
	{
		return;
	}

	// First, make a folder in the Clothes directory.
	const LLUUID clothing_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
	LLUUID folder_id = gInventory.createNewCategory(clothing_folder_id,
													LLFolderType::FT_NONE,
													new_folder_name);

	bool found_first_item = false;
	bool no_link = !gSavedSettings.getBOOL("UseInventoryLinks") ||
				   (!gIsInSecondLife && !gSavedSettings.getBOOL("OSAllowInventoryLinks"));
	bool do_link = gSavedSettings.getBOOL("UseInventoryLinksAlways") &&
				   (gIsInSecondLife || gSavedSettings.getBOOL("OSAllowInventoryLinks"));
	bool cloth_link = gSavedSettings.getBOOL("UseInventoryLinksForClothes") &&
					  (gIsInSecondLife || gSavedSettings.getBOOL("OSAllowInventoryLinks"));

	///////////////////
	// Wearables

	if (wearables_to_include.count())
	{
		// Then, iterate though each of the wearables and save copies of them
		// in the folder.
		S32 i;
		S32 count = wearables_to_include.count();
		LLPointer<LLRefCount> cbdone = NULL;
		LLWearableType::EType type;
		LLWearable* old_wearable;
		LLWearable* new_wearable;
		LLViewerInventoryItem* item;
		std::string new_name;
		for (i = 0; i < count; ++i)
		{
			type = (LLWearableType::EType)wearables_to_include[i];
			bool use_link = do_link ||
							(cloth_link && type >= LLWearableType::WT_SHIRT);
			for (U32 index = 0; index < getWearableCount(type); index++)
			{
				old_wearable = getWearable(type, index);
				if (old_wearable)
				{
					item = gInventory.getItem(old_wearable->getItemID());
					if (!item)
					{
						llwarns << "Could not find inventory item for wearable type: "
								<< (U32)type << " - index: " << index << llendl;
						continue;
					}
					new_name = item->getName();
					if (rename_clothing)
					{
						new_name = new_folder_name;
						new_name.append(" ");
						new_name.append(old_wearable->getTypeLabel());
						LLStringUtil::truncate(new_name, DB_INV_ITEM_NAME_STR_LEN);
					}

					if (!use_link && (no_link || isWearableCopyable(type, index)))
					{
						new_wearable = LLWearableList::instance().createCopy(old_wearable);
						if (rename_clothing)
						{
							new_wearable->setName(new_name);
						}

						S32 todo = addWearableToAgentInventoryCallback::CALL_NONE;
						if (!found_first_item)
						{
							found_first_item = true;
							/* set the focus to the first item */
							todo |= addWearableToAgentInventoryCallback::CALL_MAKENEWOUTFITDONE;
							/* send the agent wearables update when done */
							cbdone = new sendAgentWearablesUpdateCallback;
						}
						LLPointer<LLInventoryCallback> cb =
							new addWearableToAgentInventoryCallback(cbdone,
																	type,
																	index,
																	new_wearable,
																	todo);
						if (isWearableCopyable(type, index))
						{
							copy_inventory_item(gAgentID,
												item->getPermissions().getOwner(),
												item->getLinkedUUID(),
												folder_id,
												new_name,
												cb);
						}
						else
						{
							move_inventory_item(gAgentID,
												gAgent.getSessionID(),
												item->getLinkedUUID(),
												folder_id,
												new_name,
												cb);
						}
					}
					else
					{
						link_inventory_item(gAgentID,
											item->getLinkedUUID(),
											folder_id,
											item->getName(),	// Links cannot be given arbitrary names...
											build_order_string(type, index),	// For auto-ordering on outfit wearing
											LLAssetType::AT_LINK,
											LLPointer<LLInventoryCallback>(NULL));
					}
				}
			}
		}
		gInventory.notifyObservers();
	}

	///////////////////
	// Attachments

	if (attachments_to_include.count())
	{
		LLViewerJointAttachment* attachment;
		for (S32 i = 0; i < attachments_to_include.count(); i++)
		{
			S32 attachment_pt = attachments_to_include[i];
			attachment = get_if_there(gAgentAvatarp->mAttachmentPoints,
									  attachment_pt, (LLViewerJointAttachment*)NULL);
			if (!attachment) continue;
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
 			{
				LLViewerObject* attached_object = *attachment_iter;
				if (!attached_object) continue;
				const LLUUID& item_id = attached_object->getAttachmentItemID();
				if (item_id.isNull()) continue;
				LLInventoryItem* item = gInventory.getItem(item_id);
				if (!item) continue;
				if (!do_link && (no_link ||
					item->getPermissions().allowCopyBy(gAgentID)))
				{
					const LLUUID& old_folder_id = item->getParentUUID();

					move_inventory_item(gAgentID,
										gAgent.getSessionID(),
										item->getLinkedUUID(),
										folder_id,
										item->getName(),
										LLPointer<LLInventoryCallback>(NULL));

					if (item->getPermissions().allowCopyBy(gAgentID))
					{
						copy_inventory_item(gAgentID,
											item->getPermissions().getOwner(),
											item->getLinkedUUID(),
											old_folder_id,
											item->getName(),
											LLPointer<LLInventoryCallback>(NULL));
					}
				}
				else
				{
					link_inventory_item(gAgentID,
										item->getLinkedUUID(),
										folder_id,
										item->getName(),
										item->getDescription(),
										LLAssetType::AT_LINK,
										LLPointer<LLInventoryCallback>(NULL));
				}
			}
		}
		gInventory.notifyObservers();
	} 
}

void LLAgentWearables::makeNewOutfitDone(LLWearableType::EType type, U32 index)
{
	LLUUID first_item_id = getWearableItemID(type, index);
	// Open the inventory and select the first item we added.
	if (first_item_id.notNull())
	{
		LLInventoryView* view = LLInventoryView::getActiveInventory();
		if (view)
		{
			view->getPanel()->setSelection(first_item_id, TAKE_FOCUS_NO);
		}
	}
}

void LLAgentWearables::addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												   LLWearable* wearable,
												   const LLUUID& category_id,
												   BOOL notify)
{
	create_inventory_item(gAgentID,
						  gAgent.getSessionID(),
						  category_id,
						  wearable->getTransactionID(),
						  wearable->getName(),
						  wearable->getDescription(),
						  wearable->getAssetType(),
						  LLInventoryType::IT_WEARABLE,
						  wearable->getType(),
						  wearable->getPermissions().getMaskNextOwner(),
						  cb);
}

void LLAgentWearables::removeWearable(const LLWearableType::EType type,
									  bool do_remove_all, U32 index)
{
	bool is_teen_and_underwear = gAgent.isTeen() &&
								 (type == LLWearableType::WT_UNDERSHIRT ||
								  type == LLWearableType::WT_UNDERPANTS);
//MK
	if (gRRenabled && !gAgent.mRRInterface.canUnwear(type))
	{
		return;
	}
//mk
	U32 count = getWearableCount(type);
	if (count == 0 || (is_teen_and_underwear && count == 1))
	{
		// no wearable to remove or teen trying to remove their last underwear
		return;
	}

	if (do_remove_all)
	{
		if (is_teen_and_underwear)
		{
			// Remove all but one layer
			for (U32 index = count - 1; index > 0; index--)
			{
				removeWearableFinal(type, false, index);
			}
		}
		else
		{
			removeWearableFinal(type, true, 0);
		}
	}
	else
	{
		LLWearable* old_wearable = getWearable(type, index);

		if (old_wearable)
		{
			if (old_wearable->isDirty())
			{
				LLSD payload;
				payload["wearable_type"] = (S32)type;
				payload["wearable_index"] = (S32)index;
				// Bring up view-modal dialog: Save changes? Yes, No, Cancel
				LLNotifications::instance().add("WearableSave", LLSD(), payload,
												onRemoveWearableDialog);
				return;
			}
			else
			{
				removeWearableFinal(type, do_remove_all, index);
			}
		}
	}
}

// static 
bool LLAgentWearables::onRemoveWearableDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLWearableType::EType type = (LLWearableType::EType)notification["payload"]["wearable_type"].asInteger();
	S32 index = (S32)notification["payload"]["wearable_index"].asInteger();
	switch (option)
	{
		case 0:  // "Save"
			gAgentWearables.saveWearable(type, index);
			gAgentWearables.removeWearableFinal(type, false, index);
			break;

		case 1:  // "Don't Save"
			gAgentWearables.removeWearableFinal(type, false, index);
			break;

		case 2: // "Cancel"
			break;

		default:
			llassert(0);
			break;
	}
	return false;
}

// Called by removeWearable() and onRemoveWearableDialog() to actually do the removal.
void LLAgentWearables::removeWearableFinal(const LLWearableType::EType type,
										   bool do_remove_all,
										   U32 index)
{
	if (do_remove_all)
	{
		S32 max_entry = mWearableDatas[type].size() - 1;
		for (S32 i = max_entry; i >= 0; i--)
		{
			LLWearable* old_wearable = getWearable(type, i);
			//queryWearableCache(); // moved below
			if (old_wearable)
			{
//MK
				if (gRRenabled)
				{
					LLViewerInventoryItem* old_item;
					old_item = gInventory.getItem(old_wearable->getItemID());
					if (old_item && !gAgent.mRRInterface.canUnwear(old_item))
					{
						continue;
					}
				}
//mk
				popWearable(old_wearable);
				old_wearable->removeFromAvatar(TRUE);
			}
		}
		mWearableDatas[type].clear();
	}
	else
	{
		LLWearable* old_wearable = getWearable(type, index);
		//queryWearableCache(); // moved below

		if (old_wearable)
		{
//MK
			if (gRRenabled)
			{
				LLViewerInventoryItem* old_item;
				old_item = gInventory.getItem(old_wearable->getItemID());
				if (old_item && !gAgent.mRRInterface.canUnwear(old_item))
				{
					return;
				}
			}
//mk
			popWearable(old_wearable);
			old_wearable->removeFromAvatar(TRUE);
		}
	}

//MK
	if (gRRenabled)
	{
		std::string layer = gAgent.mRRInterface.getOutfitLayerAsString(type);
		if (!layer.empty())
		{
			gAgent.mRRInterface.notify(LLUUID::null, "unworn legally " + layer, "");
		}
	}
//mk

	queryWearableCache();

	// Update the server
	updateServer();
}

// Assumes existing wearables are not dirty.
void LLAgentWearables::setWearableOutfit(const LLInventoryItem::item_array_t& items,
										 const LLDynamicArray<LLWearable*>& wearables,
										 BOOL remove)
{
	LL_DEBUGS("Wearables") << "setWearableOutfit() start" << LL_ENDL;

	S32 i;
	S32 count = wearables.count();
	if (count == 0) return;
	llassert(items.count() == count);

	mIsSettingOutfit = true;

	// Keep track of all worn AT_BODYPART wearables that are to be replaced
	// with a new bodypart of the same type, so that we can remove them prior
	// to wearing the new ones (removing the replaced body parts must be done
	// to avoid seeing removed items still flagged as "worn" (label in bold)
	// in the inventory when they are copies of the newly worn body part (e.g.
	// when you have a copy of the same eyes in the newly worn outfit folder
	// as the eyes you were wearing before).
	// Also check for duplicate body parts (so to wear only one of each type),
	// scanning backwards to stay compatible with older viewers behaviour as
	// to which duplicate part will actually be worn in the end.
	std::set<LLWearableType::EType> new_bodyparts;
	std::set<S32> skip_wearable;
	for (i = count - 1; i >= 0; i--)
	{
		LLWearableType::EType type = wearables[i]->getType();
		if (LLWearableType::getAssetType(type) == LLAssetType::AT_BODYPART)
		{
			if (new_bodyparts.count(type) ||
//MK
				(gRRenabled && !gAgent.mRRInterface.canUnwear(type)))
//mk
			{
				// Do not try and wear two body parts of the same type
				// or to replace a body part that RestrainedLove locked
				skip_wearable.insert(i);
			}
			else
			{
				// Keep track of what body part type is present in the new
				// outift we are going to wear
				new_bodyparts.insert(type);
			}
		}
	}

	// When remove == true, this loop removes all clothing.
	// Note that removeWearable() will also take care of checking whether
	// the items can actually be removed (for teens and underwear, and for
	// RestrainedLove locked items).
	// It also always removes the body parts that will be replaced with the new
	// wearables.
	for (i = 0; i < (S32)LLWearableType::WT_COUNT; i++)
	{
		LLWearableType::EType type = (LLWearableType::EType)i;
		if (new_bodyparts.count(type) ||
			(remove &&
			 LLWearableType::getAssetType(type) == LLAssetType::AT_CLOTHING))
		{
			removeWearable(type, true, 0);
		}
	}

	BOOL no_multiple_physics = gSavedSettings.getBOOL("NoMultiplePhysics");
	BOOL no_multiple_shoes = gSavedSettings.getBOOL("NoMultipleShoes");
	BOOL no_multiple_skirts = gSavedSettings.getBOOL("NoMultipleSkirts");

	for (i = 0; i < count; i++)
	{
		if (skip_wearable.count(i)) continue;

		LLWearable* new_wearable = wearables[i];
		LLPointer<LLInventoryItem> new_item = items[i];

		llassert(new_wearable);
		if (new_wearable)
		{
			LLWearableType::EType type = new_wearable->getType();

			new_wearable->setName(new_item->getName());
			new_wearable->setItemID(new_item->getUUID());

			if (((no_multiple_physics && type == LLWearableType::WT_PHYSICS) ||
				 (no_multiple_shoes && type == LLWearableType::WT_SHOES) ||
				 (no_multiple_skirts && type == LLWearableType::WT_SKIRT) ||
				 LLWearableType::getAssetType(type) == LLAssetType::AT_BODYPART))
			{
				// exactly one wearable per body part, or per Physics, or per
				// cloth types which combination is confusing to the user
				setWearable(type, 0, new_wearable);
			}
			else
			{
				pushWearable(type, new_wearable);
			}
		}
	}

	mIsSettingOutfit = false;

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->setCompositeUpdatesEnabled(TRUE);
		gAgentAvatarp->updateVisualParams();
		if (!gAgentAvatarp->getIsCloud())
		{
			// If we have not yet declouded, we may want to use baked texture
			// UUIDs sent from the first objectUpdate message don't overwrite
			// these. If we have already declouded, we've saved these ids as
			// the last known good textures and can invalidate without
			// re-clouding.
			gAgentAvatarp->invalidateAll();
		}
	}

	// Start rendering & update the server
	mWearablesLoaded = TRUE; 
	queryWearableCache();
	updateServer();

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->dumpAvatarTEs("setWearableOutfit");
	}

	LL_DEBUGS("Wearables") << "setWearableOutfit() end" << LL_ENDL;
}

// User has picked "wear on avatar" from a menu.
void LLAgentWearables::setWearableItem(LLInventoryItem* new_item,
									   LLWearable* new_wearable,
									   bool do_append)
{
	if (isWearingItem(new_item->getUUID()))
	{
		llwarns << "wearable " << new_item->getUUID() << " is already worn"
				<< llendl;
		return;
	}

	const LLWearableType::EType type = new_wearable->getType();

//MK
	if (gRRenabled &&
		(gAgent.mRRInterface.contains("addoutfit") ||
		 gAgent.mRRInterface.contains("addoutfit:" +
									  gAgent.mRRInterface.getOutfitLayerAsString(type))))
	{
		return;
	}
//mk
	if (!do_append)
	{
		// Remove old wearable, if any
		// MULTI_WEARABLE: hardwired to 0
		LLWearable* old_wearable = getWearable(type, 0);
		if (old_wearable)
		{
//MK
			if (gRRenabled && !gAgent.mRRInterface.canUnwear(type))
			{
				// cannot remove this outfit, so cannot replace it either
				return;
			}
//mk
			const LLUUID& old_item_id = old_wearable->getItemID();
			if (old_wearable->getAssetID() == new_wearable->getAssetID() &&
				old_item_id == new_item->getUUID())
			{
				LL_DEBUGS("Wearables") << "No change to wearable asset and item: "
									   << LLWearableType::getTypeName(type)
									   << LL_ENDL;
				return;
			}

			if (old_wearable->isDirty())
			{
				// Bring up modal dialog: Save changes? Yes, No, Cancel
				LLSD payload;
				payload["item_id"] = new_item->getUUID();
				LLNotifications::instance().add("WearableSave", LLSD(), payload,
												boost::bind(onSetWearableDialog,
															_1, _2, new_wearable));
				return;
			}
		}
	}

	setWearableFinal(new_item, new_wearable, do_append);
}

// static 
bool LLAgentWearables::onSetWearableDialog(const LLSD& notification, const LLSD& response, LLWearable* wearable)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLInventoryItem* new_item = gInventory.getItem(notification["payload"]["item_id"].asUUID());
	U32 index = gAgentWearables.getWearableIndex(wearable);
	if (!new_item)
	{
		llwarns << "Callback onSetWearableDialog() called for a NULL new item !"
				<< llendl;
		delete wearable;
		return false;
	}

	switch (option)
	{
		case 0:  // "Save"
			gAgentWearables.saveWearable(wearable->getType(), index);
			gAgentWearables.setWearableFinal(new_item, wearable);
			break;

		case 1:  // "Don't Save"
			gAgentWearables.setWearableFinal(new_item, wearable);
			break;

		case 2: // "Cancel"
			break;

		default:
			llassert(0);
			break;
	}

	delete wearable;
	return false;
}

// Called from setWearableItem() and onSetWearableDialog() to actually set the
// wearable.
// MULTI_WEARABLE: unify code after null objects are gone.
void LLAgentWearables::setWearableFinal(LLInventoryItem* new_item,
										LLWearable* new_wearable,
										bool do_append)
{
	const LLWearableType::EType type = new_wearable->getType();

	mIsSettingOutfit = true;

	if (type == LLWearableType::WT_SHAPE || type == LLWearableType::WT_SKIN ||
		type == LLWearableType::WT_HAIR || type == LLWearableType::WT_EYES)
	{
		// Can't wear more than one body part of each type
		do_append = false;
	}

	if (do_append && getWearableItemID(type, 0).notNull())
	{
		new_wearable->setItemID(new_item->getUUID());
		mWearableDatas[type].push_back(new_wearable);
		llinfos << "Added additional wearable for type " << type
				<< " size is now " << mWearableDatas[type].size() << llendl;
		if (gFloaterCustomize)
		{
			gFloaterCustomize->updateWearableType(type, new_wearable);
		}
	}
	else
	{
		// Replace the old wearable with a new one.
		llassert(new_item->getAssetUUID() == new_wearable->getAssetID());

		LLWearable* old_wearable = getWearable(type, 0);
		LLUUID old_item_id;
		if (old_wearable)
		{
			old_item_id = old_wearable->getItemID();
		}
		new_wearable->setItemID(new_item->getUUID());
		setWearable(type, 0, new_wearable);

		if (old_item_id.notNull())
		{
			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
			gInventory.notifyObservers();
		}
		LL_DEBUGS("Wearables") << "Replaced current element 0 for type " << type
							   << " size is now " << mWearableDatas[type].size()
							   << LL_ENDL;
	}

	//llinfos << "LLVOAvatar::setWearableItem()" << llendl;

//MK
	if (gRRenabled)
	{
		// Notify that this layer has been worn
		std::string layer =  gAgent.mRRInterface.getOutfitLayerAsString(type);
		gAgent.mRRInterface.notify(LLUUID::null, "worn legally " + layer, "");
	}
//mk

	mIsSettingOutfit = false;

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->setCompositeUpdatesEnabled(TRUE);
		gAgentAvatarp->updateVisualParams();
		if (!gAgentAvatarp->getIsCloud())
		{
			// If we have not yet declouded, we may want to use baked texture
			// UUIDs sent from the first objectUpdate message don't overwrite
			// these. If we have already declouded, we've saved these ids as
			// the last known good textures and can invalidate without
			// re-clouding.
			gAgentAvatarp->invalidateAll();
		}
	}

	queryWearableCache();
	updateServer();
}

void LLAgentWearables::queryWearableCache()
{
	if (!areWearablesLoaded())
	{
		return;
	}

	// Look up affected baked textures.
	// If they exist:
	//		disallow updates for affected layersets (until dataserver responds with cache request.)
	//		If cache miss, turn updates back on and invalidate composite.
	//		If cache hit, modify baked texture entries.
	//
	// Cache requests contain list of hashes for each baked texture entry.
	// Response is list of valid baked texture assets. (same message)

	gMessageSystem->newMessageFast(_PREHASH_AgentCachedTexture);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addS32Fast(_PREHASH_SerialNum, gAgentQueryManager.mWearablesCacheQueryID);

	S32 num_queries = 0;
	for (U8 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
	{
		LLUUID hash_id = computeBakedTextureHash((EBakedTextureIndex) baked_index);
		if (hash_id.notNull())
		{
			num_queries++;
			// *NOTE: make sure at least one request gets packed

			ETextureIndex te_index = LLVOAvatarDictionary::bakedToLocalTextureIndex((EBakedTextureIndex)baked_index);

			//llinfos << "Requesting texture for hash " << hash << " in baked texture slot " << baked_index << llendl;
			gMessageSystem->nextBlockFast(_PREHASH_WearableData);
			gMessageSystem->addUUIDFast(_PREHASH_ID, hash_id);
			gMessageSystem->addU8Fast(_PREHASH_TextureIndex, (U8)te_index);
		}

		gAgentQueryManager.mActiveCacheQueries[baked_index] = gAgentQueryManager.mWearablesCacheQueryID;
	}

	//VWR-22113: gAgent.getRegion() can return null if invalid, seen here on logout
	if (gAgent.getRegion())
	{
		llinfos << "Requesting texture cache entry for " << num_queries
				<< " baked textures" << llendl;
		gMessageSystem->sendReliable(gAgent.getRegion()->getHost());
		gAgentQueryManager.mNumPendingQueries++;
		gAgentQueryManager.mWearablesCacheQueryID++;
	}
}

LLUUID LLAgentWearables::computeBakedTextureHash(LLVOAvatarDefines::EBakedTextureIndex baked_index,
												 BOOL generate_valid_hash) // Set to false if you want to upload the baked texture w/o putting it in the cache
{
	LLUUID hash_id;
	bool hash_computed = false;
	LLMD5 hash;
	const LLVOAvatarDictionary::BakedEntry* baked_dict;
	baked_dict = LLVOAvatarDictionary::getInstance()->getBakedTexture(baked_index);

	for (U8 i = 0; i < baked_dict->mWearables.size(); i++)
	{
		const LLWearableType::EType baked_type = baked_dict->mWearables[i];
		const U32 num_wearables = getWearableCount(baked_type);
		for (U32 index = 0; index < num_wearables; ++index)
		{
			const LLWearable* wearable = getWearable(baked_type,index);
			if (wearable)
			{
				LLUUID asset_id = wearable->getAssetID();
				hash.update((const unsigned char*)asset_id.mData, UUID_BYTES);
				hash_computed = true;
			}
		}
	}
	if (hash_computed)
	{
		hash.update((const unsigned char*)baked_dict->mWearablesHashID.mData, UUID_BYTES);

		// Add some garbage into the hash so that it becomes invalid.
		if (!generate_valid_hash)
		{
			if (isAgentAvatarValid())
			{
				hash.update((const unsigned char*)gAgentAvatarp->getID().mData, UUID_BYTES);
			}
		}
		hash.finalize();
		hash.raw_digest(hash_id.mData);
	}

	return hash_id;
}

// User has picked "remove from avatar" from a menu.
// static
void LLAgentWearables::userRemoveWearable(const LLWearableType::EType& type,
										  const U32& index)
{
	if (type != LLWearableType::WT_SHAPE && type != LLWearableType::WT_SKIN &&
		type != LLWearableType::WT_HAIR && type != LLWearableType::WT_EYES)
	{
		gAgentWearables.removeWearable(type, false, index);
	}
}

//static 
void LLAgentWearables::userRemoveWearablesOfType(const LLWearableType::EType &type)
{
	if (type != LLWearableType::WT_SHAPE && type != LLWearableType::WT_SKIN &&
		type != LLWearableType::WT_HAIR && type != LLWearableType::WT_EYES)
	{
		gAgentWearables.removeWearable(type, true, 0);
	}
}

void LLAgentWearables::userRemoveAllClothes()
{
	// We have to do this up front to avoid having to deal with the case of
	// multiple wearables being dirty.
	if (gFloaterCustomize)
	{
		gFloaterCustomize->askToSaveIfDirty(LLAgentWearables::userRemoveAllClothesStep2,
											NULL);
	}
	else
	{
		userRemoveAllClothesStep2(TRUE, NULL);
	}
}

void LLAgentWearables::userRemoveAllClothesStep2(BOOL proceed, void* userdata)
{
	if (proceed)
	{
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_SHIRT);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_PANTS);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_SHOES);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_SOCKS);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_JACKET);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_GLOVES);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_UNDERSHIRT);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_UNDERPANTS);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_SKIRT);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_ALPHA);
		gAgentWearables.userRemoveWearablesOfType(LLWearableType::WT_TATTOO);
	}
}

#if 0	// Currently unused in the Cool VL Viewer
// Combines userRemoveAllAttachments() and userAttachMultipleAttachments() logic to
// get attachments into desired state with minimal number of adds/removes.
void LLAgentWearables::userUpdateAttachments(LLInventoryModel::item_array_t& obj_item_array)
{
	// Possible cases:
	// already wearing but not in request set -> take off.
	// already wearing and in request set -> leave alone.
	// not wearing and in request set -> put on.

	if (!isAgentAvatarValid()) return;

	std::set<LLUUID> requested_item_ids;
	std::set<LLUUID> current_item_ids;
	for (S32 i = 0; i < obj_item_array.count(); i++)
	{
		requested_item_ids.insert(obj_item_array[i].get()->getLinkedUUID());
	}

	// Build up list of objects to be removed and items currently attached.
	llvo_vec_t objects_to_remove;
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *objectp = (*attachment_iter);
			if (objectp)
			{
				LLUUID object_item_id = objectp->getAttachmentItemID();
				if (requested_item_ids.find(object_item_id) != requested_item_ids.end())
				{
					// Object currently worn, was requested.
					// Flag as currently worn so we won't have to add it again.
					current_item_ids.insert(object_item_id);
				}
				else
				{
					// object currently worn, not requested.
					objects_to_remove.push_back(objectp);
				}
			}
		}
	}

	LLInventoryModel::item_array_t items_to_add;
	for (LLInventoryModel::item_array_t::iterator it = obj_item_array.begin();
		 it != obj_item_array.end(); ++it)
	{
		LLUUID linked_id = (*it).get()->getLinkedUUID();
		if (current_item_ids.find(linked_id) == current_item_ids.end())
		{
			// Requested attachment is not worn yet.
			items_to_add.push_back(*it);
		}
	}
	//S32 remove_count = objects_to_remove.size();
	//S32 add_count = items_to_add.size();
	//llinfos << "remove " << remove_count << " add " << add_count << llendl;

	// Remove everything in objects_to_remove
	userRemoveMultipleAttachments(objects_to_remove);

	// Add everything in items_to_add
	userAttachMultipleAttachments(items_to_add);
}
#endif

void LLAgentWearables::userRemoveMultipleAttachments(llvo_vec_t& objects_to_remove)
{
	if (!isAgentAvatarValid()) return;

	if (objects_to_remove.empty())
		return;

	gMessageSystem->newMessage("ObjectDetach");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	for (llvo_vec_t::iterator it = objects_to_remove.begin();
		 it != objects_to_remove.end(); ++it)
	{
		LLViewerObject* objectp = *it;
//MK
		if (gRRenabled && !gAgent.mRRInterface.canDetach(objectp))
		{
			continue;
		}
//mk
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, objectp->getLocalID());
	}
	gMessageSystem->sendReliable(gAgent.getRegionHost());
}

void LLAgentWearables::userRemoveAllAttachments()
{
	if (!isAgentAvatarValid()) return;

	llvo_vec_t objects_to_remove;

	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *attached_object = (*attachment_iter);
			if (attached_object)
			{
				objects_to_remove.push_back(attached_object);
			}
		}
	}
	userRemoveMultipleAttachments(objects_to_remove);
}

void LLAgentWearables::userAttachMultipleAttachments(LLInventoryModel::item_array_t& obj_item_array)
{
	// Build a compound message to send all the objects that need to be rezzed.
	S32 obj_count = obj_item_array.count();

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

	for (S32 i = 0; i < obj_count; ++i)
	{
		if (i % OBJECTS_PER_PACKET == 0)
		{
			// Start a new message chunk
			msg->newMessageFast(_PREHASH_RezMultipleAttachmentsFromInv);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_HeaderData);
			msg->addUUIDFast(_PREHASH_CompoundMsgID, compound_msg_id );
			msg->addU8Fast(_PREHASH_TotalObjects, obj_count );
			msg->addBOOLFast(_PREHASH_FirstDetachAll, false );
		}

		const LLInventoryItem* item = obj_item_array.get(i).get();
		msg->nextBlockFast(_PREHASH_ObjectData );
		msg->addUUIDFast(_PREHASH_ItemID, item->getLinkedUUID());
		msg->addUUIDFast(_PREHASH_OwnerID, item->getPermissions().getOwner());
		msg->addU8Fast(_PREHASH_AttachmentPt, 0 | ATTACHMENT_ADD);	// Wear at the previous or default attachment point
		pack_permissions_slam(msg, item->getFlags(), item->getPermissions());
		msg->addStringFast(_PREHASH_Name, item->getName());
		msg->addStringFast(_PREHASH_Description, item->getDescription());

		if (obj_count == i + 1 || i % OBJECTS_PER_PACKET == OBJECTS_PER_PACKET - 1)
		{
			// End of message chunk
			msg->sendReliable(gAgent.getRegion()->getHost());
		}
	}
}

// Returns false if the given wearable is already topmost/bottommost
// (depending on closer_to_body parameter).
bool LLAgentWearables::canMoveWearable(const LLUUID& item_id, bool closer_to_body)
{
	const LLWearable* wearable = getWearableFromItemID(item_id);
	if (!wearable) return false;

	LLWearableType::EType wtype = wearable->getType();
	const LLWearable* marginal_wearable = closer_to_body ? getBottomWearable(wtype)
														 : getTopWearable(wtype);
	if (!marginal_wearable) return false;

	return wearable != marginal_wearable;
}

BOOL LLAgentWearables::areWearablesLoaded() const
{
	return mWearablesLoaded;
}

// MULTI-WEARABLE: DEPRECATED: item pending count relies on old messages that
// don't support multi-wearables. do not trust to be accurate
void LLAgentWearables::updateWearablesLoaded()
{
	mWearablesLoaded = TRUE;
	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		if (getWearableItemID((LLWearableType::EType)i, 0).notNull() &&
			!getWearable((LLWearableType::EType)i, 0))
		{
			mWearablesLoaded = FALSE;
			break;
		}
	}
	LL_DEBUGS("Wearables") << "mWearablesLoaded = " << mWearablesLoaded
						   << LL_ENDL;
}

bool LLAgentWearables::canWearableBeRemoved(const LLWearable* wearable) const
{
	if (!wearable) return false;

	LLWearableType::EType type = wearable->getType();
//MK
	if (gRRenabled && !gAgent.mRRInterface.canUnwear(type))
	{
		return false;
	}
//mk
	// Make sure the user always has at least one shape, skin, eyes, and hair
	// type currently worn.
	return getWearableCount(type) > 1 ||
		   (type != LLWearableType::WT_SHAPE &&
			type != LLWearableType::WT_SKIN &&
			type != LLWearableType::WT_HAIR &&
			type != LLWearableType::WT_EYES);		  
}

void LLAgentWearables::animateAllWearableParams(F32 delta, BOOL upload_bake)
{
	for (S32 type = 0; type < LLWearableType::WT_COUNT; ++type)
	{
		for (S32 count = 0;
			 count < (S32)getWearableCount((LLWearableType::EType)type);
			 ++count)
		{
			LLWearable* wearable = getWearable((LLWearableType::EType)type,
											   count);
			llassert(wearable);
			if (wearable)
			{
				wearable->animateParams(delta, upload_bake);
			}
		}
	}
}

// static
void LLAgentWearables::createWearable(LLWearableType::EType type,
									  const LLUUID& parent_id,
									  bool wear)
{
	if (type == LLWearableType::WT_INVALID || type == LLWearableType::WT_NONE) return;

	LLWearable* wearable = LLWearableList::instance().createNewWearable(type);
	LLAssetType::EType asset_type = wearable->getAssetType();
	LLInventoryType::EType inv_type = LLInventoryType::IT_WEARABLE;
#if 0	// Not implemented/used in the Cool VL Viewer
	LLPointer<LLInventoryCallback> cb = wear ? new LLWearAndEditCallback : NULL;
#else
	LLPointer<LLInventoryCallback> cb = NULL;
#endif

	LLUUID folder_id;
	if (parent_id.notNull())
	{
		folder_id = parent_id;
	}
	else
	{
		LLFolderType::EType folder_type = LLFolderType::assetTypeToFolderType(asset_type);
		folder_id = gInventory.findCategoryUUIDForType(folder_type);
	}

	create_inventory_item(gAgentID, gAgent.getSessionID(), folder_id,
						  wearable->getTransactionID(), wearable->getName(),
						  wearable->getDescription(), asset_type, inv_type,
						  wearable->getType(),
						  wearable->getPermissions().getMaskNextOwner(), cb);
}

void LLAgentWearables::updateServer()
{
	sendAgentWearablesUpdate();
	gAgent.sendAgentSetAppearance();
	gInventory.notifyObservers();

	// Ensure the new outfit will be saved
	gWearablesListDirty = true;

	// Notify the Make New Outfit floater, if opened
	LLFloaterMakeNewOutfit::setDirty();
}

// EOF
