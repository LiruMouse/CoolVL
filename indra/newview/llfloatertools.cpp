/** 
 * @file llfloatertools.cpp
 * @brief The edit tools, including move, position, land, etc.
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

#include "llfloatertools.h"

#include "llbutton.h"
#include "llcoord.h"
#include "lldraghandle.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llmenugl.h"
#include "llslider.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterobjectweights.h"
#include "llfloateropenobject.h"
#include "llmeshrepository.h"
#include "llpanelcontents.h"
#include "llpanelface.h"
#include "llpanelinventory.h"
#include "llpanelland.h"
#include "llpanelobject.h"
#include "llpanelpermissions.h"
#include "llpanelvolume.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "qltoolalign.h"
#include "lltoolbrush.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolindividual.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltoolpipette.h"
#include "lltoolplacer.h"
#include "lltoolselectland.h"
#include "llviewercontrol.h"
#include "llviewerjoystick.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvograss.h"
#include "llvotree.h"

// Globals
LLFloaterTools* gFloaterTools = NULL;

const std::string PANEL_NAMES[LLFloaterTools::PANEL_COUNT] =
{
	std::string("General"), 	// PANEL_GENERAL,
	std::string("Object"), 	// PANEL_OBJECT,
	std::string("Features"),	// PANEL_FEATURES,
	std::string("Texture"),	// PANEL_FACE,
	std::string("Content"),	// PANEL_CONTENTS,
};

// Local prototypes
void commit_select_tool(LLUICtrl *ctrl, void *data);
void commit_select_component(LLUICtrl *ctrl, void *data);
void click_show_more(void*);
void click_popup_info(void*);
void click_popup_done(void*);
void click_popup_minimize(void*);
void click_popup_grab_drag(LLUICtrl *, void*);
void click_popup_grab_lift(LLUICtrl *, void*);
void click_popup_grab_spin(LLUICtrl *, void*);
void click_popup_rotate_left(void*);
void click_popup_rotate_reset(void*);
void click_popup_rotate_right(void*);
void click_popup_dozer_mode(LLUICtrl *, void *user);
void commit_slider_dozer_size(LLUICtrl *, void*);
void commit_slider_dozer_force(LLUICtrl *, void*);
void click_apply_to_selection(void*);
void commit_radio_zoom(LLUICtrl *, void*);
void commit_radio_orbit(LLUICtrl *, void*);
void commit_radio_pan(LLUICtrl *, void*);
void commit_grid_mode(LLUICtrl *, void*);
void commit_slider_zoom(LLUICtrl *, void*);
void click_count(void*);

//static
void* LLFloaterTools::createPanelPermissions(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelPermissions = new LLPanelPermissions("General");
	return floater->mPanelPermissions;
}
//static
void* LLFloaterTools::createPanelObject(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelObject = new LLPanelObject("Object");
	return floater->mPanelObject;
}

//static
void* LLFloaterTools::createPanelVolume(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelVolume = new LLPanelVolume("Features");
	return floater->mPanelVolume;
}

//static
void* LLFloaterTools::createPanelFace(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelFace = new LLPanelFace("Texture");
	return floater->mPanelFace;
}

//static
void* LLFloaterTools::createPanelContents(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelContents = new LLPanelContents("Contents");
	return floater->mPanelContents;
}

//static
void* LLFloaterTools::createPanelContentsInventory(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelContents->mPanelInventory = new LLPanelInventory(std::string("ContentsInventory"), LLRect());
	return floater->mPanelContents->mPanelInventory;
}

//static
void* LLFloaterTools::createPanelLandInfo(void* data)
{
	LLFloaterTools* floater = (LLFloaterTools*)data;
	floater->mPanelLandInfo = new LLPanelLandInfo(std::string("land info panel"));
	return floater->mPanelLandInfo;
}

void LLFloaterTools::toolsPrecision()
{
	static LLCachedControl<U32> decimals(gSavedSettings, "DecimalsForTools");
	if (mPrecision != decimals)
	{
		mPrecision = decimals;
		if (mPrecision > 5)
		{
			mPrecision = 5;
		}
		getChild<LLSpinCtrl>("Pos X")->setPrecision(mPrecision);
		getChild<LLSpinCtrl>("Pos Y")->setPrecision(mPrecision);
		getChild<LLSpinCtrl>("Pos Z")->setPrecision(mPrecision);
		getChild<LLSpinCtrl>("Scale X")->setPrecision(mPrecision);
		getChild<LLSpinCtrl>("Scale Y")->setPrecision(mPrecision);
		getChild<LLSpinCtrl>("Scale Z")->setPrecision(mPrecision);
		getChild<LLSpinCtrl>("Rot X")->setPrecision(mPrecision);
		getChild<LLSpinCtrl>("Rot Y")->setPrecision(mPrecision);
		getChild<LLSpinCtrl>("Rot Z")->setPrecision(mPrecision);
	}
}

//virtual
BOOL LLFloaterTools::postBuild()
{
	// Hide until tool selected
	setVisible(FALSE);

	// Since we constantly show and hide this during drags, don't make sounds
	// on visibility changes.
	setSoundFlags(LLView::SILENT);

	getDragHandle()->setEnabled(!gSavedSettings.getBOOL("ToolboxAutoMove"));

	LLRect rect;
	mBtnFocus = getChild<LLButton>("button focus");
	mBtnFocus->setClickedCallback(setEditTool,
								  (void*)LLToolCamera::getInstance());

	mBtnMove = getChild<LLButton>("button move");
	mBtnMove->setClickedCallback(setEditTool,
								 (void*)LLToolGrab::getInstance());

	mBtnEdit = getChild<LLButton>("button edit");
	mBtnEdit->setClickedCallback(setEditTool,
								 (void*)LLToolCompTranslate::getInstance());

	mBtnCreate = getChild<LLButton>("button create");
	mBtnCreate->setClickedCallback(setEditTool,
								   (void*)LLToolCompCreate::getInstance());

	mBtnLand = getChild<LLButton>("button land");
	mBtnLand->setClickedCallback(setEditTool,
								 (void*)LLToolSelectLand::getInstance());

	mTextStatus = getChild<LLTextBox>("text status");

	mRadioZoom = getChild<LLCheckBoxCtrl>("radio zoom");
	mRadioZoom->setCommitCallback(commit_radio_zoom);
	mRadioZoom->setCallbackUserData(this);

	mRadioOrbit = getChild<LLCheckBoxCtrl>("radio orbit");
	mRadioOrbit->setCommitCallback(commit_radio_orbit);
	mRadioOrbit->setCallbackUserData(this);

	mRadioPan = getChild<LLCheckBoxCtrl>("radio pan");
	mRadioPan->setCommitCallback(commit_radio_pan);
	mRadioPan->setCallbackUserData(this);

	mSliderZoom = getChild<LLSlider>("slider zoom");
	mSliderZoom->setCommitCallback(commit_slider_zoom);
	mSliderZoom->setCallbackUserData(this);

	mRadioMove = getChild<LLCheckBoxCtrl>("radio move");
	mRadioMove->setCommitCallback(click_popup_grab_drag);
	mRadioMove->setCallbackUserData(this);

	mRadioLift = getChild<LLCheckBoxCtrl>("radio lift");
	mRadioLift->setCommitCallback(click_popup_grab_lift);
	mRadioLift->setCallbackUserData(this);

	mRadioSpin = getChild<LLCheckBoxCtrl>("radio spin");
	mRadioSpin->setCommitCallback(click_popup_grab_spin);
	mRadioSpin->setCallbackUserData(this);

	mRadioPosition = getChild<LLCheckBoxCtrl>("radio position");
	mRadioPosition->setCommitCallback(commit_select_tool);
	mRadioPosition->setCallbackUserData((void*)LLToolCompTranslate::getInstance());

	mRadioAlign = getChild<LLCheckBoxCtrl>("radio align");
	mRadioAlign->setCommitCallback(commit_select_tool);
	mRadioAlign->setCallbackUserData((void*)QLToolAlign::getInstance());

	mRadioRotate = getChild<LLCheckBoxCtrl>("radio rotate");
	mRadioRotate->setCommitCallback(commit_select_tool);
	mRadioRotate->setCallbackUserData((void*)LLToolCompRotate::getInstance());

	mRadioStretch = getChild<LLCheckBoxCtrl>("radio stretch");
	mRadioStretch->setCommitCallback(commit_select_tool);
	mRadioStretch->setCallbackUserData((void*)LLToolCompScale::getInstance());

	mRadioSelectFace = getChild<LLCheckBoxCtrl>("radio select face");
	mRadioSelectFace->setCommitCallback(commit_select_tool);
	mRadioSelectFace->setCallbackUserData((void*)LLToolFace::getInstance());

	mCheckSelectIndividual = getChild<LLCheckBoxCtrl>("checkbox edit linked parts");
	mCheckSelectIndividual->setCommitCallback(commit_select_component);
	mCheckSelectIndividual->setCallbackUserData(this);
	mCheckSelectIndividual->setValue((BOOL)gSavedSettings.getBOOL("EditLinkedParts"));

	mBtnGridOptions = getChild<LLButton>("Grid Options");
	mBtnGridOptions->setClickedCallback(onClickGridOptions, this);

	mCheckStretchUniform = getChild<LLCheckBoxCtrl>("checkbox uniform");
	mCheckStretchUniform->setValue((BOOL)gSavedSettings.getBOOL("ScaleUniform"));

	mCheckStretchTexture = getChild<LLCheckBoxCtrl>("checkbox stretch textures");
	mCheckStretchTexture->setValue((BOOL)gSavedSettings.getBOOL("ScaleStretchTextures"));

	mCheckLimitDrag = getChild<LLCheckBoxCtrl>("checkbox limit drag distance");
	mCheckLimitDrag->setValue((BOOL)gSavedSettings.getBOOL("LimitDragDistance"));

	mTextGridMode = getChild<LLTextBox>("text ruler mode");

	mComboGridMode = getChild<LLComboBox>("combobox grid mode");
	mComboGridMode->setCommitCallback(commit_grid_mode);
	mComboGridMode->setCallbackUserData(this);

	mBtnLink = getChild<LLButton>("Link");
	mBtnLink->setClickedCallback(onClickLink, this);

	mBtnUnlink = getChild<LLButton>("Unlink");
	mBtnUnlink->setClickedCallback(onClickUnlink, this);

	mTextObjectCount = getChild<LLTextBox>("obj_count");
	mTextObjectCount->setClickedCallback(click_count, this);
	mTextPrimCount = getChild<LLTextBox>("prim_count");
	mTextPrimCount->setClickedCallback(click_count, this);

	toolsPrecision();

	//
	// Create Buttons
	//

	static const std::string toolNames[]= {
			"ToolCube",
			"ToolPrism",
			"ToolPyramid",
			"ToolTetrahedron",
			"ToolCylinder",
			"ToolHemiCylinder",
			"ToolCone",
			"ToolHemiCone",
			"ToolSphere",
			"ToolHemiSphere",
			"ToolTorus",
			"ToolTube",
			"ToolRing",
			"ToolTree",
			"ToolGrass"
		};

	void* toolData[] = {
			&LLToolPlacerPanel::sCube,
			&LLToolPlacerPanel::sPrism,
			&LLToolPlacerPanel::sPyramid,
			&LLToolPlacerPanel::sTetrahedron,
			&LLToolPlacerPanel::sCylinder,
			&LLToolPlacerPanel::sCylinderHemi,
			&LLToolPlacerPanel::sCone,
			&LLToolPlacerPanel::sConeHemi,
			&LLToolPlacerPanel::sSphere,
			&LLToolPlacerPanel::sSphereHemi,
			&LLToolPlacerPanel::sTorus,
			&LLToolPlacerPanel::sSquareTorus,
			&LLToolPlacerPanel::sTriangleTorus,
			&LLToolPlacerPanel::sTree,
			&LLToolPlacerPanel::sGrass
		};

	for (size_t t = 0; t < LL_ARRAY_SIZE(toolNames); ++t)
	{
		LLButton* found = getChild<LLButton>(toolNames[t]);
		if (found)
		{
			found->setClickedCallback(setObjectType, toolData[t]);
			mButtons.push_back(found);
		}
		else
		{
			llwarns << "Tool button not found! DOA Pending." << llendl;
		}
	}

	mCheckCopySelection = getChild<LLCheckBoxCtrl>("checkbox copy selection");
	mCheckCopySelection->setValue(gSavedSettings.getBOOL("CreateToolCopySelection"));

	mCheckSticky = getChild<LLCheckBoxCtrl>("checkbox sticky");
	mCheckSticky->setValue(gSavedSettings.getBOOL("CreateToolKeepSelected"));

	mCheckCopyCenters = getChild<LLCheckBoxCtrl>("checkbox copy centers");
	mCheckCopyCenters->setValue(gSavedSettings.getBOOL("CreateToolCopyCenters"));

	mCheckCopyRotates = getChild<LLCheckBoxCtrl>("checkbox copy rotates");
	mCheckCopyRotates->setValue(gSavedSettings.getBOOL("CreateToolCopyRotates"));

	mRadioSelectLand = getChild<LLCheckBoxCtrl>("radio select land");
	mRadioSelectLand->setCommitCallback(commit_select_tool);
	mRadioSelectLand->setCallbackUserData((void*)LLToolSelectLand::getInstance());

	mRadioDozerFlatten = getChild<LLCheckBoxCtrl>("radio flatten");
	mRadioDozerFlatten->setCommitCallback(click_popup_dozer_mode);
	mRadioDozerFlatten->setCallbackUserData((void*)0);

	mRadioDozerRaise = getChild<LLCheckBoxCtrl>("radio raise");
	mRadioDozerRaise->setCommitCallback(click_popup_dozer_mode);
	mRadioDozerRaise->setCallbackUserData((void*)1);

	mRadioDozerLower = getChild<LLCheckBoxCtrl>("radio lower");
	mRadioDozerLower->setCommitCallback(click_popup_dozer_mode);
	mRadioDozerLower->setCallbackUserData((void*)2);

	mRadioDozerSmooth = getChild<LLCheckBoxCtrl>("radio smooth");
	mRadioDozerSmooth->setCommitCallback(click_popup_dozer_mode);
	mRadioDozerSmooth->setCallbackUserData((void*)3);

	mRadioDozerNoise = getChild<LLCheckBoxCtrl>("radio noise");
	mRadioDozerNoise->setCommitCallback(click_popup_dozer_mode);
	mRadioDozerNoise->setCallbackUserData((void*)4);

	mRadioDozerRevert = getChild<LLCheckBoxCtrl>("radio revert");
	mRadioDozerRevert->setCommitCallback(click_popup_dozer_mode);
	mRadioDozerRevert->setCallbackUserData((void*)5);

	mBtnApplyToSelection = getChild<LLButton>("button apply to selection");
	mBtnApplyToSelection->setClickedCallback(click_apply_to_selection, NULL);

	mSliderDozerSize = getChild<LLSlider>("slider brush size");
	mSliderDozerSize->setCommitCallback(commit_slider_dozer_size);
	mSliderDozerSize->setValue(gSavedSettings.getF32("LandBrushSize"));

	mSliderDozerForce = getChild<LLSlider>("slider force");
	mSliderDozerSize->setCommitCallback(commit_slider_dozer_force);
	// the setting stores the actual force multiplier, but the slider is
	// logarithmic, so we convert here
	mSliderDozerSize->setValue((F32)log10(gSavedSettings.getF32("LandBrushForce")));

	mTextBulldozer = getChild<LLTextBox>("Bulldozer:");
	mTextDozerSize = getChild<LLTextBox>("Dozer Size:");
	mTextStrength = getChild<LLTextBox>("Strength:");

	mComboTreesGrass = getChild<LLComboBox>("tree_grass");
	mComboTreesGrass->setCommitCallback(onSelectTreesGrass);
	mComboTreesGrass->setCallbackUserData(this);

	mTextTreeGrass = getChild<LLTextBox>("tree_grass_label");

	mBtnToolTree = getChild<LLButton>("ToolTree");
	mBtnToolGrass = getChild<LLButton>("ToolGrass");

	mTab = getChild<LLTabContainer>("Object Info Tabs");
	mTab->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
	mTab->setBorderVisible(FALSE);
	mTab->selectFirstTab();

	mStatusText["rotate"] = getString("status_rotate");
	mStatusText["scale"] = getString("status_scale");
	mStatusText["move"] = getString("status_move");
	mStatusText["modifyland"] = getString("status_modifyland");
	mStatusText["camera"] = getString("status_camera");
	mStatusText["grab"] = getString("status_grab");
	mStatusText["place"] = getString("status_place");
	mStatusText["selectland"] = getString("status_selectland");

	mGridScreenText = getString("grid_screen_text");
	mGridLocalText = getString("grid_local_text");
	mGridWorldText = getString("grid_world_text");
	mGridReferenceText = getString("grid_reference_text");
	mGridAttachmentText = getString("grid_attachment_text");

	return TRUE;
}

// Create the popupview with a dummy center.  It will be moved into place
// during LLViewerWindow's per-frame hover processing.
LLFloaterTools::LLFloaterTools()
:	LLFloater(std::string("toolbox floater")),
	mDirty(true),
	mPrecision(3),
	mLastObjectCount(-1),
	mLastPrimCount(-1),
	mLastLandImpact(-1)
{
	setAutoFocus(FALSE);
	LLCallbackMap::map_t factory_map;
	factory_map["General"] = LLCallbackMap(createPanelPermissions, this);		// LLPanelPermissions
	factory_map["Object"] = LLCallbackMap(createPanelObject, this);				// LLPanelObject
	factory_map["Features"] = LLCallbackMap(createPanelVolume, this);			// LLPanelVolume
	factory_map["Texture"] = LLCallbackMap(createPanelFace, this);				// LLPanelFace
	factory_map["Contents"] = LLCallbackMap(createPanelContents, this);			// LLPanelContents
	factory_map["ContentsInventory"] = LLCallbackMap(createPanelContentsInventory,
													 this);						// LLPanelContents
	factory_map["land info panel"] = LLCallbackMap(createPanelLandInfo, this);	// LLPanelLandInfo

	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_tools.xml",
												 &factory_map, FALSE);
}

//virtual
LLFloaterTools::~LLFloaterTools()
{
	llinfos << "Floater Tools destroyed" << llendl;
	// children automatically deleted
}

void LLFloaterTools::setStatusText(const std::string& text)
{
	std::map<std::string, std::string>::iterator iter = mStatusText.find(text);
	if (iter != mStatusText.end())
	{
		mTextStatus->setText(iter->second);
	}
	else
	{
		mTextStatus->setText(text);
	}
}

//virtual
void LLFloaterTools::refresh()
{
	const S32 INFO_WIDTH = getRect().getWidth();
	const S32 INFO_HEIGHT = 384;
	LLRect object_info_rect(0, 0, INFO_WIDTH, -INFO_HEIGHT);
	BOOL all_volume = LLSelectMgr::getInstance()->selectionAllPCode(LL_PCODE_VOLUME);

	S32 idx_features = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_FEATURES]);
	S32 idx_face = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_FACE]);
	S32 idx_contents = mTab->getPanelIndexByTitle(PANEL_NAMES[PANEL_CONTENTS]);

	S32 selected_index = mTab->getCurrentPanelIndex();

	if (!all_volume &&
		(selected_index == idx_features || selected_index == idx_face ||
		 selected_index == idx_contents))
	{
		mTab->selectFirstTab();
	}

	mTab->enableTabButton(idx_features, all_volume);
	mTab->enableTabButton(idx_face, all_volume);
	mTab->enableTabButton(idx_contents, all_volume);

	// Refresh object and prim count labels
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	S32 objects = selection->getRootObjectCount();
	S32 prims = selection->getObjectCount();
	S32 cost = prims;
	if (gMeshRepo.meshRezEnabled())
	{
		cost = (S32)(selection->getSelectedObjectCost() + 0.5f);
	}
	if (mLastObjectCount != objects || mLastPrimCount != prims ||
		mLastLandImpact != cost)
	{
		mLastObjectCount = objects;
		mLastPrimCount = prims;
		mLastLandImpact = cost;

		std::string count = llformat("%d", objects);
		mTextObjectCount->setTextArg("[COUNT]", count);

		count = llformat("%d", prims);
		if (cost != prims)
		{
			count += llformat(" (%d)", cost);
		}
		mTextPrimCount->setTextArg("[COUNT]", count);
	}

	toolsPrecision();

	// Refresh child tabs
	mPanelPermissions->refresh();
	mPanelObject->refresh();
	mPanelVolume->refresh();
	mPanelFace->refresh();
	mPanelContents->refresh();
	mPanelLandInfo->refresh();
}

//virtual
void LLFloaterTools::draw()
{
	if (mDirty)
	{
		refresh();
		mDirty = false;
	}

	//mCheckSelectIndividual->set(gSavedSettings.getBOOL("EditLinkedParts"));
	LLFloater::draw();
}

void LLFloaterTools::dirty()
{
	mDirty = true; 
	LLFloaterOpenObject::dirty();
}

// Clean up any tool state that should not persist when the floater is closed.
void LLFloaterTools::resetToolState()
{
	gCameraBtnZoom = TRUE;
	gCameraBtnOrbit = FALSE;
	gCameraBtnPan = FALSE;

	gGrabBtnSpin = FALSE;
	gGrabBtnVertical = FALSE;
}

void LLFloaterTools::updatePopup(LLCoordGL center, MASK mask)
{
	LLTool* tool = LLToolMgr::getInstance()->getCurrentTool();

	// HACK to allow seeing the buttons when you have the app in a window.
	// Keep the visibility the same as it 
	if (tool == gToolNull)
	{
		return;
	}

	if (isMinimized())
	{	// SL looks odd if we draw the tools while the window is minimized
		return;
	}

	// Focus buttons
	BOOL focus_visible = (tool == LLToolCamera::getInstance());

	mBtnFocus->setToggleState(focus_visible);

	mRadioZoom->setVisible(focus_visible);
	mRadioOrbit->setVisible(focus_visible);
	mRadioPan->setVisible(focus_visible);
	mSliderZoom->setVisible(focus_visible);
	mSliderZoom->setEnabled(gCameraBtnZoom);

	mRadioZoom->set(!gCameraBtnOrbit && !gCameraBtnPan &&
					mask != MASK_ORBIT && mask != (MASK_ORBIT | MASK_ALT) &&
					mask != MASK_PAN && mask != (MASK_PAN | MASK_ALT));

	mRadioOrbit->set(gCameraBtnOrbit || mask == MASK_ORBIT ||
					 mask == (MASK_ORBIT | MASK_ALT));

	mRadioPan->set(gCameraBtnPan || mask == MASK_PAN ||
				   mask == (MASK_PAN | MASK_ALT));

	// multiply by correction factor because volume sliders go [0, 0.5]
	mSliderZoom->setValue(gAgent.getCameraZoomFraction() * 0.5f);

	// Move buttons
	BOOL move_visible = (tool == LLToolGrab::getInstance());

	mBtnMove->setToggleState(move_visible);

	// HACK - highlight buttons for next click
	mRadioMove->setVisible(move_visible);
	mRadioMove->set(!gGrabBtnSpin && !gGrabBtnVertical &&
					mask != MASK_VERTICAL && mask != MASK_SPIN);

	mRadioLift->setVisible(move_visible);
	mRadioLift->set(gGrabBtnVertical || mask == MASK_VERTICAL);

	mRadioSpin->setVisible(move_visible);
	mRadioSpin->set(gGrabBtnSpin || mask == MASK_SPIN);

	// Edit buttons
	BOOL edit_visible = tool == LLToolCompTranslate::getInstance() ||
						tool == QLToolAlign::getInstance() ||
						tool == LLToolCompRotate::getInstance() ||
						tool == LLToolCompScale::getInstance() ||
						tool == LLToolFace::getInstance() ||
						tool == LLToolIndividual::getInstance() ||
						tool == LLToolPipette::getInstance();

	mBtnEdit->setToggleState(edit_visible);

	mRadioPosition->setVisible(edit_visible);
	mRadioAlign->setVisible(edit_visible);
	mRadioRotate->setVisible(edit_visible);
	mRadioStretch->setVisible(edit_visible);

	mRadioSelectFace->setVisible(edit_visible);
	mRadioSelectFace->set(tool == LLToolFace::getInstance());

	mBtnLink->setVisible(edit_visible);
	mBtnUnlink->setVisible(edit_visible);

	mBtnLink->setEnabled(LLSelectMgr::instance().enableLinkObjects());
	mBtnUnlink->setEnabled(LLSelectMgr::instance().enableUnlinkObjects());

	mCheckSelectIndividual->setVisible(edit_visible);
	//mCheckSelectIndividual->set(gSavedSettings.getBOOL("EditLinkedParts"));

	mRadioPosition->set(tool == LLToolCompTranslate::getInstance());
	mRadioAlign ->set(tool == QLToolAlign::getInstance());
	mRadioRotate->set(tool == LLToolCompRotate::getInstance());
	mRadioStretch->set(tool == LLToolCompScale::getInstance());

	mComboGridMode->setVisible(edit_visible);
	S32 index = mComboGridMode->getCurrentIndex();
	mComboGridMode->removeall();
	switch (mObjectSelection->getSelectType())
	{
		case SELECT_TYPE_HUD:
			mComboGridMode->add(mGridScreenText);
			mComboGridMode->add(mGridLocalText);
			//mComboGridMode->add(mGridReferenceText);
			break;
		case SELECT_TYPE_WORLD:
			mComboGridMode->add(mGridWorldText);
			mComboGridMode->add(mGridLocalText);
			mComboGridMode->add(mGridReferenceText);
			break;
		case SELECT_TYPE_ATTACHMENT:
			mComboGridMode->add(mGridAttachmentText);
			mComboGridMode->add(mGridLocalText);
			mComboGridMode->add(mGridReferenceText);
			break;
	}
	mComboGridMode->setCurrentByIndex(index);

	mTextGridMode->setVisible(edit_visible);

	mBtnGridOptions->setVisible(edit_visible /* || tool == LLToolGrab::getInstance() */);

	//mCheckSelectLinked->setVisible(edit_visible);
	mCheckStretchUniform->setVisible(edit_visible);
	mCheckStretchTexture->setVisible(edit_visible);
	mCheckLimitDrag->setVisible(edit_visible);

	// Create buttons
	BOOL create_visible = (tool == LLToolCompCreate::getInstance());

	mBtnCreate->setToggleState(	tool == LLToolCompCreate::getInstance());

	updateTreeGrassCombo(create_visible);

	if (mCheckCopySelection->get())
	{
		// don't highlight any placer button
		for (std::vector<LLButton*>::size_type i = 0; i < mButtons.size(); i++)
		{
			mButtons[i]->setToggleState(FALSE);
			mButtons[i]->setVisible(create_visible);
		}
	}
	else
	{
		// Highlight the correct placer button
		for (std::vector<LLButton*>::size_type i = 0, count = mButtons.size();
			 i < count; ++i)
		{
			LLPCode pcode = LLToolPlacer::getObjectType();
			void* userdata = mButtons[i]->getCallbackUserData();
			LLPCode* cur = (LLPCode*)userdata;

			BOOL state = (pcode == *cur);
			mButtons[i]->setToggleState(state);
			mButtons[i]->setVisible(create_visible);
		}
	}

	mCheckSticky->setVisible(create_visible);
	mCheckCopySelection->setVisible(create_visible);
	mCheckCopyCenters->setVisible(create_visible);
	mCheckCopyRotates->setVisible(create_visible);

	mCheckCopyCenters->setEnabled(mCheckCopySelection->get());
	mCheckCopyRotates->setEnabled(mCheckCopySelection->get());

	// Land buttons
	BOOL land_visible = (tool == LLToolBrushLand::getInstance() ||
						 tool == LLToolSelectLand::getInstance());

	mBtnLand->setToggleState(land_visible);

	//mRadioEditLand->set(tool == LLToolBrushLand::getInstance());
	mRadioSelectLand->set(tool == LLToolSelectLand::getInstance());

	//mRadioEditLand->setVisible(land_visible);
	mRadioSelectLand->setVisible(land_visible);

	static LLCachedControl<S32> dozer_mode(gSavedSettings,
										   "RadioLandBrushAction");

	mRadioDozerFlatten->set(tool == LLToolBrushLand::getInstance() &&
							dozer_mode == 0);
	mRadioDozerFlatten->setVisible(land_visible);

	mRadioDozerRaise->set(tool == LLToolBrushLand::getInstance() &&
						  dozer_mode == 1);
	mRadioDozerRaise->setVisible(land_visible);

	mRadioDozerLower->set(tool == LLToolBrushLand::getInstance() &&
						  dozer_mode == 2);
	mRadioDozerLower->setVisible(land_visible);

	mRadioDozerSmooth->set(tool == LLToolBrushLand::getInstance() &&
						   dozer_mode == 3);
	mRadioDozerSmooth->setVisible(land_visible);

	mRadioDozerNoise->set(tool == LLToolBrushLand::getInstance() &&
						  dozer_mode == 4);
	mRadioDozerNoise->setVisible(land_visible);

	mRadioDozerRevert->set(tool == LLToolBrushLand::getInstance() &&
						   dozer_mode == 5);
	mRadioDozerRevert->setVisible(land_visible);

	mBtnApplyToSelection->setVisible(land_visible);
	mBtnApplyToSelection->setEnabled(land_visible &&
									 !LLViewerParcelMgr::getInstance()->selectionEmpty() &&
									 tool != LLToolSelectLand::getInstance());

	mSliderDozerSize->setVisible(land_visible);
	mTextBulldozer->setVisible(land_visible);
	mTextDozerSize->setVisible(land_visible);

	mSliderDozerForce->setVisible(land_visible);
	mTextStrength->setVisible(land_visible);

	mTextObjectCount->setVisible(!land_visible);
	mTextPrimCount->setVisible(!land_visible);
	mTab->setVisible(!land_visible);
	mPanelLandInfo->setVisible(land_visible);
}

