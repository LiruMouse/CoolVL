/** 
 * @file RRInterface.cpp
 * @author Marine Kelley
 * @brief Implementation of the RLV features
 *
 * RLV Source Code
 * The source code in this file("Source Code") is provided by Marine Kelley
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Marine Kelley.  Terms of
 * the GPL can be found in doc/GPL-license.txt in the distribution of the
 * original source of the Second Life Viewer, or online at 
 * http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL SOURCE CODE FROM MARINE KELLEY IS PROVIDED "AS IS." MARINE KELLEY 
 * MAKES NO WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING 
 * ITS ACCURACY, COMPLETENESS OR PERFORMANCE.
 */

#include "llviewerprecompiledheaders.h"

#include "RRInterface.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llcachename.h"
#include "lldrawpoolalpha.h"
#include "llfloaterchat.h"
#include "llfloaterdaycycle.h"
#include "llfloaterenvsettings.h"
#include "llfloatermap.h"
#include "llfloaterpostprocess.h"
#include "hbfloaterrlv.h"
#include "llfloatersettingsdebug.h"
#include "llfloaterwater.h"
#include "llfloaterwindlight.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llgesturemgr.h"
#include "llhudtext.h"
#include "llinventorybridge.h"
#include "llinventoryview.h"
#include "lloverlaybar.h"
#include "llselectmgr.h"
#include "llstartup.h"
#include "lltracker.h"
#include "llurlsimstring.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
//#include "llwaterparammanager.h"
#include "llwlparammanager.h"
#include "llwearable.h"
#include "llworld.h"
#include "pipeline.h"

// Global and static variables initialization.
BOOL gRRenabled = TRUE;
BOOL RRInterface::sRRNoSetEnv = FALSE;
BOOL RRInterface::sRestrainedLoveDebug = FALSE;
BOOL RRInterface::sUntruncatedEmotes = FALSE;
BOOL RRInterface::sCanOoc = TRUE;
RRInterface::rr_command_map_t RRInterface::sCommandsMap;
std::string RRInterface::sBlackList;
std::string RRInterface::sRolePlayBlackList;
std::string RRInterface::sVanillaBlackList;
std::string RRInterface::sRecvimMessage = "The Resident you messaged is prevented from reading your IMs at the moment, please try again later.";
std::string RRInterface::sSendimMessage = "*** IM blocked by sender's viewer";

// --
// Local functions
std::string dumpList2String(std::deque<std::string> list, std::string sep, S32 size = -1)
{
	bool found_one = false;
	if (size < 0) size = (S32)list.size();
	std::string res = "";
	for (S32 i = 0; i < (S32)list.size() && i < size; ++i) {
		if (found_one) res += sep;
		found_one = true;
		res += list[i];
	}
	return res;
}

S32 match(std::deque<std::string> list, std::string str, bool& exact_match)
{
	// does str contain list[0]/list[1]/.../list[n] ?
	// yes => return the size of the list
	// no  => try again after removing the last element
	// return 0 if never found
	// Exception: if str starts with a "~" character, the match must be exact.
	// exact_match is an output, set to true when strict matching is found,
	// false otherwise.
	U32 size = list.size();
	std::string dump;
	exact_match = false;
	while (size > 0) {
		dump = dumpList2String(list, "/", (S32)size);
		if (str == dump) {
			exact_match = true;
			return (S32)size;
		} else if (!str.empty() && str[0] == '~') {
			return 0;
		} else if (str.find(dump) != std::string::npos) {
			return (S32)size;
		}
		size--;
	}
	return 0;
}

std::deque<std::string> getSubList(std::deque<std::string> list, S32 min, S32 max = -1)
{
	if (min < 0) min = 0;
	if (max < 0) max = list.size() - 1;
	std::deque<std::string> res;
	for (S32 i = min; i <= max; ++i) {
		res.push_back(list[i]);
	}
	return res;
}

bool findMultiple(std::deque<std::string> list, std::string str)
{
	// returns true if all the tokens in list are contained into str
	U32 size = list.size();
	for (U32 i = 0; i < size; i++) {
		if (str.find(list[i]) == std::string::npos) return false;
	}
	return true;
}

void refreshCachedVariable(std::string var)
{
	// Call this function when adding/removing a restriction only, i.e. in this file
	// Test the cached variables in the code of the viewer itself
	if (!isAgentAvatarValid()) return;

	bool contained = gAgent.mRRInterface.contains(var);
	if (var == "detach" || var.find("detach:") == 0 ||
		var.find("addattach") == 0 || var.find("remattach") == 0) {
		contained = gAgent.mRRInterface.contains("detach") ||
					gAgent.mRRInterface.containsSubstr("detach:") ||
					gAgent.mRRInterface.containsSubstr("addattach") ||
					gAgent.mRRInterface.containsSubstr("remattach");
		gAgent.mRRInterface.mContainsDetach = contained;
		gAgent.mRRInterface.mHasLockedHuds = gAgent.mRRInterface.hasLockedHuds();
		if (gAgent.mRRInterface.mHasLockedHuds) {
			// To force the viewer to render the HUDs again, just in case
			LLPipeline::sShowHUDAttachments = TRUE;
		}
	}
	else if (var == "showinv")				gAgent.mRRInterface.mContainsShowinv = contained;
	else if (var == "unsit")				gAgent.mRRInterface.mContainsUnsit = contained;
	else if (var == "fartouch")				gAgent.mRRInterface.mContainsFartouch = contained;
	else if (var == "touchfar")				gAgent.mRRInterface.mContainsFartouch = contained;
	else if (var == "showworldmap")			gAgent.mRRInterface.mContainsShowworldmap = contained;
	else if (var == "showminimap")			gAgent.mRRInterface.mContainsShowminimap = contained;
	else if (var == "showloc")				gAgent.mRRInterface.mContainsShowloc = contained;
	else if (var == "shownames")			gAgent.mRRInterface.mContainsShownames = contained;
	else if (var == "setenv")				gAgent.mRRInterface.mContainsSetenv = contained;
	else if (var == "setdebug")				gAgent.mRRInterface.mContainsSetdebug = contained;
	else if (var == "fly")					gAgent.mRRInterface.mContainsFly = contained;
	else if (var == "edit")					gAgent.mRRInterface.mContainsEdit = (gAgent.mRRInterface.containsWithoutException("edit")); // || gAgent.mRRInterface.containsSubstr("editobj"));
	else if (var == "rez")					gAgent.mRRInterface.mContainsRez = contained;
	else if (var == "showhovertextall")		gAgent.mRRInterface.mContainsShowhovertextall = contained;
	else if (var == "showhovertexthud")		gAgent.mRRInterface.mContainsShowhovertexthud = contained;
	else if (var == "showhovertextworld")	gAgent.mRRInterface.mContainsShowhovertextworld = contained;
	else if (var == "defaultwear")			gAgent.mRRInterface.mContainsDefaultwear = contained;
	else if (var == "permissive")			gAgent.mRRInterface.mContainsPermissive = contained;
	else if (var == "temprun")				gAgent.mRRInterface.mContainsRun = contained;
	else if (var == "alwaysrun")			gAgent.mRRInterface.mContainsAlwaysRun = contained;

	gAgent.mRRInterface.mContainsTp = gAgent.mRRInterface.contains("tplm") ||
									  gAgent.mRRInterface.contains("tploc") ||
									  gAgent.mRRInterface.contains("tplure") ||
									  (gAgent.mRRInterface.mContainsUnsit &&
									   gAgentAvatarp->mIsSitting);

	gAgent.mRRInterface.refreshTPflag(true);
}

void updateAllHudTexts()
{
	LLHUDText::TextObjectIterator text_it;

	for (text_it = LLHUDText::sTextObjects.begin();
		 text_it != LLHUDText::sTextObjects.end();
		 ++text_it)
	{
		LLHUDText *hudText = *text_it;
		if (hudText && !hudText->mLastMessageText.empty()) {
			// do not update the floating names of the avatars around
			LLViewerObject* obj = hudText->getSourceObject();
			if (obj && !obj->isAvatar()) {
				hudText->setStringUTF8(hudText->mLastMessageText);
			}
		}
	}
}

void updateOneHudText(LLUUID uuid)
{
	LLViewerObject* obj = gObjectList.findObject(uuid);
	if (obj) {
		if (obj->mText.notNull()) {
			LLHUDText *hudText = obj->mText.get();
			if (hudText && !hudText->mLastMessageText.empty()) {
				hudText->setStringUTF8(hudText->mLastMessageText);
			}
		}
	}
}
// --

RRInterface::RRInterface()
:	mInventoryFetched(false),
	mAllowCancelTp(true),
	mSitTargetId(),
	mLastLoadedPreset(),
	mReattaching(false),
	mReattachTimeout(false),
	mSnappingBackToLastStandingLocation(false),
	mHasLockedHuds(false),
	mContainsDetach(false),
	mContainsShowinv(false),
	mContainsUnsit(false),
	mContainsFartouch(false),
	mContainsShowworldmap(false),
	mContainsShowminimap(false),
	mContainsShowloc(false),
	mContainsShownames(false),
	mContainsSetenv(false),
	mContainsSetdebug(false),
	mContainsFly(false),
	mContainsEdit(false),
	mContainsRez(false),
	mContainsShowhovertextall(false),
	mContainsShowhovertexthud(false),
	mContainsShowhovertextworld(false),
	mContainsDefaultwear(false),
	mContainsPermissive(false),
	mContainsRun(false),
	mContainsAlwaysRun(false),
	mContainsTp(false),
	mHandleNoStrip(false),
	mHandleNoRelay(false),
	mLaunchTimestamp((U32)LLDate::now().secondsSinceEpoch())
{
	mAllowedS32 = ",";

	mAllowedU32 = 
	",AvatarSex"				// 0 female, 1 male (unreliable: depends on shape)
	",RenderResolutionDivisor"	// simulate blur, default is 1
	",";

	mAllowedF32 = ",";
	mAllowedBOOLEAN = ",";
	mAllowedSTRING = ",";
	mAllowedVEC3 = ",";
	mAllowedVEC3D = ",";
	mAllowedRECT = ",";
	mAllowedCOL4 = ",";
	mAllowedCOL3 = ",";
	mAllowedCOL4U = ",";

	mAssetsToReattach.clear();

	mJustDetached.uuid.setNull();
	mJustDetached.attachpt = "";
	mJustReattached.uuid.setNull();
	mJustReattached.attachpt = "";
}

RRInterface::~RRInterface()
{
}

// init() must be called at an early stage, to setup all RestrainedLove session
// variables. It is called from settings_to_globals() in llappviewer.cpp.
// This can't be done in the constructor for RRInterface, because calling
// gSavedSettings.get*() at that stage would cause crashes under Windows
// (working fine under Linux): probably a race condition in constructors.

