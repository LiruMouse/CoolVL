# -*- cmake -*-

set(DB_FIND_QUIETLY ON)
set(DB_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindBerkeleyDB)
else (STANDALONE)
  set(DB_LIBRARIES db-5.1)
  set(DB_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
