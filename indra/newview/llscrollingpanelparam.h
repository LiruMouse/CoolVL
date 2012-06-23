/** 
 * @file llscrollingpanelparam.h
 * @brief the scrolling panel containing a list of visual param 
 *  	  panels
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

#ifndef LL_SCROLLINGPANELPARAM_H
#define LL_SCROLLINGPANELPARAM_H

#include "llbutton.h"
#include "llscrollingpanellist.h"
#include "llsliderctrl.h"
#include "lltextbox.h"

class LLViewerJointMesh;
class LLViewerVisualParam;
class LLWearable;
class LLVisualParamHint;
class LLViewerVisualParam;
class LLPanelEditWearable;

class LLScrollingPanelParam : public LLScrollingPanel
{
public:
	LLScrollingPanelParam(LLPanelEditWearable* panel,
						  LLViewerJointMesh* mesh,
						  LLViewerVisualParam* param,
						  BOOL allow_modify,
						  LLWearable* wearable,
						  BOOL use_hints = TRUE);
	virtual ~LLScrollingPanelParam();

	virtual void		draw();
	virtual void		setVisible(BOOL visible);
	virtual void		updatePanel(BOOL allow_modify);

	static void			onSliderMouseDown(LLUICtrl* ctrl, void* userdata);
	static void			onSliderMoved(LLUICtrl* ctrl, void* userdata);
	static void			onSliderMouseUp(LLUICtrl* ctrl, void* userdata);

	static void			onHintMinMouseDown(void* userdata);
	static void			onHintMinHeldDown(void* userdata);
	static void			onHintMaxMouseDown(void* userdata);
	static void			onHintMaxHeldDown(void* userdata);
	static void			onHintMinMouseUp(void* userdata);
	static void			onHintMaxMouseUp(void* userdata);

	void				onHintMouseDown(LLVisualParamHint* hint);
	void				onHintHeldDown(LLVisualParamHint* hint);

	F32					weightToPercent(F32 weight);
	F32					percentToWeight(F32 percent);

public:
	LLPanelEditWearable*			mPanelParams;
	LLWearable*						mWearable;
	LLViewerVisualParam* 			mParam;
	LLPointer<LLVisualParamHint>	mHintMin;
	LLPointer<LLVisualParamHint>	mHintMax;
	LLButton*						mLess;
	LLButton*						mMore;
	LLSliderCtrl*					mSlider;
	LLTextBox*						mMinParamText;
	LLTextBox*						mMaxParamText;

	static S32 				sUpdateDelayFrames;

protected:
	LLTimer				mMouseDownTimer;	// timer for how long mouse has been held down on a hint.
	F32					mLastHeldTime;

	BOOL				mAllowModify;
};

#endif
