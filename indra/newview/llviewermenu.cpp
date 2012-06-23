/** 
 * @file llviewermenu.cpp
 * @brief Builds menus out of items.
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

#include "llviewermenu.h" 

// system library includes
#include <iostream>
#include <fstream>
#include <sstream>

// linden library includes
#include "indra_constants.h"
#include "llaudioengine.h"
#include "llavatarnamecache.h"
#include "llassetstorage.h"
#include "llchat.h"
#include "llclipboard.h"
#include "llfeaturemanager.h"
#include "llfocusmgr.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llinstantmessage.h"
#include "llkeyboard.h"
#include "llmenugl.h"
#include "llnotifications.h"
#include "llparcel.h"
#include "llpermissionsflags.h"
#include "llprimitive.h"
#include "llrect.h"
#include "llresmgr.h"
#include "llsecondlifeurls.h"
#include "llsdserialize.h"
#include "llstring.h"
#include "lltimer.h"
#include "lltransactiontypes.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "lluuid.h"
#include "llvfile.h"
#include "llview.h"
#include "llvolume.h"
#include "llvolumemgr.h"
#include "llxfermanager.h"
#include "message.h"
#include "object_flags.h"
#include "raytrace.h"
#include "roles_constants.h"

// newview includes
#include "llagent.h"
#include "llagentpilot.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llappviewer.h"
#include "llbox.h"
#include "llcallingcard.h"
#include "llcompilequeue.h"
#include "llconsole.h"
#include "lldebugview.h"
#include "lldirpicker.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpooltree.h"
#include "llface.h"
#include "llfasttimerview.h"
#include "llfirstuse.h"
#include "llfloaterabout.h"
#include "llfloateractivespeakers.h"
#include "llfloateranimpreview.h"
#include "jcfloaterareasearch.h"
#include "llfloateravatarinfo.h"
#include "llfloateravatarlist.h"
#include "llfloateravatartextures.h"
#include "llfloaterbeacons.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbump.h"
#include "llfloaterbuy.h"
#include "llfloaterbuycontents.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterbuyland.h"
#include "llfloatercamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloatercustomize.h"
#include "llfloaterdaycycle.h"
#include "hbfloaterdebugtags.h"
#include "llfloaterdirectory.h"
#include "llfloaterdisplayname.h"
#include "llfloatereditui.h"
#include "llfloaterenvsettings.h"
#include "llfloaterfriends.h"
#include "llfloaterfonttest.h"
#include "llfloatergesture.h"
#include "llfloatergodtools.h"
#include "llfloatergroupinfo.h"
#include "llfloatergroupinvite.h"
#include "llfloatergroups.h"
#include "hbfloatergrouptitles.h"
#include "llfloaterhtmlcurrency.h"
#include "llfloaterhtmlsimple.h"
#include "llfloaterhud.h"
#include "llfloaterinspect.h"
#include "llfloaterlagmeter.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "llfloatermap.h"
#include "llfloatermediabrowser.h"
#include "slfloatermediafilter.h"
#include "llfloatermemleak.h"
#include "llfloatermute.h"
#include "llfloaternotificationsconsole.h"
#include "llfloateropenobject.h"
#include "llfloaterpay.h"
#include "llfloaterpermissionsmgr.h"
#include "llfloaterperms.h"
#include "llfloaterpostprocess.h"
#include "llfloaterpreference.h"
#include "llfloaterregiondebugconsole.h"
#include "llfloaterregioninfo.h"
#include "llfloaterreporter.h"
#include "hbfloaterrlv.h"
#include "llfloaterscriptdebug.h"
#include "llfloatersettingsdebug.h"
#include "llfloaterstats.h"
#include "llfloaterteleport.h"
#include "llfloaterteleporthistory.h"
#include "llfloatertools.h"
#include "llfloaterwater.h"
#include "llfloaterwindlight.h"
#include "llfloaterworldmap.h"
#include "llframestats.h"
#include "llframestatview.h"
#include "llgroupmgr.h"
#include "llhoverview.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llimview.h"
#include "llinventoryview.h"
#include "llmemoryview.h"
#include "llmenucommands.h"
#include "llmimetypes.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "llmutelist.h"
#include "llpanellogin.h"
#include "llpanelobject.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llstatusbar.h"
#include "llstatview.h"
#include "llsurfacepatch.h"
#include "lltexlayer.h"
#include "lltexturecache.h"
#include "lltextureview.h"
#include "lltool.h"
#include "lltoolbar.h"
#include "lltoolcomp.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltoolplacer.h"
#include "lltoolselectland.h"
#include "lluploaddialog.h"
#include "lluserauth.h"
#include "llvelocitybar.h"
#include "llvieweraudio.h"				// audio_preload_ui_sounds()
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewergenericmessage.h"
#include "llviewergesture.h"
#include "llviewerinventory.h"
#include "llviewerjoystick.h"
#include "llviewermenufile.h"			// init_menu_file()
#include "llviewermessage.h"
#include "llviewernetwork.h"
#include "llviewerobjectbackup.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexturelist.h"		// gTextureList
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwaterparammanager.h"
#include "llweb.h"
#include "llwlanimator.h"
#include "llwlparammanager.h"
#include "llworld.h"
#include "llworldmap.h"
#include "pipeline.h"

using namespace LLOldEvents;
using namespace LLVOAvatarDefines;

void init_client_menu(LLMenuGL* menu);
void init_server_menu(LLMenuGL* menu);

void init_debug_world_menu(LLMenuGL* menu);
void init_debug_rendering_menu(LLMenuGL* menu);
void init_debug_ui_menu(LLMenuGL* menu);
void init_debug_xui_menu(LLMenuGL* menu);
void init_debug_avatar_menu(LLMenuGL* menu);
//MK
void init_restrained_life_menu(LLMenuGL* menu);
//mk
void init_debug_baked_texture_menu(LLMenuGL* menu);

LLVOAvatar* find_avatar_from_object(LLViewerObject* object);
LLVOAvatar* find_avatar_from_object(const LLUUID& object_id);

//
// Evil hackish imported globals
//
extern BOOL gRenderAvatar;
extern BOOL gOcclusionCull;
//
// Globals
//

LLMenuBarGL*			gMenuBarView = NULL;
LLViewerMenuHolderGL*	gMenuHolder = NULL;
LLMenuGL*				gPopupMenuView = NULL;
LLMenuBarGL*			gLoginMenuBarView = NULL;

// Pie menus
LLPieMenu* gPieSelf   = NULL;
LLPieMenu* gPieAvatar = NULL;
LLPieMenu* gPieObject = NULL;
LLPieMenu* gPieAttachment = NULL;
LLPieMenu* gPieLand   = NULL;

// local constants
const std::string CLIENT_MENU_NAME("Advanced");
const std::string SERVER_MENU_NAME("Admin");

LLMenuGL* gAttachSubMenu = NULL;
LLMenuGL* gDetachSubMenu = NULL;
LLMenuGL* gTakeOffClothes = NULL;
LLPieMenu* gAttachScreenPieMenu = NULL;
LLPieMenu* gAttachPieMenu = NULL;
LLPieMenu* gAttachBodyPartPieMenus[8];
LLPieMenu* gDetachPieMenu = NULL;
LLPieMenu* gDetachScreenPieMenu = NULL;
LLPieMenu* gDetachBodyPartPieMenus[8];

LLMenuItemCallGL* gAFKMenu = NULL;
LLMenuItemCallGL* gBusyMenu = NULL;

typedef LLMemberListener<LLView> view_listener_t;

//
// Local prototypes
//
void handle_leave_group(void *);

// Edit menu
void handle_dump_group_info(void *);
void handle_dump_capabilities_info(void *);
void handle_dump_focus(void*);

// Advanced->Consoles menu
void handle_show_notifications_console(void*);
void handle_region_debug_console(void*);
void handle_region_dump_settings(void*);
void handle_region_dump_temp_asset_data(void*);
void handle_region_clear_temp_asset_data(void*);

// Object pie menu
BOOL sitting_on_selection();

void near_sit_object();
void label_sit_or_stand(std::string& label, void*);
// buy and take alias into the same UI positions, so these
// declarations handle this mess.
BOOL is_selection_buy_not_take();
S32 selection_price();
BOOL enable_take();
void handle_take();
bool confirm_take(const LLSD& notification, const LLSD& response);
BOOL enable_buy(void*); 
void handle_buy(void *);
void handle_buy_object(LLSaleInfo sale_info);
void handle_buy_contents(LLSaleInfo sale_info);
void label_touch(std::string& label, void*);

// Land pie menu
void near_sit_down_point(BOOL success, void *);

// Avatar pie menu
void handle_follow(void *userdata);
void handle_talk_to(void *userdata);

// Debug menu
void toggle_build_options(void* user_data);
void reload_ui(void*);
void handle_agent_stop_moving(void*);
void print_packets_lost(void*);
void drop_packet(void*);
void velocity_interpolate(void* data);
void toggle_water_audio(void);
void handle_rebake_textures(void*);
BOOL check_admin_override(void*);
void handle_admin_override_toggle(void*);

void toggle_glow(void *);
BOOL check_glow(void *);

void toggle_vertex_shaders(void *);
BOOL check_vertex_shaders(void *);

void toggle_cull_small(void *);

void toggle_show_xui_names(void *);
BOOL check_show_xui_names(void *);

BOOL enable_picker_actions(void*);

// Debug UI
void handle_web_search_demo(void*);
void handle_web_browser_test(void*);
void handle_save_to_xml(void*);
void handle_load_from_xml(void*);

// God menu
void handle_god_mode(void*);
void handle_leave_god_mode(void*);
BOOL enable_god_options(void*);

BOOL is_inventory_visible(void* user_data);
void handle_reset_view();

void disabled_duplicate(void*);
void handle_duplicate_in_place(void*);
void handle_repeat_duplicate(void*);

void handle_export(void*);
// void handle_deed_object_to_group(void*);
// BOOL enable_deed_object_to_group(void*);
void handle_object_owner_self(void*);
void handle_object_owner_permissive(void*);
void handle_object_lock(void*);
void handle_object_asset_ids(void*);
void force_take_copy(void*);

void handle_force_parcel_owner_to_me(void*);
void handle_force_parcel_to_content(void*);
void handle_claim_public_land(void*);

void handle_god_request_havok(void *);
void handle_god_request_avatar_geometry(void *);	// Hack for easy testing of new avatar geometry
void reload_personal_settings_overrides(void *);
void slow_mo_animations(void *);
BOOL is_slow_mo_animations(void*);
void handle_disconnect_viewer(void *);

void force_error_breakpoint(void *);
void force_error_llerror(void *);
void force_error_bad_memory_access(void *);
void force_error_infinite_loop(void *);
void force_error_software_exception(void *);
void force_error_driver_crash(void *);

void handle_stopall(void*);
#ifdef SEND_HINGES
void handle_hinge(void*);
void handle_ptop(void*);
void handle_lptop(void*);
void handle_wheel(void*);
void handle_dehinge(void*);
BOOL enable_dehinge(void*);
#endif
void handle_force_delete(void*);
void print_object_info(void*);
void print_agent_nvpairs(void*);
void export_info_callback(LLAssetInfo *info, void **user_data, S32 result);
void export_data_callback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type, void **user_data, S32 result);
void upload_done_callback(const LLUUID& uuid, void* user_data, S32 result, LLExtStat ext_status);
BOOL menu_check_build_tool(void* user_data);
void handle_reload_settings(void*);
void focus_here(void*);
void dump_select_mgr(void*);
void dump_volume_mgr(void*);
void dump_inventory(void*);
void decode_ui_sounds(void*);
void toggle_visibility(void*);
BOOL get_visibility(void*);

// Avatar Pie menu
void request_friendship(const LLUUID& agent_id);

// Tools menu
void handle_force_unlock(void*);
void handle_selected_texture_info(void*);
void reload_selected_texture(void*);
void handle_refresh_baked_textures(LLVOAvatar* avatar);
void handle_dump_image_list(void*);

void handle_crash(void*);
void handle_dump_followcam(void*);
BOOL check_message_logging(void*);
void handle_viewer_toggle_message_log(void*);
void handle_send_postcard(void*);
void handle_gestures_old(void*);
void handle_focus(void *);
BOOL enable_buy_land(void*);
void handle_move(void*);
void handle_show_inventory(void*);
void handle_activate(void*);
BOOL enable_activate(void*);

// Help menu
void handle_buy_currency(void*);

void handle_test_male(void *);
void handle_test_female(void *);
void handle_toggle_pg(void*);
void handle_dump_attachments(void *);
void handle_dump_avatar_local_textures(void*);
void handle_avatar_textures(void*);
void handle_grab_baked_texture(void*);
BOOL enable_grab_baked_texture(void*);
void handle_dump_region_object_cache(void*);

BOOL menu_ui_enabled(void *user_data);
BOOL menu_check_control(void* user_data);
void menu_toggle_variable(void* user_data);
BOOL menu_check_variable(void* user_data);
BOOL enable_land_selected(void*);
BOOL enable_more_than_one_selected(void*);
BOOL enable_selection_you_own_all(void*);
BOOL enable_selection_you_own_one(void*);
BOOL enable_save_into_inventory(void*);
BOOL enable_save_into_task_inventory(void*);
BOOL enable_not_thirdperson(void*);
// BOOL enable_export_selected(void *);
BOOL enable_have_card(void*);
BOOL enable_detach(void*);
BOOL enable_region_owner(void*);
void menu_toggle_attached_lights(void* user_data);
void menu_toggle_attached_particles(void* user_data);
void menu_toggle_wind_audio(void* user_data);

class LLMenuParcelObserver : public LLParcelObserver
{
public:
	LLMenuParcelObserver();
	~LLMenuParcelObserver();
	virtual void changed();
};

static LLMenuParcelObserver* gMenuParcelObserver = NULL;

LLMenuParcelObserver::LLMenuParcelObserver()
{
	LLViewerParcelMgr::getInstance()->addObserver(this);
}

LLMenuParcelObserver::~LLMenuParcelObserver()
{
	LLViewerParcelMgr::getInstance()->removeObserver(this);
}

void LLMenuParcelObserver::changed()
{
	gMenuHolder->childSetEnabled("Land Buy Pass", LLPanelLandGeneral::enableBuyPass(NULL));

	BOOL buyable = enable_buy_land(NULL);
	gMenuHolder->childSetEnabled("Land Buy", buyable);
	gMenuHolder->childSetEnabled("Buy Land...", buyable);
}

//-----------------------------------------------------------------------------
// Menu Construction
//-----------------------------------------------------------------------------

// code required to calculate anything about the menus
void pre_init_menus()
{
	// static information
	LLColor4 color;
	color = gColors.getColor("MenuDefaultBgColor");
	LLMenuGL::setDefaultBackgroundColor(color);
	color = gColors.getColor("MenuItemEnabledColor");
	LLMenuItemGL::setEnabledColor(color);
	color = gColors.getColor("MenuItemDisabledColor");
	LLMenuItemGL::setDisabledColor(color);
	color = gColors.getColor("MenuItemHighlightBgColor");
	LLMenuItemGL::setHighlightBGColor(color);
	color = gColors.getColor("MenuItemHighlightFgColor");
	LLMenuItemGL::setHighlightFGColor(color);
}

void initialize_menus();

//-----------------------------------------------------------------------------
// Initialize main menus
//
// HOW TO NAME MENUS:
//
// First Letter Of Each Word Is Capitalized, Even At Or And
//
// Items that lead to dialog boxes end in "..."
//
// Break up groups of more than 6 items with separators
//-----------------------------------------------------------------------------

BOOL enable_picker_actions(void*)
{
	return !LLFilePickerThread::isInUse() && !LLDirPickerThread::isInUse() ? TRUE : FALSE;
}

void set_underclothes_menu_options()
{
	if (gMenuHolder && gAgent.isTeen())
	{
		gMenuHolder->getChild<LLView>("Self Underpants", TRUE)->setVisible(FALSE);
		gMenuHolder->getChild<LLView>("Self Undershirt", TRUE)->setVisible(FALSE);
	}
	if (gMenuBarView && gAgent.isTeen())
	{
		gMenuBarView->getChild<LLView>("Menu Underpants", TRUE)->setVisible(FALSE);
		gMenuBarView->getChild<LLView>("Menu Undershirt", TRUE)->setVisible(FALSE);
	}
}

void init_menus()
{
	S32 top = gViewerWindow->getRootView()->getRect().getHeight();
	S32 width = gViewerWindow->getRootView()->getRect().getWidth();

	//
	// Main menu bar
	//
	gMenuHolder = new LLViewerMenuHolderGL();
	gMenuHolder->setRect(LLRect(0, top, width, 0));
	gMenuHolder->setFollowsAll();

	LLMenuGL::sMenuContainer = gMenuHolder;

	// Initialize actions
	initialize_menus();

	///
	/// Popup menu
	///
	/// The popup menu is now populated by the show_context_menu()
	/// method.

	gPopupMenuView = new LLMenuGL("Popup");
	gPopupMenuView->setVisible(FALSE);
	gMenuHolder->addChild(gPopupMenuView);

	///
	/// Pie menus
	///
	gPieSelf = LLUICtrlFactory::getInstance()->buildPieMenu("menu_pie_self.xml", gMenuHolder);

	// TomY TODO: what shall we do about these?
	gDetachScreenPieMenu = gMenuHolder->getChild<LLPieMenu>("Object Detach HUD", true);
	gDetachPieMenu = gMenuHolder->getChild<LLPieMenu>("Object Detach", true);

	gPieAvatar = LLUICtrlFactory::getInstance()->buildPieMenu("menu_pie_avatar.xml", gMenuHolder);

	gPieObject = LLUICtrlFactory::getInstance()->buildPieMenu("menu_pie_object.xml", gMenuHolder);

	gAttachScreenPieMenu = gMenuHolder->getChild<LLPieMenu>("Object Attach HUD");
	gAttachPieMenu = gMenuHolder->getChild<LLPieMenu>("Object Attach");

	gPieAttachment = LLUICtrlFactory::getInstance()->buildPieMenu("menu_pie_attachment.xml", gMenuHolder);

	gPieLand = LLUICtrlFactory::getInstance()->buildPieMenu("menu_pie_land.xml", gMenuHolder);

	///
	/// set up the colors
	///
	LLColor4 color;

	LLColor4 pie_color = gColors.getColor("PieMenuBgColor");
	gPieSelf->setBackgroundColor(pie_color);
	gPieAvatar->setBackgroundColor(pie_color);
	gPieObject->setBackgroundColor(pie_color);
	gPieAttachment->setBackgroundColor(pie_color);
	gPieLand->setBackgroundColor(pie_color);

	color = gColors.getColor("MenuPopupBgColor");
	gPopupMenuView->setBackgroundColor(color);

	// If we are not in production, use a different color to make it apparent.
	if (LLViewerLogin::getInstance()->isInProductionGrid())
	{
		color = gColors.getColor("MenuBarBgColor");
	}
	else
	{
		color = gColors.getColor("MenuNonProductionBgColor");
	}
	gMenuBarView = (LLMenuBarGL*)LLUICtrlFactory::getInstance()->buildMenu("menu_viewer.xml", gMenuHolder);
	gMenuBarView->setRect(LLRect(0, top, 0, top - MENU_BAR_HEIGHT));
	gMenuBarView->setBackgroundColor(color);

    // gMenuBarView->setItemVisible("Tools", FALSE);
	gMenuBarView->arrange();

	gMenuHolder->addChild(gMenuBarView);

	// menu holder appears on top of menu bar so you can see the menu title
	// flash when an item is triggered (the flash occurs in the holder)
	gViewerWindow->getRootView()->addChild(gMenuHolder);
   
    gViewerWindow->setMenuBackgroundColor(false, 
        LLViewerLogin::getInstance()->isInProductionGrid());

	// Assume L$10 for now, the server will tell us the real cost at login
	const std::string upload_cost("10");
	gMenuHolder->childSetLabelArg("Upload Image", "[COST]", upload_cost);
	gMenuHolder->childSetLabelArg("Upload Sound", "[COST]", upload_cost);
	gMenuHolder->childSetLabelArg("Upload Animation", "[COST]", upload_cost);
	gMenuHolder->childSetLabelArg("Bulk Upload", "[COST]", upload_cost);

	gAFKMenu = gMenuBarView->getChild<LLMenuItemCallGL>("Set Away", TRUE);
	gBusyMenu = gMenuBarView->getChild<LLMenuItemCallGL>("Set Busy", TRUE);
	gAttachSubMenu = gMenuBarView->getChildMenuByName("Attach Object", TRUE);
	gDetachSubMenu = gMenuBarView->getChildMenuByName("Detach Object", TRUE);

	// TomY TODO convert these two
	LLMenuGL*menu;

	menu = new LLMenuGL(CLIENT_MENU_NAME);
	init_client_menu(menu);
	gMenuBarView->appendMenu(menu);
	menu->updateParent(LLMenuGL::sMenuContainer);

	menu = new LLMenuGL(SERVER_MENU_NAME);
	init_server_menu(menu);
	gMenuBarView->appendMenu(menu);
	menu->updateParent(LLMenuGL::sMenuContainer);

	gMenuBarView->createJumpKeys();

	// Let land based option enable when parcel changes
	gMenuParcelObserver = new LLMenuParcelObserver();

	//
	// Debug menu visiblity
	//
	show_debug_menus();

	gLoginMenuBarView = (LLMenuBarGL*)LLUICtrlFactory::getInstance()->buildMenu("menu_login.xml",
																				gMenuHolder);
	// Add the debug settings item to the login menu bar
	menu = new LLMenuGL(CLIENT_MENU_NAME);
	menu->append(new LLMenuItemCallGL("Debug Settings...",
									  LLFloaterSettingsDebug::show, NULL, NULL,
									  'S', MASK_CONTROL|MASK_ALT));
	gLoginMenuBarView->appendMenu(menu);
	menu->updateParent(LLMenuGL::sMenuContainer);

	LLRect menuBarRect = gLoginMenuBarView->getRect();
	gLoginMenuBarView->setRect(LLRect(menuBarRect.mLeft, menuBarRect.mTop,
							   gViewerWindow->getRootView()->getRect().getWidth() - menuBarRect.mLeft,
							   menuBarRect.mBottom));

	gLoginMenuBarView->setBackgroundColor(color);

	gMenuHolder->addChild(gLoginMenuBarView);
}

#if	0	// *TODO: Implement a proper permissions manager floater
void show_permissions_control(void*)
{
	LLFloaterPermissionsMgr::show();
}
#endif

void handle_debug_tags(void*)
{
	HBFloaterDebugTags::showInstance();
}

void init_client_menu(LLMenuGL* menu)
{
	LLMenuGL* sub_menu = NULL;

#if	0	// *TODO: Implement a proper permissions manager floater and add it to
		//		  the View menu.
	menu->append(new LLMenuItemCallGL("Permissions Control", &show_permissions_control));
#endif

	{
		// *TODO: Translate
		LLMenuGL* sub = new LLMenuGL("Consoles");
		menu->appendMenu(sub);
		sub->append(new LLMenuItemCheckGL("Fast Timers", &toggle_visibility,
										  NULL, &get_visibility,
										  (void*)gDebugView->mFastTimerView,
										  '9', MASK_CONTROL|MASK_SHIFT));
		sub->append(new LLMenuItemCheckGL("Frame Console", &toggle_visibility,
										  NULL, &get_visibility,
										  (void*)gDebugView->mFrameStatView,
										  '2', MASK_CONTROL|MASK_SHIFT));
		sub->append(new LLMenuItemCheckGL("Texture Console", &toggle_visibility,
										  NULL, &get_visibility,
										  (void*)gTextureView,
										  '3', MASK_CONTROL|MASK_SHIFT));
		if (gAuditTexture)
		{
			sub->append(new LLMenuItemCheckGL("Texture Size Console", 
											  &toggle_visibility, NULL,
											  &get_visibility,
											  (void*)gTextureSizeView,
											  '5', MASK_CONTROL|MASK_SHIFT));
			sub->append(new LLMenuItemCheckGL("Texture Category Console", 
											  &toggle_visibility, NULL,
											  &get_visibility,
											  (void*)gTextureCategoryView,
											  '6', MASK_CONTROL|MASK_SHIFT));
		}

#if MEM_TRACK_MEM
		sub->append(new LLMenuItemCheckGL("Memory", &toggle_visibility, NULL,
										  &get_visibility,
										  (void*)gDebugView->mMemoryView,
										  '0', MASK_CONTROL|MASK_SHIFT));
#endif
		sub->appendSeparator();

		LLView* debugview = gDebugView->mDebugConsolep;
		sub->append(new LLMenuItemCheckGL("Debug Console", &toggle_visibility,
										  NULL, &get_visibility,
										  debugview,
										  '4', MASK_CONTROL|MASK_SHIFT));
		sub->append(new LLMenuItemCallGL("Debug tags", &handle_debug_tags, NULL));
		{
			LLMenuGL* sub2 = new LLMenuGL("Info to Debug Console");
			sub->appendMenu(sub2);
			sub2->append(new LLMenuItemCallGL("Region Info", 
											  &handle_region_dump_settings));
			sub2->append(new LLMenuItemCallGL("Capabilities Info",
											  &handle_dump_capabilities_info));
			sub2->append(new LLMenuItemCallGL("Group Info",
											  &handle_dump_group_info));
			sub2->append(new LLMenuItemCallGL("Packets Lost Info",
											  &print_packets_lost));
			sub2->append(new LLMenuItemCallGL("Dump Inventory",
											  &dump_inventory));
			sub2->append(new LLMenuItemCallGL("Dump SelectMgr",
											  &dump_select_mgr));
			sub2->append(new LLMenuItemCallGL("Dump Focus Holder",
											  &handle_dump_focus, NULL, NULL,
											  'F', MASK_ALT | MASK_CONTROL));
			sub2->append(new LLMenuItemCallGL("Selected Object Info",
											  &print_object_info, NULL, NULL,
											  'P', MASK_CONTROL | MASK_SHIFT));
			sub2->append(new LLMenuItemCallGL("Agent Info",
											  &print_agent_nvpairs, NULL, NULL,
											  'P', MASK_SHIFT));
			sub2->append(new LLMenuItemCallGL("Memory Stats", &output_statistics));
			sub2->createJumpKeys();
		}
		sub->appendSeparator();

		// Debugging view for unified notifications
		sub->append(new LLMenuItemCallGL("Notifications Console...",
					&handle_show_notifications_console, NULL, NULL, '5', MASK_CONTROL|MASK_SHIFT));
		sub->append(new LLMenuItemCallGL("Region Debug Console", 
					&handle_region_debug_console, NULL, NULL, 'C', MASK_CONTROL|MASK_SHIFT));

		sub->createJumpKeys();
	}

	sub_menu = new LLMenuGL("HUD Info");
	{
		sub_menu->append(new LLMenuItemCheckGL("Show Velocity Info", 
												&toggle_visibility,
												NULL,
												&get_visibility,
												(void*)gVelocityBar));

		sub_menu->append(new LLMenuItemToggleGL("Show Camera Info", &gDisplayCameraPos));
		sub_menu->append(new LLMenuItemToggleGL("Show Wind Info", &gDisplayWindInfo));
		sub_menu->append(new LLMenuItemToggleGL("Show FOV Info", &gDisplayFOV));
		sub_menu->append(new LLMenuItemCheckGL("Show Time", menu_toggle_control, NULL, menu_check_control, (void*)"DebugShowTime"));
		sub_menu->append(new LLMenuItemCheckGL("Show Render Info", menu_toggle_control, NULL, menu_check_control, (void*)"DebugShowRenderInfo"));
		sub_menu->append(new LLMenuItemCheckGL("Show Mesh Queue", menu_toggle_control, NULL, menu_check_control, (void*)"DebugShowMeshQueue"));
		sub_menu->append(new LLMenuItemCheckGL("Show Matrices", menu_toggle_control, NULL, menu_check_control, (void*)"DebugShowRenderMatrices"));
		sub_menu->append(new LLMenuItemCheckGL("Show Color Under Cursor", menu_toggle_control, NULL, menu_check_control, (void*)"DebugShowColor"));
		sub_menu->createJumpKeys();
	}
	menu->appendMenu(sub_menu);

	menu->appendSeparator();

	menu->append(new LLMenuItemCheckGL("High-res Snapshot",
									   &menu_toggle_control, NULL,
									   &menu_check_control,
									   (void*)"HighResSnapshot"));

	menu->append(new LLMenuItemCheckGL("Quiet Snapshots to Disk",
									   &menu_toggle_control, NULL,
									   &menu_check_control,
									   (void*)"QuietSnapshotsToDisk"));

	menu->append(new LLMenuItemCheckGL("Show Mouselook Crosshairs",
									   &menu_toggle_control, NULL,
									   &menu_check_control,
									   (void*)"ShowCrosshairs"));

	menu->append(new LLMenuItemCheckGL("Debug Permissions",
									   &menu_toggle_control, NULL,
									   &menu_check_control,
									   (void*)"DebugPermissions"));

	menu->appendSeparator();

	sub_menu = new LLMenuGL("Rendering");
	init_debug_rendering_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("World");
	init_debug_world_menu(sub_menu);
	menu->appendMenu(sub_menu);

	// only include region teleport UI if we are using agent domain
	if (gSavedSettings.getBOOL("OpenGridProtocol"))
	{
		sub_menu = new LLMenuGL("Interop");
		sub_menu->append(new LLMenuItemCallGL("Teleport Region...",
						 &LLFloaterTeleport::show, NULL, NULL));
		menu->appendMenu(sub_menu);
	}
	sub_menu = new LLMenuGL("UI");
	init_debug_ui_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("XUI");
	init_debug_xui_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("Character");
	init_debug_avatar_menu(sub_menu);
	menu->appendMenu(sub_menu);
//MK
	if (gRRenabled)
	{
		sub_menu = new LLMenuGL("RestrainedLove");
		init_restrained_life_menu(sub_menu);
		menu->appendMenu(sub_menu);
	}
//mk
	{
		LLMenuGL* sub = NULL;
		sub = new LLMenuGL("Network");

		sub->append(new LLMenuItemCheckGL("Use Web Map Tiles",
										  menu_toggle_control,
										  NULL, menu_check_control,
										  (void*)"UseWebMapTiles"));

		sub->append(new LLMenuItemCheckGL("Use HTTP Inventory Fetches",
										  menu_toggle_control,
										  NULL, menu_check_control,
										  (void*)"UseHTTPInventory"));

		sub->appendSeparator();

		sub->append(new LLMenuItemCheckGL("Server Messages Logging",  
					&handle_viewer_toggle_message_log,  NULL,
					&check_message_logging, NULL));

		sub->appendSeparator();

		sub->append(new LLMenuItemCheckGL("Velocity Interpolate Objects", 
										  &velocity_interpolate, NULL, 
										  &menu_check_control,
										  (void*)"VelocityInterpolate"));
		sub->append(new LLMenuItemCheckGL("Ping Interpolate Object Positions", 
										  &menu_toggle_control, NULL, 
										  &menu_check_control,
										  (void*)"PingInterpolate"));

		sub->appendSeparator();

		sub->append(new LLMenuItemCheckGL("Multi-Threaded Curl (after restart)", 
										  &menu_toggle_control, NULL, 
										  &menu_check_control,
										  (void*)"CurlUseMultipleThreads"));

		sub->appendSeparator();

		sub->append(new LLMenuItemCallGL("Drop a Packet", &drop_packet, NULL,
										 NULL, 'L', MASK_ALT | MASK_CONTROL));

		menu->appendMenu(sub);
		sub->createJumpKeys();
	}
	{
		LLMenuGL* sub = NULL;
		sub = new LLMenuGL("Caches");

		sub->append(new LLMenuItemCallGL("Clear Group Cache", 
										 LLGroupMgr::debugClearAllGroups));

		sub->append(new LLMenuItemCheckGL("Time-Sliced Texture Cache Purges",
										  menu_toggle_control, NULL,
										  menu_check_control,
										  (void*)"CachePurgeTimeSliced"));
		menu->appendMenu(sub);
		sub->createJumpKeys();
	}
	{
		LLMenuGL* sub = NULL;
		sub = new LLMenuGL("Recorder");

		sub->append(new LLMenuItemCheckGL("Full Session Logging", &menu_toggle_control, NULL, &menu_check_control, (void*)"StatsSessionTrackFrameStats"));

		sub->append(new LLMenuItemCallGL("Start Logging", &LLFrameStats::startLogging, NULL));
		sub->append(new LLMenuItemCallGL("Stop Logging", &LLFrameStats::stopLogging, NULL));
		sub->append(new LLMenuItemCallGL("Log 10 Seconds", &LLFrameStats::timedLogging10, NULL));
		sub->append(new LLMenuItemCallGL("Log 30 Seconds", &LLFrameStats::timedLogging30, NULL));
		sub->append(new LLMenuItemCallGL("Log 60 Seconds", &LLFrameStats::timedLogging60, NULL));
		sub->appendSeparator();
		sub->append(new LLMenuItemCallGL("Start Playback", &LLAgentPilot::startPlayback, NULL));
		sub->append(new LLMenuItemCallGL("Stop Playback", &LLAgentPilot::stopPlayback, NULL));
		sub->append(new LLMenuItemToggleGL("Loop Playback", &LLAgentPilot::sLoop));
		sub->append(new LLMenuItemCallGL("Start Record", &LLAgentPilot::startRecord, NULL));
		sub->append(new LLMenuItemCallGL("Stop Record", &LLAgentPilot::saveRecord, NULL));

		menu->appendMenu(sub);
		sub->createJumpKeys();
	}
	{
		LLMenuGL* sub = NULL;
		sub = new LLMenuGL("Media");
		sub->append(new LLMenuItemCallGL("Reload MIME types", &LLMIMETypes::reload));
		sub->append(new LLMenuItemCallGL("Web Browser Test", &handle_web_browser_test));
		menu->appendMenu(sub);
		sub->createJumpKeys();
	}

	menu->appendSeparator();

	menu->append(new LLMenuItemCheckGL("Memory Usage Safety Check",
									   &menu_toggle_control, NULL,
									   &menu_check_control,
									   (void*)"MainMemorySafetyCheck"));

	menu->append(new LLMenuItemCheckGL("Allow Swapping",
									   &menu_toggle_control, NULL,
									   &menu_check_control,
									   (void*)"AllowSwapping"));

	menu->appendSeparator();

	menu->append(new LLMenuItemToggleGL("Show Updates", &gShowObjectUpdates));

	menu->appendSeparator(); 

	menu->append(new LLMenuItemCallGL("Compress Images to JPEG2000...",
									  &handle_compress_image,
									  &enable_picker_actions, NULL));

	menu->append(new LLMenuItemCheckGL("Limit Select Distance", 
									   &menu_toggle_control, NULL, 
									   &menu_check_control,
									   (void*)"LimitSelectDistance"));

	menu->append(new LLMenuItemCheckGL("Disable Camera Constraints", 
									   &menu_toggle_control, NULL, 
									   &menu_check_control,
									   (void*)"DisableCameraConstraints"));

	menu->append(new LLMenuItemCheckGL("Mouse Smoothing",
									   &menu_toggle_control, NULL,
									   &menu_check_control,
									   (void*)"MouseSmooth"));
	menu->appendSeparator();

#if LL_WINDOWS
	menu->append(new LLMenuItemCheckGL("Console Window", 
									   &menu_toggle_control, NULL, 
									   &menu_check_control,
									   (void*)"ShowConsoleWindow"));
#endif

	if (gSavedSettings.getBOOL("QAMode"))
	{
		LLMenuGL* sub = NULL;
		sub = new LLMenuGL("Debugging");
#if LL_WINDOWS
        sub->append(new LLMenuItemCallGL("Force Breakpoint",
										 &force_error_breakpoint, NULL, NULL,
										 'B', MASK_CONTROL | MASK_ALT));
#endif
		sub->append(new LLMenuItemCallGL("Force LLError And Crash", &force_error_llerror));
        sub->append(new LLMenuItemCallGL("Force Bad Memory Access", &force_error_bad_memory_access));
		sub->append(new LLMenuItemCallGL("Force Infinite Loop", &force_error_infinite_loop));
		sub->append(new LLMenuItemCallGL("Force Driver Crash", &force_error_driver_crash));
		sub->append(new LLMenuItemCallGL("Force Disconnect Viewer", &handle_disconnect_viewer));
#if 0	// *NOTE:Mani this isn't handled yet...
		sub->append(new LLMenuItemCallGL("Force Software Exception", &force_error_unhandled_exception));
#endif
		sub->createJumpKeys();
		menu->appendMenu(sub);
	}

	menu->append(new LLMenuItemCheckGL("Output Debug Minidump", 
									   &menu_toggle_control, NULL, 
									   &menu_check_control,
									   (void*)"SaveMinidump"));

	menu->append(new LLMenuItemCallGL("Debug Settings...",
									  LLFloaterSettingsDebug::show, NULL, NULL,
									  'S', MASK_ALT | MASK_CONTROL));
#if 0	// neither of these works particularly well at the moment
	menu->append(new LLMenuItemCallGL("Reload UI XML", &reload_ui, NULL, NULL));
	menu->append(new LLMenuItemCallGL("Reload settings/colors",
				 &handle_reload_settings, NULL, NULL));
#endif
	menu->append(new LLMenuItemCallGL("Reload personal setting overrides", 
				 &reload_personal_settings_overrides, NULL, NULL, KEY_F10,
				 MASK_CONTROL|MASK_SHIFT));

	menu->append(new LLMenuItemCheckGL("View Admin Options",
									   &handle_admin_override_toggle, NULL,
									   &check_admin_override, NULL,
									   'V', MASK_CONTROL | MASK_ALT));

	menu->append(new LLMenuItemCallGL("Request Admin Status",
									  &handle_god_mode,
									  &enable_god_options, NULL,
									  'G', MASK_ALT | MASK_CONTROL));

	menu->append(new LLMenuItemCallGL("Leave Admin Status",
									  &handle_leave_god_mode,
									  &enable_god_options, NULL,
									  'G', MASK_ALT | MASK_SHIFT | MASK_CONTROL));

	menu->createJumpKeys();
}

void init_debug_world_menu(LLMenuGL* menu)
{
	menu->append(new LLMenuItemCheckGL("Sim Sun Override", 
									   &menu_toggle_control,
									   NULL, 
									   &menu_check_control,
									   (void*)"SkyOverrideSimSunPosition"));
	menu->append(new LLMenuItemCallGL("Dump Scripted Camera",
		&handle_dump_followcam, NULL, NULL));
	menu->append(new LLMenuItemCheckGL("Fixed Weather", 
									   &menu_toggle_control,
									   NULL, 
									   &menu_check_control,
									   (void*)"FixedWeather"));
	menu->append(new LLMenuItemCheckGL("Disable Wind Audio", 
									   &menu_toggle_wind_audio,
									   NULL, 
									   &menu_check_control,
									   (void*)"DisableWindAudio"));
	menu->append(new LLMenuItemCallGL("Dump Region Object Cache",
		&handle_dump_region_object_cache, NULL, NULL));
	menu->createJumpKeys();
}

void export_menus_to_xml_callback(LLFilePicker::ESaveFilter type,
								  std::string& filename,
								  void* user_data)
{
	if (!filename.empty())
	{
		llofstream out(filename);
		LLXMLNodePtr node = gMenuBarView->getXML();
		node->writeToOstream(out);
		out.close();
	}
}

void handle_export_menus_to_xml(void*)
{
	// Open the file save dialog
	(new LLSaveFilePicker(LLFilePicker::FFSAVE_XML,
						  export_menus_to_xml_callback))->getSaveFile("menu_bar.xml");
}

extern BOOL gDebugClicks;
extern BOOL gDebugWindowProc;
extern BOOL gDebugTextEditorTips;

#if 0	// This has no real action: obviously editable UI was never implemented
void edit_ui(void*)
{
	LLFloater::setEditModeEnabled(!LLFloater::getEditModeEnabled());
}
#endif

void init_debug_ui_menu(LLMenuGL* menu)
{
#if !LL_DARWIN
	menu->append(new LLMenuItemCheckGL("Use a non-blocking file picker", menu_toggle_control, NULL, menu_check_control, (void*)"NonBlockingFilePicker"));
#endif
	menu->append(new LLMenuItemCheckGL("Use default system color picker", menu_toggle_control, NULL, menu_check_control, (void*)"UseDefaultColorPicker"));
	menu->append(new LLMenuItemCheckGL("Show search panel in overlay bar", menu_toggle_control, NULL, menu_check_control, (void*)"ShowSearchBar"));
	menu->appendSeparator();

#if 0	// This has no real action: obviously editable UI was never implemented
	menu->append(new LLMenuItemCallGL("Editable UI", &edit_ui));
	menu->appendSeparator();
#endif

	menu->append(new LLMenuItemCallGL("Decode all UI sounds", &decode_ui_sounds));
	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Zoom Dependent Resize Handles", menu_toggle_control, NULL, menu_check_control, (void*)"ZoomDependentResizeHandles"));
	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Debug SelectMgr", menu_toggle_control, NULL, menu_check_control, (void*)"DebugSelectMgr"));
	menu->append(new LLMenuItemToggleGL("Debug Clicks", &gDebugClicks));
	menu->append(new LLMenuItemToggleGL("Debug Views", &LLView::sDebugRects));
	menu->append(new LLMenuItemCheckGL("Show Name Tooltips", toggle_show_xui_names, NULL, check_show_xui_names, NULL));
	menu->append(new LLMenuItemToggleGL("Debug Mouse Events", &LLView::sDebugMouseHandling));
	menu->append(new LLMenuItemToggleGL("Debug Keys", &LLView::sDebugKeys));
	menu->append(new LLMenuItemToggleGL("Debug WindowProc", &gDebugWindowProc));
	menu->append(new LLMenuItemToggleGL("Debug Text Editor Tips", &gDebugTextEditorTips));

	menu->createJumpKeys();
}

void init_debug_xui_menu(LLMenuGL* menu)
{
	menu->append(new LLMenuItemCallGL("Font Test...", LLFloaterFontTest::show));
	menu->append(new LLMenuItemCallGL("Export Menus to XML...", handle_export_menus_to_xml, &enable_picker_actions, NULL));
	menu->append(new LLMenuItemCallGL("Edit UI...", LLFloaterEditUI::show));
	menu->append(new LLMenuItemCallGL("Load from XML...", handle_load_from_xml, &enable_picker_actions, NULL));
	menu->append(new LLMenuItemCallGL("Save to XML...", handle_save_to_xml, &enable_picker_actions, NULL));
	menu->append(new LLMenuItemCheckGL("Show XUI Names", toggle_show_xui_names, NULL, check_show_xui_names, NULL));

	//menu->append(new LLMenuItemCallGL("Buy Currency...", handle_buy_currency));
	menu->createJumpKeys();
}

BOOL deferred_rendering_enabled(void* user_data)
{
	std::string setting(static_cast<char*>(user_data));
	BOOL deferred = gSavedSettings.getBOOL("RenderDeferred");
	if (setting == "RenderDeferredGI")
	{
		return deferred && gSavedSettings.getBOOL("RenderDeferredSSAO");
	}
	else
	{
		return deferred;
	}
}

BOOL force_mesh_deformer_enabled(void* user_data)
{
	return gSavedSettings.getBOOL("MeshEnableDeformer");
}

BOOL wireframe_check(void* user_data)
{
	return gUseWireframe;
}

void handle_toggle_wireframe(void*)
{
	gUseWireframe = !gUseWireframe;
	LLPipeline::updateRenderDeferred();
	gPipeline.resetVertexBuffers();
}

void init_debug_rendering_menu(LLMenuGL* menu)
{
	LLMenuGL* sub_menu = NULL;

	///////////////////////////
	//
	// Debug menu for types/pools
	//
	sub_menu = new LLMenuGL("Types");
	menu->appendMenu(sub_menu);

	sub_menu->append(new LLMenuItemCheckGL("Simple",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_SIMPLE,	'1', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Alpha",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_ALPHA, '2', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Tree",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_TREE, '3', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Character",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_AVATAR, '4', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("SurfacePatch",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_TERRAIN, '5', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Sky",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_SKY, '6', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Water",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_WATER, '7', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Ground",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_GROUND, '8', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Volume",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_VOLUME, '9', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Grass",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_GRASS, '0', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Clouds",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_CLOUDS, '-', MASK_CONTROL|MASK_ALT| MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Particles",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_PARTICLES, '=', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Bump",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_BUMP, '\\', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->createJumpKeys();
	sub_menu = new LLMenuGL("Features");
	menu->appendMenu(sub_menu);
	sub_menu->append(new LLMenuItemCheckGL("UI",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_UI, KEY_F1, MASK_SHIFT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Selected",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_SELECTED, KEY_F2, MASK_SHIFT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Highlighted",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_HIGHLIGHTED, KEY_F3, MASK_SHIFT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Dynamic Textures",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES, KEY_F4, MASK_SHIFT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Foot Shadows", 
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_FOOT_SHADOWS, KEY_F5, MASK_SHIFT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Fog",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_FOG, KEY_F6, MASK_SHIFT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Test FRInfo",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_FR_INFO, KEY_F8, MASK_SHIFT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Flexible Objects", 
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_FLEXIBLE, KEY_F9, MASK_SHIFT|MASK_CONTROL));
	sub_menu->createJumpKeys();

	/////////////////////////////
	//
	// Debug menu for info displays
	//
	sub_menu = new LLMenuGL("Info Displays");
	menu->appendMenu(sub_menu);

	sub_menu->append(new LLMenuItemCheckGL("Verify",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_VERIFY));
	sub_menu->append(new LLMenuItemCheckGL("BBoxes",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_BBOXES));
	sub_menu->append(new LLMenuItemCheckGL("Normals",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_NORMALS));
	sub_menu->append(new LLMenuItemCheckGL("Points",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_POINTS));
	sub_menu->append(new LLMenuItemCheckGL("Octree",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_OCTREE));
	sub_menu->append(new LLMenuItemCheckGL("Shadow Frusta",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA));
	sub_menu->append(new LLMenuItemCheckGL("Physics Shapes",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_PHYSICS_SHAPES));
	sub_menu->append(new LLMenuItemCheckGL("Occlusion",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_OCCLUSION));
	sub_menu->append(new LLMenuItemCheckGL("Render Batches",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_BATCH_SIZE));
	sub_menu->append(new LLMenuItemCheckGL("Update Type",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_UPDATE_TYPE));
	sub_menu->append(new LLMenuItemCheckGL("Animated Textures",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_TEXTURE_ANIM));
	sub_menu->append(new LLMenuItemCheckGL("Texture Priority",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY));
	sub_menu->append(new LLMenuItemCheckGL("Avatar Rendering Cost",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_SHAME));
	sub_menu->append(new LLMenuItemCheckGL("Texture Area (sqrt(A))",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_TEXTURE_AREA));
	sub_menu->append(new LLMenuItemCheckGL("Face Area (sqrt(A))",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_FACE_AREA));
	sub_menu->append(new LLMenuItemCheckGL("LOD Info",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_LOD_INFO));
	sub_menu->append(new LLMenuItemCheckGL("Build Queue",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_BUILD_QUEUE));
	sub_menu->append(new LLMenuItemCheckGL("Lights",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_LIGHTS));
	sub_menu->append(new LLMenuItemCheckGL("Particles",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_PARTICLES));
	sub_menu->append(new LLMenuItemCheckGL("Composition",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_COMPOSITION));
	sub_menu->append(new LLMenuItemCheckGL("Glow",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_GLOW));
	sub_menu->append(new LLMenuItemCheckGL("Raycasting",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_RAYCAST));
	sub_menu->append(new LLMenuItemCheckGL("Sculpt",
										   &LLPipeline::toggleRenderDebug, NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_SCULPTED));

	sub_menu = new LLMenuGL("Render Tests");

	sub_menu->append(new LLMenuItemCheckGL("Camera Offset", 
										  &menu_toggle_control,
										  NULL, 
										  &menu_check_control,
										  (void*)"CameraOffset"));

	sub_menu->append(new LLMenuItemToggleGL("Randomize Framerate", &gRandomizeFramerate));

	sub_menu->append(new LLMenuItemToggleGL("Periodic Slow Frame", &gPeriodicSlowFrame));

	sub_menu->append(new LLMenuItemToggleGL("Frame Test", &LLPipeline::sRenderFrameTest));

	sub_menu->createJumpKeys();

	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("Deferred Rendering");

	sub_menu->append(new LLMenuItemCheckGL("Deferred Rendering", 
										  &menu_toggle_control,
										  NULL, 
										  &menu_check_control,
										  (void*)"RenderDeferred"));

	sub_menu->append(new LLMenuItemCheckGL("Depth Of Field", 
										  &menu_toggle_control,
										  &deferred_rendering_enabled, 
										  &menu_check_control,
										  (void*)"RenderDepthOfField"));

	sub_menu->append(new LLMenuItemCheckGL("Screen Space Ambiant Occlusion", 
										  &menu_toggle_control,
										  &deferred_rendering_enabled, 
										  &menu_check_control,
										  (void*)"RenderDeferredSSAO"));

	sub_menu->append(new LLMenuItemCheckGL("Global Illumination", 
										  &menu_toggle_control,
										  &deferred_rendering_enabled, 
										  &menu_check_control,
										  (void*)"RenderDeferredGI"));

	sub_menu->createJumpKeys();

	menu->appendMenu(sub_menu);

	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Axes", menu_toggle_control, NULL, menu_check_control, (void*)"ShowAxes"));

	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Hide Selected Objects", menu_toggle_control, NULL, menu_check_control, (void*)"HideSelectedObjects"));
	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Tangent Basis", menu_toggle_control, NULL, menu_check_control, (void*)"ShowTangentBasis"));
	menu->append(new LLMenuItemCallGL("Selected Texture Info", handle_selected_texture_info, NULL, NULL, 'T', MASK_CONTROL|MASK_SHIFT|MASK_ALT));
	menu->append(new LLMenuItemCallGL("Reload Selected Texture", reload_selected_texture, NULL, NULL, 'U', MASK_CONTROL|MASK_SHIFT));
	//menu->append(new LLMenuItemCallGL("Dump Image List", handle_dump_image_list, NULL, NULL, 'I', MASK_CONTROL|MASK_SHIFT));

	menu->append(new LLMenuItemCheckGL("Wireframe", handle_toggle_wireframe, NULL, wireframe_check, NULL, 'R', MASK_CONTROL|MASK_SHIFT));

	LLMenuItemCheckGL* item;
	item = new LLMenuItemCheckGL("Debug GL", menu_toggle_control, NULL, menu_check_control, (void*)"RenderDebugGL");
	menu->append(item);

	item = new LLMenuItemCheckGL("Debug Pipeline", menu_toggle_control, NULL, menu_check_control, (void*)"RenderDebugPipeline");
	menu->append(item);

	item = new LLMenuItemCheckGL("Automatic Alpha Masks (non-deferred)", menu_toggle_control, NULL, menu_check_control, (void*)"RenderAutoMaskAlphaNonDeferred");
	menu->append(item);

	item = new LLMenuItemCheckGL("Automatic Alpha Masks (deferred)", menu_toggle_control, NULL, menu_check_control, (void*)"RenderAutoMaskAlphaDeferred");
	menu->append(item);

	item = new LLMenuItemCheckGL("Animate Textures", menu_toggle_control, NULL, menu_check_control, (void*)"AnimateTextures");
	menu->append(item);

	item = new LLMenuItemCheckGL("Disable Textures", menu_toggle_variable, NULL, menu_check_variable, (void*)&LLViewerTexture::sDontLoadVolumeTextures);
	menu->append(item);

	item = new LLMenuItemCheckGL("Load Textures Progressively", menu_toggle_control, NULL, menu_check_control, (void*)"TextureProgressiveLoad");
	menu->append(item);
#if 0	// This setting is way too dangerous to expose it to anyone...
	item = new LLMenuItemCheckGL("Full Res Textures", menu_toggle_control, NULL, menu_check_control, (void*)"TextureLoadFullRes");
	menu->append(item);
#endif
	item = new LLMenuItemCheckGL("Boost Attachments Textures", menu_toggle_control, NULL, menu_check_control, (void*)"TextureBoostAttachments");
	menu->append(item);

	item = new LLMenuItemCheckGL("Run Multiple Threads", menu_toggle_control, NULL, menu_check_control, (void*)"RunMultipleThreads");
	menu->append(item);

	item = new LLMenuItemCheckGL("Cheesy Beacon", menu_toggle_control, NULL, menu_check_control, (void*)"CheesyBeacon");
	menu->append(item);

	item = new LLMenuItemCheckGL("Attached Lights", menu_toggle_attached_lights, NULL, menu_check_control, (void*)"RenderAttachedLights");
	menu->append(item);

	item = new LLMenuItemCheckGL("Attached Particles", menu_toggle_attached_particles, NULL, menu_check_control, (void*)"RenderAttachedParticles");
	menu->append(item);

	std::string audit_label = gAuditTexture ? "Audit Texture" : "Audit Texture (after restart)";
	item = new LLMenuItemCheckGL(audit_label, menu_toggle_control, NULL, menu_check_control, (void*)"AuditTexture");
	menu->append(item);

	menu->appendSeparator();

	item = new LLMenuItemCheckGL("Load Meshes At Max LOD", menu_toggle_control, NULL, menu_check_control, (void*)"MeshLoadHighLOD");
	menu->append(item);

	item = new LLMenuItemCheckGL("Enable Mesh Deformer", menu_toggle_control, NULL, menu_check_control, (void*)"MeshEnableDeformer");
	menu->append(item);

	item = new LLMenuItemCheckGL("Force Mesh Deformer", menu_toggle_control, force_mesh_deformer_enabled, menu_check_control, (void*)"MeshForceDeformer");
	menu->append(item);

	if (gSavedSettings.getBOOL("QAMode"))
	{
		menu->appendSeparator();
		menu->append(new LLMenuItemCallGL("Memory Leaking Simulation", LLFloaterMemLeak::show, NULL, NULL));
	}

	menu->createJumpKeys();
}

void handle_rebuild_avatar(void*)
{
	if (isAgentAvatarValid())
	{
		gViewerWindow->stopGL();
		gAgentAvatarp->buildCharacter();
		gViewerWindow->restoreGL("Rebuilding your avatar...");
	}
}

void init_debug_avatar_menu(LLMenuGL* menu)
{
	LLMenuGL* sub_menu = new LLMenuGL("Grab Baked Texture");
	init_debug_baked_texture_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("Character Tests");

	// HACK for easy testing of avatar geometry
	sub_menu->append(new LLMenuItemCallGL("Toggle Character Geometry",
										  &handle_god_request_avatar_geometry,
										  &enable_god_customer_service, NULL));

	sub_menu->append(new LLMenuItemCallGL("Test Male", handle_test_male));

	sub_menu->append(new LLMenuItemCallGL("Test Female", handle_test_female));

	sub_menu->append(new LLMenuItemCallGL("Force Params to Default",
										  &LLAgent::clearVisualParams, NULL));

	sub_menu->append(new LLMenuItemCallGL("Toggle PG", handle_toggle_pg));
#if 0	// This does not work at all...
	sub_menu->append(new LLMenuItemCheckGL("Allow Select Avatar",
										   menu_toggle_control, NULL,
										   menu_check_control,
										  (void*)"AllowSelectAvatar"));
#endif
	sub_menu->createJumpKeys();
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("Character Debugging");
	sub_menu->append(new LLMenuItemToggleGL("Debug Joint Updates",
											&LLVOAvatar::sJointDebug));
	sub_menu->append(new LLMenuItemToggleGL("Debug Character Visibility",
											&LLVOAvatar::sDebugInvisible));
	sub_menu->append(new LLMenuItemCheckGL("Show Collision Skeleton",
										   &LLPipeline::toggleRenderDebug,
										   NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_AVATAR_VOLUME));
#if 0	// Disabling collision plane due to DEV-14477 -brad
	sub_menu->append(new LLMenuItemToggleGL("Show Collision Plane",
											&LLVOAvatar::sShowFootPlane));
#endif
#if 0	// This doesn't seem to produce any visible result...
	sub_menu->append(new LLMenuItemToggleGL("Show Attachment Points",
											&LLVOAvatar::sShowAttachmentPoints));
#endif
	sub_menu->append(new LLMenuItemCheckGL("Display Agent Target",
										   &LLPipeline::toggleRenderDebug,
										   NULL,
										   &LLPipeline::toggleRenderDebugControl,
										   (void*)LLPipeline::RENDER_DEBUG_AGENT_TARGET));
	sub_menu->append(new LLMenuItemCallGL("Dump Attachments",
										  &handle_dump_attachments));
	sub_menu->append(new LLMenuItemCallGL("Dump Local Textures",
										  &handle_dump_avatar_local_textures,
										  &enable_non_faked_god, NULL));
	sub_menu->createJumpKeys();
	menu->appendMenu(sub_menu);

	menu->appendSeparator();
	menu->append(new LLMenuItemCallGL("Appearance To XML...",
									  &LLVOAvatar::dumpArchetypeXML,
									  &enable_picker_actions, NULL));
	menu->append(new LLMenuItemCallGL("Rebuild avatar",
									  handle_rebuild_avatar));
	menu->appendSeparator();
	menu->append(new LLMenuItemToggleGL("Tap-Tap-Hold To Run",
										&gAllowTapTapHoldRun));
	menu->append(new LLMenuItemCheckGL("Spoof Mouse-Look Mode",
									   menu_toggle_control, NULL,
									   menu_check_control,
									   (void*)"SpoofMouseLook",
									   'M',
									   MASK_SHIFT | MASK_ALT | MASK_CONTROL));
	menu->appendSeparator();
	menu->append(new LLMenuItemToggleGL("Animation Info",
										&LLVOAvatar::sShowAnimationDebug));
	menu->append(new LLMenuItemCheckGL("Slow Motion Animations",
									   &slow_mo_animations, NULL,
									   &is_slow_mo_animations, NULL));
	menu->appendSeparator();
	menu->append(new LLMenuItemToggleGL("Show Look At",
										&LLHUDEffectLookAt::sDebugLookAt));
	menu->append(new LLMenuItemToggleGL("Show Point At",
										&LLHUDEffectPointAt::sDebugPointAt));
	menu->appendSeparator();
	menu->append(new LLMenuItemToggleGL("Disable LOD",
										&LLViewerJoint::sDisableLOD));
	menu->appendSeparator();
	menu->append(new LLMenuItemCallGL("Rebake Textures",
									  &handle_rebake_textures, NULL, NULL, 'R',
									  MASK_ALT | MASK_CONTROL));
	menu->append(new LLMenuItemCallGL("View Avatar Textures",
									  &handle_avatar_textures,
									  &enable_non_faked_god, NULL));
	menu->createJumpKeys();
}

//MK
void handle_restrictions_list(void*)
{
	HBFloaterRLV::showInstance();
}

void init_restrained_life_menu(LLMenuGL* menu)
{
	menu->append(new LLMenuItemCallGL("Restrictions list", &handle_restrictions_list, NULL));
	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Allow Wear and Add to/Replace Outfit", menu_toggle_control, NULL, menu_check_control, (void*) "RestrainedLoveAllowWear"));
	menu->append(new LLMenuItemCheckGL("Forbid give to #RLV/", menu_toggle_control, NULL, menu_check_control, (void*) "RestrainedLoveForbidGiveToRLV"));
	menu->append(new LLMenuItemCheckGL("Show '...' for muted text when deafened", menu_toggle_control, NULL, menu_check_control, (void*) "RestrainedLoveShowEllipsis"));
	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Debug mode", menu_toggle_control, NULL, menu_check_control, (void*) "RestrainedLoveDebug"));
}
//mk

void init_debug_baked_texture_menu(LLMenuGL* menu)
{
	menu->append(new LLMenuItemCallGL("Hair", handle_grab_baked_texture, enable_grab_baked_texture, (void*) BAKED_HAIR));
	menu->append(new LLMenuItemCallGL("Iris", handle_grab_baked_texture, enable_grab_baked_texture, (void*) BAKED_EYES));
	menu->append(new LLMenuItemCallGL("Head", handle_grab_baked_texture, enable_grab_baked_texture, (void*) BAKED_HEAD));
	menu->append(new LLMenuItemCallGL("Upper Body", handle_grab_baked_texture, enable_grab_baked_texture, (void*) BAKED_UPPER));
	menu->append(new LLMenuItemCallGL("Lower Body", handle_grab_baked_texture, enable_grab_baked_texture, (void*) BAKED_LOWER));
	menu->append(new LLMenuItemCallGL("Skirt", handle_grab_baked_texture, enable_grab_baked_texture, (void*) BAKED_SKIRT));
	menu->createJumpKeys();
}

void init_server_menu(LLMenuGL* menu)
{
	{
		LLMenuGL* sub = new LLMenuGL("Object");
		menu->appendMenu(sub);

		sub->append(new LLMenuItemCallGL("Take Copy",
										 &force_take_copy, &enable_god_customer_service, NULL,
										 'O', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->append(new LLMenuItemCallGL("Force Owner To Me", 
					&handle_object_owner_self, &enable_god_customer_service));
		sub->append(new LLMenuItemCallGL("Force Owner Permissive", 
					&handle_object_owner_permissive, &enable_god_customer_service));
		sub->append(new LLMenuItemCallGL("Delete", 
					&handle_force_delete, &enable_god_customer_service, NULL, KEY_DELETE, MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->append(new LLMenuItemCallGL("Lock", 
					&handle_object_lock, &enable_god_customer_service, NULL, 'L', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->append(new LLMenuItemCallGL("Get Asset IDs", 
					&handle_object_asset_ids, &enable_god_customer_service, NULL, 'I', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->createJumpKeys();
	}
	{
		LLMenuGL* sub = new LLMenuGL("Parcel");
		menu->appendMenu(sub);

		sub->append(new LLMenuItemCallGL("Owner To Me",
										 &handle_force_parcel_owner_to_me,
										 &enable_god_customer_service, NULL));
		sub->append(new LLMenuItemCallGL("Set to Linden Content",
										 &handle_force_parcel_to_content,
										 &enable_god_customer_service, NULL,
										 'C', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->appendSeparator();
		sub->append(new LLMenuItemCallGL("Claim Public Land",
										 &handle_claim_public_land, &enable_god_customer_service));

		sub->createJumpKeys();
	}
	{
		LLMenuGL* sub = new LLMenuGL("Region");
		menu->appendMenu(sub);
		sub->append(new LLMenuItemCallGL("Dump Temp Asset Data",
			&handle_region_dump_temp_asset_data,
			&enable_god_customer_service, NULL));
		sub->createJumpKeys();
	}
	menu->append(new LLMenuItemCallGL("God Tools...", 
		&LLFloaterGodTools::show, &enable_god_basic, NULL));

	menu->appendSeparator();

	menu->append(new LLMenuItemCallGL("Save Region State", 
		&LLPanelRegionTools::onSaveState, &enable_god_customer_service, NULL));

	menu->createJumpKeys();
}

static std::vector<LLPointer<view_listener_t> > sMenus;

//-----------------------------------------------------------------------------
// cleanup_menus()
//-----------------------------------------------------------------------------
void cleanup_menus()
{
	delete gMenuParcelObserver;
	gMenuParcelObserver = NULL;

	delete gPieSelf;
	gPieSelf = NULL;

	delete gPieAvatar;
	gPieAvatar = NULL;

	delete gPieObject;
	gPieObject = NULL;

	delete gPieAttachment;
	gPieAttachment = NULL;

	delete gPieLand;
	gPieLand = NULL;

	delete gMenuBarView;
	gMenuBarView = NULL;

	delete gPopupMenuView;
	gPopupMenuView = NULL;

	delete gMenuHolder;
	gMenuHolder = NULL;

	// NULLifiy menu children pointers
	gDetachScreenPieMenu = NULL;
	gDetachPieMenu = NULL;
	gAttachScreenPieMenu = NULL;
	gAttachPieMenu = NULL;

	sMenus.clear();
}

//-----------------------------------------------------------------------------
// Object pie menu
//-----------------------------------------------------------------------------

class LLObjectReportAbuse : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (objectp)
		{
			LLFloaterReporter::showFromObject(objectp->getID());
		}
		return true;
	}
};

// Enabled it you clicked an object
class LLObjectEnableReportAbuse : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLSelectMgr::getInstance()->getSelection()->getObjectCount() != 0;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLObjectTouch : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (!object) return true;

		LLPickInfo pick = LLToolPie::getInstance()->getPick();

//MK
		if (gRRenabled && !gAgent.mRRInterface.canTouch(object, pick.mIntersection))
		{
			return true;
		}
//mk

		LLMessageSystem	*msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_ObjectGrab);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_LocalID, object->mLocalID);
		msg->addVector3Fast(_PREHASH_GrabOffset, LLVector3::zero);
		msg->nextBlock("SurfaceInfo");
		msg->addVector3("UVCoord", LLVector3(pick.mUVCoords));
		msg->addVector3("STCoord", LLVector3(pick.mSTCoords));
		msg->addS32Fast(_PREHASH_FaceIndex, pick.mObjectFace);
		msg->addVector3("Position", pick.mIntersection);
		msg->addVector3("Normal", pick.mNormal);
		msg->addVector3("Binormal", pick.mBinormal);
		msg->sendMessage(object->getRegion()->getHost());

		// *NOTE: Hope the packets arrive safely and in order or else
		// there will be some problems.
		// *TODO: Just fix this bad assumption.
		msg->newMessageFast(_PREHASH_ObjectDeGrab);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_LocalID, object->mLocalID);
		msg->nextBlock("SurfaceInfo");
		msg->addVector3("UVCoord", LLVector3(pick.mUVCoords));
		msg->addVector3("STCoord", LLVector3(pick.mSTCoords));
		msg->addS32Fast(_PREHASH_FaceIndex, pick.mObjectFace);
		msg->addVector3("Position", pick.mIntersection);
		msg->addVector3("Normal", pick.mNormal);
		msg->addVector3("Binormal", pick.mBinormal);
		msg->sendMessage(object->getRegion()->getHost());

		return true;
	}
};

// One object must have touch sensor
class LLObjectEnableTouch : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		bool new_value = obj && obj->flagHandleTouch();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);

		// Update label based on the node touch name if available.
		LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();

		std::string touch_text;
		if (node && node->mValid && !node->mTouchName.empty())
		{
			touch_text =  node->mTouchName;
		}
		else
		{
			touch_text = userdata["data"].asString();
		}

		gMenuHolder->childSetText("Object Touch", touch_text);
		if (gMenuHolder->getChild<LLView>("Attachment Object Touch", TRUE, FALSE))
		{
			gMenuHolder->childSetText("Attachment Object Touch", touch_text);
		}

		return true;
	}
};

void label_touch(std::string& label, void*)
{
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	if (node && node->mValid && !node->mTouchName.empty())
	{
		label.assign(node->mTouchName);
	}
	else
	{
		label.assign("Touch");
	}
}

bool handle_object_open()
{
	LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
	if (!obj) return true;
//MK
	if (gRRenabled)
	{
		if (!gAgent.mRRInterface.canEdit(obj))
		{
			return true;
		}
		if (!gAgent.mRRInterface.canTouchFar(obj, LLToolPie::getInstance()->getPick().mIntersection))
		{
			return true;
		}
	}
//mk

	LLFloaterOpenObject::show();
	return true;
}

class LLObjectOpen : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return handle_object_open();
	}
};

class LLObjectEnableOpen : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Look for contents in root object, which is all the LLFloaterOpenObject
		// understands.
		LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		bool new_value = (obj != NULL);
		if (new_value)
		{
			LLViewerObject* root = obj->getRootEdit();
			if (!root) new_value = false;
			else new_value = root->allowOpen();
//MK
			if (gRRenabled && new_value)
			{
				if (!gAgent.mRRInterface.canEdit(obj))
				{
					new_value = false;
				}
				else if (!gAgent.mRRInterface.canTouchFar(obj, LLToolPie::getInstance()->getPick().mIntersection))
				{
					new_value = false;
				}
			}
//mk
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLViewCheckCameraFrontView : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		ECameraMode mode = gAgent.getCameraMode();
		bool new_value = mode != CAMERA_MODE_MOUSELOOK &&
						 mode != CAMERA_MODE_CUSTOMIZE_AVATAR;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLToolsCheckBuildMode : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLToolMgr::getInstance()->inEdit();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

bool toggle_build_mode()
{
	if (!gFloaterTools) return false;

	if (LLToolMgr::getInstance()->inBuildMode())
	{
		if (gSavedSettings.getBOOL("EditCameraMovement"))
		{
			// just reset the view, will pull us out of edit mode
			handle_reset_view();
		}
		else
		{
			// manually disable edit mode, but do not affect the camera
			gAgent.resetView(false);
			gFloaterTools->close();
			gViewerWindow->showCursor();
		}
		// avoid spurious avatar movements pulling out of edit mode
		LLViewerJoystick::getInstance()->setNeedsReset();
	}
	else
	{
//MK
		if (gRRenabled && (gAgent.mRRInterface.mContainsRez ||
						   gAgent.mRRInterface.mContainsEdit))
		{
			return false;
		}
//mk
		ECameraMode camMode = gAgent.getCameraMode();
		if (CAMERA_MODE_MOUSELOOK == camMode ||	CAMERA_MODE_CUSTOMIZE_AVATAR == camMode)
		{
			// pull the user out of mouselook or appearance mode when entering build mode
			handle_reset_view();
		}

		if (gSavedSettings.getBOOL("EditCameraMovement"))
		{
			// camera should be set
			if (LLViewerJoystick::getInstance()->getOverrideCamera())
			{
				handle_toggle_flycam();
			}

			if (gAgent.getFocusOnAvatar())
			{
				// zoom in if we're looking at the avatar
				gAgent.setFocusOnAvatar(FALSE, ANIMATE);
				gAgent.setFocusGlobal(gAgent.getPositionGlobal() + 2.0 * LLVector3d(gAgent.getAtAxis()));
				gAgent.cameraZoomIn(0.666f);
				gAgent.cameraOrbitOver(30.f * DEG_TO_RAD);
			}
		}

		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool(LLToolCompCreate::getInstance());

		// Could be first use
		LLFirstUse::useBuild();

		gAgent.resetView(false);

		// avoid spurious avatar movements
		LLViewerJoystick::getInstance()->setNeedsReset();
	}

	return true;
}

class LLToolsBuildMode : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return toggle_build_mode();
	}
};

class LLViewJoystickFlycam : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		handle_toggle_flycam();
		return true;
	}
};

class LLViewCheckJoystickFlycam : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_val = LLViewerJoystick::getInstance()->getOverrideCamera();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_val);
		return true;
	}
};

void handle_toggle_flycam()
{
	LLViewerJoystick::getInstance()->toggleFlycam();
}

class LLObjectBuild : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && (gAgent.mRRInterface.mContainsRez ||
						   gAgent.mRRInterface.mContainsEdit))
		{
			return false;
		}
//mk
		if (gAgent.getFocusOnAvatar() && !LLToolMgr::getInstance()->inEdit() && gSavedSettings.getBOOL("EditCameraMovement"))
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gAgent.cameraZoomIn(0.666f);
			gAgent.cameraOrbitOver(30.f * DEG_TO_RAD);
			gViewerWindow->moveCursorToCenter();
		}
		else if (gSavedSettings.getBOOL("EditCameraMovement"))
		{
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gViewerWindow->moveCursorToCenter();
		}

		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool(LLToolCompCreate::getInstance());

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

class LLObjectEdit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (!gFloaterTools) return false;
//MK
		if (gRRenabled)
		{
			LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
			if (obj && !gAgent.mRRInterface.canEdit(obj))
			{
				return false;
			}
			if (obj && !gAgent.mRRInterface.canTouchFar(obj, LLToolPie::getInstance()->getPick().mIntersection))
			{
				return false;
			}
		}
//mk
		LLViewerParcelMgr::getInstance()->deselectLand();

		if (gAgent.getFocusOnAvatar() && !LLToolMgr::getInstance()->inEdit())
		{
			LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

			if (selection->getSelectType() == SELECT_TYPE_HUD || !gSavedSettings.getBOOL("EditCameraMovement"))
			{
				// always freeze camera in space, even if camera doesn't move
				// so, for example, follow cam scripts can't affect you when in build mode
				gAgent.setFocusGlobal(gAgent.calcFocusPositionTargetGlobal(), LLUUID::null);
				gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			}
			else
			{
				gAgent.setFocusOnAvatar(FALSE, ANIMATE);
				LLViewerObject* selected_objectp = selection->getFirstRootObject();
				if (selected_objectp)
				{
					// zoom in on object center instead of where we clicked, as we need to see the manipulator handles
					gAgent.setFocusGlobal(selected_objectp->getPositionGlobal(), selected_objectp->getID());
					gAgent.cameraZoomIn(0.666f);
					gAgent.cameraOrbitOver(30.f * DEG_TO_RAD);
					gViewerWindow->moveCursorToCenter();
				}
			}
		}

		gFloaterTools->open();		/* Flawfinder: ignore */

		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		gFloaterTools->setEditTool(LLToolCompTranslate::getInstance());

		LLViewerJoystick::getInstance()->moveObjects(true);
		LLViewerJoystick::getInstance()->setNeedsReset(true);

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

