/** 
 * @file llfloateravatarlist.cpp
 * @brief Avatar list/radar floater
 *
 * @author Dale Glass <dale@daleglass.net>, (C) 2007
 */

/**
 * Rewritten by jcool410
 * Removed usage of globals
 * Removed TrustNET
 * Added utilization of "minimap" data
 * Heavily modified by Henri Beauchamp (the laggy spying tool becomes a true,
 * low lag radar)
 */

#include "llviewerprecompiledheaders.h"

#include <time.h>
#include <string.h>
#include <map>

#include "llfloateravatarlist.h"

#include "llavatarconstants.h"
#include "llcachename.h"
#include "llfasttimer.h"
#include "llradiogroup.h"
#include "llregionflags.h"
#include "llscrolllistctrl.h"
#include "llsdutil.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llcallbacklist.h"
#include "llchat.h"
#include "llfloateravatarinfo.h"
#include "llfloaterchat.h"
#include "llfloatermute.h"
#include "llfloaterreporter.h"
#include "llimview.h"
#include "lltracker.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llweb.h"
#include "llworld.h"

/**
 * @brief How long to keep people who are gone in the list and in memory.
 */
const F32 DEAD_KEEP_TIME = 10.0f;

extern U32 gFrameCount;
LLFrameTimer LLFloaterAvatarList::sUpdateTimer;

typedef enum e_radar_alert_type
{
	ALERT_TYPE_SIM = 0,
	ALERT_TYPE_DRAW = 1,
	ALERT_TYPE_SHOUTRANGE = 2,
	ALERT_TYPE_CHATRANGE = 3
} ERadarAlertType;

void announce(std::string msg)
{
	static LLCachedControl<S32> radar_chat_keys_channel(gSavedSettings,
														"RadarChatKeysChannel");
	//llinfos << "Radar broadcasting key: " << key.asString()
	// << " - on channel " << gSavedSettings.getS32("RadarChatKeysChannel")
	// << llendl;
	gMessageSystem->newMessage("ScriptDialogReply");
	gMessageSystem->nextBlock("AgentData");
	gMessageSystem->addUUID("AgentID", gAgentID);
	gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
	gMessageSystem->nextBlock("Data");
	gMessageSystem->addUUID("ObjectID", gAgentID);
	gMessageSystem->addS32("ChatChannel", (S32)radar_chat_keys_channel);
	gMessageSystem->addS32("ButtonIndex", 1);
	gMessageSystem->addString("ButtonLabel", msg);
	gAgent.sendReliableMessage();
}

void chat_avatar_status(std::string name, LLUUID key, ERadarAlertType type, bool entering)
{
	static LLCachedControl<bool> radar_chat_alerts(gSavedSettings,
												   "RadarChatAlerts");
	static LLCachedControl<bool> radar_alert_sim(gSavedSettings,
												 "RadarAlertSim");
	static LLCachedControl<bool> radar_alert_draw(gSavedSettings,
												  "RadarAlertDraw");
	static LLCachedControl<bool> radar_alert_shout_range(gSavedSettings,
														 "RadarAlertShoutRange");
	static LLCachedControl<bool> radar_alert_chat_range(gSavedSettings,
														"RadarAlertChatRange");
	static LLCachedControl<bool> radar_chat_keys(gSavedSettings,
												 "RadarChatKeys");
	if (radar_chat_alerts)
	{
		LLChat chat;
		// *TODO: translate
		std::string message =  name + " has " + (entering ? "entered" : "left");
		switch(type)
		{
			case ALERT_TYPE_SIM:
				if (radar_alert_sim)
				{
					chat.mText = message + " the sim.";
				}
				break;

			case ALERT_TYPE_DRAW:
				if (radar_alert_draw)
				{
					chat.mText = message + " draw distance.";
				}
				break;

			case ALERT_TYPE_SHOUTRANGE:
				if (radar_alert_shout_range)
				{
					chat.mText = message + " shout range.";
				}
				break;

			case ALERT_TYPE_CHATRANGE:
				if (radar_alert_chat_range)
				{
					chat.mText = message + " chat range.";
				}
				break;
		}
		if (chat.mText != "")
		{
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			LLFloaterChat::addChat(chat);
		}
	}
	if (radar_chat_keys && entering && type == ALERT_TYPE_SIM)
	{
		announce(key.asString());
	}
}

LLAvatarListEntry::LLAvatarListEntry(const LLUUID& id,
									 const std::string &name,
									 const LLVector3d &position)
:	mID(id),
	mName(name),
	mDisplayName(name),
	mPosition(position),
	mDrawPosition(),
	mMarked(FALSE),
	mFocused(FALSE),
	mUpdateTimer(),
	mFrame(gFrameCount),
	mInSimFrame(U32_MAX),
	mInDrawFrame(U32_MAX),
	mInChatFrame(U32_MAX),
	mInShoutFrame(U32_MAX)
{
}

