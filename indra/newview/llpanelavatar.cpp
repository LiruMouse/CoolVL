/** 
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llpanelavatar.h"

#include "llavatarconstants.h"
#include "llcachename.h"
#include "llcheckboxctrl.h"
#include "llclassifiedflags.h"
#include "llfloater.h"
#include "llfontgl.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llpluginclassmedia.h"
#include "llscrolllistctrl.h"
#include "lltabcontainer.h"
#include "lltabcontainervertical.h"
#include "lltexturectrl.h"
#include "lluiconstants.h"
#include "lluictrlfactory.h"
#include "roles_constants.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llfloateravatarinfo.h"
#include "llfloaterfriends.h"
#include "llfloatergroupinfo.h"
#include "llfloatermediabrowser.h"
#include "llfloatermute.h"
#include "llfloaterworldmap.h"
#include "llimview.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llnameeditor.h"
#include "llmutelist.h"
#include "llpanelclassified.h"
#include "llpanelpick.h"
#include "llstatusbar.h"
#include "lltooldraganddrop.h"
#include "llviewercontrol.h"
#include "llviewergenericmessage.h"	// for send_generic_message()
#include "llviewermenu.h"			// *FIX: for is_agent_friend()
#include "llviewernetwork.h"		// for gIsInSecondLife
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llweb.h"

// Statics
std::list<LLPanelAvatar*> LLPanelAvatar::sAllPanels;
BOOL LLPanelAvatar::sAllowFirstLife = FALSE;

extern void handle_lure(const LLUUID& invitee);
extern void handle_pay_by_id(const LLUUID& payee);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLDropTarget : public LLView
{
public:
	LLDropTarget(const std::string& name, const LLRect& rect, const LLUUID& agent_id);
	~LLDropTarget();

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	void setAgentID(const LLUUID &agent_id)		{ mAgentID = agent_id; }
protected:
	LLUUID mAgentID;
};

LLDropTarget::LLDropTarget(const std::string& name,
						   const LLRect& rect,
						   const LLUUID& agent_id)
:	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mAgentID(agent_id)
{
}

LLDropTarget::~LLDropTarget()
{
}

void LLDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "LLDropTarget::doDrop()" << llendl;
}

BOOL LLDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	if (getParent())
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mAgentID, LLUUID::null, drop,
												 cargo_type, cargo_data, accept);

		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// LLPanelAvatarTab()
//-----------------------------------------------------------------------------
LLPanelAvatarTab::LLPanelAvatarTab(const std::string& name,
								   const LLRect &rect, 
								   LLPanelAvatar* panel_avatar)
:	LLPanel(name, rect),
	mPanelAvatar(panel_avatar),
	mDataRequested(false)
{ }

// virtual
void LLPanelAvatarTab::draw()
{
	refresh();

	LLPanel::draw();
}

void LLPanelAvatarTab::sendAvatarProfileRequestIfNeeded(const std::string& method)
{
	if (!mDataRequested)
	{
		std::vector<std::string> strings;
		strings.push_back(mPanelAvatar->getAvatarID().asString());
		send_generic_message(method, strings);
		mDataRequested = true;
	}
}

//-----------------------------------------------------------------------------
// LLPanelAvatarSecondLife()
//-----------------------------------------------------------------------------
LLPanelAvatarSecondLife::LLPanelAvatarSecondLife(const std::string& name, 
												 const LLRect &rect, 
												 LLPanelAvatar* panel_avatar) 
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mPartnerID()
{
}

BOOL LLPanelAvatarSecondLife::postBuild(void)
{
	childSetEnabled("born", false);
	childSetEnabled("partner_edit", false);
	childSetAction("partner_help", onClickPartnerHelp, this);
	childSetAction("partner_info", onClickPartnerInfo, this);
	childSetEnabled("partner_info", mPartnerID.notNull());

	childSetAction("?", onClickPublishHelp, this);
	bool own_avatar = (getPanelAvatar()->getAvatarID() == gAgentID);
	enableControls(own_avatar);

	childSetVisible("About:", LLPanelAvatar::sAllowFirstLife);
	childSetVisible("(500 chars)", LLPanelAvatar::sAllowFirstLife);
	childSetVisible("about", LLPanelAvatar::sAllowFirstLife);

	childSetVisible("allow_publish", LLPanelAvatar::sAllowFirstLife);
	childSetVisible("?", LLPanelAvatar::sAllowFirstLife);

	childSetVisible("online_yes", false);

	childSetAction("Find on Map", LLPanelAvatar::onClickTrack, getPanelAvatar());
	childSetAction("Instant Message...", LLPanelAvatar::onClickIM, getPanelAvatar());

	childSetAction("Add Friend...", LLPanelAvatar::onClickAddFriend, getPanelAvatar());
	childSetAction("Pay...", LLPanelAvatar::onClickPay, getPanelAvatar());
	childSetAction("Mute", LLPanelAvatar::onClickMute, getPanelAvatar());

	childSetAction("Offer Teleport...", LLPanelAvatar::onClickOfferTeleport, getPanelAvatar());

	childSetDoubleClickCallback("groups", onDoubleClickGroup, this);

	getChild<LLTextureCtrl>("img")->setFallbackImageName("default_profile_picture.j2c");

	return TRUE;
}

void LLPanelAvatarSecondLife::refresh()
{
	updatePartnerName();
}

void LLPanelAvatarSecondLife::updatePartnerName()
{
	if (mPartnerID.notNull())
	{
		std::string first, last;
		BOOL found = gCacheName->getName(mPartnerID, first, last);
		if (found)
		{
			childSetTextArg("partner_edit", "[FIRST]", first);
			childSetTextArg("partner_edit", "[LAST]", last);
		}
		childSetEnabled("partner_info", TRUE);
	}
}

// Empty the data out of the controls, since we have to wait for new
// data off the network.
void LLPanelAvatarSecondLife::clearControls()
{
	LLTextureCtrl* image_ctrl = getChild<LLTextureCtrl>("img");
	if (image_ctrl)
	{
		image_ctrl->setImageAssetID(LLUUID::null);
	}
	childSetValue("about", "");
	childSetValue("born", "");
	childSetValue("acct", "");

	childSetTextArg("partner_edit", "[FIRST]", LLStringUtil::null);
	childSetTextArg("partner_edit", "[LAST]", LLStringUtil::null);

	mPartnerID = LLUUID::null;

	LLScrollListCtrl*	group_list = getChild<LLScrollListCtrl>("groups"); 
	if (group_list)
	{
		group_list->deleteAllItems();
	}
}

void LLPanelAvatarSecondLife::enableControls(BOOL own_avatar)
{
	childSetEnabled("img", own_avatar);
	childSetEnabled("about", own_avatar);
	childSetVisible("allow_publish", own_avatar);
	childSetEnabled("allow_publish", own_avatar);
	childSetVisible("?", own_avatar);
	childSetEnabled("?", own_avatar);
}

// static
void LLPanelAvatarSecondLife::onDoubleClickGroup(void* data)
{
	LLPanelAvatarSecondLife* self = (LLPanelAvatarSecondLife*)data;
	if (!self) return;
	LLScrollListCtrl* group_list = self->getChild<LLScrollListCtrl>("groups"); 
	if (group_list)
	{
		LLScrollListItem* item = group_list->getFirstSelected();

		if (item && item->getUUID().notNull())
		{
			llinfos << "Show group info " << item->getUUID() << llendl;

			LLFloaterGroupInfo::showFromUUID(item->getUUID());
		}
	}
}

// static
void LLPanelAvatarSecondLife::onClickPublishHelp(void *)
{
	LLNotifications::instance().add("ClickPublishHelpAvatar");
}

// static
void LLPanelAvatarSecondLife::onClickPartnerHelp(void *)
{
	LLNotifications::instance().add("ClickPartnerHelpAvatar", LLSD(), LLSD(),
									onClickPartnerHelpLoadURL);
}

// static 
bool LLPanelAvatarSecondLife::onClickPartnerHelpLoadURL(const LLSD& notification,
														const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLWeb::loadURL("http://secondlife.com/partner");
	}
	return false;
}

// static
void LLPanelAvatarSecondLife::onClickPartnerInfo(void *data)
{
	LLPanelAvatarSecondLife* self = (LLPanelAvatarSecondLife*) data;
	if (self->mPartnerID.notNull())
	{
		LLFloaterAvatarInfo::showFromProfile(self->mPartnerID,
											 self->getScreenRect());
	}
}

//-----------------------------------------------------------------------------
// LLPanelAvatarFirstLife()
//-----------------------------------------------------------------------------
LLPanelAvatarFirstLife::LLPanelAvatarFirstLife(const std::string& name, 
											   const LLRect &rect, 
											   LLPanelAvatar* panel_avatar) 
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}

BOOL LLPanelAvatarFirstLife::postBuild(void)
{
	bool own_avatar = (getPanelAvatar()->getAvatarID() == gAgentID);
	enableControls(own_avatar);

	getChild<LLTextureCtrl>("img")->setFallbackImageName("default_profile_picture.j2c");

	return TRUE;
}

void LLPanelAvatarFirstLife::enableControls(BOOL own_avatar)
{
	childSetEnabled("img", own_avatar);
	childSetEnabled("about", own_avatar);
}

//-----------------------------------------------------------------------------
// LLPanelAvatarWeb
//-----------------------------------------------------------------------------
LLPanelAvatarWeb::LLPanelAvatarWeb(const std::string& name,
								   const LLRect& rect, 
								   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mWebBrowser(NULL)
{
}

LLPanelAvatarWeb::~LLPanelAvatarWeb()
{
}

BOOL LLPanelAvatarWeb::postBuild(void)
{
	childSetKeystrokeCallback("url_edit", onURLKeystroke, this);
	childSetCommitCallback("load", onCommitLoad, this);
	childSetCommitCallback("sl_web_profile", onCommitSLWebProfile, this);
	childSetEnabled("sl_web_profile", gIsInSecondLife);

	childSetAction("web_profile_help", onClickWebProfileHelp, this);

	childSetCommitCallback("url_edit", onCommitURL, this);

	childSetControlName("auto_load", "AutoLoadWebProfiles");

	mWebBrowser = getChild<LLMediaCtrl>("profile_html");
	mWebBrowser->addObserver(this);

	return TRUE;
}

void LLPanelAvatarWeb::refresh()
{
	if (mNavigateTo != "")
	{
		llinfos << "Loading " << mNavigateTo << llendl;
		mWebBrowser->navigateTo(mNavigateTo);
		mNavigateTo = "";
	}
}

void LLPanelAvatarWeb::enableControls(BOOL own_avatar)
{
	mCanEditURL = own_avatar;
	childSetEnabled("url_edit", own_avatar);
}

void LLPanelAvatarWeb::setWebURL(std::string url)
{
	bool changed_url = (mHome != url);

	mHome = url;
	bool have_url = !mHome.empty();

	childSetText("url_edit", mHome);
	childSetEnabled("load", mHome.length() > 0);

	if (have_url && gSavedSettings.getBOOL("AutoLoadWebProfiles"))
	{
		if (changed_url)
		{
			load(mHome);
		}
	}
	else
	{
		childSetVisible("profile_html", false);
		childSetVisible("status_text", false);
	}
}

// static
void LLPanelAvatarWeb::onCommitURL(LLUICtrl* ctrl, void* data)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)data;

	if (!self) return;

	self->mHome = self->childGetText("url_edit");
	self->load(self->childGetText("url_edit"));
}

// static
void LLPanelAvatarWeb::onClickWebProfileHelp(void *)
{
	LLNotifications::instance().add("ClickWebProfileHelpAvatar");
}

void LLPanelAvatarWeb::load(std::string url)
{
	bool have_url = (!url.empty());

	childSetVisible("profile_html", have_url);
	childSetVisible("status_text", have_url);
	childSetText("status_text", LLStringUtil::null);

	if (have_url)
	{
		if (mCanEditURL)
		{
			childSetEnabled("url_edit", false);
		}
		childSetText("url_edit", getString("loading"));

		mNavigateTo = url;
	}
}

//static
void LLPanelAvatarWeb::onURLKeystroke(LLLineEditor* editor, void* data)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)data;
	if (!self) return;
	LLSD::String url = editor->getText();
	self->childSetEnabled("load", url.length() > 0);
	return;
}

// static
void LLPanelAvatarWeb::onCommitLoad(LLUICtrl* ctrl, void* data)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)data;
	if (!self || !ctrl) return;

	std::string valstr = ctrl->getValue().asString();
	std::string urlstr = self->childGetText("url_edit");
	if (valstr == "builtin")
	{
		 // open in built-in browser
		if (!self->mHome.empty())
		{
			LLFloaterMediaBrowser::showInstance(urlstr);
		}
	}
	else if (valstr == "open")
	{
		 // open in user's external browser
		if (!urlstr.empty())
		{
			LLWeb::loadURLExternal(urlstr);
		}
	}
	else if (valstr == "home")
	{
		 // reload profile owner's home page
		if (!self->mHome.empty())
		{
			self->mWebBrowser->setTrusted(false);
			self->load(self->mHome);
		}
	}
	else
	{
		 // load url string into browser panel
		if (!urlstr.empty())
		{
			self->mWebBrowser->setTrusted(false);
			self->load(urlstr);
		}
	}
}

// static
void LLPanelAvatarWeb::onCommitSLWebProfile(LLUICtrl* ctrl, void * data)
{
	LLPanelAvatarWeb* self = (LLPanelAvatarWeb*)data;
	if (!self || !ctrl) return;

	std::string user_name = self->getPanelAvatar()->getAvatarUserName();
	if (user_name.empty()) return;

	std::string urlstr = getProfileURL(user_name);
	if (urlstr.empty()) return;

	std::string valstr = ctrl->getValue().asString();
	if (valstr == "sl_builtin")
	{
		 // open in a trusted built-in browser
		LLFloaterMediaBrowser::showInstance(urlstr, true);
	}
	else if (valstr == "sl_open")
	{
		 // open in user's external browser
		LLWeb::loadURLExternal(urlstr);
	}
	else
	{
		// Load SL's web-based avatar profile (trusted)
		self->mWebBrowser->setTrusted(true);
		self->load(urlstr);
	}
}

void LLPanelAvatarWeb::handleMediaEvent(LLPluginClassMedia* self,
										EMediaEvent event)
{
	switch (event)
	{
		case MEDIA_EVENT_STATUS_TEXT_CHANGED:
			childSetText("status_text", self->getStatusText());
			break;
		case MEDIA_EVENT_LOCATION_CHANGED:
			childSetText("url_edit", self->getLocation());
			enableControls(mCanEditURL);
			break;
		default:
			// Having a default case makes the compiler happy.
			break;
	}
}

//-----------------------------------------------------------------------------
// LLPanelAvatarAdvanced
//-----------------------------------------------------------------------------
LLPanelAvatarAdvanced::LLPanelAvatarAdvanced(const std::string& name, 
											 const LLRect& rect, 
											 LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar),
	mWantToCount(0),
	mSkillsCount(0),
	mWantToEdit(NULL),
	mSkillsEdit(NULL)
{
}

BOOL LLPanelAvatarAdvanced::postBuild()
{
	for (size_t ii = 0; ii < LL_ARRAY_SIZE(mWantToCheck); ++ii)
	{
		mWantToCheck[ii] = NULL;
	}

	for (size_t ii = 0; ii < LL_ARRAY_SIZE(mSkillsCheck); ++ii)
	{
		mSkillsCheck[ii] = NULL;
	}

	mWantToCount = (8 > LL_ARRAY_SIZE(mWantToCheck)) ? LL_ARRAY_SIZE(mWantToCheck) : 8;
	for (S32 tt=0; tt < mWantToCount; ++tt)
	{
		std::string ctlname = llformat("chk%d", tt);
		mWantToCheck[tt] = getChild<LLCheckBoxCtrl>(ctlname);
	}

	mSkillsCount = (6 > LL_ARRAY_SIZE(mSkillsCheck)) ? LL_ARRAY_SIZE(mSkillsCheck) : 6;
	for (S32 tt = 0; tt < mSkillsCount; ++tt)
	{
		//Find the Skills checkboxes and save off thier controls
		std::string ctlname = llformat("schk%d", tt);
		mSkillsCheck[tt] = getChild<LLCheckBoxCtrl>(ctlname);
	}

	mWantToEdit = getChild<LLLineEditor>("want_to_edit");
	mSkillsEdit = getChild<LLLineEditor>("skills_edit");
	childSetVisible("skills_edit", LLPanelAvatar::sAllowFirstLife);
	childSetVisible("want_to_edit", LLPanelAvatar::sAllowFirstLife);

	return TRUE;
}

void LLPanelAvatarAdvanced::enableControls(BOOL own_avatar)
{
	S32 t;
	for (t = 0; t < mWantToCount; t++)
	{
		if (mWantToCheck[t])mWantToCheck[t]->setEnabled(own_avatar);
	}
	for (t = 0; t < mSkillsCount; t++)
	{
		if (mSkillsCheck[t])mSkillsCheck[t]->setEnabled(own_avatar);
	}

	if (mWantToEdit) mWantToEdit->setEnabled(own_avatar);
	if (mSkillsEdit) mSkillsEdit->setEnabled(own_avatar);
	childSetEnabled("languages_edit", own_avatar);
}

void LLPanelAvatarAdvanced::setWantSkills(U32 want_to_mask,
										  const std::string& want_to_text,
										  U32 skills_mask,
										  const std::string& skills_text,
										  const std::string& languages_text)
{
	for (int id =0;id<mWantToCount;id++)
	{
		mWantToCheck[id]->set(want_to_mask & 1<<id);
	}
	for (int id =0;id<mSkillsCount;id++)
	{
		mSkillsCheck[id]->set(skills_mask & 1<<id);
	}
	if (mWantToEdit && mSkillsEdit)
	{
		mWantToEdit->setText(want_to_text);
		mSkillsEdit->setText(skills_text);
	}

	childSetText("languages_edit",languages_text);
}

void LLPanelAvatarAdvanced::getWantSkills(U32* want_to_mask,
										  std::string& want_to_text,
										  U32* skills_mask,
										  std::string& skills_text,
										  std::string& languages_text)
{
	if (want_to_mask)
	{
		*want_to_mask = 0;
		for (int t=0;t<mWantToCount;t++)
		{
			if (mWantToCheck[t]->get())
				*want_to_mask |= 1<<t;
		}
	}
	if (skills_mask)
	{
		*skills_mask = 0;
		for (int t=0;t<mSkillsCount;t++)
		{
			if (mSkillsCheck[t]->get())
				*skills_mask |= 1<<t;
		}
	}
	if (mWantToEdit)
	{
		want_to_text = mWantToEdit->getText();
	}

	if (mSkillsEdit)
	{
		skills_text = mSkillsEdit->getText();
	}

	languages_text = childGetText("languages_edit");
}

//-----------------------------------------------------------------------------
// LLPanelAvatarNotes()
//-----------------------------------------------------------------------------
LLPanelAvatarNotes::LLPanelAvatarNotes(const std::string& name,
									   const LLRect& rect,
									   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}

BOOL LLPanelAvatarNotes::postBuild(void)
{
	childSetCommitCallback("notes edit", onCommitNotes, this);

	LLTextEditor* te = getChild<LLTextEditor>("notes edit");
	if (te) te->setCommitOnFocusLost(TRUE);
	return TRUE;
}

void LLPanelAvatarNotes::refresh()
{
	sendAvatarProfileRequestIfNeeded("avatarnotesrequest");
}

void LLPanelAvatarNotes::clearControls()
{
	childSetEnabled("notes edit", false);
	childSetText("notes edit", getString("Loading"));
}

// static
void LLPanelAvatarNotes::onCommitNotes(LLUICtrl*, void* userdata)
{
	LLPanelAvatarNotes* self = (LLPanelAvatarNotes*)userdata;
	self->getPanelAvatar()->sendAvatarNotesUpdate();
}

//-----------------------------------------------------------------------------
// LLPanelAvatarClassified()
//-----------------------------------------------------------------------------
LLPanelAvatarClassified::LLPanelAvatarClassified(const std::string& name,
												 const LLRect& rect,
												 LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}

BOOL LLPanelAvatarClassified::postBuild(void)
{
	childSetAction("New...", onClickNew, this);
	childSetAction("Delete...", onClickDelete, this);
	return TRUE;
}

void LLPanelAvatarClassified::refresh()
{
	BOOL self = (gAgentID == getPanelAvatar()->getAvatarID());

	LLTabContainer* tabs = getChild<LLTabContainer>("classified tab");

	S32 tab_count = tabs ? tabs->getTabCount() : 0;

	bool allow_new = tab_count < MAX_CLASSIFIEDS;
	bool allow_delete = (tab_count > 0);
	bool show_help = (tab_count == 0);

	// *HACK: Don't allow making new classifieds from inside the directory.
	// The logic for save/don't save when closing is too hairy, and the 
	// directory is conceptually read-only. JC
	bool in_directory = false;
	LLView* view = this;
	while (view)
	{
		if (view->getName() == "directory")
		{
			in_directory = true;
			break;
		}
		view = view->getParent();
	}
	childSetEnabled("New...", self && !in_directory && allow_new);
	childSetVisible("New...", !in_directory);
	childSetEnabled("Delete...", self && !in_directory && allow_delete);
	childSetVisible("Delete...", !in_directory);
	childSetVisible("classified tab", !show_help);

	sendAvatarProfileRequestIfNeeded("avatarclassifiedsrequest");
}

BOOL LLPanelAvatarClassified::canClose()
{
	LLTabContainer* tabs = getChild<LLTabContainer>("classified tab");
	for (S32 i = 0; i < tabs->getTabCount(); i++)
	{
		LLPanelClassified* panel = (LLPanelClassified*)tabs->getPanelByIndex(i);
		if (!panel->canClose())
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL LLPanelAvatarClassified::titleIsValid()
{
	LLTabContainer* tabs = getChild<LLTabContainer>("classified tab");
	if (tabs)
	{
		LLPanelClassified* panel = (LLPanelClassified*)tabs->getCurrentPanel();
		if (panel)
		{
			if (!panel->titleIsValid())
			{
				return FALSE;
			};
		};
	};

	return TRUE;
}

void LLPanelAvatarClassified::apply()
{
	LLTabContainer* tabs = getChild<LLTabContainer>("classified tab");
	for (S32 i = 0; i < tabs->getTabCount(); i++)
	{
		LLPanelClassified* panel = (LLPanelClassified*)tabs->getPanelByIndex(i);
		panel->apply();
	}
}

void LLPanelAvatarClassified::deleteClassifiedPanels()
{
	LLTabContainer* tabs = getChild<LLTabContainer>("classified tab");
	if (tabs)
	{
		tabs->deleteAllTabs();
	}

	childSetVisible("New...", false);
	childSetVisible("Delete...", false);
	childSetVisible("loading_text", true);
}

void LLPanelAvatarClassified::processAvatarClassifiedReply(LLMessageSystem* msg, void**)
{
	S32 block = 0;
	S32 block_count = 0;
	LLUUID classified_id;
	std::string classified_name;
	LLPanelClassified* panel_classified = NULL;

	LLTabContainer* tabs = getChild<LLTabContainer>("classified tab");

	// Don't remove old panels.  We need to be able to process multiple
	// packets for people who have lots of classifieds. JC

	block_count = msg->getNumberOfBlocksFast(_PREHASH_Data);
	for (block = 0; block < block_count; block++)
	{
		msg->getUUIDFast(_PREHASH_Data, _PREHASH_ClassifiedID, classified_id, block);
		msg->getStringFast(_PREHASH_Data, _PREHASH_Name, classified_name, block);

		panel_classified = new LLPanelClassified(false, false);

		panel_classified->setClassifiedID(classified_id);

		// This will request data from the server when the pick is first drawn.
		panel_classified->markForServerRequest();

		// The button should automatically truncate long names for us
		if (tabs)
		{
			tabs->addTabPanel(panel_classified, classified_name);
		}
	}

	// Make sure somebody is highlighted.  This works even if there
	// are no tabs in the container.
	if (tabs)
	{
		tabs->selectFirstTab();
	}

	childSetVisible("New...", true);
	childSetVisible("Delete...", true);
	childSetVisible("loading_text", false);
}

// Create a new classified panel.  It will automatically handle generating
// its own id when it's time to save.
// static
void LLPanelAvatarClassified::onClickNew(void* data)
{
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
	{
		return;
	}
//mk
	LLPanelAvatarClassified* self = (LLPanelAvatarClassified*)data;
	if (!self) return;

	LLNotifications::instance().add("AddClassified", LLSD(), LLSD(),
									boost::bind(&LLPanelAvatarClassified::callbackNew,
												self, _1, _2));

}

bool LLPanelAvatarClassified::callbackNew(const LLSD& notification,
										  const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLPanelClassified* panel_classified = new LLPanelClassified(false, false);
		panel_classified->initNewClassified();
		LLTabContainer*	tabs = getChild<LLTabContainer>("classified tab");
		if (tabs)
		{
			tabs->addTabPanel(panel_classified, panel_classified->getClassifiedName());
			tabs->selectLastTab();
		}
	}
	return false;
}

// static
void LLPanelAvatarClassified::onClickDelete(void* data)
{
	LLPanelAvatarClassified* self = (LLPanelAvatarClassified*)data;
	if (!self) return;

	LLTabContainer*	tabs = self->getChild<LLTabContainer>("classified tab");
	LLPanelClassified* panel_classified = NULL;
	if (tabs)
	{
		panel_classified = (LLPanelClassified*)tabs->getCurrentPanel();
	}
	if (!panel_classified) return;

	LLSD args;
	args["NAME"] = panel_classified->getClassifiedName();
	LLNotifications::instance().add("DeleteClassified", args, LLSD(),
									boost::bind(&LLPanelAvatarClassified::callbackDelete,
												self, _1, _2));

}

bool  LLPanelAvatarClassified::callbackDelete(const LLSD& notification,
											  const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLTabContainer*	tabs = getChild<LLTabContainer>("classified tab");
	LLPanelClassified* panel_classified=NULL;
	if (tabs)
	{
		panel_classified = (LLPanelClassified*)tabs->getCurrentPanel();
	}

	LLMessageSystem* msg = gMessageSystem;

	if (!panel_classified) return false;

	if (0 == option)
	{
		msg->newMessageFast(_PREHASH_ClassifiedDelete);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Data);
		msg->addUUIDFast(_PREHASH_ClassifiedID, panel_classified->getClassifiedID());
		gAgent.sendReliableMessage();

		if (tabs)
		{
			tabs->removeTabPanel(panel_classified);
		}
		delete panel_classified;
		panel_classified = NULL;
	}
	return false;
}

//-----------------------------------------------------------------------------
// LLPanelAvatarPicks()
//-----------------------------------------------------------------------------
LLPanelAvatarPicks::LLPanelAvatarPicks(const std::string& name, 
									   const LLRect& rect,
									   LLPanelAvatar* panel_avatar)
:	LLPanelAvatarTab(name, rect, panel_avatar)
{
}

BOOL LLPanelAvatarPicks::postBuild(void)
{
	childSetAction("New...", onClickNew, this);
	childSetAction("Delete...", onClickDelete, this);
	return TRUE;
}

void LLPanelAvatarPicks::refresh()
{
	BOOL self = (gAgentID == getPanelAvatar()->getAvatarID());
	LLTabContainer*	tabs = getChild<LLTabContainer>("picks tab");
	S32 tab_count = tabs ? tabs->getTabCount() : 0;
	childSetEnabled("New...",    self && tab_count < MAX_AVATAR_PICKS);
	childSetEnabled("Delete...", self && tab_count > 0);
	childSetVisible("New...",    self && getPanelAvatar()->isEditable());
	childSetVisible("Delete...", self && getPanelAvatar()->isEditable());

	sendAvatarProfileRequestIfNeeded("avatarpicksrequest");
}

void LLPanelAvatarPicks::deletePickPanels()
{
	LLTabContainer* tabs = getChild<LLTabContainer>("picks tab");
	if (tabs)
	{
		tabs->deleteAllTabs();
	}

	childSetVisible("New...", false);
	childSetVisible("Delete...", false);
	childSetVisible("loading_text", true);
}

void LLPanelAvatarPicks::processAvatarPicksReply(LLMessageSystem* msg, void**)
{
	S32 block = 0;
	S32 block_count = 0;
	LLUUID pick_id;
	std::string pick_name;
	LLPanelPick* panel_pick = NULL;

	LLTabContainer* tabs =  getChild<LLTabContainer>("picks tab");

	// Clear out all the old panels.  We'll replace them with the correct
	// number of new panels.
	deletePickPanels();

	// The database needs to know for which user to look up picks.
	LLUUID avatar_id = getPanelAvatar()->getAvatarID();

	block_count = msg->getNumberOfBlocks("Data");
	for (block = 0; block < block_count; block++)
	{
		msg->getUUID("Data", "PickID", pick_id, block);
		msg->getString("Data", "PickName", pick_name, block);

		panel_pick = new LLPanelPick(FALSE);
		panel_pick->setPickID(pick_id, avatar_id);
		// This will request data from the server when the pick is first
		// drawn.
		panel_pick->markForServerRequest();

		// The button should automatically truncate long names for us
		if (tabs)
		{
			tabs->addTabPanel(panel_pick, pick_name);
		}
	}

	// Make sure somebody is highlighted.  This works even if there
	// are no tabs in the container.
	if (tabs)
	{
		tabs->selectFirstTab();
	}

	childSetVisible("New...", true);
	childSetVisible("Delete...", true);
	childSetVisible("loading_text", false);
}

// Create a new pick panel.  It will automatically handle generating
// its own id when it's time to save.
// static
void LLPanelAvatarPicks::onClickNew(void* data)
{
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
	{
		return;
	}
//mk
	LLPanelAvatarPicks* self = (LLPanelAvatarPicks*)data;
	if (!self) return;

	LLPanelPick* panel_pick = new LLPanelPick(FALSE);
	panel_pick->initNewPick();

	LLTabContainer* tabs = self->getChild<LLTabContainer>("picks tab");
	if (tabs)
	{
		tabs->addTabPanel(panel_pick, panel_pick->getPickName());
		tabs->selectLastTab();
	}
}

// static
void LLPanelAvatarPicks::onClickDelete(void* data)
{
	LLPanelAvatarPicks* self = (LLPanelAvatarPicks*)data;
	if (!self) return;

	LLTabContainer* tabs = self->getChild<LLTabContainer>("picks tab");
	LLPanelPick* panel_pick = tabs ? (LLPanelPick*)tabs->getCurrentPanel()
								   : NULL;

	if (!panel_pick) return;

	LLSD args;
	args["PICK"] = panel_pick->getPickName();

	LLNotifications::instance().add("DeleteAvatarPick", args, LLSD(),
									boost::bind(&LLPanelAvatarPicks::callbackDelete,
												self, _1, _2));
}

// static
bool LLPanelAvatarPicks::callbackDelete(const LLSD& notification,
										const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	LLTabContainer* tabs = getChild<LLTabContainer>("picks tab");
	LLPanelPick* panel_pick = tabs ? (LLPanelPick*)tabs->getCurrentPanel()
								   : NULL;
	LLMessageSystem* msg = gMessageSystem;

	if (!panel_pick) return false;

	if (0 == option)
	{
		// If the viewer has a hacked god-mode, then this call will
		// fail.
		if (gAgent.isGodlike())
		{
			msg->newMessage("PickGodDelete");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgentID);
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("PickID", panel_pick->getPickID());
			// *HACK: We need to send the pick's creator id to accomplish
			// the delete, and we don't use the query id for anything. JC
			msg->addUUID("QueryID", panel_pick->getPickCreatorID());
		}
		else
		{
			msg->newMessage("PickDelete");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgentID);
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("PickID", panel_pick->getPickID());
		}
		gAgent.sendReliableMessage();

		if (tabs)
		{
			tabs->removeTabPanel(panel_pick);
		}
		delete panel_pick;
		panel_pick = NULL;
	}
	return false;
}

//-----------------------------------------------------------------------------
// LLPanelAvatar
//-----------------------------------------------------------------------------
LLPanelAvatar::LLPanelAvatar(const std::string& name,
							 const LLRect &rect,
							 BOOL allow_edit)
:	LLPanel(name, rect, FALSE),
	mPanelSecondLife(NULL),
	mPanelAdvanced(NULL),
	mPanelClassified(NULL),
	mPanelPicks(NULL),
	mPanelNotes(NULL),
	mPanelFirstLife(NULL),
	mPanelWeb(NULL),
	mDropTarget(NULL),
	mAvatarID(LLUUID::null),			// mAvatarID is set with setAvatarID()
	mAvatarUserName(std::string("")),
	mHaveProperties(FALSE),
	mHaveStatistics(FALSE),
	mHaveNotes(false),
	mLastNotes(),
	mAllowEdit(allow_edit)
{
	sAllPanels.push_back(this);

	LLCallbackMap::map_t factory_map;

	factory_map["2nd Life"] = LLCallbackMap(createPanelAvatarSecondLife, this);
	factory_map["WebProfile"] = LLCallbackMap(createPanelAvatarWeb, this);
	factory_map["Interests"] = LLCallbackMap(createPanelAvatarInterests, this);
	factory_map["Picks"] = LLCallbackMap(createPanelAvatarPicks, this);
	factory_map["Classified"] = LLCallbackMap(createPanelAvatarClassified, this);
	factory_map["1st Life"] = LLCallbackMap(createPanelAvatarFirstLife, this);
	factory_map["My Notes"] = LLCallbackMap(createPanelAvatarNotes, this);

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar.xml",
											   &factory_map);

	selectTab(0);
}

BOOL LLPanelAvatar::postBuild(void)
{
	mTab = getChild<LLTabContainer>("tab");
	childSetAction("Kick",		onClickKick,		this);
	childSetAction("Freeze",	onClickFreeze,		this);
	childSetAction("Unfreeze",	onClickUnfreeze,	this);
	childSetAction("csr_btn",	onClickCSR, 		this);
	childSetAction("OK",		onClickOK,			this);
	childSetAction("Cancel",	onClickCancel,		this);

	if (mTab && !sAllowFirstLife)
	{
		LLPanel* panel = mTab->getPanelByName("1st Life");
		if (panel) mTab->removeTabPanel(panel);

		panel = mTab->getPanelByName("WebProfile");
		if (panel) mTab->removeTabPanel(panel);
	}
	childSetVisible("Kick",		false);
	childSetEnabled("Kick",		false);
	childSetVisible("Freeze",	false);
	childSetEnabled("Freeze",	false);
	childSetVisible("Unfreeze",	false);
	childSetEnabled("Unfreeze",	false);
	childSetVisible("csr_btn",	false);
	childSetEnabled("csr_btn",	false);

	return TRUE;
}

LLPanelAvatar::~LLPanelAvatar()
{
	sAllPanels.remove(this);
}

BOOL LLPanelAvatar::canClose()
{
	return mPanelClassified && mPanelClassified->canClose();
}

void LLPanelAvatar::setOnlineStatus(EOnlineStatus online_status)
{
	// Online status NO could be because they are hidden
	// If they are a friend, we may know the truth!
	if (online_status != ONLINE_STATUS_YES && mIsFriend &&
		LLAvatarTracker::instance().isBuddyOnline(mAvatarID))
	{
		online_status = ONLINE_STATUS_YES;
	}

	mPanelSecondLife->childSetVisible("online_yes", online_status == ONLINE_STATUS_YES);

	// Since setOnlineStatus gets called after setAvatarID
	// need to make sure that "Offer Teleport" doesn't get set
	// to TRUE again for yourself
	if (mAvatarID != gAgentID)
	{
		childSetVisible("Offer Teleport...", true);
	}

	if (gAgent.isGodlike())
	{
		childSetEnabled("Offer Teleport...", true);
		childSetToolTip("Offer Teleport...", getString("TeleportGod"));
	}
	else if (gAgent.inPrelude())
	{
		childSetEnabled("Offer Teleport...", false);
		childSetToolTip("Offer Teleport...", getString("TeleportPrelude"));
	}
	else
	{
		childSetEnabled("Offer Teleport...", true);
		childSetToolTip("Offer Teleport...", getString("TeleportNormal"));
	}
}

void LLPanelAvatar::setAvatarID(const LLUUID &avatar_id, const std::string &name,
								EOnlineStatus online_status)
{
	if (avatar_id.isNull()) return;

//	bool avatar_changed = (avatar_id != mAvatarID);

	mAvatarID = avatar_id;

	// Determine if we have their calling card.
	mIsFriend = is_agent_friend(mAvatarID); 

	// setOnlineStatus uses mIsFriend
	setOnlineStatus(online_status);

	bool own_avatar = (mAvatarID == gAgentID);
	bool avatar_is_friend = LLAvatarTracker::instance().getBuddyInfo(mAvatarID) != NULL;

	mPanelSecondLife->enableControls(own_avatar && mAllowEdit);
	mPanelWeb->enableControls(own_avatar && mAllowEdit);
	mPanelAdvanced->enableControls(own_avatar && mAllowEdit);
	// Teens don't have this.
	if (mPanelFirstLife) mPanelFirstLife->enableControls(own_avatar && mAllowEdit);

	LLView *target_view = getChild<LLView>("drop_target_rect");
	if (target_view)
	{
		if (mDropTarget)
		{
			delete mDropTarget;
		}
		mDropTarget = new LLDropTarget("drop target", target_view->getRect(),
									   mAvatarID);
		addChild(mDropTarget);
		mDropTarget->setAgentID(mAvatarID);
	}

	LLNameEditor* name_edit = getChild<LLNameEditor>("name");
	std::string avname = name;
	if (name_edit)
	{
		if (name.empty())
		{
			name_edit->setNameID(avatar_id, FALSE);
		}
		else
		{
			name_edit->setText(avname);
		}
		childSetVisible("name", true);
	}
	LLNameEditor* complete_name_edit = getChild<LLNameEditor>("complete_name");
	if (complete_name_edit)
	{
		if (LLAvatarNameCache::useDisplayNames())
		{
			LLAvatarName avatar_name;
			if (LLAvatarNameCache::get(avatar_id, &avatar_name))
			{
				// Always show "Display Name [Legacy Name]" for security reasons
				avname = avatar_name.getNames();
				mAvatarUserName = avatar_name.mUsername;
			}
			else
			{
				avname = name_edit->getText();
				LLAvatarNameCache::get(avatar_id,
									   boost::bind(&LLPanelAvatar::completeNameCallback,
												   _1, _2, this));
			}
			complete_name_edit->setText(avname);
			childSetVisible("name", false);
			childSetVisible("complete_name", TRUE);
		}
		else
		{
			childSetVisible("complete_name", FALSE);
		}
	}
	// We can't set a tooltip on a text input box ("name" or "complete_name"),
	// so we set it on the profile picture... This can be helpful with very
	// long names (which would *appear* truncated in the textbox; even if it's
	// still possible to pan through the name with the mouse).
	LLTextureCtrl* image_ctrl = mPanelSecondLife->getChild<LLTextureCtrl>("img");
	if (image_ctrl)
	{
		std::string tooltip = avname;
		if (!own_avatar)
		{
			tooltip += "\n" + getString("click_to_enlarge");
		}
		image_ctrl->setToolTip(tooltip);
	}

// 	if (avatar_changed)
	{
		// While we're waiting for data off the network, clear out the
		// old data.
		mPanelSecondLife->clearControls();

		mPanelPicks->deletePickPanels();
		mPanelPicks->setDataRequested(false);

		mPanelClassified->deleteClassifiedPanels();
		mPanelClassified->setDataRequested(false);

		mHaveNotes = false;
		mLastNotes.clear();
		mPanelNotes->clearControls();
		mPanelNotes->setDataRequested(false);

		// Request just the first two pages of data.  The picks,
		// classifieds, and notes will be requested when that panel
		// is made visible. JC
		sendAvatarPropertiesRequest();

		if (own_avatar)
		{
			if (mAllowEdit)
			{
				// OK button disabled until properties data arrives
				childSetVisible("OK", true);
				childSetEnabled("OK", false);
				childSetVisible("Cancel", true);
				childSetEnabled("Cancel", true);
			}
			else
			{
				childSetVisible("OK", false);
				childSetEnabled("OK", false);
				childSetVisible("Cancel", false);
				childSetEnabled("Cancel", false);
			}
			childSetVisible("Instant Message...", false);
			childSetEnabled("Instant Message...", false);
			childSetVisible("Mute", false);
			childSetEnabled("Mute", false);
			childSetVisible("Offer Teleport...", false);
			childSetEnabled("Offer Teleport...", false);
			childSetVisible("drop target", false);
			childSetEnabled("drop target", false);
			childSetVisible("Find on Map", false);
			childSetEnabled("Find on Map", false);
			childSetVisible("Add Friend...", false);
			childSetEnabled("Add Friend...", false);
			childSetVisible("Pay...", false);
			childSetEnabled("Pay...", false);
		}
		else
		{
			childSetVisible("OK", false);
			childSetEnabled("OK", false);

			childSetVisible("Cancel", false);
			childSetEnabled("Cancel", false);

			childSetVisible("Instant Message...", true);
			childSetEnabled("Instant Message...", false);
			childSetVisible("Mute", true);
			childSetEnabled("Mute", false);

			childSetVisible("drop target", true);
			childSetEnabled("drop target", false);

			childSetVisible("Find on Map", true);
			// Note: we don't always know online status, so always allow gods
			// to try to track
			childSetEnabled("Find on Map", gAgent.isGodlike() ||
										   is_agent_mappable(mAvatarID));
			if (!mIsFriend)
			{
				childSetToolTip("Find on Map", getString("ShowOnMapNonFriend"));
			}
			else if (ONLINE_STATUS_YES != online_status)
			{
				childSetToolTip("Find on Map", getString("ShowOnMapFriendOffline"));
			}
			else
			{
				childSetToolTip("Find on Map", getString("ShowOnMapFriendOnline"));
			}
			childSetVisible("Add Friend...", true);
			childSetEnabled("Add Friend...", !avatar_is_friend);
			childSetVisible("Pay...", true);
			childSetEnabled("Pay...", false);
		}
		LLNameEditor* avatar_key = getChild<LLNameEditor>("avatar_key");
		if (avatar_key)
		{
			avatar_key->setText(avatar_id.asString());
		}
	}

	bool is_god = gAgent.isGodlike();

	childSetVisible("Kick", is_god);
	childSetEnabled("Kick", is_god);
	childSetVisible("Freeze", is_god);
	childSetEnabled("Freeze", is_god);
	childSetVisible("Unfreeze", is_god);
	childSetEnabled("Unfreeze", is_god);
	childSetVisible("csr_btn", is_god);
	childSetEnabled("csr_btn", is_god && gIsInSecondLife);
}

void LLPanelAvatar::completeNameCallback(const LLUUID& agent_id,
										 const LLAvatarName& avatar_name,
										 void *userdata)
{
	if (!LLAvatarNameCache::useDisplayNames())
	{
		return;
	}
	// look up all panels which have this avatar
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID == agent_id)
		{
			LLLineEditor* complete_name_edit = self->getChild<LLLineEditor>("complete_name");
			if (complete_name_edit)
			{
				self->mAvatarUserName = avatar_name.mUsername;
				// Always show "Display Name [Legacy Name]" for security reasons
				std::string avname = avatar_name.getNames();
				complete_name_edit->setText(avname);
				if (self->mPanelSecondLife)
				{
					LLTextureCtrl* image_ctrl = self->mPanelSecondLife->getChild<LLTextureCtrl>("img");
					if (image_ctrl)
					{
						std::string tooltip = avname;
						if (agent_id != gAgentID)
						{
							tooltip += "\n" + self->getString("click_to_enlarge");
						}
						image_ctrl->setToolTip(tooltip);
					}
				}
			}
		}
	}
}

void LLPanelAvatar::resetGroupList()
{
	// only get these updates asynchronously via the group floater, which works
	// on the agent only
	if (mAvatarID != gAgentID)
	{
		return;
	}

	if (mPanelSecondLife)
	{
		LLScrollListCtrl* group_list = mPanelSecondLife->getChild<LLScrollListCtrl>("groups");
		if (group_list)
		{
			group_list->deleteAllItems();

			S32 count = gAgent.mGroups.count();
			LLUUID id;

			for (S32 i = 0; i < count; ++i)
			{
				LLGroupData group_data = gAgent.mGroups.get(i);
				id = group_data.mID;
				std::string group_string;
				/* Show group title?  DUMMY_POWER for Don Grep
				if (group_data.mOfficer)
				{
					group_string = "Officer of ";
				}
				else
				{
					group_string = "Member of ";
				}
				*/

				group_string += group_data.mName;

				LLSD row;
				row["id"] = id ;
				row["columns"][0]["value"] = group_string;
				row["columns"][0]["font"] = "SANSSERIF_SMALL";
				row["columns"][0]["width"] = 0;
				group_list->addElement(row);
			}
			group_list->sortByColumnIndex(0, TRUE);
		}
	}
}

