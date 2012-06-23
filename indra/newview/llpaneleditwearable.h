/** 
 * @file llpaneleditwearable.h
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

#ifndef LL_LLPANELEDITWEARABLE_H
#define LL_LLPANELEDITWEARABLE_H

#include "llbutton.h"
#include "lliconctrl.h"
#include "llmodaldialog.h"
#include "llpanel.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lluictrl.h"

#include "llfilepicker.h"
#include "llvoavatardefines.h"
#include "llvoavatarself.h"
#include "llwearabletype.h"

////////////////////////////////////////////////////////////////////////////

class LLWearableSaveAsDialog : public LLModalDialog
{
public:
	LLWearableSaveAsDialog(const std::string& desc,
						   void (*commit_cb)(LLWearableSaveAsDialog*, void*),
						   void* userdata);

	virtual void startModal();

	const std::string& getItemName()	{ return mItemName; }

private:
	static void onSave(void* userdata);
	static void onCancel(void* userdata);

private:
	std::string	mItemName;
	void		(*mCommitCallback)(LLWearableSaveAsDialog*, void*);
	void*		mCallbackUserData;
};

/////////////////////////////////////////////////////////////////////
// LLPanelEditWearable

enum ESubpart
{
	SUBPART_SHAPE_HEAD = 1, // avoid 0
	SUBPART_SHAPE_EYES,
	SUBPART_SHAPE_EARS,
	SUBPART_SHAPE_NOSE,
	SUBPART_SHAPE_MOUTH,
	SUBPART_SHAPE_CHIN,
	SUBPART_SHAPE_TORSO,
	SUBPART_SHAPE_LEGS,
	SUBPART_SHAPE_WHOLE,
	SUBPART_SHAPE_DETAIL,
	SUBPART_SKIN_COLOR,
	SUBPART_SKIN_FACEDETAIL,
	SUBPART_SKIN_MAKEUP,
	SUBPART_SKIN_BODYDETAIL,
	SUBPART_HAIR_COLOR,
	SUBPART_HAIR_STYLE,
	SUBPART_HAIR_EYEBROWS,
	SUBPART_HAIR_FACIAL,
	SUBPART_EYES,
	SUBPART_SHIRT,
	SUBPART_PANTS,
	SUBPART_SHOES,
	SUBPART_SOCKS,
	SUBPART_JACKET,
	SUBPART_GLOVES,
	SUBPART_UNDERSHIRT,
	SUBPART_UNDERPANTS,
	SUBPART_SKIRT,
	SUBPART_ALPHA,
	SUBPART_TATTOO,
	SUBPART_PHYSICS_BREASTS_UPDOWN,
	SUBPART_PHYSICS_BREASTS_INOUT,
	SUBPART_PHYSICS_BREASTS_LEFTRIGHT,
	SUBPART_PHYSICS_BELLY_UPDOWN,
	SUBPART_PHYSICS_BUTT_UPDOWN,
	SUBPART_PHYSICS_BUTT_LEFTRIGHT,
	SUBPART_PHYSICS_ADVANCED
};

struct LLSubpart
{
	LLSubpart() : mSex(SEX_BOTH), mVisualHint(true) {}

	std::string		mButtonName;
	std::string		mTargetJoint;
	std::string		mEditGroup;
	LLVector3d		mTargetOffset;
	LLVector3d		mCameraOffset;
	ESex			mSex;
	bool			mVisualHint;
};

////////////////////////////////////////////////////////////////////////////

class LLPanelEditWearable : public LLPanel
{
public:
	LLPanelEditWearable(LLWearableType::EType type);
	virtual					~LLPanelEditWearable();

	virtual BOOL 			postBuild();
	virtual void			draw();
	virtual BOOL			isDirty() const;	// LLUICtrl

	void					addSubpart(const std::string& name,
									   ESubpart id, LLSubpart* part);
	void					addTextureDropTarget(LLVOAvatarDefines::ETextureIndex te,
												 const std::string& name,
												 const LLUUID& default_image_id,
												 BOOL allow_no_texture);
	void					addInvisibilityCheckbox(LLVOAvatarDefines::ETextureIndex te,
													const std::string& name);
	void					addColorSwatch(LLVOAvatarDefines::ETextureIndex te,
										   const std::string& name);

	const std::string&		getLabel()			{ return LLWearableType::getTypeLabel(mType); }
	LLWearableType::EType	getType()			{ return mType; }

	LLSubpart*				getCurrentSubpart()	{ return mSubpartList[mCurrentSubpart]; }
	ESubpart				getDefaultSubpart();
	void					setSubpart(ESubpart subpart);
	void					switchToDefaultSubpart();

	void 					setWearable(LLWearable* wearable,
										U32 perm_mask,
										BOOL is_complete);
	LLWearable*				getWearable()		{ return mWearable; }
	U32						getWearableIndex()	{ return mLayer; }

	void 					setUIPermissions(U32 perm_mask, BOOL is_complete);

	void					hideTextureControls();
	bool					textureIsInvisible(LLVOAvatarDefines::ETextureIndex te);
	void					initPreviousTextureList();
	void					initPreviousTextureListEntry(LLVOAvatarDefines::ETextureIndex te);

	virtual void			setVisible(BOOL visible);

private:
	// Callbacks
	static void				onCommitSexChange(LLUICtrl*, void* userdata);
	static void				onBtnImport(void* userdata);
	static void 			importCallback(LLFilePicker::ELoadFilter type,
										   std::string& filename,
										   std::deque<std::string>& files,
										   void* userdata);

	static void				onBtnSubpart(void* userdata);
	static void				onBtnTakeOff(void* userdata);
	static void				onBtnSave(void* userdata);

	static void				onBtnSaveAs(void* userdata);
	static void				onSaveAsCommit(LLWearableSaveAsDialog* save_as_dialog,
										   void* userdata);
	static void				onBtnRevert(void* userdata);
	static void				onBtnTakeOffDialog(S32 option, void* userdata);
	static void				onBtnCreateNew(void* userdata);
	static void				onTextureCommit(LLUICtrl* ctrl, void* userdata);
	static void				onInvisibilityCommit(LLUICtrl* ctrl, void* userdata);
	static void				onColorCommit(LLUICtrl* ctrl, void* userdata);
	static void				onCommitLayer(LLUICtrl*, void* userdata);

	void					setMaxLayers();

private:
	LLWearable*						mWearable;
	LLWearableType::EType			mType;
	U32								mLayer;
	BOOL							mCanTakeOff;
	std::map<std::string, S32>		mTextureList;
	std::map<std::string, S32>		mInvisibilityList;
	std::map<std::string, S32>		mColorList;
	std::map<ESubpart, LLSubpart*>	mSubpartList;
	std::map<S32, LLUUID>			mPreviousTextureList;
	ESubpart						mCurrentSubpart;

	LLSpinCtrl*						mSpinLayer;

	LLButton*						mButtonImport;
	LLButton*						mButtonCreateNew;
	LLButton*						mButtonSave;
	LLButton*						mButtonSaveAs;
	LLButton*						mButtonRevert;
	LLButton*						mButtonTakeOff;

	LLUICtrl*						mSexRadio;

	LLIconCtrl*						mWearableIcon;
	LLIconCtrl*						mLockIcon;

	LLTextBox*						mNotWornInstructions;
	LLTextBox*						mNoModifyInstructions;
	LLTextBox*						mTitle;
	LLTextBox*						mTitleNoModify;
	LLTextBox*						mTitleNotWorn;
	LLTextBox*						mTitleLoading;
	LLTextBox*						mPath;
};

#endif
