/** 
 * @file llwearabletype.cpp
 * @brief LLWearableType class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
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

#include "llviewerprecompiledheaders.h"
#include "llwearabletype.h"
#include "lltrans.h"

struct WearableEntry : public LLDictionaryEntry
{
	WearableEntry(const std::string &name,
				  const std::string& default_new_name,
				  LLAssetType::EType assetType,
				  LLInventoryIcon::EIconName iconName,
				  BOOL disable_camera_switch = FALSE,
				  BOOL allow_multiwear = TRUE)
	:	LLDictionaryEntry(name),
		mAssetType(assetType),
		mDefaultNewName(default_new_name),
		mLabel(LLTrans::getString(name)),
		mIconName(iconName),
		mDisableCameraSwitch(disable_camera_switch),
		mAllowMultiwear(allow_multiwear)
	{
	}
	const LLAssetType::EType mAssetType;
	const std::string mLabel;			//keep mLabel for backward compatibility
	const std::string mDefaultNewName;
	LLInventoryIcon::EIconName mIconName;
	BOOL mDisableCameraSwitch;
	BOOL mAllowMultiwear;
};

class LLWearableDictionary : public LLSingleton<LLWearableDictionary>,
							 public LLDictionary<LLWearableType::EType, WearableEntry>
{
public:
	LLWearableDictionary();
};

LLWearableDictionary::LLWearableDictionary()
{
	addEntry(LLWearableType::WT_SHAPE,        new WearableEntry("shape",       "New Shape",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_SHAPE, FALSE, FALSE));
	addEntry(LLWearableType::WT_SKIN,         new WearableEntry("skin",        "New Skin",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_SKIN, FALSE, FALSE));
	addEntry(LLWearableType::WT_HAIR,         new WearableEntry("hair",        "New Hair",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_HAIR, FALSE, FALSE));
	addEntry(LLWearableType::WT_EYES,         new WearableEntry("eyes",        "New Eyes",			LLAssetType::AT_BODYPART, 	LLInventoryIcon::ICONNAME_BODYPART_EYES, FALSE, FALSE));
	addEntry(LLWearableType::WT_SHIRT,        new WearableEntry("shirt",       "New Shirt",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SHIRT, FALSE, TRUE));
	addEntry(LLWearableType::WT_PANTS,        new WearableEntry("pants",       "New Pants",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_PANTS, FALSE, TRUE));
	addEntry(LLWearableType::WT_SHOES,        new WearableEntry("shoes",       "New Shoes",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SHOES, FALSE, TRUE));
	addEntry(LLWearableType::WT_SOCKS,        new WearableEntry("socks",       "New Socks",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SOCKS, FALSE, TRUE));
	addEntry(LLWearableType::WT_JACKET,       new WearableEntry("jacket",      "New Jacket",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_JACKET, FALSE, TRUE));
	addEntry(LLWearableType::WT_GLOVES,       new WearableEntry("gloves",      "New Gloves",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_GLOVES, FALSE, TRUE));
	addEntry(LLWearableType::WT_UNDERSHIRT,   new WearableEntry("undershirt",  "New Undershirt",	LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_UNDERSHIRT, FALSE, TRUE));
	addEntry(LLWearableType::WT_UNDERPANTS,   new WearableEntry("underpants",  "New Underpants",	LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_UNDERPANTS, FALSE, TRUE));
	addEntry(LLWearableType::WT_SKIRT,        new WearableEntry("skirt",       "New Skirt",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_SKIRT, FALSE, TRUE));
	addEntry(LLWearableType::WT_ALPHA,        new WearableEntry("alpha",       "New Alpha",			LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_ALPHA, FALSE, TRUE));
	addEntry(LLWearableType::WT_TATTOO,       new WearableEntry("tattoo",      "New Tattoo",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_TATTOO, FALSE, TRUE));
	addEntry(LLWearableType::WT_PHYSICS,      new WearableEntry("physics",     "New Physics",		LLAssetType::AT_CLOTHING, 	LLInventoryIcon::ICONNAME_CLOTHING_PHYSICS, TRUE, TRUE));

	addEntry(LLWearableType::WT_INVALID,      new WearableEntry("invalid",     "Invalid Wearable", 	LLAssetType::AT_NONE, 		LLInventoryIcon::ICONNAME_NONE, FALSE, FALSE));
	addEntry(LLWearableType::WT_NONE,      	  new WearableEntry("none",        "Invalid Wearable", 	LLAssetType::AT_NONE, 		LLInventoryIcon::ICONNAME_NONE, FALSE, FALSE));
}

// static
LLWearableType::EType LLWearableType::typeNameToType(const std::string& type_name)
{
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const LLWearableType::EType wearable = dict->lookup(type_name);
	return wearable;
}

// static
const std::string& LLWearableType::getTypeName(LLWearableType::EType type)
{ 
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const WearableEntry* entry = dict->lookup(type);
	if (!entry) return getTypeName(WT_INVALID);
	return entry->mName;
}

// static
std::string LLWearableType::getCapitalizedTypeName(LLWearableType::EType type)
{ 
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const WearableEntry* entry = dict->lookup(type);
	std::string name;
	if (entry)
	{
		name = entry->mName;
	}
	else
	{
		name = getTypeName(WT_INVALID);
	}
	name[0] = toupper(name[0]);
	return name;
}

//static
const std::string& LLWearableType::getTypeDefaultNewName(LLWearableType::EType type)
{ 
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const WearableEntry* entry = dict->lookup(type);
	if (!entry) return getTypeDefaultNewName(WT_INVALID);
	return entry->mDefaultNewName;
}

// static
const std::string& LLWearableType::getTypeLabel(LLWearableType::EType type)
{ 
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const WearableEntry* entry = dict->lookup(type);
	if (!entry) return getTypeLabel(WT_INVALID);
	return entry->mLabel;
}

// static
LLAssetType::EType LLWearableType::getAssetType(LLWearableType::EType type)
{
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const WearableEntry* entry = dict->lookup(type);
	if (!entry) return getAssetType(WT_INVALID);
	return entry->mAssetType;
}

// static
LLInventoryIcon::EIconName LLWearableType::getIconName(LLWearableType::EType type)
{
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const WearableEntry* entry = dict->lookup(type);
	if (!entry) return getIconName(WT_INVALID);
	return entry->mIconName;
} 

// static
BOOL LLWearableType::getDisableCameraSwitch(LLWearableType::EType type)
{
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const WearableEntry* entry = dict->lookup(type);
	if (!entry) return FALSE;
	return entry->mDisableCameraSwitch;
}

// static
BOOL LLWearableType::getAllowMultiwear(LLWearableType::EType type)
{
	const LLWearableDictionary* dict = LLWearableDictionary::getInstance();
	const WearableEntry* entry = dict->lookup(type);
	if (!entry) return FALSE;
	return entry->mAllowMultiwear;
}
