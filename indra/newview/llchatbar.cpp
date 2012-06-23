/** 
 * @file llchatbar.cpp
 * @brief LLChatBar class implementation
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

//MK
#include "linden_common.h"
//mk

#include "llviewerprecompiledheaders.h"

#include "llchatbar.h"

#include "imageids.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llframetimer.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmultigesture.h"
#include "llparcel.h"
#include "llrect.h"
#include "llspellcheck.h"
#include "llstring.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "message.h"

#include "llagent.h"
#include "llcommandhandler.h"	// secondlife:///app/chat/ support
#include "llfloaterchat.h"
#include "hbfloatertextinput.h"
#include "llgesturemgr.h"
#include "llinventorymodel.h"
#include "lluiconstants.h"
#include "llviewergesture.h"	// for triggering gestures
#include "llviewermenu.h"		// for deleting object with DEL key
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llworld.h"

//
// Globals
//
const F32 AGENT_TYPING_TIMEOUT = 5.f;	// seconds

LLChatBar* gChatBar = NULL;

//static
BOOL LLChatBar::sSwappedShortcuts = FALSE;
std::set<std::string> LLChatBar::sIgnoredNames;

// legacy calllback glue
void toggleChatHistory(void* user_data);
void send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel);

class LLChatBarGestureObserver : public LLGestureManagerObserver
{
public:
	LLChatBarGestureObserver(LLChatBar* chat_barp) : mChatBar(chat_barp){}
	virtual ~LLChatBarGestureObserver() {}
	virtual void changed() { mChatBar->refreshGestures(); }
private:
	LLChatBar* mChatBar;
};

//
// Functions
//

//inline constructor
// for chat bars embedded in floaters, etc
LLChatBar::LLChatBar(const std::string& name) 
:	LLPanel(name, LLRect(), BORDER_NO),
	mInputEditor(NULL),
	mOpenTextEditorButton(NULL),
	mSayFlyoutButton(NULL),
	mGestureLabelTimer(),
	mLastSpecialChatChannel(0),
	mIsBuilt(false),
	mHasScrolledOnce(false),
	mGestureCombo(NULL),
	mSecondary(true),
	mObserver(NULL)
{
	sSwappedShortcuts = gSavedSettings.getBOOL("SwapShoutWhisperShortcuts");
	mLastSwappedShortcuts = sSwappedShortcuts;
}

LLChatBar::LLChatBar(const std::string& name, const LLRect& rect) 
:	LLPanel(name, rect, BORDER_NO),
	mInputEditor(NULL),
	mOpenTextEditorButton(NULL),
	mHistoryButton(NULL),
	mSayFlyoutButton(NULL),
	mGestureLabelTimer(),
	mLastSpecialChatChannel(0),
	mIsBuilt(false),
	mHasScrolledOnce(false),
	mGestureCombo(NULL),
	mSecondary(false),
	mObserver(NULL)
{
	sSwappedShortcuts = gSavedSettings.getBOOL("SwapShoutWhisperShortcuts");
	mLastSwappedShortcuts = sSwappedShortcuts;

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_chat_bar.xml");

	setIsChrome(TRUE);
	setFocusRoot(TRUE);

	setRect(rect); // override xml rect

	setBackgroundOpaque(TRUE);
	setBackgroundVisible(TRUE);

	// Start visible if we left the app while chatting.
	setVisible(gSavedSettings.getBOOL("ChatVisible"));

	// Apply custom layout.
	layout();
}

LLChatBar::~LLChatBar()
{
	gGestureManager.removeObserver(mObserver);
	delete mObserver;
	mObserver = NULL;
	HBFloaterTextInput::abort(mInputEditor);
	// LLView destructor cleans up children
}

BOOL LLChatBar::postBuild()
{
	mHistoryButton = getChild<LLButton>("History", TRUE, FALSE);
	if (mHistoryButton)
	{
		mHistoryButton->setClickedCallback(toggleChatHistory, this);
	}

	mSayFlyoutButton = getChild<LLFlyoutButton>("Say", TRUE, FALSE);
	if (mSayFlyoutButton)
	{
		mSayFlyoutButton->setCommitCallback(onClickSay);
		mSayFlyoutButton->setCallbackUserData(this);
		if (sSwappedShortcuts)
		{
			mSayFlyoutButton->setToolTip(getString("swapped_shortcuts"));
		}
		else
		{
			mSayFlyoutButton->setToolTip(getString("normal_shortcuts"));
		}
	}

	mOpenTextEditorButton = getChild<LLButton>("open_text_editor_btn", TRUE, FALSE);
	if (mOpenTextEditorButton)
	{
		mOpenTextEditorButton->setClickedCallback(onClickOpenTextEditor, this);
	}

	// attempt to bind to an existing combo box named gesture
	setGestureCombo(getChild<LLComboBox>("Gesture", TRUE, FALSE));

	mInputEditor = getChild<LLLineEditor>("Chat Editor", TRUE, FALSE);
	if (mInputEditor)
	{
		mInputEditor->setCallbackUserData(this);
		mInputEditor->setKeystrokeCallback(&onInputEditorKeystroke);
		mInputEditor->setScrolledCallback(&onInputEditorScrolled, this);
		mInputEditor->setFocusLostCallback(&onInputEditorFocusLost, this);
		mInputEditor->setFocusReceivedCallback(&onInputEditorGainFocus, this);
		mInputEditor->setCommitOnFocusLost(FALSE);
		mInputEditor->setRevertOnEsc(FALSE);
		mInputEditor->setIgnoreTab(TRUE);
		mInputEditor->setPassDelete(TRUE);
		mInputEditor->setReplaceNewlinesWithSpaces(FALSE);

		mInputEditor->setMaxTextLength(DB_CHAT_MSG_STR_LEN);
		mInputEditor->setEnableLineHistory(TRUE);
	}

	mIsBuilt = true;

	return TRUE;
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLChatBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
	if (mIsBuilt)
	{
		layout();
	}
}

// virtual
BOOL LLChatBar::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;

	if (HBFloaterTextInput::hasFloaterFor(mInputEditor))
	{
		HBFloaterTextInput::show(mInputEditor);
		return TRUE;
	}

	// ALT-RETURN is reserved for windowed/fullscreen toggle
	if (KEY_RETURN == key)
	{
		if ((sSwappedShortcuts && mask == MASK_SHIFT) ||
			(!sSwappedShortcuts && mask == MASK_CONTROL))
		{
			// shout
			sendChat(CHAT_TYPE_SHOUT);
			handled = TRUE;
		}
		else if ((sSwappedShortcuts && mask == MASK_CONTROL) ||
				 (!sSwappedShortcuts && mask == MASK_SHIFT))
		{
			// whisper
			sendChat(CHAT_TYPE_WHISPER);
			handled = TRUE;
		}
		else if (mask == MASK_NONE)
		{
			// say
			sendChat(CHAT_TYPE_NORMAL);
			handled = TRUE;
		}
		else if (mInputEditor && mask == (MASK_SHIFT | MASK_CONTROL))
		{
			S32 cursor = mInputEditor->getCursor();
			std::string text = mInputEditor->getText();
			// For some reason, the event is triggered twice: let's insert only
			// one newline character.
			if (cursor == 0 || text[cursor - 1] != '\n')
			{
				text = text.insert(cursor, "\n");
				mInputEditor->setText(text);
				mInputEditor->setCursor(cursor + 1);
			}
			handled = TRUE;
		}
	}
	else if (mInputEditor && KEY_TAB == key && mask == MASK_NONE &&
			 gSavedSettings.getBOOL("TabAutoCompleteName"))
	{
		std::string text = mInputEditor->getText();
		S32 word_start = 0;
		S32 word_len = 0;
		S32 cursor = mInputEditor->getCursor();
		S32 pos = cursor;
		if (pos > 0 && pos != text.length() - 1)
		{
			// Make sure the word will be found if the cursor is at its end
			pos--;
		}
		if (mInputEditor->getWordBoundriesAt(pos, &word_start, &word_len))
		{
			std::string word = text.substr(word_start, word_len);
			std::string suggestion = getMatchingAvatarName(word);
			if (suggestion != word)
			{
				text = text.replace(word_start, word_len, suggestion);
				mInputEditor->setText(text);
				if (gSavedSettings.getBOOL("SelectAutoCompletedPart"))
				{
					mInputEditor->setSelection(cursor,
											   cursor + suggestion.length() - word.length());
				}
				else
				{
					mInputEditor->setCursor(cursor + suggestion.length() - word.length());
				}
			}
		}
		handled = TRUE;
	}
	// only do this in main chatbar
	else if (KEY_ESCAPE == key && mask == MASK_NONE && gChatBar &&
			 gChatBar == this)
	{
		stopChat();
		handled = TRUE;
	}

	return handled;
}

void LLChatBar::layout()
{
	// If this is not the main chat bar, return
	if (mSecondary) return;

	LLRect r = getRect();

	// Get the width of the chat bar
	S32 rect_width = r.getWidth();

	// Padding (hard-coded) and origin of first element
	S32 pad = 4;
	S32 x = pad;
	// Width consumed by the buttons and gesture combo (i.e. all elements but
	// the input line).
	S32 consumed_width = x;

	// Calculate the elements height and centering
	S32 height = BTN_HEIGHT;
	if (height < 20)
	{
		height = 20;
	}
	S32 y = (r.getHeight() - height) / 2;
	if (y < 2)
	{
		height = r.getHeight() - 4;
		y = 2;
	}

	// Gesture combo width
	S32 gesture_width = 0;
	if (mGestureCombo)
	{
		r = mGestureCombo->getRect();
		gesture_width = r.getWidth();
		consumed_width += gesture_width + pad;
	}

	// Say button width
	S32 say_btn_width = 0;
	if (mSayFlyoutButton)
	{
		r = mSayFlyoutButton->getRect();
		say_btn_width = r.getWidth();
		consumed_width += say_btn_width + pad;
	}

	// Editor button width
	S32 editor_btn_width = 0;
	if (mOpenTextEditorButton)
	{
		r = mOpenTextEditorButton->getRect();
		// Keep the button round if it is already
		if (r.getWidth() == r.getHeight())
		{
			editor_btn_width = height;
		}
		else
		{
			editor_btn_width = r.getWidth();
		}
		consumed_width += editor_btn_width + pad;
	}

	// History button width
	if (mHistoryButton)
	{
		r = mHistoryButton->getRect();
		S32 history_btn_width = r.getWidth();
		r.setOriginAndSize(x, y, history_btn_width, height);
		mHistoryButton->setRect(r);
		x += history_btn_width + pad;
		consumed_width += history_btn_width + pad;
	}

	if (mInputEditor)
	{
		S32 input_width = rect_width - (consumed_width + pad);
		r.setOriginAndSize(x, y + 2, input_width, height - 2);
		mInputEditor->reshape(r.getWidth(), r.getHeight(), TRUE);
		mInputEditor->setRect(r);
		x += input_width + pad;
	}

	if (mOpenTextEditorButton)
	{
		r.setOriginAndSize(x, y, editor_btn_width, height);
		mOpenTextEditorButton->setRect(r);
		x += editor_btn_width + pad;
	}

	if (mSayFlyoutButton)
	{
		r.setOriginAndSize(x, y, say_btn_width, height);
		mSayFlyoutButton->reshape(r.getWidth(), r.getHeight(), TRUE);
		mSayFlyoutButton->setRect(r);
		x += say_btn_width + pad;
	}

	r.setOriginAndSize(x, y, gesture_width, height);
	if (mGestureCombo)
	{
		mGestureCombo->setRect(r);
	}
}

void LLChatBar::refresh()
{
	if (!mSecondary)
	{
		// call superclass setVisible so that we don't overwrite the saved setting
		static LLCachedControl<bool> chat_visible(gSavedSettings, "ChatVisible");
		LLPanel::setVisible((bool)chat_visible);
	}

	// HACK: Leave the name of the gesture in place for a few seconds.
	const F32 SHOW_GESTURE_NAME_TIME = 2.f;
	if (mGestureLabelTimer.getStarted() &&
		mGestureLabelTimer.getElapsedTimeF32() > SHOW_GESTURE_NAME_TIME)
	{
		LLCtrlListInterface* gestures = mGestureCombo ? mGestureCombo->getListInterface()
													  : NULL;
		if (gestures)
		{
			gestures->selectFirstItem();
		}
		mGestureLabelTimer.stop();
	}

	if (gAgent.getTypingTime() > AGENT_TYPING_TIMEOUT &&
		gAgent.getRenderState() & AGENT_STATE_TYPING)
	{
		gAgent.stopTyping();
	}

	if (!mSecondary && mHistoryButton)
	{
		mHistoryButton->setValue(LLFloaterChat::instanceVisible(LLSD()));
	}

	if (mInputEditor)
	{
		bool has_text_editor = HBFloaterTextInput::hasFloaterFor(mInputEditor);
		bool empty = mInputEditor->getText().size() == 0;
		if (empty && !has_text_editor)
		{
			// Reset this flag if the chat input line is empty
			mHasScrolledOnce = false;
		}
		mInputEditor->setEnabled(!has_text_editor);
		if (mSayFlyoutButton)
		{
			mSayFlyoutButton->setEnabled(!empty && !has_text_editor);
		}
		if (mGestureCombo)
		{
			mGestureCombo->setEnabled(!has_text_editor);
		}
	}

	if (mSayFlyoutButton && sSwappedShortcuts != mLastSwappedShortcuts)
	{
		mLastSwappedShortcuts = sSwappedShortcuts;
		if (sSwappedShortcuts)
		{
			mSayFlyoutButton->setToolTip(getString("swapped_shortcuts"));
		}
		else
		{
			mSayFlyoutButton->setToolTip(getString("normal_shortcuts"));
		}
	}
}

void LLChatBar::refreshGestures()
{
	if (mGestureCombo)
	{
		//store current selection so we can maintain it
		std::string cur_gesture = mGestureCombo->getValue().asString();
		mGestureCombo->selectFirstItem();
		std::string label = mGestureCombo->getValue().asString();;
		// clear
		mGestureCombo->clearRows();

		// collect list of unique gestures
		std::map <std::string, BOOL> unique;
		LLGestureManager::item_map_t::iterator it;
		for (it = gGestureManager.mActive.begin();
			 it != gGestureManager.mActive.end(); ++it)
		{
			LLMultiGesture* gesture = (*it).second;
			if (gesture)
			{
				if (!gesture->mTrigger.empty())
				{
					unique[gesture->mTrigger] = TRUE;
				}
			}
		}

		// add unique gestures
		std::map <std::string, BOOL>::iterator it2;
		for (it2 = unique.begin(); it2 != unique.end(); ++it2)
		{
			mGestureCombo->addSimpleElement((*it2).first);
		}

		mGestureCombo->sortByName();
		// Insert label after sorting, at top, with separator below it
		mGestureCombo->addSeparator(ADD_TOP);
		mGestureCombo->addSimpleElement(getString("gesture_label"), ADD_TOP);

		if (!cur_gesture.empty())
		{ 
			mGestureCombo->selectByValue(LLSD(cur_gesture));
		}
		else
		{
			mGestureCombo->selectFirstItem();
		}
	}
}

// Move the cursor to the correct input field.
void LLChatBar::setKeyboardFocus(BOOL focus)
{
	if (focus)
	{
		if (mInputEditor)
		{
			mInputEditor->setFocus(TRUE);
			mInputEditor->selectAll();
		}
	}
	else if (gFocusMgr.childHasKeyboardFocus(this))
	{
		if (mInputEditor)
		{
			mInputEditor->deselect();
		}
		setFocus(FALSE);
	}
}

// Ignore arrow keys in chat bar
void LLChatBar::setIgnoreArrowKeys(BOOL b)
{
	if (mInputEditor)
	{
		mInputEditor->setIgnoreArrowKeys(b);
	}
}

bool LLChatBar::hasTextEditor()
{
	return HBFloaterTextInput::hasFloaterFor(mInputEditor);
}

BOOL LLChatBar::inputEditorHasFocus()
{
	return mInputEditor && mInputEditor->hasFocus();
}

std::string LLChatBar::getCurrentChat()
{
	return mInputEditor ? mInputEditor->getText() : LLStringUtil::null;
}

void LLChatBar::setGestureCombo(LLComboBox* combo)
{
	mGestureCombo = combo;
	if (mGestureCombo)
	{
		mGestureCombo->setCommitCallback(onCommitGesture);
		mGestureCombo->setCallbackUserData(this);

		// now register observer since we have a place to put the results
		mObserver = new LLChatBarGestureObserver(this);
		gGestureManager.addObserver(mObserver);

		// refresh list from current active gestures
		refreshGestures();
	}
}

//-----------------------------------------------------------------------
// Internal functions
//-----------------------------------------------------------------------

// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
// Otherwise returns input and channel 0.
LLWString LLChatBar::stripChannelNumber(const LLWString &mesg, S32* channel)
{
	if (mesg[0] == '/' && mesg[1] == '/')
	{
		// This is a "repeat channel send"
		*channel = mLastSpecialChatChannel;
		return mesg.substr(2, mesg.length() - 2);
	}
	else if (mesg[0] == '/' && mesg[1] && LLStringOps::isDigit(mesg[1]))
	{
		// This a special "/20" speak on a channel
		S32 pos = 0;

		// Copy the channel number into a string
		LLWString channel_string;
		llwchar c;
		do
		{
			c = mesg[pos+1];
			channel_string.push_back(c);
			pos++;
		}
		while (c && pos < 64 && LLStringOps::isDigit(c));

		// Move the pointer forward to the first non-whitespace char
		// Check isspace before looping, so we can handle "/33foo"
		// as well as "/33 foo"
		while (c && iswspace(c))
		{
			c = mesg[pos+1];
			pos++;
		}

		mLastSpecialChatChannel = strtol(wstring_to_utf8str(channel_string).c_str(), NULL, 10);
		*channel = mLastSpecialChatChannel;
		return mesg.substr(pos, mesg.length() - pos);
	}
	else
	{
		// This is normal chat.
		*channel = 0;
		return mesg;
	}
}

void LLChatBar::sendChat(EChatType type)
{
	if (mInputEditor)
	{
		LLWString text = mInputEditor->getConvertedText();
		if (!text.empty())
		{
			// store sent line in history, duplicates will get filtered
			if (mInputEditor) mInputEditor->updateHistory();
			// Check if this is destined for another channel
			S32 channel = 0;
			stripChannelNumber(text, &channel);

			std::string utf8text = wstring_to_utf8str(text);
			// Try to trigger a gesture, if not chat to a script.
			std::string utf8_revised_text;
			if (0 == channel)
			{
				if (gSavedSettings.getBOOL("AutoCloseOOC"))
				{
					// Try to find any unclosed OOC chat (i.e. an opening
					// double parenthesis without a matching closing double
					// parenthesis.
					if (utf8text.find("((") != -1 && utf8text.find("))") == -1)
					{
						if (utf8text.at(utf8text.length() - 1) == ')')
						{
							// cosmetic: add a space first to avoid a closing triple parenthesis
							utf8text += " ";
						}
						// add the missing closing double parenthesis.
						utf8text += "))";
					}
				}

				// Convert MU*s style poses into IRC emotes here.
				if (gSavedSettings.getBOOL("AllowMUpose") && utf8text.find(":") == 0 && utf8text.length() > 3)
				{
					if (utf8text.find(":'") == 0)
					{
						utf8text.replace(0, 1, "/me");
	 				}
					else if (isalpha(utf8text.at(1)))	// Do not prevent smileys and such.
					{
						utf8text.replace(0, 1, "/me ");
					}
				}
//MK
////			// discard returned "found" boolean
////			gGestureManager.triggerAndReviseString(utf8text, &utf8_revised_text);
				BOOL found_gesture = gGestureManager.triggerAndReviseString(utf8text, &utf8_revised_text);

				if (gRRenabled && gAgent.mRRInterface.contains("sendchat") && !gAgent.mRRInterface.containsSubstr ("redirchat:"))
				{
					// user is forbidden to send any chat message on channel 0 except emotes and OOC text
					utf8_revised_text = gAgent.mRRInterface.crunchEmote (utf8_revised_text, 20);
					if (found_gesture && utf8_revised_text=="...") utf8_revised_text="";
				}
//mk
			}
			else
			{
//MK
				std::ostringstream stream;
				stream << "" << channel;
				if (gRRenabled && gAgent.mRRInterface.containsWithoutException("sendchannel", stream.str()))
				{
					utf8_revised_text = "";
				}
				else
//mk
					utf8_revised_text = utf8text;
			}

			utf8_revised_text = utf8str_trim(utf8_revised_text);

			if (!utf8_revised_text.empty())
			{
				// Chat with animation
				sendChatFromViewer(utf8_revised_text, type, TRUE);
			}
		}
	}

	childSetValue("Chat Editor", LLStringUtil::null);

	gAgent.stopTyping();

	// If the user wants to stop chatting on hitting return, lose focus
	// and go out of chat mode.
	if (gChatBar == this && gSavedSettings.getBOOL("CloseChatOnReturn"))
	{
		stopChat();
	}
}

//-----------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------

// static 
void LLChatBar::startChat(const char* line)
{
	if (!gChatBar) return;

	gChatBar->setVisible(TRUE);
	gChatBar->setKeyboardFocus(TRUE);
	gSavedSettings.setBOOL("ChatVisible", TRUE);

	if (gChatBar->mInputEditor)
	{
		if (line)
		{
			std::string line_string(line);
			gChatBar->mInputEditor->setText(line_string);
		}
		// always move cursor to end so users don't obliterate chat when accidentally hitting WASD
		gChatBar->mInputEditor->setCursorToEnd();
	}
}

// Exit "chat mode" and do the appropriate focus changes
// static
void LLChatBar::stopChat()
{
	if (!gChatBar) return;

	// In simple UI mode, we never release focus from the chat bar
	gChatBar->setKeyboardFocus(FALSE);

	// If we typed a movement key and pressed return during the
	// same frame, the keyboard handlers will see the key as having
	// gone down this frame and try to move the avatar.
	gKeyboard->resetKeys();
	gKeyboard->resetMaskKeys();

	// stop typing animation
	gAgent.stopTyping();

	// hide chat bar so it doesn't grab focus back
	gChatBar->setVisible(FALSE);
	gSavedSettings.setBOOL("ChatVisible", FALSE);
}

void LLChatBar::setVisible(BOOL visible)
{
	// If this is not the main chat bar, return
	if (mSecondary) return;

	gSavedSettings.setBOOL("ChatVisible", visible);
	LLPanel::setVisible(visible);
}

// static
void LLChatBar::onInputEditorKeystroke(LLLineEditor* caller, void* userdata)
{
	LLChatBar* self = (LLChatBar*)userdata;

	LLWString raw_text;
	if (self->mInputEditor) raw_text = self->mInputEditor->getWText();

	// Can't trim the end, because that will cause autocompletion
	// to eat trailing spaces that might be part of a gesture.
	LLWStringUtil::trimHead(raw_text);

	S32 length = raw_text.length();

	if (length > 0 && raw_text[0] != '/')  // forward slash is used for escape (eg. emote) sequences
	{
//MK
		if (!gRRenabled || !gAgent.mRRInterface.containsSubstr ("redirchat:"))
//mk
			gAgent.startTyping();
	}
	else
	{
		gAgent.stopTyping();
	}

	KEY key = gKeyboard->currentKey();

	// Ignore "special" keys, like backspace, arrows, etc.
	if (length > 1 && raw_text[0] == '/' && key < KEY_SPECIAL)
	{
		// we're starting a gesture, attempt to autocomplete

		std::string utf8_trigger = wstring_to_utf8str(raw_text);
		std::string utf8_out_str(utf8_trigger);

		if (gGestureManager.matchPrefix(utf8_trigger, &utf8_out_str))
		{
			if (self->mInputEditor)
			{
				std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
				self->mInputEditor->setText(utf8_trigger + rest_of_match); // keep original capitalization for user-entered part
				S32 outlength = self->mInputEditor->getLength(); // in characters

				// Select to end of line, starting from the character
				// after the last one the user typed.
				self->mInputEditor->setSelection(length, outlength);
			}
		}
	}
}

// static
void LLChatBar::onInputEditorScrolled(LLLineEditor* caller, void* userdata)
{
	LLChatBar* self = (LLChatBar*)userdata;
	if (!self || !caller) return;

	if (!self->mHasScrolledOnce &&
		gSavedSettings.getBOOL("AutoOpenTextInput"))
	{
		self->mHasScrolledOnce = true;
		HBFloaterTextInput::show(caller);
	}
}

// static
void LLChatBar::onInputEditorFocusLost(LLFocusableElement* caller, void* userdata)
{
	// stop typing animation
	gAgent.stopTyping();
}

// static
void LLChatBar::onInputEditorGainFocus(LLFocusableElement* caller, void* userdata)
{
	LLFloaterChat::setHistoryCursorAndScrollToEnd();
}

// static
void LLChatBar::onClickSay(LLUICtrl* ctrl, void* userdata)
{
	e_chat_type chat_type = CHAT_TYPE_NORMAL;
	if (ctrl->getValue().asString() == "shout")
	{
		chat_type = CHAT_TYPE_SHOUT;
	}
	else if (ctrl->getValue().asString() == "whisper")
	{
		chat_type = CHAT_TYPE_WHISPER;
	}
	LLChatBar* self = (LLChatBar*) userdata;
	self->sendChat(chat_type);
}

// static
void LLChatBar::onClickOpenTextEditor(void* userdata)
{
	LLChatBar* self = (LLChatBar*)userdata;
	if (self && self->mInputEditor)
	{
		self->mHasScrolledOnce = true;
		HBFloaterTextInput::show(self->mInputEditor);
	}
}

void LLChatBar::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}

void LLChatBar::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
{
	// Look for "/20 foo" channel chats.
	S32 channel = 0;
	LLWString out_text = stripChannelNumber(wtext, &channel);
	std::string utf8_out_text = wstring_to_utf8str(out_text);
	if (!utf8_out_text.empty())
	{
		utf8_out_text = utf8str_truncate(utf8_out_text, MAX_MSG_STR_LEN);
	}

	std::string utf8_text = wstring_to_utf8str(wtext);
	utf8_text = utf8str_trim(utf8_text);
	if (!utf8_text.empty())
	{
		utf8_text = utf8str_truncate(utf8_text, MAX_STRING - 1);
	}

//MK
	if (gRRenabled && channel == 0)
	{
		// transform the type according to chatshout, chatnormal and chatwhisper restrictions
		if (type == CHAT_TYPE_WHISPER && gAgent.mRRInterface.contains("chatwhisper"))
		{
			type = CHAT_TYPE_NORMAL;
		}
		if (type == CHAT_TYPE_SHOUT && gAgent.mRRInterface.contains("chatshout"))
		{
			type = CHAT_TYPE_NORMAL;
		}
		if ((type == CHAT_TYPE_SHOUT || type == CHAT_TYPE_NORMAL)
			&& gAgent.mRRInterface.contains("chatnormal"))
		{
			type = CHAT_TYPE_WHISPER;
		}

		if (gAgent.mRRInterface.containsSubstr ("redirchat:"))
		{
			animate = false;
		}
	}
//mk
	// Don't animate for chats people can't hear (chat to scripts)
	if (animate && channel == 0)
	{
		LLUUID anim;
		if (type == CHAT_TYPE_WHISPER)
		{
			LL_DEBUGS("SendChat") << "You whisper " << utf8_text << LL_ENDL;
			anim = ANIM_AGENT_WHISPER;
		}
		else if (type == CHAT_TYPE_NORMAL)
		{
			LL_DEBUGS("SendChat") << "You say " << utf8_text << LL_ENDL;
			anim = ANIM_AGENT_TALK;
		}
		else if (type == CHAT_TYPE_SHOUT)
		{
			LL_DEBUGS("SendChat") << "You shout " << utf8_text << LL_ENDL;
			anim = ANIM_AGENT_SHOUT;
		}
		else
		{
			LL_WARNS("SendChat") << "sendChatFromViewer() - invalid volume" << LL_ENDL;
			return;
		}
		U32 play_anim = gSavedSettings.getU32("PlayChatAnims");
		if (play_anim == 0)
		{
			animate = FALSE;
		}
		else if (play_anim == 1)
		{
			if ((utf8_out_text.find("/me ") == 0 || utf8_out_text.find("/me'") == 0) &&
				utf8_out_text.find('"') == std::string::npos)
			{
				// Do not animate for pure emotes
				animate = FALSE;
			}
		}
		if (animate)
		{
			gAgent.sendAnimationRequest(anim, ANIM_REQUEST_START);
		}
	}
	if (channel != 0 && type != CHAT_TYPE_START && type != CHAT_TYPE_STOP)
	{
		LL_DEBUGS("SendChat") << "Chat channel: " << channel << " - Text: " << utf8_text << LL_ENDL;
	}

	send_chat_from_viewer(utf8_out_text, type, channel);
}

void send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel)
{
//MK
	if (gRRenabled && channel >= 2147483647 && gAgent.mRRInterface.contains("sendchat"))
	{
		// When prevented from talking, remove the ability to talk on the DEBUG_CHANNEL altogether, since it is a way of cheating
		return;
	}

	if (gRRenabled && channel == 0)
	{
		std::string restriction;

		// We might want to redirect this chat or emote (and exit this function early on)
		if (utf8_out_text.find("/me ") == 0 || utf8_out_text.find("/me'") == 0) // emote
		{
			if (gAgent.mRRInterface.containsSubstr("rediremote:"))
			{
				restriction = "rediremote:";
			}
		}
		else if (utf8_out_text.find("((") != 0 || utf8_out_text.find("))") != utf8_out_text.length() - 2)
		{
			if (gAgent.mRRInterface.containsSubstr("redirchat:"))
			{
				restriction = "redirchat:";
			}
		}

		if (!restriction.empty())
		{
			// Public chat or emote redirected => for each redirection, send the same message on the target channel
			RRMAP::iterator it = gAgent.mRRInterface.mSpecialObjectBehaviours.begin();
			std::string behav;
			while (it != gAgent.mRRInterface.mSpecialObjectBehaviours.end())
			{
				behav = it->second;
				if (behav.find (restriction) == 0)
				{
					S32 ch = atoi (behav.substr (restriction.length()).c_str());
					std::ostringstream stream;
					stream << "" << ch;
					if (!gAgent.mRRInterface.containsWithoutException ("sendchannel", stream.str()))
					{
						if (ch > 0 && ch < 2147483647)
						{
							LLMessageSystem* msg = gMessageSystem;
							msg->newMessageFast(_PREHASH_ChatFromViewer);
							msg->nextBlockFast(_PREHASH_AgentData);
							msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
							msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
							msg->nextBlockFast(_PREHASH_ChatData);
							msg->addStringFast(_PREHASH_Message, utf8_out_text);
							msg->addU8Fast(_PREHASH_Type, type);
							msg->addS32("Channel", ch);

							gAgent.sendReliableMessage();
						}
					}
				}
				it++;
			}

			LLViewerStats::getInstance()->incStat(LLViewerStats::ST_CHAT_COUNT);

			// We have redirected the chat message, don't send it on the original channel
			return;
		}
	}

	std::string crunchedText = utf8_out_text;

	// There is a redirection in force but this particular message is an emote
	// or an OOC text, so we didn't redirect it. However it has not gone
	// through crunchEmote yet, so we need to do this here to prevent cheating
	// with emote-like chat (true emotes must however go through untouched).
	if (gRRenabled && channel == 0 && gAgent.mRRInterface.containsSubstr ("redirchat:"))
	{
		crunchedText = gAgent.mRRInterface.crunchEmote(crunchedText);
	}
//mk

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ChatFromViewer);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ChatData);
////msg->addStringFast(_PREHASH_Message, utf8_out_text);
//MK
	msg->addStringFast(_PREHASH_Message, crunchedText);
//mk
	msg->addU8Fast(_PREHASH_Type, type);
	msg->addS32("Channel", channel);

	gAgent.sendReliableMessage();

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_CHAT_COUNT);
}

// static
void LLChatBar::onCommitGesture(LLUICtrl* ctrl, void* data)
{
	LLChatBar* self = (LLChatBar*)data;
	LLCtrlListInterface* gestures = self->mGestureCombo ? self->mGestureCombo->getListInterface()
														: NULL;
	if (gestures)
	{
		S32 index = gestures->getFirstSelectedIndex();
		if (index == 0)
		{
			return;
		}
		const std::string& trigger = gestures->getSelectedValue().asString();

//MK
		if (!gRRenabled || !gAgent.mRRInterface.contains("sendchat"))
		{
//mk
			// pretend the user chatted the trigger string, to invoke
			// substitution and logging.
			std::string text(trigger);
			std::string revised_text;
			gGestureManager.triggerAndReviseString(text, &revised_text);

			revised_text = utf8str_trim(revised_text);
			if (!revised_text.empty())
			{
				// Don't play nodding animation
				self->sendChatFromViewer(revised_text, CHAT_TYPE_NORMAL, FALSE);
			}
//MK
		}
//mk
	}
	self->mGestureLabelTimer.start();
	if (self->mGestureCombo != NULL)
	{
		// free focus back to chat bar
		self->mGestureCombo->setFocus(FALSE);
	}
}

//static
std::string LLChatBar::getMatchingAvatarName(const std::string& match)
{
	std::string suggestion = match;
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
	{
		return suggestion;
	}
//mk
	bool add_to_ignore = LLSpellCheck::instance().getSpellCheck() &&
						 gSavedSettings.getBOOL("AddAvatarNamesToIgnore");
	std::vector<LLUUID> avatars;
	std::vector<LLVector3d> positions;
	LLWorld::getInstance()->getAvatars(&avatars, &positions,
									   gAgent.getPositionGlobal(),
									   gSavedSettings.getF32("NearMeRange"));
	std::set<std::string> matches;
	std::string longest_match;
	size_t len = 0;
	std::string first_name, last_name, display_name, name;
	std::string pattern = match;
	LLStringUtil::toLower(pattern);
	LLAvatarName avatar_name;
	for (std::vector<LLUUID>::iterator it = avatars.begin();
		 it != avatars.end(); it++)
	{
		const LLUUID id = *it;
		if (gCacheName->getName(id, first_name, last_name))
		{
			name = first_name;
			LLStringUtil::toLower(name);
			if (name.length() && name.find(pattern) == 0)
			{
				LL_DEBUGS("NameAutoCompletion") << "Inserting matching first name: "
												<< first_name << LL_ENDL;
				matches.insert(first_name);
				if (name.length() > len)
				{
					len = name.length();
					longest_match = first_name;
				}
			}
			if (last_name.length() && last_name != "Resident")
			{
				name = last_name;
				LLStringUtil::toLower(name);
				if (name.find(pattern) == 0)
				{
					LL_DEBUGS("NameAutoCompletion") << "Inserting matching last name: "
													<< last_name << LL_ENDL;
					matches.insert(last_name);
					if (name.length() > len)
					{
						len = name.length();
						longest_match = last_name;
					}
				}
			}
			else
			{
				last_name.clear();
			}
		}
		else
		{
			first_name.clear();
			last_name.clear();			
		}
		display_name.clear();
		if (LLAvatarNameCache::useDisplayNames() &&
			LLAvatarNameCache::get(id, &avatar_name))
		{
			display_name = avatar_name.mDisplayName;
			if (display_name != first_name &&
				display_name != first_name + " " + last_name)
			{
				name = display_name;
				LLStringUtil::toLower(name);
				if (name.length() && name.find(pattern) == 0)
				{
					LL_DEBUGS("NameAutoCompletion") << "Inserting matching display name: "
													<< display_name << LL_ENDL;
					matches.insert(display_name);
					if (name.length() > len)
					{
						len = name.length();
						longest_match = display_name;
					}
				}
			}
			else
			{
				display_name.clear();
			}
		}
		if (add_to_ignore)
		{
			name = first_name + " " + last_name + " " + display_name;
			// Display names can change, so don't rely on avatar UUIDs
			if (!sIgnoredNames.count(name))
			{
				sIgnoredNames.insert(name);
				LLSpellCheck::instance().addWordsToIgnoreList(name);
			}
		}
	}
	if (matches.size() == 1)
	{
		suggestion = *(matches.begin());
		LL_DEBUGS("NameAutoCompletion") << "Only one match found: "
										<< suggestion << LL_ENDL;
	}
	else if (matches.size() > 1)
	{
		// Find the first common letters for all matches.
		for (size_t i = match.length(); i <= len; i++)
		{
			pattern = utf8str_truncate(longest_match, (S32)i);
			LLStringUtil::toLower(pattern);
			for (std::set<std::string>::iterator it = matches.begin();
				 it != matches.end(); it++)
			{
				name = *it;
				LLStringUtil::toLower(name);
				if (name.find(pattern) != 0)
				{
					return suggestion;
				}
			}
			suggestion = utf8str_truncate(longest_match, (S32)i);
		}
		LL_DEBUGS("NameAutoCompletion") << "Several matches found, returning the common letters: "
										<< suggestion << LL_ENDL;
	}
	else
	{
		LL_DEBUGS("NameAutoCompletion") << "No match found, returning the search string: "
										<< suggestion << LL_ENDL;
	}

	return suggestion;
}

void toggleChatHistory(void* user_data)
{
	LLFloaterChat::toggleInstance(LLSD());
}

class LLChatHandler : public LLCommandHandler
{
public:
	// not allowed from outside the app
	LLChatHandler() : LLCommandHandler("chat", true) { }

    // Your code here
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		if (tokens.size() < 2) return false;
		S32 channel = tokens[0].asInteger();
		std::string mesg = tokens[1].asString();
		EChatType type = CHAT_TYPE_NORMAL;
//MK
		if (gRRenabled && channel == 0 && gAgent.mRRInterface.contains("chatnormal"))
		{
			type = CHAT_TYPE_WHISPER;
		}
//mk
		send_chat_from_viewer(mesg, type, channel);
		return true;
	}
};

// Creating the object registers with the dispatcher.
LLChatHandler gChatHandler;
