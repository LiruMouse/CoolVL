/** 
 * @file llfloaterabout.cpp
 * @author James Cook
 * @brief The about box from Help->About
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

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

#include "llfloaterabout.h"

#include "llaudioengine.h"
#include "llcurl.h"
#include "llimagej2c.h"
#include "llsecondlifeurls.h"
#include "llsys.h"
#include "llui.h"				// for tr()
#include "lluictrlfactory.h"
#include "lluri.h"
#include "llversionviewer.h"

#include "llagent.h"
#include "llappviewer.h" 
#include "llmediactrl.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llweb.h"
#include "llwindow.h"

extern LLMemoryInfo gSysMemory;
extern U32 gPacketsIn;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

LLFloaterAbout* LLFloaterAbout::sInstance = NULL;

static std::string get_viewer_release_notes_url();

///----------------------------------------------------------------------------
/// Class LLServerReleaseNotesURLFetcher
///----------------------------------------------------------------------------
class LLServerReleaseNotesURLFetcher : public LLHTTPClient::Responder
{
	LOG_CLASS(LLServerReleaseNotesURLFetcher);
public:

	static void startFetch();
	/*virtual*/ void completedHeader(U32 status,
									 const std::string& reason,
									 const LLSD& content);
	/*virtual*/ void completedRaw(U32 status,
								  const std::string& reason,
								  const LLChannelDescriptors& channels,
								  const LLIOPipe::buffer_ptr_t& buffer);
};

///----------------------------------------------------------------------------
/// Class LLFloaterAbout
///----------------------------------------------------------------------------

// Default constructor
LLFloaterAbout::LLFloaterAbout() 
:	LLFloater(std::string("floater_about"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_about.xml");
	sInstance = this;
}

// Destroys the object
LLFloaterAbout::~LLFloaterAbout()
{
	sInstance = NULL;
}

BOOL LLFloaterAbout::postBuild()
{
	center();

	childSetAction("copy_button", onClickCopyToClipboard, this);
	childSetAction("close_button", onClickClose, this);

	LLViewerTextEditor* text = getChild<LLViewerTextEditor>("credits", true);
	text->setCursorPos(0);
	text->setEnabled(FALSE);
	text->setTakesFocus(TRUE);
	text->setHandleEditKeysDirectly(TRUE);

	text = getChild<LLViewerTextEditor>("licenses", true);
	text->setCursorPos(0);
	text->setEnabled(FALSE);
	text->setTakesFocus(TRUE);
	text->setHandleEditKeysDirectly(TRUE);

	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("tos");
	if (web_browser)
	{
		web_browser->navigateToLocalPage("tpv", "policy.html");
	}

	std::string support_url;
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		std::string url = region->getCapability("ServerReleaseNotes");
		if (!url.empty())
		{
			if (url.find("/cap/") != std::string::npos)
			{
				// The URL is itself a capability URL: start fetching the
				// actual server release notes URL
				LL_DEBUGS("About") << "Fetching release notes URL from cap: "
								   << url << LL_ENDL;
				LLServerReleaseNotesURLFetcher::startFetch();
				support_url = LLTrans::getString("RetrievingData");
			}
			else
			{
				// On OpenSim grids, we could still get a direct URL
				LL_DEBUGS("About") << "Got release notes URL: "
								   << url << LL_ENDL;
				support_url = url;
			}
		}
	}
	setSupportText(support_url);

	return TRUE;
}

void LLFloaterAbout::updateServerReleaseNotesURL(const std::string& url)
{
	setSupportText(url);
}

static std::string get_viewer_release_notes_url()
{
	std::ostringstream version;
	version << LL_VERSION_MAJOR << "."
			<< LL_VERSION_MINOR << "."
			<< LL_VERSION_PATCH << "."
			<< LL_VERSION_BUILD;

	LLSD query;
	query["channel"] = gSavedSettings.getString("VersionChannelName");
	query["version"] = version.str();

	std::ostringstream url;
	url << RELEASE_NOTES_BASE_URL << LLURI::mapToQueryString(query);

	return url.str();
}

