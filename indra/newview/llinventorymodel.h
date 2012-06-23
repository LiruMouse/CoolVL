/** 
 * @file llinventorymodel.h
 * @brief LLInventoryModel class header file
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

#ifndef LL_LLINVENTORYMODEL_H
#define LL_LLINVENTORYMODEL_H

#include "llassettype.h"
#include "llfoldertype.h"
#include "lldarray.h"
#include "lluuid.h"
#include "llpermissionsflags.h"
#include "llstring.h"

#include <map>
#include <set>
#include <string>
#include <vector>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryObserver
//
// This class is designed to be a simple abstract base class which can
// relay messages when the inventory changes.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryObserver
{
public:
	// This enumeration is a way to refer to what changed in a more
	// human readable format. You can mask the value provided by
	// chaged() to see if the observer is interested in the change.
	enum 
	{
		NONE = 0,
		LABEL = 1,			// name changed
		INTERNAL = 2,		// internal change, eg, asset uuid different
		ADD = 4,			// something added
		REMOVE = 8,			// something deleted
		STRUCTURE = 16,		// structural change, eg, item or folder moved
		CALLING_CARD = 32,	// online, grant status, cancel, etc change
		REBUILD = 128,		// icon changed, for example. Rebuild all.
		ALL = 0xffffffff
	};
	virtual ~LLInventoryObserver() {};
	virtual void changed(U32 mask) = 0;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryModel
//
// This class represents a collection of inventory, and provides
// efficient ways to access that information. This class could in
// theory be used for any place where you need inventory, though it
// optimizes for time efficiency - not space efficiency, probably
// making it inappropriate for use on tasks.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class LLInventoryObject;
class LLInventoryItem;
class LLInventoryCategory;
class LLViewerInventoryItem;
class LLViewerInventoryCategory;
class LLViewerInventoryItem;
class LLViewerInventoryCategory;
class LLMessageSystem;
class LLInventoryCollectFunctor;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLInventoryModel
//
// Represents a collection of inventory, and provides efficient ways to access 
// that information.
//   NOTE: This class could in theory be used for any place where you need 
//   inventory, though it optimizes for time efficiency - not space efficiency, 
//   probably making it inappropriate for use on tasks.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryModel
{
public:
	friend class LLInventoryModelFetchDescendentsResponder;

	enum EHasChildren
	{
		CHILDREN_NO,
		CHILDREN_YES,
		CHILDREN_MAYBE
	};

	typedef LLDynamicArray<LLPointer<LLViewerInventoryCategory> > cat_array_t;
	typedef LLDynamicArray<LLPointer<LLViewerInventoryItem> > item_array_t;
	typedef std::set<LLUUID> changed_items_t;

	class fetchInventoryResponder: public LLHTTPClient::Responder
	{
	public:
		fetchInventoryResponder(const LLSD& request_sd) : mRequestSD(request_sd) {};
		void result(const LLSD& content);
		void error(U32 status, const std::string& reason);

		typedef std::vector<LLViewerInventoryCategory*> folder_ref_t;

	protected:
		LLSD mRequestSD;
	};

	//--------------------------------------------------------------------
	// Constructors / Destructors
	//--------------------------------------------------------------------
public:
	LLInventoryModel();
	~LLInventoryModel();
	void cleanupInventory();

protected:
	// Empty the entire contents
	void empty();

	//--------------------------------------------------------------------
	// Initialization
	//--------------------------------------------------------------------
public:
	// The inventory model usage is sensitive to the initial construction of the model
	bool isInventoryUsable() const;

private:
	bool mIsAgentInvUsable; // used to handle an invalid inventory state

	//--------------------------------------------------------------------
	// Root Folders
	//--------------------------------------------------------------------
public:
	// The following are set during login with data from the server
	void setRootFolderID(const LLUUID& id);
	void setLibraryOwnerID(const LLUUID& id);
	void setLibraryRootFolderID(const LLUUID& id);

	const LLUUID &getRootFolderID() const;
	const LLUUID &getLibraryOwnerID() const;
	const LLUUID &getLibraryRootFolderID() const;

private:
	LLUUID mRootFolderID;
	LLUUID mLibraryRootFolderID;
	LLUUID mLibraryOwnerID;

	//--------------------------------------------------------------------
	// Structure
	//--------------------------------------------------------------------
public:
	// Methods to load up inventory skeleton & meat. These are used
	// during authentication. Returns true if everything parsed.
	bool loadSkeleton(const LLSD& options, const LLUUID& owner_id);
	// Legacy method for old XML-RPC login (to remove once llstartup.cpp is
	// updated to v2/3 viewer code).
	typedef std::map<std::string, std::string> response_t;
	typedef std::vector<response_t> options_t;
	bool loadSkeleton(const options_t& options, const LLUUID& owner_id);

	// Brute force method to rebuild the entire parent-child relations.
	void buildParentChildMap();

	// Call on logout to save a terse representation.
	void cache(const LLUUID& parent_folder_id, const LLUUID& agent_id);

private:
	// Information for tracking the actual inventory. We index this
	// information in a lot of different ways so we can access
	// the inventory using several different identifiers.
	// mCategoryMap and mItemMap store uuid->object mappings. 
	typedef std::map<LLUUID, LLPointer<LLViewerInventoryCategory> > cat_map_t;
	typedef std::map<LLUUID, LLPointer<LLViewerInventoryItem> > item_map_t;
	cat_map_t mCategoryMap;
	item_map_t mItemMap;
	// This last set of indices is used to map parents to children.
	typedef std::map<LLUUID, cat_array_t*> parent_cat_map_t;
	typedef std::map<LLUUID, item_array_t*> parent_item_map_t;
	parent_cat_map_t mParentChildCategoryTree;
	parent_item_map_t mParentChildItemTree;


	//--------------------------------------------------------------------
	// Login
	//--------------------------------------------------------------------
private:
	const static S32 sCurrentInvCacheVersion; // expected inventory cache version

	//
	// Accessors
	//

	//--------------------------------------------------------------------
	// Descendents
	//--------------------------------------------------------------------
public:
	// Make sure we have the descendents in the structure.
	void fetchDescendentsOf(const LLUUID& folder_id);

	// Return the direct descendents of the id provided.Set passed
	// in values to NULL if the call fails.
	//    NOTE: The array provided points straight into the guts of
	//    this object, and should only be used for read operations, since
	//    modifications may invalidate the internal state of the inventory.
	void getDirectDescendentsOf(const LLUUID& cat_id,
								cat_array_t*& categories,
								item_array_t*& items) const;

	// Starting with the object specified, add its descendents to the
	// array provided, but do not add the inventory object specified
	// by id. There is no guaranteed order. 
	//    NOTE: Neither array will be erased before adding objects to it. 
	//    Do not store a copy of the pointers collected - use them, and 
	//    collect them again later if you need to reference the same objects.
	enum {
		EXCLUDE_TRASH = FALSE,
		INCLUDE_TRASH = TRUE
	};
	void collectDescendents(const LLUUID& id,
							cat_array_t& categories,
							item_array_t& items,
							BOOL include_trash);

	void collectDescendentsIf(const LLUUID& id,
							  cat_array_t& categories,
							  item_array_t& items,
							  BOOL include_trash,
							  LLInventoryCollectFunctor& add);

	// Check if one object has a parent chain up to the category specified by UUID.
	BOOL isObjectDescendentOf(const LLUUID& obj_id, const LLUUID& cat_id);

	//--------------------------------------------------------------------
	// Find
	//--------------------------------------------------------------------
public:
	// Returns the uuid of the category that specifies 'type' as what it
	// defaults to containing. The category is not necessarily only for that type.
	// NOTE: If create_folder is true, this will create a new inventory category
	// on the fly if one does not exist.
	LLUUID findCategoryUUIDForType(LLFolderType::EType preferred_type,
								   bool create_folder = true);

	// Get first descendant of the child object under the specified parent
	const LLViewerInventoryCategory *getFirstDescendantOf(const LLUUID& master_parent_id,
														  const LLUUID& obj_id) const;

	// Get the object by id. Returns NULL if not found.
	//   NOTE: Use the pointer returned for read operations - do
	//   not modify the object values in place or you will break stuff.
	LLInventoryObject* getObject(const LLUUID& id) const;

	// Get the item by id. Returns NULL if not found.
	//    NOTE: Use the pointer for read operations - use the
	//    updateItem() method to actually modify values.
	LLViewerInventoryItem* getItem(const LLUUID& id) const;

	// Get the category by id. Returns NULL if not found.
	//    NOTE: Use the pointer for read operations - use the
	//    updateCategory() method to actually modify values.
	LLViewerInventoryCategory* getCategory(const LLUUID& id) const;

	// Get the inventoryID that this item points to, else just return item_id
	const LLUUID& getLinkedItemID(const LLUUID& object_id) const;

protected:
	// Internal method which looks for a category with the specified
	// preferred type. Returns LLUUID::null if not found
 	LLUUID findCatUUID(LLFolderType::EType preferred_type);

private:
	mutable LLPointer<LLViewerInventoryItem> mLastItem; // cache recent lookups

	//--------------------------------------------------------------------
	// Count
	//--------------------------------------------------------------------
public:
	// Return the number of items or categories
	S32 getItemCount() const;
	S32 getCategoryCount() const;

	//
	// Mutators
	//
public:
	// Change an existing item with a matching item_id or add the item
	// to the current inventory. Returns the change mask generated by
	// the update. No notification will be sent to observers. This
	// method will only generate network traffic if the item had to be
	// reparented.
	//    NOTE: In usage, you will want to perform cache accounting
	//    operations in LLInventoryModel::accountForUpdate() or
	//    LLViewerInventoryItem::updateServer() before calling this method.
	U32 updateItem(const LLViewerInventoryItem* item);

	// Change an existing item with the matching id or add
	// the category. No notifcation will be sent to observers. This
	// method will only generate network traffic if the item had to be
	// reparented.
	//    NOTE: In usage, you will want to perform cache accounting
	//    operations in accountForUpdate() or LLViewerInventoryCategory::
	//    updateServer() before calling this method.
	void updateCategory(const LLViewerInventoryCategory* cat);

	// Move the specified object id to the specified category and
	// update the internal structures. No cache accounting,
	// observer notification, or server update is performed.
	void moveObject(const LLUUID& object_id, const LLUUID& cat_id);

	//--------------------------------------------------------------------
	// Delete
	//--------------------------------------------------------------------
public:
	// Delete a particular inventory object by ID. Will purge one
	// object from the internal data structures, maintaining a
	// consistent internal state. No cache accounting, observer
	// notification, or server update is performed.
	void deleteObject(const LLUUID& id);
	void removeItem(const LLUUID& item_id);

	// Collects and purges the descendants of the id
	// provided. If the category is not found, no action is
	// taken. This method goes through the long winded process of
	// removing server representation of folders and items while doing
	// cache accounting in a fairly efficient manner. This method does
	// not notify observers (though maybe it should...)
	void purgeDescendentsOf(const LLUUID& id);

	//--------------------------------------------------------------------
	// Creation
	//--------------------------------------------------------------------
public:
	// Returns the UUID of the new category. If you want to use the default 
	// name based on type, pass in a NULL to the 'name' parameter.
	LLUUID createNewCategory(const LLUUID& parent_id,
							 LLFolderType::EType preferred_type,
							 const std::string& name,
							 void (*callback)(const LLSD&, void*) = NULL,
							 void* user_data = NULL);

protected:
	// Internal methods that add inventory and make sure that all of
	// the internal data structures are consistent. These methods
	// should be passed pointers of newly created objects, and the
	// instance will take over the memory management from there.
	void addCategory(LLViewerInventoryCategory* category);
	void addItem(LLViewerInventoryItem* item);

	//
	// Category accounting.
	//
public:
	// Represents the number of items added or removed from a category.
	struct LLCategoryUpdate
	{
		LLCategoryUpdate() : mDescendentDelta(0) {}
		LLCategoryUpdate(const LLUUID& category_id, S32 delta)
		:	mCategoryID(category_id),
			mDescendentDelta(delta) {}
		LLUUID mCategoryID;
		S32 mDescendentDelta;
	};
	typedef std::vector<LLCategoryUpdate> update_list_t;

	// This exists to make it easier to account for deltas in a map.
	struct LLInitializedS32
	{
		LLInitializedS32() : mValue(0) {}
		LLInitializedS32(S32 value) : mValue(value) {}
		S32 mValue;
		LLInitializedS32& operator++() { ++mValue; return *this; }
		LLInitializedS32& operator--() { --mValue; return *this; }
	};
	typedef std::map<LLUUID, LLInitializedS32> update_map_t;

	// Call these methods when there are category updates, but call
	// them *before* the actual update so the method can do descendent
	// accounting correctly.
	void accountForUpdate(const LLCategoryUpdate& update);
	void accountForUpdate(const update_list_t& updates);
	void accountForUpdate(const update_map_t& updates);

	// Return (yes/no/maybe) child status of category children.
	EHasChildren categoryHasChildren(const LLUUID& cat_id) const;

	// Returns true if category version is known and theoretical
	// descendents == actual descendents.
	bool isCategoryComplete(const LLUUID& cat_id) const;

	//
	// Notifications
	//
public:
	// Called by the idle loop.  Only updates if new state is detected.  Call 
	// notifyObservers() manually to update regardless of whether state change 
	// has been indicated.
	void idleNotifyObservers();

	// Call to explicitly update everyone on a new state.
	void notifyObservers();

	// Allows outsiders to tell the inventory if something has
	// been changed 'under the hood', but outside the control of the
	// inventory. The next notify will include that notification.
	void addChangedMask(U32 mask, const LLUUID& referent);
	const changed_items_t& getChangedIDs() { return mChangedItemIDs; }

protected:
	// Updates all linked items pointing to this id.
	void addChangedMaskForLinks(const LLUUID& object_id, U32 mask);

private:
	// Flag set when notifyObservers is being called, to look for bugs
	// where it's called recursively.
	bool mIsNotifyObservers;
	// Variables used to track what has changed since the last notify.
	U32 mModifyMask;
	changed_items_t mChangedItemIDs;


	//--------------------------------------------------------------------
	// Observers
	//--------------------------------------------------------------------
public:
	// If the observer is destroyed, be sure to remove it.
	void addObserver(LLInventoryObserver* observer);
	void removeObserver(LLInventoryObserver* observer);
	BOOL containsObserver(LLInventoryObserver* observer);

private:
	typedef std::set<LLInventoryObserver*> observer_list_t;
	observer_list_t mObservers;

	//
	// Misc Methods 
	//

	//--------------------------------------------------------------------
	// Callbacks
	//--------------------------------------------------------------------
public:
	// message handling functionality
	static void registerCallbacks(LLMessageSystem* msg);

	//--------------------------------------------------------------------
	// File I/O
	//--------------------------------------------------------------------
protected:
	static bool loadFromFile(const std::string& filename,
							 cat_array_t& categories,
							 item_array_t& items,
							 bool& is_cache_obsolete); 
	static bool saveToFile(const std::string& filename,
						   const cat_array_t& categories,
						   const item_array_t& items); 

	//--------------------------------------------------------------------
	// Message handling functionality
	//--------------------------------------------------------------------
public:
	static void processUpdateCreateInventoryItem(LLMessageSystem* msg, void**);
	static void removeInventoryItem(LLUUID agent_id, LLMessageSystem* msg, const char* msg_label);
	static void processRemoveInventoryItem(LLMessageSystem* msg, void**);
	static void processUpdateInventoryFolder(LLMessageSystem* msg, void**);
	static void removeInventoryFolder(LLUUID agent_id, LLMessageSystem* msg);
	static void processRemoveInventoryFolder(LLMessageSystem* msg, void**);
	static void processRemoveInventoryObjects(LLMessageSystem* msg, void**);
	static void processSaveAssetIntoInventory(LLMessageSystem* msg, void**);
	static void processBulkUpdateInventory(LLMessageSystem* msg, void**);
	static void processInventoryDescendents(LLMessageSystem* msg, void**);
	static void processMoveInventoryItem(LLMessageSystem* msg, void**);
	static void processFetchInventoryReply(LLMessageSystem* msg, void**);

protected:
	bool messageUpdateCore(LLMessageSystem* msg, bool do_accounting);

	//--------------------------------------------------------------------
	// Locking
	//--------------------------------------------------------------------
public:
	void lockDirectDescendentArrays(const LLUUID& cat_id,
									cat_array_t*& categories,
									item_array_t*& items);
	void unlockDirectDescendentArrays(const LLUUID& cat_id);

protected:
	cat_array_t* getUnlockedCatArray(const LLUUID& id);
	item_array_t* getUnlockedItemArray(const LLUUID& id);

private:
	std::map<LLUUID, bool> mCategoryLock;
	std::map<LLUUID, bool> mItemLock;

	//--------------------------------------------------------------------
	// Debugging
	//--------------------------------------------------------------------
public:
	void dumpInventory();

	//--------------------------------------------------------------------
	// Other
	//--------------------------------------------------------------------
public:
	// Generates a string containing the path to the item specified by
	// item_id.
	void appendPath(const LLUUID& id, std::string& path);
};

// a special inventory model for the agent
extern LLInventoryModel gInventory;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCollectFunctor
//
// Base class for LLInventoryModel::collectDescendentsIf() method
// which accepts an instance of one of these objects to use as the
// function to determine if it should be added. Derive from this class
// and override the () operator to return TRUE if you want to collect
// the category or item passed in.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryCollectFunctor
{
public:
	virtual ~LLInventoryCollectFunctor(){};
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item) = 0;

	static bool itemTransferCommonlyAllowed(LLInventoryItem* item);
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLAssetIDMatches
//
// This functor finds inventory items pointing to the specified asset
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLViewerInventoryItem;

class LLAssetIDMatches : public LLInventoryCollectFunctor
{
public:
	LLAssetIDMatches(const LLUUID& asset_id) : mAssetID(asset_id) {}
	virtual ~LLAssetIDMatches() {}
	bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);

protected:
	LLUUID mAssetID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLLinkedItemIDMatches
//
// This functor finds inventory items linked to the specific inventory id.
// Assumes the inventory id is itself not a linked item.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLLinkedItemIDMatches : public LLInventoryCollectFunctor
{
public:
	LLLinkedItemIDMatches(const LLUUID& item_id) : mBaseItemID(item_id) {}
	virtual ~LLLinkedItemIDMatches() {}
	bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);

protected:
	LLUUID mBaseItemID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIsType
//
// Implementation of a LLInventoryCollectFunctor which returns TRUE if
// the type is the type passed in during construction.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLIsType : public LLInventoryCollectFunctor
{
public:
	LLIsType(LLAssetType::EType type) : mType(type) {}
	virtual ~LLIsType() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLAssetType::EType mType;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIsNotType
//
// Implementation of a LLInventoryCollectFunctor which returns FALSE if the
// type is the type passed in during construction, otherwise false.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLIsNotType : public LLInventoryCollectFunctor
{
public:
	LLIsNotType(LLAssetType::EType type) : mType(type) {}
	virtual ~LLIsNotType() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLAssetType::EType mType;
};

class LLIsTypeWithPermissions : public LLInventoryCollectFunctor
{
public:
	LLIsTypeWithPermissions(LLAssetType::EType type, const PermissionBit perms, const LLUUID &agent_id, const LLUUID &group_id) 
		: mType(type), mPerm(perms), mAgentID(agent_id), mGroupID(group_id) {}
	virtual ~LLIsTypeWithPermissions() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLAssetType::EType mType;
	PermissionBit mPerm;
	LLUUID			mAgentID;
	LLUUID			mGroupID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIsClone
//
// Implementation of a LLInventoryCollectFunctor which returns TRUE if
// the object is a clone of the item passed in during
// construction.
//
// *NOTE: Since clone information is determined based off of asset id
// (or creator with calling cards), if the id is NULL, it has no
// clones - even itself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//class LLIsClone : public LLInventoryCollectFunctor
//{
//public:
//	LLIsClone(LLViewerInventoryItem* item) : mItem(item) {}
//	virtual ~LLIsClone() {}
//	virtual bool operator()(LLViewerInventoryCategory* cat,
//							LLViewerInventoryItem* item);
//protected:
//	LLPointer<LLViewerInventoryItem> mItem;
//};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLBuddyCollector
//
// Simple class that collects calling cards that are not null, and not
// the agent. Duplicates are possible.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLBuddyCollector : public LLInventoryCollectFunctor
{
public:
	LLBuddyCollector() {}
	virtual ~LLBuddyCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLUniqueBuddyCollector
//
// Simple class that collects calling cards that are not null, and not
// the agent. Duplicates are discarded.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLUniqueBuddyCollector : public LLInventoryCollectFunctor
{
public:
	LLUniqueBuddyCollector() {}
	virtual ~LLUniqueBuddyCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);

protected:
	std::set<LLUUID> mSeen;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLParticularBuddyCollector
//
// Simple class that collects calling cards that match a particular uuid
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLParticularBuddyCollector : public LLInventoryCollectFunctor
{
public:
	LLParticularBuddyCollector(const LLUUID& id) : mBuddyID(id) {}
	virtual ~LLParticularBuddyCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	LLUUID mBuddyID;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLNameCategoryCollector
//
// Collects categories based on case-insensitive match of prefix
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLNameCategoryCollector : public LLInventoryCollectFunctor
{
public:
	LLNameCategoryCollector(const std::string& name) : mName(name) {}
	virtual ~LLNameCategoryCollector() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
protected:
	std::string mName;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryCompletionObserver
//
// Class which can be used as a base class for doing something when
// when all observed items are locally complete. This class implements
// the changed() method of LLInventoryObserver and declares a new
// method named done() which is called when all watched items have
// complete information in the inventory model.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryCompletionObserver : public LLInventoryObserver
{
public:
	LLInventoryCompletionObserver() {}
	virtual void changed(U32 mask);

	void watchItem(const LLUUID& id);

protected:
	virtual void done() = 0;

	typedef std::vector<LLUUID> item_ref_t;
	item_ref_t mComplete;
	item_ref_t mIncomplete;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchObserver
//
// This class is much like the LLInventoryCompletionObserver, except
// that it handles all the the fetching necessary. Override the done()
// method to do the thing you want.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryFetchObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchObserver() {}
	virtual void changed(U32 mask);

	typedef std::vector<LLUUID> item_ref_t;

	bool isFinished() const;
	void fetchItems(const item_ref_t& ids);
	virtual void done() = 0;

protected:
	item_ref_t mComplete;
	item_ref_t mIncomplete;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchDescendentsObserver
//
// This class is much like the LLInventoryCompletionObserver, except
// that it handles fetching based on category. Override the done()
// method to do the thing you want.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchDescendentsObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchDescendentsObserver() {}
	virtual void changed(U32 mask);

	typedef std::vector<LLUUID> folder_ref_t;
	void fetchDescendents(const folder_ref_t& ids);
	bool isFinished() const;
	virtual void done() = 0;

protected:
	bool isCategoryComplete(LLViewerInventoryCategory* cat);
	folder_ref_t mIncompleteFolders;
	folder_ref_t mCompleteFolders;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFetchComboObserver
//
// This class does an appropriate combination of fetch descendents and
// item fetches based on completion of categories and items. Much like
// the fetch and fetch descendents, this will call done() when everything
// has arrived.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFetchComboObserver : public LLInventoryObserver
{
public:
	LLInventoryFetchComboObserver() : mDone(false) {}
	virtual void changed(U32 mask);

	typedef std::vector<LLUUID> folder_ref_t;
	typedef std::vector<LLUUID> item_ref_t;
	void fetch(const folder_ref_t& folder_ids, const item_ref_t& item_ids);

	virtual void done() = 0;

protected:
	bool mDone;
	folder_ref_t mCompleteFolders;
	folder_ref_t mIncompleteFolders;
	item_ref_t mCompleteItems;
	item_ref_t mIncompleteItems;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryExistenceObserver
//
// This class is used as a base class for doing somethign when all the
// observed item ids exist in the inventory somewhere. You can derive
// a class from this class and implement the done() method to do
// something useful.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryExistenceObserver : public LLInventoryObserver
{
public:
	LLInventoryExistenceObserver() {}
	virtual void changed(U32 mask);

	void watchItem(const LLUUID& id);

protected:
	virtual void done() = 0;

	typedef std::vector<LLUUID> item_ref_t;
	item_ref_t mExist;
	item_ref_t mMIA;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryAddedObserver
//
// This class is used as a base class for doing something when 
// a new item arrives in inventory.
// It does not watch for a certain UUID, rather it acts when anything is added
// Derive a class from this class and implement the done() method to do
// something useful.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryAddedObserver : public LLInventoryObserver
{
public:
	LLInventoryAddedObserver() : mAdded() {}
	virtual void changed(U32 mask);

protected:
	virtual void done() = 0;

	typedef std::vector<LLUUID> item_ref_t;
	item_ref_t mAdded;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryTransactionObserver
//
// Class which can be used as a base class for doing something when an
// inventory transaction completes.
//
// *NOTE: This class is not quite complete. Avoid using unless you fix up it's
// functionality gaps.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryTransactionObserver : public LLInventoryObserver
{
public:
	LLInventoryTransactionObserver(const LLTransactionID& transaction_id);
	virtual void changed(U32 mask);

protected:
	typedef std::vector<LLUUID> folder_ref_t;
	typedef std::vector<LLUUID> item_ref_t;
	virtual void done(const folder_ref_t& folders, const item_ref_t& items) = 0;

	LLTransactionID mTransactionID;
};

#endif // LL_LLINVENTORYMODEL_H
