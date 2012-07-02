/** 
 * @file llfloateractivespeakers.cpp
 * @brief Management interface for muting and controlling volume of residents currently speaking
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llfloateractivespeakers.h"

#include "llbutton.h"
#include "llscrolllistctrl.h"
#include "llsdutil.h"
#include "llslider.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llfloateravatarinfo.h"
#include "llfloaterobjectiminfo.h"
#include "llimpanel.h"				// LLVoiceChannel
#include "llimview.h"
#include "llmutelist.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llworld.h"

using namespace LLOldEvents;

// seconds of not being on voice channel before removed from list of active speakers
const F32 SPEAKER_TIMEOUT = 10.f;
// seconds of mouse inactivity before it's ok to sort regardless of mouse-in-view.
const F32 RESORT_TIMEOUT = 5.f;
const LLColor4 INACTIVE_COLOR(0.3f, 0.3f, 0.3f, 0.5f);
const LLColor4 ACTIVE_COLOR(0.5f, 0.5f, 0.5f, 1.f);
const F32 TYPING_ANIMATION_FPS = 2.5f;

LLSpeaker::LLSpeaker(const LLUUID& id,
					 const std::string& name,
					 const ESpeakerType type)
:	mStatus(LLSpeaker::STATUS_TEXT_ONLY),
	mLastSpokeTime(0.f), 
	mSpeechVolume(0.f), 
	mHasSpoken(FALSE),
	mDotColor(LLColor4::white),
	mID(id),
	mTyping(FALSE),
	mSortIndex(0),
	mType(type),
	mIsModerator(FALSE),
	mModeratorMutedVoice(FALSE),
	mModeratorMutedText(FALSE)
{
	if (name.empty() && type == SPEAKER_AGENT)
	{
		lookupName();
	}
	else
	{
		mDisplayName = name;
		mLegacyName = name;
	}

	LLMuteList* ml = LLMuteList::getInstance();
	if (ml)
	{
		LLVoiceClient::getInstance()->setUserVolume(id,
													ml->getSavedResidentVolume(id));
	}

	mActivityTimer.resetWithExpiry(SPEAKER_TIMEOUT);
}

void LLSpeaker::lookupName()
{
	LLAvatarNameCache::get(mID, boost::bind(&LLSpeaker::onAvatarNameLookup,
											_1, _2,
											new LLHandle<LLSpeaker>(getHandle())));
}

//static 
void LLSpeaker::onAvatarNameLookup(const LLUUID& id,
								   const LLAvatarName& avatar_name,
								   void* user_data)
{
	LLSpeaker* speaker_ptr = ((LLHandle<LLSpeaker>*)user_data)->get();

	if (speaker_ptr)
	{
		// Must keep "Resident" last names, thus the "true"
		speaker_ptr->mLegacyName = avatar_name.getLegacyName(true);
		if (LLAvatarNameCache::useDisplayNames())
		{
			// Always show "Display Name [Legacy Name]" for security reasons
			speaker_ptr->mDisplayName = avatar_name.getNames();
		}
		else
		{
			// "Resident" last names stripped when appropriate
			speaker_ptr->mDisplayName = avatar_name.getLegacyName();
		}
	}

	delete (LLHandle<LLSpeaker>*)user_data;
}

LLSpeakerTextModerationEvent::LLSpeakerTextModerationEvent(LLSpeaker* source)
: LLEvent(source, "Speaker text moderation event")
{
}

LLSD LLSpeakerTextModerationEvent::getValue()
{
	return std::string("text");
}

LLSpeakerVoiceModerationEvent::LLSpeakerVoiceModerationEvent(LLSpeaker* source)
:	LLEvent(source, "Speaker voice moderation event")
{
}

LLSD LLSpeakerVoiceModerationEvent::getValue()
{
	return std::string("voice");
}

LLSpeakerListChangeEvent::LLSpeakerListChangeEvent(LLSpeakerMgr* source,
												   const LLUUID& speaker_id)
:	LLEvent(source, "Speaker added/removed from speaker mgr"),
	mSpeakerID(speaker_id)
{
}

LLSD LLSpeakerListChangeEvent::getValue()
{
	return mSpeakerID;
}

// helper sort class
struct LLSortRecentSpeakers
{
	bool operator()(const LLPointer<LLSpeaker> lhs,
					const LLPointer<LLSpeaker> rhs) const;
};

bool LLSortRecentSpeakers::operator()(const LLPointer<LLSpeaker> lhs,
									  const LLPointer<LLSpeaker> rhs) const
{
	// Sort first on status
	if (lhs->mStatus != rhs->mStatus) 
	{
		return (lhs->mStatus < rhs->mStatus);
	}

	// and then on last speaking time
	if (lhs->mLastSpokeTime != rhs->mLastSpokeTime)
	{
		return (lhs->mLastSpokeTime > rhs->mLastSpokeTime);
	}

	// and finally (only if those are both equal), on name.
	return (lhs->mDisplayName.compare(rhs->mDisplayName) < 0);
}

//
// LLFloaterActiveSpeakers
//

LLFloaterActiveSpeakers::LLFloaterActiveSpeakers(const LLSD& seed)
:	mPanel(NULL)
{
	mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, NULL);
	// do not automatically open singleton floaters (as result of getInstance())
	BOOL no_open = FALSE;
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_active_speakers.xml",
												 &getFactoryMap(), no_open);
	// RN: for now, we poll voice client every frame to get voice amplitude
	// feedback
	//LLVoiceClient::getInstance()->addObserver(this);
	mPanel->refreshSpeakers();
}

LLFloaterActiveSpeakers::~LLFloaterActiveSpeakers()
{
}

void LLFloaterActiveSpeakers::onOpen()
{
	gSavedSettings.setBOOL("ShowActiveSpeakers", TRUE);
}

void LLFloaterActiveSpeakers::onClose(bool app_quitting)
{
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowActiveSpeakers", FALSE);
	}
	setVisible(FALSE);
}

void LLFloaterActiveSpeakers::draw()
{
	// update state every frame to get live amplitude feedback
	mPanel->refreshSpeakers();
	LLFloater::draw();
}

BOOL LLFloaterActiveSpeakers::postBuild()
{
	mPanel = getChild<LLPanelActiveSpeakers>("active_speakers_panel");
	return TRUE;
}

void LLFloaterActiveSpeakers::onChange()
{
	//refresh();
}

//static
void* LLFloaterActiveSpeakers::createSpeakersPanel(void* data)
{
	// don't show text only speakers
	return new LLPanelActiveSpeakers(LLActiveSpeakerMgr::getInstance(), FALSE);
}

//
// LLPanelActiveSpeakers::SpeakerMuteListener
//
bool LLPanelActiveSpeakers::SpeakerMuteListener::handleEvent(LLPointer<LLEvent> event,
															 const LLSD& userdata)
{
	LLPointer<LLSpeaker> speakerp = (LLSpeaker*)event->getSource();
	if (speakerp.isNull()) return false;

	// update UI on confirmation of moderator mutes
	if (mPanel->mModeratorAllowVoiceCtrl)
	{
		if (event->getValue().asString() == "voice")
		{
			mPanel->mModeratorAllowVoiceCtrl->setValue(!speakerp->mModeratorMutedVoice);
		}
	}
	if (mPanel->mModeratorAllowTextCtrl)
	{
		if (event->getValue().asString() == "text")
		{
			mPanel->mModeratorAllowTextCtrl->setValue(!speakerp->mModeratorMutedText);
		}
	}
	return true;
}

//
// LLPanelActiveSpeakers::SpeakerAddListener
//
bool LLPanelActiveSpeakers::SpeakerAddListener::handleEvent(LLPointer<LLEvent> event,
															const LLSD& userdata)
{
	mPanel->addSpeaker(event->getValue().asUUID());
	return true;
}

//
// LLPanelActiveSpeakers::SpeakerRemoveListener
//
bool LLPanelActiveSpeakers::SpeakerRemoveListener::handleEvent(LLPointer<LLEvent> event,
															   const LLSD& userdata)
{
	mPanel->removeSpeaker(event->getValue().asUUID());
	return true;
}

//
// LLPanelActiveSpeakers::SpeakerClearListener
//
bool LLPanelActiveSpeakers::SpeakerClearListener::handleEvent(LLPointer<LLEvent> event,
															  const LLSD& userdata)
{
	mPanel->mSpeakerList->clearRows();
	return true;
}

//
// LLPanelActiveSpeakers
//
LLPanelActiveSpeakers::LLPanelActiveSpeakers(LLSpeakerMgr* data_source,
											 BOOL show_text_chatters)
:	mSpeakerList(NULL),
	mSpeakerVolumeSlider(NULL),
	mMuteVoiceCtrl(NULL),
	mMuteTextCtrl(NULL),
	mModeratorAllowVoiceCtrl(NULL),
	mModeratorAllowTextCtrl(NULL),
	mModerationModeCtrl(NULL),
	mModeratorControlsText(NULL),
	mNameText(NULL),
	mProfileBtn(NULL),
	mShowTextChatters(show_text_chatters),
	mSpeakerMgr(data_source)
{
	setMouseOpaque(FALSE);
	mSpeakerMuteListener = new SpeakerMuteListener(this);
	mSpeakerAddListener = new SpeakerAddListener(this);
	mSpeakerRemoveListener = new SpeakerRemoveListener(this);
	mSpeakerClearListener = new SpeakerClearListener(this);

	mSpeakerMgr->addListener(mSpeakerAddListener, "add");
	mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
	mSpeakerMgr->addListener(mSpeakerClearListener, "clear");
}

BOOL LLPanelActiveSpeakers::postBuild()
{
	std::string sort_column = gSavedSettings.getString("FloaterActiveSpeakersSortColumn");
	BOOL sort_ascending = gSavedSettings.getBOOL("FloaterActiveSpeakersSortAscending");

	mSpeakerList = getChild<LLScrollListCtrl>("speakers_list");
	mSpeakerList->sortByColumn(sort_column, sort_ascending);
	mSpeakerList->setDoubleClickCallback(onDoubleClickSpeaker);
	mSpeakerList->setCommitOnSelectionChange(TRUE);
	mSpeakerList->setCommitCallback(onSelectSpeaker);
	mSpeakerList->setSortChangedCallback(onSortChanged);
	mSpeakerList->setCallbackUserData(this);

	mMuteTextCtrl = getChild<LLUICtrl>("mute_text_btn", TRUE, FALSE);
	if (mMuteTextCtrl)
	{
		childSetCommitCallback("mute_text_btn", onClickMuteTextCommit, this);
	}

	mMuteVoiceCtrl = getChild<LLUICtrl>("mute_check", TRUE, FALSE);
	if (mMuteVoiceCtrl)
	{
		// For the mute check box, in floater_chat_history.xml
		childSetCommitCallback("mute_check", onClickMuteVoiceCommit, this);
	}

	if (getChild<LLButton>("mute_btn", TRUE, FALSE))
	{
		// For the mute buttons, everywhere else
		childSetAction("mute_btn", onClickMuteVoice, this);
	}

	mSpeakerVolumeSlider = getChild<LLSlider>("speaker_volume", TRUE, FALSE);
	if (mSpeakerVolumeSlider)
	{
		mSpeakerVolumeSlider->setCommitCallback(onVolumeChange);
		mSpeakerVolumeSlider->setCallbackUserData(this);
	}

	mNameText = getChild<LLTextBox>("resident_name", TRUE, FALSE);

	mProfileBtn = getChild<LLButton>("profile_btn", TRUE, FALSE);
	if (mProfileBtn)
	{
		childSetAction("profile_btn", onClickProfile, this);
	}

	mModeratorAllowVoiceCtrl = getChild<LLUICtrl>("moderator_allow_voice",
												  TRUE, FALSE);
	if (mModeratorAllowVoiceCtrl)
	{
		mModeratorAllowVoiceCtrl->setCommitCallback(onModeratorMuteVoice);
		mModeratorAllowVoiceCtrl->setCallbackUserData(this);

		mModeratorAllowTextCtrl = getChild<LLUICtrl>("moderator_allow_text",
													 TRUE, FALSE);
		if (mModeratorAllowTextCtrl)
		{
			mModeratorAllowTextCtrl->setCommitCallback(onModeratorMuteText);
			mModeratorAllowTextCtrl->setCallbackUserData(this);
		}

		mModerationModeCtrl = getChild<LLUICtrl>("moderation_mode",
												 TRUE, FALSE);
		if (mModerationModeCtrl)
		{
			mModerationModeCtrl->setCommitCallback(onChangeModerationMode);
			mModerationModeCtrl->setCallbackUserData(this);
		}

		mModeratorControlsText = getChild<LLTextBox>("moderator_controls_label",
													 TRUE, FALSE);
	}

	// update speaker UI
	handleSpeakerSelect();

	return TRUE;
}

void LLPanelActiveSpeakers::addSpeaker(const LLUUID& speaker_id)
{
	if (mSpeakerList->getItemIndex(speaker_id) >= 0)
	{
		// already have this speaker
		return;
	}

	LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp)
	{
		// since we are forced to sort by text, encode sort order as string
		std::string speaking_order_sort_string = llformat("%010d",
														  speakerp->mSortIndex);

		LLSD row;
		row["id"] = speaker_id;

		LLSD& columns = row["columns"];

		columns[0]["column"] = "icon_speaking_status";
		columns[0]["type"] = "icon";
		columns[0]["value"] = "icn_active-speakers-dot-lvl0.tga";

		std::string speaker_name;
		if (speakerp->mDisplayName.empty())
		{
			speaker_name = LLCacheName::getDefaultName();
		}
		else
		{
			speaker_name = speakerp->mDisplayName;
		}
		columns[1]["column"] = "speaker_name";
		columns[1]["type"] = "text";
		columns[1]["value"] = speaker_name;

		columns[2]["column"] = "speaking_status";
		columns[2]["type"] = "text";

		// print speaking ordinal in a text-sorting friendly manner
		columns[2]["value"] = speaking_order_sort_string;

		mSpeakerList->addElement(row);
	}
}

void LLPanelActiveSpeakers::removeSpeaker(const LLUUID& speaker_id)
{
	mSpeakerList->deleteSingleItem(mSpeakerList->getItemIndex(speaker_id));
}

void LLPanelActiveSpeakers::handleSpeakerSelect()
{
	LLUUID speaker_id = mSpeakerList->getValue().asUUID();
	LLPointer<LLSpeaker> selected_speakerp = mSpeakerMgr->findSpeaker(speaker_id);

	if (selected_speakerp.notNull())
	{
		// since setting these values is delayed by a round trip to the Vivox servers
		// update them only when selecting a new speaker or
		// asynchronously when an update arrives
		if (mModeratorAllowVoiceCtrl)
		{
			mModeratorAllowVoiceCtrl->setValue(selected_speakerp ?
											   !selected_speakerp->mModeratorMutedVoice :
											   TRUE);
		}
		if (mModeratorAllowTextCtrl)
		{
			mModeratorAllowTextCtrl->setValue(selected_speakerp ?
											  !selected_speakerp->mModeratorMutedText :
											  TRUE);
		}

		mSpeakerMuteListener->clearDispatchers();
		selected_speakerp->addListener(mSpeakerMuteListener);
	}
}

void LLPanelActiveSpeakers::refreshSpeakers()
{
	static const LLUIImagePtr icon_image_0 = LLUI::getUIImage("icn_active-speakers-dot-lvl0.tga");
	static const LLUIImagePtr icon_image_1 = LLUI::getUIImage("icn_active-speakers-dot-lvl1.tga");
	static const LLUIImagePtr icon_image_2 = LLUI::getUIImage("icn_active-speakers-dot-lvl2.tga");
	static const LLUIImagePtr mute_icon_image = LLUI::getUIImage("mute_icon.tga");

	// store off current selection and scroll state to preserve across list
	// rebuilds
	LLUUID selected_id = mSpeakerList->getSelectedValue().asUUID();
	S32 scroll_pos = mSpeakerList->getScrollInterface()->getScrollPos();

	// Decide whether it's ok to resort the list then update the speaker
	// manager appropriately. Rapid resorting by activity makes it hard to
	// interact with speakers in the list so we freeze the sorting while the
	// user appears to be interacting with the control. We assume this is the
	// case whenever the mouse pointer is within the active speaker panel and
	// hasn't been motionless for more than a few seconds. see DEV-6655 -MG
	LLRect screen_rect;
	localRectToScreen(getLocalRect(), &screen_rect);
	BOOL mouse_in_view = screen_rect.pointInRect(gViewerWindow->getCurrentMouseX(),
												 gViewerWindow->getCurrentMouseY());
	F32 mouses_last_movement = gMouseIdleTimer.getElapsedTimeF32();
	BOOL sort_ok = ! (mouse_in_view && mouses_last_movement<RESORT_TIMEOUT);
	mSpeakerMgr->update(sort_ok);

	std::vector<LLScrollListItem*> items = mSpeakerList->getAllData();

	LLSpeakerMgr::speaker_list_t speaker_list;
	mSpeakerMgr->getSpeakerList(&speaker_list, mShowTextChatters);
	for (std::vector<LLScrollListItem*>::iterator item_it = items.begin(),
												  end = items.end();
		 item_it != end; ++item_it)
	{
		LLScrollListItem* itemp = *item_it;
		LLUUID speaker_id = itemp->getUUID();

		LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
		if (!speakerp)
		{
			continue;
		}

		// since we are forced to sort by text, encode sort order as string
		std::string speaking_order_sort_string = llformat("%010d",
														  speakerp->mSortIndex);

		LLScrollListIcon* icon_cell = dynamic_cast<LLScrollListIcon*>(itemp->getColumn(0));
		if (icon_cell)
		{
			LLUIImagePtr icon_image_id;

			S32 icon_image_idx = llmin(2, llfloor(3.0f * speakerp->mSpeechVolume /
												  LLVoiceClient::OVERDRIVEN_POWER_LEVEL));
			switch (icon_image_idx)
			{
				case 0:
					icon_image_id = icon_image_0;
					break;
				case 1:
					icon_image_id = icon_image_1;
					break;
				case 2:
					icon_image_id = icon_image_2;
					break;
			}

			LLColor4 icon_color;
			if (speakerp->mStatus == LLSpeaker::STATUS_MUTED)
			{
				icon_cell->setImage(mute_icon_image);
				if (speakerp->mModeratorMutedVoice)
				{
					icon_color.setVec(0.5f, 0.5f, 0.5f, 1.f);
				}
				else
				{
					icon_color.setVec(1.f, 71.f / 255.f, 71.f / 255.f, 1.f);
				}
			}
			else
			{
				icon_cell->setImage(icon_image_id);
				icon_color = speakerp->mDotColor;

				// if voice is disabled for this speaker
				if (speakerp->mStatus > LLSpeaker::STATUS_VOICE_ACTIVE)
				{
					// non voice speakers have hidden icons, render as transparent
					icon_color.setVec(0.f, 0.f, 0.f, 0.f);
				}
			}

			icon_cell->setColor(icon_color);

			// if voice is disabled for this speaker
			if (speakerp->mStatus > LLSpeaker::STATUS_VOICE_ACTIVE &&
				speakerp->mStatus != LLSpeaker::STATUS_MUTED)
			{
				// non voice speakers have hidden icons, render as transparent
				icon_cell->setColor(LLColor4::transparent);
			}
		}

		// update name column
		LLScrollListCell* name_cell = itemp->getColumn(1);
		if (name_cell)
		{
			//FIXME: remove hard coding of font colors
			if (speakerp->mStatus == LLSpeaker::STATUS_NOT_IN_CHANNEL)
			{
				// draw inactive speakers in gray
				name_cell->setColor(LLColor4::grey4);
			}
			else
			{
				name_cell->setColor(LLColor4::black);
			}

			std::string speaker_name;
			if (speakerp->mDisplayName.empty())
			{
				speaker_name = LLCacheName::getDefaultName();
			}
			else
			{
				speaker_name = speakerp->mDisplayName;
			}

			if (speakerp->mIsModerator)
			{
				speaker_name += " " + getString("moderator_label");
			}

			name_cell->setValue(speaker_name);
			((LLScrollListText*)name_cell)->setFontStyle(speakerp->mIsModerator ? LLFontGL::BOLD
																				: LLFontGL::NORMAL);
		}

		// update speaking order column
		LLScrollListCell* speaking_status_cell = itemp->getColumn(2);
		if (speaking_status_cell)
		{
			// print speaking ordinal in a text-sorting friendly manner
			speaking_status_cell->setValue(speaking_order_sort_string);
		}
	}

	// we potentially modified the sort order by touching the list items
	mSpeakerList->setSorted(FALSE);

	LLMuteList* ml = LLMuteList::getInstance();
	LLVoiceClient* voice_client = LLVoiceClient::getInstance();

	LLPointer<LLSpeaker> selected_speakerp = mSpeakerMgr->findSpeaker(selected_id);
	// update UI for selected participant
	bool valid_speaker = selected_id.notNull() && selected_id != gAgentID &&
						 selected_speakerp.notNull();
	bool speaker_on_voice = LLVoiceClient::voiceEnabled() &&
							voice_client->getVoiceEnabled(selected_id);
	if (mMuteVoiceCtrl)
	{
		mMuteVoiceCtrl->setValue(ml && ml->isMuted(selected_id, LLMute::flagVoiceChat));
		mMuteVoiceCtrl->setEnabled(speaker_on_voice && valid_speaker && 
								   (selected_speakerp->mType == LLSpeaker::SPEAKER_AGENT ||
									selected_speakerp->mType == LLSpeaker::SPEAKER_EXTERNAL));

	}
	if (mMuteTextCtrl)
	{
		mMuteTextCtrl->setValue(ml && ml->isMuted(selected_id, LLMute::flagTextChat));
		mMuteTextCtrl->setEnabled(valid_speaker &&
								  selected_speakerp->mType != LLSpeaker::SPEAKER_EXTERNAL &&
								  ml && !ml->isLinden(selected_speakerp->mLegacyName));
	}

	if (mSpeakerVolumeSlider)
	{
		mSpeakerVolumeSlider->setValue(voice_client->getUserVolume(selected_id));
		mSpeakerVolumeSlider->setEnabled(speaker_on_voice && valid_speaker &&
										 (selected_speakerp->mType == LLSpeaker::SPEAKER_AGENT ||
										 selected_speakerp->mType == LLSpeaker::SPEAKER_EXTERNAL));
	}

	if (mModeratorAllowVoiceCtrl)
	{
		mModeratorAllowVoiceCtrl->setEnabled(selected_id.notNull() &&
											 mSpeakerMgr->isVoiceActive() &&
											 voice_client->getVoiceEnabled(selected_id));
	}
	if (mModeratorAllowTextCtrl)
	{
		mModeratorAllowTextCtrl->setEnabled(selected_id.notNull());
	}
	if (mModeratorControlsText)
	{
		mModeratorControlsText->setEnabled(selected_id.notNull());
	}

	if (mProfileBtn)
	{
		mProfileBtn->setEnabled(selected_id.notNull() &&
								selected_speakerp.notNull() &&
								selected_speakerp->mType != LLSpeaker::SPEAKER_EXTERNAL);
	}

	// show selected user name in large font
	if (mNameText)
	{
		if (selected_speakerp)
		{
			mNameText->setValue(selected_speakerp->mDisplayName);
		}
		else
		{
			mNameText->setValue(LLStringUtil::null);
		}
	}

	//update moderator capabilities
	LLPointer<LLSpeaker> self_speakerp = mSpeakerMgr->findSpeaker(gAgentID);
	if (self_speakerp)
	{
		childSetVisible("moderation_mode_panel",
						self_speakerp->mIsModerator &&
						mSpeakerMgr->isVoiceActive());
		childSetVisible("moderator_controls", self_speakerp->mIsModerator);
	}

	// keep scroll value stable
	mSpeakerList->getScrollInterface()->setScrollPos(scroll_pos);
}

void LLPanelActiveSpeakers::setSpeaker(const LLUUID& id,
									   const std::string& name,
									   LLSpeaker::ESpeakerStatus status,
									   LLSpeaker::ESpeakerType type,
									   const LLUUID& owner_id)
{
	mSpeakerMgr->setSpeaker(id, name, status, type, owner_id);
}

void LLPanelActiveSpeakers::setVoiceModerationCtrlMode(const BOOL& moderated_voice)
{
	if (mModerationModeCtrl)
	{
		std::string value = moderated_voice ? "moderated" : "unmoderated";
		mModerationModeCtrl->setValue(value);
	}
}

//static
void LLPanelActiveSpeakers::onClickMuteTextCommit(LLUICtrl* ctrl,
												  void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	LLMuteList* ml = LLMuteList::getInstance();
	if (!panelp || !ml) return;

	LLUUID speaker_id = panelp->mSpeakerList->getValue().asUUID();
	BOOL is_muted = ml->isMuted(speaker_id, LLMute::flagTextChat);

	//fill in name using voice client's copy of name cache
	LLPointer<LLSpeaker> speakerp = panelp->mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull())
	{
		return;
	}

	std::string name = speakerp->mLegacyName;

	LLMute mute(speaker_id, name,
				speakerp->mType == LLSpeaker::SPEAKER_AGENT ? LLMute::AGENT
															: LLMute::OBJECT);

	if (!is_muted)
	{
		ml->add(mute, LLMute::flagTextChat);
	}
	else
	{
		ml->remove(mute, LLMute::flagTextChat);
	}
}

//static
void LLPanelActiveSpeakers::onClickMuteVoice(void* user_data)
{
	onClickMuteVoiceCommit(NULL, user_data);
}

//static
void LLPanelActiveSpeakers::onClickMuteVoiceCommit(LLUICtrl* ctrl, void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	LLMuteList* ml = LLMuteList::getInstance();
	if (!panelp || !ml) return;

	LLUUID speaker_id = panelp->mSpeakerList->getValue().asUUID();
	BOOL is_muted = ml->isMuted(speaker_id, LLMute::flagVoiceChat);

	LLPointer<LLSpeaker> speakerp = panelp->mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull())
	{
		return;
	}

	std::string name = speakerp->mLegacyName;

	// muting voice means we're dealing with an agent
	LLMute mute(speaker_id, name, LLMute::AGENT);

	if (!is_muted)
	{
		ml->add(mute, LLMute::flagVoiceChat);
	}
	else
	{
		ml->remove(mute, LLMute::flagVoiceChat);
	}
}

//static
void LLPanelActiveSpeakers::onVolumeChange(LLUICtrl* source, void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	if (panelp && panelp->mSpeakerVolumeSlider)
	{
		LLUUID speaker_id = panelp->mSpeakerList->getValue().asUUID();
		F32 new_volume = panelp->mSpeakerVolumeSlider->getValue().asReal();

		LLVoiceClient::getInstance()->setUserVolume(speaker_id, new_volume);

		// store this volume setting for future sessions
		LLMuteList* ml = LLMuteList::getInstance();
		if (ml)
		{
			ml->setSavedResidentVolume(speaker_id, new_volume);
		}
	}
}

//static 
void LLPanelActiveSpeakers::onClickProfile(void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	if (!panelp) return;

	LLUUID speaker_id = panelp->mSpeakerList->getValue().asUUID();
	LLPointer<LLSpeaker> speakerp = panelp->mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull()) return;

	if (speakerp->mType == LLSpeaker::SPEAKER_AGENT)
	{
		LLFloaterAvatarInfo::showFromDirectory(speaker_id);
	}
	else if (speakerp->mType == LLSpeaker::SPEAKER_OBJECT)
	{
		LLViewerObject* object = gObjectList.findObject(speaker_id);
		if (!object)
		{
			// Others' HUDs are not in our objects list: use the HUD owner
			// to find out their actual position...
			object = gObjectList.findObject(speakerp->mOwnerID);
		}
		if (object
//MK
			&& !(gRRenabled && gAgent.mRRInterface.mContainsShowloc))
//mk
		{
			if (!gRRenabled || !gAgent.mRRInterface.mContainsShowloc)
			{
				LLVector3 pos = object->getPositionRegion();
				S32 x = llround((F32)fmod((F64)pos.mV[VX], (F64)REGION_WIDTH_METERS));
				S32 y = llround((F32)fmod((F64)pos.mV[VY], (F64)REGION_WIDTH_METERS));
				S32 z = llround((F32)pos.mV[VZ]);
				std::ostringstream location;
				location << object->getRegion()->getName() << "/" << x << "/"
						 << y << "/" << z;
				LLObjectIMInfo::show(speaker_id, speakerp->mDisplayName,
									 location.str(), speakerp->mOwnerID,
									 false);
			}
		}
	}
}

//static
void LLPanelActiveSpeakers::onDoubleClickSpeaker(void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	if (!panelp) return;

	LLUUID speaker_id = panelp->mSpeakerList->getValue().asUUID();

	LLPointer<LLSpeaker> speakerp = panelp->mSpeakerMgr->findSpeaker(speaker_id);

	if (gIMMgr && speaker_id != gAgentID && speakerp.notNull())
	{
		gIMMgr->addSession(speakerp->mLegacyName, IM_NOTHING_SPECIAL, speaker_id);
	}
}

//static
void LLPanelActiveSpeakers::onSelectSpeaker(LLUICtrl* source, void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	if (panelp)
	{
		panelp->handleSpeakerSelect();
	}
}

//static
void LLPanelActiveSpeakers::onSortChanged(void* user_data)
{
	LLPanelActiveSpeakers* panelp = (LLPanelActiveSpeakers*)user_data;
	if (!panelp) return;

	std::string sort_column = panelp->mSpeakerList->getSortColumnName();
	BOOL sort_ascending = panelp->mSpeakerList->getSortAscending();
	gSavedSettings.setString("FloaterActiveSpeakersSortColumn", sort_column);
	gSavedSettings.setBOOL("FloaterActiveSpeakersSortAscending", sort_ascending);
}

//static 
void LLPanelActiveSpeakers::onModeratorMuteVoice(LLUICtrl* ctrl,
												 void* user_data)
{
	LLPanelActiveSpeakers* self = (LLPanelActiveSpeakers*)user_data;
	if (!self || !self->mSpeakerList || !gAgent.getRegion()) return;

	std::string url = gAgent.getRegion()->getCapability("ChatSessionRequest");
	LLSD data;
	data["method"] = "mute update";
	data["session-id"] = self->mSpeakerMgr->getSessionID();
	data["params"] = LLSD::emptyMap();
	data["params"]["agent_id"] = self->mSpeakerList->getValue();
	data["params"]["mute_info"] = LLSD::emptyMap();
	// ctrl value represents ability to type, so invert
	data["params"]["mute_info"]["voice"] = !ctrl->getValue();

	class MuteVoiceResponder : public LLHTTPClient::Responder
	{
	public:
		MuteVoiceResponder(const LLUUID& session_id)
		{
			mSessionID = session_id;
		}

		virtual void error(U32 status, const std::string& reason)
		{
			llwarns << status << ": " << reason << llendl;

			if (gIMMgr)
			{
				// 403 == you're not a mod
				// should be disabled if you're not a moderator
				LLFloaterIMPanel* floaterp;

				floaterp = gIMMgr->findFloaterBySession(mSessionID);

				if (floaterp)
				{
					if (403 == status)
					{
						floaterp->showSessionEventError("mute",
														"not_a_moderator");
					}
					else
					{
						floaterp->showSessionEventError("mute", "generic");
					}
				}
			}
		}

	private:
		LLUUID mSessionID;
	};

	LLHTTPClient::post(url, data,
					   new MuteVoiceResponder(self->mSpeakerMgr->getSessionID()));
}

//static 
void LLPanelActiveSpeakers::onModeratorMuteText(LLUICtrl* ctrl,
												void* user_data)
{
	LLPanelActiveSpeakers* self = (LLPanelActiveSpeakers*)user_data;
	if (!self || !self->mSpeakerList || !gAgent.getRegion()) return;

	std::string url = gAgent.getRegion()->getCapability("ChatSessionRequest");
	LLSD data;
	data["method"] = "mute update";
	data["session-id"] = self->mSpeakerMgr->getSessionID();
	data["params"] = LLSD::emptyMap();
	data["params"]["agent_id"] = self->mSpeakerList->getValue();
	data["params"]["mute_info"] = LLSD::emptyMap();
	// ctrl value represents ability to type, so invert
	data["params"]["mute_info"]["text"] = !ctrl->getValue();

	class MuteTextResponder : public LLHTTPClient::Responder
	{
	public:
		MuteTextResponder(const LLUUID& session_id)
		{
			mSessionID = session_id;
		}

		virtual void error(U32 status, const std::string& reason)
		{
			llwarns << status << ": " << reason << llendl;

			if (gIMMgr)
			{
				// 403 == you're not a mod
				// should be disabled if you're not a moderator
				LLFloaterIMPanel* floaterp;

				floaterp = gIMMgr->findFloaterBySession(mSessionID);

				if (floaterp)
				{
					if (403 == status)
					{
						floaterp->showSessionEventError("mute",
														"not_a_moderator");
					}
					else
					{
						floaterp->showSessionEventError("mute", "generic");
					}
				}
			}
		}

	private:
		LLUUID mSessionID;
	};

	LLHTTPClient::post(url, data,
					   new MuteTextResponder(self->mSpeakerMgr->getSessionID()));
}

//static
void LLPanelActiveSpeakers::onChangeModerationMode(LLUICtrl* ctrl,
												   void* user_data)
{
	LLPanelActiveSpeakers* self = (LLPanelActiveSpeakers*)user_data;
	if (!gAgent.getRegion()) return;

	std::string url = gAgent.getRegion()->getCapability("ChatSessionRequest");
	LLSD data;
	data["method"] = "session update";
	data["session-id"] = self->mSpeakerMgr->getSessionID();
	data["params"] = LLSD::emptyMap();

	data["params"]["update_info"] = LLSD::emptyMap();

	data["params"]["update_info"]["moderated_mode"] = LLSD::emptyMap();
	if (ctrl->getValue().asString() == "unmoderated")
	{
		data["params"]["update_info"]["moderated_mode"]["voice"] = false;
	}
	else if (ctrl->getValue().asString() == "moderated")
	{
		data["params"]["update_info"]["moderated_mode"]["voice"] = true;
	}

	struct ModerationModeResponder : public LLHTTPClient::Responder
	{
		virtual void error(U32 status, const std::string& reason)
		{
			llwarns << status << ": " << reason << llendl;
		}
	};

	LLHTTPClient::post(url, data, new ModerationModeResponder());
}

//
// LLSpeakerMgr
//

LLSpeakerMgr::LLSpeakerMgr(LLVoiceChannel* channelp)
:	mVoiceChannel(channelp)
{
}

LLSpeakerMgr::~LLSpeakerMgr()
{
}

LLPointer<LLSpeaker> LLSpeakerMgr::setSpeaker(const LLUUID& id,
											  const std::string& name,
											  LLSpeaker::ESpeakerStatus status,
											  LLSpeaker::ESpeakerType type,
											  const LLUUID& owner_id)
{
	if (id.isNull()) return NULL;

	LLPointer<LLSpeaker> speakerp;
	if (mSpeakers.find(id) == mSpeakers.end())
	{
		speakerp = new LLSpeaker(id, name, type);
		speakerp->mStatus = status;
		speakerp->mOwnerID = owner_id;
		mSpeakers.insert(std::make_pair(speakerp->mID, speakerp));
		mSpeakersSorted.push_back(speakerp);
		fireEvent(new LLSpeakerListChangeEvent(this, speakerp->mID), "add");
	}
	else
	{
		speakerp = findSpeaker(id);
		if (speakerp.notNull())
		{
			// keep highest priority status (lowest value) instead of
			// overriding current value
			speakerp->mStatus = llmin(speakerp->mStatus, status);
			speakerp->mActivityTimer.resetWithExpiry(SPEAKER_TIMEOUT);
			// RN: due to a weird behavior where IMs from attached objects come
			// from the wearer's agent_id we need to override speakers that we
			// think are objects when we find out they are really residents
			if (type == LLSpeaker::SPEAKER_AGENT)
			{
				speakerp->mType = LLSpeaker::SPEAKER_AGENT;
				speakerp->lookupName();
			}
		}
	}

	return speakerp;
}

void LLSpeakerMgr::update(BOOL resort_ok)
{
	if (!LLVoiceClient::instanceExists())
	{
		return;
	}

	LLVoiceClient* voice_client = LLVoiceClient::getInstance();

	static LLCachedControl<LLColor4U> speaking(gSavedSettings,
											   "SpeakingColor");
	static LLCachedControl<LLColor4U> overdriven(gSavedSettings,
												 "OverdrivenColor");
	LLColor4 speaking_color = LLColor4(speaking);
	LLColor4 overdriven_color = LLColor4(speaking_color);

	if (resort_ok) // only allow list changes when user is not interacting with it
	{
		updateSpeakerList();
	}

	// update status of all current speakers
	BOOL voice_channel_active = (mVoiceChannel && mVoiceChannel->isActive()) ||
								(!mVoiceChannel &&
								 voice_client->inProximalChannel());
	for (speaker_map_t::iterator speaker_it = mSpeakers.begin(),
								 end = mSpeakers.end();
		 speaker_it != end; )
	{
		LLUUID speaker_id = speaker_it->first;
		LLSpeaker* speakerp = speaker_it->second;

		speaker_map_t::iterator  cur_speaker_it = speaker_it++;

		if (voice_channel_active &&
			voice_client->getVoiceEnabled(speaker_id))
		{
			speakerp->mSpeechVolume = voice_client->getCurrentPower(speaker_id);
			BOOL moderator_muted_voice;
			moderator_muted_voice = voice_client->getIsModeratorMuted(speaker_id);
			if (moderator_muted_voice != speakerp->mModeratorMutedVoice)
			{
				speakerp->mModeratorMutedVoice = moderator_muted_voice;
				speakerp->fireEvent(new LLSpeakerVoiceModerationEvent(speakerp));
			}

			if (voice_client->getOnMuteList(speaker_id) ||
				speakerp->mModeratorMutedVoice)
			{
				speakerp->mStatus = LLSpeaker::STATUS_MUTED;
			}
			else if (voice_client->getIsSpeaking(speaker_id))
			{
				// reset inactivity expiration
				if (speakerp->mStatus != LLSpeaker::STATUS_SPEAKING)
				{
					speakerp->mLastSpokeTime = mSpeechTimer.getElapsedTimeF32();
					speakerp->mHasSpoken = TRUE;
				}
				speakerp->mStatus = LLSpeaker::STATUS_SPEAKING;
				// interpolate between active color and full speaking color
				// based on power of speech output
				speakerp->mDotColor = speaking_color;
				if (speakerp->mSpeechVolume > LLVoiceClient::OVERDRIVEN_POWER_LEVEL)
				{
					speakerp->mDotColor = overdriven_color;
				}
			}
			else
			{
				speakerp->mSpeechVolume = 0.f;
				speakerp->mDotColor = ACTIVE_COLOR;

				if (speakerp->mHasSpoken)
				{
					// have spoken once, not currently speaking
					speakerp->mStatus = LLSpeaker::STATUS_HAS_SPOKEN;
				}
				else
				{
					// default state for being in voice channel
					speakerp->mStatus = LLSpeaker::STATUS_VOICE_ACTIVE;
				}
			}
		}
		// speaker no longer registered in voice channel, demote to text only
		else if (speakerp->mStatus != LLSpeaker::STATUS_NOT_IN_CHANNEL)
		{
			if (speakerp->mType == LLSpeaker::SPEAKER_EXTERNAL)
			{
				// external speakers should be timed out when they leave the
				// voice channel (since they only exist via SLVoice)
				speakerp->mStatus = LLSpeaker::STATUS_NOT_IN_CHANNEL;
			}
			else
			{
				speakerp->mStatus = LLSpeaker::STATUS_TEXT_ONLY;
				speakerp->mSpeechVolume = 0.f;
				speakerp->mDotColor = ACTIVE_COLOR;
			}
		}
	}

	if (resort_ok)  // only allow list changes when user is not interacting with it
	{
		// sort by status then time last spoken
		std::sort(mSpeakersSorted.begin(), mSpeakersSorted.end(),
				  LLSortRecentSpeakers());
	}

	// for recent speakers who are not currently speaking, show "recent" color
	// dot for most recent fading to "active" color

	S32 recent_speaker_count = 0;
	S32 sort_index = 0;
	speaker_list_t::iterator sorted_speaker_it;
	for (sorted_speaker_it = mSpeakersSorted.begin(); 
		 sorted_speaker_it != mSpeakersSorted.end(); )
	{
		LLPointer<LLSpeaker> speakerp = *sorted_speaker_it;

		// color code recent speakers who are not currently speaking
		if (speakerp->mStatus == LLSpeaker::STATUS_HAS_SPOKEN)
		{
			speakerp->mDotColor = lerp(speaking_color, ACTIVE_COLOR,
									   clamp_rescale((F32)recent_speaker_count,
													 -2.f, 3.f, 0.f, 1.f));
			recent_speaker_count++;
		}

		// stuff sort ordinal into speaker so the ui can sort by this value
		speakerp->mSortIndex = sort_index++;

		// remove speakers that have been gone too long
		if (speakerp->mStatus == LLSpeaker::STATUS_NOT_IN_CHANNEL &&
			speakerp->mActivityTimer.hasExpired())
		{
			fireEvent(new LLSpeakerListChangeEvent(this, speakerp->mID),
					  "remove");

			mSpeakers.erase(speakerp->mID);
			sorted_speaker_it = mSpeakersSorted.erase(sorted_speaker_it);
		}
		else
		{
			++sorted_speaker_it;
		}
	}
}

void LLSpeakerMgr::updateSpeakerList()
{
	// are we bound to the currently active voice channel?
	if ((mVoiceChannel && mVoiceChannel->isActive()) ||
		(!mVoiceChannel && LLVoiceClient::getInstance()->inProximalChannel()))
	{
		LLVoiceClient::participantMap* participants;
		participants = LLVoiceClient::getInstance()->getParticipantList();
		if (participants)
		{
			// add new participants to our list of known speakers
			for (LLVoiceClient::participantMap::iterator
					participant_it = participants->begin(),
					end = participants->end();
				 participant_it != end; ++participant_it)
			{
				LLVoiceClient::participantState* participantp = participant_it->second;
				setSpeaker(participantp->mAvatarID, participantp->mLegacyName,
						   LLSpeaker::STATUS_VOICE_ACTIVE,
						   (participantp->isAvatar() ? LLSpeaker::SPEAKER_AGENT
													 : LLSpeaker::SPEAKER_EXTERNAL));
			}
		}
	}
}

const LLPointer<LLSpeaker> LLSpeakerMgr::findSpeaker(const LLUUID& speaker_id)
{
	speaker_map_t::iterator found_it = mSpeakers.find(speaker_id);
	if (found_it == mSpeakers.end())
	{
		return NULL;
	}
	return found_it->second;
}

void LLSpeakerMgr::getSpeakerList(speaker_list_t* speaker_list,
								  BOOL include_text)
{
	speaker_list->clear();
	for (speaker_map_t::iterator speaker_it = mSpeakers.begin(),
								 end = mSpeakers.end();
		 speaker_it != end; ++speaker_it)
	{
		LLPointer<LLSpeaker> speakerp = speaker_it->second;
		// what about text only muted or inactive?
		if (include_text || speakerp->mStatus != LLSpeaker::STATUS_TEXT_ONLY)
		{
			speaker_list->push_back(speakerp);
		}
	}
}

const LLUUID LLSpeakerMgr::getSessionID() 
{ 
	return mVoiceChannel->getSessionID(); 
}

void LLSpeakerMgr::setSpeakerTyping(const LLUUID& speaker_id, BOOL typing)
{
	LLPointer<LLSpeaker> speakerp = findSpeaker(speaker_id);
	if (speakerp.notNull())
	{
		speakerp->mTyping = typing;
	}
}

// speaker has chatted via either text or voice
void LLSpeakerMgr::speakerChatted(const LLUUID& speaker_id)
{
	LLPointer<LLSpeaker> speakerp = findSpeaker(speaker_id);
	if (speakerp.notNull())
	{
		speakerp->mLastSpokeTime = mSpeechTimer.getElapsedTimeF32();
		speakerp->mHasSpoken = TRUE;
	}
}

BOOL LLSpeakerMgr::isVoiceActive()
{
	// mVoiceChannel = NULL means current voice channel, whatever it is
	return LLVoiceClient::voiceEnabled() && mVoiceChannel &&
		   mVoiceChannel->isActive();
}

//
// LLIMSpeakerMgr
//
LLIMSpeakerMgr::LLIMSpeakerMgr(LLVoiceChannel* channel) : LLSpeakerMgr(channel)
{
}

void LLIMSpeakerMgr::updateSpeakerList()
{
	// don't do normal updates which are pulled from voice channel
	// rely on user list reported by sim

	// We need to do this to allow PSTN callers into group chats to show in the list.
	LLSpeakerMgr::updateSpeakerList();

	return;
}

void LLIMSpeakerMgr::setSpeakers(const LLSD& speakers)
{
	if (!speakers.isMap()) return;

	if (speakers.has("agent_info") && speakers["agent_info"].isMap())
	{
		LLSD::map_const_iterator speaker_it;
		for (speaker_it = speakers["agent_info"].beginMap();
			 speaker_it != speakers["agent_info"].endMap(); ++speaker_it)
		{
			LLUUID agent_id(speaker_it->first);

			LLPointer<LLSpeaker> speakerp = setSpeaker(agent_id,
													   LLStringUtil::null,
													   LLSpeaker::STATUS_TEXT_ONLY);

			if (speaker_it->second.isMap())
			{
				speakerp->mIsModerator = speaker_it->second["is_moderator"];
				speakerp->mModeratorMutedText = speaker_it->second["mutes"]["text"];
			}
		}
	}
	else if (speakers.has("agents") && speakers["agents"].isArray())
	{
		// older, more decprecated way. Need here for using older version of
		// servers
		LLSD::array_const_iterator speaker_it;
		for (speaker_it = speakers["agents"].beginArray();
			speaker_it != speakers["agents"].endArray(); ++speaker_it)
		{
			const LLUUID agent_id = (*speaker_it).asUUID();

			LLPointer<LLSpeaker> speakerp = setSpeaker(agent_id,
													   LLStringUtil::null,
													   LLSpeaker::STATUS_TEXT_ONLY);
		}
	}
}

void LLIMSpeakerMgr::updateSpeakers(const LLSD& update)
{
	if (!update.isMap()) return;

	if (update.has("agent_updates") && update["agent_updates"].isMap())
	{
		LLSD::map_const_iterator update_it;
		for (update_it = update["agent_updates"].beginMap();
			 update_it != update["agent_updates"].endMap(); ++update_it)
		{
			LLUUID agent_id(update_it->first);
			LLPointer<LLSpeaker> speakerp = findSpeaker(agent_id);

			LLSD agent_data = update_it->second;

			if (agent_data.isMap() && agent_data.has("transition"))
			{
				if (speakerp.notNull() &&
					agent_data["transition"].asString() == "LEAVE")
				{
					speakerp->mStatus = LLSpeaker::STATUS_NOT_IN_CHANNEL;
					speakerp->mDotColor = INACTIVE_COLOR;
					speakerp->mActivityTimer.resetWithExpiry(SPEAKER_TIMEOUT);
				}
				else if (agent_data["transition"].asString() == "ENTER")
				{
					// add or update speaker
					speakerp = setSpeaker(agent_id);
				}
				else
				{
					llwarns << "bad membership list update "
							<< ll_print_sd(agent_data["transition"]) << llendl;
				}
			}

			if (speakerp.isNull()) continue;

			// should have a valid speaker from this point on
			if (agent_data.isMap() && agent_data.has("info"))
			{
				LLSD agent_info = agent_data["info"];

				if (agent_info.has("is_moderator"))
				{
					speakerp->mIsModerator = agent_info["is_moderator"];
				}

				if (agent_info.has("mutes"))
				{
					speakerp->mModeratorMutedText = agent_info["mutes"]["text"];
				}
			}
		}
	}
	else if (update.has("updates") && update["updates"].isMap())
	{
		LLSD::map_const_iterator update_it;
		for (update_it = update["updates"].beginMap();
			 update_it != update["updates"].endMap(); ++update_it)
		{
			LLUUID agent_id(update_it->first);
			LLPointer<LLSpeaker> speakerp = findSpeaker(agent_id);

			std::string agent_transition = update_it->second.asString();
			if (agent_transition == "LEAVE" && speakerp.notNull())
			{
				speakerp->mStatus = LLSpeaker::STATUS_NOT_IN_CHANNEL;
				speakerp->mDotColor = INACTIVE_COLOR;
				speakerp->mActivityTimer.resetWithExpiry(SPEAKER_TIMEOUT);
			}
			else if (agent_transition == "ENTER")
			{
				// add or update speaker
				speakerp = setSpeaker(agent_id);
			}
			else
			{
				llwarns << "bad membership list update "
						<< agent_transition << llendl;
			}
		}
	}
}

//
// LLActiveSpeakerMgr
//

LLActiveSpeakerMgr::LLActiveSpeakerMgr() : LLSpeakerMgr(NULL)
{
}

void LLActiveSpeakerMgr::updateSpeakerList()
{
	// point to whatever the current voice channel is
	mVoiceChannel = LLVoiceChannel::getCurrentVoiceChannel();

	// always populate from active voice channel
	if (LLVoiceChannel::getCurrentVoiceChannel() != mVoiceChannel)
	{
		fireEvent(new LLSpeakerListChangeEvent(this, LLUUID::null), "clear");
		mSpeakers.clear();
		mSpeakersSorted.clear();
		mVoiceChannel = LLVoiceChannel::getCurrentVoiceChannel();
	}
	LLSpeakerMgr::updateSpeakerList();

	// clean up text only speakers
	for (speaker_map_t::iterator speaker_it = mSpeakers.begin(),
								 end = mSpeakers.end();
		 speaker_it != end; ++speaker_it)
	{
		LLUUID speaker_id = speaker_it->first;
		LLSpeaker* speakerp = speaker_it->second;
		if (speakerp->mStatus == LLSpeaker::STATUS_TEXT_ONLY)
		{
			// automatically flag text only speakers for removal
			speakerp->mStatus = LLSpeaker::STATUS_NOT_IN_CHANNEL;
		}
	}
}

//
// LLLocalSpeakerMgr
//

LLLocalSpeakerMgr::LLLocalSpeakerMgr()
:	LLSpeakerMgr(LLVoiceChannelProximal::getInstance())
{
}

LLLocalSpeakerMgr::~LLLocalSpeakerMgr()
{
}

void LLLocalSpeakerMgr::updateSpeakerList()
{
	// pull speakers from voice channel
	LLSpeakerMgr::updateSpeakerList();

	if (gDisconnected)//the world is cleared.
	{
		return;
	}

	// pick up non-voice speakers in chat range
	std::vector<LLUUID> avatar_ids;
	std::vector<LLVector3d> positions;
	LLWorld::getInstance()->getAvatars(&avatar_ids, &positions,
									   gAgent.getPositionGlobal(),
									   CHAT_NORMAL_RADIUS);
	for (U32 i = 0, count = avatar_ids.size(); i < count; ++i)
	{
		setSpeaker(avatar_ids[i]);
	}

	// check if text only speakers have moved out of chat range
	for (speaker_map_t::iterator speaker_it = mSpeakers.begin(),
								 end = mSpeakers.end();
		 speaker_it != end; ++speaker_it)
	{
		LLUUID speaker_id = speaker_it->first;
		LLSpeaker* speakerp = speaker_it->second;
		if (speakerp->mStatus == LLSpeaker::STATUS_TEXT_ONLY)
		{
			LLVOAvatar* avatarp = gObjectList.findAvatar(speaker_id);
			if (!avatarp ||
				dist_vec(avatarp->getPositionAgent(),
						 gAgent.getPositionAgent()) > CHAT_NORMAL_RADIUS)
			{
				speakerp->mStatus = LLSpeaker::STATUS_NOT_IN_CHANNEL;
				speakerp->mDotColor = INACTIVE_COLOR;
				speakerp->mActivityTimer.resetWithExpiry(SPEAKER_TIMEOUT);
			}
		}
	}
}
