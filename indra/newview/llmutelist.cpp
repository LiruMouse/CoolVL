/** 
 * @file llmutelist.cpp
 * @author Richard Nelson, James Cook
 * @brief Management of list of muted players
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

/*
 * How should muting work?
 * Mute an avatar
 * Mute a specific object (accidentally spamming)
 *
 * right-click avatar, mute
 * see list of recent chatters, mute
 * type a name to mute?
 *
 * show in list whether chatter is avatar or object
 *
 * need fast lookup by id
 * need lookup by name, doesn't have to be fast
 */

#include "llviewerprecompiledheaders.h"

#include "boost/tokenizer.hpp"

#include "llmutelist.h"

#include "llcachename.h"
#include "llcrc.h"
#include "lldir.h"
#include "lldispatcher.h"
#include "llnotifications.h"
#include "llsdserialize.h"
#include "llxfermanager.h"
#include "message.h"

#include "llagent.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "llimpanel.h"
#include "llimview.h"
#include "llviewergenericmessage.h"	// for gGenericDispatcher
#include "llviewernetwork.h"		// for gIsInSecondLife
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"				// for particle system banning

namespace 
{
	// This method is used to return an object to mute given an object id.
	// Its used by the LLMute constructor and LLMuteList::isMuted.
	LLViewerObject* get_object_to_mute_from_id(LLUUID object_id)
	{
		LLViewerObject *objectp = gObjectList.findObject(object_id);
		if (objectp && !objectp->isAvatar())
		{
			LLViewerObject *parentp = objectp->getRootEdit();
			if (parentp)
			{
				objectp = parentp;
			}
		}
		return objectp;
	}
}

// "emptymutelist"
class LLDispatchEmptyMuteList : public LLDispatchHandler
{
public:
	virtual bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& strings)
	{
		LLMuteList::getInstance()->setLoaded();
		return true;
	}
};

static LLDispatchEmptyMuteList sDispatchEmptyMuteList;

//-----------------------------------------------------------------------------
// LLMute()
//-----------------------------------------------------------------------------
char LLMute::CHAT_SUFFIX[] = " (chat)";
char LLMute::VOICE_SUFFIX[] = " (voice)";
char LLMute::PARTICLES_SUFFIX[] = " (particles)";
char LLMute::SOUNDS_SUFFIX[] = " (sounds)";

char LLMute::BY_NAME_SUFFIX[] = " (by name)";
char LLMute::AGENT_SUFFIX[] = " (resident)";
char LLMute::OBJECT_SUFFIX[] = " (object)";
char LLMute::GROUP_SUFFIX[] = " (group)";

LLMute::LLMute(const LLUUID& id, const std::string& name, EType type, U32 flags)
:	mID(id),
	mName(name),
	mType(type),
	mFlags(flags)
{
	// muting is done by root objects only - try to find this objects root
	LLViewerObject* mute_object = get_object_to_mute_from_id(id);
	if (mute_object && mute_object->getID() != id)
	{
		mID = mute_object->getID();
		LLNameValue* firstname = mute_object->getNVPair("FirstName");
		LLNameValue* lastname = mute_object->getNVPair("LastName");
		if (firstname && lastname)
		{
			mName.assign(firstname->getString());
			mName.append(" ");
			mName.append(lastname->getString());
		}
		mType = mute_object->isAvatar() ? AGENT : OBJECT;
	}
	if (mType == AGENT && mName.find(" ") == std::string::npos)
	{
		// Residents must always appear with their legacy name in the mute list
		mName += " Resident";
	}
}

std::string LLMute::getNameAndType() const
{
	std::string name_with_suffix = mName;
	switch (mType)
	{
		case BY_NAME:
		default:
			name_with_suffix += BY_NAME_SUFFIX;
			break;
		case AGENT:
			name_with_suffix += AGENT_SUFFIX;
			break;
		case OBJECT:
			name_with_suffix += OBJECT_SUFFIX;
			break;
		case GROUP:
			name_with_suffix += GROUP_SUFFIX;
			break;
	}
	if (mFlags != 0)
	{
		if (~mFlags & flagTextChat)
		{
			name_with_suffix += CHAT_SUFFIX;
		}
		if (~mFlags & flagVoiceChat)
		{
			name_with_suffix += VOICE_SUFFIX;
		}
		if (~mFlags & flagObjectSounds)
		{
			name_with_suffix += SOUNDS_SUFFIX;
		}
		if (~mFlags & flagParticles)
		{
			name_with_suffix += PARTICLES_SUFFIX;
		}
	}
	return name_with_suffix;
}

