/** 
 * @file llfloatermute.cpp
 * @brief Container for mute list
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

#include "llfloatermute.h"

#include "llcheckboxctrl.h"
#include "llfocusmgr.h"
#include "llscrolllistctrl.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llfloateravatarpicker.h"
#include "llmutelist.h"

//-----------------------------------------------------------------------------
// LLFloaterMuteObjectUI()
//-----------------------------------------------------------------------------
// Class for handling mute object by name floater.
class LLFloaterMuteObjectUI : public LLFloater
{
public:
	typedef void(*callback_t)(const std::string&, void*);

	static LLFloaterMuteObjectUI* show(callback_t callback, void* userdata);
	virtual BOOL postBuild();

protected:
	LLFloaterMuteObjectUI();
	virtual ~LLFloaterMuteObjectUI();
	virtual BOOL handleKeyHere(KEY key, MASK mask);

private:
	// UI Callbacks
	static void onBtnOk(void *data);
	static void onBtnCancel(void *data);

	void (*mCallback)(const std::string& objectName, void* userdata);
	void* mCallbackUserData;

	static LLFloaterMuteObjectUI* sInstance;
};

LLFloaterMuteObjectUI* LLFloaterMuteObjectUI::sInstance = NULL;

LLFloaterMuteObjectUI::LLFloaterMuteObjectUI()
:	LLFloater(std::string("Mute object by name")),
	mCallback(NULL),
	mCallbackUserData(NULL)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_mute_object.xml", NULL);
}

// Destroys the object
LLFloaterMuteObjectUI::~LLFloaterMuteObjectUI()
{
	gFocusMgr.releaseFocusIfNeeded(this);
	sInstance = NULL;
}

LLFloaterMuteObjectUI* LLFloaterMuteObjectUI::show(callback_t callback,
												   void* userdata)
{
	const bool firstInstantiation = (sInstance == NULL);
	if (firstInstantiation)
	{
		sInstance = new LLFloaterMuteObjectUI;
	}
	sInstance->mCallback = callback;
	sInstance->mCallbackUserData = userdata;
  
	sInstance->open();
	if (firstInstantiation)
	{
		sInstance->center();
	}

	return sInstance;
}

BOOL LLFloaterMuteObjectUI::postBuild()
{
	childSetAction("OK", onBtnOk, this);
	childSetAction("Cancel", onBtnCancel, this);
	return TRUE;
}

void LLFloaterMuteObjectUI::onBtnOk(void* userdata)
{
	LLFloaterMuteObjectUI* self = (LLFloaterMuteObjectUI*)userdata;
	if (!self) return;

	if (self->mCallback)
	{
		const std::string& text = self->childGetValue("object_name").asString();
		self->mCallback(text, self->mCallbackUserData);
	}
	self->close();
}

void LLFloaterMuteObjectUI::onBtnCancel(void* userdata)
{
	LLFloaterMuteObjectUI* self = (LLFloaterMuteObjectUI*)userdata;
	if (!self) return;

	self->close();
}

BOOL LLFloaterMuteObjectUI::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		onBtnOk(this);
		return TRUE;
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		onBtnCancel(this);
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}

//
// Member Functions
//

//-----------------------------------------------------------------------------
// LLFloaterMute()
//-----------------------------------------------------------------------------
LLFloaterMute::LLFloaterMute(const LLSD& seed)
:	LLFloater(std::string("mute floater"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_mute.xml", NULL, FALSE);
}

// LLMuteListObserver callback interface implementation.
// virtual
void LLFloaterMute::onChange()
{
	refreshMuteList();
}

BOOL LLFloaterMute::postBuild()
{
	LLMuteList* ml = LLMuteList::getInstance();
	if (!ml) return FALSE;

	childSetCommitCallback("mutes", onSelectName, this);
	childSetAction("update_mutes", onClickUpdateMutes, this);
	childSetAction("mute_resident", onClickPick, this);
	childSetAction("mute_by_name", onClickMuteByName, this);
	childSetAction("unmute", onClickRemove, this);

	childSetCommitCallback("mute_all", onMuteAllToggled, this);
	childSetCommitCallback("mute_chat", onMuteTypeToggled, this);
	childSetCommitCallback("mute_voice", onMuteTypeToggled, this);
	childSetCommitCallback("mute_sounds", onMuteTypeToggled, this);
	childSetCommitCallback("mute_particles", onMuteTypeToggled, this);

	mMuteList = getChild<LLScrollListCtrl>("mutes");
	mMuteList->setCommitOnSelectionChange(TRUE);

	ml->addObserver(this);

	refreshMuteList();

	return TRUE;
}

//-----------------------------------------------------------------------------
// ~LLFloaterMute()
//-----------------------------------------------------------------------------
LLFloaterMute::~LLFloaterMute()
{
}

//-----------------------------------------------------------------------------
// refreshMuteList()
//-----------------------------------------------------------------------------
void LLFloaterMute::refreshMuteList()
{
	mMuteList->deleteAllItems();

	std::vector<LLMute> mutes = LLMuteList::getInstance()->getMutes();
	std::vector<LLMute>::iterator it;
	for (it = mutes.begin(); it != mutes.end(); ++it)
	{
		std::string name_and_type = it->getNameAndType();
		mMuteList->addStringUUIDItem(name_and_type, it->mID);
	}

	updateButtons();
}

void LLFloaterMute::selectMute(const LLUUID& mute_id)
{
	mMuteList->selectByID(mute_id);
	mMuteList->setScrollPos(mMuteList->getFirstSelectedIndex());
	updateButtons();
}

//-----------------------------------------------------------------------------
// updateButtons()
//-----------------------------------------------------------------------------
void LLFloaterMute::updateButtons()
{
	bool selected = (mMuteList->getFirstSelected() != NULL); // Entry selected
	bool enabled = selected;
	bool mute_all = false;
	U32 flags = 0;

	if (selected)
	{
		LLUUID id = mMuteList->getStringUUIDSelectedItem();
		std::vector<LLMute> mutes = LLMuteList::getInstance()->getMutes();
		std::vector<LLMute>::iterator it;
		for (it = mutes.begin(); it != mutes.end(); it++)
		{
			if (it->mID == id)
			{
				break;
			}
		}
		if (it == mutes.end())
		{
			// Shoud never happen...
			enabled = false;
		}
		else
		{
			enabled = it->mType == LLMute::AGENT || it->mType == LLMute::GROUP;
			mute_all = it->mFlags == 0;
			if (!mute_all && enabled)
			{
				flags = ~(it->mFlags);
			}
		}
	}

	childSetEnabled("update_mutes", false);	// Mutes are up to date
	childSetEnabled("unmute", selected);

	childSetEnabled("mute_all", enabled && !mute_all);
	childSetEnabled("mute_chat", enabled);
	childSetEnabled("mute_voice", enabled);
	childSetEnabled("mute_sounds", enabled);
	childSetEnabled("mute_particles", enabled);

	childSetValue("mute_all", mute_all);
	childSetValue("mute_chat", (flags & LLMute::flagTextChat) != 0);
	childSetValue("mute_voice", (flags & LLMute::flagVoiceChat) != 0);
	childSetValue("mute_sounds", (flags & LLMute::flagObjectSounds) != 0);
	childSetValue("mute_particles", (flags & LLMute::flagParticles) != 0);
}

//
// Static functions
//

//-----------------------------------------------------------------------------
// onSelectName()
//-----------------------------------------------------------------------------
void LLFloaterMute::onSelectName(LLUICtrl *caller, void *data)
{
	LLFloaterMute *floater = (LLFloaterMute*)data;

	floater->updateButtons();
}

//-----------------------------------------------------------------------------
// onClickRemove()
//-----------------------------------------------------------------------------
void LLFloaterMute::onClickRemove(void *data)
{
	LLFloaterMute* floater = (LLFloaterMute *)data;

	std::string name = floater->mMuteList->getSelectedItemLabel();
	LLUUID id = floater->mMuteList->getStringUUIDSelectedItem();
	LLMute mute(id);
	mute.setFromDisplayName(name); // now mute.mName has the suffix trimmed off

	S32 last_selected = floater->mMuteList->getFirstSelectedIndex();
	if (LLMuteList::getInstance()->remove(mute))
	{
		// Above removals may rebuild this dialog.

		if (last_selected == floater->mMuteList->getItemCount())
		{
			// we were on the last item, so select the last item again
			floater->mMuteList->selectNthItem(last_selected - 1);
		}
		else
		{
			// else select the item after the last item previously selected
			floater->mMuteList->selectNthItem(last_selected);
		}
	}
	floater->updateButtons();
}

//-----------------------------------------------------------------------------
// onClickPick()
//-----------------------------------------------------------------------------
void LLFloaterMute::onClickPick(void *data)
{
	LLFloaterMute* floaterp = (LLFloaterMute*)data;
	const BOOL allow_multiple = FALSE;
	const BOOL close_on_select = TRUE;
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(onPickUser,
																data,
																allow_multiple,
																close_on_select);
	floaterp->addDependentFloater(picker);
}

//-----------------------------------------------------------------------------
// onPickUser()
//-----------------------------------------------------------------------------
void LLFloaterMute::onPickUser(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* user_data)
{
	LLFloaterMute* floaterp = (LLFloaterMute*)user_data;
	if (!floaterp) return;
	if (names.empty() || ids.empty()) return;

	LLMute mute(ids[0], names[0], LLMute::AGENT);
	LLMuteList::getInstance()->add(mute);
	floaterp->updateButtons();
}

void LLFloaterMute::onClickMuteByName(void* data)
{
	LLFloaterMuteObjectUI* picker = LLFloaterMuteObjectUI::show(callbackMuteByName,data);
	assert(picker);

	LLFloaterMute* floaterp = (LLFloaterMute*)data;
	floaterp->addDependentFloater(picker);
}

void LLFloaterMute::callbackMuteByName(const std::string& text, void* data)
{
	if (text.empty()) return;

	LLMute mute(LLUUID::null, text, LLMute::BY_NAME);
	LLMuteList::getInstance()->add(mute);
}

void LLFloaterMute::onMuteAllToggled(LLUICtrl* ctrl, void* data)
{
	LLFloaterMute* self  = (LLFloaterMute*)data;
	if (self)
	{
		self->childSetValue("mute_chat", FALSE);
		self->childSetValue("mute_voice", FALSE);
		self->childSetValue("mute_sounds", FALSE);
		self->childSetValue("mute_particles", FALSE);
		self->childSetEnabled("update_mutes", true);
	}
}

void LLFloaterMute::onMuteTypeToggled(LLUICtrl* ctrl, void* data)
{
	LLFloaterMute* self  = (LLFloaterMute*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;
	if (!self || !check) return;
	if (check->get())
	{
		self->childSetValue("mute_all", FALSE);
		self->childSetEnabled("mute_all", true);
	}
	else
	{
		BOOL enabled = !(self->childGetValue("mute_chat") ||
						 self->childGetValue("mute_voice") ||
						 self->childGetValue("mute_sounds") ||
						 self->childGetValue("mute_particles"));
		self->childSetValue("mute_all", enabled);
		self->childSetEnabled("mute_all", !enabled);
	}
	self->childSetEnabled("update_mutes", true);
}

void LLFloaterMute::onClickUpdateMutes(void *data)
{
	LLFloaterMute* self = (LLFloaterMute*)data;

	std::string name = self->mMuteList->getSelectedItemLabel();
	LLUUID id = self->mMuteList->getStringUUIDSelectedItem();
	LLMute mute(id);
	mute.setFromDisplayName(name); // now mute.mName has the suffix trimmed off

	U32 flags = 0;
	if (!self->childGetValue("mute_all"))
	{
		if (self->childGetValue("mute_chat"))
		{
			flags |= LLMute::flagTextChat;
		}
		if (self->childGetValue("mute_voice"))
		{
			flags |= LLMute::flagVoiceChat;
		}
		if (self->childGetValue("mute_sounds"))
		{
			flags |= LLMute::flagObjectSounds;
		}
		if (self->childGetValue("mute_particles"))
		{
			flags |= LLMute::flagParticles;
		}
	}

	// Refresh the mute entry by removing the mute then re-adding it.
	S32 last_selected = self->mMuteList->getFirstSelectedIndex();
	LLMuteList::getInstance()->remove(mute);
	LLMuteList::getInstance()->add(mute, flags);
	self->mMuteList->selectNthItem(last_selected);

	self->updateButtons();
}
