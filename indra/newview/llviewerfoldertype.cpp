/**
 * @file llviewerfoldertype.cpp
 * @brief Implementation of LLViewerFolderType functionality.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
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

#include "llviewerfoldertype.h"

#include "lldictionary.h"

static const std::string empty_string;

struct ViewerFolderEntry : public LLDictionaryEntry
{
	// Constructor for non-ensembles
	ViewerFolderEntry(const std::string& category_name,
					  const std::string& icon_name,
					  const LLUIImagePtr icon,
					  BOOL is_quiet,
					  bool hide_if_empty = false,
					  // No reverse lookup needed, so in most cases just
					  // leave this blank:
					  const std::string& dictionary_name = empty_string)
	:	LLDictionaryEntry(dictionary_name),
		mNewCategoryName(category_name),
		mIconName(icon_name),
		mIcon(icon),
		mIsQuiet(is_quiet),
		mHideIfEmpty(hide_if_empty)
	{
	}

	const std::string mIconName;		// name of the folder icon
	const LLUIImagePtr mIcon;			// pointer to the icon itself
	const std::string mNewCategoryName;	// default name when creating a new folder of this type
	BOOL mIsQuiet;						// folder doesn't need a UI update when changed
	bool mHideIfEmpty;					// folder not shown if empty
};

class LLViewerFolderDictionary : public LLSingleton<LLViewerFolderDictionary>,
								 public LLDictionary<LLFolderType::EType, ViewerFolderEntry>
{
public:
	LLViewerFolderDictionary();
};

LLViewerFolderDictionary::LLViewerFolderDictionary()
{
	static const LLUIImagePtr plain_icon = LLUI::getUIImage("inv_folder_plain_closed.tga");
	static const LLUIImagePtr texture_icon = LLUI::getUIImage("inv_folder_texture.tga");
	static const LLUIImagePtr sound_icon = LLUI::getUIImage("inv_folder_sound.tga");
	static const LLUIImagePtr callingcard_icon = LLUI::getUIImage("inv_folder_callingcard.tga");
	static const LLUIImagePtr landmark_icon = LLUI::getUIImage("inv_folder_landmark.tga");
	static const LLUIImagePtr clothing_icon = LLUI::getUIImage("inv_folder_clothing.tga");
	static const LLUIImagePtr object_icon = LLUI::getUIImage("inv_folder_object.tga");
	static const LLUIImagePtr notecard_icon = LLUI::getUIImage("inv_folder_notecard.tga");
	static const LLUIImagePtr script_icon = LLUI::getUIImage("inv_folder_script.tga");
	static const LLUIImagePtr bodypart_icon = LLUI::getUIImage("inv_folder_bodypart.tga");
	static const LLUIImagePtr trash_icon = LLUI::getUIImage("inv_folder_trash.tga");
	static const LLUIImagePtr snapshot_icon = LLUI::getUIImage("inv_folder_snapshot.tga");
	static const LLUIImagePtr lostandfound_icon = LLUI::getUIImage("inv_folder_lostandfound.tga");
	static const LLUIImagePtr animation_icon = LLUI::getUIImage("inv_folder_animation.tga");
	static const LLUIImagePtr gesture_icon = LLUI::getUIImage("inv_folder_gesture.tga");
	static const LLUIImagePtr inbox_icon = LLUI::getUIImage("inv_folder_inbox.tga");
	static const LLUIImagePtr outbox_icon = LLUI::getUIImage("inv_folder_outbox.tga");

	//       													    	  NEW CATEGORY NAME FOLDER ICON NAME                FOLDER ICON POINTER QUIET   HIDE IF EMPTY
	//      												  		     |-----------------|-------------------------------|-------------------|-------|-------------|
	addEntry(LLFolderType::FT_TEXTURE,				new ViewerFolderEntry("Textures",		"inv_folder_texture.tga",		texture_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_SOUND,				new ViewerFolderEntry("Sounds",			"inv_folder_sound.tga",			sound_icon,			FALSE,	true));
	addEntry(LLFolderType::FT_CALLINGCARD,			new ViewerFolderEntry("Calling Cards",	"inv_folder_callingcard.tga",	callingcard_icon,	TRUE,	true));
	addEntry(LLFolderType::FT_LANDMARK,				new ViewerFolderEntry("Landmarks",		"inv_folder_landmark.tga",		landmark_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_CLOTHING,				new ViewerFolderEntry("Clothing",		"inv_folder_clothing.tga",		clothing_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_OBJECT,				new ViewerFolderEntry("Objects",		"inv_folder_object.tga",		object_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_NOTECARD,				new ViewerFolderEntry("Notecards",		"inv_folder_notecard.tga",		notecard_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_ROOT_INVENTORY,		new ViewerFolderEntry("My Inventory",	"inv_folder_plain_closed.tga",	plain_icon,			FALSE,	false));
	addEntry(LLFolderType::FT_LSL_TEXT,				new ViewerFolderEntry("Scripts",		"inv_folder_script.tga",		script_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_BODYPART,				new ViewerFolderEntry("Body Parts",		"inv_folder_bodypart.tga",		bodypart_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_TRASH,				new ViewerFolderEntry("Trash",			"inv_folder_trash.tga",			trash_icon,			TRUE,	false));
	addEntry(LLFolderType::FT_SNAPSHOT_CATEGORY,	new ViewerFolderEntry("Photo Album",	"inv_folder_snapshot.tga",		snapshot_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_LOST_AND_FOUND,		new ViewerFolderEntry("Lost And Found",	"inv_folder_lostandfound.tga",	lostandfound_icon,	TRUE,	true));
	addEntry(LLFolderType::FT_ANIMATION,			new ViewerFolderEntry("Animations",		"inv_folder_animation.tga",		animation_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_GESTURE,				new ViewerFolderEntry("Gestures",		"inv_folder_gesture.tga",		gesture_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_MESH,					new ViewerFolderEntry("Meshes",			"inv_folder_plain_closed.tga",	plain_icon,			FALSE,	true));
	addEntry(LLFolderType::FT_INBOX,				new ViewerFolderEntry("Inbox",			"inv_folder_inbox.tga",			inbox_icon,			FALSE,	true));
	addEntry(LLFolderType::FT_OUTBOX,				new ViewerFolderEntry("Outbox",			"inv_folder_outbox.tga",		outbox_icon,		FALSE,	true));
	addEntry(LLFolderType::FT_NONE,					new ViewerFolderEntry("New Folder",		"inv_folder_plain_closed.tga",	plain_icon,			FALSE,	false,	"default"));
#if 0
	addEntry(LLFolderType::FT_FAVORITE,				new ViewerFolderEntry("Favorites",		"inv_folder_plain_closed.tga",	plain_icon,			TRUE,	false));
	addEntry(LLFolderType::FT_CURRENT_OUTFIT,		new ViewerFolderEntry("Current Outfit",	"inv_folder_plain_closed.tga",	plain_icon,			TRUE,	false));
	addEntry(LLFolderType::FT_OUTFIT,				new ViewerFolderEntry("New Outfit",		"inv_folder_plain_closed.tga",	plain_icon,			TRUE,	false));
	addEntry(LLFolderType::FT_MY_OUTFITS,			new ViewerFolderEntry("My Outfits",		"inv_folder_plain_closed.tga",	plain_icon,			TRUE,	false));
#endif
}

const std::string& LLViewerFolderType::lookupXUIName(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry* entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mName;
	}
	return badLookup();
}

LLFolderType::EType LLViewerFolderType::lookupTypeFromXUIName(const std::string& name)
{
	return LLViewerFolderDictionary::getInstance()->lookup(name);
}

const std::string& LLViewerFolderType::lookupIconName(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry* entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mIconName;
	}

	// Error condition. Return something so that we don't show a grey box in
	// inventory view.
	const ViewerFolderEntry* default_entry = LLViewerFolderDictionary::getInstance()->lookup(LLFolderType::FT_NONE);
	if (!default_entry)
	{
		llerrs << "Missing FT_NONE entry in LLViewerFolderDictionary !"
			   << llendl;
	}
	return default_entry->mIconName;
}

const LLUIImagePtr LLViewerFolderType::lookupIcon(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry* entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mIcon;
	}

	const ViewerFolderEntry* default_entry = LLViewerFolderDictionary::getInstance()->lookup(LLFolderType::FT_NONE);
	if (!default_entry)
	{
		llerrs << "Missing FT_NONE entry in LLViewerFolderDictionary !"
			   << llendl;
	}
	return default_entry->mIcon;
}

BOOL LLViewerFolderType::lookupIsQuietType(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry* entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mIsQuiet;
	}
	return FALSE;
}

bool LLViewerFolderType::lookupIsHiddenIfEmpty(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry* entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mHideIfEmpty;
	}
	return false;
}

const std::string& LLViewerFolderType::lookupNewCategoryName(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry* entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mNewCategoryName;
	}
	return badLookup();
}

LLFolderType::EType LLViewerFolderType::lookupTypeFromNewCategoryName(const std::string& name)
{
	for (LLViewerFolderDictionary::const_iterator
			iter = LLViewerFolderDictionary::getInstance()->begin(),
			end = LLViewerFolderDictionary::getInstance()->end();
		 iter != end; ++iter)
	{
		const ViewerFolderEntry* entry = iter->second;
		if (entry->mNewCategoryName == name)
		{
			return iter->first;
		}
	}
	return FT_NONE;
}