void LLMute::setFromDisplayName(const std::string& display_name)
{
	size_t pos = 0;
	mName = display_name;

	pos = mName.rfind(GROUP_SUFFIX);
	if (pos != std::string::npos)
	{
		mName.erase(pos);
		mType = GROUP;
		return;
	}

	pos = mName.rfind(OBJECT_SUFFIX);
	if (pos != std::string::npos)
	{
		mName.erase(pos);
		mType = OBJECT;
		return;
	}

	pos = mName.rfind(AGENT_SUFFIX);
	if (pos != std::string::npos)
	{
		mName.erase(pos);
		mType = AGENT;
		return;
	}

	pos = mName.rfind(BY_NAME_SUFFIX);
	if (pos != std::string::npos)
	{
		mName.erase(pos);
		mType = BY_NAME;
		return;
	}

	llwarns << "Unable to set mute from display name " << display_name << llendl;
	return;
}

/* static */
LLMuteList* LLMuteList::getInstance()
{
	// Register callbacks at the first time that we find that the message system has been created.
	static BOOL registered = FALSE;
	if (!registered && gMessageSystem != NULL)
	{
		registered = TRUE;
		// Register our various callbacks
		gMessageSystem->setHandlerFuncFast(_PREHASH_MuteListUpdate, processMuteListUpdate);
		gMessageSystem->setHandlerFuncFast(_PREHASH_UseCachedMuteList, processUseCachedMuteList);
	}
	return LLSingleton<LLMuteList>::getInstance(); // Call the "base" implementation.
}

//-----------------------------------------------------------------------------
// LLMuteList()
//-----------------------------------------------------------------------------
LLMuteList::LLMuteList()
:	mIsLoaded(FALSE),
	mUserVolumesLoaded(FALSE)
{
	gGenericDispatcher.addHandler("emptymutelist", &sDispatchEmptyMuteList);
}

void LLMuteList::loadUserVolumes()
{
	// call once, after LLDir::setLindenUserDir() has been called
	if (mUserVolumesLoaded)
		return;
	mUserVolumesLoaded = TRUE;

	// load per-resident voice volume information
	// conceptually, this is part of the mute list information, although it is only stored locally
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "volume_settings.xml");

	LLSD settings_llsd;
	llifstream file;
	file.open(filename);
	if (file.is_open())
	{
		LLSDSerialize::fromXML(settings_llsd, file);
	}

	for (LLSD::map_const_iterator iter = settings_llsd.beginMap();
		 iter != settings_llsd.endMap(); ++iter)
	{
		mUserVolumeSettings.insert(std::make_pair(LLUUID(iter->first), (F32)iter->second.asReal()));
	}
}

//-----------------------------------------------------------------------------
// ~LLMuteList()
//-----------------------------------------------------------------------------
LLMuteList::~LLMuteList()
{
	// If we quit from the login screen we will not have an SL account
	// name.  Don't try to save, otherwise we'll dump a file in
	// C:\Program Files\SecondLife\  JC
	std::string user_dir = gDirUtilp->getLindenUserDir();
	if (!user_dir.empty())
	{
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "volume_settings.xml");
		LLSD settings_llsd;

		for (user_volume_map_t::iterator iter = mUserVolumeSettings.begin();
			 iter != mUserVolumeSettings.end(); ++iter)
		{
			settings_llsd[iter->first.asString()] = iter->second;
		}

		llofstream file;
		file.open(filename);
		LLSDSerialize::toPrettyXML(settings_llsd, file);
	}
}

BOOL LLMuteList::isLinden(const std::string& name) const
{
	// We don't know the last name for admins in OpenSim, so...
	if (!gIsInSecondLife) return FALSE;

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(name, sep);
	tokenizer::iterator token_iter = tokens.begin();

	if (token_iter == tokens.end()) return FALSE;
	token_iter++;
	if (token_iter == tokens.end()) return FALSE;

	std::string last_name = *token_iter;
	return last_name == "Linden";
}