//static
void RRInterface::init()
{
	// Info commands (not "blacklistable").
	sCommandsMap.insert(rr_command_entry_t("version", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("versionnew", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("versionnum", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("versionnumbl", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getcommand", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getstatus", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getstatusall", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getsitid", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getoutfit", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getattach", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getinv", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getinvworn", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getpath", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getpathnew", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("findfolder", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getgroup", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getdebug_", RR_INFO));
	sCommandsMap.insert(rr_command_entry_t("getenv_", RR_INFO));

	// Miscellaneous non-info commands that are not "blacklistable".
	sCommandsMap.insert(rr_command_entry_t("notify", RR_MISCELLANEOUS));
	sCommandsMap.insert(rr_command_entry_t("clear", RR_MISCELLANEOUS));
	sCommandsMap.insert(rr_command_entry_t("detachme%f", RR_MISCELLANEOUS));
	sCommandsMap.insert(rr_command_entry_t("setrot%f", RR_MISCELLANEOUS));
	sCommandsMap.insert(rr_command_entry_t("adjustheight%f", RR_MISCELLANEOUS));
	sCommandsMap.insert(rr_command_entry_t("emote", RR_MISCELLANEOUS));
	sCommandsMap.insert(rr_command_entry_t("relayed", RR_MISCELLANEOUS));

	// Normal commands, "blacklistable".

	// Movement restrictions
	sCommandsMap.insert(rr_command_entry_t("fly", RR_MOVE));
	sCommandsMap.insert(rr_command_entry_t("temprun", RR_MOVE));
	sCommandsMap.insert(rr_command_entry_t("alwaysrun", RR_MOVE));
	sVanillaBlackList += "fly,temprun,alwaysrun,";

	// Chat sending restrictions
	sCommandsMap.insert(rr_command_entry_t("sendchat", RR_SENDCHAT));
	sCommandsMap.insert(rr_command_entry_t("chatshout", RR_SENDCHAT));
	sCommandsMap.insert(rr_command_entry_t("chatnormal", RR_SENDCHAT));
	sCommandsMap.insert(rr_command_entry_t("chatwhisper", RR_SENDCHAT));
	sVanillaBlackList += "sendchat,chatshout,chatnormal,chatwhisper,";

	// Chat receiving restrictions
	sCommandsMap.insert(rr_command_entry_t("recvchat", RR_RECEIVECHAT ));
	sCommandsMap.insert(rr_command_entry_t("recvchat_sec", RR_RECEIVECHAT ));
	sCommandsMap.insert(rr_command_entry_t("recvchatfrom", RR_RECEIVECHAT ));
	sVanillaBlackList += "recvchat,recvchat_sec,recvchatfrom,";

	// Chat on private channels restrictions
	sCommandsMap.insert(rr_command_entry_t("sendchannel", RR_CHANNEL));
	sCommandsMap.insert(rr_command_entry_t("sendchannel_sec", RR_CHANNEL));
	sRolePlayBlackList += "sendchannel,sendchannel_sec,";
	sVanillaBlackList += "sendchannel,sendchannel_sec,";

	// Chat and emotes redirections
	sCommandsMap.insert(rr_command_entry_t("redirchat", RR_REDIRECTION));
	sCommandsMap.insert(rr_command_entry_t("rediremote", RR_REDIRECTION));

	// Emotes restrictions
	sCommandsMap.insert(rr_command_entry_t("recvemote", RR_EMOTE));
	sCommandsMap.insert(rr_command_entry_t("recvemote_sec", RR_EMOTE));
	sCommandsMap.insert(rr_command_entry_t("recvemotefrom", RR_EMOTE));
	sRolePlayBlackList += "recvemote,recvemote_sec,recvemotefrom,";
	sVanillaBlackList += "recvemote,recvemote_sec,recvemotefrom,";

	// Instant messaging restrictions
	sCommandsMap.insert(rr_command_entry_t("sendim", RR_INSTANTMESSAGE));
	sCommandsMap.insert(rr_command_entry_t("sendim_sec", RR_INSTANTMESSAGE));
	sCommandsMap.insert(rr_command_entry_t("sendimto", RR_INSTANTMESSAGE));
	sCommandsMap.insert(rr_command_entry_t("startim", RR_INSTANTMESSAGE));
	sCommandsMap.insert(rr_command_entry_t("startimto", RR_INSTANTMESSAGE));
	sCommandsMap.insert(rr_command_entry_t("recvim", RR_INSTANTMESSAGE));
	sCommandsMap.insert(rr_command_entry_t("recvim_sec", RR_INSTANTMESSAGE));
	sCommandsMap.insert(rr_command_entry_t("recvimfrom", RR_INSTANTMESSAGE));
	sRolePlayBlackList += "sendim,sendim_sec,sendimto,startim,startimto,recvim,recvim_sec,recvimfrom,";
	sVanillaBlackList += "sendim,sendim_sec,sendimto,startim,startimto,recvim,recvim_sec,recvimfrom,";

	// Teleport restrictions
	sCommandsMap.insert(rr_command_entry_t("tplm", RR_TELEPORT));
	sCommandsMap.insert(rr_command_entry_t("tploc", RR_TELEPORT));
	sCommandsMap.insert(rr_command_entry_t("tplure", RR_TELEPORT));
	sCommandsMap.insert(rr_command_entry_t("tplure_sec", RR_TELEPORT));
	sCommandsMap.insert(rr_command_entry_t("sittp", RR_TELEPORT));
	sCommandsMap.insert(rr_command_entry_t("standtp", RR_TELEPORT));
	sCommandsMap.insert(rr_command_entry_t("tpto%f", RR_TELEPORT));
	sCommandsMap.insert(rr_command_entry_t("accepttp", RR_TELEPORT));
	sVanillaBlackList += "tplm,tploc,tplure,tplure_sec,sittp,standtp,accepttp,"; // tpto used by teleporters: allow

	// Inventory access restrictions
	sCommandsMap.insert(rr_command_entry_t("showinv", RR_INVENTORY));
	sCommandsMap.insert(rr_command_entry_t("viewnote", RR_INVENTORY));
	sCommandsMap.insert(rr_command_entry_t("viewscript", RR_INVENTORY));
	sCommandsMap.insert(rr_command_entry_t("viewtexture", RR_INVENTORY));
	sCommandsMap.insert(rr_command_entry_t("unsharedwear", RR_INVENTORYLOCK));
	sCommandsMap.insert(rr_command_entry_t("unsharedunwear", RR_INVENTORYLOCK));
	sRolePlayBlackList += "showinv,viewnote,viewscript,viewtexture,unsharedwear,unsharedunwear,";
	sVanillaBlackList += "showinv,viewnote,viewscript,viewtexture,unsharedwear,unsharedunwear,";

	// Building restrictions
	sCommandsMap.insert(rr_command_entry_t("edit", RR_BUILD));
	sCommandsMap.insert(rr_command_entry_t("editobj", RR_BUILD));
	sCommandsMap.insert(rr_command_entry_t("rez", RR_BUILD));
	sRolePlayBlackList += "edit,editobj,rez,";
	sVanillaBlackList += "edit,editobj,rez,";

	// Sitting restrictions
	sCommandsMap.insert(rr_command_entry_t("unsit", RR_SIT));
	sCommandsMap.insert(rr_command_entry_t("unsit%f", RR_SIT));
	sCommandsMap.insert(rr_command_entry_t("sit", RR_SIT));
	sCommandsMap.insert(rr_command_entry_t("sit%f", RR_SIT));
	sVanillaBlackList += "unsit,unsit%f,sit,sit%f,";

	// Locking commands
	sCommandsMap.insert(rr_command_entry_t("detach", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("detachthis", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("detachallthis", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("detachthis_except", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("detachallthis_except", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("attachthis", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("attachallthis", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("attachthis_except", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("attachallthis_except", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("addattach", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("remattach", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("addoutfit", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("remoutfit", RR_LOCK));
	sCommandsMap.insert(rr_command_entry_t("defaultwear", RR_LOCK));
	sVanillaBlackList += "detach,detachthis,detachallthis,detachthis_except,detachallthis_except,attachthis,attachallthis,attachthis_except,attachallthis_except,addattach,remattach,addoutfit,remoutfit,defaultwear,";

	// Detach/remove commands
	sCommandsMap.insert(rr_command_entry_t("detach%f", RR_DETACH));
	sCommandsMap.insert(rr_command_entry_t("detachall%f", RR_DETACH));
	sCommandsMap.insert(rr_command_entry_t("detachthis%f", RR_DETACH));
	sCommandsMap.insert(rr_command_entry_t("detachallthis%f", RR_DETACH));
	sCommandsMap.insert(rr_command_entry_t("remattach%f", RR_DETACH));
	sCommandsMap.insert(rr_command_entry_t("remoutfit%f", RR_DETACH));

	// Attach/wear commands
	sCommandsMap.insert(rr_command_entry_t("attach%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachover%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachoverorreplace%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachall%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachallover%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachalloverorreplace%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachthis%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachthisover%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachthisover%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachthisoverorreplace%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachallthis%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachallthisover%f", RR_ATTACH));
	sCommandsMap.insert(rr_command_entry_t("attachallthisoverorreplace%f", RR_ATTACH));

	// Touch restrictions
	sCommandsMap.insert(rr_command_entry_t("fartouch", RR_TOUCH));
	sCommandsMap.insert(rr_command_entry_t("touchfar", RR_TOUCH));
	sCommandsMap.insert(rr_command_entry_t("touchall", RR_TOUCH));
	sCommandsMap.insert(rr_command_entry_t("touchworld", RR_TOUCH));
	sCommandsMap.insert(rr_command_entry_t("touchthis", RR_TOUCH));
	sCommandsMap.insert(rr_command_entry_t("touchme", RR_TOUCH));
	sCommandsMap.insert(rr_command_entry_t("touchattach", RR_TOUCH));
	sCommandsMap.insert(rr_command_entry_t("touchattachself", RR_TOUCH));
	sCommandsMap.insert(rr_command_entry_t("touchattachother", RR_TOUCH));
	sVanillaBlackList += "fartouch,touchfar,touchall,touchworld,touchthis,touchme,touchattach,touchattachself,touchattachother,";

	// Location/mapping restrictions
	sCommandsMap.insert(rr_command_entry_t("showworldmap", RR_LOCATION));
	sCommandsMap.insert(rr_command_entry_t("showminimap", RR_LOCATION));
	sCommandsMap.insert(rr_command_entry_t("showloc", RR_LOCATION));
	sRolePlayBlackList += "showworldmap,showminimap,showloc,";
	sVanillaBlackList += "showworldmap,showminimap,showloc,";

	// Name viewing restrictions
	sCommandsMap.insert(rr_command_entry_t("shownames", RR_NAME));
	sCommandsMap.insert(rr_command_entry_t("showhovertextall", RR_NAME));
	sCommandsMap.insert(rr_command_entry_t("showhovertext", RR_NAME));
	sCommandsMap.insert(rr_command_entry_t("showhovertexthud", RR_NAME));
	sCommandsMap.insert(rr_command_entry_t("showhovertextworld", RR_NAME));
	sRolePlayBlackList += "shownames,showhovertextall,showhovertext,showhovertexthud,showhovertextworld,";
	sVanillaBlackList += "shownames,showhovertextall,showhovertext,showhovertexthud,showhovertextworld,";

	// Group restrictions
	sCommandsMap.insert(rr_command_entry_t("setgroup", RR_GROUP));
	sCommandsMap.insert(rr_command_entry_t("setgroup%f", RR_GROUP));
	sRolePlayBlackList += "setgroup,";
	sVanillaBlackList += "setgroup,";	// @setgroup=force May be used as a helper: allow

	// Permissions/extra-restriction commands.
	sCommandsMap.insert(rr_command_entry_t("permissive", RR_PERM));
	sCommandsMap.insert(rr_command_entry_t("acceptpermission", RR_PERM));
	sVanillaBlackList += "permissive,acceptpermission,";

	// Debug settings commands.
	sCommandsMap.insert(rr_command_entry_t("setdebug", RR_DEBUG));
	sCommandsMap.insert(rr_command_entry_t("setdebug_%f", RR_DEBUG));
	sRolePlayBlackList += "setdebug";
	sVanillaBlackList += "setdebug,setdebug_%f,";

	sVanillaBlackList += "setenv";

	gRRenabled = gSavedSettings.getBOOL("RestrainedLove");
	if (gRRenabled) {
		sRRNoSetEnv = gSavedSettings.getBOOL("RestrainedLoveNoSetEnv");
		sRestrainedLoveDebug = gSavedSettings.getBOOL("RestrainedLoveDebug");
		sUntruncatedEmotes = gSavedSettings.getBOOL("RestrainedLoveUntruncatedEmotes");
		sCanOoc = gSavedSettings.getBOOL("RestrainedLoveCanOoc");
		sRecvimMessage = gSavedSettings.getString("RestrainedLoveRecvimMessage");
		sSendimMessage = gSavedSettings.getString("RestrainedLoveSendimMessage");
		sBlackList = gSavedSettings.getString("RestrainedLoveBlacklist");

		if (!sRRNoSetEnv) {
			sCommandsMap.insert(rr_command_entry_t("setenv", RR_ENVIRONMENT));
			sCommandsMap.insert(rr_command_entry_t("setenv_%f", RR_ENVIRONMENT));
		}

		llinfos << "RestrainedLove enabled and initialized." << llendl;
	}
}

void RRInterface::refreshTPflag(bool save)
{
	static bool last_value = gSavedPerAccountSettings.getBOOL("RestrainedLoveTPOK");
	bool new_value = !gAgent.mRRInterface.mContainsTp;
	if (new_value != last_value) {
		last_value = new_value;
		gSavedPerAccountSettings.setBOOL("RestrainedLoveTPOK", new_value);
		if (save) {
			gSavedPerAccountSettings.saveToFile(gSavedSettings.getString("PerAccountSettingsFile"), TRUE);
		}
	}
}

std::string RRInterface::getVersion()
{
	return RR_VIEWER_NAME" viewer v"RR_VERSION;
}

std::string RRInterface::getVersion2()
{
	return RR_VIEWER_NAME_NEW" viewer v"RR_VERSION;
}

std::string RRInterface::getVersionNum()
{
	std::string res = RR_VERSION_NUM;
	if (!sBlackList.empty()) {
		res += "," + sBlackList;
	}
	return res;
}

bool RRInterface::isAllowed(LLUUID object_uuid, std::string action, bool log_it)
{
	bool debug = sRestrainedLoveDebug && log_it;
	if (debug) {
		llinfos << object_uuid.asString() << "      " << action << llendl;
	}
	RRMAP::iterator it = mSpecialObjectBehaviours.find(object_uuid.asString());
	while (it != mSpecialObjectBehaviours.end() &&
		   it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString())) {
		if (debug) {
			llinfos << "  checking " << it->second << llendl;
		}
		if (it->second == action) {
			if (debug) {
				llinfos << "  => forbidden. " << llendl;
			}
			return false;
		}
		it++;
	}
	if (debug) {
		llinfos << "  => allowed. " << llendl;
	}
	return true;
}

bool RRInterface::contains(std::string action)
{
	RRMAP::iterator it = mSpecialObjectBehaviours.begin();
	LLStringUtil::toLower(action);
//	llinfos << "looking for " << action << llendl;
	while (it != mSpecialObjectBehaviours.end()) {
		if (it->second == action) {
//			llinfos << "found " << it->second << llendl;
			return true;
		}
		it++;
	}
	return false;
}

bool RRInterface::containsSubstr(std::string action)
{
	RRMAP::iterator it = mSpecialObjectBehaviours.begin();
	LLStringUtil::toLower(action);
//	llinfos << "looking for " << action << llendl;
	while (it != mSpecialObjectBehaviours.end()) {
		if (it->second.find(action) != std::string::npos) {
//			llinfos << "found " << it->second << llendl;
			return true;
		}
		it++;
	}
	return false;
}

bool RRInterface::containsWithoutException(std::string action,
										   std::string except /* = "" */)
{
	// action is a restriction like @sendim, which can accept exceptions
	// (@sendim:except_uuid=add)
	// action_sec is the same action, with "_sec" appended (like @sendim_sec)

	LLStringUtil::toLower(action);
	std::string action_sec = action + "_sec";
	LLUUID uuid;

	// 1. If except is empty, behave like contains(), but looking for both
	// action and action_sec
	if (except.empty()) {
		return contains(action) || contains(action_sec);
	}

	// 2. For each action_sec, if we don't find an exception tied to the same
	// object, return true. If @permissive is set, then even action needs the
	// exception to be tied to the same object, not just action_sec (@permissive
	// restrains the scope of all the exceptions to their own objects)
	RRMAP::iterator it = mSpecialObjectBehaviours.begin();
	while (it != mSpecialObjectBehaviours.end()) {
		if (it->second == action_sec ||
			(it->second == action && mContainsPermissive)) {
			uuid.set(it->first);
			if (isAllowed(uuid, action + ":" + except, false) &&
				isAllowed(uuid, action_sec + ":" + except, false)) { // we use isAllowed because we need to check the object, but it really means "does not contain"
				return true;
			}
		}
		it++;
	}

	// 3. If we didn't return yet, but the map contains action, just look for
	// except_uuid without regard to its object, if none is found return true
	if (contains(action)) {
		if (!contains(action + ":" + except) && !contains(action_sec + ":" + except)) {
			return true;
		}
	}

	// 4. Finally return false if we didn't find anything
	return false;
}

bool RRInterface::isFolderLocked(LLInventoryCategory* cat)
{
	const LLFolderType::EType folder_type = cat->getPreferredType();
	if (LLFolderType::lookupIsProtectedType(folder_type)) return false;

	if (contains("unsharedwear") && !isUnderRlvShare(cat)) return true;
	if (isFolderLockedWithoutException(cat, "attach") != FolderLock_unlocked) return true;
	if (isFolderLockedWithoutException(cat, "detach") != FolderLock_unlocked) return true;
	return false;
}

FolderLock RRInterface::isFolderLockedWithoutException(LLInventoryCategory* cat, std::string attach_or_detach)
{
	if (cat == NULL) return FolderLock_unlocked;

	if (sRestrainedLoveDebug) {
		llinfos << "isFolderLockedWithoutException(" << cat->getName() << ", " << attach_or_detach << ")" << llendl;
	}
	// For each object that is locking this folder, check whether it also issues exceptions to this lock
	std::deque<std::string> commands_list;
	std::string command;
	std::string behav;
	std::string option;
	std::string param;
	std::string this_command;
	std::string this_behav;
	std::string this_option;
	std::string this_param;
	bool this_object_locks;
	FolderLock current_lock = FolderLock_unlocked;
	for (RRMAP::iterator it = mSpecialObjectBehaviours.begin(); it != mSpecialObjectBehaviours.end(); ++it) {
		LLUUID uuid = LLUUID(it->first);
		command = it->second;
		if (sRestrainedLoveDebug) {
			llinfos << "command = " << command << llendl;
		}
		// param will always be equal to "n" in this case since we added it to command, but we don't care about this here
		if (parseCommand(command + "=n", behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
		{
			// find whether this object has issued a "{attach|detach}[all]this" command on a folder that is either this one, or a parent
			this_object_locks = false;
			if (behav == attach_or_detach + "this") {
				if (getCategoryUnderRlvShare(option) == cat) {
					this_object_locks = true;
				}
			} else if (behav == attach_or_detach + "allthis") {
				if (isUnderFolder(getCategoryUnderRlvShare(option), cat)) {
					this_object_locks = true;
				}
			}

			// This object has issued such a command, check whether it has issued an exception to it as well
			if (this_object_locks) {
				commands_list = getListOfRestrictions(uuid);
				FolderLock this_lock = isFolderLockedWithoutExceptionAux(cat, attach_or_detach, commands_list);
				if (this_lock == FolderLock_locked_without_except) return FolderLock_locked_without_except;
				else current_lock = this_lock;
				if (sRestrainedLoveDebug) {
					llinfos << "this_lock=" << this_lock << llendl;
				}
			}
		}
	}

	// Finally, return unlocked since we didn't find any lock on this folder
	return current_lock;
}

FolderLock RRInterface::isFolderLockedWithoutExceptionAux(LLInventoryCategory* cat, std::string attach_or_detach, std::deque<std::string> list_of_restrictions)
{
	// list_of_restrictions contains the list of restrictions issued by one particular object, at least one is supposed to be a "{attach|detach}[all]this"
	// For each folder from cat up to the root folder, check :
	// - if we are on cat and we find "{attach|detach}this_except", there is an exception, keep looking up
	// - if we are on cat and we find "{attach|detach}this", there is no exception, return FolderLock_locked_without_except
	// - if we are on a parent and we find "{attach|detach}allthis_except", there is an exception, keep looking up
	// - if we are on a parent and we find "{attach|detach}allthis", if we found an exception return FolderLock_locked_with_except, else return FolderLock_locked_without_except
	// - finally, if we are on the root, return FolderLocked_unlocked(whether there was an exception or not)
	if (cat == NULL) {
		return FolderLock_unlocked;
	}

	if (sRestrainedLoveDebug) {
		llinfos << "isFolderLockedWithoutExceptionAux(" << cat->getName() << ", " << attach_or_detach << ", [" << dumpList2String(list_of_restrictions, ",") << "])" << llendl;
	}

	FolderLock current_lock = FolderLock_unlocked;
	std::string command;
	std::string behav;
	std::string option;
	std::string param;

	LLInventoryCategory* it = NULL;
	LLInventoryCategory* cat_option = NULL;
	const LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);

	const LLUUID& cat_id = cat->getUUID();
	it = gInventory.getCategory(cat_id);

	do {
		if (sRestrainedLoveDebug) {
			llinfos << "it=" << it->getName() << llendl;
		}

		for (U32 i = 0; i < list_of_restrictions.size(); ++i)
		{
			command = list_of_restrictions[i];
			if (sRestrainedLoveDebug) {
				llinfos << "command2=" << command << llendl;
			}

			// param will always be equal to "n" in this case since we added it to command, but we don't care about this here
			if (parseCommand(command + "=n", behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
			{
				cat_option = getCategoryUnderRlvShare(option);
				if (cat_option == it) {
					if (it == cat) {
						if (behav == attach_or_detach+"this_except" || behav == attach_or_detach+"allthis_except") {
							current_lock = FolderLock_locked_with_except;
						}
						else if (behav == attach_or_detach+"this"|| behav == attach_or_detach+"allthis") {
							return FolderLock_locked_without_except;
						}
					}
					else {
						if (behav == attach_or_detach+"allthis_except") {
							current_lock = FolderLock_locked_with_except;
						}
						else if (behav == attach_or_detach+"allthis") {
							if (current_lock == FolderLock_locked_with_except) return FolderLock_locked_with_except;
							else return FolderLock_locked_without_except;
						}
					}
				}
			}
		}

		const LLUUID& parent_id = it->getParentUUID();
		it = gInventory.getCategory(parent_id);
	} while (it && it->getUUID() != root_id);
	return FolderLock_unlocked; // this should never happen since list_of_commands is supposed to contain at least one "{attach|detach}[all]this" restriction
}

bool RRInterface::isBlacklisted(std::string command, std::string option, bool force)
{
	if (mHandleNoRelay && !option.empty() &&
		option.find("norelay") != std::string::npos) {
		return true;
	}

	if (sBlackList.empty()) return false;

	size_t i = command.find('_');
	if (i != std::string::npos && command.find("_sec") != i
		&& command.find("_except") != i) {
		command = command.substr(0, i + 1);
	}
	if (force) {
		command += "%f";
	}

	rr_command_map_t::iterator it = sCommandsMap.find(command);
	if (it == sCommandsMap.end()) {
		return false;
	}

	S32 type = it->second;
	if (type == (S32)RR_INFO || type == (S32)RR_MISCELLANEOUS) {
		return false;
	}

	std::string blacklist = "," + sBlackList + ",";
	return blacklist.find("," + command + ",") != std::string::npos;
}

bool RRInterface::add(LLUUID object_uuid, std::string action, std::string option)
{
	if (sRestrainedLoveDebug) {
		llinfos << object_uuid.asString() << "       " << action << "      " << option << llendl;
	}

	std::string canon_action = action;
	if (!option.empty()) action += ":" + option;

	if (isAllowed(object_uuid, action)) {
		// Notify if needed
		notify(object_uuid, action, "=n");

		// Check the action against the blacklist
		if (isBlacklisted(canon_action, option)) {
			llinfos << "Blacklisted RestrainedLove command: " << action
					<< "=n for object " << object_uuid.asString() << llendl;
			return true;
		}

		// Actions to do BEFORE inserting the new behav
		if (action == "showinv") {
			//LLInventoryView::cleanup();
			for (S32 i = 0; i < LLInventoryView::sActiveViews.count(); ++i) {
				if (LLInventoryView::sActiveViews.get(i)->getVisible()) {
					LLInventoryView::sActiveViews.get(i)->setVisible(false);
				}
			}
		}
		else if (action == "showminimap") {
			if (LLFloaterMap::getInstance()->getVisible()) {
				LLFloaterMap::toggleInstance();
			}
		}
		else if (action == "shownames") {
			LLFloaterChat::getInstance()->childSetVisible("active_speakers_panel", false);
		}
		else if (action == "fly") {
 			gAgent.setFlying(FALSE);
   		}
		else if (action == "temprun") {
			if (gAgent.getRunning()) {
				if (gAgent.getAlwaysRun()) gAgent.clearAlwaysRun();
				gAgent.clearRunning();
				gAgent.sendWalkRun(false);
			}
		}
		else if (action == "alwaysrun") {
			if (gAgent.getAlwaysRun()) {
				if (gAgent.getRunning()) gAgent.clearRunning();
				gAgent.clearAlwaysRun();
				gAgent.sendWalkRun(false);
			}
		}
		else if (action == "showworldmap" || action == "showloc") {
			if (gFloaterWorldMap && gFloaterWorldMap->getVisible()) {
				LLFloaterWorldMap::toggle(NULL);
			}
		}
		else if (action == "edit") {
			LLPipeline::setRenderBeacons(FALSE);
			LLPipeline::setRenderScriptedBeacons(FALSE);
			LLPipeline::setRenderScriptedTouchBeacons(FALSE);
			LLPipeline::setRenderPhysicalBeacons(FALSE);
			LLPipeline::setRenderSoundBeacons(FALSE);
			LLPipeline::setRenderParticleBeacons(FALSE);
			LLPipeline::setRenderHighlights(FALSE);
			LLDrawPoolAlpha::sShowDebugAlpha = FALSE;
		}
		else if (action == "setenv") {
			if (sRRNoSetEnv) {
				return true;
			}
			LLFloaterEnvSettings::instance()->close();
			LLFloaterWater::instance()->close();
			LLFloaterPostProcess::instance()->close();
			LLFloaterDayCycle::instance()->close();
			LLFloaterWindLight::instance()->close();
			gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
			gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
		}
		else if (action == "setdebug") {
			if (!sRRNoSetEnv) {
				gSavedSettings.setBOOL("VertexShaderEnable", TRUE);
				gSavedSettings.setBOOL("WindLightUseAtmosShaders", TRUE);
			}
		}

		// Insert the new behav
		mSpecialObjectBehaviours.insert(std::pair<std::string, std::string> (object_uuid.asString(), action));
		refreshCachedVariable(action);
		HBFloaterRLV::setDirty();

		// Actions to do AFTER inserting the new behav
		if (action == "showhovertextall" || action == "showloc" || action == "shownames"
			|| action == "showhovertexthud" || action == "showhovertextworld") {
			updateAllHudTexts();
		}
		if (canon_action == "showhovertext") {
			updateOneHudText(LLUUID(option));
		}

		// Update the stored last standing location, to allow grabbers to transport a victim inside a cage while sitting, and restrict them
		// before standing up. If we didn't do this, the avatar would snap back to a safe location when being unsitted by the grabber,
		// which would be rather silly.
		if (action == "standtp") {
			gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal());
		}

		return true;
	}
	return false;
}

bool RRInterface::remove(LLUUID object_uuid, std::string action, std::string option)
{
	if (sRestrainedLoveDebug) {
		llinfos << object_uuid.asString() << "       " << action << "      " << option << llendl;
	}

	std::string canon_action = action;
	if (!option.empty()) action += ":" + option;

	// Notify if needed
	notify(object_uuid, action, "=y");

	// Actions to do BEFORE removing the behav

	// Remove the behav
	RRMAP::iterator it = mSpecialObjectBehaviours.find(object_uuid.asString());
	while (it != mSpecialObjectBehaviours.end() &&
			it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
	{
		if (sRestrainedLoveDebug) {
			llinfos << "  checking " << it->second << llendl;
		}
		if (it->second == action) {
			mSpecialObjectBehaviours.erase(it);
			if (sRestrainedLoveDebug) {
				llinfos << "  => removed. " << llendl;
			}
			refreshCachedVariable(action);
			HBFloaterRLV::setDirty();

			// Actions to do AFTER removing the behav
			if (action == "shownames" || action == "showloc" ||
				action == "showhovertexthud" || action == "showhovertextall" ||
				action == "showhovertextworld") {
				updateAllHudTexts();
			}
			if (canon_action == "showhovertext") {
				updateOneHudText(LLUUID(option));
			}

			return true;
		}
		it++;
	}

	return false;
}

bool RRInterface::clear(LLUUID object_uuid, std::string command)
{
	if (sRestrainedLoveDebug) {
		llinfos << object_uuid.asString() << "   /   " << command << llendl;
	}

	// Notify if needed
	notify(object_uuid, "clear" + (command.empty() ? "" : ":" + command), "");

	RRMAP::iterator it;
	it = mSpecialObjectBehaviours.begin();
	while (it != mSpecialObjectBehaviours.end()) {
		if (sRestrainedLoveDebug) {
			llinfos << "  checking " << it->second << llendl;
		}
		if (it->first == object_uuid.asString() &&
			(command.empty() || it->second.find(command) != std::string::npos)) {
			notify(object_uuid, it->second, "=y");
			if (sRestrainedLoveDebug) {
				llinfos << it->second << " => removed. " << llendl;
			}
			std::string tmp = it->second;
			mSpecialObjectBehaviours.erase(it);
			refreshCachedVariable(tmp);
			it = mSpecialObjectBehaviours.begin();
		}
		else {
			it++;
		}
	}
	HBFloaterRLV::setDirty();
	updateAllHudTexts();
	return true;
}

void RRInterface::replace(LLUUID what, LLUUID by)
{
	RRMAP::iterator it;
	LLUUID uuid;
	it = mSpecialObjectBehaviours.begin();
	while (it != mSpecialObjectBehaviours.end()) {
		uuid.set(it->first);
		if (uuid == what) {
			// found the UUID to replace => add a copy of the command with the new UUID
			mSpecialObjectBehaviours.insert(std::pair<std::string, std::string> (by.asString(), it->second));
		}
		it++;
	}
	// and then clear the old UUID
	clear(what, "");
	HBFloaterRLV::setDirty();
}

bool RRInterface::garbageCollector(bool all) {
	RRMAP::iterator it;
	bool res = false;
	LLUUID uuid;
	LLViewerObject* objp = NULL;
	it = mSpecialObjectBehaviours.begin();
	while (it != mSpecialObjectBehaviours.end()) {
		uuid.set(it->first);
		if (all || !uuid.isNull()) {
//			if (sRestrainedLoveDebug) {
//				llinfos << "testing " << it->first << llendl;
//			}
			objp = gObjectList.findObject(uuid);
			if (!objp) {
				if (sRestrainedLoveDebug) {
					llinfos << it->first << " not found => cleaning... " << llendl;
				}
				clear(uuid);
				res = true;
				it = mSpecialObjectBehaviours.begin();
			} else {
				it++;
			}
		} else {
			if (sRestrainedLoveDebug) {
				llinfos << "ignoring " << it->second << llendl;
			}
			it++;
		}
    }
    return res;
}

std::deque<std::string> RRInterface::parse(std::string str, std::string sep)
{
	size_t ind;
	size_t length = sep.length();
	std::string token;
	std::deque<std::string> res;

	do {
		ind = str.find(sep);
		if (ind != std::string::npos) {
			token = str.substr (0, ind);
			if (!token.empty())
			{
				res.push_back(token);
			}
			str = str.substr(ind + length);
		}
		else if (!str.empty()) {
			res.push_back(str);
		}
	} while (ind != std::string::npos);

	return res;
}

void RRInterface::notify(LLUUID object_uuid, std::string action, std::string suffix)
{
	// scan the list of restrictions, when finding "notify" say the restriction on the specified channel
	RRMAP::iterator it;
	size_t length = 7; // size of "notify:"
	S32 size;
	std::deque<std::string> tokens;
	LLUUID uuid;
	std::string rule;
	it = mSpecialObjectBehaviours.begin();

	while (it != mSpecialObjectBehaviours.end()) {
		uuid.set(it->first);
		rule = it->second; // we are looking for rules like "notify:2222;tp", if action contains "tp" then notify the scripts on channel 2222
		if (rule.find("notify:") == 0) {
			// found a possible notification to send
			rule = rule.substr(length); // keep right part only(here "2222;tp")
			tokens = parse(rule, ";");
			size = tokens.size();
			if (size == 1 || (size > 1 && action.find(tokens[1]) != std::string::npos)) {
				answerOnChat(tokens[0], "/" + action + suffix); // suffix can be "=n", "=y" or whatever else we want, "/" is needed to avoid some clever griefing
			}
		}
		it++;
	}
}

bool RRInterface::parseCommand(std::string command, std::string& behaviour,
							   std::string& option, std::string& param)
{
	size_t ind = command.find('=');
	behaviour = command;
	option.clear();
	param.clear();
	if (ind != std::string::npos) {
		behaviour = command.substr(0, ind);
		param = command.substr(ind + 1);
		ind = behaviour.find(':');
		if (ind != std::string::npos) {
			option = behaviour.substr(ind + 1);
			behaviour = behaviour.substr(0, ind); // keep in this order(option first, then behav) or crash
		}
		return true;
	}
	return false;
}

bool RRInterface::handleCommand(LLUUID uuid, std::string command,
								bool recursing /* = false*/)
{
	// 1. check the command is actually a single one or a list of commands separated by ","
	if (command.find(',') != std::string::npos) {
		mHandleNoRelay = false;
		bool res = true;
		std::deque<std::string> list_of_commands = parse(command, ",");
		for (U32 i = 0; i < list_of_commands.size(); ++i) {
			if (!handleCommand(uuid, list_of_commands.at(i), true)) res = false;
		}
		mHandleNoRelay = false;
		return res;
	}

	if (!recursing) mHandleNoRelay = false;

	// 2. this is a single command, possibly inside a 1-level recursive call(unimportant)
	// if the viewer is not fully initialized and the user does not have control of their avatar,
	// don't execute the command but retain it for later, when it is fully initialized
	// If there is another object still waiting to be automatically reattached, retain all RLV commands
	// as well to avoid an infinite loop if the one it will kick off is also locked.
	// This is valid as the object that would possibly be kicked off by the one to reattach, will have
	// its restrictions wiped out by the garbage collector
	if (LLStartUp::getStartupState() != STATE_STARTED ||
		(!mAssetsToReattach.empty() && !mReattachTimeout)) {
		Command cmd;
		cmd.uuid = uuid;
		cmd.command = command;
		if (sRestrainedLoveDebug) {
			llinfos << "Queuing command: " << command << llendl;
		}
		mQueuedCommands.push_back(cmd);
		return true;
	}

	// 3. parse the command, which is of one of these forms :
	// behav=param
	// behav:option=param
	std::string behav;
	std::string option;
	std::string param;
	LLStringUtil::toLower(command);
	if (parseCommand(command, behav, option, param)) // detach=n, recvchat=n, recvim=n, unsit=n, recvim:<uuid>=add, clear=tplure:
	{
		if (sRestrainedLoveDebug) {
			llinfos << "[" << uuid.asString() << "]  [" << behav << "]  [" << option << "] [" << param << "]" << llendl;
		}
		if (behav == "version") return answerOnChat(param, getVersion());
		else if (behav == "versionnew") return answerOnChat(param, getVersion2());
		else if (behav == "versionnum") return answerOnChat(param, RR_VERSION_NUM);
		else if (behav == "versionnumbl") return answerOnChat(param, getVersionNum());
		else if (behav == "getblacklist") return answerOnChat (param, dumpList2String(getBlacklist(option), ","));
		else if (behav == "getoutfit") return answerOnChat(param, getOutfit(option));
		else if (behav == "getattach") return answerOnChat(param, getAttachments(option));
		else if (behav == "getstatus") return answerOnChat(param, getStatus(uuid, option));
		else if (behav == "getstatusall") {
			uuid.setNull();
			return answerOnChat(param, getStatus(uuid, option));
		}
		else if (behav == "getcommand") return answerOnChat(param, getCommand(option));
		else if (behav == "getinv") return answerOnChat(param, getInventoryList(option));
		else if (behav == "getinvworn") return answerOnChat(param, getInventoryList(option, true));
		else if (behav == "getsitid") return answerOnChat(param, getSitTargetId().asString());
		else if (behav == "getpath") return answerOnChat(param, getFullPath(getItem(uuid), option, false)); // option can be empty (=> find path to object) or the name of an attach pt or the name of a clothing layer
		else if (behav == "getpathnew") return answerOnChat(param, getFullPath(getItem(uuid), option)); // option can be empty (=> find path to object) or the name of an attach pt or the name of a clothing layer
		else if (behav == "findfolder") return answerOnChat(param, getFullPath(findCategoryUnderRlvShare(option)));
		else if (behav.find("getenv_") == 0) return answerOnChat(param, getEnvironment(behav));
		else if (behav.find("getdebug_") == 0) return answerOnChat(param, getDebugSetting(behav));
		else if (behav == "getgroup") {
			LLUUID group_id = gAgent.getGroupID();
			std::string group_name = "none";
			if (group_id.notNull()) {
				gCacheName->getGroupName(group_id, group_name);
			}
			return answerOnChat(param, group_name);
		} else {
			if (param == "n" || param == "add") {
				add(uuid, behav, option);
			} else if (param == "y" || param == "rem") {
				remove(uuid, behav, option);
			} else if (behav == "clear") {
				clear(uuid, param);
			} else if (param == "force") {
				force(uuid, behav, option);
			} else {
				return false;
			}
		}
	} else {
		if (sRestrainedLoveDebug) {
			llinfos << uuid.asString() << "       " << behav << llendl;
		}
		if (behav == "clear") {
			clear(uuid);
		} else if (behav == "relayed") {
			if (recursing) {
				mHandleNoRelay = true;
			}
		} else {
			return false;
		}
	}
	return true;
}

bool RRInterface::fireCommands()
{
	bool ok = true;
	if (mQueuedCommands.size()) {
		if (sRestrainedLoveDebug) {
			llinfos << "Firing commands : " << mQueuedCommands.size() << llendl;
		}
		Command cmd;
		while (!mQueuedCommands.empty()) {
			cmd = mQueuedCommands[0];
			ok = ok & handleCommand(cmd.uuid, cmd.command);
			mQueuedCommands.pop_front();
		}
	}
	return ok;
}

static void force_sit(LLUUID object_uuid)
{
	// Note : Although it would make sense that only the UUID should be needed, we actually need to send an
	// offset to the sim, therefore we need the object to be known to the viewer. In other words, issuing @sit=force
	// right after a teleport is not going to work because the object will not have had time to rez.
	LLViewerObject *object = gObjectList.findObject(object_uuid);
	if (object) {
		if (gAgent.mRRInterface.mContainsUnsit &&
			isAgentAvatarValid() && gAgentAvatarp->mIsSitting) {
			// Do not allow a script to force the avatar to sit somewhere if already forced to stay sitting here
			return;
		}
		if (gAgent.mRRInterface.contains("sit"))
		{
			return;
		}
		if (gAgent.mRRInterface.contains("sittp")) {
			// Do not allow a script to force the avatar to sit somewhere far when under @sittp
			LLVector3 pos = object->getPositionRegion();
			pos -= gAgent.getPositionAgent();
			if (pos.magVec() >= 1.5)
			{
				return;
			}
		}
		if (!isAgentAvatarValid() || !gAgentAvatarp->mIsSitting)
		{
			// We are now standing, and we want to sit down => store our current location so that we can snap back here when we stand up, if under @standtp
			gAgent.mRRInterface.mLastStandingLocation = LLVector3d(gAgent.getPositionGlobal());
		}
		gMessageSystem->newMessageFast(_PREHASH_AgentRequestSit);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID);
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_TargetObject);
		gMessageSystem->addUUIDFast(_PREHASH_TargetID, object->mID);
		gMessageSystem->addVector3Fast(_PREHASH_Offset,
									   gAgent.calcFocusOffset(object,
															  gAgent.getPositionAgent(),
															  0, 0));
		object->getRegion()->sendReliableMessage();
	}
}

bool RRInterface::force(LLUUID object_uuid, std::string command, std::string option)
{
	if (sRestrainedLoveDebug) {
		llinfos << command << "     " << option << llendl;
	}

	// Check the command against the blacklist
	if (isBlacklisted(command, option, true)) {
		if (!option.empty()) {
			command += ":" + option;
		}
		llinfos << "Blacklisted RestrainedLove command: " << command
				<< "=force for object " << object_uuid.asString() << llendl;
		return true;
	}

	bool res = true;
	mHandleNoStrip = true;

	if (command == "sit") { // sit:UUID
		bool allowed_to_sittp = true;
		if (!isAllowed(object_uuid, "sittp")) {
			allowed_to_sittp = false;
			remove(object_uuid, "sittp", "");
		}
		LLUUID uuid(option);
		force_sit(uuid);
		if (!allowed_to_sittp) {
			add(object_uuid, "sittp", "");
		}
	} else if (command == "unsit") { // unsit
		if (sRestrainedLoveDebug) {
			llinfos << "trying to unsit" << llendl;
		}
		if (isAgentAvatarValid() && gAgentAvatarp->mIsSitting) {
			if (sRestrainedLoveDebug) {
				llinfos << "Found sitting avatar object" << llendl;
			}
			if (gAgent.mRRInterface.mContainsUnsit) {
				if (sRestrainedLoveDebug) {
					llinfos << "prevented from unsitting" << llendl;
				}
			} else {
				if (sRestrainedLoveDebug) {
					llinfos << "unsitting agent" << llendl;
				}
				LLOverlayBar::onClickStandUp(NULL);
				send_agent_update(TRUE, TRUE);
			}
		}
	} else if (command == "remoutfit") { // remoutfit or remoutfit:shoes
		if (option.empty()) {
			gAgentWearables.removeWearable(LLWearableType::WT_GLOVES, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_JACKET, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_PANTS, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_SHIRT, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_SHOES, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_SKIRT, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_SOCKS, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_UNDERPANTS, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_UNDERSHIRT, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_ALPHA, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_TATTOO, true, 0);
			gAgentWearables.removeWearable(LLWearableType::WT_PHYSICS, true, 0);
		} else {
			LLWearableType::EType type = getOutfitLayerAsType(option);
			if (type != LLWearableType::WT_INVALID) {
				 // clothes only, not skin, eyes, hair or shape
				if (LLWearableType::getAssetType(type) == LLAssetType::AT_CLOTHING) {
					gAgentWearables.removeWearable(type, true, 0); // remove by layer
				}
			} else {
				// remove by category (in RLV share)
				forceDetachByName(option, false);
			}
		}
	} else if (command == "detach" || command == "remattach") { // detach:chest=force OR detach:restraints/cuffs=force (@remattach is a synonym)
		LLViewerJointAttachment* attachpt = findAttachmentPointFromName(option, true); // exact name
		if (attachpt != NULL || option.empty()) {
			res = forceDetach(option);	// remove by attach pt
		} else {
			forceDetachByName(option, false);
		}
	} else if (command == "detachme") { // detachme=force to detach this object specifically
		res = forceDetachByUuid(object_uuid.asString()); // remove by uuid
	} else if (command == "detachthis") { // detachthis=force to detach the folder containing this object
		std::string pathes_str = getFullPath(getItem(object_uuid), option);
		std::deque<std::string> pathes = parse(pathes_str, ",");
		for (U32 i = 0; i < pathes.size(); ++i) {
			res &= forceDetachByName(pathes.at(i), false);
		}
	} else if (command == "detachall") { // detachall:cuffs=force to detach a folder and its subfolders
		res = forceDetachByName(option, true);
	} else if (command == "detachallthis") { // detachallthis=force to detach the folder containing this object and also its subfolders
		std::string pathes_str = getFullPath(getItem(object_uuid), option);
		std::deque<std::string> pathes = parse(pathes_str, ",");
		for (U32 i = 0; i < pathes.size(); ++i) {
			res &= forceDetachByName(pathes.at(i), true);
		}
	} else if (command == "tpto") { // tpto:X/Y/Z=force(X, Y, Z are GLOBAL coordinates)
		bool allowed_to_tploc = true;
		bool allowed_to_unsit = true;
		bool allowed_to_sittp = true;
		if (!isAllowed(object_uuid, "tploc")) {
			allowed_to_tploc = false;
			remove(object_uuid, "tploc", "");
		}
		if (!isAllowed(object_uuid, "unsit")) {
			allowed_to_unsit = false;
			remove(object_uuid, "unsit", "");
		}
		if (!isAllowed(object_uuid, "sittp")) {
			allowed_to_sittp = false;
			remove(object_uuid, "sittp", "");
		}
		res = forceTeleport(option);
		if (!allowed_to_tploc) add(object_uuid, "tploc", "");
		if (!allowed_to_unsit) add(object_uuid, "unsit", "");
		if (!allowed_to_sittp) add(object_uuid, "sittp", "");
	} else if (command == "attach" || command == "addoutfit") { // attach:cuffs=force
		// Will have to be changed back to AttachHow_replace eventually, but
		// not before a clear and early communication
		res = forceAttach(option, false, AttachHow_over_or_replace);
	} else if (command == "attachover" || command == "addoutfitover") { // attachover:cuffs=force
		res = forceAttach(option, false, AttachHow_over);
	} else if (command == "attachoverorreplace" ||
			   command == "addoutfitoverorreplace") { // attachoverorreplace:cuffs=force
		res = forceAttach(option, false, AttachHow_over_or_replace);
	} else if (command == "attachthis" || command == "addoutfitthis") { // attachthis=force to attach the folder containing this object
		std::string pathes_str = getFullPath(getItem(object_uuid), option);
		if (!pathes_str.empty()) {
			std::deque<std::string> pathes = parse(pathes_str, ",");
			for (U32 i = 0; i < pathes.size(); ++i) {
				res &= forceAttach(pathes.at(i), false, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
			}
		}
	} else if (command == "attachthisover" || command == "addoutfitthisover") { // attachthisover=force to attach the folder containing this object
		std::string pathes_str = getFullPath(getItem(object_uuid), option);
		if (!pathes_str.empty()) {
			std::deque<std::string> pathes = parse(pathes_str, ",");
			for (U32 i = 0; i < pathes.size(); ++i) {
				res &= forceAttach(pathes.at(i), false, AttachHow_over);
			}
		}
	} else if (command == "attachthisoverorreplace" ||
			   command == "addoutfitthisoverorreplace") { // attachthisoverorreplace=force to attach the folder containing this object
		std::string pathes_str = getFullPath(getItem(object_uuid), option);
		if (!pathes_str.empty()) {
			std::deque<std::string> pathes = parse(pathes_str, ",");
			for (U32 i = 0; i < pathes.size(); ++i) {
				res &= forceAttach(pathes.at(i), false, AttachHow_over_or_replace);
			}
		}
	} else if (command == "attachall" || command == "addoutfitall") { // attachall:cuffs=force to attach a folder and its subfolders
		res = forceAttach(option, true, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
	} else if (command == "attachallover" || command == "addoutfitallover") { // attachallover:cuffs=force to attach a folder and its subfolders
		res = forceAttach(option, true, AttachHow_over);
	} else if (command == "attachalloverorreplace" ||
			   command == "addoutfitalloverorreplace") { // attachalloverorreplace:cuffs=force to attach a folder and its subfolders
		res = forceAttach(option, true, AttachHow_over_or_replace);
	} else if (command == "attachallthis" || command == "addoutfitallthis") { // attachallthis=force to attach the folder containing this object and its subfolders
		std::string pathes_str = getFullPath(getItem(object_uuid), option);
		if (!pathes_str.empty()) {
			std::deque<std::string> pathes = parse(pathes_str, ",");
			for (U32 i = 0; i < pathes.size(); ++i) {
				res &= forceAttach(pathes.at(i), true, AttachHow_over_or_replace); // Will have to be changed back to AttachHow_replace eventually, but not before a clear and early communication
			}
		}
	} else if (command == "attachallthisover" ||
			   command == "addoutfitallthisover") { // attachallthisover=force to attach the folder containing this object and its subfolders
		std::string pathes_str = getFullPath(getItem(object_uuid), option);
		if (!pathes_str.empty()) {
			std::deque<std::string> pathes = parse(pathes_str, ",");
			for (U32 i = 0; i < pathes.size(); ++i) {
				res &= forceAttach(pathes.at(i), true, AttachHow_over);
			}
		}
	} else if (command == "attachallthisoverorreplace" ||
			   command == "addoutfitallthisoverorreplace") { // attachallthisoverorreplace=force to attach the folder containing this object and its subfolders
		std::string pathes_str = getFullPath(getItem(object_uuid), option);
		if (!pathes_str.empty()) {
			std::deque<std::string> pathes = parse(pathes_str, ",");
			for (U32 i = 0; i < pathes.size(); ++i) {
				res &= forceAttach(pathes.at(i), true, AttachHow_over_or_replace);
			}
		}
	} else if (command.find("setenv_") == 0) {
		bool allowed = true;
		if (!sRRNoSetEnv) {
			if (!isAllowed(object_uuid, "setenv")) {
				allowed = false;
				remove(object_uuid, "setenv", "");
			}
			if (!mContainsSetenv) res = forceEnvironment(command, option);
			if (!allowed) add(object_uuid, "setenv", "");
		}
	} else if (command.find("setdebug_") == 0) {
		bool allowed = true;
		if (!isAllowed(object_uuid, "setdebug")) {
			allowed = false;
			remove(object_uuid, "setdebug", "");
		}
		if (!contains("setdebug")) res = forceDebugSetting(command, option);
		if (!allowed) add(object_uuid, "setdebug", "");
	} else if (command == "setrot") { // setrot:angle_radians=force
		if (isAgentAvatarValid()) {
			F32 val = atof(option.c_str());
			gAgent.startCameraAnimation();
			LLVector3 rot(0.0, 1.0, 0.0);
			rot = rot.rotVec(-val, LLVector3::z_axis);
			rot.normalize();
			gAgent.resetAxes(rot);
		}
	} else if (command == "adjustheight") { // adjustheight:adjustment_centimeters=force or adjustheight:ref_pelvis_to_foot;scalar[;delta]=force
		if (isAgentAvatarValid()) {
			F32 val = (F32)atoi(option.c_str()) / 100.0f;
			size_t i = option.find(';');
			if (i != std::string::npos && i + 1 < option.length()) {
				F32 scalar = (F32)atof(option.substr(i + 1).c_str());
				if (scalar != 0.0f) {
					if (sRestrainedLoveDebug) {
						llinfos << "Pelvis to foot = " << gAgentAvatarp->getPelvisToFoot() << "m" << llendl;
					}
					val = (atof(option.c_str()) - gAgentAvatarp->getPelvisToFoot()) * scalar;
					option = option.substr(i + 1);
					i = option.find(';');
					if (i != std::string::npos && i + 1 < option.length()) {
						val += (F32)atof(option.substr(i + 1).c_str());
					}
				}
			}
#if 0	// With the new mesh avatars, larger values many be needed (for tiny avies, for example)
			if (val > 1.0f) {
				val = 1.0f;
			} else if (val < -1.0f) {
				val = -1.0f;
			}
#endif
			gSavedSettings.setF32("AvatarOffsetZ", val);
		}
	} else if (command == "setgroup") {
		std::string target_group_name = option;
		LLStringUtil::toLower(target_group_name);
		if (target_group_name != "none") { // "none" is not localized here because a script should not have to bother about viewer language
			S32 nb = gAgent.mGroups.count();
			for (S32 i = 0; i < nb; ++i) {
				LLGroupData group = gAgent.mGroups.get(i);
				std::string this_group_name = group.mName;
				LLStringUtil::toLower(this_group_name);
				if (this_group_name == target_group_name) {
					LLMessageSystem* msg = gMessageSystem;
					msg->newMessageFast(_PREHASH_ActivateGroup);
					msg->nextBlockFast(_PREHASH_AgentData);
					msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
					msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
					msg->addUUIDFast(_PREHASH_GroupID, group.mID);
					gAgent.sendReliableMessage();
					break;
				}
			}
		} else {
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_ActivateGroup);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->addUUIDFast(_PREHASH_GroupID, LLUUID::null);
			gAgent.sendReliableMessage();
		}
		// not found => do nothing
	}

	mHandleNoStrip = false;
	return res;
}

void RRInterface::removeWearableItemFromAvatar(LLViewerInventoryItem* item_or_link)
{
	if (!item_or_link) return;

	LLViewerInventoryItem* item = item_or_link;
	if (LLViewerInventoryItem* linked_item = item_or_link->getLinkedItem())
	{
		item = linked_item;
	}

	if (item->getInventoryType() != LLInventoryType::IT_WEARABLE ||
		!canUnwear(item)) {
		return;
	}

	LLWearable* wearable = gAgentWearables.getWearableFromItemID(item->getUUID());
	if (!wearable) return;

	LLWearableType::EType type = wearable->getType();
	U32 index = gAgentWearables.getWearableIndex(wearable);
	gAgentWearables.removeWearable(type, false, index);
}

bool RRInterface::answerOnChat(std::string channel, std::string msg)
{
	S32 chan = (S32)atoi(channel.c_str());
	if (chan == 0) {
		// protection against abusive "@getstatus=0" commands, or against a
		// non-numerical channel
		return false;
	}
	if (msg.length() > (size_t)(chan > 0 ? 1023 : 255)) {
		llwarns << "Too large an answer: maximum is " << (chan > 0 ? "1023 characters" : "255 characters for a negative channel") << ". Truncated reply." << llendl;
		msg = msg.substr(0,(size_t)(chan > 0 ? 1022 : 254));
	}
	if (chan > 0) {
		std::ostringstream temp;
		temp << "/" << chan << " " << msg;
		gChatBar->sendChatFromViewer(temp.str(), CHAT_TYPE_SHOUT, FALSE);
	} else {
		gMessageSystem->newMessage("ScriptDialogReply");
		gMessageSystem->nextBlock("AgentData");
		gMessageSystem->addUUID("AgentID", gAgentID);
		gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
		gMessageSystem->nextBlock("Data");
		gMessageSystem->addUUID("ObjectID", gAgentID);
		gMessageSystem->addS32("ChatChannel", chan);
		gMessageSystem->addS32("ButtonIndex", 1);
		gMessageSystem->addString("ButtonLabel", msg);
		gAgent.sendReliableMessage();
	}
	if (sRestrainedLoveDebug) {
		llinfos << "/" << chan << " " << msg << llendl;
	}
	return true;
}

std::string RRInterface::crunchEmote(std::string msg, U32 truncateTo) {
	std::string crunched = msg;

	if (msg.find("/me ") == 0 || msg.find("/me'") == 0) {
		// Only allow emotes without "spoken" text.
		// Forbid text containing any symbol which could be used as quotes.
		if (msg.find('"') != std::string::npos || msg.find("''") != std::string::npos
		    || msg.find('(')  != std::string::npos || msg.find(')')  != std::string::npos
		    || msg.find(" -") != std::string::npos || msg.find("- ") != std::string::npos
		    || msg.find('*')  != std::string::npos || msg.find('=')  != std::string::npos
		    || msg.find('^')  != std::string::npos || msg.find('_')  != std::string::npos
		    || msg.find('?')  != std::string::npos || msg.find('~')  != std::string::npos)
		{
			crunched = "...";
		}
		else if (truncateTo > 0 && !sUntruncatedEmotes && !contains("emote")) {
			// Only allow short emotes.
			size_t i = msg.find('.');
			if (i != std::string::npos) {
				crunched = msg.substr(0, ++i);
			}
			if (crunched.length() > truncateTo) {
				crunched = crunched.substr(0, truncateTo);
			}
		}
	}
	else if (msg.find('/') == 0) {
		// only allow short gesture names(to avoid cheats).
		if (msg.length() > 7) { // allows things like "/ao off", "/hug X"
			crunched = "...";
		}
	}
	else if (msg.find("((") != 0 || msg.find("))") != msg.length() - 2 || !sCanOoc) {
		// Only allow OOC chat, starting with "((" and ending with "))".
		crunched = "...";
	}
	return crunched;
}

std::string RRInterface::getOutfitLayerAsString(LLWearableType::EType layer)
{
	switch (layer) {
		case LLWearableType::WT_SKIN:		return WS_SKIN;
		case LLWearableType::WT_GLOVES:		return WS_GLOVES;
		case LLWearableType::WT_JACKET:		return WS_JACKET;
		case LLWearableType::WT_PANTS:		return WS_PANTS;
		case LLWearableType::WT_SHIRT:		return WS_SHIRT;
		case LLWearableType::WT_SHOES:		return WS_SHOES;
		case LLWearableType::WT_SKIRT:		return WS_SKIRT;
		case LLWearableType::WT_SOCKS:		return WS_SOCKS;
		case LLWearableType::WT_UNDERPANTS:	return WS_UNDERPANTS;
		case LLWearableType::WT_UNDERSHIRT:	return WS_UNDERSHIRT;
		case LLWearableType::WT_ALPHA:		return WS_ALPHA;
		case LLWearableType::WT_TATTOO:		return WS_TATTOO;
		case LLWearableType::WT_PHYSICS:	return WS_PHYSICS;
		case LLWearableType::WT_EYES:		return WS_EYES;
		case LLWearableType::WT_HAIR:		return WS_HAIR;
		case LLWearableType::WT_SHAPE:		return WS_SHAPE;
		default:			return "";
	}
}

LLWearableType::EType RRInterface::getOutfitLayerAsType(std::string layer)
{
	if (layer == WS_SKIN)		return LLWearableType::WT_SKIN;
	if (layer == WS_GLOVES)		return LLWearableType::WT_GLOVES;
	if (layer == WS_JACKET)		return LLWearableType::WT_JACKET;
	if (layer == WS_PANTS)		return LLWearableType::WT_PANTS;
	if (layer == WS_SHIRT)		return LLWearableType::WT_SHIRT;
	if (layer == WS_SHOES)		return LLWearableType::WT_SHOES;
	if (layer == WS_SKIRT)		return LLWearableType::WT_SKIRT;
	if (layer == WS_SOCKS)		return LLWearableType::WT_SOCKS;
	if (layer == WS_UNDERPANTS)	return LLWearableType::WT_UNDERPANTS;
	if (layer == WS_UNDERSHIRT)	return LLWearableType::WT_UNDERSHIRT;
	if (layer == WS_ALPHA)		return LLWearableType::WT_ALPHA;
	if (layer == WS_TATTOO)		return LLWearableType::WT_TATTOO;
	if (layer == WS_PHYSICS)	return LLWearableType::WT_PHYSICS;
	if (layer == WS_EYES)		return LLWearableType::WT_EYES;
	if (layer == WS_HAIR)		return LLWearableType::WT_HAIR;
	if (layer == WS_SHAPE)		return LLWearableType::WT_SHAPE;
	return LLWearableType::WT_INVALID;
}

std::string RRInterface::getOutfit(std::string layer)
{
	if (layer == WS_SKIN)		return (gAgentWearables.getWearable(LLWearableType::WT_SKIN, 0) != NULL ? "1" : "0");
	if (layer == WS_GLOVES)		return (gAgentWearables.getWearable(LLWearableType::WT_GLOVES, 0) != NULL ? "1" : "0");
	if (layer == WS_JACKET)		return (gAgentWearables.getWearable(LLWearableType::WT_JACKET, 0) != NULL ? "1" : "0");
	if (layer == WS_PANTS)		return (gAgentWearables.getWearable(LLWearableType::WT_PANTS, 0) != NULL ? "1" : "0");
	if (layer == WS_SHIRT)		return (gAgentWearables.getWearable(LLWearableType::WT_SHIRT, 0) != NULL ? "1" : "0");
	if (layer == WS_SHOES)		return (gAgentWearables.getWearable(LLWearableType::WT_SHOES, 0) != NULL ? "1" : "0");
	if (layer == WS_SKIRT)		return (gAgentWearables.getWearable(LLWearableType::WT_SKIRT, 0) != NULL ? "1" : "0");
	if (layer == WS_SOCKS)		return (gAgentWearables.getWearable(LLWearableType::WT_SOCKS, 0) != NULL ? "1" : "0");
	if (layer == WS_UNDERPANTS)	return (gAgentWearables.getWearable(LLWearableType::WT_UNDERPANTS, 0) != NULL ? "1" : "0");
	if (layer == WS_UNDERSHIRT)	return (gAgentWearables.getWearable(LLWearableType::WT_UNDERSHIRT, 0) != NULL ? "1" : "0");
	if (layer == WS_ALPHA)		return (gAgentWearables.getWearable(LLWearableType::WT_ALPHA, 0) != NULL ? "1" : "0");
	if (layer == WS_TATTOO)		return (gAgentWearables.getWearable(LLWearableType::WT_TATTOO, 0) != NULL ? "1" : "0");
	if (layer == WS_PHYSICS)	return (gAgentWearables.getWearable(LLWearableType::WT_PHYSICS, 0) != NULL ? "1" : "0");
	if (layer == WS_EYES)		return (gAgentWearables.getWearable(LLWearableType::WT_EYES, 0) != NULL ? "1" : "0");
	if (layer == WS_HAIR)		return (gAgentWearables.getWearable(LLWearableType::WT_HAIR, 0) != NULL ? "1" : "0");
	if (layer == WS_SHAPE)		return (gAgentWearables.getWearable(LLWearableType::WT_SHAPE, 0) != NULL ? "1" : "0");
	return getOutfit(WS_GLOVES) + getOutfit(WS_JACKET)     + getOutfit(WS_PANTS) +
		   getOutfit(WS_SHIRT)  + getOutfit(WS_SHOES)      + getOutfit(WS_SKIRT) +
		   getOutfit(WS_SOCKS)  + getOutfit(WS_UNDERPANTS) + getOutfit(WS_UNDERSHIRT) +
		   getOutfit(WS_SKIN)   + getOutfit(WS_EYES)       + getOutfit(WS_HAIR) +
		   getOutfit(WS_SHAPE)  + getOutfit(WS_ALPHA)      + getOutfit(WS_TATTOO) +
		   getOutfit(WS_PHYSICS);
}

std::string RRInterface::getAttachments(std::string attachpt)
{
	std::string res = "";
	std::string name;
	if (!isAgentAvatarValid()) {
		llwarns << "NULL avatar pointer. Aborting." << llendl;
		return res;
	}
	if (attachpt.empty()) res += "0"; // to match the LSL macros
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		iter != gAgentAvatarp->mAttachmentPoints.end(); iter++)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter;
		LLViewerJointAttachment* attachment = curiter->second;
		name = attachment->getName();
		LLStringUtil::toLower(name);
		if (sRestrainedLoveDebug) {
			llinfos << "trying <" << name << ">" << llendl;
		}
		if (attachpt.empty() || attachpt == name) {
			if (attachment->getNumObjects() > 0) res+="1"; //attachment->getName();
			else res += "0";
		}
	}
	return res;
}

std::string RRInterface::getStatus(LLUUID object_uuid, std::string rule)
{
	std::string res;
	std::string name;
	std::string separator = "/";
	// If rule contains a specification of the separator, extract it
	size_t ind = rule.find (";");
	if (ind != std::string::npos) {
		separator = rule.substr(ind + 1);
		rule = rule.substr(0, ind);
	}
	if (separator.empty()) {
		separator = "/"; // Prevent a hack to force the avatar to say something
	}

	RRMAP::iterator it;
	if (object_uuid.isNull()) {
		it = mSpecialObjectBehaviours.begin();
	} else {
		it = mSpecialObjectBehaviours.find(object_uuid.asString());
	}

	while (it != mSpecialObjectBehaviours.end() &&
		   (object_uuid.isNull() ||
			it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString())))
	{
		if (rule.empty() || it->second.find(rule) != std::string::npos) {
			res += separator;
			res += it->second;
		}
		it++;
	}
	return res;
}

std::string RRInterface::getCommand(std::string match, bool blacklist)
{
	std::string res, command, name, temp;
	size_t i;
	bool force;
	LLStringUtil::toLower(match);
	rr_command_map_t::iterator it;
	for (it = sCommandsMap.begin(); it != sCommandsMap.end(); it++) {
		command = it->first;
		i = command.find("%f");
		force = i != std::string::npos;
		name = force ? command.substr(0, i) : command;
		temp = res + "/";
		if (match.empty() || command.find(match) != std::string::npos) {
			if (temp.find("/" + command + "/") == std::string::npos
				&& (blacklist || !isBlacklisted(name, "", force))) {
				res += "/" + command;
			}
		}
	}
	return res;
}

std::string RRInterface::getCommandsByType(S32 type, bool blacklist)
{
	std::string res, command, name, temp;
	S32 cmdtype;
	size_t i;
	bool force;
	rr_command_map_t::iterator it;
	for (it = sCommandsMap.begin(); it != sCommandsMap.end(); it++) {
		cmdtype = (S32)it->second;
		if (cmdtype == type)
		{
			command = it->first;
			i = command.find("%f");
			force = i != std::string::npos;
			name = force ? command.substr(0, i) : command;
			temp = res + "/";
			if (temp.find("/" + command + "/") == std::string::npos
				&& (blacklist || !isBlacklisted(name, "", force))) {
				res += "/" + command;
			}
		}
	}
	return res;
}

std::deque<std::string> RRInterface::getBlacklist(std::string filter)
{
	std::deque<std::string> list, res;
	list = parse(sBlackList, ",");
	res.clear();

	size_t size = list.size();
	for (size_t i = 0; i < size; i++) {
		if (filter.empty() || list[i].find(filter) != std::string::npos) {
			res.push_back(list[i]);
		}
	}

	return res;
}

bool RRInterface::forceDetach(std::string attachpt)
{
	std::string name;
	bool res = false;
	if (!isAgentAvatarValid()) return res;
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end(); iter++)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter;
		LLViewerJointAttachment* attachment = curiter->second;
		name = attachment->getName();
		LLStringUtil::toLower(name);
		if (sRestrainedLoveDebug) {
			llinfos << "trying <" << name << ">" << llendl;
		}
		if (attachpt.empty() || attachpt == name) {
			if (sRestrainedLoveDebug) {
				llinfos << "found => detaching" << llendl;
			}
			detachAllObjectsFromAttachment(attachment);
			res = true;
		}
	}
	return res;
}

bool RRInterface::forceDetachByUuid(std::string object_uuid)
{
	bool res = false;
	if (!isAgentAvatarValid()) return res;
	LLViewerObject* object = gObjectList.findObject(LLUUID(object_uuid));
	if (object) {
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
			 iter != gAgentAvatarp->mAttachmentPoints.end(); iter++)
		{
			LLVOAvatar::attachment_map_t::iterator curiter = iter;
			LLViewerJointAttachment* attachment = curiter->second;
			if (attachment && attachment->isObjectAttached(object)) {
				detachObject(object);
				res = true;
			}
		}
	}
	return res;
}

bool RRInterface::hasLockedHuds()
{
	if (!isAgentAvatarValid()) return false;
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end(); iter++)
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter;
		LLViewerJointAttachment* attachment = curiter->second;
		LLViewerObject* obj;
		if (attachment) {
			for (U32 i = 0; i < attachment->mAttachedObjects.size(); ++i) {
				obj = attachment->mAttachedObjects.at(i);
				if (obj && obj->isHUDAttachment() && !canDetach(obj)) return true;
			}
		}
	}
	return false;
}

std::deque<LLInventoryItem*> RRInterface::getListOfLockedItems(LLInventoryCategory* root)
{
	std::deque<LLInventoryItem*> res;
	std::deque<LLInventoryItem*> tmp;
	res.clear();

	if (root && isAgentAvatarValid()) {

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(root->getUUID(), cats, items);
		S32 count;
		S32 count_tmp;
		S32 i;
		S32 j;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;
		//		LLViewerObject* attached_object = NULL;
		std::string attach_point_name = "";

		// Try to find locked items in the current category
		count = items->count();
		for (i = 0; i < count; ++i) {
			item = items->get(i);
			// If this is an object, add it if it is worn and locked, or worn and its attach point is locked
			if (item && item->getType() == LLAssetType::AT_OBJECT) {
				LLViewerObject* attached_object = gAgentAvatarp->getWornAttachment(item->getUUID());
				if (attached_object) {
					attach_point_name = gAgentAvatarp->getAttachedPointName(item->getLinkedUUID());
					if (!gAgent.mRRInterface.canDetach(attached_object)) {
						if (sRestrainedLoveDebug) {
							llinfos << "found a locked object : " << item->getName() << " on " << attach_point_name << llendl;
						}
						res.push_back(item);
					}
				}
			}
			// If this is a piece of clothing, add it if the avatar can't
			// unwear clothes, or if this layer itself can't be unworn
			else if (item && item->getType() == LLAssetType::AT_CLOTHING) {
				if (gAgent.mRRInterface.contains("remoutfit")
					|| gAgent.mRRInterface.containsSubstr("remoutfit:")
					) {
					if (sRestrainedLoveDebug) {
						llinfos << "found a locked clothing : " << item->getName() << llendl;
					}
					res.push_back(item);
				}
			}
		}

		// We have all the locked objects contained directly in this folder,
		// now add all the ones contained in children folders recursively
		count = cats->count();
		for (i = 0; i < count; ++i) {
			cat = cats->get(i);
			tmp = getListOfLockedItems(cat);
			count_tmp = tmp.size();
			for (j = 0; j < count_tmp; ++j) {
				item = tmp[j];
				if (item) res.push_back(item);
			}
		}

		if (sRestrainedLoveDebug) {
			llinfos << "number of locked objects under " << root->getName()
					<< " =  " << res.size() << llendl;
		}
	}

	return res;
}

std::deque<std::string> RRInterface::getListOfRestrictions(LLUUID object_uuid,
														   std::string rule /*= ""*/)
{
	std::deque<std::string> res;
	std::string name;
	RRMAP::iterator it;
	if (object_uuid.isNull()) {
		it = mSpecialObjectBehaviours.begin();
	}
	else {
		it = mSpecialObjectBehaviours.find(object_uuid.asString());
	}
	while (it != mSpecialObjectBehaviours.end() &&
			(object_uuid.isNull() || it != mSpecialObjectBehaviours.upper_bound(object_uuid.asString()))
	)
	{
		if (rule.empty() || it->second.find(rule) != std::string::npos) {
			res.push_back(it->second);
		}
		it++;
	}
	return res;
}

std::string RRInterface::getInventoryList(std::string path,
										  bool withWornInfo /* = false */)
{
	std::string res = "";
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	LLInventoryCategory* root = NULL;
	if (path.empty()) root = getRlvShare();
	else root = getCategoryUnderRlvShare(path);

	if (root) {
		gInventory.getDirectDescendentsOf(root->getUUID(), cats, items);
		if (cats) {
			S32 count = cats->count();
			bool found_one = false;
			if (withWornInfo) {
				std::string worn_items = getWornItems(root);
				res += "|";
				found_one = true;
				if (worn_items == "n") {
					res += "10";
				} else if (worn_items == "N") {
					res += "30";
				} else {
					res += worn_items;
				}
			}
			for (S32 i = 0; i < count; ++i) {
				LLInventoryCategory* cat = cats->get(i);
				std::string name = cat->getName();
				if (!name.empty() && name[0] !=  '.' &&
					(!mHandleNoRelay ||
					 name.find("norelay") == std::string::npos)) { // hidden folders => invisible to the list
					if (found_one) res += ",";
					res += name.c_str();
					if (withWornInfo) {
						std::string worn_items = getWornItems(cat);
						res += "|";
						found_one = true;
						if (worn_items == "n") {
							res += "10";
						} else if (worn_items == "N") {
							res += "30";
						} else {
							res += worn_items;
						}
					}
					found_one = true;
				}
			}
		}
	}

	return res;
}

std::string RRInterface::getWornItems(LLInventoryCategory* cat)
{
	// Returns a string of 2 digits according to the proportion of worn items in this folder and its children :
	// First digit is this folder, second digit is children folders
	// 0 : No item contained in the folder
	// 1 : Some items contained but none is worn
	// 2 : Some items contained and some of them are worn
	// 3 : Some items contained and all of them are worn
	std::string res_as_string = "0";
	S32 res			= 0;
	S32 subRes		= 0;
	S32 prevSubRes	= 0;
	S32 nbItems		= 0;
	S32 nbWorn		= 0;
	bool isNoMod	= false;
	bool isRoot		= (getRlvShare() == cat);

	// if cat exists, scan all the items inside it
	if (cat) {

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;

		// retrieve all the objects contained in this folder
		gInventory.getDirectDescendentsOf(cat->getUUID(), cats, items);
		if (!isRoot && items) { // do not scan the shared root

			// scan them one by one
			S32 count = items->count();
			for (S32 i = 0; i < count; ++i) {

				LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->get(i);

				if (item) {
					if (item->getType() == LLAssetType::AT_OBJECT ||
						item->getType() == LLAssetType::AT_CLOTHING ||
						item->getType() == LLAssetType::AT_BODYPART) {
						nbItems++;
					}
					if (gAgentWearables.isWearingItem(item->getUUID()) ||
						(isAgentAvatarValid() &&
						 gAgentAvatarp->isWearingAttachment(item->getUUID()))) {
						nbWorn++;
					}

					// special case: this item is no-mod, hence we need to check
					// its parent folder is correctly named, and that the item
					// is alone in its folder. If so, then the calling method
					// will have to deal with a special character instead of a
					// number
					if (count == 1 &&
						item->getType() == LLAssetType::AT_OBJECT &&
						!item->getPermissions().allowModifyBy(gAgentID)) {
						if (findAttachmentPointFromName(cat->getName()) != NULL) {
							isNoMod = true;
						}
					}
				}
			}
		}

		// scan every subfolder of the folder we are scanning, recursively
		// note : in the case of no-mod items we shouldn't have sub-folders, so no need to check
		if (cats && !isNoMod) {

			S32 count = cats->count();
			for (S32 i = 0; i < count; ++i) {

				LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->get(i);

				if (cat_child) {
					std::string tmp = getWornItems(cat_child);
					// translate the result for no-mod items into something the upper levels can understand
					if (tmp == "N") {
						if (!isRoot) {
							nbWorn++;
							nbItems++;
							subRes = 3;
						}
					}
					else if (tmp == "n") {
						if (!isRoot) {
							nbItems++;
							subRes = 1;
						}
					}
					else if (!cat_child->getName().empty() && cat_child->getName()[0] != '.') { // we don't want to include invisible folders, except the ones containing a no-mod item
						// This is an actual sub-folder with several items and sub-folders inside,
						// so retain its score to integrate it into the current one
						// As it is a sub-folder, to integrate it we need to reduce its score first(consider "0" as "ignore")
						// "00" = 0, "01" = 1, "10" = 1, "30" = 3, "03" = 3, "33" = 3, all the rest gives 2(some worn, some not worn)
						if     (tmp == "00")                               subRes = 0;
						else if (tmp == "11" || tmp == "01" || tmp == "10") subRes = 1;
						else if (tmp == "33" || tmp == "03" || tmp == "30") subRes = 3;
						else subRes = 2;

						// Then we must combine with the previous sibling sub-folders
						// Same rule as above, set to 2 in all cases except when prevSubRes == subRes or when either == 0(nothing present, ignore)
						if     (prevSubRes == 0 && subRes == 0) subRes = 0;
						else if (prevSubRes == 0 && subRes == 1) subRes = 1;
						else if (prevSubRes == 1 && subRes == 0) subRes = 1;
						else if (prevSubRes == 1 && subRes == 1) subRes = 1;
						else if (prevSubRes == 0 && subRes == 3) subRes = 3;
						else if (prevSubRes == 3 && subRes == 0) subRes = 3;
						else if (prevSubRes == 3 && subRes == 3) subRes = 3;
						else subRes = 2;
						prevSubRes = subRes;
					}
				}
			}
		}
	}

	if (isNoMod) {
		// the folder contains one no-mod object and is named from an attachment point
		// => return a special character that will be handled by the calling method
		if (nbWorn > 0) return "N";
		else return "n";
	}
	else {
		if (isRoot || nbItems == 0) res = 0; // forcibly hide all items contained directly under #RLV
		else if (nbWorn >= nbItems) res = 3;
		else if (nbWorn > 0) res = 2;
		else res = 1;
	}
	std::stringstream str;
	str << res;
	str << subRes;
	res_as_string = str.str();
	return res_as_string;
}

LLInventoryCategory* RRInterface::getRlvShare()
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(
					gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY), cats, items
	);

	if (cats) {
		S32 count = cats->count();
		for (S32 i = 0; i < count; ++i) {
			LLInventoryCategory* cat = cats->get(i);
			std::string name = cat->getName();
			if (name == RR_SHARED_FOLDER) {
//				if (sRestrainedLoveDebug) {
//					llinfos << "found " << name << llendl;
//				}
				return cat;
			}
		}
	}
	return NULL;
}

bool RRInterface::isUnderRlvShare(LLInventoryItem* item)
{
	const LLUUID& cat_id = item->getParentUUID();
	return isUnderFolder(getRlvShare(), gInventory.getCategory(cat_id));
/*
	if (item == NULL) return false;
	LLInventoryCategory* res = NULL;
	LLInventoryCategory* rlv = getRlvShare();
	if (rlv == NULL) return false;
	const LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);

	const LLUUID& cat_id = item->getParentUUID();
	res = gInventory.getCategory(cat_id);

	while (res && res->getUUID() != root_id) {
		if (res == rlv) return true;
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory(parent_id);
	}
	return false;
*/
}

bool RRInterface::isUnderRlvShare(LLInventoryCategory* cat)
{
	return isUnderFolder(getRlvShare(), cat);
/*
	if (cat == NULL) return false;
	LLInventoryCategory* res = NULL;
	LLInventoryCategory* rlv = getRlvShare();
	if (rlv == NULL) return false;
	const LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);

	const LLUUID& cat_id = cat->getParentUUID();
	res = gInventory.getCategory(cat_id);

	while (res && res->getUUID() != root_id) {
		if (res == rlv) return true;
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory(parent_id);
	}
	return false;
*/
}

bool RRInterface::isUnderFolder(LLInventoryCategory* cat_parent,
								LLInventoryCategory* cat_child)
{
	if (cat_parent == NULL || cat_child == NULL) {
		return false;
	}
	if (cat_child == cat_parent) {
		return true;
	}
	LLInventoryCategory* res = NULL;
	const LLUUID root_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);

	const LLUUID& cat_id = cat_child->getParentUUID();
	res = gInventory.getCategory(cat_id);

	while (res && res->getUUID() != root_id) {
		if (res == cat_parent) {
			return true;
		}
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory(parent_id);
	}
	return false;
}

/*
void RRInterface::renameAttachment(LLInventoryItem* item, LLViewerJointAttachment* attachment)
{
  // DEPRECATED : done directly in the viewer code
	// if item is worn and shared, check its name
	// if it doesn't contain the name of attachment, append it
	// (but truncate the name first if it's too long)
	if (!item || !attachment) return;

	if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(item->getUUID())) {
		if (isUnderRlvShare(item)) {
			LLViewerJointAttachment* attachpt = findAttachmentPointFromName(item->getName());
			if (attachpt == NULL) {

			}
		}
	}
}
*/

LLInventoryCategory* RRInterface::getCategoryUnderRlvShare(std::string catName, LLInventoryCategory* root)
{
	if (root == NULL) root = getRlvShare();
	if (catName.empty()) return root;
	LLStringUtil::toLower(catName);
	std::deque<std::string> tokens = parse(catName, "/");

	// Preliminary action : remove everything after pipes("|"), including pipes themselves
	// This way we can feed the result of a @getinvworn command directly into this method
	// without having to clean what is after the pipes
	S32 nb = tokens.size();
	for (S32 i = 0; i < nb; ++i) {
		std::string tok = tokens[i];
		S32 ind = tok.find('|');
		if (ind != std::string::npos) {
			tok = tok.substr(0, ind);
			tokens[i] = tok;
		}
	}

	if (root) {
		bool exact_match = false;
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(root->getUUID(), cats, items);

		if (cats) {
			S32 count = cats->count();
			LLInventoryCategory* cat = NULL;

			// we need to scan first and retain the best match
			S32 max_size_index = -1;
			S32 max_size = 0;

			for (S32 i = 0; i < count; ++i) {
				cat = cats->get(i);
				std::string name = cat->getName();
				if (!name.empty() && name[0] !=  '.') { // ignore invisible folders
					LLStringUtil::toLower(name);

					S32 size = match(tokens, name, exact_match);
					if (size > max_size || (exact_match && size == max_size)) {
						max_size = size;
						max_size_index = i;
					}
				}
			}

			// only now we can grab the best match and either continue deeper or return it
			if (max_size > 0) {
				cat = cats->get(max_size_index);
				if (max_size == tokens.size()) {
					return cat;
				} else {
					return getCategoryUnderRlvShare(dumpList2String(getSubList(tokens, max_size), "/"), cat);
				}
			}
		}
	}

	if (sRestrainedLoveDebug) {
		llinfos << "category not found" << llendl;
	}
	return NULL;
}

LLInventoryCategory* RRInterface::findCategoryUnderRlvShare(std::string catName, LLInventoryCategory* root)
{
	if (root == NULL) root = getRlvShare();
	LLStringUtil::toLower(catName);
	std::deque<std::string> tokens = parse(catName, "&&");

	if (root) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(root->getUUID(), cats, items);

		if (cats)
		{
			S32 count = cats->count();
			LLInventoryCategory* cat = NULL;

			for (S32 i = 0; i < count; ++i)
			{
				cat = cats->get(i);

				// search deeper first
				LLInventoryCategory* found = findCategoryUnderRlvShare(catName, cat);
				if (found != NULL) return found;

			}
		}
		// return this category if it matches
		std::string name = root->getName();
		LLStringUtil::toLower(name);
		// We can't find invisible folders('.') and dropped folders('~')
		if (!name.empty() && name[0] != '.' && name[0] != '~' && findMultiple(tokens, name)) return root;
	}
	// didn't find anything
	return NULL;
}

std::string RRInterface::findAttachmentNameFromPoint(LLViewerJointAttachment* attachpt)
{
	// return the lowercased name of the attachment, or empty if null
	if (attachpt == NULL) return "";
	std::string res = attachpt->getName();
	LLStringUtil::toLower(res);
	return res;
}

// This struct is meant to be used in RRInterface::findAttachmentPointFromName below
typedef struct
{
	S32 length;
	S32 index;
	LLViewerJointAttachment* attachment;
} Candidate;

LLViewerJointAttachment* RRInterface::findAttachmentPointFromName(std::string objectName, bool exactName)
{
	// for each possible attachment point, check whether its name appears in the name of
	// the item.
	// We are going to scan the whole list of attachments, but we won't decide which one to take right away.
	// Instead, for each matching point, we will store in lists the following results :
	// - length of its name
	// - right-most index where it is found in the name
	// - a pointer to that attachment point
	// When we have that list, choose the highest index, and in case of ex-aequo choose the longest length
	if (objectName.length() < 3)
	{
		// No need to bother: the shorter attachment name is "Top" with 3 characters...
		return NULL;
	}
	if (!isAgentAvatarValid()) {
		llwarns << "NULL avatar pointer. Aborting." << llendl;
		return NULL;
	}
	if (sRestrainedLoveDebug) {
		llinfos << "Searching attachment name with " << (exactName ? "exact match" : "partial matches") << " in: " << objectName << llendl;
	}
	LLStringUtil::toLower(objectName);
	std::string attachName;
	bool found_one = false;
	std::vector<Candidate> candidates;

	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment) {
			attachName = attachment->getName();
			LLStringUtil::toLower(attachName);
//			if (sRestrainedLoveDebug) {
//				llinfos << "Trying attachment: " << attachName << llendl;
//			}
			if (exactName) {
				if (objectName == attachName) return attachment;
			} else {
				size_t ind = objectName.rfind(attachName);
				if (ind != std::string::npos
					&& objectName.substr(0, ind).find('(') != std::string::npos
					&& objectName.substr(ind).find(')') != std::string::npos) {
					Candidate new_candidate;
					new_candidate.index = ind;
					new_candidate.length = attachName.length();
					new_candidate.attachment = attachment;
					candidates.push_back(new_candidate);
					found_one = true;
					if (sRestrainedLoveDebug) {
						llinfos << "New candidate: '" << attachName << "', index=" << new_candidate.index << ", length=" << new_candidate.length << llendl;
					}
				}
			}
		}
	}
	if (!found_one) {
		if (sRestrainedLoveDebug) {
			llinfos << "No attachment found." << llendl;
		}
		return NULL;
	}
	// Now that we have at least one candidate, we have to decide which one to return
	LLViewerJointAttachment* res = NULL;
	Candidate candidate;
	U32 i;
	S32 ind_res = -1;
	S32 max_index = -1;
	S32 max_length = -1;
	// Find the highest index
	for (i = 0; i < candidates.size(); ++i) {
		candidate = candidates[i];
		if (candidate.index > max_index) max_index = candidate.index;
	}
	// Find the longest match among the ones found at that index
	for (i = 0; i < candidates.size(); ++i) {
		candidate = candidates[i];
		if (candidate.index == max_index) {
			if (candidate.length > max_length) {
				max_length = candidate.length;
				ind_res = i;
			}
		}
	}
	// Return this attachment point
	if (ind_res > -1) {
		candidate = candidates[ind_res];
		res = candidate.attachment;
		if (sRestrainedLoveDebug && res) {
			llinfos << "Returning: '" << res->getName() << "'" << llendl;
		}
	}
	return res;
}

LLViewerJointAttachment* RRInterface::findAttachmentPointFromParentName(LLInventoryItem* item)
{
	if (item) {
		// => look in parent folder(this could be a no-mod item), use its name to find the target
		// attach point
		LLViewerInventoryCategory* cat;
		const LLUUID& parent_id = item->getParentUUID();
		cat = gInventory.getCategory(parent_id);
		return findAttachmentPointFromName(cat->getName());
	}
	return NULL;
}

S32 RRInterface::findAttachmentPointNumber(LLViewerJointAttachment* attachment)
{
	if (isAgentAvatarValid()) {
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
			 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
		{
			if (iter->second == attachment) {
				return iter->first;
			}
		}
	}
	return -1;
}

void RRInterface::detachObject(LLViewerObject* object)
{
	// Handle the detach message to the sim here, after a check
	if (!object) return;
	if (gRRenabled && !gAgent.mRRInterface.canDetach(object)) return;

	gMessageSystem->newMessage("ObjectDetach");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgentID );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
	gMessageSystem->sendReliable( gAgent.getRegionHost() );
}

void RRInterface::detachAllObjectsFromAttachment(LLViewerJointAttachment* attachment)
{
	if (!attachment) return;
	LLViewerObject* object;

	// We need to remove all the objects from attachment->mAttachedObjects, one by one.
	// To do this, and in order to avoid any race condition, we are going to copy the list and 
	// iterate on the copy instead of the original which changes everytime something is
	// attached and detached, asynchronously.
	LLViewerJointAttachment::attachedobjs_vec_t attachedObjectsCopy = attachment ->mAttachedObjects;

	for (U32 i = 0; i < attachedObjectsCopy.size(); ++i) {
		object = attachedObjectsCopy.at(i);
		detachObject(object);
	}
}

bool RRInterface::canDetachAllObjectsFromAttachment(LLViewerJointAttachment* attachment)
{
	if (!attachment) return false;
	LLViewerObject* object;

	for (U32 i = 0; i <  attachment ->mAttachedObjects.size(); ++i) {
		object =  attachment ->mAttachedObjects.at(i);
		if (!gAgent.mRRInterface.canDetach(object)) return false;
	}

	return true;
}

void RRInterface::fetchInventory(LLInventoryCategory* root)
{
	// do this only once on login

	if (mInventoryFetched) return;

	bool last_step = false;

	if (root == NULL) {
		root = getRlvShare();
		last_step = true;
	}

	if (root) {
		LLViewerInventoryCategory* viewer_root = (LLViewerInventoryCategory*) root;
		viewer_root->fetch();

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;

		// retrieve all the shared folders
		gInventory.getDirectDescendentsOf(viewer_root->getUUID(), cats, items);
		if (cats) {
			S32 count = cats->count();
			for (S32 i = 0; i < count; ++i) {
				LLInventoryCategory* cat = (LLInventoryCategory*)cats->get(i);
				fetchInventory(cat);
			}
		}
	}

	if (last_step) mInventoryFetched = true;
}

bool RRInterface::forceAttach(std::string category, bool recursive, AttachHow how)
{
	// recursive is true in the case of an attachall command
	// find the category under RLV shared folder
	if (category.empty()) return true; // just a safety
	LLInventoryCategory* cat = getCategoryUnderRlvShare(category);
	bool isRoot = (getRlvShare() == cat);
	bool replacing = (how == AttachHow_replace ||
					  how == AttachHow_over_or_replace); // we're replacing for now, but the name of the category could decide otherwise
	// if exists, wear all the items inside it
	if (cat) {
		// If the name of the category begins with a "+", then we force to stack instead of replacing
		if (how == AttachHow_over_or_replace) {
			if (cat->getName().find(RR_ADD_FOLDER_PREFIX) == 0) {
				replacing = false;
			}
		}

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;

		// retrieve all the objects contained in this folder
		gInventory.getDirectDescendentsOf(cat->getUUID(), cats, items);
		if (items) {
			// wear them one by one
			S32 count = items->count();
			for (S32 i = 0; i < count; ++i) {
				if (!isRoot) {
					LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->get(i);
					if (sRestrainedLoveDebug) {
						llinfos << "trying to attach " << item->getName() << llendl;
					}

					// this is an object to attach somewhere
					if (item && item->getType() == LLAssetType::AT_OBJECT) {
						LLViewerJointAttachment* attachpt = findAttachmentPointFromName(item->getName());

						if (attachpt) {
							if (sRestrainedLoveDebug) {
								llinfos << "attaching item to " << attachpt->getName() << llendl;
							}
							if (replacing) {
								// we're replacing => mimick rezAttachment without displaying an Xml alert to confirm
								S32 number = findAttachmentPointNumber(attachpt);
								if (canDetach(attachpt->getName()) && canAttach(item))
								{
									LLSD payload;
									payload["item_id"] = item->getLinkedUUID();
									payload["attachment_point"] = number;
									LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
								}
							}
							else {
								// we're stacking => call rezAttachment directly
								LLAppearanceMgr::instance().rezAttachment(item,
																		  attachpt,
																		  false);
							}
						}
						else {
							// attachment point is not in the name => stack
							LLAppearanceMgr::instance().rezAttachment(item,
																	  attachpt,
																	  false);
						}
					}
					// this is a piece of clothing
					else if (item->getType() == LLAssetType::AT_CLOTHING
							 || item->getType() == LLAssetType::AT_BODYPART) {
						LLAppearanceMgr::instance().wearInventoryItemOnAvatar(item);
					}
					// this is a gesture
					else if (item->getType() == LLAssetType::AT_GESTURE) {
						if (!gGestureManager.isGestureActive(item->getLinkedUUID())) {
							gGestureManager.activateGesture(item->getLinkedUUID());
						}
					}
				}
			}
		}

		// scan every subfolder of the folder we are attaching, in order to attach no-mod items
		if (cats) {

			// for each subfolder, attach the first item it contains according to its name
			S32 count = cats->count();
			for (S32 i = 0; i < count; ++i) {
				LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->get(i);
				LLViewerJointAttachment* attachpt = findAttachmentPointFromName(cat_child->getName());

				if (!isRoot && attachpt) {
					// this subfolder is properly named => attach the first item it contains
					LLInventoryModel::cat_array_t* cats_grandchildren; // won't be used here
					LLInventoryModel::item_array_t* items_grandchildren; // actual no-mod item(s)
					gInventory.getDirectDescendentsOf(cat_child->getUUID(), 
													  cats_grandchildren, items_grandchildren);

					if (items_grandchildren && items_grandchildren->count() == 1) {
						LLViewerInventoryItem* item_grandchild = 
								(LLViewerInventoryItem*)items_grandchildren->get(0);

						if (item_grandchild && item_grandchild->getType() == LLAssetType::AT_OBJECT
							&& !item_grandchild->getPermissions().allowModifyBy(gAgentID)
							&& findAttachmentPointFromParentName(item_grandchild) != NULL) { // it is no-mod and its parent is named correctly
							// we use the attach point from the name of the folder, not the no-mod item
							if (replacing) {
								// mimick rezAttachment without displaying an Xml alert to confirm
								S32 number = findAttachmentPointNumber(attachpt);
								if (canDetach(attachpt->getName()) && canAttach(item_grandchild))
								{
									LLSD payload;
									payload["item_id"] = item_grandchild->getLinkedUUID();
									payload["attachment_point"] = number;
									LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
								}
							}
							else {
								// we're stacking => call rezAttachment directly
								LLAppearanceMgr::instance().rezAttachment(item_grandchild,
																		  attachpt, false);
							}
						}
					}
				}

				if (recursive && cat_child->getName().find('.') != 0) { // attachall and not invisible)
					forceAttach(getFullPath(cat_child), recursive, how);
				}
			}
		}
	}
	return true;
}

bool RRInterface::forceDetachByName(std::string category, bool recursive)
{
	// find the category under RLV shared folder
	if (category.empty()) return true; // just a safety
	LLInventoryCategory* cat = getCategoryUnderRlvShare(category);
	if (!isAgentAvatarValid()) return false;
	bool isRoot = (getRlvShare() == cat);

	// if exists, detach/unwear all the items inside it
	if (cat) {
		if (mHandleNoStrip) {
			std::string name = cat->getName();
			LLStringUtil::toLower(name);
			if (name.find("nostrip") != std::string::npos) return false;
		}

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;

		// retrieve all the objects contained in this folder
		gInventory.getDirectDescendentsOf(cat->getUUID(), cats, items);
		if (items) {

			// unwear them one by one
			S32 count = items->count();
			for (S32 i = 0; i < count; ++i) {
				if (!isRoot) {
					LLViewerInventoryItem* item = (LLViewerInventoryItem*)items->get(i);
					if (sRestrainedLoveDebug) {
						llinfos << "trying to detach " << item->getName() << llendl;
					}

					if (item->getType() == LLAssetType::AT_OBJECT) {
						// this is an attached object
						// find the attachpoint from which to detach
						for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
							 iter != gAgentAvatarp->mAttachmentPoints.end(); )
						{
							LLVOAvatar::attachment_map_t::iterator curiter = iter++;
							LLViewerJointAttachment* attachment = curiter->second;
							LLViewerObject* object = gAgentAvatarp->getWornAttachment(item->getUUID());
							if (object && attachment && attachment->isObjectAttached(object)) {
								detachObject(object);
								break;
							}
						}
					} else if (item->getType() == LLAssetType::AT_CLOTHING) {
						// this is a piece of clothing
						if (canDetach(item)) removeWearableItemFromAvatar(item);
					} else if (item->getType() == LLAssetType::AT_GESTURE) {
						// this is a gesture
						if (gGestureManager.isGestureActive(item->getLinkedUUID())) {
							gGestureManager.deactivateGesture(item->getLinkedUUID());
						}
					}
				}
			}
		}

		if (cats) {
			// for each subfolder, detach the first item it contains (only for
			// single no-mod items contained in appropriately named folders)
			S32 count = cats->count();
			for (S32 i = 0; i < count; ++i) {
				LLViewerInventoryCategory* cat_child = (LLViewerInventoryCategory*)cats->get(i);
				if (mHandleNoStrip) {
					std::string name = cat_child->getName();
					LLStringUtil::toLower(name);
					if (name.find("nostrip") != std::string::npos) continue;
				}

				LLInventoryModel::cat_array_t* cats_grandchildren; // won't be used here
				LLInventoryModel::item_array_t* items_grandchildren; // actual no-mod item(s)
				gInventory.getDirectDescendentsOf(cat_child->getUUID(), 
													cats_grandchildren, items_grandchildren);

				if (!isRoot && items_grandchildren && items_grandchildren->count() == 1) { // only one item
					LLViewerInventoryItem* item_grandchild = 
							(LLViewerInventoryItem*)items_grandchildren->get(0);

					if (item_grandchild &&
						item_grandchild->getType() == LLAssetType::AT_OBJECT &&
						!item_grandchild->getPermissions().allowModifyBy(gAgentID) &&
						findAttachmentPointFromParentName(item_grandchild) != NULL) { // and it is no-mod and its parent is named correctly
						// detach this object
						// find the attachpoint from which to detach
						for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
							 iter != gAgentAvatarp->mAttachmentPoints.end(); )
						{
							LLVOAvatar::attachment_map_t::iterator curiter = iter++;
							LLViewerJointAttachment* attachment = curiter->second;
							LLViewerObject* object = gAgentAvatarp->getWornAttachment(item_grandchild->getUUID());
							if (object && attachment && attachment->isObjectAttached(object)) {
								detachObject(object);
								break;
							}
						}
					}
				}

				if (recursive && cat_child->getName().find('.') != 0) {
					// detachall and not invisible
					forceDetachByName(getFullPath(cat_child), recursive);
				}
			}
		}
	}
	return true;
}

