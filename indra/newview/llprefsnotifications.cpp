/** 
 * @file llprefsnotifications.cpp
 * @brief Notifications preferences panel
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Adapted by Henri Beauchamp from llpanelmsgs.cpp
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

#include "llprefsnotifications.h"

#include "llnotifications.h"
#include "llscrolllistctrl.h"
#include "lluictrlfactory.h"

#include "llfirstuse.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

class LLPrefsNotificationsImpl : public LLPanel
{
public:
	LLPrefsNotificationsImpl();
	/*virtual*/ ~LLPrefsNotificationsImpl() { };

	/*virtual*/ void draw();

	void apply();
	void cancel();

	void buildLists();
	void resetAllIgnored();
	void setAllIgnored();
	void refreshValues();

private:
	static void onClickEnablePopup(void* user_data);
	static void onClickDisablePopup(void* user_data);
	static void onClickResetDialogs(void* user_data);
	static void onClickSkipDialogs(void* user_data);

	BOOL mAutoAcceptNewInventory;
	BOOL mRejectNewInventoryWhenBusy;
	BOOL mShowNewInventory;
	BOOL mShowInInventory;
	BOOL mNotifyMoneyChange;
	BOOL mNotifyServerVersion;
	BOOL mChatOnlineNotification;
	BOOL mHideNotificationsInChat;
	BOOL mScriptErrorsAsChat;
	BOOL mTeleportHistoryInChat;
	F32	mMoneyChangeThreshold;
	F32	mHealthReductionThreshold;
};

//-----------------------------------------------------------------------------
LLPrefsNotificationsImpl::LLPrefsNotificationsImpl() : 
	LLPanel("Notifications Preferences Panel")
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_notifications.xml");
	childSetAction("enable_popup", onClickEnablePopup, this);
	childSetAction("disable_popup", onClickDisablePopup, this);
	childSetAction("reset_dialogs_btn", onClickResetDialogs, this);
	childSetAction("skip_dialogs_btn", onClickSkipDialogs, this);
	refreshValues();
	buildLists();
}

void LLPrefsNotificationsImpl::refreshValues()
{
	mAutoAcceptNewInventory		= gSavedSettings.getBOOL("AutoAcceptNewInventory");
	mRejectNewInventoryWhenBusy	= gSavedSettings.getBOOL("RejectNewInventoryWhenBusy");
	mShowNewInventory			= gSavedSettings.getBOOL("ShowNewInventory");
	mShowInInventory			= gSavedSettings.getBOOL("ShowInInventory");
	mNotifyMoneyChange			= gSavedSettings.getBOOL("NotifyMoneyChange");
	mNotifyServerVersion		= gSavedSettings.getBOOL("NotifyServerVersion");
	mChatOnlineNotification		= gSavedSettings.getBOOL("ChatOnlineNotification");
	mHideNotificationsInChat	= gSavedSettings.getBOOL("HideNotificationsInChat");
	mScriptErrorsAsChat			= gSavedSettings.getBOOL("ScriptErrorsAsChat");
	mTeleportHistoryInChat		= gSavedSettings.getBOOL("TeleportHistoryInChat");
	mMoneyChangeThreshold		= gSavedSettings.getF32("UISndMoneyChangeThreshold");
	mHealthReductionThreshold	= gSavedSettings.getF32("UISndHealthReductionThreshold");
}

