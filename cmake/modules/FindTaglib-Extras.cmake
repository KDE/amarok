# - Try to find the Taglib-Extras library
#
# Example call to find a version thats at least 1.4:
#  find_package(Taglib-Extras 1.4)
# Once done this will define
#
#  TAGLIB-EXTRAS_FOUND - system has the taglib-extras library
#  TAGLIB-EXTRAS_INCLUDE_DIR - the taglib-extras include directory
#  TAGLIB-EXTRAS_DEFINITIONS - defines to be set when using taglib-extras
#  TAGLIB-EXTRAS_LIBRARIES - The libraries needed to use taglib-extras

# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(TAGLIB-EXTRAS_LIBRARIES AND TAGLIB-EXTRAS_INCLUDE_DIR)
  #in cache already
  set(TAGLIB-EXTRAS_FOUND TRUE)
else(TAGLIB-EXTRAS_LIBRARIES AND TAGLIB-EXTRAS_INCLUDE_DIR)
  if(NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(PC_TAGLIB-EXTRAS taglib-extras>=${Taglib-Extras_FIND_VERSION})
    set(TAGLIB-EXTRAS_DEFINITIONS ${PC_TAGLIB-EXTRAS_CFLAGS_OTHER})
  endif(NOT WIN32)

  #find include dir
  find_path(TAGLIB-EXTRAS_INCLUDE_DIR 
    NAMES tfile_helper.h
    HINTS ${PC_TAGLIB-EXTRAS_INCLUDEDIR} ${PC_TAGLIB-EXTRAS_INCLUDE_DIRS}
    PATH_SUFFIXES taglib-extras
    PATHS ${KDE4_INCLUDE_DIR} ${INCLUDE_INSTALL_DIR}
  )

  if(WIN32)
    #find release version
    find_library(TAGLIB-EXTRAS_LIBRARIES_RELEASE
      NAMES tag-extras
      HINTS ${PC_TAGLIB-EXTRAS_LIBDIR} ${PC_TAGLIB-EXTRAS_LIBRARY_DIRS}
      PATHS ${KDE4_LIB_DIR} ${LIB_INSTALL_DIR}
    )

    #find debug version
    find_library(TAGLIB-EXTRAS_LIBRARIES_DEBUG
      NAMES tag-extrasd
      HINTS ${PC_TAGLIB-EXTRAS_LIBDIR} ${PC_TAGLIB-EXTRAS_LIBRARY_DIRS}
      PATHS ${KDE4_LIB_DIR} ${LIB_INSTALL_DIR}
    )

    if(TAGLIB-EXTRAS_LIBRARIES_DEBUG AND TAGLIB-EXTRAS_LIBRARIES_RELEASE)
      #both are set so add both to the LIBRARIES variable
      set(TAGLIB-EXTRAS_LIBRARIES optimized ${TAGLIB-EXTRAS_LIBRARIES_RELEASE}
                                  debug ${TAGLIB-EXTRAS_LIBRARIES_DEBUG})
    else(TAGLIB-EXTRAS_LIBRARIES_DEBUG AND TAGLIB-EXTRAS_LIBRARIES_RELEASE)
      if(TAGLIB-EXTRAS_LIBRARIES_RELEASE)
        #only release available
        set(TAGLIB-EXTRAS_LIBRARIES ${TAGLIB-EXTRAS_LIBRARIES_RELEASE})
      else(TAGLIB-EXTRAS_LIBRARIES_RELEASE)
        #only debug available
        set(TAGLIB-EXTRAS_LIBRARIES ${TAGLIB-EXTRAS_LIBRARIES_DEBUG})
      endif(TAGLIB-EXTRAS_LIBRARIES_RELEASE)
    endif(TAGLIB-EXTRAS_LIBRARIES_DEBUG AND TAGLIB-EXTRAS_LIBRARIES_RELEASE)
    mark_as_advanced(TAGLIB-EXTRAS_LIBRARIES_DEBUG TAGLIB-EXTRAS_LIBRARIES_RELEASE)
  else(WIN32)
    find_library(TAGLIB-EXTRAS_LIBRARIES 
      NAMES tag-extras
      HINTS ${PC_TAGLIB-EXTRAS_LIBDIR} ${PC_TAGLIB-EXTRAS_LIBRARY_DIRS}
      PATHS ${KDE4_LIB_DIR} ${LIB_INSTALL_DIR}
    )
  endif(WIN32)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Taglib-Extras DEFAULT_MSG TAGLIB-EXTRAS_INCLUDE_DIR TAGLIB-EXTRAS_LIBRARIES)

  mark_as_advanced(TAGLIB-EXTRAS_LIBRARIES TAGLIB-EXTRAS_INCLUDE_DIR)

endif(TAGLIB-EXTRAS_LIBRARIES AND TAGLIB-EXTRAS_INCLUDE_DIR)

