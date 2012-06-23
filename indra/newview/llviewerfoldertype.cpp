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
#include "llmemory.h"

static const std::string empty_string;

struct ViewerFolderEntry : public LLDictionaryEntry
{
	// Constructor for non-ensembles
	ViewerFolderEntry(const std::string &new_category_name, // default name when creating a new category of this type
					  const std::string &icon_name_open,	// name of the folder icon
					  const std::string &icon_name_closed,
					  BOOL is_quiet,						// folder doesn't need a UI update when changed
					  bool hide_if_empty = false,			// folder not shown if empty
					  const std::string &dictionary_name = empty_string) // no reverse lookup needed on non-ensembles, so in most cases just leave this blank
	:	LLDictionaryEntry(dictionary_name),
		mNewCategoryName(new_category_name),
		mIconNameOpen(icon_name_open),
		mIconNameClosed(icon_name_closed),
		mIsQuiet(is_quiet),
		mHideIfEmpty(hide_if_empty)
	{
		mAllowedNames.clear();
	}

#if SUPPORT_ENSEMBLES
	// Constructor for ensembles
	ViewerFolderEntry(const std::string &xui_name, 			// name of the xui menu item
					  const std::string &new_category_name, // default name when creating a new category of this type
					  const std::string &icon_name, 		// name of the folder icon
					  const std::string allowed_names) 		// allowed item typenames for this folder type
	:	LLDictionaryEntry(xui_name),
		mIconNameOpen(icon_name),
		mIconNameClosed(icon_name),
		mNewCategoryName(new_category_name),
		mIsQuiet(FALSE),
		mHideIfEmpty(false)
	{
		const std::string delims (",");
		LLStringUtilBase<char>::getTokens(allowed_names, mAllowedNames, delims);
	}
#endif

	bool getIsAllowedName(const std::string &name) const
	{
		if (mAllowedNames.empty())
			return false;
		for (name_vec_t::const_iterator iter = mAllowedNames.begin();
			 iter != mAllowedNames.end(); iter++)
		{
			if (name == (*iter))
				return true;
		}
		return false;
	}
	const std::string mIconNameOpen;
	const std::string mIconNameClosed;
	const std::string mNewCategoryName;
	typedef std::vector<std::string> name_vec_t;
	name_vec_t mAllowedNames;
	BOOL mIsQuiet;
	bool mHideIfEmpty;
};

class LLViewerFolderDictionary : public LLSingleton<LLViewerFolderDictionary>,
								 public LLDictionary<LLFolderType::EType, ViewerFolderEntry>
{
public:
	LLViewerFolderDictionary();
#if SUPPORT_ENSEMBLES
protected:
	bool initEnsemblesFromFile(); // Reads in ensemble information from foldertypes.xml
#endif
};