//virtual
BOOL LLFloaterTools::canClose()
{
	// don't close when quitting, so camera will stay put
	return !LLApp::isExiting();
}

//virtual
void LLFloaterTools::onOpen()
{
	mParcelSelection = LLViewerParcelMgr::getInstance()->getFloatingParcelSelection();
	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
}

//virtual
void LLFloaterTools::onClose(bool app_quitting)
{
	setMinimized(FALSE);
	setVisible(FALSE);
	mTab->setVisible(FALSE);

	LLViewerJoystick::getInstance()->moveAvatar(false);

    // Different from handle_reset_view in that it doesn't actually move the
	// camera if EditCameraMovement is not set.
	gAgent.resetView(gSavedSettings.getBOOL("EditCameraMovement"));

	// exit component selection mode
	LLSelectMgr::getInstance()->promoteSelectionToRoot();
	gSavedSettings.setBOOL("EditLinkedParts", FALSE);

	gViewerWindow->showCursor();

	resetToolState();

	mParcelSelection = NULL;
	mObjectSelection = NULL;

	if (!gAgent.cameraMouselook())
	{
		// Switch back to basic toolset
		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		// we were already in basic toolset, using build tools
		// so manually reset tool to default (pie menu tool)
		LLToolMgr::getInstance()->getCurrentToolset()->selectFirstTool();
	}
	else 
	{
		// Switch back to mouselook toolset
		LLToolMgr::getInstance()->setCurrentToolset(gMouselookToolset);

		gViewerWindow->hideCursor();
		gViewerWindow->moveCursorToCenter();
	}
}

