/**
 * @file llinventorybridge.cpp
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
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

#include <utility> // for std::pair<>

#include "llinventorybridge.h"

#include "llcallingcard.h"
#include "llfocusmgr.h"
#include "llevent.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llnotifications.h"
#include "llradiogroup.h"
#include "llresmgr.h"
#include "llscrollcontainer.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "message.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llfloateravatarinfo.h"
#include "llfloaterchat.h"
#include "llfloatercustomize.h"
#include "llfloateropenobject.h"
#include "llfloaterproperties.h"
#include "llfloaterworldmap.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "llinventoryclipboard.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryview.h"
#include "llmarketplacefunctions.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewlandmark.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llimview.h"
#include "llselectmgr.h"
#include "lltooldraganddrop.h"
#include "llviewerassettype.h"
#include "llviewercontrol.h"
#include "llviewerfoldertype.h"
#include "llviewerinventory.h"
#include "llviewermessage.h"
#include "llviewernetwork.h"				// for gIsInSecondLife
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearable.h"
#include "llwearablelist.h"

using namespace LLOldEvents;

// Helpers
// bug in busy count inc/dec right now, logic is complex... do we really need it?
void inc_busy_count()
{
// 	gViewerWindow->getWindow()->incBusyCount();
//  check balance of these calls if this code is changed to ever actually
//  *do* something!
}
void dec_busy_count()
{
// 	gViewerWindow->getWindow()->decBusyCount();
//  check balance of these calls if this code is changed to ever actually
//  *do* something!
}

// Function declarations
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv*);
#if RESTORE_TO_WORLD
bool restore_to_world_callback(const LLSD& notification, const LLSD& response, LLItemBridge* self);
#endif

// +=================================================+
// |        LLInvFVBridge                            |
// +=================================================+

const std::string& LLInvFVBridge::getName() const
{
	LLInventoryObject* obj = getInventoryObject();
	if (obj)
	{
		return obj->getName();
	}
	return LLStringUtil::null;
}

const std::string& LLInvFVBridge::getDisplayName() const
{
	return getName();
}

// Folders have full perms
PermissionMask LLInvFVBridge::getPermissionMask() const
{

	return PERM_ALL;
}

// virtual
LLFolderType::EType LLInvFVBridge::getPreferredType() const
{
	return LLFolderType::FT_NONE;
}

// Folders don't have creation dates.
time_t LLInvFVBridge::getCreationDate() const
{
	return 0;
}

// Can be destroyed (or moved to trash)
BOOL LLInvFVBridge::isItemRemovable()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model)
	{
		return FALSE;
	}
	const LLInventoryObject *obj = model->getItem(mUUID);
	if (obj && obj->getIsLinkType())
	{
		return TRUE;
	}
	if (model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID()))
	{
		return TRUE;
	}
	return FALSE;
}

// Can be moved to another folder
BOOL LLInvFVBridge::isItemMovable()
{
	BOOL can_move = FALSE;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (model)
	{
		can_move = model->isObjectDescendentOf(mUUID,
											   gInventory.getRootFolderID());
	}
	return can_move;
}

// *TODO: make sure this does the right thing
void LLInvFVBridge::showProperties()
{
	LLShowProps::showProperties(mUUID);
}

void LLInvFVBridge::removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch)
{
	// Deactivate gestures when moving them into Trash
	LLInvFVBridge* bridge;
	LLInventoryModel* model = mInventoryPanel->getModel();
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	S32 count = batch.count();
	S32 i,j;
	for (i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if (!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if (item)
		{
			if (LLAssetType::AT_GESTURE == item->getType())
			{
				gGestureManager.deactivateGesture(item->getUUID());
			}
		}
	}
	for (i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if (!bridge || !bridge->isItemRemovable()) continue;
		cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if (cat)
		{
			gInventory.collectDescendents(cat->getUUID(), descendent_categories, descendent_items, FALSE);
			for (j=0; j<descendent_items.count(); j++)
			{
				if (LLAssetType::AT_GESTURE == descendent_items[j]->getType())
				{
					gGestureManager.deactivateGesture(descendent_items[j]->getUUID());
				}
			}
		}
	}
	removeBatchNoCheck(batch);
}

void LLInvFVBridge::removeBatchNoCheck(LLDynamicArray<LLFolderViewEventListener*>& batch)
{
	// this method moves a bunch of items and folders to the trash. As
	// per design guidelines for the inventory model, the message is
	// built and the accounting is performed first. After all of that,
	// we call LLInventoryModel::moveObject() to move everything
	// around.
	LLInvFVBridge* bridge;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return;
	LLMessageSystem* msg = gMessageSystem;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	std::vector<LLUUID> move_ids;
	LLInventoryModel::update_map_t update;
	bool start_new_message = true;
	S32 count = batch.count();
	S32 i;
	for (i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if (!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if (item)
		{
			if (item->getParentUUID() == trash_id) continue;
			move_ids.push_back(item->getUUID());
			LLPreview::hide(item->getUUID());
			--update[item->getParentUUID()];
			++update[trash_id];
			if (start_new_message)
			{
				start_new_message = false;
				msg->newMessageFast(_PREHASH_MoveInventoryItem);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOLFast(_PREHASH_Stamp, TRUE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_ItemID, item->getUUID());
			msg->addUUIDFast(_PREHASH_FolderID, trash_id);
			msg->addString("NewName", NULL);
			if (msg->isSendFullFast(_PREHASH_InventoryData))
			{
				start_new_message = true;
				gAgent.sendReliableMessage();
				gInventory.accountForUpdate(update);
				update.clear();
			}
		}
	}
	if (!start_new_message)
	{
		start_new_message = true;
		gAgent.sendReliableMessage();
		gInventory.accountForUpdate(update);
		update.clear();
	}
	for (i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if (!bridge || !bridge->isItemRemovable()) continue;
		cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if (cat)
		{
			if (cat->getParentUUID() == trash_id) continue;
			move_ids.push_back(cat->getUUID());
			--update[cat->getParentUUID()];
			++update[trash_id];
			if (start_new_message)
			{
				start_new_message = false;
				msg->newMessageFast(_PREHASH_MoveInventoryFolder);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOL("Stamp", TRUE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_FolderID, cat->getUUID());
			msg->addUUIDFast(_PREHASH_ParentID, trash_id);
			if (msg->isSendFullFast(_PREHASH_InventoryData))
			{
				start_new_message = true;
				gAgent.sendReliableMessage();
				gInventory.accountForUpdate(update);
				update.clear();
			}
		}
	}
	if (!start_new_message)
	{
		gAgent.sendReliableMessage();
		gInventory.accountForUpdate(update);
	}

	// move everything.
	std::vector<LLUUID>::iterator it = move_ids.begin();
	std::vector<LLUUID>::iterator end = move_ids.end();
	for ( ; it != end; ++it)
	{
		gInventory.moveObject((*it), trash_id);
	}

	// notify inventory observers.
	model->notifyObservers();
}

BOOL LLInvFVBridge::isClipboardPasteable() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model)
	{
		return FALSE;
	}

	BOOL is_agent_inventory = model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID());
	if (!LLInventoryClipboard::instance().hasContents() || !is_agent_inventory)
	{
		return FALSE;
	}

	const LLUUID &agent_id = gAgentID;
	LLDynamicArray<LLUUID> objects;
	LLInventoryClipboard::instance().retrieve(objects);
	S32 count = objects.count();
	for (S32 i = 0; i < count; i++)
	{
		const LLUUID &item_id = objects.get(i);

		// Can't paste folders
		const LLInventoryCategory *cat = model->getCategory(item_id);
		if (cat)
		{
			return FALSE;
		}

		const LLInventoryItem *item = model->getItem(item_id);
		if (item)
		{
			if (!item->getPermissions().allowCopyBy(agent_id))
			{
				return FALSE;
			}
		}
	}
//MK
	if (gRRenabled)
	{
		// Don't allow if either the destination folder or the source folder is locked
		LLViewerInventoryCategory *current_cat = (LLViewerInventoryCategory*)model->getCategory(mUUID);
		if (current_cat)
		{
			for (S32 i = objects.count() - 1; i >= 0; --i)
			{
				const LLUUID &obj_id = objects.get(i);
				if (gAgent.mRRInterface.isFolderLocked(current_cat) ||
					gAgent.mRRInterface.isFolderLocked(gInventory.getCategory(model->getObject(obj_id)->getParentUUID())))
				{
					return FALSE;
				}
			}
			LLInventoryClipboard::instance().retrieveCuts(objects);
			count = objects.count();
			for (S32 i = objects.count() - 1; i >= 0; --i)
			{
				const LLUUID &obj_id = objects.get(i);
				if (gAgent.mRRInterface.isFolderLocked(current_cat) ||
					gAgent.mRRInterface.isFolderLocked(gInventory.getCategory(model->getObject(obj_id)->getParentUUID())))
				{
					return FALSE;
				}
			}
		}
	}
//mk
	return TRUE;
}

BOOL LLInvFVBridge::isClipboardPasteableAsLink() const
{
	if (!gIsInSecondLife && !gSavedSettings.getBOOL("OSAllowInventoryLinks"))
	{
		return FALSE;
	}
	if (!LLInventoryClipboard::instance().hasCopiedContents() ||
		!isAgentInventory())
	{
		return FALSE;
	}
	const LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model)
	{
		return FALSE;
	}

	LLDynamicArray<LLUUID> objects;
	LLInventoryClipboard::instance().retrieve(objects);
	S32 count = objects.count();
	for (S32 i = 0; i < count; i++)
	{
		LLUUID object_id = objects.get(i);

		const LLInventoryItem* item = model->getItem(object_id);
		if (item && !LLAssetType::lookupCanLink(item->getActualType()))
		{
			return FALSE;
		}

		const LLViewerInventoryCategory* cat = model->getCategory(object_id);
		if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
		{
			return FALSE;
		}
	}
//MK
	if (gRRenabled)
	{
		// Don't allow if either the destination folder or the source folder is locked
		LLViewerInventoryCategory* current_cat = (LLViewerInventoryCategory*)model->getCategory(mUUID);
		if (current_cat)
		{
			for (S32 i = objects.count() - 1; i >= 0; --i)
			{
				const LLUUID &obj_id = objects.get(i);
				if (gAgent.mRRInterface.isFolderLocked(current_cat) ||
					gAgent.mRRInterface.isFolderLocked(gInventory.getCategory(model->getObject(obj_id)->getParentUUID())))
				{
					return FALSE;
				}
			}
		}
	}
//mk
	return TRUE;
}

void hideContextEntries(LLMenuGL& menu,
						const std::vector<std::string> &entries_to_show,
						const std::vector<std::string> &disabled_entries)
{
	const LLView::child_list_t *list = menu.getChildList();

	LLView::child_list_t::const_iterator itor;
	for (itor = list->begin(); itor != list->end(); ++itor)
	{
		std::string name = (*itor)->getName();

		// descend into split menus:
		LLMenuItemBranchGL* branchp = dynamic_cast<LLMenuItemBranchGL*>(*itor);
		if ((name == "More") && branchp)
		{
			hideContextEntries(*branchp->getBranch(), entries_to_show,
							   disabled_entries);
		}

		bool found = false;
		std::vector<std::string>::const_iterator itor2;
		for (itor2 = entries_to_show.begin(); itor2 != entries_to_show.end();
			 ++itor2)
		{
			if (*itor2 == name)
			{
				found = true;
			}
		}
		if (!found)
		{
			(*itor)->setVisible(FALSE);
		}
		else
		{
			for (itor2 = disabled_entries.begin();
				 itor2 != disabled_entries.end(); ++itor2)
			{
				if (*itor2 == name)
				{
					(*itor)->setEnabled(FALSE);
				}
			}
		}
	}
}

// Helper for commonly-used entries
void LLInvFVBridge::getClipboardEntries(bool show_asset_id,
										std::vector<std::string> &items,
										std::vector<std::string> &disabled_items,
										U32 flags)
{
	const LLInventoryObject *obj = getInventoryObject();

	if (obj)
	{
		if (obj->getIsLinkType())
		{
			items.push_back(std::string("Find Original"));
			if (isLinkedObjectMissing())
			{
				disabled_items.push_back(std::string("Find Original"));
			}

			items.push_back(std::string("Copy Separator"));
		}
		else
		{
			items.push_back(std::string("Rename"));
			if (!isItemRenameable() || (flags & FIRST_SELECTED_ITEM) == 0)
			{
				disabled_items.push_back(std::string("Rename"));
			}

			if (show_asset_id)
			{
				items.push_back(std::string("Copy Asset UUID"));
				if ((!(isItemPermissive() || gAgent.isGodlike()))
					|| (flags & FIRST_SELECTED_ITEM) == 0)
				{
					disabled_items.push_back(std::string("Copy Asset UUID"));
				}
			}

			items.push_back(std::string("Copy Separator"));

			items.push_back(std::string("Copy"));
			if (!isItemCopyable())
			{
				disabled_items.push_back(std::string("Copy"));
			}
		}
	}

	items.push_back(std::string("Cut"));
	if (!isItemMovable())
	{
		disabled_items.push_back(std::string("Cut"));
	}

	items.push_back(std::string("Paste"));
	if (!isClipboardPasteable() || (flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Paste"));
	}

	items.push_back(std::string("Paste As Link"));
	if (!isClipboardPasteableAsLink() || (flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Paste As Link"));
	}

	items.push_back(std::string("Paste Separator"));

	items.push_back(std::string("Delete"));
	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Delete"));
	}
}

void LLInvFVBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	//lldebugs << "LLInvFVBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if (isInTrash())
	{
		const LLInventoryObject *obj = getInventoryObject();
		if (obj && obj->getIsLinkType())
		{
			items.push_back(std::string("Find Original"));
			if (isLinkedObjectMissing())
			{
				disabled_items.push_back(std::string("Find Original"));
			}
		}
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}
		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}
	hideContextEntries(menu, items, disabled_items);
}

// *TODO: remove this
BOOL LLInvFVBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	BOOL rv = FALSE;

	const LLInventoryObject* obj = getInventoryObject();

	if (obj)
	{
		*type = LLViewerAssetType::lookupDragAndDropType(obj->getActualType());
		if (*type == DAD_NONE)
		{
			return FALSE;
		}

		*id = obj->getUUID();
		//object_ids.put(obj->getUUID());

		if (*type == DAD_CATEGORY)
		{
			LLInventoryModelBackgroundFetch::instance().start(obj->getUUID());
		}

		rv = TRUE;
	}

	return rv;
}

LLInventoryObject* LLInvFVBridge::getInventoryObject() const
{
	LLInventoryObject* obj = NULL;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (model)
	{
		obj = (LLInventoryObject*)model->getObject(mUUID);
	}
	return obj;
}

BOOL LLInvFVBridge::isInTrash() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	return model->isObjectDescendentOf(mUUID, trash_id);
}

BOOL LLInvFVBridge::isLinkedObjectInTrash() const
{
	if (isInTrash()) return TRUE;

	const LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		LLInventoryModel* model = mInventoryPanel->getModel();
		if (!model) return FALSE;
		const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
		return model->isObjectDescendentOf(obj->getLinkedUUID(), trash_id);
	}
	return FALSE;
}

BOOL LLInvFVBridge::isLinkedObjectMissing() const
{
	const LLInventoryObject *obj = getInventoryObject();
	if (!obj)
	{
		return TRUE;
	}
	if (obj->getIsLinkType() && LLAssetType::lookupIsLinkType(obj->getType()))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL LLInvFVBridge::isAgentInventory() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;
	if (gInventory.getRootFolderID() == mUUID) return TRUE;
	return model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID());
}

BOOL LLInvFVBridge::isItemPermissive() const
{
	return FALSE;
}

// static
void LLInvFVBridge::changeItemParent(LLInventoryModel* model,
									 LLViewerInventoryItem* item,
									 const LLUUID& new_parent,
									 BOOL restamp)
{
	if (!model || !item)
	{
		return;
	}
	if (item->getParentUUID() != new_parent)
	{
//MK
		if (gRRenabled)
		{
			LLInventoryCategory* cat_parent = gInventory.getCategory(item->getParentUUID());
			LLInventoryCategory* cat_new_parent = gInventory.getCategory(new_parent);
			// We can move this category if we are moving it from a non shared
			// folder to another one, even if both folders are locked
			if ((gAgent.mRRInterface.isUnderRlvShare(cat_parent) ||
				 gAgent.mRRInterface.isUnderRlvShare(cat_new_parent)) &&
				(gAgent.mRRInterface.isFolderLocked(cat_parent) ||
				 gAgent.mRRInterface.isFolderLocked(cat_new_parent)))
			{
				return;
			}
		}
//mk
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(item->getParentUUID(), -1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setParent(new_parent);
		new_item->updateParentOnServer(restamp);
		model->updateItem(new_item);
		model->notifyObservers();
	}
}

// static
void LLInvFVBridge::changeCategoryParent(LLInventoryModel* model,
										 LLViewerInventoryCategory* cat,
										 const LLUUID& new_parent,
										 BOOL restamp)
{
	if (!model || !cat)
	{
		return;
	}
//MK
	if (gRRenabled)
	{
		LLInventoryCategory* cat_new_parent = gInventory.getCategory(new_parent);
		// We can move this category if we are moving it from a non shared
		// folder to another one, even if both folders are locked
		if ((gAgent.mRRInterface.isUnderRlvShare(cat) ||
			 gAgent.mRRInterface.isUnderRlvShare(cat_new_parent)) &&
			(gAgent.mRRInterface.isFolderLocked(cat) ||
			 gAgent.mRRInterface.isFolderLocked(cat_new_parent)))
		{
			return;
		}
	}
//mk
	if (cat->getParentUUID() != new_parent)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(cat->getParentUUID(), -1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
		new_cat->setParent(new_parent);
		new_cat->updateParentOnServer(restamp);
		model->updateCategory(new_cat);
		model->notifyObservers();
	}
}

std::string safe_inv_type_lookup(LLInventoryType::EType inv_type)
{
	std::string rv = LLInventoryType::lookupHumanReadable(inv_type);
	if (rv.empty())
	{
		rv = "<invalid>";
	}
	return rv;
}

LLInvFVBridge* LLInvFVBridge::createBridge(LLAssetType::EType asset_type,
										   LLAssetType::EType actual_asset_type,
										   LLInventoryType::EType inv_type,
										   LLInventoryPanel* inventory,
										   const LLUUID& uuid,
										   U32 flags)
{
	static LLUUID last_uuid;
	bool warn = false;
	LLInvFVBridge* new_listener = NULL;
	switch (asset_type)
	{
	case LLAssetType::AT_TEXTURE:
		if (inv_type != LLInventoryType::IT_TEXTURE &&
			inv_type != LLInventoryType::IT_SNAPSHOT)
		{
			warn = true;
		}
		new_listener = new LLTextureBridge(inventory, uuid, inv_type);
		break;

	case LLAssetType::AT_SOUND:
		if (inv_type != LLInventoryType::IT_SOUND)
		{
			warn = true;
		}
		new_listener = new LLSoundBridge(inventory, uuid);
		break;

	case LLAssetType::AT_LANDMARK:
		if (inv_type != LLInventoryType::IT_LANDMARK)
		{
			warn = true;
		}
		new_listener = new LLLandmarkBridge(inventory, uuid, flags);
		break;

	case LLAssetType::AT_CALLINGCARD:
		if (inv_type != LLInventoryType::IT_CALLINGCARD)
		{
			warn = true;
		}
		new_listener = new LLCallingCardBridge(inventory, uuid);
		break;

	case LLAssetType::AT_SCRIPT:
		if (inv_type != LLInventoryType::IT_LSL)
		{
			warn = true;
		}
		new_listener = new LLScriptBridge(inventory, uuid);
		break;

	case LLAssetType::AT_OBJECT:
		if (inv_type != LLInventoryType::IT_OBJECT &&
			inv_type != LLInventoryType::IT_ATTACHMENT)
		{
			warn = true;
		}
		new_listener = new LLObjectBridge(inventory, uuid, inv_type, flags);
		break;

	case LLAssetType::AT_NOTECARD:
		if (inv_type != LLInventoryType::IT_NOTECARD)
		{
			warn = true;
		}
		new_listener = new LLNotecardBridge(inventory, uuid);
		break;

	case LLAssetType::AT_ANIMATION:
		if (inv_type != LLInventoryType::IT_ANIMATION)
		{
			warn = true;
		}
		new_listener = new LLAnimationBridge(inventory, uuid);
		break;

	case LLAssetType::AT_GESTURE:
		if (inv_type != LLInventoryType::IT_GESTURE)
		{
			warn = true;
		}
		new_listener = new LLGestureBridge(inventory, uuid);
		break;

	case LLAssetType::AT_LSL_TEXT:
		if (inv_type != LLInventoryType::IT_LSL)
		{
			warn = true;
		}
		new_listener = new LLLSLTextBridge(inventory, uuid);
		break;

	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
		if (inv_type != LLInventoryType::IT_WEARABLE)
		{
			warn = true;
		}
		new_listener = new LLWearableBridge(inventory, uuid, asset_type,
											inv_type, (LLWearableType::EType)flags);
		break;

	case LLAssetType::AT_CATEGORY:
		if (actual_asset_type == LLAssetType::AT_LINK_FOLDER)
		{
			// Create a link folder handler instead.
			new_listener = new LLLinkFolderBridge(inventory, uuid);
			break;
		}
		new_listener = new LLFolderBridge(inventory, uuid);
		break;

	case LLAssetType::AT_LINK:
	case LLAssetType::AT_LINK_FOLDER:
		// Only should happen for broken links.
		new_listener = new LLLinkItemBridge(inventory, uuid);
		break;

	case LLAssetType::AT_MESH:
		if (inv_type != LLInventoryType::IT_MESH)
		{
			warn = true;
		}
		new_listener = new LLMeshBridge(inventory, uuid);
		break;

	default:
		llinfos << "Unhandled asset type (llassetstorage.h): "
				<< (S32)asset_type << llendl;
		break;
	}

	if (warn && uuid != last_uuid)
	{
		last_uuid = uuid;
		llwarns << LLAssetType::lookup(asset_type)
				<< " asset has inventory type "
				<< safe_inv_type_lookup(inv_type)
				<< " on uuid " << uuid << llendl;
	}

	if (new_listener)
	{
		new_listener->mInvType = inv_type;
	}

	return new_listener;
}

// +=================================================+
// |        LLItemBridge                             |
// +=================================================+

void LLItemBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("goto" == action)
	{
		gotoItem(folder);
	}
	else if ("open" == action)
	{
		openItem();
	}
	else if ("properties" == action)
	{
		showProperties();
	}
	else if ("purge" == action)
	{
		LLInventoryCategory* cat = model->getCategory(mUUID);
		if (cat)
		{
			model->purgeDescendentsOf(mUUID);
		}
		LLInventoryObject* obj = model->getObject(mUUID);
		if (!obj) return;
		obj->removeFromServer();
		LLPreview::hide(mUUID);
		model->deleteObject(mUUID);
		model->notifyObservers();
	}
#if RESTORE_TO_WORLD
	else if ("restoreToWorld" == action)
	{
		LLNotifications::instance().add("ObjectRestoreToWorld", LLSD(), LLSD(),
										boost::bind(restore_to_world_callback,
													_1, _2, this));
	}
#endif
	else if ("restore" == action)
	{
		restoreItem();
	}
	else if ("copy_uuid" == action)
	{
		// Single item only
		LLInventoryItem* item = model->getItem(mUUID);
		if (!item) return;
		LLUUID asset_id = item->getAssetUUID();
		std::string buffer;
		asset_id.toString(buffer);

		gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(buffer));
		return;
	}
	else if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	else if ("cut" == action)
	{
		cutToClipboard();
		return;
	}
	else if ("paste" == action)
	{
		// Single item only
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		LLFolderViewItem* folder_view_itemp = folder->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getListener()->pasteFromClipboard();
		return;
	}
	else if ("paste_link" == action)
	{
		// Single item only
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		LLFolderViewItem* folder_view_itemp = folder->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getListener()->pasteLinkFromClipboard();
		return;
	}
}

void LLItemBridge::selectItem()
{
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)getItem();
	if (item && !item->isFinished())
	{
		item->fetchFromServer();
	}
}

void LLItemBridge::restoreItem()
{
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)getItem();
	if (item)
	{
		LLInventoryModel* model = mInventoryPanel->getModel();
		const LLUUID new_parent = model->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(item->getType()));
		// do not restamp on restore.
		LLInvFVBridge::changeItemParent(model, item, new_parent, FALSE);
	}
}

#if RESTORE_TO_WORLD
bool restore_to_world_callback(const LLSD& notification,
							   const LLSD& response,
							   LLItemBridge* self)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (self && option == 0)
	{
		self->restoreToWorld();
	}
	return false;
}

void LLItemBridge::restoreToWorld()
{
	LLViewerInventoryItem* itemp = (LLViewerInventoryItem*)getItem();
	if (itemp)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("RezRestoreToWorld");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

		msg->nextBlockFast(_PREHASH_InventoryData);
		itemp->packMessage(msg);
		msg->sendReliable(gAgent.getRegion()->getHost());
	}

	//Similar functionality to the drag and drop rez logic
	BOOL remove_from_inventory = FALSE;

	// Remove local inventory copy, sim will deal with permissions and removing
	// the item from the actual inventory if its a no-copy etc
	if (!itemp->getPermissions().allowCopyBy(gAgentID))
	{
		remove_from_inventory = TRUE;
	}

	// Check if it's in the trash. (again similar to the normal rez logic)
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (gInventory.isObjectDescendentOf(itemp->getUUID(), trash_id))
	{
		remove_from_inventory = TRUE;
	}

	if (remove_from_inventory)
	{
		gInventory.deleteObject(itemp->getUUID());
		gInventory.notifyObservers();
	}
}
#endif

void LLItemBridge::gotoItem(LLFolderView *folder)
{
	LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		LLInventoryView *view = LLInventoryView::getActiveInventory();
		if (view)
		{
			view->getPanel()->setSelection(obj->getLinkedUUID(), TAKE_FOCUS_NO);
		}
	}
}

LLUIImagePtr LLItemBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLInventoryIcon::ICONNAME_OBJECT);
}

PermissionMask LLItemBridge::getPermissionMask() const
{
	LLViewerInventoryItem* item = getItem();
	PermissionMask perm_mask = 0;
	if (item)
	{
		BOOL copy = item->getPermissions().allowCopyBy(gAgentID);
		BOOL mod = item->getPermissions().allowModifyBy(gAgentID);
		BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
															gAgentID);

		if (copy) perm_mask |= PERM_COPY;
		if (mod)  perm_mask |= PERM_MODIFY;
		if (xfer) perm_mask |= PERM_TRANSFER;

	}
	return perm_mask;
}

const std::string& LLItemBridge::getDisplayName() const
{
	if (mDisplayName.empty())
	{
		buildDisplayName(getItem(), mDisplayName);
	}
	return mDisplayName;
}

void LLItemBridge::buildDisplayName(LLInventoryItem* item, std::string& name)
{
	if (item)
	{
		name.assign(item->getName());
	}
	else
	{
		name.assign(LLStringUtil::null);
	}
}

std::string LLItemBridge::getLabelSuffix() const
{
	std::string suffix;
	LLInventoryItem* item = getItem();
	if (item)
	{
		// it's a bit confusing to put nocopy/nomod/etc on calling cards.
		if (LLAssetType::AT_CALLINGCARD != item->getType()
		   && item->getPermissions().getOwner() == gAgentID)
		{
			BOOL copy = item->getPermissions().allowCopyBy(gAgentID);
			BOOL mod = item->getPermissions().allowModifyBy(gAgentID);
			BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
																gAgentID);
			// *TODO: Translate
			static std::string LINK = " (link)";
			static std::string BROKEN_LINK = " (broken link)";
			if (LLAssetType::lookupIsLinkType(item->getType())) return BROKEN_LINK;
			if (item->getIsLinkType()) return LINK;

			const char* EMPTY = "";
			const char* NO_COPY = " (no copy)";
			const char* NO_MOD = " (no modify)";
			const char* NO_XFER = " (no transfer)";
			const char* scopy;
			if (copy) scopy = EMPTY;
			else scopy = NO_COPY;
			const char* smod;
			if (mod) smod = EMPTY;
			else smod = NO_MOD;
			const char* sxfer;
			if (xfer) sxfer = EMPTY;
			else sxfer = NO_XFER;
			suffix = llformat("%s%s%s",scopy,smod,sxfer);
		}
	}
	return suffix;
}

time_t LLItemBridge::getCreationDate() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		return item->getCreationDate();
	}
	return 0;
}

BOOL LLItemBridge::isItemRenameable() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		return (item->getPermissions().allowModifyBy(gAgentID));
	}
	return FALSE;
}

BOOL LLItemBridge::renameItem(const std::string& new_name)
{
	if (!isItemRenameable()) return FALSE;
	LLPreview::rename(mUUID, getPrefix() + new_name);
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;
	LLViewerInventoryItem* item = getItem();
	if (item && (item->getName() != new_name))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(new_name);
		buildDisplayName(new_item, mDisplayName);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);
		model->notifyObservers();
	}
	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}

BOOL LLItemBridge::removeItem()
{
	if (!isItemRemovable())
	{
		return FALSE;
	}
	// move it to the trash
	LLPreview::hide(mUUID, TRUE);
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = getItem();

	// if item is not already in trash
	if (item && !model->isObjectDescendentOf(mUUID, trash_id))
	{
		// move to trash, and restamp
		LLInvFVBridge::changeItemParent(model, item, trash_id, TRUE);
		// delete was successful
		return TRUE;
	}
	else
	{
		// tried to delete already item in trash (should purge?)
		return FALSE;
	}
}

BOOL LLItemBridge::isItemCopyable() const
{
	LLViewerInventoryItem* item = getItem();
	if (item && !item->getIsLinkType())
	{
		if (!gIsInSecondLife && !gSavedSettings.getBOOL("OSAllowInventoryLinks"))
		{
			return (item->getPermissions().allowCopyBy(gAgentID));
		}
		else
		{
			// All items can be copied since you can at least
			// paste-as-link the item even though you still may not
			// be able paste the item itself.
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLItemBridge::copyToClipboard() const
{
	if (isItemCopyable())
	{
		LLInventoryClipboard::instance().add(mUUID);
		return TRUE;
	}
	return FALSE;
}

BOOL LLItemBridge::cutToClipboard() const
{
	LLInventoryClipboard::instance().addCut(mUUID);
	return TRUE;
}

LLViewerInventoryItem* LLItemBridge::getItem() const
{
	LLViewerInventoryItem* item = NULL;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (model)
	{
		item = (LLViewerInventoryItem*)model->getItem(mUUID);
	}
	return item;
}

BOOL LLItemBridge::isItemPermissive() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		U32 mask = item->getPermissions().getMaskBase();
		if ((mask & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// +=================================================+
// |        LLFolderBridge                           |
// +=================================================+

LLFolderBridge* LLFolderBridge::sSelf = NULL;

// Can be moved to another folder
BOOL LLFolderBridge::isItemMovable()
{
	BOOL can_move = FALSE;
	LLInventoryObject* obj = getInventoryObject();
	if (obj)
	{
		can_move = !LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)obj)->getPreferredType());
	}
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (model && can_move)
	{
		can_move = model->isObjectDescendentOf(mUUID,
											   gInventory.getRootFolderID());
	}
	return can_move;
}

void LLFolderBridge::selectItem()
{
}

// Can be destroyed (or moved to trash)
BOOL LLFolderBridge::isItemRemovable()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model ||
		!model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID()))
	{
		return FALSE;
	}

	LLInventoryCategory* category = model->getCategory(mUUID);
	if (!isAgentAvatarValid() || !category)
	{
		return FALSE;
	}

	// Allow to remove calling cards sub-folders created by v2 viewers...
	bool removable_calling_cards = false;
	LLFolderType::EType cat_type = category->getPreferredType();
	if (cat_type == LLFolderType::FT_CALLINGCARD)
	{
		const LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);
		LLInventoryCategory* parent = gInventory.getCategory(category->getParentUUID());
		if (parent && parent->getUUID() != root_id)
		{
			removable_calling_cards = true;
		}
	}

	if (!removable_calling_cards &&
		LLFolderType::lookupIsProtectedType(cat_type))
	{
		return FALSE;
	}

	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	gInventory.collectDescendents(mUUID, descendent_categories,
								  descendent_items, FALSE);

	S32 i;
	for (i = 0; i < descendent_categories.count(); i++)
	{
		LLInventoryCategory* category = descendent_categories[i];
		if (LLFolderType::lookupIsProtectedType(category->getPreferredType()))
		{
			return FALSE;
		}
	}

	for (i = 0; i < descendent_items.count(); i++)
	{
		LLInventoryItem* item = descendent_items[i];
		if (!item->getIsLinkType())
		{
			if (item->getType() == LLAssetType::AT_CLOTHING ||
				item->getType() == LLAssetType::AT_BODYPART)
			{
				if (gAgentWearables.isWearingItem(item->getUUID()))
				{
					return FALSE;
				}
			}
			else if (item->getType() == LLAssetType::AT_OBJECT)
			{
				if (gAgentAvatarp->isWearingAttachment(item->getUUID()))
				{
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}

BOOL LLFolderBridge::isUpToDate() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;
	LLViewerInventoryCategory* category;
	category = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	return category &&
		   category->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN;
}

BOOL LLFolderBridge::isClipboardPasteableAsLink() const
{
	// Check normal paste-as-link permissions
	if (!LLInvFVBridge::isClipboardPasteableAsLink())
	{
		return FALSE;
	}

	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model)
	{
		return FALSE;
	}

	const LLViewerInventoryCategory *current_cat = getCategory();
	if (current_cat)
	{
		const LLUUID &current_cat_id = current_cat->getUUID();
		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		S32 count = objects.count();
		for (S32 i = 0; i < count; i++)
		{
			const LLUUID &obj_id = objects.get(i);
			const LLInventoryCategory *cat = model->getCategory(obj_id);
			if (cat)
			{
				const LLUUID &cat_id = cat->getUUID();
				// Don't allow recursive pasting
				if (cat_id == current_cat_id ||
					model->isObjectDescendentOf(current_cat_id, cat_id))
				{
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

S32 get_folder_levels(LLInventoryCategory* inv_cat)
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(inv_cat->getUUID(), cats, items);

	S32 max_child_levels = 0;

	for (S32 i=0; i < cats->count(); ++i)
	{
		LLInventoryCategory* category = cats->get(i);
		max_child_levels = llmax(max_child_levels, get_folder_levels(category));
	}

	return 1 + max_child_levels;
}

S32 get_folder_path_length(const LLUUID& ancestor_id, const LLUUID& descendant_id)
{
	S32 depth = 0;

	if (ancestor_id == descendant_id) return depth;

	LLInventoryCategory* category = gInventory.getCategory(descendant_id);

	while (category)
	{
		LLUUID parent_id = category->getParentUUID();

		if (parent_id.isNull()) break;

		depth++;

		if (parent_id == ancestor_id) return depth;

		category = gInventory.getCategory(parent_id);
	}

	llwarns << "get_folder_path_length() couldn't trace a path from the descendant to the ancestor"
			<< llendl;
	return -1;
}

BOOL LLFolderBridge::dragCategoryIntoFolder(LLInventoryCategory* inv_cat,
											BOOL drop)
{
	// This should never happen, but if an inventory item is incorrectly
	// parented, the UI will get confused and pass in a NULL.
	if (!inv_cat)
	{
		return FALSE;
	}

	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model || !isAgentAvatarValid() || !isAgentInventory())
	{
		return FALSE;
	}

	// check to make sure source is agent inventory, and is represented there.
	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL is_agent_inventory = (model->getCategory(inv_cat->getUUID()) != NULL &&
							   LLToolDragAndDrop::SOURCE_AGENT == source);

	const LLUUID& cat_id = inv_cat->getUUID();
	const LLUUID& outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);

	BOOL move_is_into_outbox = model->isObjectDescendentOf(mUUID, outbox_id);
	BOOL move_is_from_outbox = model->isObjectDescendentOf(cat_id, outbox_id);

	BOOL accept = FALSE;
	S32 i;
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	if (is_agent_inventory)
	{
		// Is the destination the trash?
		const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
		BOOL move_is_into_trash = (mUUID == trash_id ||
								   model->isObjectDescendentOf(mUUID, trash_id));
		BOOL is_movable = (!LLFolderType::lookupIsProtectedType(inv_cat->getPreferredType()));
		if (is_movable)
		{
			model->collectDescendents(cat_id, descendent_categories,
									  descendent_items, FALSE);
			for (i = 0; i < descendent_categories.count(); i++)
			{
				LLInventoryCategory* category = descendent_categories[i];
				if (LLFolderType::lookupIsProtectedType(category->getPreferredType()))
				{
					// ...can't move "special folders" like Textures
					is_movable = FALSE;
					break;
				}
			}

			if (is_movable && move_is_into_trash)
			{
				for (i = 0; i < descendent_items.count(); i++)
				{
					LLInventoryItem* item = descendent_items[i];
					if (item->getIsLinkType())
					{
						// Inventory links can always be destroyed
						continue;
					}
					if (item->getType() == LLAssetType::AT_CLOTHING ||
						item->getType() == LLAssetType::AT_BODYPART)
					{
						if (gAgentWearables.isWearingItem(item->getUUID()))
						{
							// It's generally movable, but not into the trash!
							is_movable = FALSE;
							break;
						}
					}
					else if (item->getType() == LLAssetType::AT_OBJECT)
					{
						if (gAgentAvatarp->isWearingAttachment(item->getLinkedUUID()))
						{
							// It's generally movable, but not into the trash!
							is_movable = FALSE;
							break;
						}
					}
				}
			}

			if (is_movable && move_is_into_outbox)
			{
				S32 nested_folder_levels = get_folder_path_length(outbox_id, mUUID) + get_folder_levels(inv_cat);

				if (nested_folder_levels > gSavedSettings.getU32("InventoryOutboxMaxFolderDepth"))
				{
					is_movable = FALSE;
				}
				else
				{
					S32 dragged_folder_count = descendent_categories.count();
					S32 existing_item_count = 0;
					S32 existing_folder_count = 0;

					const LLViewerInventoryCategory* master_folder;
					master_folder = model->getFirstDescendantOf(outbox_id, mUUID);

					if (master_folder != NULL)
					{
						if (model->isObjectDescendentOf(cat_id, master_folder->getUUID()))
						{
							// Don't use count because we're already inside the
							// same category anyway
							dragged_folder_count = 0;
						}
						else
						{
							// Include the master folder in the count
							existing_folder_count = 1;

							// If we're in the drop operation as opposed to the
							// drag without drop, we are doing a single
							// category at a time so don't block based on the
							// total amount of cargo data items
							if (drop)
							{
								dragged_folder_count += 1;
							}
							else
							{
								// The cargo id's count is a total of
								// categories AND items but we err on the side
								// of prevention rather than letting too many
								// folders into the hierarchy of the outbox,
								// when we're dragging the item to a new parent
								dragged_folder_count += LLToolDragAndDrop::instance().getCargoCount();
							}
						}

						// Tally the total number of categories and items
						// inside the master folder

						LLInventoryModel::cat_array_t existing_categories;
						LLInventoryModel::item_array_t existing_items;

						model->collectDescendents(master_folder->getUUID(),
												  existing_categories,
												  existing_items, FALSE);

						existing_folder_count += existing_categories.count();
						existing_item_count += existing_items.count();
					}
					else
					{
						// Assume a single category is being dragged to the
						// outbox since we evaluate one at a time when not
						// putting them under a parent item.
						dragged_folder_count += 1;
					}

					S32 nested_folder_count = existing_folder_count + dragged_folder_count;
					S32 nested_item_count = existing_item_count + descendent_items.count();

					if (nested_folder_count > gSavedSettings.getU32("InventoryOutboxMaxFolderCount") ||
						nested_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
					{
						is_movable = FALSE;
					}

					for (i = 0; i < descendent_items.count(); i++)
					{
						LLInventoryItem* item = descendent_items[i];
						if (!can_copy_to_outbox(item))
						{
							is_movable = FALSE;
							break;
						}
					}
				}
			}
		}

		accept = is_movable &&
				 mUUID != cat_id &&								// Can't move a folder into itself
				 mUUID != inv_cat->getParentUUID() &&			// Avoid moves that would change nothing
				 !model->isObjectDescendentOf(mUUID, cat_id);	// Avoid circularity
		if (accept && drop)
		{
			// Look for any gestures and deactivate them
			if (move_is_into_trash)
			{
				for (i = 0; i < descendent_items.count(); i++)
				{
					LLInventoryItem* item = descendent_items[i];
					if (item->getType() == LLAssetType::AT_GESTURE &&
						gGestureManager.isGestureActive(item->getUUID()))
					{
						gGestureManager.deactivateGesture(item->getUUID());
					}
				}
			}
			else if (move_is_into_outbox && !move_is_from_outbox)
			{
				copy_folder_to_outbox(inv_cat, mUUID, cat_id);
			}
			else
			{
				// Reparent the folder and restamp children if it's moving
				// into trash.
				LLInvFVBridge::changeCategoryParent(model,
													(LLViewerInventoryCategory*)inv_cat,
													mUUID,
													move_is_into_trash);
			}
		}
	}
	else if (LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		if (move_is_into_outbox)
		{
			accept = FALSE;
		}
		else
		{
			// content category has same ID as object itself
			LLUUID object_id = inv_cat->getUUID();
			LLUUID category_id = mUUID;
			accept = move_inv_category_world_to_agent(object_id, category_id, drop);
		}
	}

	return accept;
}

void warn_move_inventory(LLViewerObject* object, LLMoveInv* move_inv)
{
	const char* dialog = NULL;
	if (object->flagScripted())
	{
		dialog = "MoveInventoryFromScriptedObject";
	}
	else
	{
		dialog = "MoveInventoryFromObject";
	}
	LLNotifications::instance().add(dialog, LLSD(), LLSD(),
									boost::bind(move_task_inventory_callback,
												_1, _2, move_inv));
}

// Move/copy all inventory items from the Contents folder of an in-world
// object to the agent's inventory, inside a given category.
BOOL move_inv_category_world_to_agent(const LLUUID& object_id,
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*),
									  void* user_data)
{
	// Make sure the object exists. If we allowed dragging from
	// anonymous objects, it would be possible to bypass
	// permissions.
	// content category has same ID as object itself
	LLViewerObject* object = gObjectList.findObject(object_id);
	if (!object)
	{
		llinfos << "Object not found for drop." << llendl;
		return FALSE;
	}

	// this folder is coming from an object, as there is only one folder in an object, the root,
	// we need to collect the entire contents and handle them as a group
	InventoryObjectList inventory_objects;
	object->getInventoryContents(inventory_objects);

	if (inventory_objects.empty())
	{
		llinfos << "Object contents not found for drop." << llendl;
		return FALSE;
	}

	BOOL accept = TRUE;
	BOOL is_move = FALSE;

	// coming from a task. Need to figure out if the person can
	// move/copy this item.
	InventoryObjectList::iterator it = inventory_objects.begin();
	InventoryObjectList::iterator end = inventory_objects.end();
	for ( ; it != end; ++it)
	{
		// coming from a task. Need to figure out if the person can
		// move/copy this item.
		LLPermissions perm(((LLInventoryItem*)((LLInventoryObject*)(*it)))->getPermissions());
		if ((perm.allowCopyBy(gAgentID, gAgent.getGroupID())
			&& perm.allowTransferTo(gAgentID)))
//			|| gAgent.isGodlike())
		{
			accept = TRUE;
		}
		else if (object->permYouOwner())
		{
			// If the object cannot be copied, but the object the
			// inventory is owned by the agent, then the item can be
			// moved from the task to agent inventory.
			is_move = TRUE;
			accept = TRUE;
		}
		else
		{
			accept = FALSE;
			break;
		}
	}

	if (drop && accept)
	{
		it = inventory_objects.begin();
		InventoryObjectList::iterator first_it = inventory_objects.begin();
		LLMoveInv* move_inv = new LLMoveInv;
		move_inv->mObjectID = object_id;
		move_inv->mCategoryID = category_id;
		move_inv->mCallback = callback;
		move_inv->mUserData = user_data;

		for ( ; it != end; ++it)
		{
			two_uuids_t two(category_id, (*it)->getUUID());
			move_inv->mMoveList.push_back(two);
		}

		if (is_move)
		{
			// Callback called from within here.
			warn_move_inventory(object, move_inv);
		}
		else
		{
			LLNotification::Params params("MoveInventoryFromObject");
			params.functor(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
			LLNotifications::instance().forceResponse(params, 0);
		}
	}
	return accept;
}

bool LLFindWearables::operator()(LLInventoryCategory* cat,
								 LLInventoryItem* item)
{
	if (item)
	{
		if (item->getType() == LLAssetType::AT_CLOTHING ||
			item->getType() == LLAssetType::AT_BODYPART)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//Used by LLFolderBridge as callback for directory recursion.
class LLRightClickInventoryFetchObserver : public LLInventoryFetchObserver
{
public:
	LLRightClickInventoryFetchObserver()
	:	mCopyItems(false)
	{ };
	LLRightClickInventoryFetchObserver(const LLUUID& cat_id, bool copy_items)
	:	mCatID(cat_id),
		mCopyItems(copy_items)
	{ };
	virtual void done()
	{
		// we've downloaded all the items, so repaint the dialog
		LLFolderBridge::staticFolderOptionsMenu();

		gInventory.removeObserver(this);
		delete this;
	}

protected:
	LLUUID mCatID;
	bool mCopyItems;
};

//Used by LLFolderBridge as callback for directory recursion.
class LLRightClickInventoryFetchDescendentsObserver
:	public LLInventoryFetchDescendentsObserver
{
public:
	LLRightClickInventoryFetchDescendentsObserver(bool copy_items)
	:	mCopyItems(copy_items)
	{}
	~LLRightClickInventoryFetchDescendentsObserver() {}
	virtual void done();

protected:
	bool mCopyItems;
};

void LLRightClickInventoryFetchDescendentsObserver::done()
{
	// Avoid passing a NULL-ref as mCompleteFolders.front() down to
	// gInventory.collectDescendents()
	if (mCompleteFolders.empty())
	{
		llwarns << "LLRightClickInventoryFetchDescendentsObserver::done with empty mCompleteFolders" << llendl;
		dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}

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
#if 0 // HACK/TODO: Why?
	// This early causes a giant menu to get produced, and doesn't seem to be needed.
	if (!count)
	{
		llwarns << "Nothing fetched in category " << mCompleteFolders.front()
				<< llendl;
		dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}
#endif

	LLRightClickInventoryFetchObserver* outfit;
	outfit = new LLRightClickInventoryFetchObserver(mCompleteFolders.front(), mCopyItems);
	LLInventoryFetchObserver::item_ref_t ids;
	for (S32 i = 0; i < count; ++i)
	{
		ids.push_back(item_array.get(i)->getUUID());
	}

	// clean up, and remove this as an observer since the call to the
	// outfit could notify observers and throw us into an infinite loop.
	dec_busy_count();
	gInventory.removeObserver(this);
	delete this;

	// increment busy count and either tell the inventory to check &
	// call done, or add this object to the inventory for observation.
	inc_busy_count();

	// do the fetch
	outfit->fetchItems(ids);
	outfit->done();	// Not interested in waiting and this will be right 99% of the time.

	// Uncomment the following code for laggy Inventory UI.
/*	if (outfit->isFinished())
	{
		// everything is already here - call done.
		outfit->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(outfit);
	}*/
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryWearObserver
//
// Observer for "copy and wear" operation to support knowing
// when the all of the contents have been added to inventory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryCopyAndWearObserver : public LLInventoryObserver
{
public:
	LLInventoryCopyAndWearObserver(const LLUUID& cat_id,
								   int count,
								   bool folder_added = false)
	:	mCatID(cat_id),
		mContentsCount(count),
		mFolderAdded(folder_added)
	{}
	virtual ~LLInventoryCopyAndWearObserver() {}
	virtual void changed(U32 mask);

protected:
	LLUUID mCatID;
	int    mContentsCount;
	bool   mFolderAdded;
};

