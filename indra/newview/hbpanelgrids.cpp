/** 
 * @file hbpanelgrids.cpp
 * @author Henri Beauchamp
 * @brief Grid parameters configuration panel
 *
 * $LicenseInfo:firstyear=2011&license=viewergpl$
 * 
 * Copyright (c) 2011, Henri Beauchamp.
 * Note: XML parser code borrowed from Hippo Viewer (c) unknown author
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

#include "boost/regex.hpp"

#include "hbpanelgrids.h"

#include "llcheckboxctrl.h"
#include "lldir.h"
#include "llhttpclient.h"
#include "lllineeditor.h"
#include "llnotifications.h"
#include "llradiogroup.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "llstring.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llstartup.h"
#include "llviewernetwork.h"

class HBPanelGridsImpl : public LLPanel
{
public:
	HBPanelGridsImpl();
	/*virtual*/ ~HBPanelGridsImpl();

	/*virtual*/ void draw();

	void apply();
	void cancel();

	enum XmlState 
	{
		XML_VOID,
		XML_GRIDNAME,
		XML_GRIDNICK,
		XML_LOGINURI,
		XML_HELPERURI,
		XML_LOGINPAGE,
		XML_WEBSITE,
		XML_SUPPORT,
		XML_ACCOUNT,
		XML_PASSWORD,
	};
	XmlState mXmlState;

	void getParams();
	void clearParams(bool clear_name = true);
	void updateGridParameters(std::string& result);
	void copyParams();
	void saveParams();
	void deleteGrid();
	void addGrid();

	void setDirty()							{ mIsDirty = true; }
	void setQueryActive(bool active)		{ mQueryActive = active; mIsDirty = true; }

	static HBPanelGridsImpl* getInstance()	{ return sInstance; }

	static void onXmlElementStart(void* data, const XML_Char* name, const XML_Char** atts);
	static void onXmlElementEnd(void* data, const XML_Char* name);
	static void onXmlCharacterData(void* data, const XML_Char* s, int len);

	static void onClickGetParams(void *data);
	static void onClickClearParams(void *data);
	static void onClickUpdateGrid(void *data);
	static void onClickDeleteGrid(void *data);
	static void onClickAddGrid(void *data);

	static void onURIEditorKeystroke(LLLineEditor* caller, void* data);
	static void onNameEditorKeystroke(LLLineEditor* caller, void* data);
	static void onCommitCheckBoxLoginURI(LLUICtrl* ctrl, void* data);
	static void onCommitRadioPreferredName(LLUICtrl* ctrl, void* data);
	static void onSelectGrid(LLUICtrl* ctrl, void* data);

private:
	bool mIsDirty;
	bool mIsDirtyList;
	bool mQueryActive;
	bool mListChanged;

	std::string mGridDomain;
	std::string mGridCustomName;
	std::string mGridName;
	std::string mGridNick;
	std::string mEnteredLoginURI;
	std::string mLoginURI;
	std::string mHelperURI;
	std::string mLoginPage;
	std::string mWebsiteURL;
	std::string mSupportURL;
	std::string mAccountURL;
	std::string mPasswordURL;

	LLScrollListCtrl* mGridsScrollList;

	LLSD mSavedGridsList;
	static LLSD sGridsList;

	static HBPanelGridsImpl* sInstance;
};

class GridParametersReceived : public LLHTTPClient::Responder
{
	LOG_CLASS(GridParametersReceived);

public:
    GridParametersReceived(const std::string& uri)
	:	mURI(uri)
	{
		if (HBPanelGridsImpl::getInstance())
		{
			HBPanelGridsImpl::getInstance()->setQueryActive(true);
		}
	}

