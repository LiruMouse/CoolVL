# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
use_prebuilt_binary(colladadom)
if (LINUX OR DARWIN)
	use_prebuilt_binary(pcre)
endif (LINUX OR DARWIN)
if (LINUX)
	use_prebuilt_binary(libxml)
endif (LINUX)
use_prebuilt_binary(boost)

set(LLPRIMITIVE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llprimitive
    )

if (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        debug libcollada14dom22-d
        debug libboost_filesystem-vc80-mt-gd-1_45
        debug libboost_system-vc80-gd-mt-1_45
        optimized libcollada14dom22
        optimized libboost_filesystem-vc80-mt-1_45
        optimized libboost_system-vc80-mt-1_45
        )
elseif (LINUX)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        collada14dom
        minizip
        xml2
       	pcrecpp
        pcre
        boost_system-mt
        )
elseif (DARWIN)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        collada14dom
        minizip
        xml2
       	pcrecpp
        pcre
        boost_system-mt
        )
endif (WINDOWS)
