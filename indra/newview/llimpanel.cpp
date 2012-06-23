/**
 * @file llimpanel.cpp
 * @brief LLIMPanel class definition
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

#include "llimpanel.h"

#include "indra_constants.h"
#include "llbutton.h"
#include "llchat.h"
#include "llerror.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llhttpclient.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llnotifications.h"
#include "llrect.h"
#include "llresmgr.h"
#include "llslider.h"
#include "llstring.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "message.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llconsole.h"
#include "llfloateractivespeakers.h"
#include "llfloateravatarinfo.h"
#include "llfloaterchat.h"
#include "llfloatergroupinfo.h"
#include "hbfloatertextinput.h"
#include "llimview.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "lllogchat.h"
#include "llmutelist.h"
#include "llstylemap.h"
#include "llviewertexteditor.h"
#include "llviewermessage.h"
#include "llviewerstats.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llweb.h"

//
// Constants
//
const S32 LINE_HEIGHT = 16;
const S32 MIN_WIDTH = 200;
const S32 MIN_HEIGHT = 130;
const U32 DEFAULT_RETRIES_COUNT = 3;

//
// Statics
//
static std::string sTitleString = "Instant Message with [NAME]";
static std::string sTypingStartString = "[NAME]: ...";
static std::string sSessionStartString = "Starting session with [NAME] please wait.";
static std::string sDefaultTextString = "Click here to instant message.";
static std::string sUnavailableTextString;
static std::string sMutedTextString;

LLVoiceChannel::voice_channel_map_t LLVoiceChannel::sVoiceChannelMap;
LLVoiceChannel::voice_channel_map_uri_t LLVoiceChannel::sVoiceChannelURIMap;
LLVoiceChannel* LLVoiceChannel::sCurrentVoiceChannel = NULL;
LLVoiceChannel* LLVoiceChannel::sSuspendedVoiceChannel = NULL;

BOOL LLVoiceChannel::sSuspended = FALSE;

std::set<LLFloaterIMPanel*> LLFloaterIMPanel::sFloaterIMPanels;

void session_starter_helper(const LLUUID& temp_session_id,
							const LLUUID& other_participant_id,
							EInstantMessage im_type)
{
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
	msg->addUUIDFast(_PREHASH_ToAgentID, other_participant_id);
	msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
	msg->addU8Fast(_PREHASH_Dialog, im_type);
	msg->addUUIDFast(_PREHASH_ID, temp_session_id);
	msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary

	std::string name;
	gAgent.buildFullname(name);

	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, LLStringUtil::null);
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
}

void start_deprecated_conference_chat(const LLUUID& temp_session_id,
									  const LLUUID& creator_id,
									  const LLUUID& other_participant_id,
									  const LLSD& agents_to_invite)
{
	U8* bucket;
	U8* pos;
	S32 count;
	S32 bucket_size;

	// *FIX: this could suffer from endian issues
	count = agents_to_invite.size();
	bucket_size = UUID_BYTES * count;
	bucket = new U8[bucket_size];
	pos = bucket;

	for (S32 i = 0; i < count; ++i)
	{
		LLUUID agent_id = agents_to_invite[i].asUUID();

		memcpy(pos, &agent_id, UUID_BYTES);
		pos += UUID_BYTES;
	}

	session_starter_helper(temp_session_id, other_participant_id,
						   IM_SESSION_CONFERENCE_START);

	gMessageSystem->addBinaryDataFast(_PREHASH_BinaryBucket, bucket, bucket_size);

	gAgent.sendReliableMessage();

	delete[] bucket;
}

class LLStartConferenceChatResponder : public LLHTTPClient::Responder
{
public:
	LLStartConferenceChatResponder(const LLUUID& temp_session_id,
								   const LLUUID& creator_id,
								   const LLUUID& other_participant_id,
								   const LLSD& agents_to_invite)
	{
		mTempSessionID = temp_session_id;
		mCreatorID = creator_id;
		mOtherParticipantID = other_participant_id;
		mAgents = agents_to_invite;
	}

	virtual void error(U32 statusNum, const std::string& reason)
	{
		//try an "old school" way.
		if (statusNum == 400)
		{
			start_deprecated_conference_chat(mTempSessionID, mCreatorID,
											 mOtherParticipantID, mAgents);
		}

		//else throw an error back to the client?
		//in theory we should have just have these error strings
		//etc. set up in this file as opposed to the IMMgr,
		//but the error string were unneeded here previously
		//and it is not worth the effort switching over all
		//the possible different language translations
	}

private:
	LLUUID mTempSessionID;
	LLUUID mCreatorID;
	LLUUID mOtherParticipantID;

	LLSD mAgents;
};

// Returns true if any messages were sent, false otherwise.
// Is sort of equivalent to "does the server need to do anything?"
bool send_start_session_messages(const LLUUID& temp_session_id,
								 const LLUUID& other_participant_id,
								 const LLDynamicArray<LLUUID>& ids,
								 EInstantMessage dialog)
{
	if (dialog == IM_SESSION_GROUP_START)
	{
		session_starter_helper(temp_session_id, other_participant_id, dialog);
		gMessageSystem->addBinaryDataFast(_PREHASH_BinaryBucket,
										  EMPTY_BINARY_BUCKET,
										  EMPTY_BINARY_BUCKET_SIZE);
		gAgent.sendReliableMessage();

		return true;
	}
	else if (dialog == IM_SESSION_CONFERENCE_START)
	{
		LLSD agents;
		for (int i = 0; i < (S32)ids.size(); i++)
		{
			agents.append(ids.get(i));
		}

		//we have a new way of starting conference calls now
		LLViewerRegion* region = gAgent.getRegion();
		if (region)
		{
			std::string url = region->getCapability("ChatSessionRequest");
			LLSD data;
			data["method"] = "start conference";
			data["session-id"] = temp_session_id;
			data["params"] = agents;

			LLHTTPClient::post(url, data,
							   new LLStartConferenceChatResponder(temp_session_id,
																  gAgentID,
																  other_participant_id,
																  data["params"]));
		}
		else
		{
			start_deprecated_conference_chat(temp_session_id,
											 gAgentID,
											 other_participant_id,
											 agents);
		}
	}

	return false;
}

class LLVoiceCallCapResponder : public LLHTTPClient::Responder
{
public:
	LLVoiceCallCapResponder(const LLUUID& session_id) : mSessionID(session_id) {};

	virtual void error(U32 status, const std::string& reason);	// called with bad status codes
	virtual void result(const LLSD& content);

private:
	LLUUID mSessionID;
};


void LLVoiceCallCapResponder::error(U32 status, const std::string& reason)
{
	llwarns << "LLVoiceCallCapResponder::error(" << status << ": "
			<< reason << ")" << llendl;
	LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(mSessionID);
	if (channelp)
	{
		if (403 == status)
		{
			//403 == no ability
			LLNotifications::instance().add("VoiceNotAllowed",
											channelp->getNotifyArgs());
		}
		else
		{
			LLNotifications::instance().add("VoiceCallGenericError",
											channelp->getNotifyArgs());
		}
		channelp->deactivate();
	}
}

void LLVoiceCallCapResponder::result(const LLSD& content)
{
	LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(mSessionID);
	if (channelp)
	{
		//*TODO: DEBUG SPAM
		LLSD::map_const_iterator iter;
		for (iter = content.beginMap(); iter != content.endMap(); ++iter)
		{
			llinfos << "LLVoiceCallCapResponder::result got " << iter->first
					<< llendl;
		}

		channelp->setChannelInfo(content["voice_credentials"]["channel_uri"].asString(),
								 content["voice_credentials"]["channel_credentials"].asString());
	}
}

//
// LLVoiceChannel
//
LLVoiceChannel::LLVoiceChannel(const LLUUID& session_id,
							   const std::string& session_name)
:	mSessionID(session_id),
	mState(STATE_NO_CHANNEL_INFO),
	mSessionName(session_name),
	mIgnoreNextSessionLeave(FALSE)
{
	mNotifyArgs["VOICE_CHANNEL_NAME"] = mSessionName;

	if (!sVoiceChannelMap.insert(std::make_pair(session_id, this)).second)
	{
		// a voice channel already exists for this session id, so this instance
		// will be orphaned the end result should simply be the failure to make
		// voice calls
		llwarns << "Duplicate voice channels registered for session_id "
				<< session_id << llendl;
	}
}

LLVoiceChannel::~LLVoiceChannel()
{
	// Don't use LLVoiceClient::getInstance() here -- this can get called
	// during atexit() time and that singleton MAY have already been destroyed.
	if (LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver(this);
	}

	sVoiceChannelMap.erase(mSessionID);
	sVoiceChannelURIMap.erase(mURI);
}

void LLVoiceChannel::setChannelInfo(const std::string& uri,
									const std::string& credentials)
{
	setURI(uri);

	mCredentials = credentials;

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		if (mURI.empty())
		{
			LLNotifications::instance().add("VoiceChannelJoinFailed",
											mNotifyArgs);
			llwarns << "Received empty URI for channel " << mSessionName
					<< llendl;
			deactivate();
		}
		else if (mCredentials.empty())
		{
			LLNotifications::instance().add("VoiceChannelJoinFailed",
											mNotifyArgs);
			llwarns << "Received empty credentials for channel "
					<< mSessionName << llendl;
			deactivate();
		}
		else
		{
			setState(STATE_READY);

			// If we are supposed to be active, reconnect. This will happen on
			// initial connect, as we request credentials on first use
			if (sCurrentVoiceChannel == this)
			{
				// just in case we got new channel info while active
				// should move over to new channel
				activate();
			}
		}
	}
}

void LLVoiceChannel::onChange(EStatusType type, const std::string &channelURI, bool proximal)
{
	if (channelURI != mURI)
	{
		return;
	}

	if (type < BEGIN_ERROR_STATUS)
	{
		handleStatusChange(type);
	}
	else
	{
		handleError(type);
	}
}

void LLVoiceChannel::handleStatusChange(EStatusType type)
{
	// status updates
	switch (type)
	{
	case STATUS_LOGIN_RETRY:
		LLNotifications::instance().add("VoiceLoginRetry");
		break;
	case STATUS_LOGGED_IN:
		break;
	case STATUS_LEFT_CHANNEL:
		if (callStarted() && !mIgnoreNextSessionLeave && !sSuspended)
		{
			// if forceably removed from channel
			// update the UI and revert to default channel
			LLNotifications::instance().add("VoiceChannelDisconnected", mNotifyArgs);
			deactivate();
		}
		mIgnoreNextSessionLeave = FALSE;
		break;
	case STATUS_JOINING:
		if (callStarted())
		{
			setState(STATE_RINGING);
		}
		break;
	case STATUS_JOINED:
		if (callStarted())
		{
			setState(STATE_CONNECTED);
		}
	default:
		break;
	}
}

// default behavior is to just deactivate channel
// derived classes provide specific error messages
void LLVoiceChannel::handleError(EStatusType type)
{
	deactivate();
	setState(STATE_ERROR);
}

BOOL LLVoiceChannel::isActive()
{
	// only considered active when currently bound channel matches what our
	// channel
	return callStarted() &&
		   LLVoiceClient::getInstance()->getCurrentChannel() == mURI;
}

BOOL LLVoiceChannel::callStarted()
{
	return mState >= STATE_CALL_STARTED;
}

void LLVoiceChannel::deactivate()
{
	if (mState >= STATE_RINGING)
	{
		// ignore session leave event
		mIgnoreNextSessionLeave = TRUE;
	}

	if (callStarted())
	{
		setState(STATE_HUNG_UP);
		// mute the microphone if required when returning to the proximal
		// channel
		if (sCurrentVoiceChannel == this &&
			gSavedSettings.getBOOL("AutoDisengageMic"))
		{
			gSavedSettings.setBOOL("PTTCurrentlyEnabled", true);
		}
	}
	LLVoiceClient::getInstance()->removeObserver(this);

	if (sCurrentVoiceChannel == this)
	{
		// default channel is proximal channel
		sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
		sCurrentVoiceChannel->activate();
	}
}

void LLVoiceChannel::activate()
{
	if (callStarted())
	{
		return;
	}

	// deactivate old channel and mark ourselves as the active one
	if (sCurrentVoiceChannel != this)
	{
		// mark as current before deactivating the old channel to prevent
		// activating the proximal channel between IM calls
		LLVoiceChannel* old_channel = sCurrentVoiceChannel;
		sCurrentVoiceChannel = this;
		if (old_channel)
		{
			old_channel->deactivate();
		}
	}

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		// responsible for setting status to active
		getChannelInfo();
	}
	else
	{
		setState(STATE_CALL_STARTED);
	}

	LLVoiceClient::getInstance()->addObserver(this);
}

void LLVoiceChannel::getChannelInfo()
{
	// pretend we have everything we need
	if (sCurrentVoiceChannel == this)
	{
		setState(STATE_CALL_STARTED);
	}
}

//static
LLVoiceChannel* LLVoiceChannel::getChannelByID(const LLUUID& session_id)
{
	voice_channel_map_t::iterator found_it = sVoiceChannelMap.find(session_id);
	if (found_it == sVoiceChannelMap.end())
	{
		return NULL;
	}
	else
	{
		return found_it->second;
	}
}

//static
LLVoiceChannel* LLVoiceChannel::getChannelByURI(std::string uri)
{
	voice_channel_map_uri_t::iterator found_it = sVoiceChannelURIMap.find(uri);
	if (found_it == sVoiceChannelURIMap.end())
	{
		return NULL;
	}
	else
	{
		return found_it->second;
	}
}

void LLVoiceChannel::updateSessionID(const LLUUID& new_session_id)
{
	sVoiceChannelMap.erase(sVoiceChannelMap.find(mSessionID));
	mSessionID = new_session_id;
	sVoiceChannelMap.insert(std::make_pair(mSessionID, this));
}

void LLVoiceChannel::setURI(std::string uri)
{
	sVoiceChannelURIMap.erase(mURI);
	mURI = uri;
	sVoiceChannelURIMap.insert(std::make_pair(mURI, this));
}

void LLVoiceChannel::setState(EState state)
{
	if (!gIMMgr) return;

	switch (state)
	{
		case STATE_RINGING:
			gIMMgr->addSystemMessage(mSessionID, "ringing", mNotifyArgs);
			break;
		case STATE_CONNECTED:
			gIMMgr->addSystemMessage(mSessionID, "connected", mNotifyArgs);
			break;
		case STATE_HUNG_UP:
			gIMMgr->addSystemMessage(mSessionID, "hang_up", mNotifyArgs);
			break;
		default:
			break;
	}

	mState = state;
}

//static
void LLVoiceChannel::initClass()
{
	sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
}

//static
void LLVoiceChannel::suspend()
{
	if (!sSuspended)
	{
		sSuspendedVoiceChannel = sCurrentVoiceChannel;
		sSuspended = TRUE;
	}
}

//static
void LLVoiceChannel::resume()
{
	if (sSuspended)
	{
		if (LLVoiceClient::getInstance()->voiceEnabled())
		{
			if (sSuspendedVoiceChannel)
			{
				sSuspendedVoiceChannel->activate();
			}
			else
			{
				LLVoiceChannelProximal::getInstance()->activate();
			}
		}
		sSuspended = FALSE;
	}
}

//
// LLVoiceChannelGroup
//

LLVoiceChannelGroup::LLVoiceChannelGroup(const LLUUID& session_id,
										 const std::string& session_name)
:	LLVoiceChannel(session_id, session_name)
{
	mRetries = DEFAULT_RETRIES_COUNT;
	mIsRetrying = FALSE;
}

void LLVoiceChannelGroup::deactivate()
{
	if (callStarted())
	{
		LLVoiceClient::getInstance()->leaveNonSpatialChannel();
	}
	LLVoiceChannel::deactivate();
}

void LLVoiceChannelGroup::activate()
{
	if (callStarted()) return;

	LLVoiceChannel::activate();

	if (callStarted())
	{
		// we have the channel info, just need to use it now
		LLVoiceClient::getInstance()->setNonSpatialChannel(mURI, mCredentials);
	}
}

void LLVoiceChannelGroup::getChannelInfo()
{
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		std::string url = region->getCapability("ChatSessionRequest");
		LLSD data;
		data["method"] = "call";
		data["session-id"] = mSessionID;
		LLHTTPClient::post(url, data, new LLVoiceCallCapResponder(mSessionID));
	}
}

void LLVoiceChannelGroup::setChannelInfo(const std::string& uri,
										 const std::string& credentials)
{
	setURI(uri);

	mCredentials = credentials;

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		if (!mURI.empty() && !mCredentials.empty())
		{
			setState(STATE_READY);

			// If we are supposed to be active, reconnect. This will happen on
			// initial connect, as we request credentials on first use
			if (sCurrentVoiceChannel == this)
			{
				// just in case we got new channel info while active
				// should move over to new channel
				activate();
			}
		}
		else
		{
			//*TODO: notify user
			llwarns << "Received invalid credentials for channel "
					<< mSessionName << llendl;
			deactivate();
		}
	}
	else if (mIsRetrying)
	{
		// we have the channel info, just need to use it now
		LLVoiceClient::getInstance()->setNonSpatialChannel(mURI, mCredentials);
	}
}

void LLVoiceChannelGroup::handleStatusChange(EStatusType type)
{
	// status updates
	if (type == STATUS_JOINED)
	{
		mRetries = 3;
		mIsRetrying = FALSE;
	}

	LLVoiceChannel::handleStatusChange(type);
}

void LLVoiceChannelGroup::handleError(EStatusType status)
{
	std::string notify;
	switch (status)
	{
		case ERROR_CHANNEL_LOCKED:
		case ERROR_CHANNEL_FULL:
			notify = "VoiceChannelFull";
			break;
		case ERROR_NOT_AVAILABLE:
			// clear URI and credentials, set the state to be no info and
			// activate
			if (mRetries > 0)
			{
				mRetries--;
				mIsRetrying = TRUE;
				mIgnoreNextSessionLeave = TRUE;
				getChannelInfo();
				return;
			}
			else
			{
				notify = "VoiceChannelJoinFailed";
				mRetries = DEFAULT_RETRIES_COUNT;
				mIsRetrying = FALSE;
			}
			break;
		case ERROR_UNKNOWN:
		default:
			break;
	}

	// notification
	if (!notify.empty())
	{
		LLNotificationPtr notification;
		notification = LLNotifications::instance().add(notify, mNotifyArgs);
		// echo to im window
		if (gIMMgr)
		{
			gIMMgr->addMessage(mSessionID, LLUUID::null, SYSTEM_FROM,
							   notification->getMessage());
		}
	}

	LLVoiceChannel::handleError(status);
}

void LLVoiceChannelGroup::setState(EState state)
{
	if (state == STATE_RINGING)
	{
		if (!mIsRetrying && gIMMgr)
		{
			gIMMgr->addSystemMessage(mSessionID, "ringing", mNotifyArgs);
		}
		mState = state;
	}
	else
	{
		LLVoiceChannel::setState(state);
	}
}

//
// LLVoiceChannelProximal
//
LLVoiceChannelProximal::LLVoiceChannelProximal()
:	LLVoiceChannel(LLUUID::null, LLStringUtil::null)
{
	activate();
}

BOOL LLVoiceChannelProximal::isActive()
{
	return callStarted() && LLVoiceClient::getInstance()->inProximalChannel();
}

void LLVoiceChannelProximal::activate()
{
	if (callStarted()) return;

	LLVoiceChannel::activate();

	if (callStarted())
	{
		// this implicitly puts you back in the spatial channel
		LLVoiceClient::getInstance()->leaveNonSpatialChannel();
	}
}

void LLVoiceChannelProximal::onChange(EStatusType type,
									  const std::string &channelURI,
									  bool proximal)
{
	if (!proximal)
	{
		return;
	}

	if (type < BEGIN_ERROR_STATUS)
	{
		handleStatusChange(type);
	}
	else
	{
		handleError(type);
	}
}

void LLVoiceChannelProximal::handleStatusChange(EStatusType status)
{
	// status updates
	switch (status)
	{
		case STATUS_LEFT_CHANNEL:
			// do not notify user when leaving proximal channel
			return;
		case STATUS_VOICE_DISABLED:
			if (gIMMgr)
			{
				gIMMgr->addSystemMessage(LLUUID::null, "unavailable",
										 mNotifyArgs);
			}
			return;
		default:
			break;
	}
	LLVoiceChannel::handleStatusChange(status);
}

void LLVoiceChannelProximal::handleError(EStatusType status)
{
	if (status == ERROR_CHANNEL_LOCKED || status == ERROR_CHANNEL_FULL)
	{
		LLNotifications::instance().add("ProximalVoiceChannelFull",
										mNotifyArgs);
	}

	LLVoiceChannel::handleError(status);
}

void LLVoiceChannelProximal::deactivate()
{
	if (callStarted())
	{
		setState(STATE_HUNG_UP);
	}
}

//
// LLVoiceChannelP2P
//
LLVoiceChannelP2P::LLVoiceChannelP2P(const LLUUID& session_id,
									 const std::string& session_name,
									 const LLUUID& other_user_id)
:	LLVoiceChannelGroup(session_id, session_name),
	mOtherUserID(other_user_id),
	mReceivedCall(FALSE)
{
	// make sure URI reflects encoded version of other user's agent id
	setURI(LLVoiceClient::getInstance()->sipURIFromID(other_user_id));
}

void LLVoiceChannelP2P::handleStatusChange(EStatusType type)
{
	// status updates
	if (type == STATUS_LEFT_CHANNEL)
	{
		if (callStarted() && !mIgnoreNextSessionLeave && !sSuspended)
		{
			if (mState == STATE_RINGING)
			{
				// other user declined call
				LLNotifications::instance().add("P2PCallDeclined",
												mNotifyArgs);
			}
			else
			{
				// other user hung up
				LLNotifications::instance().add("VoiceChannelDisconnectedP2P",
												mNotifyArgs);
			}
			deactivate();
		}
		mIgnoreNextSessionLeave = FALSE;
		return;
	}

	LLVoiceChannel::handleStatusChange(type);
}

void LLVoiceChannelP2P::handleError(EStatusType type)
{
	if (type == ERROR_NOT_AVAILABLE)
	{
		LLNotifications::instance().add("P2PCallNoAnswer", mNotifyArgs);
	}

	LLVoiceChannel::handleError(type);
}

void LLVoiceChannelP2P::activate()
{
	if (callStarted()) return;

	LLVoiceChannel::activate();

	if (callStarted())
	{
		// no session handle yet, we're starting the call
		if (mSessionHandle.empty())
		{
			mReceivedCall = FALSE;
			LLVoiceClient::getInstance()->callUser(mOtherUserID);
		}
		// otherwise answering the call
		else
		{
			LLVoiceClient::getInstance()->answerInvite(mSessionHandle);

			// Using the session handle invalidates it.
			// Clear it out here so we can't reuse it by accident.
			mSessionHandle.clear();
		}
	}
}

void LLVoiceChannelP2P::getChannelInfo()
{
	// pretend we have everything we need, since P2P doesn't use channel info
	if (sCurrentVoiceChannel == this)
	{
		setState(STATE_CALL_STARTED);
	}
}

// receiving session from other user who initiated call
void LLVoiceChannelP2P::setSessionHandle(const std::string& handle,
										 const std::string &inURI)
{
	BOOL needs_activate = FALSE;
	if (callStarted())
	{
		// defer to lower agent id when already active
		if (mOtherUserID < gAgentID)
		{
			// pretend we haven't started the call yet, so we can connect to
			// this session instead
			deactivate();
			needs_activate = TRUE;
		}
		else
		{
			// we are active and have priority, invite the other user again
			// under the assumption they will join this new session
			mSessionHandle.clear();
			LLVoiceClient::getInstance()->callUser(mOtherUserID);
			return;
		}
	}

	mSessionHandle = handle;

	// The URI of a p2p session should always be the other end's SIP URI.
	if (!inURI.empty())
	{
		setURI(inURI);
	}
	else
	{
		setURI(LLVoiceClient::getInstance()->sipURIFromID(mOtherUserID));
	}

	mReceivedCall = TRUE;

	if (needs_activate)
	{
		activate();
	}
}

void LLVoiceChannelP2P::setState(EState state)
{
	// you only "answer" voice invites in p2p mode
	// so provide a special purpose message here
	if (mReceivedCall && state == STATE_RINGING)
	{
		if (gIMMgr)
		{
			gIMMgr->addSystemMessage(mSessionID, "answering", mNotifyArgs);
		}
		mState = state;
		return;
	}
	LLVoiceChannel::setState(state);
}

//
// LLFloaterIMPanel
//
LLFloaterIMPanel::LLFloaterIMPanel(const std::string& session_label,
								   const LLUUID& session_id,
								   const LLUUID& other_participant_id,
								   EInstantMessage dialog)
:	LLFloater(session_label, LLRect(), session_label),
	mInputEditor(NULL),
	mHistoryEditor(NULL),
	mSendButton(NULL),
	mOpenTextEditorButton(NULL),
	mStartCallButton(NULL),
	mEndCallButton(NULL),
	mToggleSpeakersButton(NULL),
	mSpeakerVolumeSlider(NULL),
	mMuteButton(NULL),
	mSessionUUID(session_id),
	mVoiceChannel(NULL),
	mSessionInitialized(FALSE),
	mSessionStartMsgPos(0),
	mOtherParticipantUUID(other_participant_id),
	mDialog(dialog),
	mHasScrolledOnce(false),
	mTyping(FALSE),
	mOtherTyping(FALSE),
	mTypingLineStartIndex(0),
	mSentTypingState(TRUE),
	mNumUnreadMessages(0),
	mShowSpeakersOnConnect(TRUE),
	mAutoConnect(FALSE),
	mTextIMPossible(TRUE),
	mProfileButtonEnabled(TRUE),
	mCallBackEnabled(TRUE),
	mSpeakers(NULL),
	mSpeakerPanel(NULL),
	mFirstKeystrokeTimer(),
	mLastKeystrokeTimer()
{
	sFloaterIMPanels.insert(this);
	init(session_label);
}

LLFloaterIMPanel::LLFloaterIMPanel(const std::string& session_label,
								   const LLUUID& session_id,
								   const LLUUID& other_participant_id,
								   const LLDynamicArray<LLUUID>& ids,
								   EInstantMessage dialog)
:	LLFloater(session_label, LLRect(), session_label),
	mInputEditor(NULL),
	mHistoryEditor(NULL),
	mSendButton(NULL),
	mOpenTextEditorButton(NULL),
	mStartCallButton(NULL),
	mEndCallButton(NULL),
	mToggleSpeakersButton(NULL),
	mSpeakerVolumeSlider(NULL),
	mMuteButton(NULL),
	mSessionUUID(session_id),
	mVoiceChannel(NULL),
	mSessionInitialized(FALSE),
	mSessionStartMsgPos(0),
	mOtherParticipantUUID(other_participant_id),
	mDialog(dialog),
	mHasScrolledOnce(false),
	mTyping(FALSE),
	mOtherTyping(FALSE),
	mTypingLineStartIndex(0),
	mSentTypingState(TRUE),
	mShowSpeakersOnConnect(TRUE),
	mAutoConnect(FALSE),
	mTextIMPossible(TRUE),
	mProfileButtonEnabled(TRUE),
	mCallBackEnabled(TRUE),
	mSpeakers(NULL),
	mSpeakerPanel(NULL),
	mFirstKeystrokeTimer(),
	mLastKeystrokeTimer()
{
	sFloaterIMPanels.insert(this);
	mSessionInitialTargetIDs = ids;
	init(session_label);
}

void LLFloaterIMPanel::init(const std::string& session_label)
{
	mSessionLabel = mSessionLog = session_label;
	mProfileButtonEnabled = FALSE;

	std::string xml_filename;
	switch (mDialog)
	{
	case IM_SESSION_GROUP_START:
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		xml_filename = "floater_instant_message_group.xml";
		mVoiceChannel = new LLVoiceChannelGroup(mSessionUUID, mSessionLabel);
		break;

	case IM_SESSION_INVITE:
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		if (gAgent.isInGroup(mSessionUUID))
		{
			xml_filename = "floater_instant_message_group.xml";
		}
		else // must be invite to ad hoc IM
		{
			xml_filename = "floater_instant_message_ad_hoc.xml";
		}
		mVoiceChannel = new LLVoiceChannelGroup(mSessionUUID, mSessionLabel);
		break;

	case IM_SESSION_P2P_INVITE:
		xml_filename = "floater_instant_message.xml";
		mProfileButtonEnabled = TRUE;
		if (LLAvatarName::sOmitResidentAsLastName)
		{
			mSessionLabel = LLCacheName::cleanFullName(mSessionLabel);
		}
		mVoiceChannel = new LLVoiceChannelP2P(mSessionUUID, mSessionLabel, mOtherParticipantUUID);
		break;

	case IM_SESSION_CONFERENCE_START:
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		xml_filename = "floater_instant_message_ad_hoc.xml";
		mVoiceChannel = new LLVoiceChannelGroup(mSessionUUID, mSessionLabel);
		break;

	case IM_NOTHING_SPECIAL:	// just received text from another user
		xml_filename = "floater_instant_message.xml";
		mTextIMPossible = LLVoiceClient::getInstance()->isSessionTextIMPossible(mSessionUUID);
		mProfileButtonEnabled = LLVoiceClient::getInstance()->isParticipantAvatar(mSessionUUID);
		if (mProfileButtonEnabled && LLAvatarName::sOmitResidentAsLastName)
		{
			mSessionLabel = LLCacheName::cleanFullName(mSessionLabel);
		}
		mCallBackEnabled = LLVoiceClient::getInstance()->isSessionCallBackPossible(mSessionUUID);
		mVoiceChannel = new LLVoiceChannelP2P(mSessionUUID, mSessionLabel, mOtherParticipantUUID);
		break;

	default:
		llwarns << "Unknown session type" << llendl;
		xml_filename = "floater_instant_message.xml";
		break;
	}

	mSpeakers = new LLIMSpeakerMgr(mVoiceChannel);

	LLUICtrlFactory::getInstance()->buildFloater(this, xml_filename,
												 &getFactoryMap(), FALSE);

	if (mProfileButtonEnabled && mSessionLog.find(' ') == std::string::npos)
	{
		// Make sure the IM log file will be unique (avoid getting both
		// "JohnDoe.txt" and "JohnDoe Resident.txt", depending on how
		// the IM session was started)
		mSessionLog += " Resident";
	}

	setTitle(mSessionLabel);
	if (mProfileButtonEnabled)
	{
		lookupName();
	}

	mInputEditor->setMaxTextLength(DB_IM_MSG_STR_LEN);
	// enable line history support for instant message bar
	mInputEditor->setEnableLineHistory(TRUE);

	if (gSavedPerAccountSettings.getBOOL("LogShowHistory"))
	{
		LLLogChat::loadHistory(mSessionLog, &chatFromLogFile, (void *)this);
	}

	if (!mSessionInitialized)
	{
		if (!send_start_session_messages(mSessionUUID, mOtherParticipantUUID,
				 						 mSessionInitialTargetIDs, mDialog))
		{
			//we don't need to need to wait for any responses
			//so we're already initialized
			mSessionInitialized = TRUE;
			mSessionStartMsgPos = 0;
		}
		else
		{
			//locally echo a little "starting session" message
			LLUIString session_start = sSessionStartString;

			session_start.setArg("[NAME]", getTitle());
			mSessionStartMsgPos = mHistoryEditor->getWText().length();

			addHistoryLine(session_start,
						   gSavedSettings.getColor4("SystemChatColor"),
						   false);
		}
	}
}

void LLFloaterIMPanel::lookupName()
{
	LLAvatarNameCache::get(mOtherParticipantUUID,
						   boost::bind(&LLFloaterIMPanel::onAvatarNameLookup,
									   _1, _2, this));
}

//static
void LLFloaterIMPanel::onAvatarNameLookup(const LLUUID& id,
										  const LLAvatarName& avatar_name,
										  void* user_data)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)user_data;

	if (self && sFloaterIMPanels.count(self) != 0)
	{
		// Always show "Display Name [Legacy Name]" for security reasons
		std::string title = avatar_name.getNames();
		if (!title.empty())
		{
			self->setTitle(title);
		}
	}
}

LLFloaterIMPanel::~LLFloaterIMPanel()
{
	sFloaterIMPanels.erase(this);

	delete mSpeakers;
	mSpeakers = NULL;

	// End the text IM session if necessary
	if (LLVoiceClient::instanceExists() && mOtherParticipantUUID.notNull())
	{
		if (mDialog == IM_NOTHING_SPECIAL || mDialog == IM_SESSION_P2P_INVITE)
		{
			LLVoiceClient::getInstance()->endUserIMSession(mOtherParticipantUUID);
		}
	}

	// Kicks you out of the voice channel if it is currently active

	// HAVE to do this here -- if it happens in the LLVoiceChannel destructor it
	// will call the wrong version (since the object's partially deconstructed
	// at that point).
	mVoiceChannel->deactivate();

	delete mVoiceChannel;
	mVoiceChannel = NULL;

	//delete focus lost callback
	if (mInputEditor)
	{
		mInputEditor->setFocusLostCallback(NULL);
	}
}

BOOL LLFloaterIMPanel::postBuild()
{
	requires<LLLineEditor>("chat_editor");
	requires<LLTextEditor>("im_history");

	if (checkRequirements())
	{
		mInputEditor = getChild<LLLineEditor>("chat_editor");
		mInputEditor->setFocusReceivedCallback(onInputEditorFocusReceived, this);
		mInputEditor->setFocusLostCallback(onInputEditorFocusLost, this);
		mInputEditor->setKeystrokeCallback(onInputEditorKeystroke);
		mInputEditor->setScrolledCallback(onInputEditorScrolled, this);
		mInputEditor->setCommitCallback(onCommitChat);
		mInputEditor->setCallbackUserData(this);
		mInputEditor->setCommitOnFocusLost(FALSE);
		mInputEditor->setRevertOnEsc(FALSE);
		mInputEditor->setReplaceNewlinesWithSpaces(FALSE);

		if (getChild<LLButton>("profile_callee_btn", TRUE, FALSE))
		{
			childSetAction("profile_callee_btn", onClickProfile, this);
			if (!mProfileButtonEnabled)
			{
				childSetEnabled("profile_callee_btn", false);
			}
		}
		if (getChild<LLButton>("group_info_btn", TRUE, FALSE))
		{
			childSetAction("group_info_btn", onClickGroupInfo, this);
		}

		mStartCallButton = getChild<LLButton>("start_call_btn", TRUE, FALSE);
		if (mStartCallButton)
		{
			mStartCallButton->setClickedCallback(onClickStartCall, this);
			mEndCallButton = getChild<LLButton>("end_call_btn");
			mEndCallButton->setClickedCallback(onClickEndCall, this);
		}

		mSendButton = getChild<LLButton>("send_btn", TRUE, FALSE);
		if (mSendButton)
		{
			mSendButton->setClickedCallback(onClickSend, this);
		}

		mOpenTextEditorButton = getChild<LLButton>("open_text_editor_btn", TRUE, FALSE);
		if (mOpenTextEditorButton)
		{
			mOpenTextEditorButton->setClickedCallback(onClickOpenTextEditor, this);
		}

		mToggleSpeakersButton = getChild<LLButton>("toggle_active_speakers_btn", TRUE, FALSE);
		if (mToggleSpeakersButton)
		{
			mToggleSpeakersButton->setClickedCallback(onClickToggleActiveSpeakers, this);
		}

		//LLButton* close_btn = getChild<LLButton>("close_btn");
		//close_btn->setClickedCallback(&LLFloaterIMPanel::onClickClose, this);

		mHistoryEditor = getChild<LLViewerTextEditor>("im_history");
		mHistoryEditor->setParseHTML(TRUE);
		mHistoryEditor->setParseHighlights(TRUE);

		if (mDialog == IM_SESSION_GROUP_START)
		{
			childSetEnabled("profile_btn", FALSE);
		}

		sTitleString = getString("title_string");
		sTypingStartString = getString("typing_start_string");
		sSessionStartString = getString("session_start_string");
		sDefaultTextString = getString("default_text_label");

		if (mSpeakerPanel)
		{
			mSpeakerPanel->refreshSpeakers();
		}

		if (mDialog == IM_NOTHING_SPECIAL)
		{
			mMuteButton = getChild<LLButton>("mute_btn", TRUE, FALSE);
			if (mMuteButton)
			{
				mMuteButton->setClickedCallback(onClickMuteVoice, this);
				mSpeakerVolumeSlider = getChild<LLSlider>("speaker_volume");
				childSetCommitCallback("speaker_volume", onVolumeChange, this);
			}
		}

		setDefaultBtn("send_btn");

		return TRUE;
	}

	return FALSE;
}

void* LLFloaterIMPanel::createSpeakersPanel(void* data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)data;
	floaterp->mSpeakerPanel = new LLPanelActiveSpeakers(floaterp->mSpeakers, TRUE);
	return floaterp->mSpeakerPanel;
}

//static
void LLFloaterIMPanel::onClickMuteVoice(void* user_data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)user_data;
	if (floaterp)
	{
		LLMuteList* ml = LLMuteList::getInstance();
		if (!ml) return;
		BOOL is_muted = ml->isMuted(floaterp->mOtherParticipantUUID, LLMute::flagVoiceChat);

		LLMute mute(floaterp->mOtherParticipantUUID, floaterp->getTitle(), LLMute::AGENT);
		if (!is_muted)
		{
			ml->add(mute, LLMute::flagVoiceChat);
		}
		else
		{
			ml->remove(mute, LLMute::flagVoiceChat);
		}
	}
}

//static
void LLFloaterIMPanel::onVolumeChange(LLUICtrl* source, void* user_data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)user_data;
	if (floaterp && LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->setUserVolume(floaterp->mOtherParticipantUUID, (F32)source->getValue().asReal());
	}
}

// virtual
void LLFloaterIMPanel::draw()
{
	LLViewerRegion* region = gAgent.getRegion();

	BOOL enable_connect = region &&
						  region->getCapability("ChatSessionRequest") != "" &&
					  	  mCallBackEnabled && mSessionInitialized &&
						  LLVoiceClient::voiceEnabled();

	if (mStartCallButton)
	{
		// hide/show start call and end call buttons
		BOOL call_started = mVoiceChannel &&
							mVoiceChannel->getState() >= LLVoiceChannel::STATE_CALL_STARTED;

		mStartCallButton->setVisible(LLVoiceClient::voiceEnabled() && !call_started);
		mStartCallButton->setEnabled(enable_connect);
		mEndCallButton->setVisible(LLVoiceClient::voiceEnabled() && call_started);
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
		if (mSendButton)
		{
			mSendButton->setEnabled(!empty && !has_text_editor);
		}

		LLPointer<LLSpeaker> self_speaker = mSpeakers->findSpeaker(gAgentID);
		if (!mTextIMPossible)
		{
			if (sUnavailableTextString.empty())
			{
				sUnavailableTextString = getString("unavailable_text_label");
			}
			mInputEditor->setEnabled(FALSE);
			mInputEditor->setLabel(sUnavailableTextString);
		}
		else if (self_speaker.notNull() && self_speaker->mModeratorMutedText)
		{
			if (sMutedTextString.empty())
			{
				sMutedTextString = getString("muted_text_label");
			}
			mInputEditor->setEnabled(FALSE);
			mInputEditor->setLabel(sMutedTextString);
		}
		else
		{
			mInputEditor->setEnabled(!has_text_editor);
			mInputEditor->setLabel(sDefaultTextString);
		}
	}

	if (mAutoConnect && enable_connect)
	{
		onClickStartCall(this);
		mAutoConnect = FALSE;
	}

	// show speakers window when voice first connects
	if (mShowSpeakersOnConnect && mSpeakerPanel && mVoiceChannel->isActive())
	{
		mSpeakerPanel->setVisible(TRUE);
		mShowSpeakersOnConnect = FALSE;
	}
	if (mToggleSpeakersButton)
	{
		mToggleSpeakersButton->setValue(mSpeakerPanel && mSpeakerPanel->getVisible());
	}

	if (mTyping)
	{
		// Time out if user hasn't typed for a while.
		if (mLastKeystrokeTimer.getElapsedTimeF32() > LLAgent::TYPING_TIMEOUT_SECS)
		{
			setTyping(FALSE);
		}

		// If we are typing, and it's been a little while, send the
		// typing indicator
		if (!mSentTypingState && mFirstKeystrokeTimer.getElapsedTimeF32() > 1.f)
		{
			sendTypingState(TRUE);
			mSentTypingState = TRUE;
		}
	}

	// use embedded panel if available
	if (mSpeakerPanel)
	{
		if (mSpeakerPanel->getVisible())
		{
			mSpeakerPanel->refreshSpeakers();
		}
	}
	else if (mMuteButton && LLVoiceClient::instanceExists())
	{
		// refresh volume and mute
		mSpeakerVolumeSlider->setVisible(LLVoiceClient::voiceEnabled() &&
										 mVoiceChannel->isActive());
		mSpeakerVolumeSlider->setValue(LLVoiceClient::getInstance()->getUserVolume(mOtherParticipantUUID));

		LLMuteList* ml = LLMuteList::getInstance();
		mMuteButton->setValue(ml && ml->isMuted(mOtherParticipantUUID,
												LLMute::flagVoiceChat));
		mMuteButton->setVisible(LLVoiceClient::voiceEnabled() &&
								mVoiceChannel->isActive());
	}
	LLFloater::draw();
}

class LLSessionInviteResponder : public LLHTTPClient::Responder
{
public:
	LLSessionInviteResponder(const LLUUID& session_id)
	{
		mSessionID = session_id;
	}

	void error(U32 statusNum, const std::string& reason)
	{
		llinfos << "Error inviting all agents to session" << llendl;
		//throw something back to the viewer here?
	}

private:
	LLUUID mSessionID;
};

BOOL LLFloaterIMPanel::inviteToSession(const LLDynamicArray<LLUUID>& ids)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
	{
		return FALSE;
	}

	S32 count = ids.count();

	if (isInviteAllowed() && count > 0)
	{
		llinfos << "LLFloaterIMPanel::inviteToSession() - inviting participants" << llendl;

		std::string url = region->getCapability("ChatSessionRequest");

		LLSD data;
		data["params"] = LLSD::emptyArray();
		for (int i = 0; i < count; i++)
		{
			data["params"].append(ids.get(i));
		}
		data["method"] = "invite";
		data["session-id"] = mSessionUUID;
		LLHTTPClient::post(url, data, new LLSessionInviteResponder(mSessionUUID));
	}
	else
	{
		llinfos << "LLFloaterIMPanel::inviteToSession - no need to invite agents for "
				<< mDialog << llendl;
		// successful add, because everyone that needed to get added was added.
	}

	return TRUE;
}

void LLFloaterIMPanel::addHistoryLine(const std::string &utf8msg,
									  const LLColor4& color,
									  bool log_to_file,
									  const LLUUID& source,
									  const std::string& const_name)
{
	std::string name = const_name;
	// start tab flashing when receiving im for background session from user
	if (source != LLUUID::null)
	{
		LLMultiFloater* hostp = getHost();
		if (!isInVisibleChain() && hostp && source != gAgentID)
		{
			hostp->setFloaterFlashing(this, TRUE);
		}
	}

	// Now we're adding the actual line of text, so erase the
	// "Foo is typing..." text segment, and the optional timestamp
	// if it was present. JC
	removeTypingIndicator(NULL);

	// Actually add the line
	std::string timestring;
	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("IMShowTimestamps"))
	{
		timestring = mHistoryEditor->appendTime(prepend_newline);
		prepend_newline = false;
	}

	// 'name' is a sender name that we want to hotlink so that clicking on it opens a profile.
	if (!name.empty()) // If name exists, then add it to the front of the message.
	{
		// Don't hotlink any messages from the system (e.g. "Second Life:"), so just add those in plain text.
		if (name == SYSTEM_FROM)
		{
			mHistoryEditor->appendColoredText(name,false,prepend_newline,color);
		}
		else
		{
			LLUUID av_id = source;
			if (av_id == LLUUID::null)
			{
				std::string self_name;
				gAgent.buildFullname(self_name);
				if (name == self_name)
				{
					av_id = gAgentID;
				}
			}
			else if (LLAvatarNameCache::useDisplayNames())
			{
				LLAvatarName avatar_name;
				if (LLAvatarNameCache::get(av_id, &avatar_name))
				{
					if (LLAvatarNameCache::useDisplayNames() == 2)
					{
						name = avatar_name.mDisplayName;
					}
					else
					{
						name = avatar_name.getNames();
					}
				}
			}
			// Convert the name to a hotlink and add to message.
			const LLStyleSP &source_style = LLStyleMap::instance().lookupAgent(source);
			mHistoryEditor->appendStyledText(name,false,prepend_newline,source_style);
		}
		prepend_newline = false;
	}
	mHistoryEditor->appendColoredText(utf8msg, false, prepend_newline, color);

	if (log_to_file && gSavedPerAccountSettings.getBOOL("LogInstantMessages"))
	{
		std::string histstr = "";
		if (gSavedPerAccountSettings.getBOOL("IMLogTimestamp"))
		{
			histstr = LLLogChat::timestamp(gSavedPerAccountSettings.getBOOL("LogTimestampDate"));
		}
		histstr += name + utf8msg;

		LLLogChat::saveHistory(mSessionLog, histstr);
	}

	if (!isInVisibleChain())
	{
		mNumUnreadMessages++;
	}

	if (source != LLUUID::null)
	{
		mSpeakers->speakerChatted(source);
		mSpeakers->setSpeakerTyping(source, FALSE);
	}
}

void LLFloaterIMPanel::setVisible(BOOL b)
{
	LLPanel::setVisible(b);

	LLMultiFloater* hostp = getHost();
	if (b && hostp)
	{
		hostp->setFloaterFlashing(this, FALSE);

		/* Don't change containing floater title - leave it "Instant Message" JC
		LLUIString title = sTitleString;
		title.setArg("[NAME]", mSessionLabel);
		hostp->setTitle( title );
		*/
	}
}

