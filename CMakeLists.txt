cmake_minimum_required(VERSION 3.16)

project(Amarok)

# Remove all warnings, ease the porting to cmake 3.x
if (POLICY CMP0028)
    cmake_policy(SET CMP0028 NEW)
endif()

###############

set(KF5_MIN_VERSION "5.78.0")
set(QT_REQUIRED_VERSION "5.15.0")
find_package(PkgConfig REQUIRED)
find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
#include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddTests)
include(ECMAddAppIcon)
include(ECMPoQmTools)
include(FindPkgConfig)
include(CMakePushCheckState)
include(GenerateExportHeader)

find_package(Qt5 ${QT_REQUIRED_VERSION} REQUIRED COMPONENTS Core DBus Gui QuickWidgets Qml Sql Svg Test Widgets Xml)

find_package(Qt5QuickControls2 ${QT_REQUIRED_VERSION})
set_package_properties(Qt5QuickControls2 PROPERTIES TYPE RUNTIME PURPOSE "Needed by the player's context area")

find_package(Qt5WebEngine ${QT_REQUIRED_VERSION})
set_package_properties(Qt5WebEngine PROPERTIES TYPE OPTIONAL PURPOSE "Needed by the wikipedia applet")

find_package(Qt5UiTools ${QT_REQUIRED_VERSION})
set_package_properties(Qt5UiTools PROPERTIES TYPE OPTIONAL PURPOSE "Needed by the scripting engine")

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Attica
    Codecs
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DBusAddons
    Declarative
    DNSSD
    DocTools
    GlobalAccel
    GuiAddons
    I18n
    IconThemes
    KCMUtils
    KIO
    NewStuff
    Notifications
    Package
    Solid
    TextEditor
    ThreadWeaver
    WidgetsAddons
    WindowSystem
)

find_package(KF5Kirigami2 ${KF5_MIN_VERSION})
set_package_properties(KF5Kirigami2 PROPERTIES TYPE RUNTIME PURPOSE "Needed by the player's context area")

###############
option(WITH_UTILITIES "Enable building of utilities" ON)
option(WITH_PLAYER "Enable building of main Amarok player" ON)
# option(WITH_MP3Tunes "Enable mp3tunes in the Amarok player, requires multiple extra dependencies" ON) #build is disabled for now, no need to show option
option(WITH_IPOD "Enable iPod support in Amarok" ON)
option(WITH_PLAYGROUND "Enable building of playground scripts and applets (WARNING: some of them might have legal issues!)" OFF)

############### Taglib
set(TAGLIB_MIN_VERSION "1.12")
find_package(Taglib REQUIRED)
set_package_properties( Taglib PROPERTIES DESCRIPTION "Support for Audio metadata." URL "http://developer.kde.org/~wheeler/taglib.html" TYPE REQUIRED PURPOSE "Required for tag reading" )

# Check if TagLib is built with ASF and MP4 support
include(CheckCXXSourceCompiles)
cmake_push_check_state()
set(CMAKE_REQUIRED_INCLUDES "${TAGLIB_INCLUDES}")
set(CMAKE_REQUIRED_LIBRARIES "${TAGLIB_LIBRARIES}")

check_cxx_source_compiles("#include <asftag.h>
int main() { TagLib::ASF::Tag tag; return 0;}" TAGLIB_ASF_FOUND)
if( NOT TAGLIB_ASF_FOUND )
    message(FATAL_ERROR "TagLib does not have ASF support compiled in.")
endif()

check_cxx_source_compiles("#include <mp4tag.h>
int main() { TagLib::MP4::Tag tag(0, 0); return 0;}" TAGLIB_MP4_FOUND)
if( NOT TAGLIB_MP4_FOUND )
    message(FATAL_ERROR "TagLib does not have MP4 support compiled in.")
endif()

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
cmake_pop_check_state()

set(TAGLIB-EXTRAS_MIN_VERSION "1.0")
find_package(Taglib-Extras)
set(TAGLIB_EXTRAS_FOUND ${TAGLIB-EXTRAS_FOUND}) # we need a c-compatible name for the include file

include(CheckTagLibFileName)

check_taglib_filename(COMPLEX_TAGLIB_FILENAME)
###############


#Needed to conditionally build tests and gui
find_package(Googlemock)
set_package_properties( GOOGLEMOCK PROPERTIES DESCRIPTION "Used in Amarok's tests." URL "https://github.com/google/googlemock" TYPE OPTIONAL )

if(NOT GOOGLEMOCK_FOUND)
    set(BUILD_TESTING OFF)
endif()

if(BUILD_TESTING)
   add_definitions(-DDEBUG)
endif()

if(WITH_DESKTOP_UI)
    add_definitions(-DDESKTOP_UI)
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--as-needed")
    endif()
endif ()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_BINARY_DIR}/shared
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# WORKAROUND for Clang bug: http://llvm.org/bugs/show_bug.cgi?id=15651
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delayed-template-parsing")
endif ()

####################################################################################
# CMAKE_AUTOMOC

if(NOT CMAKE_VERSION VERSION_LESS "3.10.0")
    # CMake 3.9+ warns about automoc on files without Q_OBJECT, and doesn't know about other macros.
    # 3.10+ lets us provide more macro names that require automoc.
    list(APPEND CMAKE_AUTOMOC_MACRO_NAMES AMAROK_EXPORT_SIMPLE_IMPORTER_PLUGIN)
endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_NO_CAST_TO_ASCII)

