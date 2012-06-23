/** 
 * @file llfloatercustomize.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloatercustomize.h"

#include "llassetstorage.h"
#include "llscrollcontainer.h"
#include "llscrollingpanellist.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llfloatermakenewoutfit.h"
#include "llpaneleditwearable.h"
#include "llscrollingpanelparam.h"
#include "lltoolmorph.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewervisualparam.h"
#include "llvoavatarself.h"

using namespace LLVOAvatarDefines;

//*TODO:translate : The ui xml for this really needs to be integrated with
// the appearance paramaters

// Globals
LLFloaterCustomize* gFloaterCustomize = NULL;

////////////////////////////////////////////////////////////////////////////
// Local Constants 

/////////////////////////////////////////////////////////////////////
// LLFloaterCustomizeObserver

class LLFloaterCustomizeObserver : public LLInventoryObserver
{
public:
	LLFloaterCustomizeObserver(LLFloaterCustomize* fc) : mFC(fc) {}
	virtual ~LLFloaterCustomizeObserver() {}
	virtual void changed(U32 mask) { mFC->updateScrollingPanelUI(); }

protected:
	LLFloaterCustomize* mFC;
};

////////////////////////////////////////////////////////////////////////////

// statics
LLWearableType::EType LLFloaterCustomize::sCurrentWearableType = LLWearableType::WT_INVALID;

struct WearablePanelData
{
	WearablePanelData(LLFloaterCustomize* floater, LLWearableType::EType type)
	:	mFloater(floater), mType(type) {}
	LLFloaterCustomize* mFloater;
	LLWearableType::EType mType;
};

LLFloaterCustomize::LLFloaterCustomize()
:	LLFloater(std::string("customize")),
	mScrollingPanelList(NULL),
	mInventoryObserver(NULL),
	mNextStepAfterSaveCallback(NULL),
	mNextStepAfterSaveUserdata(NULL)
{
	memset(&mWearablePanelList[0], 0, sizeof(char*)*LLWearableType::WT_COUNT); // Initialize to 0

	if (isAgentAvatarValid())
	{
		gSavedSettings.setU32("AvatarSex", gAgentAvatarp->getSex() == SEX_MALE);
	}

	mResetParams = new LLVisualParamReset();

	// create the observer which will watch for matching incoming inventory
	mInventoryObserver = new LLFloaterCustomizeObserver(this);
	gInventory.addObserver(mInventoryObserver);

	LLCallbackMap::map_t factory_map;
	for (U32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		LLWearableType::EType type = (LLWearableType::EType)i;
		std::string name = LLWearableType::getCapitalizedTypeName(type);
		factory_map[name] = LLCallbackMap(createWearablePanel,
										  (new WearablePanelData(this, type)));
	}
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_customize.xml",
												 &factory_map);
}

BOOL LLFloaterCustomize::postBuild()
{
	childSetAction("Make Outfit", onBtnMakeOutfit, this);
	childSetAction("Ok", onBtnOk, this);
	childSetAction("Cancel", LLFloater::onClickClose, this);

	// Wearable panels
	initWearablePanels();

	// Tab container
	for (U32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		LLWearableType::EType type = (LLWearableType::EType)i;
		childSetTabChangeCallback("customize tab container",
								  LLWearableType::getCapitalizedTypeName(type),
								  onTabChanged,
								  (void*)type,
								  onTabPrecommit);
	}

	// Remove underwear panels for teens
	if (gAgent.isTeen())
	{
		LLTabContainer* tab_container = getChild<LLTabContainer>("customize tab container",
																 TRUE, FALSE);
		if (tab_container)
		{
			LLPanel* panel = tab_container->getPanelByName("Undershirt");
			if (panel)
			{
				tab_container->removeTabPanel(panel);
			}
			panel = tab_container->getPanelByName("Underpants");
			if (panel)
			{
				tab_container->removeTabPanel(panel);
			}
		}
	}

	// Scrolling Panel
	initScrollingPanelList();

	return TRUE;
}

void LLFloaterCustomize::open()
{
	LLFloater::open();
	// childShowTab depends on gFloaterCustomize being defined and therefore
	// must be called after the constructor. - Nyx
	childShowTab("customize tab container", "Shape", true);
	setCurrentWearableType(LLWearableType::WT_SHAPE);
}

////////////////////////////////////////////////////////////////////////////

// static
void LLFloaterCustomize::updateAvatarHeightDisplay()
{
	if (gFloaterCustomize && isAgentAvatarValid())
	{
		F32 shoes = gAgentAvatarp->getVisualParamWeight("Shoe_Heels") * 0.08f;
		shoes += gAgentAvatarp->getVisualParamWeight("Shoe_Platform") * 0.07f;
		gFloaterCustomize->getChild<LLTextBox>("ShoesText")->setValue(llformat("%.2f", shoes) + "m");
		F32 avatar_size = gAgentAvatarp->mBodySize.mV[VZ] - shoes + 0.17f; //mBodySize is actually quite a bit off.
		gFloaterCustomize->getChild<LLTextBox>("HeightTextM")->setValue(llformat("%.2f", avatar_size) + "m");
		F32 feet = avatar_size / 0.3048f;
		F32 inches = (feet - (F32)((U32)feet)) * 12.0f;
		gFloaterCustomize->getChild<LLTextBox>("HeightTextI")->setValue(llformat("%d'%d\"", (U32)feet, (U32)inches));
		gFloaterCustomize->getChild<LLTextBox>("PelvisToFootText")->setValue(llformat("%.2f", gAgentAvatarp->getPelvisToFoot()) + "m");
	}
}

// static
void LLFloaterCustomize::setCurrentWearableType(LLWearableType::EType type)
{
	if (LLFloaterCustomize::sCurrentWearableType != type)
	{
		LLFloaterCustomize::sCurrentWearableType = type; 

		S32 type_int = (S32)type;
		if (gFloaterCustomize &&
			gFloaterCustomize->mWearablePanelList[type_int])
		{
			std::string panelname = gFloaterCustomize->mWearablePanelList[type_int]->getName();
			gFloaterCustomize->childShowTab("customize tab container", panelname);
			gFloaterCustomize->switchToDefaultSubpart();
		}
	}
}

// static
void LLFloaterCustomize::onBtnOk(void* userdata)
{
	gAgentWearables.saveAllWearables();

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->invalidateAll();
		gAgentAvatarp->requestLayerSetUploads();
		gAgent.sendAgentSetAppearance();
	}

	if (gFloaterView)
	{
		gFloaterView->sendChildToBack((LLFloaterCustomize*)userdata);
	}
	handle_reset_view();  // Calls askToSaveIfDirty
}

// static
void LLFloaterCustomize::onBtnMakeOutfit(void* userdata)
{
	LLFloaterMakeNewOutfit::showInstance();
}

////////////////////////////////////////////////////////////////////////////

// static
void* LLFloaterCustomize::createWearablePanel(void* userdata)
{
	WearablePanelData* data = (WearablePanelData*)userdata;
	LLWearableType::EType type = data->mType;
	LLPanelEditWearable* panel;
	if (gAgent.isTeen() &&
		(data->mType == LLWearableType::WT_UNDERSHIRT ||
		 data->mType == LLWearableType::WT_UNDERPANTS))
	{
		panel = NULL;
	}
	else
	{
		panel = new LLPanelEditWearable(type);
	}
	data->mFloater->mWearablePanelList[type] = panel;
	delete data;
	return panel;
}

void LLFloaterCustomize::initWearablePanels()
{
	LLSubpart* part;

	/////////////////////////////////////////
	// Shape
	LLPanelEditWearable* panel = mWearablePanelList[LLWearableType::WT_SHAPE];

	// body
	part = new LLSubpart();
	part->mTargetJoint = "mPelvis";
	part->mEditGroup = "shape_body";
	part->mTargetOffset.setVec(0.f, 0.f, 0.1f);
	part->mCameraOffset.setVec(-2.5f, 0.5f, 0.8f);
	panel->addSubpart("Body", SUBPART_SHAPE_WHOLE, part);

	// head supparts
	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "shape_head";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Head", SUBPART_SHAPE_HEAD, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "shape_eyes";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Eyes", SUBPART_SHAPE_EYES, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "shape_ears";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Ears", SUBPART_SHAPE_EARS, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "shape_nose";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Nose", SUBPART_SHAPE_NOSE, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "shape_mouth";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Mouth", SUBPART_SHAPE_MOUTH, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "shape_chin";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Chin", SUBPART_SHAPE_CHIN, part);

	// torso
	part = new LLSubpart();
	part->mTargetJoint = "mTorso";
	part->mEditGroup = "shape_torso";
	part->mTargetOffset.setVec(0.f, 0.f, 0.3f);
	part->mCameraOffset.setVec(-1.f, 0.15f, 0.3f);
	panel->addSubpart("Torso", SUBPART_SHAPE_TORSO, part);

	// legs
	part = new LLSubpart();
	part->mTargetJoint = "mPelvis";
	part->mEditGroup = "shape_legs";
	part->mTargetOffset.setVec(0.f, 0.f, -0.5f);
	part->mCameraOffset.setVec(-1.6f, 0.15f, -0.5f);
	panel->addSubpart("Legs", SUBPART_SHAPE_LEGS, part);

	if (panel->getChild<LLUICtrl>("sex radio", TRUE, FALSE))
	{
		panel->childSetCommitCallback("sex radio",
									  LLPanelEditWearable::onCommitSexChange,
									  panel);
	}

	/////////////////////////////////////////
	// Skin
	panel = mWearablePanelList[LLWearableType::WT_SKIN];

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "skin_color";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Skin Color", SUBPART_SKIN_COLOR, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "skin_facedetail";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Face Detail", SUBPART_SKIN_FACEDETAIL, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "skin_makeup";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Makeup", SUBPART_SKIN_MAKEUP, part);

	part = new LLSubpart();
	part->mTargetJoint = "mPelvis";
	part->mEditGroup = "skin_bodydetail";
	part->mTargetOffset.setVec(0.f, 0.f, -0.2f);
	part->mCameraOffset.setVec(-2.5f, 0.5f, 0.5f);
	panel->addSubpart("Body Detail", SUBPART_SKIN_BODYDETAIL, part);

	panel->addTextureDropTarget(TEX_HEAD_BODYPAINT, "Head Skin", LLUUID::null, TRUE);
	panel->addTextureDropTarget(TEX_UPPER_BODYPAINT, "Upper Body", LLUUID::null, TRUE);
	panel->addTextureDropTarget(TEX_LOWER_BODYPAINT, "Lower Body", LLUUID::null, TRUE);

	/////////////////////////////////////////
	// Hair
	panel = mWearablePanelList[LLWearableType::WT_HAIR];

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "hair_color";
	part->mTargetOffset.setVec(0.f, 0.f, 0.10f);
	part->mCameraOffset.setVec(-0.4f, 0.05f, 0.10f);
	panel->addSubpart("Color", SUBPART_HAIR_COLOR, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "hair_style";
	part->mTargetOffset.setVec(0.f, 0.f, 0.10f);
	part->mCameraOffset.setVec(-0.4f, 0.05f, 0.10f);
	panel->addSubpart("Style", SUBPART_HAIR_STYLE, part);

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "hair_eyebrows";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Eyebrows", SUBPART_HAIR_EYEBROWS, part);

	part = new LLSubpart();
	part->mSex = SEX_MALE;
	part->mTargetJoint = "mHead";
	part->mEditGroup = "hair_facial";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart("Facial", SUBPART_HAIR_FACIAL, part);

	panel->addTextureDropTarget(TEX_HAIR, "Texture",
								LLUUID(gSavedSettings.getString("UIImgDefaultHairUUID")),
								FALSE);

	/////////////////////////////////////////
	// Eyes
	panel = mWearablePanelList[LLWearableType::WT_EYES];

	part = new LLSubpart();
	part->mTargetJoint = "mHead";
	part->mEditGroup = "eyes";
	part->mTargetOffset.setVec(0.f, 0.f, 0.05f);
	part->mCameraOffset.setVec(-0.5f, 0.05f, 0.07f);
	panel->addSubpart(LLStringUtil::null, SUBPART_EYES, part);

	panel->addTextureDropTarget(TEX_EYES_IRIS, "Iris",
								LLUUID(gSavedSettings.getString("UIImgDefaultEyesUUID")),
								FALSE);

	/////////////////////////////////////////
	// Shirt
	panel = mWearablePanelList[LLWearableType::WT_SHIRT];

	part = new LLSubpart();
	part->mTargetJoint = "mTorso";
	part->mEditGroup = "shirt";
	part->mTargetOffset.setVec(0.f, 0.f, 0.3f);
	part->mCameraOffset.setVec(-1.f, 0.15f, 0.3f);
	panel->addSubpart(LLStringUtil::null, SUBPART_SHIRT, part);

	panel->addTextureDropTarget(TEX_UPPER_SHIRT, "Fabric",
								LLUUID(gSavedSettings.getString("UIImgDefaultShirtUUID")),
								FALSE);

	panel->addColorSwatch(TEX_UPPER_SHIRT, "Color/Tint");

	/////////////////////////////////////////
	// Pants
	panel = mWearablePanelList[LLWearableType::WT_PANTS];

	part = new LLSubpart();
	part->mTargetJoint = "mPelvis";
	part->mEditGroup = "pants";
	part->mTargetOffset.setVec(0.f, 0.f, -0.5f);
	part->mCameraOffset.setVec(-1.6f, 0.15f, -0.5f);
	panel->addSubpart(LLStringUtil::null, SUBPART_PANTS, part);

	panel->addTextureDropTarget(TEX_LOWER_PANTS, "Fabric",
								LLUUID(gSavedSettings.getString("UIImgDefaultPantsUUID")),
								FALSE);

	panel->addColorSwatch(TEX_LOWER_PANTS, "Color/Tint");

	/////////////////////////////////////////
	// Shoes
	panel = mWearablePanelList[LLWearableType::WT_SHOES];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mPelvis";
		part->mEditGroup = "shoes";
		part->mTargetOffset.setVec(0.f, 0.f, -0.5f);
		part->mCameraOffset.setVec(-1.6f, 0.15f, -0.5f);
		panel->addSubpart(LLStringUtil::null, SUBPART_SHOES, part);

		panel->addTextureDropTarget(TEX_LOWER_SHOES, "Fabric",
									LLUUID(gSavedSettings.getString("UIImgDefaultShoesUUID")),
									FALSE);

		panel->addColorSwatch(TEX_LOWER_SHOES, "Color/Tint");
	}

	/////////////////////////////////////////
	// Socks
	panel = mWearablePanelList[LLWearableType::WT_SOCKS];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mPelvis";
		part->mEditGroup = "socks";
		part->mTargetOffset.setVec(0.f, 0.f, -0.5f);
		part->mCameraOffset.setVec(-1.6f, 0.15f, -0.5f);
		panel->addSubpart(LLStringUtil::null, SUBPART_SOCKS, part);

		panel->addTextureDropTarget(TEX_LOWER_SOCKS, "Fabric",
									LLUUID(gSavedSettings.getString("UIImgDefaultSocksUUID")),
									FALSE);

		panel->addColorSwatch(TEX_LOWER_SOCKS, "Color/Tint");
	}

	/////////////////////////////////////////
	// Jacket
	panel = mWearablePanelList[LLWearableType::WT_JACKET];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mTorso";
		part->mEditGroup = "jacket";
		part->mTargetOffset.setVec(0.f, 0.f, 0.f);
		part->mCameraOffset.setVec(-2.f, 0.1f, 0.3f);
		panel->addSubpart(LLStringUtil::null, SUBPART_JACKET, part);

		panel->addTextureDropTarget(TEX_UPPER_JACKET, "Upper Fabric",
									LLUUID(gSavedSettings.getString("UIImgDefaultJacketUUID")),
									FALSE);
		panel->addTextureDropTarget(TEX_LOWER_JACKET, "Lower Fabric",
									LLUUID(gSavedSettings.getString("UIImgDefaultJacketUUID")),
									FALSE);

		panel->addColorSwatch(TEX_UPPER_JACKET, "Color/Tint");
	}

	/////////////////////////////////////////
	// Skirt
	panel = mWearablePanelList[LLWearableType::WT_SKIRT];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mPelvis";
		part->mEditGroup = "skirt";
		part->mTargetOffset.setVec(0.f, 0.f, -0.5f);
		part->mCameraOffset.setVec(-1.6f, 0.15f, -0.5f);
		panel->addSubpart(LLStringUtil::null, SUBPART_SKIRT, part);

		panel->addTextureDropTarget(TEX_SKIRT, "Fabric",
									LLUUID(gSavedSettings.getString("UIImgDefaultSkirtUUID")),
									FALSE);

		panel->addColorSwatch(TEX_SKIRT, "Color/Tint");
	}

	/////////////////////////////////////////
	// Gloves
	panel = mWearablePanelList[LLWearableType::WT_GLOVES];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mTorso";
		part->mEditGroup = "gloves";
		part->mTargetOffset.setVec(0.f, 0.f, 0.f);
		part->mCameraOffset.setVec(-1.f, 0.15f, 0.f);
		panel->addSubpart(LLStringUtil::null, SUBPART_GLOVES, part);

		panel->addTextureDropTarget(TEX_UPPER_GLOVES, "Fabric",
									LLUUID(gSavedSettings.getString("UIImgDefaultGlovesUUID")),
									FALSE);

		panel->addColorSwatch(TEX_UPPER_GLOVES, "Color/Tint");
	}

	/////////////////////////////////////////
	// Undershirt
	panel = mWearablePanelList[LLWearableType::WT_UNDERSHIRT];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mTorso";
		part->mEditGroup = "undershirt";
		part->mTargetOffset.setVec(0.f, 0.f, 0.3f);
		part->mCameraOffset.setVec(-1.f, 0.15f, 0.3f);
		panel->addSubpart(LLStringUtil::null, SUBPART_UNDERSHIRT, part);

		panel->addTextureDropTarget(TEX_UPPER_UNDERSHIRT, "Fabric",
									LLUUID(gSavedSettings.getString("UIImgDefaultUnderwearUUID")),
									FALSE);

		panel->addColorSwatch(TEX_UPPER_UNDERSHIRT, "Color/Tint");
	}

	/////////////////////////////////////////
	// Underpants
	panel = mWearablePanelList[LLWearableType::WT_UNDERPANTS];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mPelvis";
		part->mEditGroup = "underpants";
		part->mTargetOffset.setVec(0.f, 0.f, -0.5f);
		part->mCameraOffset.setVec(-1.6f, 0.15f, -0.5f);
		panel->addSubpart(LLStringUtil::null, SUBPART_UNDERPANTS, part);

		panel->addTextureDropTarget(TEX_LOWER_UNDERPANTS, "Fabric",
									LLUUID(gSavedSettings.getString("UIImgDefaultUnderwearUUID")),
									FALSE);

		panel->addColorSwatch(TEX_LOWER_UNDERPANTS, "Color/Tint");
	}

	/////////////////////////////////////////
	// Alpha
	panel = mWearablePanelList[LLWearableType::WT_ALPHA];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mPelvis";
		part->mEditGroup = "alpha";
		part->mTargetOffset.setVec(0.f, 0.f, 0.1f);
		part->mCameraOffset.setVec(-2.5f, 0.5f, 0.8f);
		panel->addSubpart(LLStringUtil::null, SUBPART_ALPHA, part);

		panel->addTextureDropTarget(TEX_LOWER_ALPHA, "Lower Alpha",
									LLUUID(gSavedSettings.getString("UIImgDefaultAlphaUUID")),
									TRUE);
		panel->addTextureDropTarget(TEX_UPPER_ALPHA, "Upper Alpha",
									LLUUID(gSavedSettings.getString("UIImgDefaultAlphaUUID")),
									TRUE);
		panel->addTextureDropTarget(TEX_HEAD_ALPHA, "Head Alpha",
									LLUUID(gSavedSettings.getString("UIImgDefaultAlphaUUID")),
									TRUE);
		panel->addTextureDropTarget(TEX_EYES_ALPHA, "Eye Alpha",
									LLUUID(gSavedSettings.getString("UIImgDefaultAlphaUUID")),
									TRUE);
		panel->addTextureDropTarget(TEX_HAIR_ALPHA, "Hair Alpha",
									LLUUID(gSavedSettings.getString("UIImgDefaultAlphaUUID")),
									TRUE);

		panel->addInvisibilityCheckbox(TEX_LOWER_ALPHA, "lower alpha texture invisible");
		panel->addInvisibilityCheckbox(TEX_UPPER_ALPHA, "upper alpha texture invisible");
		panel->addInvisibilityCheckbox(TEX_HEAD_ALPHA, "head alpha texture invisible");
		panel->addInvisibilityCheckbox(TEX_EYES_ALPHA, "eye alpha texture invisible");
		panel->addInvisibilityCheckbox(TEX_HAIR_ALPHA, "hair alpha texture invisible");
	}

	/////////////////////////////////////////
	// Tattoo
	panel = mWearablePanelList[LLWearableType::WT_TATTOO];

	if (panel)
	{
		part = new LLSubpart();
		part->mTargetJoint = "mPelvis";
		part->mEditGroup = "tattoo";
		part->mTargetOffset.setVec(0.f, 0.f, 0.1f);
		part->mCameraOffset.setVec(-2.5f, 0.5f, 0.8f);
		panel->addSubpart(LLStringUtil::null, SUBPART_TATTOO, part);

		panel->addTextureDropTarget(TEX_LOWER_TATTOO, "Lower Tattoo",
									LLUUID::null,
									TRUE);
		panel->addTextureDropTarget(TEX_UPPER_TATTOO, "Upper Tattoo",
									LLUUID::null,
									TRUE);
		panel->addTextureDropTarget(TEX_HEAD_TATTOO, "Head Tattoo",
									LLUUID::null,
									TRUE);
		panel->addColorSwatch(TEX_LOWER_TATTOO, "Color/Tint");
	}

	/////////////////////////////////////////
	// Physics
	panel = mWearablePanelList[LLWearableType::WT_PHYSICS];

	if (panel)
	{
		part = new LLSubpart();
		part->mSex = SEX_FEMALE;
		part->mTargetJoint = "mTorso";
		part->mEditGroup = "physics_breasts_updown";
		part->mTargetOffset.setVec(0.f, 0.f, 0.1f);
		part->mCameraOffset.setVec(-0.8f, 0.15f, 0.38f);
		part->mVisualHint = false;
		panel->addSubpart("Breast Bounce", SUBPART_PHYSICS_BREASTS_UPDOWN, part);

		part = new LLSubpart();
		part->mSex = SEX_FEMALE;
		part->mTargetJoint = "mTorso";
		part->mEditGroup = "physics_breasts_inout";
		part->mTargetOffset.setVec(0.f, 0.f, 0.1f);
		part->mCameraOffset.setVec(-0.8f, 0.15f, 0.38f);
		part->mVisualHint = false;
		panel->addSubpart("Breast Cleavage", SUBPART_PHYSICS_BREASTS_INOUT, part);

		part = new LLSubpart();
		part->mSex = SEX_FEMALE;
		part->mTargetJoint = "mTorso";
		part->mEditGroup = "physics_breasts_leftright";
		part->mTargetOffset.setVec(0.f, 0.f, 0.1f);
		part->mCameraOffset.setVec(-0.8f, 0.15f, 0.38f);
		part->mVisualHint = false;
		panel->addSubpart("Breast Sway", SUBPART_PHYSICS_BREASTS_LEFTRIGHT, part);

		part = new LLSubpart();
		part->mTargetJoint = "mTorso";
		part->mEditGroup = "physics_belly_updown";
		part->mTargetOffset.setVec(0.f, 0.f, 0.1f);
		part->mCameraOffset.setVec(-0.8f, 0.15f, 0.38f);
		part->mVisualHint = false;
		panel->addSubpart("Belly Bounce", SUBPART_PHYSICS_BELLY_UPDOWN, part);

		part = new LLSubpart();
		part->mTargetJoint = "mPelvis";
		part->mEditGroup = "physics_butt_updown";
		part->mTargetOffset.setVec(0.f, 0.f, -0.1f);
		part->mCameraOffset.setVec(0.3f, 0.8f, -0.1f);
		part->mVisualHint = false;
		panel->addSubpart("Butt Bounce", SUBPART_PHYSICS_BUTT_UPDOWN, part);

		part = new LLSubpart();
		part->mTargetJoint = "mPelvis";
		part->mEditGroup = "physics_butt_leftright";
		part->mTargetOffset.setVec(0.f, 0.f, -0.1f);
		part->mCameraOffset.setVec(0.3f, 0.8f, -0.1f);
		part->mVisualHint = false;
		panel->addSubpart("Butt Sway", SUBPART_PHYSICS_BUTT_LEFTRIGHT, part);

		part = new LLSubpart();
		part->mTargetJoint = "mTorso";
		part->mEditGroup = "physics_advanced";
		part->mTargetOffset.setVec(0.f, 0.f, 0.1f);
		part->mCameraOffset.setVec(-2.5f, 0.5f, 0.8f);
		part->mVisualHint = false;
		panel->addSubpart("Advanced Parameters", SUBPART_PHYSICS_ADVANCED, part);
	}
}

////////////////////////////////////////////////////////////////////////////

LLFloaterCustomize::~LLFloaterCustomize()
{
	llinfos << "Destroying LLFloaterCustomize" << llendl;
	mResetParams = NULL;
	gInventory.removeObserver(mInventoryObserver);
	delete mInventoryObserver;
}

void LLFloaterCustomize::switchToDefaultSubpart()
{
	getCurrentWearablePanel()->switchToDefaultSubpart();
}

void LLFloaterCustomize::draw()
{
	if (!isMinimized())
	{
		// Only do this if we are in the customize avatar mode and not
		// transitioning into or out of it
		// *TODO: This is a sort of expensive call, which only needs to be
		// called when the tabs change or an inventory item arrives. Figure
		// out some way to avoid this if possible.
		updateInventoryUI();

		updateAvatarHeightDisplay();

		LLScrollingPanelParam::sUpdateDelayFrames = 0;
	}

	LLFloater::draw();
}

BOOL LLFloaterCustomize::isDirty() const
{
	for (S32 i = 0; i < LLWearableType::WT_COUNT; i++)
	{
		if (mWearablePanelList[i] && mWearablePanelList[i]->isDirty())
		{
			return TRUE;
		}
	}
	return FALSE;
}

// static
void LLFloaterCustomize::onTabPrecommit(void* userdata, bool from_click)
{
	LLWearableType::EType type = (LLWearableType::EType)(intptr_t) userdata;
	if (type != LLWearableType::WT_INVALID && gFloaterCustomize &&
		gFloaterCustomize->getCurrentWearableType() != type)
	{
		gFloaterCustomize->askToSaveIfDirty(onCommitChangeTab, userdata);
	}
	else
	{
		onCommitChangeTab(TRUE, NULL);
	}
}

// static
void LLFloaterCustomize::onTabChanged(void* userdata, bool from_click)
{
	LLWearableType::EType wearable_type = (LLWearableType::EType) (intptr_t)userdata;
	if (wearable_type != LLWearableType::WT_INVALID)
	{
		LLFloaterCustomize::setCurrentWearableType(wearable_type);
	}
}

void LLFloaterCustomize::onClose(bool app_quitting)
{
	// since this window is potentially staying open, push to back to let next
	// window take focus
	if (gFloaterView)
	{
		gFloaterView->sendChildToBack(this);
	}
	handle_reset_view();  // Calls askToSaveIfDirty
}

// static
void LLFloaterCustomize::onCommitChangeTab(BOOL proceed, void* userdata)
{
	if (!proceed || !gFloaterCustomize)
	{
		return;
	}

	LLTabContainer* tab_container = gFloaterCustomize->getChild<LLTabContainer>("customize tab container", TRUE, FALSE);
	if (tab_container)
	{
		tab_container->setTab(-1);
	}
}

////////////////////////////////////////////////////////////////////////////

const S32 LOWER_BTN_HEIGHT = 18 + 8;
const S32 FLOATER_CUSTOMIZE_BUTTON_WIDTH = 82;
const S32 FLOATER_CUSTOMIZE_BOTTOM_PAD = 30;
const S32 LINE_HEIGHT = 16;
const S32 HEADER_PAD = 8;
const S32 HEADER_HEIGHT = 3 * (LINE_HEIGHT + LLFLOATER_VPAD) +
						  (2 * LLPANEL_BORDER_WIDTH) + HEADER_PAD;

void LLFloaterCustomize::initScrollingPanelList()
{
	LLScrollableContainerView* scroll_container =
		getChild<LLScrollableContainerView>("panel_container", TRUE, FALSE);
	// LLScrollingPanelList's do not import correctly 
	//mScrollingPanelList = LLUICtrlFactory::getScrollingPanelList(this, "panel_list");
	mScrollingPanelList = new LLScrollingPanelList(std::string("panel_list"), LLRect());
	if (scroll_container)
	{
		scroll_container->setScrolledView(mScrollingPanelList);
		scroll_container->addChild(mScrollingPanelList);
	}
}

void LLFloaterCustomize::clearScrollingPanelList()
{
	if (mScrollingPanelList)
	{
		mScrollingPanelList->clearPanels();
	}
}

void LLFloaterCustomize::generateVisualParamHints(LLPanelEditWearable* panel,
												  LLViewerJointMesh* joint_mesh,
												  LLFloaterCustomize::param_map& params,
												  LLWearable* wearable,
												  bool use_hints)
{
	// sorted_params is sorted according to magnitude of effect from
	// least to greatest. Adding to the front of the child list
	// reverses that order.
	if (mScrollingPanelList)
	{
		mScrollingPanelList->clearPanels();
		param_map::iterator end = params.end();
		for (param_map::iterator it = params.begin(); it != end; ++it)
		{
			mScrollingPanelList->addPanel(new LLScrollingPanelParam(panel,
																	joint_mesh,
																	(*it).second.second,
																	(*it).second.first,
																	wearable,
																	use_hints));
		}
	}
}

void LLFloaterCustomize::setWearable(LLWearableType::EType type,
									 LLWearable* wearable,
									 U32 perm_mask,
									 BOOL is_complete)
{
	llassert(type < LLWearableType::WT_COUNT);
	LLPanelEditWearable* panel = mWearablePanelList[type];
	if (panel && isAgentAvatarValid())
	{
		gSavedSettings.setU32("AvatarSex", (gAgentAvatarp->getSex() == SEX_MALE));
		panel->setWearable(wearable, perm_mask, is_complete);
		BOOL allow_modify = wearable && (perm_mask & PERM_MODIFY) ? is_complete
																  : FALSE;
		updateScrollingPanelList(allow_modify);
	}
}

void LLFloaterCustomize::updateScrollingPanelList(BOOL allow_modify)
{
	if (mScrollingPanelList)
	{
		LLScrollingPanelParam::sUpdateDelayFrames = 0;
		mScrollingPanelList->updatePanels(allow_modify);
	}
}

void LLFloaterCustomize::askToSaveIfDirty(void(*next_step_callback)(BOOL proceed, void* userdata),
										  void* userdata)
{
	if (isDirty())
	{
		// Ask if user wants to save, then continue to next step afterwards
		mNextStepAfterSaveCallback = next_step_callback;
		mNextStepAfterSaveUserdata = userdata;

		// Bring up view-modal dialog: Save changes? Yes, No, Cancel
		LLNotifications::instance().add("SaveClothingBodyChanges", LLSD(), LLSD(),
			boost::bind(&LLFloaterCustomize::onSaveDialog, this, _1, _2));
		return;
	}

	// Try to move to the next step
	if (next_step_callback)
	{
		next_step_callback(TRUE, userdata);
	}
}

bool LLFloaterCustomize::onSaveDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	BOOL proceed = FALSE;
	U32 index = 0;
	LLPanelEditWearable* panel = mWearablePanelList[sCurrentWearableType];
	if (panel)
	{
		index = panel->getWearableIndex();
	}

	switch (option)
	{
		case 0:  // "Save"
		{
			gAgentWearables.saveWearable(sCurrentWearableType, index);
			proceed = TRUE;
			break;
		}

		case 1:  // "Don't Save"
		{
			gAgentWearables.revertWearable(sCurrentWearableType, index);
			proceed = TRUE;
			break;
		}

		case 2: // "Cancel"
			break;

		default:
			llassert(0);
	}

	if (mNextStepAfterSaveCallback)
	{
		mNextStepAfterSaveCallback(proceed, mNextStepAfterSaveUserdata);
	}
	return false;
}

// fetch observer
class LLCurrentlyWorn : public LLInventoryFetchObserver
{
public:
	LLCurrentlyWorn() {}
	~LLCurrentlyWorn() {}
	virtual void done() { /* no operation necessary */}
};