void LLAvatarListEntry::setPosition(LLVector3d position, bool this_sim,
									bool drawn, bool chatrange, bool shoutrange)
{
	if (drawn)
	{
		mDrawPosition = position;
	}
	else if (mInDrawFrame == U32_MAX)
	{
		mDrawPosition.setZero();
	}

	mPosition = position;

	mFrame = gFrameCount;
	if (this_sim)
	{
		if (mInSimFrame == U32_MAX)
		{
			chat_avatar_status(mName, mID, ALERT_TYPE_SIM, true);
		}
		mInSimFrame = mFrame;
	}
	if (drawn)
	{
		if (mInDrawFrame == U32_MAX)
		{
			chat_avatar_status(mName, mID, ALERT_TYPE_DRAW, true);
		}
		mInDrawFrame = mFrame;
	}
	if (shoutrange)
	{
		if (mInShoutFrame == U32_MAX)
		{
			chat_avatar_status(mName, mID, ALERT_TYPE_SHOUTRANGE, true);
		}
		mInShoutFrame = mFrame;
	}
	if (chatrange)
	{
		if (mInChatFrame == U32_MAX)
		{
			chat_avatar_status(mName, mID, ALERT_TYPE_CHATRANGE, true);
		}
		mInChatFrame = mFrame;
	}

	mUpdateTimer.start();
}

bool LLAvatarListEntry::getAlive()
{
	U32 current = gFrameCount;
	if (mInSimFrame != U32_MAX && current - mInSimFrame >= 2)
	{
		mInSimFrame = U32_MAX;
		chat_avatar_status(mName, mID, ALERT_TYPE_SIM, false);
	}
	if (mInDrawFrame != U32_MAX && current - mInDrawFrame >= 2)
	{
		mInDrawFrame = U32_MAX;
		chat_avatar_status(mName, mID, ALERT_TYPE_DRAW, false);
	}
	if (mInShoutFrame != U32_MAX && current - mInShoutFrame >= 2)
	{
		mInShoutFrame = U32_MAX;
		chat_avatar_status(mName, mID, ALERT_TYPE_SHOUTRANGE, false);
	}
	if (mInChatFrame != U32_MAX && current - mInChatFrame >= 2)
	{
		mInChatFrame = U32_MAX;
		chat_avatar_status(mName, mID, ALERT_TYPE_CHATRANGE, false);
	}
	return current - mFrame <= 2;
}

F32 LLAvatarListEntry::getEntryAgeSeconds()
{
	return mUpdateTimer.getElapsedTimeF32();
}

BOOL LLAvatarListEntry::isDead()
{
	return getEntryAgeSeconds() > DEAD_KEEP_TIME;
}

LLFloaterAvatarList* LLFloaterAvatarList::sInstance = NULL;

LLFloaterAvatarList::LLFloaterAvatarList()
:	LLFloater(std::string("radar")),
	mUpdatesPerSecond(LLCachedControl<U32>(gSavedSettings,
										   "RadarUpdatesPerSecond", 4))
{
	llassert_always(sInstance == NULL);
	sInstance = this;
}

LLFloaterAvatarList::~LLFloaterAvatarList()
{
	gIdleCallbacks.deleteFunction(LLFloaterAvatarList::callbackIdle);
	sInstance = NULL;
}

//static
void LLFloaterAvatarList::toggle(void*)
{
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
	{
		if (sInstance && sInstance->getVisible())
		{	
			sInstance->close(false);
		}
	}
//mk
	if (sInstance)
	{
		if (sInstance->getVisible())
		{
			sInstance->close(false);
		}
		else
		{
			sInstance->open();
		}
	}
	else
	{
		showInstance();
	}
}

//static
void LLFloaterAvatarList::showInstance()
{
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
	{
		return;
	}
//mk
	if (sInstance)
	{
		if (!sInstance->getVisible())
		{
			sInstance->open();
		}
	}
	else
	{
		sInstance = new LLFloaterAvatarList();
		LLUICtrlFactory::getInstance()->buildFloater(sInstance,
													 "floater_radar.xml");
	}
}

void LLFloaterAvatarList::onOpen()
{
	gSavedSettings.setBOOL("ShowRadar", TRUE);
	sInstance->setVisible(TRUE);
}

void LLFloaterAvatarList::onClose(bool app_quitting)
{
	sInstance->setVisible(FALSE);
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowRadar", FALSE);
	}
	if (!gSavedSettings.getBOOL("RadarKeepOpen") || app_quitting)
	{
		destroy();
	}
}

