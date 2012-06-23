/**
 * @file llpaneldirfind.cpp
 * @brief The "All" panel in the Search directory.
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

#include "llpaneldirfind.h"

// linden library includes
#include "llclassifiedflags.h"
#include "llfontgl.h"
#include "llparcel.h"
#include "llqueryflags.h"
#include "message.h"

// viewer project includes
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llmenucommands.h"
#include "llmenugl.h"
#include "llpluginclassmedia.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llviewertexturelist.h"
#include "llviewermessage.h"
#include "llfloateravatarinfo.h"
#include "lldir.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llfloaterdirectory.h"
#include "llpaneldirbrowser.h"
#include "lluserauth.h"
#include "llweb.h"

#if LL_MSVC
// disable boost::lexical_cast warning
#pragma warning (disable:4702)
#endif
#include "boost/tokenizer.hpp"
#include "boost/lexical_cast.hpp"

std::string LLPanelDirFind::sSearchToken = "";

//---------------------------------------------------------------------------
// LLPanelDirFindAll - Google search appliance based search
//---------------------------------------------------------------------------

class LLPanelDirFindAll
:	public LLPanelDirFind
{
public:
	LLPanelDirFindAll(const std::string& name, LLFloaterDirectory* floater);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);
	/*virtual*/ void search(const std::string& search_text);
};

LLPanelDirFindAll::LLPanelDirFindAll(const std::string& name, LLFloaterDirectory* floater)
:	LLPanelDirFind(name, floater, "find_browser")
{
}

//---------------------------------------------------------------------------
// LLPanelDirFind - Base class for all browser-based search tabs
//---------------------------------------------------------------------------

LLPanelDirFind::LLPanelDirFind(const std::string& name, LLFloaterDirectory* floater, const std::string& browser_name)
:	LLPanelDirBrowser(name, floater),
	mWebBrowser(NULL),
	mBrowserName(browser_name)
{
}

BOOL LLPanelDirFind::postBuild()
{
	LLPanelDirBrowser::postBuild();

	childSetAction("back_btn", onClickBack, this);
	childSetAction("forward_btn", onClickForward, this);

	// showcase doesn't have other buttons/UI elements
	if (hasChild("incmature"))
	{
		childSetCommitCallback("search_editor", onCommitSearch, this);
		childSetCommitCallback("new_search", onCommitNewSearch, this);
		childSetAction("search_btn", onClickSearch, this);
		childSetAction("?", onClickHelp, this);

		// Teens don't get mature checkbox
		if (gAgent.wantsPGOnly())
		{
			childSetValue("incmature", FALSE);
			childSetValue("incadult", FALSE);
			childHide("incmature");
			childHide("incadult");
			childSetValue("incpg", TRUE);
			childDisable("incpg");
		}		

		if (!gAgent.canAccessMature())
		{
			childSetValue("incmature", FALSE);
			childDisable("incmature");
		}

		if (!gAgent.canAccessAdult())
		{
			childSetValue("incadult", FALSE);
			childDisable("incadult");
		}

		BOOL new_search = childGetValue("new_search").asBoolean();
		childSetVisible("Category", new_search);
		childSetVisible("OldCategory", !new_search);
		childSetVisible("incpg", !new_search);
		childSetVisible("incmature", !new_search);
		childSetVisible("incadult", !new_search);
	}

	mWebBrowser = getChild<LLMediaCtrl>(mBrowserName);
	if (mWebBrowser)
	{
		mWebBrowser->addObserver(this);
		
		// need to handle secondlife:///app/ URLs for direct teleports
		mWebBrowser->setTrusted(true);

		// redirect 404 pages from S3 somewhere else
		mWebBrowser->set404RedirectUrl(getString("redirect_404_url"));

		search("");
	}

	return TRUE;
}

LLPanelDirFind::~LLPanelDirFind()
{
}

// virtual
void LLPanelDirFind::draw()
{
	// enable/disable buttons depending on state
	if (mWebBrowser)
	{
		childSetEnabled("back_btn", mWebBrowser->canNavigateBack());
		childSetEnabled("forward_btn", mWebBrowser->canNavigateForward());
	}

	// showcase doesn't have those buttons/UI elements
	if (hasChild("incmature") && !childGetValue("new_search").asBoolean())
	{
		updateMaturityCheckbox();
	}

	LLPanelDirBrowser::draw();
}

// When we show any browser-based view, we want to hide all
// the right-side XUI detail panels.
// virtual
void LLPanelDirFind::onVisibilityChange(BOOL new_visibility)
{
	if (new_visibility)
	{
		mFloaterDirectory->hideAllDetailPanels();
	}
	LLPanel::onVisibilityChange(new_visibility);
}

