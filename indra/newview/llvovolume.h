/** 
 * @file llvovolume.h
 * @brief LLVOVolume class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLVOVOLUME_H
#define LL_LLVOVOLUME_H

#include "llapr.h"
#include "llframetimer.h"
#include "m3math.h"		// LLMatrix3
#include "m4math.h"		// LLMatrix4

#include "lldeformerworker.h"
#include "llviewerobject.h"
#include "llviewertexture.h"
#ifdef MEDIA_ON_PRIM
#include "llviewermedia.h"
#endif

#include <map>

class LLViewerTextureAnim;
class LLDrawPool;
class LLSelectNode;
class LLVOAvatar;
class LLMeshSkinInfo;
#ifdef MEDIA_ON_PRIM
class LLObjectMediaDataClient;
class LLObjectMediaNavigateClient;

typedef std::vector<viewer_media_t> media_list_t;
#endif

enum LLVolumeInterfaceType
{
	INTERFACE_FLEXIBLE = 1,
};

class LLRiggedVolume : public LLVolume
{
public:
	LLRiggedVolume(const LLVolumeParams& params)
		: LLVolume(params, 0.f)
	{
	}

	void update(const LLMeshSkinInfo* skin, LLVOAvatar* avatar, const LLVolume* src_volume);
};

const S32 LL_DEFORMER_WEIGHT_COUNT = 3;

class LLDeformedVolume : public LLVolume
{
public:
	LLDeformedVolume()
	:	LLVolume(LLVolumeParams(), 0.f)
	{
	}

	enum EBaseShape
	{
		BASESHAPE_MALE = 0,
		BASESHAPE_FEMALE = 1,
		NUM_BASESHAPES = 2
	};

	// used as a key for the deform table cache
	class LLDeformTableCacheIndex
	{
	public:
		LLDeformTableCacheIndex(S32 shape, const LLUUID& id,
								S32 face, S32 vertex_count)
		:	mBaseShape(shape),
			mID(id),
			mFace(face),
			mVertexCount(vertex_count)
		{
		}

		S32 mBaseShape;
		LLUUID mID;
		S32 mFace;
		S32 mVertexCount;

		bool operator<(const LLDeformTableCacheIndex& lhs) const
		{
			if (mBaseShape < lhs.mBaseShape)
				return TRUE;
			if (mBaseShape > lhs.mBaseShape)
				return FALSE;
			if (mID < lhs.mID)
				return TRUE;
			if (mID > lhs.mID)
				return FALSE;
			if (mFace < lhs.mFace)
				return TRUE;
			if (mFace > lhs.mFace)
				return FALSE;
			if (mVertexCount < lhs.mVertexCount)
				return TRUE;
			
			return FALSE;
		}
	};

	// These are entries in the deformer table.
	// They map vertices on the volume mesh to vertices on the avatar mesh(es).
	struct LLDeformMap
	{
		U8 mMesh[LL_DEFORMER_WEIGHT_COUNT];
		U16 mVertex[LL_DEFORMER_WEIGHT_COUNT];
		F32 mWeight[LL_DEFORMER_WEIGHT_COUNT];
	};

	typedef std::vector<struct LLDeformMap> deform_table_t;
	typedef std::map<LLDeformTableCacheIndex, deform_table_t > deform_cache_t;
	deform_cache_t mDeformCache;

	// apply deformation to source volume and store it in this volume
	bool deform(LLVolume* source, LLVOAvatar* avatar,
				const LLMeshSkinInfo* skin, S32 face,
				LLPointer<LLDrawable> drawable);

	// look-up the cached deform table
	deform_table_t* getDeformTable(LLVolume* source,
								   S32 base_shape,
								   LLVOAvatar* avatar,
								   const LLMeshSkinInfo* skin,
								   S32 face,
								   LLPointer<LLDrawable> drawable);

	// compute the cached deform table
	void computeDeformTable(LLPointer<LLDeformerWorker::Request> request);

	// initialize the base avatar shapes
	static void initializeBaseAvatars();

	// base avatar shapes
	// indexed by: basetype, meshid, vert/index
	static std::vector<std::vector<std::vector<LLVector3> > >	sAvatarPositions;
	static std::vector<std::vector<std::vector<S32> > >			sAvatarIndices;
};

// Base class for implementations of the volume - Primitive, Flexible Object, etc.
class LLVolumeInterface
{
public:
	virtual ~LLVolumeInterface() { }
	virtual LLVolumeInterfaceType getInterfaceType() const = 0;
	virtual BOOL doIdleUpdate(LLAgent &agent, LLWorld &world, const F64 &time) = 0;
	virtual BOOL doUpdateGeometry(LLDrawable *drawable) = 0;
	virtual LLVector3 getPivotPosition() const = 0;
	virtual void onSetVolume(const LLVolumeParams &volume_params, const S32 detail) = 0;
	virtual void onSetScale(const LLVector3 &scale, BOOL damped) = 0;
	virtual void onParameterChanged(U16 param_type, LLNetworkData *data, BOOL in_use, bool local_origin) = 0;
	virtual void onShift(const LLVector4a &shift_vector) = 0;
	virtual bool isVolumeUnique() const = 0; // Do we need a unique LLVolume instance?
	virtual bool isVolumeGlobal() const = 0; // Are we in global space?
	virtual bool isActive() const = 0; // Is this object currently active?
	virtual const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const = 0;
	virtual void updateRelativeXform() = 0;
	virtual U32 getID() const = 0;
	virtual void preRebuild() = 0;
};

// Class which embodies all Volume objects (with pcode LL_PCODE_VOLUME)
class LLVOVolume : public LLViewerObject
{
	LOG_CLASS(LLVOVolume);
protected:
	virtual				~LLVOVolume();

public:
	static		void	initClass();
#ifdef MEDIA_ON_PRIM
	static		void	cleanupClass();
#endif
	static 		void 	preUpdateGeom();
	
	enum 
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD0) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD1) |
							(1 << LLVertexBuffer::TYPE_COLOR)
	};

public:
	LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	/*virtual*/ void markDead(); // Override (and call through to parent)

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);

				void	deleteFaces();

				void	animateTextures();
	/*virtual*/ BOOL	idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	            BOOL    isVisible() const;
	/*virtual*/ BOOL	isActive() const;
	/*virtual*/ BOOL	isAttachment() const;
	/*virtual*/ BOOL	isRootEdit() const; // overridden for sake of attachments treating themselves as a root object
	/*virtual*/ BOOL	isHUDAttachment() const;

				void	generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point);
	/*virtual*/	BOOL	setParent(LLViewerObject* parent);
				S32		getLOD() const							{ return mLOD; }
	const LLVector3		getPivotPositionAgent() const;
	const LLMatrix4&	getRelativeXform() const				{ return mRelativeXform; }
	const LLMatrix3&	getRelativeXformInvTrans() const		{ return mRelativeXformInvTrans; }
				typedef std::map<LLUUID, S32> texture_cost_t;
				U32 	getRenderCost(texture_cost_t& textures) const;
	/*virtual*/	F32		getStreamingCost(S32* bytes = NULL, S32* visible_bytes = NULL, F32* unscaled_value = NULL) const;
	/*virtual*/ U32		getTriangleCount(S32* vcount = NULL) const;
	/*virtual*/ U32		getHighLODTriangleCount();
	/*virtual*/	const LLMatrix4	getRenderMatrix() const;

	/*virtual*/ BOOL lineSegmentIntersect(const LLVector3& start, const LLVector3& end, 
										  S32 face = -1,                        // which face to check, -1 = ALL_SIDES
										  BOOL pick_transparent = FALSE,
										  S32* face_hit = NULL,                 // which face was hit
										  LLVector3* intersection = NULL,       // return the intersection point
										  LLVector2* tex_coord = NULL,          // return the texture coordinates of the intersection point
										  LLVector3* normal = NULL,             // return the surface normal at the intersection point
										  LLVector3* bi_normal = NULL);			// return the surface bi-normal at the intersection point
	
				LLVector3 agentPositionToVolume(const LLVector3& pos) const;
				LLVector3 agentDirectionToVolume(const LLVector3& dir) const;
				LLVector3 volumePositionToAgent(const LLVector3& dir) const;
				LLVector3 volumeDirectionToAgent(const LLVector3& dir) const;

				
				BOOL	getVolumeChanged() const				{ return mVolumeChanged; }
				
	/*virtual*/ F32  	getRadius() const						{ return mVObjRadius; };
				const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const;

				void	markForUpdate(BOOL priority)			{ LLViewerObject::markForUpdate(priority); mVolumeChanged = TRUE; }

	/*virtual*/ void	onShift(const LLVector4a &shift_vector); // Called when the drawable shifts

	/*virtual*/ void	parameterChanged(U16 param_type, bool local_origin);
	/*virtual*/ void	parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin);

	/*virtual*/ U32		processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, const EObjectUpdateType update_type,
											LLDataPacker *dp);

	/*virtual*/ void	setSelected(BOOL sel);
	/*virtual*/ BOOL	setDrawableParent(LLDrawable* parentp);

	/*virtual*/ void	setScale(const LLVector3 &scale, BOOL damped);

