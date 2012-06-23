# -*- cmake -*-

include(Audio)

set(LLAUDIO_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llaudio
    )

if (NOT WINDOWS)
  add_definitions(-DOV_EXCLUDE_STATIC_CALLBACKS)
endif (NOT WINDOWS)

set(LLAUDIO_LIBRARIES llaudio ${OPENAL_LIBRARIES})
