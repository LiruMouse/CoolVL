/** 
 * @file llmarketplacefunctions.cpp
 * @brief Implementation of assorted functions related to the marketplace
 *
 * $LicenseInfo:firstyear=2012&license=viewergpl$
 * 
 * Copyright (c) 2012, Linden Research, Inc.
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

#include "llmarketplacefunctions.h"

#include "llhttpclient.h"
#include "llnotifications.h"
#include "lltimer.h"

#include "llagent.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"
#include "llweb.h"

//
// Helpers
//

namespace LLMarketplaceImport
{
	// Basic interface for this namespace

	bool hasSessionCookie();
	bool inProgress();
	bool resultPending();
	U32 getResultStatus();
	const LLSD& getResults();

	bool establishMarketplaceSessionCookie();
	bool pollStatus();
	bool triggerImport();

	// Internal state variables

	static std::string sMarketplaceCookie = "";
	static LLSD sImportId = LLSD::emptyMap();
	static bool sImportInProgress = false;
	static bool sImportPostPending = false;
	static bool sImportGetPending = false;
	static U32 sImportResultStatus = 0;
	static LLSD sImportResults = LLSD::emptyMap();

	static LLTimer slmGetTimer;
	static LLTimer slmPostTimer;

	// Responders

	class LLImportPostResponder : public LLHTTPClient::Responder
	{
	public:
		LLImportPostResponder() : LLCurl::Responder() {}

		void completed(U32 status, const std::string& reason, const LLSD& content)
		{
			slmPostTimer.stop();

			LL_DEBUGS("Outbox") << "\nSLM POST status: " << status
								<< "\nSLM POST reason: " << reason
								<< "\nSLM POST content: " << content.asString()
								<< "\nSLM POST timer: " << slmPostTimer.getElapsedTimeF32()
								<< LL_ENDL;

			if (status == MarketplaceErrorCodes::IMPORT_REDIRECT ||
				status == MarketplaceErrorCodes::IMPORT_JOB_TIMEOUT ||
				status == MarketplaceErrorCodes::IMPORT_AUTHENTICATION_ERROR)
			{
				LL_DEBUGS("Outbox") << "SLM POST clearing marketplace cookie due to authentication failure or timeout"
									<< LL_ENDL;

				sMarketplaceCookie.clear();
			}

			sImportInProgress = (status == MarketplaceErrorCodes::IMPORT_DONE);
			sImportPostPending = false;
			sImportResultStatus = status;
			sImportId = content;
		}
	};

	class LLImportGetResponder : public LLHTTPClient::Responder
	{
	public:
		LLImportGetResponder() : LLCurl::Responder() {}

		void completedHeader(U32 status, const std::string& reason, const LLSD& content)
		{
			const std::string& set_cookie_string = content["set-cookie"].asString();

			if (!set_cookie_string.empty())
			{
				LL_DEBUGS("Outbox") << "Marketplace cookie set." << LL_ENDL;
				sMarketplaceCookie = set_cookie_string;
			}
		}

		void completed(U32 status, const std::string& reason, const LLSD& content)
		{
			slmGetTimer.stop();

			LL_DEBUGS("Outbox") << "\nSLM GET status: " << status
								<< "\nSLM GET reason: " << reason
								<< "\nSLM GET content: " << content.asString()
								<< "\nSLM GET timer: " << slmGetTimer.getElapsedTimeF32()
								<< LL_ENDL;

			if (status == MarketplaceErrorCodes::IMPORT_JOB_TIMEOUT ||
				status == MarketplaceErrorCodes::IMPORT_AUTHENTICATION_ERROR)
			{
				LL_DEBUGS("Outbox") << "SLM GET clearing marketplace cookie due to authentication failure or timeout"
									<< LL_ENDL;

				sMarketplaceCookie.clear();
			}

			sImportInProgress = (status == MarketplaceErrorCodes::IMPORT_PROCESSING);
			sImportGetPending = false;
			sImportResultStatus = status;
			sImportResults = content;
		}
	};

	// Basic API

	bool hasSessionCookie()
	{
		return !sMarketplaceCookie.empty();
	}

	bool inProgress()
	{
		return sImportInProgress;
	}

	bool resultPending()
	{
		return (sImportPostPending || sImportGetPending);
	}

	U32 getResultStatus()
	{
		return sImportResultStatus;
	}

	const LLSD& getResults()
	{
		return sImportResults;
	}

	static std::string getInventoryImportURL()
	{
		std::string url = gSavedSettings.getString("MarketplaceURL");
		LLStringUtil::format_map_t subs;
		subs["[AGENT_ID]"] = gAgentID.getString();
		url = LLWeb::expandURLSubstitutions(url, subs);
		return url;
	}

	bool establishMarketplaceSessionCookie()
	{
		if (hasSessionCookie())
		{
			return false;
		}

		sImportInProgress = true;
		sImportGetPending = true;

		std::string url = getInventoryImportURL();

		LL_DEBUGS("Outbox") << "Fetching Marketplace cookie.\nSLM GET: "
							<< url << LL_ENDL;

		slmGetTimer.start();
		LLHTTPClient::get(url, new LLImportGetResponder(),
						  LLViewerMedia::getHeaders());

		return true;
	}

	bool pollStatus()
	{
		if (!hasSessionCookie())
		{
			return false;
		}

		sImportGetPending = true;

		std::string url = getInventoryImportURL();

		url += sImportId.asString();

		// Make the headers for the post
		LLSD headers = LLSD::emptyMap();
		headers["Accept"] = "*/*";
		headers["Cookie"] = sMarketplaceCookie;
		headers["Content-Type"] = "application/llsd+xml";
		headers["User-Agent"] = LLViewerMedia::getCurrentUserAgent();

		LL_DEBUGS("Outbox") << "SLM GET: " << url << LL_ENDL;

		slmGetTimer.start();
		LLHTTPClient::get(url, new LLImportGetResponder(), headers);

		return true;
	}

	bool triggerImport()
	{
		if (!hasSessionCookie())
		{
			return false;
		}

		sImportId = LLSD::emptyMap();
		sImportInProgress = true;
		sImportPostPending = true;
		sImportResultStatus = MarketplaceErrorCodes::IMPORT_PROCESSING;
		sImportResults = LLSD::emptyMap();

		std::string url = getInventoryImportURL();

		// Make the headers for the post
		LLSD headers = LLSD::emptyMap();
		headers["Accept"] = "*/*";
		headers["Connection"] = "Keep-Alive";
		headers["Cookie"] = sMarketplaceCookie;
		headers["Content-Type"] = "application/xml";
		headers["User-Agent"] = LLViewerMedia::getCurrentUserAgent();

		LL_DEBUGS("Outbox") << "SLM POST: " << url << LL_ENDL;

		slmPostTimer.start();
		LLHTTPClient::post(url, LLSD(), new LLImportPostResponder(), headers);

		return true;
	}
}

