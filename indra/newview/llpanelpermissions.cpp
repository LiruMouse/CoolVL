/**
 * @file llpanelpermissions.cpp
 * @brief LLPanelPermissions class implementation
 * This class represents the panel in the build view for
 * viewing/editing object names, owners, permissions, etc.
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

#include "llpanelpermissions.h"

#include "llbutton.h"
#include "llcategory.h"
#include "llcheckboxctrl.h"
#include "llclickaction.h"
#include "llcombobox.h"
#include "lldbstrings.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "llpermissions.h"
#include "llradiogroup.h"
#include "llstring.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "lluuid.h"
#include "roles_constants.h"

#include "llagent.h"
#include "llselectmgr.h"
#include "llstatusbar.h"			// for getBalance()
#include "llfloateravatarinfo.h"
#include "llfloatergroupinfo.h"
#include "llfloatergroups.h"
#include "llnamebox.h"
#include "lluiconstants.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"

///----------------------------------------------------------------------------
/// Class llpanelpermissions
///----------------------------------------------------------------------------

// Default constructor
LLPanelPermissions::LLPanelPermissions(const std::string& title) :
	LLPanel(title)
{
	setMouseOpaque(FALSE);
}

//virtual
LLPanelPermissions::~LLPanelPermissions()
{
	// base class will take care of everything
}

//virtual
BOOL LLPanelPermissions::postBuild()
{
	// Object name

	mTextObjectName = getChild<LLTextBox>("Name:");

	mEditorObjectName = getChild<LLLineEditor>("Object Name");
	mEditorObjectName->setCommitCallback(onCommitName);
	mEditorObjectName->setCallbackUserData(this);
	mEditorObjectName->setPrevalidate(LLLineEditor::prevalidatePrintableNotPipe);

	// Object description

	mTextObjectDesc = getChild<LLTextBox>("Description:");

	mEditorObjectDesc = getChild<LLLineEditor>("Object Description");
	mEditorObjectDesc->setCommitCallback(onCommitDesc);
	mEditorObjectDesc->setCallbackUserData(this);
	mEditorObjectDesc->setPrevalidate(LLLineEditor::prevalidatePrintableNotPipe);

	// Object creator

	mTextCreatorLabel = getChild<LLTextBox>("Creator:");
	mTextCreatorName = getChild<LLTextBox>("Creator Name");

	mButtonCreatorProfile = getChild<LLButton>("button creator profile");
	mButtonCreatorProfile->setClickedCallback(onClickCreator, this);

	// Object owner

	mTextOwnerLabel = getChild<LLTextBox>("Owner:");
	mTextOwnerName = getChild<LLTextBox>("Owner Name");

	mButtonOwnerProfile = getChild<LLButton>("button owner profile");
	mButtonOwnerProfile->setClickedCallback(onClickOwner, this);

	// Object group

	mTextGroupName = getChild<LLTextBox>("Group:");

	LLTextBox* group_name = getChild<LLTextBox>("Group Name Proxy");
	mNameBoxGroupName = new LLNameBox("Group Name", group_name->getRect());
	addChild(mNameBoxGroupName);

	mButtonSetGroup = getChild<LLButton>("button set group");
	mButtonSetGroup->setClickedCallback(onClickGroup, this);

	// Permissions

	mTextPermissions = getChild<LLTextBox>("Permissions:");
	mTextPermissionsModify = getChild<LLTextBox>("perm_modify");

	mCheckShareWithGroup = getChild<LLCheckBoxCtrl>("checkbox share with group");
	mCheckShareWithGroup->setCommitCallback(onCommitGroupShare);
	mCheckShareWithGroup->setCallbackUserData(this);

	mButtonDeed = getChild<LLButton>("button deed");
	mButtonDeed->setClickedCallback(onClickDeedToGroup, this);

	mCheckAllowEveryoneMove = getChild<LLCheckBoxCtrl>("checkbox allow everyone move");
	mCheckAllowEveryoneMove->setCommitCallback(onCommitEveryoneMove);
	mCheckAllowEveryoneMove->setCallbackUserData(this);

	mCheckAllowEveryoneCopy = getChild<LLCheckBoxCtrl>("checkbox allow everyone copy");
	mCheckAllowEveryoneCopy->setCommitCallback(onCommitEveryoneCopy);
	mCheckAllowEveryoneCopy->setCallbackUserData(this);

	mCheckShowInSearch = getChild<LLCheckBoxCtrl>("search_check");
	mCheckShowInSearch->setCommitCallback(onCommitIncludeInSearch);
	mCheckShowInSearch->setCallbackUserData(this);

	mCheckForSale = getChild<LLCheckBoxCtrl>("checkbox for sale");
	mCheckForSale->setCommitCallback(onCommitSaleInfo);
	mCheckForSale->setCallbackUserData(this);

	mTextCost = getChild<LLTextBox>("Cost");

	mEditorCost = getChild<LLLineEditor>("Edit Cost");
	mEditorCost->setCommitCallback(onCommitSaleInfo);
	mEditorCost->setCallbackUserData(this);
	mEditorCost->setPrevalidate(LLLineEditor::prevalidateNonNegativeS32);

	mRadioSaleType = getChild<LLRadioGroup>("sale type");
	mRadioSaleType->setCommitCallback(onCommitSaleType);
	mRadioSaleType->setCallbackUserData(this);

	mTextNextOwnerCan = getChild<LLTextBox>("Next owner can:");

	mCheckNextCanModify = getChild<LLCheckBoxCtrl>("checkbox next owner can modify");
	mCheckNextCanModify->setCommitCallback(onCommitNextOwnerModify);
	mCheckNextCanModify->setCallbackUserData(this);

	mCheckNextCanCopy = getChild<LLCheckBoxCtrl>("checkbox next owner can copy");
	mCheckNextCanCopy->setCommitCallback(onCommitNextOwnerCopy);
	mCheckNextCanCopy->setCallbackUserData(this);

	mCheckNextCanTransfer = getChild<LLCheckBoxCtrl>("checkbox next owner can transfer");
	mCheckNextCanTransfer->setCommitCallback(onCommitNextOwnerTransfer);
	mCheckNextCanTransfer->setCallbackUserData(this);

	mTextClickAction = getChild<LLTextBox>("label click action");

	mComboClickAction = getChild<LLComboBox>("clickaction");
	mComboClickAction->setCommitCallback(onCommitClickAction);
	mComboClickAction->setCallbackUserData(this);

	mTextDebugPermB = getChild<LLTextBox>("B:");
	mTextDebugPermO = getChild<LLTextBox>("O:");
	mTextDebugPermG = getChild<LLTextBox>("G:");
	mTextDebugPermE = getChild<LLTextBox>("E:");
	mTextDebugPermN = getChild<LLTextBox>("N:");
	mTextDebugPermF = getChild<LLTextBox>("F:");

	mCostTotal   = getString("Cost Total");
	mCostDefault = getString("Cost Default");
	mCostPerUnit = getString("Cost Per Unit");
	mCostMixed   = getString("Cost Mixed");
	mSaleMixed   = getString("Sale Mixed");

	return TRUE;
}

//virtual
void LLPanelPermissions::refresh()
{
	// Static variables, to prevent calling getString() on each refresh()
	static std::string MODIFY_INFO_STRINGS[] =
	{
		getString("text modify info 1"),
		getString("text modify info 2"),
		getString("text modify info 3"),
		getString("text modify info 4"),
		getString("text modify warning")
	};

	static std::string text_deed = getString("text deed");
	static std::string text_deed_continued = getString("text deed continued");

	static LLCachedControl<bool> warn_deed_object(gSavedSettings,
												  "WarnDeedObject");
	std::string deed_text = warn_deed_object ? text_deed_continued : text_deed;
	mButtonDeed->setLabelSelected(deed_text);
	mButtonDeed->setLabelUnselected(deed_text);

	BOOL root_selected = TRUE;
	LLSelectMgr* selectmgr = LLSelectMgr::getInstance();
	LLSelectNode* nodep = selectmgr->getSelection()->getFirstRootNode();
	S32 object_count = selectmgr->getSelection()->getRootObjectCount();
	if (!nodep || 0 == object_count)
	{
		nodep = selectmgr->getSelection()->getFirstNode();
		object_count = selectmgr->getSelection()->getObjectCount();
		root_selected = FALSE;
	}

	//BOOL attachment_selected = selectmgr->getSelection()->isAttachment();
	//attachment_selected = false;
	LLViewerObject* objectp = NULL;
	if (nodep) objectp = nodep->getObject();
	if (!nodep || !objectp)// || attachment_selected)
	{
		// ...nothing selected
		mTextObjectName->setEnabled(FALSE);
		mEditorObjectName->setText(LLStringUtil::null);
		mEditorObjectName->setEnabled(FALSE);

		mTextObjectDesc->setEnabled(FALSE);
		mEditorObjectDesc->setText(LLStringUtil::null);
		mEditorObjectDesc->setEnabled(FALSE);

		mTextCreatorLabel->setEnabled(FALSE);
		mTextCreatorName->setText(LLStringUtil::null);
		mTextCreatorName->setEnabled(FALSE);
		mButtonCreatorProfile->setEnabled(FALSE);

		mTextOwnerLabel->setEnabled(FALSE);
		mTextOwnerName->setText(LLStringUtil::null);
		mTextOwnerName->setEnabled(FALSE);
		mButtonOwnerProfile->setEnabled(FALSE);

		mTextGroupName->setEnabled(FALSE);
		mNameBoxGroupName->setText(LLStringUtil::null);
		mNameBoxGroupName->setEnabled(FALSE);
		mButtonSetGroup->setEnabled(FALSE);

		mTextPermissions->setEnabled(FALSE);

		mTextPermissionsModify->setEnabled(FALSE);
		mTextPermissionsModify->setText(LLStringUtil::null);

		mCheckShareWithGroup->setValue(FALSE);
		mCheckShareWithGroup->setEnabled(FALSE);
		mButtonDeed->setEnabled(FALSE);

		mCheckAllowEveryoneMove->setValue(FALSE);
		mCheckAllowEveryoneMove->setEnabled(FALSE);
		mCheckAllowEveryoneCopy->setValue(FALSE);
		mCheckAllowEveryoneCopy->setEnabled(FALSE);

		// Next owner can:
		mTextNextOwnerCan->setEnabled(FALSE);
		mCheckNextCanModify->setValue(FALSE);
		mCheckNextCanModify->setEnabled(FALSE);
		mCheckNextCanCopy->setValue(FALSE);
		mCheckNextCanCopy->setEnabled(FALSE);
		mCheckNextCanTransfer->setValue(FALSE);
		mCheckNextCanTransfer->setEnabled(FALSE);

		// checkbox include in search
		mCheckShowInSearch->setValue(FALSE);
		mCheckShowInSearch->setEnabled(FALSE);

		// checkbox for sale
		mCheckForSale->setValue(FALSE);
		mCheckForSale->setEnabled(FALSE);

		mRadioSaleType->setSelectedIndex(-1);
		mRadioSaleType->setEnabled(FALSE);

		mTextCost->setText(mCostDefault);
		mTextCost->setEnabled(FALSE);
		mEditorCost->setText(LLStringUtil::null);
		mEditorCost->setEnabled(FALSE);

		mTextClickAction->setEnabled(FALSE);
		mComboClickAction->setEnabled(FALSE);
		mComboClickAction->clear();
		mTextDebugPermB->setVisible(FALSE);
		mTextDebugPermO->setVisible(FALSE);
		mTextDebugPermG->setVisible(FALSE);
		mTextDebugPermE->setVisible(FALSE);
		mTextDebugPermN->setVisible(FALSE);
		mTextDebugPermF->setVisible(FALSE);

		return;
	}

	mTextPermissions->setEnabled(TRUE);

	// figure out a few variables
	BOOL is_one_object = (object_count == 1);

	// BUG: fails if a root and non-root are both single-selected.
	BOOL is_perm_modify = selectmgr->selectGetModify() ||
						  (selectmgr->getSelection()->getFirstRootNode() &&
						   selectmgr->selectGetRootsModify());

	S32 string_index = is_perm_modify ? 0 : 2;
	if (!is_one_object)
	{
		++string_index;
	}
	mTextPermissionsModify->setEnabled(TRUE);
	mTextPermissionsModify->setText(MODIFY_INFO_STRINGS[string_index]);

	// Update creator text field
	mTextCreatorLabel->setEnabled(TRUE);
	BOOL creators_identical;
	std::string creator_name;
	creators_identical = selectmgr->selectGetCreator(mCreatorID, creator_name);

	mTextCreatorName->setText(creator_name);
	mTextCreatorName->setEnabled(TRUE);
	mButtonCreatorProfile->setEnabled(creators_identical && mCreatorID.notNull());

	// Update owner text field
	mTextOwnerLabel->setEnabled(TRUE);

	BOOL owners_identical;
	std::string owner_name;
	owners_identical = selectmgr->selectGetOwner(mOwnerID, owner_name);

	if (mOwnerID.isNull())
	{
		if (selectmgr->selectIsGroupOwned())
		{
			// Group owned already displayed by selectGetOwner
		}
		else
		{
			// Display last owner if public
			std::string last_owner_name;
			selectmgr->selectGetLastOwner(mLastOwnerID, last_owner_name);

			// It should never happen that the last owner is null and the owner
			// is null, but it seems to be a bug in the simulator right now. JC
			if (!mLastOwnerID.isNull() && !last_owner_name.empty())
			{
				owner_name.append(", last ");
				owner_name.append(last_owner_name);
			}
		}
	}

//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
	{
		owner_name = gAgent.mRRInterface.getDummyName(owner_name);
	}
//mk
	mTextOwnerName->setText(owner_name);
	mTextOwnerName->setEnabled(TRUE);
	mButtonOwnerProfile->setEnabled(owners_identical &&
									(mOwnerID.notNull() ||
									 selectmgr->selectIsGroupOwned()));

	// update group text field
	mTextGroupName->setEnabled(TRUE);
	mNameBoxGroupName->setText(LLStringUtil::null);
	LLUUID group_id;
	BOOL groups_identical = selectmgr->selectGetGroup(group_id);
	if (groups_identical)
	{
		mNameBoxGroupName->setNameID(group_id, true);
		mNameBoxGroupName->setEnabled(TRUE);
	}
	else
	{
		mNameBoxGroupName->setNameID(LLUUID::null, true);
		mNameBoxGroupName->refresh(LLUUID::null, LLStringUtil::null, true);
		mNameBoxGroupName->setEnabled(FALSE);
	}

	mButtonSetGroup->setEnabled(owners_identical && mOwnerID == gAgentID);

	// figure out the contents of the name, description, & category
	mTextObjectName->setEnabled(TRUE);
	mTextObjectDesc->setEnabled(TRUE);
	BOOL edit_name_desc = FALSE;
	if (is_one_object && objectp->permModify())
	{
		edit_name_desc = TRUE;
	}

	const LLFocusableElement* keyboard_focus_view = gFocusMgr.getKeyboardFocus();
	if (is_one_object)
	{
		if (keyboard_focus_view != mEditorObjectName)
		{
			mEditorObjectName->setText(nodep->mName);
		}

		if (keyboard_focus_view != mEditorObjectDesc)
		{
			mEditorObjectDesc->setText(nodep->mDescription);
		}
	}
	else
	{
		mEditorObjectName->setText(LLStringUtil::null);
		mEditorObjectDesc->setText(LLStringUtil::null);
	}

	mEditorObjectName->setEnabled(edit_name_desc);
	mEditorObjectDesc->setEnabled(edit_name_desc);

	S32 total_sale_price = 0;
	S32 individual_sale_price = 0;
	BOOL is_for_sale_mixed = FALSE;
	BOOL is_sale_price_mixed = FALSE;
	U32 num_for_sale = FALSE;
    selectmgr->selectGetAggregateSaleInfo(num_for_sale,
										   is_for_sale_mixed,
										   is_sale_price_mixed,
										   total_sale_price,
										   individual_sale_price);

	const BOOL self_owned = (gAgentID == mOwnerID);
	const BOOL group_owned = selectmgr->selectIsGroupOwned();
	const BOOL public_owned = (mOwnerID.isNull() && !selectmgr->selectIsGroupOwned());
	const BOOL can_transfer = selectmgr->selectGetRootsTransfer();
	const BOOL can_copy = selectmgr->selectGetRootsCopy();

	if (!owners_identical)
	{
		mTextCost->setEnabled(FALSE);
		mEditorCost->setText(LLStringUtil::null);
		mEditorCost->setEnabled(FALSE);
	}
	// You own these objects.
	else if (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id,GP_OBJECT_SET_SALE)))
	{
		// If there are multiple items for sale then set text to PRICE PER UNIT.
		if (num_for_sale > 1)
		{
			mTextCost->setText(mCostPerUnit);
		}
		else
		{
			mTextCost->setText(mCostDefault);
		}

		if (keyboard_focus_view != mEditorCost)
		{
			// If the sale price is mixed then set the cost to MIXED, otherwise
			// set to the actual cost.
			if (num_for_sale > 0 && is_for_sale_mixed)
			{
				mEditorCost->setText(mSaleMixed);
			}
			else if (num_for_sale > 0 && is_sale_price_mixed)
			{
				mEditorCost->setText(mCostMixed);
			}
			else
			{
				mEditorCost->setText(llformat("%d", individual_sale_price));
			}
		}
		// The edit fields are only enabled if you can sell this object
		// and the sale price is not mixed.
		BOOL enable_edit = (num_for_sale && can_transfer) ? !is_for_sale_mixed : false;
		mTextCost->setEnabled(enable_edit);
		mEditorCost->setEnabled(enable_edit);
	}
	// Someone, not you, owns these objects.
	else if (!public_owned)
	{
		mTextCost->setEnabled(FALSE);
		mEditorCost->setEnabled(FALSE);

		// Don't show a price if none of the items are for sale.
		if (num_for_sale)
		{
			mEditorCost->setText(llformat("%d", total_sale_price));
		}
		else
		{
			mEditorCost->setText(LLStringUtil::null);
		}

		// If multiple items are for sale, set text to TOTAL PRICE.
		if (num_for_sale > 1)
		{
			mTextCost->setText(mCostTotal);
		}
		else
		{
			mTextCost->setText(mCostDefault);
		}
	}
	// This is a public object.
	else
	{
		mTextCost->setText(mCostDefault);
		mTextCost->setEnabled(FALSE);

		mEditorCost->setText(LLStringUtil::null);
		mEditorCost->setEnabled(FALSE);
	}

	// Enable and disable the permissions checkboxes
	// based on who owns the object.
	// TODO: Creator permissions

	BOOL valid_base_perms		= FALSE;
	BOOL valid_owner_perms		= FALSE;
	BOOL valid_group_perms		= FALSE;
	BOOL valid_everyone_perms	= FALSE;
	BOOL valid_next_perms		= FALSE;

	U32 base_mask_on;
	U32 base_mask_off;
	U32 owner_mask_on;
	U32 owner_mask_off;
	U32 group_mask_on;
	U32 group_mask_off;
	U32 everyone_mask_on;
	U32 everyone_mask_off;
	U32 next_owner_mask_on = 0;
	U32 next_owner_mask_off = 0;

	valid_base_perms = selectmgr->selectGetPerm(PERM_BASE,
									  &base_mask_on,
									  &base_mask_off);

	valid_owner_perms = selectmgr->selectGetPerm(PERM_OWNER,
									  &owner_mask_on,
									  &owner_mask_off);

	valid_group_perms = selectmgr->selectGetPerm(PERM_GROUP,
									  &group_mask_on,
									  &group_mask_off);

	valid_everyone_perms = selectmgr->selectGetPerm(PERM_EVERYONE,
									  &everyone_mask_on,
									  &everyone_mask_off);

	valid_next_perms = selectmgr->selectGetPerm(PERM_NEXT_OWNER,
									  &next_owner_mask_on,
									  &next_owner_mask_off);

	static LLCachedControl<bool> debug_permissions(gSavedSettings,
												   "DebugPermissions");
	if (debug_permissions)
	{
		std::string perm_string;
		if (valid_base_perms)
		{
			perm_string = "B: ";
			perm_string += mask_to_string(base_mask_on);
			mTextDebugPermB->setText(perm_string);
			mTextDebugPermB->setVisible(TRUE);

			perm_string = "O: ";
			perm_string += mask_to_string(owner_mask_on);
			mTextDebugPermO->setText(perm_string);
			mTextDebugPermO->setVisible(TRUE);

			perm_string = "G: ";
			perm_string += mask_to_string(group_mask_on);
			mTextDebugPermG->setText(perm_string);
			mTextDebugPermG->setVisible(TRUE);

			perm_string = "E: ";
			perm_string += mask_to_string(everyone_mask_on);
			mTextDebugPermE->setText(perm_string);
			mTextDebugPermE->setVisible(TRUE);

			perm_string = "N: ";
			perm_string += mask_to_string(next_owner_mask_on);
			mTextDebugPermN->setText(perm_string);
			mTextDebugPermN->setVisible(TRUE);
		}
		perm_string = "F: ";
		U32 flag_mask = 0x0;
		if (objectp->permMove())		flag_mask |= PERM_MOVE;
		if (objectp->permModify())		flag_mask |= PERM_MODIFY;
		if (objectp->permCopy())		flag_mask |= PERM_COPY;
		if (objectp->permTransfer())	flag_mask |= PERM_TRANSFER;
		perm_string += mask_to_string(flag_mask);
		mTextDebugPermF->setText(perm_string);
		mTextDebugPermF->setVisible(TRUE);
	}
	else
	{
		mTextDebugPermB->setVisible(FALSE);
		mTextDebugPermO->setVisible(FALSE);
		mTextDebugPermG->setVisible(FALSE);
		mTextDebugPermE->setVisible(FALSE);
		mTextDebugPermN->setVisible(FALSE);
		mTextDebugPermF->setVisible(FALSE);
	}

	bool has_change_perm_ability = false;
	bool has_change_sale_ability = false;

	if (valid_base_perms &&
		(self_owned ||
		 (group_owned && gAgent.hasPowerInGroup(group_id,
												GP_OBJECT_MANIPULATE))))
	{
		has_change_perm_ability = true;
	}
	if (valid_base_perms &&
		(self_owned ||
		 (group_owned && gAgent.hasPowerInGroup(group_id,
												GP_OBJECT_SET_SALE))))
	{
		has_change_sale_ability = true;
	}

	if (!has_change_perm_ability && !has_change_sale_ability && !root_selected)
	{
		// ...must select root to choose permissions
		mTextPermissionsModify->setValue(MODIFY_INFO_STRINGS[4]);
	}

	if (has_change_perm_ability)
	{
		mCheckShareWithGroup->setEnabled(TRUE);
		mCheckAllowEveryoneMove->setEnabled(owner_mask_on & PERM_MOVE);
		mCheckAllowEveryoneCopy->setEnabled((owner_mask_on & PERM_COPY) &&
											(owner_mask_on & PERM_TRANSFER));
	}
	else
	{
		mCheckShareWithGroup->setEnabled(FALSE);
		mCheckAllowEveryoneMove->setEnabled(FALSE);
		mCheckAllowEveryoneCopy->setEnabled(FALSE);
	}

	if (has_change_sale_ability && (owner_mask_on & PERM_TRANSFER))
	{
		mCheckForSale->setEnabled(can_transfer ||
								  (!can_transfer && num_for_sale));
		// Set the checkbox to tentative if the prices of each object selected
		// are not the same.
		mCheckForSale->setTentative(is_for_sale_mixed);
		mRadioSaleType->setEnabled(num_for_sale && can_transfer &&
								   !is_sale_price_mixed);

		mTextNextOwnerCan->setEnabled(TRUE);
		mCheckNextCanModify->setEnabled(base_mask_on & PERM_MODIFY);
		mCheckNextCanCopy->setEnabled(base_mask_on & PERM_COPY);
		mCheckNextCanTransfer->setEnabled(next_owner_mask_on & PERM_COPY);
	}
	else
	{
		mCheckForSale->setEnabled(FALSE);
		mRadioSaleType->setEnabled(FALSE);

		mTextNextOwnerCan->setEnabled(FALSE);
		mCheckNextCanModify->setEnabled(FALSE);
		mCheckNextCanCopy->setEnabled(FALSE);
		mCheckNextCanTransfer->setEnabled(FALSE);
	}

	if (valid_group_perms)
	{
		if ((group_mask_on & PERM_COPY) && (group_mask_on & PERM_MODIFY) &&
			(group_mask_on & PERM_MOVE))
		{
			mCheckShareWithGroup->setValue(TRUE);
			mCheckShareWithGroup->setTentative(FALSE);
			mButtonDeed->setEnabled(!group_owned && can_transfer &&
									(owner_mask_on & PERM_TRANSFER) &&
									gAgent.hasPowerInGroup(group_id,
														   GP_OBJECT_DEED));
		}
		else if ((group_mask_off & PERM_COPY) &&
				 (group_mask_off & PERM_MODIFY) &&
				 (group_mask_off & PERM_MOVE))
		{
			mCheckShareWithGroup->setValue(FALSE);
			mCheckShareWithGroup->setTentative(FALSE);
			mButtonDeed->setEnabled(FALSE);
		}
		else
		{
			mCheckShareWithGroup->setValue(TRUE);
			mCheckShareWithGroup->setTentative(TRUE);
			mButtonDeed->setEnabled(!group_owned && can_transfer &&
									(group_mask_on & PERM_MOVE) &&
									(owner_mask_on & PERM_TRANSFER) &&
									gAgent.hasPowerInGroup(group_id,
														   GP_OBJECT_DEED));
 		}
	}

	if (valid_everyone_perms)
	{
		// Move
		if (everyone_mask_on & PERM_MOVE)
		{
			mCheckAllowEveryoneMove->setValue(TRUE);
			mCheckAllowEveryoneMove->setTentative(FALSE);
		}
		else if (everyone_mask_off & PERM_MOVE)
		{
			mCheckAllowEveryoneMove->setValue(FALSE);
			mCheckAllowEveryoneMove->setTentative(FALSE);
		}
		else
		{
			mCheckAllowEveryoneMove->setValue(TRUE);
			mCheckAllowEveryoneMove->setTentative(TRUE);
		}

		// Copy == everyone can't copy
		if (everyone_mask_on & PERM_COPY)
		{
			mCheckAllowEveryoneCopy->setValue(TRUE);
			mCheckAllowEveryoneCopy->setTentative(!can_copy || !can_transfer);
		}
		else if (everyone_mask_off & PERM_COPY)
		{
			mCheckAllowEveryoneCopy->setValue(FALSE);
			mCheckAllowEveryoneCopy->setTentative(FALSE);
		}
		else
		{
			mCheckAllowEveryoneCopy->setValue(TRUE);
			mCheckAllowEveryoneCopy->setTentative(TRUE);
		}
	}

	if (valid_next_perms)
	{
		// Modify == next owner canot modify
		if (next_owner_mask_on & PERM_MODIFY)
		{
			mCheckNextCanModify->setValue(TRUE);
			mCheckNextCanModify->setTentative(FALSE);
		}
		else if (next_owner_mask_off & PERM_MODIFY)
		{
			mCheckNextCanModify->setValue(FALSE);
			mCheckNextCanModify->setTentative(FALSE);
		}
		else
		{
			mCheckNextCanModify->setValue(TRUE);
			mCheckNextCanModify->setTentative(TRUE);
		}

		// Copy == next owner cannot copy
		if (next_owner_mask_on & PERM_COPY)
		{
			mCheckNextCanCopy->setValue(TRUE);
			mCheckNextCanCopy->setTentative(!can_copy);
		}
		else if (next_owner_mask_off & PERM_COPY)
		{
			mCheckNextCanCopy->setValue(FALSE);
			mCheckNextCanCopy->setTentative(FALSE);
		}
		else
		{
			mCheckNextCanCopy->setValue(TRUE);
			mCheckNextCanCopy->setTentative(TRUE);
		}

		// Transfer == next owner cannot transfer
		if (next_owner_mask_on & PERM_TRANSFER)
		{
			mCheckNextCanTransfer->setValue(TRUE);
			mCheckNextCanTransfer->setTentative(!can_transfer);
		}
		else if (next_owner_mask_off & PERM_TRANSFER)
		{
			mCheckNextCanTransfer->setValue(FALSE);
			mCheckNextCanTransfer->setTentative(FALSE);
		}
		else
		{
			mCheckNextCanTransfer->setValue(TRUE);
			mCheckNextCanTransfer->setTentative(TRUE);
		}
	}

	// reflect sale information
	LLSaleInfo sale_info;
	BOOL valid_sale_info = selectmgr->selectGetSaleInfo(sale_info);
	LLSaleInfo::EForSale sale_type = sale_info.getSaleType();

	if (valid_sale_info)
	{
		mRadioSaleType->setSelectedIndex((S32)sale_type - 1);
		// unfortunately this doesn't do anything at the moment:
		mRadioSaleType->setTentative(FALSE);
	}
	else
	{
		// default option is sell copy, determined to be safest
		mRadioSaleType->setSelectedIndex((S32)LLSaleInfo::FS_COPY - 1);
		// unfortunately this doesn't do anything at the moment:
		mRadioSaleType->setTentative(TRUE);
	}

	mCheckForSale->setValue(num_for_sale != 0);

	// HACK: There are some old objects in world that are set for sale, but are
	// no-transfer. We need to let users turn for-sale off, but only if
	// for-sale is set.
	bool cannot_actually_sell = !can_transfer ||
								(!can_copy &&
								 sale_type == LLSaleInfo::FS_COPY);
	if (num_for_sale && has_change_sale_ability && cannot_actually_sell)
	{
		mCheckForSale->setEnabled(TRUE);
	}

	// Check search status of objects
	BOOL all_volume = selectmgr->selectionAllPCode(LL_PCODE_VOLUME);
	bool include_in_search;
	bool all_include_in_search = selectmgr->selectionGetIncludeInSearch(&include_in_search);
	mCheckShowInSearch->setEnabled(has_change_sale_ability && all_volume);
	mCheckShowInSearch->setValue(include_in_search);
	mCheckShowInSearch->setTentative(!all_include_in_search);

	// Click action (touch, sit, buy)
	U8 click_action = 0;
	if (selectmgr->selectionGetClickAction(&click_action))
	{
		mComboClickAction->setCurrentByIndex((S32)click_action);
	}
	mTextClickAction->setEnabled(is_perm_modify && all_volume);
	mComboClickAction->setEnabled(is_perm_modify && all_volume);
}

//static
void LLPanelPermissions::onClickClaim(void*)
{
	// try to claim ownership
	LLSelectMgr::getInstance()->sendOwner(gAgentID, gAgent.getGroupID());
}

//static
void LLPanelPermissions::onClickRelease(void*)
{
	// try to release ownership
	LLSelectMgr::getInstance()->sendOwner(LLUUID::null, LLUUID::null);
}

//static
void LLPanelPermissions::onClickCreator(void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;

	LLFloaterAvatarInfo::showFromObject(self->mCreatorID);
}

//static
void LLPanelPermissions::onClickOwner(void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;

	if (LLSelectMgr::getInstance()->selectIsGroupOwned())
	{
		LLUUID group_id;
		LLSelectMgr::getInstance()->selectGetGroup(group_id);
		LLFloaterGroupInfo::showFromUUID(group_id);
	}
	else
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
		{
			return;
		}
//mk
		LLFloaterAvatarInfo::showFromObject(self->mOwnerID);
	}
}

void LLPanelPermissions::onClickGroup(void* data)
{
	LLPanelPermissions* panelp = (LLPanelPermissions*)data;
	LLUUID owner_id;
	std::string name;
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id,
																	   name);
	LLFloater* parent_floater = gFloaterView->getParentFloater(panelp);

	if (owners_identical && (owner_id == gAgentID))
	{
		LLFloaterGroupPicker* fg;
		fg = LLFloaterGroupPicker::showInstance(LLSD(gAgentID));
		fg->setSelectCallback(cbGroupID, data);

		if (parent_floater)
		{
			LLRect rect;
			rect = gFloaterView->findNeighboringPosition(parent_floater, fg);
			fg->setOrigin(rect.mLeft, rect.mBottom);
			parent_floater->addDependentFloater(fg);
		}
	}
}

//static
void LLPanelPermissions::cbGroupID(LLUUID group_id, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	if (self)
	{
		self->mNameBoxGroupName->setNameID(group_id, TRUE);
	}
	LLSelectMgr::getInstance()->sendGroup(group_id);
}

bool callback_deed_to_group(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLUUID group_id;
		BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
		if (group_id.notNull() && groups_identical &&
			gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED))
		{
			LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id,
												  FALSE);
//			LLViewerStats::getInstance()->incStat(LLViewerStats::ST_RELEASE_COUNT);
		}
	}
	return false;
}

void LLPanelPermissions::onClickDeedToGroup(void* data)
{
	LLNotifications::instance().add("DeedObjectToGroup", LLSD(), LLSD(),
									 callback_deed_to_group);
}

///----------------------------------------------------------------------------
/// Permissions checkboxes
///----------------------------------------------------------------------------

//static
void LLPanelPermissions::onCommitPerm(LLUICtrl* ctrl, void* data, U8 field,
									  U32 perm)
{
	LLViewerObject* object;
	object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
	if (object && ctrl)
	{
		LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
		BOOL new_state = check->get();
		LLSelectMgr::getInstance()->selectionSetObjectPermissions(field,
																  new_state,
																  perm);
	}
}

//static
void LLPanelPermissions::onCommitGroupShare(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_GROUP, PERM_MODIFY | PERM_MOVE | PERM_COPY);
}

//static
void LLPanelPermissions::onCommitEveryoneMove(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_EVERYONE, PERM_MOVE);
}

//static
void LLPanelPermissions::onCommitEveryoneCopy(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_EVERYONE, PERM_COPY);
}

//static
void LLPanelPermissions::onCommitNextOwnerModify(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_MODIFY);
}

//static
void LLPanelPermissions::onCommitNextOwnerCopy(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_COPY);
}

//static
void LLPanelPermissions::onCommitNextOwnerTransfer(LLUICtrl* ctrl, void* data)
{
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_TRANSFER);
}

//static
void LLPanelPermissions::onCommitName(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	if (self)
	{
		LLSelectMgr::getInstance()->selectionSetObjectName(self->mEditorObjectName->getText());
	}
}

//static
void LLPanelPermissions::onCommitDesc(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	if (self)
	{
		LLSelectMgr::getInstance()->selectionSetObjectDescription(self->mEditorObjectDesc->getText());
	}
}

//static
void LLPanelPermissions::onCommitSaleInfo(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	if (self)
	{
		self->setAllSaleInfo();
	}
}

//static
void LLPanelPermissions::onCommitSaleType(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	if (self)
	{
		self->setAllSaleInfo();
	}
}

void LLPanelPermissions::setAllSaleInfo()
{
	LLSaleInfo::EForSale sale_type = LLSaleInfo::FS_NOT;

	// Set the sale type if the object(s) are for sale.
	if (mCheckForSale->get())
	{
		switch (mRadioSaleType->getSelectedIndex())
		{
			case 0:
				sale_type = LLSaleInfo::FS_ORIGINAL;
				break;
			case 1:
				sale_type = LLSaleInfo::FS_COPY;
				break;
			case 2:
				sale_type = LLSaleInfo::FS_CONTENTS;
				break;
			default:
				sale_type = LLSaleInfo::FS_COPY;
				break;
		}
	}

	S32 price = -1;

	// Don't extract the price if it's labeled as MIXED or is empty.
	const std::string& price_string = mEditorCost->getText();
	if (!price_string.empty() && price_string != mCostMixed &&
		price_string != mSaleMixed)
	{
		price = atoi(price_string.c_str());
	}
	else
	{
		price = DEFAULT_PRICE;
	}

	// If somehow an invalid price, turn the sale off.
	if (price < 0)
	{
		sale_type = LLSaleInfo::FS_NOT;
	}

	// Force the sale price of not-for-sale items to DEFAULT_PRICE.
	if (sale_type == LLSaleInfo::FS_NOT)
	{
		price = DEFAULT_PRICE;
	}
	// Pack up the sale info and send the update.
	LLSaleInfo sale_info(sale_type, price);
	LLSelectMgr::getInstance()->selectionSetObjectSaleInfo(sale_info);

	// If turned off for-sale, make sure click-action buy is turned
	// off as well
	if (sale_type == LLSaleInfo::FS_NOT)
	{
		U8 click_action = 0;
		LLSelectMgr::getInstance()->selectionGetClickAction(&click_action);
		if (click_action == CLICK_ACTION_BUY)
		{
			LLSelectMgr::getInstance()->selectionSetClickAction(CLICK_ACTION_TOUCH);
		}
	}
}

struct LLSelectionPayable : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* obj)
	{
		// can pay if you or your parent has money() event in script
		LLViewerObject* parent = (LLViewerObject*)obj->getParent();
		return obj->flagTakesMoney() || (parent && parent->flagTakesMoney());
	}
};

//static
void LLPanelPermissions::onCommitClickAction(LLUICtrl* ctrl, void*)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if (!box) return;

	LLSelectMgr* selectmgr = LLSelectMgr::getInstance();
	U8 click_action = (U8)box->getCurrentIndex();
	if (click_action == CLICK_ACTION_BUY)
	{
		LLSaleInfo sale_info;
		selectmgr->selectGetSaleInfo(sale_info);
		if (!sale_info.isForSale())
		{
			LLNotifications::instance().add("CantSetBuyObject");

			// Set click action back to its old value
			U8 click_action = 0;
			selectmgr->selectionGetClickAction(&click_action);
			box->setCurrentByIndex((S32)click_action);

			return;
		}
	}
	else if (click_action == CLICK_ACTION_PAY)
	{
		// Verify object has script with money() handler
		LLSelectionPayable payable;
		bool can_pay = selectmgr->getSelection()->applyToObjects(&payable);
		if (!can_pay)
		{
			// Warn, but do it anyway.
			LLNotifications::instance().add("ClickActionNotPayable");
		}
	}
	selectmgr->selectionSetClickAction(click_action);
}

//static
void LLPanelPermissions::onCommitIncludeInSearch(LLUICtrl* ctrl, void*)
{
	LLCheckBoxCtrl* box = (LLCheckBoxCtrl*)ctrl;
	llassert(box);

	LLSelectMgr::getInstance()->selectionSetIncludeInSearch(box->get());
}