bool RRInterface::forceTeleport(std::string location)
{
	// location must be X/Y/Z where X, Y and Z are ABSOLUTE coordinates => use a script in-world to translate from local to global
	std::string loc(location);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;
	std::deque<std::string> tokens = parse(location, "/");
	if (tokens.size() == 3) {
		x = atoi(tokens.at(0).c_str());
		y = atoi(tokens.at(1).c_str());
		z = atoi(tokens.at(2).c_str());
	}
	else {
		return false;
	}

	if (sRestrainedLoveDebug) {
		llinfos << tokens.at(0) << "," << tokens.at(1) << "," << tokens.at(2) << "     " << x << "," << y << "," << z << llendl;
	}
	LLVector3d pos_global;
	pos_global.mdV[VX] = (F32)x;
	pos_global.mdV[VY] = (F32)y;
	pos_global.mdV[VZ] = (F32)z;

	mAllowCancelTp = false; // will be checked once receiving the tp order from the sim, then set to true again

	gAgent.teleportViaLocation(pos_global);
	return true;
}

std::string RRInterface::stringReplace(std::string s,
									   std::string what,
									   std::string by,
									   bool caseSensitive /* = false */)
{
//	llinfos << "trying to replace <" << what << "> in <" << s << "> by <" << by << ">" << llendl;
	if (what.empty() || what == " ") return s; // avoid an infinite loop
	size_t ind;
	size_t old_ind = 0;
	size_t len_what = what.length();
	size_t len_by = by.length();
	if (len_by == 0) len_by = 1; // avoid an infinite loop

	while ((ind = s.find("%20")) != std::string::npos) // unescape
	{
		s = s.replace(ind, 3, " ");
	}

	std::string lower = s;
	if (!caseSensitive) {
		LLStringUtil::toLower(lower);
		LLStringUtil::toLower(what);
	}

	while ((ind = lower.find(what, old_ind)) != std::string::npos)
	{
//		llinfos << "ind=" << ind << "    old_ind=" << old_ind << llendl;
		s = s.replace(ind, len_what, by);
		old_ind = ind + len_by;
		lower = s;
		if (!caseSensitive) LLStringUtil::toLower(lower);
	}
	return s;
}

