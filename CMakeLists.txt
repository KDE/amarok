project(Amarok)

cmake_minimum_required(VERSION 2.6.2)

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

message(STATUS "${CMAKE_MODULE_PATH}")

###############
find_package(ECM 1.7.0 REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(FeatureSummary)
include(ECMAddTests)
include(ECMAddAppIcon)

find_package( Qt5 REQUIRED COMPONENTS Core DBus Widgets Test )
find_package( KF5 REQUIRED COMPONENTS CoreAddons Solid )
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

find_package( Qt4 4.8.3 COMPONENTS QtCore QtGui QtScript QtSvg QtXml QtWebKit REQUIRED )
find_package( KDE4 4.8.4 REQUIRED )


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
find_package( QCA2 )

macro_optional_find_package( LibLastFm )
set( LIBLASTFM_MIN_VERSION "1.0.0" )
if( LIBLASTFM_FOUND )
    macro_ensure_version( ${LIBLASTFM_MIN_VERSION} ${LIBLASTFM_VERSION} LIBLASTFM_FOUND )
endif( LIBLASTFM_FOUND )
macro_bool_to_01( LIBLASTFM_FOUND HAVE_LIBLASTFM )

macro_optional_find_package( FFmpeg )

if( FFMPEG_FOUND )
    macro_optional_find_package( LibOFA )
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
    include(MacroLogFeature)

    macro_log_feature( KDE4_FOUND "kdelibs" "The toolkit Amarok uses to build" "http://www.kde.org" TRUE ${KDE_MIN_VERSION} "" )

    macro_log_feature( QT_QTOPENGL_FOUND "QtOpenGL" "Required for the spectrum analyzer" "http://qt-project.org" FALSE "" "" )

    find_package(MySQLAmarok REQUIRED)
    if( WITH_MYSQL_EMBEDDED )
        set( BUILD_MYSQLE_COLLECTION TRUE )
        macro_log_feature( MYSQL_EMBEDDED_FOUND "mysqld" "Embedded MySQL Libraries" "http://www.mysql.com" TRUE "" "" )
    else( WITH_MYSQL_EMBEDDED )
        add_definitions( "-DNO_MYSQL_EMBEDDED" )
    endif( WITH_MYSQL_EMBEDDED )
    macro_log_feature( MYSQL_FOUND "mysql" "MySQL Server Libraries" "http://www.mysql.com" TRUE "" "" )

    # zlib is required for mysql embedded
    find_package(ZLIB REQUIRED)
    macro_log_feature( ZLIB_FOUND "zlib" "zlib" "" TRUE "" "" )

    macro_log_feature( QCA2_FOUND "qca2" "Qt Cryptographic Architecture" "http://delta.affinix.com/qca/" FALSE "" "" )

    # QJson is required for the PlaydarCollection
    macro_optional_find_package(QJSON)
    macro_log_feature( QJSON_FOUND "QJson" "Qt JSON Parser used for the Playdar Collection" "http://qjson.sourceforge.net/" FALSE "" "" )

    # We tell users that we need 1.0.3, but we really check just >= 1.0.0. This is because
    # upstream forgot to update version in lastfm/global.h, so it looks like 1.0.2. :-(
    # will be fixed in liblastfm-1.0.4
    set( LIBLASTFM_MIN_VERSION "1.0.3" )
    macro_log_feature( LIBLASTFM_FOUND "liblastfm" "Enable Last.Fm service, including scrobbling, song submissions, and suggested song dynamic playlists"
                       "http://cdn.last.fm/client/liblastfm-${LIBLASTFM_MIN_VERSION}.tar.gz" FALSE ${LIBLASTFM_MIN_VERSION} "" )

    macro_log_feature( FFMPEG_FOUND "ffmpeg" "Libraries and tools for handling multimedia data" "http://www.ffmpeg.org/" FALSE "0.7" "" )

    if( FFMPEG_FOUND )
        macro_log_feature( LIBOFA_FOUND "libofa" "Enable MusicDNS service" "http://code.google.com/p/musicip-libofa/" FALSE "0.9.x" "" )
    endif( FFMPEG_FOUND )

    ##gpodder Service
    macro_optional_find_package( Mygpo-qt 1.0.7 QUIET )
    macro_log_feature( LIBMYGPO_QT_FOUND "libmygpo-qt" "Enable gpodder.net service" "http://wiki.gpodder.org/wiki/Libmygpo-qt" FALSE "1.0.7" "" )
    macro_bool_to_01( LIBMYGPO_QT_FOUND HAVE_LIBMYGPOQT )

    if( WITH_IPOD )
        find_package(Ipod)
        SET(IPOD_MIN_VERSION "0.8.2")
        if( IPOD_FOUND AND NOT WIN32 )
            macro_ensure_version(${IPOD_MIN_VERSION} ${IPOD_VERSION} IPOD_FOUND)
        endif( IPOD_FOUND AND NOT WIN32 )
        macro_log_feature( IPOD_FOUND "libgpod" "Support Apple iPod/iPad/iPhone audio devices" "http://sourceforge.net/projects/gtkpod/" FALSE ${IPOD_MIN_VERSION} "" )
        macro_optional_find_package(GDKPixBuf)
        macro_log_feature( GDKPIXBUF_FOUND "GDK-PixBuf" "Support for artwork on iPod audio devices via GDK-PixBuf" "http://developer.gnome.org/arch/imaging/gdkpixbuf.html" FALSE "2.0.x" "" )
    endif( WITH_IPOD )

    macro_optional_find_package(Mtp)
    macro_log_feature( MTP_FOUND "libmtp" "Enable Support for portable media devices that use the media transfer protocol" "http://libmtp.sourceforge.net/" FALSE "1.0.0" "")

    if( WITH_MP3Tunes )
        find_package(CURL)
        macro_log_feature( CURL_FOUND "curl" "cURL provides the necessary network libraries required by mp3tunes." "http://curl.haxx.se" FALSE "" "" )

        find_package(LibXml2)
        macro_log_feature( LIBXML2_FOUND "libxml2" "LibXML2 is an XML parser required by mp3tunes." "http://www.xmlsoft.org" FALSE "" "" )

        macro_optional_find_package(OpenSSL)
        macro_optional_find_package(Libgcrypt)
        if ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
            set (_mp3tunes_crypto TRUE )
        else ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
            message( SEND_ERROR "Building with mp3tunes support REQUIRES either OpenSSL or GNU Libgcrypt" )
        endif ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
        macro_log_feature( _mp3tunes_crypto "openssl or libgcrypt" "OpenSSL or GNU Libgcrypt provides cryptographic functions required by mp3tunes." "http://www.openssl.org/ or http://www.gnupg.org/download/#libgcrypt" FALSE "" "" )

        find_package(Loudmouth)
        macro_log_feature( LOUDMOUTH_FOUND "loudmouth" "Loudmouth is the communication backend needed by mp3tunes for syncing." "http://www.loudmouth-project.org" FALSE "" "" )

        include(CheckQtGlib)
        macro_log_feature(QT4_GLIB_SUPPORT "Qt4 Glib support" "Qt4 must be compiled with glib support for mp3tunes" "http://www.trolltech.com" FALSE "" "")
    endif( WITH_MP3Tunes )

    if( WITH_IPOD OR WITH_MP3Tunes )
    find_package(GObject)
    macro_log_feature( GOBJECT_FOUND "gobject" "Required by libgpod and mp3tunes." "http://www.gtk.org" FALSE "2.x" "" )
    find_package(GLIB2)
    macro_log_feature( GLIB2_FOUND "glib2" "Required by libgpod and mp3tunes" "http://www.gtk.org" FALSE "2.x" "")
    endif( WITH_IPOD OR WITH_MP3Tunes )

    find_program( CLAMZ_FOUND clamz PATH )
    macro_log_feature( CLAMZ_FOUND "clamz" "Optional requirement to download songs from the Amazon MP3 store. Highly recommended on Linux, as the official downloader from Amazon is quite broken on many systems." "https://code.google.com/p/clamz/" FALSE )

    find_package(PythonInterp)
    macro_log_feature(PYTHONINTERP_FOUND "Python" "Required for generating the autocompletion file for the script console" "https://www.python.org" FALSE "2.x" "")

    include_directories( ${KDE4_INCLUDES} )

    if( KDE4_BUILD_TESTS AND NOT WIN32 )
        ENABLE_TESTING()
        add_subdirectory( tests )
    endif( KDE4_BUILD_TESTS AND NOT WIN32 )

    add_subdirectory( src )

    # Also display taglib in the feature log
    macro_log_feature( TAGLIB_FOUND "taglib" "Support for Audio metadata." "http://developer.kde.org/~wheeler/taglib.html" TRUE "${TAGLIB_MIN_VERSION}" "Required for tag reading" )
    # following line is here (and not near TAGLIB_MOD_FOUND) because there may be no MacroLogFeature without kdelibs
    macro_log_feature( TAGLIB_MOD_FOUND "taglib" "Additional support for Audio metadata of mod, s3m, it and xm files." "http://developer.kde.org/~wheeler/taglib.html" FALSE "1.8" "" )
    macro_log_feature( TAGLIB_OPUS_FOUND "taglib" "Additional support for Audio metadata of opus files." "http://developer.kde.org/~wheeler/taglib.html" FALSE "1.9" "" )

    macro_display_feature_log()

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