#ifdef MEDIA_ON_PRIM
	/*virtual*/ void	setNumTEs(const U8 num_tes);
#endif
	/*virtual*/ void	setTEImage(const U8 te, LLViewerTexture *imagep);
	/*virtual*/ S32		setTETexture(const U8 te, const LLUUID &uuid);
	/*virtual*/ S32		setTEColor(const U8 te, const LLColor3 &color);
	/*virtual*/ S32		setTEColor(const U8 te, const LLColor4 &color);
	/*virtual*/ S32		setTEBumpmap(const U8 te, const U8 bump);
	/*virtual*/ S32		setTEShiny(const U8 te, const U8 shiny);
	/*virtual*/ S32		setTEFullbright(const U8 te, const U8 fullbright);
	/*virtual*/ S32		setTEBumpShinyFullbright(const U8 te, const U8 bump);
	/*virtual*/ S32		setTEMediaFlags(const U8 te, const U8 media_flags);
	/*virtual*/ S32		setTEGlow(const U8 te, const F32 glow);
	/*virtual*/ S32		setTEScale(const U8 te, const F32 s, const F32 t);
	/*virtual*/ S32		setTEScaleS(const U8 te, const F32 s);
	/*virtual*/ S32		setTEScaleT(const U8 te, const F32 t);
	/*virtual*/ S32		setTETexGen(const U8 te, const U8 texgen);
	/*virtual*/ S32		setTEMediaTexGen(const U8 te, const U8 media);
	/*virtual*/ BOOL 	setMaterial(const U8 material);

				void	setTexture(const S32 face);
				S32     getIndexInTex() const		{ return mIndexInTex; }
				void    setIndexInTex(S32 index)	{ mIndexInTex = index; }

	/*virtual*/ BOOL	setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume = false);
				void	updateSculptTexture();
				void	sculpt();
				void	updateRelativeXform();
	/*virtual*/ BOOL	updateGeometry(LLDrawable *drawable);
	/*virtual*/ void	updateFaceSize(S32 idx);
	/*virtual*/ BOOL	updateLOD();
				void	updateRadius();
	/*virtual*/ void	updateTextures();
				void	updateTextureVirtualSize(bool forced = false);

				void	updateFaceFlags();
				void	regenFaces();
				BOOL	genBBoxes(BOOL force_global);
				void	preRebuild();
	virtual		void	updateSpatialExtents(LLVector4a& min, LLVector4a& max);
	virtual		F32		getBinRadius();
	
	virtual U32 getPartitionType() const;

	// For Lights
	void setIsLight(BOOL is_light);
	void setLightColor(const LLColor3& color);
	void setLightIntensity(F32 intensity);
	void setLightRadius(F32 radius);
	void setLightFalloff(F32 falloff);
	void setLightCutoff(F32 cutoff);
	void setLightTextureID(LLUUID id);
	void setSpotLightParams(LLVector3 params);

	BOOL		getIsLight() const;
	LLColor3	getLightBaseColor() const; // not scaled by intensity
	LLColor3	getLightColor() const; // scaled by intensity
	LLUUID		getLightTextureID() const;
	bool		isLightSpotlight() const;
	LLVector3	getSpotLightParams() const;
	void		updateSpotLightPriority();
	F32			getSpotLightPriority() const;

	LLViewerTexture* getLightTexture();
	F32 getLightIntensity() const;
	F32 getLightRadius() const;
	F32 getLightFalloff() const;
	F32 getLightCutoff() const;

	// Flexible Objects
	U32 getVolumeInterfaceID() const;
	virtual BOOL isFlexible() const;
	virtual BOOL isSculpted() const;
	virtual BOOL isMesh() const;
	virtual BOOL hasLightTexture() const;

	BOOL isVolumeGlobal() const;
	BOOL canBeFlexible() const;
	BOOL setIsFlexible(BOOL is_flexible);