std::string RRInterface::getDummyName(std::string name, EChatAudible audible /* = CHAT_AUDIBLE_FULLY */)
{
	std::string res;
	size_t len = name.length();
	if (len == 0) return res; // just to avoid crashing in some cases

	// We use mLaunchTimestamp in order to modify the scrambling when the
	// session restarts (it stays consistent during the session though)
	// But in crashy situations, let's not make it change at EVERY session,
	// more like once a day or so
	// A day is 86400 seconds, the closest power of two is 65536, that's a
	// 16 bits shift.
	// Very lame hash function I know... but it should be linear enough (the
	// old length method was way too gaussian with a peak at 11 to 16
	// characters)
	unsigned char hash = name.at(0) + name.at(len - 1) + len + (mLaunchTimestamp >> 16);

	unsigned char mod = hash % 28;
	switch (mod) {
		case 0:		res = "A resident";			break;
		case 1:		res = "This resident";		break;
		case 2:		res = "That resident";		break;
		case 3:		res = "An individual";		break;
		case 4:		res = "This individual";	break;
		case 5:		res = "That individual";	break;
		case 6:		res = "A person";			break;
		case 7:		res = "This person";		break;
		case 8:		res = "That person";		break;
		case 9:		res = "A stranger";			break;
		case 10:	res = "This stranger";		break;
		case 11:	res = "That stranger";		break;
		case 12:	res = "A human being";		break;
		case 13:	res = "This human being";	break;
		case 14:	res = "That human being";	break;
		case 15:	res = "An agent";			break;
		case 16:	res = "This agent";			break;
		case 17:	res = "That agent";			break;
		case 18:	res = "A soul";				break;
		case 19:	res = "This soul";			break;
		case 20:	res = "That soul";			break;
		case 21:	res = "Somebody";			break;
		case 22:	res = "Anonymous one";		break;
		case 23:	res = "Someone";			break;
		case 24:	res = "Mysterious one";		break;
		case 25:	res = "An unknown being";	break;
		case 26:	res = "Unidentified one";	break;
		default:	res = "An unknown person";	break;
	}
	if (audible == CHAT_AUDIBLE_BARELY) {
		res += " afar";
	}
	return res;
}

