/**
 * @file llprogressbar.cpp
 * @brief LLProgressBar class implementation
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

#include "linden_common.h"

#include "llprogressbar.h"

#include "indra_constants.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llglheaders.h"
#include "llimagegl.h"
#include "llmath.h"
#include "lltimer.h"

static LLRegisterWidget<LLProgressBar> r("progress_bar");

LLProgressBar::LLProgressBar(const std::string& name, const LLRect& rect)
:	LLView(name, rect, FALSE),
	mPercentDone(0.f),
	mColorBackground(LLUI::sLoginProgressBarBgColor)
{
	// Defaults:
	setImageBar("progressbar_fill.tga");
	setImageShadow("progressbar_track.tga");
}

LLProgressBar::~LLProgressBar()
{
	gFocusMgr.releaseFocusIfNeeded(this);
}

void LLProgressBar::draw()
{
	static LLTimer timer;

	mImageShadow->draw(getLocalRect(), mColorBackground);

	LLRect progress_rect = getLocalRect();
	progress_rect.mRight = llround(getRect().getWidth() * mPercentDone / 100.f);
	mImageBar->draw(progress_rect);
}

void LLProgressBar::setPercent(const F32 percent)
{
	mPercentDone = llclamp(percent, 0.f, 100.f);
}

void LLProgressBar::setImageBar(const std::string& bar_name)
{
	mImageBar = LLUI::getUIImage(bar_name);
}

void LLProgressBar::setImageShadow(const std::string& shadow_name)
{
	mImageShadow = LLUI::getUIImage(shadow_name);
}

void LLProgressBar::setColorBackground(const LLColor4& c)
{
	mColorBackground = c;
}

// virtual
LLXMLNodePtr LLProgressBar::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLView::getXML();
	node->setName(LL_PROGRESS_BAR_TAG);
	return node;
}

// static
LLView* LLProgressBar::fromXML(LLXMLNodePtr node, LLView *parent,
							   LLUICtrlFactory *factory)
{
	std::string name("progress_bar");
	node->getAttributeString("name", name);

	LLProgressBar *progress = new LLProgressBar(name, LLRect());

	std::string image_bar;
	if (node->hasAttribute("image_bar"))
	{
		node->getAttributeString("image_bar", image_bar);
	}
	if (image_bar != LLStringUtil::null)
	{
		progress->setImageBar(image_bar);
	}

	std::string image_shadow;
	if (node->hasAttribute("image_shadow"))
	{
		node->getAttributeString("image_shadow", image_shadow);
	}
	if (image_shadow != LLStringUtil::null)
	{
		progress->setImageShadow(image_shadow);
	}

	LLColor4 color_bg;
	if (node->hasAttribute("color_bg"))
	{
		node->getAttributeColor4("color_bg", color_bg);
		progress->setColorBackground(color_bg);
	}

	progress->initFromXML(node, parent);

	return progress;
}
