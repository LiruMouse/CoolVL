/** 
 * @file llmarketplacefunctions.h
 * @brief Miscellaneous marketplace-related functions and classes
 * class definition
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

#ifndef LL_LLMARKETPLACEFUNCTIONS_H
#define LL_LLMARKETPLACEFUNCTIONS_H

#include "llinventory.h"
#include "llsingleton.h"

namespace MarketplaceErrorCodes
{
	enum eCode
	{
		IMPORT_DONE = 200,
		IMPORT_PROCESSING = 202,
		IMPORT_REDIRECT = 302,
		IMPORT_AUTHENTICATION_ERROR = 401,
		IMPORT_DONE_WITH_ERRORS = 409,
		IMPORT_JOB_FAILED = 410,
		IMPORT_JOB_TIMEOUT = 499,
	};
}

class LLMarketplaceInventoryImporter
:	public LLSingleton<LLMarketplaceInventoryImporter>
{
public:
	static void update();

	LLMarketplaceInventoryImporter();

	void initialize();
	bool triggerImport();
	bool isImportInProgress() const		{ return mImportInProgress; }

protected:
	void reinitializeAndTriggerImport();
	void updateImport();

private:
	bool mAutoTriggerImport;
	bool mImportInProgress;
	bool mInitialized;
};

bool can_copy_to_outbox(LLInventoryItem* inv_item);

void copy_item_to_outbox(LLInventoryItem* inv_item,
						 LLUUID dest_folder,
						 const LLUUID& top_level_folder);

void copy_folder_to_outbox(LLInventoryCategory* inv_cat,
						   const LLUUID& dest_folder,
						   const LLUUID& top_level_folder);

#endif // LL_LLMARKETPLACEFUNCTIONS_H
