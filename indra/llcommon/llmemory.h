/**
 * @file llmemory.h
 * @brief Memory allocation/deallocation header-stuff goes here.
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
#ifndef LL_MEMORY_H
#define LL_MEMORY_H

#include <string.h>		// for memcpy()
#if !LL_WINDOWS
#include <stdint.h>
#endif

#include "llmemtype.h"

///////////////////////////////////////////////////////////////////////////////
// FIXME: LL_USE_TCMALLOC should be positionned by cmake but it currently fails
#ifdef LL_USE_TCMALLOC
#undef LL_USE_TCMALLOC
#endif
// Hard-code this for now...
#define LL_USE_TCMALLOC 1
///////////////////////////////////////////////////////////////////////////////

#if LL_USE_TCMALLOC

// ll_aligned_foo_16 are not needed with tcmalloc
#define ll_aligned_malloc_16 malloc
#define ll_aligned_realloc_16 realloc
#define ll_aligned_free_16 free

#else // LL_USE_TCMALLOC

// returned hunk MUST be freed with ll_aligned_free_16().
inline void* ll_aligned_malloc_16(size_t size)
{
#if defined(LL_WINDOWS)
	return _aligned_malloc(size, 16);
#elif defined(LL_DARWIN)
	return malloc(size); // default osx malloc is 16 byte aligned.
#else
	void* rtn;
	if (LL_LIKELY(0 == posix_memalign(&rtn, 16, size)))
	{
		return rtn;
	}
	else // bad alignment requested, or out of memory
	{
		return NULL;
	}
#endif
}

// returned hunk MUST be freed with ll_aligned_free_16().
inline void* ll_aligned_realloc_16(void* ptr, size_t size)
{
#if defined(LL_WINDOWS)
	return _aligned_realloc(ptr, size, 16);
#elif defined(LL_DARWIN)
	return realloc(ptr, size); // default osx malloc is 16 byte aligned.
#else
	return realloc(ptr, size); // FIXME not guaranteed to be aligned.
#endif
}

inline void ll_aligned_free_16(void* p)
{
#if defined(LL_WINDOWS)
	_aligned_free(p);
#elif defined(LL_DARWIN)
	free(p);
#else
	free(p); // posix_memalign() is compatible with heap deallocator
#endif
}

#endif // LL_USE_TCMALLOC

class LL_COMMON_API LLMemory
{
public:
	static void initClass();
	static void cleanupClass();
	static void freeReserve();
	// Return the resident set size of the current process, in bytes.
	// Return value is zero if not known.
	static U64 getCurrentRSS();
	static U32 getWorkingSetSize();

private:
	static char* reserveMem;
};

//----------------------------------------------------------------------------

// LLRefCount moved to llrefcount.h

// LLPointer moved to llpointer.h

// LLSafeHandle moved to llsafehandle.h

// LLSingleton moved to llsingleton.h

LL_COMMON_API void ll_assert_aligned_func(uintptr_t ptr, U32 alignment);

#if !LL_USE_TCMALLOC
#define ll_assert_aligned(ptr,alignment) ll_assert_aligned_func(reinterpret_cast<uintptr_t>(ptr), ((U32)alignment))
#else
#define ll_assert_aligned(ptr,alignment)
#endif

#endif