BOOL LLFloaterAvatarList::postBuild()
{
	// Default values
	mTracking = FALSE;

	// Set callbacks
	childSetAction("profile_btn", onClickProfile, this);
	childSetAction("im_btn", onClickIM, this);
	childSetAction("offer_btn", onClickTeleportOffer, this);
	childSetAction("track_btn", onClickTrack, this);
	childSetAction("mark_btn", onClickMark, this);
	childSetAction("focus_btn", onClickFocus, this);
	childSetAction("prev_in_list_btn", onClickPrevInList, this);
	childSetAction("next_in_list_btn", onClickNextInList, this);
	childSetAction("prev_marked_btn", onClickPrevMarked, this);
	childSetAction("next_marked_btn", onClickNextMarked, this);
	
	childSetAction("get_key_btn", onClickGetKey, this);

	childSetAction("freeze_btn", onClickFreeze, this);
	childSetAction("eject_btn", onClickEject, this);
	childSetAction("mute_btn", onClickMute, this);
	childSetAction("ar_btn", onClickAR, this);
	childSetAction("teleport_btn", onClickTeleport, this);
	childSetAction("estate_eject_btn", onClickEjectFromEstate, this);

	childSetAction("send_keys_btn", onClickSendKeys, this);

	// Get a pointer to the scroll list from the interface
	mAvatarList = getChild<LLScrollListCtrl>("avatar_list");
	mAvatarList->sortByColumn("distance", TRUE);
	mAvatarList->setCommitOnSelectionChange(TRUE);
	childSetCommitCallback("avatar_list", onSelectName, this);
	refreshAvatarList();

	gIdleCallbacks.addFunction(LLFloaterAvatarList::callbackIdle);

	return TRUE;
}

void LLFloaterAvatarList::updateAvatarList()
{
	if (sInstance != this) return;

//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
	{
		close();
	}
//mk

	//llinfos << "radar refresh: updating map" << llendl;

	LLVector3d mypos = gAgent.getPositionGlobal();

	{
		std::vector<LLUUID> avatar_ids;
		std::vector<LLVector3d> positions;
		LLWorld::instance().getAvatars(&avatar_ids, &positions, mypos,
									   65536.0f);

		size_t i;
		size_t count = avatar_ids.size();
		for (i = 0; i < count; ++i)
		{
			std::string name;
			const LLUUID &avid = avatar_ids[i];
			if (avid.isNull())
			{
				continue;
			}

			LLVector3d position;
			LLVOAvatar* avatarp = gObjectList.findAvatar(avid);

			if (avatarp)
			{
				// Get avatar data
				position = gAgent.getPosGlobalFromAgent(avatarp->getCharacterPosition());
				name = avatarp->getFullname();

				if (name.empty() && !gCacheName->getFullName(avid, name))
				{
					continue; //prevent (Loading...)
				}

				std::string display_name = name;
				if (LLAvatarNameCache::useDisplayNames())
				{
					LLAvatarName avatar_name;
					if (LLAvatarNameCache::get(avid, &avatar_name))
					{
						if (LLAvatarNameCache::useDisplayNames() == 2)
						{
							display_name = avatar_name.mDisplayName;
						}
						else
						{
							display_name = avatar_name.getNames();
						}
					}
				}

				if (mAvatars.count(avid) > 0)
				{
					// Avatar already in list, update position
					F32 dist = (F32)(position - mypos).magVec();
					mAvatars[avid].setPosition(position,
											   (avatarp->getRegion() == gAgent.getRegion()),
											   true, dist < 20.0, dist < 100.0);
				}
				else
				{
					// Avatar not there yet, add it
					LLAvatarListEntry entry(avid, name, position);
					mAvatars[avid] = entry;
				}

				// update avatar display name.
				mAvatars[avid].setDisplayName(display_name);
			}
			else
			{
				if (i < positions.size())
				{
					position = positions[i];
				}
				else
				{
					continue;
				}

				if (!gCacheName->getFullName(avid, name))
				{
					continue; // prevent (Loading...)
				}

				std::string display_name = name;
				if (LLAvatarNameCache::useDisplayNames())
				{
					LLAvatarName avatar_name;
					if (LLAvatarNameCache::get(avid, &avatar_name))
					{
						if (LLAvatarNameCache::useDisplayNames() == 2)
						{
							display_name = avatar_name.mDisplayName;
						}
						else
						{
							display_name = avatar_name.getNames();
						}
					}
				}

				if (mAvatars.count(avid) > 0)
				{
					// Avatar already in list, update position
					F32 dist = (F32)(position - mypos).magVec();
					mAvatars[avid].setPosition(position,
											   gAgent.getRegion()->pointInRegionGlobal(position),
											   false, dist < 20.0,
											   dist < 100.0);
				}
				else
				{
					// Avatar not there yet, add it
					LLAvatarListEntry entry(avid, name, position);
					mAvatars[avid] = entry;
				}

				// update avatar display name.
				mAvatars[avid].setDisplayName(display_name);
			}
		}
	}

	expireAvatarList();
	refreshAvatarList();
	refreshTracker();
}