BOOL LLMuteList::add(const LLMute& mute, U32 flags)
{
	// Can't mute text from Lindens
	if (mute.mType == LLMute::AGENT && isLinden(mute.mName) &&
		(flags == 0 || (flags & LLMute::flagTextChat)))
	{
		LLNotifications::instance().add("MuteLinden");
		return FALSE;
	}

	if (mute.mID.notNull())
	{
		LLViewerObject* vobj = gObjectList.findObject(mute.mID);
		if (mute.mID == gAgent.getID())
		{
			if (flags != LLMute::flagVoiceChat)
			{
				// Can't mute self.
				LLNotifications::instance().add("MuteSelf");
				return FALSE;
			}
		}
		else if (vobj && vobj->permYouOwner())
		{
			// Can't mute our own objects
			LLNotifications::instance().add("MuteOwnObject");
			return FALSE;
		}
	}

	if (mute.mType == LLMute::BY_NAME)
	{
		// Can't mute empty string by name
		if (mute.mName.empty()) 
		{
			llwarns << "Trying to mute empty string by-name" << llendl;
			return FALSE;
		}

		// Null mutes must have uuid null
		if (mute.mID.notNull())
		{
			llwarns << "Trying to add by-name mute with non-null id" << llendl;
			return FALSE;
		}

		if (!isAgentAvatarValid())
		{
			return FALSE;
		}

		std::string name;
		name.assign(gAgentAvatarp->getNVPair("FirstName")->getString());
		name.append(" ");
		name.append(gAgentAvatarp->getNVPair("LastName")->getString());
		if (mute.mName ==  name)
		{
			// Can't mute self.
			LLNotifications::instance().add("MuteSelf");
			return FALSE;
		}

		std::pair<string_set_t::iterator, bool> result = mLegacyMutes.insert(mute.mName);
		if (result.second)
		{
			llinfos << "Muting by name " << mute.mName << llendl;
			updateAdd(mute);
			notifyObservers();
			return TRUE;
		}
		else
		{
			// was duplicate
			LLNotifications::instance().add("MuteByNameFailed");
			return FALSE;
		}
	}
	else
	{
		// Need a local (non-const) copy to set up flags properly.
		LLMute localmute = mute;

		// If an entry for the same entity is already in the list, remove it, saving flags as necessary.
		mute_set_t::iterator it = mMutes.find(localmute);
		if (it != mMutes.end())
		{
			// This mute is already in the list.  Save the existing entry's flags if that's warranted.
			localmute.mFlags = it->mFlags;

			mMutes.erase(it);
			// Don't need to call notifyObservers() here, since it will happen after the entry has been re-added below.
		}
		else
		{
			// There was no entry in the list previously.  Fake things up by making it look like the previous entry had all properties unmuted.
			localmute.mFlags = LLMute::flagAll;
		}

		if (flags)
		{
			// The user passed some combination of flags.
			// Make sure those flag bits are turned off (i.e. those properties
			// will be muted) and that mFlags will not be 0 (0 = full mute,
			// including things not covered by flags such as script dialogs,
			// inventory offers, avatar rendering, etc...).
			localmute.mFlags = LLMute::flagPartialMute | (localmute.mFlags & ~flags);
		}
		else
		{
			// The user passed 0.  Make sure all flag bits are turned off (i.e. all properties will be muted).
			localmute.mFlags = 0;
		}

		// (re)add the mute entry.
		{
			std::pair<mute_set_t::iterator, bool> result = mMutes.insert(localmute);
			if (result.second)
			{
				llinfos << "Muting " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << llendl;
				updateAdd(localmute);
				notifyObservers();
				if (!(localmute.mFlags & LLMute::flagParticles))
				{
					//Kill all particle systems owned by muted task
					if (localmute.mType == LLMute::AGENT || localmute.mType == LLMute::OBJECT)
					{
						LLViewerPartSim::getInstance()->clearParticlesByOwnerID(localmute.mID);
					}
				}
				return TRUE;
			}
		}
	}

	// If we were going to return success, we'd have done it by now.
	return FALSE;
}