void LLFloaterIMPanel::setInputFocus(BOOL b)
{
	mInputEditor->setFocus(b);
}

void LLFloaterIMPanel::selectAll()
{
	mInputEditor->selectAll();
}

void LLFloaterIMPanel::selectNone()
{
	mInputEditor->deselect();
}

BOOL LLFloaterIMPanel::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;
	if (KEY_RETURN == key)
	{
		if (mask == MASK_NONE || mask == MASK_CONTROL || mask == MASK_SHIFT)
		{
			sendMsg();
			handled = TRUE;

			// Close talk panels on hitting return but not shift-return or
			// control-return
			if (gIMMgr && !gSavedSettings.getBOOL("PinTalkViewOpen") &&
				!(mask & MASK_CONTROL) && !(mask & MASK_SHIFT))
			{
				gIMMgr->toggle(NULL);
			}
		}
		else if (mask == (MASK_SHIFT | MASK_CONTROL))
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
	else if (KEY_ESCAPE == key && mask == MASK_NONE)
	{
		handled = TRUE;
		gFocusMgr.setKeyboardFocus(NULL);

		// Close talk panel with escape
		if (gIMMgr && !gSavedSettings.getBOOL("PinTalkViewOpen"))
		{
			gIMMgr->toggle(NULL);
		}
	}

	// May need to call base class LLPanel::handleKeyHere if not handled
	// in order to tab between buttons.  JNC 1.2.2002
	return handled;
}