// static
//-----------------------------------------------------------------------------
// onClickIM()
//-----------------------------------------------------------------------------
void LLPanelAvatar::onClickIM(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	if (!self || !gIMMgr) return;

	gIMMgr->setFloaterOpen(TRUE);

	std::string name;
	LLNameEditor* nameedit = self->mPanelSecondLife->getChild<LLNameEditor>("name");
	if (nameedit) name = nameedit->getText();
	gIMMgr->addSession(name, IM_NOTHING_SPECIAL, self->mAvatarID);
}

// static
//-----------------------------------------------------------------------------
// onClickTrack()
//-----------------------------------------------------------------------------
void LLPanelAvatar::onClickTrack(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	if (!self) return;

	if (gFloaterWorldMap)
	{
		std::string name;
		LLNameEditor* nameedit = self->mPanelSecondLife->getChild<LLNameEditor>("name");
		if (nameedit) name = nameedit->getText();
		gFloaterWorldMap->trackAvatar(self->mAvatarID, name);
		LLFloaterWorldMap::show(NULL, TRUE);
	}
}

// static
void LLPanelAvatar::onClickAddFriend(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	if (!self) return;

	LLNameEditor* name_edit = self->mPanelSecondLife->getChild<LLNameEditor>("name");
	if (name_edit)
	{
		LLFloaterFriends::requestFriendshipDialog(self->getAvatarID(),
												  name_edit->getText());
	}
}

