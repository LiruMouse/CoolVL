# -*- cmake -*-

project(viewer)

include(00-Common)
include(Boost)
include(DBusGlib)
include(DirectX)
include(ELFIO)
include(FMOD)
include(OPENAL)
include(Hunspell)
include(FindOpenGL)
include(JsonCpp)
include(LLAudio)
include(LLCharacter)
include(LLCommon)
include(LLConvexDecomposition)
include(LLImage)
include(LLImageJ2COJ)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLPlugin)
include(LLPrimitive)
include(LLRender)
include(LLUI)
include(LLVFS)
include(LLWindow)
include(LLXML)
include(LScript)
include(Linking)
include(NDOF)
include(GooglePerfTools)
include(TemplateCheck)
include(UI)
include(LLKDU)
include(ViewerMiscLibs)
include(GLOD)
include(ViewerArtwork.cmake)

if (WINDOWS)
    include(CopyWinLibs)
endif (WINDOWS)

include_directories(
    ${DBUSGLIB_INCLUDE_DIRS}
    ${ELFIO_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
    ${GLOD_INCLUDE_DIR}
    ${HUNSPELL_INCLUDE_DIR}
    ${LLAUDIO_INCLUDE_DIRS}
    ${LLCHARACTER_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCONVEXDECOMP_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLPLUGIN_INCLUDE_DIRS}
    ${LLPRIMITIVE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLUI_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}/lscript_compile
    ${LIBS_PREBUILT_DIR}/include/collada
    ${LIBS_PREBUILT_DIR}/include/collada/1.4
	${HACD_INCLUDE_DIR}
    )

set(viewer_SOURCE_FILES
    llagent.cpp
    llagentaccess.cpp
    llagentdata.cpp
    llagentlanguage.cpp
    llagentpilot.cpp
    llagentwearables.cpp
    llanimstatelabels.cpp
	llappearancemgr.cpp
    llappviewer.cpp
    llassetuploadresponders.cpp
    llassetuploadqueue.cpp
    llaudiosourcevo.cpp
    llbox.cpp
    llcallbacklist.cpp
    llcallingcard.cpp
    llcaphttpsender.cpp
    llchatbar.cpp
    llclassifiedinfo.cpp
    llclassifiedstatsresponder.cpp
    llcloud.cpp
    llcolorscheme.cpp
    llcolorswatch.cpp
    llcommandhandler.cpp
    llcommandlineparser.cpp
    llcompass.cpp
    llcompilequeue.cpp
    llconfirmationmanager.cpp
    llconsole.cpp
    llcontainerview.cpp
    llcurrencyuimanager.cpp
    llcylinder.cpp
    lldebugmessagebox.cpp
    lldebugview.cpp
	lldeformerworker.cpp
    lldelayedgestureerror.cpp
    lldirpicker.cpp
    lldrawable.cpp
    lldrawpoolalpha.cpp
    lldrawpoolavatar.cpp
    lldrawpoolbump.cpp
    lldrawpool.cpp
    lldrawpoolground.cpp
    lldrawpoolsimple.cpp
    lldrawpoolsky.cpp
    lldrawpoolterrain.cpp
    lldrawpooltree.cpp
    lldrawpoolwater.cpp
    lldrawpoolwlsky.cpp
    lldriverparam.cpp
    lldynamictexture.cpp
    llemote.cpp
    lleventinfo.cpp
    lleventnotifier.cpp
    lleventpoll.cpp
    llface.cpp
    llfasttimerview.cpp
    llfeaturemanager.cpp
    llfilepicker.cpp
    llfirstuse.cpp
    llflexibleobject.cpp
    llfloaterabout.cpp
    llfloateractivespeakers.cpp
    llfloateranimpreview.cpp
    jcfloaterareasearch.cpp
    llfloaterauction.cpp
    llfloateravatarinfo.cpp
    llfloateravatarlist.cpp
    llfloateravatarpicker.cpp
    llfloateravatartextures.cpp
    llfloaterbeacons.cpp
    llfloaterbuildoptions.cpp
    llfloaterbulkpermission.cpp
    llfloaterbump.cpp
    llfloaterbuycontents.cpp
    llfloaterbuy.cpp
    llfloaterbuycurrency.cpp
    llfloaterbuyland.cpp
    llfloatercamera.cpp
    llfloaterchat.cpp
    llfloaterchatterbox.cpp
    llfloaterclassified.cpp
    llfloatercolorpicker.cpp
    llfloatercustomize.cpp
    llfloaterdaycycle.cpp
    hbfloaterdebugtags.cpp
    llfloaterdirectory.cpp
    llfloaterdisplayname.cpp
    llfloatereditui.cpp
    llfloaterenvsettings.cpp
    llfloaterevent.cpp
    llfloaterfriends.cpp
    llfloaterfonttest.cpp
    llfloatergesture.cpp
    llfloatergodtools.cpp
    llfloatergroupinfo.cpp
    llfloatergroupinvite.cpp
    llfloatergroups.cpp
    hbfloatergrouptitles.cpp
    llfloaterhandler.cpp
    llfloaterhardwaresettings.cpp
    llfloaterhtmlcurrency.cpp
    llfloatermediabrowser.cpp
    llfloaterhtmlsimple.cpp
    llfloaterhud.cpp
    llfloaterimagepreview.cpp
    llfloaterinspect.cpp
    llfloaterjoystick.cpp
    llfloaterlagmeter.cpp
    llfloaterland.cpp
    llfloaterlandholdings.cpp
    llfloatermakenewoutfit.cpp
    llfloatermap.cpp
    slfloatermediafilter.cpp
    llfloatermemleak.cpp
	llfloatermodelpreview.cpp
	llfloatermodeluploadbase.cpp
    llfloatermute.cpp
    llfloaternamedesc.cpp
    llfloaternewim.cpp
    llfloaternotificationsconsole.cpp
    llfloaterobjectiminfo.cpp
    llfloaterobjectweights.cpp
    llfloateropenobject.cpp
    llfloaterparcel.cpp
    llfloaterpay.cpp
    llfloaterpermissionsmgr.cpp
    llfloaterperms.cpp
    llfloaterpostcard.cpp
    llfloaterpostprocess.cpp
    llfloaterpreference.cpp
    llfloaterproperties.cpp
    llfloaterregiondebugconsole.cpp
    llfloaterregioninfo.cpp
    llfloaterreporter.cpp
    hbfloaterrlv.cpp
    llfloatersearchreplace.cpp
    llfloaterscriptdebug.cpp
    llfloatersellland.cpp
    llfloatersettingsdebug.cpp
    llfloatersnapshot.cpp
    llfloaterstats.cpp
    llfloatertelehub.cpp
    llfloaterteleporthistory.cpp
    llfloaterteleport.cpp
    hbfloatertextinput.cpp
    llfloatertools.cpp
    llfloatertopobjects.cpp
    llfloatertos.cpp
    llfloaterurldisplay.cpp
    llfloaterurlentry.cpp
    llfloatervoicedevicesettings.cpp
    llfloaterwater.cpp
    llfloaterwindlight.cpp
    llfloaterworldmap.cpp
    llfolderview.cpp
    llfollowcam.cpp
    llframestats.cpp
    llframestatview.cpp
    llgesturemgr.cpp
    llglsandbox.cpp
    llgroupmgr.cpp
    llgroupnotify.cpp
    llhomelocationresponder.cpp
    llhoverview.cpp
    llhudeffectbeam.cpp
    llhudeffect.cpp
    llhudeffectlookat.cpp
    llhudeffectpointat.cpp
    llhudeffecttrail.cpp
    llhudicon.cpp
    llhudmanager.cpp
    llhudobject.cpp
    llhudrender.cpp
    llhudtext.cpp
    llhudview.cpp
    llimpanel.cpp
    llimview.cpp
    llinventoryactions.cpp
    llinventorybridge.cpp
    llinventoryclipboard.cpp
    llinventoryicon.cpp
    llinventorymodel.cpp
    llinventorymodelbackgroundfetch.cpp
    llinventoryview.cpp
    lljoystickbutton.cpp
    lllandmarklist.cpp
    lllocaltextureobject.cpp
    lllogchat.cpp
    llloginhandler.cpp
    llsavedlogins.cpp
    llmanip.cpp
    llmaniprotate.cpp
    llmanipscale.cpp
    llmaniptranslate.cpp
    llmarketplacefunctions.cpp
    llmediactrl.cpp
    llmediaremotectrl.cpp
    llmemoryview.cpp
    llmenucommands.cpp
    llmeshrepository.cpp
    llmimetypes.cpp
    llmorphview.cpp
    llmoveview.cpp
    llmutelist.cpp
    llnamebox.cpp
    llnameeditor.cpp
    llnamelistctrl.cpp
    llnetmap.cpp
    llnotify.cpp
    lloverlaybar.cpp
    llpanelaudiovolume.cpp
    llpanelavatar.cpp
    llpanelclassified.cpp
    llpanelcontents.cpp
    llpaneldirbrowser.cpp
    llpaneldirclassified.cpp
    llpaneldirevents.cpp
    llpaneldirfind.cpp
    llpaneldirgroups.cpp
    llpaneldirland.cpp
    llpaneldirpeople.cpp
    llpaneldirplaces.cpp
    llpaneldisplay.cpp
	llpaneleditwearable.cpp
    llpanelevent.cpp
    llpanelface.cpp
    hbpanelgrids.cpp
    llpanelgroup.cpp
    llpanelgroupgeneral.cpp
    llpanelgroupinvite.cpp
    llpanelgrouplandmoney.cpp
    llpanelgroupnotices.cpp
    llpanelgrouproles.cpp
    llpanelgroupvoting.cpp
    llpanelinventory.cpp
    llpanelland.cpp
    llpanellandaudio.cpp
    llpanellandmedia.cpp
    llpanellandoptions.cpp
    llpanellogin.cpp
    llpanelmediahud.cpp
    llpanelobject.cpp
    llpanelpermissions.cpp
    llpanelpick.cpp
    llpanelplace.cpp
    llpanelskins.cpp
    llpanelvolume.cpp
    llparcelselection.cpp
    llpatchvertexarray.cpp
	llphysicsmotion.cpp
	llphysicsshapebuilderutil.cpp
    llpolymesh.cpp
    llpolymorph.cpp
    llprefschat.cpp
    hbprefscool.cpp
    llprefsgeneral.cpp
    llprefsim.cpp
    llprefsinput.cpp
    llprefsmedia.cpp
    llprefsnetwork.cpp
    llprefsnotifications.cpp
    llprefsvoice.cpp
    llpreviewanim.cpp
    llpreview.cpp
    llpreviewgesture.cpp
    llpreviewlandmark.cpp
    llpreviewnotecard.cpp
    llpreviewscript.cpp
    llpreviewsound.cpp
    llpreviewtexture.cpp
    llproductinforequest.cpp
    llprogressview.cpp
    llregionposition.cpp
    llremoteparcelrequest.cpp
    llsavedsettingsglue.cpp
	llscrollingpanelparam.cpp
    llselectmgr.cpp
    llsky.cpp
    llspatialpartition.cpp
    llsprite.cpp
    llsrv.cpp
    llstartup.cpp
    llstatbar.cpp
    llstatgraph.cpp
    llstatusbar.cpp
    llstatview.cpp
    llstylemap.cpp
    llsurface.cpp
    llsurfacepatch.cpp
    lltexglobalcolor.cpp
    lltexlayer.cpp
    lltexlayerparams.cpp
    lltexturecache.cpp
    lltexturectrl.cpp
    lltexturefetch.cpp
    lltextureinfo.cpp
    lltextureinfodetails.cpp
    lltexturestats.cpp
    lltexturestatsuploader.cpp
    lltextureview.cpp
    qltoolalign.cpp
    lltoolbar.cpp
    lltoolbrush.cpp
    lltoolcomp.cpp
    lltool.cpp
    lltooldraganddrop.cpp
    lltoolface.cpp
    lltoolfocus.cpp
    lltoolgrab.cpp
    lltoolgun.cpp
    lltoolindividual.cpp
    lltoolmgr.cpp
    lltoolmorph.cpp
    lltoolobjpicker.cpp
    lltoolpie.cpp
    lltoolpipette.cpp
    lltoolplacer.cpp
    lltoolselect.cpp
    lltoolselectland.cpp
    lltoolselectrect.cpp
    lltoolview.cpp
    lltracker.cpp
    lltrans.cpp
    lltranslate.cpp
    lluploaddialog.cpp
	lluploadfloaterobservers.cpp
    llurl.cpp
    llurldispatcher.cpp
    llurlhistory.cpp
    llurlsimstring.cpp
    llurlwhitelist.cpp
    lluserauth.cpp
    llvectorperfoptions.cpp
    llvelocitybar.cpp
    llviewchildren.cpp
    llviewerassetstorage.cpp
    llviewerassettype.cpp
    llvieweraudio.cpp
    llviewercamera.cpp
    llviewercontrol.cpp
    llviewerdisplay.cpp
    llviewerdisplayname.cpp
	llviewerfoldertype.cpp
    llviewergenericmessage.cpp
    llviewergesture.cpp
    llviewerinventory.cpp
    llviewerjointattachment.cpp
    llviewerjoint.cpp
    llviewerjointmesh.cpp
    llviewerjoystick.cpp
    llviewerkeyboard.cpp
    llviewerlayer.cpp
    llviewermedia.cpp
    llviewermediafocus.cpp
    llviewermedia_streamingaudio.cpp
    llviewermenu.cpp
    llviewermenufile.cpp
    llviewermessage.cpp
    llviewernetwork.cpp
    llviewerobject.cpp
    llviewerobjectbackup.cpp
    llviewerobjectlist.cpp
    llviewerparcelmedia.cpp
    llviewerparcelmediaautoplay.cpp
    llviewerparcelmgr.cpp
    llviewerparceloverlay.cpp
    llviewerpartsim.cpp
    llviewerpartsource.cpp
    llviewerregion.cpp
    llviewershadermgr.cpp
    llviewerstats.cpp
    llviewertexteditor.cpp
    llviewertextureanim.cpp
    llviewertexture.cpp
    llviewertexturelist.cpp
    llviewerthrottle.cpp
    llviewervisualparam.cpp
    llviewerwindow.cpp
    llvlcomposition.cpp
    llvlmanager.cpp
    llvoavatar.cpp
    llvoavatardefines.cpp
    llvoavatarself.cpp
    llvocache.cpp
    llvoclouds.cpp
    llvograss.cpp
    llvoground.cpp
    llvoiceclient.cpp
    llvoiceremotectrl.cpp
    llvoicevisualizer.cpp
    llvoinventorylistener.cpp
    llvopartgroup.cpp
    llvosky.cpp
    llvosurfacepatch.cpp
    llvotextbubble.cpp
    llvotree.cpp
    llvovolume.cpp
    llvowater.cpp
    llvowlsky.cpp
    llwatchdog.cpp
    llwaterparammanager.cpp
    llwaterparamset.cpp
    llwearable.cpp
    llwearablelist.cpp
    llwearabletype.cpp
    llweb.cpp
    llwind.cpp
    llwlanimator.cpp
    llwldaycycle.cpp
    llwlparammanager.cpp
    llwlparamset.cpp
    llworld.cpp
    llworldmap.cpp
    llmapresponders.cpp
    llworldmapview.cpp
    llxmlrpctransaction.cpp
    noise.cpp
    pipeline.cpp
    RRInterface.cpp
    )

# This gets renamed in the packaging step
set(VIEWER_BINARY_NAME "secondlife-bin" CACHE STRING
    "The name of the viewer executable to create.")

set(viewer_HEADER_FILES
    CMakeLists.txt
    ViewerInstall.cmake

    llagent.h
    llagentaccess.h
    llagentdata.h
    llagentlanguage.h
    llagentpilot.h
    llagentwearables.h
    llanimstatelabels.h
    llappearance.h
	llappearancemgr.h
    llappviewer.h
    llassetuploadresponders.h
    llassetuploadqueue.h
    llaudiosourcevo.h
    llbox.h
    llcallbacklist.h
    llcallingcard.h
    llcaphttpsender.h
    llchatbar.h
    llclassifiedinfo.h
    llclassifiedstatsresponder.h
    llcloud.h
    llcolorscheme.h
    llcolorswatch.h
    llcommandhandler.h
    llcommandlineparser.h
    llcompass.h
    llcompilequeue.h
    llconfirmationmanager.h
    llconsole.h
    llcontainerview.h
    llcurrencyuimanager.h
    llcylinder.h
    lldebugmessagebox.h
    lldebugview.h
	lldeformerworker.h
    lldelayedgestureerror.h
    lldirpicker.h
    lldrawable.h
    lldrawpool.h
    lldrawpoolalpha.h
    lldrawpoolavatar.h
    lldrawpoolbump.h
    lldrawpoolclouds.h
    lldrawpoolground.h
    lldrawpoolsimple.h
    lldrawpoolsky.h
    lldrawpoolterrain.h
    lldrawpooltree.h
    lldrawpoolwater.h
    lldrawpoolwlsky.h
    lldriverparam.h
    lldynamictexture.h
    llemote.h
    lleventinfo.h
    lleventnotifier.h
    lleventpoll.h
    llface.h
    llfasttimerview.h
    llfeaturemanager.h
    llfilepicker.h
    llfirstuse.h
    llflexibleobject.h
    llfloaterabout.h
    llfloateractivespeakers.h
    llfloateranimpreview.h
    jcfloaterareasearch.h
    llfloaterauction.h
    llfloateravatarinfo.h
    llfloateravatarlist.h
    llfloateravatarpicker.h
    llfloateravatartextures.h
    llfloaterbeacons.h
    llfloaterbuildoptions.h
    llfloaterbulkpermission.h
    llfloaterbump.h
    llfloaterbuy.h
    llfloaterbuycontents.h
    llfloaterbuycurrency.h
    llfloaterbuyland.h
    llfloatercamera.h
    llfloaterchat.h
    llfloaterchatterbox.h
    llfloaterclassified.h
    llfloatercolorpicker.h
    llfloatercustomize.h
    llfloaterdaycycle.h
    hbfloaterdebugtags.h
    llfloaterdirectory.h
    llfloaterdisplayname.h
    llfloatereditui.h
    llfloaterenvsettings.h
    llfloaterevent.h
    llfloaterfonttest.h
    llfloaterfriends.h
    llfloatergesture.h
    llfloatergodtools.h
    llfloatergroupinfo.h
    llfloatergroupinvite.h
    llfloatergroups.h
    hbfloatergrouptitles.h
    llfloaterhandler.h
    llfloaterhardwaresettings.h
    llfloaterhtmlcurrency.h
    llfloatermediabrowser.h
    llfloaterhtmlsimple.h
    llfloaterhud.h
    llfloaterimagepreview.h
    llfloaterinspect.h
    llfloaterjoystick.h
    llfloaterlagmeter.h
    llfloaterland.h
    llfloaterlandholdings.h
    llfloatermakenewoutfit.h
    llfloatermap.h
    slfloatermediafilter.h
    llfloatermemleak.h
	llfloatermodelpreview.h
	llfloatermodeluploadbase.h
    llfloatermute.h
    llfloaternamedesc.h
    llfloaternewim.h
    llfloaternotificationsconsole.h
    llfloaterobjectiminfo.h
    llfloaterobjectweights.h
    llfloateropenobject.h
    llfloaterparcel.h
    llfloaterpay.h
    llfloaterpermissionsmgr.h
    llfloaterpostcard.h
    llfloaterpostprocess.h
    llfloaterpreference.h
    llfloaterperms.h
    llfloaterproperties.h
    llfloaterregiondebugconsole.h
    llfloaterregioninfo.h
    llfloaterreporter.h
    hbfloaterrlv.h
    llfloatersearchreplace.h
    llfloaterscriptdebug.h
    llfloatersellland.h
    llfloatersettingsdebug.h
    llfloatersnapshot.h
    llfloaterstats.h
    llfloatertelehub.h
    llfloaterteleporthistory.h
    llfloaterteleport.h
    hbfloatertextinput.h
    llfloatertools.h
    llfloatertopobjects.h
    llfloatertos.h
    llfloaterurldisplay.h
    llfloaterurlentry.h
    llfloatervoicedevicesettings.h
    llfloaterwater.h
    llfloaterwindlight.h
    llfloaterworldmap.h
    llfolderview.h
    llfollowcam.h
    llframestats.h
    llframestatview.h
    llgesturemgr.h
    llgroupmgr.h
    llgroupnotify.h
    llhomelocationresponder.h
    llhoverview.h
    llhudeffect.h
    llhudeffectbeam.h
    llhudeffectlookat.h
    llhudeffectpointat.h
    llhudeffecttrail.h
    llhudicon.h
    llhudmanager.h
    llhudobject.h
    llhudrender.h
    llhudtext.h
    llhudview.h
    llimpanel.h
    llimview.h
    llinventorybridge.h
    llinventoryclipboard.h
    llinventoryicon.h
    llinventorymodel.h
    llinventorymodelbackgroundfetch.h
    llinventoryview.h
    lljoystickbutton.h
    lllandmarklist.h
    lllightconstants.h
    lllocaltextureobject.h
    lllogchat.h
    llloginhandler.h
    llsavedlogins.h
    llmanip.h
    llmaniprotate.h
    llmanipscale.h
    llmaniptranslate.h
    llmarketplacefunctions.h
    llmediactrl.h
    llmediaremotectrl.h
    llmemoryview.h
    llmenucommands.h
    llmeshrepository.h
    llmimetypes.h
    llmorphview.h
    llmoveview.h
    llmutelist.h
    llnamebox.h
    llnameeditor.h
    llnamelistctrl.h
    llnetmap.h
    llnotify.h
    lloverlaybar.h
    llpanelaudiovolume.h
    llpanelavatar.h
    llpanelclassified.h
    llpanelcontents.h
    llpaneldirbrowser.h
    llpaneldirclassified.h
    llpaneldirevents.h
    llpaneldirfind.h
    llpaneldirgroups.h
    llpaneldirland.h
    llpaneldirpeople.h
    llpaneldirplaces.h
    llpaneldisplay.h
	llpaneleditwearable.h
    llpanelevent.h
    llpanelface.h
    hbpanelgrids.h
    llpanelgroup.h
    llpanelgroupgeneral.h
    llpanelgroupinvite.h
    llpanelgrouplandmoney.h
    llpanelgroupnotices.h
    llpanelgrouproles.h
    llpanelgroupvoting.h
    llpanelinventory.h
    llpanelland.h
    llpanellandaudio.h
    llpanellandmedia.h
    llpanellandoptions.h
    llpanelLCD.h
    llpanellogin.h
    llpanelmediahud.h
    llpanelobject.h
    llpanelpermissions.h
    llpanelpick.h
    llpanelplace.h
    llpanelskins.h
    llpanelvolume.h
    llparcelselection.h
    llpatchvertexarray.h
	llphysicsmotion.h
	llphysicsshapebuilderutil.h
    llpolymesh.h
    llpolymorph.h
    llprefschat.h
    hbprefscool.h
    llprefsgeneral.h
    llprefsim.h
    llprefsinput.h
    llprefsmedia.h
    llprefsnetwork.h
    llprefsnotifications.h
    llprefsvoice.h
    llpreview.h
    llpreviewanim.h
    llpreviewgesture.h
    llpreviewlandmark.h
    llpreviewnotecard.h
    llpreviewscript.h
    llpreviewsound.h
    llpreviewtexture.h
    llproductinforequest.h
    llprogressview.h
    llregionposition.h
    llremoteparcelrequest.h
    llresourcedata.h
    llsavedsettingsglue.h
	llscrollingpanelparam.h
    llselectmgr.h
    llsky.h
    llspatialpartition.h
    llsprite.h
    llsrv.h
    llstartup.h
    llstatbar.h
    llstatgraph.h
    llstatusbar.h
    llstatview.h
    llstylemap.h
    llsurface.h
    llsurfacepatch.h
    lltable.h
    lltexglobalcolor.h
    lltexlayer.h
    lltexlayerparams.h
    lltexturecache.h
    lltexturectrl.h
    lltexturefetch.h
    lltextureinfo.h
    lltextureinfodetails.h
    lltexturestats.h
    lltexturestatsuploader.h
    lltextureview.h
    lltool.h
    qltoolalign.h
    lltoolbar.h
    lltoolbrush.h
    lltoolcomp.h
    lltooldraganddrop.h
    lltoolface.h
    lltoolfocus.h
    lltoolgrab.h
    lltoolgun.h
    lltoolindividual.h
    lltoolmgr.h
    lltoolmorph.h
    lltoolobjpicker.h
    lltoolpie.h
    lltoolpipette.h
    lltoolplacer.h
    lltoolselect.h
    lltoolselectland.h
    lltoolselectrect.h
    lltoolview.h
    lltracker.h
    lltrans.h
    lltranslate.h
    lluiconstants.h
    lluploaddialog.h
	lluploadfloaterobservers.h
    llurl.h
    llurldispatcher.h
    llurlhistory.h
    llurlsimstring.h
    llurlwhitelist.h
    lluserauth.h
    llvectorperfoptions.h
    llvelocitybar.h
    llviewchildren.h
    llviewerassetstorage.h
    llviewerassettype.h
    llvieweraudio.h
    llviewercamera.h
    llviewercontrol.h
    llviewerdisplay.h
    llviewerdisplayname.h
	llviewerfoldertype.h
    llviewergenericmessage.h
    llviewergesture.h
    llviewerinventory.h
    llviewerjoint.h
    llviewerjointattachment.h
    llviewerjointmesh.h
    llviewerjoystick.h
    llviewerkeyboard.h
    llviewerlayer.h
    llviewermedia.h
    llviewermediaobserver.h
    llviewermediafocus.h
    llviewermenu.h
    llviewermenufile.h
    llviewermessage.h
    llviewernetwork.h
    llviewerobject.h
    llviewerobjectbackup.h
    llviewerobjectlist.h
    llviewerparcelmedia.h
    llviewerparcelmediaautoplay.h
    llviewerparcelmgr.h
    llviewerparceloverlay.h
    llviewerpartsim.h
    llviewerpartsource.h
    llviewerprecompiledheaders.h
    llviewerregion.h
    llviewershadermgr.h
    llviewerstats.h
    llviewertexteditor.h
    llviewertextureanim.h
    llviewertexture.h
    llviewertexturelist.h
    llviewerthrottle.h
    llviewervisualparam.h
    llviewerwindow.h
    llvlcomposition.h
    llvlmanager.h
    llvoavatar.h
    llvoavatardefines.h
    llvoavatarself.h
    llvocache.h
    llvoclouds.h
    llvograss.h
    llvoground.h
    llvoiceclient.h
    llvoiceremotectrl.h
    llvoicevisualizer.h
    llvoinventorylistener.h
    llvopartgroup.h
    llvosky.h
    llvosurfacepatch.h
    llvotextbubble.h
    llvotree.h
    llvotreenew.h
    llvovolume.h
    llvowater.h
    llvowlsky.h
    llwatchdog.h
    llwaterparammanager.h
    llwaterparamset.h
    llwearable.h
    llwearablelist.h
    llwearabletype.h
    llweb.h
    llwind.h
    llwindebug.h
    llwlanimator.h
    llwldaycycle.h
    llwlparammanager.h
    llwlparamset.h
    llworld.h
    llworldmap.h
    llmapresponders.h
    llworldmapview.h
    llxmlrpctransaction.h
    macmain.h
    noise.h
    pipeline.h
    randgauss.h
    RRInterface.h
    VertexCache.h
    VorbisFramework.h
    )

source_group("CMake Rules" FILES ViewerInstall.cmake)

if (DARWIN)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx.cpp)

  find_library(AGL_LIBRARY AGL)
  find_library(APPKIT_LIBRARY AppKit)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(IOKIT_LIBRARY IOKit)

  set(viewer_LIBRARIES
    ${COCOA_LIBRARY}
    ${AGL_LIBRARY}
    ${IOKIT_LIBRARY}
    )

  # Add resource files to the project.
  set(viewer_RESOURCE_FILES
    ${VIEWER_BRANDING_ID}.icns
    macview.r
    gpu_table.txt
    Info-${VIEWER_BRANDING_NAME_CAMELCASE}.plist
    SecondLife.nib/
    # CMake doesn't seem to support Xcode language variants well just yet
    English.lproj/InfoPlist.strings
    English.lproj/language.txt
    German.lproj/language.txt
    Japanese.lproj/language.txt
    Korean.lproj/language.txt
    )
  set_source_files_properties(
    ${viewer_RESOURCE_FILES}
    PROPERTIES
    HEADER_FILE_ONLY TRUE
    #MACOSX_PACKAGE_LOCATION Resources #don't do this! this tells cmake to copy the files.
    )
  SOURCE_GROUP("Resources" FILES ${viewer_RESOURCE_FILES})
  list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})