void LLFloaterAvatarList::expireAvatarList()
{
	for (avatar_list_iterator_t iter = mAvatars.begin(), end = mAvatars.end();
		 iter != end; )
	{
		avatar_list_iterator_t curiter = iter++;
		LLAvatarListEntry* entry = &curiter->second;
		if (!entry->getAlive() && entry->isDead())
		{
			LL_DEBUGS("Radar") << "Radar: expiring avatar "
							   << entry->getDisplayName() << LL_ENDL;
			LLUUID av_id = entry->getID();
			if (av_id == mTrackedAvatar)
			{
				stopTracker();
			}
			mAvatars.erase(curiter);
		}
	}
}

/**
 * Redraws the avatar list
 * Only does anything if the avatar list is visible.
 */
void LLFloaterAvatarList::refreshAvatarList() 
{
	// Don't update list when interface is hidden
	if (!sInstance->getVisible()) return;

	// We rebuild the list fully each time it's refreshed
	// The assumption is that it's faster to refill it and sort than
	// to rebuild the whole list.
	LLDynamicArray<LLUUID> selected = mAvatarList->getSelectedIDs();
	S32 scrollpos = mAvatarList->getScrollPos();

	mAvatarList->deleteAllItems();

	LLVector3d mypos = gAgent.getPositionGlobal();
	LLVector3d posagent;
	posagent.setVec(gAgent.getPositionAgent());
	LLVector3d simpos = mypos - posagent;
	char temp[32];

	for (avatar_list_iterator_t iter = mAvatars.begin(), end = mAvatars.end();
		 iter != end; ++iter)
	{
		LLAvatarListEntry* entry = &iter->second;

		// Skip if avatar hasn't been around
		if (entry->isDead())
		{
			continue;
		}

		LLVector3d position = entry->getPosition();
		BOOL UnknownAltitude = false;

		LLVector3d delta = position - mypos;
		F32 distance = (F32)delta.magVec();
		static LLCachedControl<U32> unknwown_avatar_altitude(gSavedSettings,
															 "UnknownAvatarAltitude");
		if (position.mdV[VZ] == (F32)unknwown_avatar_altitude)
		{
			UnknownAltitude = true;
			distance = 9000.0;
		}
		delta.mdV[2] = 0.0f;
		F32 side_distance = (F32)delta.magVec();

		// HACK: Workaround for an apparent bug:
		// sometimes avatar entries get stuck, and are registered by the client
		// as perpetually moving in the same direction. This makes sure they
		// get removed from the visible list eventually
		if (side_distance > 2048.0f)
		{
			continue;
		}

		LLUUID av_id = entry->getID();

		LLSD element;
		element["id"] = av_id;

		LLSD& mark_column = element["columns"][LIST_MARK];
		mark_column["column"] = "marked";
		mark_column["type"] = "text";
		if (entry->isMarked())
		{
			mark_column["value"] = "X";
			mark_column["color"] = LLColor4::blue.getValue();
			mark_column["font-style"] = "BOLD";
		}
		else
		{
			mark_column["value"] = "";
		}

		LLSD& name_column = element["columns"][LIST_AVATAR_NAME];
		name_column["column"] = "avatar_name";
		name_column["type"] = "text";
		name_column["value"] = entry->getDisplayName().c_str();
		if (entry->getEntryAgeSeconds() > 1.0f)
		{
			name_column["font-style"] = "ITALIC";
		}
		else if (entry->isFocused())
		{
			name_column["font-style"] = "BOLD";
		}
		LLMuteList* ml = LLMuteList::getInstance();
		if (ml && ml->isMuted(av_id))
		{
			name_column["color"] = LLColor4::red2.getValue();
		}
		else if (is_agent_friend(av_id))
		{
			name_column["color"] = LLColor4::green3.getValue();
		}

		LLColor4 color = LLColor4::black;
		LLSD& dist_column = element["columns"][LIST_DISTANCE];
		dist_column["column"] = "distance";
		dist_column["type"] = "text";
		if (UnknownAltitude)
		{
			strcpy(temp, "?");
			if (entry->isDrawn())
			{
				color = LLColor4::green2;
			}
		}
		else
		{
			if (distance < 100.0)
			{
				snprintf(temp, sizeof(temp), "%.1f", distance);
				if (distance > 20.0f)
				{
					color = LLColor4::yellow1;
				}
				else
				{
					color = LLColor4::red;
				}
			}
			else
			{
				if (entry->isDrawn())
				{
					color = LLColor4::green2;
				}
				snprintf(temp, sizeof(temp), "%d", (S32)distance);
			}
		}
		dist_column["value"] = temp;
		dist_column["color"] = color.getValue();

		position = position - simpos;

		S32 x = (S32)position.mdV[VX];
		S32 y = (S32)position.mdV[VY];
		if (x >= 0 && x <= 256 && y >= 0 && y <= 256)
		{
			snprintf(temp, sizeof(temp), "%d, %d", x, y);
		}
		else
		{
			temp[0] = '\0';
			if (y < 0)
			{
				strcat(temp, "S");
			}
			else if (y > 256)
			{
				strcat(temp, "N");
			}
			if (x < 0)
			{
				strcat(temp, "W");
			}
			else if (x > 256)
			{
				strcat(temp, "E");
			}
		}
		LLSD& pos_column = element["columns"][LIST_POSITION];
		pos_column["column"] = "position";
		pos_column["type"] = "text";
		pos_column["value"] = temp;

		LLSD& alt_column = element["columns"][LIST_ALTITUDE];
		alt_column["column"] = "altitude";
		alt_column["type"] = "text";
		if (UnknownAltitude)
		{
			strcpy(temp, "?");
		}
		else
		{
			snprintf(temp, sizeof(temp), "%d", (S32)position.mdV[VZ]);
		}
		alt_column["value"] = temp;

		// Add to list
		mAvatarList->addElement(element, ADD_BOTTOM);
	}

	// finish
	mAvatarList->sortItems();
	mAvatarList->selectMultiple(selected);
	mAvatarList->setScrollPos(scrollpos);
}

