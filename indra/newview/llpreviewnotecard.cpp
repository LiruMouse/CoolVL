/** 
 * @file llpreviewnotecard.cpp
 * @brief Implementation of the notecard editor
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

#include "llpreviewnotecard.h"

#include "llbutton.h"
#include "lldir.h"
#include "llinventory.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llnotifications.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "lluictrlfactory.h"
#include "llvfile.h"
#include "roles_constants.h"

#include "llagent.h"
#include "llappviewer.h"				// abortQuit();
#include "llassetuploadresponders.h"
#include "lldirpicker.h"
#include "llfloatersearchreplace.h"
#include "llinventorymodel.h"
#include "llselectmgr.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

const S32 PREVIEW_MIN_WIDTH =	2 * PREVIEW_BORDER +
								2 * PREVIEW_BUTTON_WIDTH + 
								2 * PREVIEW_PAD + RESIZE_HANDLE_WIDTH;
const S32 PREVIEW_MIN_HEIGHT =	2 * PREVIEW_BORDER +
								3 * (20 + PREVIEW_PAD) +
								2 * SCROLLBAR_SIZE + 128;

///----------------------------------------------------------------------------
/// Class LLPreviewNotecard
///----------------------------------------------------------------------------

std::set<LLPreviewNotecard*> LLPreviewNotecard::sList;

// Default constructor
LLPreviewNotecard::LLPreviewNotecard(const std::string& name,
									 const LLRect& rect,
									 const std::string& title,
									 const LLUUID& item_id, 
									 const LLUUID& object_id,
									 const LLUUID& asset_id,
									 BOOL show_keep_discard,
									 LLPointer<LLViewerInventoryItem> inv_item)
:	LLPreview(name, rect, title, item_id, object_id, TRUE, PREVIEW_MIN_WIDTH,
			  PREVIEW_MIN_HEIGHT, inv_item),
	mAssetID(asset_id),
	mNotecardItemID(item_id),
	mObjectID(object_id),
	mEditor(NULL)
{
	sList.insert(this);

	LLRect curRect = rect;

	if (show_keep_discard)
	{
		LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preview_notecard_keep_discard.xml");
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildFloater(this, "floater_preview_notecard.xml");

		if (mAssetID.isNull())
		{
			const LLInventoryItem* item = getItem();
			if (item)
			{
				mAssetID = item->getAssetUUID();
			}
		}
	}	

	// only assert shape if not hosted in a multifloater
	if (!getHost())
	{
		reshape(curRect.getWidth(), curRect.getHeight(), TRUE);
		setRect(curRect);
	}

	setTitle(title);
	setNoteName(title);

	gAgent.changeCameraToDefault();
}

// virtual
LLPreviewNotecard::~LLPreviewNotecard()
{
	sList.erase(this);
}

// virtual
BOOL LLPreviewNotecard::postBuild()
{
	initMenu();

	mEditor = getChild<LLViewerTextEditor>("Notecard Editor", TRUE, FALSE);
	if (!mEditor) return FALSE;	// Bail !

	mEditor->setWordWrap(TRUE);
	mEditor->setSourceID(mNotecardItemID);
	mEditor->setHandleEditKeysDirectly(TRUE);
	mEditor->setNotecardInfo(mNotecardItemID, mObjectID);
	mEditor->makePristine();

	const LLInventoryItem* item = getItem();
	if (item) childSetText("desc", item->getDescription());
	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);

	childSetAction("Save", onClickSave, this);

	if (getChild<LLButton>("Keep", TRUE, FALSE))
	{
		childSetAction("Keep", onKeepBtn, this);
		childSetAction("Discard", onDiscardBtn, this);
	}

	childSetVisible("lock", FALSE);	

	return TRUE;
}

// virtual
void LLPreviewNotecard::draw()
{
	if (mEditor)
	{
		childSetEnabled("Save", !mEditor->isPristine() && getEnabled());
	}

	LLPreview::draw();
}

void LLPreviewNotecard::setNoteName(std::string name)
{
	if (name.find("Note: ") == 0)
	{
		name = name.substr(6);
	}
	if (name.empty())
	{
		name = "untitled";
	}
	mNoteName = name;
}

bool LLPreviewNotecard::saveItem(LLPointer<LLInventoryItem>* itemptr)
{
	LLInventoryItem* item = NULL;
	if (itemptr && itemptr->notNull())
	{
		item = (LLInventoryItem*)(*itemptr);
	}
	bool res = saveIfNeeded(item);
	if (res)
	{
		delete itemptr;
	}
	return res;
}

void LLPreviewNotecard::setEnabled(BOOL enabled)
{
	if (!mEditor) return;
	childSetEnabled("Notecard Editor", enabled);
	childSetVisible("lock", !enabled);
	childSetEnabled("desc", enabled);
	childSetEnabled("Save", enabled && !mEditor->isPristine());
}

// virtual
BOOL LLPreviewNotecard::handleKeyHere(KEY key, MASK mask)
{
	if (!mEditor) return TRUE;

	if ('S' == key && MASK_CONTROL == (mask & MASK_CONTROL))
	{
		saveIfNeeded();
		return TRUE;
	}

	if ('F' == key && (mask & MASK_CONTROL) && !(mask & (MASK_SHIFT | MASK_ALT)))
	{
		LLFloaterSearchReplace::show(mEditor);
		return TRUE;
	}

	return LLPreview::handleKeyHere(key, mask);
}

// virtual
BOOL LLPreviewNotecard::canClose()
{
	if (mForceClose || !mEditor || mEditor->isPristine())
	{
		return TRUE;
	}
	else
	{
		// Bring up view-modal dialog: Save changes? Yes, No, Cancel
		LLNotifications::instance().add("SaveChanges", LLSD(), LLSD(),
										boost::bind(&LLPreviewNotecard::handleSaveChangesDialog,
													this, _1, _2));
		return FALSE;
	}
}

const LLInventoryItem* LLPreviewNotecard::getDragItem()
{
	return mEditor ? mEditor->getDragItem() : NULL;
}

bool LLPreviewNotecard::hasEmbeddedInventory()
{
	return mEditor ? mEditor->hasEmbeddedInventory() : false;
}

void LLPreviewNotecard::refreshFromInventory()
{
	LL_DEBUGS("Notecard") << "LLPreviewNotecard::refreshFromInventory()" << LL_ENDL;
	loadAsset();
}

void LLPreviewNotecard::loadAsset()
{
	if (!mEditor) return;

	// request the asset.
	const LLInventoryItem* item = getItem();
	if (item)
	{
		if (gAgent.isGodlike() ||
			gAgent.allowOperation(PERM_COPY, item->getPermissions(),
								  GP_OBJECT_MANIPULATE))
		{
			mAssetID = item->getAssetUUID();
			if (mAssetID.isNull())
			{
				mEditor->setText(LLStringUtil::null);
				mEditor->makePristine();
				mEditor->setEnabled(TRUE);
				mAssetStatus = PREVIEW_ASSET_LOADED;
			}
			else
			{
				LLUUID* new_uuid = new LLUUID(mItemUUID);
				LLHost source_sim = LLHost::invalid;
				if (mObjectUUID.notNull())
				{
					LLViewerObject *objectp = gObjectList.findObject(mObjectUUID);
					if (objectp && objectp->getRegion())
					{
						source_sim = objectp->getRegion()->getHost();
					}
					else
					{
						// The object that we're trying to look at disappeared, bail.
						LL_WARNS("Notecard") << "Can't find object " << mObjectUUID
											 << " associated with notecard." << LL_ENDL;
						mAssetID.setNull();
						mEditor->setText(getString("no_object"));
						mEditor->makePristine();
						mEditor->setEnabled(FALSE);
						mAssetStatus = PREVIEW_ASSET_LOADED;
						delete new_uuid;
						return;
					}
				}
				gAssetStorage->getInvItemAsset(source_sim,
											   gAgent.getID(),
											   gAgent.getSessionID(),
											   item->getPermissions().getOwner(),
											   mObjectUUID,
											   item->getUUID(),
											   item->getAssetUUID(),
											   item->getType(),
											   &onLoadComplete,
											   (void*)new_uuid,
											   TRUE);
				mAssetStatus = PREVIEW_ASSET_LOADING;
			}
		}
		else
		{
			mAssetID.setNull();
			mEditor->setText(getString("not_allowed"));
			mEditor->makePristine();
			mEditor->setEnabled(FALSE);
			mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		if (!gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
								   GP_OBJECT_MANIPULATE))
		{
			mEditor->setEnabled(FALSE);
			childSetVisible("lock", TRUE);
		}
	}
	else
	{
		mEditor->setText(LLStringUtil::null);
		mEditor->makePristine();
		mEditor->setEnabled(TRUE);
		mAssetStatus = PREVIEW_ASSET_LOADED;
	}
}

// static
void LLPreviewNotecard::onLoadComplete(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status,
									   LLExtStat ext_status)
{
	llinfos << "LLPreviewNotecard::onLoadComplete()" << llendl;
	LLUUID* item_id = (LLUUID*)user_data;
	LLPreviewNotecard* preview = LLPreviewNotecard::getInstance(*item_id);
	if (preview && preview->mEditor)
	{
		if (0 == status)
		{
			LLVFile file(vfs, asset_uuid, type, LLVFile::READ);

			S32 file_length = file.getSize();

			char* buffer = new char[file_length + 1];
			file.read((U8*)buffer, file_length);		/*Flawfinder: ignore*/
			// put a EOS at the end
			buffer[file_length] = 0;

			if (file_length > 19 && !strncmp(buffer, "Linden text version", 19))
			{
				if (!preview->mEditor->importBuffer(buffer, file_length + 1))
				{
					LL_WARNS("Notecard") << "Problem importing notecard" << LL_ENDL;
				}
			}
			else
			{
				// Version 0 (just text, doesn't include version number)
				preview->mEditor->setText(LLStringExplicit(buffer));
			}

			preview->mEditor->makePristine();

			const LLInventoryItem* item = preview->getItem();
			BOOL modifiable = item && gAgent.allowOperation(PERM_MODIFY,
															item->getPermissions(),
															GP_OBJECT_MANIPULATE);
			preview->setEnabled(modifiable);
			delete[] buffer;
			preview->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			LLViewerStats::getInstance()->incStat(LLViewerStats::ST_DOWNLOAD_FAILED);

			if (LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotifications::instance().add("NotecardMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotifications::instance().add("NotecardNoPermissions");
			}
			else
			{
				LLNotifications::instance().add("UnableToLoadNotecard");
			}

			LL_WARNS("Notecard") << "Problem loading notecard: "
								 << status << LL_ENDL;
			preview->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}
	delete item_id;
}