BOOL LLFloaterIMPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg)
{

	if (mDialog == IM_NOTHING_SPECIAL)
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mOtherParticipantUUID,
												 mSessionUUID, drop, cargo_type,
												 cargo_data, accept);
	}

	// Handle case for dropping calling cards (and folders of calling cards)
	// onto invitation panel for invites
	else if (isInviteAllowed())
	{
		*accept = ACCEPT_NO;

		if (cargo_type == DAD_CALLINGCARD)
		{
			if (dropCallingCard((LLInventoryItem*)cargo_data, drop))
			{
				*accept = ACCEPT_YES_MULTI;
			}
		}
		else if (cargo_type == DAD_CATEGORY)
		{
			if (dropCategory((LLInventoryCategory*)cargo_data, drop))
			{
				*accept = ACCEPT_YES_MULTI;
			}
		}
	}
	return TRUE;
}

BOOL LLFloaterIMPanel::dropCallingCard(LLInventoryItem* item, BOOL drop)
{
	BOOL rv = isInviteAllowed();
	if (rv && item && item->getCreatorUUID().notNull())
	{
		if (drop)
		{
			LLDynamicArray<LLUUID> ids;
			ids.put(item->getCreatorUUID());
			inviteToSession(ids);
		}
	}
	else
	{
		// set to false if creator uuid is null.
		rv = FALSE;
	}
	return rv;
}