std::string RRInterface::getCensoredMessage(std::string str)
{
	// Hide every occurrence of the name of anybody around(found in cache, so not completely accurate nor completely immediate)
	std::vector<LLUUID> avatar_ids;
	LLUUID avatar_id;
	std::string name, dummy_name;
	LLAvatarName avatar_name;
	U32 i;

	LLWorld::getInstance()->getAvatars(&avatar_ids, NULL);

	for (i = 0; i < avatar_ids.size(); i++) {
		avatar_id = avatar_ids[i];
		if (gCacheName->getFullName(avatar_id, name)) {
			dummy_name = getDummyName(name);
			str = stringReplace(str, name, dummy_name); // legacy name
			if (LLAvatarNameCache::get(avatar_id, &avatar_name)) {
				if (!avatar_name.mIsDisplayNameDefault) {
					name = avatar_name.mDisplayName;
					str = stringReplace(str, name, dummy_name); // display name
				}
			}
		}
	}

	return str;
}

void updateAndSave(WLColorControl* color)
{
	if (color == NULL) return;
	color->i = color->r;
	if (color->g > color->i) {
		color->i = color->g;
	}
	if (color->b > color->i) {
		color->i = color->b;
	}
	color->update(LLWLParamManager::instance()->mCurParams);
}

void updateAndSave(WLFloatControl* floatControl)
{
	if (floatControl == NULL) return;
	floatControl->update(LLWLParamManager::instance()->mCurParams);
}