void LLInventoryCopyAndWearObserver::changed(U32 mask)
{
	if ((mask & LLInventoryObserver::ADD) != 0)
	{
		if (!mFolderAdded)
		{
			const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();

			std::set<LLUUID>::const_iterator id_it = changed_items.begin();
			std::set<LLUUID>::const_iterator id_end = changed_items.end();
			for ( ; id_it != id_end; ++id_it)
			{
				if ((*id_it) == mCatID)
				{
					mFolderAdded = TRUE;
					break;
				}
			}
		}

		if (mFolderAdded)
		{
			LLViewerInventoryCategory* category = gInventory.getCategory(mCatID);

			if (NULL == category)
			{
				llwarns << "gInventory.getCategory(" << mCatID << ") was NULL"
						<< llendl;
			}
			else
			{
				if (category->getDescendentCount() == mContentsCount)
				{
					gInventory.removeObserver(this);
					LLAppearanceMgr::instance().wearInventoryCategory(category,
																	  false,
																	  true);
					delete this;
				}
			}
		}
	}
}

void LLFolderBridge::performAction(LLFolderView* folder,
								   LLInventoryModel* model,
								   std::string action)
{
	if ("open" == action)
	{
		openItem();
	}
	else if ("cut" == action)
	{
		cutToClipboard();
	}
	else if ("paste" == action)
	{
		pasteFromClipboard();
	}
	else if ("paste_link" == action)
	{
		pasteLinkFromClipboard();
	}
	else if ("properties" == action)
	{
		showProperties();
	}
	else if ("replaceoutfit" == action)
	{
		modifyOutfit(false, false);
	}
	else if ("addtooutfit" == action)
	{
		modifyOutfit(true, false);
	}
	else if ("wearitems" == action)
	{
		modifyOutfit(true, true);
	}
	else if ("removefromoutfit" == action)
	{
		LLInventoryModel* model = mInventoryPanel->getModel();
		if (!model) return;
		LLViewerInventoryCategory* cat = getCategory();
		if (!cat) return;
//MK
		if (gRRenabled && !gAgent.mRRInterface.canDetachCategory(cat))
		{
			return;
		}
//mk
		LLAppearanceMgr::instance().removeInventoryCategoryFromAvatar(cat);
	}
	else if ("updatelinks" == action)
	{
		LLAppearanceMgr::instance().updateClothingOrderingInfo(mUUID);
		LLNotifications::instance().add("ReorderingWearablesLinks");
	}
	else if ("purge" == action)
	{
		LLViewerInventoryCategory* cat;
		cat = (LLViewerInventoryCategory*)getCategory();

		if (cat)
		{
			model->purgeDescendentsOf(mUUID);
		}
		LLInventoryObject* obj = model->getObject(mUUID);
		if (!obj) return;
		obj->removeFromServer();
		model->deleteObject(mUUID);
		model->notifyObservers();
	}
	else if ("restore" == action)
	{
		restoreItem();
	}
	else if ("send_to_marketplace" == action && gIsInSecondLifeProductionGrid)
	{
		LLMarketplaceInventoryImporter::instance().triggerImport();
	}
}

