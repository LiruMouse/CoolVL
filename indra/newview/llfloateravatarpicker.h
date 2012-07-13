/**
 * @file llfloateravatarpicker.h
 * @brief was llavatarpicker.h
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

#ifndef LLFLOATERAVATARPICKER_H
#define LLFLOATERAVATARPICKER_H

#include <deque>
#include <vector>

#include "llfloater.h"

class LLButton;
class LLFolderViewItem;
class LLInventoryPanel;
class LLLineEditor;
class LLMessageSystem;
class LLScrollListCtrl;
class LLTabContainer;
class LLUICtrl;

class LLFloaterAvatarPicker : public LLFloater
{
public:
	// Call this to select an avatar.
	// The callback function will be called with an avatar name and UUID.
	typedef void(*callback_t)(const std::vector<std::string>&,
							  const std::vector<LLUUID>&, void*);
	static LLFloaterAvatarPicker* show(callback_t callback,
									   void* userdata,
									   BOOL allow_multiple = FALSE,
									   BOOL closeOnSelect = FALSE);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);

	static LLFloaterAvatarPicker* getInstance()		{ return sInstance; }

	static void processAvatarPickerReply(LLMessageSystem* msg, void**);
	void processResponse(const LLUUID& query_id, const LLSD& content);

private:
	// do not call these directly
	LLFloaterAvatarPicker();
	/*virtual*/ ~LLFloaterAvatarPicker();

	static void	editKeystroke(LLLineEditor* caller, void* user_data);

	static void	onBtnFind(void* userdata);
	static void	onBtnSelect(void* userdata);
	static void	onBtnRefresh(void* userdata);
	static void	onRangeAdjust(LLUICtrl* source, void* data);
	static void	onBtnClose(void* userdata);
	static void	onList(LLUICtrl* ctrl, void* userdata);
	static void	onTabChanged(void* userdata, bool from_click);

	void		doCallingCardSelectionChange(const std::deque<LLFolderViewItem*>& items,
											 BOOL user_action, void* data);
	static void	onCallingCardSelectionChange(const std::deque<LLFolderViewItem*>& items,
											 BOOL user_action, void* data);

	void		populateNearMe();
	void		populateFriends();

	// Returns true if any items in the current tab are selected:
	BOOL		visibleItemsSelected() const;

	void		find();
	void		setAllowMultiple(BOOL allow_multiple);

private:
	LLTabContainer*				mResidentChooserTabs;
	LLPanel*					mSearchPanel;
	LLPanel*					mFriendsPanel;
	LLPanel*					mCallingCardsPanel;
	LLPanel*					mNearMePanel;
	LLScrollListCtrl*			mSearchResults;
	LLScrollListCtrl*			mFriends;
	LLScrollListCtrl*			mNearMe;
	LLInventoryPanel*			mInventoryPanel;
	LLButton*					mSelect;
	LLButton*					mFind;
	LLLineEditor*				mEdit;

	std::vector<LLUUID>			mSelectedInventoryAvatarIDs;
	std::vector<std::string>	mSelectedInventoryAvatarNames;
	LLUUID						mQueryID;
	bool						mNearMeListComplete;
	BOOL						mCloseOnSelect;

	void						(*mCallback)(const std::vector<std::string>& name,
											 const std::vector<LLUUID>& id,
											 void* userdata);
	void*						mCallbackUserdata;

	static LLFloaterAvatarPicker* sInstance;
};

#endif