// virtual
void LLPanelDirFindAll::reshape(S32 width, S32 height, BOOL called_from_parent = TRUE)
{
	if (mWebBrowser)
	{
		mWebBrowser->navigateTo(mWebBrowser->getCurrentNavUrl());
	}
	LLUICtrl::reshape(width, height, called_from_parent);
}

void LLPanelDirFindAll::search(const std::string& search_text)
{
	LLStringUtil::format_map_t subs;
	std::string url;
	std::string category;
	std::string maturity;
	BOOL inc_pg = childGetValue("incpg").asBoolean();
	BOOL inc_mature = childGetValue("incmature").asBoolean();
	BOOL inc_adult = childGetValue("incadult").asBoolean();
	if (!inc_pg  && !inc_mature && !inc_adult)
	{
		LLNotifications::instance().add("NoContentToSearch");
		return;
	}

	if (childGetValue("new_search").asBoolean())
	{
		url = gSavedSettings.getString("SearchURL");

		category = childGetValue("Category").asString();

		// Alas, the new search does not seem to allow selecting only one or two
		// levels of maturity among three...
		if (gAgent.prefersAdult())
		{
			maturity = "42";  // PG, Mature, Adult
		}
		else if (gAgent.prefersMature())
		{
			maturity = "21";  // PG, Mature
		}
		else
		{
			maturity = "13";  // PG
		}

		// Add the permissions token that login.cgi gave us
		subs["[AUTH_TOKEN]"] = LLPanelDirFind::getSearchToken();

		// add the user's god status
		subs["[GODLIKE]"] = gAgent.isGodlike() ? "1" : "0";
	}
	else
	{
		url = gSavedSettings.getString("OldSearchURL");

		category = childGetValue("OldCategory").asString();

		maturity = llformat("%d", (inc_pg ? SEARCH_PG : SEARCH_NONE) |
								  (inc_mature ? SEARCH_MATURE : SEARCH_NONE) |
								  (inc_adult ? SEARCH_ADULT : SEARCH_NONE));

		// Set the flag for the teen grid
		subs["[TEEN]"] = gAgent.isTeen() ? "y" : "n";
	}

	subs["[CATEGORY]"] = category;
	subs["[MATURITY]"] = maturity;
	subs["[QUERY]"] = LLURI::escape(search_text);

	// Expand all of the substitutions
	// (also adds things like [LANGUAGE], [VERSION], [OS], etc.)
	url = LLWeb::expandURLSubstitutions(url, subs);

//MK
	if (!gRRenabled || !gAgent.mRRInterface.mContainsShowloc)
	{
		llinfos << "search url " << url << llendl;
	}
//mk

	if (mWebBrowser)
	{
		mWebBrowser->navigateTo(url);
	}

	childSetText("search_editor", search_text);
}

void LLPanelDirFind::focus()
{
	childSetFocus("search_editor");
}

// static
void LLPanelDirFind::onClickBack(void* data)
{
	LLPanelDirFind* self = (LLPanelDirFind*)data;
	if (self && self->mWebBrowser)
	{
		self->mWebBrowser->navigateBack();
	}
}

// static
void LLPanelDirFind::onClickHelp(void* data)
{
	LLNotifications::instance().add("ClickSearchHelpAll");
}

// static
void LLPanelDirFind::onClickForward(void* data)
{
	LLPanelDirFind* self = (LLPanelDirFind*)data;
	if (self && self->mWebBrowser)
	{
		self->mWebBrowser->navigateForward();
	}
}

// static
void LLPanelDirFind::onCommitSearch(LLUICtrl*, void* data)
{
	onClickSearch(data);
}

// static
void LLPanelDirFind::onClickSearch(void* data)
{
	LLPanelDirFind* self = (LLPanelDirFind*)data;
	if (self)
	{
		self->search(self->childGetText("search_editor"));
		LLFloaterDirectory::sNewSearchCount++;
	}
}

// static
void LLPanelDirFind::onCommitNewSearch(LLUICtrl* ctrl, void* data)
{
	LLPanelDirFind* self = (LLPanelDirFind*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (self && ctrl)
	{
		bool new_search = check->get();
		self->childSetVisible("Category", new_search);
		self->childSetVisible("OldCategory", !new_search);
		self->childSetVisible("incpg", !new_search);
		self->childSetVisible("incmature", !new_search);
		self->childSetVisible("incadult", !new_search);
		self->search(self->childGetText("search_editor"));
	}
}

void LLPanelDirFind::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	switch(event)
	{
		case MEDIA_EVENT_NAVIGATE_BEGIN:
			childSetText("status_text", getString("loading_text"));
		break;
		
		case MEDIA_EVENT_NAVIGATE_COMPLETE:
			childSetText("status_text", getString("done_text"));
		break;
		
		default:
			// Having a default case makes the compiler happy.
		break;
	}
}