class LLObjectInspect : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled)
		{
			if (gAgent.mRRInterface.mContainsShownames)
			{
				return false;
			}
			if (!gAgent.mRRInterface.canTouchFar(LLSelectMgr::getInstance()->getSelection()->getFirstRootObject()))
			{
				return false;
			}
		}
//mk
		LLFloaterInspect::show();
		return true;
	}
};

class LLObjectDerender : public view_listener_t
{
    bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
    {
		LLViewerObject* vobj = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(TRUE);
		if (!vobj)
		{
			return true;
		}

		if (gRRenabled && gAgent.mRRInterface.mContainsUnsit &&
			gAgent.mRRInterface.isSittingOnAnySelectedObject())
		{
			// Do not derender an object we are sitting on when RestrainedLove
			// is enabled and we are forbidden to unsit.
			return true;
		}

		LLSelectMgr::getInstance()->removeObjectFromSelections(vobj->getID());

		// Don't derender ourselves neither our attachments
		if (find_avatar_from_object(vobj) != gAgentAvatarp)
		{
			// Derender by killing the object.
			gObjectList.killObject(vobj);
		}

		return true;
	}
};

class LLObjectEnableDerender : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent>, const LLSD& userdata)
	{
		bool enable = true;

		LLViewerObject* vobj = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(TRUE);
		if (vobj)
		{
			if (find_avatar_from_object(vobj) != gAgentAvatarp)
			{
				if (gRRenabled && gAgent.mRRInterface.mContainsUnsit &&
					gAgent.mRRInterface.isSittingOnAnySelectedObject())
				{
					// Do not allow to derender an object we are sitting on
					// when RestrainedLove is enabled and we are forbidden to
					// unsit.
					enable = false;
				}
			}
			else
			{
				// Don't allow to derender ourselves neither our attachments
				enable = false;
			}
		}
		else
		{
			enable = false;
		}

		gMenuHolder->findControl(userdata["control"].asString())->setValue(enable);
		return true;
	}
};