void LLFolderBridge::openItem()
{
	//lldebugs << "LLFolderBridge::openItem()" << llendl;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return;
	model->fetchDescendentsOf(mUUID);
}

BOOL LLFolderBridge::isItemRenameable() const
{
	LLViewerInventoryCategory* cat = (LLViewerInventoryCategory*)getCategory();
//MK
	if (gRRenabled &&
		gAgent.mRRInterface.isUnderRlvShare(cat) &&
		gAgent.mRRInterface.isFolderLocked(cat))
	{
		return FALSE;
	}
//mk
	if (cat && !LLFolderType::lookupIsProtectedType(cat->getPreferredType()) &&
		cat->getOwnerID() == gAgentID)
	{
		return TRUE;
	}
	return FALSE;
}

void LLFolderBridge::restoreItem()
{
	LLViewerInventoryCategory* cat;
	cat = (LLViewerInventoryCategory*)getCategory();
	if (cat)
	{
		LLInventoryModel* model = mInventoryPanel->getModel();
		const LLUUID new_parent = model->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(cat->getType()));
		// do not restamp children on restore
		LLInvFVBridge::changeCategoryParent(model, cat, new_parent, FALSE);
	}
}

LLFolderType::EType LLFolderBridge::getPreferredType() const
{
	LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
	LLViewerInventoryCategory* cat = getCategory();
	if (cat)
	{
		preferred_type = cat->getPreferredType();
	}

	return preferred_type;
}

