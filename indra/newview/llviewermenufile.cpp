/** 
 * @file llviewermenufile.cpp
 * @brief "File" menu in the main menu bar.
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

// system libraries
#include "boost/tokenizer.hpp"

#include "llviewermenufile.h"

// linden libraries
#include "lleconomy.h"
#include "llhttpclient.h"
#include "llimage.h"
#include "llmemberlistener.h"
#include "llnotifications.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llstring.h"
#include "lltransactiontypes.h"
#include "lluictrlfactory.h"
#include "lluuid.h"
#include "llvorbisencode.h"
#include "message.h"

// project includes
#include "llagent.h"
#include "llappviewer.h"
#include "llassetuploadresponders.h"
#include "lldirpicker.h"
#include "llfloateranimpreview.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterimagepreview.h"
#include "llfloatermodelpreview.h"
#include "llfloaternamedesc.h"
#include "llfloaterperms.h"
#include "llfloatersnapshot.h"
#include "llinventorymodel.h"			// gInventory
#include "llinventoryview.h"			// open_texture()
#include "llmeshrepository.h"
#include "llresourcedata.h"
#include "llstatusbar.h"
#include "lluploaddialog.h"
#include "llviewercontrol.h"			// gSavedSettings
#include "llviewermenu.h"				// gMenuHolder
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"

using namespace LLOldEvents;

std::deque<std::string> gUploadQueue;

typedef LLMemberListener<LLView> view_listener_t;

class LLFileEnableSaveAs : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = !LLFilePickerThread::isInUse() &&
						 !LLDirPickerThread::isInUse() &&
						 gFloaterView->getFrontmost() &&
						 gFloaterView->getFrontmost()->canSaveAs();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLFileEnableUpload : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = !LLFilePickerThread::isInUse() &&
						 !LLDirPickerThread::isInUse() &&
						 gStatusBar && LLGlobalEconomy::Singleton::getInstance() &&
						 gStatusBar->getBalance() >= LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLFileEnableUploadModel : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = !LLFilePickerThread::isInUse() &&
						 !LLDirPickerThread::isInUse() &&
						 gMeshRepo.meshUploadEnabled() &&
#if LL_WINDOWS
						 gSavedSettings.getBOOL("MeshUploadEnable") &&
#endif
						 LLFloaterModelPreview::sInstance == NULL;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

#if LL_WINDOWS
static std::string SOUND_EXTENSIONS = "wav";
static std::string IMAGE_EXTENSIONS = "tga bmp jpg jpeg png";
static std::string ANIM_EXTENSIONS =  "bvh";
static std::string XML_EXTENSIONS = "xml";
static std::string LSL_EXTENSIONS = "lsl txt";
static std::string SLOBJECT_EXTENSIONS = "slobject";
#endif
static std::string ALL_FILE_EXTENSIONS = "*.*";
static std::string MODEL_EXTENSIONS = "dae";

std::string build_extensions_string(LLFilePicker::ELoadFilter filter)
{
	switch (filter)
	{
#if LL_WINDOWS
	case LLFilePicker::FFLOAD_IMAGE:
		return IMAGE_EXTENSIONS;
	case LLFilePicker::FFLOAD_WAV:
		return SOUND_EXTENSIONS;
	case LLFilePicker::FFLOAD_ANIM:
		return ANIM_EXTENSIONS;
	case LLFilePicker::FFLOAD_SLOBJECT:
		return SLOBJECT_EXTENSIONS;
	case LLFilePicker::FFLOAD_MODEL:
		return MODEL_EXTENSIONS;
	case LLFilePicker::FFLOAD_XML:
	    return XML_EXTENSIONS;
	case LLFilePicker::FFLOAD_SCRIPT:
	    return LSL_EXTENSIONS;
	case LLFilePicker::FFLOAD_ALL:
		return ALL_FILE_EXTENSIONS;
#endif
    default:
	return ALL_FILE_EXTENSIONS;
	}
}

void upload_pick_callback(LLFilePicker::ELoadFilter type,
						  std::string& filename,
						  std::deque<std::string>& files,
						  void*)
{
	if (filename.empty())
	{
		return;
	}

	std::string ext = gDirUtilp->getExtension(filename);

	// strincmp doesn't like NULL pointers
	if (ext.empty())
	{
		std::string short_name = gDirUtilp->getBaseFileName(filename);

		// No extension
		LLSD args;
		args["FILE"] = short_name;
		LLNotifications::instance().add("NoFileExtension", args);
		return;
	}
	else
	{
		// There is an extension: loop over the valid extensions and compare
		// to see if the extension is valid

		//now grab the set of valid file extensions
		std::string valid_extensions = build_extensions_string(type);

		BOOL ext_valid = FALSE;

		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep(" ");
		tokenizer tokens(valid_extensions, sep);
		tokenizer::iterator token_iter;

		// Now loop over all valid file extensions and compare them to the
		// extension of the file to be uploaded
		for (token_iter = tokens.begin();
			 token_iter != tokens.end() && ext_valid != TRUE;
			 ++token_iter)
		{
			const std::string& cur_token = *token_iter;

			if (cur_token == ext || cur_token == "*.*")
			{
				// valid extension or the acceptable extension is any
				ext_valid = TRUE;
			}
		}

		if (ext_valid == FALSE)
		{
			//should only get here if the extension exists
			//but is invalid
			LLSD args;
			args["EXTENSION"] = ext;
			args["VALIDS"] = valid_extensions;
			LLNotifications::instance().add("InvalidFileExtension", args);
			return;
		}
	}

	if (type == LLFilePicker::FFLOAD_IMAGE)
	{
		LLFloaterImagePreview* floaterp = new LLFloaterImagePreview(filename);
		LLUICtrlFactory::getInstance()->buildFloater(floaterp, "floater_image_preview.xml");
	}
	else if (type == LLFilePicker::FFLOAD_WAV)
	{
		// Pre-qualify wavs to make sure the format is acceptable
		std::string error_msg;
		if (check_for_invalid_wav_formats(filename,error_msg))
		{
			llinfos << error_msg << ": " << filename << llendl;
			LLSD args;
			args["FILE"] = filename;
			LLNotifications::instance().add(error_msg, args);
			return;
		}

		LLFloaterNameDesc* floaterp = new LLFloaterNameDesc(filename);
		LLUICtrlFactory::getInstance()->buildFloater(floaterp, "floater_sound_preview.xml");
		floaterp->childSetLabelArg("ok_btn", "[AMOUNT]", llformat("%d", LLGlobalEconomy::Singleton::getInstance()->getPriceUpload()));
	}
	else if (type == LLFilePicker::FFLOAD_ANIM)
	{
		LLFloaterAnimPreview* floaterp = new LLFloaterAnimPreview(filename);
		LLUICtrlFactory::getInstance()->buildFloater(floaterp, "floater_animation_preview.xml");
	}
}

void upload_pick(LLFilePicker::ELoadFilter type)
{
 	if (gAgent.cameraMouselook())
	{
		gAgent.changeCameraToDefault();
	}

	(new LLLoadFilePicker(type, upload_pick_callback))->getFile();
}

class LLFileUploadImage : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		upload_pick(LLFilePicker::FFLOAD_IMAGE);
		return true;
	}
};

class LLFileUploadSound : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		upload_pick(LLFilePicker::FFLOAD_WAV);
		return true;
	}
};

class LLFileUploadAnim : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		upload_pick(LLFilePicker::FFLOAD_ANIM);
		return true;
	}
};

void upload_bulk_callback(LLFilePicker::ELoadFilter type,
						  std::string& filename,
						  std::deque<std::string>& files,
						  void*)
{
	if (filename.empty())
	{
		return;
	}

	// First remember if there are ongoing uploads already in progress
	bool no_upload = gUploadQueue.empty();

	while (!files.empty())
	{
		gUploadQueue.push_back(files.front());
		files.pop_front();
	}

	if (no_upload)
	{
		std::string filename = gUploadQueue.front();
		gUploadQueue.pop_front();
		std::string name = gDirUtilp->getBaseFileName(filename, true);

		std::string asset_name = name;
		LLStringUtil::replaceNonstandardASCII(asset_name, '?');
		LLStringUtil::replaceChar(asset_name, '|', '?');
		LLStringUtil::stripNonprintable(asset_name);
		LLStringUtil::trim(asset_name);

		std::string display_name = LLStringUtil::null;
		LLAssetStorage::LLStoreAssetCallback callback = NULL;
		S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
		void *userdata = NULL;

		upload_new_resource(filename, asset_name, asset_name, 0,
							LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
							LLFloaterPerms::getNextOwnerPerms(),
							LLFloaterPerms::getGroupPerms(),
							LLFloaterPerms::getEveryonePerms(),
							display_name, callback, expected_upload_cost,
							userdata);
		// *NOTE: Ew, we don't iterate over the file list here,
		// we handle the next files in upload_done_callback()
	}
}

class LLFileUploadBulk : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAgent.cameraMouselook())
		{
			gAgent.changeCameraToDefault();
		}

		(new LLLoadFilePicker(LLFilePicker::FFLOAD_ALL, upload_bulk_callback))->getFile(true);
		return true;
	}
};

class LLFileUploadModel : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloaterModelPreview* fmp = new LLFloaterModelPreview("Model Preview");
		if (fmp)
		{
			fmp->loadModel(3);
		}
		return true;
	}
};

void upload_error(const std::string& error_message, const std::string& label, const std::string& filename, const LLSD& args) 
{
	llwarns << error_message << llendl;
	LLNotifications::instance().add(label, args);
	if (LLFile::remove(filename) == -1)
	{
		LL_DEBUGS("Upload") << "unable to remove temp file" << LL_ENDL;
	}
	gUploadQueue.clear();
}

class LLFileEnableCloseWindow : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = NULL != LLFloater::getClosableFloaterFromFocus();

		// horrendously opaque, this code
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLFileCloseWindow : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloater::closeFocusedFloater();

		return true;
	}
};

class LLFileEnableCloseAllWindows : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool open_children = gFloaterView->allChildrenClosed();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(!open_children);
		return true;
	}
};

class LLFileCloseAllWindows : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool app_quitting = false;
		gFloaterView->closeAllChildren(app_quitting);

		return true;
	}
};

class LLFileSaveTexture : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloater* top = gFloaterView->getFrontmost();
		if (top)
		{
			top->saveAs();
		}
		return true;
	}
};

class LLFileTakeSnapshot : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloaterSnapshot::show(NULL);
		return true;
	}
};

void snapshot_to_disk(LLFilePicker::ESaveFilter type,
					  std::string& filename, void*)
{
	if (filename.empty()) return;

	if (!gViewerWindow->isSnapshotLocSet())
	{
		gViewerWindow->setSnapshotLoc(filename);
	}

	LLPointer<LLImageRaw> raw = new LLImageRaw;

	S32 width = gViewerWindow->getWindowDisplayWidth();
	S32 height = gViewerWindow->getWindowDisplayHeight();

	if (gSavedSettings.getBOOL("HighResSnapshot"))
	{
		width *= 2;
		height *= 2;
	}

	if (gViewerWindow->rawSnapshot(raw, width, height, TRUE, FALSE,
								   gSavedSettings.getBOOL("RenderUIInSnapshot"),
								   FALSE))
	{
		gViewerWindow->playSnapshotAnimAndSound();

		LLImageBase::setSizeOverride(TRUE);
		LLPointer<LLImageFormatted> formatted;
		switch (type)
		{
			case LLFilePicker::FFSAVE_JPEG:
				formatted = new LLImageJPEG(gSavedSettings.getS32("SnapshotQuality"));
				break;
			case LLFilePicker::FFSAVE_PNG:
				formatted = new LLImagePNG;
				break;
			case LLFilePicker::FFSAVE_BMP: 
				formatted = new LLImageBMP;
				break;
			default: 
				llwarns << "Unknown Local Snapshot format" << llendl;
				LLImageBase::setSizeOverride(FALSE);
				return;
		}

		formatted->encode(raw, 0);
		LLImageBase::setSizeOverride(FALSE);
		gViewerWindow->saveImageNumbered(formatted);
	}
}

class LLFileTakeSnapshotToDisk : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFilePicker::ESaveFilter type;
		switch (LLFloaterSnapshot::ESnapshotFormat(gSavedSettings.getS32("SnapshotFormat")))
		{
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG:
				type = LLFilePicker::FFSAVE_JPEG;
				break;
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG:
				type = LLFilePicker::FFSAVE_PNG;
				break;
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP: 
				type = LLFilePicker::FFSAVE_BMP;
				break;
			default: 
				llwarns << "Unknown Local Snapshot format" << llendl;
				return true;
		}
		std::string suggestion = gViewerWindow->getSnapshotBaseName();
		if (gViewerWindow->isSnapshotLocSet())
		{
			snapshot_to_disk(type, suggestion, NULL);
		}
		else
		{
			(new LLSaveFilePicker(type, snapshot_to_disk))->getSaveFile(suggestion);
		}
		return true;
	}
};

class LLFileQuit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLAppViewer::instance()->userQuit();
		return true;
	}
};

void compress_image_callback(LLFilePicker::ELoadFilter type,
							 std::string& filename,
							 std::deque<std::string>& files,
							 void*)
{
	if (filename.empty())
	{
		return;
	}

	LLSD args;
	std::string infile, extension, outfile, report;

	while (!files.empty())
	{
		infile = files.front();
		extension = gDirUtilp->getExtension(infile);
		EImageCodec codec = LLImageBase::getCodecFromExtension(extension);
		if (codec == IMG_CODEC_INVALID)
		{
			llinfos << "Error compressing image: " << infile
					<< " - Unknown codec !" << llendl;
		}

		outfile = gDirUtilp->getDirName(infile) + gDirUtilp->getDirDelimiter() +
				  gDirUtilp->getBaseFileName(infile, true) + ".j2c";

		llinfos << "Compressing image... Input: " << infile << " - Output: "
				<< outfile << llendl;

		if (LLViewerTextureList::createUploadFile(infile, outfile, codec))
		{
			llinfos << "Compression complete" << llendl;
			report = infile + " successfully compressed to " + outfile;
		}
		else
		{
			report = LLImage::getLastError();
			llinfos << "Compression failed: " << report << llendl;
			report = " Failed to compress " + infile + " - " + report;
		}
		args["MESSAGE"] = report;
 		LLNotifications::instance().add("SystemMessageTip", args);

		files.pop_front();
	}
}

void handle_compress_image(void*)
{
	(new LLLoadFilePicker(LLFilePicker::FFLOAD_IMAGE,
						  compress_image_callback))->getFile(true);
}

void upload_new_resource(const std::string& src_filename,
						 std::string name,
						 std::string desc,
						 S32 compression_info,
						 LLFolderType::EType destination_folder_type,
						 LLInventoryType::EType inv_type,
						 U32 next_owner_perms,
						 U32 group_perms,
						 U32 everyone_perms,
						 const std::string& display_name,
						 LLAssetStorage::LLStoreAssetCallback callback,
						 S32 expected_upload_cost,
						 void *userdata)
{
	// Generate the temporary UUID.
	std::string filename = gDirUtilp->getTempFilename();
	LLTransactionID tid;
	LLAssetID uuid;

	LLSD args;

	std::string exten = gDirUtilp->getExtension(src_filename);
	EImageCodec codec = LLImageBase::getCodecFromExtension(exten);
	LLAssetType::EType asset_type = LLAssetType::AT_NONE;
	std::string error_message;

	bool error = false;

	if (exten.empty())
	{
		std::string short_name = gDirUtilp->getBaseFileName(src_filename);

		// No extension
		error_message = llformat("No file extension for the file: '%s'\nPlease make sure the file has a correct file extension",
								 short_name.c_str());
		args["FILE"] = short_name;
 		upload_error(error_message, "NoFileExtension", filename, args);
		return;
	}
	else if (codec != IMG_CODEC_INVALID)
	{
		// It's an image file, the upload procedure is the same for all
		asset_type = LLAssetType::AT_TEXTURE;
		if (!LLViewerTextureList::createUploadFile(src_filename, filename, codec))
		{
			error_message = llformat("Problem with file %s:\n\n%s\n",
									 src_filename.c_str(),
									 LLImage::getLastError().c_str());
			args["FILE"] = src_filename;
			args["ERROR"] = LLImage::getLastError();
			upload_error(error_message, "ProblemWithFile", filename, args);
			return;
		}
	}
	else if (exten == "wav")
	{
		asset_type = LLAssetType::AT_SOUND;  // tag it as audio
		S32 encode_result = 0;

		llinfos << "Attempting to encode wav as an ogg file" << llendl;

		encode_result = encode_vorbis_file(src_filename, filename);

		if (LLVORBISENC_NOERR != encode_result)
		{
			switch (encode_result)
			{
				case LLVORBISENC_DEST_OPEN_ERR:
				    error_message = llformat("Couldn't open temporary compressed sound file for writing: %s\n", filename.c_str());
					args["FILE"] = filename;
					upload_error(error_message, "CannotOpenTemporarySoundFile", filename, args);
					break;

				default:
				  error_message = llformat("Unknown vorbis encode failure on: %s\n", src_filename.c_str());
					args["FILE"] = src_filename;
					upload_error(error_message, "UnknownVorbisEncodeFailure", filename, args);
					break;
			}
			return;
		}
	}
	else if (exten == "tmp")	 
	{	 
		// This is a generic .lin resource file
		asset_type = LLAssetType::AT_OBJECT;
		LLFILE* in = LLFile::fopen(src_filename, "rb");		/* Flawfinder: ignore */
		if (in)
		{
			// read in the file header
			char buf[16384];		/* Flawfinder: ignore */
			size_t readbytes;
			S32  version;
			if (fscanf(in, "LindenResource\nversion %d\n", &version))
			{
				if (version == 2)
				{
					// *NOTE: This buffer size is hard coded into scanf() below.
					char label[MAX_STRING];		/* Flawfinder: ignore */
					char value[MAX_STRING];		/* Flawfinder: ignore */
					S32  tokens_read;
					while (fgets(buf, 1024, in))
					{
						label[0] = '\0';
						value[0] = '\0';
						tokens_read = sscanf(buf, "%254s %254s\n", label, value);	/* Flawfinder: ignore */

						llinfos << "got: " << label << " = " << value << llendl;

						if (EOF == tokens_read)
						{
							fclose(in);
							error_message = llformat("corrupt resource file: %s", src_filename.c_str());
							args["FILE"] = src_filename;
							upload_error(error_message, "CorruptResourceFile", filename, args);
							return;
						}

						if (tokens_read == 2)
						{
							if (!strcmp("type", label))
							{
								asset_type = (LLAssetType::EType)(atoi(value));
							}
						}
						else
						{
							if (!strcmp("_DATA_", label))
							{
								// below is the data section
								break;
							}
						}
						// other values are currently discarded
					}
				}
				else
				{
					fclose(in);
					error_message = llformat("unknown linden resource file version in file: %s", src_filename.c_str());
					args["FILE"] = src_filename;
					upload_error(error_message, "UnknownResourceFileVersion", filename, args);
					return;
				}
			}
			else
			{
				// this is an original binary formatted .lin file
				// start over at the beginning of the file
				fseek(in, 0, SEEK_SET);

				const S32 MAX_ASSET_DESCRIPTION_LENGTH = 256;
				const S32 MAX_ASSET_NAME_LENGTH = 64;
				S32 header_size = 34 + MAX_ASSET_DESCRIPTION_LENGTH + MAX_ASSET_NAME_LENGTH;
				S16 type_num;

				// read in and throw out most of the header except for the type
				if (fread(buf, header_size, 1, in) != 1)
				{
					llwarns << "Short read" << llendl;
				}
				memcpy(&type_num, buf + 16, sizeof(S16));		/* Flawfinder: ignore */
				asset_type = (LLAssetType::EType)type_num;
			}

			// copy the file's data segment into another file for uploading
			LLFILE* out = LLFile::fopen(filename, "wb");		/* Flawfinder: ignore */
			if (out)
			{
				while ((readbytes = fread(buf, 1, 16384, in)))		/* Flawfinder: ignore */
				{
					if (fwrite(buf, 1, readbytes, out) != readbytes)
					{
						llwarns << "Short write" << llendl;
					}
				}
				fclose(out);
			}
			else
			{
				fclose(in);
				error_message = llformat("Unable to create output file: %s", filename.c_str());
				args["FILE"] = filename;
				upload_error(error_message, "UnableToCreateOutputFile", filename, args);
				return;
			}
			fclose(in);
		}
		else
		{	 
			llinfos << "Couldn't open .lin file " << src_filename << llendl;
		}
	}
	else if (exten == "anim")
	{
		asset_type = LLAssetType::AT_ANIMATION;
		filename = src_filename;
	}
	else if (exten == "bvh")
	{
		error_message = llformat("We do not currently support bulk upload of animation files\n");
		upload_error(error_message, "DoNotSupportBulkAnimationUpload", filename, args);
		return;
	}
	else
	{
		// Unknown extension
		// *TODO: Translate?
		error_message = llformat("Unknown file extension .%s\nExpected .wav, .tga, .bmp, .jpg, .jpeg, .bvh or .anim", exten.c_str());
		error = true;
	}

	// gen a new transaction ID for this asset
	tid.generate();

	if (!error)
	{
		uuid = tid.makeAssetID(gAgent.getSecureSessionID());
		// copy this file into the vfs for upload
		S32 file_size;
		LLAPRFile infile;
		infile.open(filename, LL_APR_RB, NULL, &file_size);
		if (infile.getFileHandle())
		{
			LLVFile file(gVFS, uuid, asset_type, LLVFile::WRITE);

			file.setMaxSize(file_size);

			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while ((file_size = infile.read(copy_buf, buf_size)))
			{
				file.write(copy_buf, file_size);
			}
		}
		else
		{
			error_message = llformat("Unable to access output file: %s", filename.c_str());
			error = true;
		}
	}

	if (!error)
	{
		std::string t_disp_name = display_name;
		if (t_disp_name.empty())
		{
			t_disp_name = src_filename;
		}
		upload_new_resource(tid,
							asset_type,
							name,
							desc,
							compression_info, // tid
							destination_folder_type,
							inv_type,
							next_owner_perms,
							group_perms,
							everyone_perms,
							display_name,
							callback,
							expected_upload_cost,
							userdata);
	}
	else
	{
		llwarns << error_message << llendl;
		LLSD args;
		args["ERROR_MESSAGE"] = error_message;
		LLNotifications::instance().add("ErrorMessage", args);
		if (LLFile::remove(filename) == -1)
		{
			LL_DEBUGS("Upload") << "unable to remove temp file" << LL_ENDL;
		}
		gUploadQueue.clear();
	}
}

