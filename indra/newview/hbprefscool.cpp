/** 
 * @file hbprefscool.cpp
 * @author Henri Beauchamp
 * @brief Cool VL Viewer preferences panel
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2010, Henri Beauchamp.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "hbprefscool.h"

#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llcolorswatch.h"
#include "llradiogroup.h"
#include "llstring.h"
#include "lltexteditor.h"
#include "lluictrlfactory.h"

#include "hbfloaterrlv.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "RRInterface.h"

class HBPrefsCoolImpl : public LLPanel
{
public:
	HBPrefsCoolImpl();
	/*virtual*/ ~HBPrefsCoolImpl() { };

	/*virtual*/ void refresh();
	/*virtual*/ void draw();

	void apply();
	void cancel();

private:
	void refreshRestrainedLove(bool enable);
	void updateRestrainedLoveUserProfile();

	static void onCommitCheckBoxShowButton(LLUICtrl* ctrl, void* user_data);
	static void onCommitCheckBoxRestrainedLove(LLUICtrl* ctrl, void* user_data);
	static void onCommitCheckBoxSpeedRez(LLUICtrl* ctrl, void* user_data);
	static void onCommitCheckBoxPrivateLookAt(LLUICtrl* ctrl, void* user_data);
	static void onCommitCheckBoxAfterRestart(LLUICtrl* ctrl, void* user_data);
	static void onCommitUserProfile(LLUICtrl* ctrl, void* user_data);
	static void onClickCustomBlackList(void* user_data);
	void refreshValues();

private:
	bool mWatchBlackListFloater;
	S32 mRestrainedLoveUserProfile;

	BOOL mHideMasterRemote;
	BOOL mShowChatButton;
	BOOL mShowIMButton;
	BOOL mShowFriendsButton;
	BOOL mShowGroupsButton;
	BOOL mShowFlyButton;
	BOOL mShowSnapshotButton;
	BOOL mShowSearchButton;
	BOOL mShowBuildButton;
	BOOL mShowRadarButton;
	BOOL mShowMiniMapButton;
	BOOL mShowMapButton;
	BOOL mShowInventoryButton;
	BOOL mUseOldChatHistory;
	BOOL mIMTabsVerticalStacking;
	BOOL mUseOldStatusBarIcons;
	BOOL mUseOldTrackingDots;
	BOOL mAllowMUpose;
	BOOL mAutoCloseOOC;
	BOOL mPrivateLookAt;
	BOOL mFetchInventoryOnLogin;
	BOOL mPreviewAnimInWorld;
	BOOL mSpeedRez;
	BOOL mRevokePermsOnStandUp;
	BOOL mRezWithLandGroup;
	BOOL mStackMinimizedTopToBottom;
	BOOL mStackMinimizedRightToLeft;
	BOOL mHideTeleportProgress;
	BOOL mHighlightOwnNameInChat;
	BOOL mHighlightOwnNameInIM;
	BOOL mHighlightDisplayName;
	BOOL mTeleportHistoryDeparture;
	BOOL mNoMultiplePhysics;
	BOOL mNoMultipleShoes;
	BOOL mNoMultipleSkirts;
	BOOL mAvatarPhysics;
	BOOL mUseNewSLLoginPage;
	BOOL mAllowMultipleViewers;
	BOOL mRestrainedLove;
	BOOL mRestrainedLoveNoSetEnv;
	BOOL mRestrainedLoveAllowWear;
	BOOL mRestrainedLoveForbidGiveToRLV;
	BOOL mRestrainedLoveShowEllipsis;
	BOOL mRestrainedLoveUntruncatedEmotes;
	BOOL mRestrainedLoveCanOoc;
	std::string mRestrainedLoveRecvimMessage;
	std::string mRestrainedLoveSendimMessage;
	std::string mRestrainedLoveBlacklist;
	U32 mRestrainedLoveReattachDelay;
	U32 mStackScreenWidthFraction;
	U32 mSpeedRezInterval;
	U32 mPrivateLookAtLimit;
	U32 mDecimalsForTools;
	U32 mFadeMouselookExitTip;
	U32 mTimeFormat;
	U32 mDateFormat;
	std::string mHighlightNicknames;
	LLColor4 mOwnNameChatColor;
};

