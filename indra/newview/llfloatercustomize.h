/** 
 * @file llfloatercustomize.h
 * @brief The customize avatar floater, triggered by "Appearance..."
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

#ifndef LL_LLFLOATERCUSTOMIZE_H
#define LL_LLFLOATERCUSTOMIZE_H

#include <map>

#include "llfloater.h"
#include "llwearable.h"

class LLInventoryObserver;
class LLPanelEditWearable;
class LLScrollableContainerView;
class LLScrollingPanelList;
class LLViewerJointMesh;
class LLViewerVisualParam;
class LLVisualParamReset;

/////////////////////////////////////////////////////////////////////
// LLFloaterCustomize

class LLFloaterCustomize : public LLFloater
{
public:
	typedef std::pair<BOOL, LLViewerVisualParam*> editable_param;
	typedef std::map<F32, editable_param> param_map;

public:
	LLFloaterCustomize();
	virtual ~LLFloaterCustomize();

	// Inherited methods from LLFloater (and above)
	/*virtual*/	BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void draw();
	/*virtual*/ void open();

	// New methods
	void			clearScrollingPanelList();
	void			generateVisualParamHints(LLPanelEditWearable* panel,
											 LLViewerJointMesh* joint_mesh,
											 param_map& params,
											 LLWearable* wearable,
											 bool use_hints);

	void 			updateScrollingPanelList(BOOL allow_modify);

	void			setWearable(LLWearableType::EType type,
								LLWearable* wearable,
								U32 perm_mask,
								BOOL is_complete);

	virtual BOOL	isDirty() const;

	void			askToSaveIfDirty(void(*next_step_callback)(BOOL proceed, void* userdata), void* userdata);

	void			switchToDefaultSubpart();

	static void		updateAvatarHeightDisplay();

	static void		setCurrentWearableType(LLWearableType::EType type);

	static LLWearableType::EType	getCurrentWearableType()	{ return sCurrentWearableType; }
	LLPanelEditWearable*			getCurrentWearablePanel()	{ return mWearablePanelList[sCurrentWearableType]; }

	void			fetchInventory();
	void			updateInventoryUI();
	void			updateScrollingPanelUI();
	void			updateWearableType(LLWearableType::EType type,
									   LLWearable* wearable);

private:
	void			initWearablePanels();
	void			initScrollingPanelList();

	// Callbacks
	static void		onBtnOk(void* userdata);
	static void		onBtnMakeOutfit(void* userdata);

	static void		onTabChanged(void* userdata, bool from_click);
	static void		onTabPrecommit(void* userdata, bool from_click);
	bool			onSaveDialog(const LLSD& notification, const LLSD& response);
	static void		onCommitChangeTab(BOOL proceed, void* userdata);

	static void*	createWearablePanel(void* userdata);

	void			(*mNextStepAfterSaveCallback)(BOOL proceed, void* userdata);

private:
	LLPanelEditWearable*			mWearablePanelList[LLWearableType::WT_COUNT];

	static LLWearableType::EType	sCurrentWearableType;

	LLScrollingPanelList*			mScrollingPanelList;
	LLScrollableContainerView*		mScrollContainer;
	LLVisualParamReset*				mResetParams;

	LLInventoryObserver*			mInventoryObserver;

	void*							mNextStepAfterSaveUserdata;
};

extern LLFloaterCustomize* gFloaterCustomize;

#endif  // LL_LLFLOATERCUSTOMIZE_H