#ifdef MEDIA_ON_PRIM
    // Functions that deal with media, or media navigation

    // Update this object's media data with the given media data array
    // (typically this is only called upon a response from a server request)
	void updateObjectMediaData(const LLSD &media_data_array, const std::string &media_version);

    // Bounce back media at the given index to its current URL (or home URL, if current URL is empty)
	void mediaNavigateBounceBack(U8 texture_index);

    // Returns whether or not this object has permission to navigate or control 
	// the given media entry
	enum MediaPermType
	{
		MEDIA_PERM_INTERACT, MEDIA_PERM_CONTROL
	};

    bool hasMediaPermission(const LLMediaEntry* media_entry, MediaPermType perm_type);
 
	void mediaNavigated(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, std::string new_location);
	void mediaEvent(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, LLViewerMediaObserver::EMediaEvent event);

	// Sync the given media data with the impl and the given te
	void syncMediaData(S32 te, const LLSD &media_data, bool merge, bool ignore_agent);

	// Send media data update to the simulator.
	void sendMediaDataUpdate();

	viewer_media_t getMediaImpl(U8 face_id) const;
	S32 getFaceIndexWithMediaImpl(const LLViewerMediaImpl* media_impl, S32 start_face_id);
	F64 getTotalMediaInterest() const;

	bool hasMedia() const;

	LLVector3 getApproximateFaceNormal(U8 face_id);

	// Returns 'true' iff the media data for this object is in flight
	bool isMediaDataBeingFetched() const;

	// Returns the "last fetched" media version, or -1 if not fetched yet
	S32 getLastFetchedMediaVersion() const	{ return mLastFetchedMediaVersion; }

	void addMDCImpl()		{ ++mMDCImplCount; }
	void removeMDCImpl()	{ --mMDCImplCount; }
	S32 getMDCImplCount()	{ return mMDCImplCount; }
