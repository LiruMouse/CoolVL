/** 
 * @file llpreviewanim.cpp
 * @brief LLPreviewAnim class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llpreviewanim.h"

#include "llbutton.h"
#include "llkeyframemotion.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"

#include "llagent.h"          // gAgent
#include "llfilepicker.h"
#include "llinventoryview.h"
#include "llvoavatarself.h"

extern LLAgent gAgent;

LLPreviewAnim::LLPreviewAnim(const std::string& name,
							 const LLRect& rect,
							 const std::string& title,
							 const LLUUID& item_uuid,
							 const S32& activate,
							 const LLUUID& object_uuid)
:	LLPreview(name, rect, title, item_uuid, object_uuid)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,
												 "floater_preview_animation.xml");

	childSetAction("Anim play btn", playAnim, this);
	childSetAction("Anim audition btn", auditionAnim, this);

	const LLInventoryItem* item = getItem();

	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetText("desc", item->getDescription());
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);

	setTitle(title);

	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}

	// preload the animation
	if (item && isAgentAvatarValid())
	{
		gAgentAvatarp->createMotion(item->getAssetUUID());
	}

	switch (activate) 
	{
		case 1:
		{
			playAnim((void *) this);
			break;
		}
		case 2:
		{
			auditionAnim((void *) this);
			break;
		}
		default:
		{
			//do nothing
		}
	}
}

// static
void LLPreviewAnim::endAnimCallback(void *userdata)
{
	LLHandle<LLFloater>* handlep = ((LLHandle<LLFloater>*)userdata);
	LLFloater* self = handlep->get();
	delete handlep; // done with the handle
	if (self)
	{
		self->childSetValue("Anim play btn", FALSE);
		self->childSetValue("Anim audition btn", FALSE);
	}
}

// static
void LLPreviewAnim::playAnim(void *userdata)
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	if (!self) return;
	const LLInventoryItem *item = self->getItem();
	if (item && isAgentAvatarValid())
	{
		LLUUID itemID=item->getAssetUUID();

		LLButton* btn = self->getChild<LLButton>("Anim play btn");
		if (btn)
		{
			btn->toggleState();
		}

		if (self->childGetValue("Anim play btn").asBoolean()) 
		{
			self->mPauseRequest = NULL;
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_START);

			LLMotion* motion = gAgentAvatarp->findMotion(itemID);
			if (motion)
			{
				motion->setDeactivateCallback(&endAnimCallback,
											  (void*)(new LLHandle<LLFloater>(self->getHandle())));
			}
		}
		else
		{
			gAgentAvatarp->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}

// static
void LLPreviewAnim::auditionAnim(void *userdata)
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	if (!self) return;
	const LLInventoryItem *item = self->getItem();
	if (item && isAgentAvatarValid())
	{
		LLUUID itemID=item->getAssetUUID();

		LLButton* btn = self->getChild<LLButton>("Anim audition btn");
		if (btn)
		{
			btn->toggleState();
		}

		if (self->childGetValue("Anim audition btn").asBoolean()) 
		{
			self->mPauseRequest = NULL;
			gAgentAvatarp->startMotion(item->getAssetUUID());

			LLMotion* motion = gAgentAvatarp->findMotion(itemID);
			if (motion)
			{
				motion->setDeactivateCallback(&endAnimCallback,
											  (void*)(new LLHandle<LLFloater>(self->getHandle())));
			}
		}
		else
		{
			gAgentAvatarp->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}

void LLPreviewAnim::onClose(bool app_quitting)
{
	const LLInventoryItem *item = getItem();
	if (item && isAgentAvatarValid())
	{
		gAgentAvatarp->stopMotion(item->getAssetUUID());
		gAgent.sendAnimationRequest(item->getAssetUUID(), ANIM_REQUEST_STOP);

		LLMotion* motion = gAgentAvatarp->findMotion(item->getAssetUUID());
		if (motion)
		{
			// *TODO: minor memory leak here, user data is never deleted
			// (Use real callbacks)
			motion->setDeactivateCallback(NULL, (void*)NULL);
		}
	}
	destroy();
}