void upload_done_callback(const LLUUID& uuid,
						  void* user_data,
						  S32 result,
						  LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLResourceData* data = (LLResourceData*)user_data;
	S32 expected_upload_cost = data ? data->mExpectedUploadCost : 0;
	//LLAssetType::EType pref_loc = data->mPreferredLocation;
	BOOL is_balance_sufficient = TRUE;

	if (data)
	{
		if (result >= 0)
		{
			LLFolderType::EType dest_loc = (data->mPreferredLocation ==  LLFolderType::FT_NONE) ?
											LLFolderType::assetTypeToFolderType(data->mAssetInfo.mType) :
											data->mPreferredLocation;

			if (LLAssetType::AT_SOUND == data->mAssetInfo.mType ||
				LLAssetType::AT_TEXTURE == data->mAssetInfo.mType ||
				LLAssetType::AT_ANIMATION == data->mAssetInfo.mType)
			{
				// Charge the user for the upload.
				LLViewerRegion* region = gAgent.getRegion();

				if (!(can_afford_transaction(expected_upload_cost)))
				{
					LLFloaterBuyCurrency::buyCurrency(llformat("Uploading %s costs", // *TODO: Translate
													  data->mAssetInfo.getName().c_str()),
													  expected_upload_cost);
					is_balance_sufficient = FALSE;
				}
				else if (region)
				{
					// Charge user for upload
					if (gStatusBar)
					{
						gStatusBar->debitBalance(expected_upload_cost);
					}

					LLMessageSystem* msg = gMessageSystem;
					msg->newMessageFast(_PREHASH_MoneyTransferRequest);
					msg->nextBlockFast(_PREHASH_AgentData);
					msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
					msg->nextBlockFast(_PREHASH_MoneyData);
					msg->addUUIDFast(_PREHASH_SourceID, gAgent.getID());
					msg->addUUIDFast(_PREHASH_DestID, LLUUID::null);
					msg->addU8("Flags", 0);
					// we tell the sim how much we were expecting to pay so it
					// can respond to any discrepancy
					msg->addS32Fast(_PREHASH_Amount, expected_upload_cost);
					msg->addU8Fast(_PREHASH_AggregatePermNextOwner, (U8)LLAggregatePermissions::AP_EMPTY);
					msg->addU8Fast(_PREHASH_AggregatePermInventory, (U8)LLAggregatePermissions::AP_EMPTY);
					msg->addS32Fast(_PREHASH_TransactionType, TRANS_UPLOAD_CHARGE);
					msg->addStringFast(_PREHASH_Description, NULL);
					msg->sendReliable(region->getHost());
				}
			}

			if (is_balance_sufficient)
			{
				// Actually add the upload to inventory
				llinfos << "Adding " << uuid << " to inventory." << llendl;
				const LLUUID folder_id = gInventory.findCategoryUUIDForType(dest_loc);
				if (folder_id.notNull())
				{
					U32 next_owner_perms = data->mNextOwnerPerm;
					if (PERM_NONE == next_owner_perms)
					{
						next_owner_perms = PERM_MOVE | PERM_TRANSFER;
					}
					create_inventory_item(gAgent.getID(),
										  gAgent.getSessionID(),
										  folder_id,
										  data->mAssetInfo.mTransactionID,
										  data->mAssetInfo.getName(),
										  data->mAssetInfo.getDescription(),
										  data->mAssetInfo.mType,
										  data->mInventoryType,
										  NOT_WEARABLE,
										  next_owner_perms,
										  LLPointer<LLInventoryCallback>(NULL));
				}
				else
				{
					llwarns << "Can't find a folder to put it in" << llendl;
				}
			}
		}
		else	// if (result >= 0)
		{
			LLSD args;
			args["FILE"] = LLInventoryType::lookupHumanReadable(data->mInventoryType);
			args["REASON"] = std::string(LLAssetStorage::getErrorString(result));
			LLNotifications::instance().add("CannotUploadReason", args);
		}

		delete data;
		data = NULL;
	}

	LLUploadDialog::modalUploadFinished();

	if (!gUploadQueue.empty() && is_balance_sufficient)
	{
		std::string next_file = gUploadQueue.front();
		gUploadQueue.pop_front();
		if (next_file.empty()) return;
		std::string asset_name = gDirUtilp->getBaseFileName(next_file, true);
		LLStringUtil::replaceNonstandardASCII(asset_name, '?');
		LLStringUtil::replaceChar(asset_name, '|', '?');
		LLStringUtil::stripNonprintable(asset_name);
		LLStringUtil::trim(asset_name);

		std::string display_name = LLStringUtil::null;
		LLAssetStorage::LLStoreAssetCallback callback = NULL;
		void *userdata = NULL;
		upload_new_resource(next_file,
							asset_name,
							asset_name,		// file
							0,
							LLFolderType::FT_NONE,
							LLInventoryType::IT_NONE,
							PERM_NONE,
							PERM_NONE,
							PERM_NONE,
							display_name,
							callback,
							expected_upload_cost, // assuming next in a group of uploads is of roughly the same type, i.e. same upload cost
							userdata);
	}
}

