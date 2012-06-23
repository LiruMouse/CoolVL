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

//file include
#include "llprefsgeneral.h"

// project includes
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lluictrlfactory.h"
#include "llurlsimstring.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llviewerregion.h"

class LLPrefsGeneralImpl : public LLPanel
{
public:
	LLPrefsGeneralImpl();
	/*virtual*/ ~LLPrefsGeneralImpl() { };

	/*virtual*/ void refresh();

	void apply();
	void cancel();

private:
	void refreshValues();

	BOOL mForceShowGrid;
	BOOL mLoginLastLocation;
	BOOL mShowStartLocation;
	BOOL mRenderHideGroupTitleAll;
	BOOL mRenderHideGroupTitle;
	BOOL mLanguageIsPublic;
	BOOL mRenderNameHideSelf;
	BOOL mSmallAvatarNames;
	BOOL mUIAutoScale;
	BOOL mLegacyNamesForFriends;
	BOOL mOmitResidentAsLastName;
	F32 mChatBubbleOpacity;
	F32 mUIScaleFactor;
	S32 mRenderName;
	S32 mAFKTimeout;
	U32 mPreferredMaturity;
	U32 mDisplayNamesUsage;
	LLColor4 mEffectColor;
	std::string mLanguage;
};

LLPrefsGeneralImpl::LLPrefsGeneralImpl()
 : LLPanel(std::string("General Preferences"))
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_general.xml");
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

	LLComboBox* combo = getChild<LLComboBox>("fade_out_combobox");
	if (combo)
	{
		combo->setCurrentByIndex(mRenderName);
	}
	childSetValue("language_combobox", mLanguage);

	// if we have no agent, we can't let them choose anything
	// if we have an agent, then we only let them choose if they have a choice
	bool can_choose = gAgent.getID().notNull() && (gAgent.isMature() || gAgent.isGodlike());
	if (can_choose)
	{
		// if they're not adult or a god, they shouldn't see the adult selection, so delete it
		if (!gAgent.isAdult() && !gAgent.isGodlike())
		{
			LLComboBox* maturity_combo = getChild<LLComboBox>("maturity_desired_combobox");
			// we're going to remove the adult entry from the combo. This obviously depends
			// on the order of items in the XML file, but there doesn't seem to be a reasonable
			// way to depend on the field in XML called 'name'.
			maturity_combo->remove(0);
		}
	}
	childSetValue("maturity_desired_combobox", (S32)gSavedSettings.getU32("PreferredMaturity"));
	std::string selected_item_label = getChild<LLComboBox>("maturity_desired_combobox")->getSelectedItemLabel();
	childSetValue("maturity_desired_textbox", selected_item_label);
	childSetVisible("maturity_desired_combobox", can_choose);
	childSetVisible("maturity_desired_textbox",	!can_choose);
}

void LLPrefsGeneralImpl::apply()
{
	LLComboBox* combo = getChild<LLComboBox>("fade_out_combobox");
	if (combo)
	{
		gSavedSettings.setS32("RenderName", combo->getCurrentIndex());
	}
	std::string language = childGetValue("language_combobox");
	if (language != mLanguage)
	{
		gSavedSettings.setString("Language", language);
		LLNotifications::instance().add("InEffectAfterRestart");
	}
	if (gAgent.getID().notNull() && (gAgent.isMature() || gAgent.isGodlike()))
	{
		S32 preferred_maturity = childGetValue("maturity_desired_combobox").asInteger();
		if (preferred_maturity != gSavedSettings.getU32("PreferredMaturity"))
		{
			gSavedSettings.setU32("PreferredMaturity", preferred_maturity);
			gAgent.sendMaturityPreferenceToServer(preferred_maturity);
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
	gSavedSettings.setU32("PreferredMaturity",			mPreferredMaturity);
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