void LLMuteList::updateAdd(const LLMute& mute)
{
	// Update the database
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateMuteListEntry);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addUUIDFast(_PREHASH_MuteID, mute.mID);
	msg->addStringFast(_PREHASH_MuteName, mute.mName);
	msg->addS32("MuteType", mute.mType);
	msg->addU32("MuteFlags", mute.mFlags);
	gAgent.sendReliableMessage();

	mIsLoaded = TRUE; // why is this here? -MG
}

BOOL LLMuteList::remove(const LLMute& mute, U32 flags)
{
	BOOL found = FALSE;

	// First, remove from main list.
	mute_set_t::iterator it = mMutes.find(mute);
	if (it != mMutes.end())
	{
		LLMute localmute = *it;
		bool remove = true; // When the caller didn't pass any flag remove the entire entry.
		if (flags)
		{
			// If the user passed mute flags, we may only want to change some flags.
			localmute.mFlags |= flags | LLMute::flagPartialMute;
			if (localmute.mFlags != (LLMute::flagAll | LLMute::flagPartialMute))
			{
				// Only some of the properties are masked out. Update the entry.
				remove = false;
			}
			else
			{
				localmute.mFlags = 0;
			}
		}

		// Always remove the entry from the set -- it will be re-added with new flags if necessary.
		mMutes.erase(it);

		if (remove)
		{
			// The entry was actually removed.  Notify the server.
			updateRemove(localmute);
			llinfos << "Unmuting " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << llendl;
		}
		else
		{
			// Flags were updated, the mute entry needs to be retransmitted to the server and re-added to the list.
			mMutes.insert(localmute);
			updateAdd(localmute);
			llinfos << "Updating mute entry " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << llendl;
		}

		// Must be after erase.
		setLoaded();  // why is this here? -MG
	}

	// Clean up any legacy mutes
	string_set_t::iterator legacy_it = mLegacyMutes.find(mute.mName);
	if (legacy_it != mLegacyMutes.end())
	{
		// Database representation of legacy mute is UUID null.
		LLMute mute(LLUUID::null, *legacy_it, LLMute::BY_NAME);
		updateRemove(mute);
		mLegacyMutes.erase(legacy_it);
		// Must be after erase.
		setLoaded(); // why is this here? -MG
	}

	return found;
}

void LLMuteList::updateRemove(const LLMute& mute)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RemoveMuteListEntry);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addUUIDFast(_PREHASH_MuteID, mute.mID);
	msg->addString("MuteName", mute.mName);
	gAgent.sendReliableMessage();
}

void notify_automute_callback(const LLUUID& agent_id,
							  const std::string& full_name,
							  bool is_group, LLMuteList::EAutoReason reason)
{
	std::string notif_name;
	switch (reason)
	{
	default:
	case LLMuteList::AR_IM:
		notif_name = "AutoUnmuteByIM";
		break;
	case LLMuteList::AR_INVENTORY:
		notif_name = "AutoUnmuteByInventory";
		break;
	case LLMuteList::AR_MONEY:
		notif_name = "AutoUnmuteByMoney";
		break;
	}

	LLSD args;
	args["NAME"] = full_name;

	LLNotificationPtr notif_ptr = LLNotifications::instance().add(notif_name, args);
	if (notif_ptr)
	{
		std::string message = notif_ptr->getMessage();

		if (reason == LLMuteList::AR_IM && gIMMgr)
		{
			LLFloaterIMPanel* timp = gIMMgr->findFloaterBySession(agent_id);
			if (timp)
			{
				timp->addHistoryLine(message);
			}
		}

		LLChat auto_chat(message);
		LLFloaterChat::addChat(auto_chat, FALSE, FALSE);
	}
}

BOOL LLMuteList::autoRemove(const LLUUID& agent_id,
							const EAutoReason reason,
							const std::string& first_name,
							const std::string& last_name)
{
	BOOL removed = FALSE;

	if (isMuted(agent_id))
	{
		LLMute automute(agent_id, LLStringUtil::null, LLMute::AGENT);
		removed = TRUE;
		remove(automute);

		std::string full_name = LLCacheName::buildFullName(first_name, last_name);
		if (full_name.empty())
		{
			if (gCacheName->getFullName(agent_id, full_name))
			{
				// name in cache, call callback directly
				notify_automute_callback(agent_id, full_name, false, reason);
			}
			else
			{
				// not in cache, lookup name from cache
				gCacheName->get(agent_id, false,
								boost::bind(&notify_automute_callback,
											_1, _2, _3, reason));
			}
		}
		else
		{
			// call callback directly
			notify_automute_callback(agent_id, full_name, false, reason);
		}
	}

	return removed;
}

