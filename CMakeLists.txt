project(Amarok)

cmake_minimum_required(VERSION 2.6.2)

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

message(STATUS "${CMAKE_MODULE_PATH}")

option(WITH_UTILITIES "Enable building of utilities" ON)
option(WITH_PLAYER "Enable building of main Amarok player" ON)
option(WITH_MP3Tunes "Enable mp3tunes in the Amarok player, requires multiple extra dependencies" ON)
option(WITH_IPOD "Enable iPod support in Amarok" ON)
option(WITH_MYSQL_EMBEDDED "Build the embedded database library -- highly recommended" ON)
option(WITH_PLAYGROUND "Enable building of playground scripts and applets (WARNING: some of them might have legal issues!)" OFF)

include(CheckLibraryExists)
check_library_exists(dl dlopen "" LIBDL_FOUND)

# --------- taglib --------
# building without taglib is stupid, but it's possible

set(TAGLIB_MIN_VERSION "1.7")
find_package(Taglib)

if(TAGLIB_FOUND)
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

    set(CMAKE_REQUIRED_INCLUDES)
    set(CMAKE_REQUIRED_LIBRARIES)

    set(TAGLIB-EXTRAS_MIN_VERSION "1.0")
    find_package(Taglib-Extras)
    set(TAGLIB_EXTRAS_FOUND ${TAGLIB-EXTRAS_FOUND}) # we need a c-compatible name for the include file

    include(CheckTagLibFileName)

    check_taglib_filename(COMPLEX_TAGLIB_FILENAME)
else()
    message(WARNING "You are building Amarok without taglib. This has been known to cause many strange behaviours and is strongly advised against.")
endif()

# --------------------------

# Figure out problems inside KAboutData before re-enabling
#execute_process( COMMAND "git" "describe" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} RESULT_VARIABLE GIT_OUTPUT_VALUE OUTPUT_VARIABLE CURRENT_GIT_VERSION )
#if(GIT_OUTPUT_VALUE EQUAL 0)
#    message(STATUS "Building git version ${CURRENT_GIT_VERSION}")
#endif(GIT_OUTPUT_VALUE EQUAL 0)


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

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/shared )

if( WITH_PLAYER )
    set(KDE_MIN_VERSION "4.6.0")
    find_package( KDE4 REQUIRED )
    include (KDE4Defaults)
    include (MacroLibrary)

    include(MacroBoolTo01)
    include(MacroLogFeature)

    macro_log_feature( KDE4_FOUND "kdelibs" "The toolkit Amarok uses to build" "http://www.kde.org" TRUE "4.6.0" "" )

    find_package( QtScriptQtBindings )
    macro_log_feature( QTSCRIPTQTBINDINGS_FOUND "qtscript-qt" "QtScript Qt Bindings" "http://code.google.com/p/qtscriptgenerator/" FALSE "" "" )

    add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

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

    # QCA2 is required for the Script Updater
    find_package(QCA2)
    macro_log_feature( QCA2_FOUND "qca2" "Qt Cryptographic Architecture" "http://delta.affinix.com/qca/" FALSE "" "" )

    # OpenGL is required for the spectrum analyzer
    find_package(OpenGL)
    macro_log_feature( OPENGL_FOUND "OpenGL" "Support for hardware acceleration (enables the Spectrum Analyzer)" FALSE "" "" )

    # QJson is required for the PlaydarCollection
    macro_optional_find_package(QJSON)
    macro_log_feature( QJSON_FOUND "QJson" "Qt JSON Parser used for the Playdar Collection" "http://qjson.sourceforge.net/" FALSE "" "" )

    # macro_optional_find_package(Strigi)
    # macro_log_feature( STRIGI_FOUND "strigi" "Index metadata of files" "http://strigi.sourceforge.net" FALSE "" "" )

    macro_optional_find_package(LibLastFm)
    macro_log_feature( LIBLASTFM_FOUND "liblastfm" "Enable Last.Fm service, including scrobbling, song submissions, and suggested song dynamic playlists" "http://cdn.last.fm/src/liblastfm-0.3.0.tar.bz2" FALSE "0.3" "" )
    macro_bool_to_01( LIBLASTFM_FOUND HAVE_LIBLASTFM )

    macro_optional_find_package( FFmpeg )
    macro_log_feature( FFMPEG_FOUND "ffmpeg" "Libraries and tools for handling multimedia data" "http://www.ffmpeg.org/" FALSE "0.6" "" )

    if( FFMPEG_FOUND )
        macro_optional_find_package(LibOFA)
        macro_log_feature( LIBOFA_FOUND "libofa" "Enable MusicDNS service" "http://code.google.com/p/musicip-libofa/" FALSE "0.9.x" "" )
        macro_bool_to_01( LIBOFA_FOUND HAVE_LIBOFA )
    endif( FFMPEG_FOUND )

    ##gpodder Service
    macro_optional_find_package( Mygpo-qt )
    macro_log_feature( LIBMYGPO_QT_FOUND "libmygpo-qt" "Enable gpodder.net service" "http://wiki.gpodder.org/wiki/Libmygpo-qt" FALSE "1.0.5" "" )
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

    #These two are currently unused
    #macro_optional_find_package(Soprano)
    #macro_log_feature( Soprano_FOUND "soprano" "Soprano is a RDF framework required by Nepomuk Collection" "http://soprano.sourceforge.net" FALSE "2.1.0" "")
    #macro_optional_find_package(Nepomuk)
    #macro_log_feature( NEPOMUK_FOUND "nepomuk" "Nepomuk Libraries required by Nepomuk Collection (part of kdelibs)" "http://www.kde.org" FALSE "4.1" "")

    macro_bool_to_01(LIBVISUAL_FOUND HAVE_LIBVISUAL)

    string( TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER )
    if( CMAKE_BUILD_TYPE_TOLOWER MATCHES debug )
        set( DEBUG_BUILD_TYPE ON )
    endif( CMAKE_BUILD_TYPE_TOLOWER MATCHES debug )

    configure_file(config-amarok.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-amarok.h )

    include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

    if( KDE4_BUILD_TESTS AND NOT WIN32 )
        ENABLE_TESTING()
        add_subdirectory( tests )
    endif( KDE4_BUILD_TESTS AND NOT WIN32 )

    add_subdirectory( src )
    add_subdirectory( external )

    # Also display taglib in the feature log
    macro_log_feature( TAGLIB_FOUND "taglib" "Support for Audio metadata." "http://developer.kde.org/~wheeler/taglib.html" FALSE "" "STRONGLY RECOMMENDED" )

    macro_display_feature_log()

    #Do not remove or modify these.  The release script substitutes in for these
    #comments with appropriate doc and translation directories.
    #PO_SUBDIR
    #DOC_SUBDIR

else( WITH_PLAYER )

    find_package( Qt4 REQUIRED )
    set( CMAKE_INCLUDE_CURRENT_DIR ON )
    add_definitions(${QT_DEFINITIONS})
    include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

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