HBPrefsCoolImpl::HBPrefsCoolImpl()
:	LLPanel(std::string("Cool Preferences Panel")),
	mWatchBlackListFloater(false)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_cool.xml");
	childSetCommitCallback("show_chat_button_check",			onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_im_button_check",				onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_friends_button_check",			onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_group_button_check",			onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_fly_button_check",				onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_snapshot_button_check",		onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_search_button_check",			onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_build_button_check",			onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_radar_button_check",			onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_minimap_button_check",			onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_map_button_check",				onCommitCheckBoxShowButton, this);
	childSetCommitCallback("show_inventory_button_check",		onCommitCheckBoxShowButton, this);
	childSetCommitCallback("restrained_love_check",				onCommitCheckBoxRestrainedLove, this);
	childSetCommitCallback("speed_rez_check",					onCommitCheckBoxSpeedRez, this);
	childSetCommitCallback("private_look_at_check",				onCommitCheckBoxPrivateLookAt, this);
	childSetCommitCallback("use_old_chat_history_check",		onCommitCheckBoxAfterRestart, this);
	childSetCommitCallback("im_tabs_vertical_stacking_check",	onCommitCheckBoxAfterRestart, this);
	childSetCommitCallback("use_old_statusbar_icons_check",		onCommitCheckBoxAfterRestart, this);
	childSetCommitCallback("use_new_sl_login_page_check",		onCommitCheckBoxAfterRestart, this);
	childSetCommitCallback("restrained_love_no_setenv_check",	onCommitCheckBoxAfterRestart, this);
	childSetCommitCallback("restrained_love_untruncated_emotes_check", onCommitCheckBoxAfterRestart, this);
	childSetCommitCallback("restrained_love_can_ooc_check",		onCommitCheckBoxAfterRestart, this);
	childSetCommitCallback("user_profile",						onCommitUserProfile, this);
	childSetAction("custom_profile_button",						onClickCustomBlackList, this);

	if (LLStartUp::getStartupState() != STATE_STARTED)
	{
		LLCheckBoxCtrl* rl = getChild<LLCheckBoxCtrl>("restrained_love_check");
		std::string tooltip = rl->getToolTip();
		tooltip += " " + getString("when_logged_in");
		rl->setToolTip(tooltip);
	}
	refresh();
}

void HBPrefsCoolImpl::draw()
{
	if (mWatchBlackListFloater && !HBFloaterBlacklistRLV::instanceExists())
	{
		mWatchBlackListFloater = false;
		updateRestrainedLoveUserProfile();
	}
	LLPanel::draw();
}

void HBPrefsCoolImpl::refreshRestrainedLove(bool enable)
{
	// Enable/disable all children in the RestrainedLove panel
	LLPanel* panel = getChild<LLPanel>("RestrainedLove");
	LLView* child = panel->getFirstChild();
	while (child)
	{
		child->setEnabled(enable);
		child = panel->findNextSibling(child);
	}

	// Enable/disable the FetchInventoryOnLogin check box
	if (enable)
	{
		gSavedSettings.setBOOL("FetchInventoryOnLogin",	TRUE);
		childSetValue("fetch_inventory_on_login_check", TRUE);
		childDisable("fetch_inventory_on_login_check");
	}
	else
	{
		childEnable("fetch_inventory_on_login_check");
	}

	// RestrainedLove check box enabled only when logged in.
	childSetEnabled("restrained_love_check",
					LLStartUp::getStartupState() == STATE_STARTED);
}

