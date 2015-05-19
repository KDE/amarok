project(Amarok)

cmake_minimum_required(VERSION 2.8.12)

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

message(STATUS "${CMAKE_MODULE_PATH}")

###############
find_package(ECM 1.7.0 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddTests)
include(ECMAddAppIcon)

#find_package( Qt4 4.8.3 COMPONENTS QtCore QtGui QtScript QtSvg QtXml QtWebKit REQUIRED )
#find_package( KDE4 4.8.4 REQUIRED )
find_package( Qt5 REQUIRED COMPONENTS Core DBus Widgets Test )
find_package( KF5 REQUIRED COMPONENTS CoreAddons Solid KDELibs4Support)
###############
option(WITH_UTILITIES "Enable building of utilities" ON)
option(WITH_PLAYER "Enable building of main Amarok player" ON)
option(WITH_MP3Tunes "Enable mp3tunes in the Amarok player, requires multiple extra dependencies" ON)
option(WITH_IPOD "Enable iPod support in Amarok" ON)
option(WITH_MYSQL_EMBEDDED "Build the embedded database library -- highly recommended" ON)
option(WITH_PLAYGROUND "Enable building of playground scripts and applets (WARNING: some of them might have legal issues!)" OFF)

############### Taglib
set(TAGLIB_MIN_VERSION "1.7")
find_package(Taglib REQUIRED)

# Check if TagLib is built with ASF and MP4 support
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES "${TAGLIB_INCLUDES}")
set(CMAKE_REQUIRED_LIBRARIES "${TAGLIB_LIBRARIES}")

