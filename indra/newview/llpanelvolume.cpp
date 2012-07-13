/**
 * @file llpanelvolume.cpp
 * @brief Object editing (position, scale, etc.) in the tools floater
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

#include "llpanelvolume.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llclickaction.h"
#include "llcombobox.h"
#include "lleconomy.h"
#include "llerror.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llmaterialtable.h"
#include "llpermissionsflags.h"
#include "llspinctrl.h"
#include "llstring.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llvolume.h"
#include "m3math.h"
#include "material_codes.h"

#include "llagent.h"
#include "llcolorswatch.h"
#include "lldrawpool.h"
#include "llfirstuse.h"
#include "llflexibleobject.h"
#include "llmanipscale.h"
#include "llmeshrepository.h"
#include "llpanelinventory.h"
#include "llpreviewscript.h"
#include "llselectmgr.h"
#include "lltexturectrl.h"
#include "lltooldraganddrop.h"
#include "lltool.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"

// "Features" Tab

LLPanelVolume::LLPanelVolume(const std::string& name)
:	LLPanel(name),
	mComboMaterialItemCount(0)
{
	setMouseOpaque(FALSE);
}

LLPanelVolume::~LLPanelVolume()
{
	// Children all cleaned up by default view destructor.
}

BOOL LLPanelVolume::postBuild()
{
	mLabelSelectSingle = getChild<LLTextBox>("select_single");
	mLabelEditObject = getChild<LLTextBox>("edit_object");

	// Flexible objects parameters

	mCheckFlexiblePath = getChild<LLCheckBoxCtrl>("Flexible1D Checkbox Ctrl");
	mCheckFlexiblePath->setCommitCallback(onCommitIsFlexible);
	mCheckFlexiblePath->setCallbackUserData(this);

	mSpinFlexSections = getChild<LLSpinCtrl>("FlexNumSections");
	mSpinFlexSections->setCommitCallback(onCommitFlexible);
	mSpinFlexSections->setCallbackUserData(this);
	mSpinFlexSections->setValidateBeforeCommit(precommitValidate);

	mSpinFlexGravity = getChild<LLSpinCtrl>("FlexGravity");
	mSpinFlexGravity->setCommitCallback(onCommitFlexible);
	mSpinFlexGravity->setCallbackUserData(this);
	mSpinFlexGravity->setValidateBeforeCommit(precommitValidate);

	mSpinFlexFriction = getChild<LLSpinCtrl>("FlexFriction");
	mSpinFlexFriction->setCommitCallback(onCommitFlexible);
	mSpinFlexFriction->setCallbackUserData(this);
	mSpinFlexFriction->setValidateBeforeCommit(precommitValidate);

	mSpinFlexWind = getChild<LLSpinCtrl>("FlexWind");
	mSpinFlexWind->setCommitCallback(onCommitFlexible);
	mSpinFlexWind->setCallbackUserData(this);
	mSpinFlexWind->setValidateBeforeCommit(precommitValidate);

	mSpinFlexTension = getChild<LLSpinCtrl>("FlexTension");
	mSpinFlexTension->setCommitCallback(onCommitFlexible);
	mSpinFlexTension->setCallbackUserData(this);
	mSpinFlexTension->setValidateBeforeCommit(precommitValidate);

	mSpinFlexForceX = getChild<LLSpinCtrl>("FlexForceX");
	mSpinFlexForceX->setCommitCallback(onCommitFlexible);
	mSpinFlexForceX->setCallbackUserData(this);
	mSpinFlexForceX->setValidateBeforeCommit(precommitValidate);

	mSpinFlexForceY = getChild<LLSpinCtrl>("FlexForceY");
	mSpinFlexForceY->setCommitCallback(onCommitFlexible);
	mSpinFlexForceY->setCallbackUserData(this);
	mSpinFlexForceY->setValidateBeforeCommit(precommitValidate);

	mSpinFlexForceZ = getChild<LLSpinCtrl>("FlexForceZ");
	mSpinFlexForceZ->setCommitCallback(onCommitFlexible);
	mSpinFlexForceZ->setCallbackUserData(this);
	mSpinFlexForceZ->setValidateBeforeCommit(precommitValidate);

	// Light parameters

	mCheckEmitLight = getChild<LLCheckBoxCtrl>("Light Checkbox Ctrl");
	mCheckEmitLight->setCommitCallback(onCommitIsLight);
	mCheckEmitLight->setCallbackUserData(this);

	mSwatchLightColor = getChild<LLColorSwatchCtrl>("colorswatch");
	mSwatchLightColor->setOnCancelCallback(onLightCancelColor);
	mSwatchLightColor->setOnSelectCallback(onLightSelectColor);
	mSwatchLightColor->setCommitCallback(onCommitLight);
	mSwatchLightColor->setCallbackUserData(this);

	mTextureLight = getChild<LLTextureCtrl>("light texture control");
	mTextureLight->setCommitCallback(onCommitLight);
	mTextureLight->setCallbackUserData(this);
	mTextureLight->setOnCancelCallback(onLightCancelTexture);
	mTextureLight->setOnSelectCallback(onLightSelectTexture);
	mTextureLight->setDragCallback(onDragTexture);
	// Don't allow (no copy) or (no transfer) textures to be selected during
	// immediate mode
	mTextureLight->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
	// Allow any texture to be used during non-immediate mode.
	mTextureLight->setNonImmediateFilterPermMask(PERM_NONE);
	LLAggregatePermissions texture_perms;
	if (LLSelectMgr::getInstance()->selectGetAggregateTexturePermissions(texture_perms))
	{
		BOOL can_copy = texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_EMPTY ||
						texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_ALL;
		BOOL can_transfer = texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_EMPTY ||
							texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_ALL;
		mTextureLight->setCanApplyImmediately(can_copy && can_transfer);
	}
	else
	{
		mTextureLight->setCanApplyImmediately(FALSE);
	}

	mSpinLightIntensity = getChild<LLSpinCtrl>("Light Intensity");
	mSpinLightIntensity->setCommitCallback(onCommitLight);
	mSpinLightIntensity->setCallbackUserData(this);
	mSpinLightIntensity->setValidateBeforeCommit(precommitValidate);

	mSpinLightRadius = getChild<LLSpinCtrl>("Light Radius");
	mSpinLightRadius->setCommitCallback(onCommitLight);
	mSpinLightRadius->setCallbackUserData(this);
	mSpinLightRadius->setValidateBeforeCommit(precommitValidate);

	mSpinLightFalloff = getChild<LLSpinCtrl>("Light Falloff");
	mSpinLightFalloff->setCommitCallback(onCommitLight);
	mSpinLightFalloff->setCallbackUserData(this);
	mSpinLightFalloff->setValidateBeforeCommit(precommitValidate);

	mSpinLightFOV = getChild<LLSpinCtrl>("Light FOV");
	mSpinLightFOV->setCommitCallback(onCommitLight);
	mSpinLightFOV->setCallbackUserData(this);
	mSpinLightFOV->setValidateBeforeCommit(precommitValidate);

	mSpinLightFocus = getChild<LLSpinCtrl>("Light Focus");
	mSpinLightFocus->setCommitCallback(onCommitLight);
	mSpinLightFocus->setCallbackUserData(this);
	mSpinLightFocus->setValidateBeforeCommit(precommitValidate);

	mSpinLightAmbiance = getChild<LLSpinCtrl>("Light Ambiance");
	mSpinLightAmbiance->setCommitCallback(onCommitLight);
	mSpinLightAmbiance->setCallbackUserData(this);
	mSpinLightAmbiance->setValidateBeforeCommit(precommitValidate);

	// Physics parameters

	mLabelPhysicsShape = getChild<LLTextBox>("label physicsshapetype");
	mLabelPhysicsParams = getChild<LLTextBox>("label physicsparams");

	mComboPhysicsShape = getChild<LLComboBox>("Physics Shape Type Combo Ctrl");
	mComboPhysicsShape->setCommitCallback(sendPhysicsShapeType);
	mComboPhysicsShape->setCallbackUserData(this);

	mSpinPhysicsGravity = getChild<LLSpinCtrl>("Physics Gravity");
	mSpinPhysicsGravity->setCommitCallback(sendPhysicsGravity);
	mSpinPhysicsGravity->setCallbackUserData(this);

	mSpinPhysicsFriction = getChild<LLSpinCtrl>("Physics Friction");
	mSpinPhysicsFriction->setCommitCallback(sendPhysicsFriction);
	mSpinPhysicsFriction->setCallbackUserData(this);

	mSpinPhysicsDensity = getChild<LLSpinCtrl>("Physics Density");
	mSpinPhysicsDensity->setCommitCallback(sendPhysicsDensity);
	mSpinPhysicsDensity->setCallbackUserData(this);

	mSpinPhysicsRestitution = getChild<LLSpinCtrl>("Physics Restitution");
	mSpinPhysicsRestitution->setCommitCallback(sendPhysicsRestitution);
	mSpinPhysicsRestitution->setCallbackUserData(this);

	mPhysicsNone = getString("None");
	mPhysicsPrim = getString("Prim");
	mPhysicsHull = getString("Convex Hull");

	// Material parameters

	mFullBright = LLTrans::getString("Fullbright");

	std::map<std::string, std::string> material_name_map;
	material_name_map["Stone"]= LLTrans::getString("Stone");
	material_name_map["Metal"]= LLTrans::getString("Metal");
	material_name_map["Glass"]= LLTrans::getString("Glass");
	material_name_map["Wood"]= LLTrans::getString("Wood");
	material_name_map["Flesh"]= LLTrans::getString("Flesh");
	material_name_map["Plastic"]= LLTrans::getString("Plastic");
	material_name_map["Rubber"]= LLTrans::getString("Rubber");
	material_name_map["Light"]= LLTrans::getString("Light");
	LLMaterialTable::basic.initTableTransNames(material_name_map);

	mLabelMaterial = getChild<LLTextBox>("label material");
	mComboMaterial = getChild<LLComboBox>("material");
	childSetCommitCallback("material", onCommitMaterial, this);
	mComboMaterial->removeall();

	for (LLMaterialTable::info_list_t::iterator
			iter = LLMaterialTable::basic.mMaterialInfoList.begin();
		 iter != LLMaterialTable::basic.mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo* minfop = *iter;
		if (minfop->mMCode != LL_MCODE_LIGHT)
		{
			mComboMaterial->add(minfop->mName);
		}
	}
	mComboMaterialItemCount = mComboMaterial->getItemCount();

	// Start with everyone disabled
	clearCtrls();

	return TRUE;
}

void LLPanelVolume::getState()
{
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
	LLViewerObject* root_objectp = objectp;
	if (!objectp)
	{
		objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
		// *FIX: shouldn't we just keep the child ?
		if (objectp)
		{
			LLViewerObject* parentp = objectp->getRootEdit();
			root_objectp = parentp ? parentp : objectp;
		}
	}

	LLVOVolume* volobjp = NULL;
	if (objectp && objectp->getPCode() == LL_PCODE_VOLUME)
	{
		volobjp = (LLVOVolume*)objectp;
	}

	if (!objectp)
	{
		// Forfeit focus
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			gFocusMgr.setKeyboardFocus(NULL);
		}

		// Disable all text input fields
		clearCtrls();

		return;
	}

	BOOL owners_identical;
	LLUUID owner_id;
	std::string owner_name;
	owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id,
																  owner_name);

	// BUG ?  Check for all objects being editable ?
	BOOL editable = root_objectp->permModify();
	BOOL single_volume = LLSelectMgr::getInstance()->selectionAllPCode(LL_PCODE_VOLUME) &&
						 LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1;

	// Select Single Message
	if (single_volume)
	{
		mLabelSelectSingle->setVisible(FALSE);
		mLabelEditObject->setVisible(TRUE);
		mLabelEditObject->setEnabled(TRUE);
	}
	else
	{
		mLabelSelectSingle->setVisible(TRUE);
		mLabelSelectSingle->setEnabled(TRUE);
		mLabelEditObject->setVisible(FALSE);
	}

	// Light properties
	BOOL is_light = volobjp && volobjp->getIsLight();
	mCheckEmitLight->setValue(is_light);
	mCheckEmitLight->setEnabled(editable && single_volume && volobjp);

	if (is_light && editable && single_volume)
	{
		mSwatchLightColor->setEnabled(TRUE);
		mSwatchLightColor->setValid(TRUE);
		mSwatchLightColor->set(volobjp->getLightBaseColor());

		mTextureLight->setEnabled(TRUE);
		mTextureLight->setValid(TRUE);
		mTextureLight->setImageAssetID(volobjp->getLightTextureID());

		mSpinLightIntensity->setEnabled(TRUE);
		mSpinLightRadius->setEnabled(TRUE);
		mSpinLightFalloff->setEnabled(TRUE);

		mSpinLightFOV->setEnabled(TRUE);
		mSpinLightFocus->setEnabled(TRUE);
		mSpinLightAmbiance->setEnabled(TRUE);

		mSpinLightIntensity->setValue(volobjp->getLightIntensity());
		mSpinLightRadius->setValue(volobjp->getLightRadius());
		mSpinLightFalloff->setValue(volobjp->getLightFalloff());

		LLVector3 params = volobjp->getSpotLightParams();
		mSpinLightFOV->setValue(params.mV[0]);
		mSpinLightFocus->setValue(params.mV[1]);
		mSpinLightAmbiance->setValue(params.mV[2]);

		mLightSavedColor = volobjp->getLightColor();
	}
	else
	{
		mSpinLightIntensity->clear();
		mSpinLightRadius->clear();
		mSpinLightFalloff->clear();

		mSwatchLightColor->setEnabled(FALSE);
		mSwatchLightColor->setValid(FALSE);

		mTextureLight->setEnabled(FALSE);
		mTextureLight->setValid(FALSE);

		mSpinLightIntensity->setEnabled(FALSE);
		mSpinLightRadius->setEnabled(FALSE);
		mSpinLightFalloff->setEnabled(FALSE);

		mSpinLightFOV->setEnabled(FALSE);
		mSpinLightFocus->setEnabled(FALSE);
		mSpinLightAmbiance->setEnabled(FALSE);
	}

	// Flexible properties
	BOOL is_flexible = volobjp && volobjp->isFlexible();
	mCheckFlexiblePath->setValue(is_flexible);
	if (is_flexible || (volobjp && volobjp->canBeFlexible()))
	{
		mCheckFlexiblePath->setEnabled(editable && single_volume && volobjp);
	}
	else
	{
		mCheckFlexiblePath->setEnabled(FALSE);
	}
	if (is_flexible && editable && single_volume)
	{
		mSpinFlexSections->setVisible(TRUE);
		mSpinFlexGravity->setVisible(TRUE);
		mSpinFlexFriction->setVisible(TRUE);
		mSpinFlexWind->setVisible(TRUE);
		mSpinFlexTension->setVisible(TRUE);
		mSpinFlexForceX->setVisible(TRUE);
		mSpinFlexForceY->setVisible(TRUE);
		mSpinFlexForceZ->setVisible(TRUE);

		mSpinFlexSections->setEnabled(TRUE);
		mSpinFlexGravity->setEnabled(TRUE);
		mSpinFlexFriction->setEnabled(TRUE);
		mSpinFlexWind->setEnabled(TRUE);
		mSpinFlexTension->setEnabled(TRUE);
		mSpinFlexForceX->setEnabled(TRUE);
		mSpinFlexForceY->setEnabled(TRUE);
		mSpinFlexForceZ->setEnabled(TRUE);

		LLFlexibleObjectData* attributes = (LLFlexibleObjectData *)objectp->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);

		mSpinFlexSections->setValue((F32)attributes->getSimulateLOD());
		mSpinFlexGravity->setValue(attributes->getGravity());
		mSpinFlexFriction->setValue(attributes->getAirFriction());
		mSpinFlexWind->setValue(attributes->getWindSensitivity());
		mSpinFlexTension->setValue(attributes->getTension());
		mSpinFlexForceX->setValue(attributes->getUserForce().mV[VX]);
		mSpinFlexForceY->setValue(attributes->getUserForce().mV[VY]);
		mSpinFlexForceZ->setValue(attributes->getUserForce().mV[VZ]);
	}
	else
	{
		mSpinFlexSections->clear();
		mSpinFlexGravity->clear();
		mSpinFlexFriction->clear();
		mSpinFlexWind->clear();
		mSpinFlexTension->clear();
		mSpinFlexForceX->clear();
		mSpinFlexForceY->clear();
		mSpinFlexForceZ->clear();

		mSpinFlexSections->setEnabled(FALSE);
		mSpinFlexGravity->setEnabled(FALSE);
		mSpinFlexFriction->setEnabled(FALSE);
		mSpinFlexWind->setEnabled(FALSE);
		mSpinFlexTension->setEnabled(FALSE);
		mSpinFlexForceX->setEnabled(FALSE);
		mSpinFlexForceY->setEnabled(FALSE);
		mSpinFlexForceZ->setEnabled(FALSE);
	}

	// Material properties

	// Update material part
	// slightly inefficient - materials are unique per object, not per TE
	U8 material_code = 0;
	struct f : public LLSelectedTEGetFunctor<U8>
	{
		U8 get(LLViewerObject* object, S32 te)
		{
			return object->getMaterial();
		}
	} func;
	bool material_same = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&func, material_code);
	if (editable && single_volume && material_same)
	{
		mComboMaterial->setEnabled(TRUE);
		mLabelMaterial->setEnabled(TRUE);
		if (material_code == LL_MCODE_LIGHT)
		{
			if (mComboMaterial->getItemCount() == mComboMaterialItemCount)
			{
				mComboMaterial->add(mFullBright);
			}
			mComboMaterial->setSimple(mFullBright);
		}
		else
		{
			if (mComboMaterial->getItemCount() != mComboMaterialItemCount)
			{
				mComboMaterial->remove(mFullBright);
			}
			// *TODO:Translate
			mComboMaterial->setSimple(std::string(LLMaterialTable::basic.getName(material_code)));
		}
	}
	else
	{
		mComboMaterial->setEnabled(FALSE);
		mLabelMaterial->setEnabled(FALSE);
	}

	// Physics properties

	bool is_physical = root_objectp && root_objectp->usePhysics();
	if (is_physical && editable)
	{
		mLabelPhysicsParams->setEnabled(TRUE);
		mSpinPhysicsGravity->setValue(objectp->getPhysicsGravity());
		mSpinPhysicsGravity->setEnabled(TRUE);
		mSpinPhysicsFriction->setValue(objectp->getPhysicsFriction());
		mSpinPhysicsFriction->setEnabled(TRUE);
		mSpinPhysicsDensity->setValue(objectp->getPhysicsDensity());
		mSpinPhysicsDensity->setEnabled(TRUE);
		mSpinPhysicsRestitution->setValue(objectp->getPhysicsRestitution());
		mSpinPhysicsRestitution->setEnabled(TRUE);
	}
	else
	{
		mLabelPhysicsParams->setEnabled(FALSE);
		mSpinPhysicsGravity->clear();
		mSpinPhysicsGravity->setEnabled(FALSE);
		mSpinPhysicsFriction->clear();
		mSpinPhysicsFriction->setEnabled(FALSE);
		mSpinPhysicsDensity->clear();
		mSpinPhysicsDensity->setEnabled(FALSE);
		mSpinPhysicsRestitution->clear();
		mSpinPhysicsRestitution->setEnabled(FALSE);
	}

	// Update the physics shape combo to include allowed physics shapes
	mComboPhysicsShape->removeall();
	mComboPhysicsShape->add(mPhysicsNone, LLSD(1));

	BOOL is_mesh = FALSE;
	LLSculptParams* sculpt_params;
	sculpt_params = (LLSculptParams*)objectp->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
	if (sculpt_params)
	{
		is_mesh = (sculpt_params->getSculptType() & LL_SCULPT_TYPE_MASK) == LL_SCULPT_TYPE_MESH;
	}

	if (is_mesh && objectp)
	{
		const LLVolumeParams &volume_params = objectp->getVolume()->getParams();
		LLUUID mesh_id = volume_params.getSculptID();
		if (gMeshRepo.hasPhysicsShape(mesh_id))
		{
			// if a mesh contains an uploaded or decomposed physics mesh,
			// allow 'Prim'
			mComboPhysicsShape->add(mPhysicsPrim, LLSD(0));
		}
	}
	else
	{
		// simple prims always allow physics shape prim
		mComboPhysicsShape->add(mPhysicsPrim, LLSD(0));
	}

	mComboPhysicsShape->add(mPhysicsHull, LLSD(2));
	mComboPhysicsShape->setValue(LLSD(objectp->getPhysicsShapeType()));
	mComboPhysicsShape->setEnabled(editable);
	mLabelPhysicsShape->setEnabled(editable);

	mObject = objectp;
	mRootObject = root_objectp;
}

// static
BOOL LLPanelVolume::precommitValidate(LLUICtrl* ctrl, void* userdata)
{
	// TODO: Richard will fill this in later.
	// FALSE means that validation failed and new value should not be commited.
	return TRUE;
}

void LLPanelVolume::refresh()
{
	getState();
	if (mObject.notNull() && mObject->isDead())
	{
		mObject = NULL;
	}

	if (mRootObject.notNull() && mRootObject->isDead())
	{
		mRootObject = NULL;
	}

#if 0 // Whatever the user's hardware, they should be able to *build* lights !
	bool visible = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_DEFERRED) > 0;
	mSpinLightFOV->setVisible(visible);
	mSpinLightFocus->setVisible(visible);
	mSpinLightAmbiance->setVisible(visible);
	mTextureLight->setVisible(visible);
#endif

	bool enable_physics = false;
	LLSD sim_features;
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		LLSD sim_features;
		region->getSimulatorFeatures(sim_features);
		enable_physics = sim_features.has("PhysicsShapeTypes");
	}
	mLabelPhysicsShape->setVisible(enable_physics);
	mComboPhysicsShape->setVisible(enable_physics);
	mLabelPhysicsParams->setVisible(enable_physics);
	mSpinPhysicsGravity->setVisible(enable_physics);
	mSpinPhysicsFriction->setVisible(enable_physics);
	mSpinPhysicsDensity->setVisible(enable_physics);
	mSpinPhysicsRestitution->setVisible(enable_physics);
    // *TODO: add/remove individual physics shape types as per the
	// PhysicsShapeTypes simulator features
}

// virtual
void LLPanelVolume::clearCtrls()
{
	LLPanel::clearCtrls();

	mLabelSelectSingle->setEnabled(FALSE);
	mLabelSelectSingle->setVisible(TRUE);
	mLabelEditObject->setEnabled(FALSE);
	mLabelEditObject->setVisible(FALSE);

	mCheckEmitLight->setEnabled(FALSE);
	mSwatchLightColor->setEnabled(FALSE);
	mSwatchLightColor->setValid(FALSE);

	mTextureLight->setEnabled(FALSE);
	mTextureLight->setValid(FALSE);

	mSpinLightIntensity->setEnabled(FALSE);
	mSpinLightRadius->setEnabled(FALSE);
	mSpinLightFalloff->setEnabled(FALSE);
	mSpinLightFOV->setEnabled(FALSE);
	mSpinLightFocus->setEnabled(FALSE);
	mSpinLightAmbiance->setEnabled(FALSE);

	mCheckFlexiblePath->setEnabled(FALSE);
	mSpinFlexSections->setEnabled(FALSE);
	mSpinFlexGravity->setEnabled(FALSE);
	mSpinFlexFriction->setEnabled(FALSE);
	mSpinFlexWind->setEnabled(FALSE);
	mSpinFlexTension->setEnabled(FALSE);
	mSpinFlexForceX->setEnabled(FALSE);
	mSpinFlexForceY->setEnabled(FALSE);
	mSpinFlexForceZ->setEnabled(FALSE);

	mSpinPhysicsGravity->setEnabled(FALSE);
	mSpinPhysicsFriction->setEnabled(FALSE);
	mSpinPhysicsDensity->setEnabled(FALSE);
	mSpinPhysicsRestitution->setEnabled(FALSE);

	mComboMaterial->setEnabled(FALSE);
	mLabelMaterial->setEnabled(FALSE);
}

//
// Static functions
//

void LLPanelVolume::sendIsLight()
{
	LLViewerObject* objectp = mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}
	LLVOVolume *volobjp = (LLVOVolume *)objectp;

	BOOL value = mCheckEmitLight->getValue();
	volobjp->setIsLight(value);
	llinfos << "update light sent" << llendl;
}

void LLPanelVolume::sendIsFlexible()
{
	LLViewerObject* objectp = mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}
	LLVOVolume *volobjp = (LLVOVolume *)objectp;

	BOOL is_flexible = mCheckFlexiblePath->getValue();
	if (is_flexible)
	{
		LLFirstUse::useFlexible();

		if (objectp->getClickAction() == CLICK_ACTION_SIT)
		{
			LLSelectMgr::getInstance()->selectionSetClickAction(CLICK_ACTION_NONE);
		}
	}

	if (volobjp->setIsFlexible(is_flexible))
	{
		mObject->sendShapeUpdate();
		LLSelectMgr::getInstance()->selectionUpdatePhantom(volobjp->flagPhantom());
	}

	llinfos << "update flexible sent" << llendl;
}

void LLPanelVolume::sendPhysicsShapeType(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*) userdata;
	if (!self || !ctrl) return;
	U8 type = ctrl->getValue().asInteger();
	LLSelectMgr::getInstance()->selectionSetPhysicsType(type);

	self->refreshCost();
}

void LLPanelVolume::sendPhysicsGravity(LLUICtrl* ctrl, void* userdata)
{
	F32 val = ctrl->getValue().asReal();
	LLSelectMgr::getInstance()->selectionSetGravity(val);
}

void LLPanelVolume::sendPhysicsFriction(LLUICtrl* ctrl, void* userdata)
{
	F32 val = ctrl->getValue().asReal();
	LLSelectMgr::getInstance()->selectionSetFriction(val);
}

void LLPanelVolume::sendPhysicsRestitution(LLUICtrl* ctrl, void* userdata)
{
	F32 val = ctrl->getValue().asReal();
	LLSelectMgr::getInstance()->selectionSetRestitution(val);
}

void LLPanelVolume::sendPhysicsDensity(LLUICtrl* ctrl, void* userdata)
{
	F32 val = ctrl->getValue().asReal();
	LLSelectMgr::getInstance()->selectionSetDensity(val);
}

void LLPanelVolume::refreshCost()
{
	LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();

	if (obj)
	{
		obj->getObjectCost();
	}
}

void LLPanelVolume::onLightCancelColor(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	if (self)
	{
		self->mSwatchLightColor->setColor(self->mLightSavedColor);
		onLightSelectColor(NULL, userdata);
	}
}

void LLPanelVolume::onLightCancelTexture(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	if (self)
	{
		self->mTextureLight->setImageAssetID(self->mLightSavedTexture);
	}
}

void LLPanelVolume::onLightSelectColor(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	if (!self || !self->mObject ||
		self->mObject->getPCode() != LL_PCODE_VOLUME)
	{
		return;
	}
	LLVOVolume* volobjp = (LLVOVolume*)self->mObject.get();
	if (!volobjp) return;

	LLColor4 clr = self->mSwatchLightColor->get();
	LLColor3 clr3(clr);
	volobjp->setLightColor(clr3);
	self->mLightSavedColor = clr;
}

void LLPanelVolume::onLightSelectTexture(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	if (!self || !self->mObject ||
		self->mObject->getPCode() != LL_PCODE_VOLUME)
	{
		return;
	}
	LLVOVolume* volobjp = (LLVOVolume*)self->mObject.get();
	if (!volobjp) return;

	LLUUID id = self->mTextureLight->getImageAssetID();
	volobjp->setLightTextureID(id);
	self->mLightSavedTexture = id;
}

// static
BOOL LLPanelVolume::onDragTexture(LLUICtrl* ctrl, LLInventoryItem* item,
								  void* userdata)
{
	BOOL accept = TRUE;
	for (LLObjectSelection::root_iterator
			iter = LLSelectMgr::getInstance()->getSelection()->root_begin(),
			end = LLSelectMgr::getInstance()->getSelection()->root_end();
		 iter != end; ++iter)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if (!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
		{
			accept = FALSE;
			break;
		}
	}
	return accept;
}

// static
void LLPanelVolume::onCommitMaterial(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	LLComboBox* box = (LLComboBox*)ctrl;
	if (self && box)
	{
		// apply the currently selected material to the object
		const std::string& material_name = box->getSimple();
		if (material_name != self->mFullBright)
		{
			U8 material_code = LLMaterialTable::basic.getMCode(material_name);
			LLSelectMgr::getInstance()->selectionSetMaterial(material_code);
		}
	}
}

// static
void LLPanelVolume::onCommitLight(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	if (!self || !self->mObject ||
		self->mObject->getPCode() != LL_PCODE_VOLUME)
	{
		return;
	}
	LLVOVolume* volobjp = (LLVOVolume*)self->mObject.get();
	if (!volobjp) return;

	volobjp->setLightIntensity((F32)self->mSpinLightIntensity->getValue().asReal());
	volobjp->setLightRadius((F32)self->mSpinLightRadius->getValue().asReal());
	volobjp->setLightFalloff((F32)self->mSpinLightFalloff->getValue().asReal());

	LLColor4 clr = self->mSwatchLightColor->get();
	volobjp->setLightColor(LLColor3(clr));

	LLUUID id = self->mTextureLight->getImageAssetID();
	if (id.notNull())
	{
		if (!volobjp->isLightSpotlight())
		{	//this commit is making this a spot light, set UI to default params
			volobjp->setLightTextureID(id);
			LLVector3 spot_params = volobjp->getSpotLightParams();
			self->mSpinLightFOV->setValue(spot_params.mV[0]);
			self->mSpinLightFocus->setValue(spot_params.mV[1]);
			self->mSpinLightAmbiance->setValue(spot_params.mV[2]);
		}
		else
		{	//modifying existing params
			LLVector3 spot_params;
			spot_params.mV[0] = (F32)self->mSpinLightFOV->getValue().asReal();
			spot_params.mV[1] = (F32)self->mSpinLightFocus->getValue().asReal();
			spot_params.mV[2] = (F32)self->mSpinLightAmbiance->getValue().asReal();
			volobjp->setSpotLightParams(spot_params);
		}
	}
	else if (volobjp->isLightSpotlight())
	{	//no longer a spot light
		volobjp->setLightTextureID(id);
		//self->mSpinLightFOV->setEnabled(FALSE);
		//self->mSpinLightFocus->setEnabled(FALSE);
		//self->mSpinLightAmbiance->setEnabled(FALSE);
	}
}

// static
void LLPanelVolume::onCommitIsLight(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	if (self)
	{
		self->sendIsLight();
	}
}

//----------------------------------------------------------------------------

// static
void LLPanelVolume::onCommitFlexible(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	if (!self) return;
	LLViewerObject* objectp = self->mObject;
	if (!objectp || objectp->getPCode() != LL_PCODE_VOLUME)
	{
		return;
	}

	LLFlexibleObjectData* attributes = (LLFlexibleObjectData*)objectp->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
	if (attributes)
	{
		LLFlexibleObjectData new_attributes;
		new_attributes = *attributes;
		new_attributes.setSimulateLOD(self->mSpinFlexSections->getValue().asInteger());
		new_attributes.setGravity((F32)self->mSpinFlexGravity->getValue().asReal());
		new_attributes.setAirFriction((F32)self->mSpinFlexFriction->getValue().asReal());
		new_attributes.setWindSensitivity((F32)self->mSpinFlexWind->getValue().asReal());
		new_attributes.setTension((F32)self->mSpinFlexTension->getValue().asReal());
		F32 fx = (F32)self->mSpinFlexForceX->getValue().asReal();
		F32 fy = (F32)self->mSpinFlexForceY->getValue().asReal();
		F32 fz = (F32)self->mSpinFlexForceZ->getValue().asReal();
		LLVector3 force(fx, fy, fz);

		new_attributes.setUserForce(force);
		objectp->setParameterEntry(LLNetworkData::PARAMS_FLEXIBLE, new_attributes, true);
	}

	// Values may fail validation
	self->refresh();
}

// static
void LLPanelVolume::onCommitIsFlexible(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	if (self)
	{
		self->sendIsFlexible();
	}
}
