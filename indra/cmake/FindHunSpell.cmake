find_path(HUNSPELL_INCLUDE_DIR hunspell.hxx
  /usr/local/include/hunspell
  /usr/local/include
  /usr/include/hunspell
  /usr/include
  )

set(HUNSPELL_NAMES ${HUNSPELL_NAMES} hunspell hunspell-1.3.0)
find_library(HUNSPELL_LIBRARY
  NAMES ${HUNSPELL_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

if(HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)
  set(HUNSPELL_LIBRARIES ${HUNSPELL_LIBRARY})
  set(HUNSPELL_FOUND "YES")
else(HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)
  set(HUNSPELL_FOUND "NO")
endif(HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)

if(HUNSPELL_FOUND)
  if(NOT HUNSPELL_FIND_QUIETLY)
    message(STATUS "Found Hunspell: ${HUNSPELL_LIBRARIES}")
  endif(NOT HUNSPELL_FIND_QUIETLY)
else(HUNSPELL_FOUND)
  if(HUNSPELL_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find HunSpell library")
  endif(HUNSPELL_FIND_REQUIRED)
endif(HUNSPELL_FOUND)

mark_as_advanced(
  HUNSPELL_LIBRARY
  HUNSPELL_INCLUDE_DIR
  )