// static
LLPreviewNotecard* LLPreviewNotecard::getInstance(const LLUUID& item_id)
{
	LLPreview* instance = NULL;
	preview_map_t::iterator found_it = LLPreview::sInstances.find(item_id);
	if (found_it != LLPreview::sInstances.end())
	{
		instance = found_it->second;
	}
	return (LLPreviewNotecard*)instance;
}

// static
void LLPreviewNotecard::onClickSave(void* user_data)
{
	//llinfos << "LLPreviewNotecard::onBtnSave()" << llendl;
	LLPreviewNotecard* preview = (LLPreviewNotecard*)user_data;
	if (preview)
	{
		preview->saveIfNeeded();
	}
}

struct LLSaveNotecardInfo
{
	LLPreviewNotecard* mSelf;
	LLUUID mItemUUID;
	LLUUID mObjectUUID;
	LLTransactionID mTransactionID;
	LLPointer<LLInventoryItem> mCopyItem;

	LLSaveNotecardInfo(LLPreviewNotecard* self,
					   const LLUUID& item_id,
					   const LLUUID& object_id,
					   const LLTransactionID& transaction_id,
					   LLInventoryItem* copyitem)
	:	mSelf(self),
		mItemUUID(item_id),
		mObjectUUID(object_id),
		mTransactionID(transaction_id),
		mCopyItem(copyitem)
	{
	}
};