// Icons for folders are based on the preferred type
LLUIImagePtr LLFolderBridge::getIcon() const
{
	std::string control;
	LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
	LLViewerInventoryCategory* cat = getCategory();
	if (cat)
	{
		preferred_type = cat->getPreferredType();
	}
	return LLViewerFolderType::lookupIcon(preferred_type);
}

BOOL LLFolderBridge::renameItem(const std::string& new_name)
{
	if (!isItemRenameable()) return FALSE;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;
	LLViewerInventoryCategory* cat = getCategory();
	if (cat && (cat->getName() != new_name))
	{
		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
		new_cat->rename(new_name);
		new_cat->updateServer(FALSE);
		model->updateCategory(new_cat);
		model->notifyObservers();
	}
	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}

BOOL LLFolderBridge::removeItem()
{
	if (!isItemRemovable())
	{
		return FALSE;
	}
	// move it to the trash
	LLPreview::hide(mUUID);
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;

	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);

	// Look for any gestures and deactivate them
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	gInventory.collectDescendents(mUUID, descendent_categories, descendent_items, FALSE);

	S32 i;
	for (i = 0; i < descendent_items.count(); i++)
	{
		LLInventoryItem* item = descendent_items[i];
		if (item->getType() == LLAssetType::AT_GESTURE
			&& gGestureManager.isGestureActive(item->getUUID()))
		{
			gGestureManager.deactivateGesture(item->getUUID());
		}
	}

	// go ahead and do the normal remove if no 'last calling
	// cards' are being removed.
	LLViewerInventoryCategory* cat = getCategory();
	if (cat)
	{
		LLInvFVBridge::changeCategoryParent(model, cat, trash_id, TRUE);
	}

	return TRUE;
}

BOOL LLFolderBridge::isClipboardPasteable() const
{
	if (LLInventoryClipboard::instance().hasContents() && isAgentInventory())
	{
		return TRUE;
	}
	return FALSE;
}

BOOL LLFolderBridge::cutToClipboard() const
{
	LLInventoryClipboard::instance().addCut(mUUID);
	return TRUE;
}

void LLFolderBridge::pasteFromClipboard()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (model && isClipboardPasteable())
	{
		const LLUUID& outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
		bool move_is_into_outbox = model->isObjectDescendentOf(mUUID, outbox_id);
		LLViewerInventoryItem* item = NULL;
		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		S32 count = objects.count();
		LLUUID parent_id(mUUID);
		if (move_is_into_outbox && count)
		{
			const LLViewerInventoryCategory* master_folder;
			master_folder = model->getFirstDescendantOf(outbox_id, parent_id);
			S32 existing_item_count = count;
			if (master_folder != NULL)
			{
				LLInventoryModel::cat_array_t existing_categories;
				LLInventoryModel::item_array_t existing_items;
				gInventory.collectDescendents(master_folder->getUUID(),
											  existing_categories,
											  existing_items, FALSE);
				existing_item_count += existing_items.count();
			}
			if (existing_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
			{
				LLNotifications::instance().add("OutboxTooManyItems");
				return;
			}
		}
		for (S32 i = 0; i < count; i++)
		{
			item = model->getItem(objects.get(i));
			if (item)
			{
				if (move_is_into_outbox)
				{
					if (can_copy_to_outbox(item))
					{
						copy_item_to_outbox(item, parent_id, LLUUID::null);
					}
				}
				else
				{
					copy_inventory_item(gAgentID,
										item->getPermissions().getOwner(),
										item->getUUID(),
										parent_id,
										std::string(),
										LLPointer<LLInventoryCallback>(NULL));
				}
			}
		}
		if (!move_is_into_outbox)
		{
			// Do cuts as well
			const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
			if (parent_id != trash_id &&
				!model->isObjectDescendentOf(parent_id, trash_id))
			{
				LLViewerInventoryCategory* cat;
				LLInventoryClipboard::instance().retrieveCuts(objects);
				count = objects.count();
				for (S32 i = 0; i < count; i++)
				{
					const LLUUID object_id = objects.get(i);
					item = model->getItem(object_id);
					if (item)
					{
						if (parent_id != item->getParentUUID())
						{
							LLInvFVBridge::changeItemParent(model, item,
															parent_id, FALSE);
						}
					}
					else
					{
						cat = model->getCategory(object_id);
						if (cat && parent_id != object_id &&
							parent_id != cat->getParentUUID() &&
							!model->isObjectDescendentOf(parent_id, object_id))
						{
							LLInvFVBridge::changeCategoryParent(model, cat,
																parent_id,
																FALSE);
						}
					}
				}
			}
		}
		model->notifyObservers();
	}
}

void LLFolderBridge::pasteLinkFromClipboard()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (model)
	{
		const LLUUID& outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
		if (model->isObjectDescendentOf(mUUID, outbox_id))
		{
			return;
		}

		const LLUUID parent_id(mUUID);

		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		for (LLDynamicArray<LLUUID>::const_iterator iter = objects.begin();
			 iter != objects.end();
			 ++iter)
		{
			const LLUUID &object_id = (*iter);
			// This should only show if the object can't find its baseobj.
			std::string description = "Broken link";

			if (LLInventoryItem *item = model->getItem(object_id))
			{
				link_inventory_item(gAgentID,
									item->getLinkedUUID(),
									parent_id,
									item->getName(),
									description,
									LLAssetType::AT_LINK,
									LLPointer<LLInventoryCallback>(NULL));
			}
		}
	}
}

void LLFolderBridge::staticFolderOptionsMenu()
{
	if (!sSelf) return;
	sSelf->folderOptionsMenu();
}

void LLFolderBridge::folderOptionsMenu()
{
	std::vector<std::string> disabled_items;

	// *TODO: Translate

	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return;

	const LLInventoryCategory* category = model->getCategory(mUUID);
	if (!category) return;

	bool is_system_folder =
			LLFolderType::lookupIsProtectedType(category->getPreferredType());

	// calling card related functionality for folders.

	// Only enable calling-card related options for non-default folders.
	if (!is_system_folder)
	{
		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (mCallingCards || checkFolderForContentsOfType(model, is_callingcard))
		{
			mItems.push_back(std::string("Calling Card Separator"));
			mItems.push_back(std::string("Conference Chat Folder"));
			mItems.push_back(std::string("IM All Contacts In Folder"));
		}
	}

	// wearables related functionality for folders.
	//is_wearable
	LLFindWearables is_wearable;
	LLIsType is_object(LLAssetType::AT_OBJECT);
	LLIsType is_gesture(LLAssetType::AT_GESTURE);

//MK
	if (!gRRenabled &&
//mk
		(mWearables ||
		 checkFolderForContentsOfType(model, is_wearable) ||
		 checkFolderForContentsOfType(model, is_object) ||
		 checkFolderForContentsOfType(model, is_gesture)))
	{
		mItems.push_back(std::string("Folder Wearables Separator"));

		// Only enable add/replace outfit for non-default folders.
		if (!is_system_folder)
		{
			mItems.push_back(std::string("Add To Outfit"));
			mItems.push_back(std::string("Wear Items"));
			mItems.push_back(std::string("Replace Outfit"));
			mItems.push_back(std::string("Update Links"));
		}
		mItems.push_back(std::string("Take Off Items"));
	}

	hideContextEntries(*mMenu, mItems, disabled_items);
}