//-----------------------------------------------------------------------------
// onClickMute()
//-----------------------------------------------------------------------------
void LLPanelAvatar::onClickMute(void *userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;

	LLUUID agent_id = self->getAvatarID();
	LLNameEditor* name_edit = self->mPanelSecondLife->getChild<LLNameEditor>("name");

	LLMuteList* ml = LLMuteList::getInstance();
	if (ml && name_edit)
	{
		std::string agent_name = name_edit->getText();

		if (ml->isMuted(agent_id))
		{
			LLFloaterMute::showInstance();
			LLFloaterMute::getInstance()->selectMute(agent_id);
		}
		else
		{
			LLMute mute(agent_id, agent_name, LLMute::AGENT);
			if (ml->add(mute))
			{
				LLFloaterMute::showInstance();
				LLFloaterMute::getInstance()->selectMute(mute.mID);
			}
		}
	}
}

// static
void LLPanelAvatar::onClickOfferTeleport(void *userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	handle_lure(self->mAvatarID);
}

// static
void LLPanelAvatar::onClickPay(void *userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	handle_pay_by_id(self->mAvatarID);
}

// static
void LLPanelAvatar::onClickOK(void *userdata)
{
	LLPanelAvatar *self = (LLPanelAvatar *)userdata;

	// JC: Only save the data if we actually got the original properties.
	// Otherwise we might save blanks into the database.
	if (self && self->mHaveProperties)
	{
		self->sendAvatarPropertiesUpdate();

		LLTabContainer* tabs = self->getChild<LLTabContainer>("tab");
		if (tabs->getCurrentPanel() != self->mPanelClassified)
		{
			self->mPanelClassified->apply();

			LLFloaterAvatarInfo *infop = LLFloaterAvatarInfo::getInstance(self->mAvatarID);
			if (infop)
			{
				infop->close();
			}
		}
		else
		{
			if (self->mPanelClassified->titleIsValid())
			{
				self->mPanelClassified->apply();

				LLFloaterAvatarInfo *infop = LLFloaterAvatarInfo::getInstance(self->mAvatarID);
				if (infop)
				{
					infop->close();
				}
			}
		}
	}
}

