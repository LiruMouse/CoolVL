/** 
 * @file llfloaterchat.cpp
 * @brief LLFloaterChat class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

/**
 * Actually the "Chat History" floater.
 * Should be llfloaterchathistory, not llfloaterchat.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterchat.h"

// linden library includes
#include "llaudioengine.h"
#include "llbutton.h"
#include "llcachename.h"
#include "llchat.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llerror.h"
#include "llfontgl.h"
#include "llstring.h"
#include "lltextparser.h"
#include "llwindow.h"
#include "message.h"

// project include
#include "llagent.h"
#include "llchatbar.h"
#include "llconsole.h"
#include "llfloateractivespeakers.h"
#include "llfloaterchatterbox.h"
#include "llfloaterhtml.h"
#include "llfloatermute.h"
#include "llfloaterscriptdebug.h"
#include "llkeyboard.h"
#include "lllogchat.h"
#include "llmutelist.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewergesture.h"			// for triggering gestures
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llstatusbar.h"
#include "llstylemap.h"
#include "llweb.h"

//
// Global statics
//
LLColor4 get_text_color(const LLChat& chat);

LLColor4 get_extended_text_color(const LLChat& chat, LLColor4 defaultColor)
{
	if (gSavedSettings.getBOOL("HighlightOwnNameInChat"))
	{
		std::string new_line = std::string(chat.mText);
		size_t name_pos = new_line.find(chat.mFromName);
		if (name_pos == 0)
		{
			new_line = new_line.substr(chat.mFromName.length());
			if (new_line.find(": ") == 0)
			{
				new_line = new_line.substr(2);
			}
			else
			{
				new_line = new_line.substr(1);
			}
		}

		if (LLFloaterChat::isOwnNameInText(new_line))
		{
			return gSavedSettings.getColor4("OwnNameChatColor");
		}
	}

	return defaultColor;
}

//
// Member Functions
//
LLFloaterChat::LLFloaterChat(const LLSD& seed)
:	LLFloater("chat floater", "FloaterChatRect", "", RESIZE_YES, 440, 100,
			  DRAG_ON_TOP, MINIMIZE_NO, CLOSE_YES),
	mChatBarPanel(NULL),
	mSpeakerPanel(NULL),
	mToggleActiveSpeakersBtn(NULL),
	mHistoryWithoutMutes(NULL),
	mHistoryWithMutes(NULL)
{
	std::string xml_file;
	if (gSavedSettings.getBOOL("UseOldChatHistory"))
	{
		xml_file = "floater_chat_history2.xml";
	}
	else
	{
		xml_file = "floater_chat_history.xml";
		mFactoryMap["chat_panel"] = LLCallbackMap(createChatPanel, this);
	}
	mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);

	// FALSE so to not automatically open singleton floaters (as result of getInstance())
	LLUICtrlFactory::getInstance()->buildFloater(this, xml_file, &getFactoryMap(), FALSE);
}

// virtual
BOOL LLFloaterChat::postBuild()
{
	if (mChatBarPanel)
	{
		mChatBarPanel->setGestureCombo(getChild<LLComboBox>("Gesture", TRUE, FALSE));
#if TRANSLATE_CHAT
		childSetCommitCallback("translate chat", onClickToggleTranslateChat, this);
		childSetValue("translate chat", gSavedSettings.getBOOL("TranslateChat"));
#endif
	}

	childSetCommitCallback("show mutes", onClickToggleShowMute, this);

	mHistoryWithoutMutes = getChild<LLViewerTextEditor>("Chat History Editor");
	mHistoryWithMutes = getChild<LLViewerTextEditor>("Chat History Editor with mute");
	mHistoryWithMutes->setVisible(FALSE);

	mToggleActiveSpeakersBtn = getChild<LLButton>("toggle_active_speakers_btn");
	mToggleActiveSpeakersBtn->setClickedCallback(onClickToggleActiveSpeakers, this);

	return TRUE;
}

// virtual
void LLFloaterChat::setVisible(BOOL visible)
{
	LLFloater::setVisible(visible);
	gSavedSettings.setBOOL("ShowChatHistory", visible);
}

// virtual
void LLFloaterChat::draw()
{
	bool active_speakers_panel = mSpeakerPanel && mSpeakerPanel->getVisible();
	mToggleActiveSpeakersBtn->setValue(active_speakers_panel);
	if (active_speakers_panel)
	{
		mSpeakerPanel->refreshSpeakers();
	}

	if (mChatBarPanel)
	{
		mChatBarPanel->refresh();
	}

	LLFloater::draw();
}

// virtual
void LLFloaterChat::onClose(bool app_quitting)
{
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowChatHistory", FALSE);
	}
	setVisible(FALSE);
}

// virtual
void LLFloaterChat::onVisibilityChange(BOOL new_visibility)
{
	// Hide the chat overlay when our history is visible.
	updateConsoleVisibility();

	// stop chat history tab from flashing when it appears
	if (new_visibility)
	{
		LLFloaterChatterBox::getInstance()->setFloaterFlashing(this, FALSE);
	}

	LLFloater::onVisibilityChange(new_visibility);
}

// virtual
void LLFloaterChat::setMinimized(BOOL minimized)
{
	LLFloater::setMinimized(minimized);
	updateConsoleVisibility();
}

void LLFloaterChat::updateConsoleVisibility()
{
	// determine whether we should show console due to not being visible
	gConsole->setVisible(!isInVisibleChain() ||						// are we not in part of UI being drawn?
						 isMinimized() ||							// are we minimized?
						 (getHost() && getHost()->isMinimized()));	// are we hosted in a minimized floater?
}

void add_timestamped_line(LLViewerTextEditor* edit, LLChat chat, const LLColor4& color)
{
	std::string line = chat.mText;
	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("ChatShowTimestamps"))
	{
		edit->appendTime(prepend_newline);
		prepend_newline = false;
	}

	// If the msg is from an agent (not yourself though),
	// extract out the sender name and replace it with the hotlinked name.
	if (chat.mSourceType == CHAT_SOURCE_AGENT && chat.mFromID != LLUUID::null)
	{
		chat.mURL = llformat("secondlife:///app/agent/%s/about",
							 chat.mFromID.asString().c_str());
	}

	// If the chat line has an associated url, link it up to the name.
	if (!chat.mURL.empty() && (line.length() > chat.mFromName.length() &&
		(chat.mFromName.empty() || line.find(chat.mFromName, 0) == 0)))
	{
//MK
		if (!gRRenabled || !gAgent.mRRInterface.mContainsShownames)
		{
//mk
			size_t pos;
			if (chat.mFromName.empty() ||
				chat.mFromName.find_first_not_of(' ') == std::string::npos)
			{
				// Name is empty... Set the link on the first word instead
				// (skipping leading spaces and the ':' separator)...
				pos = line.find_first_not_of(" :");
				if (pos == std::string::npos)
				{
					// No word found !
					pos = line.length();
					line += " ";
				}
				else
				{
					pos = line.find(' ', pos);
					if (pos == std::string::npos)
					{
						// Only one word in the line...
						pos = line.length();
						line += " ";
					}
				}
			}
			else
			{
				pos = chat.mFromName.length() + 1;
			}
			std::string start_line = line.substr(0, pos);
			line = line.substr(pos);
			const LLStyleSP &sourceStyle = LLStyleMap::instance().lookup(chat.mFromID, chat.mURL);
			edit->appendStyledText(start_line, false, prepend_newline, sourceStyle);
			prepend_newline = false;
//MK
		}
//mk
	}
	edit->appendColoredText(line, false, prepend_newline, color);
}

void log_chat_text(const LLChat& chat)
{
	std::string histstr = chat.mText;
	if (gSavedPerAccountSettings.getBOOL("LogChatTimestamp"))
	{
		bool with_date = gSavedPerAccountSettings.getBOOL("LogTimestampDate");
		histstr = LLLogChat::timestamp(with_date) + histstr;
	}
	LLLogChat::saveHistory(std::string("chat"), histstr);
}

// static
void LLFloaterChat::addChatHistory(const LLChat& chat, bool log_to_file)
{
	if (log_to_file && gSavedPerAccountSettings.getBOOL("LogChat"))
	{
		log_chat_text(chat);
	}

	LLColor4 color;
	if (log_to_file)
	{
		color = get_text_color(chat);
	}
	else
	{
		color = LLColor4::grey;	// Recap from log file.
	}

	if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
	{
		LLFloaterScriptDebug::addScriptLine(chat.mText, chat.mFromName,
											color, chat.mFromID);
		if (!gSavedSettings.getBOOL("ScriptErrorsAsChat"))
		{
			return;
		}
	}

	// could flash the chat button in the status bar here. JC
	LLFloaterChat* self = LLFloaterChat::getInstance(LLSD());

	self->mHistoryWithoutMutes->setParseHTML(TRUE);
	self->mHistoryWithMutes->setParseHTML(TRUE);

	self->mHistoryWithoutMutes->setParseHighlights(TRUE);
	self->mHistoryWithMutes->setParseHighlights(TRUE);

	if (!chat.mMuted)
	{
		add_timestamped_line(self->mHistoryWithoutMutes, chat, color);
		add_timestamped_line(self->mHistoryWithMutes, chat, color);
	}
	else
	{
		// desaturate muted chat
		LLColor4 muted_color = lerp(color, LLColor4::grey, 0.5f);
		add_timestamped_line(self->mHistoryWithMutes, chat, color);
	}

	// add objects as transient speakers that can be muted
	if (chat.mSourceType == CHAT_SOURCE_OBJECT)
	{
		self->mSpeakerPanel->setSpeaker(chat.mFromID, chat.mFromName,
										LLSpeaker::STATUS_NOT_IN_CHANNEL,
										LLSpeaker::SPEAKER_OBJECT,
										chat.mOwnerID);
	}

	// start tab flashing on incoming text from other users (ignoring system
	// text, etc)
	if (!self->isInVisibleChain() && chat.mSourceType == CHAT_SOURCE_AGENT)
	{
		LLFloaterChatterBox::getInstance()->setFloaterFlashing(self, TRUE);
	}
}

// static
void LLFloaterChat::setHistoryCursorAndScrollToEnd()
{
	LLFloaterChat* self = LLFloaterChat::getInstance(LLSD());
	if (!self) return;

	if (self->mHistoryWithoutMutes) 
	{
		self->mHistoryWithoutMutes->setCursorAndScrollToEnd();
	}
	if (self->mHistoryWithMutes)
	{
		 self->mHistoryWithMutes->setCursorAndScrollToEnd();
	}
}

//static 
void LLFloaterChat::onClickMute(void *data)
{
	LLFloaterChat* self = (LLFloaterChat*)data;

	LLMuteList* ml = LLMuteList::getInstance();
	if (!ml) return;

	LLComboBox*	chatter_combo = self->getChild<LLComboBox>("chatter combobox");

	const std::string& name = chatter_combo->getSimple();
	LLUUID id = chatter_combo->getCurrentID();

	if (name.empty()) return;

	LLMute mute(id);
	mute.setFromDisplayName(name);
	if (ml->add(mute))
	{
		LLFloaterMute::showInstance();
		LLFloaterMute::getInstance()->selectMute(mute.mID);
	}
}

//static
void LLFloaterChat::onClickToggleShowMute(LLUICtrl* ctrl, void *data)
{
	LLFloaterChat* self = (LLFloaterChat*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!check || !self || !self->mHistoryWithoutMutes ||
		!self->mHistoryWithMutes)
	{
		return;
	}

	if (check->get())
	{
		self->mHistoryWithoutMutes->setVisible(FALSE);
		self->mHistoryWithMutes->setVisible(TRUE);
		self->mHistoryWithMutes->setCursorAndScrollToEnd();
	}
	else
	{
		self->mHistoryWithMutes->setVisible(FALSE);
		self->mHistoryWithoutMutes->setVisible(TRUE);
		self->mHistoryWithoutMutes->setCursorAndScrollToEnd();
	}
}

#if TRANSLATE_CHAT
// Update the "TranslateChat" pref after "translate chat" checkbox is toggled in
// the "Local Chat" floater.
//static
void LLFloaterChat::onClickToggleTranslateChat(LLUICtrl* ctrl, void *data)
{
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!check) return;
	gSavedSettings.setBOOL("TranslateChat", check->get());
}
#endif

// Update the "translate chat" checkbox after the "TranslateChat" pref is set in
// some other place (e.g. prefs dialog).
//static
void LLFloaterChat::updateSettings()
{
	BOOL translate_chat = gSavedSettings.getBOOL("TranslateChat");
	LLFloaterChat::getInstance(LLSD())->getChild<LLCheckBoxCtrl>("translate chat")->set(translate_chat);
}

// Put a line of chat in all the right places
void LLFloaterChat::addChat(const LLChat& chat, 
							BOOL from_instant_message, 
							BOOL local_agent)
{
//MK
	if (gRRenabled && chat.mText == "")
	{
		// In case crunchEmote() returned an empty string, just abort.
		return;
	}
//mk
	LLColor4 text_color = get_text_color(chat);

	BOOL invisible_script_debug_chat = (chat.mChatType == CHAT_TYPE_DEBUG_MSG &&
										!gSavedSettings.getBOOL("ScriptErrorsAsChat"));

#if LL_LCD_COMPILE
	// add into LCD displays
	if (!invisible_script_debug_chat)
	{
		if (!from_instant_message)
		{
			AddNewChatToLCD(chat.mText);
		}
		else
		{
			AddNewIMToLCD(chat.mText);
		}
	}
#endif
	if (!invisible_script_debug_chat && !chat.mMuted && gConsole && !local_agent)
	{
		if (chat.mSourceType == CHAT_SOURCE_SYSTEM)
		{
			text_color = gSavedSettings.getColor("SystemChatColor");
		}
		else if (from_instant_message)
		{
			text_color = gSavedSettings.getColor("IMChatColor");
		}
		// We display anything if it's not an IM. If it's an IM, check pref...
		if (!from_instant_message || gSavedSettings.getBOOL("IMInChatConsole")) 
		{
			gConsole->addConsoleLine(chat.mText, text_color);
		}
	}

	if (from_instant_message && gSavedPerAccountSettings.getBOOL("LogChatIM"))
	{
		log_chat_text(chat);
	}

	if (from_instant_message && gSavedSettings.getBOOL("IMInChatHistory")) 	 
	{
		addChatHistory(chat, false);
	}

	triggerAlerts(chat.mText);

	if (!from_instant_message)
	{
		addChatHistory(chat);
	}
}

// Moved from lltextparser.cpp to break llui/llaudio library dependency.
//static
void LLFloaterChat::triggerAlerts(const std::string& text)
{
	LLTextParser* parser = LLTextParser::getInstance();
#if 0
	bool spoken = false;
#endif
	for (S32 i = 0; i < parser->mHighlights.size(); i++)
	{
		LLSD& highlight = parser->mHighlights[i];
		if (parser->findPattern(text,highlight) >= 0)
		{
			if (gAudiop)
			{
				if ((std::string)highlight["sound_lluuid"] != LLUUID::null.asString())
				{
					gAudiop->triggerSound(highlight["sound_lluuid"].asUUID(), 
										  gAgent.getID(), 1.f,
										  LLAudioEngine::AUDIO_TYPE_UI,
										  gAgent.getPositionGlobal());
				}
#if 0
				if (!spoken) 
				{
					LLTextToSpeech* text_to_speech = NULL;
					text_to_speech = LLTextToSpeech::getInstance();
					spoken = text_to_speech->speak((LLString)highlight["voice"],text); 
				}
#endif
			}
			if (highlight["flash"])
			{
				LLWindow* viewer_window = gViewerWindow->getWindow();
				if (viewer_window && viewer_window->getMinimized())
				{
					viewer_window->flashIcon(5.f);
				}
			}
		}
	}
}

LLColor4 get_text_color(const LLChat& chat)
{
	LLColor4 text_color;

	if (chat.mMuted)
	{
		text_color.setVec(0.8f, 0.8f, 0.8f, 1.f);
	}
	else
	{
		switch (chat.mSourceType)
		{
		case CHAT_SOURCE_SYSTEM:
		case CHAT_SOURCE_UNKNOWN:
			text_color = gSavedSettings.getColor4("SystemChatColor");
			break;
		case CHAT_SOURCE_AGENT:
			if (gAgent.getID() == chat.mFromID)
			{
				text_color = gSavedSettings.getColor4("UserChatColor");
			}
			else
			{
				text_color = get_extended_text_color(chat,
													 gSavedSettings.getColor4("AgentChatColor"));
			}
			break;
		case CHAT_SOURCE_OBJECT:
			if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
			{
				text_color = gSavedSettings.getColor4("ScriptErrorColor");
			}
			else if (chat.mChatType == CHAT_TYPE_OWNER)
			{
				// Message from one of our own objects
				text_color = gSavedSettings.getColor4("llOwnerSayChatColor");
			}
			else if (chat.mChatType == CHAT_TYPE_DIRECT)
			{
				// Used both for llRegionSayTo() and llInstantMesssage()
				// since there is no real reason to distinguish one from
				// another (both are seen only by us and the object may
				// pertain to anyone, us included).
				text_color = gSavedSettings.getColor4("DirectChatColor");
			}
			else
			{
				// Public object chat
				text_color = gSavedSettings.getColor4("ObjectChatColor");
			}
			break;
		default:
			text_color.setToWhite();
		}

		if (!chat.mPosAgent.isExactlyZero())
		{
			LLVector3 pos_agent = gAgent.getPositionAgent();
			F32 distance = dist_vec(pos_agent, chat.mPosAgent);
			if (distance > gAgent.getNearChatRadius())
			{
				// diminish far-off chat
				text_color.mV[VALPHA] = 0.8f;
			}
		}
	}

	return text_color;
}

//static
void LLFloaterChat::loadHistory()
{
	LLLogChat::loadHistory(std::string("chat"), &chatFromLogFile,
						   (void *)LLFloaterChat::getInstance(LLSD())); 
}

//static
void LLFloaterChat::chatFromLogFile(LLLogChat::ELogLineType type,
									std::string line,
									void* userdata)
{
	switch (type)
	{
		case LLLogChat::LOG_EMPTY:
		case LLLogChat::LOG_END:
			// *TODO: nice message from XML file here
			break;
		case LLLogChat::LOG_LINE:
		{
			LLChat chat;
			chat.mText = line;
			addChatHistory(chat, FALSE);
			break;
		}
		default:
			// nothing
			break;
	}
}

//static
void* LLFloaterChat::createSpeakersPanel(void* data)
{
	LLFloaterChat* self = (LLFloaterChat*)data;
	self->mSpeakerPanel = new LLPanelActiveSpeakers(LLLocalSpeakerMgr::getInstance(), TRUE);
	return self->mSpeakerPanel;
}

//static
void* LLFloaterChat::createChatPanel(void* data)
{
	LLFloaterChat* self = (LLFloaterChat*)data;
	self->mChatBarPanel = new LLChatBar("floating_chat_bar");
	return self->mChatBarPanel;
}

// static
void LLFloaterChat::onClickToggleActiveSpeakers(void* userdata)
{
	LLFloaterChat* self = (LLFloaterChat*)userdata;
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
	{
		if (!self->mSpeakerPanel->getVisible()) return;
	}
//mk
	self->mSpeakerPanel->setVisible(!self->mSpeakerPanel->getVisible());
}

//static 
bool LLFloaterChat::visible(LLFloater* instance, const LLSD& key)
{
	return VisibilityPolicy<LLFloater>::visible(instance, key);
}

//static 
void LLFloaterChat::show(LLFloater* instance, const LLSD& key)
{
	VisibilityPolicy<LLFloater>::show(instance, key);
}

//static 
void LLFloaterChat::hide(LLFloater* instance, const LLSD& key)
{
	if (instance->getHost())
	{
		LLFloaterChatterBox::hideInstance();
	}
	else
	{
		VisibilityPolicy<LLFloater>::hide(instance, key);
	}
}

static std::set<std::string> highlight_words;

bool make_words_list()
{
	bool changed = false;
	size_t index;
	std::string name, part;
	static std::string nicknames = "";
	if (gSavedPerAccountSettings.getString("HighlightNicknames") != nicknames)
	{
		nicknames = gSavedPerAccountSettings.getString("HighlightNicknames");
		changed = true;
	}

	LLAvatarName avatar_name;
	static std::string display_name = "";
	static bool highlight_display_name = false;
	bool do_highlight = gSavedPerAccountSettings.getBOOL("HighlightDisplayName") &&
						LLAvatarNameCache::useDisplayNames() &&
						LLAvatarNameCache::get(gAgent.getID(), &avatar_name);

	if (do_highlight != highlight_display_name)
	{
		highlight_display_name = do_highlight;
		changed = true;
		if (!highlight_display_name)
		{
			display_name = "";
		}
	}

	if (highlight_display_name)
	{
		if (avatar_name.mIsDisplayNameDefault)
		{
			name = "";
		}
		else
		{
			name = avatar_name.mDisplayName;
			LLStringUtil::toLower(name);
		}

		if (name != display_name)
		{
			display_name = name;
			changed = true;
		}
	}

	if (changed && isAgentAvatarValid())
	{
		// Rebuild the whole list
		highlight_words.clear();

		// First, fetch the avatar name (note: we don't use
		// gSavedSettings.getString("[First/Last]Name") here,
		// because those are not set when using --autologin).
		LLNameValue *firstname = gAgentAvatarp->getNVPair("FirstName");
		LLNameValue *lastname = gAgentAvatarp->getNVPair("LastName");
		name.assign(firstname->getString());
		LLStringUtil::toLower(name);
		highlight_words.insert(name);
		name.assign(lastname->getString());
		if (name != "Resident" && highlight_words.count(name) == 0)
		{
			LLStringUtil::toLower(name);
			highlight_words.insert(name);
		}

		if (!display_name.empty())
		{
			name = display_name;
			while ((index = name.find(' ')) != std::string::npos)
			{
				part = name.substr(0, index);
				name = name.substr(index + 1);
				if (part.length() > 3)
				{
					highlight_words.insert(part);
				}
			}
			if (name.length() > 3 && highlight_words.count(name) == 0)
			{
				highlight_words.insert(name);
			}
		}

		if (!nicknames.empty())
		{
			name = nicknames;
			LLStringUtil::toLower(name);
			LLStringUtil::replaceChar(name, ' ', ','); // Accept space and comma separated list
			while ((index = name.find(',')) != std::string::npos)
			{
				part = name.substr(0, index);
				name = name.substr(index + 1);
				if (part.length() > 2)
				{
					highlight_words.insert(part);
				}
			}
			if (name.length() > 2 && highlight_words.count(name) == 0)
			{
				highlight_words.insert(name);
			}
		}
	}

	return changed;
}

//static 
bool LLFloaterChat::isOwnNameInText(const std::string &text_line)
{
	if (!isAgentAvatarValid())
	{
		return false;
	}
	const std::string separators(" .,;:'!?*-()[]\"");
	bool flag;
	char before, after;
	size_t index = 0, larger_index, length = 0;
	std::set<std::string>::iterator it;
	std::string name;
	std::string text = " " + text_line + " ";
	LLStringUtil::toLower(text);

	if (make_words_list())
	{
		name = "Highlights words list changed to: ";
		flag = false;
		for (it = highlight_words.begin(); it != highlight_words.end(); it++)
		{
			if (flag) {
				name += ", ";
			}
			name += *it;
			flag = true;
		}
		llinfos << name << llendl;
	}

	do
	{
		flag = false;
		larger_index = 0;
		for (it = highlight_words.begin(); it != highlight_words.end(); it++)
		{
			name = *it;
			index = text.find(name);
			if (index != std::string::npos)
			{
				flag = true;
				before = text.at(index - 1);
				after = text.at(index + name.length());
				if (separators.find(before) != std::string::npos && separators.find(after) != std::string::npos)
				{
					return true;
				}
				if (index >= larger_index)
				{
					larger_index = index;
					length = name.length();
				}
			}
		}
		if (flag)
		{
			text = " " + text.substr(index + length);
		}
	}
	while (flag);

	return false;
}