//---------------------------------------------------------------------------
// Land pie menu
//---------------------------------------------------------------------------
class LLLandBuild : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsEdit)
		{
			return false;
		}
//mk
		LLViewerParcelMgr::getInstance()->deselectLand();

		if (gAgent.getFocusOnAvatar() && !LLToolMgr::getInstance()->inEdit() && gSavedSettings.getBOOL("EditCameraMovement"))
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gAgent.cameraZoomIn(0.666f);
			gAgent.cameraOrbitOver(30.f * DEG_TO_RAD);
			gViewerWindow->moveCursorToCenter();
		}
		else if (gSavedSettings.getBOOL("EditCameraMovement"))
		{
			// otherwise just move focus
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gViewerWindow->moveCursorToCenter();
		}

		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool(LLToolCompCreate::getInstance());

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

class LLLandBuyPass : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLPanelLandGeneral::onClickBuyPass((void *)FALSE);
		return true;
	}
};

class LLLandEnableBuyPass : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLPanelLandGeneral::enableBuyPass(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEnableEdit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// *HACK: See LLViewerParcelMgr::agentCanBuild() for the "false" flag.
		bool enable = LLViewerParcelMgr::getInstance()->agentCanBuild(false) ||
					  LLSelectMgr::getInstance()->getSelection()->isAttachment();
//MK
		if (enable && gRRenabled)
		{
			LLViewerObject* obj;
			obj = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
			if (obj && !gAgent.mRRInterface.canEdit(obj))
			{
				enable = false;
			}
		}
//mk
		gMenuHolder->findControl(userdata["control"].asString())->setValue(enable);
		return true;
	}
};

class LLSelfRemoveAllAttachments : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsDetach)
		{
			return false;
		}
//mk
		LLAgentWearables::userRemoveAllAttachments();
		return true;
	}
};

class LLSelfEnableRemoveAllAttachments : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsDetach)
		{
			return false;
		}
//mk
		bool new_value = false;
		if (isAgentAvatarValid())
		{
			for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
				 iter != gAgentAvatarp->mAttachmentPoints.end(); )
			{
				LLVOAvatar::attachment_map_t::iterator curiter = iter++;
				LLViewerJointAttachment* attachment = curiter->second;
				if (attachment->getNumObjects() > 0)
				{
					new_value = true;
					break;
				}
			}
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_has_attachments(void*)
{

	return FALSE;
}

//---------------------------------------------------------------------------
// Avatar pie menu
//---------------------------------------------------------------------------
void handle_follow(void *userdata)
{
	// follow a given avatar by ID
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
	if (objectp)
	{
		gAgent.startFollowPilot(objectp->getID());
	}
}

class LLObjectEnableMute : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
		{
			return false;
		}
//mk
		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		bool new_value = object && !object->permYouOwner();	// Don't mute our objects
		if (new_value)
		{
			LLVOAvatar* avatar = find_avatar_from_object(object); 
			if (avatar)
			{
				// It's an avatar
				LLNameValue *lastname = avatar->getNVPair("LastName");
				BOOL is_linden = lastname && !LLStringUtil::compareStrings(lastname->getString(), "Linden");
				BOOL is_self = avatar->isSelf();
				new_value = !is_linden && !is_self;
			}
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLObjectMute : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (!object) return true;

		LLMuteList* ml = LLMuteList::getInstance();
		if (!ml) return false;

		LLUUID id;
		std::string name;
		LLMute::EType type;
		U32 flags = 0;
		LLVOAvatar* avatar = find_avatar_from_object(object); 
		if (avatar)
		{
//MK
			if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
			{
				return false;
			}
//mk
			std::string data = userdata.asString();
			if (data == "chat")
			{
				flags = LLMute::flagTextChat;
			}
			else if (data == "voice")
			{
				flags = LLMute::flagVoiceChat;
			}
			else if (data == "sounds")
			{
				flags = LLMute::flagObjectSounds;
			}
			else if (data == "particles")
			{
				flags = LLMute::flagParticles;
			}

			id = avatar->getID();

			LLNameValue *firstname = avatar->getNVPair("FirstName");
			LLNameValue *lastname = avatar->getNVPair("LastName");
			if (firstname && lastname)
			{
				name = firstname->getString();
				name += " ";
				name += lastname->getString();
			}

			type = LLMute::AGENT;
		}
		else
		{
			// it's an object
			id = object->getID();

			LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
			if (node)
			{
				name = node->mName;
			}

			type = LLMute::OBJECT;
		}

		LLMute mute(id, name, type);
		if (ml->isMuted(mute.mID, mute.mName, flags))
		{
			ml->remove(mute, flags);
		}
		else if (ml->add(mute, flags))
		{
			LLFloaterMute::showInstance();
			LLFloaterMute::getInstance()->selectMute(mute.mID);
		}

		return true;
	}
};

bool handle_go_to()
{
//MK
	if (gRRenabled && gAgent.forwardGrabbed())
	{
		// When llTakeControls() has been performed on CONTROL_FWD,
		// do not allow the go to action to prevent overriding any
		// speed limitation or movement restriction.
		return true;
	}
//mk
	// JAMESDEBUG try simulator autopilot
	std::vector<std::string> strings;
	std::string val;
	LLVector3d pos = LLToolPie::getInstance()->getPick().mPosGlobal;
	val = llformat("%g", pos.mdV[VX]);
	strings.push_back(val);
	val = llformat("%g", pos.mdV[VY]);
	strings.push_back(val);
	val = llformat("%g", pos.mdV[VZ]);
	strings.push_back(val);
	send_generic_message("autopilot", strings);

	LLViewerParcelMgr::getInstance()->deselectLand();

	if (isAgentAvatarValid() && !gSavedSettings.getBOOL("AutoPilotLocksCamera"))
	{
		gAgent.setFocusGlobal(gAgent.getFocusTargetGlobal(), gAgentAvatarp->getID());
	}
	else 
	{
		// Snap camera back to behind avatar
		gAgent.setFocusOnAvatar(TRUE, ANIMATE);
	}

	// Could be first use
	LLFirstUse::useGoTo();
	return true;
}

class LLGoToObject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return handle_go_to();
	}
};

//---------------------------------------------------------------------------
// Object backup
//---------------------------------------------------------------------------

class LLObjectEnableExport : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		bool new_value = object != NULL && !LLFilePickerThread::isInUse() &&
						 !LLDirPickerThread::isInUse();
//MK
		if (new_value && gRRenabled)
		{
			new_value = !gAgent.mRRInterface.mContainsRez &&
						!gAgent.mRRInterface.mContainsEdit;
		}
//mk
		if (new_value)
		{
			struct ff : public LLSelectedNodeFunctor
			{
				ff(const LLSD& data) : LLSelectedNodeFunctor(), userdata(data)
				{
				}
				const LLSD& userdata;
				virtual bool apply(LLSelectNode* node)
				{
					// Note: the actual permission checking algorithm depends on the grid TOS and must be
					// performed for each prim and texture. This is done later in llviewerobjectbackup.cpp.
					// This means that even if the item is enabled in the menu, the export may fail should
					// the permissions not be met for each exported asset. The permissions check below
					// therefore only corresponds to the minimal permissions requirement common to all grids.
					LLPermissions *item_permissions = node->mPermissions;
					return (gAgentID == item_permissions->getOwner() &&
							(gAgentID == item_permissions->getCreator() ||
							 (item_permissions->getMaskOwner() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED));
				}
			};
			ff * the_ff = new ff(userdata);
			new_value = LLSelectMgr::getInstance()->getSelection()->applyToNodes(the_ff, false);
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLObjectExport : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (object)
		{
			if (gFloaterTools && gSavedSettings.getBOOL("NonBlockingFilePicker"))
			{
				// Open the build floater to be sure the object will stay
				// selected during the file selection since exportObject()
				// exits just after the threaded file picker is opened.
				gFloaterTools->open();
				LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
				gFloaterTools->setEditTool(LLToolCompTranslate::getInstance());
			}
			LLObjectBackup::getInstance()->exportObject();
		}
		return true;
	}
};

class LLObjectEnableImport : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = !LLFilePickerThread::isInUse() &&
						 !LLDirPickerThread::isInUse() &&
						 LLViewerParcelMgr::getInstance()->agentCanBuild();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLObjectImport : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLObjectBackup::getInstance()->importObject(FALSE);
		return true;
	}
};

class LLObjectImportUpload : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLObjectBackup::getInstance()->importObject(TRUE);
		return true;
	}
};

//---------------------------------------------------------------------------
// Parcel freeze, eject, etc.
//---------------------------------------------------------------------------
bool callback_freeze(const LLSD& notification, const LLSD& response)
{
	LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (0 == option || 1 == option)
	{
		U32 flags = 0x0;
		if (1 == option)
		{
			// unfreeze
			flags |= 0x1;
		}

		LLMessageSystem* msg = gMessageSystem;
		LLVOAvatar* avatarp = gObjectList.findAvatar(avatar_id);
		if (avatarp && avatarp->getRegion())
		{
			msg->newMessage("FreezeUser");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgentID);
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("TargetID", avatar_id);
			msg->addU32("Flags", flags);
			msg->sendReliable(avatarp->getRegion()->getHost());
		}
	}
	return false;
}

class LLAvatarFreeze : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		if (avatar)
		{
			std::string fullname = avatar->getFullname();
			LLSD payload;
			payload["avatar_id"] = avatar->getID();

			if (!fullname.empty()
//MK
				&& !(gRRenabled && gAgent.mRRInterface.mContainsShownames))
//mk
			{
				LLSD args;
				args["AVATAR_NAME"] = fullname;
				LLNotifications::instance().add("FreezeAvatarFullname",
							args,
							payload,
							callback_freeze);
			}
			else
			{
				LLNotifications::instance().add("FreezeAvatar",
							LLSD(),
							payload,
							callback_freeze);
			}
		}
		return true;
	}
};

class LLAvatarEnableDebug : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = false;
		if (gMenuHolder->getChild<LLView>("Debug", TRUE, FALSE))
		{
			std::string label = gAgent.isGodlike() ? "Debug" : "Refresh";
			gMenuHolder->childSetText("Debug", label);
			new_value = true;
		}

		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLAvatarDebug : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		if (avatar)
		{
			if (gAgent.isGodlike())
			{
				((LLVOAvatarSelf*)avatar)->dumpLocalTextures();

				if (!gAgent.getAdminOverride())
				{
					llinfos << "Dumping temporary asset data to simulator logs for avatar "
							<< avatar->getID() << llendl;
					std::vector<std::string> strings;
					strings.push_back(avatar->getID().asString());
					LLUUID invoice;
					send_generic_message("dumptempassetdata", strings, invoice);
				}

				LLFloaterAvatarTextures::show(avatar->getID());
			}
			else
			{
				handle_refresh_baked_textures(avatar);
			}
		}
		return true;
	}
};