//---------------------------------------------------------------------------
// LLPanelDirFindAllInterface
//---------------------------------------------------------------------------

// static
LLPanelDirFindAll* LLPanelDirFindAllInterface::create(LLFloaterDirectory* floater)
{
	return new LLPanelDirFindAll("find_all_panel", floater);
}

// static
void LLPanelDirFindAllInterface::search(LLPanelDirFindAll* panel,
										const std::string& search_text)
{
	panel->search(search_text);
}

// static
void LLPanelDirFindAllInterface::focus(LLPanelDirFindAll* panel)
{
	panel->focus();
}

//---------------------------------------------------------------------------
// LLPanelDirFindAllOld - deprecated if new Google search works out. JC
//---------------------------------------------------------------------------

LLPanelDirFindAllOld::LLPanelDirFindAllOld(const std::string& name, LLFloaterDirectory* floater)
	:	LLPanelDirBrowser(name, floater)
{
	mMinSearchChars = 3;
}

BOOL LLPanelDirFindAllOld::postBuild()
{
	LLPanelDirBrowser::postBuild();

	childSetKeystrokeCallback("name", &LLPanelDirBrowser::onKeystrokeName, this);

	childSetAction("Search", onClickSearch, this);
	childDisable("Search");
	setDefaultBtn("Search");

	return TRUE;
}

LLPanelDirFindAllOld::~LLPanelDirFindAllOld()
{
	// Children all cleaned up by default view destructor.
}

// virtual
void LLPanelDirFindAllOld::draw()
{
	updateMaturityCheckbox();
	LLPanelDirBrowser::draw();
}

// static
void LLPanelDirFindAllOld::onCommitScope(LLUICtrl* ctrl, void* data)
{
	LLPanelDirFindAllOld* self = (LLPanelDirFindAllOld*)data;
	self->setFocus(TRUE);
}

// static
void LLPanelDirFindAllOld::onClickSearch(void *userdata)
{
	LLPanelDirFindAllOld *self = (LLPanelDirFindAllOld *)userdata;

	if (self->childGetValue("name").asString().length() < self->mMinSearchChars)
	{
		return;
	};

	BOOL inc_pg = self->childGetValue("incpg").asBoolean();
	BOOL inc_mature = self->childGetValue("incmature").asBoolean();
	BOOL inc_adult = self->childGetValue("incadult").asBoolean();
	if (!(inc_pg || inc_mature || inc_adult))
	{
		LLNotifications::instance().add("NoContentToSearch");
		return;
	}

	self->setupNewSearch();

	// Figure out scope
	U32 scope = 0x0;
	scope |= DFQ_PEOPLE;	// people (not just online = 0x01 | 0x02)
	// places handled below
	scope |= DFQ_EVENTS;	// events
	scope |= DFQ_GROUPS;	// groups
	if (inc_pg)
	{
		scope |= DFQ_INC_PG;
	}
	if (inc_mature)
	{
		scope |= DFQ_INC_MATURE;
	}
	if (inc_adult)
	{
		scope |= DFQ_INC_ADULT;
	}

	// send the message
	LLMessageSystem *msg = gMessageSystem;
	S32 start_row = 0;
	sendDirFindQuery(msg, self->mSearchID, self->childGetValue("name").asString(), scope, start_row);

	// Also look up classified ads. JC 12/2005
	BOOL filter_auto_renew = FALSE;
	U32 classified_flags = pack_classified_flags_request(filter_auto_renew, inc_pg, inc_mature, inc_adult);
	msg->newMessage("DirClassifiedQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", self->mSearchID);
	msg->addString("QueryText", self->childGetValue("name").asString());
	msg->addU32("QueryFlags", classified_flags);
	msg->addU32("Category", 0);	// all categories
	msg->addS32("QueryStart", 0);
	gAgent.sendReliableMessage();

	// Need to use separate find places query because places are
	// sent using the more compact DirPlacesReply message.
	U32 query_flags = DFQ_DWELL_SORT;
	if (inc_pg)
	{
		query_flags |= DFQ_INC_PG;
	}
	if (inc_mature)
	{
		query_flags |= DFQ_INC_MATURE;
	}
	if (inc_adult)
	{
		query_flags |= DFQ_INC_ADULT;
	}
	msg->newMessage("DirPlacesQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("QueryData");
	msg->addUUID("QueryID", self->mSearchID );
	msg->addString("QueryText", self->childGetValue("name").asString());
	msg->addU32("QueryFlags", query_flags );
	msg->addS32("QueryStart", 0 ); // Always get the first 100 when using find ALL
	msg->addS8("Category", LLParcel::C_ANY);
	msg->addString("SimName", NULL);
	gAgent.sendReliableMessage();
}
