/** 
* @file llstatusbar.cpp
* @brief LLStatusBar class implementation
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

#include <iomanip>

#include "llviewerprecompiledheaders.h"

#include "llstatusbar.h"

// library includes
#include "llbutton.h"
#include "llframetimer.h"
#include "llfontgl.h"
#include "llparcel.h"
#include "lllineeditor.h"
#include "llrect.h"
#include "llresmgr.h"
#include "llsys.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "message.h"

// viewer includes
#include "llagent.h"
#include "llappviewer.h"
#include "llcommandhandler.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterdirectory.h"		// to spawn search
#include "llfloaterlagmeter.h"
#include "llfloaterland.h"
#include "llfloaterregioninfo.h"
#include "llfloaterscriptdebug.h"
#include "llgroupnotify.h"
#include "llhudicon.h"
#include "llnotify.h"
#include "llstatgraph.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"			// for gMenuBarView
#include "llviewerparceloverlay.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"

//
// Globals
//
LLStatusBar* gStatusBar = NULL;
S32 STATUS_BAR_HEIGHT = 0;
extern S32 MENU_BAR_HEIGHT;

// TODO: these values ought to be in the XML too
const S32 MENU_PARCEL_SPACING = 1;	// Distance from right of menu item to parcel information
const S32 SIM_STAT_WIDTH = 8;
const F32 ICON_TIMER_EXPIRY		= 3.f; // How long the balance and health icons should flash after a change.
const F32 ICON_FLASH_FREQUENCY	= 2.f;
const S32 TEXT_HEIGHT = 18;

static void onClickParcelInfo(void*);
static void onClickBalance(void*);
static void onClickBuyCurrency(void*);
static void onClickHealth(void*);
static void onClickFly(void*);
static void onClickPush(void*);
static void onClickBuild(void*);
static void onClickScripts(void*);
static void onClickNotifications(void*);
static void onClickVoice(void*);
static void onClickSee(void*);
static void onClickBuyLand(void*);
static void onClickScriptDebug(void*);

LLStatusBar::LLStatusBar(const std::string& name, const LLRect& rect)
:	LLPanel(name, LLRect(), FALSE),		// not mouse opaque
	mVisibility(true),
	mBalance(0),
	mHealth(100),
	mSquareMetersCredit(0),
	mSquareMetersCommitted(0)
{
	// status bar can possible overlay menus?
	setMouseOpaque(FALSE);
	setIsChrome(TRUE);

	mBalanceTimer = new LLFrameTimer();
	mHealthTimer = new LLFrameTimer();

	if (gSavedSettings.getBOOL("UseOldStatusBarIcons"))
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_status_bar2.xml");
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_status_bar.xml");
	}

	// status bar can never get a tab
	setFocusRoot(FALSE);

	mTextParcelName = getChild<LLTextBox>("ParcelNameText");
	mTextParcelName->setClickedCallback(onClickParcelInfo);

	mBtnScriptError = getChild<LLButton>("scriptout");
	mBtnScriptError->setClickedCallback(onClickScriptDebug, this);

	mBtnHealth = getChild<LLButton>("health");
	mBtnHealth->setClickedCallback(onClickHealth, this);
	mTextHealth = getChild<LLTextBox>("HealthText");

	mTextTime = getChild<LLTextBox>("TimeText");

	mBtnNoFly = getChild<LLButton>("no_fly");
	mBtnNoFly->setClickedCallback(onClickFly, this);

	mBtnBuyLand = getChild<LLButton>("buyland");
	mBtnBuyLand->setClickedCallback(onClickBuyLand, this);

	mBtnNoBuild = getChild<LLButton>("no_build");
	mBtnNoBuild->setClickedCallback(onClickBuild, this);

	mBtnNoPush = getChild<LLButton>("restrictpush");
	mBtnNoPush->setClickedCallback(onClickPush, this);

	mBtnNoVoice = getChild<LLButton>("status_no_voice");
	mBtnNoVoice->setClickedCallback(onClickVoice, this);

	mBtnNoSee = getChild<LLButton>("status_no_see");
	mBtnNoSee->setClickedCallback(onClickSee, this);

	mBtnBuyCurrency = getChild<LLButton>("buycurrency");
	mBtnBuyCurrency->setClickedCallback(onClickBuyCurrency, this);

	mBtnNotificationsOn = getChild<LLButton>("notifications_on");
	mBtnNotificationsOn->setClickedCallback(onClickNotifications, this);
	mBtnNotificationsOff = getChild<LLButton>("notifications_off");
	mBtnNotificationsOff->setClickedCallback(onClickNotifications, this);
	mTextNotifications = getChild<LLTextBox>("notifications");

	mBtnNoScript = getChild<LLButton>("no_scripts");
	mBtnNoScript->setClickedCallback(onClickScripts, this);

	mTextBalance = getChild<LLTextBox>("BalanceText");
	mTextBalance->setClickedCallback(onClickBalance);

	bool search_bar = gSavedSettings.getBOOL("ShowSearchBar") != FALSE;
	mBtnSearch = getChild<LLButton>("search_btn");
	mBtnSearch->setVisible(search_bar);
	mBtnSearch->setClickedCallback(onClickSearch, this);
	mBtnSearchBevel = getChild<LLButton>("menubar_search_bevel_bg");
	mBtnSearchBevel->setVisible(search_bar);
	mLineEditSearch = getChild<LLLineEditor>("search_editor");
	mLineEditSearch->setVisible(search_bar);
	mLineEditSearch->setCommitCallback(onCommitSearch);
	mLineEditSearch->setCallbackUserData(this);

	// Adding Net Stat Graph
	S32 x = getRect().getWidth() - 2;
	S32 y = 0;
	LLRect r;
	r.set(x - SIM_STAT_WIDTH, y + MENU_BAR_HEIGHT - 1, x, y + 1);
	mSGBandwidth = new LLStatGraph("BandwidthGraph", r);
	mSGBandwidth->setFollows(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mSGBandwidth->setStat(&LLViewerStats::getInstance()->mKBitStat);
	std::string text = getString("bandwidth_tooltip") + " ";
	mSGBandwidth->setLabel(text);
	mSGBandwidth->setUnits("Kbps");
	mSGBandwidth->setPrecision(0);
//	mSGBandwidth->setMouseOpaque(FALSE);
	addChild(mSGBandwidth);
	x -= SIM_STAT_WIDTH + 2;

	r.set(x - SIM_STAT_WIDTH, y + MENU_BAR_HEIGHT - 1, x, y + 1);
	mSGPacketLoss = new LLStatGraph("PacketLossPercent", r);
	mSGPacketLoss->setFollows(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mSGPacketLoss->setStat(&LLViewerStats::getInstance()->mPacketsLostPercentStat);
	text = getString("packet_loss_tooltip") + " ";
	mSGPacketLoss->setLabel(text);
	mSGPacketLoss->setUnits("%");
	mSGPacketLoss->setMin(0.f);
	mSGPacketLoss->setMax(5.f);
	mSGPacketLoss->setThreshold(0, 0.5f);
	mSGPacketLoss->setThreshold(1, 1.f);
	mSGPacketLoss->setThreshold(2, 3.f);
	mSGPacketLoss->setPrecision(1);
//	mSGPacketLoss->setMouseOpaque(FALSE);
	mSGPacketLoss->mPerSec = FALSE;
	addChild(mSGPacketLoss);
	x -= SIM_STAT_WIDTH + 2;

	r.set(x - SIM_STAT_WIDTH, y + MENU_BAR_HEIGHT - 1, x, y + 1);
	mSGMemoryUsage = new LLStatGraph("MemoryUsagePercent", r);
	mSGMemoryUsage->setFollows(FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mSGMemoryUsage->setStat(&LLViewerStats::getInstance()->mMemoryUsageStat);
	text = getString("memory_usage_tooltip") + " ";
	mSGMemoryUsage->setLabel(text);
	mSGMemoryUsage->setUnits("%");
	mSGMemoryUsage->setMin(0.f);
	mSGMemoryUsage->setMax(100.f);
	mSGMemoryUsage->setThreshold(2, 100.f);
	mSGMemoryUsage->setPrecision(0);
//	mSGMemoryUsage->setMouseOpaque(FALSE);
	mSGMemoryUsage->mPerSec = FALSE;
	addChild(mSGMemoryUsage);

	mTextStat = getChild<LLTextBox>("stat_btn");
	mTextStat->setClickedCallback(onClickStatGraph);
}

LLStatusBar::~LLStatusBar()
{
	delete mBalanceTimer;
	mBalanceTimer = NULL;

	delete mHealthTimer;
	mHealthTimer = NULL;

	// LLView destructor cleans up children
}

//-----------------------------------------------------------------------
// Overrides
//-----------------------------------------------------------------------

// virtual
void LLStatusBar::draw()
{
	if (!gMenuBarView) return;

	refresh();

	if (isBackgroundVisible())
	{
		gl_drop_shadow(0, getRect().getHeight(), getRect().getWidth(), 0,
					   LLUI::sColorDropShadow, LLUI::sDropShadowFloater);
	}
	LLPanel::draw();
}

// Per-frame updates of visibility
void LLStatusBar::refresh()
{
	static F32 saved_bwtotal = 0.0f;
	static U32 saved_mmargin = 0;
	static F32 saved_mratio = 0;

	F32 bwtotal = gViewerThrottle.getMaxBandwidth() / 1000.f;
	if (bwtotal != saved_bwtotal)
	{
		saved_bwtotal = bwtotal;
		mSGBandwidth->setMin(0.f);
		mSGBandwidth->setMax(bwtotal * 1.25f);
		mSGBandwidth->setThreshold(0, bwtotal * 0.75f);
		mSGBandwidth->setThreshold(1, bwtotal);
		mSGBandwidth->setThreshold(2, bwtotal);
	}

	static LLCachedControl<bool> main_memory_safety_check(gSavedSettings, "MainMemorySafetyCheck");
	static LLCachedControl<U32> main_memory_safety_margin(gSavedSettings, "MainMemorySafetyMargin");
	static LLCachedControl<F32> first_step_ratio(gSavedSettings, "SafetyMargin1stStepRatio");
	if (main_memory_safety_check &&
		(saved_mmargin != main_memory_safety_margin ||
		 saved_mratio != first_step_ratio))
	{
		saved_mmargin = main_memory_safety_margin;
		saved_mratio = first_step_ratio;
		U32 max_mem, max_phys;
		LLMemoryInfo::getMaxMemoryKB(max_phys, max_mem);
		// Threshold at which the texture bias gets increased
		U32 bias_mem = max_mem - (U32)(first_step_ratio * 1024.f) * main_memory_safety_margin;
		// Threshold at which the draw distance gets decreased
		U32 dd_mem = max_mem - 1024 * main_memory_safety_margin;
		mSGMemoryUsage->setThreshold(0, 100.f * (F32)bias_mem / (F32)max_mem);
		mSGMemoryUsage->setThreshold(1, 100.f * (F32)dd_mem / (F32)max_mem);
	}
	mSGMemoryUsage->setVisible(mVisibility && main_memory_safety_check);

	// *TODO: Localize / translate time
	// Get current UTC time, adjusted for the user's clock
	// being off.
	time_t utc_time;
	utc_time = time_corrected();

	// There's only one internal tm buffer.
	struct tm* internal_time;

	// Convert to Pacific, based on server's opinion of whether
	// it's daylight savings time there.
	internal_time = utc_to_pacific_time(utc_time, gPacificDaylightTime);

	static LLCachedControl<std::string> short_date_format(gSavedSettings, "ShortTimeFormat");
	std::string t;
	timeStructToFormattedString(internal_time, short_date_format, t);
	if (gPacificDaylightTime)
	{
		t += " PDT";
	}
	else
	{
		t += " PST";
	}
	mTextTime->setText(t);

	static LLCachedControl<std::string> long_date_format(gSavedSettings, "LongDateFormat");
	std::string date;
	timeStructToFormattedString(internal_time, long_date_format, date);
	mTextTime->setToolTip(date);

	LLRect r;
	const S32 MENU_RIGHT = gMenuBarView->getRightmostMenuEdge();
	S32 x = MENU_RIGHT + MENU_PARCEL_SPACING;
	S32 y = 0;

	// reshape menu bar to its content's width
	if (MENU_RIGHT != gMenuBarView->getRect().getWidth())
	{
		gMenuBarView->reshape(MENU_RIGHT, gMenuBarView->getRect().getHeight());
	}

	LLViewerRegion *region = gAgent.getRegion();
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	LLRect buttonRect;

	bool health = (parcel && parcel->getAllowDamage()) || (region && region->getAllowDamage());
	mTextHealth->setVisible(health);
	if (health)
	{
		// set visibility based on flashing
		if (!mHealthTimer->hasExpired())
		{
			health = ((S32)(mHealthTimer->getElapsedSeconds() * ICON_FLASH_FREQUENCY) & 1) != 0;
		}

		// Health
		buttonRect = mBtnHealth->getRect();
		r.setOriginAndSize(x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnHealth->setRect(r);
		x += buttonRect.getWidth();

		const S32 health_width = S32(LLFontGL::getFontSansSerifSmall()->getWidth(std::string("100%")));
		r.set(x, y + TEXT_HEIGHT - 2, x + health_width, y);
		mTextHealth->setRect(r);
		x += health_width;
	}
	mBtnHealth->setVisible(health);

	bool no_fly = (parcel && !parcel->getAllowFly()) || (region && region->getBlockFly());
	mBtnNoFly->setVisible(no_fly);
	if (no_fly)
	{
		buttonRect = mBtnNoFly->getRect();
		r.setOriginAndSize(x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnNoFly->setRect(r);
		x += buttonRect.getWidth();
	}

	bool no_build = parcel && !parcel->getAllowModify();
	mBtnNoBuild->setVisible(no_build);
	if (no_build)
	{
		buttonRect = mBtnNoBuild->getRect();
		// No Build Zone
		r.setOriginAndSize(x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnNoBuild->setRect(r);
		x += buttonRect.getWidth();
	}

	bool no_scripts = (parcel && !parcel->getAllowOtherScripts()) ||
					  (region && (region->getRegionFlags() & (REGION_FLAGS_SKIP_SCRIPTS | REGION_FLAGS_ESTATE_SKIP_SCRIPTS)));
	mBtnNoScript->setVisible(no_scripts);
	if (no_scripts)
	{
		buttonRect = mBtnNoScript->getRect();
		r.setOriginAndSize(x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnNoScript->setRect(r);
		x += buttonRect.getWidth();
	}

	bool no_push = (parcel && parcel->getRestrictPushObject()) || (region && region->getRestrictPushObject());
	mBtnNoPush->setVisible(no_push);
	if (no_push)
	{
		buttonRect = mBtnNoPush->getRect();
		r.setOriginAndSize(x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnNoPush->setRect(r);
		x += buttonRect.getWidth();
	}

	bool no_voice = parcel && !parcel->getParcelFlagAllowVoice(); 
	mBtnNoVoice->setVisible(no_voice);
	if (no_voice)
	{
		buttonRect = mBtnNoVoice->getRect();
		r.setOriginAndSize(x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnNoVoice->setRect(r);
		x += buttonRect.getWidth();
	}

	bool no_see = parcel && parcel->getHaveNewParcelLimitData() && !parcel->getSeeAVs();
	mBtnNoSee->setVisible(no_see);
	if (no_see)
	{
		buttonRect = mBtnNoSee->getRect();
		r.setOriginAndSize(x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnNoSee->setRect(r);
		x += buttonRect.getWidth();
	}

	bool canBuyLand = parcel && !parcel->isPublic() &&
					  LLViewerParcelMgr::getInstance()->canAgentBuyParcel(parcel, false);
	mBtnBuyLand->setVisible(canBuyLand);
	if (canBuyLand)
	{
		//HACK: layout tweak until this is all xml
		x += 9;
		buttonRect = mBtnBuyLand->getRect();
		r.setOriginAndSize(x, y, buttonRect.getWidth(), buttonRect.getHeight());
		mBtnBuyLand->setRect(r);
		x += buttonRect.getWidth();
	}

	std::string location_name;
	if (region)
	{
		const LLVector3& agent_pos_region = gAgent.getPositionAgent();
		S32 pos_x = lltrunc(agent_pos_region.mV[VX]);
		S32 pos_y = lltrunc(agent_pos_region.mV[VY]);
		S32 pos_z = lltrunc(agent_pos_region.mV[VZ]);

		// Round the numbers based on the velocity
		LLVector3 agent_velocity = gAgent.getVelocity();
		F32 velocity_mag_sq = agent_velocity.magVecSquared();

		const F32 FLY_CUTOFF = 6.f;		// meters/sec
		const F32 FLY_CUTOFF_SQ = FLY_CUTOFF * FLY_CUTOFF;
		const F32 WALK_CUTOFF = 1.5f;	// meters/sec
		const F32 WALK_CUTOFF_SQ = WALK_CUTOFF * WALK_CUTOFF;

		if (velocity_mag_sq > FLY_CUTOFF_SQ)
		{
			pos_x -= pos_x % 4;
			pos_y -= pos_y % 4;
		}
		else if (velocity_mag_sq > WALK_CUTOFF_SQ)
		{
			pos_x -= pos_x % 2;
			pos_y -= pos_y % 2;
		}

		mRegionDetails.mTime = mTextTime->getText();
		mRegionDetails.mBalance = mBalance;
		mRegionDetails.mAccessString = region->getSimAccessString();
		mRegionDetails.mPing = region->getNetDetailsForLCD();
		if (parcel)
		{
			location_name = region->getName()
				+ llformat(" %d, %d, %d (%s) - %s", 
						   pos_x, pos_y, pos_z,
						   region->getSimAccessString().c_str(),
						   parcel->getName().c_str());

			// keep these around for the LCD to use
			mRegionDetails.mRegionName = region->getName();
			mRegionDetails.mParcelName = parcel->getName();
			mRegionDetails.mX = pos_x;
			mRegionDetails.mY = pos_y;
			mRegionDetails.mZ = pos_z;

			mRegionDetails.mArea = parcel->getArea();
			mRegionDetails.mForSale = parcel->getForSale();
			mRegionDetails.mTraffic = LLViewerParcelMgr::getInstance()->getDwelling();
			
			if (parcel->isPublic())
			{
				mRegionDetails.mOwner = "Public";
			}
			else
			{
				if (parcel->getIsGroupOwned())
				{
					if(!parcel->getGroupID().isNull())
					{
						gCacheName->getGroupName(parcel->getGroupID(), mRegionDetails.mOwner);
					}
					else
					{
						mRegionDetails.mOwner = "Group Owned";
					}
				}
				else
				{
					// Figure out the owner's name
					gCacheName->getFullName(parcel->getOwnerID(), mRegionDetails.mOwner);
				}
			}
		}
		else
		{
			location_name = region->getName() + llformat(" %d, %d, %d (%s)",
														 pos_x, pos_y, pos_z,
														 region->getSimAccessString().c_str());
			// keep these around for the LCD to use
			mRegionDetails.mRegionName = region->getName();
			mRegionDetails.mParcelName = "Unknown";

			mRegionDetails.mX = pos_x;
			mRegionDetails.mY = pos_y;
			mRegionDetails.mZ = pos_z;
			mRegionDetails.mArea = 0;
			mRegionDetails.mForSale = FALSE;
			mRegionDetails.mOwner = "Unknown";
			mRegionDetails.mTraffic = 0.0f;
		}
	}
	else
	{
		// no region
		location_name = "(Unknown)";
		// keep these around for the LCD to use
		mRegionDetails.mRegionName = "Unknown";
		mRegionDetails.mParcelName = "Unknown";
		mRegionDetails.mAccessString = "Unknown";
		mRegionDetails.mX = 0;
		mRegionDetails.mY = 0;
		mRegionDetails.mZ = 0;
		mRegionDetails.mArea = 0;
		mRegionDetails.mForSale = FALSE;
		mRegionDetails.mOwner = "Unknown";
		mRegionDetails.mTraffic = 0.0f;
	}
//MK
	gAgent.mRRInterface.setParcelName(mRegionDetails.mParcelName);
	if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
	{
		location_name = "(Hidden) (" + region->getSimAccessString() + ")";
	}
//mk

	mTextParcelName->setText(location_name);

	// x = right edge
	// loop through: stat graphs, search btn, search text editor, money, buy money, clock
	// adjust rect
	// finally adjust parcel name rect

	S32 new_right = getRect().getWidth();

	r = mTextStat->getRect();
	r.translate(new_right - r.mRight, 0);
	mTextStat->setRect(r);
	new_right -= r.getWidth() + 15;
	mTextStat->setEnabled(TRUE);

	static LLCachedControl<bool> show_search_bar(gSavedSettings, "ShowSearchBar");
	bool search_visible = mVisibility && show_search_bar;
	if (search_visible)
	{
		r = mBtnSearchBevel->getRect();
		r.translate(new_right - r.mRight, 0);
		mBtnSearchBevel->setRect(r);

		r = mBtnSearch->getRect();
		r.translate(new_right - r.mRight, 0);
		mBtnSearch->setRect(r);
		new_right -= r.getWidth();

		r = mLineEditSearch->getRect();
		r.translate(new_right - r.mRight, 0);
		mLineEditSearch->setRect(r);
		new_right -= r.getWidth() + 6;
	}
	mLineEditSearch->setVisible(search_visible);
	mBtnSearch->setVisible(search_visible);
	mBtnSearchBevel->setVisible(search_visible);

	// Set rects of money, buy money, time
	r = mTextBalance->getRect();
	r.translate(new_right - r.mRight, 0);
	mTextBalance->setRect(r);
	new_right -= r.getWidth() - 18;

	r = mBtnBuyCurrency->getRect();
	r.translate(new_right - r.mRight, 0);
	mBtnBuyCurrency->setRect(r);
	new_right -= r.getWidth() + 6;

	r = mTextTime->getRect();
	// mTextTime->getTextPixelWidth();
	r.translate(new_right - r.mRight, 0);
	mTextTime->setRect(r);
	new_right -= r.getWidth() + 6;

	S32 left = mTextTime->getRect().mLeft;

	S32 notifications = LLNotifyBox::getNotifyBoxCount() +
						LLGroupNotifyBox::getGroupNotifyBoxCount();
	if (notifications > 0)
	{
		BOOL shown = LLNotifyBox::areNotificationsShown();
		mBtnNotificationsOn->setVisible(shown);
		mBtnNotificationsOff->setVisible(!shown);
		mTextNotifications->setText(llformat("%d", notifications));
		mTextNotifications->setVisible(TRUE);

		r = mTextNotifications->getRect();
		r.translate(new_right - r.mRight, 0);
		mTextNotifications->setRect(r);
		new_right -= r.getWidth() + 6;

		r = mBtnNotificationsOn->getRect();
		r.translate(new_right - r.mRight, 0);
		mBtnNotificationsOn->setRect(r);
		mBtnNotificationsOff->setRect(r);
		new_right -= r.getWidth() + 6;

		left = mBtnNotificationsOn->getRect().mLeft;
	}
	else
	{
		mBtnNotificationsOn->setVisible(FALSE);
		mBtnNotificationsOff->setVisible(FALSE);
		mTextNotifications->setVisible(FALSE);
	}

	if (LLHUDIcon::iconsNearby())
	{
		r = mBtnScriptError->getRect();
		r.translate(new_right - r.mRight, 0);
		mBtnScriptError->setRect(r);
		mBtnScriptError->setVisible(TRUE);
		//new_right -= r.getWidth() + 6;

		left = mBtnScriptError->getRect().mLeft;
	}
	else
	{
		mBtnScriptError->setVisible(FALSE);
	}

	// Adjust region name and parcel name
	x += 8;

	const S32 PARCEL_RIGHT =  llmin(left, mTextParcelName->getTextPixelWidth() + x + 5);
	r.set(x + 4, getRect().getHeight() - 2, PARCEL_RIGHT, 0);
	mTextParcelName->setRect(r);
}

void LLStatusBar::setVisibleForMouselook(bool visible)
{
	mVisibility = visible;
	mTextBalance->setVisible(visible);
	mTextTime->setVisible(visible);
	mBtnBuyCurrency->setVisible(visible);
	mLineEditSearch->setVisible(visible);
	mBtnSearch->setVisible(visible);
	mBtnSearchBevel->setVisible(visible);
	mSGBandwidth->setVisible(visible);
	mSGPacketLoss->setVisible(visible);
	mSGMemoryUsage->setVisible(visible);
	setBackgroundVisible(visible);
}

void LLStatusBar::debitBalance(S32 debit)
{
	setBalance(getBalance() - debit);
}

void LLStatusBar::creditBalance(S32 credit)
{
	setBalance(getBalance() + credit);
}

void LLStatusBar::setBalance(S32 balance)
{
	std::string money_str = LLResMgr::getInstance()->getMonetaryString(balance);
	std::string balance_str = "L$";
	balance_str += money_str;
	mTextBalance->setText(balance_str);

	if (mBalance &&
		(fabs((F32)(mBalance - balance)) > gSavedSettings.getF32("UISndMoneyChangeThreshold")))
	{
		if (mBalance > balance)
		{
			make_ui_sound("UISndMoneyChangeDown");
		}
		else
		{
			make_ui_sound("UISndMoneyChangeUp");
		}
	}

	if (balance != mBalance)
	{
		mBalanceTimer->reset();
		mBalanceTimer->setTimerExpirySec(ICON_TIMER_EXPIRY);
		mBalance = balance;
	}
}

// static
void LLStatusBar::sendMoneyBalanceRequest()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoneyBalanceRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MoneyData);
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null);
	gAgent.sendReliableMessage();
}

void LLStatusBar::setHealth(S32 health)
{
	//llinfos << "Setting health to: " << buffer << llendl;
	mTextHealth->setText(llformat("%d%%", health));

	if (mHealth > health)
	{
		if (mHealth > health + gSavedSettings.getF32("UISndHealthReductionThreshold"))
		{
			if (isAgentAvatarValid())
			{
				if (gAgentAvatarp->getSex() == SEX_FEMALE)
				{
					make_ui_sound("UISndHealthReductionF");
				}
				else
				{
					make_ui_sound("UISndHealthReductionM");
				}
			}
		}

		mHealthTimer->reset();
		mHealthTimer->setTimerExpirySec(ICON_TIMER_EXPIRY);
	}

	mHealth = health;
}

S32 LLStatusBar::getBalance() const
{
	return mBalance;
}


S32 LLStatusBar::getHealth() const
{
	return mHealth;
}

void LLStatusBar::setLandCredit(S32 credit)
{
	mSquareMetersCredit = credit;
}

void LLStatusBar::setLandCommitted(S32 committed)
{
	mSquareMetersCommitted = committed;
}

BOOL LLStatusBar::isUserTiered() const
{
	return (mSquareMetersCredit > 0);
}

S32 LLStatusBar::getSquareMetersCredit() const
{
	return mSquareMetersCredit;
}

S32 LLStatusBar::getSquareMetersCommitted() const
{
	return mSquareMetersCommitted;
}

S32 LLStatusBar::getSquareMetersLeft() const
{
	return mSquareMetersCredit - mSquareMetersCommitted;
}

static void onClickParcelInfo(void* data)
{
	LLViewerParcelMgr::getInstance()->selectParcelAt(gAgent.getPositionGlobal());
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
	{
		return;
	}
//mk
	LLFloaterLand::showInstance();
}

static void onClickBalance(void*)
{
	LLFloaterBuyCurrency::buyCurrency();
}

static void onClickBuyCurrency(void*)
{
	LLFloaterBuyCurrency::buyCurrency();
}

static void onClickHealth(void*)
{
	LLNotifications::instance().add("NotSafe");
}

static void onClickScriptDebug(void*)
{
	LLFloaterScriptDebug::show(LLUUID::null);
}

static void onClickFly(void*)
{
	LLNotifications::instance().add("NoFly");
}

static void onClickPush(void*)
{
	LLNotifications::instance().add("PushRestricted");
}

static void onClickVoice(void*)
{
	LLNotifications::instance().add("NoVoice");
}

static void onClickSee(void*)
{
	LLNotifications::instance().add("NoSee");
}

static void onClickBuild(void*)
{
	LLNotifications::instance().add("NoBuild");
}

static void onClickNotifications(void*)
{
	LLNotifyBox::setShowNotifications(!LLNotifyBox::areNotificationsShown());
}

static void onClickScripts(void*)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (region && (region->getRegionFlags() & REGION_FLAGS_ESTATE_SKIP_SCRIPTS))
	{
		LLNotifications::instance().add("ScriptsStopped");
	}
	else if (region && (region->getRegionFlags() & REGION_FLAGS_SKIP_SCRIPTS))
	{
		LLNotifications::instance().add("ScriptsNotRunning");
	}
	else
	{
		LLNotifications::instance().add("NoOutsideScripts");
	}
}

static void onClickBuyLand(void*)
{
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
	{
		return;
	}
//mk
	LLViewerParcelMgr::getInstance()->selectParcelAt(gAgent.getPositionGlobal());
	LLViewerParcelMgr::getInstance()->startBuyLand();
}

// static
void LLStatusBar::onCommitSearch(LLUICtrl*, void* data)
{
	// committing is the same as clicking "search"
	onClickSearch(data);
}

// static
void LLStatusBar::onClickSearch(void* data)
{
	LLStatusBar* self = (LLStatusBar*)data;
	std::string search_text = self->mLineEditSearch->getText();
	LLFloaterDirectory::showFindAll(search_text);
}

// static
void LLStatusBar::onClickStatGraph(void* data)
{
	LLFloaterLagMeter::showInstance();
}

BOOL can_afford_transaction(S32 cost)
{
	return (cost <= 0 || (gStatusBar && gStatusBar->getBalance() >= cost));
}

// Implements secondlife:///app/balance/request to request a L$ balance
// update via UDP message system. JC
class LLBalanceHandler : public LLCommandHandler
{
public:
	// Requires "trusted" browser/URL source
	LLBalanceHandler() : LLCommandHandler("balance", true) { }
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (tokens.size() == 1 && tokens[0].asString() == "request")
		{
			LLStatusBar::sendMoneyBalanceRequest();
			return true;
		}
		return false;
	}
};
// register with command dispatch system
LLBalanceHandler gBalanceHandler;