void HBPrefsCoolImpl::refreshValues()
{
	mHideMasterRemote			= gSavedSettings.getBOOL("HideMasterRemote");
	mShowChatButton				= gSavedSettings.getBOOL("ShowChatButton");
	mShowIMButton				= gSavedSettings.getBOOL("ShowIMButton");
	mShowFriendsButton			= gSavedSettings.getBOOL("ShowFriendsButton");
	mShowGroupsButton			= gSavedSettings.getBOOL("ShowGroupsButton");
	mShowFlyButton				= gSavedSettings.getBOOL("ShowFlyButton");
	mShowSnapshotButton			= gSavedSettings.getBOOL("ShowSnapshotButton");
	mShowSearchButton			= gSavedSettings.getBOOL("ShowSearchButton");
	mShowBuildButton			= gSavedSettings.getBOOL("ShowBuildButton");
	mShowRadarButton			= gSavedSettings.getBOOL("ShowRadarButton");
	mShowMiniMapButton			= gSavedSettings.getBOOL("ShowMiniMapButton");
	mShowMapButton				= gSavedSettings.getBOOL("ShowMapButton");
	mShowInventoryButton		= gSavedSettings.getBOOL("ShowInventoryButton");
	mUseOldChatHistory			= gSavedSettings.getBOOL("UseOldChatHistory");
	mIMTabsVerticalStacking		= gSavedSettings.getBOOL("IMTabsVerticalStacking");
	mUseOldStatusBarIcons		= gSavedSettings.getBOOL("UseOldStatusBarIcons");
	mUseOldTrackingDots			= gSavedSettings.getBOOL("UseOldTrackingDots");
	mAllowMUpose				= gSavedSettings.getBOOL("AllowMUpose");
	mAutoCloseOOC				= gSavedSettings.getBOOL("AutoCloseOOC");
	mPrivateLookAt				= gSavedSettings.getBOOL("PrivateLookAt");
	mPrivateLookAtLimit			= gSavedSettings.getU32("PrivateLookAtLimit");
	mRestrainedLove				= gSavedSettings.getBOOL("RestrainedLove");
	if (mRestrainedLove)
	{
		mFetchInventoryOnLogin	= TRUE;
		gSavedSettings.setBOOL("FetchInventoryOnLogin",	TRUE);
	}
	else
	{
		mFetchInventoryOnLogin	= gSavedSettings.getBOOL("FetchInventoryOnLogin");
	}
	mRestrainedLoveNoSetEnv		= gSavedSettings.getBOOL("RestrainedLoveNoSetEnv");
	mRestrainedLoveAllowWear	= gSavedSettings.getBOOL("RestrainedLoveAllowWear");
	mRestrainedLoveForbidGiveToRLV = gSavedSettings.getBOOL("RestrainedLoveForbidGiveToRLV");
	mRestrainedLoveShowEllipsis	= gSavedSettings.getBOOL("RestrainedLoveShowEllipsis");
	mRestrainedLoveUntruncatedEmotes = gSavedSettings.getBOOL("RestrainedLoveUntruncatedEmotes");
	mRestrainedLoveCanOoc		= gSavedSettings.getBOOL("RestrainedLoveCanOoc");
	mRestrainedLoveReattachDelay = gSavedSettings.getU32("RestrainedLoveReattachDelay");
	mRestrainedLoveRecvimMessage = gSavedSettings.getString("RestrainedLoveRecvimMessage");
	mRestrainedLoveSendimMessage = gSavedSettings.getString("RestrainedLoveSendimMessage");
	mRestrainedLoveBlacklist	= gSavedSettings.getString("RestrainedLoveBlacklist");
	mPreviewAnimInWorld			= gSavedSettings.getBOOL("PreviewAnimInWorld");
	mSpeedRez					= gSavedSettings.getBOOL("SpeedRez");
	mSpeedRezInterval			= gSavedSettings.getU32("SpeedRezInterval");
	mDecimalsForTools			= gSavedSettings.getU32("DecimalsForTools");
	mRevokePermsOnStandUp		= gSavedSettings.getBOOL("RevokePermsOnStandUp");
	mRezWithLandGroup			= gSavedSettings.getBOOL("RezWithLandGroup");
	mStackMinimizedTopToBottom	= gSavedSettings.getBOOL("StackMinimizedTopToBottom");
	mStackMinimizedRightToLeft	= gSavedSettings.getBOOL("StackMinimizedRightToLeft");
	mStackScreenWidthFraction	= gSavedSettings.getU32("StackScreenWidthFraction");
	mHideTeleportProgress		= gSavedSettings.getBOOL("HideTeleportProgress");
	mHighlightOwnNameInChat		= gSavedSettings.getBOOL("HighlightOwnNameInChat");
	mHighlightOwnNameInIM		= gSavedSettings.getBOOL("HighlightOwnNameInIM");
	mOwnNameChatColor			= gSavedSettings.getColor4("OwnNameChatColor");
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		mHighlightNicknames		= gSavedPerAccountSettings.getString("HighlightNicknames");
		mHighlightDisplayName	= gSavedPerAccountSettings.getBOOL("HighlightDisplayName");
	}
	mTeleportHistoryDeparture	= gSavedSettings.getBOOL("TeleportHistoryDeparture");
	mNoMultiplePhysics			= gSavedSettings.getBOOL("NoMultiplePhysics");
	mNoMultipleShoes			= gSavedSettings.getBOOL("NoMultipleShoes");
	mNoMultipleSkirts			= gSavedSettings.getBOOL("NoMultipleSkirts");
	mAvatarPhysics				= gSavedSettings.getBOOL("AvatarPhysics");
	mUseNewSLLoginPage			= gSavedSettings.getBOOL("UseNewSLLoginPage");
	mAllowMultipleViewers		= gSavedSettings.getBOOL("AllowMultipleViewers");
	mFadeMouselookExitTip		= gSavedSettings.getU32("FadeMouselookExitTip");
}