void LLFloaterAbout::setSupportText(const std::string& server_release_notes_url)
{
	LLColor4 fg_color = gColors.getColor("TextFgReadOnlyColor");
	LLViewerTextEditor* support_widget = getChild<LLViewerTextEditor>("support",
																	  true);

	// We need to prune the highlights, and clear() is not doing it...
	support_widget->removeTextFromEnd(support_widget->getMaxLength());
	// For some reason, adding style doesn't work unless this is true.
	support_widget->setParseHTML(TRUE);

	// Text styles for release notes hyperlinks
	LLStyleSP viewer_link_style(new LLStyle);
	viewer_link_style->setVisible(true);
	viewer_link_style->setFontName(LLStringUtil::null);
	viewer_link_style->setLinkHREF(get_viewer_release_notes_url());
	viewer_link_style->setColor(gSavedSettings.getColor4("HTMLLinkColor"));

	// Version string
	std::string version = LLAppViewer::instance()->getSecondLifeTitle() +
						  llformat(" %d.%d.%d (%d) %s %s (%s)\n",
						  		   LL_VERSION_MAJOR, LL_VERSION_MINOR,
								   LL_VERSION_PATCH, LL_VERSION_BUILD,
								   __DATE__, __TIME__,
								   gSavedSettings.getString("VersionChannelName").c_str());
//MK
	if (gRRenabled)
	{
		version += gAgent.mRRInterface.getVersion2 () + "\n";
	}
//mk
	support_widget->appendColoredText(version, false, false, fg_color);
	support_widget->appendStyledText(LLTrans::getString("ReleaseNotes"), false,
									 false, viewer_link_style);

	std::string support;
	support.append("\n\n");

#if LL_MSVC
    support.append(llformat("Built with MSVC version %d\n\n", _MSC_VER));
#endif

#if LL_GNUC
    support.append(llformat("Built with GCC version %d\n\n", GCC_VERSION));
#endif

	// Position
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		LLStyleSP server_link_style(new LLStyle);
		if (server_release_notes_url.find("http") == 0)
		{
			server_link_style->setVisible(true);
			server_link_style->setFontName(LLStringUtil::null);
			server_link_style->setLinkHREF(server_release_notes_url);
			server_link_style->setColor(gSavedSettings.getColor4("HTMLLinkColor"));
		}

		const LLVector3d &pos = gAgent.getPositionGlobal();
		LLUIString pos_text = getString("you_are_at");
		pos_text.setArg("[POSITION]",
						llformat("%.1f, %.1f, %.1f ", pos.mdV[VX], pos.mdV[VY],
								 pos.mdV[VZ]));
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
		{
			pos_text = "(Position hidden)\n";
		}
//mk
		support.append(pos_text);

		std::string region_text = llformat("in %s located at ",
										gAgent.getRegion()->getName().c_str());
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
		{
			region_text = "(Region hidden)\n";
		}
//mk
		support.append(region_text);

//MK
		if (!gRRenabled || !gAgent.mRRInterface.mContainsShowloc)
		{
//mk
			std::string buffer;
			buffer = gAgent.getRegion()->getHost().getHostName();
			support.append(buffer);
			support.append(" (");
			buffer = gAgent.getRegion()->getHost().getString();
			support.append(buffer);
			support.append(")\n");
			support.append(gLastVersionChannel);
			support.append("\n");
//MK
		}
		else
		{
			support.append("(Server info hidden)\n");
		}
//mk
		support_widget->appendColoredText(support, false, false, fg_color);

		if (!server_release_notes_url.empty())
		{
			std::string text;
			if (server_release_notes_url.find("http") == 0)
			{
				text = LLTrans::getString("ReleaseNotes") + "\n";
				support_widget->appendStyledText(text, false, false,
												 server_link_style);
			}
			else
			{
				text = LLTrans::getString("ReleaseNotes") + ": " +
					   server_release_notes_url + "\n";
				support_widget->appendColoredText(text, false, false, fg_color);
			}
		}
	}
	else
	{
		support_widget->appendColoredText(" \n", false, false, fg_color);
	}

	// *NOTE: Do not translate text like GPU, Graphics Card, etc -
	//  Most PC users that know what these mean will be used to the english
	//  versions and this info sometimes gets sent to support

	// CPU
	support = "CPU: ";
	support.append(gSysCPU.getCPUString());
	support.append("\n");

	U32 memory = gSysMemory.getPhysicalMemoryKB() / 1024;
	// Moved hack adjustment to Windows memory size into llsys.cpp

	std::string mem_text = llformat("Memory: %u MB\n", memory);
	support.append(mem_text);

	support.append("OS Version: ");
	support.append(LLAppViewer::instance()->getOSInfo().getOSString());
	support.append("\n");

	support.append("Graphics Card Vendor: ");
	support.append((const char*) glGetString(GL_VENDOR));
	support.append("\n");

	support.append("Graphics Card: ");
	support.append((const char*) glGetString(GL_RENDERER));
	support.append("\n");