// static
void LLPanelAvatar::onClickCancel(void *userdata)
{
	LLPanelAvatar *self = (LLPanelAvatar *)userdata;

	if (self)
	{
		LLFloaterAvatarInfo *infop;
		if ((infop = LLFloaterAvatarInfo::getInstance(self->mAvatarID)))
		{
			infop->close();
		}
		else
		{
			// We're in the Search directory and are cancelling an edit
			// to our own profile, so reset.
			self->sendAvatarPropertiesRequest();
		}
	}
}

void LLPanelAvatar::sendAvatarPropertiesRequest()
{
	//lldebugs << "LLPanelAvatar::sendAvatarPropertiesRequest()" << llendl; 
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_AvatarID, mAvatarID);
	gAgent.sendReliableMessage();
}

void LLPanelAvatar::sendAvatarNotesUpdate()
{
	std::string notes = mPanelNotes->childGetValue("notes edit").asString();

	if (!mHaveNotes || notes == mLastNotes || notes == getString("Loading"))
	{
		// no notes from server and no user updates
		return;
	}

	LLMessageSystem *msg = gMessageSystem;

	msg->newMessage("AvatarNotesUpdate");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addUUID("TargetID", mAvatarID);
	msg->addString("Notes", notes);

	gAgent.sendReliableMessage();
}