// static
void LLFloaterAvatarList::onClickIM(void* userdata)
{
	//llinfos << "LLFloaterFriends::onClickIM()" << llendl;
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;

	LLDynamicArray<LLUUID> ids = self->mAvatarList->getSelectedIDs();
	if (gIMMgr && ids.size() > 0)
	{
		if (ids.size() == 1)
		{
			// Single avatar
			LLUUID agent_id = ids[0];
			std::string fullname = self->mAvatars[agent_id].getName();
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession(fullname, IM_NOTHING_SPECIAL, agent_id);
		}
		else
		{
			// Group IM
			LLUUID session_id;
			session_id.generate();
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession("Avatars Conference",
							   IM_SESSION_CONFERENCE_START, ids[0], ids);
		}
	}
}

// static
void LLFloaterAvatarList::onClickTeleportOffer(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;

	LLDynamicArray<LLUUID> ids = self->mAvatarList->getSelectedIDs();
	if (ids.size() > 0)
	{
		handle_lure(ids);
	}
}

// static
void LLFloaterAvatarList::onClickTrack(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	
 	LLScrollListItem* item = self->mAvatarList->getFirstSelected();
	if (!item) return;

	LLUUID agent_id = item->getUUID();

	if (self->mTracking && self->mTrackedAvatar == agent_id)
	{
		self->stopTracker();
	}
	else
	{
		self->mTracking = TRUE;
		self->mTrackedAvatar = agent_id;
//		trackAvatar only works for friends allowing you to see them on map...
//		LLTracker::trackAvatar(agent_id,
//							   self->mAvatars[agent_id].getDisplayName());
		std::string name = self->mAvatars[agent_id].getDisplayName();
		if (self->mUpdatesPerSecond == 0)
		{
			name += "\n(last known position)";
		}
		LLTracker::trackLocation(self->mAvatars[agent_id].getPosition(), name,
								 "");
	}
}

void LLFloaterAvatarList::stopTracker()
{
	LLTracker::stopTracking(NULL);
	mTracking = FALSE;
}

void LLFloaterAvatarList::refreshTracker()
{
	if (!mTracking) return;

	if (LLTracker::isTracking(NULL))
	{
		LLVector3d pos;
		if (mUpdatesPerSecond > 0)
		{
			pos = mAvatars[mTrackedAvatar].getPosition();
		}
		else
		{
			LLVOAvatar* avatarp = gObjectList.findAvatar(mTrackedAvatar);
			if (!avatarp)
			{
				stopTracker();
				return;
			}
			pos = gAgent.getPosGlobalFromAgent(avatarp->getCharacterPosition());
		}

		F32 dist = (pos - LLTracker::getTrackedPositionGlobal()).magVec();
		if (dist > 1.0f)
		{
			std::string name = mAvatars[mTrackedAvatar].getDisplayName();
			LLTracker::trackLocation(pos, name, "");
		}
	}
	else
	{
		stopTracker();
	}
}

LLAvatarListEntry* LLFloaterAvatarList::getAvatarEntry(LLUUID avatar)
{
	if (avatar.isNull())
	{
		return NULL;
	}

	avatar_list_iterator_t iter = mAvatars.find(avatar);
	if (iter == mAvatars.end())
	{
		return NULL;
	}

	return &iter->second;	
}

