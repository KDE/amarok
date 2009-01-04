# - Try to find the GDK library
# Once done this will define
#
#  GDK_FOUND - system has GDK
#  GDK_INCLUDE_DIR - the GDK include directory
#  GDK_LIBRARIES - Link these to use GDK
#  GDK_DEFINITIONS - Compiler switches required for using GDK
#

if (GDK_INCLUDE_DIR AND GDK_LIBRARIES)

  # in cache already
  SET(GDK_FOUND TRUE)

else (GDK_INCLUDE_DIR AND GDK_LIBRARIES)
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    INCLUDE(UsePkgConfig)
  
    PKGCONFIG(gdk-pixbuf-2.0 _GDKIncDir _GDKLinkDir _GDKLinkFlags _GDKCflags)
  
    set(GDK_DEFINITIONS ${_GDKCflags})
  endif(NOT WIN32)

  FIND_PATH(GDK_INCLUDE_DIR gdk-pixbuf/gdk-pixbuf.h /usr/include/gtk-2.0
    ${_GDKIncDir}
  )
  
  FIND_LIBRARY(GDK_LIBRARIES NAMES gdk_pixbuf-2.0
    PATHS
    ${_GDKLinkDir}
  )

  if (GDK_INCLUDE_DIR AND GDK_LIBRARIES)
    SET(GDK_FOUND TRUE)
  else (GDK_INCLUDE_DIR AND GDK_LIBRARIES)
    SET(GDK_FOUND FALSE)
  endif (GDK_INCLUDE_DIR AND GDK_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Gdk DEFAULT_MSG GDK_INCLUDE_DIR GDK_LIBRARIES )
 
  MARK_AS_ADVANCED(GDK_INCLUDE_DIR GDK_LIBRARIES)
  
endif (GDK_INCLUDE_DIR AND GDK_LIBRARIES)

macro_bool_to_01(GDK_FOUND GDK_FOUND)