BOOL LLFloaterIMPanel::dropCategory(LLInventoryCategory* category, BOOL drop)
{
	BOOL rv = isInviteAllowed();
	if (rv && category)
	{
		LLInventoryModel::cat_array_t cats;
		LLInventoryModel::item_array_t items;
		LLUniqueBuddyCollector buddies;
		gInventory.collectDescendentsIf(category->getUUID(), cats, items,
										LLInventoryModel::EXCLUDE_TRASH,
										buddies);
		S32 count = items.count();
		if (count == 0)
		{
			rv = FALSE;
		}
		else if (drop)
		{
			LLDynamicArray<LLUUID> ids;
			for (S32 i = 0; i < count; ++i)
			{
				ids.put(items.get(i)->getCreatorUUID());
			}
			inviteToSession(ids);
		}
	}
	return rv;
}

BOOL LLFloaterIMPanel::isInviteAllowed() const
{
	return (IM_SESSION_CONFERENCE_START == mDialog ||
			IM_SESSION_INVITE == mDialog);
}

// static
void LLFloaterIMPanel::onTabClick(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->setInputFocus(TRUE);
}

// static
void LLFloaterIMPanel::onClickProfile(void* userdata)
{
	//  Bring up the Profile window
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	if (self && self->mOtherParticipantUUID.notNull())
	{
		LLFloaterAvatarInfo::showFromDirectory(self->getOtherParticipantID());
	}
}

