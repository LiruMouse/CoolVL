/** 
 * @file llavatarname.h
 * @brief Represents name-related data for an avatar, such as the
 * username/SLID ("bobsmith123" or "james.linden") and the display
 * name ("James Cook")
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
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
#ifndef LLAVATARNAME_H
#define LLAVATARNAME_H

#include <string>

class LLSD;

class LL_COMMON_API LLAvatarName
{
public:
	LLAvatarName();
	
	bool operator<(const LLAvatarName& rhs) const;

	LLSD asLLSD() const;

	void fromLLSD(const LLSD& sd);

	// For normal names, returns "James Linden (james.linden)"
	// When display names are disabled returns just "James Linden"
	std::string getCompleteName() const;

	// For normal names, returns "Whatever Display Name [John Doe]" when
	// display name and legacy name are different, or just "John Doe"
	// when they are equal or when display names are disabled.
	// When linefeed == true, the space between the display name and
	// the opening square bracket for the legacy name is replaced with
	// a line feed.
	std::string getNames(bool linefeed = false) const;

	// Returns "James Linden" or "bobsmith123 Resident" for backwards
	// compatibility with systems like voice and muting
	// Never omit "Resident" when full is true.
	std::string getLegacyName(bool full = false) const;

	// "bobsmith123" or "james.linden", US-ASCII only
	std::string mUsername;

	// "Jose' Sanchez" or "James Linden", UTF-8 encoded Unicode
	// Contains data whether or not user has explicitly set
	// a display name; may duplicate their username.
	std::string mDisplayName;

	// For "James Linden", "James"
	// For "bobsmith123", "bobsmith123"
	// Used to communicate with legacy systems like voice and muting which
	// rely on old-style names.
	std::string mLegacyFirstName;

	// For "James Linden", "Linden"
	// For "bobsmith123", "Resident"
	// see above for rationale
	std::string mLegacyLastName;

	// Under error conditions, we may insert "dummy" records with
	// names like "???" into caches as placeholders.  These can be
	// shown in UI, but are not serialized.
	bool mIsDisplayNameDefault;

	// Under error conditions, we may insert "dummy" records with
	// names equal to legacy name into caches as placeholders.
	// These can be shown in UI, but are not serialized.
	bool mIsTemporaryName;

	// Names can change, so need to keep track of when name was
	// last checked.
	// Unix time-from-epoch seconds for efficiency
	F64 mExpires;
	
	// You can only change your name every N hours, so record
	// when the next update is allowed
	// Unix time-from-epoch seconds
	F64 mNextUpdate;

	// true to prevent the displaying of "Resident" as a last name
	// in legacy names
	static bool sOmitResidentAsLastName;
};

#endif
