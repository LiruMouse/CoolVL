/** 
 * @file llprefsnetwork.cpp
 * @brief Network preferences panel
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

#include "llprefsnetwork.h"

#include "llcheckboxctrl.h"
#include "llradiogroup.h"
#include "lldirpicker.h"
#include "llfilepicker.h"
#include "llpluginclassmedia.h"
#include "lluictrlfactory.h"

#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerwindow.h"

bool LLPrefsNetwork::sSocksSettingsChanged;

LLPrefsNetwork* LLPrefsNetwork::sInstance = NULL;

LLPrefsNetwork::LLPrefsNetwork()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_network.xml");
	sInstance = this;
}

//virtual
LLPrefsNetwork::~LLPrefsNetwork()
{
	sInstance = NULL;
}

//virtual
BOOL LLPrefsNetwork::postBuild()
{
	BOOL http_fetch_enabled = gSavedSettings.getBOOL("ImagePipelineUseHTTP");
	childSetValue("http_texture_fetch", http_fetch_enabled);
	childSetCommitCallback("http_texture_fetch", onHttpTextureFetchToggled, this);
	S32 max_requests = llclamp(gSavedSettings.getS32("TextureMaxHTTPRequests"), 8, 32);
	childSetValue("max_http_requests", (F32)max_requests);
	childSetEnabled("max_http_requests", http_fetch_enabled);

	std::string cache_location = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, "");
	childSetText("cache_location", cache_location);

	childSetAction("clear_disk_cache", onClickClearDiskCache, this);
	childSetAction("set_cache", onClickSetCache, this);
	childSetAction("reset_cache", onClickResetCache, this);

	childSetEnabled("connection_port", gSavedSettings.getBOOL("ConnectionPortEnabled"));
	childSetCommitCallback("connection_port_enabled", onCommitPort, this);

	childSetValue("cache_size", (F32)gSavedSettings.getU32("CacheSize"));
	childSetValue("max_bandwidth", gSavedSettings.getF32("ThrottleBandwidthKBPS"));
	childSetValue("connection_port_enabled", gSavedSettings.getBOOL("ConnectionPortEnabled"));
	childSetValue("connection_port", (F32)gSavedSettings.getU32("ConnectionPort"));

	// Browser settings
	childSetAction("clear_browser_cache", onClickClearBrowserCache, this);
	childSetAction("clear_cookies", onClickClearCookies, this);
	childSetCommitCallback("web_proxy_enabled", onCommitWebProxyEnabled, this);

	std::string value = gSavedSettings.getBOOL("UseExternalBrowser") ? "external" : "internal";
	childSetValue("use_external_browser", value);

	childSetValue("cookies_enabled", gSavedSettings.getBOOL("CookiesEnabled"));
	childSetValue("javascript_enabled", gSavedSettings.getBOOL("BrowserJavascriptEnabled"));
	childSetValue("plugins_enabled", gSavedSettings.getBOOL("BrowserPluginsEnabled"));

	// Web Proxy settings
	childSetValue("web_proxy_enabled", gSavedSettings.getBOOL("BrowserProxyEnabled"));
	childSetValue("web_proxy_editor", gSavedSettings.getString("BrowserProxyAddress"));
	childSetValue("web_proxy_port", gSavedSettings.getS32("BrowserProxyPort"));

	childSetEnabled("proxy_text_label", gSavedSettings.getBOOL("BrowserProxyEnabled"));
	childSetEnabled("web_proxy_editor", gSavedSettings.getBOOL("BrowserProxyEnabled"));
	childSetEnabled("web_proxy_port", gSavedSettings.getBOOL("BrowserProxyEnabled"));
	childSetEnabled("Web", gSavedSettings.getBOOL("BrowserProxyEnabled"));

	// Socks 5 proxy settings, commit callbacks
	childSetCommitCallback("socks5_proxy_enabled", onCommitSocks5ProxyEnabled, this);
	childSetCommitCallback("socks5_auth", onSocksAuthChanged, this);

	//Socks 5 proxy settings, saved data
	childSetValue("socks5_proxy_enabled",   gSavedSettings.getBOOL("Socks5ProxyEnabled"));

	childSetValue("socks5_proxy_host",     gSavedSettings.getString("Socks5ProxyHost"));
	childSetValue("socks5_proxy_port",     (F32)gSavedSettings.getU32("Socks5ProxyPort"));
	childSetValue("socks5_proxy_username", gSavedSettings.getString("Socks5Username"));
	childSetValue("socks5_proxy_password", gSavedSettings.getString("Socks5Password"));
	childSetValue("socks5_auth", gSavedSettings.getString("Socks5AuthType"));

	// Other HTTP connections proxy setting
	childSetValue("http_proxy_type", gSavedSettings.getString("HttpProxyType"));

	// Socks 5 proxy settings, check if settings modified callbacks
	childSetCommitCallback("socks5_proxy_host", onSocksSettingsModified, this);
	childSetCommitCallback("socks5_proxy_port", onSocksSettingsModified, this);
	childSetCommitCallback("socks5_proxy_username", onSocksSettingsModified, this);
	childSetCommitCallback("socks5_proxy_password", onSocksSettingsModified, this);

	// Socks 5 settings, Set all controls and labels enabled state
	updateProxyEnabled(this, gSavedSettings.getBOOL("Socks5ProxyEnabled"),
					   gSavedSettings.getString("Socks5AuthType"));

	sSocksSettingsChanged = false;

	return TRUE;
}

//virtual
void LLPrefsNetwork::draw()
{
	childSetEnabled("set_cache",
					!LLFilePickerThread::isInUse() && !LLDirPickerThread::isInUse());
	LLPanel::draw();
}

void sendMediaSettings()
{
	LLViewerMedia::setCookiesEnabled(gSavedSettings.getBOOL("CookiesEnabled"));
	LLViewerMedia::setProxyConfig(gSavedSettings.getBOOL("BrowserProxyEnabled"),
								  gSavedSettings.getString("BrowserProxyAddress"),
								  gSavedSettings.getS32("BrowserProxyPort"));
}

void LLPrefsNetwork::apply()
{
	gSavedSettings.setBOOL("ImagePipelineUseHTTP", childGetValue("http_texture_fetch"));
	gSavedSettings.setS32("TextureMaxHTTPRequests", childGetValue("max_http_requests").asInteger());

	U32 cache_size = (U32)childGetValue("cache_size").asInteger();
	if (gSavedSettings.getU32("CacheSize") != cache_size)
	{
		onClickClearDiskCache(this);
		gSavedSettings.setU32("CacheSize", cache_size);
	}
	gSavedSettings.setF32("ThrottleBandwidthKBPS", childGetValue("max_bandwidth").asReal());
	gSavedSettings.setBOOL("ConnectionPortEnabled", childGetValue("connection_port_enabled"));
	gSavedSettings.setU32("ConnectionPort", childGetValue("connection_port").asInteger());

	gSavedSettings.setBOOL("Socks5ProxyEnabled", childGetValue("socks5_proxy_enabled"));
	gSavedSettings.setString("Socks5ProxyHost", childGetValue("socks5_proxy_host"));
	gSavedSettings.setU32("Socks5ProxyPort", childGetValue("socks5_proxy_port").asInteger());

	gSavedSettings.setString("Socks5AuthType", childGetValue("socks5_auth"));
	gSavedSettings.setString("Socks5Username", childGetValue("socks5_proxy_username"));
	gSavedSettings.setString("Socks5Password", childGetValue("socks5_proxy_password"));

	gSavedSettings.setBOOL("CookiesEnabled", childGetValue("cookies_enabled"));
	gSavedSettings.setBOOL("BrowserJavascriptEnabled", childGetValue("javascript_enabled"));
	gSavedSettings.setBOOL("BrowserPluginsEnabled", childGetValue("plugins_enabled"));
	gSavedSettings.setBOOL("BrowserProxyEnabled", childGetValue("web_proxy_enabled"));
	gSavedSettings.setString("BrowserProxyAddress", childGetValue("web_proxy_editor"));
	gSavedSettings.setS32("BrowserProxyPort", childGetValue("web_proxy_port"));

	gSavedSettings.setString("HttpProxyType", childGetValue("http_proxy_type"));

	bool value = childGetValue("use_external_browser").asString() == "external" ? true : false;
	gSavedSettings.setBOOL("UseExternalBrowser", value);

	sendMediaSettings();

	if (sSocksSettingsChanged &&
		LLStartUp::getStartupState() != STATE_LOGIN_WAIT)
	{
		LLNotifications::instance().add("ProxyNeedRestart");
		sSocksSettingsChanged = false;
	}
}

void LLPrefsNetwork::cancel()
{
	sendMediaSettings();
}

// static
void LLPrefsNetwork::onHttpTextureFetchToggled(LLUICtrl* ctrl, void* data)
{
	LLPrefsNetwork* self = (LLPrefsNetwork*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (self && check)
	{
		self->childSetEnabled("max_http_requests", check->get());
	}
}

// static
void LLPrefsNetwork::onClickClearDiskCache(void*)
{
	// flag client cache for clearing next time the client runs
	gSavedSettings.setBOOL("PurgeCacheOnNextStartup", TRUE);
	LLNotifications::instance().add("CacheWillClear");
}

// static
void LLPrefsNetwork::setCacheCallback(std::string& dir_name, void* data)
{
	LLPrefsNetwork* self = (LLPrefsNetwork*)data;
	if (!self || self != sInstance)
	{
		LLNotifications::instance().add("PreferencesClosed");
		return;
	}
	std::string cur_name = gSavedSettings.getString("CacheLocation");
	if (!dir_name.empty() && dir_name != cur_name)
	{
		self->childSetText("cache_location", dir_name);
		LLNotifications::instance().add("CacheWillBeMoved");
		gSavedSettings.setString("NewCacheLocation", dir_name);
	}
}

// static
void LLPrefsNetwork::onClickSetCache(void* data)
{
	std::string suggestion = gSavedSettings.getString("CacheLocation");

	(new LLCallDirPicker(LLPrefsNetwork::setCacheCallback,
						 data))->getDir(&suggestion);
}

// static
void LLPrefsNetwork::onClickResetCache(void* data)
{
 	LLPrefsNetwork* self = (LLPrefsNetwork*)data;
	if (!gSavedSettings.getString("CacheLocation").empty())
	{
		gSavedSettings.setString("NewCacheLocation", "");
		LLNotifications::instance().add("CacheWillBeMoved");
	}
	std::string cache_location = gDirUtilp->getCacheDir(true);
	self->childSetText("cache_location", cache_location);
}

// static
void LLPrefsNetwork::onCommitPort(LLUICtrl* ctrl, void* data)
{
  LLPrefsNetwork* self = (LLPrefsNetwork*)data;
  LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;

  if (!self || !check) return;
  self->childSetEnabled("connection_port", check->get());
  LLNotifications::instance().add("ChangeConnectionPort");
}

// static
void LLPrefsNetwork::onCommitSocks5ProxyEnabled(LLUICtrl* ctrl, void* data)
{
	LLPrefsNetwork* self  = (LLPrefsNetwork*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;

	if (!self || !check) return;

	sSocksSettingsChanged = true;

	updateProxyEnabled(self, check->get(), self->childGetValue("socks5_auth"));
}

// static
void LLPrefsNetwork::onSocksSettingsModified(LLUICtrl* ctrl, void* data)
{
	sSocksSettingsChanged = true;
}

// static
void LLPrefsNetwork::onSocksAuthChanged(LLUICtrl* ctrl, void* data)
{
	LLRadioGroup* radio  = static_cast<LLRadioGroup*>(ctrl);
	LLPrefsNetwork* self = static_cast<LLPrefsNetwork*>(data);

	sSocksSettingsChanged = true;

	std::string selection = radio->getValue().asString();
	updateProxyEnabled(self, self->childGetValue("socks5_proxy_enabled"), selection);
}

// static
void LLPrefsNetwork::updateProxyEnabled(LLPrefsNetwork * self, bool enabled, std::string authtype)
{
	// Manage all the enable/disable of the socks5 options from this single function
	// to avoid code duplication

	// Update all socks labels and controls except auth specific ones
	self->childSetEnabled("socks5_proxy_port",  enabled);
	self->childSetEnabled("socks5_proxy_host",  enabled);
	self->childSetEnabled("socks5_host_label",  enabled);
	self->childSetEnabled("socks5_proxy_port",  enabled);
	self->childSetEnabled("socks5_auth",        enabled);

	if (!enabled && self->childGetValue("http_proxy_type").asString() == "Socks")
	{
		self->childSetValue("http_proxy_type", "None");
	}
	self->childSetEnabled("Socks", enabled);

	// Hide the auth specific lables if authtype is none or
	// we are not enabled.
	if (!enabled || authtype.compare("None") == 0)
	{
		self->childSetEnabled("socks5_username_label", false);
		self->childSetEnabled("socks5_password_label", false);
		self->childSetEnabled("socks5_proxy_username", false);
		self->childSetEnabled("socks5_proxy_password", false);
	}

	// Only show the username and password boxes if we are enabled
	// and authtype is username pasword.
	if (enabled && authtype.compare("UserPass") == 0)
	{
		self->childSetEnabled("socks5_username_label", true);
		self->childSetEnabled("socks5_password_label", true);
		self->childSetEnabled("socks5_proxy_username", true);
		self->childSetEnabled("socks5_proxy_password", true);
	}
}

// static
void LLPrefsNetwork::onClickClearBrowserCache(void*)
{
	LLNotifications::instance().add("ConfirmClearBrowserCache", LLSD(), LLSD(),
									callback_clear_browser_cache);
}

//static
bool LLPrefsNetwork::callback_clear_browser_cache(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0) // YES
	{
		LLViewerMedia::clearAllCaches();
	}
	return false;
}

// static
void LLPrefsNetwork::onClickClearCookies(void*)
{
	LLNotifications::instance().add("ConfirmClearCookies", LLSD(), LLSD(),
									callback_clear_cookies);
}

//static
bool LLPrefsNetwork::callback_clear_cookies(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0) // YES
	{
		LLViewerMedia::clearAllCookies();
	}
	return false;
}

// static
void LLPrefsNetwork::onCommitWebProxyEnabled(LLUICtrl* ctrl, void* data)
{
	LLPrefsNetwork* self = (LLPrefsNetwork*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;

	bool enabled = check->get();
	self->childSetEnabled("web_proxy_editor", enabled);
	self->childSetEnabled("web_proxy_port", enabled);
	self->childSetEnabled("proxy_text_label", enabled);
	self->childSetEnabled("Web", enabled);
	if (!enabled && self->childGetValue("http_proxy_type").asString() == "Web")
	{
		self->childSetValue("http_proxy_type", "None");
	}
}
