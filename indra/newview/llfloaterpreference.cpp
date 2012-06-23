/** 
 * @file llfloaterpreference.cpp
 * @brief Global preferences with and without persistence.
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

/*
 * App-wide preferences.  Note that these are not per-user,
 * because we need to load many preferences before we have
 * a login name.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterpreference.h"

#include "llbutton.h"
#include "lldir.h"
#include "llresizehandle.h"			// for RESIZE_HANDLE_WIDTH
#include "llscrollbar.h"			// for SCROLLBAR_SIZE
#include "lluictrlfactory.h"
#include "message.h"

#include "llagent.h"
#include "llcommandhandler.h"
#include "llfloaterabout.h"
#include "llfloaterhardwaresettings.h"
#include "llpaneldisplay.h"
#include "hbpanelgrids.h"
#include "llpanellogin.h"
#include "llpanelLCD.h"
#include "llpanelskins.h"
#include "llprefschat.h"
#include "hbprefscool.h"
#include "llprefsgeneral.h"
#include "llprefsim.h"
#include "llprefsinput.h"
#include "llprefsmedia.h"
#include "llprefsnetwork.h"
#include "llprefsnotifications.h"
#include "llprefsvoice.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llviewerwindow.h"

const S32 PREF_BORDER = 4;
const S32 PREF_PAD = 5;
const S32 PREF_BUTTON_WIDTH = 70;
const S32 PREF_CATEGORY_WIDTH = 150;
const S32 PREF_FLOATER_MIN_HEIGHT = 2 * SCROLLBAR_SIZE + 2 * LLPANEL_BORDER_WIDTH + 96;

LLFloaterPreference* LLFloaterPreference::sInstance = NULL;

class LLPreferencesHandler : public LLCommandHandler
{
public:
	// requires trusted browser
	LLPreferencesHandler() : LLCommandHandler("preferences", true) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		LLFloaterPreference::show(NULL);
		return true;
	}
};
LLPreferencesHandler gPreferencesHandler;

// Must be done at run time, not compile time. JC
S32 pref_min_width()
{
	return 2 * PREF_BORDER + 2 * PREF_BUTTON_WIDTH + 2 * PREF_PAD +
		   RESIZE_HANDLE_WIDTH + PREF_CATEGORY_WIDTH;
}

S32 pref_min_height()
{
	return 2 * PREF_BORDER + 3 * (BTN_HEIGHT + PREF_PAD) + PREF_FLOATER_MIN_HEIGHT;
}

LLPreferenceCore::LLPreferenceCore(LLTabContainer* tab_container,
								   LLButton * default_btn)
:	mTabContainer(tab_container)
{
	mPrefsGeneral = new LLPrefsGeneral();
	mTabContainer->addTabPanel(mPrefsGeneral->getPanel(),
							   mPrefsGeneral->getPanel()->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsGeneral->getPanel()->setDefaultBtn(default_btn);

	mPrefsInput = new LLPrefsInput();
	mTabContainer->addTabPanel(mPrefsInput->getPanel(),
							   mPrefsInput->getPanel()->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsInput->getPanel()->setDefaultBtn(default_btn);

	mPrefsNetwork = new LLPrefsNetwork();
	mTabContainer->addTabPanel(mPrefsNetwork, mPrefsNetwork->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsNetwork->setDefaultBtn(default_btn);

	mDisplayPanel = new LLPanelDisplay();
	mTabContainer->addTabPanel(mDisplayPanel, mDisplayPanel->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mDisplayPanel->setDefaultBtn(default_btn);

	mAudioPanel = new LLPrefsMedia();
	mTabContainer->addTabPanel(mAudioPanel, mAudioPanel->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mAudioPanel->setDefaultBtn(default_btn);

	mPrefsChat = new LLPrefsChat();
	mTabContainer->addTabPanel(mPrefsChat->getPanel(),
							   mPrefsChat->getPanel()->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsChat->getPanel()->setDefaultBtn(default_btn);

	mPrefsIM = new LLPrefsIM();
	mTabContainer->addTabPanel(mPrefsIM->getPanel(),
							   mPrefsIM->getPanel()->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsIM->getPanel()->setDefaultBtn(default_btn);

	mPrefsVoice = new LLPrefsVoice();
	mTabContainer->addTabPanel(mPrefsVoice, mPrefsVoice->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsVoice->setDefaultBtn(default_btn);

#if LL_LCD_COMPILE
	// only add this option if we actually have a logitech keyboard/speaker set
	if (gLcdScreen->Enabled())
	{
		mLCDPanel = new LLPanelLCD();
		mTabContainer->addTabPanel(mLCDPanel, mLCDPanel->getLabel(),
								   FALSE, onTabChanged, mTabContainer);
		mLCDPanel->setDefaultBtn(default_btn);
	}
#else
	mLCDPanel = NULL;
#endif

	mPrefsNotifications = new LLPrefsNotifications();
	mTabContainer->addTabPanel(mPrefsNotifications->getPanel(),
							   mPrefsNotifications->getPanel()->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsNotifications->getPanel()->setDefaultBtn(default_btn);

	mSkinsPanel = new LLPanelSkins();
	mTabContainer->addTabPanel(mSkinsPanel, mSkinsPanel->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mSkinsPanel->setDefaultBtn(default_btn);

	mPrefsCool = new HBPrefsCool();
	mTabContainer->addTabPanel(mPrefsCool->getPanel(),
							   mPrefsCool->getPanel()->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsCool->getPanel()->setDefaultBtn(default_btn);

	mPrefsGrids = new HBPanelGrids();
	mTabContainer->addTabPanel(mPrefsGrids->getPanel(),
							   mPrefsGrids->getPanel()->getLabel(),
							   FALSE, onTabChanged, mTabContainer);
	mPrefsGrids->getPanel()->setDefaultBtn(default_btn);

	if (!mTabContainer->selectTab(gSavedSettings.getS32("LastPrefTab")))
	{
		mTabContainer->selectFirstTab();
	}
}

LLPreferenceCore::~LLPreferenceCore()
{
	if (mPrefsGeneral)
	{
		delete mPrefsGeneral;
		mPrefsGeneral = NULL;
	}
	if (mPrefsInput)
	{
		delete mPrefsInput;
		mPrefsInput = NULL;
	}
	if (mPrefsNetwork)
	{
		delete mPrefsNetwork;
		mPrefsNetwork = NULL;
	}
	if (mDisplayPanel)
	{
		delete mDisplayPanel;
		mDisplayPanel = NULL;
	}
	if (mAudioPanel)
	{
		delete mAudioPanel;
		mAudioPanel = NULL;
	}
	if (mPrefsChat)
	{
		delete mPrefsChat;
		mPrefsChat = NULL;
	}
	if (mPrefsIM)
	{
		delete mPrefsIM;
		mPrefsIM = NULL;
	}
	if (mPrefsNotifications)
	{
		delete mPrefsNotifications;
		mPrefsNotifications = NULL;
	}
	if (mSkinsPanel)
	{
		delete mSkinsPanel;
		mSkinsPanel = NULL;
	}
	if (mPrefsCool)
	{
		delete mPrefsCool;
		mPrefsCool = NULL;
	}
	if (mPrefsGrids)
	{
		delete mPrefsGrids;
		mPrefsGrids = NULL;
	}
}

void LLPreferenceCore::apply()
{
	mPrefsGeneral->apply();
	mPrefsInput->apply();
	mPrefsNetwork->apply();
	mDisplayPanel->apply();
	mAudioPanel->apply();
	mPrefsChat->apply();
	mPrefsVoice->apply();
	mPrefsIM->apply();
	mPrefsNotifications->apply();
	mSkinsPanel->apply();
	mPrefsCool->apply();
	mPrefsGrids->apply();

	// hardware menu apply
	LLFloaterHardwareSettings::instance()->apply();

#if LL_LCD_COMPILE
	// only add this option if we actually have a logitech keyboard / speaker set
	if (gLcdScreen->Enabled())
	{
		mLCDPanel->apply();
	}
#endif
}

void LLPreferenceCore::cancel()
{
	mPrefsGeneral->cancel();
	mPrefsInput->cancel();
	mPrefsNetwork->cancel();
	mDisplayPanel->cancel();
	mAudioPanel->cancel();
	mPrefsChat->cancel();
	mPrefsVoice->cancel();
	mPrefsIM->cancel();
	mPrefsNotifications->cancel();
	mSkinsPanel->cancel();
	mPrefsCool->cancel();
	mPrefsGrids->cancel();

	// cancel hardware menu
	LLFloaterHardwareSettings::instance()->cancel();

#if LL_LCD_COMPILE
	// only add this option if we actually have a logitech keyboard / speaker set
	if (gLcdScreen->Enabled())
	{
		mLCDPanel->cancel();
	}
#endif
}

// static
void LLPreferenceCore::onTabChanged(void* user_data, bool from_click)
{
	LLTabContainer* self = (LLTabContainer*)user_data;

	gSavedSettings.setS32("LastPrefTab", self->getCurrentPanelIndex());
}

void LLPreferenceCore::setPersonalInfo(const std::string& visibility, bool im_via_email, const std::string& email)
{
	mPrefsIM->setPersonalInfo(visibility, im_via_email, email);
}

void LLPreferenceCore::refreshEnabledGraphics()
{
	LLFloaterHardwareSettings::instance()->refresh();
	mDisplayPanel->refreshEnabledState();
}

//////////////////////////////////////////////
// LLFloaterPreference

LLFloaterPreference::LLFloaterPreference()
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preferences.xml");
}

BOOL LLFloaterPreference::postBuild()
{
	requires<LLButton>("About...");
	requires<LLButton>("OK");
	requires<LLButton>("Cancel");
	requires<LLButton>("Apply");
	requires<LLTabContainer>("pref core");

	if (!checkRequirements())
	{
		return FALSE;
	}

	mAboutBtn = getChild<LLButton>("About...");
	mAboutBtn->setClickedCallback(onClickAbout, this);

	mApplyBtn = getChild<LLButton>("Apply");
	mApplyBtn->setClickedCallback(onBtnApply, this);

	mCancelBtn = getChild<LLButton>("Cancel");
	mCancelBtn->setClickedCallback(onBtnCancel, this);

	mOKBtn = getChild<LLButton>("OK");
	mOKBtn->setClickedCallback(onBtnOK, this);

	mPreferenceCore = new LLPreferenceCore(getChild<LLTabContainer>("pref core"),
										   getChild<LLButton>("OK"));

	sInstance = this;

	return TRUE;
}

LLFloaterPreference::~LLFloaterPreference()
{
	sInstance = NULL;
	delete mPreferenceCore;
}

void LLFloaterPreference::apply()
{
	this->mPreferenceCore->apply();
}

void LLFloaterPreference::cancel()
{
	this->mPreferenceCore->cancel();
}

// static
void LLFloaterPreference::show(void*)
{
	if (!sInstance)
	{
		new LLFloaterPreference();
		sInstance->center();
	}

	sInstance->open();		/* Flawfinder: ignore */

	if (!gAgent.getID().isNull())
	{
		// we're logged in, so we can get this info.
		gMessageSystem->newMessageFast(_PREHASH_UserInfoRequest);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
	}

	LLPanelLogin::setAlwaysRefresh(true);
}