void LLFloaterCustomize::fetchInventory()
{
	// Fetch currently worn items
	LLInventoryFetchObserver::item_ref_t ids;
	LLUUID item_id;
	for (S32 i = 0; i < (S32)LLWearableType::WT_COUNT; i++)
	{
		LLWearableType::EType type = (LLWearableType::EType)i;
		U32 count = gAgentWearables.getWearableCount(type);
		for (U32 index = 0; index < count; index++)
		{
			item_id = gAgentWearables.getWearableItemID(type, index);
			if (item_id.notNull())
			{
				ids.push_back(item_id);
			}
		}
	}

	// Fire & forget. The mInventoryObserver will catch inventory
	// updates and correct the UI as necessary.
	LLCurrentlyWorn worn;
	worn.fetchItems(ids);
}

void LLFloaterCustomize::updateInventoryUI()
{
	bool all_complete = true;
	bool is_complete = false;
	U32 perm_mask = 0x0;
	LLPanelEditWearable* panel;
	LLViewerInventoryItem* item;
	for (S32 i = 0; i < LLWearableType::WT_COUNT; ++i)
	{
		item = NULL;
		panel = mWearablePanelList[i];
		if (panel)
		{
			U32 index = panel->getWearableIndex();
			item = (LLViewerInventoryItem*)gAgentWearables.getWearableInventoryItem(panel->getType(), index);
		}
		if (item)
		{
			is_complete = item->isFinished();
			if (!is_complete)
			{
				all_complete = false;
			}
			perm_mask = item->getPermissions().getMaskOwner();
		}
		else
		{
			is_complete = false;
			perm_mask = 0x0;
		}
		if (i == sCurrentWearableType)
		{
			if (panel)
			{
				panel->setUIPermissions(perm_mask, is_complete);
			}
			bool is_vis = panel && item && is_complete && (perm_mask & PERM_MODIFY);
			childSetVisible("panel_container", is_vis);
		}
	}

	childSetEnabled("Make Outfit", all_complete);
}