// static
void LLPanelAvatar::processAvatarPropertiesReply(LLMessageSystem *msg, void**)
{
	LLUUID agent_id;	// your id
	LLUUID avatar_id;	// target of this panel
	LLUUID image_id;
	LLUUID fl_image_id;
	LLUUID partner_id;
	std::string	about_text;
	std::string	fl_about_text;
	std::string	born_on;
	std::string	profile_url;
	BOOL allow_publish = FALSE;
	BOOL identified = FALSE;
	BOOL transacted = FALSE;
	BOOL age_verified = FALSE;
	BOOL online = FALSE;
	S32 charter_member_size = 0;
	U32 flags = 0x0;

	//llinfos << "properties packet size " << msg->getReceiveSize() << llendl;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_id);

	for (panel_list_t::iterator iter = sAllPanels.begin();
		 iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != avatar_id)
		{
			continue;
		}
		self->childSetEnabled("Instant Message...", TRUE);
		self->childSetEnabled("Pay...", TRUE);
		self->childSetEnabled("Mute", TRUE);

		self->childSetEnabled("drop target", TRUE);

		self->mHaveProperties = TRUE;
		self->enableOKIfReady();

		msg->getUUIDFast(_PREHASH_PropertiesData, _PREHASH_ImageID, image_id);
		msg->getUUIDFast(_PREHASH_PropertiesData, _PREHASH_FLImageID, fl_image_id);
		msg->getUUIDFast(_PREHASH_PropertiesData, _PREHASH_PartnerID, partner_id);
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_AboutText, about_text);
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_FLAboutText, fl_about_text);
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_BornOn, born_on);
		msg->getString("PropertiesData","ProfileURL", profile_url);
		msg->getU32Fast(_PREHASH_PropertiesData, _PREHASH_Flags, flags);

		tm t;
		if (sscanf(born_on.c_str(), "%u/%u/%u", &t.tm_mon, &t.tm_mday, &t.tm_year) == 3
			&& t.tm_year > 1900)
		{
			t.tm_year -= 1900;
			t.tm_mon--;
			t.tm_hour = t.tm_min = t.tm_sec = 0;
			timeStructToFormattedString(&t,
										gSavedSettings.getString("ShortDateFormat"),
										born_on);
		}

		identified = (flags & AVATAR_IDENTIFIED);
		transacted = (flags & AVATAR_TRANSACTED);
		allow_publish = (flags & AVATAR_ALLOW_PUBLISH);
		online = (flags & AVATAR_ONLINE);
		// Not currently reported by server for privacy considerations
		age_verified = (flags & AVATAR_AGEVERIFIED); 

		U8 caption_index = 0;
		std::string caption_text;
		charter_member_size = msg->getSize("PropertiesData", "CharterMember");
		if (1 == charter_member_size)
		{
			msg->getBinaryData("PropertiesData", "CharterMember", &caption_index, 1);
		}
		else if (1 < charter_member_size)
		{
			msg->getString("PropertiesData", "CharterMember", caption_text);
		}

		if (caption_text.empty())
		{
			LLStringUtil::format_map_t args;
			caption_text = self->mPanelSecondLife->getString("CaptionTextAcctInfo");

			const char* ACCT_TYPE[] = {
				"AcctTypeResident",
				"AcctTypeTrial",
				"AcctTypeCharterMember",
				"AcctTypeEmployee"
			};
			caption_index = llclamp(caption_index, (U8)0, (U8)(LL_ARRAY_SIZE(ACCT_TYPE)-1));
			args["[ACCTTYPE]"] = self->mPanelSecondLife->getString(ACCT_TYPE[caption_index]);

			std::string payment_text = " ";
			const S32 DEFAULT_CAPTION_LINDEN_INDEX = 3;
			if (caption_index != DEFAULT_CAPTION_LINDEN_INDEX)
			{
				if (transacted)
				{
					payment_text = "PaymentInfoUsed";
				}
				else if (identified)
				{
					payment_text = "PaymentInfoOnFile";
				}
				else
				{
					payment_text = "NoPaymentInfoOnFile";
				}
				args["[PAYMENTINFO]"] = self->mPanelSecondLife->getString(payment_text);
				std::string age_text = age_verified ? "AgeVerified" : "NotAgeVerified";
				// Do not display age verification status at this time
				//args["[[AGEVERIFICATION]]"] = self->mPanelSecondLife->getString(age_text);
				args["[AGEVERIFICATION]"] = " ";
			}
			else
			{
				args["[PAYMENTINFO]"] = " ";
				args["[AGEVERIFICATION]"] = " ";
			}
			LLStringUtil::format(caption_text, args);
		}

		self->mPanelSecondLife->childSetValue("acct", caption_text);
		self->mPanelSecondLife->childSetValue("born", born_on);

		EOnlineStatus online_status = (online) ? ONLINE_STATUS_YES : ONLINE_STATUS_NO;

		self->setOnlineStatus(online_status);

		self->mPanelWeb->setWebURL(profile_url);

		LLTextureCtrl* image_ctrl = self->mPanelSecondLife->getChild<LLTextureCtrl>("img");
		if (image_ctrl)
		{
			image_ctrl->setImageAssetID(image_id);
		}

		LLTextEditor* about_ctrl = self->getChild<LLTextEditor>("about");
		// We need to prune the highlights, and clear() is not doing it...
		about_ctrl->removeTextFromEnd(about_ctrl->getMaxLength());
		if (self->mAvatarID == gAgentID)
		{
			about_ctrl->setText(about_text);
		}
		else
		{
			about_ctrl->appendColoredText(about_text, false, false,
										  about_ctrl->getReadOnlyFgColor());
		}

		self->mPanelSecondLife->setPartnerID(partner_id);
		self->mPanelSecondLife->updatePartnerName();

		if (self->mPanelFirstLife)
		{
			// Teens don't get these

			about_ctrl = self->mPanelFirstLife->getChild<LLTextEditor>("about");
			// We need to prune the highlights, and clear() is not doing it...
			about_ctrl->removeTextFromEnd(about_ctrl->getMaxLength());
			if (self->mAvatarID == gAgentID)
			{
				about_ctrl->setText(fl_about_text);
			}
			else
			{
				about_ctrl->appendColoredText(fl_about_text, false, false,
											  about_ctrl->getReadOnlyFgColor());
			}

			LLTextureCtrl* image_ctrl = self->mPanelFirstLife->getChild<LLTextureCtrl>("img");
			if (image_ctrl)
			{
				image_ctrl->setImageAssetID(fl_image_id);
				if (avatar_id == gAgentID || fl_image_id.isNull())
				{
					image_ctrl->setToolTip(std::string(""));
				}
				else
				{
					image_ctrl->setToolTip(self->getString("click_to_enlarge"));
				}
			}

			self->mPanelSecondLife->childSetValue("allow_publish", allow_publish);
		}
	}
}