bool callback_eject(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (2 == option)
	{
		// Cancel button.
		return false;
	}
	LLUUID avatar_id = notification["payload"]["avatar_id"].asUUID();
	bool ban_enabled = notification["payload"]["ban_enabled"].asBoolean();

	if (0 == option)
	{
		// Eject button
		LLMessageSystem* msg = gMessageSystem;
		LLVOAvatar* avatarp = gObjectList.findAvatar(avatar_id);
		if (avatarp && avatarp->getRegion())
		{
			U32 flags = 0x0;
			msg->newMessage("EjectUser");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgentID);
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("TargetID", avatar_id);
			msg->addU32("Flags", flags);
			msg->sendReliable(avatarp->getRegion()->getHost());
		}
	}
	else if (ban_enabled)
	{
		// This is tricky. It is similar to say if it is not an 'Eject' button,
		// and it is also not an 'Cancel' button, and ban_enabled==ture, 
		// it should be the 'Eject and Ban' button.
		LLMessageSystem* msg = gMessageSystem;
		LLVOAvatar* avatarp = gObjectList.findAvatar(avatar_id);
		if (avatarp && avatarp->getRegion())
		{
			U32 flags = 0x1;
			msg->newMessage("EjectUser");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgentID);
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("TargetID", avatar_id);
			msg->addU32("Flags", flags);
			msg->sendReliable(avatarp->getRegion()->getHost());
		}
	}

	return false;
}

class LLAvatarEject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		if (avatar)
		{
			LLSD payload;
			payload["avatar_id"] = avatar->getID();
			std::string fullname = avatar->getFullname();
//MK
			if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
			{
				fullname = gAgent.mRRInterface.getDummyName (fullname);
			}
//mk

			const LLVector3d& pos = avatar->getPositionGlobal();
			LLParcel* parcel = LLViewerParcelMgr::getInstance()->selectParcelAt(pos)->getParcel();

			if (LLViewerParcelMgr::getInstance()->isParcelOwnedByAgent(parcel,GP_LAND_MANAGE_BANNED))
			{
                payload["ban_enabled"] = true;
				if (!fullname.empty())
				{
    				LLSD args;
    				args["AVATAR_NAME"] = fullname;
    				LLNotifications::instance().add("EjectAvatarFullname",
    							args,
    							payload,
    							callback_eject);
				}
				else
				{
    				LLNotifications::instance().add("EjectAvatarFullname",
    							LLSD(),
    							payload,
    							callback_eject);
				}
			}
			else
			{
                payload["ban_enabled"] = false;
				if (!fullname.empty())
				{
    				LLSD args;
    				args["AVATAR_NAME"] = fullname;
    				LLNotifications::instance().add("EjectAvatarFullnameNoBan",
    							args,
    							payload,
    							callback_eject);
				}
				else
				{
    				LLNotifications::instance().add("EjectAvatarNoBan",
    							LLSD(),
    							payload,
    							callback_eject);
				}
			}
		}
		return true;
	}
};

class LLAvatarEnableFreezeEject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		bool new_value = (avatar != NULL);

		if (new_value)
		{
			const LLVector3& pos = avatar->getPositionRegion();
			const LLVector3d& pos_global = avatar->getPositionGlobal();
			LLParcel* parcel = LLViewerParcelMgr::getInstance()->selectParcelAt(pos_global)->getParcel();
			LLViewerRegion* region = avatar->getRegion();
			new_value = (region != NULL);

			if (new_value)
			{
				new_value = region->isOwnedSelf(pos);
				if (!new_value || region->isOwnedGroup(pos))
				{
					new_value = LLViewerParcelMgr::getInstance()->isParcelOwnedByAgent(parcel,GP_LAND_ADMIN);
				}
			}
		}

		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLAvatarGiveCard : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
		{
			return false;
		}
//mk
		llinfos << "handle_give_card()" << llendl;
		LLViewerObject* dest = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (dest && dest->isAvatar())
		{
			bool found_name = false;
			LLSD args;
			LLNameValue* nvfirst = dest->getNVPair("FirstName");
			LLNameValue* nvlast = dest->getNVPair("LastName");
			if (nvfirst && nvlast)
			{
				std::string name = LLCacheName::buildFullName(nvfirst->getString(), nvlast->getString());
				args["NAME"] = name;
				found_name = true;
			}
			LLViewerRegion* region = dest->getRegion();
			LLHost dest_host;
			if (region)
			{
				dest_host = region->getHost();
			}
			if (found_name && dest_host.isOk())
			{
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessage("OfferCallingCard");
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_AgentBlock);
				msg->addUUIDFast(_PREHASH_DestID, dest->getID());
				LLUUID transaction_id;
				transaction_id.generate();
				msg->addUUIDFast(_PREHASH_TransactionID, transaction_id);
				msg->sendReliable(dest_host);
				LLNotifications::instance().add("OfferedCard", args);
			}
			else
			{
				LLNotifications::instance().add("CantOfferCallingCard", args);
			}
		}
		return true;
	}
};

void login_done(S32 which, void *user)
{
	llinfos << "Login done " << which << llendl;

	LLPanelLogin::close();
}

bool callback_leave_group(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLMessageSystem *msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_LeaveGroupRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_GroupData);
		msg->addUUIDFast(_PREHASH_GroupID, gAgent.mGroupID);
		gAgent.sendReliableMessage();
	}
	return false;
}

void handle_leave_group(void *)
{
	if (gAgent.getGroupID() != LLUUID::null)
	{
		LLSD args;
		args["GROUP"] = gAgent.mGroupName;
		LLNotifications::instance().add("GroupLeaveConfirmMember", args, LLSD(), callback_leave_group);
	}
}

void append_aggregate(std::string& string, const LLAggregatePermissions& ag_perm, PermissionBit bit, const char* txt)
{
	LLAggregatePermissions::EValue val = ag_perm.getValue(bit);
	std::string buffer;
	switch (val)
	{
	  case LLAggregatePermissions::AP_NONE:
		buffer = llformat("* %s None\n", txt);
		break;
	  case LLAggregatePermissions::AP_SOME:
		buffer = llformat("* %s Some\n", txt);
		break;
	  case LLAggregatePermissions::AP_ALL:
		buffer = llformat("* %s All\n", txt);
		break;
	  case LLAggregatePermissions::AP_EMPTY:
	  default:
		break;
	}
	string.append(buffer);
}

BOOL enable_buy(void*)
{
    // In order to buy, there must only be 1 purchaseable object in
    // the selection manger.
	if (LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() != 1) return FALSE;
    LLViewerObject* obj = NULL;
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	if (node)
    {
        obj = node->getObject();
        if (!obj) return FALSE;

		if (node->mSaleInfo.isForSale() && node->mPermissions->getMaskOwner() & PERM_TRANSFER &&
			(node->mPermissions->getMaskOwner() & PERM_COPY || node->mSaleInfo.getSaleType() != LLSaleInfo::FS_COPY))
		{
			if (obj->permAnyOwner()) return TRUE;
		}
    }
	return FALSE;
}

class LLObjectEnableBuy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = enable_buy(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// Note: This will only work if the selected object's data has been
// received by the viewer and cached in the selection manager.
void handle_buy_object(LLSaleInfo sale_info)
{
	if (!LLSelectMgr::getInstance()->selectGetAllRootsValid())
	{
		LLNotifications::instance().add("UnableToBuyWhileDownloading");
		return;
	}

	LLUUID owner_id;
	std::string owner_name;
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);
	if (!owners_identical)
	{
		LLNotifications::instance().add("CannotBuyObjectsFromDifferentOwners");
		return;
	}

	LLPermissions perm;
	BOOL valid = LLSelectMgr::getInstance()->selectGetPermissions(perm);
	LLAggregatePermissions ag_perm;
	valid &= LLSelectMgr::getInstance()->selectGetAggregatePermissions(ag_perm);
	if (!valid || !sale_info.isForSale() || !perm.allowTransferTo(gAgentID))
	{
		LLNotifications::instance().add("ObjectNotForSale");
		return;
	}

	S32 price = sale_info.getSalePrice();

	if (price > 0 && price > gStatusBar->getBalance())
	{
		LLFloaterBuyCurrency::buyCurrency("This object costs", price);
		return;
	}

	LLFloaterBuy::show(sale_info);
}

void handle_buy_contents(LLSaleInfo sale_info)
{
	LLFloaterBuyContents::show(sale_info);
}

void handle_region_dump_temp_asset_data(void*)
{
	llinfos << "Dumping temporary asset data to simulator logs" << llendl;
	std::vector<std::string> strings;
	LLUUID invoice;
	send_generic_message("dumptempassetdata", strings, invoice);
}

void handle_region_clear_temp_asset_data(void*)
{
	llinfos << "Clearing temporary asset data" << llendl;
	std::vector<std::string> strings;
	LLUUID invoice;
	send_generic_message("cleartempassetdata", strings, invoice);
}

void handle_region_dump_settings(void*)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		llinfos << "Damage:    " << (regionp->getAllowDamage() ? "on" : "off") << llendl;
		llinfos << "Landmark:  " << (regionp->getAllowLandmark() ? "on" : "off") << llendl;
		llinfos << "SetHome:   " << (regionp->getAllowSetHome() ? "on" : "off") << llendl;
		llinfos << "ResetHome: " << (regionp->getResetHomeOnTeleport() ? "on" : "off") << llendl;
		llinfos << "SunFixed:  " << (regionp->getSunFixed() ? "on" : "off") << llendl;
		llinfos << "BlockFly:  " << (regionp->getBlockFly() ? "on" : "off") << llendl;
		llinfos << "AllowP2P:  " << (regionp->getAllowDirectTeleport() ? "on" : "off") << llendl;
		llinfos << "Water:     " << (regionp->getWaterHeight()) << llendl;
	}
}

void handle_show_notifications_console(void *)
{
	LLFloaterNotificationConsole::showInstance();
}

void handle_region_debug_console(void *)
{
	LLFloaterRegionDebugConsole::showInstance();
}

void handle_dump_group_info(void *)
{
	llinfos << "group   " << gAgent.mGroupName << llendl;
	llinfos << "ID      " << gAgent.mGroupID << llendl;
	llinfos << "powers " << gAgent.mGroupPowers << llendl;
	llinfos << "title   " << gAgent.mGroupTitle << llendl;
	//llinfos << "insig   " << gAgent.mGroupInsigniaID << llendl;
}

void handle_dump_capabilities_info(void *)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		regionp->logActiveCapabilities();
	}
}

void handle_dump_region_object_cache(void*)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		regionp->dumpCache();
	}
}

void handle_dump_focus(void *)
{
	LLUICtrl *ctrl = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());

	llinfos << "Keyboard focus " << (ctrl ? ctrl->getName() : "(none)") << llendl;
}

class HBSelfGroupTitles : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		HBFloaterGroupTitles::show();
		return true;
	}
};

class LLSelfSitOrStand : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (isAgentAvatarValid() && gAgentAvatarp->mIsSitting)
		{
//MK
			if (gRRenabled && gAgent.mRRInterface.mContainsUnsit)
			{
				return true;
			}
//mk
			gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
//MK
			if (gRRenabled && gAgent.mRRInterface.contains("standtp"))
			{
				gAgent.mRRInterface.mSnappingBackToLastStandingLocation = true;
				gAgent.teleportViaLocationLookAt (gAgent.mRRInterface.mLastStandingLocation);
				gAgent.mRRInterface.mSnappingBackToLastStandingLocation = false;
			}
//mk
		}
		else if (!gRRenabled || !gAgent.mRRInterface.contains("sit"))
////	else
		{
			gAgent.setControlFlags(AGENT_CONTROL_SIT_ON_GROUND);
//MK
			if (gRRenabled)
			{
				// Store our current location so that we can snap back
				// here when we stand up, if under @standtp
				gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal());
			}
//mk
			// Might be first sit
			LLFirstUse::useSit();
		}
		return true;
	}
};

class LLSelfEnableSitOrStand : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = isAgentAvatarValid() && !gAgent.getFlying();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);

		std::string label;
		std::string sit_text;
		std::string stand_text;
		std::string param = userdata["data"].asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			sit_text = param.substr(0, offset);
			stand_text = param.substr(offset+1);
		}

		if (isAgentAvatarValid() && gAgentAvatarp->mIsSitting)
		{
			label = stand_text;
		}
		else
		{
			label = sit_text;
		}

		if (gMenuHolder->getChild<LLView>("Self Sit", TRUE, FALSE))
		{
			gMenuHolder->childSetText("Self Sit", label);
		}
		if (gMenuHolder->getChild<LLView>("Self Sit Attachment", TRUE, FALSE))
		{
			gMenuHolder->childSetText("Self Sit Attachment", label);
		}

		return true;
	}
};

BOOL check_admin_override(void*)
{
	return gAgent.getAdminOverride();
}

void handle_admin_override_toggle(void*)
{
	gAgent.setAdminOverride(!gAgent.getAdminOverride());

	// The above may have affected which debug menus are visible
	show_debug_menus();
}

void handle_god_mode(void*)
{
	gAgent.requestEnterGodMode();
}

void handle_leave_god_mode(void*)
{
	gAgent.requestLeaveGodMode();
}

BOOL enable_god_options(void*)
{
	BOOL may_be_linden = TRUE;	// Linden or OpenSim admin
	if (isAgentAvatarValid() && gIsInSecondLife)
	{
		LLNameValue *lastname = gAgentAvatarp->getNVPair("LastName");
		if (lastname)
		{
			std::string name = lastname->getString();
			may_be_linden = (name == "Linden" ? TRUE : FALSE);
		}
	}

	return may_be_linden;
}

void set_god_level(U8 god_level)
{
	U8 old_god_level = gAgent.getGodLevel();
	gAgent.setGodLevel(god_level);
	gIMMgr->refresh();
	LLViewerParcelMgr::getInstance()->notifyObservers();

	// Some classifieds change visibility on god mode
	LLFloaterDirectory::requestClassifieds();

	// God mode changes region visibility
	LLWorldMap::getInstance()->reset();
	LLWorldMap::getInstance()->setCurrentLayer(0);

	// inventory in items may change in god mode
	gObjectList.dirtyAllObjectInventory();

        if (gViewerWindow)
        {
            gViewerWindow->setMenuBackgroundColor(god_level > GOD_NOT,
            LLViewerLogin::getInstance()->isInProductionGrid());
        }
    
        LLSD args;
	if (god_level > GOD_NOT)
	{
		args["LEVEL"] = llformat("%d",(S32)god_level);
		LLNotifications::instance().add("EnteringGodMode", args);
	}
	else
	{
		args["LEVEL"] = llformat("%d",(S32)old_god_level);
		LLNotifications::instance().add("LeavingGodMode", args);
	}

	// changing god-level can affect which menus we see
	show_debug_menus();
}

void process_grant_godlike_powers(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if ((agent_id == gAgentID) && (session_id == gAgent.getSessionID()))
	{
		U8 god_level;
		msg->getU8Fast(_PREHASH_GrantData, _PREHASH_GodLevel, god_level);
		set_god_level(god_level);
	}
	else
	{
		llwarns << "Grant godlike for wrong agent " << agent_id << llendl;
	}
}

/*
class LLHaveCallingcard : public LLInventoryCollectFunctor
{
public:
	LLHaveCallingcard(const LLUUID& agent_id);
	virtual ~LLHaveCallingcard() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	BOOL isThere() const { return mIsThere;}
protected:
	LLUUID mID;
	BOOL mIsThere;
};

LLHaveCallingcard::LLHaveCallingcard(const LLUUID& agent_id) :
	mID(agent_id),
	mIsThere(FALSE)
{
}

bool LLHaveCallingcard::operator()(LLInventoryCategory* cat,
								   LLInventoryItem* item)
{
	if (item)
	{
		if ((item->getType() == LLAssetType::AT_CALLINGCARD)
		   && (item->getCreatorUUID() == mID))
		{
			mIsThere = TRUE;
		}
	}
	return FALSE;
}
*/

BOOL is_agent_friend(const LLUUID& agent_id)
{
	return (LLAvatarTracker::instance().getBuddyInfo(agent_id) != NULL);
}

BOOL is_agent_mappable(const LLUUID& agent_id)
{
	return (is_agent_friend(agent_id) &&
		LLAvatarTracker::instance().getBuddyInfo(agent_id)->isOnline() &&
		LLAvatarTracker::instance().getBuddyInfo(agent_id)->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION)
		);
}

// Enable a menu item when you have someone's card.
/*
BOOL enable_have_card(void *userdata)
{
	LLUUID* avatar_id = (LLUUID *)userdata;
	if (gAgent.isGodlike())
	{
		return TRUE;
	}
	else if (avatar_id)
	{
		return is_agent_friend(*avatar_id);
	}
	else
	{
		return FALSE;
	}
}
*/

// Enable a menu item when you don't have someone's card.
class LLAvatarEnableAddFriend : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
		{
			return false;
		}
//mk
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		bool new_value = avatar && !is_agent_friend(avatar->getID());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

void request_friendship(const LLUUID& dest_id)
{
	LLViewerObject* dest = gObjectList.findObject(dest_id);
	if (dest && dest->isAvatar())
	{
		std::string fullname;
		LLSD args;
		LLNameValue* nvfirst = dest->getNVPair("FirstName");
		LLNameValue* nvlast = dest->getNVPair("LastName");
		if (nvfirst && nvlast)
		{
			args["FIRST"] = nvfirst->getString();
			args["LAST"] = nvlast->getString();
			fullname = nvfirst->getString();
			fullname += " ";
			fullname += nvlast->getString();
		}
		if (!fullname.empty())
		{
			LLFloaterFriends::requestFriendshipDialog(dest_id, fullname);
		}
		else
		{
			LLNotifications::instance().add("CantOfferFriendship");
		}
	}
}

class LLEditEnableCustomizeAvatar : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = (isAgentAvatarValid() && gAgentAvatarp->isFullyLoaded()
						  && gAgentWearables.areWearablesLoaded());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditEnableDisplayName : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = (LLAvatarNameCache::useDisplayNames() != 0);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// only works on pie menu
bool handle_sit_or_stand()
{
	LLPickInfo pick = LLToolPie::getInstance()->getPick();
	LLViewerObject *object = pick.getObject();;
	if (!object || pick.mPickType == LLPickInfo::PICK_FLORA)
	{
		return true;
	}
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsUnsit)
	{
		if (isAgentAvatarValid() && gAgentAvatarp->mIsSitting)
		{
			return true;
		}
	}
//mk
	if (sitting_on_selection())
	{
		gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
//MK
		if (gRRenabled && gAgent.mRRInterface.contains("standtp"))
		{
			gAgent.mRRInterface.mSnappingBackToLastStandingLocation = true;
			gAgent.teleportViaLocationLookAt (gAgent.mRRInterface.mLastStandingLocation);
			gAgent.mRRInterface.mSnappingBackToLastStandingLocation = false;
		}
//mk
		return true;
	}

	// get object selection offset 

	if (object && object->getPCode() == LL_PCODE_VOLUME)
	{
//MK
		if (gRRenabled)
		{
			if (gAgent.mRRInterface.contains("sit"))
			{
				return true;
			}
			if (gAgent.mRRInterface.contains("sittp") || gAgent.mRRInterface.mContainsFartouch)
			{
				LLVector3 pos = object->getPositionRegion() + pick.mObjectOffset;
				pos -= gAgent.getPositionAgent ();
				if (pos.magVec () >= 1.5)
				{
					return true;
				}
			}
			if (isAgentAvatarValid() && !gAgentAvatarp->mIsSitting)
			{
				// We are now standing, and we want to sit down => store our current location so that we can snap back here when we stand up, if under @standtp
				gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal());
			}
		}
//mk
		gMessageSystem->newMessageFast(_PREHASH_AgentRequestSit);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_TargetObject);
		gMessageSystem->addUUIDFast(_PREHASH_TargetID, object->mID);
		gMessageSystem->addVector3Fast(_PREHASH_Offset, pick.mObjectOffset);

		object->getRegion()->sendReliableMessage();
	}
	return true;
}

class LLObjectSitOrStand : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return handle_sit_or_stand();
	}
};

void near_sit_down_point(BOOL success, void *)
{
	if (success
//MK
		 && !(gRRenabled && gAgent.mRRInterface.contains("sit"))
//mk
		)
	{
		gAgent.setFlying(FALSE);
		gAgent.setControlFlags(AGENT_CONTROL_SIT_ON_GROUND);
//MK
		if (gRRenabled)
		{
			// Store our current location so that we can snap back
			// here when we stand up, if under @standtp
			gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal());
		}
//mk
		// Might be first sit
		LLFirstUse::useSit();
	}
}

class LLLandSit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsUnsit)
		{
			return true;
		}
//mk
		gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
		LLViewerParcelMgr::getInstance()->deselectLand();

		LLVector3d posGlobal = LLToolPie::getInstance()->getPick().mPosGlobal;

		LLQuaternion target_rot;
		if (isAgentAvatarValid())
		{
			target_rot = gAgentAvatarp->getRotation();
		}
		else
		{
			target_rot = gAgent.getFrameAgent().getQuaternion();
		}
		gAgent.startAutoPilotGlobal(posGlobal, "Sit", &target_rot, near_sit_down_point, NULL, 0.7f);
		return true;
	}
};

class LLCreateLandmarkCallback : public LLInventoryCallback
{
public:
	/*virtual*/ void fire(const LLUUID& inv_item)
	{
		llinfos << "Created landmark with inventory id " << inv_item
			<< llendl;
	}
};

void reload_ui(void *)
{
	LLUICtrlFactory::getInstance()->rebuild();
}

class LLWorldFly : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.toggleFlying();
		return true;
	}
};

class LLWorldEnableFly : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		BOOL sitting = FALSE;
		if (isAgentAvatarValid())
		{
			sitting = gAgentAvatarp->mIsSitting;
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(!sitting);
		return true;
	}
};

void handle_agent_stop_moving(void*)
{
	// stop agent
	gAgent.setControlFlags(AGENT_CONTROL_STOP);

	// cancel autopilot
	gAgent.stopAutoPilot();
}

void print_packets_lost(void*)
{
	LLWorld::getInstance()->printPacketsLost();
}

void drop_packet(void*)
{
	gMessageSystem->mPacketRing.dropPackets(1);
}

void velocity_interpolate(void* data)
{
	BOOL toggle = gSavedSettings.getBOOL("VelocityInterpolate");
	LLMessageSystem* msg = gMessageSystem;
	if (!toggle)
	{
		msg->newMessageFast(_PREHASH_VelocityInterpolateOn);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
		llinfos << "Velocity Interpolation On" << llendl;
	}
	else
	{
		msg->newMessageFast(_PREHASH_VelocityInterpolateOff);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
		llinfos << "Velocity Interpolation Off" << llendl;
	}
	// BUG this is a hack because of the change in menu behavior.  The
	// old menu system would automatically change a control's value,
	// but the new LLMenuGL system doesn't know what a control
	// is. However, it's easy to distinguish between the two callers
	// because LLMenuGL passes in the name of the user data (the
	// control name) to the callback function, and the user data goes
	// unused in the old menu code. Thus, if data is not null, then we
	// need to swap the value of the control.
	if (data)
	{
		gSavedSettings.setBOOL(static_cast<char*>(data), !toggle);
	}
}

void menu_toggle_wind_audio(void* user_data)
{
	menu_toggle_control(user_data);
	BOOL enabled = !gSavedSettings.getBOOL("DisableWindAudio");
	if (gAudiop)
	{
		gAudiop->enableWind(enabled);
	}
}

// Callback for enablement
BOOL is_inventory_visible(void* user_data)
{
	LLInventoryView* iv = reinterpret_cast<LLInventoryView*>(user_data);
	if (iv)
	{
		return iv->getVisible();
	}
	return FALSE;
}

void handle_show_newest_map(void*)
{
	LLFloaterWorldMap::show(NULL, FALSE);
}

//-------------------------------------------------------------------
// Help menu functions
//-------------------------------------------------------------------

//
// Major mode switching
//
void reset_view_final(BOOL proceed, void*);

void handle_reset_view()
{
	if ((CAMERA_MODE_CUSTOMIZE_AVATAR == gAgent.getCameraMode()) && gFloaterCustomize)
	{
		// Show dialog box if needed.
		gFloaterCustomize->askToSaveIfDirty(reset_view_final, NULL);
	}
	else
	{
		reset_view_final(TRUE, NULL);
	}
}

class LLViewResetView : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		handle_reset_view();
		return true;
	}
};

// Note: extra parameters allow this function to be called from dialog.
void reset_view_final(BOOL proceed, void*) 
{
	if (!proceed)
	{
		return;
	}

	gAgent.resetView(TRUE, TRUE);
}

class LLViewLookAtLastChatter : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.lookAtLastChat();
		return true;
	}
};

class LLViewMouselook : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (!gAgent.cameraMouselook())
		{
			gAgent.changeCameraToMouselook();
		}
		else
		{
			gAgent.changeCameraToDefault();
		}
		return true;
	}
};

class LLViewFullscreen : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gViewerWindow->toggleFullscreen(TRUE);
		return true;
	}
};

class LLViewDefaultUISize : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gSavedSettings.setF32("UIScaleFactor", 1.0f);
		gSavedSettings.setBOOL("UIAutoScale", FALSE);
		gViewerWindow->reshape(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
		return true;
	}
};

class LLEditDuplicate : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsRez)
		{
			return true;
		}
//mk
		if (LLEditMenuHandler::gEditMenuHandler)
		{
			LLEditMenuHandler::gEditMenuHandler->duplicate();
		}
		return true;
	}
};

class LLEditEnableDuplicate : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canDuplicate();
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsRez)
		{
			new_value = false;
		}
//mk
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

void disabled_duplicate(void*)
{
	if (LLSelectMgr::getInstance()->getSelection()->getPrimaryObject())
	{
		LLNotifications::instance().add("CopyFailed");
	}
}

void handle_duplicate_in_place(void*)
{
	llinfos << "handle_duplicate_in_place" << llendl;

	LLVector3 offset(0.f, 0.f, 0.f);
	LLSelectMgr::getInstance()->selectDuplicate(offset, TRUE);
}

void handle_repeat_duplicate(void*)
{
	LLSelectMgr::getInstance()->repeatDuplicate();
}

/* dead code 30-apr-2008
void handle_deed_object_to_group(void*)
{
	LLUUID group_id;

	LLSelectMgr::getInstance()->selectGetGroup(group_id);
	LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id, FALSE);
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_RELEASE_COUNT);
}

BOOL enable_deed_object_to_group(void*)
{
	if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return FALSE;
	LLPermissions perm;
	LLUUID group_id;

	if (LLSelectMgr::getInstance()->selectGetGroup(group_id) &&
		gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) &&
		LLSelectMgr::getInstance()->selectGetPermissions(perm) &&
		perm.deedToGroup(gAgentID, group_id))
	{
		return TRUE;
	}
	return FALSE;
}

*/

/*
 * No longer able to support viewer side manipulations in this way
 *
void god_force_inv_owner_permissive(LLViewerObject* object,
									InventoryObjectList* inventory,
									S32 serial_num,
									void*)
{
	typedef std::vector<LLPointer<LLViewerInventoryItem> > item_array_t;
	item_array_t items;

	InventoryObjectList::const_iterator inv_it = inventory->begin();
	InventoryObjectList::const_iterator inv_end = inventory->end();
	for ( ; inv_it != inv_end; ++inv_it)
	{
		if (((*inv_it)->getType() != LLAssetType::AT_CATEGORY)
		   && ((*inv_it)->getType() != LLFolderType::FT_ROOT_CATEGORY))
		{
			LLInventoryObject* obj = *inv_it;
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem((LLViewerInventoryItem*)obj);
			LLPermissions perm(new_item->getPermissions());
			perm.setMaskBase(PERM_ALL);
			perm.setMaskOwner(PERM_ALL);
			new_item->setPermissions(perm);
			items.push_back(new_item);
		}
	}
	item_array_t::iterator end = items.end();
	item_array_t::iterator it;
	for (it = items.begin(); it != end; ++it)
	{
		// since we have the inventory item in the callback, it should not
		// invalidate iteration through the selection manager.
		object->updateInventory((*it), TASK_INVENTORY_ITEM_KEY, false);
	}
}
*/

void handle_object_owner_permissive(void*)
{
	// only send this if they're a god.
	if (gAgent.isGodlike())
	{
		// do the objects.
		LLSelectMgr::getInstance()->selectionSetObjectPermissions(PERM_BASE, TRUE, PERM_ALL, TRUE);
		LLSelectMgr::getInstance()->selectionSetObjectPermissions(PERM_OWNER, TRUE, PERM_ALL, TRUE);
	}
}

void handle_object_owner_self(void*)
{
	// only send this if they're a god.
	if (gAgent.isGodlike())
	{
		LLSelectMgr::getInstance()->sendOwner(gAgentID, gAgent.getGroupID(), TRUE);
	}
}

// Shortcut to set owner permissions to not editable.
void handle_object_lock(void*)
{
	LLSelectMgr::getInstance()->selectionSetObjectPermissions(PERM_OWNER, FALSE, PERM_MODIFY);
}

void handle_object_asset_ids(void*)
{
	// only send this if they're a god.
	if (gAgent.isGodlike())
	{
		LLSelectMgr::getInstance()->sendGodlikeRequest("objectinfo", "assetids");
	}
}

void handle_force_parcel_owner_to_me(void*)
{
	LLViewerParcelMgr::getInstance()->sendParcelGodForceOwner(gAgentID);
}