//static
void LLFloaterAvatarList::onClickMark(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	LLDynamicArray<LLUUID> ids = self->mAvatarList->getSelectedIDs();

	for (LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end();
		 ++itr)
	{
		LLUUID avid = *itr;
		LLAvatarListEntry* entry = self->getAvatarEntry(avid);
		if (entry != NULL)
		{
			entry->toggleMark();
		}
	}
}

//static
void LLFloaterAvatarList::onClickFocus(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	
 	LLScrollListItem* item = self->mAvatarList->getFirstSelected();
	if (item)
	{
		self->mFocusedAvatar = item->getUUID();
		self->focusOnCurrent();
	}
}

void LLFloaterAvatarList::removeFocusFromAll()
{
	for (avatar_list_iterator_t iter = mAvatars.begin(), end = mAvatars.end();
		 iter != end; ++iter)
	{
		LLAvatarListEntry* entry = &iter->second;
		entry->setFocus(FALSE);
	}
}

void LLFloaterAvatarList::focusOnCurrent()
{
	LLAvatarListEntry* entry;

	if (mAvatars.size() == 0)
	{
		return;
	}

	for (avatar_list_iterator_t iter = mAvatars.begin(), end = mAvatars.end();
		 iter != end; iter++)
	{
		entry = &iter->second;

		if (entry->isDead())
		{
			continue;
		}

		if (entry->getID() == mFocusedAvatar)
		{
			removeFocusFromAll();
			entry->setFocus(TRUE);
			gAgent.lookAtObject(mFocusedAvatar, CAMERA_POSITION_OBJECT);
			return;
		}
	}
}

void LLFloaterAvatarList::focusOnPrev(BOOL marked_only)
{
	if (mAvatars.size() == 0)
	{
		return;
	}

	LLAvatarListEntry* prev = NULL;
	LLAvatarListEntry* entry;

	for (avatar_list_iterator_t iter = mAvatars.begin(), end = mAvatars.end();
		 iter != end; ++iter)
	{
		entry = &iter->second;

		if (entry->isDead())
			continue;

		if (prev != NULL && entry->getID() == mFocusedAvatar)
		{
			break;
		}

		if ((!marked_only && entry->isDrawn()) || entry->isMarked())
		{
			prev = entry;
		}
	}

	if (prev != NULL)
	{
		removeFocusFromAll();
		prev->setFocus(TRUE);
		mFocusedAvatar = prev->getID();
		gAgent.lookAtObject(mFocusedAvatar, CAMERA_POSITION_OBJECT);
	}
}

void LLFloaterAvatarList::focusOnNext(BOOL marked_only)
{
	BOOL found = FALSE;
	LLAvatarListEntry* next = NULL;
	LLAvatarListEntry* entry;

	if (mAvatars.size() == 0)
	{
		return;
	}

	for (avatar_list_iterator_t iter = mAvatars.begin(), end = mAvatars.end();
		 iter != end; ++iter)
	{
		entry = &iter->second;

		if (entry->isDead())
		{
			continue;
		}

		if (next == NULL &&
			((!marked_only && entry->isDrawn()) || entry->isMarked()))
		{
			next = entry;
		}

		if (found && ((!marked_only && entry->isDrawn()) || entry->isMarked()))
		{
			next = entry;
			break;
		}

		if (entry->getID() == mFocusedAvatar)
		{
			found = TRUE;
		} 
	}

	if (next != NULL)
	{
		removeFocusFromAll();
		next->setFocus(TRUE);
		mFocusedAvatar = next->getID();
		gAgent.lookAtObject(mFocusedAvatar, CAMERA_POSITION_OBJECT);
	}
}

//static
void LLFloaterAvatarList::onClickPrevInList(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	self->focusOnPrev(FALSE);
}

//static
void LLFloaterAvatarList::onClickNextInList(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	self->focusOnNext(FALSE);
}

//static
void LLFloaterAvatarList::onClickPrevMarked(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	self->focusOnPrev(TRUE);
}

//static
void LLFloaterAvatarList::onClickNextMarked(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	self->focusOnNext(TRUE);
}

//static
void LLFloaterAvatarList::onClickGetKey(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
 	LLScrollListItem* item = self->mAvatarList->getFirstSelected();

	if (NULL == item) return;

	LLUUID agent_id = item->getUUID();

	char buffer[UUID_STR_LENGTH];		/*Flawfinder: ignore*/
	agent_id.toString(buffer);

	gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(buffer));
}