	void completedRaw(U32 status, const std::string& reason,
					  const LLChannelDescriptors& channels,
					  const LLIOPipe::buffer_ptr_t& buffer)
    {
		if (HBPanelGridsImpl::getInstance())
		{
			HBPanelGridsImpl::getInstance()->setQueryActive(false);

			if (status < 200 || status > 400)
			{
				LLSD args;
				args["URI"] = mURI;
				args["STATUS"] = llformat("%d", status);
				args["REASON"] = reason;
				LLNotifications::instance().add("GetGridParametersFailure", args);
				return;
			}

			U8* data = NULL;
			S32 data_size = buffer->countAfter(channels.in(), NULL);
			if (data_size > 0)
			{
				data = new U8[data_size];
				buffer->readAfter(channels.in(), NULL, data, data_size);
			}
			std::string result((char*) data, data_size);

			LL_DEBUGS("GetGridParameters") << "\n" << result << LL_ENDL;

			HBPanelGridsImpl::getInstance()->updateGridParameters(result);
		}
	}

private:
	std::string mURI;
};

HBPanelGridsImpl* HBPanelGridsImpl::sInstance = NULL;
LLSD HBPanelGridsImpl::sGridsList;

HBPanelGridsImpl::HBPanelGridsImpl()
:	LLPanel(std::string("Grids parameters")),
	mIsDirty(true),
	mIsDirtyList(true),
	mQueryActive(false),
	mListChanged(false)
{
	LLUICtrlFactory::getInstance()->buildPanel(this,
											   "panel_preferences_grids.xml");
	sInstance = this;

	if (sGridsList.beginMap() == sGridsList.endMap())
	{
		LLSD grids = LLViewerLogin::getInstance()->getGridsList();
		for (LLSD::map_iterator grid_itr = grids.beginMap();
			 grid_itr != grids.endMap(); grid_itr++)
		{
			LLSD::String key_name = grid_itr->first;
			LLSD grid_array = grid_itr->second;
			if (key_name == "grids" && grid_array.isArray())
			{
				for (S32 i = 0; i < grid_array.size(); i++)
				{
					LLSD gmap = grid_array[i];
					if (gmap.has("can_edit") &&
						gmap["can_edit"].asString() != "never")
					{
						sGridsList["grids"].append(gmap);
						LL_DEBUGS("GetGridParameters") << "Retained grid: "
													   << gmap.get("name")
													   << LL_ENDL;
					}
					else
					{
						LL_DEBUGS("GetGridParameters") << "Rejected non-editable grid: "
													   << gmap.get("name")
													   << LL_ENDL;
					}
				}
			}
		}
	}

	mSavedGridsList = sGridsList;

	mGridsScrollList = getChild<LLScrollListCtrl>("grid_selector");
	mGridsScrollList->setCommitOnSelectionChange(TRUE);
	mGridsScrollList->setCommitCallback(onSelectGrid);
	mGridsScrollList->setCallbackUserData(this);

	childSetAction("update_button", onClickUpdateGrid, this);
	childSetAction("delete_button", onClickDeleteGrid, this);
	childSetAction("add_button", onClickAddGrid, this);

	childSetAction("get_param_button", onClickGetParams, this);
	childSetAction("clear_param_button", onClickClearParams, this);

	LLLineEditor* editor = getChild<LLLineEditor>("login_uri_editor");
	editor->setKeystrokeCallback(onURIEditorKeystroke);
	editor->setCallbackUserData(this);
	editor = getChild<LLLineEditor>("grid_name_editor");
	editor->setKeystrokeCallback(onNameEditorKeystroke);
	editor->setCallbackUserData(this);

	childSetCommitCallback("retrieved_loginuri_check", onCommitCheckBoxLoginURI, this);
	childSetCommitCallback("prefer_nickname_radio", onCommitRadioPreferredName, this);
}

HBPanelGridsImpl::~HBPanelGridsImpl()
{
	sInstance = NULL;
}