// static
void LLPanelAvatar::processAvatarInterestsReply(LLMessageSystem *msg, void**)
{
	LLUUID	agent_id;	// your id
	LLUUID	avatar_id;	// target of this panel

	U32		want_to_mask;
	std::string	want_to_text;
	U32		skills_mask;
	std::string	skills_text;
	std::string	languages_text;

	//llinfos << "properties packet size " << msg->getReceiveSize() << llendl;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_id);

	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != avatar_id)
		{
			continue;
		}

		msg->getU32Fast(_PREHASH_PropertiesData, _PREHASH_WantToMask, want_to_mask);
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_WantToText, want_to_text);
		msg->getU32Fast(_PREHASH_PropertiesData, _PREHASH_SkillsMask, skills_mask);
		msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_SkillsText, skills_text);
		msg->getString(_PREHASH_PropertiesData, "LanguagesText", languages_text);

		self->mPanelAdvanced->setWantSkills(want_to_mask, want_to_text,
											skills_mask, skills_text,
											languages_text);
	}
}

// Separate function because the groups list can be very long, almost
// filling a packet. JC
// static
void LLPanelAvatar::processAvatarGroupsReply(LLMessageSystem *msg, void**)
{
	std::string	group_title;
	std::string	group_name;
	LLUUID agent_id;		// your id
	LLUUID avatar_id;		// target of this panel
	LLUUID group_id;
	LLUUID group_insignia_id;
	U64 group_powers;

	llinfos << "groups packet size " << msg->getReceiveSize() << llendl;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_id);

	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != avatar_id)
		{
			continue;
		}

		LLScrollListCtrl* group_list = self->mPanelSecondLife->getChild<LLScrollListCtrl>("groups"); 
