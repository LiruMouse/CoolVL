/** 
 * @file llpaneleditwearable.cpp
 * @brief  A LLPanel dedicated to the editing of wearables.
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

#include "llpaneleditwearable.h"

#include "llcheckboxctrl.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llcolorswatch.h"
#include "llfloatercustomize.h"
#include "llmorphview.h"
#include "lltexturectrl.h"
#include "lltoolmorph.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llvisualparam.h"
#include "llwearablelist.h"

using namespace LLVOAvatarDefines;

LLPanelEditWearable::LLPanelEditWearable(LLWearableType::EType type)
:	LLPanel(LLWearableType::getTypeLabel(type)),
	mType(type),
	mLayer(0),					// Use the first layer by default
	mSpinLayer(NULL),
	mButtonCreateNew(NULL),
	mButtonSave(NULL),
	mButtonSaveAs(NULL),
	mButtonRevert(NULL),
	mButtonTakeOff(NULL),
	mLockIcon(NULL),
	mWearableIcon(NULL),
	mNotWornInstructions(NULL),
	mNoModifyInstructions(NULL),
	mTitle(NULL),
	mTitleNoModify(NULL),
	mTitleNotWorn(NULL),
	mTitleLoading(NULL),
	mPath(NULL)
{
	mWearable = gAgentWearables.getWearable(type, mLayer);
}

BOOL LLPanelEditWearable::postBuild()
{
	mSpinLayer				= getChild<LLSpinCtrl>("layer", TRUE, FALSE);
	if (mSpinLayer)
	{
		if ((gSavedSettings.getBOOL("NoMultiplePhysics") &&
			 mType == LLWearableType::WT_PHYSICS) ||
			(gSavedSettings.getBOOL("NoMultipleShoes") &&
			 mType == LLWearableType::WT_SHOES) ||
			(gSavedSettings.getBOOL("NoMultipleSkirts") &&
			 mType == LLWearableType::WT_SKIRT))
		{
			mSpinLayer->setVisible(FALSE);
			mSpinLayer = NULL;
		}
		else
		{
			setMaxLayers();
			mSpinLayer->set((F32)mLayer);
			childSetCommitCallback("layer", onCommitLayer, this);
		}
	}

	mLockIcon				= getChild<LLIconCtrl>("lock", TRUE, FALSE);
	mWearableIcon			= getChild<LLIconCtrl>("icon", TRUE, FALSE);
	if (mWearableIcon)
	{
		LLAssetType::EType asset_type = LLWearableType::getAssetType(mType);
		std::string icon_name = LLInventoryIcon::getIconName(asset_type,
															 LLInventoryType::IT_WEARABLE,
															 mType, FALSE);
		mWearableIcon->setValue(icon_name);
	}

	mNotWornInstructions	= getChild<LLTextBox>("not worn instructions", TRUE, FALSE);
	mNoModifyInstructions	= getChild<LLTextBox>("no modify instructions", TRUE, FALSE);
	mTitle					= getChild<LLTextBox>("title", TRUE, FALSE);
	mTitleNoModify			= getChild<LLTextBox>("title_no_modify", TRUE, FALSE);
	mTitleNotWorn			= getChild<LLTextBox>("title_not_worn", TRUE, FALSE);
	mTitleLoading			= getChild<LLTextBox>("title_loading", TRUE, FALSE);
	mPath					= getChild<LLTextBox>("path", TRUE, FALSE);

	mButtonCreateNew		= getChild<LLButton>("Create New", TRUE, FALSE);
	if (mButtonCreateNew)
	{
		mButtonCreateNew->setClickedCallback(onBtnCreateNew, this);
	}

	// If PG, can't take off underclothing or shirt
	mCanTakeOff = LLWearableType::getAssetType(mType) == LLAssetType::AT_CLOTHING &&
				  !(gAgent.isTeen() &&
					(mType == LLWearableType::WT_UNDERSHIRT ||
					 mType == LLWearableType::WT_UNDERPANTS));

	mButtonTakeOff			= getChild<LLButton>("Take Off", TRUE, FALSE);
	if (mButtonTakeOff)
	{
		mButtonTakeOff->setVisible(mCanTakeOff);
		mButtonTakeOff->setClickedCallback(onBtnTakeOff, this);
	}

	mButtonSave				= getChild<LLButton>("Save", TRUE, FALSE);
	if (mButtonSave)
	{
		mButtonSave->setClickedCallback(onBtnSave, this);
	}

	mButtonSaveAs			= getChild<LLButton>("Save As", TRUE, FALSE);
	if (mButtonSaveAs)
	{
		mButtonSaveAs->setClickedCallback(onBtnSaveAs, this);
	}

	mButtonRevert			= getChild<LLButton>("Revert", TRUE, FALSE);
	if (mButtonRevert)
	{
		mButtonRevert->setClickedCallback(onBtnRevert, this);
	}

	return TRUE;
}

LLPanelEditWearable::~LLPanelEditWearable()
{
	std::for_each(mSubpartList.begin(), mSubpartList.end(), DeletePairedPointer());

	// Clear colorswatch commit callbacks that point to this object.
	for (std::map<std::string, S32>::iterator iter = mColorList.begin();
		 iter != mColorList.end(); ++iter)
	{
		childSetCommitCallback(iter->first, NULL, NULL);
	}
}

void LLPanelEditWearable::addSubpart(const std::string& name, ESubpart id,
									 LLSubpart* part)
{
	if (!name.empty())
	{
		childSetAction(name, &LLPanelEditWearable::onBtnSubpart, (void*)id);
		part->mButtonName = name;
	}
	mSubpartList[id] = part;
}

// static
void LLPanelEditWearable::onBtnSubpart(void* userdata)
{
	if (!gFloaterCustomize) return;
	LLPanelEditWearable* self = gFloaterCustomize->getCurrentWearablePanel();
	if (!self) return;
	ESubpart subpart = (ESubpart) (intptr_t)userdata;
	self->setSubpart(subpart);
}

void LLPanelEditWearable::setSubpart(ESubpart subpart)
{
	mCurrentSubpart = subpart;

	for (std::map<ESubpart, LLSubpart*>::iterator iter = mSubpartList.begin();
		 iter != mSubpartList.end(); ++iter)
	{
		LLButton* btn = getChild<LLButton>(iter->second->mButtonName, TRUE,
										   FALSE);
		if (btn)
		{
			btn->setToggleState(subpart == iter->first);
		}
	}

	LLSubpart* part = get_if_there(mSubpartList, (ESubpart)subpart,
								   (LLSubpart*)NULL);
	if (part && isAgentAvatarValid())
	{
		// Update the thumbnails we display
		LLFloaterCustomize::param_map sorted_params;
		ESex avatar_sex = gAgentAvatarp->getSex();

		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gAgentWearables.getWearableInventoryItem(mType,
																				mLayer);
		U32 perm_mask = 0x0;
		BOOL is_complete = FALSE;
		if (item)
		{
			perm_mask = item->getPermissions().getMaskOwner();
			is_complete = item->isFinished();
		}
		setUIPermissions(perm_mask, is_complete);
		BOOL editable = (perm_mask & PERM_MODIFY) && is_complete;

		for (LLViewerVisualParam* param = (LLViewerVisualParam*)gAgentAvatarp->getFirstVisualParam(); 
			 param;
			 param = (LLViewerVisualParam*)gAgentAvatarp->getNextVisualParam())
		{
			if (param->getID() == -1 || !param->isTweakable() ||
				param->getEditGroup() != part->mEditGroup ||
				!(param->getSex() & avatar_sex))
			{
				continue;
			}

			// negative getDisplayOrder() to make lowest order the highest priority
			LLFloaterCustomize::param_map::value_type vt(-param->getDisplayOrder(),
														 LLFloaterCustomize::editable_param(editable, param));
			// Check for duplicates
			llassert(sorted_params.find(-param->getDisplayOrder()) == sorted_params.end());

			sorted_params.insert(vt);
		}
		gFloaterCustomize->generateVisualParamHints(this,
													NULL,
													sorted_params,
													mWearable,
													part->mVisualHint);
		gFloaterCustomize->updateScrollingPanelUI();

		// Update the camera
		gMorphView->setCameraTargetJoint(gAgentAvatarp->getJoint(part->mTargetJoint));
		gMorphView->setCameraTargetOffset(part->mTargetOffset);
		gMorphView->setCameraOffset(part->mCameraOffset);
		gMorphView->setCameraDistToDefault();
		if (gSavedSettings.getBOOL("AppearanceCameraMovement"))
		{
			gMorphView->updateCamera();
		}
	}
}

// static
void LLPanelEditWearable::onBtnTakeOff(void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	if (!self) return;

	LLWearable* wearable = gAgentWearables.getWearable(self->mType,
													   self->mLayer);
	if (!wearable)
	{
		return;
	}

	gAgentWearables.removeWearable(self->mType, false, self->mLayer);
}

// static
void LLPanelEditWearable::onBtnSave(void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	if (!self) return;

	gAgentWearables.saveWearable(self->mType, self->mLayer);
}

// static
void LLPanelEditWearable::onBtnSaveAs(void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	if (!self) return;

	LLWearable* wearable = gAgentWearables.getWearable(self->mType,
													   self->mLayer);
	if (wearable)
	{
		LLWearableSaveAsDialog* save_as_dialog = new LLWearableSaveAsDialog(wearable->getName(),
																			onSaveAsCommit, self);
		save_as_dialog->startModal();
		// LLWearableSaveAsDialog deletes itself.
	}
}

// static
void LLPanelEditWearable::onSaveAsCommit(LLWearableSaveAsDialog* save_as_dialog,
										 void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	if (self && isAgentAvatarValid())
	{
		gAgentWearables.saveWearableAs(self->mType, self->mLayer,
									   save_as_dialog->getItemName(), FALSE);
	}
}

// static
void LLPanelEditWearable::onBtnRevert(void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	if (!self) return;

	gAgentWearables.revertWearable(self->mType, self->mLayer);
}

// static
void LLPanelEditWearable::onBtnCreateNew(void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	if (self && isAgentAvatarValid())
	{
		// Create a new wearable in the default folder for the wearable's asset
		// type.
		LLWearable* wearable;
		wearable = LLWearableList::instance().createNewWearable(self->mType);
		LLAssetType::EType asset_type = wearable->getAssetType();

		LLUUID folder_id;
		// regular UI, items get created in normal folder
		folder_id = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(asset_type));

		LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback(false);
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(), folder_id,
							  wearable->getTransactionID(), wearable->getName(),
							  wearable->getDescription(), asset_type,
							  LLInventoryType::IT_WEARABLE, wearable->getType(),
							  wearable->getPermissions().getMaskNextOwner(), cb);
	}
}

bool LLPanelEditWearable::textureIsInvisible(ETextureIndex te)
{
	if (gAgentWearables.getWearable(mType, getWearableIndex()))
	{
		if (isAgentAvatarValid())
		{
			const LLTextureEntry* current_te = gAgentAvatarp->getTE(te);
			return (current_te && current_te->getID() == IMG_INVISIBLE);
		}
	}
	return false;
}

void LLPanelEditWearable::addInvisibilityCheckbox(ETextureIndex te,
												  const std::string& name)
{
	childSetCommitCallback(name, onInvisibilityCommit, this);

	mInvisibilityList[name] = te;
}

// static
void LLPanelEditWearable::onInvisibilityCommit(LLUICtrl* ctrl, void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	LLCheckBoxCtrl* checkbox_ctrl = (LLCheckBoxCtrl*)ctrl;
	if (!self || !checkbox_ctrl || !self->mWearable || !isAgentAvatarValid())
	{
		return;
	}

	ETextureIndex te = (ETextureIndex)(self->mInvisibilityList[ctrl->getName()]);

	bool new_invis_state = checkbox_ctrl->get();
	if (new_invis_state)
	{
		LLLocalTextureObject* lto = self->mWearable->getLocalTextureObject(te);
		self->mPreviousTextureList[te] = lto->getID();

		LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(IMG_INVISIBLE);
		gAgentAvatarp->setLocalTexture(te, image, FALSE, self->mLayer);
		gAgentAvatarp->wearableUpdated(self->mType, FALSE);
	}
	else
	{
		// Try to restore previous texture, if any.
		LLUUID prev_id = self->mPreviousTextureList[(S32)te];
		if (prev_id.isNull() || prev_id == IMG_INVISIBLE)
		{
			prev_id = LLUUID(gSavedSettings.getString("UIImgDefaultAlphaUUID"));
		}
		if (prev_id.notNull())
		{
			LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(prev_id);
			if (image)
			{
				gAgentAvatarp->setLocalTexture(te, image, FALSE, self->mLayer);
				gAgentAvatarp->wearableUpdated(self->mType, FALSE);
			}
		}
	}
}

void LLPanelEditWearable::addColorSwatch(ETextureIndex te,
										 const std::string& name)
{
	childSetCommitCallback(name, LLPanelEditWearable::onColorCommit, this);
	mColorList[name] = te;
}

// static
void LLPanelEditWearable::onColorCommit(LLUICtrl* ctrl, void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	LLColorSwatchCtrl* color_ctrl = (LLColorSwatchCtrl*) ctrl;

	if (self && color_ctrl && isAgentAvatarValid() && self->mWearable)
	{
		std::map<std::string, S32>::const_iterator cl_itr = self->mColorList.find(ctrl->getName());
		if (cl_itr != self->mColorList.end())
		{
			ETextureIndex te = (ETextureIndex)cl_itr->second;

			LLColor4 old_color = self->mWearable->getClothesColor(te);
			const LLColor4& new_color = color_ctrl->get();
			if (old_color != new_color)
			{
				// Set the new version
				self->mWearable->setClothesColor(te, new_color, TRUE);
//				gAgentAvatarp->setClothesColor(te, new_color, TRUE);
				LLVisualParamHint::requestHintUpdates();
 				gAgentAvatarp->wearableUpdated(self->mType, FALSE);
			}
		}
	}
}

void LLPanelEditWearable::initPreviousTextureList()
{
	initPreviousTextureListEntry(TEX_LOWER_ALPHA);
	initPreviousTextureListEntry(TEX_UPPER_ALPHA);
	initPreviousTextureListEntry(TEX_HEAD_ALPHA);
	initPreviousTextureListEntry(TEX_EYES_ALPHA);
	initPreviousTextureListEntry(TEX_LOWER_ALPHA);
}

void LLPanelEditWearable::initPreviousTextureListEntry(ETextureIndex te)
{
	if (mWearable)
	{
		LLUUID id = LLUUID::null;
		LLLocalTextureObject* lto = mWearable->getLocalTextureObject(te);
		if (lto)
		{
			id = lto->getID();
		}
		mPreviousTextureList[te] = id;
	}
}

void LLPanelEditWearable::addTextureDropTarget(ETextureIndex te,
											   const std::string& name,
											   const LLUUID& default_image_id,
											   BOOL allow_no_texture)
{
	childSetCommitCallback(name, LLPanelEditWearable::onTextureCommit, this);
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>(name, TRUE, FALSE);
	if (texture_ctrl)
	{
		texture_ctrl->setDefaultImageAssetID(default_image_id);
		texture_ctrl->setAllowNoTexture(allow_no_texture);
		// Don't allow (no copy) or (no transfer) textures to be selected.
		texture_ctrl->setImmediateFilterPermMask(PERM_NONE);	//PERM_COPY | PERM_TRANSFER);
		texture_ctrl->setNonImmediateFilterPermMask(PERM_NONE);	//PERM_COPY | PERM_TRANSFER);
	}
	mTextureList[name] = te;
	if (mType == LLWearableType::WT_ALPHA)
	{
		initPreviousTextureListEntry(te);
	}
}

// static
void LLPanelEditWearable::onTextureCommit(LLUICtrl* ctrl, void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*) userdata;
	LLTextureCtrl* texture_ctrl = (LLTextureCtrl*) ctrl;

	if (self && ctrl && isAgentAvatarValid())
	{
		ETextureIndex te = (ETextureIndex)(self->mTextureList[ctrl->getName()]);

		// Set the new version
		LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(texture_ctrl->getImageAssetID());
		if (image->getID().isNull() || image->getID() == IMG_DEFAULT)
		{
			image = LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT_AVATAR);
		}
		self->mTextureList[ctrl->getName()] = te;
		if (self->mWearable)
		{
			gAgentAvatarp->setLocalTexture(te, image, FALSE, self->mLayer);
			LLVisualParamHint::requestHintUpdates();
			gAgentAvatarp->wearableUpdated(self->mType, FALSE);
		}
		if (self->mType == LLWearableType::WT_ALPHA &&
			image->getID() != IMG_INVISIBLE)
		{
			self->mPreviousTextureList[te] = image->getID();
		}
	}
}

ESubpart LLPanelEditWearable::getDefaultSubpart()
{
	switch (mType)
	{
		case LLWearableType::WT_SHAPE:		return SUBPART_SHAPE_WHOLE;
		case LLWearableType::WT_SKIN:		return SUBPART_SKIN_COLOR;
		case LLWearableType::WT_HAIR:		return SUBPART_HAIR_COLOR;
		case LLWearableType::WT_EYES:		return SUBPART_EYES;
		case LLWearableType::WT_SHIRT:		return SUBPART_SHIRT;
		case LLWearableType::WT_PANTS:		return SUBPART_PANTS;
		case LLWearableType::WT_SHOES:		return SUBPART_SHOES;
		case LLWearableType::WT_SOCKS:		return SUBPART_SOCKS;
		case LLWearableType::WT_JACKET:		return SUBPART_JACKET;
		case LLWearableType::WT_GLOVES:		return SUBPART_GLOVES;
		case LLWearableType::WT_UNDERSHIRT:	return SUBPART_UNDERSHIRT;
		case LLWearableType::WT_UNDERPANTS:	return SUBPART_UNDERPANTS;
		case LLWearableType::WT_SKIRT:		return SUBPART_SKIRT;
		case LLWearableType::WT_ALPHA:		return SUBPART_ALPHA;
		case LLWearableType::WT_TATTOO:		return SUBPART_TATTOO;
		case LLWearableType::WT_PHYSICS:	return SUBPART_PHYSICS_BELLY_UPDOWN;

		default:	llassert(0);		return SUBPART_SHAPE_WHOLE;
	}
}

void LLPanelEditWearable::draw()
{
	if (gFloaterCustomize->isMinimized() || !isAgentAvatarValid())
	{
		return;
	}

	BOOL has_wearable = (mWearable != NULL);
	BOOL is_dirty = isDirty();
	BOOL is_modifiable = FALSE;
	BOOL is_copyable = FALSE;
	BOOL is_complete = FALSE;
	LLViewerInventoryItem* item = NULL;
	if (has_wearable)
	{
		item = (LLViewerInventoryItem*)gAgentWearables.getWearableInventoryItem(mType,
																				mLayer);
		if (item)
		{
			const LLPermissions& perm = item->getPermissions();
			is_modifiable = perm.allowModifyBy(gAgent.getID(), gAgent.getGroupID());
			is_copyable = perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID());
			is_complete = item->isFinished();
		}
	}

	setMaxLayers();

	if (mButtonSave)
	{
		mButtonSave->setEnabled(is_modifiable && is_complete && has_wearable &&
								is_dirty);
		mButtonSave->setVisible(has_wearable || !mButtonCreateNew);
	}
	if (mButtonSaveAs)
	{
		mButtonSaveAs->setEnabled(is_copyable && is_complete && has_wearable);
		mButtonSaveAs->setVisible(has_wearable || !mButtonCreateNew);
	}
	if (mButtonRevert)
	{
		mButtonRevert->setEnabled(has_wearable && is_dirty);
		mButtonRevert->setVisible(has_wearable || !mButtonCreateNew);
	}
	if (mButtonTakeOff)
	{
		mButtonTakeOff->setEnabled(has_wearable);
		mButtonTakeOff->setVisible(mCanTakeOff && has_wearable);
	}
	if (mButtonCreateNew)
	{
		mButtonCreateNew->setVisible(!has_wearable);
	}

	if (mNotWornInstructions)
	{
		mNotWornInstructions->setVisible(!has_wearable);
	}
	if (mNoModifyInstructions)
	{
		mNoModifyInstructions->setVisible(has_wearable && !is_modifiable);
	}

	for (std::map<ESubpart, LLSubpart*>::iterator iter = mSubpartList.begin();
		 iter != mSubpartList.end(); ++iter)
	{
		std::string btn_name = iter->second->mButtonName;
		if (btn_name.empty() ||	!getChild<LLButton>(btn_name, TRUE, FALSE))
		{
			continue;
		}

		childSetVisible(btn_name, has_wearable);
		if (has_wearable && is_complete && is_modifiable)
		{
			childSetEnabled(btn_name, iter->second->mSex & gAgentAvatarp->getSex());
		}
		else
		{
			childSetEnabled(btn_name, FALSE);
		}
	}

	if (mLockIcon)		mLockIcon->setVisible(!is_modifiable);

	if (mTitle)			mTitle->setVisible(FALSE);
	if (mTitleNoModify)	mTitleNoModify->setVisible(FALSE);
	if (mTitleNotWorn)	mTitleNotWorn->setVisible(FALSE);
	if (mTitleLoading)	mTitleLoading->setVisible(FALSE);

	if (mPath)			mPath->setVisible(FALSE);

	if (has_wearable && !is_modifiable)
	{
		if (mTitleNoModify)
		{
			mTitleNoModify->setVisible(TRUE);
			// *TODO:Translate
			mTitleNoModify->setTextArg("[DESC]", item ? item->getName()
													  : mWearable->getName());
		}

		hideTextureControls();
	}
	else if (has_wearable && !is_complete)
	{
		if (mTitleLoading)
		{
			mTitleLoading->setVisible(TRUE);
			// *TODO:Translate
			mTitleLoading->setTextArg("[DESC]",
									  LLWearableType::getTypeLabel(mType));
		}

		if (mPath)
		{
			std::string path;
			const LLUUID& item_id = gAgentWearables.getWearableItemID(mType, mLayer);
			gInventory.appendPath(item_id, path);
			mPath->setVisible(TRUE);
			mPath->setTextArg("[PATH]", path);
		}

		hideTextureControls();
	}
	else if (has_wearable && is_modifiable)
	{
		if (mTitle)
		{
			mTitle->setVisible(TRUE);
			mTitle->setTextArg("[DESC]", item ? item->getName()
											  : mWearable->getName());
		}

		if (mPath)
		{
			std::string path;
			const LLUUID& item_id = gAgentWearables.getWearableItemID(mType, mLayer);
			gInventory.appendPath(item_id, path);
			mPath->setVisible(TRUE);
			mPath->setTextArg("[PATH]", path);
		}

		for (std::map<std::string, S32>::iterator iter = mTextureList.begin();
			 iter != mTextureList.end(); ++iter)
		{
			std::string name = iter->first;
			LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>(name, TRUE, FALSE);
			ETextureIndex te = (ETextureIndex)iter->second;
			childSetVisible(name, is_copyable && is_modifiable && is_complete);
			if (texture_ctrl)
			{
                LLLocalTextureObject *lto = mWearable->getLocalTextureObject(te);

				LLUUID new_id = LLUUID::null;
                if (lto && lto->getID() != IMG_DEFAULT_AVATAR)
                {
                        new_id = lto->getID();
                }

				LLUUID old_id = texture_ctrl->getImageAssetID();

				if (old_id != new_id)
				{
					// texture has changed, close the floater to avoid DEV-22461
					texture_ctrl->closeFloater();
				}

				texture_ctrl->setImageAssetID(new_id);
			}
		}

		for (std::map<std::string, S32>::iterator iter = mColorList.begin();
			 iter != mColorList.end(); ++iter)
		{
			std::string name = iter->first;
			ETextureIndex te = (ETextureIndex)iter->second;
			childSetVisible(name, is_modifiable && is_complete);
			childSetEnabled(name, is_modifiable && is_complete);
			LLColorSwatchCtrl* ctrl = getChild<LLColorSwatchCtrl>(name, TRUE, FALSE);
			if (ctrl)
			{
				ctrl->set(mWearable->getClothesColor(te));
			}
		}

		for (std::map<std::string, S32>::iterator iter = mInvisibilityList.begin();
			 iter != mInvisibilityList.end(); ++iter)
		{
			std::string name = iter->first;
			ETextureIndex te = (ETextureIndex)iter->second;
			childSetVisible(name, is_copyable && is_modifiable && is_complete);
			childSetEnabled(name, is_copyable && is_modifiable && is_complete);
			LLCheckBoxCtrl* ctrl = getChild<LLCheckBoxCtrl>(name, TRUE, FALSE);
			if (ctrl)
			{
				ctrl->set(!gAgentAvatarp->isTextureVisible(te, mWearable));
			}
		}
	}
	else
	{
		if (mTitleNotWorn)
		{
			mTitleNotWorn->setVisible(TRUE);
			// *TODO:Translate
			mTitleNotWorn->setTextArg("[DESC]",
									  LLWearableType::getTypeLabel(mType));
		}

		hideTextureControls();
	}
#if 0
	if (mWearableIcon)
	{
		mWearableIcon->setVisible(has_wearable);
	}
#endif
	LLPanel::draw();
}

void LLPanelEditWearable::hideTextureControls()
{
	for (std::map<std::string, S32>::iterator iter = mTextureList.begin();
			 iter != mTextureList.end(); ++iter)
	{
		childSetVisible(iter->first, FALSE);
	}
	for (std::map<std::string, S32>::iterator iter = mColorList.begin();
			 iter != mColorList.end(); ++iter)
	{
		childSetVisible(iter->first, FALSE);
	}
	for (std::map<std::string, S32>::iterator iter = mInvisibilityList.begin();
		 iter != mInvisibilityList.end(); ++iter)
	{
		childSetVisible(iter->first, FALSE);
	}
}

void LLPanelEditWearable::setMaxLayers()
{
	if (mSpinLayer)
	{
		U32 max = llmin(gAgentWearables.getWearableCount(mType),
						LLAgentWearables::MAX_CLOTHING_PER_TYPE - 1);
		mSpinLayer->setMaxValue((F32)max);
	}
}

void LLPanelEditWearable::setWearable(LLWearable* wearable, U32 perm_mask,
									  BOOL is_complete)
{
	mWearable = wearable;
	if (wearable)
	{
		mLayer = gAgentWearables.getWearableIndex(wearable);
		if (mLayer == LLAgentWearables::MAX_CLOTHING_PER_TYPE)
		{
			mLayer = 0;
		}
		if (mSpinLayer)
		{
			setMaxLayers();
			mSpinLayer->set((F32)mLayer);
		}
		if (mType == LLWearableType::WT_ALPHA)
		{
			initPreviousTextureList();
		}
	}
	setUIPermissions(perm_mask, is_complete);
}

void LLPanelEditWearable::switchToDefaultSubpart()
{
	setSubpart(getDefaultSubpart());
}

void LLPanelEditWearable::setVisible(BOOL visible)
{
	LLPanel::setVisible(visible);
	if (!visible)
	{
		for (std::map<std::string, S32>::iterator iter = mColorList.begin();
			 iter != mColorList.end(); ++iter)
		{
			// this forces any open color pickers to cancel their selection
			childSetEnabled(iter->first, FALSE);
		}
	}
}

BOOL LLPanelEditWearable::isDirty() const
{
	LLWearable* wearable = gAgentWearables.getWearable(mType, mLayer);
	return wearable && wearable->isDirty();
}

// static
void LLPanelEditWearable::onCommitSexChange(LLUICtrl*, void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	if (!self || !self->mWearable) return;

	if (!isAgentAvatarValid())
	{
		return;
	}

	if (!gAgentWearables.isWearableModifiable(self->mType, self->mLayer))
	{
		return;
	}

	ESex new_sex = gSavedSettings.getU32("AvatarSex") ? SEX_MALE : SEX_FEMALE;

	LLViewerVisualParam* param = (LLViewerVisualParam*)gAgentAvatarp->getVisualParam("male");
	if (!param)
	{
		return;
	}

	param->setWeight(new_sex == SEX_MALE, TRUE);

	gAgentAvatarp->updateSexDependentLayerSets(TRUE);

	gAgentAvatarp->updateVisualParams();

	gFloaterCustomize->clearScrollingPanelList();

	// Assumes that we're in the "Shape" Panel.
	self->setSubpart(SUBPART_SHAPE_WHOLE);
}

//static
void LLPanelEditWearable::onCommitLayer(LLUICtrl*, void* userdata)
{
	LLPanelEditWearable* self = (LLPanelEditWearable*)userdata;
	if (!self || !self->mSpinLayer || !gFloaterCustomize) return;

	U32 index = (U32)self->mSpinLayer->get();
	LLWearable* wearable = gAgentWearables.getWearable(self->mType, index);
	if (wearable)
	{
		gFloaterCustomize->updateWearableType(self->mType, wearable);
	}
	else
	{
		self->setWearable(NULL, PERM_ALL, TRUE);
		LLFloaterCustomize::setCurrentWearableType(self->mType);
		gFloaterCustomize->updateScrollingPanelUI();
	}
}

void LLPanelEditWearable::setUIPermissions(U32 perm_mask, BOOL is_complete)
{
	BOOL is_copyable = (perm_mask & PERM_COPY) ? TRUE : FALSE;
	BOOL is_modifiable = (perm_mask & PERM_MODIFY) ? TRUE : FALSE;

	if (mButtonSave)	mButtonSave->setEnabled(is_modifiable && is_complete);
	if (mButtonSaveAs)	mButtonSaveAs->setEnabled(is_copyable && is_complete);

	if (getChild<LLUICtrl>("sex radio", TRUE, FALSE))
	{
		childSetEnabled("sex radio", is_modifiable && is_complete);
	}

	for (std::map<std::string, S32>::iterator iter = mTextureList.begin();
		 iter != mTextureList.end(); ++iter)
	{
		childSetVisible(iter->first, is_copyable && is_modifiable && is_complete);
	}
	for (std::map<std::string, S32>::iterator iter = mColorList.begin();
		 iter != mColorList.end(); ++iter)
	{
		childSetVisible(iter->first, is_modifiable && is_complete);
	}
	for (std::map<std::string, S32>::iterator iter = mInvisibilityList.begin();
		 iter != mInvisibilityList.end(); ++iter)
	{
		childSetVisible(iter->first, is_copyable && is_modifiable && is_complete);
	}
}

////////////////////////////////////////////////////////////////////////////

LLWearableSaveAsDialog::LLWearableSaveAsDialog(const std::string& desc,
											   void (*commit_cb)(LLWearableSaveAsDialog*, void*),
											   void* userdata)
:	LLModalDialog(LLStringUtil::null, 240, 100),
	mCommitCallback(commit_cb),
	mCallbackUserData(userdata)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_wearable_save_as.xml");

	childSetAction("Save", LLWearableSaveAsDialog::onSave, this);
	childSetAction("Cancel", LLWearableSaveAsDialog::onCancel, this);
	childSetTextArg("name ed", "[DESC]", desc);
}

//virtual
void LLWearableSaveAsDialog::startModal()
{
	LLModalDialog::startModal();
	LLLineEditor* edit = getChild<LLLineEditor>("name ed", TRUE, FALSE);
	if (!edit) return;
	edit->setFocus(TRUE);
	edit->selectAll();
}

//static
void LLWearableSaveAsDialog::onSave(void* userdata)
{
	LLWearableSaveAsDialog* self = (LLWearableSaveAsDialog*)userdata;
	self->mItemName = self->childGetValue("name ed").asString();
	LLStringUtil::trim(self->mItemName);
	if (!self->mItemName.empty())
	{
		if (self->mCommitCallback)
		{
			self->mCommitCallback(self, self->mCallbackUserData);
		}
		self->close(); // destroys this object
	}
}

//static
void LLWearableSaveAsDialog::onCancel(void* userdata)
{
	LLWearableSaveAsDialog* self = (LLWearableSaveAsDialog*)userdata;
	self->close(); // destroys this object
}
