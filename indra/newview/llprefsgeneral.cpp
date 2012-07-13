/** 
 * @file llprefsgeneral.cpp
 * @brief General preferences panel in preferences floater
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Adapted by Henri Beauchamp from llpanelgeneral.cpp
 * Copyright (c) 2001-2009 Linden Research, Inc. (c) 2011 Henri Beauchamp
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

#include "llprefsgeneral.h"

#include "llavatarnamecache.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llradiogroup.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llcolorswatch.h"
#include "llstartup.h"
#include "llurlsimstring.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"

class LLPrefsGeneralImpl : public LLPanel
{
public:
	LLPrefsGeneralImpl();
	/*virtual*/ ~LLPrefsGeneralImpl() {};

	/*virtual*/ void refresh();

	void apply();
	void cancel();

private:
	void refreshValues();

private:
	LLComboBox*		mFadeOutNamesCombo;
	LLComboBox*		mMaturityCombo;
	LLComboBox*		mLanguageCombo;
	LLTextBox*		mMaturityText;
	LLTextBox*		mDisplayNameText1;
	LLTextBox*		mDisplayNameText2;
	LLTextBox*		mNoDisplayNameText;
	LLRadioGroup*	mDisplayNameRadio;
	LLCheckBoxCtrl*	mLegacyNamesCheck;
	LLCheckBoxCtrl*	mOmitResidentCheck;
	bool			mHasDisplayNames;
	bool			mCanChooseMaturity;

	BOOL			mForceShowGrid;
	BOOL			mLoginLastLocation;
	BOOL			mShowStartLocation;
	BOOL			mRenderHideGroupTitleAll;
	BOOL			mRenderHideGroupTitle;
	BOOL			mLanguageIsPublic;
	BOOL			mRenderNameHideSelf;
	BOOL			mSmallAvatarNames;
	BOOL			mUIAutoScale;
	BOOL			mLegacyNamesForFriends;
	BOOL			mOmitResidentAsLastName;
	F32				mChatBubbleOpacity;
	F32				mUIScaleFactor;
	S32				mRenderName;
	S32				mAFKTimeout;
	U32				mPreferredMaturity;
	U32				mDisplayNamesUsage;
	LLColor4		mEffectColor;
	std::string		mLanguage;
};

LLPrefsGeneralImpl::LLPrefsGeneralImpl()
:	LLPanel(std::string("General Preferences"))
{
	LLUICtrlFactory::getInstance()->buildPanel(this,
											   "panel_preferences_general.xml");

	mDisplayNameText1 = getChild<LLTextBox>("display_names_text_box1");
	mDisplayNameText2 = getChild<LLTextBox>("display_names_text_box2");
	mNoDisplayNameText = getChild<LLTextBox>("no_display_names_text_box");
	mDisplayNameRadio = getChild<LLRadioGroup>("display_names_usage");
	mLegacyNamesCheck = getChild<LLCheckBoxCtrl>("legacy_names_for_friends_check");
	mOmitResidentCheck = getChild<LLCheckBoxCtrl>("omit_resident_last_name_check");

	mFadeOutNamesCombo = getChild<LLComboBox>("fade_out_combobox");
	mMaturityCombo = getChild<LLComboBox>("maturity_desired_combobox");
	mMaturityText = getChild<LLTextBox>("maturity_desired_textbox");
	mLanguageCombo = getChild<LLComboBox>("language_combobox");

	mHasDisplayNames = LLStartUp::getStartupState() < STATE_STARTED ||
					   LLAvatarNameCache::hasNameLookupURL();

	if (LLStartUp::getStartupState() < STATE_STARTED)
	{
		mCanChooseMaturity = true;
	}
	else
	{
		mCanChooseMaturity = gAgent.isMature() || gAgent.isGodlike();
		if (mCanChooseMaturity && !gAgent.isAdult() && !gAgent.isGodlike())
		{
			// If they're not adult or a god, they shouldn't see the adult
			// selection, so delete it
			mMaturityCombo->remove(0);
		}
	}

	refresh();
}

void LLPrefsGeneralImpl::refreshValues()
{
	mForceShowGrid				= gSavedSettings.getBOOL("ForceShowGrid");
	mLoginLastLocation			= gSavedSettings.getBOOL("LoginLastLocation");
	mShowStartLocation			= gSavedSettings.getBOOL("ShowStartLocation");
	mRenderHideGroupTitleAll	= gSavedSettings.getBOOL("RenderHideGroupTitleAll");
	mRenderHideGroupTitle		= gSavedSettings.getBOOL("RenderHideGroupTitle");
	mLanguageIsPublic			= gSavedSettings.getBOOL("LanguageIsPublic");
	mRenderNameHideSelf			= gSavedSettings.getBOOL("RenderNameHideSelf");
	mSmallAvatarNames			= gSavedSettings.getBOOL("SmallAvatarNames");
	mUIAutoScale				= gSavedSettings.getBOOL("UIAutoScale");
	mLegacyNamesForFriends		= gSavedSettings.getBOOL("LegacyNamesForFriends");
	mOmitResidentAsLastName		= gSavedSettings.getBOOL("OmitResidentAsLastName");
	mChatBubbleOpacity			= gSavedSettings.getF32("ChatBubbleOpacity");
	mRenderName					= gSavedSettings.getS32("RenderName");
	mAFKTimeout					= gSavedSettings.getS32("AFKTimeout");
	mUIScaleFactor				= gSavedSettings.getF32("UIScaleFactor");
	mPreferredMaturity			= gSavedSettings.getU32("PreferredMaturity");
	mDisplayNamesUsage			= gSavedSettings.getU32("DisplayNamesUsage");
	mEffectColor				= gSavedSettings.getColor4("EffectColor");
	mLanguage					= gSavedSettings.getString("Language");
}