void temp_upload_done_callback(const LLUUID& uuid, void* user_data, S32 result, LLExtStat ext_status)
{
	LLResourceData* data = (LLResourceData*)user_data;
	if (result >= 0)
	{
		LLFolderType::EType dest_loc = data->mPreferredLocation == LLFolderType::FT_NONE ?
									   LLFolderType::assetTypeToFolderType(data->mAssetInfo.mType) :
									   data->mPreferredLocation;
		LLUUID folder_id(gInventory.findCategoryUUIDForType(dest_loc));
		LLUUID item_id;
		item_id.generate();
		LLPermissions perm;
		perm.init(gAgentID, gAgentID, gAgentID, gAgentID);
		perm.setMaskBase(PERM_ALL);
		perm.setMaskOwner(PERM_ALL);
		perm.setMaskEveryone(PERM_ALL);
		perm.setMaskGroup(PERM_ALL);
		LLPointer<LLViewerInventoryItem> item = new LLViewerInventoryItem(item_id, folder_id, perm,
													data->mAssetInfo.mTransactionID.makeAssetID(gAgent.getSecureSessionID()),
													data->mAssetInfo.mType, data->mInventoryType, data->mAssetInfo.getName(),
													"Temporary asset", LLSaleInfo::DEFAULT, LLInventoryItem::II_FLAGS_NONE,
													time_corrected());
		item->updateServer(TRUE);
		gInventory.updateItem(item);
		gInventory.notifyObservers();
		open_texture(item_id, std::string("Texture: ") + item->getName(), TRUE, LLUUID::null, FALSE);
	}
	else
	{
		LLSD args;
		args["FILE"] = LLInventoryType::lookupHumanReadable(data->mInventoryType);
		args["REASON"] = std::string(LLAssetStorage::getErrorString(result));
		LLNotifications::instance().add("CannotUploadReason", args);
	}

	LLUploadDialog::modalUploadFinished();
	delete data;
}

