/** 
 * @file llagentwearables.h
 * @brief LLAgentWearables class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#ifndef LL_LLAGENTWEARABLES_H
#define LL_LLAGENTWEARABLES_H

// libraries
#include "llinventory.h"
#include "llmemory.h"
#include "llui.h"
#include "lluuid.h"

// newview
#include "llinventorymodel.h"
#include "llviewerinventory.h"
#include "llvoavatardefines.h"

class LLInitialWearablesFetch;
class LLTexLayerTemplate;
class LLViewerObject;
class LLVOAvatarSelf;
class LLWearable;

class LLAgentWearables : public LLInitClass<LLAgentWearables>
{
	//--------------------------------------------------------------------
	// Constructors / destructors / Initializers
	//--------------------------------------------------------------------
public:
	friend class LLInitialWearablesFetch;

	LLAgentWearables();
	virtual			~LLAgentWearables();

	// LLInitClass interface
	static void		initClass() {}

	void 			setAvatarObject(LLVOAvatarSelf *avatar);
	void			createStandardWearables(BOOL female); 

protected:
	void			createStandardWearablesDone(S32 type, U32 index/* = 0*/);
	void			createStandardWearablesAllDone();

	//--------------------------------------------------------------------
	// Queries
	//--------------------------------------------------------------------
public:
	BOOL			isWearingItem(const LLUUID& item_id) const;
	BOOL			isWearableModifiable(LLWearableType::EType type, U32 index/* = 0*/) const;
	BOOL			isWearableModifiable(const LLUUID& item_id) const;

	BOOL			isWearableCopyable(LLWearableType::EType type, U32 index/* = 0*/) const;
	BOOL			areWearablesLoaded() const;
	bool			isSettingOutfit() const	{ return mIsSettingOutfit; }
	void			updateWearablesLoaded();
	bool			canMoveWearable(const LLUUID& item_id, bool closer_to_body);

	// Note: False for shape, skin, eyes, and hair, unless you have MORE than 1.
	bool			canWearableBeRemoved(const LLWearable* wearable) const;

	void			animateAllWearableParams(F32 delta, BOOL upload_bake);

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	const LLUUID		getWearableItemID(LLWearableType::EType type, U32 index/* = 0*/) const;
	const LLUUID		getWearableAssetID(LLWearableType::EType type, U32 index/* = 0*/) const;
	const LLWearable*	getWearableFromItemID(const LLUUID& item_id) const;
	LLWearable*			getWearableFromItemID(const LLUUID& item_id);
	LLWearable*			getWearableFromAssetID(const LLUUID& asset_id);
	LLInventoryItem*	getWearableInventoryItem(LLWearableType::EType type, U32 index/* = 0*/);
	static BOOL			selfHasWearable(LLWearableType::EType type);
	LLWearable*			getWearable(const LLWearableType::EType type, U32 index/* = 0*/);
	const LLWearable* 	getWearable(const LLWearableType::EType type, U32 index/* = 0*/) const;
	LLWearable*			getTopWearable(const LLWearableType::EType type);
	LLWearable*			getBottomWearable(const LLWearableType::EType type);
	U32					getWearableCount(const LLWearableType::EType type) const;
	U32					getWearableCount(const U32 tex_index) const;

	static const U32 MAX_CLOTHING_PER_TYPE = 5; 

	//--------------------------------------------------------------------
	// Setters
	//--------------------------------------------------------------------

private:
	// Low-level data structure setter - public access is via setWearableItem, etc.
	void 			setWearable(const LLWearableType::EType type, U32 index, LLWearable *wearable);
	U32 			pushWearable(const LLWearableType::EType type, LLWearable *wearable);
	void			wearableUpdated(LLWearable *wearable);
	void 			popWearable(LLWearable *wearable);
	void			popWearable(const LLWearableType::EType type, U32 index);

public:
	void			setWearableItem(LLInventoryItem* new_item, LLWearable* wearable, bool do_append = false);
	void			setWearableOutfit(const LLInventoryItem::item_array_t& items, const LLDynamicArray< LLWearable* >& wearables, BOOL remove);
	void			setWearableName(const LLUUID& item_id, const std::string& new_name);
	void			addLocalTextureObject(const LLWearableType::EType wearable_type, const LLVOAvatarDefines::ETextureIndex texture_type, U32 wearable_index);
	U32				getWearableIndex(const LLWearable* wearable) const;

protected:
	void			setWearableFinal(LLInventoryItem* new_item, LLWearable* new_wearable, bool do_append = false);
	static bool		onSetWearableDialog(const LLSD& notification, const LLSD& response, LLWearable* wearable);

	void			addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												LLWearable* wearable, 
												const LLUUID& category_id = LLUUID::null,
												BOOL notify = TRUE);
	void 			addWearabletoAgentInventoryDone(const LLWearableType::EType type,
													const U32 index,
													const LLUUID& item_id,
													LLWearable* wearable);
	void			recoverMissingWearable(const LLWearableType::EType type, U32 index/* = 0*/);
	void			recoverMissingWearableDone();

	//--------------------------------------------------------------------
	// Editing/moving wearables
	//--------------------------------------------------------------------

