/** 
 * @file hbfloatertextinput.cpp
 * @brief HBFloaterTextInput class implementation
 *
 * $LicenseInfo:firstyear=2012&license=viewergpl$
 * 
 * Copyright (c) 2012, Henri Beauchamp.
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

#include "hbfloatertextinput.h"

#include "lllineeditor.h"
#include "llstring.h"
#include "lltexteditor.h"
#include "lluictrlfactory.h"

#include "llviewercontrol.h"

//static
std::map<LLLineEditor*, HBFloaterTextInput*>  HBFloaterTextInput::sInstancesMap;

HBFloaterTextInput::HBFloaterTextInput(LLLineEditor* input_line,
									   const std::string& dest)
:	LLFloater(std::string("text input")),
	mCallerLineEditor(input_line),
	mMustClose(false)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_text_input.xml");
	std::string title;
	if (dest.empty())
	{
		title = getString("chat");
		mRectControl = "ChatInputEditorRect";
	}
	else
	{
		LLStringUtil::format_map_t arg;
		arg["[NAME]"] = dest;
		title = getString("im", arg);
		mRectControl = "IMInputEditorRect";
	}
	setTitle(title);

	LLRect rect = gSavedSettings.getRect(mRectControl);
	reshape(rect.getWidth(), rect.getHeight());
	setRect(rect);

	sInstancesMap.insert(std::make_pair(input_line, this));
}

//virtual
HBFloaterTextInput::~HBFloaterTextInput()
{
	if (mCallerLineEditor)
	{
		std::map<LLLineEditor*, HBFloaterTextInput*>::iterator it;
		it = sInstancesMap.find(mCallerLineEditor);
		if (it != sInstancesMap.end())
		{
			sInstancesMap.erase(it);
		}
		else
		{
			llwarns << "Couldn't find the floater in the instances map" << llendl;
		}
		mCallerLineEditor->setText(mTextEditor->getText());
		mCallerLineEditor->setCursorToEnd();
		mCallerLineEditor->setFocus(TRUE);
	}
	mCallerLineEditor = NULL;
	mTextEditor = NULL;

	gSavedSettings.setRect(mRectControl, getRect());
}

//virtual
BOOL HBFloaterTextInput::postBuild()
{
	mTextEditor = getChild<LLTextEditor>("text");
	mTextEditor->setOnKeystrokeCallback(onKeystrokeCallback, this);
	mTextEditor->setText(mCallerLineEditor->getText());
	mTextEditor->endOfDoc();
	mTextEditor->setFocus(TRUE);
	return TRUE;
}

//virtual
void HBFloaterTextInput::draw()
{
	if (mMustClose)
	{
		close();
	}
	else
	{
		LLFloater::draw();
	}
}

//static
HBFloaterTextInput* HBFloaterTextInput::show(LLLineEditor* input_line,
											 const std::string& dest)
{
	HBFloaterTextInput* instance = NULL;

	std::map<LLLineEditor*, HBFloaterTextInput*>::iterator it;
	it = sInstancesMap.find(input_line);
	if (it != sInstancesMap.end())
	{
		instance = (*it).second;
		instance->setFocus(TRUE);
		instance->open();
	}
	else
	{
		instance = new HBFloaterTextInput(input_line, dest);
	}

	return instance;
}

// This function *must* be invoked by the caller of show() when it gets
// destroyed. It ensures the text input floater gets destroyed in its turn
// and doesn't attempt to call back methods pertaining to the destroyed
// caller object or its children.
//static
void HBFloaterTextInput::abort(LLLineEditor* input_line)
{
	std::map<LLLineEditor*, HBFloaterTextInput*>::iterator it;
	it = sInstancesMap.find(input_line);
	if (it != sInstancesMap.end())
	{
		HBFloaterTextInput* instance = (*it).second;
		instance->mCallerLineEditor = NULL;
		instance->close();
		sInstancesMap.erase(it);
	}
}

//static
bool HBFloaterTextInput::hasFloaterFor(LLLineEditor* input_line)
{
	return sInstancesMap.find(input_line) != sInstancesMap.end();
}

//static
BOOL HBFloaterTextInput::onKeystrokeCallback(KEY key, MASK mask,
											 LLTextEditor* caller,
											 void* userdata)
{
	HBFloaterTextInput* self = (HBFloaterTextInput*)userdata;

	if (self && key == KEY_RETURN && mask == MASK_NONE)
	{
		// Flag for closing. We can't close now because then we would destroy
		// the object to which pertains the method that called us...
		self->mMustClose = true;
		return TRUE;
	}

	return FALSE;
}
