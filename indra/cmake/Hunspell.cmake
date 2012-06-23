# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindHunSpell)
else (STANDALONE)
  use_prebuilt_binary(hunspell)

  set(HUNSPELL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/hunspell)

  if (LINUX)
    set(HUNSPELL_LIBRARY hunspell-1.3)
  elseif (DARWIN)
    set(HUNSPELL_LIBRARY hunspell-1.3.0)
  elseif (WINDOWS)
    set(HUNSPELL_LIBRARY libhunspell)
  else ()
    message(FATAL_ERROR "Invalid platform")
  endif ()
endif (STANDALONE)