//virtual
void HBPanelGridsImpl::draw()
{
	if (mIsDirty)
	{
		// Grids list
		if (mIsDirtyList)
		{
			S32 old_count = mGridsScrollList->getItemCount();
			S32 scrollpos = mGridsScrollList->getScrollPos();
			S32 selected = mGridsScrollList->getFirstSelectedIndex();
			mGridsScrollList->deleteAllItems();

			for (LLSD::map_iterator grid_itr = sGridsList.beginMap();
				 grid_itr != sGridsList.endMap(); grid_itr++)
			{
				LLSD::String key_name = grid_itr->first;
				LLSD grid_array = grid_itr->second;
				if (key_name == "grids" && grid_array.isArray())
				{
					for (S32 i = 0; i < grid_array.size(); i++)
					{
						LLSD gmap = grid_array[i];
						LLSD element;
						std::string style = "NORMAL";
						std::string grid_id = gmap["name"].asString();
						if (gmap.has("can_edit") &&
							gmap["can_edit"].asString() == "false")
						{
							style = "BOLD";
							grid_id = "@@|" + grid_id;
						}
						element["id"] = grid_id;
						element["columns"][0]["value"] = gmap["label"].asString();
						element["columns"][0]["type"] = "text";
						element["columns"][0]["font"] = "SANSSERIF";
						element["columns"][0]["font-style"] = style;
						mGridsScrollList->addElement(element, ADD_BOTTOM);
					}
				}
			}

			S32 new_count = mGridsScrollList->getItemCount();
			if (old_count > new_count)
			{
				// A grid was just deleted
				if (selected > 0)
				{
					scrollpos = --selected;
				}
				else
				{
					scrollpos = selected = 0;
				}
			}
			else if (old_count < new_count &&
					 old_count > 0) // count == 0 when first initializing the list
			{
				// An item was just added. Let's select it and scroll to it.
				selected = scrollpos = new_count - 1;
			}
			mGridsScrollList->setScrollPos(scrollpos);
			if (selected >= 0)
			{
				mGridsScrollList->selectNthItem(selected);
			}
			mIsDirtyList = false;
		}
		mGridsScrollList->setEnabled(!mQueryActive);

		// Enable/disable the various UI elements as appropriate

		bool uri_ok = !childGetValue("login_uri_editor").asString().empty();
		bool name_ok = !childGetValue("grid_name_editor").asString().empty();
		bool grid_ok = !mIsDirtyList && mGridsScrollList->getFirstSelected() != NULL;
		childSetEnabled("update_button", !mQueryActive && uri_ok && name_ok && grid_ok);
		if (grid_ok)
		{
			grid_ok = mGridsScrollList->getValue().asString().find("@@|") == std::string::npos;
		}
		childSetEnabled("delete_button", !mQueryActive && grid_ok);
		childSetEnabled("add_button", !mQueryActive && uri_ok && name_ok);
		childSetEnabled("get_param_button", !mQueryActive && uri_ok);
		childSetEnabled("clear_param_button", !mQueryActive);

		childSetVisible("retreiving", mQueryActive);
		if (mQueryActive)
		{
			childSetVisible("domain", false);
		}
		else if (!mGridDomain.empty())
		{
			LLTextBox* domain_text = getChild<LLTextBox>("domain");
			domain_text->setTextArg("[DOMAIN]", mGridDomain);
			domain_text->setVisible(true);
		}
		else
		{
			childSetVisible("domain", false);
		}

		// Updates done
		mIsDirty = false;
	}
	LLPanel::draw();
}

void HBPanelGridsImpl::getParams()
{
	mEnteredLoginURI = childGetValue("login_uri_editor").asString();
	if (mEnteredLoginURI.empty())
	{
		LLNotifications::instance().add("MandatoryLoginUri");
		return;
	}
	std::string url = mEnteredLoginURI;
	if (mEnteredLoginURI.compare(url.length() - 1, 1, "/") != 0)
	{
		url += '/';
	}
	url += "get_grid_info";
	clearParams(false);
	LLHTTPClient::get(url, new GridParametersReceived(mEnteredLoginURI));
}

void HBPanelGridsImpl::clearParams(bool clear_name)
{
	if (clear_name)
	{
		childSetValue("grid_name_editor", "");
	}
	childSetValue("helper_uri_editor", "");
	childSetValue("login_page_editor", "");
	childSetValue("website_editor", "");
	childSetValue("new_account_editor", "");
	childSetValue("support_editor", "");
	childSetValue("forgotten_password_editor", "");

	mGridDomain = "";
	mIsDirty = true;
}

