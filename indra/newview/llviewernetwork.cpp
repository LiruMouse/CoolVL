/** 
 * @file llviewernetwork.cpp
 * @author James Cook, Richard Nelson
 * @brief Networking constants and globals for viewer.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llviewernetwork.h"

#include "llhost.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsecondlifeurls.h"
#include "llstring.h"

#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"

bool gIsInSecondLife = false;
bool gIsInSecondLifeProductionGrid = false;

unsigned char gMACAddress[MAC_ADDRESS_BYTES];		/* Flawfinder: ignore */

EGridInfo GRID_INFO_OTHER;

LLViewerLogin::LLViewerLogin()
:	mGridChoice(DEFAULT_GRID_CHOICE),
	mCurrentURI(0),
	mNameEditted(false)
{
	loadGridsList();
	parseCommandLineURIs();
}

void LLViewerLogin::loadGridsList()
{
	if (LLStartUp::getStartupState() == STATE_STARTED)
	{
		// Never change the grids list once started, else bad things will
		// happen because the grid choice is done on an index in the list...
		return;
	}

	mGridList.clear();

	LLSD array = mGridList.emptyArray();
	LLSD entry = mGridList.emptyMap();
	entry.insert("label", "None");
	entry.insert("name", "");
	entry.insert("login_uri", "");
	entry.insert("helper_uri", "");
	entry.insert("login_page", "");
	entry.insert("can_edit", "never");
	array.append(entry);

	// Add SecondLife servers (main and beta grid):

	std::string login_page;
	if (gSavedSettings.getBOOL("UseNewSLLoginPage"))
	{
		login_page = gSavedSettings.getString("NewSLLoginPage");
	}
	if (login_page.empty())
	{
		login_page = SL_LOGIN_PAGE_URL;
	}

	entry = mGridList.emptyMap();
	entry.insert("label", "SecondLife");
	entry.insert("name", "agni.lindenlab.com");
	entry.insert("login_uri", AGNI_LOGIN_URI);
	entry.insert("helper_uri", AGNI_HELPER_URI);
	entry.insert("support_url", SUPPORT_URL);
	entry.insert("register_url", CREATE_ACCOUNT_URL);
	entry.insert("password_url", FORGOTTEN_PASSWORD_URL);
	entry.insert("login_page", login_page);
	entry.insert("can_edit", "never");
	array.append(entry);

	entry = mGridList.emptyMap();
	entry.insert("label", "SecondLife Beta");
	entry.insert("name", "aditi.lindenlab.com");
	entry.insert("login_uri", ADITI_LOGIN_URI);
	entry.insert("helper_uri", ADITI_HELPER_URI);
	entry.insert("support_url", SUPPORT_URL);
	entry.insert("register_url", CREATE_ACCOUNT_URL);
	entry.insert("password_url", FORGOTTEN_PASSWORD_URL);
	entry.insert("login_page", login_page);
	entry.insert("can_edit", "never");
	array.append(entry);

	mGridList.insert("grids", array);

	mVerbose = true;
	// see if we have a grids_custom.xml file to append
	loadGridsLLSD(mGridList,
				  gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
												 "grids_custom.xml"),
				  true);
	// load the additional grids if available
	loadGridsLLSD(mGridList,
				  gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,
												 "grids.xml"));
	mVerbose = false;

	entry = mGridList.emptyMap();
	entry.insert("label", "Other");
	entry.insert("name", "");
	entry.insert("login_uri", "");
	entry.insert("helper_uri", "");
	entry.insert("can_edit", "never");
	mGridList["grids"].append(entry);

	GRID_INFO_OTHER = (EGridInfo)mGridList["grids"].size() - 1;
}

