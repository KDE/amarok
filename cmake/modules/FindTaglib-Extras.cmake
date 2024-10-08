# - Try to find the Taglib-Extras library
# Once done this will define
#
#  TAGLIB-EXTRAS_FOUND - system has the taglib library
#  TAGLIB-EXTRAS_CFLAGS - the taglib cflags
#  TAGLIB-EXTRAS_LIBRARIES - The libraries needed to use taglib

# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(NOT TAGLIB-EXTRAS_MIN_VERSION)
  set(TAGLIB-EXTRAS_MIN_VERSION "1.0")
endif()

if(NOT WIN32)
    find_program(TAGLIB-EXTRASCONFIG_EXECUTABLE NAMES taglib-extras-config PATHS
       ${BIN_INSTALL_DIR}
    )
endif()

#reset vars
set(TAGLIB-EXTRAS_LIBRARIES)
set(TAGLIB-EXTRAS_CFLAGS)

# if taglib-extras-config has been found
if(TAGLIB-EXTRASCONFIG_EXECUTABLE)

  exec_program(${TAGLIB-EXTRASCONFIG_EXECUTABLE} ARGS --version RETURN_VALUE _return_VALUE OUTPUT_VARIABLE TAGLIB-EXTRAS_VERSION)

  if(TAGLIB-EXTRAS_VERSION VERSION_LESS "${TAGLIB-EXTRAS_MIN_VERSION}")
     message(STATUS "TagLib-Extras version too old: version searched :${TAGLIB-EXTRAS_MIN_VERSION}, found ${TAGLIB-EXTRAS_VERSION}")
     set(TAGLIB-EXTRAS_FOUND FALSE)
  else()

     exec_program(${TAGLIB-EXTRASCONFIG_EXECUTABLE} ARGS --libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE TAGLIB-EXTRAS_LIBRARIES)

     exec_program(${TAGLIB-EXTRASCONFIG_EXECUTABLE} ARGS --cflags RETURN_VALUE _return_VALUE OUTPUT_VARIABLE TAGLIB-EXTRAS_CFLAGS)

     if(TAGLIB-EXTRAS_LIBRARIES AND TAGLIB-EXTRAS_CFLAGS)
        set(TAGLIB-EXTRAS_FOUND TRUE)
     endif()
     string(REGEX REPLACE " *-I" ";" TAGLIB-EXTRAS_INCLUDES "${TAGLIB-EXTRAS_CFLAGS}")
  endif()
  mark_as_advanced(TAGLIB-EXTRAS_CFLAGS TAGLIB-EXTRAS_LIBRARIES TAGLIB-EXTRAS_INCLUDES)

else()

  find_path(TAGLIB-EXTRAS_INCLUDES
    NAMES
    tfile_helper.h
    PATH_SUFFIXES taglib-extras
    PATHS
    ${INCLUDE_INSTALL_DIR}
  )

    if(NOT WIN32)
      # on non-win32 we don't need to take care about WIN32_DEBUG_POSTFIX

      find_library(TAGLIB-EXTRAS_LIBRARIES tag-extras PATHS ${LIB_INSTALL_DIR})

    else()

      # 1. get all possible libnames
      set(args PATHS ${LIB_INSTALL_DIR})
      set(newargs "")
      set(libnames_release "")
      set(libnames_debug "")

      list(LENGTH args listCount)

        # just one name
        list(APPEND libnames_release "tag-extras")
        list(APPEND libnames_debug   "tag-extrasd")

        set(newargs ${args})

      # search the release lib
      find_library(TAGLIB-EXTRAS_LIBRARIES_RELEASE
                   NAMES ${libnames_release}
                   ${newargs}
      )

      # search the debug lib
      find_library(TAGLIB-EXTRAS_LIBRARIES_DEBUG
                   NAMES ${libnames_debug}
                   ${newargs}
      )

      if(TAGLIB-EXTRAS_LIBRARIES_RELEASE AND TAGLIB-EXTRAS_LIBRARIES_DEBUG)

        # both libs found
        set(TAGLIB-EXTRAS_LIBRARIES optimized ${TAGLIB-EXTRAS_LIBRARIES_RELEASE}
                        debug     ${TAGLIB-EXTRAS_LIBRARIES_DEBUG})

      else()

        if(TAGLIB-EXTRAS_LIBRARIES_RELEASE)

          # only release found
          set(TAGLIB-EXTRAS_LIBRARIES ${TAGLIB-EXTRAS_LIBRARIES_RELEASE})

        else()

          # only debug (or nothing) found
          set(TAGLIB-EXTRAS_LIBRARIES ${TAGLIB-EXTRAS_LIBRARIES_DEBUG})

        endif()

      endif()

      mark_as_advanced(TAGLIB-EXTRAS_LIBRARIES_RELEASE)
      mark_as_advanced(TAGLIB-EXTRAS_LIBRARIES_DEBUG)

    endif()

  include(FindPackageMessage)
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Taglib-Extras DEFAULT_MSG TAGLIB-EXTRAS_INCLUDES TAGLIB-EXTRAS_LIBRARIES)

endif()


if(TAGLIB-EXTRAS_FOUND)
  if(NOT Taglib-Extras_FIND_QUIETLY AND TAGLIB-EXTRASCONFIG_EXECUTABLE)
    message(STATUS "Taglib-Extras found: ${TAGLIB-EXTRAS_LIBRARIES}")
  endif()
else()
  if(Taglib-Extras_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find Taglib-Extras")
  endif()
endif()

