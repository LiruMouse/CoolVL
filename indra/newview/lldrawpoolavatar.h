/** 
 * @file lldrawpoolavatar.h
 * @brief LLDrawPoolAvatar class definition
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

#ifndef LL_LLDRAWPOOLAVATAR_H
#define LL_LLDRAWPOOLAVATAR_H

#include "lldrawpool.h"

class LLVOAvatar;
class LLGLSLShader;
class LLFace;
class LLMeshSkinInfo;
class LLVolume;
class LLVolumeFace;
class LLVOVolume;

class LLDrawPoolAvatar : public LLFacePool
{
public:
	enum
	{
		SHADER_LEVEL_BUMP = 2,
		SHADER_LEVEL_CLOTH = 3
	};
	
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_WEIGHT |
							LLVertexBuffer::MAP_CLOTHWEIGHT
	};

	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	virtual S32 getVertexShaderLevel() const;

	LLDrawPoolAvatar();

	static LLMatrix4& getModelView();

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ S32  getNumPasses();
	/*virtual*/ void beginRenderPass(S32 pass);
	/*virtual*/ void endRenderPass(S32 pass);
	/*virtual*/ void prerender();
	/*virtual*/ void render(S32 pass = 0);

	/*virtual*/ S32 getNumDeferredPasses();
	/*virtual*/ void beginDeferredPass(S32 pass);
	/*virtual*/ void endDeferredPass(S32 pass);
	/*virtual*/ void renderDeferred(S32 pass);

	/*virtual*/ S32 getNumPostDeferredPasses();
	/*virtual*/ void beginPostDeferredPass(S32 pass);
	/*virtual*/ void endPostDeferredPass(S32 pass);
	/*virtual*/ void renderPostDeferred(S32 pass);

	/*virtual*/ S32 getNumShadowPasses();
	/*virtual*/ void beginShadowPass(S32 pass);
	/*virtual*/ void endShadowPass(S32 pass);
	/*virtual*/ void renderShadow(S32 pass);

	void beginRigid();
	void beginImpostor();
	void beginSkinned();
		
	void endRigid();
	void endImpostor();
	void endSkinned();

	void beginDeferredImpostor();
	void beginDeferredRigid();
	void beginDeferredSkinned();
	
	void endDeferredImpostor();
	void endDeferredRigid();
	void endDeferredSkinned();
	
	void beginPostDeferredAlpha();
	void endPostDeferredAlpha();

	void beginRiggedSimple();
	void beginRiggedFullbright();
	void beginRiggedFullbrightShiny();
	void beginRiggedShinySimple();
	void beginRiggedAlpha();
	void beginRiggedFullbrightAlpha();
	void beginRiggedGlow();
	void beginDeferredRiggedAlpha();

	void endRiggedSimple();
	void endRiggedFullbright();
	void endRiggedFullbrightShiny();
	void endRiggedShinySimple();
	void endRiggedAlpha();
	void endRiggedFullbrightAlpha();
	void endRiggedGlow();
	void endDeferredRiggedAlpha();

	void beginDeferredRiggedSimple();
	void beginDeferredRiggedBump();
	
	void endDeferredRiggedSimple();
	void endDeferredRiggedBump();
		
	void updateRiggedFaceVertexBuffer(LLVOAvatar* avatar,
									  LLFace* facep, 
									  const LLMeshSkinInfo* skin, 
									  LLVolume* volume,
									  const LLVolumeFace& vol_face,
									  LLVOVolume* vobj);

	void renderRigged(LLVOAvatar* avatar, U32 type, bool glow = false);
	void renderRiggedSimple(LLVOAvatar* avatar);
	void renderRiggedAlpha(LLVOAvatar* avatar);
	void renderRiggedFullbrightAlpha(LLVOAvatar* avatar);
	void renderRiggedFullbright(LLVOAvatar* avatar);
	void renderRiggedShinySimple(LLVOAvatar* avatar);
	void renderRiggedFullbrightShiny(LLVOAvatar* avatar);
	void renderRiggedGlow(LLVOAvatar* avatar);
	void renderDeferredRiggedSimple(LLVOAvatar* avatar);
	void renderDeferredRiggedBump(LLVOAvatar* avatar);

	typedef enum
	{
		RIGGED_SIMPLE = 0,
		RIGGED_FULLBRIGHT,
		RIGGED_SHINY,
		RIGGED_FULLBRIGHT_SHINY,
		RIGGED_GLOW,
		RIGGED_ALPHA,
		RIGGED_FULLBRIGHT_ALPHA,
		RIGGED_DEFERRED_BUMP,
		RIGGED_DEFERRED_SIMPLE,
		NUM_RIGGED_PASSES,
		RIGGED_UNKNOWN,
	} eRiggedPass;

	typedef enum
	{
		RIGGED_SIMPLE_MASK =			LLVertexBuffer::MAP_VERTEX | 
										LLVertexBuffer::MAP_NORMAL | 
										LLVertexBuffer::MAP_TEXCOORD0 |
										LLVertexBuffer::MAP_COLOR |
										LLVertexBuffer::MAP_WEIGHT4,

		RIGGED_FULLBRIGHT_MASK =		LLVertexBuffer::MAP_VERTEX | 
										LLVertexBuffer::MAP_TEXCOORD0 |
										LLVertexBuffer::MAP_COLOR |
										LLVertexBuffer::MAP_WEIGHT4,

		RIGGED_SHINY_MASK =				RIGGED_SIMPLE_MASK,

		RIGGED_FULLBRIGHT_SHINY_MASK =	RIGGED_SIMPLE_MASK,

		RIGGED_GLOW_MASK =				LLVertexBuffer::MAP_VERTEX | 
										LLVertexBuffer::MAP_TEXCOORD0 |
										LLVertexBuffer::MAP_WEIGHT4,

		RIGGED_ALPHA_MASK =				RIGGED_SIMPLE_MASK,

		RIGGED_FULLBRIGHT_ALPHA_MASK =	RIGGED_FULLBRIGHT_MASK,

		RIGGED_DEFERRED_BUMP_MASK =		LLVertexBuffer::MAP_VERTEX | 
										LLVertexBuffer::MAP_NORMAL | 
										LLVertexBuffer::MAP_TEXCOORD0 |
										LLVertexBuffer::MAP_BINORMAL |
										LLVertexBuffer::MAP_COLOR |
										LLVertexBuffer::MAP_WEIGHT4,

		RIGGED_DEFERRED_SIMPLE_MASK =	LLVertexBuffer::MAP_VERTEX | 
										LLVertexBuffer::MAP_NORMAL | 
										LLVertexBuffer::MAP_TEXCOORD0 |
										LLVertexBuffer::MAP_COLOR |
										LLVertexBuffer::MAP_WEIGHT4,
	} eRiggedDataMask;

	void addRiggedFace(LLFace* facep, U32 type);
	void removeRiggedFace(LLFace* facep); 

	std::vector<LLFace*> mRiggedFace[NUM_RIGGED_PASSES];
		
	/*virtual*/ LLViewerTexture *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	void renderAvatars(LLVOAvatar *single_avatar, S32 pass = -1); // renders only one avatar if single_avatar is not null.

	static BOOL sSkipOpaque;
	static BOOL sSkipTransparent;
	static S32 sDiffuseChannel;

	static LLGLSLShader* sVertexProgram;
};

class LLVertexBufferAvatar : public LLVertexBuffer
{
public:
	LLVertexBufferAvatar();
	virtual void setupVertexBuffer(U32 data_mask) const;
};

extern S32 AVATAR_OFFSET_POS;
extern S32 AVATAR_OFFSET_NORMAL;
extern S32 AVATAR_OFFSET_TEX0;
extern S32 AVATAR_OFFSET_TEX1;
extern S32 AVATAR_VERTEX_BYTES;
const S32 AVATAR_BUFFER_ELEMENTS = 8192; // Needs to be enough to store all avatar vertices.

extern BOOL gAvatarEmbossBumpMap;
#endif // LL_LLDRAWPOOLAVATAR_H
