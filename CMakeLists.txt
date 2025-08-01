cmake_minimum_required(VERSION 3.16)

project(Amarok)

###############

set(KF_MIN_VERSION "6.0.0")
set(KF_MAJOR_VERSION "6")
set(QT_MIN_VERSION "6.7.0")
set(QT_MAJOR_VERSION "6")

find_package(PkgConfig REQUIRED)
find_package(ECM ${KF_MIN_VERSION} REQUIRED CONFIG)
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
include(CheckIncludeFile)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus Gui QuickWidgets Qml Sql Svg Test Widgets Xml)

find_package(Qt${QT_MAJOR_VERSION}QuickControls2 ${QT_MIN_VERSION})
set_package_properties(Qt${QT_MAJOR_VERSION}QuickControls2 PROPERTIES TYPE RUNTIME PURPOSE "Needed by the player's context area")


find_package(Qt${QT_MAJOR_VERSION}SvgWidgets ${QT_MIN_VERSION} REQUIRED)
find_package(Qt${QT_MAJOR_VERSION}WebEngineWidgets ${QT_MIN_VERSION})
set_package_properties(Qt${QT_MAJOR_VERSION}WebEngineWidgets PROPERTIES TYPE OPTIONAL PURPOSE "Needed by the wikipedia applet")

find_package(Qt${QT_MAJOR_VERSION}UiTools ${QT_MIN_VERSION})
set_package_properties(Qt${QT_MAJOR_VERSION}UiTools PROPERTIES TYPE OPTIONAL PURPOSE "Needed by the scripting engine")

find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Codecs
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DBusAddons
    DNSSD
    DocTools
    GlobalAccel
    GuiAddons
    I18n
    IconThemes
    KCMUtils
    KIO
    Notifications
    Package
    Solid
    TextEditor
    TextWidgets
    ThreadWeaver
    WidgetsAddons
    WindowSystem
)

find_package(KF${KF_MAJOR_VERSION}ColorScheme ${KF_MIN_VERSION} REQUIRED)
find_package(KF${KF_MAJOR_VERSION}KCMUtils ${KF_MIN_VERSION} REQUIRED)
find_package(KF${KF_MAJOR_VERSION}StatusNotifierItem ${KF_MIN_VERSION} REQUIRED)
find_package(KF${KF_MAJOR_VERSION}Kirigami ${KF_MIN_VERSION})
set_package_properties(KF${KF_MAJOR_VERSION}Kirigami PROPERTIES TYPE RUNTIME PURPOSE "Needed by the player's context area")

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=1.10)
set_package_properties( GSTREAMER PROPERTIES DESCRIPTION "GStreamer is required for the audio backend" URL "https://gstreamer.freedesktop.org/" TYPE REQUIRED )

pkg_check_modules(GSTREAMER_AUDIO REQUIRED gstreamer-audio-1.0>=1.10)
set_package_properties( GSTREAMER_AUDIO PROPERTIES DESCRIPTION "GStreamer is required for the audio backend" URL "https://gstreamer.freedesktop.org/" TYPE REQUIRED )

pkg_check_modules(GSTREAMER_PLUGINS_BASE REQUIRED gstreamer-plugins-base-1.0>=1.10)
set_package_properties( GSTREAMER_PLUGINS_BASE PROPERTIES DESCRIPTION "GStreamer base plugins are required for the audio backend" URL "https://gstreamer.freedesktop.org/" TYPE REQUIRED )

set(CMAKE_REQUIRED_INCLUDES "${GSTREAMER_INCLUDE_DIRS}")
check_include_file(gst/gst.h HAVE_GST_H)
if(GSTREAMER_FOUND)
    message(STATUS "Found GStreamer: ${GSTREAMER_LIBRARIES}")
endif()
if(NOT HAVE_GST_H)
    message(FATAL_ERROR "Missing GStreamer headers - please install devel packages" )
endif()