endif (DARWIN)

if (LINUX)
    LIST(APPEND viewer_SOURCE_FILES llappviewerlinux.cpp)
    LIST(APPEND viewer_SOURCE_FILES llappviewerlinux_api_dbus.cpp)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")

    set(viewer_LIBRARIES
        Xinerama
        )
endif (LINUX)

if (WINDOWS)
    list(APPEND viewer_SOURCE_FILES
         llappviewerwin32.cpp
         llwindebug.cpp
         )

    list(APPEND viewer_HEADER_FILES
         llappviewerwin32.h
         llwindebug.h
         )

    # precompiled header configuration
    # llviewerprecompiledheaders.cpp generates
    # the .pch file.
    # All sources added to viewer_SOURCE_FILES 
    # at this point use it.
    set_source_files_properties(llviewerprecompiledheaders.cpp
        PROPERTIES
        COMPILE_FLAGS "/Ycllviewerprecompiledheaders.h"
        )        
    foreach( src_file ${viewer_SOURCE_FILES} )
        set_source_files_properties(
            ${src_file}
            PROPERTIES
            COMPILE_FLAGS "/Yullviewerprecompiledheaders.h"
            )        
    endforeach( src_file ${viewer_SOURCE_FILES} )
    list(APPEND viewer_SOURCE_FILES llviewerprecompiledheaders.cpp)
    
    # Add resource files to the project.
    # viewerRes.rc is the only buildable file, but
    # the rest are all dependencies of it.
    set(viewer_RESOURCE_FILES
        ${ARTWORK_DIR}/res/arrow.cur
        ${ARTWORK_DIR}/res/arrowcop.cur
        ${ARTWORK_DIR}/res/arrowcopmulti.cur
        ${ARTWORK_DIR}/res/arrowdrag.cur
        ${ARTWORK_DIR}/res/circleandline.cur
        ${ARTWORK_DIR}/res/icon1.ico
        ${ARTWORK_DIR}/res/llarrow.cur
        ${ARTWORK_DIR}/res/llarrowdrag.cur
        ${ARTWORK_DIR}/res/llarrowdragmulti.cur
        ${ARTWORK_DIR}/res/llarrowlocked.cur
        ${ARTWORK_DIR}/res/llgrablocked.cur
        ${ARTWORK_DIR}/res/llno.cur
        ${ARTWORK_DIR}/res/llnolocked.cur
        ${ARTWORK_DIR}/res/lltoolcamera.cur
        ${ARTWORK_DIR}/res/lltoolcreate.cur
        ${ARTWORK_DIR}/res/lltoolfocus.cur
        ${ARTWORK_DIR}/res/lltoolgrab.cur
        ${ARTWORK_DIR}/res/lltoolland.cur
        ${ARTWORK_DIR}/res/lltoolpan.cur
        ${ARTWORK_DIR}/res/lltoolpipette.cur
        ${ARTWORK_DIR}/res/lltoolrotate.cur
        ${ARTWORK_DIR}/res/lltoolscale.cur
        ${ARTWORK_DIR}/res/lltooltranslate.cur
        ${ARTWORK_DIR}/res/lltoolzoomin.cur
        ${ARTWORK_DIR}/res/lltoolzoomout.cur
        ${ARTWORK_DIR}/res/ll_icon.BMP
        ${ARTWORK_DIR}/res/ll_icon.ico
        ${ARTWORK_DIR}/res/resource.h
        ${ARTWORK_DIR}/res/toolbuy.cur
        ${ARTWORK_DIR}/res/toolopen.cur
        ${ARTWORK_DIR}/res/toolpay.cur
        ${ARTWORK_DIR}/res/toolpickobject.cur
        ${ARTWORK_DIR}/res/toolpickobject2.cur
        ${ARTWORK_DIR}/res/toolpickobject3.cur
        ${ARTWORK_DIR}/res/toolpipette.cur
        ${ARTWORK_DIR}/res/toolsit.cur
        )

    set_source_files_properties(${viewer_RESOURCE_FILES}
                                PROPERTIES HEADER_FILE_ONLY TRUE)

    if(NOT ARTWORK_IN_SOURCE)
      # The resource compiler needs to find resources here.
      include_directories(${ARTWORK_DIR}/res)
    endif(NOT ARTWORK_IN_SOURCE)

    set(viewer_RESOURCE_FILES 
        res/resource.h
        res/viewerRes.rc 
        ${viewer_RESOURCE_FILES}
        )

    SOURCE_GROUP("Resource Files" FILES ${viewer_RESOURCE_FILES})

    list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})

    find_library(DINPUT_LIBRARY dinput8 ${DIRECTX_LIBRARY_DIR})
    find_library(DXGUID_LIBRARY dxguid ${DIRECTX_LIBRARY_DIR})
    mark_as_advanced(
        DINPUT_LIBRARY
        DXGUID_LIBRARY
        )

    set(viewer_LIBRARIES
        advapi32
        comdlg32
        ${DINPUT_LIBRARY}
        ${DXGUID_LIBRARY}
        fmodvc
        gdi32
        kernel32
        odbc32
        odbccp32
        ole32
        oleaut32
        opengl32
        shell32
        user32
        Vfw32
        winspool
        )

    find_library(INTEL_MEMOPS_LIBRARY
                 NAMES ll_intel_memops
                 PATHS
                 optimized ${ARCH_PREBUILT_DIRS_RELEASE}
                 debug ${ARCH_PREBUILT_DIRS_DEBUG}
                 )
    mark_as_advanced(INTEL_MEMOPS_LIBRARY)

    if (INTEL_MEMOPS_LIBRARY)
      list(APPEND viewer_LIBRARIES ${INTEL_MEMOPS_LIBRARY})
    endif (INTEL_MEMOPS_LIBRARY)

    use_prebuilt_binary(dbghelp)