BOOL LLFolderBridge::checkFolderForContentsOfType(LLInventoryModel* model, LLInventoryCollectFunctor& is_type)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	model->collectDescendentsIf(mUUID,
								cat_array,
								item_array,
								LLInventoryModel::EXCLUDE_TRASH,
								is_type);
	return ((item_array.count() > 0) ? TRUE : FALSE);
}

// Flags unused
void LLFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	//lldebugs << "LLFolderBridge::buildContextMenu()" << llendl;
//	std::vector<std::string> disabled_items;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	const LLUUID lost_and_found_id = model->findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	const LLUUID outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);

	mItems.clear();
	mDisabledItems.clear();

	if (lost_and_found_id == mUUID)
	{
		// This is the lost+found folder.
		mItems.push_back(std::string("Empty Lost And Found"));
	}
	else if (outbox_id == mUUID)
	{
		// This is the merchant outbox folder.
#if 0	// Does not work properly...
		mItems.push_back(std::string("New Folder"));
		mItems.push_back(std::string("Copy Separator"));
#endif
		mItems.push_back(std::string("Paste"));
		if (!LLInventoryClipboard::instance().hasCopiedContents() ||
			!isClipboardPasteable() || (flags & FIRST_SELECTED_ITEM) == 0)
		{
			mDisabledItems.push_back(std::string("Paste"));
		}
		mItems.push_back(std::string("Paste Separator"));
		mItems.push_back(std::string("Marketplace Send"));
		if (LLMarketplaceInventoryImporter::instance().isImportInProgress() ||
			!gIsInSecondLifeProductionGrid)
		{
			mDisabledItems.push_back(std::string("Marketplace Send"));
		}
	}
	else if (trash_id == mUUID)
	{
		// This is the trash.
		mItems.push_back(std::string("Empty Trash"));
	}
	else if (model->isObjectDescendentOf(mUUID, trash_id))
	{
		// This is a folder in the trash.
		mItems.clear(); // clear any items that used to exist
		const LLInventoryObject *obj = getInventoryObject();
		if (obj && obj->getIsLinkType())
		{
			mItems.push_back(std::string("Find Original"));
			if (isLinkedObjectMissing())
			{
				mDisabledItems.push_back(std::string("Find Original"));
			}
		}
		mItems.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			mDisabledItems.push_back(std::string("Purge Item"));
		}

		mItems.push_back(std::string("Restore Item"));
	}
	else if (isAgentInventory()) // do not allow creating in library
	{
			// only mature accounts can create undershirts/underwear
			/*if (!gAgent.isTeen())
			{
				sub_menu->append(new LLMenuItemCallGL("New Undershirt",
													&createNewUndershirt,
													NULL,
													(void*)this));
				sub_menu->append(new LLMenuItemCallGL("New Underpants",
													&createNewUnderpants,
													NULL,
													(void*)this));
			}*/

/*		BOOL contains_calling_cards = FALSE;
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t item_array;

		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		model->collectDescendentsIf(mUUID,
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									is_callingcard);
		if (item_array.count() > 0) contains_calling_cards = TRUE;
*/
		mItems.push_back(std::string("New Folder"));
		mItems.push_back(std::string("New Script"));
		mItems.push_back(std::string("New Note"));
		mItems.push_back(std::string("New Gesture"));
		mItems.push_back(std::string("New Clothes"));
		mItems.push_back(std::string("New Body Parts"));

		getClipboardEntries(false, mItems, mDisabledItems, flags);

		//Added by spatters to force inventory pull on right-click to display folder options correctly. 07-17-06
		mCallingCards = mWearables = FALSE;

		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (checkFolderForContentsOfType(model, is_callingcard))
		{
			mCallingCards=TRUE;
		}

		LLFindWearables is_wearable;
		LLIsType is_object(LLAssetType::AT_OBJECT);
		LLIsType is_gesture(LLAssetType::AT_GESTURE);

		if (checkFolderForContentsOfType(model, is_wearable) ||
			checkFolderForContentsOfType(model, is_object) ||
			checkFolderForContentsOfType(model, is_gesture))
		{
			mWearables = TRUE;
		}

//MK
		if (gRRenabled && mWearables)
		{
			mItems.push_back("Folder Wearables Separator");
			mItems.push_back("Add To Outfit");
			mItems.push_back("Wear Items");
			mItems.push_back("Replace Outfit");
			mItems.push_back("Take Off Items");
			mItems.push_back("Update Links");
			if (gAgent.mRRInterface.mContainsDetach &&
				(!gSavedSettings.getBOOL("RestrainedLoveAllowWear") ||
				 gAgent.mRRInterface.mContainsDefaultwear))
			{
				mDisabledItems.push_back("Add To Outfit");
				mDisabledItems.push_back("Wear Items");
				mDisabledItems.push_back("Replace Outfit");
				mDisabledItems.push_back("Take Off Items");
			}
			else
			{
				LLViewerInventoryCategory* cat = (LLViewerInventoryCategory*)model->getCategory(mUUID);
				if (!gAgent.mRRInterface.canAttachCategory(cat))
				{
					mDisabledItems.push_back("Add To Outfit");
					mDisabledItems.push_back("Wear Items");
					mDisabledItems.push_back("Replace Outfit");
				}
				if (!gAgent.mRRInterface.canDetachCategory(cat))
				{
					mDisabledItems.push_back("Take Off Items");
				}
			}
		}
//mk

		mMenu = &menu;
		sSelf = this;
		LLRightClickInventoryFetchDescendentsObserver* fetch = new LLRightClickInventoryFetchDescendentsObserver(FALSE);

		LLInventoryFetchDescendentsObserver::folder_ref_t folders;
		LLViewerInventoryCategory* category = (LLViewerInventoryCategory*)model->getCategory(mUUID);
		folders.push_back(category->getUUID());
		fetch->fetchDescendents(folders);
		inc_busy_count();
		if (fetch->isFinished())
		{
			// everything is already here - call done.
			fetch->done();
		}
		else
		{
			// it's all on it's way - add an observer, and the inventory
			// will call done for us when everything is here.
			gInventory.addObserver(fetch);
		}
	}
	else
	{
		mItems.push_back(std::string("--no options--"));
		mDisabledItems.push_back(std::string("--no options--"));
	}
	hideContextEntries(menu, mItems, mDisabledItems);
}

BOOL LLFolderBridge::hasChildren() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;
	LLInventoryModel::EHasChildren has_children;
	has_children = gInventory.categoryHasChildren(mUUID);
	return has_children != LLInventoryModel::CHILDREN_NO;
}

BOOL LLFolderBridge::dragOrDrop(MASK mask, BOOL drop,
								EDragAndDropType cargo_type,
								void* cargo_data)
{
	//llinfos << "LLFolderBridge::dragOrDrop()" << llendl;
	BOOL accept = FALSE;
	switch (cargo_type)
	{
	case DAD_TEXTURE:
	case DAD_SOUND:
	case DAD_CALLINGCARD:
	case DAD_LANDMARK:
	case DAD_SCRIPT:
	case DAD_OBJECT:
	case DAD_NOTECARD:
	case DAD_CLOTHING:
	case DAD_BODYPART:
	case DAD_ANIMATION:
	case DAD_GESTURE:
	case DAD_MESH:
	case DAD_LINK:
		accept = dragItemIntoFolder((LLInventoryItem*)cargo_data, drop);
		break;
	case DAD_CATEGORY:
		accept = dragCategoryIntoFolder((LLInventoryCategory*)cargo_data, drop);
		break;
	default:
		break;
	}
	return accept;
}

LLViewerInventoryCategory* LLFolderBridge::getCategory() const
{
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (model)
	{
		cat = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	}
	return cat;
}

// static
void LLFolderBridge::pasteClipboard(void* user_data)
{
	LLFolderBridge* self = (LLFolderBridge*)user_data;
	if (self) self->pasteFromClipboard();
}

void LLFolderBridge::createNewCategory(void* user_data)
{
	LLFolderBridge* bridge = (LLFolderBridge*)user_data;
	if (!bridge) return;
	LLInventoryPanel* panel = bridge->mInventoryPanel;
	LLInventoryModel* model = panel->getModel();
	if (!model) return;
	LLUUID id;
	id = model->createNewCategory(bridge->getUUID(),
								  LLFolderType::FT_NONE,
								  LLStringUtil::null);
	model->notifyObservers();

	// At this point, the bridge has probably been deleted, but the
	// view is still there.
	panel->setSelection(id, TAKE_FOCUS_YES);
}

void LLFolderBridge::createNewShirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHIRT);
}

void LLFolderBridge::createNewPants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_PANTS);
}

void LLFolderBridge::createNewShoes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHOES);
}

void LLFolderBridge::createNewSocks(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SOCKS);
}

void LLFolderBridge::createNewJacket(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_JACKET);
}

void LLFolderBridge::createNewSkirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SKIRT);
}

void LLFolderBridge::createNewGloves(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_GLOVES);
}

void LLFolderBridge::createNewUndershirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_UNDERSHIRT);
}

void LLFolderBridge::createNewUnderpants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_UNDERPANTS);
}

void LLFolderBridge::createNewAlpha(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_ALPHA);
}

void LLFolderBridge::createNewTattoo(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_TATTOO);
}

void LLFolderBridge::createNewPhysics(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_PHYSICS);
}

void LLFolderBridge::createNewShape(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHAPE);
}

void LLFolderBridge::createNewSkin(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SKIN);
}

void LLFolderBridge::createNewHair(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_HAIR);
}

void LLFolderBridge::createNewEyes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_EYES);
}

// static
void LLFolderBridge::createWearable(LLFolderBridge* bridge, LLWearableType::EType type)
{
	if (!bridge) return;
	LLUUID parent_id = bridge->getUUID();
	LLAgentWearables::createWearable(type, parent_id);
}

// Separate function so can be called by global menu as well as right-click
// menu.

void LLFolderBridge::modifyOutfit(bool append, bool replace)
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return;
	LLViewerInventoryCategory* cat = getCategory();
	if (!cat) return;
//MK
	if (gRRenabled && !gAgent.mRRInterface.canAttachCategory(cat))
	{
		return;
	}
//mk
	LLAppearanceMgr::instance().wearInventoryCategoryOnAvatar(cat, append, replace);
}

// helper stuff
bool move_task_inventory_callback(const LLSD& notification,
								  const LLSD& response,
								  LLMoveInv* move_inv)
{
	LLFloaterOpenObject::LLCatAndWear* cat_and_wear = (LLFloaterOpenObject::LLCatAndWear*)move_inv->mUserData;
	LLViewerObject* object = gObjectList.findObject(move_inv->mObjectID);
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (option == 0 && object)
	{
		if (cat_and_wear && cat_and_wear->mWear) // && !cat_and_wear->mFolderResponded)
		{
			InventoryObjectList inventory_objects;
			object->getInventoryContents(inventory_objects);
			int contents_count = inventory_objects.size() - 1; //subtract one for containing folder

			LLInventoryCopyAndWearObserver* inventoryObserver;
			inventoryObserver = new LLInventoryCopyAndWearObserver(cat_and_wear->mCatID,
																   contents_count,
																   cat_and_wear->mFolderResponded);
			gInventory.addObserver(inventoryObserver);
		}

		two_uuids_list_t::iterator move_it;
		for (move_it = move_inv->mMoveList.begin();
			 move_it != move_inv->mMoveList.end(); ++move_it)
		{
			object->moveInventory(move_it->first, move_it->second);
		}

		// update the UI.
		dialog_refresh_all();
	}

	if (move_inv->mCallback)
	{
		move_inv->mCallback(option, move_inv->mUserData);
	}

	delete move_inv;
	return false;
}