// 		if (group_list)
// 		{
// 			group_list->deleteAllItems();
// 		}

		S32 group_count = msg->getNumberOfBlocksFast(_PREHASH_GroupData);
		if (0 == group_count)
		{
			if (group_list) group_list->addCommentText(std::string("None")); // *TODO: Translate
		}
		else
		{
			for (S32 i = 0; i < group_count; ++i)
			{
				msg->getU64(_PREHASH_GroupData, "GroupPowers", group_powers, i);
				msg->getStringFast(_PREHASH_GroupData, _PREHASH_GroupTitle, group_title, i);
				msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group_id, i);
				msg->getStringFast(_PREHASH_GroupData, _PREHASH_GroupName, group_name, i);
				msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupInsigniaID, group_insignia_id, i);

				std::string group_string;
				if (group_id.notNull())
				{
					group_string.assign(group_name);
				}
				else
				{
					group_string.assign("");
				}

				// Is this really necessary?  Remove existing entry if it exists.
				// TODO: clear the whole list when a request for data is made
				if (group_list)
				{
					S32 index = group_list->getItemIndex(group_id);
					if (index >= 0)
					{
						group_list->deleteSingleItem(index);
					}
				}

				LLSD row;
				row["id"] = group_id;
				row["columns"][0]["value"] = group_string;
				row["columns"][0]["font"] = "SANSSERIF_SMALL";
				if (group_list)
				{
					group_list->addElement(row);
				}
			}
		}
		if (group_list) group_list->sortByColumnIndex(0, TRUE);
	}
}

// Don't enable the OK button until you actually have the data.
// Otherwise you will write blanks back into the database.
void LLPanelAvatar::enableOKIfReady()
{
	if (mHaveProperties && childIsVisible("OK"))
	{
		childSetEnabled("OK", true);
	}
	else
	{
		childSetEnabled("OK", false);
	}
}