//static
void LLFloaterAvatarList::onClickSendKeys(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	LLAvatarListEntry* entry;

	if (self->mAvatars.size() == 0)
		return;

	for (avatar_list_iterator_t iter = self->mAvatars.begin(),
								end = self->mAvatars.end();
		 iter != end; ++iter)
	{
		entry = &iter->second;

		if (!entry->isDead() && entry->isInSim())
		{
			announce(entry->getID().asString());
		}
	}
}

static void send_freeze(const LLUUID& avatar_id, bool freeze)
{
	U32 flags = 0x0;
	if (!freeze)
	{
		// unfreeze
		flags |= 0x1;
	}

	LLMessageSystem* msg = gMessageSystem;
	LLVOAvatar* avatarp = gObjectList.findAvatar(avatar_id);

	if (avatarp && avatarp->getRegion())
	{
		msg->newMessage("FreezeUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgentID);
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id);
		msg->addU32("Flags", flags);
		msg->sendReliable(avatarp->getRegion()->getHost());
	}
}

static void send_eject(const LLUUID& avatar_id, bool ban)
{	
	LLMessageSystem* msg = gMessageSystem;
	LLVOAvatar* avatarp = gObjectList.findAvatar(avatar_id);

	if (avatarp && avatarp->getRegion())
	{
		U32 flags = 0x0;
		if (ban)
		{
			// eject and add to ban list
			flags |= 0x1;
		}

		msg->newMessage("EjectUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgentID);
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id);
		msg->addU32("Flags", flags);
		msg->sendReliable(avatarp->getRegion()->getHost());
	}
}

static void send_estate_message(const char* request, const LLUUID& target)
{

	LLMessageSystem* msg = gMessageSystem;
	LLUUID invoice;

	// This seems to provide an ID so that the sim can say which request it's
	// replying to. I think this can be ignored for now.
	invoice.generate();

	llinfos << "Sending estate request '" << request << "'" << llendl;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", request);
	msg->addUUID("Invoice", invoice);

	// Agent id
	msg->nextBlock("ParamList");
	msg->addString("Parameter", gAgentID.asString().c_str());

	// Target
	msg->nextBlock("ParamList");
	msg->addString("Parameter", target.asString().c_str());

	msg->sendReliable(gAgent.getRegion()->getHost());
}

static void cmd_freeze(const LLUUID& avatar, const std::string &name)
{
	send_freeze(avatar, true);
}

static void cmd_unfreeze(const LLUUID& avatar, const std::string &name)
{
	send_freeze(avatar, false);
}

static void cmd_eject(const LLUUID& avatar, const std::string &name)
{
	send_eject(avatar, false);
}

static void cmd_ban(const LLUUID& avatar, const std::string &name)
{
	send_eject(avatar, true);
}

static void cmd_profile(const LLUUID& avatar, const std::string &name)
{
	LLFloaterAvatarInfo::showFromDirectory(avatar);
}

static void cmd_estate_eject(const LLUUID &avatar, const std::string &name)
{
	send_estate_message("teleporthomeuser", avatar);
}

void LLFloaterAvatarList::doCommand(void (*func)(const LLUUID &avatar,
												 const std::string &name))
{
	LLDynamicArray<LLUUID> ids = mAvatarList->getSelectedIDs();

	for (LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end();
		 ++itr)
	{
		LLUUID avid = *itr;
		LLAvatarListEntry* entry = getAvatarEntry(avid);
		if (entry != NULL)
		{
			llinfos << "Executing command on " << entry->getDisplayName()
					<< llendl;
			func(avid, entry->getName());
		}
	}
}

std::string LLFloaterAvatarList::getSelectedNames(const std::string& separator)
{
	std::string ret;

	LLDynamicArray<LLUUID> ids = mAvatarList->getSelectedIDs();
	for (LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end();
		 ++itr)
	{
		LLUUID avid = *itr;
		LLAvatarListEntry* entry = getAvatarEntry(avid);
		if (entry != NULL)
		{
			if (!ret.empty()) ret += separator;
			ret += entry->getName();
		}
	}

	return ret;
}

std::string LLFloaterAvatarList::getSelectedName()
{
	LLUUID id = getSelectedID();
	LLAvatarListEntry* entry = getAvatarEntry(id);
	if (entry)
	{
		return entry->getName();
	}
	return "";
}

LLUUID LLFloaterAvatarList::getSelectedID()
{
	LLScrollListItem* item = mAvatarList->getFirstSelected();
	if (item) return item->getUUID();
	return LLUUID::null;
}

//static 
void LLFloaterAvatarList::callbackFreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	LLFloaterAvatarList* self = sInstance;

	if (option == 0)
	{
		self->doCommand(cmd_freeze);
	}
	else if (option == 1)
	{
		self->doCommand(cmd_unfreeze);
	}
}

//static 
void LLFloaterAvatarList::callbackEject(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	LLFloaterAvatarList* self = sInstance;
 
	if (option == 0)
	{
		self->doCommand(cmd_eject);
	}
	else if (option == 1)
	{
		self->doCommand(cmd_ban);
	}
}