#if LL_WINDOWS
    getWindow()->incBusyCount();
    getWindow()->setCursor(UI_CURSOR_ARROW);
    support.append("Windows Graphics Driver Version: ");
    LLSD driver_info = gDXHardware.getDisplayInfo();
    if (driver_info.has("DriverVersion"))
    {
        support.append(driver_info["DriverVersion"]);
    }
    support.append("\n");
    getWindow()->decBusyCount();
    getWindow()->setCursor(UI_CURSOR_ARROW);
#endif

	support.append("OpenGL Version: ");
	support.append((const char*) glGetString(GL_VERSION));
	support.append("\n");

	support.append("\n");

	support.append("libcurl Version: ");
	support.append(LLCurl::getVersionString());
	support.append("\n");

	support.append("J2C Decoder Version: ");
	support.append(LLImageJ2C::getEngineInfo());
	support.append("\n");

	support.append("Audio Driver Version: ");
	bool want_fullname = true;
	support.append(gAudiop ? gAudiop->getDriverName(want_fullname) : "(none)");
	support.append("\n");

	// TODO: Implement media plugin version query
	support.append("Qt Webkit Version: 4.7.1 (version number hard-coded)\n");

	if (gPacketsIn > 0)
	{
		LLViewerStats* stats = LLViewerStats::getInstance();
		std::string packet_loss = llformat("Packets Lost: %.0f/%.0f (%.1f%%)",
										   stats->mPacketsLostStat.getCurrent(),
										   F32(gPacketsIn),
										   100.f * stats->mPacketsLostStat.getCurrent() / F32(gPacketsIn));
		support.append(packet_loss);
		support.append("\n");
	}

	support_widget->appendColoredText(support, false, true, fg_color);

	// Fix views
	support_widget->setCursorPos(0);
	support_widget->setEnabled(FALSE);
	support_widget->setTakesFocus(TRUE);
	support_widget->setHandleEditKeysDirectly(TRUE);
}

// static
void LLFloaterAbout::show(void*)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterAbout();
	}

	sInstance->open();	 /*Flawfinder: ignore*/
}

// static
void LLFloaterAbout::onClickCopyToClipboard(void* userdata)
{
	LLFloaterAbout* self = (LLFloaterAbout*)userdata;
	LLViewerTextEditor *support = self->getChild<LLViewerTextEditor>("support",
																	 true);
	support->selectAll();
	support->copy();
	support->deselect();
}

// static
void LLFloaterAbout::onClickClose(void* userdata)
{
	LLFloaterAbout* self = (LLFloaterAbout*)userdata;
	self->close();
}

///----------------------------------------------------------------------------
/// Class LLServerReleaseNotesURLFetcher implementation
///----------------------------------------------------------------------------
// static
void LLServerReleaseNotesURLFetcher::startFetch()
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

	// We cannot display the URL returned by the ServerReleaseNotes capability
	// because opening it in an external browser will trigger a warning about
	// untrusted SSL certificate.
	// So we query the URL ourselves, expecting to find
	// an URL suitable for external browsers in the "Location:" HTTP header.
	std::string cap_url = region->getCapability("ServerReleaseNotes");
	LLHTTPClient::get(cap_url, new LLServerReleaseNotesURLFetcher);
}

// virtual
void LLServerReleaseNotesURLFetcher::completedHeader(U32 status,
													 const std::string& reason,
													 const LLSD& content)
{
	LL_DEBUGS("About") << "Status: " << status
					   << " - Reason: " << reason
					   << " - Headers: " << content << LL_ENDL;

	LLFloaterAbout* floater_about = LLFloaterAbout::getInstance();
	if (floater_about)
	{
		std::string location = content["location"].asString();
		if (location.empty())
		{
			location = floater_about->getString("ErrorFetchingServerReleaseNotesURL");
		}
		floater_about->updateServerReleaseNotesURL(location);
	}
}

// virtual
void LLServerReleaseNotesURLFetcher::completedRaw(U32 status,
												  const std::string& reason,
												  const LLChannelDescriptors& channels,
												  const LLIOPipe::buffer_ptr_t& buffer)
{
	// Do nothing.
	// We're overriding just because the base implementation tries to
	// deserialize LLSD which triggers warnings.
}
