# -*- cmake -*-
include(Prebuilt)

set(HACD_FIND_QUIETLY ON)
set(HACD_FIND_REQUIRED ON)

if (STANDALONE)
	include(FindHACD)
else (STANDALONE)
	use_prebuilt_binary(nd_hacdConvexDecomposition)
	set(HACD_LIBRARY hacd)
	set(LLCONVEXDECOMP_LIBRARY nd_hacdConvexDecomposition)
	set(HACD_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