//static
void HBPanelGridsImpl::onXmlElementStart(void* data, const XML_Char* name, const XML_Char** atts)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (strcasecmp(name, "gridnick") == 0)
	{
		self->mXmlState = XML_GRIDNICK;
	}
	else if (strcasecmp(name, "gridname") == 0)
	{
		self->mXmlState = XML_GRIDNAME;
	}
	else if (strcasecmp(name, "loginuri") == 0 ||
			 strcasecmp(name, "login") == 0)
	{
		self->mXmlState = XML_LOGINURI;
	}
	else if (strcasecmp(name, "helperuri") == 0 ||
			 strcasecmp(name, "economy") == 0)
	{
		self->mXmlState = XML_HELPERURI;
	}
	else if (strcasecmp(name, "loginpage") == 0 ||
			 strcasecmp(name, "welcome") == 0)
	{
		self->mXmlState = XML_LOGINPAGE;
	}
	else if (strcasecmp(name, "website") == 0 ||
			 strcasecmp(name, "about") == 0)
	{
		self->mXmlState = XML_WEBSITE;
	}
	else if (strcasecmp(name, "support") == 0 ||
			 strcasecmp(name, "help") == 0)
	{
		self->mXmlState = XML_SUPPORT;
	}
	else if (strcasecmp(name, "account") == 0 ||
			 strcasecmp(name, "register") == 0)
	{
		self->mXmlState = XML_ACCOUNT;
	}
	else if (strcasecmp(name, "password") == 0)
	{
		self->mXmlState = XML_PASSWORD;
	}
}

//static
void HBPanelGridsImpl::onXmlElementEnd(void* data, const XML_Char* name)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self)
	{
		self->mXmlState = XML_VOID;
	}
}

//static
void HBPanelGridsImpl::onXmlCharacterData(void* data, const XML_Char* s, int len)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	switch (self->mXmlState) 
	{
		case XML_GRIDNAME:	self->mGridName.assign(s, len);		break;
		case XML_GRIDNICK:	self->mGridNick.assign(s, len);		break;
		case XML_LOGINURI:	self->mLoginURI.assign(s, len);		break;
		case XML_HELPERURI:	self->mHelperURI.assign(s, len);	break;
		case XML_LOGINPAGE:	self->mLoginPage.assign(s, len);	break;
		case XML_WEBSITE:	self->mWebsiteURL.assign(s, len);	break;
		case XML_SUPPORT:	self->mSupportURL.assign(s, len);	break;
		case XML_ACCOUNT:	self->mAccountURL.assign(s, len);  break;
		case XML_PASSWORD:	self->mPasswordURL.assign(s, len);  break;
		case XML_VOID:
		default:
			break;
	}
}

void HBPanelGridsImpl::updateGridParameters(std::string& result)
{
	mGridName.clear();
	mGridNick.clear();
	mLoginURI.clear();
	mHelperURI.clear();
	mLoginPage.clear();
	mWebsiteURL.clear();
	mSupportURL.clear();
	mAccountURL.clear();
	mPasswordURL.clear();

	bool success = true;
	XML_Parser parser = XML_ParserCreate(0);
	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, onXmlElementStart, onXmlElementEnd);
	XML_SetCharacterDataHandler(parser, onXmlCharacterData);
	mXmlState = XML_VOID;
	if (!XML_Parse(parser, result.data(), result.size(), TRUE)) 
	{
		llwarns << "XML Parse Error: " << XML_ErrorString(XML_GetErrorCode(parser)) << llendl;
		success = false;
	}
	XML_ParserFree(parser);

	if (mGridName.empty() && !mGridNick.empty())
	{
		mGridName = mGridNick;
	}
	if (mGridCustomName.empty())
	{
		mGridCustomName = mGridName;
	}
	if (mGridName.empty())
	{
		mGridName = mGridCustomName;
	}
	if (mGridNick.empty())
	{
		mGridNick = mGridName;
	}
	S32 choice = childGetValue("prefer_nickname_radio").asInteger();
	std::string name;
	switch (choice)
	{
		case 1:
			name = mGridName;
			break;
		case 2:
			name = mGridNick;
			break;
		default:
			name = mGridCustomName;
	}
	childSetValue("grid_name_editor", name);

	if (mLoginURI.empty())
	{
		mLoginURI = mEnteredLoginURI;
	}
	std::string login_uri =
		childGetValue("retrieved_loginuri_check").asBoolean() ? mLoginURI
															  : mEnteredLoginURI;
	childSetValue("login_uri_editor", login_uri);

	childSetValue("helper_uri_editor", mHelperURI);
	childSetValue("login_page_editor", mLoginPage);
	childSetValue("website_editor", mWebsiteURL);
	childSetValue("new_account_editor", mAccountURL);
	childSetValue("support_editor", mSupportURL);
	childSetValue("forgotten_password_editor", mPasswordURL);

	mIsDirty = true;
}