void LLFloaterTools::showPanel(EInfoPanel panel)
{
	llassert(panel >= 0 && panel < PANEL_COUNT);
	mTab->selectTabByName(PANEL_NAMES[panel]);
}

void click_popup_info(void*)
{
//	gBuildView->setPropertiesPanelOpen(TRUE);
}

void click_popup_done(void*)
{
	handle_reset_view();
}

void click_popup_grab_drag(LLUICtrl*, void*)
{
	gGrabBtnVertical = FALSE;
	gGrabBtnSpin = FALSE;
}

void click_popup_grab_lift(LLUICtrl*, void*)
{
	gGrabBtnVertical = TRUE;
	gGrabBtnSpin = FALSE;
}

void click_popup_grab_spin(LLUICtrl*, void*)
{
	gGrabBtnVertical = FALSE;
	gGrabBtnSpin = TRUE;
}

void commit_radio_zoom(LLUICtrl *, void*)
{
	gCameraBtnZoom = TRUE;
	gCameraBtnOrbit = FALSE;
	gCameraBtnPan = FALSE;
}

void commit_radio_orbit(LLUICtrl *, void*)
{
	gCameraBtnZoom = FALSE;
	gCameraBtnOrbit = TRUE;
	gCameraBtnPan = FALSE;
}

