/** 
 * @file llappearancemgr.h
 * @brief Manager for initiating appearance changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#ifndef LL_LLAPPEARANCEMGR_H
#define LL_LLAPPEARANCEMGR_H

#include "llsingleton.h"

#include "llinventorymodel.h"
#include "llwearable.h"

class LLViewerInventoryItem;
class LLViewerJointAttachment;
class LLWearableHoldingPattern;

struct OnWearStruct
{
	OnWearStruct(const LLUUID& uuid, bool replace = true)
	:	mUUID(uuid), mReplace(replace) {}
	LLUUID mUUID;
	bool mReplace;
};

struct OnRemoveStruct
{
	OnRemoveStruct(const LLUUID& uuid) : mUUID(uuid) {}
	LLUUID mUUID;
};

class LLAppearanceMgr : public LLSingleton<LLAppearanceMgr>
{
	LOG_CLASS(LLAppearanceMgr);

	friend class LLSingleton<LLAppearanceMgr>;

protected:
	LLAppearanceMgr()	{}
	~LLAppearanceMgr()	{}

public:
	typedef std::vector<LLInventoryModel::item_array_t> wearables_by_type_t;

	void wearOutfitByName(const std::string& name);

	void wearInventoryItemOnAvatar(LLInventoryItem* item, bool replace = true);

	void wearInventoryCategory(LLInventoryCategory* category,
							   bool copy, bool append);

	void wearInventoryCategoryOnAvatar(LLInventoryCategory* category,
									   bool append, bool replace = false);

	void removeInventoryCategoryFromAvatar(LLInventoryCategory* category);

	bool wearItemOnAvatar(const LLUUID& item_id_to_wear, bool replace = true);

	void rezAttachment(LLViewerInventoryItem* item, 
					   LLViewerJointAttachment* attachment,
					   bool replace = false);

	void checkOutfit();	// check, save, restore outfit from outfit.xml

	static void sortItemsByActualDescription(LLInventoryModel::item_array_t& items);

	// Check ordering information on wearables stored in links' descriptions
	// and update if it is invalid
	void updateClothingOrderingInfo(LLUUID cat_id);

private:
	void getDescendentsOfAssetType(const LLUUID& category, 
								   LLInventoryModel::item_array_t& items,
								   LLAssetType::EType type);

	void getUserDescendents(const LLUUID& category, 
							LLInventoryModel::item_array_t& wear_items,
							LLInventoryModel::item_array_t& obj_items,
							LLInventoryModel::item_array_t& gest_items);

	static void wearInventoryCategoryOnAvatarStep2(BOOL proceed, void* data);
	static void wearInventoryCategoryOnAvatarStep3(LLWearableHoldingPattern* holder);
	static void onWearableAssetFetch(LLWearable* wearable, void* data);

	static void removeInventoryCategoryFromAvatarStep2(BOOL proceed, void* data);
};

std::string build_order_string(LLWearableType::EType type, U32 i);

#endif