//static 
void LLFloaterAvatarList::callbackEjectFromEstate(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	LLFloaterAvatarList* self = sInstance;

	if (option == 0)
	{
		self->doCommand(cmd_estate_eject);
	}
}

//static
void LLFloaterAvatarList::callbackIdle(void* userdata)
{
	static U32 last_update_frame = 0;

	LLFastTimer t(LLFastTimer::FTM_IDLE_CB_RADAR);

	if (!sInstance) return;

	if (gFrameCount - last_update_frame > 4)	// In case of slow rendering do not cause more lag...
	{
		if (sInstance->mUpdatesPerSecond > 0)
		{
			if (sUpdateTimer.getElapsedTimeF32() >= 1.f / (F32)sInstance->mUpdatesPerSecond)
			{
				sInstance->updateAvatarList();
				sUpdateTimer.reset();
				last_update_frame = gFrameCount;
			}
		}
		else
		{
			sInstance->refreshTracker();
		}
	}
}

void LLFloaterAvatarList::onClickFreeze(void* userdata)
{
	LLSD args;
	LLSD payload;
	args["AVATAR_NAME"] = ((LLFloaterAvatarList*)userdata)->getSelectedNames();
	LLNotifications::instance().add("FreezeAvatarFullname", args, payload,
									callbackFreeze);
}

//static
void LLFloaterAvatarList::onClickEject(void* userdata)
{
	LLSD args;
	LLSD payload;
	args["AVATAR_NAME"] = ((LLFloaterAvatarList*)userdata)->getSelectedNames();
	LLNotifications::instance().add("EjectAvatarFullname", args, payload,
									callbackEject);
}

//static
void LLFloaterAvatarList::onClickMute(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;

	LLMuteList* ml = LLMuteList::getInstance();
	if (!ml) return;

	LLDynamicArray<LLUUID> ids = self->mAvatarList->getSelectedIDs();
	if (ids.size() > 0)
	{
		for (LLDynamicArray<LLUUID>::iterator itr = ids.begin();
			 itr != ids.end(); ++itr)
		{
			LLUUID agent_id = *itr;
		
			std::string name = self->mAvatars[agent_id].getName();
			if (ml->isMuted(agent_id))
			{
				LLMute mute(agent_id, name, LLMute::AGENT);
				ml->remove(mute);	
			}
			else
			{
				LLMute mute(agent_id, name, LLMute::AGENT);
				if (ml->add(mute))
				{
					LLFloaterMute::showInstance();
					LLFloaterMute::getInstance()->selectMute(mute.mID);
				}
			}
		}
	}
}

//static
void LLFloaterAvatarList::onClickEjectFromEstate(void* userdata)
{
	LLSD args;
	LLSD payload;
	args["EVIL_USER"] = ((LLFloaterAvatarList*)userdata)->getSelectedNames();
	LLNotifications::instance().add("EstateKickUser", args, payload,
									callbackEjectFromEstate);
}

//static
void LLFloaterAvatarList::onClickAR(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
 	LLScrollListItem* item = self->mAvatarList->getFirstSelected();
	if (item)
	{
		LLUUID agent_id = item->getUUID();
		LLAvatarListEntry* entry = self->getAvatarEntry(agent_id);
		if (entry)
		{
			LLFloaterReporter::showFromObject(entry->getID());
		}
	}
}

// static
void LLFloaterAvatarList::onClickProfile(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
	self->doCommand(cmd_profile);
}

//static
void LLFloaterAvatarList::onClickTeleport(void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;
 	LLScrollListItem* item = self->mAvatarList->getFirstSelected();
	if (item)
	{
		LLUUID agent_id = item->getUUID();
		LLAvatarListEntry* entry = self->getAvatarEntry(agent_id);
		if (entry)
		{
//			llinfos << "Trying to teleport to " << entry->getDisplayName()
//					<< " at " << entry->getPosition() << llendl;
			gAgent.teleportViaLocation(entry->getPosition());
		}
	}
}

//static
void LLFloaterAvatarList::onSelectName(LLUICtrl*, void* userdata)
{
	LLFloaterAvatarList* self = (LLFloaterAvatarList*)userdata;

 	LLScrollListItem* item = self->mAvatarList->getFirstSelected();
	if (item)
	{
		LLUUID agent_id = item->getUUID();
		LLAvatarListEntry* entry = self->getAvatarEntry(agent_id);
		if (entry)
		{
			BOOL enabled = entry->isDrawn();
			self->childSetEnabled("focus_btn", enabled);
			self->childSetEnabled("prev_in_list_btn", enabled);
			self->childSetEnabled("next_in_list_btn", enabled);
		}
	}
}
