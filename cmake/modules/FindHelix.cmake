# - Try to find the Helix or Realplayer libraries
# Once done this will define
#
#  HELIX_FOUND - system has the Realplayer or Helixplayer libraries installed
#  HELIX_LIBRARY - The helix client core library location
#  HELIX_LIBRARY_PATH - location of the package
#


if (HELIX_LIBRARY)
  # Already in cache, be silent
  set(HELIX_FIND_QUIETLY TRUE)
endif (HELIX_LIBRARY)

EXEC_PROGRAM( dirname ARGS "${CMAKE_CURRENT_LIST_FILE}" OUTPUT_VARIABLE cmakeModulesPath )
EXEC_PROGRAM( ${cmakeModulesPath}/RealPlayerLocation.rb OUTPUT_VARIABLE possible_path )

FIND_PATH(HELIX_LIBRARY clntcore.so
 ${possible_path}/common
 /usr/local/Helix*/common
 /usr/local/helix*/common
 /usr/local/Real*/common
 /usr/local/real*/common
 /usr/local/share/Helix*/common
 /usr/local/share/helix*/common
 /usr/local/share/Real*/common
 /usr/local/share/real*/common
 /usr/local/lib/Helix*/common
 /usr/local/lib/helix*/common
 /usr/local/lib/Real*/common
 /usr/local/lib/real*/common
 /opt/Helix*/common
 /opt/helix*/common
 /opt/Real*/common
 /opt/real*/common
 /usr/share/Helix*/common
 /usr/share/helix*/common
 /usr/share/Real*/common
 /usr/share/real*/common
 /usr/lib/Helix*/common
 /usr/lib/helix*/common
 /usr/lib/Real*/common
 /usr/lib/real*/common
 ${HOME}/Helix*/common
 ${HOME}/helix*/common
 ${HOME}/Real*/common
 ${HOME}/real*/common
)

if (HELIX_LIBRARY)
   set(HELIX_FOUND TRUE)
   get_filename_component(HELIX_LIBRARY_PATH ${HELIX_LIBRARY} PATH)
   message( STATUS "PATH: " ${HELIX_LIBRARY_PATH} )
endif (HELIX_LIBRARY)

if (HELIX_FOUND)
   if (NOT Helix_FIND_QUIETLY)
      message(STATUS "Found Helix/Realplayer: ${HELIX_LIBRARY}")
   endif (NOT Helix_FIND_QUIETLY)
else (HELIX_FOUND)
   if (Helix_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find Helix or Realplayer")
   endif (Helix_FIND_REQUIRED)
endif (HELIX_FOUND)

MARK_AS_ADVANCED(HELIX_LIBRARY)