std::vector<LLMute> LLMuteList::getMutes() const
{
	std::vector<LLMute> mutes;

	for (mute_set_t::const_iterator it = mMutes.begin();
		 it != mMutes.end();
		 ++it)
	{
		mutes.push_back(*it);
	}

	for (string_set_t::const_iterator it = mLegacyMutes.begin();
		 it != mLegacyMutes.end();
		 ++it)
	{
		LLMute legacy(LLUUID::null, *it);
		mutes.push_back(legacy);
	}

	std::sort(mutes.begin(), mutes.end(), compare_by_name());
	return mutes;
}

//-----------------------------------------------------------------------------
// loadFromFile()
//-----------------------------------------------------------------------------
BOOL LLMuteList::loadFromFile(const std::string& filename)
{
	if (!filename.size())
	{
		llwarns << "Mute List Filename is Empty!" << llendl;
		return FALSE;
	}

	LLFILE* fp = LLFile::fopen(filename, "rb");		/*Flawfinder: ignore*/
	if (!fp)
	{
		llwarns << "Couldn't open mute list " << filename << llendl;
		return FALSE;
	}

	// *NOTE: Changing the size of these buffers will require changes
	// in the scanf below.
	char id_buffer[MAX_STRING];		/*Flawfinder: ignore*/
	char name_buffer[MAX_STRING];		/*Flawfinder: ignore*/
	char buffer[MAX_STRING];		/*Flawfinder: ignore*/
	while (!feof(fp) && fgets(buffer, MAX_STRING, fp))
	{
		id_buffer[0] = '\0';
		name_buffer[0] = '\0';
		S32 type = 0;
		U32 flags = 0;
		sscanf(buffer, " %d %254s %254[^|]| %u\n", &type, id_buffer,
			   name_buffer, &flags);	/* Flawfinder: ignore */
		LLUUID id = LLUUID(id_buffer);
		LLMute mute(id, std::string(name_buffer), (LLMute::EType)type, flags);
		if (mute.mID.isNull() || mute.mType == LLMute::BY_NAME)
		{
			mLegacyMutes.insert(mute.mName);
		}
		else
		{
			mMutes.insert(mute);
		}
	}
	fclose(fp);
	setLoaded();
	return TRUE;
}

//-----------------------------------------------------------------------------
// saveToFile()
//-----------------------------------------------------------------------------
BOOL LLMuteList::saveToFile(const std::string& filename)
{
	if (!filename.size())
	{
		llwarns << "Mute List Filename is Empty!" << llendl;
		return FALSE;
	}

	LLFILE* fp = LLFile::fopen(filename, "wb");		/*Flawfinder: ignore*/
	if (!fp)
	{
		llwarns << "Couldn't open mute list " << filename << llendl;
		return FALSE;
	}
	// legacy mutes have null uuid
	std::string id_string;
	LLUUID::null.toString(id_string);
	for (string_set_t::iterator it = mLegacyMutes.begin();
		 it != mLegacyMutes.end(); ++it)
	{
		fprintf(fp, "%d %s %s|\n", (S32)LLMute::BY_NAME, id_string.c_str(), it->c_str());
	}
	for (mute_set_t::iterator it = mMutes.begin();
		 it != mMutes.end(); ++it)
	{
		it->mID.toString(id_string);
		const std::string& name = it->mName;
		fprintf(fp, "%d %s %s|%u\n", (S32)it->mType, id_string.c_str(), name.c_str(), it->mFlags);
	}
	fclose(fp);
	return TRUE;
}

