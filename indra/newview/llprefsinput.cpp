/** 
 * @file llprefsinput.cpp
 * @brief Input preferences panel
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Adapted by Henri Beauchamp from llpanelinput.cpp
 * Copyright (c) 2004-2009 Linden Research, Inc. (c) 2011 Henri Beauchamp
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

#include "llprefsinput.h"

#include "llfloaterjoystick.h"
#include "llsliderctrl.h"
#include "lluictrlfactory.h"

#include "llviewercamera.h"
#include "llviewercontrol.h"

class LLPrefsInputImpl : public LLPanel
{
public:
	LLPrefsInputImpl();
	/*virtual*/ ~LLPrefsInputImpl() { };

	void apply();
	void cancel();

private:
	static void onClickJoystickSetup(void* user_data);
	static void onCommitRadioDoubleClickAction(LLUICtrl* ctrl, void* user_data);
	void refreshValues();

	F32 mMouseSensitivity;
	F32 mCameraAngle;
	F32 mCameraOffsetScale;
	U32 mCameraToPelvisRotDeviation;
	U32 mDoubleClickAction;
	BOOL mDoubleClickScriptedObject;
	BOOL mInvertMouse;
	BOOL mFirstPersonAvatarVisible;
	BOOL mCameraIgnoreCollisions;
	BOOL mEditCameraMovement;
	BOOL mAppearanceCameraMovement;
	BOOL mSitCameraFrontView;
	BOOL mAutomaticFly;
	BOOL mArrowKeysMoveAvatar;
	BOOL mMouseLookUseRotDeviation;
};

static void onFOVAdjust(LLUICtrl* source, void* data)
{
	LLSliderCtrl* slider = dynamic_cast<LLSliderCtrl*>(source);
	LLViewerCamera::getInstance()->setDefaultFOV(slider->getValueF32());
}

LLPrefsInputImpl::LLPrefsInputImpl() 
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_input.xml");
	childSetAction("joystick_setup_button", onClickJoystickSetup, (void*)this);
	childSetCommitCallback("double_click_action", onCommitRadioDoubleClickAction, this);
	LLSliderCtrl* fov_slider = getChild<LLSliderCtrl>("camera_fov");
	fov_slider->setCommitCallback(&onFOVAdjust);
	fov_slider->setMinValue(LLViewerCamera::getInstance()->getMinView());
	fov_slider->setMaxValue(LLViewerCamera::getInstance()->getMaxView());
	fov_slider->setValue(LLViewerCamera::getInstance()->getView());
	refreshValues();
	childSetEnabled("double_click_scripted_object_check", mDoubleClickAction != 0);
}

//static
void LLPrefsInputImpl::onClickJoystickSetup(void* user_data)
{
	LLPrefsInputImpl* prefs = (LLPrefsInputImpl*)user_data;
	LLFloaterJoystick* floaterp = LLFloaterJoystick::showInstance();
	LLFloater* parent_floater = gFloaterView->getParentFloater(prefs);
	if (parent_floater)
	{
		parent_floater->addDependentFloater(floaterp, FALSE);
	}
}

//static
void LLPrefsInputImpl::onCommitRadioDoubleClickAction(LLUICtrl* ctrl,
													  void* user_data)
{
	LLPrefsInputImpl* self = (LLPrefsInputImpl*)user_data;
	if (!self) return;

	bool enable = gSavedSettings.getU32("DoubleClickAction") != 0;
	self->childSetEnabled("double_click_scripted_object_check", enable);
}

void LLPrefsInputImpl::refreshValues()
{
	mCameraAngle				= gSavedSettings.getF32("CameraAngle");
	mCameraOffsetScale			= gSavedSettings.getF32("CameraOffsetScale");
	mMouseSensitivity			= gSavedSettings.getF32("MouseSensitivity");
	mCameraToPelvisRotDeviation	= gSavedSettings.getU32("CameraToPelvisRotDeviation");
	mDoubleClickAction			= gSavedSettings.getU32("DoubleClickAction");
	mDoubleClickScriptedObject	= gSavedSettings.getBOOL("DoubleClickScriptedObject");
	mInvertMouse				= gSavedSettings.getBOOL("InvertMouse");
	mFirstPersonAvatarVisible	= gSavedSettings.getBOOL("FirstPersonAvatarVisible");
	mCameraIgnoreCollisions		= gSavedSettings.getBOOL("CameraIgnoreCollisions");
	mEditCameraMovement			= gSavedSettings.getBOOL("EditCameraMovement");
	mAppearanceCameraMovement	= gSavedSettings.getBOOL("AppearanceCameraMovement");
	mSitCameraFrontView			= gSavedSettings.getBOOL("SitCameraFrontView");
	mAutomaticFly				= gSavedSettings.getBOOL("AutomaticFly");
	mArrowKeysMoveAvatar		= gSavedSettings.getBOOL("ArrowKeysMoveAvatar");
	mMouseLookUseRotDeviation	= gSavedSettings.getBOOL("MouseLookUseRotDeviation");
}

void LLPrefsInputImpl::apply()
{
	refreshValues();
}

void LLPrefsInputImpl::cancel()
{
	LLViewerCamera::getInstance()->setDefaultFOV(mCameraAngle);
	gSavedSettings.setF32("CameraAngle",				LLViewerCamera::getInstance()->getView());
	gSavedSettings.setF32("CameraOffsetScale",			mCameraOffsetScale);
	gSavedSettings.setF32("MouseSensitivity",			mMouseSensitivity);
	gSavedSettings.setU32("CameraToPelvisRotDeviation",	mCameraToPelvisRotDeviation);
	gSavedSettings.setU32("DoubleClickAction",			mDoubleClickAction);
	gSavedSettings.setBOOL("DoubleClickScriptedObject",	mDoubleClickScriptedObject);
	gSavedSettings.setBOOL("InvertMouse",				mInvertMouse);
	gSavedSettings.setBOOL("FirstPersonAvatarVisible",	mFirstPersonAvatarVisible);
	gSavedSettings.setBOOL("CameraIgnoreCollisions",	mCameraIgnoreCollisions);
	gSavedSettings.setBOOL("EditCameraMovement",		mEditCameraMovement);
	gSavedSettings.setBOOL("AppearanceCameraMovement",	mAppearanceCameraMovement);
	gSavedSettings.setBOOL("SitCameraFrontView",		mSitCameraFrontView);
	gSavedSettings.setBOOL("AutomaticFly",				mAutomaticFly);
	gSavedSettings.setBOOL("ArrowKeysMoveAvatar",		mArrowKeysMoveAvatar);
	gSavedSettings.setBOOL("MouseLookUseRotDeviation",	mMouseLookUseRotDeviation);
}

//---------------------------------------------------------------------------

LLPrefsInput::LLPrefsInput()
:	impl(* new LLPrefsInputImpl())
{
}

LLPrefsInput::~LLPrefsInput()
{
	delete &impl;
}

void LLPrefsInput::apply()
{
	impl.apply();
}

void LLPrefsInput::cancel()
{
	impl.cancel();
}

LLPanel* LLPrefsInput::getPanel()
{
	return &impl;
}