bool LLPreviewNotecard::saveIfNeeded(LLInventoryItem* copyitem)
{
	if (!mEditor) return false;

	if (!gAssetStorage)
	{
		LL_WARNS("Notecard") << "Not connected to an asset storage system." << LL_ENDL;
		return false;
	}

	if (!mEditor->isPristine())
	{
		// We need to update the asset information
		LLTransactionID tid;
		tid.generate();
		LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

		LLVFile file(gVFS, asset_id, LLAssetType::AT_NOTECARD, LLVFile::APPEND);

		std::string buffer;
		if (!mEditor->exportBuffer(buffer))
		{
			return false;
		}

		mEditor->makePristine();

		S32 size = buffer.length() + 1;
		file.setMaxSize(size);
		file.write((U8*)buffer.c_str(), size);

		const LLInventoryItem* item = getItem();
		// save it out to database
		if (item)
		{			
			std::string agent_url = gAgent.getRegion()->getCapability("UpdateNotecardAgentInventory");
			std::string task_url = gAgent.getRegion()->getCapability("UpdateNotecardTaskInventory");
			if (mObjectUUID.isNull() && !agent_url.empty())
			{
				// Saving into agent inventory
				mAssetStatus = PREVIEW_ASSET_LOADING;
				setEnabled(FALSE);
				LLSD body;
				body["item_id"] = mItemUUID;
				llinfos << "Saving notecard " << mItemUUID
						<< " into agent inventory via " << agent_url << llendl;
				LLHTTPClient::post(agent_url, body,
								   new LLUpdateAgentInventoryResponder(body,
																	   asset_id,
																	   LLAssetType::AT_NOTECARD));
			}
			else if (!mObjectUUID.isNull() && !task_url.empty())
			{
				// Saving into task inventory
				mAssetStatus = PREVIEW_ASSET_LOADING;
				setEnabled(FALSE);
				LLSD body;
				body["task_id"] = mObjectUUID;
				body["item_id"] = mItemUUID;
				llinfos << "Saving notecard " << mItemUUID << " into task "
						<< mObjectUUID << " via " << task_url << llendl;
				LLHTTPClient::post(task_url, body,
								   new LLUpdateTaskInventoryResponder(body,
																	  asset_id,
																	  LLAssetType::AT_NOTECARD));
			}
			else if (gAssetStorage)
			{
				LLSaveNotecardInfo* info = new LLSaveNotecardInfo(this,
																  mItemUUID,
																  mObjectUUID,
																  tid,
																  copyitem);
				gAssetStorage->storeAssetData(tid, LLAssetType::AT_NOTECARD,
											  &onSaveComplete, (void*)info,
											  FALSE);
			}
		}
	}
	return true;
}