// static
void LLFloaterIMPanel::onClickGroupInfo(void* userdata)
{
	//  Bring up the Profile window
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	LLFloaterGroupInfo::showFromUUID(self->mSessionUUID);
}

// static
void LLFloaterIMPanel::onClickClose(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	if (self)
	{
		self->close();
	}
}

// static
void LLFloaterIMPanel::onClickStartCall(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->mVoiceChannel->activate();
}

// static
void LLFloaterIMPanel::onClickEndCall(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->getVoiceChannel()->deactivate();
}

// static
void LLFloaterIMPanel::onClickSend(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	self->sendMsg();
}

// static
void LLFloaterIMPanel::onClickOpenTextEditor(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	if (self && self->mInputEditor && !self->mSessionLabel.empty())
	{
		self->mHasScrolledOnce = true;
		HBFloaterTextInput::show(self->mInputEditor, self->mSessionLabel,
								 &setIMTyping, self);
	}
}

// static
void LLFloaterIMPanel::onClickToggleActiveSpeakers(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	if (self && self->mSpeakerPanel)
	{
		self->mSpeakerPanel->setVisible(!self->mSpeakerPanel->getVisible());
	}
}

// static
void LLFloaterIMPanel::onCommitChat(LLUICtrl* caller, void* userdata)
{
	LLFloaterIMPanel* self= (LLFloaterIMPanel*) userdata;
	self->sendMsg();
}

