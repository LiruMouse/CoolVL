/**
 * @file llpanelface.cpp
 * @brief Panel in the tools floater for editing face textures, colors, etc.
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

#include "llpanelface.h"

#include "llbutton.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "lllineeditor.h"
#include "llpluginclassmedia.h"
#include "llstring.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"

#include "llcolorswatch.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llselectmgr.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "lltooldraganddrop.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerstats.h"

//
// Methods
//

LLPanelFace::LLPanelFace(const std::string& name)
:	LLPanel(name),
	mComboTexGen(NULL),
	mTexScaleU(NULL),
	mTexScaleV(NULL),
	mTexOffsetU(NULL),
	mTexOffsetV(NULL),
	mTexRot(NULL),
	mCheckTexFlipS(NULL),
	mCheckTexFlipT(NULL)
{
}

LLPanelFace::~LLPanelFace()
{
	// Children all cleaned up by default view destructor.
}

//virtual
BOOL LLPanelFace::postBuild()
{
	setMouseOpaque(FALSE);

	mTextureCtrl = getChild<LLTextureCtrl>("texture control");
	mTextureCtrl->setDefaultImageAssetID(LLUUID(gSavedSettings.getString("DefaultObjectTexture")));
	mTextureCtrl->setCommitCallback(onCommitTexture);
	mTextureCtrl->setOnCancelCallback(onCancelTexture);
	mTextureCtrl->setOnSelectCallback(onSelectTexture);
	mTextureCtrl->setDragCallback(onDragTexture);
	mTextureCtrl->setCallbackUserData(this);
	mTextureCtrl->setFollowsTop();
	mTextureCtrl->setFollowsLeft();
	// Don't allow (no copy) or (no transfer) textures to be selected during
	// immediate mode
	mTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
	// Allow any texture to be used during non-immediate mode.
	mTextureCtrl->setNonImmediateFilterPermMask(PERM_NONE);
	LLAggregatePermissions texture_perms;
	if (LLSelectMgr::getInstance()->selectGetAggregateTexturePermissions(texture_perms))
	{
		BOOL can_copy = texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_EMPTY ||
						texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_ALL;
		BOOL can_transfer =	texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_EMPTY ||
							texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_ALL;
		mTextureCtrl->setCanApplyImmediately(can_copy && can_transfer);
	}
	else
	{
		mTextureCtrl->setCanApplyImmediately(FALSE);
	}

	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	mColorSwatch->setCommitCallback(onCommitColor);
	mColorSwatch->setOnCancelCallback(onCancelColor);
	mColorSwatch->setOnSelectCallback(onSelectColor);
	mColorSwatch->setCallbackUserData(this);
	mColorSwatch->setCanApplyImmediately(TRUE);

	mLabelColorTransp = getChild<LLTextBox>("color trans");

	mCtrlColorTransp = getChild<LLSpinCtrl>("ColorTrans");
	mCtrlColorTransp->setCommitCallback(onCommitAlpha);
	mCtrlColorTransp->setCallbackUserData(this);
	mCtrlColorTransp->setPrecision(0);

	mCheckFullbright = getChild<LLCheckBoxCtrl>("checkbox fullbright");
	mCheckFullbright->setCommitCallback(onCommitFullbright);
	mCheckFullbright->setCallbackUserData(this);

	mComboTexGen = getChild<LLComboBox>("combobox texgen");
	mComboTexGen->setCommitCallback(onCommitTexGen);
	mComboTexGen->setCallbackUserData(this);

	mLabelGlow = getChild<LLTextBox>("glow label");

	mCtrlGlow = getChild<LLSpinCtrl>("glow");
	mCtrlGlow->setCommitCallback(onCommitGlow);
	mCtrlGlow->setCallbackUserData(this);

	mLabelShininess = getChild<LLTextBox>("label shininess");

	mComboShininess = getChild<LLComboBox>("combobox shininess");
	mComboShininess->setCommitCallback(onCommitShiny);
	mComboShininess->setCallbackUserData(this);

	mLabelBumpiness = getChild<LLTextBox>("label bumpiness");

	mComboBumpiness = getChild<LLComboBox>("combobox bumpiness");
	mComboBumpiness->setCommitCallback(onCommitBump);
	mComboBumpiness->setCallbackUserData(this);

	mCheckPlanarAlign = getChild<LLCheckBoxCtrl>("checkbox planar align");
	mCheckPlanarAlign->setCommitCallback(onCommitPlanarAlign);
	mCheckPlanarAlign->setCallbackUserData(this);

	mLabelRepeats = getChild<LLTextBox>("rpt");
	mLabelTexScale = getChild<LLTextBox>("tex scale");
	mLabelTexOffset = getChild<LLTextBox>("tex offset");
	mLabelTexRotate = getChild<LLTextBox>("tex rotate");
	mLabelTexGen = getChild<LLTextBox>("tex gen");

	mRepeatsPerMeterText = getString("string repeats per meter");
	mRepeatsPerFaceText = getString("string repeats per face");

	mTexScaleU = getChild<LLSpinCtrl>("TexScaleU");
	mTexScaleU->setCommitCallback(onCommitTextureInfo);
	mTexScaleU->setCallbackUserData(this);

	mTexScaleV = getChild<LLSpinCtrl>("TexScaleV");
	mTexScaleV->setCommitCallback(onCommitTextureInfo);
	mTexScaleV->setCallbackUserData(this);

	mTexOffsetU = getChild<LLSpinCtrl>("TexOffsetU");
	mTexOffsetU->setCommitCallback(onCommitTextureInfo);
	mTexOffsetU->setCallbackUserData(this);

	mTexOffsetV = getChild<LLSpinCtrl>("TexOffsetV");
	mTexOffsetV->setCommitCallback(onCommitTextureInfo);
	mTexOffsetV->setCallbackUserData(this);

	mTexRot = getChild<LLSpinCtrl>("TexRot");
	mTexRot->setCommitCallback(onCommitTextureInfo);
	mTexRot->setCallbackUserData(this);

	mRepeats = getChild<LLSpinCtrl>("rptctrl");
	//mRepeats->setCommitCallback(onCommitTextureInfo);
	//mRepeats->setCallbackUserData(this);

	mCheckTexFlipS = getChild<LLCheckBoxCtrl>("checkbox flip s");
	mCheckTexFlipS->setCommitCallback(onCommitTextureInfo);
	mCheckTexFlipS->setCallbackUserData(this);

	mCheckTexFlipT = getChild<LLCheckBoxCtrl>("checkbox flip t");
	mCheckTexFlipT->setCommitCallback(onCommitTextureInfo);
	mCheckTexFlipT->setCallbackUserData(this);

	mButtonApply = getChild<LLButton>("button apply");
	mButtonApply->setClickedCallback(onClickApply, this);

	mLabelAlignMedia = getChild<LLTextBox>("textbox autofix");

	mButtonAlign = getChild<LLButton>("button align");
	mButtonAlign->setClickedCallback(onClickAutoFix, this);

	clearCtrls();

	return TRUE;
}

struct LLPanelFaceSetTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetTEFunctor(LLPanelFace* panel) : mPanel(panel) {}
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		BOOL valid;
		F32 value;

		LLComboBox* texgen = mPanel->getComboTexGen();
		LLSpinCtrl*	spinctrl = mPanel->getTexScaleU();
		LLCheckBoxCtrl*	check = mPanel->getTexFlipS();
		if (spinctrl)
		{
			valid = !spinctrl->getTentative() || !check->getTentative();
			if (valid)
			{
				value = spinctrl->get();
				if (check->get())
				{
					value = -value;
				}
				if (texgen && texgen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleS(te, value);
			}
		}

		spinctrl = mPanel->getTexScaleV();
		check = mPanel->getTexFlipT();
		if (spinctrl)
		{
			valid = !spinctrl->getTentative() || !check->getTentative();
			if (valid)
			{
				value = spinctrl->get();
				if (check->get())
				{
					value = -value;
				}
				if (texgen && texgen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleT(te, value);
			}
		}

		spinctrl = mPanel->getTexOffsetU();
		if (spinctrl)
		{
			valid = !spinctrl->getTentative();
			if (valid)
			{
				value = spinctrl->get();
				object->setTEOffsetS(te, value);
			}
		}

		spinctrl = mPanel->getTexOffsetV();
		if (spinctrl)
		{
			valid = !spinctrl->getTentative();
			if (valid)
			{
				value = spinctrl->get();
				object->setTEOffsetT(te, value);
			}
		}

		spinctrl = mPanel->getTexRot();
		if (spinctrl)
		{
			valid = !spinctrl->getTentative();
			if (valid)
			{
				value = spinctrl->get() * DEG_TO_RAD;
				object->setTERotation(te, value);
			}
		}
		return true;
	}

private:
	LLPanelFace* mPanel;
};

// Functor that aligns a face to mCenterFace
struct LLPanelFaceSetAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetAlignedTEFunctor(LLPanelFace* panel, LLFace* center_face) :
		mPanel(panel),
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return true;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{	//volume face does not exist, can't be aligned
			return true;
		}

		bool set_aligned = true;
		if (facep == mCenterFace)
		{
			set_aligned = false;
		}
		if (set_aligned)
		{
			LLVector2 uv_offset, uv_scale;
			F32 uv_rot;
			set_aligned = facep->calcAlignedPlanarTE(mCenterFace, &uv_offset, &uv_scale, &uv_rot);
			if (set_aligned)
			{
				object->setTEOffset(te, uv_offset.mV[VX], uv_offset.mV[VY]);
				object->setTEScale(te, uv_scale.mV[VX], uv_scale.mV[VY]);
				object->setTERotation(te, uv_rot);
			}
		}
		if (!set_aligned)
		{
			LLPanelFaceSetTEFunctor setfunc(mPanel);
			setfunc.apply(object, te);
		}
		return true;
	}

private:
	LLPanelFace* mPanel;
	LLFace* mCenterFace;
};

// Functor that tests if a face is aligned to mCenterFace
struct LLPanelFaceGetIsAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceGetIsAlignedTEFunctor(LLFace* center_face) :
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return false;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{	//volume face does not exist, can't be aligned
			return false;
		}

		if (facep == mCenterFace)
		{
			return true;
		}

		LLVector2 aligned_st_offset, aligned_st_scale;
		F32 aligned_st_rot;
		if (facep->calcAlignedPlanarTE(mCenterFace, &aligned_st_offset, &aligned_st_scale, &aligned_st_rot))
		{
			const LLTextureEntry* tep = facep->getTextureEntry();
			LLVector2 st_offset, st_scale;
			tep->getOffset(&st_offset.mV[VX], &st_offset.mV[VY]);
			tep->getScale(&st_scale.mV[VX], &st_scale.mV[VY]);
			F32 st_rot = tep->getRotation();
			// needs a fuzzy comparison, because of fp errors
			if (is_approx_equal_fraction(st_offset.mV[VX], aligned_st_offset.mV[VX], 16) &&
				is_approx_equal_fraction(st_offset.mV[VY], aligned_st_offset.mV[VY], 16) &&
				is_approx_equal_fraction(st_scale.mV[VX], aligned_st_scale.mV[VX], 16) &&
				is_approx_equal_fraction(st_scale.mV[VY], aligned_st_scale.mV[VY], 16) &&
				is_approx_equal_fraction(st_rot, aligned_st_rot, 14))
			{
				return true;
			}
		}
		return false;
	}

private:
	LLFace* mCenterFace;
};

struct LLPanelFaceSendFunctor : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* object)
	{
		object->sendTEUpdate();
		return true;
	}
};

//virtual
void LLPanelFace::refresh()
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	LLViewerObject* objectp = selection->getFirstObject();
	if (objectp && objectp->getPCode() == LL_PCODE_VOLUME &&
		objectp->permModify())
	{
		BOOL editable = objectp->permModify();
		mButtonApply->setEnabled(editable);

		// only turn on auto-align button if there is a media renderer and the
		// media is loaded
		mLabelAlignMedia->setEnabled(FALSE);
		mButtonAlign->setEnabled(FALSE);

		// Texture
		bool identical;
		LLUUID id;
		{
			struct f1 : public LLSelectedTEGetFunctor<LLUUID>
			{
				LLUUID get(LLViewerObject* object, S32 te)
				{
					LLViewerTexture* image = object->getTEImage(te);
					return image ? image->getID() : LLUUID::null;
				}
			} func;
			identical = selection->getSelectedTEValue(&func, id);
		}
		if (identical)
		{
			// All selected have the same texture
			mTextureCtrl->setTentative(FALSE);
			mTextureCtrl->setEnabled(editable);
			mTextureCtrl->setImageAssetID(id);
		}
		else if (id.isNull())
		{
			// None selected
			mTextureCtrl->setTentative(FALSE);
			mTextureCtrl->setEnabled(FALSE);
			mTextureCtrl->setImageAssetID(LLUUID::null);
		}
		else
		{
			// Tentative: multiple selected with different textures
			mTextureCtrl->setTentative(TRUE);
			mTextureCtrl->setEnabled(editable);
			mTextureCtrl->setImageAssetID(id);
		}

		if (LLViewerMedia::textureHasMedia(id))
		{
			mLabelAlignMedia->setEnabled(editable);
			mButtonAlign->setEnabled(editable);
		}

		LLAggregatePermissions texture_perms;
		if (LLSelectMgr::getInstance()->selectGetAggregateTexturePermissions(texture_perms))
		{
			BOOL can_copy =
				texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_EMPTY ||
				texture_perms.getValue(PERM_COPY) == LLAggregatePermissions::AP_ALL;
			BOOL can_transfer =
				texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_EMPTY ||
				texture_perms.getValue(PERM_TRANSFER) == LLAggregatePermissions::AP_ALL;
			mTextureCtrl->setCanApplyImmediately(can_copy && can_transfer);
		}
		else
		{
			mTextureCtrl->setCanApplyImmediately(FALSE);
		}

		// planar align
		bool align_planar = mCheckPlanarAlign->get();
		bool identical_planar_aligned = false;
		bool texgens_identical;
		bool is_planar;
		bool enabled;
		{
			struct f1 : public LLSelectedTEGetFunctor<bool>
			{
				bool get(LLViewerObject* object, S32 face)
				{
					return (object->getTE(face)->getTexGen() == LLTextureEntry::TEX_GEN_PLANAR);
				}
			} func;
			texgens_identical = selection->getSelectedTEValue(&func, is_planar);

			enabled = editable && texgens_identical && is_planar;
			if (align_planar && enabled)
			{
				struct f2 : public LLSelectedTEGetFunctor<LLFace*>
				{
					LLFace* get(LLViewerObject* object, S32 te)
					{
						return object->mDrawable ? object->mDrawable->getFace(te) : NULL;
					}
				} get_te_face_func;
				LLFace* last_face;
				selection->getSelectedTEValue(&get_te_face_func, last_face);
				LLPanelFaceGetIsAlignedTEFunctor get_is_aligned_func(last_face);
				// this will determine if the texture param controls are tentative:
				identical_planar_aligned = selection->applyToTEs(&get_is_aligned_func);
			}
		}
		mCheckPlanarAlign->setValue(align_planar && enabled);
		mCheckPlanarAlign->setEnabled(enabled);

		// Texture scale
		mLabelTexScale->setEnabled(editable);
		F32 scale_s = 1.f;
		{
			struct f2 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mScaleS;
				}
			} func;
			identical = selection->getSelectedTEValue(&func, scale_s);
		}
		identical = align_planar ? identical_planar_aligned : identical;
		mTexScaleU->setValue(editable ? llabs(scale_s) : 0);
		mTexScaleU->setTentative(LLSD((BOOL)(!identical)));
		mTexScaleU->setEnabled(editable);
		mCheckTexFlipS->setValue(LLSD((BOOL)(scale_s < 0 ? TRUE : FALSE)));
		mCheckTexFlipS->setTentative(LLSD((BOOL)((!identical) ? TRUE : FALSE)));
		mCheckTexFlipS->setEnabled(editable);

		F32 scale_t = 1.f;
		{
			struct f3 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mScaleT;
				}
			} func;
			identical = selection->getSelectedTEValue(&func, scale_t);
		}
		identical = align_planar ? identical_planar_aligned : identical;

		mTexScaleV->setValue(llabs(editable ? llabs(scale_t) : 0));
		mTexScaleV->setTentative(LLSD((BOOL)(!identical)));
		mTexScaleV->setEnabled(editable);
		mCheckTexFlipT->setValue(LLSD((BOOL)(scale_t< 0 ? TRUE : FALSE)));
		mCheckTexFlipT->setTentative(LLSD((BOOL)((!identical) ? TRUE : FALSE)));
		mCheckTexFlipT->setEnabled(editable);

		// Texture offset
		mLabelTexOffset->setEnabled(editable);

		F32 offset_s = 0.f;
		{
			struct f4 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mOffsetS;
				}
			} func;
			identical = selection->getSelectedTEValue(&func, offset_s);
		}
		identical = align_planar ? identical_planar_aligned : identical;
		mTexOffsetU->setValue(editable ? offset_s : 0);
		mTexOffsetU->setTentative(!identical);
		mTexOffsetU->setEnabled(editable);

		F32 offset_t = 0.f;
		{
			F32 offset_t = 0.f;
			struct f5 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mOffsetT;
				}
			} func;
			identical = selection->getSelectedTEValue(&func, offset_t);
		}
		identical = align_planar ? identical_planar_aligned : identical;
		mTexOffsetV->setValue(editable ? offset_t : 0);
		mTexOffsetV->setTentative(!identical);
		mTexOffsetV->setEnabled(editable);

		// Texture rotation
		mLabelTexRotate->setEnabled(editable);

		F32 rotation = 0.f;
		{
			struct f6 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->mRotation;
				}
			} func;
			identical = selection->getSelectedTEValue(&func, rotation);
		}
		identical = align_planar ? identical_planar_aligned : identical;
		mTexRot->setValue(editable ? rotation * RAD_TO_DEG : 0);
		mTexRot->setTentative(identical);
		mTexRot->setEnabled(editable);

		// Color swatch
		LLColor4 color = LLColor4::white;
		{
			struct f7 : public LLSelectedTEGetFunctor<LLColor4>
			{
				LLColor4 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->getColor();
				}
			} func;
			identical = selection->getSelectedTEValue(&func, color);
		}
		mColorSwatch->setOriginal(color);
		mColorSwatch->set(color, TRUE);
		mColorSwatch->setValid(editable);
		mColorSwatch->setEnabled(editable);
		mColorSwatch->setCanApplyImmediately(editable);

		mLabelColorTransp->setEnabled(editable);

		F32 transparency = (1.f - color.mV[VALPHA]) * 100.f;
		mCtrlColorTransp->setValue(editable ? transparency : 0);
		mCtrlColorTransp->setEnabled(editable);

		// Glow
		mLabelGlow->setEnabled(editable);
		F32 glow = 0.f;
		{
			struct f8 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return object->getTE(face)->getGlow();
				}
			} func;
			identical = selection->getSelectedTEValue(&func, glow);
		}

		mCtrlGlow->setValue(glow);
		mCtrlGlow->setEnabled(editable);
		mCtrlGlow->setTentative(!identical);

		// Shiny
		mLabelShininess->setEnabled(editable);
		F32 shinyf = 0.f;
		{
			struct f9 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getShiny());
				}
			} func;
			identical = selection->getSelectedTEValue(&func, shinyf);
		}

		LLCtrlSelectionInterface* combobox_shininess = mComboShininess->getSelectionInterface();
		if (combobox_shininess)
		{
			combobox_shininess->selectNthItem((S32)shinyf);
		}
		else
		{
			llwarns << "failed getSelectionInterface() for 'combobox shininess'"
					<< llendl;
		}
		mComboShininess->setEnabled(editable);
		mComboShininess->setTentative(!identical);

		// Bump
		mLabelBumpiness->setEnabled(editable);
		F32 bumpf = 0.f;
		{
			struct f10 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getBumpmap());
				}
			} func;
			identical = selection->getSelectedTEValue(&func, bumpf);
		}
		LLCtrlSelectionInterface* combobox_bumpiness = mComboBumpiness->getSelectionInterface();
		if (combobox_bumpiness)
		{
			combobox_bumpiness->selectNthItem((S32)bumpf);
		}
		else
		{
			llwarns << "failed getSelectionInterface() for 'combobox bumpiness'"
					<< llendl;
		}
		mComboBumpiness->setEnabled(editable);
		mComboBumpiness->setTentative(!identical);

		// Texgen
		mLabelTexGen->setEnabled(editable);
		F32 genf = 0.f;
		{
			struct f11 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getTexGen());
				}
			} func;
			identical = selection->getSelectedTEValue(&func, genf);
		}
		S32 selected_texgen = ((S32)genf) >> TEM_TEX_GEN_SHIFT;
		LLCtrlSelectionInterface* combobox_texgen = mComboTexGen->getSelectionInterface();
		if (combobox_texgen)
		{
			combobox_texgen->selectNthItem(selected_texgen);
		}
		else
		{
			llwarns << "failed getSelectionInterface() for 'combobox texgen'"
					<< llendl;
		}
		mComboTexGen->setEnabled(editable);
		mComboTexGen->setTentative(!identical);

		if (selected_texgen == 1)
		{
			mLabelTexScale->setText(mRepeatsPerMeterText);
			mTexScaleU->setValue(2.0f * mTexScaleU->getValue().asReal());
			mTexScaleV->setValue(2.0f * mTexScaleV->getValue().asReal());
		}
		else
		{
			mLabelTexScale->setText(mRepeatsPerFaceText);
		}

		F32 fullbrightf = 0.f;
		{
			struct f12 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					return (F32)(object->getTE(face)->getFullbright());
				}
			} func;
			identical = selection->getSelectedTEValue(&func, fullbrightf);
		}

		mCheckFullbright->setValue((S32)fullbrightf);
		mCheckFullbright->setEnabled(editable);
		mCheckFullbright->setTentative(!identical);

		// Repeats per meter
		mLabelRepeats->setEnabled(editable);

		F32 repeats = 1.f;
		{
			struct f13 : public LLSelectedTEGetFunctor<F32>
			{
				F32 get(LLViewerObject* object, S32 face)
				{
					U32 s_axis = VX;
					U32 t_axis = VY;
					// BUG: Only repeats along S axis
					// BUG: Only works for boxes.
					LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
					return object->getTE(face)->mScaleS / object->getScale().mV[s_axis];
				}
			} func;
			identical = selection->getSelectedTEValue(&func, repeats);
		}

		mRepeats->setValue(editable ? repeats : 0);
		mRepeats->setTentative(!identical);
		enabled = editable && (!mComboTexGen || mComboTexGen->getCurrentIndex() != 1);
		mRepeats->setEnabled(enabled);
	}
	else
	{
		// Disable all UICtrls
		clearCtrls();

		// Disable non-UICtrls
		mTextureCtrl->setImageAssetID(LLUUID::null);
		mTextureCtrl->setFallbackImageName("locked_image.j2c");
		mTextureCtrl->setEnabled(FALSE);  // this is a LLUICtrl, but we don't want it to have keyboard focus so we add it as a child, not a ctrl.
		//mTextureCtrl->setValid(FALSE);

		mColorSwatch->setEnabled(FALSE);
		mColorSwatch->setFallbackImageName("locked_image.j2c");
		mColorSwatch->setValid(FALSE);

		mLabelColorTransp->setEnabled(FALSE);
		mLabelRepeats->setEnabled(FALSE);
		mLabelTexScale->setEnabled(FALSE);
		mLabelTexOffset->setEnabled(FALSE);
		mLabelTexRotate->setEnabled(FALSE);
		mLabelTexGen->setEnabled(FALSE);
		mLabelGlow->setEnabled(FALSE);
		mLabelShininess->setEnabled(FALSE);
		mLabelBumpiness->setEnabled(FALSE);

		mLabelAlignMedia->setEnabled(FALSE);
		mButtonAlign->setEnabled(FALSE);

		mButtonApply->setEnabled(FALSE);
	}
}

void LLPanelFace::sendTexture()
{
	if (!mTextureCtrl->getTentative())
	{
		// we grab the item id first, because we want to do a permissions check
		// in the selection manager. ARGH!
		LLUUID id = mTextureCtrl->getImageItemID();
		if (id.isNull())
		{
			id = mTextureCtrl->getImageAssetID();
		}
		LLSelectMgr::getInstance()->selectionSetImage(id);
	}
}

void LLPanelFace::sendBump()
{
	U8 bump = (U8)mComboBumpiness->getCurrentIndex() & TEM_BUMP_MASK;
	LLSelectMgr::getInstance()->selectionSetBumpmap(bump);
}

void LLPanelFace::sendTexGen()
{
	U8 tex_gen = (U8)mComboTexGen->getCurrentIndex() << TEM_TEX_GEN_SHIFT;
	LLSelectMgr::getInstance()->selectionSetTexGen(tex_gen);
}

void LLPanelFace::sendShiny()
{
	U8 shiny = (U8)mComboShininess->getCurrentIndex() & TEM_SHINY_MASK;
	LLSelectMgr::getInstance()->selectionSetShiny(shiny);
}

void LLPanelFace::sendFullbright()
{
	U8 fullbright = mCheckFullbright->get() ? TEM_FULLBRIGHT_MASK : 0;
	LLSelectMgr::getInstance()->selectionSetFullbright(fullbright);
}

void LLPanelFace::sendColor()
{
	LLColor4 color = mColorSwatch->get();
	LLSelectMgr::getInstance()->selectionSetColorOnly(color);
}

void LLPanelFace::sendAlpha()
{
	F32 alpha = (100.f - mCtrlColorTransp->get()) / 100.f;
	LLSelectMgr::getInstance()->selectionSetAlphaOnly(alpha);
}

void LLPanelFace::sendGlow()
{
	F32 glow = mCtrlGlow->get();
	LLSelectMgr::getInstance()->selectionSetGlow(glow);
}

void LLPanelFace::sendTextureInfo()
{
	if (mCheckPlanarAlign->getValue().asBoolean())
	{
		struct f1 : public LLSelectedTEGetFunctor<LLFace *>
		{
			LLFace* get(LLViewerObject* object, S32 te)
			{
				return object->mDrawable ? object->mDrawable->getFace(te) : NULL;
			}
		} get_last_face_func;
		LLFace* last_face;
		LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_last_face_func,
																	   last_face);
		LLPanelFaceSetAlignedTEFunctor setfunc(this, last_face);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}
	else
	{
		LLPanelFaceSetTEFunctor setfunc(this);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

//
// Static functions
//

// static
F32 LLPanelFace::valueGlow(LLViewerObject* object, S32 face)
{
	return (F32)(object->getTE(face)->getGlow());
}

// static
void LLPanelFace::onCommitColor(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendColor();
}

// static
void LLPanelFace::onCommitAlpha(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendAlpha();
}

// static
void LLPanelFace::onCancelColor(LLUICtrl* ctrl, void* userdata)
{
	LLSelectMgr::getInstance()->selectionRevertColors();
}

// static
void LLPanelFace::onSelectColor(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	LLSelectMgr::getInstance()->saveSelectedObjectColors();
	self->sendColor();
}

// static
void LLPanelFace::onCommitBump(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendBump();
}

// static
void LLPanelFace::onCommitTexGen(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTexGen();
}

// static
void LLPanelFace::onCommitShiny(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendShiny();
}

// static
void LLPanelFace::onCommitFullbright(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendFullbright();
}

// static
void LLPanelFace::onCommitGlow(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendGlow();
}

// static
BOOL LLPanelFace::onDragTexture(LLUICtrl*, LLInventoryItem* item, void*)
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
void LLPanelFace::onCommitTexture(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT);

	self->sendTexture();
}

// static
void LLPanelFace::onCancelTexture(LLUICtrl* ctrl, void* userdata)
{
	LLSelectMgr::getInstance()->selectionRevertTextures();
}

// static
void LLPanelFace::onSelectTexture(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	LLSelectMgr::getInstance()->saveSelectedObjectTextures();
	self->sendTexture();
}

// static
void LLPanelFace::onCommitTextureInfo(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTextureInfo();
}

// Commit the number of repeats per meter
// static
void LLPanelFace::onClickApply(void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;

	gFocusMgr.setKeyboardFocus(NULL);

	F32 repeats_per_meter = self->mRepeats->getValue().asReal();
	LLSelectMgr::getInstance()->selectionTexScaleAutofit(repeats_per_meter);
}

// commit the fit media texture to prim button

struct LLPanelFaceSetMediaFunctor : public LLSelectedTEFunctor
{
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		// TODO: the media impl pointer should actually be stored by the texture
		viewer_media_t impl;
		impl = LLViewerMedia::getMediaImplFromTextureID(object->getTE(te)->getID());
		// only do this if it's a media texture
		if (impl.notNull())
		{
			LLPluginClassMedia* media = impl->getMediaPlugin();
			if (media)
			{
				S32 media_width = media->getWidth();
				S32 media_height = media->getHeight();
				S32 texture_width = media->getTextureWidth();
				S32 texture_height = media->getTextureHeight();
				F32 scale_s = (F32)media_width / (F32)texture_width;
				F32 scale_t = (F32)media_height / (F32)texture_height;

				// set scale and adjust offset
				object->setTEScaleS(te, scale_s);
				// don't need to flip Y anymore since QT does this for us now:
				object->setTEScaleT(te, scale_t);
				object->setTEOffsetS(te, -(1.0f - scale_s) / 2.0f);
				object->setTEOffsetT(te, -(1.0f - scale_t) / 2.0f);
			}
		}
		return true;
	};
};

void LLPanelFace::onClickAutoFix(void* userdata)
{
	LLPanelFaceSetMediaFunctor setfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

// static
void LLPanelFace::onCommitPlanarAlign(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->refresh();
	self->sendTextureInfo();
}