// static
void LLPreviewNotecard::onSaveComplete(const LLUUID& asset_uuid,
									   void* user_data,
									   S32 status,
									   LLExtStat ext_status)
{
	LLSaveNotecardInfo* info = (LLSaveNotecardInfo*)user_data;
	if (info && 0 == status)
	{
		if (info->mObjectUUID.isNull())
		{
			LLViewerInventoryItem* item;
			item = (LLViewerInventoryItem*)gInventory.getItem(info->mItemUUID);
			if (item)
			{
				LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
				new_item->setAssetUUID(asset_uuid);
				new_item->setTransactionID(info->mTransactionID);
				new_item->updateServer(FALSE);
				gInventory.updateItem(new_item);
				gInventory.notifyObservers();
			}
			else
			{
				LL_WARNS("Notecard") << "Inventory item for notecard "
									 << info->mItemUUID
									 << " is no longer in agent inventory."
									 << LL_ENDL;
			}
		}
		else
		{
			LLViewerObject* object = gObjectList.findObject(info->mObjectUUID);
			LLViewerInventoryItem* item = NULL;
			if (object)
			{
				item = (LLViewerInventoryItem*)object->getInventoryObject(info->mItemUUID);
			}
			if (object && item)
			{
				item->setAssetUUID(asset_uuid);
				item->setTransactionID(info->mTransactionID);
				object->updateInventory(item, TASK_INVENTORY_ITEM_KEY, false);
				dialog_refresh_all();
			}
			else
			{
				LLNotifications::instance().add("SaveNotecardFailObjectNotFound");
			}
		}
		// Perform item copy to inventory
		if (info->mCopyItem.notNull())
		{
			LLViewerTextEditor* editor = info->mSelf->mEditor;
			if (editor)
			{
				editor->copyInventory(info->mCopyItem);
			}
		}

		// Find our window and close it if requested.
		LLPreviewNotecard* previewp = (LLPreviewNotecard*)LLPreview::find(info->mItemUUID);
		if (previewp && previewp->mCloseAfterSave)
		{
			previewp->close();
		}
	}
	else
	{
		LL_WARNS("Notecard") << "Problem saving notecard: " << status << LL_ENDL;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotifications::instance().add("SaveNotecardFailReason", args);
	}

	std::string uuid_string;
	asset_uuid.toString(uuid_string);
	std::string filename;
	filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, uuid_string) + ".tmp";
	LLFile::remove(filename);
	delete info;
}