endif (WINDOWS)

# Add the xui files. This is handy for searching for xui elements
# from within the IDE.

file(GLOB viewer_XUI_FILE_GLOB_LIST
     ${CMAKE_CURRENT_SOURCE_DIR}/skins/default/xui/en-us/*.xml)

set(viewer_XUI_FILES

    ${CMAKE_CURRENT_SOURCE_DIR}/skins/default/colors.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/skins/default/colors_base.xml



    )

list(APPEND viewer_XUI_FILES ${viewer_XUI_FILE_GLOB_LIST})
list(SORT viewer_XUI_FILES)

source_group("XUI Files" FILES ${viewer_XUI_FILES})

set_source_files_properties(${viewer_XUI_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_XUI_FILES})

set(viewer_APPSETTINGS_FILES
    app_settings/anim.ini
    app_settings/cmd_line.xml
    app_settings/grass.xml
    app_settings/grids.xml
    app_settings/high_graphics.xml
    app_settings/keys.ini
    app_settings/keywords.ini
    app_settings/logcontrol.xml
    app_settings/low_graphics.xml
    app_settings/mid_graphics.xml
    app_settings/mime_types_linux.xml
    app_settings/mime_types_mac.xml
    app_settings/mime_types_windows.xml
    app_settings/settings.xml
    app_settings/settings_crash_behavior.xml
    app_settings/settings_files.xml
    app_settings/settings_per_account.xml
    app_settings/std_bump.ini
    app_settings/trees.xml
    app_settings/ultra_graphics.xml
    app_settings/viewerart.xml
    ${CMAKE_SOURCE_DIR}/../etc/message.xml
    ${CMAKE_SOURCE_DIR}/../scripts/messages/message_template.msg
    )

use_prebuilt_binary(artwork-common)
use_prebuilt_binary(dictionaries)

source_group("App Settings" FILES ${viewer_APPSETTINGS_FILES})

set_source_files_properties(${viewer_APPSETTINGS_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_APPSETTINGS_FILES})

if(ARTWORK_EXISTS)
  set(viewer_CHARACTER_FILES
    ${ARTWORK_DIR}/character/attentions.xml
    ${ARTWORK_DIR}/character/attentionsN.xml
    ${ARTWORK_DIR}/character/avatar_lad.xml
    ${ARTWORK_DIR}/character/avatar_skeleton.xml
    ${ARTWORK_DIR}/character/genepool.xml
    )

  source_group("Character File" FILES ${viewer_CHARACTER_FILES})

  set_source_files_properties(${viewer_CHARACTER_FILES}
                              PROPERTIES HEADER_FILE_ONLY TRUE)

  list(APPEND viewer_SOURCE_FILES ${viewer_CHARACTER_FILES})
endif(ARTWORK_EXISTS)

if (WINDOWS)
  file(GLOB viewer_INSTALLER_FILES installers/windows/*.nsi)

  source_group("Installer Files" FILES ${viewer_INSTALLER_FILES})

  set_source_files_properties(${viewer_INSTALLER_FILES}
                              PROPERTIES HEADER_FILE_ONLY TRUE)

  list(APPEND viewer_SOURCE_FILES ${viewer_INSTALLER_FILES})
endif (WINDOWS)

if (OPENAL)
  set(LLSTARTUP_COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS} -DLL_OPENAL")
endif (OPENAL)

if (FMOD)
  set(LLSTARTUP_COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS} -DLL_FMOD")

  if (NOT WINDOWS)
    set(fmodwrapper_SOURCE_FILES fmodwrapper.cpp)
    add_library(fmodwrapper SHARED ${fmodwrapper_SOURCE_FILES})
    set(fmodwrapper_needed_LIBRARIES ${FMOD_LIBRARY})
    if (DARWIN)
      list(APPEND fmodwrapper_needed_LIBRARIES ${CARBON_LIBRARY})
      set_target_properties(
        fmodwrapper
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_NAME_DIR "@executable_path/../Resources"
        LINK_FLAGS "-unexported_symbols_list \"${CMAKE_CURRENT_SOURCE_DIR}/fmod_hidden_symbols.exp\""
        )
    endif (DARWIN)
    set(FMODWRAPPER_LIBRARY fmodwrapper)
    target_link_libraries(fmodwrapper ${fmodwrapper_needed_LIBRARIES})
  endif (NOT WINDOWS)
endif (FMOD)

set_source_files_properties(llstartup.cpp PROPERTIES COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS}")

list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

set_source_files_properties(${viewer_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(${VIEWER_BINARY_NAME}
    WIN32
    MACOSX_BUNDLE
    ${viewer_SOURCE_FILES}
    )
check_message_template(${VIEWER_BINARY_NAME})

if (LLKDU_LIBRARY)
  add_dependencies(${VIEWER_BINARY_NAME} ${LLKDU_LIBRARY})
endif (LLKDU_LIBRARY)

# We package by default on Linux so we can run from newview/packaged.
if (LINUX)
  set(PACKAGE_DEFAULT ON)
else (LINUX)
  set(PACKAGE_DEFAULT OFF)
endif (LINUX)
set(PACKAGE ${PACKAGE_DEFAULT} CACHE BOOL
    "Add a package target that builds an installer package.")

if (WINDOWS)
    if(MSVC71)
        set(release_flags "/MAP:Release/${VIEWER_BINARY_NAME}.map /MAPINFO:LINES")
    else(MSVC71)
        set(release_flags "/MAP:Release/${VIEWER_BINARY_NAME}.map")
    endif(MSVC71)
    
    set_target_properties(${VIEWER_BINARY_NAME}
        PROPERTIES
        LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:WINDOWS ${TCMALLOC_LINK_FLAGS}" 
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\" /INCREMENTAL:NO"
        LINK_FLAGS_RELEASE ${release_flags}
        )

    # sets the 'working directory' for debugging from visual studio.
    if (NOT UNATTENDED)
        add_custom_command(
            TARGET ${VIEWER_BINARY_NAME} PRE_BUILD
            COMMAND ${CMAKE_SOURCE_DIR}/tools/vstool/vstool.exe
            ARGS
              --solution 
              ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.sln
              --workingdir
              ${VIEWER_BINARY_NAME}
              ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Setting the ${VIEWER_BINARY_NAME} working directory for debugging."
            )
    endif (NOT UNATTENDED)

    add_custom_command(
        TARGET ${VIEWER_BINARY_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
          -E
          copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/messages/message_template.msg
          ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/app_settings/message_template.msg
        COMMENT "Copying message_template.msg to the runtime folder."
        )

    add_custom_command(
        TARGET ${VIEWER_BINARY_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
          -E
          copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/../../etc/message.xml
          ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/app_settings/message.xml
        COMMENT "Copying message.xml to the runtime folder."
        )
       
    add_dependencies(${VIEWER_BINARY_NAME} copy_win_libs)

    if (EXISTS ${CMAKE_SOURCE_DIR}/copy_win_scripts)
      add_dependencies(${VIEWER_BINARY_NAME} copy_win_scripts)
    endif (EXISTS ${CMAKE_SOURCE_DIR}/copy_win_scripts)
        
    add_custom_command(
        OUTPUT ${CMAKE_CFG_INTDIR}/touched.bat
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
          --configuration=${CMAKE_CFG_INTDIR}
          --channel=${VIEWER_CHANNEL}
          --login_channel=${VIEWER_LOGIN_CHANNEL}
          --branding_id=${VIEWER_BRANDING_ID}
          --grid=${GRID}
          --source=${CMAKE_CURRENT_SOURCE_DIR}
              --artwork=${ARTWORK_DIR}
              --build=${CMAKE_CURRENT_BINARY_DIR}
              --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
              --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/touched.bat
        DEPENDS ${VIEWER_BINARY_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        )

    add_dependencies(${VIEWER_BINARY_NAME} SLPlugin media_plugin_quicktime media_plugin_webkit)

    if (PACKAGE)
      add_custom_target(package ALL DEPENDS ${CMAKE_CFG_INTDIR}/touched.bat)
      add_dependencies(package windows-crash-logger)
    endif (PACKAGE)
endif (WINDOWS)

target_link_libraries(${VIEWER_BINARY_NAME}
    ${GOOGLE_PERFTOOLS_LIBRARIES}
    ${LLAUDIO_LIBRARIES}
    ${LLCHARACTER_LIBRARIES}
    ${LLIMAGE_LIBRARIES}
    ${LLIMAGEJ2COJ_LIBRARIES}
    ${LLINVENTORY_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLPLUGIN_LIBRARIES}
    ${LLPRIMITIVE_LIBRARIES}
    ${LLRENDER_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${LLUI_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLWINDOW_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LSCRIPT_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${NDOF_LIBRARY}
    ${viewer_LIBRARIES}
    ${BOOST_PROGRAM_OPTIONS_LIBRARY}
    ${BOOST_REGEX_LIBRARY}
    ${DBUSGLIB_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${FMODWRAPPER_LIBRARY}
    ${GLOD_LIBRARIES}
    ${HUNSPELL_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${SDL_LIBRARY}
    ${SMARTHEAP_LIBRARY}
    ${UI_LIBRARIES}
    ${WINDOWS_LIBRARIES}
    ${XMLRPCEPI_LIBRARIES}
    ${ELFIO_LIBRARIES}
    ${LLCONVEXDECOMP_LIBRARY}
    ${TCMALLOC_LIBRARIES}
    ${HACD_LIBRARY}
    )

if (LINUX)
  find_program(EUSTRIP eu-strip)
  if (EUSTRIP)
    add_custom_command(
        OUTPUT secondlife-stripped
        COMMAND eu-strip
        ARGS --strip-debug --remove-comment -o secondlife-stripped -f ${VIEWER_BINARY_NAME}.debug ${VIEWER_BINARY_NAME}
        DEPENDS ${VIEWER_BINARY_NAME}
        COMMENT "Creating the stripped binary and the debug symbols files."
        )
  else (EUSTRIP)
    add_custom_command(
        OUTPUT secondlife-stripped
        COMMAND strip
        ARGS --strip-debug -o secondlife-stripped ${VIEWER_BINARY_NAME}
        DEPENDS ${VIEWER_BINARY_NAME}
        COMMENT "Creating the stripped binary."
        )
  endif (EUSTRIP)

  set(product ${VIEWER_BRANDING_NAME_CAMELCASE}-${ARCH}-${viewer_VERSION})

  add_custom_command(
      OUTPUT ${product}.tar.bz2
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        --grid=${GRID}
        --channel=${VIEWER_CHANNEL}
        --login_channel=${VIEWER_LOGIN_CHANNEL}
        --branding_id=${VIEWER_BRANDING_ID}
        --installer_name=${product}
        --arch=${ARCH}
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --artwork=${ARTWORK_DIR}
        --build=${CMAKE_CURRENT_BINARY_DIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/packaged
        --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
        --buildtype=${CMAKE_BUILD_TYPE}
      DEPENDS secondlife-stripped ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      )

  add_dependencies(${VIEWER_BINARY_NAME} SLPlugin media_plugin_gstreamer010 media_plugin_webkit)

  if (PACKAGE)
    add_custom_target(package ALL DEPENDS ${product}.tar.bz2)
    add_dependencies(package linux-crash-logger-strip-target)
  endif (PACKAGE)
endif (LINUX)

if (DARWIN)
  set(product ${VIEWER_BRANDING_NAME})
  set_target_properties(
    ${VIEWER_BINARY_NAME}
    PROPERTIES
    OUTPUT_NAME "${product}"
    MACOSX_BUNDLE_INFO_STRING "info string - localize me"
    MACOSX_BUNDLE_ICON_FILE "${VIEWER_BRANDING_ID}.icns"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${VIEWER_BRANDING_NAME}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "ververver"
    MACOSX_BUNDLE_BUNDLE_NAME "${VIEWER_BRANDING_NAME}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "asdf"
    MACOSX_BUNDLE_BUNDLE_VERSION "asdf"
    MACOSX_BUNDLE_COPYRIGHT "copyright linden lab 2007 - localize me and run me through a legal wringer"
    )

  add_custom_command(
    TARGET ${VIEWER_BINARY_NAME} POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      --grid=${GRID}
      --actions=copy
      --configuration=${CMAKE_CFG_INTDIR}
      --source=${CMAKE_CURRENT_SOURCE_DIR}
      --artwork=${ARTWORK_DIR}
      --branding_id=${VIEWER_BRANDING_ID}
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
      --buildtype=${CMAKE_BUILD_TYPE}
    DEPENDS ${VIEWER_BINARY_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
    )

  add_dependencies(${VIEWER_BINARY_NAME} SLPlugin media_plugin_quicktime media_plugin_webkit)

  if (PACKAGE)
      add_custom_target(package ALL DEPENDS ${VIEWER_BINARY_NAME}) 
      add_dependencies(package mac-crash-logger)

      add_custom_command(
        TARGET package POST_BUILD
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
          --grid=${GRID}
          --configuration=${CMAKE_CFG_INTDIR}
          --channel=${VIEWER_CHANNEL}
          --login_channel=${VIEWER_LOGIN_CHANNEL}
          --branding_id=${VIEWER_BRANDING_ID}
          --source=${CMAKE_CURRENT_SOURCE_DIR}
          --artwork=${ARTWORK_DIR}
          --build=${CMAKE_CURRENT_BINARY_DIR}
          --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
          --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
          --buildtype=${CMAKE_BUILD_TYPE}
        DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      )


      add_custom_command(
        TARGET package POST_BUILD
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
          --grid=${GRID}
          --configuration=${CMAKE_CFG_INTDIR}
          --channel=${VIEWER_CHANNEL}
          --login_channel=${VIEWER_LOGIN_CHANNEL}
          --branding_id=${VIEWER_BRANDING_ID}
          --source=${CMAKE_CURRENT_SOURCE_DIR}
          --artwork=${ARTWORK_DIR}
          --build=${CMAKE_CURRENT_BINARY_DIR}
          --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
          --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
          --buildtype=${CMAKE_BUILD_TYPE}
        DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      )

  endif (PACKAGE)
endif (DARWIN)

if (INSTALL)
  include(${CMAKE_CURRENT_SOURCE_DIR}/ViewerInstall.cmake)
endif (INSTALL)

# Don't do these for DARWIN or LINUX here -- they're taken care of by viewer_manifest.py
if (WINDOWS)

  get_target_property(BUILT_LLCOMMON llcommon LOCATION)
  add_custom_command(
      TARGET ${VIEWER_BINARY_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      ARGS
      -E
      copy_if_different
      ${BUILT_LLCOMMON}
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
      COMMENT "Copying llcommon.dll to the runtime folder."
      )

  get_target_property(BUILT_SLPLUGIN SLPlugin LOCATION)
  add_custom_command(
      TARGET ${VIEWER_BINARY_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      ARGS
        -E
        copy_if_different
        ${BUILT_SLPLUGIN}
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
      COMMENT "Copying SLPlugin executable to the runtime folder."
      )

  get_target_property(BUILT_WEBKIT_PLUGIN media_plugin_webkit LOCATION)
    add_custom_command(
        TARGET ${VIEWER_BINARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
          -E
          copy_if_different
          ${BUILT_WEBKIT_PLUGIN}
          ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/llplugin
        COMMENT "Copying WebKit Plugin to the runtime folder."
        )

  get_target_property(BUILT_QUICKTIME_PLUGIN media_plugin_quicktime LOCATION)
    add_custom_command(
        TARGET ${VIEWER_BINARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
          -E
          copy_if_different
          ${BUILT_QUICKTIME_PLUGIN}
          ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/llplugin
        COMMENT "Copying Quicktime Plugin to the runtime folder."
        )

endif (WINDOWS)

if (DARWIN)
# Don't do this here -- it's taken care of by viewer_manifest.py
#  add_custom_command(TARGET ${VIEWER_BINARY_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release/libllqtwebkit.dylib ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/llplugin/
#    DEPENDS ${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release/libllqtwebkit.dylib
#  )
endif (DARWIN)