const EGridInfo LLViewerLogin::gridIndexInList(LLSD& grids,
											   std::string name,
											   std::string label)
{
	bool has_name = !name.empty();
	bool has_label = !label.empty();

	if (!has_name && !has_label) return -1;

	LLStringUtil::toLower(name);
	LLStringUtil::toLower(label);

	for (LLSD::map_iterator grid_itr = grids.beginMap();
		 grid_itr != grids.endMap(); grid_itr++)
	{
		LLSD::String key_name = grid_itr->first;
		LLSD grid_array = grid_itr->second;
		if (key_name == "grids" && grid_array.isArray())
		{
			std::string temp;
			for (S32 i = 0; i < grid_array.size(); i++)
			{
				if (has_name)
				{
					temp = grid_array[i]["name"].asString();
					LLStringUtil::toLower(temp);
					if (temp == name)
					{
						return i;
					}
				}
				if (has_label)
				{
					temp = grid_array[i]["label"].asString();
					LLStringUtil::toLower(temp);
					if (temp == label)
					{
						return i;
					}
				}
			}
		}
	}
	return -1;
}

void LLViewerLogin::loadGridsLLSD(LLSD& grids,
								  std::string xml_filename,
								  bool can_edit)
{
	LLSD other_grids;
	llifstream llsd_xml;
	llsd_xml.open(xml_filename.c_str(), std::ios::in | std::ios::binary);

	if (llsd_xml.is_open())
	{
		if (mVerbose)
		{
			llinfos << "Reading grid info: " << xml_filename << llendl;
		}
		LLSDSerialize::fromXML(other_grids, llsd_xml);
		for (LLSD::map_iterator grid_itr = other_grids.beginMap(); 
			 grid_itr != other_grids.endMap(); grid_itr++)
		{
			LLSD::String key_name = grid_itr->first;
			LLSD grid_array = grid_itr->second;
			if (mVerbose)
			{
				llinfos << "reading: " << key_name << llendl;
			}
			if (key_name == "grids" && grid_array.isArray())
			{
				for (S32 i = 0; i < grid_array.size(); i++)
				{
					LLSD gmap = grid_array[i];
					if (gmap.has("name") && gmap.has("label") && 
						gmap.has("login_uri") && gmap.has("helper_uri"))
					{
						if (gridIndexInList(grids, gmap["name"].asString(),
											gmap["label"].asString()) != -1)
						{
							if (mVerbose)
							{
								llinfos << "Skipping overridden grid parameters for: "
										<< gmap.get("name") << llendl;
							}
						}
						else
						{
							gmap.insert("can_edit", can_edit ? "true" : "false");
							grids["grids"].append(gmap);
							if (mVerbose)
							{
								llinfos << "Added grid: " << gmap.get("name") << llendl;
							}
						}
					}
					else
					{
						if (mVerbose)
						{
							if (gmap.has("name"))
							{
								llwarns << "Incomplete grid definition for: "
										<< gmap.get("name") << llendl;
							}
							else
							{
								llwarns << "Incomplete grid definition: no name specified"
										<< llendl;
							}
						}
					}
				}
			}
			else if (mVerbose)
			{
				llwarns << "\"" << key_name << "\" is not an array" << llendl;
			}
		}
		llsd_xml.close();
	}
}

void LLViewerLogin::setMenuColor() const
{
	if (mGridList["grids"][mGridChoice].has("menu_color"))
	{
		std::string colorName = mGridList["grids"][mGridChoice].get("menu_color").asString();
		LLColor4 color4;
		LLColor4::parseColor(colorName.c_str(), &color4);
		if (color4 != LLColor4::black)
		{
			gMenuBarView->setBackgroundColor(color4);
		}
	}
}

void LLViewerLogin::setGridChoice(EGridInfo grid)
{
	if (grid < 0 || grid > GRID_INFO_OTHER)
	{
		llwarns << "Invalid grid index specified." << llendl;
		grid = DEFAULT_GRID_CHOICE;
	}

	mGridChoice = grid;
	std::string name = mGridList["grids"][grid].get("label").asString();
	LLStringUtil::toLower(name);
	if (name == "other")
	{
		// *FIX: Mani - could this possibly be valid?
		mGridName = "other";
		setHelperURI("");
		setLoginPageURI("");
	}
	else
	{
		mGridName = mGridList["grids"][grid].get("label").asString();
		setGridURI(mGridList["grids"][grid].get("login_uri").asString());
		setHelperURI(mGridList["grids"][grid].get("helper_uri").asString());
		setLoginPageURI(mGridList["grids"][grid].get("login_page").asString());
		mWebsiteURL = mGridList["grids"][grid].get("website_url").asString();
		mSupportURL = mGridList["grids"][grid].get("support_url").asString();
		mAccountURL = mGridList["grids"][grid].get("register_url").asString();
		mPasswordURL = mGridList["grids"][grid].get("password_url").asString();
	}

	gSavedSettings.setS32("ServerChoice", mGridChoice);
	gSavedSettings.setString("CustomServer", mGridName);
}

