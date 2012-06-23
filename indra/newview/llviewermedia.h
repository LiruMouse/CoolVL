/**
 * @file llviewermedia.h
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

#ifndef LLVIEWERMEDIA_H
#define LLVIEWERMEDIA_H

#include "llfocusmgr.h"
#include "llpanel.h"
#include "llpluginclassmediaowner.h"
#include "v4color.h"

#include "llurl.h"
#include "llviewermediaobserver.h"

class LLUUID;
class LLViewerMediaImpl;
class LLViewerTexture;

typedef LLPointer<LLViewerMediaImpl> viewer_media_t;
///////////////////////////////////////////////////////////////////////////////
//
class LLViewerMediaEventEmitter
{
public:
	virtual ~LLViewerMediaEventEmitter();

	bool addObserver( LLViewerMediaObserver* subject );
	bool remObserver( LLViewerMediaObserver* subject );
	void emitEvent(LLPluginClassMedia* self, LLPluginClassMediaOwner::EMediaEvent event);

private:
	typedef std::list< LLViewerMediaObserver* > observerListType;
	observerListType mObservers;
};

class LLViewerMedia
{
	LOG_CLASS(LLViewerMedia);
public:
	// Special case early init for just web browser component
	// so we can show login screen.  See .cpp file for details. JC

	static viewer_media_t newMediaImpl(const std::string& media_url,
									   const LLUUID& texture_id,
									   S32 media_width, 
									   S32 media_height, 
									   U8 media_auto_scale,
									   U8 media_loop,
									   std::string mime_type = "none/none");

	static void removeMedia(LLViewerMediaImpl* media);
	static LLViewerMediaImpl* getMediaImplFromTextureID(const LLUUID& texture_id);
	static std::string getCurrentUserAgent();
	static void updateBrowserUserAgent();
	static bool handleSkinCurrentChanged(const LLSD& /*newvalue*/);
	static bool textureHasMedia(const LLUUID& texture_id);
	static void setVolume(F32 volume);

	static void updateMedia();

	static void cleanupClass();

	// Clear all cookies for all plugins
	static void clearAllCookies();

	// Clear all plugins' caches
	static void clearAllCaches();

	// Set the "cookies enabled" flag for all loaded plugins
	static void setCookiesEnabled(bool enabled);

	// Set the proxy config for all loaded plugins
	static void setProxyConfig(bool enable, const std::string &host, int port);

	static LLPluginCookieStore *getCookieStore();
	static void loadCookieFile();
	static void saveCookieFile();
	static void addCookie(const std::string &name,
						  const std::string &value,
						  const std::string &domain,
						  const LLDate &expires,
						  const std::string &path = std::string("/"),
						  bool secure = false);
	static void addSessionCookie(const std::string &name,
								 const std::string &value,
								 const std::string &domain,
								 const std::string &path = std::string("/"),
								 bool secure = false);
	static void removeCookie(const std::string &name,
							 const std::string &domain,
							 const std::string &path = std::string("/"));

	static void openIDSetup(const std::string &openid_url, const std::string &openid_token);
	static void openIDCookieResponse(const std::string &cookie);

	static LLSD getHeaders();

private:
	static void setOpenIDCookie();

	static LLPluginCookieStore *sCookieStore;
	static LLURL sOpenIDURL;
	static std::string sOpenIDCookie;
};

