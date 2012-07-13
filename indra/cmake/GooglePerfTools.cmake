# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindGooglePerfTools)
else (STANDALONE)
  if (WINDOWS)
    if (USE_TCMALLOC)
      use_prebuilt_binary(tcmalloc)
      set(TCMALLOC_LIBRARIES 
          debug libtcmalloc_minimal-debug
          optimized libtcmalloc_minimal)
      set(TCMALLOC_LINK_FLAGS  "/INCLUDE:__tcmalloc")
    else (USE_TCMALLOC)
      set(TCMALLOC_LIBRARIES)
      set(TCMALLOC_LINK_FLAGS)
    endif (USE_TCMALLOC)
    set(GOOGLE_PERFTOOLS_FOUND "YES")
  endif (WINDOWS)
  if (LINUX)
    if (USE_TCMALLOC)
      use_prebuilt_binary(tcmalloc)
      set(TCMALLOC_LIBRARIES tcmalloc)
    else (USE_TCMALLOC)
      set(TCMALLOC_LIBRARIES)
    endif (USE_TCMALLOC)
#   set(STACKTRACE_LIBRARIES stacktrace)
    set(PROFILER_LIBRARIES profiler)
    set(GOOGLE_PERFTOOLS_INCLUDE_DIR
        ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
    set(GOOGLE_PERFTOOLS_FOUND "YES")
  endif (LINUX)
endif (STANDALONE)

if (GOOGLE_PERFTOOLS_FOUND)
  # XXX Disable temporarily, until we have compilation issues on 64-bit Etch sorted.
  set(USE_GOOGLE_PERFTOOLS OFF CACHE BOOL "Build with Google PerfTools support.")
endif (GOOGLE_PERFTOOLS_FOUND)

if (WINDOWS)
  set(USE_GOOGLE_PERFTOOLS ON)
endif (WINDOWS)

if (USE_GOOGLE_PERFTOOLS)
  include_directories(${GOOGLE_PERFTOOLS_INCLUDE_DIR})
  set(GOOGLE_PERFTOOLS_LIBRARIES ${TCMALLOC_LIBRARIES} ${STACKTRACE_LIBRARIES} ${PROFILER_LIBRARIES})
endif (USE_GOOGLE_PERFTOOLS)