void LLPrefsNotificationsImpl::buildLists()
{
	LLScrollListCtrl& disabled_popups = getChildRef<LLScrollListCtrl>("disabled_popups");
	LLScrollListCtrl& enabled_popups = getChildRef<LLScrollListCtrl>("enabled_popups");

	disabled_popups.deleteAllItems();
	enabled_popups.deleteAllItems();

	LLNotifications::TemplateMap::const_iterator iter;
	for (iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd(); ++iter)
	{
		LLNotificationTemplatePtr templatep = iter->second;
		LLNotificationFormPtr formp = templatep->mForm;

		LLNotificationForm::EIgnoreType ignore = formp->getIgnoreType();
		if (ignore == LLNotificationForm::IGNORE_NO) continue;

		LLSD row;
		row["columns"][0]["value"] = formp->getIgnoreMessage();
		row["columns"][0]["font"] = "SANSSERIF_SMALL";
		row["columns"][0]["width"] = 300;

		LLScrollListItem* item = NULL;

		bool show_popup = gSavedSettings.getWarning(templatep->mName);
		if (!show_popup)
		{
			if (ignore == LLNotificationForm::IGNORE_WITH_LAST_RESPONSE)
			{
				LLSD last_response = LLUI::sConfigGroup->getLLSD("Default" + templatep->mName);
				if (!last_response.isUndefined())
				{
					for (LLSD::map_const_iterator it = last_response.beginMap();
						 it != last_response.endMap(); ++it)
					{
						if (it->second.asBoolean())
						{
							row["columns"][1]["value"] =
								formp->getElement(it->first)["ignore"].asString();
							break;
						}
					}
				}
				row["columns"][1]["font"] = "SANSSERIF_SMALL";
				row["columns"][1]["width"] = 160;
			}
			item = disabled_popups.addElement(row, ADD_SORTED);
		}
		else
		{
			item = enabled_popups.addElement(row, ADD_SORTED);
		}

		if (item)
		{
			item->setUserdata((void*)&iter->first);
		}
	}
}

void LLPrefsNotificationsImpl::draw()
{
	LLScrollListCtrl& disabled_popups = getChildRef<LLScrollListCtrl>("disabled_popups");
	LLScrollListCtrl& enabled_popups = getChildRef<LLScrollListCtrl>("enabled_popups");

	if (disabled_popups.getFirstSelected())
	{
		childEnable("enable_popup");
	}
	else
	{
		childDisable("enable_popup");
	}

	if (enabled_popups.getFirstSelected())
	{
		childEnable("disable_popup");
	}
	else
	{
		childDisable("disable_popup");
	}

	LLPanel::draw();
}

void LLPrefsNotificationsImpl::apply()
{
	refreshValues();
}

void LLPrefsNotificationsImpl::cancel()
{
	gSavedSettings.setBOOL("AutoAcceptNewInventory",		mAutoAcceptNewInventory);
	gSavedSettings.setBOOL("RejectNewInventoryWhenBusy",	mRejectNewInventoryWhenBusy);
	gSavedSettings.setBOOL("ShowNewInventory",				mShowNewInventory);
	gSavedSettings.setBOOL("ShowInInventory",				mShowInInventory);
	gSavedSettings.setBOOL("NotifyMoneyChange",				mNotifyMoneyChange);
	gSavedSettings.setBOOL("NotifyServerVersion",			mNotifyServerVersion);
	gSavedSettings.setBOOL("ChatOnlineNotification",		mChatOnlineNotification);
	gSavedSettings.setBOOL("HideNotificationsInChat",		mHideNotificationsInChat);
	gSavedSettings.setBOOL("ScriptErrorsAsChat",			mScriptErrorsAsChat);
	gSavedSettings.setBOOL("TeleportHistoryInChat",			mTeleportHistoryInChat);
	gSavedSettings.setF32("UISndMoneyChangeThreshold",		mMoneyChangeThreshold);
	gSavedSettings.setF32("UISndHealthReductionThreshold",	mHealthReductionThreshold);

}

void LLPrefsNotificationsImpl::resetAllIgnored()
{
	LLNotifications::TemplateMap::const_iterator iter;
	for (iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd(); ++iter)
	{
		if (iter->second->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			gSavedSettings.setWarning(iter->first, TRUE);
		}
	}
}