void handle_force_parcel_to_content(void*)
{
	LLViewerParcelMgr::getInstance()->sendParcelGodForceToContent();
}

void handle_claim_public_land(void*)
{
	if (LLViewerParcelMgr::getInstance()->getSelectionRegion() != gAgent.getRegion())
	{
		LLNotifications::instance().add("ClaimPublicLand");
		return;
	}

	LLVector3d west_south_global;
	LLVector3d east_north_global;
	LLViewerParcelMgr::getInstance()->getSelection(west_south_global, east_north_global);
	LLVector3 west_south = gAgent.getPosAgentFromGlobal(west_south_global);
	LLVector3 east_north = gAgent.getPosAgentFromGlobal(east_north_global);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GodlikeMessage");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgentID);
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", "claimpublicland");
	msg->addUUID("Invoice", LLUUID::null);
	std::string buffer;
	buffer = llformat("%f", west_south.mV[VX]);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	buffer = llformat("%f", west_south.mV[VY]);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	buffer = llformat("%f", east_north.mV[VX]);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	buffer = llformat("%f", east_north.mV[VY]);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	gAgent.sendReliableMessage();
}

void handle_god_request_havok(void *)
{
	if (gAgent.isGodlike())
	{
		LLSelectMgr::getInstance()->sendGodlikeRequest("havok", "infoverbose");
	}
}

//void handle_god_request_foo(void *)
//{
//	if (gAgent.isGodlike())
//	{
//		LLSelectMgr::getInstance()->sendGodlikeRequest(GOD_WANTS_FOO);
//	}
//}

//void handle_god_request_terrain_save(void *)
//{
//	if (gAgent.isGodlike())
//	{
//		LLSelectMgr::getInstance()->sendGodlikeRequest("terrain", "save");
//	}
//}

//void handle_god_request_terrain_load(void *)
//{
//	if (gAgent.isGodlike())
//	{
//		LLSelectMgr::getInstance()->sendGodlikeRequest("terrain", "load");
//	}
//}

// HACK for easily testing new avatar geometry
void handle_god_request_avatar_geometry(void *)
{
	if (gAgent.isGodlike())
	{
		LLSelectMgr::getInstance()->sendGodlikeRequest("avatar toggle", NULL);
	}
}

void derez_objects(EDeRezDestination dest, const LLUUID& dest_id)
{
	if (gAgent.cameraMouselook())
	{
		gAgent.changeCameraToDefault();
	}
	//gInventoryView->setPanelOpen(TRUE);

	std::string error;
	LLDynamicArray<LLViewerObject*> derez_objects;

	// Check conditions that we can't deal with, building a list of
	// everything that we'll actually be derezzing.
	LLViewerRegion* first_region = NULL;
	for (LLObjectSelection::valid_root_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		LLViewerRegion* region = object->getRegion();
		if (!first_region)
		{
			first_region = region;
		}
		else
		{
			if (region != first_region)
			{
				// Derez doesn't work at all if the some of the objects
				// are in regions besides the first object selected.

				// ...crosses region boundaries
				error = "AcquireErrorObjectSpan";
				break;
			}
		}
		if (object->isAvatar())
		{
			// ...don't acquire avatars
			continue;
		}

		// If AssetContainers are being sent back, they will appear as 
		// boxes in the owner's inventory.
		if (object->getNVPair("AssetContainer")
			&& dest != DRD_RETURN_TO_OWNER)
		{
			// this object is an asset container, derez its contents, not it
			llwarns << "Attempt to derez deprecated AssetContainer object type not supported." << llendl;
			/*
			object->requestInventory(container_inventory_arrived, 
				(void *)(BOOL)(DRD_TAKE_INTO_AGENT_INVENTORY == dest));
			*/
			continue;
		}
		BOOL can_derez_current = FALSE;
		switch (dest)
		{
		case DRD_TAKE_INTO_AGENT_INVENTORY:
		case DRD_TRASH:
			if ((node->mPermissions->allowTransferTo(gAgentID) && object->permModify())
				|| (node->allowOperationOnNode(PERM_OWNER, GP_OBJECT_MANIPULATE)))
			{
				can_derez_current = TRUE;
			}
			break;

		case DRD_RETURN_TO_OWNER:
			can_derez_current = TRUE;
			break;

		default:
			if ((node->mPermissions->allowTransferTo(gAgentID)
				&& object->permCopy())
			   || gAgent.isGodlike())
			{
				can_derez_current = TRUE;
			}
			break;
		}
		if (can_derez_current)
		{
			derez_objects.put(object);
		}
	}

	// This constant is based on (1200 - HEADER_SIZE) / 4 bytes per
	// root.  I lopped off a few (33) to provide a bit
	// pad. HEADER_SIZE is currently 67 bytes, most of which is UUIDs.
	// This gives us a maximum of 63500 root objects - which should
	// satisfy anybody.
	const S32 MAX_ROOTS_PER_PACKET = 250;
	const S32 MAX_PACKET_COUNT = 254;
	F32 packets = ceil((F32)derez_objects.count() / (F32)MAX_ROOTS_PER_PACKET);
	if (packets > (F32)MAX_PACKET_COUNT)
	{
		error = "AcquireErrorTooManyObjects";
	}

	if (error.empty() && derez_objects.count() > 0)
	{
		U8 d = (U8)dest;
		LLUUID tid;
		tid.generate();
		U8 packet_count = (U8)packets;
		S32 object_index = 0;
		S32 objects_in_packet = 0;
		LLMessageSystem* msg = gMessageSystem;
		for (U8 packet_number = 0; packet_number < packet_count; ++packet_number)
		{
			msg->newMessageFast(_PREHASH_DeRezObject);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_AgentBlock);
			msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
			msg->addU8Fast(_PREHASH_Destination, d);
			msg->addUUIDFast(_PREHASH_DestinationID, dest_id);
			msg->addUUIDFast(_PREHASH_TransactionID, tid);
			msg->addU8Fast(_PREHASH_PacketCount, packet_count);
			msg->addU8Fast(_PREHASH_PacketNumber, packet_number);
			objects_in_packet = 0;
			while ((object_index < derez_objects.count())
				  && (objects_in_packet++ < MAX_ROOTS_PER_PACKET))

			{
				LLViewerObject* object = derez_objects.get(object_index++);
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
				// VEFFECT: DerezObject
				LLHUDEffectSpiral* effectp = (LLHUDEffectSpiral*)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
				effectp->setPositionGlobal(object->getPositionGlobal());
				effectp->setColor(LLColor4U(gAgent.getEffectColor()));
			}
			msg->sendReliable(first_region->getHost());
		}
		make_ui_sound("UISndObjectRezOut");

		// Busy count decremented by inventory update, so only increment
		// if will be causing an update.
		if (dest != DRD_RETURN_TO_OWNER)
		{
			gViewerWindow->getWindow()->incBusyCount();
		}
	}
	else if (!error.empty())
	{
		LLNotifications::instance().add(error);
	}
}

class LLToolsTakeCopy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return true;

		const LLUUID& category_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
		derez_objects(DRD_ACQUIRE_TO_AGENT_INVENTORY, category_id);

		return true;
	}
};

// You can return an object to its owner if it is on your land.
class LLObjectReturn : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return true;
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsRez)
		{
			return false;
		}

		if (gRRenabled && gAgent.mRRInterface.mContainsUnsit
			&& gAgent.mRRInterface.isSittingOnAnySelectedObject())
		{
			return false;
		}
//mk
		mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();

		LLNotifications::instance().add("ReturnToOwner", LLSD(), LLSD(), boost::bind(&LLObjectReturn::onReturnToOwner, this, _1, _2));
		return true;
	}

	bool onReturnToOwner(const LLSD& notification, const LLSD& response)
	{
		S32 option = LLNotification::getSelectedOption(notification, response);
		if (0 == option)
		{
			// Ignore category ID for this derez destination.
			derez_objects(DRD_RETURN_TO_OWNER, LLUUID::null);
		}

		// drop reference to current selection
		mObjectSelection = NULL;
		return false;
	}

protected:
	LLObjectSelectionHandle mObjectSelection;
};

// Allow return to owner if one or more of the selected items is
// over land you own.
class LLObjectEnableReturn : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsRez)
		{
			return false;
		}

		if (gRRenabled && gAgent.mRRInterface.mContainsUnsit
			&& gAgent.mRRInterface.isSittingOnAnySelectedObject())
		{
			return false;
		}
//mk
		bool new_value = false;
		if (gAgent.isGodlike())
		{
			new_value = true;
		}
		else
		{
			LLViewerRegion* region = gAgent.getRegion();
			if (region)
			{
				// Estate owners and managers can always return objects.
				if (region->canManageEstate())
				{
					new_value = true;
				}
				else
				{
					struct f : public LLSelectedObjectFunctor
					{
						virtual bool apply(LLViewerObject* obj)
						{
//MK
							if (gRRenabled && obj->isSeat() &&
								gAgent.mRRInterface.mContainsUnsit)
							{
								return false;
							}
//mk
							return obj->permModify() || obj->isReturnable();
						}
					} func;
					const bool firstonly = true;
					new_value = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
				}
			}
		}

		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

void force_take_copy(void*)
{
	if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return;
	const LLUUID& category_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
	derez_objects(DRD_FORCE_TO_GOD_INVENTORY, category_id);
}

void handle_take()
{
	// we want to use the folder this was derezzed from if it's
	// available. Otherwise, derez to the normal place.
	if (LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		return;
	}
//MK
	if (gRRenabled &&
		(gAgent.mRRInterface.mContainsRez ||
		 (gAgent.mRRInterface.mContainsUnsit &&
		  gAgent.mRRInterface.isSittingOnAnySelectedObject())))
	{
		return;
	}
//mk
	bool you_own_everything = true;
	bool locked_but_takeable_object = false;
	bool ambiguous_destination = false;
	LLUUID category_id, new_cat_id;
	LLUUID trash = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLUUID library = gInventory.getLibraryRootFolderID();

	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if (object)
		{
			if (!object->permYouOwner())
			{
				you_own_everything = false;
			}

			if (!object->permMove())
			{
				locked_but_takeable_object = true;
			}
		}
		new_cat_id = node->mFolderID;
		// Check that the category exists and is not inside the trash
		// neither inside the library...
		if (!ambiguous_destination && new_cat_id.notNull() &&
			gInventory.getCategory(new_cat_id) && new_cat_id != trash &&
			!gInventory.isObjectDescendentOf(new_cat_id, trash) &&
			!gInventory.isObjectDescendentOf(new_cat_id, library))
		{
			if (category_id.isNull())
			{
				category_id = new_cat_id;
			}
			else if (category_id != new_cat_id)
			{
				// We have found two potential destinations.
				ambiguous_destination = true;
			}
		}
	}
	if (ambiguous_destination || category_id.isNull())
	{
		// Use the default "Objects" category.
		category_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
	}

	LLSD payload;
	payload["folder_id"] = category_id;
	LLNotification::Params params("ConfirmObjectTakeLock");
	params.payload(payload).functor(confirm_take);
	if (locked_but_takeable_object || !you_own_everything)
	{
		if (locked_but_takeable_object && you_own_everything)
		{
			params.name("ConfirmObjectTakeLock");

		}
		else if (!locked_but_takeable_object && !you_own_everything)
		{
			params.name("ConfirmObjectTakeNoOwn");
		}
		else
		{
			params.name("ConfirmObjectTakeLockNoOwn");
		}

		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 0);
	}
}

bool confirm_take(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (enable_take() && (option == 0))
	{
		derez_objects(DRD_TAKE_INTO_AGENT_INVENTORY, notification["payload"]["folder_id"].asUUID());
	}
	return false;
}

// You can take an item when it is public and transferrable, or when
// you own it. We err on the side of enabling the item when at least
// one item selected can be copied to inventory.
BOOL enable_take()
{
	if (sitting_on_selection())
	{
		return FALSE;
	}
//MK
	if (gRRenabled && gAgent.mRRInterface.mContainsRez)
	{
		return FALSE;
	}
//mk
	for (LLObjectSelection::valid_root_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if (object->isAvatar())
		{
			// ...don't acquire avatars
			continue;
		}

		if ((node->mPermissions->getOwner() == gAgentID) ||
			(object->permModify() &&
			 node->mPermissions->allowTransferTo(gAgentID)))
		{
			return TRUE;
		}
	}
	return FALSE;
}

class LLToolsBuyOrTake : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLSelectMgr::getInstance()->getSelection()->isEmpty())
		{
			return true;
		}

		if (is_selection_buy_not_take())
		{
			S32 total_price = selection_price();

			if (total_price <= gStatusBar->getBalance() || total_price == 0)
			{
				handle_buy(NULL);
			}
			else
			{
				LLFloaterBuyCurrency::buyCurrency(
					"Buying this costs", total_price);
			}
		}
		else
		{
			handle_take();
		}
		return true;
	}
};

class LLToolsEnableBuyOrTake : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool is_buy = is_selection_buy_not_take();
		bool new_value = is_buy ? enable_buy(NULL) : enable_take();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);

		// Update label
		std::string label;
		std::string buy_text;
		std::string take_text;
		std::string param = userdata["data"].asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			buy_text = param.substr(0, offset);
			take_text = param.substr(offset+1);
		}
		if (is_buy)
		{
			label = buy_text;
		}
		else
		{
			label = take_text;
		}
		gMenuHolder->childSetText("Pie Object Take", label);
		if (gMenuHolder->getChild<LLView>("Menu Object Take", TRUE, FALSE))
		{
			gMenuHolder->childSetText("Menu Object Take", label);
		}

		return true;
	}
};

// This is a small helper function to determine if we have a buy or a
// take in the selection. This method is to help with the aliasing
// problems of putting buy and take in the same pie menu space. After
// a fair amont of discussion, it was determined to prefer buy over
// take. The reasoning follows from the fact that when users walk up
// to buy something, they will click on one or more items. Thus, if
// anything is for sale, it becomes a buy operation, and the server
// will group all of the buy items, and copyable/modifiable items into
// one package and give the end user as much as the permissions will
// allow. If the user wanted to take something, they will select fewer
// and fewer items until only 'takeable' items are left. The one
// exception is if you own everything in the selection that is for
// sale, in this case, you can't buy stuff from yourself, so you can
// take it.
// return value = TRUE if selection is a 'buy'.
//                FALSE if selection is a 'take'
BOOL is_selection_buy_not_take()
{
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if (obj && !(obj->permYouOwner()) && (node->mSaleInfo.isForSale()))
		{
			// you do not own the object and it is for sale, thus,
			// it's a buy
			return TRUE;
		}
	}
	return FALSE;
}

S32 selection_price()
{
	S32 total_price = 0;
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* obj = node->getObject();
		if (obj && !(obj->permYouOwner()) && (node->mSaleInfo.isForSale()))
		{
			// you do not own the object and it is for sale.
			// Add its price.
			total_price += node->mSaleInfo.getSalePrice();
		}
	}

	return total_price;
}

bool callback_show_buy_currency(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		llinfos << "Loading page " << BUY_CURRENCY_URL << llendl;
		LLWeb::loadURL(BUY_CURRENCY_URL);
	}
	return false;
}

void show_buy_currency(const char* extra)
{
	// Don't show currency web page for branded clients.

	std::ostringstream mesg;
	if (extra != NULL)
	{
		mesg << extra << "\n \n";
	}
	mesg << "Go to " << BUY_CURRENCY_URL << "\nfor information on purchasing currency?";

	LLSD args;
	if (extra != NULL)
	{
		args["EXTRA"] = extra;
	}
	args["URL"] = BUY_CURRENCY_URL;
	LLNotifications::instance().add("PromptGoToCurrencyPage", args, LLSD(), callback_show_buy_currency);
}

void handle_buy_currency(void*)
{
//	LLFloaterBuyCurrency::buyCurrency();
}

void handle_buy(void*)
{
	if (LLSelectMgr::getInstance()->getSelection()->isEmpty()) return;

	LLSaleInfo sale_info;
	BOOL valid = LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
	if (!valid) return;

	if (sale_info.getSaleType() == LLSaleInfo::FS_CONTENTS)
	{
		handle_buy_contents(sale_info);
	}
	else
	{
		handle_buy_object(sale_info);
	}
}

class LLObjectBuy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		handle_buy(NULL);
		return true;
	}
};

BOOL sitting_on_selection()
{
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	if (!node)
	{
		return FALSE;
	}

	if (!node->mValid)
	{
		return FALSE;
	}

	LLViewerObject* root_object = node->getObject();
	if (!root_object)
	{
		return FALSE;
	}

	// Need to determine if avatar is sitting on this object
	if (!isAgentAvatarValid())
	{
		return FALSE;
	}

	return gAgentAvatarp->mIsSitting && gAgentAvatarp->getRoot() == root_object;
}

class LLToolsSaveToObjectInventory : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
		if (node && (node->mValid) && (!node->mFromTaskID.isNull()))
		{
			// *TODO: check to see if the fromtaskid object exists.
			derez_objects(DRD_SAVE_INTO_TASK_INVENTORY, node->mFromTaskID);
		}
		return true;
	}
};

// Round the position of all root objects to the grid
class LLToolsSnapObjectXY : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		F64 snap_size = (F64)gSavedSettings.getF32("GridResolution");

		for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
			 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
		{
			LLSelectNode* node = *iter;
			LLViewerObject* obj = node->getObject();
			if (obj->permModify())
			{
				LLVector3d pos_global = obj->getPositionGlobal();
				F64 round_x = fmod(pos_global.mdV[VX], snap_size);
				if (round_x < snap_size * 0.5)
				{
					// closer to round down
					pos_global.mdV[VX] -= round_x;
				}
				else
				{
					// closer to round up
					pos_global.mdV[VX] -= round_x;
					pos_global.mdV[VX] += snap_size;
				}

				F64 round_y = fmod(pos_global.mdV[VY], snap_size);
				if (round_y < snap_size * 0.5)
				{
					pos_global.mdV[VY] -= round_y;
				}
				else
				{
					pos_global.mdV[VY] -= round_y;
					pos_global.mdV[VY] += snap_size;
				}

				obj->setPositionGlobal(pos_global, FALSE);
			}
		}
		LLSelectMgr::getInstance()->sendMultipleUpdate(UPD_POSITION);
		return true;
	}
};

// Determine if the option to cycle between linked prims is shown
class LLToolsEnableSelectNextPart : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = (gSavedSettings.getBOOL("EditLinkedParts") &&
				 !LLSelectMgr::getInstance()->getSelection()->isEmpty());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// Cycle selection through linked children in selected object.
// FIXME: Order of children list is not always the same as sim's idea of link order. This may confuse
// resis. Need link position added to sim messages to address this.
class LLToolsSelectNextPart : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		S32 object_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
		if (gSavedSettings.getBOOL("EditLinkedParts") && object_count)
		{
			LLViewerObject* selected = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
			if (selected && selected->getRootEdit())
			{
				bool fwd = (userdata.asString() == "next");
				bool prev = (userdata.asString() == "previous");
				bool ifwd = (userdata.asString() == "includenext");
				bool iprev = (userdata.asString() == "includeprevious");
				LLViewerObject* to_select = NULL;
				LLViewerObject::child_list_t children = selected->getRootEdit()->getChildren();
				children.push_front(selected->getRootEdit());	// need root in the list too

				for (LLViewerObject::child_list_t::iterator iter = children.begin(); iter != children.end(); ++iter)
				{
					if ((*iter)->isSelected())
					{
						if (object_count > 1 && (fwd || prev))	// multiple selection, find first or last selected if not include
						{
							to_select = *iter;
							if (fwd)
							{
								// stop searching if going forward; repeat to get last hit if backward
								break;
							}
						}
						else if ((object_count == 1) || (ifwd || iprev))	// single selection or include
						{
							if (fwd || ifwd)
							{
								++iter;
								while (iter != children.end() && ((*iter)->isAvatar() || (ifwd && (*iter)->isSelected())))
								{
									++iter;	// skip sitting avatars and selected if include
								}
							}
							else // backward
							{
								iter = (iter == children.begin() ? children.end() : iter);
								--iter;
								while (iter != children.begin() && ((*iter)->isAvatar() || (iprev && (*iter)->isSelected())))
								{
									--iter;	// skip sitting avatars and selected if include
								}
							}
							iter = (iter == children.end() ? children.begin() : iter);
							to_select = *iter;
							break;
						}
					}
				}

				if (to_select)
				{
					if (gFloaterTools && gFocusMgr.childHasKeyboardFocus(gFloaterTools))
					{
						gFocusMgr.setKeyboardFocus(NULL);	// force edit toolbox to commit any changes
					}
					if (fwd || prev)
					{
						LLSelectMgr::getInstance()->deselectAll();
					}
					LLSelectMgr::getInstance()->selectObjectOnly(to_select);
					return true;
				}
			}
		}
		return true;
	}
};

class LLToolsEnableLink : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLSelectMgr::getInstance()->enableLinkObjects();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLToolsLink : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLSelectMgr::getInstance()->linkObjects();
		return true;
	}
};

class LLToolsEnableUnlink : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLSelectMgr::getInstance()->enableUnlinkObjects();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLToolsUnlink : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLSelectMgr::getInstance()->unlinkObjects();
		return true;
	}
};

class LLToolsStopAllAnimations : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.stopCurrentAnimations();
		return true;
	}
};

class LLToolsReleaseKeys : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsDetach)
		{
			return false;
		}
//mk
		gAgent.forceReleaseControls();

		return true;
	}
};

class LLToolsEnableReleaseKeys : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gMenuHolder->findControl(userdata["control"].asString())->setValue(gAgent.anyControlGrabbed());
		return true;
	}
};

#ifdef SEND_HINGES
void handle_hinge(void*)
{
	LLSelectMgr::getInstance()->sendHinge(1);
}

void handle_ptop(void*)
{
	LLSelectMgr::getInstance()->sendHinge(2);
}

void handle_lptop(void*)
{
	LLSelectMgr::getInstance()->sendHinge(3);
}

void handle_wheel(void*)
{
	LLSelectMgr::getInstance()->sendHinge(4);
}

void handle_dehinge(void*)
{
	LLSelectMgr::getInstance()->sendDehinge();
}

BOOL enable_dehinge(void*)
{
	LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstEditableObject();
	return obj && !obj->isAttachment();
}
#endif

class LLEditEnableCut : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canCut();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditCut : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLEditMenuHandler::gEditMenuHandler)
		{
			LLEditMenuHandler::gEditMenuHandler->cut();
		}
		return true;
	}
};

class LLEditEnableCopy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canCopy();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditCopy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLEditMenuHandler::gEditMenuHandler)
		{
			LLEditMenuHandler::gEditMenuHandler->copy();
		}
		return true;
	}
};

class LLEditEnablePaste : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canPaste();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditPaste : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLEditMenuHandler::gEditMenuHandler)
		{
			LLEditMenuHandler::gEditMenuHandler->paste();
		}
		return true;
	}
};

class LLEditEnableDelete : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canDoDelete();
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsRez
			&& (LLEditMenuHandler::gEditMenuHandler == LLSelectMgr::getInstance())) // the Delete key must not be inhibited for text
		{
			new_value = false;
		}
//mk
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditDelete : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// If a text field can do a deletion, it gets precedence over deleting
		// an object in the world.
		if (LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canDoDelete())
		{
			LLEditMenuHandler::gEditMenuHandler->doDelete();
		}

		// and close any pie/context menus when done
		gMenuHolder->hideMenus();

		// When deleting an object we may not actually be done
		// Keep selection so we know what to delete when confirmation is needed about the delete
		gPieObject->hide(TRUE);
		return true;
	}
};

class LLObjectEnableDelete : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLSelectMgr::getInstance()->canDoDelete();
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsRez)
		{
			new_value = false;
		}
//mk
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditSearch : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloaterDirectory::toggleFind(NULL);
		return true;
	}
};

class LLObjectDelete : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLSelectMgr::getInstance())
		{
			LLSelectMgr::getInstance()->doDelete();
		}

		// and close any pie/context menus when done
		gMenuHolder->hideMenus();

		// When deleting an object we may not actually be done
		// Keep selection so we know what to delete when confirmation is needed about the delete
		gPieObject->hide(TRUE);
		return true;
	}
};

void handle_force_delete(void*)
{
	LLSelectMgr::getInstance()->selectForceDelete();
}

class LLViewEnableJoystickFlycam : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = (gSavedSettings.getBOOL("JoystickEnabled") && gSavedSettings.getBOOL("JoystickFlycamEnabled"));
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLViewEnableLastChatter : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// *TODO: add check that last chatter is in range
		bool new_value = (gAgent.cameraThirdPerson() && gAgent.getLastChatter().notNull());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLViewToggleRadar: public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloaterAvatarList::toggle(0);
		bool vis = false;
		if (LLFloaterAvatarList::getInstance())
		{
			vis = (bool)LLFloaterAvatarList::getInstance()->getVisible();
		}
		return true;
	}
};

class LLEditEnableDeselect : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canDeselect();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditDeselect : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLEditMenuHandler::gEditMenuHandler)
		{
			LLEditMenuHandler::gEditMenuHandler->deselect();
		}
		return true;
	}
};

class LLEditEnableSelectAll : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canSelectAll();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditSelectAll : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLEditMenuHandler::gEditMenuHandler)
		{
			LLEditMenuHandler::gEditMenuHandler->selectAll();
		}
		return true;
	}
};

class LLEditEnableUndo : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canUndo();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditUndo : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canUndo())
		{
			LLEditMenuHandler::gEditMenuHandler->undo();
		}
		return true;
	}
};

class LLEditEnableRedo : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canRedo();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditRedo : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (LLEditMenuHandler::gEditMenuHandler && LLEditMenuHandler::gEditMenuHandler->canRedo())
		{
			LLEditMenuHandler::gEditMenuHandler->redo();
		}
		return true;
	}
};

void print_object_info(void*)
{
	LLSelectMgr::getInstance()->selectionDump();
}

void print_agent_nvpairs(void*)
{
	LLViewerObject *objectp;

	llinfos << "Agent Name Value Pairs" << llendl;

	objectp = gObjectList.findObject(gAgentID);
	if (objectp)
	{
		objectp->printNameValuePairs();
	}
	else
	{
		llinfos << "Can't find agent object" << llendl;
	}

	llinfos << "Camera at " << gAgent.getCameraPositionGlobal() << llendl;
}

void show_debug_menus()
{
	// this can get called at login screen where there is no menu so only toggle it if one exists
	if (gMenuBarView)
	{
		BOOL debug = gSavedSettings.getBOOL("UseDebugMenus");

		if (debug)
		{
			LLFirstUse::useDebugMenus();
		}

		gMenuBarView->setItemVisible(CLIENT_MENU_NAME, debug);
		gMenuBarView->setItemEnabled(CLIENT_MENU_NAME, debug);

		// Server ('Admin') menu hidden when not in godmode.
		const bool show_server_menu = debug && (gAgent.getGodLevel() > GOD_NOT);
		gMenuBarView->setItemVisible(SERVER_MENU_NAME, show_server_menu);
		BOOL is_linden = enable_god_options(NULL);
		gMenuBarView->setItemEnabled(SERVER_MENU_NAME, show_server_menu && is_linden);

		gMenuBarView->arrange(); // clean-up positioning 
	};
}

// LLUUID gExporterRequestID;
// std::string gExportDirectory;

// LLUploadDialog *gExportDialog = NULL;

// void handle_export_selected(void *)
// {
// 	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
// 	if (selection->isEmpty())
// 	{
// 		return;
// 	}
// 	llinfos << "Exporting selected objects:" << llendl;

// 	gExporterRequestID.generate();
// 	gExportDirectory = "";

// 	LLMessageSystem* msg = gMessageSystem;
// 	msg->newMessageFast(_PREHASH_ObjectExportSelected);
// 	msg->nextBlockFast(_PREHASH_AgentData);
// 	msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
// 	msg->addUUIDFast(_PREHASH_RequestID, gExporterRequestID);
// 	msg->addS16Fast(_PREHASH_VolumeDetail, 4);

// 	for (LLObjectSelection::root_iterator iter = selection->root_begin();
// 		 iter != selection->root_end(); iter++)
// 	{
// 		LLSelectNode* node = *iter;
// 		LLViewerObject* object = node->getObject();
// 		msg->nextBlockFast(_PREHASH_ObjectData);
// 		msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
// 		llinfos << "Object: " << object->getID() << llendl;
// 	}
// 	msg->sendReliable(gAgent.getRegion()->getHost());

// 	gExportDialog = LLUploadDialog::modalUploadDialog("Exporting selected objects...");
// }

BOOL menu_check_build_tool(void* user_data)
{
	S32 index = (intptr_t) user_data;
	return LLToolMgr::getInstance()->getCurrentToolset()->isToolSelected(index);
}