check_cxx_source_compiles("#include <asftag.h>
int main() { TagLib::ASF::Tag tag; return 0;}" TAGLIB_ASF_FOUND)
if( NOT TAGLIB_ASF_FOUND )
    message(FATAL_ERROR "TagLib does not have ASF support compiled in.")
endif( NOT TAGLIB_ASF_FOUND )

check_cxx_source_compiles("#include <mp4tag.h>
int main() { TagLib::MP4::Tag tag(0, 0); return 0;}" TAGLIB_MP4_FOUND)
if( NOT TAGLIB_MP4_FOUND )
    message(FATAL_ERROR "TagLib does not have MP4 support compiled in.")
endif( NOT TAGLIB_MP4_FOUND )

check_cxx_source_compiles("#include <modtag.h>
#include <modfile.h>
#include <s3mfile.h>
#include <itfile.h>
#include <xmfile.h>
using namespace TagLib;
int main() { char *s; Mod::Tag tag; Mod::File modfile(s); S3M::File s3mfile(s);
IT::File itfile(s); XM::File xmfile(s); return 0; }" TAGLIB_MOD_FOUND)

check_cxx_source_compiles("#include <opusfile.h>
int main() { char *s; TagLib::Ogg::Opus::File opusfile(s); return 0;}" TAGLIB_OPUS_FOUND)

set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)

set(TAGLIB-EXTRAS_MIN_VERSION "1.0")
find_package(Taglib-Extras)
set(TAGLIB_EXTRAS_FOUND ${TAGLIB-EXTRAS_FOUND}) # we need a c-compatible name for the include file

include(CheckTagLibFileName)

check_taglib_filename(COMPLEX_TAGLIB_FILENAME)
###############


# Needed to conditionally build tests and gui
if(KDE4_BUILD_TESTS)
   add_definitions(-DDEBUG)
endif()

if(WITH_DESKTOP_UI)
    add_definitions(-DDESKTOP_UI)
endif()
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
if (CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--as-needed")
    endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
endif (CMAKE_COMPILER_IS_GNUCXX)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_BINARY_DIR}/shared
)



SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")  # Require C++11

# WORKAROUND for Clang bug: http://llvm.org/bugs/show_bug.cgi?id=15651
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delayed-template-parsing")
endif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32)


include( KDE4Defaults )
include( MacroBoolTo01 )
include( MacroLibrary )
add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

# QCA2 is required for the Script Updater
find_package(Qca-qt5 2.1.0)

find_package( LibLastFm )
set( LIBLASTFM_MIN_VERSION "1.0.0" )
if( LIBLASTFM_FOUND )
    if ( ${LIBLASTFM_MIN_VERSION} VERSION_LESS ${LIBLASTFM_VERSION} )
        set( LIBLASTFM_FOUND TRUE )
    endif()
endif( LIBLASTFM_FOUND )
macro_bool_to_01( LIBLASTFM_FOUND HAVE_LIBLASTFM )

find_package( FFmpeg )

if( FFMPEG_FOUND )
    find_package( LibOFA )
    macro_bool_to_01( LIBOFA_FOUND HAVE_LIBOFA )
endif( FFMPEG_FOUND )

string( TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER )
if( CMAKE_BUILD_TYPE_TOLOWER MATCHES debug )
    set( DEBUG_BUILD_TYPE ON )
    add_definitions(-Wall -Wextra)
endif( CMAKE_BUILD_TYPE_TOLOWER MATCHES debug )

# this needs to be here because also code in shared/ needs config.h. This is also the
# reason why various checks are above why they belong under if( WITH_PLAYER )
configure_file( shared/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/shared/config.h )

add_subdirectory( data )
add_subdirectory( images )
add_subdirectory( shared )

if( WITH_PLAYER )
    set_package_properties( QTOPENGL PROPERTIES DESCRIPTION "Required for the spectrum analyzer" URL "http://qt-project.org" TYPE OPTIONAL )

    find_package(MySQLAmarok REQUIRED)
    if( WITH_MYSQL_EMBEDDED )
        set( BUILD_MYSQLE_COLLECTION TRUE )
        set_package_properties( MYSQLD PROPERTIES DESCRIPTION "Embedded MySQL Libraries" URL "http://www.mysql.com" TYPE REQUIRED )
    else( WITH_MYSQL_EMBEDDED )
        add_definitions( "-DNO_MYSQL_EMBEDDED" )
    endif( WITH_MYSQL_EMBEDDED )
    set_package_properties( MYSQL PROPERTIES DESCRIPTION "MySQL Server Libraries" URL "http://www.mysql.com" TYPE REQUIRED )

    # zlib is required for mysql embedded
    find_package(ZLIB REQUIRED)
    set_package_properties( ZLIB PROPERTIES DESCRIPTION "zlib" TYPE REQUIRED )

    set_package_properties( QCA2 PROPERTIES DESCRIPTION "Qt Cryptographic Architecture" URL "http://delta.affinix.com/qca/" TYPE OPTIONAL )

    # QJson is required for the PlaydarCollection
    find_package(QJSON)
    set_package_properties( QJSON PROPERTIES DESCRIPTION "Qt JSON Parser used for the Playdar Collection" URL "http://qjson.sourceforge.net/" TYPE OPTIONAL )

    # We tell users that we need 1.0.3, but we really check just >= 1.0.0. This is because
    # upstream forgot to update version in lastfm/global.h, so it looks like 1.0.2. :-(
    # will be fixed in liblastfm-1.0.4
    set( LIBLASTFM_MIN_VERSION "1.0.3" )
    set_package_properties( LIBLASTFM PROPERTIES DESCRIPTION "Enable Last.Fm service, including scrobbling, song submissions, and suggested song dynamic playlists" URL "http://cdn.last.fm/client/liblastfm-${LIBLASTFM_MIN_VERSION}.tar.gz" TYPE OPTIONAL )

    set_package_properties( FFMPEG PROPERTIES DESCRIPTION "Libraries and tools for handling multimedia data" URL "http://www.ffmpeg.org/" TYPE OPTIONAL )

    if( FFMPEG_FOUND )
        set_package_properties(LIBOFA PROPERTIES DESCRIPTION "Enable MusicDNS service" URL "http://code.google.com/p/musicip-libofa/" TYPE OPTIONAL)
    endif( FFMPEG_FOUND )

    ##gpodder Service
    find_package( Mygpo-qt 1.0.7 QUIET )
    set_package_properties( LIBMYGPO_QT PROPERTIES DESCRIPTION "Enable gpodder.net service" URL "http://wiki.gpodder.org/wiki/Libmygpo-qt" TYPE OPTIONAL )
    macro_bool_to_01( LIBMYGPO_QT_FOUND HAVE_LIBMYGPOQT )

    if( WITH_IPOD )
        find_package(Ipod)
        SET(IPOD_MIN_VERSION "0.8.2")
        if( IPOD_FOUND AND NOT WIN32 )
            if ( ${IPOD_MIN_VERSION} VERSION_LESS ${IPOD_VERSION} )
                set( IPOD_FOUND TRUE )
            endif()
        endif( IPOD_FOUND AND NOT WIN32 )
        set_package_properties( IPOD PROPERTIES DESCRIPTION "Support Apple iPod/iPad/iPhone audio devices" URL "http://sourceforge.net/projects/gtkpod/" TYPE OPTIONAL )
        find_package(GDKPixBuf)
        set_package_properties( GDKPIXBUF PROPERTIES DESCRIPTION "Support for artwork on iPod audio devices via GDK-PixBuf" URL "http://developer.gnome.org/arch/imaging/gdkpixbuf.html" TYPE OPTIONAL )
    endif( WITH_IPOD )

    find_package(Mtp)
    set_package_properties(MTP PROPERTIES DESCRIPTION "Enable Support for portable media devices that use the media transfer protocol" URL "http://libmtp.sourceforge.net/" TYPE OPTIONAL )

    if( WITH_MP3Tunes )
        find_package(CURL)
        set_package_properties( MTP PROPERTIES DESCRIPTION "Enable Support for portable media devices that use the media transfer protocol" URL "http://libmtp.sourceforge.net/" TYPE OPTIONAL )

        find_package(LibXml2)
        set_package_properties( LIBXML2 PROPERTIES DESCRIPTION "LibXML2 is an XML parser required by mp3tunes." URL "http://www.xmlsoft.org" TYPE OPTIONAL )

        find_package(OpenSSL)
        find_package(Libgcrypt)
        if ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
            set (_mp3tunes_crypto TRUE )
        else ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
            message( SEND_ERROR "Building with mp3tunes support REQUIRES either OpenSSL or GNU Libgcrypt" )
        endif ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
        set_package_properties( OPENSSL_OR_LIBGCRYPT PROPERTIES DESCRIPTION "OpenSSL or GNU Libgcrypt provides cryptographic functions required by mp3tunes." URL "http://www.openssl.org/ or http://www.gnupg.org/download/#libgcrypt" TYPE OPTIONAL )

        find_package(Loudmouth)
        set_package_properties( LOUDMOUTH PROPERTIES DESCRIPTION "Loudmouth is the communication backend needed by mp3tunes for syncing." URL "http://www.loudmouth-project.org" TYPE OPTIONAL )

        include(CheckQtGlib)
        set_package_properties( QT4_GLIB PROPERTIES DESCRIPTION "Qt4 must be compiled with glib support for mp3tunes" URL "http://www.trolltech.com" TYPE OPTIONAL )
    endif( WITH_MP3Tunes )

    if( WITH_IPOD OR WITH_MP3Tunes )
    find_package(GObject)
    set_package_properties( GOBJECT PROPERTIES DESCRIPTION "Required by libgpod and mp3tunes." URL "http://www.gtk.org" TYPE OPTIONAL )
    find_package(GLIB2)
    set_package_properties( GLIB2 PROPERTIES DESCRIPTION "Required by libgpod and mp3tunes" URL "http://www.gtk.org" TYPE OPTIONAL )
    endif( WITH_IPOD OR WITH_MP3Tunes )

    find_program( CLAMZ_FOUND clamz PATH )
    set_package_properties( CLAMZ PROPERTIES DESCRIPTION "Optional requirement to download songs from the Amazon MP3 store. Highly recommended on Linux, as the official downloader from Amazon is quite broken on many systems." URL "https://code.google.com/p/clamz/" TYPE OPTIONAL )

    find_package(PythonInterp)
    set_package_properties( PYTHON PROPERTIES DESCRIPTION "Required for generating the autocompletion file for the script console" URL "https://www.python.org" TYPE OPTIONAL )


    if( KDE4_BUILD_TESTS AND NOT WIN32 )
        ENABLE_TESTING()
        add_subdirectory( tests )
    endif( KDE4_BUILD_TESTS AND NOT WIN32 )

    add_subdirectory( src )

    # Also display taglib in the feature log
    set_package_properties( TAGLIB PROPERTIES DESCRIPTION "Support for Audio metadata." URL "http://developer.kde.org/~wheeler/taglib.html" TYPE REQUIRED PURPOSE "Required for tag reading" )
    # following line is here (and not near TAGLIB_MOD_FOUND) because there may be no MacroLogFeature without kdelibs
    set_package_properties( TAGLIB_MOD PROPERTIES DESCRIPTION "Additional support for Audio metadata of mod, s3m, it and xm files." URL "http://developer.kde.org/~wheeler/taglib.html" TYPE OPTIONAL )
    set_package_properties( TAGLIB_OPUS PROPERTIES DESCRIPTION "Additional support for Audio metadata of opus files." URL "http://developer.kde.org/~wheeler/taglib.html" TYPE OPTIONAL )

    feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

    #Do not remove or modify these.  The release script substitutes in for these
    #comments with appropriate doc and translation directories.
    #PO_SUBDIR
    #DOC_SUBDIR

endif( WITH_PLAYER )

if( WITH_UTILITIES )
    set(EXEC_INSTALL_PREFIX  ${CMAKE_INSTALL_PREFIX}       CACHE PATH  "Base directory for executables and libraries" FORCE)
    set(BIN_INSTALL_DIR          "${EXEC_INSTALL_PREFIX}/bin"    CACHE PATH "The subdirectory to the binaries prefix (default prefix/bin)" FORCE)
    add_subdirectory( utilities )
endif( WITH_UTILITIES )

if( WITH_PLAYGROUND )
    add_subdirectory( playground )
    message(STATUS "Included playground subdirectory in configuration")
endif( WITH_PLAYGROUND )

include(CTest)