BOOL LLFolderBridge::dragItemIntoFolder(LLInventoryItem* inv_item,
										BOOL drop)
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model || !isAgentInventory() || !isAgentAvatarValid())
	{
		return FALSE;
	}

	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	const LLUUID outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);

	BOOL move_is_into_trash = (mUUID == trash_id || model->isObjectDescendentOf(mUUID, trash_id));
	BOOL move_is_into_outbox = model->isObjectDescendentOf(mUUID, outbox_id);
	BOOL move_is_from_outbox = model->isObjectDescendentOf(inv_item->getUUID(), outbox_id);

	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL accept = FALSE;
	LLViewerObject* object = NULL;
	if (LLToolDragAndDrop::SOURCE_AGENT == source)
	{

		BOOL is_movable = TRUE;
		if (inv_item->getActualType() == LLAssetType::AT_CATEGORY)
		{
			is_movable = !LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)inv_item)->getPreferredType());
		}

		if (is_movable && move_is_into_trash)
		{
			if (inv_item->getIsLinkType())
			{
				is_movable = TRUE;
			}
			else
			{
				switch (inv_item->getType())
				{
					case LLAssetType::AT_CLOTHING:
					case LLAssetType::AT_BODYPART:
						is_movable = !gAgentWearables.isWearingItem(inv_item->getUUID());
						break;

					case LLAssetType::AT_OBJECT:
						is_movable = !gAgentAvatarp->isWearingAttachment(inv_item->getUUID());
						break;
					default:
						break;
				}
			}
		}

		if (move_is_into_outbox && !move_is_from_outbox)
		{
			accept = can_copy_to_outbox(inv_item);

			if (accept)
			{
				const LLViewerInventoryCategory* master_folder;
				master_folder = model->getFirstDescendantOf(outbox_id, mUUID);

				S32 existing_item_count = LLToolDragAndDrop::instance().getCargoCount();

				if (master_folder != NULL)
				{
					LLInventoryModel::cat_array_t existing_categories;
					LLInventoryModel::item_array_t existing_items;

					gInventory.collectDescendents(master_folder->getUUID(),
												  existing_categories,
												  existing_items, FALSE);

					existing_item_count += existing_items.count();
				}

				if (existing_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
				{
					accept = FALSE;
				}
			}
		}
		else
		{
			accept = is_movable && (mUUID != inv_item->getParentUUID());
		}
		if (accept && drop)
		{
			if (move_is_into_trash &&
				inv_item->getType() == LLAssetType::AT_GESTURE &&
				gGestureManager.isGestureActive(inv_item->getUUID()))
			{
				gGestureManager.deactivateGesture(inv_item->getUUID());
			}
			// If an item is being dragged between windows, unselect
			// everything in the active window so that we don't follow
			// the selection to its new location (which is very
			// annoying).
			if (LLInventoryView::getActiveInventory())
			{
				LLInventoryPanel* active_panel = LLInventoryView::getActiveInventory()->getPanel();
				if (active_panel && mInventoryPanel != active_panel)
				{
					active_panel->unSelectAll();
				}
			}

			if (move_is_into_outbox && (!move_is_from_outbox || mUUID == outbox_id))
			{
				copy_item_to_outbox(inv_item, mUUID, LLUUID::null);
			}
			else
			{
				LLInvFVBridge::changeItemParent(model,
												(LLViewerInventoryItem*)inv_item,
												mUUID,
												move_is_into_trash); // restamp if moving into trash.
			}
		}
	}
	else if (LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		// Make sure the object exists. If we allowed dragging from
		// anonymous objects, it would be possible to bypass
		// permissions.
		object = gObjectList.findObject(inv_item->getParentUUID());
		if (!object)
		{
			llinfos << "Object not found for drop." << llendl;
			return FALSE;
		}

		// coming from a task. Need to figure out if the person can
		// move/copy this item.
		LLPermissions perm(inv_item->getPermissions());
		BOOL is_move = FALSE;
		if ((perm.allowCopyBy(gAgentID, gAgent.getGroupID())
			 && perm.allowTransferTo(gAgentID)))
//		   || gAgent.isGodlike())

		{
			accept = TRUE;
		}
		else if (object->permYouOwner())
		{
			// If the object cannot be copied, but the object the
			// inventory is owned by the agent, then the item can be
			// moved from the task to agent inventory.
			is_move = TRUE;
			accept = TRUE;
		}
		if (move_is_into_outbox)
		{
			accept = FALSE;
		}
		if (drop && accept)
		{
			LLMoveInv* move_inv = new LLMoveInv;
			move_inv->mObjectID = inv_item->getParentUUID();
			two_uuids_t item_pair(mUUID, inv_item->getUUID());
			move_inv->mMoveList.push_back(item_pair);
			move_inv->mCallback = NULL;
			move_inv->mUserData = NULL;
			if (is_move)
			{
				warn_move_inventory(object, move_inv);
			}
			else
			{
				LLNotification::Params params("MoveInventoryFromObject");
				params.functor(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
				LLNotifications::instance().forceResponse(params, 0);
			}
		}

	}
	else if (LLToolDragAndDrop::SOURCE_NOTECARD == source)
	{
		accept = !move_is_into_outbox;
		if (accept && drop)
		{
			copy_inventory_from_notecard(LLToolDragAndDrop::getInstance()->getObjectID(),
										 LLToolDragAndDrop::getInstance()->getSourceID(),
										 inv_item);
		}
	}
	else if (LLToolDragAndDrop::SOURCE_LIBRARY == source)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_item;
		if (item && item->isFinished())
		{
			accept = !move_is_into_outbox;
			if (accept && drop)
			{
				copy_inventory_item(gAgentID,
									inv_item->getPermissions().getOwner(),
									inv_item->getUUID(),
									mUUID,
									std::string(),
									LLPointer<LLInventoryCallback>(NULL));
			}
		}
	}
	else
	{
		llwarns << "unhandled drag source" << llendl;
	}
	return accept;
}

// +=================================================+
// |        LLScriptBridge (DEPRECATED)              |
// +=================================================+

LLUIImagePtr LLScriptBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_SCRIPT, LLInventoryType::IT_LSL, 0, FALSE);
}

// +=================================================+
// |        LLTextureBridge                          |
// +=================================================+

std::string LLTextureBridge::sPrefix("Texture: ");

LLUIImagePtr LLTextureBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_TEXTURE, mInvType, 0, FALSE);
}

void open_texture(const LLUUID& item_id,
				   const std::string& title,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
//MK
	if (gRRenabled && gAgent.mRRInterface.contains ("viewtexture"))
	{
		return;
	}
//mk
	// See if we can bring an exiting preview to the front
	if (!LLPreview::show(item_id, take_focus))
	{
		// There isn't one, so make a new preview
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewTextureRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);

		LLPreviewTexture* preview;
		preview = new LLPreviewTexture("preview texture",
										  rect,
										  title,
										  item_id,
										  LLUUID::null,
										  show_keep_discard);
		preview->setSourceID(source_id);
		if (take_focus) preview->setFocus(TRUE);

		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}

void LLTextureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		open_texture(mUUID, getPrefix() + item->getName(), FALSE);
	}
}

// +=================================================+
// |        LLSoundBridge                            |
// +=================================================+

std::string LLSoundBridge::sPrefix("Sound: ");

LLUIImagePtr LLSoundBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_SOUND, LLInventoryType::IT_SOUND, 0, FALSE);
}

void LLSoundBridge::openItem()
{
// Changed this back to the way it USED to work:
// only open the preview dialog through the contextual right-click menu
// double-click just plays the sound

	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		openSoundPreview((void*)this);
		//send_uuid_sound_trigger(item->getAssetUUID(), 1.0);
	}

//	if (!LLPreview::show(mUUID))
//	{
//		S32 left, top;
//		gFloaterView->getNewFloaterPosition(&left, &top);
//		LLRect rect = gSavedSettings.getRect("PreviewSoundRect");
//		rect.translate(left - rect.mLeft, top - rect.mTop);
//			new LLPreviewSound("preview sound",
//							   rect,
//							   getPrefix() + getName(),
//							   mUUID));
//	}
}

void LLSoundBridge::previewItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		send_sound_trigger(item->getAssetUUID(), 1.0);
	}
}

void LLSoundBridge::openSoundPreview(void* which)
{
	LLSoundBridge *me = (LLSoundBridge *)which;
	if (!LLPreview::show(me->mUUID))
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewSoundRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);
		LLPreviewSound* preview = new LLPreviewSound("preview sound",
										   rect,
										   me->getPrefix() + me->getName(),
										   me->mUUID);
		preview->setFocus(TRUE);
		// Keep entirely onscreen.
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}

void LLSoundBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	//lldebugs << "LLTextureBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	// *TODO: Translate
	if (isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Sound Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Sound Separator"));
	items.push_back(std::string("Sound Play"));

	hideContextEntries(menu, items, disabled_items);
}

// +=================================================+
// |        LLLandmarkBridge                         |
// +=================================================+

std::string LLLandmarkBridge::sPrefix("Landmark:  ");

LLUIImagePtr LLLandmarkBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_LANDMARK, LLInventoryType::IT_LANDMARK, mVisited, FALSE);
}

void LLLandmarkBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	// *TODO: Translate
	//lldebugs << "LLLandmarkBridge::buildContextMenu()" << llendl;
	if (isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Landmark Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Landmark Separator"));
	items.push_back(std::string("Teleport To Landmark"));

	hideContextEntries(menu, items, disabled_items);

}

// virtual
void LLLandmarkBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("teleport" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			gAgent.teleportViaLandmark(item->getAssetUUID());

			// we now automatically track the landmark you're teleporting to
			// because you'll probably arrive at a telehub instead
			if (gFloaterWorldMap)
			{
				gFloaterWorldMap->trackLandmark(item->getUUID());  // remember this must be the item UUID, not the asset UUID
			}
		}
	}
	if ("about" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			open_landmark(item, std::string("  ") + getPrefix() + item->getName(), FALSE);
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

void open_landmark(LLViewerInventoryItem* inv_item,
				   const std::string& title,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
	// See if we can bring an exiting preview to the front
	if (!LLPreview::show(inv_item->getUUID(), take_focus))
	{
		// There isn't one, so make a new preview
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewLandmarkRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);

		LLPreviewLandmark* preview = new LLPreviewLandmark(title,
								  rect,
								  title,
								  inv_item->getUUID(),
								  show_keep_discard,
								  inv_item);
		preview->setSourceID(source_id);
		if (take_focus) preview->setFocus(TRUE);
		// keep onscreen
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}

static bool open_landmark_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	LLUUID asset_id = notification["payload"]["asset_id"].asUUID();
	LLUUID item_id = notification["payload"]["item_id"].asUUID();
	if (option == 0)
	{
		// HACK: This is to demonstrate teleport on double click for landmarks
		gAgent.teleportViaLandmark(asset_id);

		// we now automatically track the landmark you're teleporting to
		// because you'll probably arrive at a telehub instead
		if (gFloaterWorldMap)
		{
			gFloaterWorldMap->trackLandmark(item_id); // remember this is the item UUID not the asset UUID
		}
	}

	return false;
}
static LLNotificationFunctorRegistration open_landmark_callback_reg("TeleportFromLandmark", open_landmark_callback);

void LLLandmarkBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		// Opening (double-clicking) a landmark immediately teleports,
		// but warns you the first time.
		// open_landmark(item, std::string("  ") + getPrefix() + item->getName(), FALSE);
		LLSD payload;
		payload["asset_id"] = item->getAssetUUID();
		payload["item_id"] = item->getUUID();
		LLNotifications::instance().add("TeleportFromLandmark", LLSD(), payload);
	}
}

// +=================================================+
// |        LLCallingCardObserver                    |
// +=================================================+
void LLCallingCardObserver::changed(U32 mask)
{
	mBridgep->refreshFolderViewItem();
}

// +=================================================+
// |        LLCallingCardBridge                      |
// +=================================================+

std::string LLCallingCardBridge::sPrefix("Calling Card: ");

LLCallingCardBridge::LLCallingCardBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
	LLItemBridge(inventory, uuid)
{
	mObserver = new LLCallingCardObserver(this);
	LLAvatarTracker::instance().addObserver(mObserver);
}

LLCallingCardBridge::~LLCallingCardBridge()
{
	LLAvatarTracker::instance().removeObserver(mObserver);
	delete mObserver;
}

void LLCallingCardBridge::refreshFolderViewItem()
{
	LLFolderViewItem* itemp = mInventoryPanel->getRootFolder()->getItemByID(mUUID);
	if (itemp)
	{
		itemp->refresh();
	}
}

// virtual
void LLCallingCardBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("begin_im" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgentID) &&
			(!item->getCreatorUUID().isNull()))
		{
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession(item->getName(), IM_NOTHING_SPECIAL, item->getCreatorUUID());
		}
	}
	else if ("lure" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgentID) &&
			(!item->getCreatorUUID().isNull()))
		{
			handle_lure(item->getCreatorUUID());
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

LLUIImagePtr LLCallingCardBridge::getIcon() const
{
	BOOL online = FALSE;
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		online = LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID());
	}
	return LLInventoryIcon::getIcon(LLAssetType::AT_CALLINGCARD, LLInventoryType::IT_CALLINGCARD, online, FALSE);
}

std::string LLCallingCardBridge::getLabelSuffix() const
{
	LLViewerInventoryItem* item = getItem();
	if (item && LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()))
	{
		return LLItemBridge::getLabelSuffix() + " (online)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

void LLCallingCardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item && !item->getCreatorUUID().isNull())
	{
		BOOL online;
		online = LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID());
		LLFloaterAvatarInfo::showFromFriend(item->getCreatorUUID(), online);
	}
}

void LLCallingCardBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	//lldebugs << "LLCallingCardBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if (isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		LLInventoryItem* item = getItem();
		BOOL good_card = (item
						  && (LLUUID::null != item->getCreatorUUID())
						  && (item->getCreatorUUID() != gAgentID));
		BOOL user_online = (LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()));
		items.push_back(std::string("Send Instant Message Separator"));
		items.push_back(std::string("Send Instant Message"));
		items.push_back(std::string("Offer Teleport..."));
		items.push_back(std::string("Conference Chat"));

		if (!good_card)
		{
			disabled_items.push_back(std::string("Send Instant Message"));
		}
		if (!good_card || !user_online)
		{
			disabled_items.push_back(std::string("Offer Teleport..."));
			disabled_items.push_back(std::string("Conference Chat"));
		}
	}
	hideContextEntries(menu, items, disabled_items);
}

BOOL LLCallingCardBridge::dragOrDrop(MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data)
{
	LLViewerInventoryItem* item = getItem();
	BOOL rv = FALSE;
	if (item)
	{
		// check the type
		switch (cargo_type)
		{
		case DAD_TEXTURE:
		case DAD_SOUND:
		case DAD_LANDMARK:
		case DAD_SCRIPT:
		case DAD_CLOTHING:
		case DAD_OBJECT:
		case DAD_NOTECARD:
		case DAD_BODYPART:
		case DAD_ANIMATION:
		case DAD_GESTURE:
		case DAD_MESH:
			{
				LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
				const LLPermissions& perm = inv_item->getPermissions();
				if (gInventory.getItem(inv_item->getUUID())
				   && perm.allowOperationBy(PERM_TRANSFER, gAgentID))
				{
					rv = TRUE;
					if (drop)
					{
						LLToolDragAndDrop::giveInventory(item->getCreatorUUID(),
														 (LLInventoryItem*)cargo_data);
					}
				}
				else
				{
					// It's not in the user's inventory (it's probably in
					// an object's contents), so disallow dragging it here.
					// You can't give something you don't yet have.
					rv = FALSE;
				}
				break;
			}
		case DAD_CATEGORY:
			{
				LLInventoryCategory* inv_cat = (LLInventoryCategory*)cargo_data;
				if (gInventory.getCategory(inv_cat->getUUID()))
				{
					rv = TRUE;
					if (drop)
					{
						LLToolDragAndDrop::giveInventoryCategory(
							item->getCreatorUUID(),
							inv_cat);
					}
				}
				else
				{
					// It's not in the user's inventory (it's probably in
					// an object's contents), so disallow dragging it here.
					// You can't give something you don't yet have.
					rv = FALSE;
				}
				break;
			}
		default:
			break;
		}
	}
	return rv;
}

// +=================================================+
// |        LLNotecardBridge                         |
// +=================================================+

std::string LLNotecardBridge::sPrefix("Note: ");

LLUIImagePtr LLNotecardBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_NOTECARD,
									LLInventoryType::IT_NOTECARD, 0, FALSE);
}