bool LLPreviewNotecard::handleSaveChangesDialog(const LLSD& notification,
												const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	switch (option)
	{
		case 0:  // "Yes"
			mCloseAfterSave = TRUE;
			LLPreviewNotecard::onClickSave((void*)this);
			break;

		case 1:  // "No"
			mForceClose = TRUE;
			close();
			break;

		case 2: // "Cancel"
		default:
			// If we were quitting, we didn't really mean it.
			LLAppViewer::instance()->abortQuit();
			break;
	}
	return false;
}

void LLPreviewNotecard::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPreview::reshape(width, height, called_from_parent);

	if (!isMinimized())
	{
		// So that next time you open a notecard it will have the same
		// height and width  (although not the same position).
		gSavedSettings.setRect("NotecardEditorRect", getRect());
	}
}

LLTextEditor* LLPreviewNotecard::getEditor()
{
	return mEditor;
}

void LLPreviewNotecard::initMenu()
{
	LLMenuItemCallGL* menuItem = getChild<LLMenuItemCallGL>("Load From File");
	menuItem->setMenuCallback(onLoadFromFile, this);
	menuItem->setEnabledCallback(enableSaveLoadFile);

	menuItem = getChild<LLMenuItemCallGL>("Save To File");
	menuItem->setMenuCallback(onSaveToFile, this);
	menuItem->setEnabledCallback(enableSaveLoadFile);

	menuItem = getChild<LLMenuItemCallGL>("Save To Inventory");
	menuItem->setMenuCallback(onClickSave, this);
	menuItem->setEnabledCallback(hasChanged);

	menuItem = getChild<LLMenuItemCallGL>("Undo");
	menuItem->setMenuCallback(onUndoMenu, this);
	menuItem->setEnabledCallback(enableUndoMenu);

	menuItem = getChild<LLMenuItemCallGL>("Redo");
	menuItem->setMenuCallback(onRedoMenu, this);
	menuItem->setEnabledCallback(enableRedoMenu);

	menuItem = getChild<LLMenuItemCallGL>("Cut");
	menuItem->setMenuCallback(onCutMenu, this);
	menuItem->setEnabledCallback(enableCutMenu);

	menuItem = getChild<LLMenuItemCallGL>("Copy");
	menuItem->setMenuCallback(onCopyMenu, this);
	menuItem->setEnabledCallback(enableCopyMenu);

	menuItem = getChild<LLMenuItemCallGL>("Paste");
	menuItem->setMenuCallback(onPasteMenu, this);
	menuItem->setEnabledCallback(enablePasteMenu);

	menuItem = getChild<LLMenuItemCallGL>("Select All");
	menuItem->setMenuCallback(onSelectAllMenu, this);
	menuItem->setEnabledCallback(enableSelectAllMenu);

	menuItem = getChild<LLMenuItemCallGL>("Deselect");
	menuItem->setMenuCallback(onDeselectMenu, this);
	menuItem->setEnabledCallback(enableDeselectMenu);

	menuItem = getChild<LLMenuItemCallGL>("Search / Replace...");
	menuItem->setMenuCallback(onSearchMenu, this);
	menuItem->setEnabledCallback(NULL);
}

//static
BOOL LLPreviewNotecard::hasChanged(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return !self->mEditor->isPristine() && self->getEnabled();
	}
	else
	{
		return FALSE;
	}
}

//static
BOOL LLPreviewNotecard::enableSaveLoadFile(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return !LLFilePickerThread::isInUse() && !LLDirPickerThread::isInUse()
			   && self->getEnabled();
	}
	else
	{
		return FALSE;
	}
}