// static
void LLFloaterPreference::onClickAbout(void*)
{
	LLFloaterAbout::show(NULL);
}

// static 
void LLFloaterPreference::onBtnOK(void* userdata)
{
	LLFloaterPreference *fp =(LLFloaterPreference *)userdata;
	// commit any outstanding text entry
	if (fp->hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}

	if (fp->canClose())
	{
		fp->apply();
		fp->close(false);

		gSavedSettings.saveToFile(gSavedSettings.getString("ClientSettingsFile"), TRUE);

		std::string crash_settings_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, CRASH_SETTINGS_FILE);
		// save all settings, even if equals defaults
		gCrashSettings.saveToFile(crash_settings_filename, FALSE);
	}
	else
	{
		// Show beep, pop up dialog, etc.
		llinfos << "Can't close preferences!" << llendl;
	}

	LLPanelLogin::refreshLocation(false);
}

// static 
void LLFloaterPreference::onBtnApply(void* userdata)
{
	LLFloaterPreference *fp =(LLFloaterPreference *)userdata;
	if (fp->hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	fp->apply();

	LLPanelLogin::refreshLocation(false);
}

void LLFloaterPreference::onClose(bool app_quitting)
{
	LLPanelLogin::setAlwaysRefresh(false);
	cancel(); // will be a no-op if OK or apply was performed just prior.
	LLFloater::onClose(app_quitting);
}

// static 
void LLFloaterPreference::onBtnCancel(void* userdata)
{
	LLFloaterPreference *fp =(LLFloaterPreference *)userdata;
	if (fp->hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		}
	}
	fp->close(); // side effect will also cancel any unsaved changes.
}

// static
void LLFloaterPreference::updateUserInfo(const std::string& visibility,
										 bool im_via_email,
										 const std::string& email)
{
	if (sInstance && sInstance->mPreferenceCore)
	{
		sInstance->mPreferenceCore->setPersonalInfo(visibility, im_via_email,
													email);
	}
}

void LLFloaterPreference::refreshEnabledGraphics()
{
	sInstance->mPreferenceCore->refreshEnabledGraphics();
}