void HBPrefsCoolImpl::updateRestrainedLoveUserProfile()
{
	std::string blacklist = gSavedSettings.getString("RestrainedLoveBlacklist");
	if (blacklist.empty())
	{
		mRestrainedLoveUserProfile = 0;
	}
	else if (blacklist == RRInterface::sRolePlayBlackList)
	{
		mRestrainedLoveUserProfile = 1;
	}
	else if (blacklist == RRInterface::sVanillaBlackList)
	{
		mRestrainedLoveUserProfile = 2;
	}
	else
	{
		mRestrainedLoveUserProfile = 3;
	}
	LLRadioGroup* radio = getChild<LLRadioGroup>("user_profile");
	radio->selectNthItem(mRestrainedLoveUserProfile);
}

void HBPrefsCoolImpl::refresh()
{
	refreshValues();

	if (LLStartUp::getStartupState() != STATE_STARTED)
	{
		childDisable("highlight_nicknames_text");
		childDisable("highlight_display_name_check");
	}
	else
	{
		childSetValue("highlight_nicknames_text", mHighlightNicknames);
		childSetValue("highlight_display_name_check", mHighlightDisplayName);
	}

	// RestrainedLove
	refreshRestrainedLove(mRestrainedLove);
	updateRestrainedLoveUserProfile();
	LLWString message = utf8str_to_wstring(gSavedSettings.getString("RestrainedLoveRecvimMessage"));
	LLWStringUtil::replaceChar(message, '^', '\n');
	childSetText("receive_im_message_editor", wstring_to_utf8str(message));
	message = utf8str_to_wstring(gSavedSettings.getString("RestrainedLoveSendimMessage"));
	LLWStringUtil::replaceChar(message, '^', '\n');
	childSetText("send_im_message_editor", wstring_to_utf8str(message));

	if (mSpeedRez)
	{
		childEnable("speed_rez_interval");
		childEnable("speed_rez_seconds");
	}
	else
	{
		childDisable("speed_rez_interval");
		childDisable("speed_rez_seconds");
	}

	std::string format = gSavedSettings.getString("ShortTimeFormat");
	if (format.find("%p") == -1)
	{
		mTimeFormat = 0;
	}
	else
	{
		mTimeFormat = 1;
	}

	format = gSavedSettings.getString("ShortDateFormat");
	if (format.find("%m/%d/%") != -1)
	{
		mDateFormat = 2;
	}
	else if (format.find("%d/%m/%") != -1)
	{
		mDateFormat = 1;
	}
	else
	{
		mDateFormat = 0;
	}

	// time format combobox
	LLComboBox* combo = getChild<LLComboBox>("time_format_combobox");
	if (combo)
	{
		combo->setCurrentByIndex(mTimeFormat);
	}

	// date format combobox
	combo = getChild<LLComboBox>("date_format_combobox");
	if (combo)
	{
		combo->setCurrentByIndex(mDateFormat);
	}
}

