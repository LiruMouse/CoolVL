# -*- cmake -*-

include(APR)
include(Boost)
include(EXPAT)
include(ZLIB)
include(GooglePerfTools)

set(LLCOMMON_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llcommon
    ${APRUTIL_INCLUDE_DIR}
    ${APR_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    )

if (LINUX)
    # In order to support using ld.gold on linux, we need to explicitely
    # specify all libraries that llcommon uses.
    # llcommon uses `clock_gettime' which is provided by librt on linux.
    set(LLCOMMON_LIBRARIES llcommon rt)
else (LINUX)
    set(LLCOMMON_LIBRARIES llcommon)
endif (LINUX)

add_definitions(${TCMALLOC_FLAG})

set(LLCOMMON_LIBRARIES llcommon)