// static
void LLFloaterIMPanel::onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata )
{
	LLFloaterIMPanel* self= (LLFloaterIMPanel*) userdata;
	self->mHistoryEditor->setCursorAndScrollToEnd();
}

// static
void LLFloaterIMPanel::onInputEditorFocusLost(LLFocusableElement* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->setTyping(FALSE);
}

// static
void LLFloaterIMPanel::onInputEditorKeystroke(LLLineEditor* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	std::string text = self->mInputEditor->getText();
	if (!text.empty())
	{
		self->setTyping(TRUE);
	}
	else
	{
		// Deleting all text counts as stopping typing.
		self->setTyping(FALSE);
	}
}

// static
void LLFloaterIMPanel::onInputEditorScrolled(LLLineEditor* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	if (!self || !caller) return;

	if (!self->mHasScrolledOnce &&  !self->mSessionLabel.empty() &&
		gSavedSettings.getBOOL("AutoOpenTextInput"))
	{
		self->mHasScrolledOnce = true;
		HBFloaterTextInput::show(caller, self->mSessionLabel,
								 &setIMTyping, self);
	}
}

void LLFloaterIMPanel::onClose(bool app_quitting)
{
	HBFloaterTextInput::abort(mInputEditor);

	setTyping(FALSE);

	if (mSessionUUID.notNull())
	{
		std::string name;
		gAgent.buildFullname(name);
		pack_instant_message(gMessageSystem,
							 gAgentID,
							 FALSE,
							 gAgent.getSessionID(),
							 mOtherParticipantUUID,
							 name,
							 LLStringUtil::null,
							 IM_ONLINE,
							 IM_SESSION_LEAVE,
							 mSessionUUID);
		gAgent.sendReliableMessage();
	}
	if (gIMMgr)
	{
		gIMMgr->removeSession(mSessionUUID);
	}

	destroy();
}

