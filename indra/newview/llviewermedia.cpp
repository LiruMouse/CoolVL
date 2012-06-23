/**
 * @file llviewermedia.cpp
 * @brief Client interface to the media engine
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llviewermedia.h"

#include "llpluginclassmedia.h"
#include "llplugincookiestore.h"
#include "lluuid.h"
#include "llkeyboard.h"
#include "llversionviewer.h"

#include "llfilepicker.h"
#include "llfloateravatarinfo.h"	// for getProfileURL() function
#include "llhoverview.h"
#include "llmimetypes.h"
#include "llurldispatcher.h"
#include "llviewercontrol.h"
#include "llviewermediafocus.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llweb.h"

// Move this to its own file.

LLViewerMediaEventEmitter::~LLViewerMediaEventEmitter()
{
	observerListType::iterator iter = mObservers.begin();

	while (iter != mObservers.end())
	{
		LLViewerMediaObserver *self = *iter;
		iter++;
		remObserver(self);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaEventEmitter::addObserver(LLViewerMediaObserver* observer)
{
	if (!observer)
		return false;

	if (std::find(mObservers.begin(), mObservers.end(), observer) != mObservers.end())
		return false;

	mObservers.push_back(observer);
	observer->mEmitters.push_back(this);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaEventEmitter::remObserver(LLViewerMediaObserver* observer)
{
	if (!observer)
		return false;

	mObservers.remove(observer);
	observer->mEmitters.remove(this);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void LLViewerMediaEventEmitter::emitEvent(LLPluginClassMedia* media, LLPluginClassMediaOwner::EMediaEvent event)
{
	observerListType::iterator iter = mObservers.begin();

	while (iter != mObservers.end())
	{
		LLViewerMediaObserver *self = *iter;
		++iter;
		self->handleMediaEvent(media, event);
	}
}

// Move this to its own file.
LLViewerMediaObserver::~LLViewerMediaObserver()
{
	std::list<LLViewerMediaEventEmitter *>::iterator iter = mEmitters.begin();

	while (iter != mEmitters.end())
	{
		LLViewerMediaEventEmitter *self = *iter;
		iter++;
		self->remObserver(this);
	}
}

// Move this to its own file.
// helper class that tries to download a URL from a web site and calls a method
// on the Panel Land Media and to discover the MIME type
class LLMimeDiscoveryResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLMimeDiscoveryResponder);

public:
	LLMimeDiscoveryResponder(viewer_media_t media_impl)
	:	mMediaImpl(media_impl),
		mInitialized(false)
	{
	}

	/*virtual*/ void completedHeader(U32 status, const std::string& reason,
									 const LLSD& content)
	{
		std::string media_type = content["content-type"].asString();
		std::string::size_type idx1 = media_type.find_first_of(";");
		std::string mime_type = media_type.substr(0, idx1);

		LL_DEBUGS("Media") << "status is " << status << ", media type \""
						   << media_type << "\"" << LL_ENDL;

		if (mime_type.empty())
		{
			// Some sites don't return any content-type header at all.
			// Treat an empty mime type as text/html.
			mime_type = "text/html";
		}
		completeAny(status, mime_type);
	}

	void completeAny(U32 status, const std::string& mime_type)
	{
		if (!mInitialized && !mime_type.empty())
		{
			if (mMediaImpl->initializeMedia(mime_type))
			{
				mInitialized = true;
				mMediaImpl->play();
			}
		}
	}

public:
	viewer_media_t mMediaImpl;
	bool mInitialized;
};

class LLViewerMediaOpenIDResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLViewerMediaOpenIDResponder);

public:
	LLViewerMediaOpenIDResponder()
	{
	}

	~LLViewerMediaOpenIDResponder()
	{
	}

	/*virtual*/ void completedHeader(U32 status, const std::string& reason,
									   const LLSD& content)
	{
		LL_DEBUGS("Media") << "status = " << status << ", reason = " << reason << LL_ENDL;
		LL_DEBUGS("Media") << content << LL_ENDL;
		std::string cookie = content["set-cookie"].asString();

		LLViewerMedia::openIDCookieResponse(cookie);
	}

	/*virtual*/ void completedRaw(U32 status,
								  const std::string& reason,
								  const LLChannelDescriptors& channels,
								  const LLIOPipe::buffer_ptr_t& buffer)
	{
		// This is just here to disable the default behavior (attempting to
		// parse the response as llsd). We don't care about the content of the
		// response, only the set-cookie header.
	}
};

class LLViewerMediaWebProfileResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLViewerMediaWebProfileResponder);
public:
	LLViewerMediaWebProfileResponder(std::string host)
	{
		mHost = host;
	}

	~LLViewerMediaWebProfileResponder()
	{
	}

	/*virtual*/ void completedHeader(U32 status, const std::string& reason,
									   const LLSD& content)
	{
		LL_DEBUGS("Media") << "status = " << status << ", reason = " << reason << LL_ENDL;
		LL_DEBUGS("Media") << content << LL_ENDL;

		std::string cookie = content["set-cookie"].asString();

		LLViewerMedia::getCookieStore()->setCookiesFromHost(cookie, mHost);
	}

	 void completedRaw(U32 status,
					  const std::string& reason,
					  const LLChannelDescriptors& channels,
					  const LLIOPipe::buffer_ptr_t& buffer)
	{
		// This is just here to disable the default behavior (attempting to
		// parse the response as llsd). We don't care about the content of the
		// response, only the set-cookie header.
	}

	std::string mHost;
};

LLPluginCookieStore *LLViewerMedia::sCookieStore = NULL;
LLURL LLViewerMedia::sOpenIDURL;
std::string LLViewerMedia::sOpenIDCookie;
typedef std::list<LLViewerMediaImpl*> impl_list;
static impl_list sViewerMediaImplList;
static std::string sUpdatedCookies;
static const char *PLUGIN_COOKIE_FILE_NAME = "plugin_cookies.txt";

//////////////////////////////////////////////////////////////////////////////////////////
// LLViewerMedia