void open_notecard(LLViewerInventoryItem* inv_item,
				   const std::string& title,
				   const LLUUID& object_id,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
//MK
	if (gRRenabled && gAgent.mRRInterface.contains ("viewnote"))
	{
		return;
	}
//mk
	// See if we can bring an existing preview to the front
	if (!LLPreview::show(inv_item->getUUID(), take_focus))
	{
		// There isn't one, so make a new preview
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("NotecardEditorRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);
		LLPreviewNotecard* preview;
		preview = new LLPreviewNotecard("preview notecard", rect, title,
						inv_item->getUUID(), object_id, inv_item->getAssetUUID(),
						show_keep_discard, inv_item);
		preview->setSourceID(source_id);
		if (take_focus) preview->setFocus(TRUE);
		// Force to be entirely onscreen.
		gFloaterView->adjustToFitScreen(preview, FALSE);

		//if (source_id.notNull())
		//{
		//	// look for existing tabbed view for content from same source
		//	LLPreview* existing_preview = LLPreview::getPreviewForSource(source_id);
		//	if (existing_preview)
		//	{
		//		// found existing preview from this source
		//		// is it already hosted in a multi-preview window?
		//		LLMultiPreview* preview_hostp = (LLMultiPreview*)existing_preview->getHost();
		//		if (!preview_hostp)
		//		{
		//			// create new multipreview if it doesn't exist
		//			LLMultiPreview* preview_hostp = new LLMultiPreview(existing_preview->getRect());
		//			preview_hostp->addFloater(existing_preview);
		//		}
		//		// add this preview to existing host
		//		preview_hostp->addFloater(preview);
		//	}
		//}
	}
}

void LLNotecardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		open_notecard(item, getPrefix() + item->getName(), LLUUID::null, FALSE);
	}
}

// +=================================================+
// |        LLGestureBridge                          |
// +=================================================+

std::string LLGestureBridge::sPrefix("Gesture: ");

LLUIImagePtr LLGestureBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_GESTURE, LLInventoryType::IT_GESTURE, 0, FALSE);
}

LLFontGL::StyleFlags LLGestureBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;

	if (gGestureManager.isGestureActive(mUUID))
	{
		font |= LLFontGL::BOLD;
	}

	const LLViewerInventoryItem* item = getItem();
	if (item && item->getIsLinkType())
	{
		font |= LLFontGL::ITALIC;
	}

	return (LLFontGL::StyleFlags)font;
}

std::string LLGestureBridge::getLabelSuffix() const
{
	if (gGestureManager.isGestureActive(mUUID))
	{
		return LLItemBridge::getLabelSuffix() + " (active)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

// virtual
void LLGestureBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("activate" == action)
	{
		gGestureManager.activateGesture(mUUID);

		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;

		// Since we just changed the suffix to indicate (active)
		// the server doesn't need to know, just the viewer.
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else if ("deactivate" == action)
	{
		gGestureManager.deactivateGesture(mUUID);

		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;

		// Since we just changed the suffix to indicate (active)
		// the server doesn't need to know, just the viewer.
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else LLItemBridge::performAction(folder, model, action);
}

void LLGestureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (!item) return;

	// See if we can bring an existing preview to the front
	if (!LLPreview::show(mUUID))
	{
		LLUUID item_id = mUUID;
		std::string title = getPrefix() + item->getName();
		LLUUID object_id = LLUUID::null;

		// TODO: save the rectangle
		LLPreviewGesture* preview = LLPreviewGesture::show(title, item_id, object_id);
		preview->setFocus(TRUE);

		// Force to be entirely onscreen.
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}

BOOL LLGestureBridge::removeItem()
{
	// Force close the preview window, if it exists
	gGestureManager.deactivateGesture(mUUID);
	return LLItemBridge::removeItem();
}

void LLGestureBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	//lldebugs << "LLGestureBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if (isInTrash())
	{
		const LLInventoryObject *obj = getInventoryObject();
		if (obj && obj->getIsLinkType())
		{
			items.push_back(std::string("Find Original"));
			if (isLinkedObjectMissing())
			{
				disabled_items.push_back(std::string("Find Original"));
			}
		}
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Gesture Separator"));
		items.push_back(std::string("Activate"));
		items.push_back(std::string("Deactivate"));

		/*menu.append(new LLMenuItemCallGL("Activate",
										 handleActivateGesture,
										 enableActivateGesture,
										 (void*)this));
		menu.append(new LLMenuItemCallGL("Deactivate",
										 handleDeactivateGesture,
										 enableDeactivateGesture,
										 (void*)this));*/
	}
	hideContextEntries(menu, items, disabled_items);
}

// +=================================================+
// |        LLAnimationBridge                        |
// +=================================================+

std::string LLAnimationBridge::sPrefix("Animation: ");

LLUIImagePtr LLAnimationBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_ANIMATION, LLInventoryType::IT_ANIMATION, 0, FALSE);
}

void LLAnimationBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	//lldebugs << "LLAnimationBridge::buildContextMenu()" << llendl;
	if (isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Animation Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Animation Separator"));
	items.push_back(std::string("Animation Play"));
	items.push_back(std::string("Animation Audition"));

	hideContextEntries(menu, items, disabled_items);

}

// virtual
void LLAnimationBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	S32 activate = 0;

	if (action == "playworld" || action == "playlocal")
	{
		activate = (action == "playworld") ? 1 : 2;

		// See if we can bring an existing preview to the front
		if (!LLPreview::show(mUUID))
		{
			// There isn't one, so make a new preview
			LLViewerInventoryItem* item = getItem();
			if (item)
			{
				S32 left, top;
				gFloaterView->getNewFloaterPosition(&left, &top);
				LLRect rect = gSavedSettings.getRect("PreviewAnimRect");
				rect.translate(left - rect.mLeft, top - rect.mTop);
				LLPreviewAnim* preview = new LLPreviewAnim("preview anim",
										rect,
										getPrefix() + item->getName(),
										mUUID,
										activate);
				// Force to be entirely onscreen.
				gFloaterView->adjustToFitScreen(preview, FALSE);
			}
		}
	}
	else
	{
		LLItemBridge::performAction(folder, model, action);
	}
}

void LLAnimationBridge::openItem()
{
	// See if we can bring an existing preview to the front
	if (!LLPreview::show(mUUID))
	{
		// There isn't one, so make a new preview
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLRect rect = gSavedSettings.getRect("PreviewAnimRect");
			rect.translate(left - rect.mLeft, top - rect.mTop);
			LLPreviewAnim* preview = new LLPreviewAnim("preview anim",
									rect,
									getPrefix() + item->getName(),
									mUUID,
									0);
			preview->setFocus(TRUE);
			// Force to be entirely onscreen.
			gFloaterView->adjustToFitScreen(preview, FALSE);
		}
	}
}

// +=================================================+
// |        LLObjectBridge                           |
// +=================================================+

// static
std::string LLObjectBridge::sPrefix("Object: ");

BOOL LLObjectBridge::isItemRemovable()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model)
	{
		return FALSE;
	}
	const LLInventoryObject *obj = model->getItem(mUUID);
	if (obj && obj->getIsLinkType())
	{
		return TRUE;
	}
	if (!isAgentAvatarValid()) return FALSE;
	if (gAgentAvatarp->isWearingAttachment(mUUID)) return FALSE;
	return LLInvFVBridge::isItemRemovable();
}

LLUIImagePtr LLObjectBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_OBJECT, mInvType, mAttachPt, mIsMultiObject);
}

// virtual
void LLObjectBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("attach" == action || "attach_add" == action)
	{
		bool replace = ("attach" == action); // Replace if "Wear"ing.
		LLUUID object_id = mUUID;
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gInventory.getItem(object_id);
		if (item && gInventory.isObjectDescendentOf(object_id, gInventory.getRootFolderID()))
		{
//MK
			if (gRRenabled && gAgent.mRRInterface.canAttach(item))
			{
				LLViewerJointAttachment* attachmentp = NULL;
				// if it's a no-mod item, the containing folder has priority to
				// decide where to wear it
				if (!item->getPermissions().allowModifyBy(gAgentID))
				{
					attachmentp = gAgent.mRRInterface.findAttachmentPointFromParentName(item);
					if (attachmentp)
					{
						LLAppearanceMgr::instance().rezAttachment(item,
																  attachmentp,
																  replace);
					}
					else
					{
						// but the name itself could also have the information => check
						attachmentp = gAgent.mRRInterface.findAttachmentPointFromName(item->getName());
						if (attachmentp)
						{
							LLAppearanceMgr::instance().rezAttachment(item,
																	  attachmentp,
																	  replace);
						}
						else if (!gAgent.mRRInterface.mContainsDefaultwear &&
								 gSavedSettings.getBOOL("RestrainedLoveAllowWear"))
						{
							LLAppearanceMgr::instance().rezAttachment(item,
																	  NULL,
																	  replace);
						}
					}
				}
				else
				{
					// this is a mod item, wear it according to its name
					attachmentp = gAgent.mRRInterface.findAttachmentPointFromName(item->getName());
					if (attachmentp)
					{
						LLAppearanceMgr::instance().rezAttachment(item,
																  attachmentp,
																  replace);
					}
					else if (!gAgent.mRRInterface.mContainsDefaultwear &&
							 gSavedSettings.getBOOL("RestrainedLoveAllowWear"))
					{
						LLAppearanceMgr::instance().rezAttachment(item,
																  NULL,
																  replace);
					}
				}
			}
			else
//mk
			{
				LLAppearanceMgr::instance().rezAttachment(item, NULL, replace);
			}
		}
		else if (item && item->isFinished())
		{
			// must be in library. copy it to our inventory and put it on.
			LLPointer<LLInventoryCallback> cb = new RezAttachmentCallback(0, replace);
			copy_inventory_item(gAgentID, item->getPermissions().getOwner(),
								item->getUUID(), LLUUID::null, std::string(), cb);
		}
		gFocusMgr.setKeyboardFocus(NULL);
	}
	else if ("detach" == action)
	{
		LLInventoryItem* item = gInventory.getItem(mUUID);
		if (item)
		{
			LLVOAvatarSelf::detachAttachmentIntoInventory(item->getLinkedUUID());
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

void LLObjectBridge::openItem()
{
	if (isAgentAvatarValid())
	{
		if (gAgentAvatarp->isWearingAttachment(mUUID))
		{
//MK
			if (gRRenabled &&
				!gAgent.mRRInterface.canDetach(gAgentAvatarp->getWornAttachment(mUUID)))
			{
				return;
			}
//mk
			performAction(NULL, NULL, "detach");
		}
		else
		{
			performAction(NULL, NULL, "attach");
		}
	}
}

LLFontGL::StyleFlags LLObjectBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;

	if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(mUUID))
	{
		font |= LLFontGL::BOLD;
	}

	const LLViewerInventoryItem* item = getItem();
	if (item && item->getIsLinkType())
	{
		font |= LLFontGL::ITALIC;
	}

	return (LLFontGL::StyleFlags)font;
}

std::string LLObjectBridge::getLabelSuffix() const
{
	if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(mUUID))
	{
		std::string attachment_point_name = gAgentAvatarp->getAttachedPointName(mUUID);
		LLStringUtil::toLower(attachment_point_name);
		return LLItemBridge::getLabelSuffix() + std::string(" (worn on ") + attachment_point_name + std::string(")");
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

void LLObjectBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model)
	{
		return;
	}
	// *TODO: Translate
	const LLUUID lost_and_found_id = model->findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if (isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else if (model->isObjectDescendentOf(mUUID, lost_and_found_id))
	{
#if RESTORE_TO_WORLD
		items.push_back(std::string("Restore to Last Position"));
#endif
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}
	else
	{
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		LLInventoryItem* item = getItem();
		if (item)
		{
			if (!isAgentAvatarValid())
			{
				return;
			}

			items.push_back(std::string("Attach Separator"));

			if (gAgentAvatarp->isWearingAttachment(mUUID))
			{
				items.push_back(std::string("Detach From Yourself"));
//MK
				if (gRRenabled &&
					!gAgent.mRRInterface.canDetach(gAgentAvatarp->getWornAttachment(mUUID)))
				{
					disabled_items.push_back(std::string("Detach From Yourself"));
				}
//mk
			}
			else
			{
				items.push_back(std::string("Object Wear"));
				items.push_back(std::string("Object Add"));
				if (!gAgentAvatarp->canAttachMoreObjects())
				{
					disabled_items.push_back(std::string("Object Add"));
				}
				items.push_back(std::string("Attach To"));
				items.push_back(std::string("Attach To HUD"));
//MK
				if (gRRenabled && gAgent.mRRInterface.mContainsDetach &&
					(gAgent.mRRInterface.mContainsDefaultwear ||
					 !gSavedSettings.getBOOL("RestrainedLoveAllowWear")) &&
					 !gAgent.mRRInterface.findAttachmentPointFromName(item->getName()) &&
					 !gAgent.mRRInterface.findAttachmentPointFromParentName(item))
				{
					disabled_items.push_back(std::string("Object Wear"));
				}
//mk

				LLMenuGL* attach_menu = menu.getChildMenuByName("Attach To", TRUE);
				LLMenuGL* attach_hud_menu = menu.getChildMenuByName("Attach To HUD", TRUE);
				if (attach_menu && attach_menu->getChildCount() == 0 &&
					attach_hud_menu && attach_hud_menu->getChildCount() == 0)
				{
					for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
						 iter != gAgentAvatarp->mAttachmentPoints.end(); )
					{
						LLVOAvatar::attachment_map_t::iterator curiter = iter++;
						LLViewerJointAttachment* attachment = curiter->second;
						LLMenuItemCallGL *new_item;
						if (attachment->getIsHUDAttachment())
						{
							attach_hud_menu->append(new_item = new LLMenuItemCallGL(attachment->getName(),
												    NULL, //&LLObjectBridge::attachToAvatar,
													NULL, &attach_label, (void*)attachment));
						}
						else
						{
							attach_menu->append(new_item = new LLMenuItemCallGL(attachment->getName(),
												NULL, //&LLObjectBridge::attachToAvatar,
												NULL, &attach_label, (void*)attachment));
						}
						LLSimpleListener* callback = mInventoryPanel->getListenerByName("Inventory.AttachObject");
						if (callback)
						{
							new_item->addListener(callback, "on_click",
												  LLSD(attachment->getName()));
						}
					}
				}
//MK
				if (gRRenabled &&
					!gAgent.mRRInterface.canAttach((LLViewerInventoryItem*)item))
				{
					disabled_items.push_back(std::string("Object Wear"));
					disabled_items.push_back(std::string("Object Add"));
					disabled_items.push_back(std::string("Attach To"));
					disabled_items.push_back(std::string("Attach To HUD"));
				}
//mk
			}
		}
	}
	hideContextEntries(menu, items, disabled_items);
}

BOOL LLObjectBridge::renameItem(const std::string& new_name)
{
	if (!isItemRenameable()) return FALSE;
	LLPreview::rename(mUUID, getPrefix() + new_name);
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model) return FALSE;
	LLViewerInventoryItem* item = getItem();
	if (item && (item->getName() != new_name))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(new_name);
		buildDisplayName(new_item, mDisplayName);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);
		model->notifyObservers();

		if (isAgentAvatarValid())
		{
			LLViewerObject* obj = gAgentAvatarp->getWornAttachment(item->getUUID());
			if (obj)
			{
				LLSelectMgr::getInstance()->deselectAll();
				LLSelectMgr::getInstance()->addAsIndividual(obj, SELECT_ALL_TES, FALSE);
				LLSelectMgr::getInstance()->selectionSetObjectName(new_name);
				LLSelectMgr::getInstance()->deselectAll();
			}
		}
	}
	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}