// static
void LLPreviewNotecard::loadFromFileCallback(LLFilePicker::ELoadFilter type,
											 std::string& filename,
											 std::deque<std::string>& files,
											 void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (!self || !sList.count(self))
	{
		LLNotifications::instance().add("LoadNoteAborted");
		return;
	}
	if (!filename.empty())
	{
		std::ifstream file(filename.c_str());
		if (!file.fail())
		{
			self->mEditor->clear();
			std::string line, text;
			while (!file.eof())
			{
				getline(file, line);
				text += line + "\n";
			}
			file.close();
			LLWString wtext = utf8str_to_wstring(text);
			LLWStringUtil::replaceTabsWithSpaces(wtext, 4);
			text = wstring_to_utf8str(wtext);
			self->mEditor->setText(text);
		}
	}
}

// static
void LLPreviewNotecard::onLoadFromFile(void* userdata)
{
	(new LLLoadFilePicker(LLFilePicker::FFLOAD_TEXT,
						  LLPreviewNotecard::loadFromFileCallback,
						  userdata))->getFile();
}

//static
void LLPreviewNotecard::saveToFileCallback(LLFilePicker::ESaveFilter type,
										   std::string& filename,
										   void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (!self || !sList.count(self))
	{
		LLNotifications::instance().add("SaveNoteAborted");
		return;
	}

	if (!filename.empty())
	{
		std::string lcname = filename;
		LLStringUtil::toLower(lcname);
		if (lcname.find(".txt") != lcname.length() - 4)
		{
			filename += ".txt";
		}
		std::ofstream file(filename.c_str());
		if (!file.fail())
		{
			file << self->mEditor->getText();
			file.close();
		}
	}
}

// static
void LLPreviewNotecard::onSaveToFile(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (!self || !sList.count(self)) return;
	std::string suggestion = self->mNoteName + ".txt";
	(new LLSaveFilePicker(LLFilePicker::FFSAVE_TEXT,
						  LLPreviewNotecard::saveToFileCallback,
						  userdata))->getSaveFile(suggestion);
}

// static 
void LLPreviewNotecard::onSearchMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		LLFloaterSearchReplace::show(self->mEditor);
	}
}

// static 
void LLPreviewNotecard::onUndoMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		self->mEditor->undo();
	}
}

// static 
void LLPreviewNotecard::onRedoMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		self->mEditor->redo();
	}
}

// static 
void LLPreviewNotecard::onCutMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		self->mEditor->cut();
	}
}

// static 
void LLPreviewNotecard::onCopyMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		self->mEditor->copy();
	}
}

// static 
void LLPreviewNotecard::onPasteMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		self->mEditor->paste();
	}
}

// static 
void LLPreviewNotecard::onSelectAllMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		self->mEditor->selectAll();
	}
}

// static 
void LLPreviewNotecard::onDeselectMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		self->mEditor->deselect();
	}
}

// static 
BOOL LLPreviewNotecard::enableUndoMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return self->mEditor->canUndo();
	}
	else
	{
		return FALSE;
	}
}

// static 
BOOL LLPreviewNotecard::enableRedoMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return self->mEditor->canRedo();
	}
	else
	{
		return FALSE;
	}
}

// static 
BOOL LLPreviewNotecard::enableCutMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return self->mEditor->canCut();
	}
	else
	{
		return FALSE;
	}
}

// static 
BOOL LLPreviewNotecard::enableCopyMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return self->mEditor->canCopy();
	}
	else
	{
		return FALSE;
	}
}

// static 
BOOL LLPreviewNotecard::enablePasteMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return self->mEditor->canPaste();
	}
	else
	{
		return FALSE;
	}
}

// static 
BOOL LLPreviewNotecard::enableSelectAllMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return self->mEditor->canSelectAll();
	}
	else
	{
		return FALSE;
	}
}

// static 
BOOL LLPreviewNotecard::enableDeselectMenu(void* userdata)
{
	LLPreviewNotecard* self = (LLPreviewNotecard*)userdata;
	if (self && self->mEditor)
	{
		return self->mEditor->canDeselect();
	}
	else
	{
		return FALSE;
	}
}

// EOF