void LLPanelAvatar::sendAvatarPropertiesUpdate()
{
	llinfos << "Sending avatarinfo update" << llendl;
	BOOL allow_publish = FALSE;
	BOOL mature = FALSE;
	if (LLPanelAvatar::sAllowFirstLife)
	{
		allow_publish = childGetValue("allow_publish");
		//A profile should never be mature.
		mature = FALSE;
	}
	U32 want_to_mask = 0x0;
	U32 skills_mask = 0x0;
	std::string want_to_text;
	std::string skills_text;
	std::string languages_text;
	mPanelAdvanced->getWantSkills(&want_to_mask, want_to_text, &skills_mask, skills_text, languages_text);

	LLUUID first_life_image_id;
	std::string first_life_about_text;
	if (mPanelFirstLife)
	{
		first_life_about_text = mPanelFirstLife->childGetValue("about").asString();
		LLTextureCtrl* image_ctrl = mPanelFirstLife->getChild<LLTextureCtrl>("img");
		if (image_ctrl)
		{
			first_life_image_id = image_ctrl->getImageAssetID();
		}
	}

	std::string about_text = mPanelSecondLife->childGetValue("about").asString();

	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_PropertiesData);

	LLTextureCtrl* image_ctrl = mPanelSecondLife->getChild<LLTextureCtrl>("img");
	if (image_ctrl)
	{
		msg->addUUIDFast(_PREHASH_ImageID, image_ctrl->getImageAssetID());
	}
	else
	{
		msg->addUUIDFast(_PREHASH_ImageID, LLUUID::null);
	}
//	msg->addUUIDFast(_PREHASH_ImageID, mPanelSecondLife->mimage_ctrl->getImageAssetID());
	msg->addUUIDFast(_PREHASH_FLImageID, first_life_image_id);
	msg->addStringFast(_PREHASH_AboutText, about_text);
	msg->addStringFast(_PREHASH_FLAboutText, first_life_about_text);

	msg->addBOOL("AllowPublish", allow_publish);
	msg->addBOOL("MaturePublish", mature);
	msg->addString("ProfileURL", mPanelWeb->getWebURL());
	gAgent.sendReliableMessage();

	msg->newMessage("AvatarInterestsUpdate");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_PropertiesData);
	msg->addU32Fast(_PREHASH_WantToMask, want_to_mask);
	msg->addStringFast(_PREHASH_WantToText, want_to_text);
	msg->addU32Fast(_PREHASH_SkillsMask, skills_mask);
	msg->addStringFast(_PREHASH_SkillsText, skills_text);
	msg->addString("LanguagesText", languages_text);
	gAgent.sendReliableMessage();
}

void LLPanelAvatar::selectTab(S32 tabnum)
{
	if (mTab)
	{
		mTab->selectTab(tabnum);
	}
}

void LLPanelAvatar::selectTabByName(std::string tab_name)
{
	if (mTab)
	{
		if (tab_name.empty())
		{
			mTab->selectFirstTab();
		}
		else
		{
			mTab->selectTabByName(tab_name);
		}
	}
}

void LLPanelAvatar::processAvatarNotesReply(LLMessageSystem *msg, void**)
{
	// extract the agent id
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);

	LLUUID target_id;
	msg->getUUID("Data", "TargetID", target_id);

	// look up all panels which have this avatar
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != target_id)
		{
			continue;
		}

		std::string text;
		msg->getString("Data", "Notes", text);
		self->childSetValue("notes edit", text);
		self->childSetEnabled("notes edit", true);
		self->mHaveNotes = true;
		self->mLastNotes = text;
	}
}

void LLPanelAvatar::processAvatarClassifiedReply(LLMessageSystem *msg, void** userdata)
{
	LLUUID agent_id;
	LLUUID target_id;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TargetID, target_id);

	// look up all panels which have this avatar target
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != target_id)
		{
			continue;
		}

		self->mPanelClassified->processAvatarClassifiedReply(msg, userdata);
	}
}

void LLPanelAvatar::processAvatarPicksReply(LLMessageSystem *msg, void** userdata)
{
	LLUUID agent_id;
	LLUUID target_id;

	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("AgentData", "TargetID", target_id);

	// look up all panels which have this avatar target
	for (panel_list_t::iterator iter = sAllPanels.begin(); iter != sAllPanels.end(); ++iter)
	{
		LLPanelAvatar* self = *iter;
		if (self->mAvatarID != target_id)
		{
			continue;
		}

		self->mPanelPicks->processAvatarPicksReply(msg, userdata);
	}
}

// static
void LLPanelAvatar::onClickKick(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	if (!self) return;

	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect(left, top, left + 400, top - 300);

	LLSD payload;
	payload["avatar_id"] = self->mAvatarID;
	LLNotifications::instance().add("KickUser", LLSD(), payload, finishKick);
}

//static
bool LLPanelAvatar::finishKick(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID, gAgentID);
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID, avatar_id);
		msg->addU32("KickFlags", KICK_FLAGS_DEFAULT);
		msg->addStringFast(_PREHASH_Reason, response["message"].asString());
		gAgent.sendReliableMessage();
	}
	return false;
}

// static
void LLPanelAvatar::onClickFreeze(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	if (!self) return;

	LLSD payload;
	payload["avatar_id"] = self->mAvatarID;
	LLNotifications::instance().add("FreezeUser", LLSD(), payload, LLPanelAvatar::finishFreeze);
}

// static
bool LLPanelAvatar::finishFreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID, gAgentID);
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID, avatar_id);
		msg->addU32("KickFlags", KICK_FLAGS_FREEZE);
		msg->addStringFast(_PREHASH_Reason, response["message"].asString());
		gAgent.sendReliableMessage();
	}
	return false;
}

// static
void LLPanelAvatar::onClickUnfreeze(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*) userdata;
	if (!self) return;

	LLSD payload;
	payload["avatar_id"] = self->mAvatarID;
	LLNotifications::instance().add("UnFreezeUser", LLSD(), payload, LLPanelAvatar::finishUnfreeze);
}

// static
bool LLPanelAvatar::finishUnfreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	std::string text = response["message"].asString();
	if (option == 0)
	{
		LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID, gAgentID);
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID, avatar_id);
		msg->addU32("KickFlags", KICK_FLAGS_UNFREEZE);
		msg->addStringFast(_PREHASH_Reason, text);
		gAgent.sendReliableMessage();
	}
	return false;
}

// static
void LLPanelAvatar::onClickCSR(void* userdata)
{
	LLPanelAvatar* self = (LLPanelAvatar*)userdata;
	if (!self) return;

	LLNameEditor* name_edit = self->getChild<LLNameEditor>("name");
	if (!name_edit) return;

	std::string name = name_edit->getText();
	if (name.empty()) return;

	std::string url = "http://csr.lindenlab.com/agent/";

	// slow and stupid, but it's late
	S32 len = name.length();
	for (S32 i = 0; i < len; i++)
	{
		if (name[i] == ' ')
		{
			url += "%20";
		}
		else
		{
			url += name[i];
		}
	}

	LLWeb::loadURL(url);
}

void* LLPanelAvatar::createPanelAvatarSecondLife(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelSecondLife = new LLPanelAvatarSecondLife(std::string("2nd Life"),
														 LLRect(), self);
	return self->mPanelSecondLife;
}

void* LLPanelAvatar::createPanelAvatarWeb(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelWeb = new LLPanelAvatarWeb(std::string("Web"), LLRect(), self);
	return self->mPanelWeb;
}

void* LLPanelAvatar::createPanelAvatarInterests(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelAdvanced = new LLPanelAvatarAdvanced(std::string("Interests"),
													 LLRect(), self);
	return self->mPanelAdvanced;
}

void* LLPanelAvatar::createPanelAvatarPicks(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelPicks = new LLPanelAvatarPicks(std::string("Picks"), LLRect(),
											   self);
	return self->mPanelPicks;
}

void* LLPanelAvatar::createPanelAvatarClassified(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelClassified = new LLPanelAvatarClassified(std::string("Classified"),
														 LLRect(), self);
	return self->mPanelClassified;
}

void* LLPanelAvatar::createPanelAvatarFirstLife(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelFirstLife = new LLPanelAvatarFirstLife(std::string("1st Life"),
													   LLRect(), self);
	return self->mPanelFirstLife;
}

void* LLPanelAvatar::createPanelAvatarNotes(void* data)
{
	LLPanelAvatar* self = (LLPanelAvatar*)data;
	self->mPanelNotes = new LLPanelAvatarNotes(std::string("My Notes"),
											   LLRect(), self);
	return self->mPanelNotes;
}
