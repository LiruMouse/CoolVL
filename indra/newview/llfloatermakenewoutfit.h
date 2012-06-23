/** 
 * @file llfloatermakenewoutfit.h
 * @brief The Make New Outfit floater - header file
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

#ifndef LL_LLFLOATERMAKENEWOUTFIT_H
#define LL_LLFLOATERMAKENEWOUTFIT_H

#include "lldarray.h"
#include "llfloater.h"

class LLFloaterMakeNewOutfit : public LLFloater
{
public:
	LLFloaterMakeNewOutfit();
	/*virtual*/ ~LLFloaterMakeNewOutfit();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	void getIncludedItems(LLDynamicArray<S32> &wearables_to_include,
						  LLDynamicArray<S32> &attachments_to_include);

	static void showInstance();

	static void setDirty();

private:
	static void onCommitCheckBox(LLUICtrl* ctrl, void* user_data);
	static void onCommitCheckBoxLinkAll(LLUICtrl* ctrl, void* user_data);
	static void onButtonSave(void* user_data);
	static void onButtonCancel(void* user_data);

	bool mIsDirty;
	std::vector<std::pair<std::string, S32> > mCheckBoxList;

	static LLFloaterMakeNewOutfit* sInstance;
};

#endif	// LL_LLFLOATERMAKENEWOUTFIT_H