void LLViewerLogin::setGridChoice(const std::string& grid_name)
{
	// Set the grid choice based on a string.
	// The string can be:
	// - a grid label from the gGridInfo table 
	// - an ip address
	if (!grid_name.empty())
	{
		// find the grid choice from the user setting.
		std::string pattern(grid_name);
		LLStringUtil::toLower(pattern);
		for (EGridInfo grid_index = GRID_INFO_NONE; grid_index < GRID_INFO_OTHER; grid_index++)
		{
			std::string label = mGridList["grids"][grid_index].get("label").asString();
			std::string name = mGridList["grids"][grid_index].get("name").asString();
			LLStringUtil::toLower(label);
			LLStringUtil::toLower(name);
			if (label.find(pattern) == 0 || name.find(pattern) == 0)
			{
				// Found a matching label in the list...
				setGridChoice(grid_index);
				break;
			}
		}

		mGridChoice = GRID_INFO_OTHER;
		mGridName = grid_name;
		gSavedSettings.setS32("ServerChoice", mGridChoice);
		gSavedSettings.setString("CustomServer", mGridName);
	}
}

void LLViewerLogin::setGridURI(const std::string& uri)
{
	std::vector<std::string> uri_list;
	uri_list.push_back(uri);
	setGridURIs(uri_list);
}

void LLViewerLogin::setGridURIs(const std::vector<std::string>& urilist)
{
	mGridURIs.clear();
	mGridURIs.insert(mGridURIs.begin(), urilist.begin(), urilist.end());
	mCurrentURI = 0;
}

std::string LLViewerLogin::getGridLabel()
{
	if (mGridChoice == GRID_INFO_NONE)
	{
		return "None";
	}
	else if (mGridChoice < GRID_INFO_OTHER)
	{
		return mGridList["grids"][mGridChoice].get("label").asString();
	}
	else if (!mGridName.empty())
	{
		return mGridName;
	}
	else
	{
		return LLURI(getCurrentGridURI()).hostName();
	}
}

std::string LLViewerLogin::getKnownGridLabel(EGridInfo grid) const
{
	if (grid > GRID_INFO_NONE && grid < GRID_INFO_OTHER)
	{
		return mGridList["grids"][grid].get("label").asString();
	}
	return mGridList["grids"][GRID_INFO_NONE].get("label").asString();
}

const std::vector<std::string>& LLViewerLogin::getCommandLineURIs()
{
	return mCommandLineURIs;
}

const std::vector<std::string>& LLViewerLogin::getGridURIs()
{
	return mGridURIs;
}

void LLViewerLogin::parseCommandLineURIs()
{
	// return the login uri set on the command line.
	LLControlVariable* c = gSavedSettings.getControl("CmdLineLoginURI");
	if (c)
	{
		LLSD v = c->getValue();
		if (!v.isUndefined())
		{
			bool foundRealURI = false;
			if (v.isArray())
			{
				for (LLSD::array_const_iterator itr = v.beginArray();
					 itr != v.endArray(); ++itr)
				{
					std::string uri = itr->asString();
					if (!uri.empty())
					{
						foundRealURI = true;
						mCommandLineURIs.push_back(uri);
					}
				}
			}
			else if (v.isString())
			{
				std::string uri = v.asString();
				if (!uri.empty())
				{
					foundRealURI = true;
					mCommandLineURIs.push_back(uri);
				}
			}

			if (foundRealURI)
			{
				mGridChoice = GRID_INFO_OTHER;
				mCurrentURI = 0;
				mGridName = getGridLabel();
			}
		}
	}

	setLoginPageURI(gSavedSettings.getString("LoginPage"));
	setHelperURI(gSavedSettings.getString("CmdLineHelperURI"));
}