find_package(Phonon4Qt5 4.6.60 REQUIRED NO_MODULE)
add_definitions(-DPHONON_LIB_SONAME=\"${PHONON_LIB_SONAME}\")

find_package( LibLastFm )
set( LIBLASTFM_MIN_VERSION "1.0.0" )
if( LIBLASTFM_FOUND )
    if ( ${LIBLASTFM_MIN_VERSION} VERSION_LESS ${LIBLASTFM_VERSION} )
        set( LIBLASTFM_FOUND TRUE )
    endif()
endif()

string( TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER )
if( CMAKE_BUILD_TYPE_TOLOWER MATCHES debug )
    set( DEBUG_BUILD_TYPE ON )
    add_definitions(-Wall -Wextra)
endif()

add_subdirectory( data )
add_subdirectory( images )

if( WITH_PLAYER )
    find_package(Threads REQUIRED)

    find_package(MySQL)

    if( NOT MySQL_FOUND )
        find_package(MariaDB REQUIRED)
    endif()

    find_package(MySQLe)
    if( MySQLe_FOUND )
        set( BUILD_MYSQLE_COLLECTION TRUE )
        # zlib is required for mysql embedded
        find_package(ZLIB REQUIRED)
        set_package_properties( ZLIB PROPERTIES DESCRIPTION "zlib" TYPE REQUIRED )
    else()
        add_definitions( "-DNO_MYSQL_EMBEDDED" )
    endif()

    # We tell users that we need 1.0.3, but we really check just >= 1.0.0. This is because
    # upstream forgot to update version in lastfm/global.h, so it looks like 1.0.2. :-(
    # will be fixed in liblastfm-1.0.4
    set( LIBLASTFM_MIN_VERSION "1.0.3" )
    set_package_properties( LibLastFm PROPERTIES DESCRIPTION "Enable Last.Fm service, including scrobbling, song submissions, and suggested song dynamic playlists" URL "http://cdn.last.fm/client/liblastfm-${LIBLASTFM_MIN_VERSION}.tar.gz" TYPE OPTIONAL )

    find_package(FFmpeg)
    set_package_properties(FFmpeg PROPERTIES
        DESCRIPTION "Libraries and tools for handling multimedia data"
        URL "https://www.ffmpeg.org/"
        TYPE OPTIONAL
        PURPOSE "Enable MusicDNS service"
    )

    find_package(LibOFA)
    set_package_properties(LibOFA PROPERTIES
        DESCRIPTION "Open-source audio fingerprint by MusicIP"
        URL "http://code.google.com/p/musicip-libofa/"
        TYPE OPTIONAL
        PURPOSE "Enable MusicDNS service"
    )

    ## gpodder Service
    find_package(Mygpo-qt5 1.1.0 CONFIG)
    set_package_properties(Mygpo-qt5 PROPERTIES
        DESCRIPTION "A Qt/C++ library wrapping the gpodder.net Webservice."
        URL "http://wiki.gpodder.org/wiki/Libmygpo-qt"
        TYPE OPTIONAL
        PURPOSE "Synchronize podcast subscriptions with gpodder.net"
    )

    if( WITH_IPOD )
        find_package(Ipod)
        set(IPOD_MIN_VERSION "0.8.2")
        if( IPOD_FOUND AND NOT WIN32 )
            if ( ${IPOD_MIN_VERSION} VERSION_LESS ${IPOD_VERSION} )
                set( IPOD_FOUND TRUE )
            endif()
        endif()
        set_package_properties( Ipod PROPERTIES DESCRIPTION "Support Apple iPod/iPad/iPhone audio devices" URL "http://sourceforge.net/projects/gtkpod/" TYPE OPTIONAL )
        find_package(GDKPixBuf)
        set_package_properties( GDKPixBuf PROPERTIES DESCRIPTION "Support for artwork on iPod audio devices via GDK-PixBuf" URL "http://developer.gnome.org/arch/imaging/gdkpixbuf.html" TYPE OPTIONAL )
    endif()

    find_package(Mtp 1.0.0)
    set_package_properties(Mtp PROPERTIES
        TYPE OPTIONAL
        PURPOSE "Enable Support for portable media devices that use the media transfer protocol"
    )

    if( WITH_MP3Tunes )
        find_package(CURL)
        set_package_properties( CURL PROPERTIES DESCRIPTION "Used to transfer data with URLs" URL "https://curl.haxx.se/" TYPE OPTIONAL )

        find_package(LibXml2)
        set_package_properties( LibXml2 PROPERTIES DESCRIPTION "LibXML2 is an XML parser required by mp3tunes." URL "http://www.xmlsoft.org" TYPE OPTIONAL )

        find_package(OpenSSL)
        find_package(Libgcrypt)
        if ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
            set (_mp3tunes_crypto TRUE )
        else ()
            message( SEND_ERROR "Building with mp3tunes support REQUIRES either OpenSSL or GNU Libgcrypt" )
        endif ()
        set_package_properties( OpenSSL PROPERTIES DESCRIPTION "OpenSSL or GNU Libgcrypt provides cryptographic functions required by mp3tunes." URL "http://www.openssl.org/ or http://www.gnupg.org/download/#libgcrypt" TYPE OPTIONAL )
        set_package_properties( Libgcrypt PROPERTIES DESCRIPTION "OpenSSL or GNU Libgcrypt provides cryptographic functions required by mp3tunes." URL "http://www.openssl.org/ or http://www.gnupg.org/download/#libgcrypt" TYPE OPTIONAL )

        find_package(Loudmouth)
        set_package_properties( Loudmouth PROPERTIES DESCRIPTION "Loudmouth is the communication backend needed by mp3tunes for syncing." URL "http://www.loudmouth-project.org" TYPE OPTIONAL )

        include(CheckQtGlib)
        set_package_properties( QT5_GLIB PROPERTIES DESCRIPTION "Qt5 must be compiled with glib support for mp3tunes" URL "http://www.trolltech.com" TYPE OPTIONAL )
    endif()

    if( WITH_IPOD OR WITH_MP3Tunes )
        pkg_search_module( GOBJECT REQUIRED gobject-2.0 )
        set_package_properties( GOBJECT PROPERTIES DESCRIPTION "Required by libgpod and mp3tunes." URL "http://www.gtk.org" TYPE OPTIONAL )
        pkg_search_module( GLIB2 glib-2.0 )
        set_package_properties( GLIB2 PROPERTIES DESCRIPTION "Required by libgpod and mp3tunes" URL "http://www.gtk.org" TYPE OPTIONAL )
    endif()

    find_package(Python3 COMPONENTS Interpreter)
    set_package_properties(Python3 PROPERTIES
        DESCRIPTION "Required for generating the autocompletion file for the script console"
        URL "https://www.python.org"
        TYPE OPTIONAL
    )

    find_package(FFTW3 REQUIRED)

    if( BUILD_TESTING AND NOT WIN32 )
        enable_testing()
        add_subdirectory( tests )
    endif()

    add_subdirectory( src )
    add_subdirectory( doc )

    feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

    ki18n_install( po )
    kdoctools_install( po )
endif()

# this needs to be here because also code in shared/ needs config.h. This is also the
# reason why various checks are above why they belong under if( WITH_PLAYER )
configure_file( shared/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/shared/config.h )

add_subdirectory( shared )

if( WITH_UTILITIES )
    add_subdirectory( utilities )
    ecm_install_po_files_as_qm( poqm )
endif()

if( WITH_PLAYGROUND )
    add_subdirectory( playground )
    message(STATUS "Included playground subdirectory in configuration")
endif()