void HBPanelGridsImpl::copyParams()
{
	mGridDomain = mGridsScrollList->getValue().asString();
	if (mGridDomain.empty()) return;
	if (mGridDomain.find("@@|") == 0)
	{
		mGridDomain = mGridDomain.substr(3);
	}

	EGridInfo i = LLViewerLogin::getInstance()->gridIndexInList(sGridsList,
																mGridDomain);
	if (i != -1)
	{
		mGridCustomName = mGridName = mGridNick = sGridsList["grids"][i].get("label").asString();
		childSetValue("grid_name_editor", mGridCustomName);

		mLoginURI = mEnteredLoginURI = sGridsList["grids"][i].get("login_uri").asString();
		childSetValue("login_uri_editor", mLoginURI);

		mHelperURI = sGridsList["grids"][i].get("helper_uri").asString();
		childSetValue("helper_uri_editor", mHelperURI);

		mLoginPage = sGridsList["grids"][i].get("login_page").asString();
		childSetValue("login_page_editor", mLoginPage);

		mWebsiteURL = sGridsList["grids"][i].get("website_url").asString();
		childSetValue("website_editor", mWebsiteURL);

		mSupportURL = sGridsList["grids"][i].get("support_url").asString();
		childSetValue("support_editor", mSupportURL);

		mAccountURL = sGridsList["grids"][i].get("register_url").asString();
		childSetValue("new_account_editor", mAccountURL);

		mPasswordURL = sGridsList["grids"][i].get("password_url").asString();
		childSetValue("forgotten_password_editor", mPasswordURL);

		mIsDirty = true;
	}
}

void HBPanelGridsImpl::saveParams()
{
	mGridDomain = mGridsScrollList->getValue().asString();
	if (mGridDomain.empty()) return;
	if (mGridDomain.find("@@|") == 0)
	{
		mGridDomain = mGridDomain.substr(3);
	}

	EGridInfo i = LLViewerLogin::getInstance()->gridIndexInList(sGridsList,
																mGridDomain);
	if (i != -1)
	{
		std::string name = childGetValue("grid_name_editor").asString();
		LLStringUtil::trim(name);
		if (name.empty())
		{
			LLNotifications::instance().add("MandatoryGridName");
			return;
		}
		std::string uri = childGetValue("login_uri_editor").asString();
		LLStringUtil::trim(uri);
		if (uri.empty())
		{
			LLNotifications::instance().add("MandatoryLoginUri");
			return;
		}
		sGridsList["grids"][i]["label"] = mGridCustomName = mGridName = mGridNick = name;
		sGridsList["grids"][i]["login_uri"] = mLoginURI = mEnteredLoginURI = uri;

		mHelperURI = childGetValue("helper_uri_editor").asString();
		LLStringUtil::trim(mHelperURI);
		sGridsList["grids"][i]["helper_uri"] = mHelperURI;

		mLoginPage = childGetValue("login_page_editor").asString();
		LLStringUtil::trim(mLoginPage);
		sGridsList["grids"][i]["login_page"] = mLoginPage;

		mWebsiteURL = childGetValue("website_editor").asString();
		LLStringUtil::trim(mWebsiteURL);
		sGridsList["grids"][i]["website_url"] = mWebsiteURL;

		mSupportURL = childGetValue("support_editor").asString();
		LLStringUtil::trim(mSupportURL);
		sGridsList["grids"][i]["support_url"] = mSupportURL;

		mAccountURL = childGetValue("new_account_editor").asString();
		LLStringUtil::trim(mAccountURL);
		sGridsList["grids"][i]["register_url"] = mAccountURL;

		mPasswordURL = childGetValue("forgotten_password_editor").asString();
		LLStringUtil::trim(mPasswordURL);
		sGridsList["grids"][i]["password_url"] = mPasswordURL;

		sGridsList["grids"][i]["can_edit"] = "true";

		mIsDirty = mIsDirtyList = mListChanged = true;
	}
}