void commit_radio_pan(LLUICtrl *, void*)
{
	gCameraBtnZoom = FALSE;
	gCameraBtnOrbit = FALSE;
	gCameraBtnPan = TRUE;
}

void commit_slider_zoom(LLUICtrl *ctrl, void*)
{
	// renormalize value, since max "volume" level is 0.5 for some reason
	F32 zoom_level = (F32)ctrl->getValue().asReal() * 2.f; // / 0.5f;
	gAgent.setCameraZoomFraction(zoom_level);
}

void click_popup_rotate_left(void*)
{
	LLSelectMgr::getInstance()->selectionRotateAroundZ(45.f);
	dialog_refresh_all();
}

void click_popup_rotate_reset(void*)
{
	LLSelectMgr::getInstance()->selectionResetRotation();
	dialog_refresh_all();
}

void click_popup_rotate_right(void*)
{
	LLSelectMgr::getInstance()->selectionRotateAroundZ(-45.f);
	dialog_refresh_all();
}

void click_popup_dozer_mode(LLUICtrl*, void* data)
{
	S32 mode = (S32)(intptr_t)data;
	gFloaterTools->setEditTool(LLToolBrushLand::getInstance());
	gSavedSettings.setS32("RadioLandBrushAction", mode);
}

void commit_slider_dozer_size(LLUICtrl* ctrl, void*)
{
	F32 size = (F32)ctrl->getValue().asReal();
	gSavedSettings.setF32("LandBrushSize", size);
}

