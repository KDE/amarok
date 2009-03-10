project(Amarok)

cmake_minimum_required(VERSION 2.6.2)


#We depend on kde4.2 now.. I believe this is the easiest way to do it.
set(KDE_MIN_VERSION "4.2.0")
find_package( KDE4 REQUIRED )
include (KDE4Defaults)
include (MacroLibrary)

include(CheckTypeSize)
include(MacroBoolTo01)
include(MacroLogFeature)

macro_log_feature( KDE4_FOUND "kdelibs" "The toolkit Amarok uses to build" "http://www.kde.org" TRUE "4.2.0" "" )

set( CMAKE_MODULE_PATH    ${CMAKE_MODULE_PATH}
                          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

#this line has to be before add_defintions, but after the KDE4 cmake includes
#because they are used to determine install locations
include(CheckQtScriptBindings)
if( NOT QTSCRIPT_QT_BINDINGS_FOUND )
#we're keeping the script generator in our code to give some
#time to transition to depending on it seperate
    add_subdirectory( src/scriptengine/generator )
endif( NOT QTSCRIPT_QT_BINDINGS_FOUND )

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include(CheckLibraryExists)
check_library_exists(dl dlopen "" LIBDL_FOUND)

set(TAGLIB_MIN_VERSION "1.5")
find_package(Taglib)
macro_log_feature( TAGLIB_FOUND "taglib" "Read and write tags in audio files" "http://www.freshmeat.net/projects/taglib" TRUE "1.5.0" "")


find_package(Strigi REQUIRED)
macro_log_feature( STRIGI_FOUND "strigi" "Index metadata of files" "http://strigi.sourceforge.net" TRUE "" "" )

find_package(MySQLAmarok REQUIRED)
macro_log_feature( MYSQL_EMBEDDED_FOUND "mysqld" "Embedded MySQL" "http://www.mysql.com" TRUE "" "" )

# zlib is required for mysql embedded
find_package(ZLIB REQUIRED)
macro_log_feature( ZLIB_FOUND "zlib" "zlib" "" TRUE "" "" )

include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${TAGLIB_INCLUDES})
include(CheckTagLibFileName)

#macro_optional_find_package(Njb)
#macro_log_feature( NJB_FOUND "libnjb" "Support Creative Labs nomad jukebox devices" "http://www.sf.net/projects/libnjb" FALSE "2.2.4" "" )

#macro_optional_find_package(Ifp)
#macro_log_feature( IFP_FOUND "libifp" "Support iRiver iFP audio devices" "http://ifp-driver.sourceforge.net/libifp/" FALSE "1.0.0.2" "")

macro_optional_find_package(GLIB2)
macro_log_feature( GLIB2_FOUND "glib2" "Required by libgpod and mp3tunes" "http://www.gtk.org" FALSE "2.x" "")

macro_optional_find_package(Gdk)
macro_log_feature( GDK_FOUND "Gdk" "Support for artwork on iPod audio devices via GdkPixbuf" "http://developer.gnome.org/arch/imaging/gdkpixbuf.html" FALSE "2.0.x" "" )

macro_optional_find_package(Ipod)
if(IPOD_FOUND)
macro_ensure_version("0.7.0" ${IPOD_VERSION} IPOD_0_7)
endif(IPOD_FOUND)
macro_log_feature( IPOD_0_7 "libgpod" "Support Apple iPod audio devices" "http://sourceforge.net/projects/gtkpod/" FALSE "0.7.0" "" )

#macro_optional_find_package(MySQL)
#macro_log_feature( MYSQL_FOUND "mysql" "Store Collection in a mysql database instead of a sqlite one" "http://www.mysql.com" FALSE "4.1.0" "" )

macro_optional_find_package(Mtp)
macro_log_feature( MTP_FOUND "libmtp" "Enable Support for portable media devices that use the media transfer protocol" "http://libmtp.sourceforge.net/" FALSE "0.3.0" "")

#macro_optional_find_package(OpenGL)
#macro_log_feature( OPENGL_FOUND "opengl" "Support for gl-accellerated visualization rendering" "" FALSE "" "")

find_package(CURL)
macro_log_feature( CURL_FOUND "curl" "cURL provides the necessary network libraries required by mp3tunes." "http://curl.haxx.se" FALSE "" "" )

find_package(LibXml2)
macro_log_feature( LIBXML2_FOUND "libxml2" "LibXML2 is an XML parser required by mp3tunes." "http://www.xmlsoft.org" FALSE "" "" )

find_package(OpenSSL)
find_package(Libgcrypt)
if ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
set (_mp3tunes_crypto TRUE )
endif ( OPENSSL_FOUND OR LIBGCRYPT_FOUND )
macro_log_feature( _mp3tunes_crypto "openssl or libgcrypt" "OpenSSL or GNU Libgcrypt provides cryptographic functions required by mp3tunes." "http://www.openssl.org/ or http://www.gnupg.org/download/#libgcrypt" FALSE "" "" )

find_package(GObject)
macro_log_feature( GOBJECT_FOUND "gobject" "Required by mp3tunes." "http://www.gtk.org" FALSE "2.x" "" )

macro_optional_find_package(Loudmouth)
macro_log_feature( LOUDMOUTH_FOUND "loudmouth" "Loudmouth is the communication backend needed by mp3tunes for synching." "http://www.loudmouth-project.org" FALSE "" "" )

#find_package(KdeMultimedia)
#macro_log_feature( KDEMULTIMEDIA_FOUND "kdemultimedia" "Support for playing audio cds" "http://www.kde.org" FALSE "4.0" "")

macro_optional_find_package(Soprano)
macro_log_feature( Soprano_FOUND "soprano" "Soprano is a RDF framework required by Nepomuk Collection" "http://soprano.sourceforge.net" FALSE "2.1.0" "")

macro_optional_find_package(Nepomuk)
macro_log_feature( NEPOMUK_FOUND "nepomuk" "Nepomuk Libraries required by Nepomuk Collection (part of kdelibs)" "http://www.kde.org" FALSE "4.1" "")

#macro_optional_find_package(LibLastFm)
#macro_log_feature( LIBLASTFM_FOUND "liblastfm" "LibLastFM is needed for amarok to support last.fm." "http://www.last.fm" FALSE "" "" )


include(CheckQtGlib)
macro_log_feature(QT4_GLIB_SUPPORT "Qt4 Glib support" "Qt4 must be compiled with glib support for mp3tunes" "http://www.trolltech.com" FALSE "" "")

macro_optional_find_package(QCA2)
macro_log_feature( QCA2_FOUND "Qt Cryptographic Architecture 2" "Requiered for building the Ampache service" "http://delta.affinix.com/download/qca/2.0/" FALSE "2.0" "")

macro_bool_to_01(LIBVISUAL_FOUND HAVE_LIBVISUAL)

check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)

check_taglib_filename(COMPLEX_TAGLIB_FILENAME)

configure_file(config-amarok.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-amarok.h )

add_subdirectory( src )
add_subdirectory( tests )

macro_display_feature_log()


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0")