static LLAssetID upload_new_resource_prep(const LLTransactionID& tid,
										  LLAssetType::EType asset_type,
										  LLInventoryType::EType& inventory_type,
										  std::string& name,
										  const std::string& display_name,
										  std::string& description)
{
	LLAssetID uuid = generate_asset_id_for_new_upload(tid);

	increase_new_upload_stats(asset_type);

	assign_defaults_and_show_upload_message(asset_type,
											inventory_type,
											name,
											display_name,
											description);

	return uuid;
}

LLSD generate_new_resource_upload_capability_body(LLAssetType::EType asset_type,
												  const std::string& name,
												  const std::string& desc,
												  LLFolderType::EType destination_folder_type,
												  LLInventoryType::EType inv_type,
												  U32 next_owner_perms,
												  U32 group_perms,
												  U32 everyone_perms)
{
	LLSD body;

	body["folder_id"] = gInventory.findCategoryUUIDForType(destination_folder_type == LLFolderType::FT_NONE ?
														   LLFolderType::assetTypeToFolderType(asset_type) :
														   destination_folder_type);

	body["asset_type"] = LLAssetType::lookup(asset_type);
	body["inventory_type"] = LLInventoryType::lookup(inv_type);
	body["name"] = name;
	body["description"] = desc;
	body["next_owner_mask"] = LLSD::Integer(next_owner_perms);
	body["group_mask"] = LLSD::Integer(group_perms);
	body["everyone_mask"] = LLSD::Integer(everyone_perms);

	return body;
}