void LLPrefsNotificationsImpl::setAllIgnored()
{
	LLNotifications::TemplateMap::const_iterator iter;
	for (iter = LLNotifications::instance().templatesBegin();
		 iter != LLNotifications::instance().templatesEnd(); ++iter)
	{
		if (iter->second->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			gSavedSettings.setWarning(iter->first, FALSE);
		}
	}
}

//static 
void LLPrefsNotificationsImpl::onClickEnablePopup(void* user_data)
{
	LLPrefsNotificationsImpl* panelp = (LLPrefsNotificationsImpl*)user_data;
	if (!panelp) return;

	LLScrollListCtrl& disabled_popups = panelp->getChildRef<LLScrollListCtrl>("disabled_popups");

	std::vector<LLScrollListItem*> items = disabled_popups.getAllSelected();
	std::vector<LLScrollListItem*>::iterator iter;
	for (iter = items.begin(); iter != items.end(); ++iter)
	{
		LLNotificationTemplatePtr templatep =
			LLNotifications::instance().getTemplate(*(std::string*)((*iter)->getUserdata()));
		if (templatep->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			gSavedSettings.setWarning(templatep->mName, TRUE);
		}
	}

	panelp->buildLists();
}

//static 
void LLPrefsNotificationsImpl::onClickDisablePopup(void* user_data)
{
	LLPrefsNotificationsImpl* panelp = (LLPrefsNotificationsImpl*)user_data;
	if (!panelp) return;

	LLScrollListCtrl& disabled_popups = panelp->getChildRef<LLScrollListCtrl>("enabled_popups");

	std::vector<LLScrollListItem*> items = disabled_popups.getAllSelected();
	std::vector<LLScrollListItem*>::iterator iter;
	for (iter = items.begin(); iter != items.end(); ++iter)
	{
		LLNotificationTemplatePtr templatep =
			LLNotifications::instance().getTemplate(*(std::string*)((*iter)->getUserdata()));
		if (templatep->mForm->getIgnoreType() != LLNotificationForm::IGNORE_NO)
		{
			gSavedSettings.setWarning(templatep->mName, FALSE);
		}
	}

	panelp->buildLists();
}

bool callback_reset_dialogs(const LLSD& notification, const LLSD& response,
							LLPrefsNotificationsImpl* panelp)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		if (panelp)
		{
			panelp->resetAllIgnored();
			LLFirstUse::resetFirstUse();
			panelp->buildLists();
		}
	}
	return false;
}

// static
void LLPrefsNotificationsImpl::onClickResetDialogs(void* user_data)
{
	LLNotifications::instance().add("ResetShowNextTimeDialogs", LLSD(), LLSD(),
									boost::bind(&callback_reset_dialogs, _1, _2,
												(LLPrefsNotificationsImpl*)user_data));
}

bool callback_skip_dialogs(const LLSD& notification, const LLSD& response,
						   LLPrefsNotificationsImpl* panelp)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		if (panelp)
		{
			panelp->setAllIgnored();
			LLFirstUse::disableFirstUse();
			panelp->buildLists();
		}
	}
	return false;
}

// static
void LLPrefsNotificationsImpl::onClickSkipDialogs(void* user_data)
{
	LLNotifications::instance().add("SkipShowNextTimeDialogs", LLSD(), LLSD(),
									boost::bind(&callback_skip_dialogs, _1, _2,
												(LLPrefsNotificationsImpl*)user_data));
}

//---------------------------------------------------------------------------

LLPrefsNotifications::LLPrefsNotifications()
:	impl(* new LLPrefsNotificationsImpl())
{
}

LLPrefsNotifications::~LLPrefsNotifications()
{
	delete &impl;
}

void LLPrefsNotifications::draw()
{
	impl.draw();
}

void LLPrefsNotifications::apply()
{
	impl.apply();
}

void LLPrefsNotifications::cancel()
{
	impl.cancel();
}

LLPanel* LLPrefsNotifications::getPanel()
{
	return &impl;
}