pkg_search_module(GLIB REQUIRED glib-2.0)
set_package_properties( GLIB2 PROPERTIES DESCRIPTION "GLib 2 is required to compile the gstreamer audio backend" URL "https://www.gtk.org" TYPE REQUIRED )

pkg_search_module( GOBJECT REQUIRED gobject-2.0 )
set_package_properties( GOBJECT PROPERTIES DESCRIPTION "GObject is required to compile the gstreamer audio backend" URL "https://www.gtk.org" TYPE OPTIONAL )

###############
if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU)
  option(WITH_X11 "Build with X11 integration" ON)
endif()
option(WITH_UTILITIES "Enable building of utilities" ON)
option(WITH_PLAYER "Enable building of main Amarok player" ON)
# option(WITH_MP3Tunes "Enable mp3tunes in the Amarok player, requires multiple extra dependencies" ON) #build is disabled for now, no need to show option
option(WITH_GPODDER "Enable gpodder.net support" ON)
option(WITH_IPOD "Enable iPod support in Amarok" ON)
option(WITH_LASTFM "Enable last.fm support" ON)
option(WITH_EMBEDDED_DB "Build the embedded database library" ON)

############### Taglib
set(TAGLIB_MIN_VERSION "1.12")
find_package(Taglib REQUIRED)
set_package_properties( Taglib PROPERTIES DESCRIPTION "Support for Audio metadata." URL "https://taglib.org/" TYPE REQUIRED PURPOSE "Required for tag reading" )

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

if( WITH_LASTFM )
    find_package( LibLastFm 1.2.0 )
    set( LIBLASTFM_MIN_VERSION "1.2.0" )
    if( LIBLASTFM_FOUND )
        if ( ${LIBLASTFM_MIN_VERSION} VERSION_LESS ${LIBLASTFM_VERSION} )
            set( LIBLASTFM_FOUND TRUE )
        endif()
    endif()
endif()

string( TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER )
if( CMAKE_BUILD_TYPE_TOLOWER MATCHES debug )
    set( DEBUG_BUILD_TYPE ON )
    if (MSVC)
        add_compile_options(/W4 /WX)
    else()
        add_definitions(-Wall -Wextra)
    endif()
endif()

add_subdirectory( data )
add_subdirectory( images )

if( WITH_PLAYER )
    find_package(Threads REQUIRED)

    find_package(MySQL)

    if( NOT MySQL_FOUND )
        find_package(MariaDB REQUIRED)
    endif()

    if( WITH_EMBEDDED_DB )
        find_package(MariaDBd REQUIRED)
        set( BUILD_MYSQLE_COLLECTION TRUE )
        # zlib is required for mysql embedded
        find_package(ZLIB REQUIRED)
        set_package_properties( ZLIB PROPERTIES DESCRIPTION "zlib" TYPE REQUIRED )
    else()
        add_definitions( "-DNO_MYSQL_EMBEDDED" )
        message("Building without embedded MariaDB/MySQL as requested")
    endif()


    # Qt6 support is currently only available in liblastfm forks
    set( LIBLASTFM_MIN_VERSION "1.2.0" )
    set_package_properties( LibLastFm PROPERTIES DESCRIPTION "Enable Last.Fm service, including scrobbling, song submissions, and suggested song dynamic playlists" URL "https://github.com/Mazhoon/liblastfm/" TYPE OPTIONAL )


    if( WITH_GPODDER )
        ## gpodder Service
        find_package(Mygpo-qt6 1.2.0 CONFIG)
        set_package_properties(Mygpo-qt6 PROPERTIES
            DESCRIPTION "A Qt/C++ library wrapping the gpodder.net Webservice."
            URL "http://wiki.gpodder.org/wiki/Libmygpo-qt"
            TYPE OPTIONAL
            PURPOSE "Synchronize podcast subscriptions with gpodder.net"
        )
    endif()

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

