/** 
 * @file llprefsmedia.h
 * @brief Media preference definitions
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLPREFSMEDIA_H
#define LL_LLPREFSMEDIA_H

#include "llpanel.h"

class LLUICtrl;

class LLPrefsMedia : public LLPanel
{
public:
	LLPrefsMedia();
	/*virtual*/ ~LLPrefsMedia() { };

	void apply() { refreshValues(); }
	void cancel();	// Cancel the changed values.

	virtual BOOL postBuild();

	static void* createVolumePanel(void* data);

	static void onCommitCheckBoxMedia(LLUICtrl* ctrl, void* user_data);
	static void onCommitCheckBoxAudio(LLUICtrl* ctrl, void* user_data);
	static void onCommitCheckBoxFilter(LLUICtrl* ctrl, void* user_data);

private:
	void refreshValues();

	bool	mRunningFMOD;

	F32		mPreviousVolume;
	F32		mPreviousMusicVolume;
	F32		mPreviousMediaVolume;
	F32		mPreviousSFX;
	F32		mPreviousUI;
	F32		mPreviousEnvironment;
	F32		mPreviousDoppler;
	F32		mPreviousDistance;
	F32		mPreviousRolloff;

	BOOL	mPreviousStreamingMusic;
	BOOL	mPreviousNotifyStreamChanges;
	BOOL	mPreviousStreamingVideo;
	BOOL	mPreviousParcelMediaAutoPlay;
	BOOL	mPreviousMediaOnAPrimUI;
	BOOL	mPreviousMediaEnableFilter;
	BOOL	mPreviousMediaLookupIP;

	BOOL	mPreviousMuteAudio;
	BOOL	mPreviousMuteWhenMinimized;
	BOOL	mPreviousEnableGestureSounds;
};

#endif