void upload_new_resource(const LLTransactionID &tid,
						 LLAssetType::EType asset_type,
						 std::string name,
						 std::string desc,
						 S32 compression_info,
						 LLFolderType::EType destination_folder_type,
						 LLInventoryType::EType inv_type,
						 U32 next_owner_perms,
						 U32 group_perms,
						 U32 everyone_perms,
						 const std::string& display_name,
						 LLAssetStorage::LLStoreAssetCallback callback,
						 S32 expected_upload_cost,
						 void *userdata)
{
	if (gDisconnected)
	{
		return;
	}

	BOOL temp_upload = FALSE;
	if (LLAssetType::AT_TEXTURE == asset_type)
	{
		temp_upload = gSavedSettings.getBOOL("TemporaryUpload");
		if (temp_upload)
		{
			name = "[temp] " + name;
		}
		gSavedSettings.setBOOL("TemporaryUpload", FALSE);
	}

	LLAssetID uuid = upload_new_resource_prep(tid, asset_type, inv_type,
											  name, display_name, desc);

	llinfos << "*** Uploading: "
			<< "\nType: " << LLAssetType::lookup(asset_type)
			<< "\nUUID: " << uuid
			<< "\nName: " << name
			<< "\nDesc: " << desc
			<< "\nFolder: "
			<< gInventory.findCategoryUUIDForType(destination_folder_type == LLFolderType::FT_NONE ?
												  LLFolderType::assetTypeToFolderType(asset_type) :
												  destination_folder_type)
			<< "\nExpected Upload Cost: " << expected_upload_cost << llendl;

	std::string url = gAgent.getRegion()->getCapability("NewFileAgentInventory");
	if (!url.empty() && !temp_upload)
	{
		llinfos << "New Agent Inventory via capability" << llendl;
		LLSD body;
		body = generate_new_resource_upload_capability_body(asset_type,
															name,
															desc,
															destination_folder_type,
															inv_type,
															next_owner_perms,
															group_perms,
															everyone_perms);

		body["expected_upload_cost"] = LLSD::Integer(expected_upload_cost);

		LLHTTPClient::post(url, body,
						   new LLNewAgentInventoryResponder(body, uuid, asset_type));
	}
	else
	{
		if (!temp_upload)
		{
			llinfos << "NewAgentInventory capability not found, new agent inventory via asset system." << llendl;
			// check for adequate funds
			// TODO: do this check on the sim
			if (LLAssetType::AT_SOUND == asset_type ||
				LLAssetType::AT_TEXTURE == asset_type ||
				LLAssetType::AT_ANIMATION == asset_type)
			{
				S32 balance = gStatusBar->getBalance();
				if (balance < expected_upload_cost)
				{
					// insufficient funds, bail on this upload
					LLFloaterBuyCurrency::buyCurrency("Uploading costs", expected_upload_cost);
					return;
				}
			}
		}

		LLResourceData* data = new LLResourceData;
		data->mAssetInfo.mTransactionID = tid;
		data->mAssetInfo.mUuid = uuid;
		data->mAssetInfo.mType = asset_type;
		data->mAssetInfo.mCreatorID = gAgentID;
		data->mInventoryType = inv_type;
		data->mNextOwnerPerm = next_owner_perms;
		data->mExpectedUploadCost = expected_upload_cost;
		data->mUserData = userdata;
		data->mAssetInfo.setName(name);
		data->mAssetInfo.setDescription(desc);
		data->mPreferredLocation = destination_folder_type;

		LLAssetStorage::LLStoreAssetCallback asset_callback = temp_upload ? &temp_upload_done_callback : &upload_done_callback;
		if (callback)
		{
			asset_callback = callback;
		}
		gAssetStorage->storeAssetData(data->mAssetInfo.mTransactionID,
									  data->mAssetInfo.mType,
									  asset_callback,
									  (void*)data,
									  temp_upload,
									  TRUE,
									  temp_upload);
	}
}