const std::string LLViewerLogin::getCurrentGridURI()
{
	return (mGridURIs.size() > mCurrentURI ? mGridURIs[mCurrentURI] : std::string());
}

bool LLViewerLogin::tryNextURI()
{
	if (++mCurrentURI < mGridURIs.size())
	{
		return true;
	}
	else
	{
		mCurrentURI = 0;
		return false;
	}
}

const std::string LLViewerLogin::getStaticGridHelperURI(const EGridInfo grid) const
{
	std::string helper_uri;
	// grab URI from selected grid
	if (grid > GRID_INFO_NONE && grid < GRID_INFO_OTHER)
	{
		helper_uri = mGridList["grids"][grid].get("helper_uri").asString();
	}

	if (helper_uri.empty())
	{
		// what do we do with unnamed/miscellaneous grids?
		// for now, operations that rely on the helper URI (currency/land purchasing) will fail
		llwarns << "Missing Helper URI for this grid ! Currency/land purchasing) will fail..." << llendl;
	}
	return helper_uri;
}

const std::string LLViewerLogin::getHelperURI() const
{
	return mHelperURI;
}

void LLViewerLogin::setHelperURI(const std::string& uri)
{
	mHelperURI = uri;
}

const std::string LLViewerLogin::getLoginPageURI() const
{
	return mLoginPageURI;
}

void LLViewerLogin::setLoginPageURI(const std::string& uri)
{
	mLoginPageURI = uri;
}

const std::string LLViewerLogin::getStaticGridURI(const EGridInfo grid) const
{
	// If its a known grid choice, get the uri from the table,
	// else try the grid name.
	if (grid > GRID_INFO_NONE && grid < GRID_INFO_OTHER)
	{
		return mGridList["grids"][grid].get("login_uri").asString();
	}
	else
	{
		return std::string("");
	}
}

bool LLViewerLogin::isInProductionGrid()
{
	return (getCurrentGridURI().find("aditi") == std::string::npos);
}

const std::string LLViewerLogin::getCurrentGridIP()
{
	std::string domain = getDomain(getCurrentGridURI());

	// Get the IP
	LLHost host;
	host.setHostByName(domain);
	return host.getIPString();
}

void LLViewerLogin::setIsInSecondlife()
{
	std::string ip = getCurrentGridIP();
	gIsInSecondLife = (ip.find("216.82.") == 0);
	gIsInSecondLifeProductionGrid = gIsInSecondLife && isInProductionGrid();
}

//static
const std::string LLViewerLogin::getDomain(const std::string& url)
{
	if (url.empty())
	{
		return url;
	}

	std::string domain = url;
	LLStringUtil::toLower(domain);

	size_t pos = domain.find("//");

	if (pos != std::string::npos)
	{
		size_t count = domain.size() - pos + 2;
		domain = domain.substr(pos + 2, count);
	}

	// Check that there is at least one slash in the URL and add a trailing
	// one if not
	if (domain.find('/') == std::string::npos)
	{
		domain += '/';
	}

	// Paranoia: If there's a user:password@ part, remove it
	pos = domain.find('@');
	if (pos != std::string::npos && pos < domain.find('/'))	// if '@' is not before the first '/', then it's not a user:password
	{
		size_t count = domain.size() - pos + 1;
		domain = domain.substr(pos + 1, count);
	}

	pos = domain.find(':');  
	if (pos != std::string::npos && pos < domain.find('/'))
	{
		// Keep anything before the port number and strip the rest off
		domain = domain.substr(0, pos);
	}
	else
	{
		pos = domain.find('/');	// We earlier made sure that there's one
		domain = domain.substr(0, pos);
	}

	return domain;
}
