# -*- cmake -*-
include(Prebuilt)

set(LLCONVEXDECOMP_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)

# Note: we don't have access to proprietary libraries, so it makes no sense
#		bothering with the (INSTALL_PROPRIETARY AND NOT STANDALONE) case !

#use_prebuilt_binary(llconvexdecompositionstub)
#set(LLCONVEXDECOMP_LIBRARY llconvexdecompositionstub)

include(HACD)