void commit_slider_dozer_force(LLUICtrl* ctrl, void*)
{
	// the slider is logarithmic, so we exponentiate to get the actual force
	// multiplier
	F32 dozer_force = pow(10.f, (F32)ctrl->getValue().asReal());
	gSavedSettings.setF32("LandBrushForce", dozer_force);
}

void click_apply_to_selection(void* data)
{
	LLToolBrushLand::getInstance()->modifyLandInSelectionGlobal();
}

void commit_select_tool(LLUICtrl *ctrl, void *data)
{
	S32 show_owners = gSavedSettings.getBOOL("ShowParcelOwners");
	gFloaterTools->setEditTool(data);
	gSavedSettings.setBOOL("ShowParcelOwners", show_owners);
}

void commit_select_component(LLUICtrl *ctrl, void *data)
{
	LLFloaterTools* floaterp = (LLFloaterTools*)data;

	//forfeit focus
	if (gFocusMgr.childHasKeyboardFocus(floaterp))
	{
		gFocusMgr.setKeyboardFocus(NULL);
	}

	BOOL select_individuals = floaterp->getSelectIndividuals();
	gSavedSettings.setBOOL("EditLinkedParts", select_individuals);
	floaterp->dirty();

	if (select_individuals)
	{
		LLSelectMgr::getInstance()->demoteSelectionToIndividuals();
	}
	else
	{
		LLSelectMgr::getInstance()->promoteSelectionToRoot();
	}
}