void LLFloaterIMPanel::onVisibilityChange(BOOL new_visibility)
{
	if (new_visibility)
	{
		mNumUnreadMessages = 0;
	}
}

void deliver_message(const std::string& utf8_text,
					 const LLUUID& im_session_id,
					 const LLUUID& other_participant_id,
					 EInstantMessage dialog)
{
	std::string name;
	bool sent = false;
	gAgent.buildFullname(name);

	const LLRelationship* info = NULL;
	info = LLAvatarTracker::instance().getBuddyInfo(other_participant_id);

	U8 offline = (!info || info->isOnline()) ? IM_ONLINE : IM_OFFLINE;

	if (offline == IM_OFFLINE && LLVoiceClient::instanceExists() &&
		LLVoiceClient::getInstance()->isOnlineSIP(other_participant_id))
	{
		// User is online through the OOW connector, but not with a regular
		// viewer. Try to send the message via SLVoice.
		sent = LLVoiceClient::getInstance()->sendTextMessage(other_participant_id,
															 utf8_text);
	}

	if (!sent)
	{
		// Send message normally.

		// default to IM_SESSION_SEND unless it's nothing special - in
		// which case it's probably an IM to everyone.
		U8 new_dialog = dialog;

		if (dialog != IM_NOTHING_SPECIAL)
		{
			new_dialog = IM_SESSION_SEND;
		}
		pack_instant_message(gMessageSystem,
							 gAgentID,
							 FALSE,
							 gAgent.getSessionID(),
							 other_participant_id,
							 name.c_str(),
							 utf8_text.c_str(),
							 offline,
							 (EInstantMessage)new_dialog,
							 im_session_id);
		gAgent.sendReliableMessage();
	}

	// If there is a mute list and this is not a group chat...
	LLMuteList* ml = LLMuteList::getInstance();
	if (ml)
	{
		// ... the target should not be in our mute list for some message types.
		// Auto-remove them if present.
		switch (dialog)
		{
			case IM_NOTHING_SPECIAL:
			case IM_GROUP_INVITATION:
			case IM_INVENTORY_OFFERED:
//			case IM_SESSION_INVITE:	// enabling this makes it impossible to mute permanently a resident who initiated a group IM session (posting in the group chat would unmute them)
			case IM_SESSION_P2P_INVITE:
			case IM_SESSION_CONFERENCE_START:
			case IM_SESSION_SEND: // This one is marginal - erring on the side of hearing.
			case IM_LURE_USER:
			case IM_GODLIKE_LURE_USER:
			case IM_FRIENDSHIP_OFFERED:
				ml->autoRemove(other_participant_id, LLMuteList::AR_IM);
				break;
			default: ; // do nothing
		}
	}
}

void LLFloaterIMPanel::sendMsg()
{
	if (!gAgent.isGodlike() && mDialog == IM_NOTHING_SPECIAL &&
		mOtherParticipantUUID.isNull())
	{
		llinfos << "Cannot send IM to everyone unless you're a god." << llendl;
		return;
	}

	if (mInputEditor)
	{
		LLWString text = mInputEditor->getConvertedText();
//MK
		if (gRRenabled &&
			(gAgent.mRRInterface.containsWithoutException("sendim",
														  mOtherParticipantUUID.asString()) ||
			 gAgent.mRRInterface.contains("sendimto:" + mOtherParticipantUUID.asString())))
		{
			// user is forbidden to send IMs and the receiver is no exception
			// signal both the sender and the receiver
			text = utf8str_to_wstring(RRInterface::sSendimMessage);
		}
//mk
		if (!text.empty())
		{
			// store sent line in history, duplicates will get filtered
			if (mInputEditor) mInputEditor->updateHistory();

			// Convert to UTF8 for transport
			std::string utf8_text = wstring_to_utf8str(text);

			if (gSavedSettings.getBOOL("AutoCloseOOC"))
			{
				// Try to find any unclosed OOC chat (i.e. an opening
				// double parenthesis without a matching closing double
				// parenthesis.
				if (utf8_text.find("((") != -1 && utf8_text.find("))") == -1)
				{
					if (utf8_text.at(utf8_text.length() - 1) == ')')
					{
						// cosmetic: add a space first to avoid a closing triple parenthesis
						utf8_text += " ";
					}
					// add the missing closing double parenthesis.
					utf8_text += "))";
				}
			}

			// Convert MU*s style poses into IRC emotes here.
			if (gSavedSettings.getBOOL("AllowMUpose") && utf8_text.find(":") == 0 && utf8_text.length() > 3)
			{
				if (utf8_text.find(":'") == 0)
				{
					utf8_text.replace(0, 1, "/me");
				}
				else if (isalpha(utf8_text.at(1)))	// Do not prevent smileys and such.
				{
					utf8_text.replace(0, 1, "/me ");
				}
			}

			// Truncate
			utf8_text = utf8str_truncate(utf8_text, MAX_MSG_BUF_SIZE - 1);

			if (mSessionInitialized)
			{
				deliver_message(utf8_text, mSessionUUID, mOtherParticipantUUID,
								mDialog);

				// local echo
				if (mDialog == IM_NOTHING_SPECIAL &&
					mOtherParticipantUUID.notNull())
				{
					std::string history_echo;
					gAgent.buildFullname(history_echo);
					if (LLAvatarNameCache::useDisplayNames())
					{
						LLAvatarName avatar_name;
						if (LLAvatarNameCache::get(gAgentID, &avatar_name))
						{
							if (LLAvatarNameCache::useDisplayNames() == 2)
							{
								history_echo = avatar_name.mDisplayName;
							}
							else
							{
								history_echo = avatar_name.getNames();
							}
						}
					}

					// Look for IRC-style emotes here.
					std::string prefix = utf8_text.substr(0, 4);
					if (prefix == "/me " || prefix == "/me'")
					{
						utf8_text.replace(0, 3, "");
					}
					else
					{
						history_echo += ": ";
					}
					history_echo += utf8_text;

					BOOL other_was_typing = mOtherTyping;

					addHistoryLine(history_echo,
								   gSavedSettings.getColor("IMChatColor"),
								   true, gAgentID);

					if (other_was_typing)
					{
						addTypingIndicator(mOtherTypingName);
					}
				}
			}
			else
			{
				// Queue up the message to send once the session is initialized
				mQueuedMsgsForInit.append(utf8_text);
			}
		}

		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_IM_COUNT);

		mInputEditor->setText(LLStringUtil::null);
	}

	// Don't need to actually send the typing stop message, the other
	// client will infer it from receiving the message.
	mTyping = FALSE;
	mSentTypingState = TRUE;
}