void handle_reload_settings(void*)
{
	gSavedSettings.resetToDefaults();
	gSavedSettings.loadFromFile(gSavedSettings.getString("ClientSettingsFile"));

	llinfos << "Loading colors from colors.xml" << llendl;
	std::string color_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"colors.xml");
	gColors.resetToDefaults();
	gColors.loadFromFileLegacy(color_file, FALSE, TYPE_COL4U);
}

class LLWorldSetHomeLocation : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// we just send the message and let the server check for failure cases
		// server will echo back a "Home position set." alert if it succeeds
		// and the home location screencapture happens when that alert is recieved
		gAgent.setStartPosition(START_LOCATION_ID_HOME);
		return true;
	}
};

class LLWorldTeleportHome : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.teleportHome();
		return true;
	}
};

class LLWorldTPtoGround : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (isAgentAvatarValid())
		{
			LLVector3 pos = gAgent.getPositionAgent();
			pos.mV[VZ] = LLWorld::getInstance()->resolveLandHeightAgent(pos);
			LLVector3d pos_global = from_region_handle(gAgent.getRegion()->getHandle());
			pos_global += LLVector3d((F64)pos.mV[VX], (F64)pos.mV[VY], (F64)pos.mV[VZ]);
			gAgent.teleportViaLocation(pos_global);
		}
		return true;
	}
};

class LLWorldAlwaysRun : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// as well as altering the default walk-vs-run state,
		// we also change the *current* walk-vs-run state.
		if (gAgent.getAlwaysRun())
		{
			gAgent.clearAlwaysRun();
			gAgent.clearRunning();
		}
//MK
		else if (!gRRenabled || !gAgent.mRRInterface.mContainsAlwaysRun)
//mk
////	else
		{
			gAgent.setAlwaysRun();
			gAgent.setRunning();
		}

		// tell the simulator.
		gAgent.sendWalkRun(gAgent.getAlwaysRun());

		return true;
	}
};

class LLWorldCheckAlwaysRun : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.getAlwaysRun();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldSitOnGround : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (isAgentAvatarValid() && !gAgentAvatarp->mIsSitting
//MK
			&& !(gRRenabled && gAgent.mRRInterface.contains("sit"))
//mk
			)
		{
			gAgent.setControlFlags(AGENT_CONTROL_SIT_ON_GROUND);
//MK
			if (gRRenabled)
			{
				// Store our current location so that we can snap back
				// here when we stand up, if under @standtp
				gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal());
			}
//mk
		}
		return true;
	}
};

class LLWorldEnableSitOnGround : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = isAgentAvatarValid() && !gAgentAvatarp->mIsSitting;
//MK
		if (gRRenabled && gAgent.mRRInterface.contains("sit"))
		{
			new_value = false;
		}
//mk
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldSetAway : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAgent.getAFK())
		{
			gAgent.clearAFK();
		}
		else
		{
			gAgent.setAFK();
		}
		return true;
	}
};

class LLWorldSetBusy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAgent.getBusy())
		{
			gAgent.clearBusy();
		}
		else
		{
			gAgent.setBusy();
			LLNotifications::instance().add("BusyModeSet");
		}
		return true;
	}
};

class LLWorldCreateLandmark : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
		{
			return true;
		}
//mk
		LLViewerRegion* agent_region = gAgent.getRegion();
		if (!agent_region)
		{
			llwarns << "No agent region" << llendl;
			return true;
		}
		LLParcel* agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		if (!agent_parcel)
		{
			llwarns << "No agent parcel" << llendl;
			return true;
		}
		if (!agent_parcel->getAllowLandmark()
			&& !LLViewerParcelMgr::isParcelOwnedByAgent(agent_parcel, GP_LAND_ALLOW_LANDMARK))
		{
			LLNotifications::instance().add("CannotCreateLandmarkNotOwner");
			return true;
		}

		LLUUID folder_id;
		folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
		std::string pos_string;
		gAgent.buildLocationString(pos_string);

		create_inventory_item(gAgentID, gAgent.getSessionID(),
							  folder_id, LLTransactionID::tnull,
							  pos_string, pos_string, // name, desc
							  LLAssetType::AT_LANDMARK,
							  LLInventoryType::IT_LANDMARK,
							  NOT_WEARABLE, PERM_ALL, 
							  new LLCreateLandmarkCallback);
		return true;
	}
};

class LLToolsLookAtSelection : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		const F32 PADDING_FACTOR = 2.f;
		BOOL zoom = (userdata.asString() == "zoom");
		if (!LLSelectMgr::getInstance()->getSelection()->isEmpty())
		{
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);

			LLBBox selection_bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
			F32 angle_of_view = llmax(0.1f, LLViewerCamera::getInstance()->getAspect() > 1.f ? LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect() : LLViewerCamera::getInstance()->getView());
			F32 distance = selection_bbox.getExtentLocal().magVec() * PADDING_FACTOR / atan(angle_of_view);

			LLVector3 obj_to_cam = LLViewerCamera::getInstance()->getOrigin() - selection_bbox.getCenterAgent();
			obj_to_cam.normVec();

			LLUUID object_id;
			if (LLSelectMgr::getInstance()->getSelection()->getPrimaryObject())
			{
				object_id = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject()->mID;
			}
			if (zoom)
			{
				gAgent.setCameraPosAndFocusGlobal(LLSelectMgr::getInstance()->getSelectionCenterGlobal() + LLVector3d(obj_to_cam * distance), 
												LLSelectMgr::getInstance()->getSelectionCenterGlobal(), 
												object_id);
			}
			else
			{
				gAgent.setFocusGlobal(LLSelectMgr::getInstance()->getSelectionCenterGlobal(), object_id);
			}
		}
		return true;
	}
};

void callback_invite_to_group(LLUUID group_id, void *user_data)
{
	std::vector<LLUUID> agent_ids;
	agent_ids.push_back(*(LLUUID *)user_data);

	LLFloaterGroupInvite::showForGroup(group_id, &agent_ids);
}

void invite_to_group(const LLUUID& dest_id)
{
	LLViewerObject* dest = gObjectList.findObject(dest_id);
	if (dest && dest->isAvatar())
	{
		LLFloaterGroupPicker* widget;
		widget = LLFloaterGroupPicker::showInstance(LLSD(gAgentID));
		if (widget)
		{
			widget->center();
			widget->setPowersMask(GP_MEMBER_INVITE);
			widget->setSelectCallback(callback_invite_to_group, (void *)&dest_id);
		}
	}
}

class LLAvatarInviteToGroup : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
		{
			return false;
		}
//mk
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		if (avatar)
		{
			invite_to_group(avatar->getID());
		}
		return true;
	}
};

class LLAvatarAddFriend : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
		{
			return false;
		}
//mk
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		if (avatar && !is_agent_friend(avatar->getID()))
		{
			request_friendship(avatar->getID());
		}
		return true;
	}
};

bool complete_give_money(const LLSD& notification, const LLSD& response, LLObjectSelectionHandle handle)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		gAgent.clearBusy();
	}

	LLViewerObject* objectp = handle->getPrimaryObject();

	// Show avatar's name if paying attachment
	if (objectp && objectp->isAttachment())
	{
		while (objectp && !objectp->isAvatar())
		{
			objectp = (LLViewerObject*)objectp->getParent();
		}
	}

	if (objectp)
	{
		if (objectp->isAvatar())
		{
//MK
			if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
			{
				return false;
			}
//mk
			const BOOL is_group = FALSE;
			LLFloaterPay::payDirectly(&give_money,
									  objectp->getID(),
									  is_group);
		}
		else
		{
			LLFloaterPay::payViaObject(&give_money, objectp->getID());
		}
	}
	return false;
}

bool handle_give_money_dialog()
{
	LLNotification::Params params("BusyModePay");
	params.functor(boost::bind(complete_give_money, _1, _2, LLSelectMgr::getInstance()->getSelection()));

	if (gAgent.getBusy())
	{
		// warn users of being in busy mode during a transaction
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 1);
	}
	return true;
}

class LLPayObject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return handle_give_money_dialog();
	}
};

class LLEnablePayObject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		bool new_value = (avatar != NULL);
		if (!new_value)
		{
			LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
			if (object)
			{
				LLViewerObject *parent = (LLViewerObject *)object->getParent();
				if ((object->flagTakesMoney()) || (parent && parent->flagTakesMoney()))
				{
					new_value = true;
				}
			}
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLObjectEnableSitOrStand : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = false;
		LLViewerObject* dest_object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();

		if (dest_object)
		{
			if (dest_object->getPCode() == LL_PCODE_VOLUME)
			{
				new_value = true;
			}
//MK
			if (gRRenabled)
			{
				if (gAgent.mRRInterface.contains("sit"))
				{
					new_value = false;
				}
				if (gAgent.mRRInterface.contains("sittp") || gAgent.mRRInterface.mContainsFartouch)
				{
					LLPickInfo pick = LLToolPie::getInstance()->getPick();
					LLVector3 pos = dest_object->getPositionRegion() + pick.mObjectOffset;
					pos -= gAgent.getPositionAgent();
					if (pos.magVec() >= 1.5)
					{
						new_value = false;
					}
				}
			}
//mk
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);

		// Update label
		std::string label;
		std::string sit_text;
		std::string stand_text;
		std::string param = userdata["data"].asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			sit_text = param.substr(0, offset);
			stand_text = param.substr(offset+1);
		}
		if (sitting_on_selection())
		{
			label = stand_text;
		}
		else
		{
			LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
			if (node && node->mValid && !node->mSitName.empty())
			{
				label.assign(node->mSitName);
			}
			else
			{
				label = sit_text;
			}
		}
		gMenuHolder->childSetText("Object Sit", label);

		return true;
	}
};

void decode_ui_sounds(void*)
{
	audio_preload_ui_sounds(true);
}

void dump_select_mgr(void*)
{
	LLSelectMgr::getInstance()->dump();
}

void dump_inventory(void*)
{
	gInventory.dumpInventory();
}

// forcibly unlock an object
void handle_force_unlock(void*)
{
	// First, make it public.
	LLSelectMgr::getInstance()->sendOwner(LLUUID::null, LLUUID::null, TRUE);

	// Second, lie to the viewer and mark it editable and unowned

	struct f : public LLSelectedObjectFunctor
	{
		virtual bool apply(LLViewerObject* object)
		{
			object->mFlags |= FLAGS_OBJECT_MOVE;
			object->mFlags |= FLAGS_OBJECT_MODIFY;
			object->mFlags |= FLAGS_OBJECT_COPY;

			object->mFlags &= ~FLAGS_OBJECT_ANY_OWNER;
			object->mFlags &= ~FLAGS_OBJECT_YOU_OWNER;
			return true;
		}
	} func;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&func);
}

void handle_dump_followcam(void*)
{
	LLFollowCamMgr::dump();
}

BOOL check_message_logging(void*)
{
	return gMessageSystem->mVerboseLog;
}

void handle_viewer_toggle_message_log(void*)
{
	if (gMessageSystem->mVerboseLog)
	{
		gMessageSystem->stopLogging();
	}
	else
	{
		gMessageSystem->startLogging();
	}
}

// TomY TODO: Move!
class LLShowFloater : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string floater_name = userdata.asString();
		if (floater_name == "gestures")
		{
			LLFloaterGesture::toggleVisibility();
		}
		else if (floater_name == "appearance")
		{
			if (gAgentWearables.areWearablesLoaded())
			{
				gAgent.changeCameraToCustomizeAvatar();
			}
		}
		else if (floater_name == "friends")
		{
			LLFloaterFriends::toggle(NULL);
		}
		else if (floater_name == "groups")
		{
			LLFloaterGroups::toggle(NULL);
		}
		else if (floater_name == "preferences")
		{
			LLFloaterPreference::show(NULL);
		}
		else if (floater_name == "toolbar")
		{
			LLToolBar::toggle(NULL);
		}
		else if (floater_name == "displayname")
		{
			LLFloaterDisplayName::show();
		}
		else if (floater_name == "chat history")
		{
			LLFloaterChat::toggleInstance(LLSD());
		}
		else if (floater_name == "teleport history")
		{
			gFloaterTeleportHistory->toggle();
		}
		else if (floater_name == "im")
		{
			LLIMMgr::toggle(NULL);
		}
		else if (floater_name == "inventory")
		{
			LLInventoryView::toggleVisibility(NULL);
		}
		else if (floater_name == "mute list")
		{
			LLFloaterMute::toggleInstance();
		}
		else if (floater_name == "media filter")
		{
			SLFloaterMediaFilter::toggleInstance();
		}
		else if (floater_name == "camera controls")
		{
			LLFloaterCamera::toggleInstance();
		}
		else if (floater_name == "movement controls")
		{
			LLFloaterMove::toggleInstance();
		}
		else if (floater_name == "world map")
		{
			LLFloaterWorldMap::toggle(NULL);
		}
		else if (floater_name == "mini map")
		{
			LLFloaterMap::toggleInstance();
		}
		else if (floater_name == "stat bar")
		{
			LLFloaterStats::toggleInstance();
		}
		else if (floater_name == "my land")
		{
			LLFloaterLandHoldings::show(NULL);
		}
		else if (floater_name == "about land")
		{
			if (LLViewerParcelMgr::getInstance()->selectionEmpty())
			{
				LLViewerParcelMgr::getInstance()->selectParcelAt(gAgent.getPositionGlobal());
			}
//MK
			if (!gRRenabled || !gAgent.mRRInterface.mContainsShowloc)
			{
//mk
				LLFloaterLand::showInstance();
//MK
			}
//mk
		}
		else if (floater_name == "buy land")
		{
			if (LLViewerParcelMgr::getInstance()->selectionEmpty())
			{
				LLViewerParcelMgr::getInstance()->selectParcelAt(gAgent.getPositionGlobal());
			}
//MK
			if (!gRRenabled || !gAgent.mRRInterface.mContainsShowloc)
			{
//mk
				LLViewerParcelMgr::getInstance()->startBuyLand();
//MK
			}
//mk
		}
		else if (floater_name == "about region")
		{
//MK
			if (!gRRenabled || !gAgent.mRRInterface.mContainsShowloc)
			{
//mk
				LLFloaterRegionInfo::showInstance();
//MK
			}
//mk
		}
		else if (floater_name == "areasearch")
		{
			JCFloaterAreaSearch::toggle();
		}
		else if (floater_name == "grid options")
		{
			LLFloaterBuildOptions::show(NULL);
		}
		else if (floater_name == "script errors")
		{
			LLFloaterScriptDebug::show(LLUUID::null);
		}
		else if (floater_name == "help f1")
		{
			llinfos << "Spawning HTML help window" << llendl;
			gViewerHtmlHelp.show();
		}
		else if (floater_name == "help tutorial")
		{
			LLFloaterHUD::showHUD();
		}
		else if (floater_name == "complaint reporter")
		{
			// Prevent menu from appearing in screen shot.
			gMenuHolder->hideMenus();
			LLFloaterReporter::showFromMenu(COMPLAINT_REPORT);
		}
		else if (floater_name == "mean events")
		{
			if (!gNoRender)
			{
				LLFloaterBump::show(NULL);
			}
		}
		else if (floater_name == "lag meter")
		{
			LLFloaterLagMeter::showInstance();
		}
		else if (floater_name == "bug reporter")
		{
			// Prevent menu from appearing in screen shot.
			gMenuHolder->hideMenus();
			LLFloaterReporter::showFromMenu(BUG_REPORT);
		}
		else if (floater_name == "buy currency")
		{
			LLFloaterBuyCurrency::buyCurrency();
		}
		else if (floater_name == "about")
		{
			LLFloaterAbout::show(NULL);
		}
		else if (floater_name == "active speakers")
		{
			LLFloaterActiveSpeakers::toggleInstance(LLSD());
		}
		else if (floater_name == "beacons")
		{
			LLFloaterBeacons::toggleInstance(LLSD());
		}
		else if (floater_name == "perm prefs")
		{
			LLFloaterPerms::toggleInstance(LLSD());
		}
		return true;
	}
};

class LLFloaterVisible : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string control_name = userdata["control"].asString();
		std::string floater_name = userdata["data"].asString();
		bool new_value = false;
		if (floater_name == "friends")
		{
			new_value = LLFloaterFriends::visible(NULL);
		}
		else if (floater_name == "groups")
		{
			new_value = LLFloaterGroups::visible(NULL);
		}
		else if (floater_name == "communicate")
		{
			new_value = LLFloaterChatterBox::instanceVisible();
		}
		else if (floater_name == "toolbar")
		{
			new_value = LLToolBar::visible(NULL);
		}
		else if (floater_name == "chat history")
		{
			new_value = LLFloaterChat::instanceVisible();
		}
		else if (floater_name == "teleport history")
		{
			new_value = gFloaterTeleportHistory->getVisible();
		}
		else if (floater_name == "im")
		{
			new_value = LLFloaterChatterBox::instanceVisible(0);
		}
		else if (floater_name == "mute list")
		{
			new_value = LLFloaterMute::instanceVisible();
		}
		else if (floater_name == "media filter")
		{
			new_value = SLFloaterMediaFilter::instanceVisible();
		}
		else if (floater_name == "camera controls")
		{
			new_value = LLFloaterCamera::instanceVisible();
		}
		else if (floater_name == "movement controls")
		{
			new_value = LLFloaterMove::instanceVisible();
		}
		else if (floater_name == "stat bar")
		{
			new_value = LLFloaterStats::instanceVisible();
		}
		else if (floater_name == "active speakers")
		{
			new_value = LLFloaterActiveSpeakers::instanceVisible(LLSD());
		}
		else if (floater_name == "beacons")
		{
			new_value = LLFloaterBeacons::instanceVisible(LLSD());
		}
		else if (floater_name == "inventory")
		{
			LLInventoryView* iv = LLInventoryView::getActiveInventory(); 
			new_value = (NULL != iv && TRUE == iv->getVisible());
		}
		else if (floater_name == "areasearch")
		{
			JCFloaterAreaSearch* instn = JCFloaterAreaSearch::getInstance();
			if (!instn) new_value = false;
			else new_value = instn->getVisible();
		}
		gMenuHolder->findControl(control_name)->setValue(new_value);
		return true;
	}
};

bool callback_show_url(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLWeb::loadURL(notification["payload"]["url"].asString());
	}
	return false;
}

class LLPromptShowURL : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string param = userdata.asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			std::string alert = param.substr(0, offset);
			std::string url = param.substr(offset+1);

			if (gSavedSettings.getBOOL("UseExternalBrowser"))
			{ 
    			LLSD payload;
    			payload["url"] = url;
    			LLNotifications::instance().add(alert, LLSD(), payload, callback_show_url);
			}
			else
			{
		        LLWeb::loadURL(url);
			}
		}
		else
		{
			llinfos << "PromptShowURL invalid parameters! Expecting \"ALERT,URL\"." << llendl;
		}
		return true;
	}
};

bool callback_show_file(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLWeb::loadURL(notification["payload"]["url"]);
	}
	return false;
}

class LLPromptShowFile : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string param = userdata.asString();
		std::string::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			std::string alert = param.substr(0, offset);
			std::string file = param.substr(offset+1);

			LLSD payload;
			payload["url"] = file;
			LLNotifications::instance().add(alert, LLSD(), payload, callback_show_file);
		}
		else
		{
			llinfos << "PromptShowFile invalid parameters! Expecting \"ALERT,FILE\"." << llendl;
		}
		return true;
	}
};

class LLShowAgentProfile : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLUUID agent_id;
		if (userdata.asString() == "agent")
		{
			agent_id = gAgentID;
		}
		else if (userdata.asString() == "hit object")
		{
//MK
			if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
			{
				return false;
			}
//mk
			LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
			if (objectp)
			{
				agent_id = objectp->getID();
			}
		}
		else
		{
			agent_id = userdata.asUUID();
		}

		LLVOAvatar* avatar = find_avatar_from_object(agent_id);
		if (avatar)
		{
			LLFloaterAvatarInfo::show(avatar->getID());
		}
		return true;
	}
};

void handle_focus(void *)
{
	if (gDisconnected)
	{
		return;
	}

	if (gAgent.getFocusOnAvatar())
	{
		// zoom in if we're looking at the avatar
		gAgent.setFocusOnAvatar(FALSE, ANIMATE);
		gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
		gAgent.cameraZoomIn(0.666f);
	}
	else
	{
		gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
	}

	gViewerWindow->moveCursorToCenter();

	// Switch to camera toolset
//	LLToolMgr::getInstance()->setCurrentToolset(gCameraToolset);
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool(LLToolCamera::getInstance());
}

class LLLandEdit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (!gFloaterTools) return false;
//MK
		if (gRRenabled && (gAgent.mRRInterface.mContainsRez || gAgent.mRRInterface.mContainsEdit))
		{
			return false;
		}
//mk
		if (gAgent.getFocusOnAvatar() && gSavedSettings.getBOOL("EditCameraMovement"))
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());

			gAgent.cameraOrbitOver(F_PI * 0.25f);
			gViewerWindow->moveCursorToCenter();
		}
		else if (gSavedSettings.getBOOL("EditCameraMovement"))
		{
			gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
			gViewerWindow->moveCursorToCenter();
		}

		LLViewerParcelMgr::getInstance()->selectParcelAt(LLToolPie::getInstance()->getPick().mPosGlobal);

		gFloaterView->bringToFront(gFloaterTools);

		// Switch to land edit toolset
		LLToolMgr::getInstance()->getCurrentToolset()->selectTool(LLToolSelectLand::getInstance());
		return true;
	}
};

class LLWorldEnableBuyLand : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLViewerParcelMgr::getInstance()->canAgentBuyParcel(
								LLViewerParcelMgr::getInstance()->selectionEmpty()
									? LLViewerParcelMgr::getInstance()->getAgentParcel()
									: LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel(),
								false);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_buy_land(void*)
{
	return LLViewerParcelMgr::getInstance()->canAgentBuyParcel(
				LLViewerParcelMgr::getInstance()->getParcelSelection()->getParcel(), false);
}

void handle_move(void*)
{
	if (gAgent.getFocusOnAvatar())
	{
		// zoom in if we're looking at the avatar
		gAgent.setFocusOnAvatar(FALSE, ANIMATE);
		gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());

		gAgent.cameraZoomIn(0.666f);
	}
	else
	{
		gAgent.setFocusGlobal(LLToolPie::getInstance()->getPick());
	}

	gViewerWindow->moveCursorToCenter();

	LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool(LLToolGrab::getInstance());
}

class LLObjectAttachToAvatar : public view_listener_t
{
public:
	static void setObjectSelection(LLObjectSelectionHandle selection) { sObjectSelection = selection; }

private:
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsRez)
		{
			return false; // we can't take objects when unable to rez
		}
//mk
		setObjectSelection(LLSelectMgr::getInstance()->getSelection());
		LLViewerObject* selectedObject = sObjectSelection->getFirstRootObject();
		if (selectedObject)
		{
			S32 index = userdata.asInteger();
			LLViewerJointAttachment* attachment_point = NULL;
			if (index > 0)
				attachment_point = get_if_there(gAgentAvatarp->mAttachmentPoints, index, (LLViewerJointAttachment*)NULL);
//MK
			if (gRRenabled)
			{
				if (index == 0 && gAgent.mRRInterface.mContainsDetach)
				{
					setObjectSelection (NULL);
					return false; // something is locked and we're attempting a Wear in-world
				}
				if (attachment_point
					&& !gAgent.mRRInterface.canAttach(NULL, attachment_point->getName()))
				{
					setObjectSelection (NULL);
					return false;
				}
			}
//mk
			confirm_replace_attachment(0, attachment_point);
		}
		return true;
	}

protected:
	static LLObjectSelectionHandle sObjectSelection;
};

LLObjectSelectionHandle LLObjectAttachToAvatar::sObjectSelection;

void near_attach_object(BOOL success, void *user_data)
{
	if (success)
	{
		gAgent.setFlying(FALSE);
		LLViewerJointAttachment *attachment = (LLViewerJointAttachment *)user_data;

		U8 attachment_id = 0;
		if (attachment)
		{
			for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
				 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
			{
				if (iter->second == attachment)
				{
					attachment_id = iter->first;
					break;
				}
			}
		}
		else
		{
			// interpret 0 as "default location"
			attachment_id = 0;
		}
		LLSelectMgr::getInstance()->sendAttach(attachment_id);
	}
	LLObjectAttachToAvatar::setObjectSelection(NULL);
}

void confirm_replace_attachment(S32 option, void* user_data)
{
	if (option == 0/*YES*/)
	{
		LLViewerObject* selectedObject = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
		if (selectedObject)
		{
			const F32 MIN_STOP_DISTANCE = 1.f;	// meters
			const F32 ARM_LENGTH = 0.5f;		// meters
			const F32 SCALE_FUDGE = 1.5f;

			F32 stop_distance = SCALE_FUDGE * selectedObject->getMaxScale() + ARM_LENGTH;
			if (stop_distance < MIN_STOP_DISTANCE)
			{
				stop_distance = MIN_STOP_DISTANCE;
			}

			LLVector3 walkToSpot = selectedObject->getPositionAgent();

			// make sure we stop in front of the object
			LLVector3 delta = walkToSpot - gAgent.getPositionAgent();
			delta.normVec();
			delta = delta * 0.5f;
			walkToSpot -= delta;

			gAgent.startAutoPilotGlobal(gAgent.getPosGlobalFromAgent(walkToSpot), "Attach", NULL, near_attach_object, user_data, stop_distance);
			gAgent.clearFocusObject();
		}
	}
}

class LLAttachmentDrop : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Called when the user clicked on an object attached to them
		// and selected "Drop".
		LLViewerObject *object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (!object)
		{
			llwarns << "handle_drop_attachment() - no object to drop" << llendl;
			return true;
		}

		LLViewerObject *parent = (LLViewerObject*)object->getParent();
		while (parent)
		{
			if (parent->isAvatar())
			{
				break;
			}
			object = parent;
			parent = (LLViewerObject*)parent->getParent();
		}

		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}

		if (object->isAvatar())
		{
			llwarns << "Trying to detach avatar from avatar." << llendl;
			return true;
		}

		// The sendDropAttachment() method works on the list of selected
		// objects.  Thus we need to clear the list, make sure it only
		// contains the object the user clicked, send the message,
		// then clear the list.
		LLSelectMgr::getInstance()->sendDropAttachment();
		return true;
	}
};

// called from avatar pie menu
void handle_detach_from_avatar(void* user_data)
{
	LLViewerJointAttachment *attachment = (LLViewerJointAttachment*)user_data;

	if (attachment->getNumObjects() > 0)
	{
//MK
		if (gRRenabled && !gAgent.mRRInterface.canDetachAllObjectsFromAttachment(attachment))
		{
			return;
		}
//mk
		gMessageSystem->newMessage("ObjectDetach");
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

		for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator iter = attachment->mAttachedObjects.begin();
			 iter != attachment->mAttachedObjects.end();
			 iter++)
		{
			LLViewerObject *attached_object = (*iter);
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, attached_object->getLocalID());
		}
		gMessageSystem->sendReliable(gAgent.getRegionHost());
	}
}

void attach_label(std::string& label, void* user_data)
{
	LLViewerJointAttachment *attachment = (LLViewerJointAttachment*)user_data;
	if (attachment)
	{
		label = attachment->getName();
		for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			const LLViewerObject* attached_object = (*attachment_iter);
			if (attached_object)
			{
				LLViewerInventoryItem* itemp = gInventory.getItem(attached_object->getAttachmentItemID());
				if (itemp)
				{
					label += std::string(" (") + itemp->getName() + std::string(")");
					break;
				}
			}
		}
	}
}

void detach_label(std::string& label, void* user_data)
{
	LLViewerJointAttachment *attachment = (LLViewerJointAttachment*)user_data;
	if (attachment)
	{
		label = attachment->getName();
		for (LLViewerJointAttachment::attachedobjs_vec_t::const_iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			const LLViewerObject* attached_object = (*attachment_iter);
			if (attached_object)
			{
				LLViewerInventoryItem* itemp = gInventory.getItem(attached_object->getAttachmentItemID());
				if (itemp)
				{
					label += std::string(" (") + itemp->getName() + std::string(")");
					break;
				}
			}
		}
	}
}

class LLAttachmentDetach : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Called when the user clicked on an object attached to them
		// and selected "Detach".
		LLViewerObject *object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}
//MK
		if (gRRenabled && !gAgent.mRRInterface.canDetachAllSelectedObjects())
		{
			return true;
		}
//mk
		LLViewerObject *parent = (LLViewerObject*)object->getParent();
		while (parent)
		{
			if (parent->isAvatar())
			{
				break;
			}
			object = parent;
			parent = (LLViewerObject*)parent->getParent();
		}

		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}

		if (object->isAvatar())
		{
			llwarns << "Trying to detach avatar from avatar." << llendl;
			return true;
		}

		// The sendDetach() method works on the list of selected
		// objects.  Thus we need to clear the list, make sure it only
		// contains the object the user clicked, send the message,
		// then clear the list.
		// We use deselectAll to update the simulator's notion of what's
		// selected, and removeAll just to change things locally.
		//RN: I thought it was more useful to detach everything that was selected
		if (LLSelectMgr::getInstance()->getSelection()->isAttachment())
		{
			LLSelectMgr::getInstance()->sendDetach();
		}
		return true;
	}
};