//
// Interface class
//

//static
void LLMarketplaceInventoryImporter::update()
{
	if (instanceExists())
	{
		LLMarketplaceInventoryImporter::instance().updateImport();
	}
}

LLMarketplaceInventoryImporter::LLMarketplaceInventoryImporter()
:	mAutoTriggerImport(false),
	mImportInProgress(false),
	mInitialized(false)
{
}

void LLMarketplaceInventoryImporter::initialize()
{
	llassert(!mInitialized);

	if (!LLMarketplaceImport::hasSessionCookie())
	{
		LLMarketplaceImport::establishMarketplaceSessionCookie();
	}
}

void LLMarketplaceInventoryImporter::reinitializeAndTriggerImport()
{
	mInitialized = false;
	initialize();
	mAutoTriggerImport = true;
}

bool LLMarketplaceInventoryImporter::triggerImport()
{
	const bool import_triggered = LLMarketplaceImport::triggerImport();

	if (!import_triggered)
	{
		reinitializeAndTriggerImport();
	}

	return import_triggered;
}

void LLMarketplaceInventoryImporter::updateImport()
{
	const bool in_progress = LLMarketplaceImport::inProgress();

	if (in_progress && !LLMarketplaceImport::resultPending())
	{
		const bool polling_status = LLMarketplaceImport::pollStatus();

		if (!polling_status)
		{
			reinitializeAndTriggerImport();
		}
	}

	if (mImportInProgress != in_progress)
	{
		mImportInProgress = in_progress;

		// If we are no longer in progress
		if (!mImportInProgress)
		{
			U32 status = LLMarketplaceImport::getResultStatus();

			if (mInitialized)
			{
				// Report results
				if (status == MarketplaceErrorCodes::IMPORT_DONE)
				{
					LLNotifications::instance().add("OutboxImportComplete");
				}
				else if (status == MarketplaceErrorCodes::IMPORT_DONE_WITH_ERRORS)
				{
					LLNotifications::instance().add("OutboxImportHadErrors");
				}
				else
				{
					LLNotifications::instance().add("OutboxImportFailed");
				}
			}
			else
			{
				// Look for results success
				mInitialized = LLMarketplaceImport::hasSessionCookie();

				if (mInitialized)
				{
					// Follow up with auto trigger of import
					if (mAutoTriggerImport)
					{
						mAutoTriggerImport = false;
						mImportInProgress = triggerImport();
					}
				}
				else if (status != MarketplaceErrorCodes::IMPORT_DONE)
				{
					LLNotifications::instance().add("OutboxInitFailed");
				}
			}
		}
	}
}