void LLFloaterIMPanel::updateSpeakersList(const LLSD& speaker_updates)
{
	mSpeakers->updateSpeakers(speaker_updates);
}

void LLFloaterIMPanel::processSessionUpdate(const LLSD& session_update)
{
	if (session_update.has("moderated_mode") &&
		session_update["moderated_mode"].has("voice"))
	{
		BOOL voice_moderated = session_update["moderated_mode"]["voice"];

		if (voice_moderated)
		{
			setTitle(mSessionLabel + " " + getString("moderated_chat_label"));
		}
		else
		{
			setTitle(mSessionLabel);
		}

		// Update the speakers dropdown too
		mSpeakerPanel->setVoiceModerationCtrlMode(voice_moderated);
	}
}

void LLFloaterIMPanel::setSpeakers(const LLSD& speaker_list)
{
	mSpeakers->setSpeakers(speaker_list);
}

void LLFloaterIMPanel::sessionInitReplyReceived(const LLUUID& session_id)
{
	mSessionUUID = session_id;
	mVoiceChannel->updateSessionID(session_id);
	mSessionInitialized = TRUE;

	//we assume the history editor hasn't moved at all since
	//we added the starting session message
	//so, we count how many characters to remove
	S32 chars_to_remove = mHistoryEditor->getWText().length() - mSessionStartMsgPos;
	mHistoryEditor->removeTextFromEnd(chars_to_remove);

	//and now, send the queued msg
	LLSD::array_iterator iter;
	for (iter = mQueuedMsgsForInit.beginArray();
		 iter != mQueuedMsgsForInit.endArray(); ++iter)
	{
		deliver_message(iter->asString(), mSessionUUID, mOtherParticipantUUID,
						mDialog);
	}
}

void LLFloaterIMPanel::requestAutoConnect()
{
	mAutoConnect = TRUE;
}

void LLFloaterIMPanel::setTyping(BOOL typing)
{
	if (typing)
	{
		// Every time you type something, reset this timer
		mLastKeystrokeTimer.reset();

		if (!mTyping)
		{
			// You just started typing.
			mFirstKeystrokeTimer.reset();

			// Will send typing state after a short delay.
			mSentTypingState = FALSE;
		}

		mSpeakers->setSpeakerTyping(gAgentID, TRUE);
	}
	else
	{
		if (mTyping)
		{
			// you just stopped typing, send state immediately
			sendTypingState(FALSE);
			mSentTypingState = TRUE;
		}
		mSpeakers->setSpeakerTyping(gAgentID, FALSE);
	}

	mTyping = typing;
}

void LLFloaterIMPanel::sendTypingState(BOOL typing)
{
	// Don't want to send typing indicators to multiple people, potentially too
	// much network traffic. Only send in person-to-person IMs.
	if (mDialog != IM_NOTHING_SPECIAL) return;

	std::string name;
	gAgent.buildFullname(name);

	pack_instant_message(gMessageSystem,
						 gAgentID,
						 FALSE,
						 gAgent.getSessionID(),
						 mOtherParticipantUUID,
						 name,
						 std::string("typing"),
						 IM_ONLINE,
						 (typing ? IM_TYPING_START : IM_TYPING_STOP),
						 mSessionUUID);
	gAgent.sendReliableMessage();
}

void LLFloaterIMPanel::processIMTyping(const LLIMInfo* im_info, BOOL typing)
{
	if (typing)
	{
		// other user started typing
		addTypingIndicator(im_info->mName);
	}
	else
	{
		// other user stopped typing
		removeTypingIndicator(im_info);
	}
}

void LLFloaterIMPanel::addTypingIndicator(const std::string &name)
{
	// we may have lost a "stop-typing" packet, don't add it twice
	if (!mOtherTyping)
	{
		mTypingLineStartIndex = mHistoryEditor->getWText().length();
		LLUIString typing_start = sTypingStartString;
		typing_start.setArg("[NAME]", name);
		addHistoryLine(typing_start,
					   gSavedSettings.getColor4("SystemChatColor"), false);
		mOtherTypingName = name;
		mOtherTyping = TRUE;
	}
	// MBW -- XXX -- merge from release broke this (argument to this function
	// changed from an LLIMInfo to a name). Richard will fix.
//	mSpeakers->setSpeakerTyping(im_info->mFromID, TRUE);
}

void LLFloaterIMPanel::removeTypingIndicator(const LLIMInfo* im_info)
{
	if (mOtherTyping)
	{
		// Must do this first, otherwise addHistoryLine calls us again.
		mOtherTyping = FALSE;

		S32 chars_to_remove = mHistoryEditor->getWText().length() - mTypingLineStartIndex;
		mHistoryEditor->removeTextFromEnd(chars_to_remove);
		if (im_info)
		{
			mSpeakers->setSpeakerTyping(im_info->mFromID, FALSE);
		}
	}
}

//static
void LLFloaterIMPanel::setIMTyping(void* caller, BOOL typing)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)caller;
	if (self)
	{
		self->setTyping(typing);
	}
}

//static
void LLFloaterIMPanel::chatFromLogFile(LLLogChat::ELogLineType type, std::string line, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	std::string message = line;

	switch (type)
	{
	case LLLogChat::LOG_EMPTY:
		// add warning log enabled message
		if (gSavedPerAccountSettings.getBOOL("LogInstantMessages"))
		{
			message = LLFloaterChat::getInstance()->getString("IM_logging_string");
		}
		break;
	case LLLogChat::LOG_END:
		// add log end message
		if (gSavedPerAccountSettings.getBOOL("LogInstantMessages"))
		{
			message = LLFloaterChat::getInstance()->getString("IM_logging_string");
		}
		break;
	case LLLogChat::LOG_LINE:
		// just add normal lines from file
		break;
	default:
		// nothing
		break;
	}

	//self->addHistoryLine(line, LLColor4::grey, FALSE);
	self->mHistoryEditor->appendColoredText(message, false, true, LLColor4::grey);
}

void LLFloaterIMPanel::showSessionStartError(const std::string& error_string)
{
	//the error strings etc. should be really be static and local
	//to this file instead of in the LLFloaterIM
	//but they were in llimview.cpp first and unfortunately
	//some translations into non English languages already occurred
	//thus making it a tad harder to change over to a
	//"correct" solution.  The best solution
	//would be to store all of the misc. strings into
	//their own XML file which would be read in by any LLIMPanel
	//post build function instead of repeating the same info
	//in the group, adhoc and normal IM xml files.
	LLSD args;
	args["REASON"] = LLFloaterIM::sErrorStringsMap[error_string];
	args["RECIPIENT"] = getTitle();

	LLSD payload;
	payload["session_id"] = mSessionUUID;

	LLNotifications::instance().add("ChatterBoxSessionStartError", args,
									payload, onConfirmForceCloseError);
}

void LLFloaterIMPanel::showSessionEventError(const std::string& event_string,
											 const std::string& error_string)
{
	LLSD args;
	args["REASON"] = LLFloaterIM::sErrorStringsMap[error_string];
	args["EVENT"] = LLFloaterIM::sEventStringsMap[event_string];
	args["RECIPIENT"] = getTitle();

	LLNotifications::instance().add("ChatterBoxSessionEventError", args);
}

void LLFloaterIMPanel::showSessionForceClose(const std::string& reason_string)
{
	LLSD args;

	args["NAME"] = getTitle();
	args["REASON"] = LLFloaterIM::sForceCloseSessionMap[reason_string];

	LLSD payload;
	payload["session_id"] = mSessionUUID;

	LLNotifications::instance().add("ForceCloseChatterBoxSession", args,
									payload,
									LLFloaterIMPanel::onConfirmForceCloseError);
}

bool LLFloaterIMPanel::onConfirmForceCloseError(const LLSD& notification, const LLSD& response)
{
	//only 1 option really
	LLUUID session_id = notification["payload"]["session_id"];

	if (gIMMgr)
	{
		LLFloaterIMPanel* floaterp = gIMMgr->findFloaterBySession(session_id);
		if (floaterp)
		{
			floaterp->close(FALSE);
		}
	}
	return false;
}