void HBPrefsCoolImpl::cancel()
{
	gSavedSettings.setBOOL("HideMasterRemote",			mHideMasterRemote);
	gSavedSettings.setBOOL("ShowChatButton",			mShowChatButton);
	gSavedSettings.setBOOL("ShowIMButton",				mShowIMButton);
	gSavedSettings.setBOOL("ShowFriendsButton",			mShowFriendsButton);
	gSavedSettings.setBOOL("ShowGroupsButton",			mShowGroupsButton);
	gSavedSettings.setBOOL("ShowFlyButton",				mShowFlyButton);
	gSavedSettings.setBOOL("ShowSnapshotButton",		mShowSnapshotButton);
	gSavedSettings.setBOOL("ShowSearchButton",			mShowSearchButton);
	gSavedSettings.setBOOL("ShowBuildButton",			mShowBuildButton);
	gSavedSettings.setBOOL("ShowRadarButton",			mShowRadarButton);
	gSavedSettings.setBOOL("ShowMiniMapButton",			mShowMiniMapButton);
	gSavedSettings.setBOOL("ShowMapButton",				mShowMapButton);
	gSavedSettings.setBOOL("ShowInventoryButton",		mShowInventoryButton);
	gSavedSettings.setBOOL("UseOldChatHistory",			mUseOldChatHistory);
	gSavedSettings.setBOOL("IMTabsVerticalStacking",	mIMTabsVerticalStacking);
	gSavedSettings.setBOOL("UseOldStatusBarIcons",		mUseOldStatusBarIcons);
	gSavedSettings.setBOOL("UseOldTrackingDots",		mUseOldTrackingDots);
	gSavedSettings.setBOOL("AllowMUpose",				mAllowMUpose);
	gSavedSettings.setBOOL("AutoCloseOOC",				mAutoCloseOOC);
	gSavedSettings.setBOOL("PrivateLookAt",				mPrivateLookAt);
	gSavedSettings.setU32("PrivateLookAtLimit",			mPrivateLookAtLimit);
	gSavedSettings.setBOOL("FetchInventoryOnLogin",		mFetchInventoryOnLogin);
	gSavedSettings.setBOOL("RestrainedLove",			mRestrainedLove);
	gSavedSettings.setBOOL("RestrainedLoveNoSetEnv",	mRestrainedLoveNoSetEnv);
	gSavedSettings.setBOOL("RestrainedLoveAllowWear",	mRestrainedLoveAllowWear);
	gSavedSettings.setBOOL("RestrainedLoveForbidGiveToRLV", mRestrainedLoveForbidGiveToRLV);
	gSavedSettings.setBOOL("RestrainedLoveShowEllipsis", mRestrainedLoveShowEllipsis);
	gSavedSettings.setBOOL("RestrainedLoveUntruncatedEmotes", mRestrainedLoveUntruncatedEmotes);
	gSavedSettings.setBOOL("RestrainedLoveCanOoc",		mRestrainedLoveCanOoc);
	gSavedSettings.setU32("RestrainedLoveReattachDelay", mRestrainedLoveReattachDelay);
	gSavedSettings.setString("RestrainedLoveRecvimMessage", mRestrainedLoveRecvimMessage);
	gSavedSettings.setString("RestrainedLoveSendimMessage", mRestrainedLoveSendimMessage);
	gSavedSettings.setString("RestrainedLoveBlacklist",	mRestrainedLoveBlacklist);
	gSavedSettings.setBOOL("PreviewAnimInWorld",		mPreviewAnimInWorld);
	gSavedSettings.setBOOL("SpeedRez",					mSpeedRez);
	gSavedSettings.setU32("SpeedRezInterval",			mSpeedRezInterval);
	gSavedSettings.setU32("DecimalsForTools",			mDecimalsForTools);
	gSavedSettings.setBOOL("RevokePermsOnStandUp",		mRevokePermsOnStandUp);
	gSavedSettings.setBOOL("RezWithLandGroup",			mRezWithLandGroup);
	gSavedSettings.setBOOL("StackMinimizedTopToBottom",	mStackMinimizedTopToBottom);
	gSavedSettings.setBOOL("StackMinimizedRightToLeft",	mStackMinimizedRightToLeft);
	gSavedSettings.setU32("StackScreenWidthFraction",	mStackScreenWidthFraction);
	gSavedSettings.setBOOL("HideTeleportProgress",		mHideTeleportProgress);
	gSavedSettings.setBOOL("HighlightOwnNameInChat",	mHighlightOwnNameInChat);
	gSavedSettings.setBOOL("HighlightOwnNameInIM",		mHighlightOwnNameInIM);
	gSavedSettings.setColor4("OwnNameChatColor",		mOwnNameChatColor);
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		gSavedPerAccountSettings.setString("HighlightNicknames", mHighlightNicknames);
		gSavedPerAccountSettings.setBOOL("HighlightDisplayName", mHighlightDisplayName);
	}
	gSavedSettings.setBOOL("TeleportHistoryDeparture",	mTeleportHistoryDeparture);
	gSavedSettings.setBOOL("NoMultiplePhysics",			mNoMultiplePhysics);
	gSavedSettings.setBOOL("NoMultipleShoes",			mNoMultipleShoes);
	gSavedSettings.setBOOL("NoMultipleSkirts",			mNoMultipleSkirts);
	gSavedSettings.setBOOL("AvatarPhysics",				mAvatarPhysics);
	gSavedSettings.setBOOL("UseNewSLLoginPage",			mUseNewSLLoginPage);
	gSavedSettings.setBOOL("AllowMultipleViewers",		mAllowMultipleViewers);
	gSavedSettings.setU32("FadeMouselookExitTip",		mFadeMouselookExitTip);
}