void LLFloaterCustomize::updateScrollingPanelUI()
{
	LLPanelEditWearable* panel = mWearablePanelList[sCurrentWearableType];
	if (panel)
	{
		U32 index = panel->getWearableIndex();
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gAgentWearables.getWearableInventoryItem(panel->getType(),
																				index);
		BOOL allow_modify = FALSE;
		if (item)
		{
			U32 perm_mask = item->getPermissions().getMaskOwner();
			allow_modify = (perm_mask & PERM_MODIFY) ? item->isFinished()
													 : FALSE;
		}
		updateScrollingPanelList(allow_modify);
	}
}

void LLFloaterCustomize::updateWearableType(LLWearableType::EType type,
											LLWearable* wearable)
{
	setCurrentWearableType(type);

	U32 perm_mask = PERM_NONE;
	BOOL is_complete = FALSE;
	if (!wearable && gAgentWearables.getWearableCount(type))
	{
		wearable = gAgentWearables.getWearable(type, 0);	// Select the first layer
	}
	if (wearable)
	{
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gInventory.getItem(wearable->getItemID());
		if (item)
		{
			perm_mask = item->getPermissions().getMaskOwner();
			is_complete = item->isFinished();
			if (!is_complete)
			{
				item->fetchFromServer();
			}
		}
	}
	else
	{
		perm_mask = PERM_ALL;
		is_complete = TRUE;
	}

	setWearable(type, wearable, perm_mask, is_complete);
}