bool RRInterface::forceEnvironment(std::string command, std::string option)
{
	// command is "setenv_<something>"
	F32 val = (F32)atof(option.c_str());

	size_t length = 7; // size of "setenv_"
	command = command.substr(length);
	LLWLParamManager* params = LLWLParamManager::instance();

	params->mAnimator.mIsRunning = false;
	params->mAnimator.mUseLindenTime = false;

	if (command == "daytime") {
		if (val > 1.0) val = 1.0;
		if (val >= 0.0) {
			params->mAnimator.setDayTime(val);
			params->mAnimator.update(params->mCurParams);
		}
		else {
			LLWLParamManager::instance()->mAnimator.mIsRunning = true;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = true;
		}
	}

	else if (command == "bluehorizonr") {
		params->mBlueHorizon.r = val * 2;
		updateAndSave(&(params->mBlueHorizon));
	}
	else if (command == "bluehorizong") {
		params->mBlueHorizon.g = val * 2;
		updateAndSave(&(params->mBlueHorizon));
	}
	else if (command == "bluehorizonb") {
		params->mBlueHorizon.b = val * 2;
		updateAndSave(&(params->mBlueHorizon));
	}
	else if (command == "bluehorizoni") {
		F32 old_intensity = llmax(params->mBlueHorizon.r, params->mBlueHorizon.g, params->mBlueHorizon.b);
		if (val == 0 || old_intensity == 0) {
			params->mBlueHorizon.r = params->mBlueHorizon.g = params->mBlueHorizon.b = val * 2;
		} else {
			params->mBlueHorizon.r *= val * 2 / old_intensity;
			params->mBlueHorizon.g *= val * 2 / old_intensity;
			params->mBlueHorizon.b *= val * 2 / old_intensity;
		}
		updateAndSave(&(params->mBlueHorizon));
	}

	else if (command == "bluedensityr") {
		params->mBlueDensity.r = val * 2;
		updateAndSave(&(params->mBlueDensity));
	}
	else if (command == "bluedensityg") {
		params->mBlueDensity.g = val * 2;
		updateAndSave(&(params->mBlueDensity));
	}
	else if (command == "bluedensityb") {
		params->mBlueDensity.b = val * 2;
		updateAndSave(&(params->mBlueDensity));
	}
	else if (command == "bluedensityi") {
		F32 old_intensity = llmax(params->mBlueDensity.r, params->mBlueDensity.g, params->mBlueDensity.b);
		if (val == 0 || old_intensity == 0) {
			params->mBlueDensity.r = params->mBlueDensity.g = params->mBlueDensity.b = val * 2;
		} else {
			params->mBlueDensity.r *= val * 2 / old_intensity;
			params->mBlueDensity.g *= val * 2 / old_intensity;
			params->mBlueDensity.b *= val * 2 / old_intensity;
		}
		updateAndSave(&(params->mBlueDensity));
	}

	else if (command == "hazehorizon") {
		params->mHazeHorizon.r = val * 2;
		params->mHazeHorizon.g = val * 2;
		params->mHazeHorizon.b = val * 2;
		updateAndSave(&(params->mHazeHorizon));
	}
	else if (command == "hazedensity") {
		params->mHazeDensity.r = val * 2;
		params->mHazeDensity.g = val * 2;
		params->mHazeDensity.b = val * 2;
		updateAndSave(&(params->mHazeDensity));
	}

	else if (command == "densitymultiplier") {
		params->mDensityMult.x = val / 1000;
		updateAndSave(&(params->mDensityMult));
//		LLWaterParamManager* water_params = LLWaterParamManager::instance();
//		water_params->mFogDensity.mExp = 5.0;
//		water_params->mFogDensity.update(water_params->mCurParams);
	}

	else if (command == "distancemultiplier") {
		params->mDistanceMult.x = val;
		updateAndSave(&(params->mDistanceMult));
//		LLWaterParamManager* water_params = LLWaterParamManager::instance();
//		water_params->mUnderWaterFogMod.mX = 1.0;
//		water_params->mUnderWaterFogMod.update(water_params->mCurParams);
	}

	else if (command == "maxaltitude") {
		params->mMaxAlt.x = val;
		updateAndSave(&(params->mMaxAlt));
	}

	else if (command == "sunmooncolorr") {
		params->mSunlight.r = val * 3;
		updateAndSave(&(params->mSunlight));
	}
	else if (command == "sunmooncolorg") {
		params->mSunlight.g = val * 3;
		updateAndSave(&(params->mSunlight));
	}
	else if (command == "sunmooncolorb") {
		params->mSunlight.b = val * 3;
		updateAndSave(&(params->mSunlight));
	}
	else if (command == "sunmooncolori") {
		F32 old_intensity = llmax(params->mSunlight.r, params->mSunlight.g, params->mSunlight.b);
		if (val == 0 || old_intensity == 0) {
			params->mSunlight.r = params->mSunlight.g = params->mSunlight.b = val * 3;
		} else {
			params->mSunlight.r *= val * 3 / old_intensity;
			params->mSunlight.g *= val * 3 / old_intensity;
			params->mSunlight.b *= val * 3 / old_intensity;
		}
		updateAndSave(&(params->mSunlight));
	}

	else if (command == "ambientr") {
		params->mAmbient.r = val * 3;
		updateAndSave(&(params->mAmbient));
	}
	else if (command == "ambientg") {
		params->mAmbient.g = val * 3;
		updateAndSave(&(params->mAmbient));
	}
	else if (command == "ambientb") {
		params->mAmbient.b = val * 3;
		updateAndSave(&(params->mAmbient));
	}
	else if (command == "ambienti") {
		F32 old_intensity = llmax(params->mAmbient.r, params->mAmbient.g, params->mAmbient.b);
		if (val == 0 || old_intensity == 0) {
			params->mAmbient.r = params->mAmbient.g = params->mAmbient.b = val * 3;
		} else {
			params->mAmbient.r *= val * 3 / old_intensity;
			params->mAmbient.g *= val * 3 / old_intensity;
			params->mAmbient.b *= val * 3 / old_intensity;
		}
		updateAndSave(&(params->mAmbient));
	}

	else if (command == "sunglowfocus") {
		params->mGlow.b = -val * 5;
		updateAndSave(&(params->mGlow));
	}
	else if (command == "sunglowsize") {
		params->mGlow.r = (2 - val) * 20;
		updateAndSave(&(params->mGlow));
	}

	else if (command == "scenegamma") {
		params->mWLGamma.x = val;
		updateAndSave(&(params->mWLGamma));
	}

	else if (command == "sunmoonposition") {
		params->mCurParams.setSunAngle(F_TWO_PI * val);
	}

	else if (command == "eastangle") {
		params->mCurParams.setEastAngle(F_TWO_PI * val);
	}

	else if (command == "starbrightness") {
		params->mCurParams.setStarBrightness(val);
	}

	else if (command == "cloudcolorr") {
		params->mCloudColor.r = val;
		updateAndSave(&(params->mCloudColor));
	}
	else if (command == "cloudcolorg") {
		params->mCloudColor.g = val;
		updateAndSave(&(params->mCloudColor));
	}
	else if (command == "cloudcolorb") {
		params->mCloudColor.b = val;
		updateAndSave(&(params->mCloudColor));
	}
	else if (command == "cloudcolori") {
		F32 old_intensity = llmax(params->mCloudColor.r, params->mCloudColor.g, params->mCloudColor.b);
		if (val == 0 || old_intensity == 0) {
			params->mCloudColor.r = params->mCloudColor.g = params->mCloudColor.b = val;
		} else {
			params->mCloudColor.r *= val / old_intensity;
			params->mCloudColor.g *= val / old_intensity;
			params->mCloudColor.b *= val / old_intensity;
		}
		updateAndSave(&(params->mCloudColor));
	}

	else if (command == "cloudx") {
		params->mCloudMain.r = val;
		updateAndSave(&(params->mCloudMain));
	}
	else if (command == "cloudy") {
		params->mCloudMain.g = val;
		updateAndSave(&(params->mCloudMain));
	}
	else if (command == "cloudd") {
		params->mCloudMain.b = val;
		updateAndSave(&(params->mCloudMain));
	}

	else if (command == "clouddetailx") {
		params->mCloudDetail.r = val;
		updateAndSave(&(params->mCloudDetail));
	}
	else if (command == "clouddetaily") {
		params->mCloudDetail.g = val;
		updateAndSave(&(params->mCloudDetail));
	}
	else if (command == "clouddetaild") {
		params->mCloudDetail.b = val;
		updateAndSave(&(params->mCloudDetail));
	}

	else if (command == "cloudcoverage") {
		params->mCloudCoverage.x = val;
		updateAndSave(&(params->mCloudCoverage));
	}

	else if (command == "cloudscale") {
		params->mCloudScale.x = val;
		updateAndSave(&(params->mCloudScale));
	}

	else if (command == "cloudscrollx") {
		params->mCurParams.setCloudScrollX(val+10);
	}
	else if (command == "cloudscrolly") {
		params->mCurParams.setCloudScrollY(val+10);
	}

	// sunglowfocus 0-0.5, sunglowsize 0-2, scenegamma 0-10, starbrightness 0-2
	// cloudcolor rgb 0-1, cloudxydensity xyd 0-1, cloudcoverage 0-1, cloudscale 0-1, clouddetail xyd 0-1
	// cloudscrollx 0-1, cloudscrolly 0-1, drawclassicclouds 0/1

	else if (command == "preset") {
		params->loadPreset(option);
	}

	// send the current parameters to shaders
	LLWLParamManager::instance()->propagateParameters();

	// Update the floaters, when they are open
	if (LLFloaterWindLight::isOpen()) {
		LLFloaterWindLight::instance()->syncMenu();
	}
	if (LLFloaterDayCycle::isOpen()) {
		LLFloaterDayCycle::instance()->syncMenu();
	}

	return true;
}

