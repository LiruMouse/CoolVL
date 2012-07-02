/** 
 * @file llwearablelist.cpp
 * @brief LLWearableList class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llwearablelist.h"

#include "llassetstorage.h"
#include "llnotifications.h"
#include "message.h"

#include "llagent.h"
#include "lltrans.h"
#include "llviewerinventory.h"
#include "llviewerstats.h"
#include "llvoavatar.h"
#include "llwearabletype.h"

// Callback struct
struct LLWearableArrivedData
{
	LLWearableArrivedData(LLAssetType::EType asset_type,
						  const std::string& wearable_name,
						  void(*asset_arrived_callback)(LLWearable*, void* userdata),
						  void* userdata) 
	:	mAssetType(asset_type),
		mCallback(asset_arrived_callback), 
		mUserdata(userdata),
		mName(wearable_name),
		mRetries(0)
	{}

	LLAssetType::EType mAssetType;
	void (*mCallback)(LLWearable*, void* userdata);
	void* mUserdata;
	std::string mName;
	S32	mRetries;
};

////////////////////////////////////////////////////////////////////////////
// LLWearableList

LLWearableList::~LLWearableList()
{
	cleanup();
}

void LLWearableList::cleanup() 
{
	for_each(mList.begin(), mList.end(), DeletePairedPointer());
	mList.clear();
}

void LLWearableList::getAsset(const LLAssetID& assetID,
							  const std::string& wearable_name,
							  LLAssetType::EType asset_type,
							  void(*asset_arrived_callback)(LLWearable*, void* userdata),
							  void* userdata)
{
	llassert(asset_type == LLAssetType::AT_CLOTHING || asset_type == LLAssetType::AT_BODYPART);
	LLWearable* instance = get_if_there(mList, assetID, (LLWearable*)NULL);
	if (instance)
	{
		asset_arrived_callback(instance, userdata);
	}
	else
	{
		gAssetStorage->getAssetData(assetID, asset_type,
									LLWearableList::processGetAssetReply,
									(void*)new LLWearableArrivedData(asset_type,
																	 wearable_name,
																	 asset_arrived_callback,
																	 userdata),
									TRUE);
	}
}

// static
void LLWearableList::processGetAssetReply(const char* filename,
										  const LLAssetID& uuid,
										  void* userdata,
										  S32 status,
										  LLExtStat ext_status)
{
	BOOL isNewWearable = FALSE;
	LLWearableArrivedData* data = (LLWearableArrivedData*) userdata;
	LLWearable* wearable = NULL; // NULL indicates failure

	if (!filename)
	{
		llwarns << "Bad Wearable Asset: missing file." << llendl;
	}
	else if (status >= 0)
	{
		// read the file
		LLFILE* fp = LLFile::fopen(std::string(filename), "rb");		/*Flawfinder: ignore*/
		if (!fp)
		{
			llwarns << "Bad Wearable Asset: unable to open file: '" << filename
					<< "'" << llendl;
		}
		else
		{
			wearable = new LLWearable(uuid);
			bool res = wearable->importFile(fp);
			if (!res)
			{
				if (wearable->getType() == LLWearableType::WT_COUNT)
				{
					isNewWearable = TRUE;
				}
				delete wearable;
				wearable = NULL;
			}

			fclose(fp);
			if (filename)
			{
				LLFile::remove(std::string(filename));
			}
		}
	}
	else
	{
		if (filename)
		{
			LLFile::remove(std::string(filename));
		}
		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_DOWNLOAD_FAILED);

		llwarns << "Wearable download failed: "
				<< LLAssetStorage::getErrorString(status) << " " << uuid
				<< llendl;

		switch (status)
		{
			case LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE:
			{
				// Fail
				break;
			}
			default:
			{
				static const S32 MAX_RETRIES = 3;
				if (data->mRetries < MAX_RETRIES)
				{
					// Try again
					data->mRetries++;
					gAssetStorage->getAssetData(uuid, data->mAssetType,
												LLWearableList::processGetAssetReply,
												userdata);  // re-use instead of deleting.
					return;
				}
				else
				{
					// Fail
					break;
				}
			}
		}
	}

	if (wearable) // success
	{
		LLWearableList::instance().mList[uuid] = wearable;
		LL_DEBUGS("Wearables") << "Success getting wearable: " << uuid.asString()
							   << LL_ENDL;
	}
	else
	{
		LLSD args;
		args["TYPE"] = LLTrans::getString(LLAssetType::lookupHumanReadable(data->mAssetType));
		if (isNewWearable)
		{
			LLNotifications::instance().add("InvalidWearable");
		}
		else if (data->mName.empty())
		{
			LLNotifications::instance().add("FailedToFindWearableUnnamed", args);
		}
		else
		{
			args["DESC"] = data->mName;
			LLNotifications::instance().add("FailedToFindWearable", args);
		}
	}
	// Always call callback; wearable will be NULL if we failed
	{
		if (data->mCallback)
		{
			data->mCallback(wearable, data->mUserdata);
		}
	}
	delete data;
}

LLWearable* LLWearableList::createCopy(LLWearable* old_wearable,
									   const std::string& new_name)
{
	LL_DEBUGS("Wearables") << "LLWearableList::createCopy()" << LL_ENDL;

	LLWearable *wearable = generateNewWearable();
	wearable->copyDataFrom(old_wearable);

	LLPermissions perm(old_wearable->getPermissions());
	perm.setOwnerAndGroup(LLUUID::null, gAgent.getID(), LLUUID::null, true);
	wearable->setPermissions(perm);
	if (!new_name.empty()) wearable->setName(new_name);

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}

LLWearable* LLWearableList::createNewWearable(LLWearableType::EType type)
{
	LL_DEBUGS("Wearables") << "LLWearableList::createNewWearable()" << LL_ENDL;

	LLWearable *wearable = generateNewWearable();
	wearable->setType(type);

	std::string name = LLTrans::getString(LLWearableType::getTypeDefaultNewName(wearable->getType()));
	wearable->setName(name);

	LLPermissions perm;
	perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
	perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE, PERM_MOVE | PERM_TRANSFER);
	wearable->setPermissions(perm);

	// Description and sale info have default values.
	wearable->setParamsToDefaults();
	wearable->setTexturesToDefaults();

	//mark all values (params & images) as saved
	wearable->saveValues();

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}

LLWearable *LLWearableList::generateNewWearable()
{
	LLTransactionID tid;
	tid.generate();
	LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

	LLWearable* wearable = new LLWearable(tid);
	mList[new_asset_id] = wearable;
	return wearable;
}