LLViewerFolderDictionary::LLViewerFolderDictionary()
{
	//       													    	  NEW CATEGORY NAME         FOLDER OPEN                     FOLDER CLOSED                   QUIET?      HIDE IF EMPTY?
	//      												  		     |-------------------------|-------------------------------|-------------------------------|-----------|--------------|
	addEntry(LLFolderType::FT_TEXTURE, 				new ViewerFolderEntry("Textures",				"inv_folder_texture.tga",		"inv_folder_texture.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_SOUND, 				new ViewerFolderEntry("Sounds",					"inv_folder_sound.tga",			"inv_folder_sound.tga",			FALSE,		true));
	addEntry(LLFolderType::FT_CALLINGCARD, 			new ViewerFolderEntry("Calling Cards",			"inv_folder_callingcard.tga",	"inv_folder_callingcard.tga",	FALSE,		true));
	addEntry(LLFolderType::FT_LANDMARK, 			new ViewerFolderEntry("Landmarks",				"inv_folder_landmark.tga",		"inv_folder_landmark.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_CLOTHING, 			new ViewerFolderEntry("Clothing",				"inv_folder_clothing.tga",		"inv_folder_clothing.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_OBJECT, 				new ViewerFolderEntry("Objects",				"inv_folder_object.tga",		"inv_folder_object.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_NOTECARD, 			new ViewerFolderEntry("Notecards",				"inv_folder_notecard.tga",		"inv_folder_notecard.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_ROOT_INVENTORY, 		new ViewerFolderEntry("My Inventory",			"inv_folder_plain_closed.tga",	"inv_folder_plain_closed.tga",	FALSE,		false));
	addEntry(LLFolderType::FT_LSL_TEXT, 			new ViewerFolderEntry("Scripts",				"inv_folder_script.tga",		"inv_folder_script.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_BODYPART, 			new ViewerFolderEntry("Body Parts",				"inv_folder_bodypart.tga",		"inv_folder_bodypart.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_TRASH, 				new ViewerFolderEntry("Trash",					"inv_folder_trash.tga",			"inv_folder_trash.tga",			TRUE,		false));
	addEntry(LLFolderType::FT_SNAPSHOT_CATEGORY, 	new ViewerFolderEntry("Photo Album",			"inv_folder_snapshot.tga",		"inv_folder_snapshot.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_LOST_AND_FOUND, 		new ViewerFolderEntry("Lost And Found",	   		"inv_folder_lostandfound.tga",	"inv_folder_lostandfound.tga",	TRUE,		true));
	addEntry(LLFolderType::FT_ANIMATION, 			new ViewerFolderEntry("Animations",				"inv_folder_animation.tga",		"inv_folder_animation.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_GESTURE, 				new ViewerFolderEntry("Gestures",				"inv_folder_gesture.tga",		"inv_folder_gesture.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_MESH, 				new ViewerFolderEntry("Meshes",					"inv_folder_plain_closed.tga",	"inv_folder_plain_closed.tga",	FALSE,		true));
	addEntry(LLFolderType::FT_INBOX, 				new ViewerFolderEntry("Inbox",					"inv_folder_inbox.tga",			"inv_folder_inbox.tga",			FALSE,		true));
	addEntry(LLFolderType::FT_OUTBOX, 				new ViewerFolderEntry("Outbox",					"inv_folder_outbox.tga",		"inv_folder_outbox.tga",		FALSE,		true));
	addEntry(LLFolderType::FT_NONE, 				new ViewerFolderEntry("New Folder",				"inv_folder_plain_closed.tga",	"inv_folder_plain_closed.tga",	FALSE,		false,	"default"));
#if 0
	addEntry(LLFolderType::FT_FAVORITE, 			new ViewerFolderEntry("Favorites",				"Inv_SysOpen",					"Inv_SysClosed",				FALSE));
	addEntry(LLFolderType::FT_CURRENT_OUTFIT, 		new ViewerFolderEntry("Current Outfit",			"Inv_SysOpen",					"Inv_SysClosed",				TRUE));
	addEntry(LLFolderType::FT_OUTFIT, 				new ViewerFolderEntry("New Outfit",				"Inv_LookFolderOpen",			"Inv_LookFolderClosed",			TRUE));
	addEntry(LLFolderType::FT_MY_OUTFITS, 			new ViewerFolderEntry("My Outfits",				"Inv_SysOpen",					"Inv_SysClosed",				TRUE));

#if SUPPORT_ENSEMBLES
	initEnsemblesFromFile();
#else
	for (U32 type = (U32)LLFolderType::FT_ENSEMBLE_START; type <= (U32)LLFolderType::FT_ENSEMBLE_END; ++type)
	{
		addEntry((LLFolderType::EType)type, 		new ViewerFolderEntry("New Folder",				"Inv_FolderOpen",				"Inv_FolderClosed",				FALSE));
	}
#endif
#endif
}

#if SUPPORT_ENSEMBLES
bool LLViewerFolderDictionary::initEnsemblesFromFile()
{
	std::string xml_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "foldertypes.xml");
	LLXmlTree folder_def;
	if (!folder_def.parseFile(xml_filename))
	{
		llerrs << "Failed to parse folders file " << xml_filename << llendl;
		return false;
	}

	LLXmlTreeNode* rootp = folder_def.getRoot();
	for (LLXmlTreeNode* ensemble = rootp->getFirstChild();
		 ensemble;
		 ensemble = rootp->getNextChild())
	{
		if (!ensemble->hasName("ensemble"))
		{
			llwarns << "Invalid ensemble definition node " << ensemble->getName() << llendl;
			continue;
		}

		S32 ensemble_type;
		static LLStdStringHandle ensemble_num_string = LLXmlTree::addAttributeString("foldertype_num");
		if (!ensemble->getFastAttributeS32(ensemble_num_string, ensemble_type))
		{
			llwarns << "No ensemble type defined" << llendl;
			continue;
		}


		if (ensemble_type < S32(LLFolderType::FT_ENSEMBLE_START) || ensemble_type > S32(LLFolderType::FT_ENSEMBLE_END))
		{
			llwarns << "Exceeded maximum ensemble index" << LLFolderType::FT_ENSEMBLE_END << llendl;
			break;
		}

		std::string xui_name;
		static LLStdStringHandle xui_name_string = LLXmlTree::addAttributeString("xui_name");
		if (!ensemble->getFastAttributeString(xui_name_string, xui_name))
		{
			llwarns << "No xui name defined" << llendl;
			continue;
		}

		std::string icon_name;
		static LLStdStringHandle icon_name_string = LLXmlTree::addAttributeString("icon_name");
		if (!ensemble->getFastAttributeString(icon_name_string, icon_name))
		{
			llwarns << "No ensemble icon name defined" << llendl;
			continue;
		}

		std::string allowed_names;
		static LLStdStringHandle allowed_names_string = LLXmlTree::addAttributeString("allowed");
		if (!ensemble->getFastAttributeString(allowed_names_string, allowed_names))
		{
		}

		// Add the entry and increment the asset number.
		const static std::string new_ensemble_name = "New Ensemble";
		addEntry(LLFolderType::EType(ensemble_type), new ViewerFolderEntry(xui_name, new_ensemble_name, icon_name, allowed_names));
	}

	return true;
}
#endif

const std::string &LLViewerFolderType::lookupXUIName(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mName;
	}
	return badLookup();
}

LLFolderType::EType LLViewerFolderType::lookupTypeFromXUIName(const std::string &name)
{
	return LLViewerFolderDictionary::getInstance()->lookup(name);
}

const std::string &LLViewerFolderType::lookupIconName(LLFolderType::EType folder_type, BOOL is_open)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		if (is_open)
		{
			return entry->mIconNameOpen;
		}
		else
		{
			return entry->mIconNameClosed;
		}
	}
	
	// Error condition.  Return something so that we don't show a grey box in inventory view.
	const ViewerFolderEntry *default_entry = LLViewerFolderDictionary::getInstance()->lookup(LLFolderType::FT_NONE);
	if (default_entry)
	{
		return default_entry->mIconNameClosed;
	}
	
	// Should not get here unless there's something corrupted with the FT_NONE entry.
	return badLookup();
}

BOOL LLViewerFolderType::lookupIsQuietType(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
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

const std::string &LLViewerFolderType::lookupNewCategoryName(LLFolderType::EType folder_type)
{
	const ViewerFolderEntry *entry = LLViewerFolderDictionary::getInstance()->lookup(folder_type);
	if (entry)
	{
		return entry->mNewCategoryName;
	}
	return badLookup();
}

LLFolderType::EType LLViewerFolderType::lookupTypeFromNewCategoryName(const std::string& name)
{
	for (LLViewerFolderDictionary::const_iterator iter = LLViewerFolderDictionary::getInstance()->begin();
		 iter != LLViewerFolderDictionary::getInstance()->end();
		 iter++)
	{
		const ViewerFolderEntry *entry = iter->second;
		if (entry->mNewCategoryName == name)
		{
			return iter->first;
		}
	}
	return FT_NONE;
}


U64 LLViewerFolderType::lookupValidFolderTypes(const std::string& item_name)
{
	U64 matching_folders = 0;
	for (LLViewerFolderDictionary::const_iterator iter = LLViewerFolderDictionary::getInstance()->begin();
		 iter != LLViewerFolderDictionary::getInstance()->end();
		 iter++)
	{
		const ViewerFolderEntry *entry = iter->second;
		if (entry->getIsAllowedName(item_name))
		{
			matching_folders |= 1LL << iter->first;
		}
	}
	return matching_folders;
}
