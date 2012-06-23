/** 
 * @file llscrollingpanelparam.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llscrollingpanelparam.h"

#include "llagent.h"
#include "llfloatercustomize.h"
#include "llpaneleditwearable.h"
#include "lltoolmorph.h"
#include "llviewerjointmesh.h"
#include "llviewervisualparam.h"
#include "llvoavatarself.h"
#include "llwearable.h"

//static
S32 LLScrollingPanelParam::sUpdateDelayFrames = 0;

const S32 BTN_BORDER = 2;
const S32 PARAM_HINT_WIDTH = 128;
const S32 PARAM_HINT_HEIGHT = 128;
const S32 PARAM_HINT_LABEL_HEIGHT = 16;
const S32 PARAM_PANEL_WIDTH = 2 * (3 * BTN_BORDER + PARAM_HINT_WIDTH +  LLPANEL_BORDER_WIDTH);
const S32 PARAM_PANEL_HEIGHT = 2 * BTN_BORDER + PARAM_HINT_HEIGHT + PARAM_HINT_LABEL_HEIGHT + 4 * LLPANEL_BORDER_WIDTH; 
const F32 PARAM_STEP_TIME_THRESHOLD = 0.25f;

LLScrollingPanelParam::LLScrollingPanelParam(LLPanelEditWearable* panel,
											 LLViewerJointMesh* mesh,
											 LLViewerVisualParam* param,
											 BOOL allow_modify,
											 LLWearable* wearable,
											 BOOL use_hints)
:	LLScrollingPanel("LLScrollingPanelParam",
					 LLRect(0, PARAM_PANEL_HEIGHT, PARAM_PANEL_WIDTH, 0)),
	mPanelParams(panel),
	mWearable(wearable),
	mParam(param),
	mAllowModify(allow_modify),
	mLess(NULL),
	mMore(NULL),
	mHintMin(NULL),
	mHintMax(NULL),
	mSlider(NULL),
	mMinParamText(NULL),
	mMaxParamText(NULL)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_scrolling_param.xml");

	// Set up the slider
	mSlider = getChild<LLSliderCtrl>("param slider", TRUE, FALSE);
	if (mSlider)
	{
		mSlider->setValue(weightToPercent(param->getWeight()));
		mSlider->setLabelArg("[DESC]", param->getDisplayName());
		mSlider->setEnabled(mAllowModify);
		mSlider->setCommitCallback(LLScrollingPanelParam::onSliderMoved);
		mSlider->setCallbackUserData(this);
	}

	if (use_hints)
	{
		S32 pos_x = 2 * LLPANEL_BORDER_WIDTH;
		S32 pos_y = 3 * LLPANEL_BORDER_WIDTH + SLIDERCTRL_HEIGHT;
		F32 min_weight = param->getMinWeight();
		F32 max_weight = param->getMaxWeight();

		LLViewerVisualParam* wearable_param;
		if (wearable)
		{
			wearable_param = (LLViewerVisualParam*)wearable->getVisualParam(param->getID());
		}
		else
		{
			llwarns << "wearable is NULL !... Using viewer visualparam."
					<< llendl;
			wearable_param = param;
		}

		mHintMin = new LLVisualParamHint(pos_x, pos_y,
										 PARAM_HINT_WIDTH, PARAM_HINT_HEIGHT,
										 mesh, wearable_param, wearable,
										 min_weight);
		pos_x += PARAM_HINT_WIDTH + 3 * BTN_BORDER;
		mHintMax = new LLVisualParamHint(pos_x, pos_y,
										 PARAM_HINT_WIDTH, PARAM_HINT_HEIGHT,
										 mesh, wearable_param, wearable,
										 max_weight);

		mHintMin->setAllowsUpdates(FALSE);
		mHintMax->setAllowsUpdates(FALSE);

		mMinParamText = getChild<LLTextBox>("min param text", TRUE, FALSE);
		if (mMinParamText)
		{
			// *TODO::translate
			std::string min_name = param->getMinDisplayName();
			mMinParamText->setValue(min_name);
		}
		mMaxParamText = getChild<LLTextBox>("max param text", TRUE, FALSE);
		if (mMaxParamText)
		{
			// *TODO::translate
			std::string max_name = param->getMaxDisplayName();
			mMaxParamText->setValue(max_name);
		}

		mLess = getChild<LLButton>("less", TRUE, FALSE);
		if (mLess)
		{
			mLess->setMouseDownCallback(LLScrollingPanelParam::onHintMinMouseDown);
			mLess->setMouseUpCallback(LLScrollingPanelParam::onHintMinMouseUp);
			mLess->setHeldDownCallback(LLScrollingPanelParam::onHintMinHeldDown);
			mLess->setHeldDownDelay(PARAM_STEP_TIME_THRESHOLD);
		}

		mMore = getChild<LLButton>("more", TRUE, FALSE);
		if (mMore)
		{
			mMore->setMouseDownCallback(LLScrollingPanelParam::onHintMaxMouseDown);
			mMore->setMouseUpCallback(LLScrollingPanelParam::onHintMaxMouseUp);
			mMore->setHeldDownCallback(LLScrollingPanelParam::onHintMaxHeldDown);
			mMore->setHeldDownDelay(PARAM_STEP_TIME_THRESHOLD);
		}
	}
	else
	{
		// Kill everything that isn't the slider...
		child_list_t to_remove;
		child_list_t::const_iterator it;
		for (it = getChildList()->begin(); it != getChildList()->end(); it++)
		{
			if (*it != mSlider && (*it)->getName() != "panel border")
			{
				to_remove.push_back(*it);
			}
		}
		for (it = to_remove.begin(); it != to_remove.end(); it++)
		{
			removeChild(*it, TRUE);
		}
		if (mSlider)
		{
			mSlider->translate(0, PARAM_HINT_HEIGHT);
		}
		reshape(getRect().getWidth(), getRect().getHeight() - PARAM_HINT_HEIGHT);
	}

	setVisible(FALSE);
	setBorderVisible(FALSE);
}

LLScrollingPanelParam::~LLScrollingPanelParam()
{
	mHintMin = NULL;
	mHintMax = NULL;
}

void LLScrollingPanelParam::updatePanel(BOOL allow_modify)
{
	if (mWearable != mPanelParams->getWearable())
	{
		LL_DEBUGS("Appearance") << "Wearable change detected for parameter "
								<< mParam->getID() << LL_ENDL;
		// The wearable changed...
		mWearable = mPanelParams->getWearable();
		if (mWearable && mHintMin)
		{
			LL_DEBUGS("Appearance") << "Updating visual hints for parameter "
									<< mParam->getID() << LL_ENDL;
			LLViewerVisualParam* wearable_param;
			wearable_param = (LLViewerVisualParam*)mWearable->getVisualParam(mParam->getID());
			mHintMin->setWearable(mWearable, wearable_param);
			mHintMax->setWearable(mWearable, wearable_param);
			LLVisualParamHint::requestHintUpdates(mHintMin, mHintMax);
		}
	}
	if (!mWearable)
	{
		setVisible(FALSE);
		// not editing a wearable just now, no update necessary
		return;
	}
	if (mSlider)
	{
		F32 current_weight = mWearable->getVisualParamWeight(mParam->getID());
		mSlider->setValue(weightToPercent(current_weight));
	}
	if (mHintMin)
	{
		mHintMin->requestUpdate(sUpdateDelayFrames++);
		mHintMax->requestUpdate(sUpdateDelayFrames++);
	}

	mAllowModify = allow_modify;
	if (mSlider)
	{
		mSlider->setEnabled(mAllowModify);
	}
	if (mLess)
	{
		mLess->setEnabled(mAllowModify);
	}
	if (mMore)
	{
		mMore->setEnabled(mAllowModify);
	}
}

void LLScrollingPanelParam::setVisible(BOOL visible)
{
	if (getVisible() != visible)
	{
		LLPanel::setVisible(visible);
		setBorderVisible(FALSE);
		setMouseOpaque(visible);
		if (mHintMin)
		{
			mHintMin->setAllowsUpdates(visible);
			mHintMax->setAllowsUpdates(visible);
			if (visible)
			{
				mHintMin->setUpdateDelayFrames(sUpdateDelayFrames++);
				mHintMax->setUpdateDelayFrames(sUpdateDelayFrames++);
			}
		}
	}
}

void LLScrollingPanelParam::draw()
{
	if (gFloaterCustomize->isMinimized() || !mWearable)
	{
		return;
	}

	if (mLess)
	{
		mLess->setVisible(mHintMin ? mHintMin->getVisible() : false);
	}
	if (mMore)
	{
		mMore->setVisible(mHintMax ? mHintMax->getVisible() : false);
	}

	// Draw all the children except for the labels
	if (mMinParamText)
	{
		mMinParamText->setVisible(FALSE);
	}
	if (mMaxParamText)
	{
		mMaxParamText->setVisible(FALSE);
	}
	LLPanel::draw();

	// Draw the hints over the "less" and "more" buttons.
	if (mHintMin)
	{
		glPushMatrix();
		{
			const LLRect& r = mHintMin->getRect();
			F32 left = (F32)(r.mLeft + BTN_BORDER);
			F32 bot  = (F32)(r.mBottom + BTN_BORDER);
			glTranslatef(left, bot, 0.f);
			mHintMin->draw();
		}
		glPopMatrix();
		glPushMatrix();
		{
			const LLRect& r = mHintMax->getRect();
			F32 left = (F32)(r.mLeft + BTN_BORDER);
			F32 bot  = (F32)(r.mBottom + BTN_BORDER);
			glTranslatef(left, bot, 0.f);
			mHintMax->draw();
		}
		glPopMatrix();
	}

	// Draw labels on top of the buttons
	if (mMinParamText)
	{
		mMinParamText->setVisible(TRUE);
		drawChild(mMinParamText, BTN_BORDER, BTN_BORDER);
	}
	if (mMaxParamText)
	{
		mMaxParamText->setVisible(TRUE);
		drawChild(mMaxParamText, BTN_BORDER, BTN_BORDER);
	}
}

// static
void LLScrollingPanelParam::onSliderMoved(LLUICtrl* ctrl, void* userdata)
{
	LLSliderCtrl* slider = (LLSliderCtrl*)ctrl;
	LLScrollingPanelParam* self = (LLScrollingPanelParam*)userdata;
	if (self && self->mWearable && slider && isAgentAvatarValid())
	{
		LLViewerVisualParam* param = self->mParam;
		F32 current_weight = self->mWearable->getVisualParamWeight(param->getID());
		F32 new_weight = self->percentToWeight((F32)slider->getValue().asReal());
		if (current_weight != new_weight)
		{
			self->mWearable->setVisualParamWeight(param->getID(),
												  new_weight, FALSE);
			self->mWearable->writeToAvatar();
			gAgentAvatarp->updateVisualParams();
			if (gFloaterCustomize)
			{
				gFloaterCustomize->updateAvatarHeightDisplay();
			}
		}
	}
}

// static
void LLScrollingPanelParam::onSliderMouseDown(LLUICtrl* ctrl, void* userdata)
{
}

// static
void LLScrollingPanelParam::onSliderMouseUp(LLUICtrl* ctrl, void* userdata)
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	if (self)
	{
		LLVisualParamHint::requestHintUpdates(self->mHintMin, self->mHintMax);
	}
}

// static
void LLScrollingPanelParam::onHintMinMouseDown(void* userdata)
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	if (self)
	{
		self->onHintMouseDown(self->mHintMin);
	}
}

// static
void LLScrollingPanelParam::onHintMaxMouseDown(void* userdata)
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	if (self)
	{
		self->onHintMouseDown(self->mHintMax);
	}
}

void LLScrollingPanelParam::onHintMouseDown(LLVisualParamHint* hint)
{
	if (hint && mWearable && isAgentAvatarValid())
	{
		// morph towards this result
		F32 current_weight = mWearable->getVisualParamWeight(hint->getVisualParam()->getID());

		// if we have maxed out on this morph, we shouldn't be able to click it
		if (hint->getVisualParamWeight() != current_weight)
		{
			mMouseDownTimer.reset();
			mLastHeldTime = 0.f;
		}
	}
}

// static
void LLScrollingPanelParam::onHintMinHeldDown(void* userdata)
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	if (self)
	{
		self->onHintHeldDown(self->mHintMin);
	}
}

// static
void LLScrollingPanelParam::onHintMaxHeldDown(void* userdata)
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	if (self)
	{
		self->onHintHeldDown(self->mHintMax);
	}
}

void LLScrollingPanelParam::onHintHeldDown(LLVisualParamHint* hint)
{
	if (!hint || !mWearable || !isAgentAvatarValid()) return;

	F32 current_weight = mWearable->getVisualParamWeight(hint->getVisualParam()->getID());

	if (current_weight != hint->getVisualParamWeight())
	{
		const F32 FULL_BLEND_TIME = 2.f;
		F32 elapsed_time = mMouseDownTimer.getElapsedTimeF32() - mLastHeldTime;
		mLastHeldTime += elapsed_time;

		F32 new_weight;
		if (current_weight > hint->getVisualParamWeight())
		{
			new_weight = current_weight - elapsed_time / FULL_BLEND_TIME;
		}
		else
		{
			new_weight = current_weight + elapsed_time / FULL_BLEND_TIME;
		}

		// Make sure we're not taking the slider out of bounds
		// (this is where some simple UI limits are stored)
		F32 new_percent = weightToPercent(new_weight);
		if (mSlider)
		{
			if (mSlider->getMinValue() < new_percent &&
				new_percent < mSlider->getMaxValue())
			{
				mWearable->setVisualParamWeight(hint->getVisualParam()->getID(),
												new_weight, FALSE);
				mWearable->writeToAvatar();
				gAgentAvatarp->updateVisualParams();

				mSlider->setValue(weightToPercent(new_weight));
			}
		}
	}
}

// static
void LLScrollingPanelParam::onHintMinMouseUp(void* userdata)
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	if (self && self->mWearable && isAgentAvatarValid())
	{
		F32 elapsed_time = self->mMouseDownTimer.getElapsedTimeF32();
		LLVisualParamHint* hint = self->mHintMin;

		if (hint && elapsed_time < PARAM_STEP_TIME_THRESHOLD)
		{
			// step in direction
			F32 current_weight = self->mWearable->getVisualParamWeight(hint->getVisualParam()->getID());
			F32 range = self->mHintMax->getVisualParamWeight() -
						self->mHintMin->getVisualParamWeight();
			// step a fraction in the negative direction
			F32 new_weight = current_weight - range / 10.f;
			F32 new_percent = self->weightToPercent(new_weight);
			if (self->mSlider)
			{
				if (self->mSlider->getMinValue() < new_percent &&
					new_percent < self->mSlider->getMaxValue())
				{
					self->mWearable->setVisualParamWeight(hint->getVisualParam()->getID(),
														  new_weight, TRUE);
					self->mWearable->writeToAvatar();
					self->mSlider->setValue(self->weightToPercent(new_weight));
				}
			}
		}
		LLVisualParamHint::requestHintUpdates(self->mHintMin, self->mHintMax);
	}
}

void LLScrollingPanelParam::onHintMaxMouseUp(void* userdata)
{
	LLScrollingPanelParam* self = (LLScrollingPanelParam*) userdata;
	if (self && self->mWearable && isAgentAvatarValid())
	{
		F32 elapsed_time = self->mMouseDownTimer.getElapsedTimeF32();
		LLVisualParamHint* hint = self->mHintMax;

		if (hint && elapsed_time < PARAM_STEP_TIME_THRESHOLD)
		{
			// step in direction
			F32 current_weight = self->mWearable->getVisualParamWeight(hint->getVisualParam()->getID());
			F32 range = self->mHintMax->getVisualParamWeight() -
						self->mHintMin->getVisualParamWeight();
			// step a fraction in the negative direction
			F32 new_weight = current_weight + range / 10.f;
			F32 new_percent = self->weightToPercent(new_weight);
			if (self->mSlider)
			{
				if (self->mSlider->getMinValue() < new_percent &&
					new_percent < self->mSlider->getMaxValue())
				{
					self->mWearable->setVisualParamWeight(hint->getVisualParam()->getID(),
														  new_weight, TRUE);
					self->mWearable->writeToAvatar();
					self->mSlider->setValue(self->weightToPercent(new_weight));
				}
			}
		}
		LLVisualParamHint::requestHintUpdates(self->mHintMin, self->mHintMax);
	}
}

F32 LLScrollingPanelParam::weightToPercent(F32 weight)
{
	LLViewerVisualParam* param = mParam;
	return (weight - param->getMinWeight()) /
		   (param->getMaxWeight() - param->getMinWeight()) * 100.f;
}

F32 LLScrollingPanelParam::percentToWeight(F32 percent)
{
	LLViewerVisualParam* param = mParam;
	return percent / 100.f * (param->getMaxWeight() - param->getMinWeight()) +
		   param->getMinWeight();
}