void LLPrefsGeneralImpl::refresh()
{
	refreshValues();

	mFadeOutNamesCombo->setCurrentByIndex(mRenderName);

	mMaturityCombo->setValue((S32)gSavedSettings.getU32("PreferredMaturity"));
	mMaturityCombo->setVisible(mCanChooseMaturity);
	mMaturityText->setVisible(!mCanChooseMaturity);
	std::string selected_item_label = mMaturityCombo->getSelectedItemLabel();
	mMaturityText->setValue(selected_item_label);

	mDisplayNameText1->setVisible(mHasDisplayNames);
	mDisplayNameText2->setVisible(mHasDisplayNames);
	mNoDisplayNameText->setVisible(!mHasDisplayNames);
	mDisplayNameRadio->setEnabled(mHasDisplayNames);
	mLegacyNamesCheck->setEnabled(mHasDisplayNames);
	mOmitResidentCheck->setEnabled(mHasDisplayNames);

	mLanguageCombo->setValue(mLanguage);
}

void LLPrefsGeneralImpl::apply()
{
	gSavedSettings.setS32("RenderName", mFadeOutNamesCombo->getCurrentIndex());

	std::string language = mLanguageCombo->getValue();
	if (language != mLanguage)
	{
		gSavedSettings.setString("Language", language);
		LLNotifications::instance().add("InEffectAfterRestart");
	}
	if (mCanChooseMaturity)
	{
		S32 preferred_maturity = mMaturityCombo->getValue().asInteger();
		if (preferred_maturity != gSavedSettings.getU32("PreferredMaturity"))
		{
			gSavedSettings.setU32("PreferredMaturity", preferred_maturity);
		}
	}
	refreshValues();
}

void LLPrefsGeneralImpl::cancel()
{
	gSavedSettings.setBOOL("ForceShowGrid",				mForceShowGrid);
	gSavedSettings.setBOOL("LoginLastLocation",			mLoginLastLocation);
	gSavedSettings.setBOOL("ShowStartLocation",			mShowStartLocation);
	gSavedSettings.setBOOL("RenderHideGroupTitleAll",	mRenderHideGroupTitleAll);
	gSavedSettings.setBOOL("RenderHideGroupTitle",		mRenderHideGroupTitle);
	gSavedSettings.setBOOL("LanguageIsPublic",			mLanguageIsPublic);
	gSavedSettings.setBOOL("RenderNameHideSelf",		mRenderNameHideSelf);
	gSavedSettings.setBOOL("SmallAvatarNames",			mSmallAvatarNames);
	gSavedSettings.setBOOL("UIAutoScale",				mUIAutoScale);
	gSavedSettings.setBOOL("LegacyNamesForFriends",		mLegacyNamesForFriends);
	gSavedSettings.setBOOL("OmitResidentAsLastName",	mOmitResidentAsLastName);
	gSavedSettings.setF32("ChatBubbleOpacity",			mChatBubbleOpacity);
	gSavedSettings.setS32("RenderName",					mRenderName);
	gSavedSettings.setS32("AFKTimeout",					mAFKTimeout);
	gSavedSettings.setF32("UIScaleFactor",				mUIScaleFactor);
	if (mPreferredMaturity != gSavedSettings.getU32("PreferredMaturity"))
	{
		gSavedSettings.setU32("PreferredMaturity",		mPreferredMaturity);
	}
	gSavedSettings.setU32("DisplayNamesUsage",			mDisplayNamesUsage);
	gSavedSettings.setColor4("EffectColor",				mEffectColor);
	gSavedSettings.setString("Language",				mLanguage);
}

//---------------------------------------------------------------------------

LLPrefsGeneral::LLPrefsGeneral()
 : impl(* new LLPrefsGeneralImpl())
{
}

LLPrefsGeneral::~LLPrefsGeneral()
{
	delete &impl;
}

void LLPrefsGeneral::apply()
{
	impl.apply();
}

void LLPrefsGeneral::cancel()
{
	impl.cancel();
}

LLPanel* LLPrefsGeneral::getPanel()
{
	return &impl;
}