//Adding an observer for a Jira 2422 and needs to be a fetch observer
//for Jira 3119
class LLWornItemFetchedObserver : public LLInventoryFetchObserver
{
public:
	LLWornItemFetchedObserver() {}
	virtual ~LLWornItemFetchedObserver() {}

protected:
	virtual void done()
	{
		gPieAttachment->buildDrawLabels();
		gInventory.removeObserver(this);
		delete this;
	}
};

// You can only drop items on parcels where you can build.
class LLAttachmentEnableDrop : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Add an inventory observer to only allow dropping the newly attached
		// item once it exists in your inventory.  Look at Jira 2422. -jwolk

		// A bug occurs when you wear/drop an item before it actively is added
		// to your inventory if this is the case (you're on a slow sim, etc.),
		// a copy of the object, well, a newly created object with the same
		// properties, is placed in your inventory. Therefore, we disable the
		// drop option until the item is in your inventory

		LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
		LLViewerJointAttachment* attachment_pt = NULL;
		LLInventoryItem* item = NULL;

		if (object)
		{
    		S32 attachmentID  = ATTACHMENT_ID_FROM_STATE(object->getState());
			attachment_pt = get_if_there(gAgentAvatarp->mAttachmentPoints,
										 attachmentID,
										 (LLViewerJointAttachment*)NULL);

			if (attachment_pt)
			{
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator
						attachment_iter = attachment_pt->mAttachedObjects.begin(),
						end = attachment_pt->mAttachedObjects.end();
					 attachment_iter != end; ++attachment_iter)
				{
					// make sure item is in your inventory (it could be a
					// delayed attach message being sent from the sim) so check
					// to see if the item is in the inventory already
					item = gInventory.getItem((*attachment_iter)->getAttachmentItemID());
					if (!item)
					{
						// Item does not exist, make an observer to enable the
						// pie menu when the item finishes fetching worst case
						// scenario if a fetch is already out there (being sent
						// from a slow sim) we refetch and there are 2 fetches
						LLWornItemFetchedObserver* wornItemFetched;
						wornItemFetched = new LLWornItemFetchedObserver();
						// add item to the inventory item to be fetched:
						LLInventoryFetchObserver::item_ref_t items;

						items.push_back((*attachment_iter)->getAttachmentItemID());

						wornItemFetched->fetchItems(items);
						gInventory.addObserver(wornItemFetched);
					}
				}
			}
		}

		// now check to make sure that the item is actually in the inventory
		// before we enable dropping it
		bool new_value = enable_detach(NULL) && item &&
						 LLViewerParcelMgr::getInstance()->agentCanBuild();

		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_detach(void*)
{
	LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getPrimaryObject();
	if (!object) return FALSE;
	if (!object->isAttachment()) return FALSE;
//MK
	if (gRRenabled)
	{
		if (!gAgent.mRRInterface.canDetach(object))
		{
			return FALSE;
		}

		// prevent a clever workaround that allowed to detach several objects
		// at the same time by selecting them
		if (gAgent.mRRInterface.mContainsDetach &&
			LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() > 1)
		{
			return FALSE;
		}
	}
//mk
	// Find the avatar who owns this attachment
	LLViewerObject* avatar = object;
	while (avatar)
	{
		// ...if it's you, good to detach
		if (avatar->getID() == gAgentID)
		{
			return TRUE;
		}

		avatar = (LLViewerObject*)avatar->getParent();
	}

	return FALSE;
}

class LLAttachmentEnableDetach : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = enable_detach(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// Used to tell if the selected object can be attached to your avatar.
BOOL object_selected_and_point_valid(void *user_data)
{
	//LLViewerJointAttachment *attachment = (LLViewerJointAttachment *)user_data;
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	for (LLObjectSelection::root_iterator iter = selection->root_begin();
		 iter != selection->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		LLViewerObject::const_child_list_t& child_list = object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); iter++)
		{
			LLViewerObject* child = *iter;
			if (child->isAvatar())
			{
				return FALSE;
			}
		}
	}

	return (selection->getRootObjectCount() == 1) && 
		(selection->getFirstRootObject()->getPCode() == LL_PCODE_VOLUME) && 
		selection->getFirstRootObject()->permYouOwner() &&
		!((LLViewerObject*)selection->getFirstRootObject()->getRoot())->isAvatar() && 
		(selection->getFirstRootObject()->getNVPair("AssetContainer") == NULL);
}

BOOL object_is_wearable()
{
	if (!object_selected_and_point_valid(NULL))
	{
		return FALSE;
	}
	if (sitting_on_selection())
	{
		return FALSE;
	}
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	for (LLObjectSelection::valid_root_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		if (node->mPermissions->getOwner() == gAgentID)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// Also for seeing if object can be attached.  See above.
class LLObjectEnableWear : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsDetach)
		{
			return false;
		}
//mk
		bool is_wearable = object_selected_and_point_valid(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(is_wearable);
		return TRUE;
	}
};

BOOL object_attached(void *user_data)
{
	LLViewerJointAttachment *attachment = (LLViewerJointAttachment *)user_data;

	return attachment->getNumObjects() > 0;
}

class LLAvatarSendIM : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShownames)
		{
			return false;
		}
//mk
		LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
		if (avatar)
		{
			std::string name("IM");
			LLNameValue *first = avatar->getNVPair("FirstName");
			LLNameValue *last = avatar->getNVPair("LastName");
			if (first && last)
			{
				name.assign(first->getString());
				name.append(" ");
				name.append(last->getString());
			}

			gIMMgr->setFloaterOpen(TRUE);
			//EInstantMessage type = have_agent_callingcard(gLastHitObjectID)
			//	? IM_SESSION_ADD : IM_SESSION_CARDLESS_START;
			gIMMgr->addSession(name,
								IM_NOTHING_SPECIAL,
								avatar->getID());
		}
		return true;
	}
};

void handle_activate(void*)
{
}

BOOL enable_activate(void*)
{
	return FALSE;
}

namespace
{
	struct QueueObjects : public LLSelectedObjectFunctor
	{
		BOOL scripted;
		BOOL modifiable;
		LLFloaterScriptQueue* mQueue;
		QueueObjects(LLFloaterScriptQueue* q) : mQueue(q), scripted(FALSE), modifiable(FALSE) {}
		virtual bool apply(LLViewerObject* obj)
		{
			scripted = obj->flagScripted();
			modifiable = obj->permModify();

			if (scripted && modifiable)
			{
				mQueue->addObject(obj->getID());
				return false;
			}
			else
			{
				return true; // fail: stop applying
			}
		}
	};
}

void queue_actions(LLFloaterScriptQueue* q, const std::string& noscriptmsg, const std::string& nomodmsg)
{
	QueueObjects func(q);
	LLSelectMgr *mgr = LLSelectMgr::getInstance();
	LLObjectSelectionHandle selectHandle = mgr->getSelection();
	bool fail = selectHandle->applyToObjects(&func);
	if (fail)
	{
		if (!func.scripted)
		{
			LLNotifications::instance().add(noscriptmsg);
		}
		else if (!func.modifiable)
		{
			LLNotifications::instance().add(nomodmsg);
		}
		else
		{
			llerrs << "Bad logic." << llendl;
		}
	}
	else
	{
		if (!q->start())
		{
			llwarns << "Unexpected script compile failure." << llendl;
		}
	}
}

void handle_compile_queue(std::string to_lang)
{
	LLFloaterCompileQueue* queue;
	if (to_lang == "mono")
	{
		queue = LLFloaterCompileQueue::create(TRUE);
	}
	else
	{
		queue = LLFloaterCompileQueue::create(FALSE);
	}
	queue_actions(queue, "CannotRecompileSelectObjectsNoScripts", "CannotRecompileSelectObjectsNoPermission");
}

void handle_reset_selection(void)
{
	LLFloaterResetQueue* queue = LLFloaterResetQueue::create();
	queue_actions(queue, "CannotResetSelectObjectsNoScripts", "CannotResetSelectObjectsNoPermission");
}

void handle_set_run_selection(void)
{
	LLFloaterRunQueue* queue = LLFloaterRunQueue::create();
	queue_actions(queue, "CannotSetRunningSelectObjectsNoScripts", "CannotSerRunningSelectObjectsNoPermission");
}

void handle_set_not_run_selection(void)
{
	LLFloaterNotRunQueue* queue = LLFloaterNotRunQueue::create();
	queue_actions(queue, "CannotSetRunningNotSelectObjectsNoScripts", "CannotSerRunningNotSelectObjectsNoPermission");
}

class LLToolsSelectedScriptAction : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		// If there is at least one object locked in the selection, don't allow anything
		if (gRRenabled && !gAgent.mRRInterface.canDetachAllSelectedObjects())
		{
			return true;
		}
//mk
		std::string action = userdata.asString();
		if (action == "compile mono")
		{
			handle_compile_queue("mono");
		}
		if (action == "compile lsl")
		{
			handle_compile_queue("lsl");
		}
		else if (action == "reset")
		{
			handle_reset_selection();
		}
		else if (action == "start")
		{
			handle_set_run_selection();
		}
		else if (action == "stop")
		{
			handle_set_not_run_selection();
		}
		return true;
	}
};

void handle_selected_texture_info(void*)
{
	for (LLObjectSelection::valid_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_end(); iter++)
	{
		LLSelectNode* node = *iter;

		std::string msg;
		msg.assign("Texture info for primitive \"");
		msg.append(node->mName);
		msg.append("\" (UUID: ");
		msg.append(node->getObject()->getID().asString());
		msg.append("):");
		LLChat chat(msg);
		LLFloaterChat::addChat(chat);

		U8 te_count = node->getObject()->getNumTEs();
		// map from texture ID to list of faces using it
		typedef std::map< LLUUID, std::vector<U8> > map_t;
		map_t faces_per_texture;
		for (U8 i = 0; i < te_count; i++)
		{
			if (!node->isTESelected(i)) continue;

			LLViewerTexture* img = node->getObject()->getTEImage(i);
			LLUUID image_id = img->getID();
			faces_per_texture[image_id].push_back(i);
		}
		// Per-texture, dump which faces are using it.
		map_t::iterator it;
		for (it = faces_per_texture.begin(); it != faces_per_texture.end(); ++it)
		{
			LLUUID image_id = it->first;
			U8 te = it->second[0];
			LLViewerTexture* img = node->getObject()->getTEImage(te);
			S32 height = img->getHeight();
			S32 width = img->getWidth();
			S32 components = img->getComponents();
			std::string image_id_string;
			if (node->mPermissions->getOwner() == gAgentID &&
				((gIsInSecondLife && gAgentID == node->mPermissions->getCreator()) ||
				(!gIsInSecondLife && (node->mPermissions->getMaskOwner() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)))
			{
				image_id_string = image_id.asString() + " ";
			}
			msg = llformat("%s%dx%d %s on face ",
								image_id_string.c_str(),
								width,
								height,
								(components == 4 ? "alpha" : "opaque"));
			for (U8 i = 0; i < it->second.size(); ++i)
			{
				msg.append(llformat("%d ", (S32)(it->second[i])));
			}
			LLChat chat(msg);
			LLFloaterChat::addChat(chat);
		}
	}
}

void reload_selected_texture(void*)
{
	std::set<LLUUID> reloaded;
	for (LLObjectSelection::valid_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->valid_end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if (!object) continue;
		if (object->getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
		{
			LLSculptParams* sculpt;
			sculpt = (LLSculptParams*)object->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
			if ((sculpt->getSculptType() & LL_SCULPT_TYPE_MASK) != LL_SCULPT_TYPE_MESH)
			{
				LLUUID uuid = sculpt->getSculptTexture();
				if (uuid.notNull())
				{
					if (!reloaded.count(uuid))
					{
						LLAppViewer::getTextureCache()->removeFromCache(uuid); // remove cache entry
						LLViewerFetchedTexture* tex;
						tex = LLViewerTextureManager::getFetchedTexture(uuid, TRUE,
																		LLViewerTexture::BOOST_NONE,
																		LLViewerTexture::LOD_TEXTURE);
						if (tex)
						{
							tex->forceRefetch();			// force a reload of the raw image from network
							tex->destroyGLTexture();		// will force a reload of image in GL
						}
						reloaded.insert(uuid);					// mark as reloaded
					}
					object->markForUpdate(TRUE);	// force an object geometry rebuild
				}
			}
		}
		U8 te_count = object->getNumTEs();
		for (U8 i = 0; i < te_count; i++)
		{
			if (!node->isTESelected(i)) continue;

			LLViewerTexture* img = object->getTEImage(i);
			if (img)
			{
				LLUUID uuid = img->getID();
				if (uuid.notNull())
				{
					object->setTETexture(i, IMG_DEFAULT);	// to flag as texture changed
					if (!reloaded.count(uuid))
					{
						LLAppViewer::getTextureCache()->removeFromCache(uuid); // remove cache entry
						LLViewerFetchedTexture* tex = LLViewerTextureManager::staticCastToFetchedTexture(img);
						if (tex)
						{
							tex->forceRefetch();				// force a reload of the raw image from network
						}
						img->destroyGLTexture();				// will force a reload of image in GL
						reloaded.insert(uuid);					// mark as reloaded
					}
					object->setTETexture(i, uuid);			// will rebind the texture in GL
				}
			}
		}
	}
}

void reload_texture(const LLUUID& id)
{
	// Remove the cache entry
	LLAppViewer::getTextureCache()->removeFromCache(id);
	LLViewerFetchedTexture* te = LLViewerTextureManager::getFetchedTexture(id);
	if (te)
	{
		// Force a reload of the raw image from network
		te->forceRefetch();
		// Force a reload of the image in GL from the fresh raw image
		te->destroyGLTexture();
	}
}

// static
void handle_refresh_baked_textures(LLVOAvatar* avatar)
{
	// Force-reload the avatar's known baked textures
	reload_texture(avatar->getTE(TEX_HAIR_BAKED)->getID());
	reload_texture(avatar->getTE(TEX_EYES_BAKED)->getID());
	reload_texture(avatar->getTE(TEX_HEAD_BAKED)->getID());
	reload_texture(avatar->getTE(TEX_UPPER_BAKED)->getID());
	reload_texture(avatar->getTE(TEX_LOWER_BAKED)->getID());
	reload_texture(avatar->getTE(TEX_SKIRT_BAKED)->getID());
	// Request again the baked textures in case we would have missed a refresh
	// (new baked texture UUID missed due to a lost packet, for example).
	avatar->sendAvatarTexturesRequest(true);
}

void handle_dump_image_list(void*)
{
	gTextureList.dump();
}

void handle_test_male(void*)
{
//MK
	if (gRRenabled && (gAgent.mRRInterface.mContainsDetach ||
		gAgent.mRRInterface.contains("remoutfit") ||
		gAgent.mRRInterface.contains("addoutfit")))
	{
		return;
	}
//mk
	LLAppearanceMgr::instance().wearOutfitByName("Male Shape & Outfit");
	//gGestureList.requestResetFromServer(TRUE);
}

void handle_test_female(void*)
{
//MK
	if (gRRenabled && (gAgent.mRRInterface.mContainsDetach ||
		gAgent.mRRInterface.contains("remoutfit") ||
		gAgent.mRRInterface.contains("addoutfit")))
	{
		return;
	}
//mk
	LLAppearanceMgr::instance().wearOutfitByName("Female Shape & Outfit");
	//gGestureList.requestResetFromServer(FALSE);
}

void handle_toggle_pg(void*)
{
	gAgent.setTeen(!gAgent.isTeen());

	LLFloaterWorldMap::reloadIcons(NULL);

	llinfos << "PG status set to " << (S32)gAgent.isTeen() << llendl;
}

void handle_dump_attachments(void*)
{
	if (!isAgentAvatarValid())
	{
		llinfos << "NO AVATAR" << llendl;
		return;
	}
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		S32 key = curiter->first;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *attached_object = (*attachment_iter);
			BOOL visible = (attached_object != NULL &&
							attached_object->mDrawable.notNull() && 
							!attached_object->mDrawable->isRenderType(0));
			LLVector3 pos;
			if (visible) pos = attached_object->mDrawable->getPosition();
			llinfos << "ATTACHMENT " << key << ": item_id=" << attached_object->getAttachmentItemID()
					<< (attached_object ? " present " : " absent ")
					<< (visible ? "visible " : "invisible ")
					<<  " at " << pos
					<< " and " << (visible ? attached_object->getPosition() : LLVector3::zero)
					<< llendl;
		}
	}
}

//---------------------------------------------------------------------
// Callbacks for enabling/disabling items
//---------------------------------------------------------------------

BOOL menu_ui_enabled(void *user_data)
{
	BOOL high_res = gSavedSettings.getBOOL("HighResSnapshot");
	return !high_res;
}

void menu_toggle_control(void* user_data)
{
	std::string setting(static_cast<char*>(user_data));
	BOOL checked = !gSavedSettings.getBOOL(setting);
	if (checked && setting == "HighResSnapshot")
	{
		// High Res Snapshot active, must uncheck RenderUIInSnapshot
		gSavedSettings.setBOOL("RenderUIInSnapshot", FALSE);
	}
	else if (!checked && setting == "RenderDeferredSSAO")
	{
		// Global Illumination requires SSAO to prevent black screens
		gSavedSettings.setBOOL("RenderDeferredGI", FALSE);
	}
	gSavedSettings.setBOOL(setting, checked);
}

// these are used in the gl menus to set control values.
class LLToggleControl : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string control_name = userdata.asString();
		BOOL checked = gSavedSettings.getBOOL(control_name);
		if (control_name == "HighResSnapshot" && !checked)
		{
			// High Res Snapshot active, must uncheck RenderUIInSnapshot
			gSavedSettings.setBOOL("RenderUIInSnapshot", FALSE);
		}
		gSavedSettings.setBOOL(control_name, !checked);
		return true;
	}
};

BOOL menu_check_control(void* user_data)
{
	return gSavedSettings.getBOOL((char*)user_data);
}

// 
void menu_toggle_variable(void* user_data)
{
	BOOL checked = *(BOOL*)user_data;
	*(BOOL*)user_data = !checked;
}

BOOL menu_check_variable(void* user_data)
{
	return *(BOOL*)user_data;
}

BOOL enable_land_selected(void*)
{
	return !(LLViewerParcelMgr::getInstance()->selectionEmpty());
}

void menu_toggle_attached_lights(void* user_data)
{
	menu_toggle_control(user_data);
	LLPipeline::sRenderAttachedLights = gSavedSettings.getBOOL("RenderAttachedLights");
}

void menu_toggle_attached_particles(void* user_data)
{
	menu_toggle_control(user_data);
	LLPipeline::sRenderAttachedParticles = gSavedSettings.getBOOL("RenderAttachedParticles");
}

class LLSomethingSelected : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = !(LLSelectMgr::getInstance()->getSelection()->isEmpty());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLSomethingSelectedNoHUD : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
		bool new_value = !(selection->isEmpty()) && !(selection->getSelectType() == SELECT_TYPE_HUD);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_more_than_one_selected(void*)
{
	return (LLSelectMgr::getInstance()->getSelection()->getObjectCount() > 1);
}

static bool is_editable_selected()
{
	return (LLSelectMgr::getInstance()->getSelection()->getFirstEditableObject() != NULL);
}

class LLEditableSelected : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gMenuHolder->findControl(userdata["control"].asString())->setValue(is_editable_selected());
		return true;
	}
};

class LLEditableSelectedMono : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerRegion* region = gAgent.getRegion();
		if (region && gMenuHolder && gMenuHolder->findControl(userdata["control"].asString()))
		{
			bool have_cap = (! region->getCapability("UpdateScriptTask").empty());
			bool selected = is_editable_selected() && have_cap;
			gMenuHolder->findControl(userdata["control"].asString())->setValue(selected);
			return true;
		}
		return false;
	}
};

class LLToolsEnableTakeCopy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool all_valid = false;
		if (LLSelectMgr::getInstance())
		{
			all_valid = true;
			struct f : public LLSelectedObjectFunctor
			{
				virtual bool apply(LLViewerObject* obj)
				{
//MK
					if (gRRenabled && obj->isSeat() &&
						gAgent.mRRInterface.mContainsUnsit)
					{
						return true;
					}
//mk
					return (!obj->permCopy() || obj->isAttachment());
				}
			} func;
			const bool firstonly = true;
			bool any_invalid = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
			all_valid = !any_invalid;
		}

		gMenuHolder->findControl(userdata["control"].asString())->setValue(all_valid);
		return true;
	}
};

BOOL enable_selection_you_own_all(void*)
{
	if (LLSelectMgr::getInstance())
	{
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* obj)
			{
				return (!obj->permYouOwner());
			}
		} func;
		const bool firstonly = true;
		bool no_perms = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
		if (no_perms)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL enable_selection_you_own_one(void*)
{
	if (LLSelectMgr::getInstance())
	{
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* obj)
			{
				return (obj->permYouOwner());
			}
		} func;
		const bool firstonly = true;
		bool any_perms = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
		if (!any_perms)
		{
			return FALSE;
		}
	}
	return TRUE;
}

class LLHasAsset : public LLInventoryCollectFunctor
{
public:
	LLHasAsset(const LLUUID& id) : mAssetID(id), mHasAsset(FALSE) {}
	virtual ~LLHasAsset() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	BOOL hasAsset() const { return mHasAsset; }

protected:
	LLUUID mAssetID;
	BOOL mHasAsset;
};

bool LLHasAsset::operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
{
	if (item && item->getAssetUUID() == mAssetID)
	{
		mHasAsset = TRUE;
	}
	return FALSE;
}

BOOL enable_save_into_inventory(void*)
{
	// *TODO: clean this up
	// find the last root
	LLSelectNode* last_node = NULL;
	for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
	{
		last_node = *iter;
	}

	// check all pre-req's for save into inventory.
	if (last_node && last_node->mValid && !last_node->mItemID.isNull() &&
		last_node->mPermissions->getOwner() == gAgentID &&
		gInventory.getItem(last_node->mItemID) != NULL)
	{
		LLViewerObject* obj = last_node->getObject();
		if (obj && !obj->isAttachment())
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL enable_save_into_task_inventory(void*)
{
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	if (node && (node->mValid) && (!node->mFromTaskID.isNull()))
	{
		// *TODO: check to see if the fromtaskid object exists.
		LLViewerObject* obj = node->getObject();
		if (obj && !obj->isAttachment())
		{
			return TRUE;
		}
	}
	return FALSE;
}

class LLToolsEnableSaveToObjectInventory : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = enable_save_into_task_inventory(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_not_thirdperson(void*)
{
	return !gAgent.cameraThirdPerson();
}

// BOOL enable_export_selected(void *)
// {
// 	if (LLSelectMgr::getInstance()->getSelection()->isEmpty())
// 	{
// 		return FALSE;
// 	}
// 	if (!gExporterRequestID.isNull())
// 	{
// 		return FALSE;
// 	}
// 	if (!LLUploadDialog::modalUploadIsFinished())
// 	{
// 		return FALSE;
// 	}
// 	return TRUE;
// }

class LLViewEnableMouselook : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// You can't go directly from customize avatar to mouselook.
		// TODO: write code with appropriate dialogs to handle this transition.
		bool new_value = (CAMERA_MODE_CUSTOMIZE_AVATAR != gAgent.getCameraMode() && !LLPipeline::sFreezeTime);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLToolsEnableToolNotPie : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = (LLToolMgr::getInstance()->getBaseTool() != LLToolPie::getInstance());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldEnableCreateLandmark : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsShowloc)
		{
			return false;
		}
//mk
		bool new_value = gAgent.isGodlike() || 
			(gAgent.getRegion() && gAgent.getRegion()->getAllowLandmark());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldEnableSetHomeLocation : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.isGodlike() || 
			(gAgent.getRegion() && gAgent.getRegion()->getAllowSetHome());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldEnableTeleportHome : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		bool agent_on_prelude = (regionp && regionp->isPrelude());
		bool enable_teleport_home = gAgent.isGodlike() || !agent_on_prelude;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(enable_teleport_home);
		return true;
	}
};

BOOL enable_region_owner(void*)
{
	if (gAgent.getRegion() && gAgent.getRegion()->getOwner() == gAgentID)
		return TRUE;
	return enable_god_customer_service(NULL);
}

BOOL enable_non_faked_god(void*)
{
	return gAgent.isGodlikeWithoutAdminMenuFakery();
}

BOOL enable_god_full(void*)
{
	return gAgent.getGodLevel() >= GOD_FULL;
}

BOOL enable_god_liaison(void*)
{
	return gAgent.getGodLevel() >= GOD_LIAISON;
}

BOOL enable_god_customer_service(void*)
{
	return gAgent.getGodLevel() >= GOD_CUSTOMER_SERVICE;
}

BOOL enable_god_basic(void*)
{
	return gAgent.getGodLevel() > GOD_NOT;
}

#if 0 // 1.9.2
void toggle_vertex_shaders(void *)
{
	BOOL use_shaders = gPipeline.getUseVertexShaders();
	gPipeline.setUseVertexShaders(use_shaders);
}

BOOL check_vertex_shaders(void *)
{
	return gPipeline.getUseVertexShaders();
}
#endif

void toggle_show_xui_names(void *)
{
	BOOL showXUINames = gSavedSettings.getBOOL("ShowXUINames");

	showXUINames = !showXUINames;
	gSavedSettings.setBOOL("ShowXUINames", showXUINames);
}

BOOL check_show_xui_names(void *)
{
	return gSavedSettings.getBOOL("ShowXUINames");
}

void toggle_cull_small(void *)
{
//	gPipeline.mCullBySize = !gPipeline.mCullBySize;
//
//	gSavedSettings.setBOOL("RenderCullBySize", gPipeline.mCullBySize);
}

class LLToolsSelectOnlyMyObjects : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		BOOL cur_val = gSavedSettings.getBOOL("SelectOwnedOnly");

		gSavedSettings.setBOOL("SelectOwnedOnly", ! cur_val);

		return true;
	}
};

class LLToolsSelectOnlyMovableObjects : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		BOOL cur_val = gSavedSettings.getBOOL("SelectMovableOnly");

		gSavedSettings.setBOOL("SelectMovableOnly", ! cur_val);

		return true;
	}
};

class LLToolsSelectBySurrounding : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLSelectMgr::sRectSelectInclusive = !LLSelectMgr::sRectSelectInclusive;

		gSavedSettings.setBOOL("RectangleSelectInclusive", LLSelectMgr::sRectSelectInclusive);
		return true;
	}
};

class LLToolsShowSelectionHighlights : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gSavedSettings.setBOOL("RenderHighlightSelections", !gSavedSettings.getBOOL("RenderHighlightSelections"));
		return true;
	}
};

class LLToolsShowHiddenSelection : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// TomY TODO Merge these
		LLSelectMgr::sRenderHiddenSelections = !LLSelectMgr::sRenderHiddenSelections;

		gSavedSettings.setBOOL("RenderHiddenSelections", LLSelectMgr::sRenderHiddenSelections);
		return true;
	}
};

class LLToolsShowSelectionLightRadius : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// TomY TODO merge these
		LLSelectMgr::sRenderLightRadius = !LLSelectMgr::sRenderLightRadius;

		gSavedSettings.setBOOL("RenderLightRadius", LLSelectMgr::sRenderLightRadius);
		return true;
	}
};

class LLToolsEditLinkedParts : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		BOOL select_individuals = gSavedSettings.getBOOL("EditLinkedParts");
		if (select_individuals)
		{
			LLSelectMgr::getInstance()->demoteSelectionToIndividuals();
		}
		else
		{
			LLSelectMgr::getInstance()->promoteSelectionToRoot();
		}
		return true;
	}
};

void reload_personal_settings_overrides(void *)
{
	std::string overrides = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT,
														   "overrides.xml");
	llinfos << "Loading overrides from " << overrides << llendl;

	gSavedSettings.loadFromFile(overrides);
}

static BOOL slow_mo = FALSE;

void slow_mo_animations(void*)
{
	if (!isAgentAvatarValid()) return;
	if (slow_mo)
	{
		gAgentAvatarp->setAnimTimeFactor(1.f);
		slow_mo = FALSE;
	}
	else
	{
		gAgentAvatarp->setAnimTimeFactor(0.2f);
		slow_mo = TRUE;
	}
}

BOOL is_slow_mo_animations(void*)
{
	return slow_mo;
}

void handle_dump_avatar_local_textures(void*)
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->dumpLocalTextures();
	}
}

void handle_avatar_textures(void*)
{
	LLFloaterAvatarTextures::show(gAgentID);
}

