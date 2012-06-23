/**
 * @file llfasttimer.h
 * @brief Declaration of a fast timer.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLFASTTIMER_H
#define LL_LLFASTTIMER_H

#define FAST_TIMER_ON 1

LL_COMMON_API U64 get_cpu_clock_count();
LL_COMMON_API void assert_main_thread();

class LL_COMMON_API LLFastTimer
{
public:
	enum EFastTimerType
	{
		// high level
		FTM_FRAME,
		FTM_UPDATE,
		FTM_RENDER,
		FTM_GL_FINISH,
		FTM_SWAP,
		FTM_CLIENT_COPY,
		FTM_IDLE,
		FTM_SLEEP,

		// common messaging components
		FTM_PUMP,
		FTM_ARES,
		
		// common simulation components
		FTM_UPDATE_ANIMATION,
		FTM_UPDATE_HIDDEN_ANIMATION,
		FTM_UPDATE_TERRAIN,
		FTM_UPDATE_PRIMITIVES,
		FTM_UPDATE_PARTICLES,
		FTM_SIMULATE_PARTICLES,
		FTM_UPDATE_SKY,
		FTM_UPDATE_TEXTURES,
		FTM_UPDATE_WLPARAM,
		FTM_UPDATE_WATER,
		FTM_UPDATE_CLOUDS,
		FTM_UPDATE_GRASS,
		FTM_UPDATE_TREE,
		FTM_UPDATE_AVATAR,
		
		// common render components
		FTM_BIND_DEFERRED,
		FTM_RENDER_DEFERRED,
		FTM_ATMOSPHERICS,
		FTM_SUN_SHADOW,
		FTM_SOFTEN_SHADOW,
		FTM_EDGE_DETECTION,
		FTM_GI_TRACE,
		FTM_GI_GATHER,
		FTM_LOCAL_LIGHTS,
		FTM_PROJECTORS,
		FTM_FULLSCREEN_LIGHTS,
		FTM_POST,
		FTM_SHADOW_GEOMETRY,
		FTM_SHADOW_RENDER,
		FTM_SHADOW_TERRAIN,
		FTM_SHADOW_AVATAR,
		FTM_SHADOW_SIMPLE,
		FTM_SHADOW_ALPHA,
		FTM_SHADOW_TREE,
		
		FTM_RENDER_GEOMETRY,
		FTM_RENDER_TERRAIN,
		FTM_AVATAR_FACE,
		FTM_RENDER_SIMPLE,
		FTM_RENDER_FULLBRIGHT,
		FTM_RENDER_GLOW,
		FTM_RENDER_GRASS,
		FTM_RENDER_INVISIBLE,
		FTM_RENDER_SHINY,
		FTM_RENDER_BUMP,
		FTM_RENDER_TREES,
		FTM_VOLUME_GEOM,
		FTM_VOLUME_GEOM_PARTIAL,
		FTM_FACE_GET_GEOM,
		FTM_RENDER_CHARACTERS,
		FTM_RENDER_OCCLUSION,
		FTM_RENDER_ALPHA,
        FTM_RENDER_CLOUDS,
		FTM_RENDER_HUD,
		FTM_RENDER_PARTICLES,
		FTM_RENDER_WATER,
		FTM_RENDER_WL_SKY,
		FTM_VISIBLE_CLOUD,
		FTM_RENDER_FAKE_VBO_UPDATE,
		FTM_RENDER_TIMER,
		FTM_RENDER_UI,
		FTM_RENDER_SPELLCHECK,
		FTM_SPELLCHECK_CHECK_WORDS,
		FTM_SPELLCHECK_HIGHLIGHT,
		FTM_SPELL_WORD_BOUNDARIES,
		FTM_SPELL_WORD_UNDERLINE,
		FTM_RENDER_BLOOM,
		FTM_RENDER_BLOOM_FBO,		
		FTM_RENDER_FONTS,
		FTM_RESIZE_SCREEN_TEXTURE,
		
		// newview specific
		FTM_MESSAGES,
		FTM_MOUSEHANDLER,
		FTM_KEYHANDLER,
		FTM_REBUILD,
		FTM_STATESORT,
		FTM_STATESORT_DRAWABLE,
		FTM_STATESORT_POSTSORT,
		FTM_REBUILD_MESH,
		FTM_REBUILD_VBO,
		FTM_REBUILD_VOLUME_VB,
		FTM_REBUILD_TERRAIN_VB,
		FTM_REBUILD_PARTICLE_VB,
		FTM_REBUILD_GRASS_VB,
		FTM_REBUILD_OCCLUSION_VB,
		FTM_POOLS,
		FTM_POOLRENDER,
		FTM_IDLE_CB,
		FTM_IDLE_CB_RADAR,
		FTM_WORLD_UPDATE,
		FTM_UPDATE_MOVE,
		FTM_OCTREE_BALANCE,
		FTM_UPDATE_LIGHTS,
		FTM_CULL,
		FTM_CULL_REBOUND,
		FTM_FRUSTUM_CULL,
		FTM_OCCLUSION_EARLY_FAIL,
		FTM_GEO_UPDATE,
		FTM_GEO_RESERVE,
		FTM_GEO_LIGHT,
		FTM_GEO_SHADOW,
		FTM_GEO_SKY,
		FTM_GEN_VOLUME,
		FTM_GEN_TRIANGLES,
		FTM_GEN_FLEX,
		FTM_AUDIO_UPDATE,
		FTM_RESET_DRAWORDER,
		FTM_OBJECTLIST_UPDATE,
		FTM_AVATAR_UPDATE,
		FTM_JOINT_UPDATE,
		FTM_PHYSICS_UPDATE,
		FTM_ATTACHMENT_UPDATE,
		FTM_LOD_UPDATE,
		FTM_UPDATE_RIGGED_VOLUME,
		FTM_SKIN_RIGGED,
		FTM_SKIN_RIGGED_DEFORM,
		FTM_RIGGED_OCTREE,
		FTM_REGION_UPDATE,
		FTM_CLEANUP,
		FTM_CLEANUP_DRAWABLE,
		FTM_DELETE_FACES,
		FTM_DEREF_DRAWABLE,
		FTM_UNLINK,
		FTM_REMOVE_FROM_HIGHLIGHT_SET,
		FTM_REMOVE_FROM_LIGHT_SET,
		FTM_REMOVE_FROM_MOVE_LIST,
		FTM_REMOVE_FROM_SPATIAL_PARTITION,
		FTM_NETWORK,
		FTM_IDLE_NETWORK,
		FTM_CREATE_OBJECT,
//		FTM_LOAD_AVATAR,
		FTM_PROCESS_MESSAGES,
		FTM_PROCESS_OBJECTS,
		FTM_PROCESS_IMAGES,
		FTM_IMAGE_UPDATE,
		FTM_IMAGE_CREATE,
		FTM_IMAGE_DECODE,
		FTM_IMAGE_READBACK,
		FTM_IMAGE_UPDATE_PRIO,
		FTM_IMAGE_FETCH,
		FTM_IMAGE_MARK_DIRTY,
		FTM_IMAGE_CALLBACKS,
		FTM_IMAGE_STATS,
		FTM_PIPELINE,

//		FTM_VFILE_WAIT,			// Disabled because it would be called from a thread in the mesh repository
		FTM_FLEXIBLE_UPDATE,
		FTM_OCCLUSION_WAIT,
		FTM_OCCLUSION_READBACK,
		FTM_BUILD_OCCLUSION,
		FTM_PUSH_OCCLUSION_VERTS,
		FTM_SET_OCCLUSION_STATE,
		FTM_HUD_EFFECTS,
		FTM_HUD_UPDATE,
		FTM_INVENTORY,
		FTM_AUTO_SELECT,
		FTM_ARRANGE,
		FTM_FILTER,
		FTM_REFRESH,
		FTM_SORT,
		FTM_PICK,
		FTM_TEXTURE_CACHE,
		FTM_DECODE,
		FTM_FETCH,
		FTM_VFS,
		FTM_LFS,
		
#if 0 || !LL_RELEASE_FOR_DOWNLOAD
		// Temp
		FTM_TEMP1,
		FTM_TEMP2,
		FTM_TEMP3,
		FTM_TEMP4,
		FTM_TEMP5,
		FTM_TEMP6,
		FTM_TEMP7,
		FTM_TEMP8,
#endif
		
		FTM_OTHER, // Special, used by display code
		
		FTM_NUM_TYPES
	};
	enum { FTM_HISTORY_NUM = 60 };
	enum { FTM_MAX_DEPTH = 64 };
	
public:
	static EFastTimerType sCurType;

	LLFastTimer(EFastTimerType type)
	{
#if FAST_TIMER_ON
		mType = type;
		sCurType = type;
		// These don't get counted, because they use CPU clockticks
		//gTimerBins[gCurTimerBin]++;
		//LLTimer::sNumTimerCalls++;

		U64 cpu_clocks = get_cpu_clock_count();

		sStart[sCurDepth] = cpu_clocks;
		sCurDepth++;
		assert_main_thread();
#endif
	}

	~LLFastTimer()
	{
#if FAST_TIMER_ON
		U64 end,delta;
		int i;

		// These don't get counted, because they use CPU clockticks
		//gTimerBins[gCurTimerBin]++;
		//LLTimer::sNumTimerCalls++;
		end = get_cpu_clock_count();

		sCurDepth--;
		delta = end - sStart[sCurDepth];
		sCounter[mType] += delta;
		sCalls[mType]++;
		// Subtract delta from parents
		for (i = 0; i < sCurDepth; i++)
		{
			sStart[i] += delta;
		}
		assert_main_thread();
#endif
	}

	static void reset();
	static U64 countsPerSecond();

public:
	static int sCurDepth;
	static U64 sStart[FTM_MAX_DEPTH];
	static U64 sCounter[FTM_NUM_TYPES];
	static U64 sCalls[FTM_NUM_TYPES];
	static U64 sCountAverage[FTM_NUM_TYPES];
	static U64 sCallAverage[FTM_NUM_TYPES];
	static U64 sCountHistory[FTM_HISTORY_NUM][FTM_NUM_TYPES];
	static U64 sCallHistory[FTM_HISTORY_NUM][FTM_NUM_TYPES];
	static S32 sCurFrameIndex;
	static S32 sLastFrameIndex;
	static int sPauseHistory;
	static int sResetHistory;
	static F64 sCPUClockFrequency;
    static U64 sClockResolution;
	
private:
	EFastTimerType mType;
};

#endif // LL_LLFASTTIMER_H
