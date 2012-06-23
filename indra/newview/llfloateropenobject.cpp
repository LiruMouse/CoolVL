/** 
 * @file llfloateropenobject.cpp
 * @brief LLFloaterOpenObject class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

/*
 * Shows the contents of an object.
 * A floater wrapper for llpanelinventory
 */

#include "llviewerprecompiledheaders.h"

#include "llfloateropenobject.h"

#include "llnotifications.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llpanelinventory.h"
#include "llselectmgr.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"

LLFloaterOpenObject* LLFloaterOpenObject::sInstance = NULL;

LLFloaterOpenObject::LLFloaterOpenObject()
:	LLFloater(std::string("object_contents")),
	mPanelInventory(NULL),
	mDirty(TRUE)
{
	LLCallbackMap::map_t factory_map;
	factory_map["object_contents"] = LLCallbackMap(createPanelInventory, this);
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_openobject.xml",
												 &factory_map);
}

BOOL LLFloaterOpenObject::postBuild()
{
	childSetAction("copy_to_inventory_button", onClickMoveToInventory, this);
	if (gSavedSettings.getBOOL("EnableCopyAndWear"))
	{
		childSetAction("copy_and_wear_button", onClickMoveAndWear, this);
	}
	else
	{
		childSetVisible("copy_and_wear_button", false);
	}
	// *Note: probably do not want to translate this
	childSetTextArg("object_name", "[DESC]", std::string("Object"));

	return TRUE;
}

LLFloaterOpenObject::~LLFloaterOpenObject()
{
	sInstance = NULL;
}

void LLFloaterOpenObject::refresh()
{
	mPanelInventory->refresh();

	LLSelectNode* node = mObjectSelection->getFirstRootNode();
	if (node)
	{
		std::string name = node->mName;
		childSetTextArg("object_name", "[DESC]", name);
	}
}

void LLFloaterOpenObject::draw()
{
	if (mDirty)
	{
		refresh();
		mDirty = FALSE;
	}
	LLFloater::draw();
}

// static
void LLFloaterOpenObject::dirty()
{
	if (sInstance)
	{
		sInstance->mDirty = TRUE;
	}
}

// static
void LLFloaterOpenObject::show()
{
	LLObjectSelectionHandle object_selection = LLSelectMgr::getInstance()->getSelection();
	if (object_selection->getRootObjectCount() != 1)
	{
		LLNotifications::instance().add("UnableToViewContentsMoreThanOne");
		return;
	}
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsEdit)
	{
		return;
	}
//mk
	// Create a new instance only if needed
	if (!sInstance)
	{
		sInstance = new LLFloaterOpenObject();
		sInstance->center();
	}

	sInstance->open();		/* Flawfinder: ignore */
	sInstance->setFocus(TRUE);

	sInstance->mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
}


void LLFloaterOpenObject::moveToInventory(bool wear)
{
	if (mObjectSelection->getRootObjectCount() != 1)
	{
		LLNotifications::instance().add("OnlyCopyContentsOfSingleItem");
		return;
	}

	LLSelectNode* node = mObjectSelection->getFirstRootNode();
	if (!node) return;
	LLViewerObject* object = node->getObject();
	if (!object) return;

	LLUUID object_id = object->getID();
	std::string name = node->mName;

	// Either create a sub-folder of clothing, or of the root folder.
	LLUUID parent_category_id;
	if (wear)
	{
		parent_category_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
	}
	else
	{
		parent_category_id = gInventory.getRootFolderID();
	}

	LLCategoryCreate* cat_data = new LLCategoryCreate(object_id, wear);
	LLUUID category_id = gInventory.createNewCategory(parent_category_id,
													  LLFolderType::FT_NONE,
													  name,
													  callbackCreateInventoryCategory,
													  (void*)cat_data);

	// If we get a null category ID, we are using a capability in
	// createNewCategory and we will handle the following in the
	// callbackCreateInventoryCategory routine.
	if (category_id.notNull())
	{
		delete cat_data;

		LLCatAndWear* data = new LLCatAndWear;
		data->mCatID = category_id;
		data->mWear = wear;
		data->mFolderResponded = false;

		// Copy and/or move the items into the newly created folder.
		// Ignore any "you're going to break this item" messages.
		BOOL success = move_inv_category_world_to_agent(object_id, category_id, TRUE,
														callbackMoveInventory, 
														(void*)data);
		if (!success)
		{
			delete data;
			data = NULL;

			LLNotifications::instance().add("OpenObjectCannotCopy");
		}
	}
}

// static
void LLFloaterOpenObject::callbackCreateInventoryCategory(const LLSD& result, void* data)
{
	LLCategoryCreate* cat_data = (LLCategoryCreate*)data;

	LLUUID category_id = result["folder_id"].asUUID();
	LLCatAndWear* wear_data = new LLCatAndWear;

	wear_data->mCatID = category_id;
	wear_data->mWear = cat_data->mWear;
	wear_data->mFolderResponded = true;

 	// Copy and/or move the items into the newly created folder.
 	// Ignore any "you're going to break this item" messages.

	BOOL success = move_inv_category_world_to_agent(cat_data->mObjectID, category_id, TRUE,
 													callbackMoveInventory, 
													(void*)wear_data);
 	if (!success)
 	{
		delete wear_data;
		wear_data = NULL;

 		LLNotifications::instance().add("OpenObjectCannotCopy");
 	}
	delete cat_data;	
 }

// static
void LLFloaterOpenObject::callbackMoveInventory(S32 result, void* data)
{
	LLCatAndWear* cat = (LLCatAndWear*)data;

	if (result == 0)
	{
		LLInventoryView::showAgentInventory();
		LLInventoryView* view = LLInventoryView::getActiveInventory();
		if (view)
		{
			view->getPanel()->setSelection(cat->mCatID, TAKE_FOCUS_NO);
		}
	}

	delete cat;
}

// static
void LLFloaterOpenObject::onClickMoveToInventory(void* data)
{
	LLFloaterOpenObject* self = (LLFloaterOpenObject*)data;
	self->moveToInventory(false);
	self->close();
}

// static
void LLFloaterOpenObject::onClickMoveAndWear(void* data)
{
	LLFloaterOpenObject* self = (LLFloaterOpenObject*)data;
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsDetach)
	{
		self->moveToInventory(false);
	} else {
		self->moveToInventory(true);
	}
//mk
	self->close();
}

//static
void* LLFloaterOpenObject::createPanelInventory(void* data)
{
	LLFloaterOpenObject* floater = (LLFloaterOpenObject*)data;
	floater->mPanelInventory = new LLPanelInventory("Object Contents", LLRect());
	return floater->mPanelInventory;
}
