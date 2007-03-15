# - Try to find GObject
# Once done this will define
#
#  GOBJECT_FOUND - system has GObject
#  GOBJECT_INCLUDE_DIR - the GObject include directory
#  GOBJECT_LIBRARIES - the libraries needed to use GObject
#  GOBJECT_DEFINITIONS - Compiler switches required for using GObject
#
#  (c)2006, Tim Beaulen <tbscope@gmail.com>


IF (GOBJECT_INCLUDE_DIR AND GOBJECT_LIBRARIES)
   # in cache already
   SET(GObject_FIND_QUIETLY TRUE)
ELSE (GOBJECT_INCLUDE_DIR AND GOBJECT_LIBRARIES)
   SET(GObject_FIND_QUIETLY FALSE)
ENDIF (GOBJECT_INCLUDE_DIR AND GOBJECT_LIBRARIES)

IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   INCLUDE(UsePkgConfig)
   PKGCONFIG(gobject-2.0 _GObjectIncDir _GObjectLinkDir _GObjectLinkFlags _GObjectCflags)
   #MESSAGE(STATUS "DEBUG: GObject include directory = ${_GObjectIncDir}")
   #MESSAGE(STATUS "DEBUG: GObject link directory = ${_GObjectLinkDir}")
   #MESSAGE(STATUS "DEBUG: GObject link flags = ${_GObjectLinkFlags}")
   #MESSAGE(STATUS "DEBUG: GObject CFlags = ${_GObjectCflags}")
   SET(GOBJECT_DEFINITIONS ${_GObjectCflags})
ENDIF (NOT WIN32)

FIND_PATH(GOBJECT_INCLUDE_DIR gobject.h
   PATHS
   ${_GObjectIncDir}
   ${_GObjectIncDir}/glib-2.0/gobject/
   /usr/include/glib-2.0/gobject/
   #PATH_SUFFIXES gst
   )

FIND_LIBRARY(_GObjectLibs NAMES gobject-2.0
   PATHS
   ${_GObjectLinkDir}
   )
FIND_LIBRARY(_GModuleLibs NAMES gmodule-2.0
   PATHS
   ${_GObjectLinkDir}
   )
FIND_LIBRARY(_GThreadLibs NAMES gthread-2.0
   PATHS
   ${_GObjectLinkDir}
   )
FIND_LIBRARY(_GLibs NAMES glib-2.0
   PATHS
   ${_GObjectLinkDir}
   )

SET( GOBJECT_LIBRARIES ${_GObjectLibs} ${_GModuleLibs} ${_GThreadLibs} ${_GLibs} )

IF (GOBJECT_INCLUDE_DIR AND GOBJECT_LIBRARIES)
   SET(GOBJECT_FOUND TRUE)
ELSE (GOBJECT_INCLUDE_DIR AND GOBJECT_LIBRARIES)
   SET(GOBJECT_FOUND FALSE)
ENDIF (GOBJECT_INCLUDE_DIR AND GOBJECT_LIBRARIES)

IF (GOBJECT_FOUND)
   IF (NOT GObject_FIND_QUIETLY)
      MESSAGE(STATUS "Found GObject libraries: ${GOBJECT_LIBRARIES}")
      MESSAGE(STATUS "Found GObject includes : ${GOBJECT_INCLUDE_DIR}")
   ENDIF (NOT GObject_FIND_QUIETLY)
ELSE (GOBJECT_FOUND)
      MESSAGE(STATUS "Could NOT find GObject")
ENDIF (GOBJECT_FOUND)

MARK_AS_ADVANCED(GOBJECT_INCLUDE_DIR GOBJECT_LIBRARIES)