// Implementation functions not exported into header file
class LLViewerMediaImpl
	:	public LLMouseHandler, public LLRefCount, public LLPluginClassMediaOwner, public LLViewerMediaEventEmitter, public LLEditMenuHandler
{
	LOG_CLASS(LLViewerMediaImpl);
public:

	LLViewerMediaImpl(const std::string& media_url,
		const LLUUID& texture_id,
		S32 media_width, 
		S32 media_height, 
		U8 media_auto_scale,
		U8 media_loop,
		const std::string& mime_type);

	~LLViewerMediaImpl();
	void createMediaSource();
	void destroyMediaSource();
	void setMediaType(const std::string& media_type);
	bool initializeMedia(const std::string& mime_type);
	bool initializePlugin(const std::string& media_type);
	LLPluginClassMedia* getMediaPlugin() { return mMediaSource; }
	void setSize(int width, int height);

	void play();
	void stop();
	void pause();
	void start();
	void seek(F32 time);
	void setVolume(F32 volume);
	void focus(bool focus);
	// True if the impl has user focus.
	bool hasFocus() const;
	void mouseDown(S32 x, S32 y, MASK mask, S32 button = 0);
	void mouseUp(S32 x, S32 y, MASK mask, S32 button = 0);
	void mouseMove(S32 x, S32 y, MASK mask);
	void mouseDown(const LLVector2& texture_coords, MASK mask, S32 button = 0);
	void mouseUp(const LLVector2& texture_coords, MASK mask, S32 button = 0);
	void mouseMove(const LLVector2& texture_coords, MASK mask);
	void mouseDoubleClick(S32 x, S32 y, MASK mask, S32 button = 0);
	void scrollWheel(S32 x, S32 y, MASK mask);
	void mouseCapture();

	void navigateHome();
	void navigateTo(const std::string& url, const std::string& mime_type = "", bool rediscover_type = false);
	void navigateStop();
	bool handleKeyHere(KEY key, MASK mask);
	bool handleUnicodeCharHere(llwchar uni_char);
	bool canNavigateForward();
	bool canNavigateBack();
	std::string getMediaURL() { return mMediaURL; }
	std::string getMediaHomeURL() { return mHomeURL; }
	void clearCache();
	std::string getMimeType() { return mMimeType; }
	void getTextureSize(S32 *texture_width, S32 *texture_height);
	void scaleMouse(S32 *mouse_x, S32 *mouse_y);
	void scaleTextureCoords(const LLVector2& texture_coords, S32 *x, S32 *y);

	void update();
	void updateMovieImage(const LLUUID& image_id, BOOL active);
	void updateImagesMediaStreams();
	LLUUID getMediaTextureID();

	void suspendUpdates(bool suspend) { mSuspendUpdates = suspend; };
	bool getVisible() const { return mVisible; }
	bool isVisible() const { return mVisible; }
	void setVisible(bool visible);

	bool isMediaPlaying();
	bool isMediaPaused();
	bool hasMedia();

	void resetPreviousMediaState();

	void setTarget(const std::string& target) { mTarget = target; }

	// utility function to create a ready-to-use media instance from a desired media type.
	static LLPluginClassMedia* newSourceFromMediaType(std::string media_type,
													  LLPluginClassMediaOwner *owner, /* may be NULL */
													  S32 default_width, S32 default_height,
													  const std::string target = LLStringUtil::null);

	// Internally set our desired browser user agent string, including
	// the Second Life version and skin name.  Used because we can
	// switch skins without restarting the app.
	static void updateBrowserUserAgent();

	// Callback for when the SkinCurrent control is changed to
	// switch the user agent string to indicate the new skin.
	static bool handleSkinCurrentChanged(const LLSD& newvalue);

	// need these to handle mouseup...
	/*virtual*/ void	onMouseCaptureLost();
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

	// Grr... the only thing I want as an LLMouseHandler are the onMouseCaptureLost and handleMouseUp calls.
	// Sadly, these are all pure virtual, so I have to supply implementations here:
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks) { return FALSE; };
	/*virtual*/ BOOL	handleDoubleClick(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen) { return FALSE; };
	/*virtual*/ BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask) { return FALSE; };
	/*virtual*/ BOOL	handleMiddleMouseUp(S32 x, S32 y, MASK mask) {return FALSE; };
	/*virtual*/ std::string getName() const;
	/*virtual*/ BOOL isView() const { return FALSE; };
	/*virtual*/ void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const {};
	/*virtual*/ void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const {};
	/*virtual*/ BOOL hasMouseCapture() { return gFocusMgr.getMouseCapture() == this; };

	// Inherited from LLPluginClassMediaOwner
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, LLPluginClassMediaOwner::EMediaEvent);
	/*virtual*/ void handleCookieSet(LLPluginClassMedia* self, const std::string &cookie);

	// LLEditMenuHandler overrides
	/*virtual*/ void	cut();
	/*virtual*/ BOOL	canCut() const;

	/*virtual*/ void	copy();
	/*virtual*/ BOOL	canCopy() const;

	/*virtual*/ void	paste();
	/*virtual*/ BOOL	canPaste() const;

	void setBackgroundColor(LLColor4 color);

	bool isTrustedBrowser() { return mTrustedBrowser; }
	void setTrustedBrowser(bool trusted) { mTrustedBrowser = trusted; }

	typedef enum 
	{
		MEDIANAVSTATE_NONE,										// State is outside what we need to track for navigation.
		MEDIANAVSTATE_BEGUN,									// a MEDIA_EVENT_NAVIGATE_BEGIN has been received which was not server-directed
		MEDIANAVSTATE_FIRST_LOCATION_CHANGED,					// first LOCATION_CHANGED event after a non-server-directed BEGIN
		MEDIANAVSTATE_FIRST_LOCATION_CHANGED_SPURIOUS,			// Same as above, but the new URL is identical to the previously navigated URL.
		MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED,			// we received a NAVIGATE_COMPLETE event before the first LOCATION_CHANGED
		MEDIANAVSTATE_COMPLETE_BEFORE_LOCATION_CHANGED_SPURIOUS,// Same as above, but the new URL is identical to the previously navigated URL.
		MEDIANAVSTATE_SERVER_SENT,								// server-directed nav has been requested, but MEDIA_EVENT_NAVIGATE_BEGIN hasn't been received yet
		MEDIANAVSTATE_SERVER_BEGUN,								// MEDIA_EVENT_NAVIGATE_BEGIN has been received which was server-directed
		MEDIANAVSTATE_SERVER_FIRST_LOCATION_CHANGED,			// first LOCATION_CHANGED event after a server-directed BEGIN
		MEDIANAVSTATE_SERVER_COMPLETE_BEFORE_LOCATION_CHANGED	// we received a NAVIGATE_COMPLETE event before the first LOCATION_CHANGED
		
	} EMediaNavState;

	// Returns the current nav state of the media.
	// note that this will be updated BEFORE listeners and objects receive media messages 
	EMediaNavState getNavState() { return mMediaNavState; }
	void setNavState(EMediaNavState state);
	
