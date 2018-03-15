# - Try to find the libmtp library
# Once done this will define
#
#  MTP_FOUND - system has libmtp
#  MTP_INCLUDE_DIR - the libmtp include directory
#  MTP_LIBRARIES - Link these to use libmtp
#  MTP_DEFINITIONS - Compiler switches required for using libmtp
#

find_package(PkgConfig QUIET)
pkg_check_modules(PC_MTP QUIET libmtp>=${Mtp_FIND_VERSION})

set(MTP_DEFINITIONS ${PC_MTP_CFLAGS})

find_path(MTP_INCLUDE_DIR
    NAMES libmtp.h
    HINTS ${PC_MTP__INCLUDEDIR} ${PC_MTP_INCLUDE_DIRS}
)

find_library(MTP_LIBRARIES
    NAMES mtp
    HINTS ${PC_MTP_LIBDIR} ${PC_MTP_LIBRARY_DIRS}
)

if(PC_MTP_VERSION)
    set(MTP_VERSION_STRING ${PC_MTP_VERSION})
elseif(MTP_INCLUDE_DIR AND EXISTS "${MTP_INCLUDE_DIR}/libmtp.h")
    file(STRINGS "${MTP_INCLUDE_DIR}/libmtp.h" mtp_version_str
         REGEX "^#define[\t ]+LIBMTP_VERSION_STRING[\t ]+\".*\"")

    string(REGEX REPLACE "^#define[\t ]+LIBMTP_VERSION_STRING[\t ]+\"([^\"]*)\".*" "\\1"
           MTP_VERSION_STRING "${mtp_version_str}")
    unset(mtp_version_str)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mtp
    REQUIRED_VARS MTP_LIBRARIES MTP_INCLUDE_DIR
    VERSION_VAR MTP_VERSION_STRING
)

if(MTP_FOUND AND NOT TARGET Mtp::Mtp)
  add_library(Mtp::Mtp UNKNOWN IMPORTED)
  set_target_properties(Mtp::Mtp PROPERTIES
                        IMPORTED_LOCATION "${MTP_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${MTP_INCLUDE_DIR}")
endif()

mark_as_advanced(MTP_INCLUDE_DIR MTP_LIBRARIES)
set_package_properties(Mtp PROPERTIES
    URL "http://libmtp.sourceforge.net/"
    DESCRIPTION "An implementation of Microsoft's Media Transfer Protocol (MTP)"
)