public:
	static void		createWearable(LLWearableType::EType type,
								   const LLUUID& parent_id = LLUUID::null,
								   bool wear = false);	// wear is not implemented for now

	//--------------------------------------------------------------------
	// Removing wearables
	//--------------------------------------------------------------------
public:
	void			removeWearable(const LLWearableType::EType type, bool do_remove_all/* = false*/, U32 index/* = 0*/);
private:
	void			removeWearableFinal(const LLWearableType::EType type, bool do_remove_all/* = false*/, U32 index/* = 0*/);
protected:
	static bool		onRemoveWearableDialog(const LLSD& notification, const LLSD& response);

	//--------------------------------------------------------------------
	// Server Communication
	//--------------------------------------------------------------------
public:
	// Processes the initial wearables update message (if necessary, since the outfit folder makes it redundant)
	static void		processAgentInitialWearablesUpdate(LLMessageSystem* mesgsys, void** user_data);
	LLUUID			computeBakedTextureHash(LLVOAvatarDefines::EBakedTextureIndex baked_index,
											BOOL generate_valid_hash = TRUE);

protected:
	void			sendAgentWearablesUpdate();
	void			sendAgentWearablesRequest();
	void			queryWearableCache();
	void 			updateServer();
	static void		onInitialWearableAssetArrived(LLWearable* wearable, void* userdata);

	//--------------------------------------------------------------------
	// Outfits
	//--------------------------------------------------------------------
public:
	void			makeNewOutfit(const std::string& new_folder_name,
								  const LLDynamicArray<S32>& wearables_to_include,
								  const LLDynamicArray<S32>& attachments_to_include,
								  BOOL rename_clothing);

private:
	void			makeNewOutfitDone(LLWearableType::EType type, U32 index); 

	//--------------------------------------------------------------------
	// Save Wearables
	//--------------------------------------------------------------------
public:
	void			saveWearableAs(const LLWearableType::EType type,
								   const U32 index,
								   const std::string& new_name,
								   BOOL save_in_lost_and_found);
	void			saveWearable(const LLWearableType::EType type,
								 const U32 index,
								 BOOL send_update = TRUE,
								 const std::string new_name = "");
	void			saveAllWearables();
	void			revertWearable(const LLWearableType::EType type, const U32 index);

	//--------------------------------------------------------------------
	// Static UI hooks
	//--------------------------------------------------------------------
public:
	static void		userRemoveWearable(const LLWearableType::EType &type, const U32 &index);
	static void		userRemoveWearablesOfType(const LLWearableType::EType &type);
	static void		userRemoveAllClothes();
	static void		userRemoveAllClothesStep2(BOOL proceed, void* userdata); // userdata is NULL

	typedef std::vector<LLViewerObject*> llvo_vec_t;

#if 0	// Currently unused in the Cool VL Viewer
	static void 	userUpdateAttachments(LLInventoryModel::item_array_t& obj_item_array);
#endif
	static void		userRemoveMultipleAttachments(llvo_vec_t& llvo_array);
	static void		userRemoveAllAttachments();
	static void		userAttachMultipleAttachments(LLInventoryModel::item_array_t& obj_item_array);

	//--------------------------------------------------------------------
	// Member variables
	//--------------------------------------------------------------------
private:
	typedef std::vector<LLWearable*> wearableentry_vec_t; // all wearables of a certain type (EG all shirts)
	typedef std::map<LLWearableType::EType, wearableentry_vec_t> wearableentry_map_t;	// wearable "categories" arranged by wearable type
	wearableentry_map_t	mWearableDatas;

	static BOOL			mInitialWearablesUpdateReceived;
	BOOL				mWearablesLoaded;
	bool				mIsSettingOutfit;

	//--------------------------------------------------------------------------------
	// Support classes
	//--------------------------------------------------------------------------------
private:
	class createStandardWearablesAllDoneCallback : public LLRefCount
	{
	protected:
		~createStandardWearablesAllDoneCallback();
	};

	class sendAgentWearablesUpdateCallback : public LLRefCount
	{
	protected:
		~sendAgentWearablesUpdateCallback();
	};

	class addWearableToAgentInventoryCallback : public LLInventoryCallback
	{
	public:
		enum ETodo
		{
			CALL_NONE = 0,
			CALL_UPDATE = 1,
			CALL_RECOVERDONE = 2,
			CALL_CREATESTANDARDDONE = 4,
			CALL_MAKENEWOUTFITDONE = 8
		};

		addWearableToAgentInventoryCallback(LLPointer<LLRefCount> cb,
											LLWearableType::EType type,
											U32 index,
											LLWearable* wearable,
											U32 todo = CALL_NONE);
		virtual void fire(const LLUUID& inv_item);

	private:
		LLWearableType::EType mType;
		U32 mIndex;
		LLWearable* mWearable;
		U32 mTodo;
		LLPointer<LLRefCount> mCB;
	};

}; // LLAgentWearables

extern LLAgentWearables gAgentWearables;
extern bool gWearablesListDirty;

#endif // LL_AGENTWEARABLES_H