void HBPanelGridsImpl::deleteGrid()
{
	mGridDomain = mGridsScrollList->getValue().asString();
	if (mGridDomain.empty()) return;
	if (mGridDomain.find("@@|") == 0)	// Should never happen
	{
		mGridDomain = mGridDomain.substr(3);
		return;
	}

	// First, check to see if we have that grid listed in the original
	// grids list
	LLViewerLogin* vl = LLViewerLogin::getInstance();
	LLSD grids;
	vl->loadGridsLLSD(grids,
					  gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,
													 "grids.xml"));
	EGridInfo i = vl->gridIndexInList(grids, mGridDomain);
	if (i == -1)
	{
		// No such grid: just delete it
		i = vl->gridIndexInList(sGridsList, mGridDomain);
		sGridsList["grids"].erase(i);
		mGridDomain.clear();
	}
	else
	{
		// Copy back the grid parameters
		EGridInfo j = vl->gridIndexInList(sGridsList, mGridDomain);
		if (j != -1)
		{
			sGridsList["grids"][j]["label"]	= mGridCustomName = mGridName = mGridNick = grids["grids"][i].get("label").asString();
			sGridsList["grids"][j]["login_uri"] = mLoginURI = mEnteredLoginURI = grids["grids"][i].get("login_uri").asString();
			sGridsList["grids"][j]["helper_uri"] = mHelperURI = grids["grids"][i].get("helper_uri").asString();
			sGridsList["grids"][j]["login_page"] = mLoginPage = grids["grids"][i].get("login_page").asString();
			sGridsList["grids"][j]["website_url"] = mWebsiteURL = grids["grids"][i].get("website_url").asString();
			sGridsList["grids"][j]["support_url"] = mSupportURL = grids["grids"][i].get("support_url").asString();
			sGridsList["grids"][j]["register_url"] = mAccountURL = grids["grids"][i].get("register_url").asString();
			sGridsList["grids"][j]["password_url"] = mPasswordURL = grids["grids"][i].get("password_url").asString();
			sGridsList["grids"][j]["can_edit"] = "false";
		}
	}

	mIsDirty = mIsDirtyList = mListChanged = true;
}

// Helper functions for addGrid()