LLAssetID generate_asset_id_for_new_upload(const LLTransactionID& tid)
{
	LLAssetID uuid;

	if (gDisconnected)
	{	
		uuid.setNull();
	}
	else
	{
		uuid = tid.makeAssetID(gAgent.getSecureSessionID());
	}

	return uuid;
}

void increase_new_upload_stats(LLAssetType::EType asset_type)
{
	if (LLAssetType::AT_SOUND == asset_type)
	{
		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_UPLOAD_SOUND_COUNT);
	}
	else if (LLAssetType::AT_TEXTURE == asset_type)
	{
		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_UPLOAD_TEXTURE_COUNT);
	}
	else if (LLAssetType::AT_ANIMATION == asset_type)
	{
		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_UPLOAD_ANIM_COUNT);
	}
}

void assign_defaults_and_show_upload_message(LLAssetType::EType asset_type,
											 LLInventoryType::EType& inventory_type,
											 std::string& name,
											 const std::string& display_name,
											 std::string& description)
{
	if (LLInventoryType::IT_NONE == inventory_type)
	{
		inventory_type = LLInventoryType::defaultForAssetType(asset_type);
	}
	LLStringUtil::stripNonprintable(name);
	LLStringUtil::stripNonprintable(description);
	if (name.empty())
	{
		name = "(No Name)";
	}
	if (description.empty())
	{
		description = "(No Description)";
	}

	// At this point, we're ready for the upload.
	std::string upload_message = "Uploading...\n\n";
	upload_message.append(display_name);
	LLUploadDialog::modalUploadDialog(upload_message);
}

