/** 
 * @file llspellcheck.h
 * @brief LLSpellCheck class definition
 *
 * $LicenseInfo:firstyear=2012&license=viewergpl$
 * 
 * Copyright (c) 2009 LordGregGreg Back, 2012 Henri Beauchamp 
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

#ifndef LLSPELLCHECK_H
#define LLSPELLCHECK_H

#include "llsingleton.h"
#include "llui.h"

class Hunspell;

class LLSpellCheck : public LLSingleton<LLSpellCheck>, public LLInitClass<LLSpellCheck>
{
	friend class LLSingleton<LLSpellCheck>;
	friend class LLInitClass<LLSpellCheck>;

protected:
	LLSpellCheck();
	~LLSpellCheck();

	static void initClass();	// LLSingleton interface

public:
	std::string getDicFullPath(const std::string& file);
	std::set<std::string> getBaseDicts();
	std::set<std::string> getExtraDicts();

	void addDictionary(const std::string& dict_name);
	void setDictionary(const std::string& dict_name);
	void addToCustomDictionary(const std::string& word);
	void addToIgnoreList(const std::string& word);

	const std::string& getCurrentDict()	{ return mCurrentDictName; }

	bool checkSpelling(const std::string& word);
	S32 getSuggestions(const std::string& word,
					   std::vector<std::string>& suggestions);

	void setSpellCheck(BOOL enable)		{ mSpellCheckEnable = enable; }
	BOOL getSpellCheck()				{ return mSpellCheckEnable; }

	void setShowMisspelled(BOOL enable)	{ mShowMisspelled = enable; }
	BOOL getShowMisspelled()			{ return mShowMisspelled; }

protected:
	Hunspell*	mHunspell;
	BOOL		mSpellCheckEnable;
	BOOL		mShowMisspelled;
	std::string	mCurrentDictName;
	std::set<std::string> mIgnoreList;
};

#endif	// LLSPELLCHECK_H