void handle_grab_baked_texture(void* data)
{
	EBakedTextureIndex index = (EBakedTextureIndex)((intptr_t)data);
	if (isAgentAvatarValid())
	{
		const LLUUID& asset_id = gAgentAvatarp->grabBakedTexture(index);
		LL_INFOS("texture") << "Adding baked texture " << asset_id
							<< " to inventory." << llendl;
		LLAssetType::EType asset_type = LLAssetType::AT_TEXTURE;
		LLInventoryType::EType inv_type = LLInventoryType::IT_TEXTURE;
		LLUUID folder_id(gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE));
		if (folder_id.notNull())
		{
			std::string name;
			name = "Baked " +
				   LLVOAvatarDictionary::getInstance()->getBakedTexture(index)->mNameCapitalized +
				   " Texture";

			LLUUID item_id;
			item_id.generate();
			LLPermissions perm;
			perm.init(gAgentID, gAgentID, LLUUID::null, LLUUID::null);
			U32 next_owner_perm = PERM_MOVE | PERM_TRANSFER;
			perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE,
						   next_owner_perm);
			time_t creation_date_now = time_corrected();
			LLPointer<LLViewerInventoryItem> item
				= new LLViewerInventoryItem(item_id,
											folder_id,
											perm,
											asset_id,
											asset_type,
											inv_type,
											name,
											LLStringUtil::null,
											LLSaleInfo::DEFAULT,
											LLInventoryItem::II_FLAGS_NONE,
											creation_date_now);

			item->updateServer(TRUE);
			gInventory.updateItem(item);
			gInventory.notifyObservers();

			// Show the preview panel for textures to let user know that the
			// image is now in inventory.
			LLInventoryView* view = LLInventoryView::getActiveInventory();
			if (view)
			{
				LLFocusableElement* focus_ctrl = gFocusMgr.getKeyboardFocus();

				view->getPanel()->setSelection(item_id, TAKE_FOCUS_NO);
				view->getPanel()->openSelected();
				//LLInventoryView::dumpSelectionInformation((void*)view);
				// restore keyboard focus
				gFocusMgr.setKeyboardFocus(focus_ctrl);
			}
		}
		else
		{
			llwarns << "Can't find a folder to put it in" << llendl;
		}
	}
}

BOOL enable_grab_baked_texture(void* data)
{
	EBakedTextureIndex index = (EBakedTextureIndex)((intptr_t)data);
	if (isAgentAvatarValid())
	{
		return gAgentAvatarp->canGrabBakedTexture(index);
	}
	return FALSE;
}

// Returns a pointer to the avatar give the UUID of the avatar OR of an attachment the avatar is wearing.
// Returns NULL on failure.
LLVOAvatar* find_avatar_from_object(LLViewerObject* object)
{
	if (object)
	{
		if (object->isAttachment())
		{
			do
			{
				object = (LLViewerObject*) object->getParent();
			}
			while (object && !object->isAvatar());
		}
		else
		if (!object->isAvatar())
		{
			object = NULL;
		}
	}

	return (LLVOAvatar*) object;
}

// Returns a pointer to the avatar give the UUID of the avatar OR of an attachment the avatar is wearing.
// Returns NULL on failure.
LLVOAvatar* find_avatar_from_object(const LLUUID& object_id)
{
	return find_avatar_from_object(gObjectList.findObject(object_id));
}

void handle_disconnect_viewer(void *)
{
	LLAppViewer::instance()->forceDisconnect("Testing viewer disconnect");
}

void force_error_breakpoint(void *)
{
    LLAppViewer::instance()->forceErrorBreakpoint();
}

void force_error_llerror(void *)
{
    LLAppViewer::instance()->forceErrorLLError();
}

void force_error_bad_memory_access(void *)
{
    LLAppViewer::instance()->forceErrorBadMemoryAccess();
}

void force_error_infinite_loop(void *)
{
    LLAppViewer::instance()->forceErrorInifiniteLoop();
}

void force_error_software_exception(void *)
{
    LLAppViewer::instance()->forceErrorSoftwareException();
}

void force_error_driver_crash(void *)
{
    LLAppViewer::instance()->forceErrorDriverCrash();
}

class LLToolsUseSelectionForGrid : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLSelectMgr::getInstance()->clearGridObjects();
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* objectp)
			{
				LLSelectMgr::getInstance()->addGridObject(objectp);
				return true;
			}
		} func;
		LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func);
		LLSelectMgr::getInstance()->setGridMode(GRID_MODE_REF_OBJECT);
		if (gFloaterTools)
		{
			gFloaterTools->mComboGridMode->setCurrentByIndex((S32)GRID_MODE_REF_OBJECT);
		}
		return true;
	}
};

//
// LLViewerMenuHolderGL
//

LLViewerMenuHolderGL::LLViewerMenuHolderGL() : LLMenuHolderGL()
{
}

BOOL LLViewerMenuHolderGL::hideMenus()
{
	BOOL handled = LLMenuHolderGL::hideMenus();

	// drop pie menu selection
	mParcelSelection = NULL;
	mObjectSelection = NULL;

	gMenuBarView->clearHoverItem();
	gMenuBarView->resetMenuTrigger();

	return handled;
}

void LLViewerMenuHolderGL::setParcelSelection(LLSafeHandle<LLParcelSelection> selection) 
{ 
	mParcelSelection = selection; 
}

void LLViewerMenuHolderGL::setObjectSelection(LLSafeHandle<LLObjectSelection> selection) 
{ 
	mObjectSelection = selection; 
}

const LLRect LLViewerMenuHolderGL::getMenuRect() const
{
	return LLRect(0, getRect().getHeight() - MENU_BAR_HEIGHT, getRect().getWidth(), STATUS_BAR_HEIGHT);
}

void save_to_xml_callback(LLFilePicker::ESaveFilter type,
						  std::string& filename,
						  void* user_data)
{
	LLFloater* frontmost = (LLFloater*)user_data;
	if (!filename.empty())
	{
		if (frontmost == gFloaterView->getFrontmost())
		{
			LLUICtrlFactory::getInstance()->saveToXML(frontmost, filename);
		}
		else
		{
        	LLNotifications::instance().add("NoFrontmostFloater");
		}
	}
}

void handle_save_to_xml(void*)
{
	LLFloater* frontmost = gFloaterView->getFrontmost();
	if (!frontmost)
	{
        LLNotifications::instance().add("NoFrontmostFloater");
		return;
	}

	std::string default_name = "floater_";
	default_name += frontmost->getTitle();
	default_name += ".xml";

	LLStringUtil::toLower(default_name);
	LLStringUtil::replaceChar(default_name, ' ', '_');
	LLStringUtil::replaceChar(default_name, '/', '_');
	LLStringUtil::replaceChar(default_name, ':', '_');
	LLStringUtil::replaceChar(default_name, '"', '_');

	// Open the file save dialog
	(new LLSaveFilePicker(LLFilePicker::FFSAVE_XML,
						  save_to_xml_callback,
						  frontmost))->getSaveFile(default_name);
}

void load_from_xml_callback(LLFilePicker::ELoadFilter type,
							std::string& filename,
							std::deque<std::string>& files,
							void* user_data)
{
	if (!filename.empty())
	{
		LLFloater* floater = new LLFloater("sample_floater");
		LLUICtrlFactory::getInstance()->buildFloater(floater, filename);
		floater->setCanClose(TRUE);	// Make sure the floater can be closed !
	}
}

void handle_load_from_xml(void*)
{
	(new LLLoadFilePicker(LLFilePicker::FFLOAD_XML, load_from_xml_callback))->getFile();
}

void handle_web_browser_test(void*)
{
	LLFloaterMediaBrowser::showInstance("http://secondlife.com/app/search/slurls.html", true);
}

void handle_rebake_textures(void*)
{
	if (isAgentAvatarValid())
	{
		// Slam pending upload count to "unstick" things
		bool slam_for_debug = true;
		gAgentAvatarp->forceBakeAllTextures(slam_for_debug);
	}
}

void toggle_visibility(void* user_data)
{
	LLView* viewp = (LLView*)user_data;
	if (viewp)
	{
		viewp->setVisible(!viewp->getVisible());
	}
}

BOOL get_visibility(void* user_data)
{
	LLView* viewp = (LLView*)user_data;
	return viewp ? viewp->getVisible() : FALSE;
}

// TomY TODO: Get rid of these?
class LLViewShowHoverTips : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLHoverView::sShowHoverTips = !LLHoverView::sShowHoverTips;
		return true;
	}
};

class LLViewCheckShowHoverTips : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLHoverView::sShowHoverTips;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// TomY TODO: Get rid of these?
class LLViewHighlightTransparent : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsEdit)
		{
			return true;
		}
//mk
		LLDrawPoolAlpha::sShowDebugAlpha = !LLDrawPoolAlpha::sShowDebugAlpha;
		return true;
	}
};

class LLViewCheckHighlightTransparent : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLDrawPoolAlpha::sShowDebugAlpha;
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsEdit)
		{
			new_value = false;
		}
//mk
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLViewToggleRenderType : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string type = userdata.asString();
		if (type == "hideparticles")
		{
			LLPipeline::toggleRenderType(LLPipeline::RENDER_TYPE_PARTICLES);
		}
		return true;
	}
};

class LLViewCheckRenderType : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string type = userdata["data"].asString();
		bool new_value = false;
		if (type == "hideparticles")
		{
			new_value = LLPipeline::toggleRenderTypeControlNegated((void *)LLPipeline::RENDER_TYPE_PARTICLES);
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLViewShowHUDAttachments : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mHasLockedHuds)
		{
			return true;
		}
//mk
		LLPipeline::sShowHUDAttachments = !LLPipeline::sShowHUDAttachments;
		return true;
	}
};

class LLViewCheckHUDAttachments : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mHasLockedHuds)
		{
			return false;
		}
//mk
		bool new_value = LLPipeline::sShowHUDAttachments;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditEnableTakeOff : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string control_name = userdata["control"].asString();
		std::string clothing = userdata["data"].asString();
		bool new_value = false;
//MK
		if (gRRenabled && (gAgent.mRRInterface.contains("remoutfit") ||
						   gAgent.mRRInterface.contains("remoutfit:" + clothing)))
		{
			return false;
		}
//mk
		LLWearableType::EType type = LLWearableType::typeNameToType(clothing);
		if (type >= LLWearableType::WT_SHAPE && type < LLWearableType::WT_COUNT)
		{
			new_value = LLAgentWearables::selfHasWearable(type);
		}
		gMenuHolder->findControl(control_name)->setValue(new_value);
		return true;
	}
};

class LLEditTakeOff : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		std::string clothing = userdata.asString();
		if (clothing == "all")
		{
			LLAgentWearables::userRemoveAllClothes();
		}
		else
		{
			LLWearableType::EType type = LLWearableType::typeNameToType(clothing);
			LLAgentWearables::userRemoveWearablesOfType(type);
		}
		return true;
	}
};

class LLWorldChat : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		handle_chat(NULL);
		return true;
	}
};

class LLToolsSelectTool : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsEdit)
		{
			return true;
		}
//mk
		std::string tool_name = userdata.asString();
		if (tool_name == "focus")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(1);
		}
		else if (tool_name == "move")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(2);
		}
		else if (tool_name == "edit")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(3);
		}
		else if (tool_name == "create")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(4);
		}
		else if (tool_name == "land")
		{
			LLToolMgr::getInstance()->getCurrentToolset()->selectToolByIndex(5);
		}
		return true;
	}
};

/// WINDLIGHT callbacks
class LLWorldEnvSettings : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsSetenv)
		{
			return true;
		}
//mk
		std::string tod = userdata.asString();
		LLVector3 sun_direction;

		if (tod == "editor")
		{
			// if not there or is hidden, show it
			if (!LLFloaterEnvSettings::isOpen() || 
				!LLFloaterEnvSettings::instance()->getVisible())
			{
				LLFloaterEnvSettings::show();
			} 
		}
		else if (tod == "day")
		{
			// if not there or is hidden, show it
			if (!LLFloaterDayCycle::isOpen() || 
				!LLFloaterDayCycle::instance()->getVisible())
			{
				LLFloaterDayCycle::show();
			} 
		}
		else if (tod == "sky")
		{
			// if not there or is hidden, show it
			if (!LLFloaterWindLight::isOpen() || 
				!LLFloaterWindLight::instance()->getVisible())
			{
				LLFloaterWindLight::show();
			} 
		}
		else if (tod == "water")
		{
			// if not there or is hidden, show it
			if (!LLFloaterWater::isOpen() || 
				!LLFloaterWater::instance()->getVisible())
			{
				LLFloaterWater::show();
			} 
		}
		else if (tod == "sunrise")
		{
			// set the value, turn off animation
			LLWLParamManager::instance()->mAnimator.setDayTime(0.25);
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

			// then call update once
			LLWLParamManager::instance()->mAnimator.update(
				LLWLParamManager::instance()->mCurParams);
		}
		else if (tod == "noon")
		{
			// set the value, turn off animation
			LLWLParamManager::instance()->mAnimator.setDayTime(0.567);
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

			// then call update once
			LLWLParamManager::instance()->mAnimator.update(
				LLWLParamManager::instance()->mCurParams);
		}
		else if (tod == "sunset")
		{
			// set the value, turn off animation
			LLWLParamManager::instance()->mAnimator.setDayTime(0.75);
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

			// then call update once
			LLWLParamManager::instance()->mAnimator.update(
				LLWLParamManager::instance()->mCurParams);
		}
		else if (tod == "midnight")
		{
			// set the value, turn off animation
			LLWLParamManager::instance()->mAnimator.setDayTime(0.0);
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

			// then call update once
			LLWLParamManager::instance()->mAnimator.update(
				LLWLParamManager::instance()->mCurParams);
		}
		else
		{
			LLWLParamManager::instance()->mAnimator.mIsRunning = true;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = true;
		}
		return true;
	}
};

/// Post-Process callbacks
class LLWorldPostProcess : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
//MK
		if (gRRenabled && gAgent.mRRInterface.mContainsSetenv)
		{
			return true;
		}
//mk
		LLFloaterPostProcess::show();
		return true;
	}
};

static void addMenu(view_listener_t *menu, const std::string& name)
{
	sMenus.push_back(menu);
	menu->registerListener(gMenuHolder, name);
}

void initialize_menus()
{
	// A parameterized event handler used as ctrl-8/9/0 zoom controls below.
	class LLZoomer : public view_listener_t
	{
	public:
		// The "mult" parameter says whether "val" is a multiplier or used to set the value.
		LLZoomer(F32 val, bool mult=true) : mVal(val), mMult(mult) {}
		bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
		{
			F32 new_fov_rad = mMult ? LLViewerCamera::getInstance()->getDefaultFOV() * mVal : mVal;
			LLViewerCamera::getInstance()->setDefaultFOV(new_fov_rad);
			gSavedSettings.setF32("CameraAngle", LLViewerCamera::getInstance()->getView()); // setView may have clamped it.
			return true;
		}
	private:
		F32 mVal;
		bool mMult;
	};

	class LLAvatarReportAbuse : public view_listener_t
	{
		bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
		{
			LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject());
			if (avatar)
			{
				LLFloaterReporter::showFromObject(avatar->getID());
			}
			return true;
		}
	};

	// File menu
	init_menu_file();
	addMenu(new LLObjectImport(), "Object.Import");
	addMenu(new LLObjectImportUpload(), "Object.ImportUpload");
	addMenu(new LLObjectEnableImport(), "Object.EnableImport");

	// Edit menu
	addMenu(new LLEditUndo(), "Edit.Undo");
	addMenu(new LLEditRedo(), "Edit.Redo");
	addMenu(new LLEditCut(), "Edit.Cut");
	addMenu(new LLEditCopy(), "Edit.Copy");
	addMenu(new LLEditPaste(), "Edit.Paste");
	addMenu(new LLEditDelete(), "Edit.Delete");
	addMenu(new LLEditSearch(), "Edit.Search");
	addMenu(new LLEditSelectAll(), "Edit.SelectAll");
	addMenu(new LLEditDeselect(), "Edit.Deselect");
	addMenu(new LLEditDuplicate(), "Edit.Duplicate");
	addMenu(new LLEditTakeOff(), "Edit.TakeOff");

	addMenu(new LLEditEnableUndo(), "Edit.EnableUndo");
	addMenu(new LLEditEnableRedo(), "Edit.EnableRedo");
	addMenu(new LLEditEnableCut(), "Edit.EnableCut");
	addMenu(new LLEditEnableCopy(), "Edit.EnableCopy");
	addMenu(new LLEditEnablePaste(), "Edit.EnablePaste");
	addMenu(new LLEditEnableDelete(), "Edit.EnableDelete");
	addMenu(new LLEditEnableSelectAll(), "Edit.EnableSelectAll");
	addMenu(new LLEditEnableDeselect(), "Edit.EnableDeselect");
	addMenu(new LLEditEnableDuplicate(), "Edit.EnableDuplicate");
	addMenu(new LLEditEnableTakeOff(), "Edit.EnableTakeOff");
	addMenu(new LLEditEnableCustomizeAvatar(), "Edit.EnableCustomizeAvatar");
	addMenu(new LLEditEnableDisplayName(), "Edit.EnableDisplayName");

	// View menu
	addMenu(new LLViewMouselook(), "View.Mouselook");
	addMenu(new LLViewJoystickFlycam(), "View.JoystickFlycam");
	addMenu(new LLViewResetView(), "View.ResetView");
	addMenu(new LLViewLookAtLastChatter(), "View.LookAtLastChatter");
	addMenu(new LLViewShowHoverTips(), "View.ShowHoverTips");
	addMenu(new LLViewHighlightTransparent(), "View.HighlightTransparent");
	addMenu(new LLViewToggleRenderType(), "View.ToggleRenderType");
	addMenu(new LLViewShowHUDAttachments(), "View.ShowHUDAttachments");
	addMenu(new LLZoomer(1.2f), "View.ZoomOut");
	addMenu(new LLZoomer(1/1.2f), "View.ZoomIn");
	addMenu(new LLZoomer(DEFAULT_FIELD_OF_VIEW, false), "View.ZoomDefault");
	addMenu(new LLViewFullscreen(), "View.Fullscreen");
	addMenu(new LLViewDefaultUISize(), "View.DefaultUISize");

	addMenu(new LLViewEnableMouselook(), "View.EnableMouselook");
	addMenu(new LLViewEnableJoystickFlycam(), "View.EnableJoystickFlycam");
	addMenu(new LLViewEnableLastChatter(), "View.EnableLastChatter");
	addMenu(new LLViewToggleRadar(), "View.ToggleAvatarList");

	addMenu(new LLViewCheckCameraFrontView(), "View.CheckCameraFrontView");
	addMenu(new LLViewCheckJoystickFlycam(), "View.CheckJoystickFlycam");
	addMenu(new LLViewCheckShowHoverTips(), "View.CheckShowHoverTips");
	addMenu(new LLViewCheckHighlightTransparent(), "View.CheckHighlightTransparent");
	addMenu(new LLViewCheckRenderType(), "View.CheckRenderType");
	addMenu(new LLViewCheckHUDAttachments(), "View.CheckHUDAttachments");

	// World menu
	addMenu(new LLWorldChat(), "World.Chat");
	addMenu(new LLWorldAlwaysRun(), "World.AlwaysRun");
	addMenu(new LLWorldSitOnGround(), "World.SitOnGround");
	addMenu(new LLWorldEnableSitOnGround(), "World.EnableSitOnGround");
	addMenu(new LLWorldFly(), "World.Fly");
	addMenu(new LLWorldEnableFly(), "World.EnableFly");
	addMenu(new LLWorldCreateLandmark(), "World.CreateLandmark");
	addMenu(new LLWorldSetHomeLocation(), "World.SetHomeLocation");
	addMenu(new LLWorldTeleportHome(), "World.TeleportHome");
	addMenu(new LLWorldTPtoGround(), "World.TPtoGround");
	addMenu(new LLWorldSetAway(), "World.SetAway");
	addMenu(new LLWorldSetBusy(), "World.SetBusy");

	addMenu(new LLWorldEnableCreateLandmark(), "World.EnableCreateLandmark");
	addMenu(new LLWorldEnableSetHomeLocation(), "World.EnableSetHomeLocation");
	addMenu(new LLWorldEnableTeleportHome(), "World.EnableTeleportHome");
	addMenu(new LLWorldEnableBuyLand(), "World.EnableBuyLand");

	addMenu(new LLWorldCheckAlwaysRun(), "World.CheckAlwaysRun");

	(new LLWorldEnvSettings())->registerListener(gMenuHolder, "World.EnvSettings");
	(new LLWorldPostProcess())->registerListener(gMenuHolder, "World.PostProcess");

	// Tools menu
	addMenu(new LLToolsBuildMode(), "Tools.BuildMode");
	addMenu(new LLToolsSelectTool(), "Tools.SelectTool");
	addMenu(new LLToolsSelectOnlyMyObjects(), "Tools.SelectOnlyMyObjects");
	addMenu(new LLToolsSelectOnlyMovableObjects(), "Tools.SelectOnlyMovableObjects");
	addMenu(new LLToolsSelectBySurrounding(), "Tools.SelectBySurrounding");
	addMenu(new LLToolsShowSelectionHighlights(), "Tools.ShowSelectionHighlights");
	addMenu(new LLToolsShowHiddenSelection(), "Tools.ShowHiddenSelection");
	addMenu(new LLToolsShowSelectionLightRadius(), "Tools.ShowSelectionLightRadius");
	addMenu(new LLToolsEditLinkedParts(), "Tools.EditLinkedParts");
	addMenu(new LLToolsSnapObjectXY(), "Tools.SnapObjectXY");
	addMenu(new LLToolsUseSelectionForGrid(), "Tools.UseSelectionForGrid");
	addMenu(new LLToolsSelectNextPart(), "Tools.SelectNextPart");
	addMenu(new LLToolsLink(), "Tools.Link");
	addMenu(new LLToolsUnlink(), "Tools.Unlink");
	addMenu(new LLToolsStopAllAnimations(), "Tools.StopAllAnimations");
	addMenu(new LLToolsReleaseKeys(), "Tools.ReleaseKeys");
	addMenu(new LLToolsEnableReleaseKeys(), "Tools.EnableReleaseKeys");
	addMenu(new LLToolsLookAtSelection(), "Tools.LookAtSelection");
	addMenu(new LLToolsBuyOrTake(), "Tools.BuyOrTake");
	addMenu(new LLToolsTakeCopy(), "Tools.TakeCopy");
	addMenu(new LLToolsSaveToObjectInventory(), "Tools.SaveToObjectInventory");
	addMenu(new LLToolsSelectedScriptAction(), "Tools.SelectedScriptAction");

	addMenu(new LLToolsCheckBuildMode(), "Tools.CheckBuildMode");
	addMenu(new LLToolsEnableToolNotPie(), "Tools.EnableToolNotPie");
	addMenu(new LLToolsEnableSelectNextPart(), "Tools.EnableSelectNextPart");
	addMenu(new LLToolsEnableLink(), "Tools.EnableLink");
	addMenu(new LLToolsEnableUnlink(), "Tools.EnableUnlink");
	addMenu(new LLToolsEnableBuyOrTake(), "Tools.EnableBuyOrTake");
	addMenu(new LLToolsEnableTakeCopy(), "Tools.EnableTakeCopy");
	addMenu(new LLToolsEnableSaveToObjectInventory(), "Tools.SaveToObjectInventory");

	/*addMenu(new LLToolsVisibleBuyObject(), "Tools.VisibleBuyObject");
	addMenu(new LLToolsVisibleTakeObject(), "Tools.VisibleTakeObject");*/

	// Help menu
	// most items use the ShowFloater method

	// Self pie menu
	addMenu(new HBSelfGroupTitles(), "Self.GroupTitles");
	addMenu(new LLSelfSitOrStand(), "Self.SitOrStand");
	addMenu(new LLSelfRemoveAllAttachments(), "Self.RemoveAllAttachments");

	addMenu(new LLSelfEnableSitOrStand(), "Self.EnableSitOrStand");
	addMenu(new LLSelfEnableRemoveAllAttachments(), "Self.EnableRemoveAllAttachments");

	 // Avatar pie menu
	addMenu(new LLObjectMute(), "Avatar.Mute");
	addMenu(new LLAvatarAddFriend(), "Avatar.AddFriend");
	addMenu(new LLAvatarFreeze(), "Avatar.Freeze");
	addMenu(new LLAvatarDebug(), "Avatar.Debug");
	addMenu(new LLAvatarEnableDebug(), "Avatar.EnableDebug");
	addMenu(new LLAvatarInviteToGroup(), "Avatar.InviteToGroup");
	addMenu(new LLAvatarGiveCard(), "Avatar.GiveCard");
	addMenu(new LLAvatarEject(), "Avatar.Eject");
	addMenu(new LLAvatarSendIM(), "Avatar.SendIM");
	addMenu(new LLAvatarReportAbuse(), "Avatar.ReportAbuse");

	addMenu(new LLObjectEnableMute(), "Avatar.EnableMute");
	addMenu(new LLAvatarEnableAddFriend(), "Avatar.EnableAddFriend");
	addMenu(new LLAvatarEnableFreezeEject(), "Avatar.EnableFreezeEject");

	// Object pie menu
	addMenu(new LLObjectOpen(), "Object.Open");
	addMenu(new LLObjectBuild(), "Object.Build");
	addMenu(new LLObjectTouch(), "Object.Touch");
	addMenu(new LLObjectSitOrStand(), "Object.SitOrStand");
	addMenu(new LLObjectDelete(), "Object.Delete");
	addMenu(new LLObjectAttachToAvatar(), "Object.AttachToAvatar");
	addMenu(new LLObjectReturn(), "Object.Return");
	addMenu(new LLObjectReportAbuse(), "Object.ReportAbuse");
	addMenu(new LLObjectMute(), "Object.Mute");
	addMenu(new LLObjectDerender(), "Object.Derender");
	addMenu(new LLObjectEnableDerender(), "Object.EnableDerender");
	addMenu(new LLObjectBuy(), "Object.Buy");
	addMenu(new LLObjectEdit(), "Object.Edit");
	addMenu(new LLObjectInspect(), "Object.Inspect");
	addMenu(new LLObjectExport(), "Object.Export");

	addMenu(new LLObjectEnableOpen(), "Object.EnableOpen");
	addMenu(new LLObjectEnableTouch(), "Object.EnableTouch");
	addMenu(new LLObjectEnableSitOrStand(), "Object.EnableSitOrStand");
	addMenu(new LLObjectEnableDelete(), "Object.EnableDelete");
	addMenu(new LLObjectEnableWear(), "Object.EnableWear");
	addMenu(new LLObjectEnableReturn(), "Object.EnableReturn");
	addMenu(new LLObjectEnableReportAbuse(), "Object.EnableReportAbuse");
	addMenu(new LLObjectEnableMute(), "Object.EnableMute");
	addMenu(new LLObjectEnableBuy(), "Object.EnableBuy");
	addMenu(new LLObjectEnableExport(), "Object.EnableExport");

	/*addMenu(new LLObjectVisibleTouch(), "Object.VisibleTouch");
	addMenu(new LLObjectVisibleCustomTouch(), "Object.VisibleCustomTouch");
	addMenu(new LLObjectVisibleStandUp(), "Object.VisibleStandUp");
	addMenu(new LLObjectVisibleSitHere(), "Object.VisibleSitHere");
	addMenu(new LLObjectVisibleCustomSit(), "Object.VisibleCustomSit");*/

	// Attachment pie menu
	addMenu(new LLAttachmentDrop(), "Attachment.Drop");
	addMenu(new LLAttachmentDetach(), "Attachment.Detach");

	addMenu(new LLAttachmentEnableDrop(), "Attachment.EnableDrop");
	addMenu(new LLAttachmentEnableDetach(), "Attachment.EnableDetach");

	// Land pie menu
	addMenu(new LLLandBuild(), "Land.Build");
	addMenu(new LLLandSit(), "Land.Sit");
	addMenu(new LLLandBuyPass(), "Land.BuyPass");
	addMenu(new LLLandEdit(), "Land.Edit");

	addMenu(new LLLandEnableBuyPass(), "Land.EnableBuyPass");

	// Generic actions
	addMenu(new LLShowFloater(), "ShowFloater");
	addMenu(new LLPromptShowURL(), "PromptShowURL");
	addMenu(new LLShowAgentProfile(), "ShowAgentProfile");
	addMenu(new LLToggleControl(), "ToggleControl");

	addMenu(new LLGoToObject(), "GoToObject");
	addMenu(new LLPayObject(), "PayObject");

	addMenu(new LLEnablePayObject(), "EnablePayObject");
	addMenu(new LLEnableEdit(), "EnableEdit");

	addMenu(new LLFloaterVisible(), "FloaterVisible");
	addMenu(new LLSomethingSelected(), "SomethingSelected");
	addMenu(new LLSomethingSelectedNoHUD(), "SomethingSelectedNoHUD");
	addMenu(new LLEditableSelected(), "EditableSelected");
	addMenu(new LLEditableSelectedMono(), "EditableSelectedMono");
}