void commit_grid_mode(LLUICtrl *ctrl, void *data)   
{   
	LLComboBox* combo = (LLComboBox*)ctrl;   
    
	LLSelectMgr::getInstance()->setGridMode((EGridMode)combo->getCurrentIndex());
} 

void click_count(void* data)
{
	LLFloaterObjectWeights::show(gFloaterTools);
}

// static 
void LLFloaterTools::setObjectType(void* data)
{
	LLPCode pcode = *(LLPCode*) data;
	LLToolPlacer::setObjectType(pcode);
	gSavedSettings.setBOOL("CreateToolCopySelection", FALSE);
	gFloaterTools->updateTreeGrassCombo(true);
	gFocusMgr.setMouseCapture(NULL);
}

// static
void LLFloaterTools::onClickGridOptions(void* data)
{
	//LLFloaterTools* floaterp = (LLFloaterTools*)data;
	LLFloaterBuildOptions::show(NULL);
	// RN: this makes grid options dependent on build tools window
	//floaterp->addDependentFloater(LLFloaterBuildOptions::getInstance(), FALSE);
}

// static
void LLFloaterTools::onClickLink(void* data)
{
	LLSelectMgr::getInstance()->linkObjects();
}

// static
void LLFloaterTools::onClickUnlink(void* data)
{
	LLSelectMgr::getInstance()->unlinkObjects();
}