std::string RRInterface::getEnvironment(std::string command)
{
	F32 res = 0;
	size_t length = 7; // size of "getenv_"
	command = command.substr(length);
	LLWLParamManager* params = LLWLParamManager::instance();

	if (command == "daytime") {
		if (params->mAnimator.mIsRunning && params->mAnimator.mUseLindenTime) res = -1;
		else res = params->mAnimator.getDayTime();
	}

	else if (command == "bluehorizonr") res = params->mBlueHorizon.r / 2;
	else if (command == "bluehorizong") res = params->mBlueHorizon.g / 2;
	else if (command == "bluehorizonb") res = params->mBlueHorizon.b / 2;
	else if (command == "bluehorizoni") res = llmax(params->mBlueHorizon.r, params->mBlueHorizon.g, params->mBlueHorizon.b) / 2;

	else if (command == "bluedensityr") res = params->mBlueDensity.r / 2;
	else if (command == "bluedensityg") res = params->mBlueDensity.g / 2;
	else if (command == "bluedensityb") res = params->mBlueDensity.b / 2;
	else if (command == "bluedensityi") res = llmax(params->mBlueDensity.r, params->mBlueDensity.g, params->mBlueDensity.b) / 2;

	else if (command == "hazehorizon")  res = llmax(params->mHazeHorizon.r, params->mHazeHorizon.g, params->mHazeHorizon.b) / 2;
	else if (command == "hazedensity")  res = llmax(params->mHazeDensity.r, params->mHazeDensity.g, params->mHazeDensity.b) / 2;

	else if (command == "densitymultiplier")  res = params->mDensityMult.x*1000;
	else if (command == "distancemultiplier") res = params->mDistanceMult.x;
	else if (command == "maxaltitude")        res = params->mMaxAlt.x;

	else if (command == "sunmooncolorr") res = params->mSunlight.r / 3;
	else if (command == "sunmooncolorg") res = params->mSunlight.g / 3;
	else if (command == "sunmooncolorb") res = params->mSunlight.b / 3;
	else if (command == "sunmooncolori") res = llmax(params->mSunlight.r, params->mSunlight.g, params->mSunlight.b) / 3;

	else if (command == "ambientr") res = params->mAmbient.r / 3;
	else if (command == "ambientg") res = params->mAmbient.g / 3;
	else if (command == "ambientb") res = params->mAmbient.b / 3;
	else if (command == "ambienti") res = llmax(params->mAmbient.r, params->mAmbient.g, params->mAmbient.b) / 3;

	else if (command == "sunglowfocus")	res = -params->mGlow.b / 5;
	else if (command == "sunglowsize")	res = 2 - params->mGlow.r / 20;
	else if (command == "scenegamma")	res = params->mWLGamma.x;

	else if (command == "sunmoonposition")	res = params->mCurParams.getSunAngle() / F_TWO_PI;
	else if (command == "eastangle")		res = params->mCurParams.getEastAngle() / F_TWO_PI;
	else if (command == "starbrightness")	res = params->mCurParams.getStarBrightness();

	else if (command == "cloudcolorr") res = params->mCloudColor.r;
	else if (command == "cloudcolorg") res = params->mCloudColor.g;
	else if (command == "cloudcolorb") res = params->mCloudColor.b;
	else if (command == "cloudcolori") res = llmax(params->mCloudColor.r, params->mCloudColor.g, params->mCloudColor.b);

	else if (command == "cloudx")  res = params->mCloudMain.r;
	else if (command == "cloudy")  res = params->mCloudMain.g;
	else if (command == "cloudd")  res = params->mCloudMain.b;

	else if (command == "clouddetailx")  res = params->mCloudDetail.r;
	else if (command == "clouddetaily")  res = params->mCloudDetail.g;
	else if (command == "clouddetaild")  res = params->mCloudDetail.b;

	else if (command == "cloudcoverage")	res = params->mCloudCoverage.x;
	else if (command == "cloudscale")		res = params->mCloudScale.x;

	else if (command == "cloudscrollx") res = params->mCurParams.getCloudScrollX() - 10;
	else if (command == "cloudscrolly") res = params->mCurParams.getCloudScrollY() - 10;

	else if (command == "preset") return getLastLoadedPreset();

	std::stringstream str;
	str << res;
	return str.str();
}

bool RRInterface::forceDebugSetting(std::string command, std::string option)
{
	//	MK: As some debug settings are critical to the user's experience and others
	//	are just useless/not used, we are following a whitelist approach : only allow
	//	certain debug settings to be changed and not all.

	// command is "setdebug_<something>"

	size_t length = 9; // size of "setdebug_"
	command = command.substr(length);
	LLStringUtil::toLower(command);
	std::string allowed;
	std::string tmp;
	size_t ind;

	allowed = mAllowedU32;
	tmp = allowed;
	LLStringUtil::toLower(tmp);
	if ((ind = tmp.find("," + command + ",")) != std::string::npos) {
		gSavedSettings.setU32(allowed.substr(++ind, command.length()), atoi(option.c_str()));
		return true;
	}

	return true;
}

std::string RRInterface::getDebugSetting(std::string command)
{
	std::stringstream res;
	size_t length = 9; // size of "getdebug_"
	command = command.substr(length);
	LLStringUtil::toLower(command);
	std::string allowed;
	std::string tmp;
	size_t ind;

	allowed = mAllowedU32;
	tmp = allowed;
	LLStringUtil::toLower(tmp);
	if ((ind = tmp.find("," + command + ",")) != std::string::npos) {
		res << gSavedSettings.getU32(allowed.substr(++ind, command.length()));
	}

	return res.str();
}

std::string RRInterface::getFullPath(LLInventoryCategory* cat)
{
	if (cat == NULL) return "";
	LLInventoryCategory* rlv = gAgent.mRRInterface.getRlvShare();
	if (rlv == NULL) return "";
	LLInventoryCategory* res = cat;
	std::deque<std::string> tokens;

	while (res && res != rlv) {
		tokens.push_front(res->getName());
		const LLUUID& parent_id = res->getParentUUID();
		res = gInventory.getCategory(parent_id);
	}
	return dumpList2String(tokens, "/");
}

std::string RRInterface::getFullPath(LLInventoryItem* item, std::string option, bool full_list /*= true*/)
{
	if (sRestrainedLoveDebug) {
		llinfos << "getFullPath(" << (item ? item->getName() : "NULL") << ", "
				<< option << ", " << full_list << ")" << llendl;
	}
	// Returns the path from the shared root to this object, or to the object
	// worn at the attach point or clothing layer pointed by option if any
	if (!option.empty()) {
		// an option is specified => we don't want to check the item that
		// issued the command, but something else that is currently worn
		// (object or clothing)
		item = NULL;
		LLWearableType::EType wearable_type = gAgent.mRRInterface.getOutfitLayerAsType(option);
		if (wearable_type != LLWearableType::WT_INVALID) {
			// this is a clothing layer => replace item with the piece clothing
			std::deque<std::string> res;
			for (U32 i = 0; i < LLAgentWearables::MAX_CLOTHING_PER_TYPE; ++i) {
				LLUUID id = gAgentWearables.getWearableItemID(wearable_type, i);
				if (id.notNull()) {
					item = gInventory.getItem(id);
					// security : we would return the path even if the item was
					// not shared otherwise
					if (item && gAgent.mRRInterface.isUnderRlvShare(item)) {
						// We have found the inventory item => add its path to
						// the list. It appears to be a recursive call but the
						// level of recursivity is only 2, we won't execute
						// this instruction again in the called method since
						// "option" will be empty
						res.push_back(getFullPath(item, ""));
						if (sRestrainedLoveDebug) {
							llinfos << "res = " << dumpList2String(res, ", ")
									<< llendl;
						}
						if (!full_list) {
							// old behaviour: we only return the first folder,
							// not a full list
							break;
						}
					}
				}
			}
			return dumpList2String(res, ",");
		} else {
			// this is not a clothing layer => it has to be an attachment point
			LLViewerJointAttachment* attach_point = gAgent.mRRInterface.findAttachmentPointFromName(option, true);
			if (attach_point) {
				std::deque<std::string> res;
				for (U32 i = 0; i < attach_point->mAttachedObjects.size(); ++i) {
					LLViewerObject* attached_object = attach_point->mAttachedObjects.at(i);
					if (attached_object) {
						item = getItemAux(attached_object, gAgent.mRRInterface.getRlvShare());
						if (item != NULL && !gAgent.mRRInterface.isUnderRlvShare(item)) item = NULL; // security : we would return the path even if the item was not shared otherwise
						else {
							// We have found the inventory item => add its path to the list
							// it appears to be a recursive call but the level of recursivity is only 2, we won't execute this instruction again in the called method since "option" will be empty
							res.push_back(getFullPath(item, ""));
							if (sRestrainedLoveDebug) {
								llinfos << "res=" << dumpList2String(res, ", ") << llendl;
							}
							if (!full_list) break; // old behaviour : we only return the first folder, not a full list
						}
					}
				}
				return dumpList2String(res, ",");
			}
		}
	}

	if (!item || !gAgent.mRRInterface.isUnderRlvShare(item)) {
		// security : we would return the path even if the item was not shared
		// otherwise
		return "";
	}

	LLUUID parent_id = item->getParentUUID();
	LLInventoryCategory* parent_cat = gInventory.getCategory(parent_id);

	if (item->getType() == LLAssetType::AT_OBJECT &&
		!item->getPermissions().allowModifyBy(gAgentID)) {
		if (gAgent.mRRInterface.findAttachmentPointFromName(parent_cat->getName())) {
			// this item is no-mod and its parent folder contains the name of
			// an attach point => probably we want the full path only to the
			// containing folder of that folder
			parent_id = parent_cat->getParentUUID();
			parent_cat = gInventory.getCategory(parent_id);
			return getFullPath(parent_cat);
		}
	}

	return getFullPath(parent_cat);
}

