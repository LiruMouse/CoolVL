/** 
 * @file llpanelland.cpp
 * @brief Land information in the tool floater, NOT the "About Land" floater
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

#include "llpanelland.h"

#include "llbutton.h"
#include "llparcel.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "roles_constants.h"

#include "llagent.h"
#include "llfloaterland.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

LLPanelLandSelectObserver* LLPanelLandInfo::sObserver = NULL;
LLPanelLandInfo* LLPanelLandInfo::sInstance = NULL;

class LLPanelLandSelectObserver : public LLParcelObserver
{
public:
	LLPanelLandSelectObserver()				{}
	virtual ~LLPanelLandSelectObserver()	{}
	virtual void changed()					{ LLPanelLandInfo::refreshAll(); }
};

LLPanelLandInfo::LLPanelLandInfo(const std::string& name)
:	LLPanel(name)
{
	if (!sInstance)
	{
		sInstance = this;
	}
	if (!sObserver)
	{
		sObserver = new LLPanelLandSelectObserver();
		LLViewerParcelMgr::getInstance()->addObserver(sObserver);
	}
}

//virtual
LLPanelLandInfo::~LLPanelLandInfo()
{
	LLViewerParcelMgr::getInstance()->removeObserver(sObserver);
	delete sObserver;
	sObserver = NULL;

	sInstance = NULL;
}

//virtual
BOOL LLPanelLandInfo::postBuild()
{
	mBtnBuyLand = getChild<LLButton>("button buy land");
	mBtnBuyLand->setClickedCallback(onClickClaim, this);

	mBtnAbandonLand = getChild<LLButton>("button abandon land");
	mBtnAbandonLand->setClickedCallback(onClickRelease, this);

	mBtnDivideLand = getChild<LLButton>("button subdivide land");
	mBtnDivideLand->setClickedCallback(onClickDivide, this);

	mBtnJoinLand = getChild<LLButton>("button join land");
	mBtnJoinLand->setClickedCallback(onClickJoin, this);

	mBtnAboutLand = getChild<LLButton>("button about land");
	mBtnAboutLand->setClickedCallback(onClickAbout, this);

	childSetAction("button show owners help", onShowOwnersHelp, this);

	mTextLabelPrice = getChild<LLTextBox>("label_area_price");
	mTextPrice = getChild<LLTextBox>("label_area");

	return TRUE;
}

//virtual
void LLPanelLandInfo::refresh()
{
	LLViewerParcelMgr* parcelmgr = LLViewerParcelMgr::getInstance();
	LLParcel* parcel = parcelmgr->getParcelSelection()->getParcel();
	LLViewerRegion* regionp = parcelmgr->getSelectionRegion();

	if (!parcel || !regionp)
	{
		// nothing selected, disable panel
		mTextLabelPrice->setVisible(FALSE);
		mTextPrice->setVisible(FALSE);

		mBtnBuyLand->setEnabled(FALSE);
		mBtnAbandonLand->setEnabled(FALSE);
		mBtnDivideLand->setEnabled(FALSE);
		mBtnJoinLand->setEnabled(FALSE);
		mBtnAboutLand->setEnabled(FALSE);
	}
	else
	{
		// something selected, hooray!
		const LLUUID& owner_id = parcel->getOwnerID();
		const LLUUID& auth_buyer_id = parcel->getAuthorizedBuyerID();

		BOOL is_public = parcel->isPublic();
		BOOL is_for_sale = parcel->getForSale() &&
						   (parcel->getSalePrice() > 0 || auth_buyer_id.notNull());
		BOOL can_buy = is_for_sale && owner_id != gAgentID &&
					   (auth_buyer_id == gAgentID || auth_buyer_id.isNull());
			
		if (is_public)
		{
			mBtnBuyLand->setEnabled(TRUE);
		}
		else
		{
			mBtnBuyLand->setEnabled(can_buy);
		}

		BOOL owner_release = LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_RELEASE);
		BOOL owner_divide = LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_DIVIDE_JOIN);

		BOOL manager_releaseable = gAgent.canManageEstate() &&
								   parcel->getOwnerID() == regionp->getOwner();
		
		BOOL manager_divideable = gAgent.canManageEstate() &&
								  (parcel->getOwnerID() == regionp->getOwner() || owner_divide);

		mBtnAbandonLand->setEnabled(owner_release || manager_releaseable || gAgent.isGodlike());

		// only mainland sims are subdividable by owner
		if (regionp->getRegionFlags() & REGION_FLAGS_ALLOW_PARCEL_CHANGES)
		{
			mBtnDivideLand->setEnabled(owner_divide || manager_divideable ||
									   gAgent.isGodlike());
		}
		else
		{
			mBtnDivideLand->setEnabled(manager_divideable ||
									   gAgent.isGodlike());
		}
		
		// To join land, must have something selected, not just a single unit of
		// land, you must own part of it and it must not be a whole parcel.
		if (parcelmgr->getSelectedArea() > PARCEL_UNIT_AREA &&
			//parcelmgr->getSelfCount() > 1 &&
			!parcelmgr->getParcelSelection()->getWholeParcelSelected())
		{
			mBtnJoinLand->setEnabled(TRUE);
		}
		else
		{
			LL_DEBUGS("Land") << "Invalid selection for joining land" << LL_ENDL;
			mBtnJoinLand->setEnabled(FALSE);
		}

		mBtnAboutLand->setEnabled(TRUE);

		// show pricing information
		S32 area;
		S32 claim_price;
		S32 rent_price;
		BOOL for_sale;
		F32 dwell;
		parcelmgr->getDisplayInfo(&area, &claim_price, &rent_price, &for_sale,
								  &dwell);
		if (is_public ||
			(is_for_sale &&
			 parcelmgr->getParcelSelection()->getWholeParcelSelected()))
		{
			mTextLabelPrice->setTextArg("[PRICE]", llformat("%d",claim_price));
			mTextLabelPrice->setTextArg("[AREA]", llformat("%d",area));
			mTextLabelPrice->setVisible(TRUE);
			mTextPrice->setVisible(FALSE);
		}
		else
		{
			mTextLabelPrice->setVisible(FALSE);
			mTextPrice->setTextArg("[AREA]", llformat("%d",area));
			mTextPrice->setVisible(TRUE);
		}
	}
}

// static
void LLPanelLandInfo::refreshAll()
{
	if (sInstance)
	{
		sInstance->refresh();
	}
}

//static
void LLPanelLandInfo::onClickClaim(void*)
{
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
	{
		return;
	}
//mk
	LLViewerParcelMgr::getInstance()->startBuyLand();
}

//static
void LLPanelLandInfo::onClickRelease(void*)
{
	LLViewerParcelMgr::getInstance()->startReleaseLand();
}

// static
void LLPanelLandInfo::onClickDivide(void*)
{
	LLViewerParcelMgr::getInstance()->startDivideLand();
}

// static
void LLPanelLandInfo::onClickJoin(void*)
{
	LLViewerParcelMgr::getInstance()->startJoinLand();
}

//static
void LLPanelLandInfo::onClickAbout(void*)
{
	// Promote the rectangle selection to a parcel selection
	if (!LLViewerParcelMgr::getInstance()->getParcelSelection()->getWholeParcelSelected())
	{
		LLViewerParcelMgr::getInstance()->selectParcelInRectangle();
	}

//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
	{
		return;
	}
//mk
	LLFloaterLand::showInstance();
}

void LLPanelLandInfo::onShowOwnersHelp(void*)
{
	LLNotifications::instance().add("ShowOwnersHelp");
}