void HBPrefsCoolImpl::apply()
{
	std::string short_date, long_date, short_time, long_time, timestamp;	

	LLComboBox* combo = getChild<LLComboBox>("time_format_combobox");
	if (combo)
	{
		mTimeFormat = combo->getCurrentIndex();
	}

	combo = getChild<LLComboBox>("date_format_combobox");
	if (combo)
	{
		mDateFormat = combo->getCurrentIndex();
	}

	if (mTimeFormat == 0)
	{
		short_time = "%H:%M";
		long_time  = "%H:%M:%S";
		timestamp  = " %H:%M:%S";
	}
	else
	{
		short_time = "%I:%M %p";
		long_time  = "%I:%M:%S %p";
		timestamp  = " %I:%M %p";
	}

	if (mDateFormat == 0)
	{
		short_date = "%Y-%m-%d";
		long_date  = "%A %d %B %Y";
		timestamp  = "%a %d %b %Y" + timestamp;
	}
	else if (mDateFormat == 1)
	{
		short_date = "%d/%m/%Y";
		long_date  = "%A %d %B %Y";
		timestamp  = "%a %d %b %Y" + timestamp;
	}
	else
	{
		short_date = "%m/%d/%Y";
		long_date  = "%A, %B %d %Y";
		timestamp  = "%a %b %d %Y" + timestamp;
	}

	gSavedSettings.setString("ShortDateFormat",	short_date);
	gSavedSettings.setString("LongDateFormat",	long_date);
	gSavedSettings.setString("ShortTimeFormat",	short_time);
	gSavedSettings.setString("LongTimeFormat",	long_time);
	gSavedSettings.setString("TimestampFormat",	timestamp);

	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		gSavedPerAccountSettings.setString("HighlightNicknames", childGetValue("highlight_nicknames_text"));
		gSavedPerAccountSettings.setBOOL("HighlightDisplayName", childGetValue("highlight_display_name_check"));
	}

	LLTextEditor* text = getChild<LLTextEditor>("receive_im_message_editor");
	LLWString message = text->getWText(); 
	LLWStringUtil::replaceTabsWithSpaces(message, 4);
	LLWStringUtil::replaceChar(message, '\n', '^');
	gSavedSettings.setString("RestrainedLoveRecvimMessage",
							 std::string(wstring_to_utf8str(message)));

	text = getChild<LLTextEditor>("send_im_message_editor");
	message = text->getWText(); 
	LLWStringUtil::replaceTabsWithSpaces(message, 4);
	LLWStringUtil::replaceChar(message, '\n', '^');
	gSavedSettings.setString("RestrainedLoveSendimMessage",
							 std::string(wstring_to_utf8str(message)));

	refreshValues();
}