void init_menu_file()
{
	(new LLFileUploadImage())->registerListener(gMenuHolder, "File.UploadImage");
	(new LLFileUploadSound())->registerListener(gMenuHolder, "File.UploadSound");
	(new LLFileUploadAnim())->registerListener(gMenuHolder, "File.UploadAnim");
	(new LLFileUploadBulk())->registerListener(gMenuHolder, "File.UploadBulk");
	(new LLFileCloseWindow())->registerListener(gMenuHolder, "File.CloseWindow");
	(new LLFileCloseAllWindows())->registerListener(gMenuHolder, "File.CloseAllWindows");
	(new LLFileEnableCloseWindow())->registerListener(gMenuHolder, "File.EnableCloseWindow");
	(new LLFileEnableCloseAllWindows())->registerListener(gMenuHolder, "File.EnableCloseAllWindows");
	(new LLFileSaveTexture())->registerListener(gMenuHolder, "File.SaveTexture");
	(new LLFileTakeSnapshot())->registerListener(gMenuHolder, "File.TakeSnapshot");
	(new LLFileTakeSnapshotToDisk())->registerListener(gMenuHolder, "File.TakeSnapshotToDisk");
	(new LLFileQuit())->registerListener(gMenuHolder, "File.Quit");

	(new LLFileEnableUpload())->registerListener(gMenuHolder, "File.EnableUpload");
	(new LLFileEnableSaveAs())->registerListener(gMenuHolder, "File.EnableSaveAs");

	(new LLFileUploadModel())->registerListener(gMenuHolder, "File.UploadModel");
	(new LLFileEnableUploadModel())->registerListener(gMenuHolder, "File.EnableUploadModel");
}