void LLFloaterTools::setEditTool(void* tool_pointer)
{
	select_tool(tool_pointer);
}

void LLFloaterTools::onFocusReceived()
{
	LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
	LLFloater::onFocusReceived();
}

// static
void LLFloaterTools::onSelectTreesGrass(LLUICtrl*, void*)
{
	const std::string& selected = gFloaterTools->mComboTreesGrass->getValue();
	LLPCode pcode = LLToolPlacer::getObjectType();
	if (pcode == LLToolPlacerPanel::sTree) 
	{
		gSavedSettings.setString("LastTree", selected);
	} 
	else if (pcode == LLToolPlacerPanel::sGrass) 
	{
		gSavedSettings.setString("LastGrass", selected);
	}  
}

void LLFloaterTools::updateTreeGrassCombo(bool visible)
{
	if (visible) 
	{
		LLPCode pcode = LLToolPlacer::getObjectType();
		std::map<std::string, S32>::iterator it, end;
		std::string selected;
		if (pcode == LLToolPlacerPanel::sTree) 
		{
			mTextTreeGrass->setVisible(visible);
			mTextTreeGrass->setText(mBtnToolTree->getToolTip());

			static LLCachedControl<std::string> last_tree(gSavedSettings,
														  "LastTree");
			selected = last_tree;
			it = LLVOTree::sSpeciesNames.begin();
			end = LLVOTree::sSpeciesNames.end();
		} 
		else if (pcode == LLToolPlacerPanel::sGrass) 
		{
			mTextTreeGrass->setVisible(visible);
			mTextTreeGrass->setText(mBtnToolGrass->getToolTip());

			static LLCachedControl<std::string> last_grass(gSavedSettings,
														   "LastGrass");
			selected = last_grass;
			it = LLVOGrass::sSpeciesNames.begin();
			end = LLVOGrass::sSpeciesNames.end();
		} 
		else 
		{
			mComboTreesGrass->removeall();
			// LLComboBox::removeall() does not clear the label
			mComboTreesGrass->setLabel(LLStringExplicit(""));
			mComboTreesGrass->setEnabled(false);
			mComboTreesGrass->setVisible(false);
			mTextTreeGrass->setVisible(false);
			return;
		}

		mComboTreesGrass->removeall();
		mComboTreesGrass->add("Random");

		S32 select = 0, i = 0;

		while (it != end) 
		{
			const std::string& species = it->first;
			mComboTreesGrass->add(species);
			++i;
			if (species == selected)
			{
				select = i;
			}
			++it;
		}
		// if saved species not found, default to "Random"
		mComboTreesGrass->selectNthItem(select);
		mComboTreesGrass->setEnabled(true);
	}

	mComboTreesGrass->setVisible(visible);
	mTextTreeGrass->setVisible(visible);
}