BOOL LLMuteList::isMuted(const LLUUID& id,
						 const std::string& name,
						 U32 flags,
						 LLMute::EType type) const
{
	if (id.notNull())
	{
		// for objects, check for muting on their parent prim
		LLViewerObject* mute_object = get_object_to_mute_from_id(id);
		LLUUID id_to_check  = (mute_object) ? mute_object->getID() : id;

		// don't need name or type for lookup
		LLMute mute(id_to_check);
		mute_set_t::const_iterator mute_it = mMutes.find(mute);
		if (mute_it != mMutes.end())
		{
			// If any of the flags the caller passed are set, this item isn't
			// considered muted for this caller.
			if (flags & mute_it->mFlags)
			{
				return FALSE;
			}
			// If the mute got flags and no flag was passed by the caller,
			// this item isn't considered muted for this caller (for example
			// if we muted for chat only, we don't want the avatar to be
			// considered muted for the rest)
			if (flags == 0 &&  mute_it->mFlags != 0)
			{
				return FALSE;
			}
			return TRUE;
		}
	}

	// if no name was provided, we can't proceed further
	if (name.empty()) return FALSE;

	// The following checks are useful when we want to check for mutes
	// on something for which we don't have an UUID for, but that was
	// previously muted by UUID and not by name (legacy mute). This can
	// be used in some callbacks of llviewermessage.cpp, since not all
	// callbacks provide both object and avatar UUIDs, for example...
	// Note that partial mutes (mutes containing flags, such as flagVoiceChat
	// for example) will not be taken into account (we want to check for
	// full mutes only, here).
	if (type != LLMute::COUNT)
	{
		std::string name_and_type = name;
		switch (type)
		{
			case LLMute::AGENT:
			{
				if (name_and_type.find(" ") == std::string::npos)
				{
					// Residents always appear with their legacy name in the
					// mute list
					name_and_type += " Resident";
				}
				name_and_type += LLMute::AGENT_SUFFIX;
				break;
			}
			case LLMute::OBJECT:
			{
				name_and_type += LLMute::OBJECT_SUFFIX;
				break;
			}
			case LLMute::GROUP:
			{
				name_and_type += LLMute::GROUP_SUFFIX;
				break;
			}
			case LLMute::BY_NAME:
			default:
			{
				name_and_type += LLMute::BY_NAME_SUFFIX;
			}
		}
		std::vector<LLMute> mutes = getMutes();
		std::vector<LLMute>::iterator it;
		for (it = mutes.begin(); it != mutes.end(); ++it)
		{
			if (name_and_type == it->getNameAndType())
			{
				// If any of the flags the caller passed are set, this item
				// isn't considered muted for this caller.
				if (flags & it->mFlags)
				{
					return FALSE;
				}
				return TRUE;
			}
		}
	}

	// Agents and groups are always muted by id and thus should never appear in
	// the legacy mutes (and we don't want to mute an agent or group whose name
	// would accidentally match an object muted by name...).
	if (type == LLMute::AGENT || type == LLMute::GROUP)
	{
		return FALSE;
	}
	else
	{
		// Look in legacy pile
		string_set_t::const_iterator legacy_it = mLegacyMutes.find(name);
		return legacy_it != mLegacyMutes.end() ?  TRUE : FALSE;
	}
}

S32 LLMuteList::getMuteFlags(const LLUUID& id, std::string& description) const
{
	S32 flags = -1;			// Defaults to no mute
	description.clear();	// Empty description for no mute.

	if (id.notNull())
	{
		// for objects, check for muting on their parent prim
		LLViewerObject* mute_object = get_object_to_mute_from_id(id);
		LLUUID id_to_check  = (mute_object) ? mute_object->getID() : id;

		LLMute mute(id_to_check);
		mute_set_t::const_iterator mute_it = mMutes.find(mute);
		if (mute_it != mMutes.end())
		{
			flags = (S32)mute_it->mFlags;
			if (flags == 0)
			{
				description = "Muted";
			}
			else
			{
				flags = ~flags & LLMute::flagAll;

				if (flags & LLMute::flagTextChat)
				{
					description = 'C';
				}
				if (flags & LLMute::flagVoiceChat)
				{
					if (!description.empty())
					{
						description += '/';
					}
					description += 'V';
				}
				if (flags & LLMute::flagObjectSounds)
				{
					if (!description.empty())
					{
						description += '/';
					}
					description += 'S';
				}
				if (flags & LLMute::flagParticles)
				{
					if (!description.empty())
					{
						description += '/';
					}
					description += 'P';
				}
				description = "Muted (" + description + ")";
			}
		}
	}

	return flags;
}