//////////////////////////////////////////////////////////////////////////////////////////
// static
viewer_media_t LLViewerMedia::newMediaImpl(const std::string& media_url,
										   const LLUUID& texture_id,
										   S32 media_width,
										   S32 media_height,
										   U8 media_auto_scale,
										   U8 media_loop,
										   std::string mime_type)
{
	LLViewerMediaImpl* media_impl = getMediaImplFromTextureID(texture_id);
	if (media_impl == NULL || texture_id.isNull())
	{
		// Create the media impl
		media_impl = new LLViewerMediaImpl(media_url, texture_id, media_width,
										   media_height, media_auto_scale,
										   media_loop, mime_type);
		sViewerMediaImplList.push_back(media_impl);
	}
	else
	{
		media_impl->stop();
		media_impl->mTextureId = texture_id;
		media_impl->mMediaURL = media_url;
		media_impl->mMediaWidth = media_width;
		media_impl->mMediaHeight = media_height;
		media_impl->mMediaAutoScale = media_auto_scale;
		media_impl->mMediaLoop = media_loop;
		if (!media_url.empty())
		{
			media_impl->navigateTo(media_url, mime_type, false);
		}
	}
	return media_impl;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::removeMedia(LLViewerMediaImpl* media)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for ( ; iter != end; iter++)
	{
		if (media == *iter)
		{
			sViewerMediaImplList.erase(iter);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
LLViewerMediaImpl* LLViewerMedia::getMediaImplFromTextureID(const LLUUID& texture_id)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* media_impl = *iter;
		if (media_impl->getMediaTextureID() == texture_id)
		{
			return media_impl;
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
std::string LLViewerMedia::getCurrentUserAgent()
{
	// append our magic version number string to the browser user agent id
	// See the HTTP 1.0 and 1.1 specifications for allowed formats:
	// http://www.ietf.org/rfc/rfc1945.txt section 10.15
	// http://www.ietf.org/rfc/rfc2068.txt section 3.8
	// This was also helpful:
	// http://www.mozilla.org/build/revised-user-agent-strings.html
	std::ostringstream ua;
	ua << "SecondLife/"
	   << LL_VERSION_MAJOR << "." << LL_VERSION_MINOR << "."
	   << LL_VERSION_PATCH << "." << LL_VERSION_BUILD << " ("
	   << gSavedSettings.getString("VersionChannelName") << "; "
	   << gSavedSettings.getString("SkinCurrent") << " skin)";
	llinfos << "User agent: " << ua.str() << llendl;

	return ua.str();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::updateBrowserUserAgent()
{
	std::string user_agent = getCurrentUserAgent();

	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (pimpl->mMediaSource &&
			pimpl->mMediaSource->pluginSupportsMediaBrowser())
		{
			pimpl->mMediaSource->setBrowserUserAgent(user_agent);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::handleSkinCurrentChanged(const LLSD& /*newvalue*/)
{
	// gSavedSettings is already updated when this function is called.
	updateBrowserUserAgent();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
bool LLViewerMedia::textureHasMedia(const LLUUID& texture_id)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (pimpl->getMediaTextureID() == texture_id)
		{
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::setVolume(F32 volume)
{
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->setVolume(volume);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::updateMedia()
{
	sUpdatedCookies = getCookieStore()->getChangedCookies();
	if (!sUpdatedCookies.empty())
	{
		LL_DEBUGS("Media") << "Updated cookies will be sent to all loaded plugins: "
						   << sUpdatedCookies << LL_ENDL;
	}

	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();

	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->update();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::clearAllCookies()
{
	// Clear all cookies for all plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (pimpl->mMediaSource)
		{
			pimpl->mMediaSource->clear_cookies();
		}
	}

	// Clear all cookies from the cookie store
	getCookieStore()->setAllCookies("");

	// FIXME: this may not be sufficient, since the on-disk cookie file won't
	// get written until some browser instance exits cleanly.
	// It also won't clear cookies for other accounts, or for any account if
	// we're not logged in, and won't do anything at all if there are no webkit
	// plugins loaded.
	// Until such time as we can centralize cookie storage, the following hack
	// should cover these cases:

	// HACK: Look for cookie files in all possible places and delete them.
	// NOTE: this assumes knowledge of what happens inside the webkit plugin
	// (it's what adds 'browser_profile' to the path and names the cookie file)

	// Places that cookie files can be:
	// <getOSUserAppDir>/browser_profile/cookies
	// <getOSUserAppDir>/first_last/browser_profile/cookies
	//		(note that there may be any number of these!)
	// <getOSUserAppDir>/first_last/plugin_cookies.txt
	//		(note that there may be any number of these!)

	std::string base_dir = gDirUtilp->getOSUserAppDir() + gDirUtilp->getDirDelimiter();
	std::string target;
	std::string filename;

	LL_DEBUGS("Media") << "base dir = " << base_dir << LL_ENDL;

	// The non-logged-in version is easy
	target = base_dir;
	target += "browser_profile";
	target += gDirUtilp->getDirDelimiter();
	target += "cookies";
	LL_DEBUGS("Media") << "target = " << target << LL_ENDL;
	if (LLFile::isfile(target))
	{
		LLFile::remove(target);
	}

	// the hard part: iterate over all user directories and delete the cookie file from each one
	while (gDirUtilp->getNextFileInDir(base_dir, "*_*", filename, false))
	{
		target = base_dir;
		target += filename;
		target += gDirUtilp->getDirDelimiter();
		target += "browser_profile";
		target += gDirUtilp->getDirDelimiter();
		target += "cookies";
		LL_DEBUGS("Media") << "target = " << target << LL_ENDL;
		if (LLFile::isfile(target))
		{
			LLFile::remove(target);
		}

		// Other accounts may have new-style cookie files too -- delete them as well
		target = base_dir;
		target += filename;
		target += gDirUtilp->getDirDelimiter();
		target += PLUGIN_COOKIE_FILE_NAME;
		LL_DEBUGS("Media") << "target = " << target << LL_ENDL;
		if (LLFile::isfile(target))
		{
			LLFile::remove(target);
		}
	}

	// If we have an OpenID cookie, re-add it to the cookie store.
	setOpenIDCookie();
}

/////////////////////////////////////////////////////////////////////////////////////////
// static 
void LLViewerMedia::clearAllCaches()
{
	// Clear all plugins' caches
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		pimpl->clearCache();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// static 
void LLViewerMedia::setCookiesEnabled(bool enabled)
{
	// Set the "cookies enabled" flag for all loaded plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (pimpl->mMediaSource)
		{
			pimpl->mMediaSource->enable_cookies(enabled);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// static 
void LLViewerMedia::setProxyConfig(bool enable, const std::string &host, int port)
{
	// Set the proxy config for all loaded plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if(pimpl->mMediaSource)
		{
			pimpl->mMediaSource->proxy_setup(enable, host, port);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// static 
/////////////////////////////////////////////////////////////////////////////////////////
// static
LLPluginCookieStore *LLViewerMedia::getCookieStore()
{
	if (sCookieStore == NULL)
	{
		sCookieStore = new LLPluginCookieStore;
	}

	return sCookieStore;
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::loadCookieFile()
{
	std::string resolved_filename = gDirUtilp->getLindenUserDir();
	if (resolved_filename.empty())
	{
		// Use the unknown user cookies
		resolved_filename = gDirUtilp->getOSUserAppDir() +
							gDirUtilp->getDirDelimiter() +
							PLUGIN_COOKIE_FILE_NAME;
		llinfos << "Can't get path to per account settings, falling back to: "
				<< resolved_filename << llendl;
	}
	else
	{
		// build filename for each user
		resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT,
														   PLUGIN_COOKIE_FILE_NAME);
	}

	// open the file for reading
	llifstream file(resolved_filename);
	if (!file.is_open())
	{
		LL_WARNS("Media") << "Can't load plugin cookies from file \""
						  << PLUGIN_COOKIE_FILE_NAME << "\"" << LL_ENDL;
		return;
	}

	getCookieStore()->readAllCookies(file, true);

	file.close();

	// send the clear_cookies message to all loaded plugins
	impl_list::iterator iter = sViewerMediaImplList.begin();
	impl_list::iterator end = sViewerMediaImplList.end();
	for ( ; iter != end; iter++)
	{
		LLViewerMediaImpl* pimpl = *iter;
		if (pimpl->mMediaSource)
		{
			pimpl->mMediaSource->clear_cookies();
		}
	}

	// If we have an OpenID cookie, re-add it to the cookie store.
	setOpenIDCookie();
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::saveCookieFile()
{
	std::string resolved_filename = gDirUtilp->getLindenUserDir();
	if (resolved_filename.empty())
	{
		// Set the unknown user cookies
		resolved_filename = gDirUtilp->getOSUserAppDir() +
							gDirUtilp->getDirDelimiter() +
							PLUGIN_COOKIE_FILE_NAME;
		llinfos << "Can't get path to per account settings, falling back to: "
				<< resolved_filename << llendl;
	}
	else
	{
		// build filename for each user
		resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT,
														   PLUGIN_COOKIE_FILE_NAME);
	}

	// open a file for writing
	llofstream file (resolved_filename);
	if (!file.is_open())
	{
		LL_WARNS("Media") << "Can't open plugin cookie file \""
						  << PLUGIN_COOKIE_FILE_NAME << "\" for writing"
						  << LL_ENDL;
		return;
	}

	getCookieStore()->writePersistentCookies(file);

	file.close();
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::addCookie(const std::string &name,
							  const std::string &value,
							  const std::string &domain,
							  const LLDate &expires,
							  const std::string &path,
							  bool secure)
{
	std::stringstream cookie;

	cookie << name << "=" << LLPluginCookieStore::quoteString(value);

	if (expires.notNull())
	{
		cookie << "; expires=" << expires.asRFC1123();
	}

	cookie << "; domain=" << domain;

	cookie << "; path=" << path;

	if (secure)
	{
		cookie << "; secure";
	}

	getCookieStore()->setCookies(cookie.str());
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::addSessionCookie(const std::string &name,
									 const std::string &value,
									 const std::string &domain,
									 const std::string &path,
									 bool secure)
{
	// A session cookie just has a NULL date.
	addCookie(name, value, domain, LLDate(), path, secure);
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::removeCookie(const std::string &name,
								 const std::string &domain,
								 const std::string &path)
{
	// To remove a cookie, add one with the same name, domain, and path that expires in the past.
	addCookie(name, "", domain, LLDate(LLDate::now().secondsSinceEpoch() - 1.0), path);
}

LLSD LLViewerMedia::getHeaders()
{
	LLSD headers = LLSD::emptyMap();
	headers["Accept"] = "*/*";
	headers["Content-Type"] = "application/xml";
	headers["Cookie"] = sOpenIDCookie;
	headers["User-Agent"] = getCurrentUserAgent();

	return headers;
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::setOpenIDCookie()
{
	if (!sOpenIDCookie.empty())
	{
		// The LLURL can give me the 'authority', which is of the form:
		// [username[:password]@]hostname[:port]
		// We want just the hostname for the cookie code, but LLURL doesn't
		// seem to have a way to extract that. We therefore do it here.
		std::string authority = sOpenIDURL.mAuthority;
		std::string::size_type host_start = authority.find('@'); 
		if (host_start == std::string::npos)
		{
			// no username/password
			host_start = 0;
		}
		else
		{
			// Hostname starts after the @. 
			// (If the hostname part is empty, this may put host_start at the end of the string.  In that case, it will end up passing through an empty hostname, which is correct.)
			++host_start;
		}
		std::string::size_type host_end = authority.rfind(':'); 
		if (host_end == std::string::npos || host_end < host_start)
		{
			// no port
			host_end = authority.size();
		}

		getCookieStore()->setCookiesFromHost(sOpenIDCookie, authority.substr(host_start, host_end - host_start));

		// Do a web profile get so we can store the cookie 
		LLSD headers = LLSD::emptyMap();
		headers["Accept"] = "*/*";
		headers["Cookie"] = sOpenIDCookie;
		headers["User-Agent"] = getCurrentUserAgent();

		std::string profile_url = getProfileURL("");
		LLURL raw_profile_url(profile_url.c_str());

		LLHTTPClient::get(profile_url,  
						  new LLViewerMediaWebProfileResponder(raw_profile_url.getAuthority()),
						  headers);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::openIDSetup(const std::string &openid_url, const std::string &openid_token)
{
	LL_DEBUGS("Media") << "url = \"" << openid_url << "\", token = \""
					   << openid_token << "\"" << LL_ENDL;

	// post the token to the url 
	// the responder will need to extract the cookie(s).

	// Save the OpenID URL for later -- we may need the host when adding the cookie.
	sOpenIDURL.init(openid_url.c_str());

	// We shouldn't ever do this twice, but just in case this code gets repurposed later, clear existing cookies.
	sOpenIDCookie.clear();

	LLSD headers = LLSD::emptyMap();
	// Keep LLHTTPClient from adding an "Accept: application/llsd+xml" header
	headers["Accept"] = "*/*";
	// and use the expected content-type for a post, instead of the
	// LLHTTPClient::postRaw() default of "application/octet-stream"
	headers["Content-Type"] = "application/x-www-form-urlencoded";

	// postRaw() takes ownership of the buffer and releases it later, so we
	// need to allocate a new buffer here.
	size_t size = openid_token.size();
	U8 *data = new U8[size];
	memcpy(data, openid_token.data(), size);

	LLHTTPClient::postRaw(openid_url, data, size,
						  new LLViewerMediaOpenIDResponder(), headers);
}

/////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::openIDCookieResponse(const std::string &cookie)
{
	LL_DEBUGS("Media") << "Cookie received: \"" << cookie << "\"" << LL_ENDL;
	sOpenIDCookie += cookie;
	setOpenIDCookie();
}

//////////////////////////////////////////////////////////////////////////////////////////
// static
void LLViewerMedia::cleanupClass()
{
	// This is no longer necessary, since the list is no longer smart pointers.
#if 0
	while (!sViewerMediaImplList.empty())
	{
		sViewerMediaImplList.pop_back();
	}
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// LLViewerMediaImpl
//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl::LLViewerMediaImpl(const std::string& media_url, 
									 const LLUUID& texture_id, 
									 S32 media_width, 
									 S32 media_height, 
									 U8 media_auto_scale, 
									 U8 media_loop,
									 const std::string& mime_type)
:	mMediaSource(NULL),
	mMovieImageHasMips(false),
	mTextureId(texture_id),
	mMediaWidth(media_width),
	mMediaHeight(media_height),
	mMediaAutoScale(media_auto_scale),
	mMediaLoop(media_loop),
	mMediaURL(media_url),
	mMimeType(mime_type),
	mNeedsNewTexture(true),
	mTextureUsedWidth(0),
	mTextureUsedHeight(0),
	mSuspendUpdates(false),
	mVisible(true),
	mMediaNavState(MEDIANAVSTATE_NONE),
	mHasFocus(false),
	mMediaSourceFailed(false),
	mPreviousMediaState(MEDIA_NONE),
	mBackgroundColor(LLColor4::black) // Do not set to white or may get "white flash" bug.
{ 
	createMediaSource();
}

//////////////////////////////////////////////////////////////////////////////////////////
LLViewerMediaImpl::~LLViewerMediaImpl()
{
	if (gEditMenuHandler == this)
	{
		gEditMenuHandler = NULL;
	}

	destroyMediaSource();
	LLViewerMedia::removeMedia(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::initializeMedia(const std::string& mime_type)
{
	bool mimeTypeChanged = (mMimeType != mime_type);
	bool pluginChanged = (LLMIMETypes::implType(mCurrentMimeType) !=
						  LLMIMETypes::implType(mime_type));

	if (!mMediaSource || pluginChanged)
	{
		if (!initializePlugin(mime_type))
		{
			LL_WARNS("Plugin") << "plugin intialization failed for mime type: "
							   << mime_type << LL_ENDL;
			return false;
		}
	}
	else if (mimeTypeChanged)
	{
		// The same plugin should be able to handle the new media,
		// just update the stored mime type.
		mMimeType = mime_type;
	}

	// play();
	return (mMediaSource != NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::createMediaSource()
{
	if (!mMediaURL.empty())
	{
		navigateTo(mMediaURL, mMimeType, true);
	}
	else if (!mMimeType.empty())
	{
		if (!initializeMedia(mMimeType))
		{
			LL_WARNS("Media") << "Failed to initialize media for mime type "
							  << mMimeType << LL_ENDL;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::destroyMediaSource()
{
	mNeedsNewTexture = true;
	if (!mMediaSource)
	{
		return;
	}
	// Restore the texture
	updateMovieImage(LLUUID::null, false);
	delete mMediaSource;
	mMediaSource = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setMediaType(const std::string& media_type)
{
	mMimeType = media_type;
}

//////////////////////////////////////////////////////////////////////////////////////////
/*static*/
LLPluginClassMedia* LLViewerMediaImpl::newSourceFromMediaType(std::string media_type,
															  LLPluginClassMediaOwner *owner, /* may be NULL */
															  S32 default_width,
															  S32 default_height,
															  const std::string target)
{
	std::string plugin_basename = LLMIMETypes::implType(media_type);

	if (plugin_basename.empty())
	{
		LL_WARNS("Media") << "Couldn't find plugin for media type " << media_type << LL_ENDL;
	}
	else
	{
		std::string launcher_name = gDirUtilp->getLLPluginLauncher();
		std::string plugin_name = gDirUtilp->getLLPluginFilename(plugin_basename);
		std::string user_data_path = gDirUtilp->getOSUserAppDir();
		user_data_path += gDirUtilp->getDirDelimiter();

		// Fix for EXT-5960 - make browser profile specific to user (cache, cookies etc.)
		// If the linden username returned is blank, that can only mean we are
		// at the login page displaying login Web page or Web browser test via Develop menu.
		// In this case we just use whatever gDirUtilp->getOSUserAppDir() gives us (this
		// is what we always used before this change)
		std::string linden_user_dir = gDirUtilp->getLindenUserDir();
		if (!linden_user_dir.empty())
		{
			// gDirUtilp->getLindenUserDir() is whole path, not just Linden name
			user_data_path = linden_user_dir;
			user_data_path += gDirUtilp->getDirDelimiter();
		}

		// See if the plugin executable exists
		llstat s;
		if (LLFile::stat(launcher_name, &s))
		{
			LL_WARNS("Media") << "Couldn't find launcher at " << launcher_name << LL_ENDL;
		}
		else if (LLFile::stat(plugin_name, &s))
		{
			LL_WARNS("Media") << "Couldn't find plugin at " << plugin_name << LL_ENDL;
		}
		else
		{
			LLPluginClassMedia* media_source = new LLPluginClassMedia(owner);
			media_source->setOwner(owner);
			media_source->setSize(default_width, default_height);
			media_source->setUserDataPath(user_data_path);
			media_source->setLanguageCode(LLUI::getLanguage());
			media_source->enable_cookies(gSavedSettings.getBOOL("CookiesEnabled"));
			media_source->setPluginsEnabled(gSavedSettings.getBOOL("BrowserPluginsEnabled"));
			media_source->setJavascriptEnabled(gSavedSettings.getBOOL("BrowserJavascriptEnabled"));
			media_source->setTarget(target);

			const std::string plugin_dir = gDirUtilp->getLLPluginDir();
			if (media_source->init(launcher_name, plugin_dir, plugin_name, false))
			{
				return media_source;
			}
			else
			{
				LL_WARNS("Media") << "Failed to init plugin.  Destroying." << LL_ENDL;
				delete media_source;
			}
		}
	}

	LL_WARNS("Media") << "Plugin intialization failed for mime type: " << media_type << LL_ENDL;
	LLSD args;
	args["MIME_TYPE"] = media_type;
	LLNotifications::instance().add("NoPlugin", args);

	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::initializePlugin(const std::string& media_type)
{
	if (mMediaSource)
	{
		// Save the previous media source's last set size before destroying it.
		mMediaWidth = mMediaSource->getSetWidth();
		mMediaHeight = mMediaSource->getSetHeight();
	}

	// Always delete the old media impl first.
	destroyMediaSource();

	// and unconditionally set the mime type
	mMimeType = media_type;
	// Save the MIME type that really caused the plugin to load
	mCurrentMimeType = mMimeType;

	// If we got here, we want to ignore previous init failures.
	mMediaSourceFailed = false;

	LLPluginClassMedia* media_source = newSourceFromMediaType(media_type,
															  this,
															  mMediaWidth,
															  mMediaHeight,
															  mTarget);
	if (media_source)
	{
		media_source->setDisableTimeout(gSavedSettings.getBOOL("DebugPluginDisableTimeout"));
		media_source->setLoop(mMediaLoop);
		media_source->setAutoScale(mMediaAutoScale);
		media_source->setBrowserUserAgent(LLViewerMedia::getCurrentUserAgent());
		media_source->focus(mHasFocus);
		media_source->setBackgroundColor(mBackgroundColor);

		if (gSavedSettings.getBOOL("BrowserIgnoreSSLCertErrors"))
		{
			media_source->ignore_ssl_cert_errors(true);
		}

		// the correct way to deal with certs it to load ours from CA.pem and
		// append them to the ones Qt/WebKit loads from your system location.
		std::string ca_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "CA.pem");
		media_source->addCertificateFilePath(ca_path);

		media_source->proxy_setup(gSavedSettings.getBOOL("BrowserProxyEnabled"),
								  gSavedSettings.getString("BrowserProxyAddress"),
								  gSavedSettings.getS32("BrowserProxyPort"));

		if (mClearCache)
		{
			mClearCache = false;
			media_source->clear_cache();
		}

		// TODO: Only send cookies to plugins that need them. Ideally, the
		// plugin should tell us whether it handles cookies or not -- either
		// via the init response or through a separate message. Due to the
		// ordering of messages, it's possible we wouldn't get that information
		// back in time to send cookies before sending a navigate message,
		// which could cause odd race conditions.
		std::string all_cookies = LLViewerMedia::getCookieStore()->getAllCookies();
		LL_DEBUGS("Media") << "setting cookies: " << all_cookies << LL_ENDL;
		if (!all_cookies.empty())
		{
			media_source->set_cookies(all_cookies);
		}

		mMediaSource = media_source;
		return true;
	}

	mMediaSourceFailed = true;

	return false;
}

void LLViewerMediaImpl::setSize(int width, int height)
{
	mMediaWidth = width;
	mMediaHeight = height;
	if (mMediaSource)
	{
		mMediaSource->setSize(width, height);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::play()
{
	// first stop any previously playing media
	// stop();

	// mMediaSource->addObserver(this);
	if (mMediaSource == NULL)
	{
	 	if (!initializePlugin(mMimeType))
		{
			// Plugin failed initialization... should assert or something
			return;
		}
	}

	// updateMovieImage(mTextureId, true);

	mMediaSource->loadURI(mMediaURL);
	if (/*mMediaSource->pluginSupportsMediaTime()*/ true)
	{
		start();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::stop()
{
	if (mMediaSource)
	{
		mMediaSource->stop();
		// destroyMediaSource();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::pause()
{
	if (mMediaSource)
	{
		mMediaSource->pause();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::start()
{
	if (mMediaSource)
	{
		mMediaSource->start();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::seek(F32 time)
{
	if (mMediaSource)
	{
		mMediaSource->seek(time);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setVolume(F32 volume)
{
	if (mMediaSource)
	{
		mMediaSource->setVolume(volume);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::focus(bool focus)
{
	mHasFocus = focus;

	if (mMediaSource)
	{
		// call focus just for the hell of it, even though this apopears to be a nop
		mMediaSource->focus(focus);
		if (focus)
		{
			// spoof a mouse click to *actually* pass focus
			// Don't do this anymore -- it actually clicks through now.
//			mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, 1, 1, 0);
//			mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, 1, 1, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::hasFocus() const
{
	// FIXME: This might be able to be a bit smarter by hooking into LLViewerMediaFocus, etc.
	return mHasFocus;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::clearCache()
{
	if (mMediaSource)
	{
		mMediaSource->clear_cache();
	}
	else
	{
		mClearCache = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDown(S32 x, S32 y, MASK mask, S32 button)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOWN, button,
								 x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseUp(S32 x, S32 y, MASK mask, S32 button)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, button,
								 x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseMove(S32 x, S32 y, MASK mask)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_MOVE, 0,
								 x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//static 
void LLViewerMediaImpl::scaleTextureCoords(const LLVector2& texture_coords, S32 *x, S32 *y)
{
	F32 texture_x = texture_coords.mV[VX];
	F32 texture_y = texture_coords.mV[VY];

	// Deal with repeating textures by wrapping the coordinates into the range [0.0, 1.0)
	texture_x = fmodf(texture_x, 1.0f);
	if (texture_x < 0.0f) texture_x = 1.0 + texture_x;

	texture_y = fmodf(texture_y, 1.0f);
	if (texture_y < 0.0f) texture_y = 1.0 + texture_y;

	// scale x and y to texel units.
	*x = llround(texture_x * mMediaSource->getTextureWidth());
	*y = llround((1.0f - texture_y) * mMediaSource->getTextureHeight());

	// Adjust for the difference between the actual texture height and the amount of the texture in use.
	*y -= (mMediaSource->getTextureHeight() - mMediaSource->getHeight());
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDown(const LLVector2& texture_coords, MASK mask, S32 button)
{
	if (mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);
		mouseDown(x, y, mask, button);
	}
}

void LLViewerMediaImpl::mouseUp(const LLVector2& texture_coords, MASK mask, S32 button)
{
	if (mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);
		mouseUp(x, y, mask, button);
	}
}

void LLViewerMediaImpl::mouseMove(const LLVector2& texture_coords, MASK mask)
{
	if (mMediaSource)
	{
		S32 x, y;
		scaleTextureCoords(texture_coords, &x, &y);
		mouseMove(x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseDoubleClick(S32 x, S32 y, MASK mask, S32 button)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_DOUBLE_CLICK,
								 button, x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::scrollWheel(S32 x, S32 y, MASK mask)
{
	scaleMouse(&x, &y);
	mLastMouseX = x;
	mLastMouseY = y;
	if (mMediaSource)
	{
		mMediaSource->scrollEvent(x, y, mask);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::onMouseCaptureLost()
{
	if (mMediaSource)
	{
		mMediaSource->mouseEvent(LLPluginClassMedia::MOUSE_EVENT_UP, 0,
								 mLastMouseX, mLastMouseY, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
BOOL LLViewerMediaImpl::handleMouseUp(S32 x, S32 y, MASK mask) 
{ 
	// NOTE: this is called when the mouse is released when we have capture.
	// Due to the way mouse coordinates are mapped to the object, we can't
	// use the x and y coordinates that come in with the event.

	if (hasMouseCapture())
	{
		// Release the mouse -- this will also send a mouseup to the media
		gFocusMgr.setMouseCapture(FALSE);
	}

	return TRUE; 
}

//////////////////////////////////////////////////////////////////////////////////////////
std::string LLViewerMediaImpl::getName() const 
{ 
	if (mMediaSource)
	{
		return mMediaSource->getMediaName();
	}

	return LLStringUtil::null; 
};

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateHome()
{
	if (mMediaSource)
	{
		mMediaSource->loadURI(mHomeURL);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateTo(const std::string& _url,
								   const std::string& mime_type,
								   bool rediscover_type)
{
	// trim whitespace from front and back of URL - fixes EXT-5363
	std::string url(_url);
	LLStringUtil::trim(url);

	if (rediscover_type)
	{
		LLURI uri(url);
		std::string scheme = uri.scheme();

		llinfos << "Rediscovering media type for '" << url
				<< "' with former media source type '" << mime_type
				<< "' and new scheme '" << scheme << "'" << llendl;

		if (scheme.empty() || "http" == scheme || "https" == scheme)
		{
			// If we don't set an Accept header, LLHTTPClient will add one like this:
			//    Accept: application/llsd+xml
			// which is really not what we want.
			LLSD headers = LLSD::emptyMap();
			headers["Accept"] = "*/*";
			LLHTTPClient::getHeaderOnly(url, new LLMimeDiscoveryResponder(this),
										headers, 10.0f);
		}
		else if ("data" == scheme || "file" == scheme || "about" == scheme)
		{
			// FIXME: figure out how to really discover the type for these schemes
			// We use "data" internally for a text/html url for loading the login screen
			if (initializeMedia("text/html"))
			{
				mMediaSource->loadURI(url);
			}
		}
		else
		{
			// This catches 'rtsp://' urls
			if (initializeMedia(scheme))
			{
				mMediaSource->loadURI(url);
			}
		}
	}
	else if (mMediaSource)
	{
		mMediaSource->loadURI(url);
	}
	else if (initializeMedia(mime_type) && mMediaSource)
	{
		mMediaSource->loadURI(url);
	}
	else
	{
		LL_WARNS("Media") << "Couldn't navigate to: " << url
						  << " as there is no media type for: " << mime_type
						  << LL_ENDL;
		return;
	}
	mMediaURL = url;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::navigateStop()
{
	if (mMediaSource)
	{
		mMediaSource->browse_stop();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleKeyHere(KEY key, MASK mask)
{
	bool result = false;

	if (mMediaSource)
	{
		// FIXME: THIS IS SO WRONG.
		// Menu keys should be handled by the menu system and not passed to UI
		// elements, but this is how LLTextEditor and LLLineEditor do it...
		if (MASK_CONTROL & mask)
		{
			if (key == 'C')
			{
				mMediaSource->copy();
				result = true;
			}
			else if (key == 'V')
			{
				mMediaSource->paste();
				result = true;
			}
			else if (key == 'X')
			{
				mMediaSource->cut();
				result = true;
			}
		}

		if (!result)
		{
			LLSD native_key_data = LLSD::emptyMap(); 
			result = mMediaSource->keyEvent(LLPluginClassMedia::KEY_EVENT_DOWN,
											key, mask, native_key_data);
			// Since the viewer internal event dispatching doesn't give us
			// key-up events, simulate one here.
			(void)mMediaSource->keyEvent(LLPluginClassMedia::KEY_EVENT_UP,
										 key, mask, native_key_data);
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::handleUnicodeCharHere(llwchar uni_char)
{
	bool result = false;

	if (mMediaSource)
	{
		// only accept 'printable' characters, sigh...
		if (uni_char >= 32 &&	// discard 'control' characters
			uni_char != 127)	// SDL thinks this is 'delete'
		{
			LLSD native_key_data = LLSD::emptyMap(); 
			mMediaSource->textInput(wstring_to_utf8str(LLWString(1, uni_char)),
									gKeyboard->currentMask(FALSE),
									native_key_data);
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::canNavigateForward()
{
	BOOL result = FALSE;
	if (mMediaSource)
	{
		result = mMediaSource->getHistoryForwardAvailable();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::canNavigateBack()
{
	BOOL result = FALSE;
	if (mMediaSource)
	{
		result = mMediaSource->getHistoryBackAvailable();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateMovieImage(const LLUUID& uuid, BOOL active)
{
	// IF the media image hasn't changed, do nothing
	if (mTextureId == uuid)
	{
		return;
	}
	// If we have changed media uuid, restore the old one
	if (!mTextureId.isNull())
	{
		LLViewerTexture* oldImage = LLViewerTextureManager::findTexture(mTextureId);
		if (oldImage)
		{
			// HACK: downcast to LLViewerMediaTexture.
			// *TODO: fully implement LLViewerMediaTexture
			LLViewerMediaTexture* media_tex = (LLViewerMediaTexture*)oldImage;
			media_tex->reinit(mMovieImageHasMips);
			oldImage->mIsMediaTexture = FALSE;
		}
	}
	// If the movie is playing, set the new media image
	if (active && !uuid.isNull())
	{
		LLViewerTexture* viewerImage = LLViewerTextureManager::findTexture(uuid);
		if (viewerImage)
		{
			mTextureId = uuid;
			// Can't use mipmaps for movies because they don't update the
			// full image
			// HACK: downcast to LLViewerMediaTexture.
			// *TODO: fully implement LLViewerMediaTexture
			LLViewerMediaTexture* media_tex = (LLViewerMediaTexture*)viewerImage;
			mMovieImageHasMips = media_tex->getUseMipMaps();
			media_tex->reinit(FALSE);
			viewerImage->mIsMediaTexture = TRUE;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::update()
{
	if (mMediaSource == NULL)
	{
		return;
	}

	// If we didn't just create the impl, it may need to get cookie updates.
	if (!sUpdatedCookies.empty())
	{
		// TODO: Only send cookies to plugins that need them
		mMediaSource->set_cookies(sUpdatedCookies);
	}

	mMediaSource->idle();

	if (mMediaSource->isPluginExited())
	{
		destroyMediaSource();
		return;
	}

	if (!mMediaSource->textureValid() || mSuspendUpdates || !mVisible)
	{
		return;
	}

	LLViewerTexture* placeholder_image = updatePlaceholderImage();

	if (placeholder_image)
	{
		LLRect dirty_rect;
		if (mMediaSource->getDirty(&dirty_rect))
		{
			// Constrain the dirty rect to be inside the texture
			S32 x_pos = llmax(dirty_rect.mLeft, 0);
			S32 y_pos = llmax(dirty_rect.mBottom, 0);
			S32 width = llmin(dirty_rect.mRight, placeholder_image->getWidth()) - x_pos;
			S32 height = llmin(dirty_rect.mTop, placeholder_image->getHeight()) - y_pos;

			if (width > 0 && height > 0)
			{
				U8* data = mMediaSource->getBitsData();

				// Offset the pixels pointer to match x_pos and y_pos
				data += x_pos * mMediaSource->getTextureDepth() * mMediaSource->getBitsWidth();
				data += y_pos * mMediaSource->getTextureDepth();

				placeholder_image->setSubImage(data, 
											   mMediaSource->getBitsWidth(), 
											   mMediaSource->getBitsHeight(),
											   x_pos, y_pos, width, height,
											   TRUE);	// force a fast update (i.e. don't call analyzeAlpha, etc.)
			}

			mMediaSource->resetDirty();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::updateImagesMediaStreams()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// *TODO: Should return a LLViewerMediaTexture*
LLViewerTexture* LLViewerMediaImpl::updatePlaceholderImage()
{
	if (mTextureId.isNull())
	{
		// The code that created this instance will read from the plugin's bits.
		return NULL;
	}

	LLViewerMediaTexture* placeholder_image = (LLViewerMediaTexture*)LLViewerTextureManager::getFetchedTexture(mTextureId);

	if (placeholder_image &&
		(mNeedsNewTexture || placeholder_image->getUseMipMaps() ||
		 !placeholder_image->mIsMediaTexture ||
		  placeholder_image->getWidth() != mMediaSource->getTextureWidth() ||
		  placeholder_image->getHeight() != mMediaSource->getTextureHeight() ||
		  mTextureUsedWidth != mMediaSource->getWidth() ||
		  mTextureUsedHeight != mMediaSource->getHeight()))
	{
		llinfos << "initializing media placeholder" << llendl;
		llinfos << "movie image id " << mTextureId << llendl;

		U16 texture_width = mMediaSource->getTextureWidth();
		U16 texture_height = mMediaSource->getTextureHeight();
		S8 texture_depth = mMediaSource->getTextureDepth();

		// MEDIAOPT: check to see if size actually changed before doing work
		placeholder_image->destroyGLTexture();
		// MEDIAOPT: apparently just calling setUseMipMaps(FALSE) doesn't work?
		placeholder_image->reinit(FALSE);	// probably not needed

		// MEDIAOPT: seems insane that we actually have to make an imageraw then
		// immediately discard it
		LLPointer<LLImageRaw> raw = new LLImageRaw(texture_width,
												   texture_height,
												   texture_depth);
		raw->clear(int(mBackgroundColor.mV[VX] * 255.0f),
				   int(mBackgroundColor.mV[VY] * 255.0f),
				   int(mBackgroundColor.mV[VZ] * 255.0f),
				   0xff);

		// ask media source for correct GL image format constants
		placeholder_image->setExplicitFormat(mMediaSource->getTextureFormatInternal(),
											 mMediaSource->getTextureFormatPrimary(),
											 mMediaSource->getTextureFormatType(),
											 mMediaSource->getTextureFormatSwapBytes());

		placeholder_image->createGLTexture(0, raw);	// 0 discard

		// placeholder_image->setExplicitFormat()
		placeholder_image->setUseMipMaps(FALSE);

		// MEDIAOPT: set this dynamically on play/stop
		placeholder_image->mIsMediaTexture = true;
		mNeedsNewTexture = false;

		// If the amount of the texture being drawn by the media goes down in
		// either width or height, recreate the texture to avoid leaving parts
		// of the old image behind.
		mTextureUsedWidth = mMediaSource->getWidth();
		mTextureUsedHeight = mMediaSource->getHeight();
	}

	return placeholder_image;
}

//////////////////////////////////////////////////////////////////////////////////////////
LLUUID LLViewerMediaImpl::getMediaTextureID()
{
	return mTextureId;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setVisible(bool visible)
{
	mVisible = visible;

	if (mVisible)
	{
		if (mMediaSource && mMediaSource->isPluginExited())
		{
			destroyMediaSource();
		}

		if (!mMediaSource)
		{
			createMediaSource();
		}
	}

	if (mMediaSource)
	{
		mMediaSource->setPriority(mVisible ?
								  LLPluginClassMedia::PRIORITY_NORMAL :
								  LLPluginClassMedia::PRIORITY_HIDDEN);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::mouseCapture()
{
	gFocusMgr.setMouseCapture(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::getTextureSize(S32 *texture_width, S32 *texture_height)
{
	if (mMediaSource && mMediaSource->textureValid())
	{
		S32 real_texture_width = mMediaSource->getBitsWidth();
		S32 real_texture_height = mMediaSource->getBitsHeight();

		{
			// The "texture width" coming back from the plugin may not be a power of two (thanks to webkit).
			// It will be the correct "data width" to pass to setSubImage
			S32 i;

			for (i = 1; i < real_texture_width; i <<= 1)
				;
			*texture_width = i;

			for (i = 1; i < real_texture_height; i <<= 1)
				;
			*texture_height = i;
		}
	}
	else
	{
		*texture_width = 0;
		*texture_height = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::scaleMouse(S32 *mouse_x, S32 *mouse_y)
{
#if 0
	S32 media_width, media_height;
	S32 texture_width, texture_height;
	getMediaSize(&media_width, &media_height);
	getTextureSize(&texture_width, &texture_height);
	S32 y_delta = texture_height - media_height;

	*mouse_y -= y_delta;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaPlaying()
{
	bool result = false;

	if (mMediaSource)
	{
		EMediaStatus status = mMediaSource->getStatus();
		if (status == MEDIA_PLAYING || status == MEDIA_LOADING)
			result = true;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool LLViewerMediaImpl::isMediaPaused()
{
	return (mMediaSource && mMediaSource->getStatus() == MEDIA_PAUSED);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
bool LLViewerMediaImpl::hasMedia()
{
	return mMediaSource != NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void LLViewerMediaImpl::resetPreviousMediaState()
{
	mPreviousMediaState = MEDIA_NONE;
}

//////////////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::handleMediaEvent(LLPluginClassMedia* plugin,
										 LLPluginClassMediaOwner::EMediaEvent event)
{
	bool pass_through = true;
	switch (event)
	{
		case MEDIA_EVENT_CLICK_LINK_NOFOLLOW:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_CLICK_LINK_NOFOLLOW, uri is: "
							   << plugin->getClickURL() << LL_ENDL;
			// NOTE: this is dealt with in LLMediaCtrl::handleMediaEvent()
			// since if dealt from here, we won't be able to pass the media
			// control to LLURLDispatcher::dispatch()
			break;
		}

		case MEDIA_EVENT_CLICK_LINK_HREF:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_HREF, target is \"" << plugin->getClickTarget() << "\", uri is " << plugin->getClickURL() << LL_ENDL;
			// retrieve the event parameters
			std::string url = plugin->getClickURL();
			std::string target = plugin->getClickTarget();
//			std::string uuid = plugin->getClickUUID();
			// loadURL now handles distinguishing between _blank, _external, and other named targets.
			LLWeb::loadURL(url, target);
			break;
		}

		case MEDIA_EVENT_PLUGIN_FAILED_LAUNCH:
		{
			// The plugin failed to load properly.  Make sure the timer doesn't retry.
			// TODO: maybe mark this plugin as not loadable somehow?
			mMediaSourceFailed = true;

			// Reset the last known state of the media to defaults.
			resetPreviousMediaState();

			// TODO: may want a different message for this case?
			LLSD args;
			args["PLUGIN"] = LLMIMETypes::implType(mCurrentMimeType);
			LLNotifications::instance().add("MediaPluginFailed", args);
			break;
		}

		case MEDIA_EVENT_PLUGIN_FAILED:
		{
			// The plugin crashed.
			mMediaSourceFailed = true;

			// Reset the last known state of the media to defaults.
			resetPreviousMediaState();

			// SJB: This is getting called every frame if the plugin fails to
			// load, continuously respawining the alert!
			/*
			LLSD args;
			args["PLUGIN"] = LLMIMETypes::implType(mMimeType);
			LLNotifications::instance().add("MediaPluginFailed", args);
			*/
			break;
		}

		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_BEGIN:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_NAVIGATE_BEGIN, uri is: "
							   << plugin->getNavigateURI() << LL_ENDL;

			if (getNavState() == MEDIANAVSTATE_SERVER_SENT)
			{
				setNavState(MEDIANAVSTATE_SERVER_BEGUN);
			}
			else
			{
				setNavState(MEDIANAVSTATE_BEGUN);
			}
			break;
		}

		case LLViewerMediaObserver::MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_NAVIGATE_COMPLETE, uri is: "
							   << plugin->getNavigateURI() << LL_ENDL;

			std::string url = plugin->getNavigateURI();
			if (getNavState() == MEDIANAVSTATE_BEGUN)
			{
				if (mCurrentMediaURL == url)
				{
					// This is a navigate that takes us to the same url as the previous navigate.
					setNavState(MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS);
				}
				else
				{
					mCurrentMediaURL = url;
					setNavState(MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED);
				}
			}
			else if (getNavState() == MEDIANAVSTATE_SERVER_BEGUN)
			{
				mCurrentMediaURL = url;
				setNavState(MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED);
			}
			// all other cases need to leave the state alone.
			break;
		}

		case LLViewerMediaObserver::MEDIA_EVENT_LOCATION_CHANGED:
		{
			LL_DEBUGS("Media") << "MEDIA_EVENT_LOCATION_CHANGED, uri is: "
							   << plugin->getLocation() << LL_ENDL;

			std::string url = plugin->getLocation();

			if (getNavState() == MEDIANAVSTATE_BEGUN)
			{
				if (mCurrentMediaURL == url)
				{
					// This is a navigate that takes us to the same url as the
					// previous navigate.
					setNavState(MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS);
				}
				else
				{
					mCurrentMediaURL = url;
					setNavState(MEDIANAVSTATE_FIRST_LOCATION_CHANGED);
				}
			}
			else if (getNavState() == MEDIANAVSTATE_SERVER_BEGUN)
			{
				mCurrentMediaURL = url;
				setNavState(MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED);
			}
			else
			{
				// Don't track redirects.
				setNavState(MEDIANAVSTATE_NONE);
			}
			break;
		}

		case LLViewerMediaObserver::MEDIA_EVENT_PICK_FILE_REQUEST:
		{
			// Display a file picker
			LLFilePicker& picker = LLFilePicker::instance();
			(void)picker.getOpenFile(LLFilePicker::FFLOAD_ALL);
			plugin->sendPickFileResponse(picker.getFirstFile());
			break;
		}

		case LLViewerMediaObserver::MEDIA_EVENT_CLOSE_REQUEST:
		{
			std::string uuid = plugin->getClickUUID();

			llinfos << "MEDIA_EVENT_CLOSE_REQUEST for uuid " << uuid << llendl;

			if (uuid.empty())
			{
				// This close request is directed at this instance, let it fall
				// through.
			}
			else
			{
				// This close request is directed at another instance
				pass_through = false;
				//TODO: LLFloaterMediaBrowser::closeRequest(uuid);
			}
			break;
		}

		case LLViewerMediaObserver::MEDIA_EVENT_GEOMETRY_CHANGE:
		{
			std::string uuid = plugin->getClickUUID();

			llinfos << "MEDIA_EVENT_GEOMETRY_CHANGE for uuid " << uuid << llendl;

			if (uuid.empty())
			{
				// This geometry change request is directed at this instance,
				// let it fall through.
			}
			else
			{
				// This request is directed at another instance
				pass_through = false;
				//TODO: LLFloaterMediaBrowser::geometryChanged(uuid, plugin->getGeometryX(), plugin->getGeometryY(), plugin->getGeometryWidth(), plugin->getGeometryHeight());
			}
			break;
		}

		default:
			break;
	}

	if (pass_through)
	{
		// Just chain the event to observers.
		emitEvent(plugin, event);
	}
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void LLViewerMediaImpl::handleCookieSet(LLPluginClassMedia* self, const std::string &cookie)
{
	LLViewerMedia::getCookieStore()->setCookies(cookie);
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void LLViewerMediaImpl::cut()
{
	if (mMediaSource)
		mMediaSource->cut();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL LLViewerMediaImpl::canCut() const
{
	if (mMediaSource)
		return mMediaSource->canCut();
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void LLViewerMediaImpl::copy()
{
	if (mMediaSource)
		mMediaSource->copy();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL LLViewerMediaImpl::canCopy() const
{
	if (mMediaSource)
		return mMediaSource->canCopy();
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void LLViewerMediaImpl::paste()
{
	if (mMediaSource)
		mMediaSource->paste();
}

////////////////////////////////////////////////////////////////////////////////
// virtual
BOOL LLViewerMediaImpl::canPaste() const
{
	if (mMediaSource)
		return mMediaSource->canPaste();
	else
		return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
void LLViewerMediaImpl::setBackgroundColor(LLColor4 color)
{
	mBackgroundColor = color; 

	if (mMediaSource)
	{
		mMediaSource->setBackgroundColor(mBackgroundColor);
	}
};

void LLViewerMediaImpl::setNavState(EMediaNavState state)
{
	mMediaNavState = state;

	switch (state) 
	{
		case MEDIANAVSTATE_NONE: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_NONE" << llendl; break;
		case MEDIANAVSTATE_BEGUN: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_BEGUN" << llendl; break;
		case MEDIANAVSTATE_FIRST_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_FIRST_LOCATION_CHANGED" << llendl; break;
		case MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS" << llendl; break;
		case MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED" << llendl; break;
		case MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS" << llendl; break;
		case MEDIANAVSTATE_SERVER_SENT: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_SENT" << llendl; break;
		case MEDIANAVSTATE_SERVER_BEGUN: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_BEGUN" << llendl; break;
		case MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED" << llendl; break;
		case MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED: LL_DEBUGS("Media") << "Setting nav state to MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED" << llendl; break;
	}
}