//static
void HBPrefsCoolImpl::onCommitCheckBoxShowButton(LLUICtrl* ctrl, void* user_data)
{
	HBPrefsCoolImpl* self = (HBPrefsCoolImpl*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;

	bool enabled = check->get();
	if (enabled && gSavedSettings.getBOOL("ShowToolBar") == FALSE)
	{
		gSavedSettings.setBOOL("ShowToolBar", TRUE);
	}
}

//static
void HBPrefsCoolImpl::onCommitCheckBoxRestrainedLove(LLUICtrl* ctrl, void* user_data)
{
	HBPrefsCoolImpl* self = (HBPrefsCoolImpl*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;

	bool enable = check->get();
	self->refreshRestrainedLove(enable);
	if ((bool)self->mRestrainedLove != enable)
	{
		LLNotifications::instance().add("InEffectAfterRestart");
	}
}

//static
void HBPrefsCoolImpl::onCommitCheckBoxSpeedRez(LLUICtrl* ctrl, void* user_data)
{
	HBPrefsCoolImpl* self = (HBPrefsCoolImpl*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;

	bool enabled = check->get();
	self->childSetEnabled("speed_rez_interval", enabled);
	self->childSetEnabled("speed_rez_seconds", enabled);
}

//static
void HBPrefsCoolImpl::onCommitCheckBoxPrivateLookAt(LLUICtrl* ctrl, void* user_data)
{
	HBPrefsCoolImpl* self = (HBPrefsCoolImpl*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;

	bool enabled = check->get();
	self->childSetEnabled("private_look_at_limit", enabled);
	self->childSetEnabled("private_look_at_limit_meters", enabled);
}

//static
void HBPrefsCoolImpl::onCommitCheckBoxAfterRestart(LLUICtrl* ctrl, void* user_data)
{
	HBPrefsCoolImpl* self = (HBPrefsCoolImpl*)user_data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;

	BOOL saved = FALSE;
	std::string control = check->getControlName();
	if (control == "UseOldChatHistory")
	{
		saved = self->mUseOldChatHistory;
	}
	else if (control == "IMTabsVerticalStacking")
	{
		saved = self->mIMTabsVerticalStacking;
	}
	else if (control == "UseOldStatusBarIcons")
	{
		saved = self->mUseOldStatusBarIcons;
	}
	else if (control == "UseNewSLLoginPage")
	{
		saved = self->mUseNewSLLoginPage;
	}
	else if (control == "RestrainedLoveNoSetEnv")
	{
		saved = self->mRestrainedLoveNoSetEnv;
	}
	else if (control == "RestrainedLoveUntruncatedEmotes")
	{
		saved = self->mRestrainedLoveUntruncatedEmotes;
	}
	else if (control == "RestrainedLoveCanOoc")
	{
		saved = self->mRestrainedLoveUntruncatedEmotes;
	}
 	if (saved != check->get())
	{
		LLNotifications::instance().add("InEffectAfterRestart");
	}
}

//static
void HBPrefsCoolImpl::onCommitUserProfile(LLUICtrl* ctrl, void* user_data)
{
	HBPrefsCoolImpl* self = (HBPrefsCoolImpl*)user_data;
	LLRadioGroup* radio = (LLRadioGroup*)ctrl;
	if (!self || !radio) return;

	std::string blacklist;
	U32 profile = radio->getSelectedIndex();
	switch (profile)
	{
		case 0:
			blacklist.clear();
			break;
		case 1:
			blacklist = RRInterface::sRolePlayBlackList;
			break;
		case 2:
			blacklist = RRInterface::sVanillaBlackList;
			break;
		default:
			blacklist = gSavedSettings.getString("RestrainedLoveBlacklist");
			break;
	}
	gSavedSettings.setString("RestrainedLoveBlacklist",	blacklist);

	if (self->mRestrainedLoveUserProfile != profile)
	{
		LLNotifications::instance().add("InEffectAfterRestart");
	}
	self->mRestrainedLoveUserProfile = profile;
}

//static
void HBPrefsCoolImpl::onClickCustomBlackList(void* user_data)
{
	HBPrefsCoolImpl* self = (HBPrefsCoolImpl*)user_data;
	if (!self) return;
	HBFloaterBlacklistRLV::showInstance();
	self->mWatchBlackListFloater = true;
}

//---------------------------------------------------------------------------

HBPrefsCool::HBPrefsCool()
:	impl(* new HBPrefsCoolImpl())
{
}

HBPrefsCool::~HBPrefsCool()
{
	delete &impl;
}

void HBPrefsCool::apply()
{
	HBFloaterBlacklistRLV::closeInstance();
	impl.apply();
}

void HBPrefsCool::cancel()
{
	HBFloaterBlacklistRLV::closeInstance();
	impl.cancel();
}

LLPanel* HBPrefsCool::getPanel()
{
	return &impl;
}