bool is_ip_address(const std::string& domain)
{
	if (domain.empty()) return true; // Pretend an empty string is an IP (saves tests).
	boost::regex ipv4_format("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
	boost::regex ipv6_format("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
	return boost::regex_match(domain, ipv4_format) || boost::regex_match(domain, ipv6_format);
}

std::string sanitize(std::string str)
{
	LLStringUtil::trim(str);
	std::string temp;
	size_t len = str.size();
	for (size_t i = 0; i < len; i++)
	{
		char c = str[i];
		if (c == '_' || c == '-' || isalnum(c))
		{
			temp += tolower(c);
		}
		else if (c == ' ')
		{
			temp += '.';
		}
	}
	return temp;
}

void grid_exists_error(const std::string& name)
{
	LLSD args;
	args["NAME"] = name;
	LLNotifications::instance().add("ExistingGridName", args);
	return;
}

void HBPanelGridsImpl::addGrid()
{
	std::string uri = childGetValue("login_uri_editor").asString();
	LLStringUtil::trim(uri);
	if (uri.empty())
	{
		LLNotifications::instance().add("MandatoryLoginUri");
		return;
	}

	std::string name = childGetValue("grid_name_editor").asString();
	LLStringUtil::trim(name);
	if (name.empty())
	{
		LLNotifications::instance().add("MandatoryGridName");
		return;
	}
	mLoginURI = mEnteredLoginURI = uri;
	mGridCustomName = mGridName = mGridNick = name;
	mHelperURI = childGetValue("helper_uri_editor").asString();
	LLStringUtil::trim(mHelperURI);
	mLoginPage = childGetValue("login_page_editor").asString();
	LLStringUtil::trim(mLoginPage);
	mWebsiteURL = childGetValue("website_editor").asString();
	LLStringUtil::trim(mWebsiteURL);
	mAccountURL = childGetValue("new_account_editor").asString();
	LLStringUtil::trim(mAccountURL);
	mSupportURL = childGetValue("support_editor").asString();
	LLStringUtil::trim(mSupportURL);
	mPasswordURL = childGetValue("forgotten_password_editor").asString();
	LLStringUtil::trim(mPasswordURL);

	// Create an unique "domain" name that will be used as the key of this
	// grid in the grids map: this name can also be used as a grid name after
	// the --grid option in the command line of the viewer.
	mGridDomain = LLViewerLogin::getDomain(mLoginURI);
	if (is_ip_address(mGridDomain))
	{
		mGridDomain = LLViewerLogin::getDomain(mHelperURI);
		if (is_ip_address(mGridDomain))
		{
			mGridDomain = LLViewerLogin::getDomain(mLoginPage);
			if (is_ip_address(mGridDomain))
			{
				mGridDomain = LLViewerLogin::getDomain(mAccountURL);
				if (is_ip_address(mGridDomain))
				{
					mGridDomain = LLViewerLogin::getDomain(mSupportURL);
					if (is_ip_address(mGridDomain))
					{
						mGridDomain = LLViewerLogin::getDomain(mPasswordURL);
						if (is_ip_address(mGridDomain))
						{
							mGridDomain = sanitize(mGridName);
							if (is_ip_address(mGridDomain))
							{
								LLNotifications::instance().add("AddGridFailure");
								return;
							}
							mGridDomain += ".net";
						}
					}
				}
			}
		}
	}
	LLStringUtil::toLower(mGridDomain);

	// Remove some meaningless common prefixes to try and get a cleaner
	// domain name
	if (mGridDomain.find("grid.") == 0 && mGridDomain.length() > 8)
	{
		mGridDomain = mGridDomain.substr(5);
	}
	else if (mGridDomain.find("login.") == 0 && mGridDomain.length() > 9)
	{
		mGridDomain = mGridDomain.substr(6);
	}
	else if (mGridDomain.find("www.") == 0 && mGridDomain.length() > 7)
	{
		mGridDomain = mGridDomain.substr(4);
	}

	// Verify that we don't add a grid that already exists.

	if (mGridDomain == "agni.lindenlab.com" ||
		mGridDomain == "aditi.lindenlab.com")
	{
		grid_exists_error(mGridDomain);
		return;
	}

	std::string lc_name = mGridName;
	LLStringUtil::toLower(lc_name);
	if (lc_name == "secondlife" || lc_name == "secondlife beta" ||
		lc_name == "other" || lc_name == "none")
	{
		grid_exists_error(name);
		return;
	}

	EGridInfo i = LLViewerLogin::getInstance()->gridIndexInList(sGridsList,
																mGridDomain);
	if (i != -1)
	{
		grid_exists_error(mGridDomain);
		return;
	}

	i = LLViewerLogin::getInstance()->gridIndexInList(sGridsList, "", name);
	if (i != -1)
	{
		grid_exists_error(name);
		return;
	}

	// All OK: we can now add it !

	LLSD entry = sGridsList.emptyMap();
	entry.insert("name", mGridDomain);
	entry.insert("label", mGridName);
	entry.insert("login_uri", mLoginURI);
	entry.insert("helper_uri", mHelperURI);
	entry.insert("login_page", mLoginPage);
	entry.insert("website_url", mWebsiteURL);
	entry.insert("register_url", mAccountURL);
	entry.insert("support_url", mSupportURL);
	entry.insert("password_url", mPasswordURL);
	entry.insert("can_edit", "true");
	sGridsList["grids"].append(entry);

	mIsDirty = mIsDirtyList = mListChanged = true;
}

void HBPanelGridsImpl::apply()
{
	// Create a custom grids list out of listed editable grids
	LLSD grids;
	for (LLSD::map_iterator grid_itr = sGridsList.beginMap();
		 grid_itr != sGridsList.endMap(); grid_itr++)
	{
		LLSD::String key_name = grid_itr->first;
		LLSD grid_array = grid_itr->second;
		if (key_name == "grids" && grid_array.isArray())
		{
			for (S32 i = 0; i < grid_array.size(); i++)
			{
				LLSD gmap = grid_array[i];
				if (gmap.has("can_edit") &&
					gmap["can_edit"].asString() == "true")
				{
					grids["grids"].append(gmap);
				}
			}
		}
	}

	// Save the custom grids list
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
														  "grids_custom.xml");
	llofstream list_file(filename);
	LLSDSerialize::toPrettyXML(grids, list_file);
	list_file.close();
	llinfos << "Saved file: " << filename << llendl;

	if (mListChanged && LLStartUp::getStartupState() != STATE_STARTED)
	{
		LLViewerLogin::getInstance()->loadGridsList();
		LLStartUp::refreshLoginPanel();
	}

	// All changes saved
	mSavedGridsList.clear();
	mSavedGridsList = sGridsList;
	mListChanged = false;
}

void HBPanelGridsImpl::cancel()
{
	// Beware: cancel() is *also* called after apply() when pressing "OK" to
	//         close the Preferences floater.
	sGridsList.clear();
	sGridsList = mSavedGridsList;
	mIsDirty = mIsDirtyList = true;
}

//static
void HBPanelGridsImpl::onClickGetParams(void *data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self)
	{
		self->getParams();
	}
}

