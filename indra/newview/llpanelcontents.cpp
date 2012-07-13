/**
 * @file llpanelcontents.cpp
 * @brief Object contents panel in the tools floater.
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

#include "llpanelcontents.h"

#include "llbutton.h"
#include "llpermissionsflags.h"

#include "llagent.h"
#include "llfloaterbulkpermission.h"
#include "llpanelinventory.h"
#include "llselectmgr.h"
#include "llviewerassettype.h"
#include "llviewerobject.h"

LLPanelContents::LLPanelContents(const std::string& name)
:	LLPanel(name),
	mPanelInventory(NULL)
{
}

//virtual
LLPanelContents::~LLPanelContents()
{
	// Children all cleaned up by default view destructor.
}

//virtual
BOOL LLPanelContents::postBuild()
{
	setMouseOpaque(FALSE);

	mButtonNewScript = getChild<LLButton>("button new script");
	mButtonNewScript->setClickedCallback(onClickNewScript, this);

	childSetAction("button permissions", onClickPermissions, this);

	return TRUE;
}

void LLPanelContents::getState(LLViewerObject* objectp)
{
	if (!objectp)
	{
		mButtonNewScript->setEnabled(FALSE);
		return;
	}

	LLSelectMgr* selectmgr = LLSelectMgr::getInstance();
	LLUUID group_id;
	// sets group_id as a side effect SL-23488
	selectmgr->selectGetGroup(group_id);

	// BUG ? Check for all objects being editable ?
	BOOL editable = gAgent.isGodlike() ||
					(objectp->permModify() &&
					 // solves SL-23488
					 (objectp->permYouOwner() ||
					  (!group_id.isNull() && gAgent.isInGroup(group_id))));
	BOOL all_volume = selectmgr->selectionAllPCode(LL_PCODE_VOLUME);

	// Edit script button - ok if object is editable and there's an unambiguous
	// destination for the object.
	BOOL enabled = editable && all_volume &&
				   (selectmgr->getSelection()->getRootObjectCount() == 1 ||
					selectmgr->getSelection()->getObjectCount() == 1);
	mButtonNewScript->setEnabled(enabled);
}

//virtual
void LLPanelContents::refresh()
{
	LLViewerObject* object;
	object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(TRUE);
	if (object)
	{
		getState(object);
		if (mPanelInventory)
		{
			mPanelInventory->refresh();
		}
	}
}

//static
void LLPanelContents::onClickNewScript(void *userdata)
{
	LLViewerObject* object;
	object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(TRUE);
	if (object)
	{
//MK
		if (gRRenabled)
		{
			// can't edit objects that someone is sitting on, when prevented
			// from sit-tping
			if (object->isSeat() &&
				(gAgent.mRRInterface.mContainsUnsit) ||
				 gAgent.mRRInterface.contains("sittp"))
			{
				return;
			}

			if (!gAgent.mRRInterface.canDetach(object))
			{
				return;
			}
		}
//mk
		LLPermissions perm;
		perm.init(gAgentID, gAgentID, LLUUID::null, LLUUID::null);
		perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE,
					   PERM_MOVE | PERM_TRANSFER);
		std::string desc;
		LLViewerAssetType::generateDescriptionFor(LLAssetType::AT_LSL_TEXT, desc);
		LLPointer<LLViewerInventoryItem> new_item;
		new_item = new LLViewerInventoryItem(LLUUID::null, LLUUID::null,
											 perm, LLUUID::null,
											 LLAssetType::AT_LSL_TEXT,
											 LLInventoryType::IT_LSL,
											 std::string("New Script"),
											 desc, LLSaleInfo::DEFAULT,
											 LLViewerInventoryItem::II_FLAGS_NONE,
											 time_corrected());
		object->saveScript(new_item, TRUE, true);
	}
}

//static
void LLPanelContents::onClickPermissions(void *userdata)
{
	LLPanelContents* self = (LLPanelContents*)userdata;
	if (self)
	{
		gFloaterView->getParentFloater(self)->addDependentFloater(LLFloaterBulkPermission::showInstance());
	}
}