public:
	// a single media url with some data and an impl.
	LLPluginClassMedia* mMediaSource;
	LLUUID mTextureId;
	bool  mMovieImageHasMips;
	std::string mMediaURL;
	std::string mHomeURL;
	std::string mMimeType;
	std::string mCurrentMediaURL;	// The most current media url from the plugin (via the "location changed" or "navigate complete" events).
	std::string mCurrentMimeType;	// The MIME type that caused the currently loaded plugin to be loaded.
	S32 mLastMouseX;				// save the last mouse coord we get, so when we lose capture we can simulate a mouseup at that point.
	S32 mLastMouseY;
	S32 mMediaWidth;
	S32 mMediaHeight;
	bool mMediaAutoScale;
	bool mMediaLoop;
	bool mNeedsNewTexture;
	S32 mTextureUsedWidth;
	S32 mTextureUsedHeight;
	bool mSuspendUpdates;
	bool mVisible;
	EMediaNavState mMediaNavState;
	bool mHasFocus;
	bool mMediaSourceFailed;
	int mPreviousMediaState;
	bool mClearCache;
	LLColor4 mBackgroundColor;
	bool mTrustedBrowser;
	std::string mTarget;

private:
	LLViewerTexture* updatePlaceholderImage(); // Should really return a LLViewerMediaTexture*
};

#endif	// LLVIEWERMEDIA_H