//static
void HBPanelGridsImpl::onClickClearParams(void *data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self)
	{
		self->clearParams();
	}
}

// static
void HBPanelGridsImpl::onURIEditorKeystroke(LLLineEditor* caller, void* data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self)
	{
		self->setDirty();
	}
}

//static
void HBPanelGridsImpl::onCommitCheckBoxLoginURI(LLUICtrl* ctrl, void* data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (self && check)
	{
		self->childSetValue("login_uri_editor", check->get() ? self->mLoginURI
															 : self->mEnteredLoginURI);
	}
}

// static
void HBPanelGridsImpl::onNameEditorKeystroke(LLLineEditor* caller, void* data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self && caller)
	{
		self->setDirty();
		S32 choice = self->childGetValue("prefer_nickname_radio").asInteger();
		if (choice != 0)
		{
			self->getChild<LLRadioGroup>("prefer_nickname_radio")->selectFirstItem();
		}
		self->mGridCustomName = caller->getValue().asString();
	}
}

//static
void HBPanelGridsImpl::onCommitRadioPreferredName(LLUICtrl* ctrl, void* data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	LLRadioGroup* radio = (LLRadioGroup*)ctrl;
	if (self && radio)
	{
		S32 choice = radio->getValue().asInteger();
		std::string name;
		switch (choice)
		{
			case 1:
				name = self->mGridName;
				break;
			case 2:
				name = self->mGridNick;
				break;
			default:
				name = self->mGridCustomName;
		}
		self->childSetValue("grid_name_editor", name);
	}
}

// static
void HBPanelGridsImpl::onSelectGrid(LLUICtrl* ctrl, void* data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self)
	{
		self->copyParams();
	}
}

//static
void HBPanelGridsImpl::onClickUpdateGrid(void *data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self)
	{
		self->saveParams();
	}
}

//static
void HBPanelGridsImpl::onClickDeleteGrid(void *data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self)
	{
		self->deleteGrid();
	}
}

//static
void HBPanelGridsImpl::onClickAddGrid(void *data)
{
	HBPanelGridsImpl* self = (HBPanelGridsImpl*)data;
	if (self)
	{
		self->addGrid();
	}
}

//---------------------------------------------------------------------------

HBPanelGrids::HBPanelGrids()
:	impl(* new HBPanelGridsImpl())
{
}

HBPanelGrids::~HBPanelGrids()
{
	delete &impl;
}

void HBPanelGrids::apply()
{
	impl.apply();
}

void HBPanelGrids::cancel()
{
	impl.cancel();
}

LLPanel* HBPanelGrids::getPanel()
{
	return &impl;
}