//
// Outbox related inventory functions
//

bool can_copy_to_outbox(LLInventoryItem* inv_item)
{
	// Collapse links directly to items/folders
	LLViewerInventoryItem* viitem = (LLViewerInventoryItem*) inv_item;
	LLViewerInventoryItem* linked_item = viitem->getLinkedItem();
	if (linked_item != NULL)
	{
		inv_item = linked_item;
	}

	return inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgentID) &&
		   inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID);

	return true;
}

LLUUID create_folder_in_outbox_for_item(LLInventoryItem* item,
										const LLUUID& destFolderId)
{
	llassert(item);
	llassert(destFolderId.notNull());

	LLUUID created_folder_id = gInventory.createNewCategory(destFolderId,
															LLFolderType::FT_NONE,
															item->getName());
	gInventory.notifyObservers();

	return created_folder_id;
}

void copy_item_to_outbox(LLInventoryItem* inv_item, LLUUID dest_folder,
						 const LLUUID& top_level_folder)
{
	// Collapse links directly to items/folders
	LLViewerInventoryItem* viitem = (LLViewerInventoryItem*) inv_item;
	LLViewerInventoryCategory* linked_category = viitem->getLinkedCategory();
	if (linked_category != NULL)
	{
		copy_folder_to_outbox(linked_category, dest_folder, top_level_folder);
	}
	else
	{
		LLViewerInventoryItem* linked_item = viitem->getLinkedItem();
		if (linked_item != NULL)
		{
			inv_item = (LLInventoryItem*) linked_item;
		}
		
		// Check for copy permissions
		if (inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgentID,
														gAgent.getGroupID()))
		{
			// when moving item directly into outbox create folder with that name
			if (dest_folder == gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX,
																  false))
			{
				dest_folder = create_folder_in_outbox_for_item(inv_item,
															   dest_folder);
			}
			
			copy_inventory_item(gAgentID,
								inv_item->getPermissions().getOwner(),
								inv_item->getUUID(),
								dest_folder,
								inv_item->getName(),
								LLPointer<LLInventoryCallback>(NULL));
		}
		else
		{
			LLNotifications::instance().add("OutboxNoCopyItem");
		}
	}
}

void copy_folder_to_outbox(LLInventoryCategory* inv_cat,
						   const LLUUID& dest_folder,
						   const LLUUID& top_level_folder)
{
	LLUUID new_folder_id = gInventory.createNewCategory(dest_folder,
														LLFolderType::FT_NONE,
														inv_cat->getName());
	gInventory.notifyObservers();

	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(inv_cat->getUUID(),cat_array,item_array);

	// copy the vector because otherwise the iterator won't be happy if we
	// delete from it
	LLInventoryModel::item_array_t item_array_copy = *item_array;

	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin();
		 iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
		copy_item_to_outbox(item, new_folder_id, top_level_folder);
	}

	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;

	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin();
		 iter != cat_array_copy.end(); iter++)
	{
		LLViewerInventoryCategory* category = *iter;
		copy_folder_to_outbox(category, new_folder_id, top_level_folder);
	}
}