// +=================================================+
// |        LLLSLTextBridge                          |
// +=================================================+

std::string LLLSLTextBridge::sPrefix("Script: ");

LLUIImagePtr LLLSLTextBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_SCRIPT, LLInventoryType::IT_LSL, 0, FALSE);
}

void LLLSLTextBridge::openItem()
{
//MK
	if (gRRenabled && gAgent.mRRInterface.contains ("viewscript"))
	{
		return;
	}
//mk
	// See if we can bring an exiting preview to the front
	if (!LLPreview::show(mUUID))
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			// There isn't one, so make a new preview
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLRect rect = gSavedSettings.getRect("PreviewScriptRect");
			rect.translate(left - rect.mLeft, top - rect.mTop);

			LLPreviewLSL* preview =	new LLPreviewLSL("preview lsl text",
											 rect,
											 getPrefix() + item->getName(),
											 mUUID);
			preview->setFocus(TRUE);
			// keep onscreen
			gFloaterView->adjustToFitScreen(preview, FALSE);
		}
	}
}

// +=================================================+
// |        LLWearableBridge                         |
// +=================================================+

BOOL LLWearableBridge::renameItem(const std::string& new_name)
{
	if (gAgentWearables.isWearingItem(mUUID))
	{
		gAgentWearables.setWearableName(mUUID, new_name);
	}
	return LLItemBridge::renameItem(new_name);
}

BOOL LLWearableBridge::isItemRemovable()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if (!model)
	{
		return FALSE;
	}
	const LLInventoryObject *obj = model->getItem(mUUID);
	if (obj && obj->getIsLinkType())
	{
		return TRUE;
	}
	if (gAgentWearables.isWearingItem(mUUID))
	{
		return FALSE;
	}
	return LLInvFVBridge::isItemRemovable();
}

LLFontGL::StyleFlags LLWearableBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;

	if (gAgentWearables.isWearingItem(mUUID))
	{
		font |= LLFontGL::BOLD;
	}

	const LLViewerInventoryItem* item = getItem();
	if (item && item->getIsLinkType())
	{
		font |= LLFontGL::ITALIC;
	}

	return (LLFontGL::StyleFlags)font;
}

std::string LLWearableBridge::getLabelSuffix() const
{
	if (gAgentWearables.isWearingItem(mUUID))
	{
		return LLItemBridge::getLabelSuffix() + " (worn)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

LLUIImagePtr LLWearableBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(mAssetType, mInvType, mWearableType, FALSE);
}

// virtual
void LLWearableBridge::performAction(LLFolderView* folder,
									 LLInventoryModel* model,
									 std::string action)
{
	if ("wear" == action)
	{
		wearOnAvatar();
	}
	else if ("wear_add" == action)
	{
		wearOnAvatar(false);
	}
	else if ("edit" == action)
	{
		editOnAvatar();
		return;
	}
	else if ("take_off" == action)
	{
		if (gAgentWearables.isWearingItem(mUUID))
		{
			LLViewerInventoryItem* item = getItem();
			if (item
//MK
				&& !gRRenabled || gAgent.mRRInterface.canUnwear(item))
//mk
			{
				LLWearableList::instance().getAsset(item->getAssetUUID(),
													item->getName(),
													item->getType(),
													LLWearableBridge::onRemoveFromAvatarArrived,
													new OnRemoveStruct(item->getLinkedUUID()));
			}
		}
	}
	else
	{
		LLItemBridge::performAction(folder, model, action);
	}
}

void LLWearableBridge::openItem()
{
	if (isInTrash())
	{
		LLNotifications::instance().add("CannotWearTrash");
	}
	else if (isAgentInventory())
	{
		if (gAgentWearables.isWearingItem(mUUID))
		{
			performAction(NULL, NULL, "take_off");
		}
		else
 		{
			performAction(NULL, NULL, "wear");
		}
	}
	else
	{
		// must be in the inventory library. copy it to our inventory
		// and put it on right away.
		LLViewerInventoryItem* item = getItem();
		if (item && item->isFinished())
		{
			LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback();
			copy_inventory_item(gAgentID,
								item->getPermissions().getOwner(),
								item->getUUID(),
								LLUUID::null,
								std::string(),
								cb);
		}
		else if (item)
		{
			// *TODO: We should fetch the item details, and then do
			// the operation above.
			LLNotifications::instance().add("CannotWearInfoNotComplete");
		}
	}
}

void LLWearableBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	//lldebugs << "LLWearableBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if (isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{	// FWIW, it looks like SUPPRESS_OPEN_ITEM is not set anywhere
		BOOL no_open = ((flags & SUPPRESS_OPEN_ITEM) == SUPPRESS_OPEN_ITEM);

		// If we have clothing, don't add "Open" as it's the same action as
		// "Wear"   SL-18976
		LLViewerInventoryItem* item = getItem();
		if (!no_open && item)
		{
			no_open = (item->getType() == LLAssetType::AT_CLOTHING) ||
					  (item->getType() == LLAssetType::AT_BODYPART);
		}
		if (!no_open)
		{
			items.push_back(std::string("Open"));
		}

		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Wearable Separator"));

		if (gAgentWearables.isWearingItem(mUUID))
		{
			items.push_back(std::string("Wearable Edit"));
			if ((flags & FIRST_SELECTED_ITEM) == 0)
			{
				disabled_items.push_back(std::string("Wearable Edit"));
			}
		}
		else
		{
			items.push_back(std::string("Wearable Wear"));
//MK
			if (gRRenabled && !gAgent.mRRInterface.canWear(item))
			{
				disabled_items.push_back(std::string("Wearable Wear"));
			}
//mk
		}
		if (item && (item->getType() == LLAssetType::AT_CLOTHING))
		{
			if (gAgentWearables.isWearingItem(mUUID))
			{
				items.push_back(std::string("Take Off"));
//MK
				if (gRRenabled && !gAgent.mRRInterface.canUnwear(item))
				{
					disabled_items.push_back(std::string("Take Off"));
				}
//mk
			}
			else
			{
				items.push_back(std::string("Wearable Add"));
//MK
				if (gRRenabled && !gAgent.mRRInterface.canWear(item))
				{
					disabled_items.push_back(std::string("Wearable Add"));
				}
//mk
			}
		}
	}
	hideContextEntries(menu, items, disabled_items);
}

// Called from menus
// static
BOOL LLWearableBridge::canWearOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if (!self) return FALSE;
	if (!self->isAgentInventory())
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)self->getItem();
		if (!item || !item->isFinished()) return FALSE;
	}
	return (!gAgentWearables.isWearingItem(self->mUUID));
}

// Called from menus
// static
void LLWearableBridge::onWearOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if (!self) return;
	self->wearOnAvatar();
}

void LLWearableBridge::wearOnAvatar(bool replace)
{
	// Don't wear anything until initial wearables are loaded, can
	// destroy clothing items.
	if (!gAgentWearables.areWearablesLoaded())
	{
		LLNotifications::instance().add("CanNotChangeAppearanceUntilLoaded");
		return;
	}

	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(item->getLinkedUUID(),
													 replace);
	}
}

// static
void LLWearableBridge::onWearOnAvatarArrived(LLWearable* wearable, void* userdata)
{
	OnWearStruct* on_wear_struct = (OnWearStruct*)userdata;
	LLUUID item_id = on_wear_struct->mUUID;
	bool replace = on_wear_struct->mReplace;

	if (wearable)
	{
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gInventory.getItem(item_id);
		if (item)
		{
			if (item->getAssetUUID() == wearable->getAssetID())
			{
				gAgentWearables.setWearableItem(item, wearable, !replace);
				gInventory.notifyObservers();
				//self->getFolderItem()->refreshFromRoot();
			}
			else
			{
				llinfos << "By the time wearable asset arrived, its inv item already pointed to a different asset."
						<< llendl;
			}
		}
	}
	delete on_wear_struct;
}

// static
BOOL LLWearableBridge::canEditOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if (!self) return FALSE;

	return (gAgentWearables.isWearingItem(self->mUUID));
}

// static
void LLWearableBridge::onEditOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if (self)
	{
		self->editOnAvatar();
	}
}

// *TODO: implement v3's way and allow wear & edit
void LLWearableBridge::editOnAvatar()
{
	LLUUID linked_id = gInventory.getLinkedItemID(mUUID);
	LLWearable* wearable = gAgentWearables.getWearableFromItemID(linked_id);
	if (wearable)
	{
		// Set the tab to the right wearable.
		LLFloaterCustomize::setCurrentWearableType(wearable->getType());

		if (CAMERA_MODE_CUSTOMIZE_AVATAR != gAgent.getCameraMode())
		{
			// Start Avatar Customization
			gAgent.changeCameraToCustomizeAvatar();
		}
	}
}

// static
BOOL LLWearableBridge::canRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if (self && LLAssetType::AT_BODYPART != self->mAssetType)
	{
		return gAgentWearables.isWearingItem(self->mUUID);
	}
	return FALSE;
}

// static
void LLWearableBridge::onRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if (!self) return;
	if (gAgentWearables.isWearingItem(self->mUUID))
	{
		LLViewerInventoryItem* item = self->getItem();
		if (item)
		{
			LLWearableList::instance().getAsset(item->getAssetUUID(),
												item->getName(),
												item->getType(),
												onRemoveFromAvatarArrived,
												new OnRemoveStruct(LLUUID(self->mUUID)));
		}
	}
}

bool get_is_item_worn(const LLUUID& id)
{
	const LLViewerInventoryItem* item = gInventory.getItem(id);
	if (!item)
	{
		return false;
	}

	switch (item->getType())
	{
		case LLAssetType::AT_OBJECT:
		{
			if (isAgentAvatarValid() &&
				gAgentAvatarp->isWearingAttachment(item->getLinkedUUID()))
			{
				return true;
			}
			break;
		}
		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
		{
			if (gAgentWearables.isWearingItem(item->getLinkedUUID()))
			{
				return true;
			}
			break;
		}
		case LLAssetType::AT_GESTURE:
		{
			if (gGestureManager.isGestureActive(item->getLinkedUUID()))
			{
				return true;
			}
			break;
		}
		default:
			break;
	}
	return false;
}

// static
void LLWearableBridge::onRemoveFromAvatarArrived(LLWearable* wearable,
												 void* userdata)
{
	OnRemoveStruct *on_remove_struct = (OnRemoveStruct*) userdata;
	const LLUUID& item_id = gInventory.getLinkedItemID(on_remove_struct->mUUID);
	if (wearable)
	{
		if (get_is_item_worn(item_id))
		{
			LLWearableType::EType type = wearable->getType();
			U32 index = gAgentWearables.getWearableIndex(wearable);
			gAgentWearables.userRemoveWearable(type, index);
		}
	}

	gInventory.notifyObservers();

	delete on_remove_struct;
}

// +=================================================+
// |        LLLinkItemBridge                         |
// +=================================================+
// For broken links

std::string LLLinkItemBridge::sPrefix("Link: ");

LLUIImagePtr LLLinkItemBridge::getIcon() const
{
	if (LLViewerInventoryItem *item = getItem())
	{
		U32 attachment_point = (item->getFlags() & 0xff); // low byte of inventory flags
		bool is_multi =  LLInventoryItem::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS & item->getFlags();

		return LLInventoryIcon::getIcon(item->getActualType(),
										item->getInventoryType(),
										attachment_point, is_multi);
	}
	return LLInventoryIcon::getIcon(LLAssetType::AT_LINK,
									LLInventoryType::IT_NONE, 0, FALSE);
}

void LLLinkItemBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	//lldebugs << "LLLink::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	items.push_back(std::string("Find Original"));
	disabled_items.push_back(std::string("Find Original"));

	if (isInTrash())
	{
		disabled_items.push_back(std::string("Find Original"));
		if (isLinkedObjectMissing())
		{
			disabled_items.push_back(std::string("Find Original"));
		}
		items.push_back(std::string("Purge Item"));
		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Properties"));
		items.push_back(std::string("Find Original"));
		if (isLinkedObjectMissing())
		{
			disabled_items.push_back(std::string("Find Original"));
		}
		items.push_back(std::string("Delete"));
	}
	hideContextEntries(menu, items, disabled_items);
}

// +=================================================+
// |        LLMeshBridge                             |
// +=================================================+

std::string LLMeshBridge::sPrefix("Mesh: ");

LLUIImagePtr LLMeshBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_MESH, LLInventoryType::IT_MESH,
									0, FALSE);
}

void LLMeshBridge::openItem()
{
	// open mesh
}

void LLMeshBridge::previewItem()
{
	// preview mesh
}

void LLMeshBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	//lldebugs << "LLMeshBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if (isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	hideContextEntries(menu, items, disabled_items);
}

// virtual
void LLMeshBridge::performAction(LLFolderView* folder, LLInventoryModel* model,
								 std::string action)
{
	// do action

	LLItemBridge::performAction(folder, model, action);
}

// +=================================================+
// |        LLLinkBridge                             |
// +=================================================+
// For broken links.

std::string LLLinkFolderBridge::sPrefix("Link: ");

LLUIImagePtr LLLinkFolderBridge::getIcon() const
{
	return LLUI::getUIImage("inv_link_folder.tga");
}

void LLLinkFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	//lldebugs << "LLLink::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if (isInTrash())
	{
		items.push_back(std::string("Find Original"));
		if (isLinkedObjectMissing())
		{
			disabled_items.push_back(std::string("Find Original"));
		}
		items.push_back(std::string("Purge Item"));
		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Find Original"));
		if (isLinkedObjectMissing())
		{
			disabled_items.push_back(std::string("Find Original"));
		}
		items.push_back(std::string("Delete"));
	}
	hideContextEntries(menu, items, disabled_items);
}

void LLLinkFolderBridge::performAction(LLFolderView* folder,
									   LLInventoryModel* model,
									   std::string action)
{
	if ("goto" == action)
	{
		gotoItem(folder);
		return;
	}
	LLItemBridge::performAction(folder,model,action);
}

void LLLinkFolderBridge::gotoItem(LLFolderView *folder)
{
	const LLUUID &cat_uuid = getFolderID();
	if (!cat_uuid.isNull())
	{
		if (LLFolderViewItem *base_folder = folder->getItemByID(cat_uuid))
		{
			if (LLInventoryModel* model = mInventoryPanel->getModel())
			{
				model->fetchDescendentsOf(cat_uuid);
			}
			base_folder->setOpen(TRUE);
			folder->setSelectionFromRoot(base_folder,TRUE);
			folder->scrollToShowSelection();
		}
	}
}

const LLUUID &LLLinkFolderBridge::getFolderID() const
{
	if (LLViewerInventoryItem *link_item = getItem())
	{
		if (const LLViewerInventoryCategory *cat = link_item->getLinkedCategory())
		{
			const LLUUID& cat_uuid = cat->getUUID();
			return cat_uuid;
		}
	}
	return LLUUID::null;
}