LLInventoryItem* RRInterface::getItemAux(LLViewerObject* attached_object, LLInventoryCategory* root)
{
	// auxiliary function for getItem()
	if (!attached_object) return NULL;
	if (root && isAgentAvatarValid()) {
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(root->getUUID(), cats, items);
		S32 count;
		S32 i;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;

		// Try to find the item in the current category
		count = items->count();
		for (i = 0; i < count; ++i) {
			item = items->get(i);
			if (item && (item->getType() == LLAssetType::AT_OBJECT || item->getType() == LLAssetType::AT_CLOTHING) &&
				gAgentAvatarp->getWornAttachment(item->getUUID()) == attached_object)
			{
				// found the item in the current category
				return item;
			}
		}

		// We didn't find it here => browse the children categories
		count = cats->count();
		for (i = 0; i < count; ++i) {
			cat = cats->get(i);
			item = getItemAux(attached_object, cat);
			if (item != NULL) return item;
		}
	}
	// We didn't find it(this should not happen)
	return NULL;
}

LLInventoryItem* RRInterface::getItem(LLUUID wornObjectUuidInWorld)
{
	// return the inventory item corresponding to the viewer object which UUID is "wornObjectUuidInWorld", if any
	LLViewerObject* object = gObjectList.findObject(wornObjectUuidInWorld);
	if (object != NULL) {
		object = object->getRootEdit();
		if (object->isAttachment()) {
			return gInventory.getItem(object->getAttachmentItemID());
		}
	}
	// This object is not worn => it has nothing to do with any inventory item
	return NULL;
}

void RRInterface::attachObjectByUUID(LLUUID assetUUID, S32 attachPtNumber)
{
	// caution: this method does NOT check that the target attach point is
	// already used by a locked item
	if (!isAgentAvatarValid()) return;
	LLSD payload;
	payload["item_id"] = assetUUID;
	if (gAgentAvatarp->allowMultipleAttachments() &&
		gAgentAvatarp->canAttachMoreObjects())
	{
		payload["attachment_point"] = attachPtNumber | ATTACHMENT_ADD;
	}
	else
	{
		payload["attachment_point"] = attachPtNumber;
	}
	LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
}

bool RRInterface::canDetachAllSelectedObjects()
{
	for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
		LLViewerObject* object = (*curiter)->getObject();
		if (object && !canDetach(object))
		{
			return false;
		}
	}
	return true;
}

bool RRInterface::isSittingOnAnySelectedObject()
{
	if (!isAgentAvatarValid() || !gAgentAvatarp->mIsSitting) {
		return false;
	}

	for (LLObjectSelection::iterator iter = LLSelectMgr::getInstance()->getSelection()->begin();
		 iter != LLSelectMgr::getInstance()->getSelection()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
		LLViewerObject* object = (*curiter)->getObject();
		if (object && object->isSeat())
		{
			return true;
		}
	}
	return false;
}

bool RRInterface::canAttachCategory(LLInventoryCategory* folder, bool with_exceptions /*= true*/)
{
	// return false if :
	// - at least one object issued a @attachthis:folder restriction
	// - at least one item in this folder is to be worn on a @attachthis:attachpt restriction
	// - at least one piece of clothing in this folder is to be worn on a @attachthis:layer restriction
	// - any parent folder returns false with @attachallthis
	if (!folder || !isAgentAvatarValid()) return true;
	LLInventoryCategory* rlvShare = getRlvShare();
	if (!rlvShare || !isUnderRlvShare(folder)) {
		return !contains("unsharedwear");
	}
	return canAttachCategoryAux(folder, false, false, with_exceptions);
}

bool RRInterface::canAttachCategoryAux(LLInventoryCategory* folder, bool in_parent, bool in_no_mod, bool with_exceptions /*= true*/)
{
	if (!isAgentAvatarValid()) return true;
	FolderLock folder_lock = FolderLock_unlocked;
	if (folder) {
		// check @attachthis:folder in all restrictions
		RRMAP::iterator it = mSpecialObjectBehaviours.begin();
		//LLInventoryCategory* restricted_cat;
		std::string path_to_check;
		std::string restriction = "attachthis";
		if (in_parent) restriction = "attachallthis";
		folder_lock = isFolderLockedWithoutException(folder, "attach");
		if (folder_lock == FolderLock_locked_without_except) {
			return false;
		}

		if (!with_exceptions && folder_lock == FolderLock_locked_with_except) {
			return false;
		}

		//while (it != mSpecialObjectBehaviours.end()) {
		//	if (it->second.find(restriction+":") == 0) {
		//		path_to_check = it->second.substr(restriction.length()+1); // remove ":" as well
		//		restricted_cat = getCategoryUnderRlvShare(path_to_check);
		//		if (restricted_cat == folder) return false;
		//	}
		//	it++;
		//}

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(folder->getUUID(), cats, items);
		S32 count;
		S32 i;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;

		// Try to find the item in the current category
		count = items->count();
		for (i = 0; i < count; ++i) {
			item = items->get(i);
			if (item) {
				if (item->getType() == LLAssetType::AT_OBJECT) {
					LLViewerJointAttachment* attachpt = NULL;
					if (in_no_mod) {
						if (item->getPermissions().allowModifyBy(gAgentID) || count > 1) return true;
						LLInventoryCategory* parent = gInventory.getCategory(folder->getParentUUID());
						attachpt = findAttachmentPointFromName(parent->getName());
					}
					else {
						attachpt = findAttachmentPointFromName(item->getName());
					}
					if (attachpt && contains(restriction + ":" + attachpt->getName())) return false;
				}
				else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
					LLWearable* wearable = gAgentWearables.getWearableFromItemID(item->getLinkedUUID());
					if (wearable) {
						if (contains(restriction + ":" + getOutfitLayerAsString(wearable->getType()))) return false;
					}
				}
			}
		}

		// now check all no-mod items => look at the sub-categories and return false if any of them returns false on a call to canAttachCategoryAux()
		count = cats->count();
		for (i = 0; i < count; ++i) {
			cat = cats->get(i);
			if (cat) {
				std::string name = cat->getName();
				if (!name.empty() && name[0] ==  '.' && findAttachmentPointFromName(name) != NULL) {
					if (!canAttachCategoryAux(cat, false, true, with_exceptions)) return false;
				}
			}
		}
	}
	if (folder == getRlvShare()) return true;
	if (!in_no_mod && folder_lock == FolderLock_unlocked) {
		return canAttachCategoryAux(gInventory.getCategory(folder->getParentUUID()), true, false, with_exceptions); // check for @attachallthis in the parent
	}
	return true;
}

bool RRInterface::canDetachCategory(LLInventoryCategory* folder, bool with_exceptions /*= true*/)
{
	// return false if :
	// - at least one object contained in this folder issued a @detachthis restriction
	// - at least one object issued a @detachthis:folder restriction
	// - at least one worn attachment in this folder is worn on a @detachthis:attachpt restriction
	// - at least one worn piece of clothing in this folder is worn on a @detachthis:layer restriction
	// - any parent folder returns false with @detachallthis
	if (!folder || !isAgentAvatarValid()) return true;

	if (mHandleNoStrip) {
		std::string name = folder->getName();
		LLStringUtil::toLower(name);
		if (name.find("nostrip") != std::string::npos) return false;
	}

	LLInventoryCategory* rlvShare = getRlvShare();
	if (!rlvShare || !isUnderRlvShare(folder)) {
		return !contains("unsharedunwear");
	}

	return canDetachCategoryAux(folder, false, false, with_exceptions);
}

bool RRInterface::canDetachCategoryAux(LLInventoryCategory* folder, bool in_parent, bool in_no_mod, bool with_exceptions /*= true*/)
{
	if (!isAgentAvatarValid()) return true;
	FolderLock folder_lock = FolderLock_unlocked;
	if (folder) {
		// check @detachthis:folder in all restrictions
		RRMAP::iterator it = mSpecialObjectBehaviours.begin();
		//LLInventoryCategory* restricted_cat;
		std::string path_to_check;
		std::string restriction = "detachthis";
		if (in_parent) restriction = "detachallthis";
		folder_lock = isFolderLockedWithoutException(folder, "detach");
		if (folder_lock == FolderLock_locked_without_except) {
			return false;
		}

		if (!with_exceptions && folder_lock == FolderLock_locked_with_except) {
			return false;
		}

		//while (it != mSpecialObjectBehaviours.end()) {
		//	if (it->second.find(restriction+":") == 0) {
		//		path_to_check = it->second.substr(restriction.length()+1); // remove ":" as well
		//		restricted_cat = getCategoryUnderRlvShare(path_to_check);
		//		if (restricted_cat == folder) return false;
		//	}
		//	it++;
		//}

		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		gInventory.getDirectDescendentsOf(folder->getUUID(), cats, items);
		S32 count;
		S32 i;
		LLInventoryItem* item = NULL;
		LLInventoryCategory* cat = NULL;

		// Try to find the item in the current category
		count = items->count();
		for (i = 0; i < count; ++i) {
			item = items->get(i);
			if (item) {
				if (item->getType() == LLAssetType::AT_OBJECT) {
					if (in_no_mod) {
						if (item->getPermissions().allowModifyBy(gAgentID) || count > 1) return true;
					}
					LLViewerObject* attached_object = gAgentAvatarp->getWornAttachment(item->getLinkedUUID());
					if (attached_object) {
						if (!isAllowed(attached_object->getRootEdit()->getID(), restriction)) return false;
						if (contains(restriction + ":" + gAgentAvatarp->getAttachedPointName(item->getLinkedUUID()))) return false;
					}
				}
				else if (item->getType() == LLAssetType::AT_CLOTHING || item->getType() == LLAssetType::AT_BODYPART) {
					LLWearable* wearable = gAgentWearables.getWearableFromItemID(item->getLinkedUUID());
					if (wearable) {
						if (contains(restriction + ":" + getOutfitLayerAsString(wearable->getType()))) return false;
					}
				}
			}
		}

		// now check all no-mod items => look at the sub-categories and return
		// false if any of them returns false on a call to canDetachCategoryAux()
		count = cats->count();
		for (i = 0; i < count; ++i) {
			cat = cats->get(i);
			if (cat) {
				std::string name = cat->getName();
				if (!name.empty() && name[0] ==  '.' && findAttachmentPointFromName(name) != NULL) {
					if (!canDetachCategoryAux(cat, false, true)) return false;
				}
			}
		}
	}
	if (folder == getRlvShare()) return true;
	if (!in_no_mod && folder_lock == FolderLock_unlocked) {
		return canDetachCategoryAux(gInventory.getCategory(folder->getParentUUID()), true, false, with_exceptions); // check for @detachallthis in the parent
	}
	return true;
}

bool RRInterface::canUnwear(LLViewerInventoryItem* item)
{
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	if (mRestoringOutfit) {
		return true;
	}

	if (item) {
		if (item->getType() ==  LLAssetType::AT_OBJECT) {
			return canDetach(item);
		} else if (item->getType() == LLAssetType::AT_CLOTHING ||
				   item->getType() == LLAssetType::AT_BODYPART) {
			if (!canUnwear(item->getWearableType())) return false;
			LLInventoryCategory* parent = gInventory.getCategory(item->getParentUUID());
			if (!canDetachCategory(parent)) return false;
		}
	}
	return true;
}

bool RRInterface::canUnwear(LLWearableType::EType type)
{
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	if (mRestoringOutfit) {
		return true;
	}

	if (contains("remoutfit")) {
		return false;
	}
	else if (contains("remoutfit:" + getOutfitLayerAsString(type))) {
		return false;
	}
	return true;
}

bool RRInterface::canWear(LLViewerInventoryItem* item)
{
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	if (mRestoringOutfit) {
		return true;
	}

	if (item) {
		LLInventoryCategory* parent = gInventory.getCategory(item->getParentUUID());
		if (item->getType() ==  LLAssetType::AT_OBJECT) {
			LLViewerJointAttachment* attachpt = findAttachmentPointFromName(item->getName());
			if (attachpt) {
				if (!canAttach(NULL, attachpt->getName())) return false;
			}
			return canAttachCategory(parent);
		}
		else if (item->getType() == LLAssetType::AT_CLOTHING ||
				 item->getType() == LLAssetType::AT_BODYPART) {
			if (!canWear(item->getWearableType())) return false;
			if (!canAttachCategory(parent)) return false;
		}
	}
	return true;
}

bool RRInterface::canWear(LLWearableType::EType type)
{
	// If we are still a cloud, we can always wear bodyparts because we are
	// still logging on
	if (gAgentAvatarp && gAgentAvatarp->getIsCloud())
	{
		if (type == LLWearableType::WT_SHAPE ||
			type == LLWearableType::WT_HAIR ||
			type == LLWearableType::WT_EYES ||
			type == LLWearableType::WT_SKIN) {
			return true;
		}
	}
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	else if (mRestoringOutfit) {
		return true;
	}
	else if (contains("addoutfit")) {
		return false;
	}
	else if (contains("addoutfit:" + getOutfitLayerAsString(type))) {
		return false;
	}
	return true;
}

bool RRInterface::canDetach(LLViewerInventoryItem* item)
{
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	if (mRestoringOutfit) {
		return true;
	}
		
	if (!item || !isAgentAvatarValid()) return true;

	if (mHandleNoStrip) {
		std::string name = item->getName();
		LLStringUtil::toLower(name);
		if (name.find("nostrip") != std::string::npos) return false;
	}

	if (item->getType() == LLAssetType::AT_OBJECT) {
		// we'll check canDetachCategory() inside this function
		return canDetach(gAgentAvatarp->getWornAttachment(item->getLinkedUUID()));
	}
	else if (item->getType() == LLAssetType::AT_CLOTHING) {
		LLInventoryCategory* cat_parent = gInventory.getCategory(item->getParentUUID());
		if (cat_parent && !canDetachCategory(cat_parent)) return false;
		const LLWearable* wearable = gAgentWearables.getWearableFromItemID(item->getUUID());
		if (wearable) return canUnwear(wearable->getType());
	}
	return true;
}

bool RRInterface::canDetach(LLViewerObject* attached_object)
{
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	if (mRestoringOutfit) {
		return true;
	}

	if (!attached_object) return true;
	LLViewerObject* root = attached_object->getRootEdit();
	if (!root) return true;

	// Check all the current restrictions, if "detach" is issued from a child
	// prim of the root prim of attached_object, then the whole object is
	// undetachable
	RRMAP::iterator it = mSpecialObjectBehaviours.begin();
	while (it != mSpecialObjectBehaviours.end()) {
		if (it->second == "detach") {
			LLViewerObject* this_prim = gObjectList.findObject(LLUUID(it->first));
			if (this_prim && (this_prim->getRootEdit() == root)) {
				return false;
			}
		}
		it++;
	}

	if (!isAllowed(attached_object->getID(), "detach", false)) return false;

	LLInventoryItem* item = getItem(root->getID());
	if (item) {
		LLInventoryCategory* cat_parent = gInventory.getCategory(item->getParentUUID());
		if (cat_parent && !canDetachCategory(cat_parent)) return false;

		if (mHandleNoStrip) {
			std::string name = item->getName();
			LLStringUtil::toLower(name);
			if (name.find("nostrip") != std::string::npos) return false;
		}

		if (isAgentAvatarValid()) {
			std::string attachpt = gAgentAvatarp->getAttachedPointName(item->getLinkedUUID());
			if (contains("detach:" + attachpt)) return false;
			if (contains("remattach")) return false;
			if (contains("remattach:" + attachpt)) return false;
		}
	}
	return true;
}

bool RRInterface::canDetach(std::string attachpt)
{
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	if (mRestoringOutfit) {
		return true;
	}

	LLStringUtil::toLower(attachpt);
	if (contains("detach:" + attachpt)) return false;
	if (contains("remattach")) return false;
	if (contains("remattach:" + attachpt)) return false;
	LLViewerJointAttachment* attachment = findAttachmentPointFromName(attachpt, true);
	if (!canDetachAllObjectsFromAttachment(attachment)) return false;
	return true;
}

bool RRInterface::canAttach(LLViewerObject* object_to_attach, std::string attachpt)
{
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	if (mRestoringOutfit) {
		return true;
	}

	// Attention : this function does not check if we are replacing and there is a locked object already present on the attachment point
	LLStringUtil::toLower(attachpt);
	if (contains("addattach")) return false;
	if (contains("addattach:" + attachpt)) return false;
	if (object_to_attach) {
		LLInventoryItem* item = getItem(object_to_attach->getRootEdit()->getID());
		if (item) {
			LLInventoryCategory* cat_parent = gInventory.getCategory(item->getParentUUID());
			if (cat_parent && !canAttachCategory(cat_parent)) return false;
		}
	}
	return true;
}

bool RRInterface::canAttach(LLViewerInventoryItem* item)
{
	// If mRestoringOutfit == true, the check is done while restoring our
	// outfit after logging in => always allow
	if (mRestoringOutfit) {
		return true;
	}

	if (contains("addattach")) return false;
	if (!item) return true;
	LLViewerJointAttachment* attachpt = findAttachmentPointFromName(item->getName());
	if (attachpt && contains("addattach:" + attachpt->getName())) return false;
	LLInventoryCategory* cat_parent = gInventory.getCategory(item->getParentUUID());
	if (cat_parent && !canAttachCategory(cat_parent)) return false;
	return true;
}

bool RRInterface::canEdit(LLViewerObject* object)
{
	if (!object) return false;
	LLViewerObject* root = object->getRootEdit();
	if (!root) return false;
	if (containsWithoutException("edit", root->getID().asString())) return false;
	if (contains("editobj:" + root->getID().asString())) return false;
	return true;
}

bool RRInterface::canTouch(LLViewerObject* object, LLVector3 pick_intersection /* = LLVector3::zero */)
{
	if (!object) return true;

	LLViewerObject* root = object->getRootEdit();
	if (!root) return true;

	if (!root->isHUDAttachment() && contains("touchall")) return false;

//	if (!root->isHUDAttachment() && contains("touchallnonhud")) return false;

//	if (root->isHUDAttachment() && contains("touchhud")) return false;

	if (contains("touchthis:" + root->getID().asString())) return false;

	if (!isAllowed(root->getID(), "touchme")) return true; // to check the presence of "touchme" on this object, which means that we can touch it

	if (!canTouchFar(object, pick_intersection)) return false;

	if (root->isAttachment()) {
		if (!root->isHUDAttachment()) {
			if (contains("touchattach")) return false;

			LLInventoryItem* inv_item = getItem(root->getID());
			if (inv_item) { // this attachment is in my inv => it belongs to me
				if (contains("touchattachself")) {
					return false;
				}
			}
			else { // this attachment is not in my inv => it does not belong to me
				if (contains("touchattachother")) {
					return false;
				}
			}
		}
	}
	else if (containsWithoutException("touchworld", root->getID().asString())) {
		return false;
	}
	return true;
}

bool RRInterface::canTouchFar(LLViewerObject* object, LLVector3 pick_intersection /* = LLVector3::zero */)
{
	if (!object) return true;
	if (object->isHUDAttachment()) return true;

	LLVector3 pos = object->getPositionRegion();
	if (pick_intersection != LLVector3::zero) pos = pick_intersection;
	pos -= gAgent.getPositionAgent();
	F32 dist = pos.magVec();
	if (mContainsFartouch && dist >= 1.5) return false;

	return true;
}