#endif

	void notifyMeshLoaded();

	//rigged volume update (for raycasting)
	void updateRiggedVolume();
	LLRiggedVolume* getRiggedVolume();

	//returns true if volume should be treated as a rigged volume
	// - Build tools are open
	// - object is an attachment
	// - object is attached to self
	// - object is rendered as rigged
	bool treatAsRigged();

	//clear out rigged volume and revert back to non-rigged state for picking/LOD/distance updates
	void clearRiggedVolume();

	// deformed volume (rigged attachments) follow avatar morph shape changes
	LLDeformedVolume* getDeformedVolume();

protected:
	S32	computeLODDetail(F32	distance, F32 radius);
	BOOL calcLOD();
	LLFace* addFace(S32 face_index);
	void updateTEData();

#ifdef MEDIA_ON_PRIM
	void requestMediaDataUpdate(bool isNew);
	void cleanUpMediaImpls();
	void addMediaImpl(LLViewerMediaImpl* media_impl, S32 texture_index) ;
	void removeMediaImpl(S32 texture_index) ;
#endif

public:
	LLViewerTextureAnim *mTextureAnimp;
	U8 mTexAnimMode;

private:
	friend class LLDrawable;

	BOOL		mFaceMappingChanged;
	LLFrameTimer mTextureUpdateTimer;
	S32			mLOD;
	BOOL		mLODChanged;
	BOOL		mSculptChanged;
	F32			mSpotLightPriority;
	LLMatrix4	mRelativeXform;
	LLMatrix3	mRelativeXformInvTrans;
	BOOL		mVolumeChanged;
	F32			mVObjRadius;
	LLVolumeInterface *mVolumeImpl;
	LLPointer<LLViewerFetchedTexture> mSculptTexture;
	LLPointer<LLViewerFetchedTexture> mLightTexture;
	S32			mIndexInTex;
#ifdef MEDIA_ON_PRIM
	media_list_t mMediaImplList;
	S32			mLastFetchedMediaVersion; // as fetched from the server, starts as -1
	S32			mMDCImplCount;
#endif

	LLPointer<LLRiggedVolume> mRiggedVolume;
	LLPointer<LLDeformedVolume> mDeformedVolume;

	// statics
public:
	static F32 sLODSlopDistanceFactor;// Changing this to zero, effectively disables the LOD transition slop 
	static F32 sLODFactor;				// LOD scale factor
	static F32 sDistanceFactor;			// LOD distance factor

#ifdef MEDIA_ON_PRIM
	static LLPointer<LLObjectMediaDataClient> sObjectMediaClient;
	static LLPointer<LLObjectMediaNavigateClient> sObjectMediaNavigateClient;
#endif

	static const U32 ARC_TEXTURE_COST = 5;

protected:
	static S32 sNumLODChanges;

	friend class LLVolumeImplFlexible;
};

#endif // LL_LLVOVOLUME_H