//-----------------------------------------------------------------------------
// requestFromServer()
//-----------------------------------------------------------------------------
void LLMuteList::requestFromServer(const LLUUID& agent_id)
{
	loadUserVolumes();

	std::string agent_id_string;
	std::string filename;
	agent_id.toString(agent_id_string);
	filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string) + ".cached_mute";
	LLCRC crc;
	crc.update(filename);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MuteListRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, agent_id);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addU32Fast(_PREHASH_MuteCRC, crc.getCRC());
	gAgent.sendReliableMessage();
}

//-----------------------------------------------------------------------------
// cache()
//-----------------------------------------------------------------------------

void LLMuteList::cache(const LLUUID& agent_id)
{
	// Write to disk even if empty.
	if (mIsLoaded)
	{
		std::string agent_id_string;
		std::string filename;
		agent_id.toString(agent_id_string);
		filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string) + ".cached_mute";
		saveToFile(filename);
	}
}

void LLMuteList::setSavedResidentVolume(const LLUUID& id, F32 volume)
{
	// store new value in volume settings file
	mUserVolumeSettings[id] = volume;
}

F32 LLMuteList::getSavedResidentVolume(const LLUUID& id)
{
	const F32 DEFAULT_VOLUME = 0.5f;

	user_volume_map_t::iterator found_it = mUserVolumeSettings.find(id);
	if (found_it != mUserVolumeSettings.end())
	{
		return found_it->second;
	}
	//FIXME: assumes default, should get this from somewhere
	return DEFAULT_VOLUME;
}

//-----------------------------------------------------------------------------
// Static message handlers
//-----------------------------------------------------------------------------

void LLMuteList::processMuteListUpdate(LLMessageSystem* msg, void**)
{
	llinfos << "LLMuteList::processMuteListUpdate()" << llendl;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_MuteData, _PREHASH_AgentID, agent_id);
	if (agent_id != gAgent.getID())
	{
		llwarns << "Got an mute list update for the wrong agent." << llendl;
		return;
	}
	std::string unclean_filename;
	msg->getStringFast(_PREHASH_MuteData, _PREHASH_Filename, unclean_filename);
	std::string filename = LLDir::getScrubbedFileName(unclean_filename);

	std::string *local_filename_and_path = new std::string(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, filename));
	gXferManager->requestFile(*local_filename_and_path,
							  filename,
							  LL_PATH_CACHE,
							  msg->getSender(),
							  TRUE, // make the remote file temporary.
							  onFileMuteList,
							  (void**)local_filename_and_path,
							  LLXferManager::HIGH_PRIORITY);
}

void LLMuteList::processUseCachedMuteList(LLMessageSystem* msg, void**)
{
	llinfos << "LLMuteList::processUseCachedMuteList()" << llendl;

	std::string agent_id_string;
	gAgent.getID().toString(agent_id_string);
	std::string filename;
	filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string) + ".cached_mute";
	LLMuteList::getInstance()->loadFromFile(filename);
}

void LLMuteList::onFileMuteList(void** user_data, S32 error_code, LLExtStat ext_status)
{
	llinfos << "LLMuteList::processMuteListFile()" << llendl;

	std::string* local_filename_and_path = (std::string*)user_data;
	if (local_filename_and_path && !local_filename_and_path->empty() && error_code == 0)
	{
		LLMuteList::getInstance()->loadFromFile(*local_filename_and_path);
		LLFile::remove(*local_filename_and_path);
	}
	delete local_filename_and_path;
}

void LLMuteList::addObserver(LLMuteListObserver* observer)
{
	mObservers.insert(observer);
}

void LLMuteList::removeObserver(LLMuteListObserver* observer)
{
	mObservers.erase(observer);
}

void LLMuteList::setLoaded()
{
	mIsLoaded = TRUE;
	notifyObservers();
}

void LLMuteList::notifyObservers()
{
	for (observer_set_t::iterator it = mObservers.begin();
		 it != mObservers.end(); )
	{
		LLMuteListObserver* observer = *it;
		observer->onChange();
		// In case onChange() deleted an entry.
		it = mObservers.upper_bound(observer);
	}
}
