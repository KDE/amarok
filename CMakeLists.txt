project(Amarok)


#We depend on kde4.1 now.. I believe this is the easiest way to do it.
#set(KDE_MIN_VERSION "4.00.61")
find_package(KDE4)

include (KDE4Defaults)
include (MacroLibrary)

include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckPrototypeExists)
include(CheckTypeSize)
include(MacroBoolTo01)
include(MacroLogFeature)

macro_log_feature( KDE4_FOUND "kdelibs" "The toolkit Amarok uses to build" "http://www.kde.org" TRUE "4.00.61" "" )

set( CMAKE_MODULE_PATH    ${CMAKE_MODULE_PATH}
                          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

find_package(Taglib)
macro_log_feature( TAGLIB_FOUND "taglib" "Read and write tags in audio files" "http://www.freshmeat.net/projects/taglib" TRUE "1.5.0" "")
if (WIN32)
    find_package(Ruby)
#NOTE: cmake's FindRuby does not set a RUBY_FOUND variable, making this macro not possible.
#     macro_log_feature( Ruby_FOUND "ruby" "Support for various services within amarok that use Ruby" "http://www.ruby.org" FALSE "1.8" "")
else (WIN32)
    find_package(Ruby REQUIRED)
#NOTE: cmake's FindRuby does not set a RUBY_FOUND variable, making this macro not possible.
#     macro_log_feature( Ruby_FOUND "ruby" "Support for various services within amarok that use Ruby" "http://www.ruby.org" TRUE "1.8" "")
endif (WIN32)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${TAGLIB_INCLUDES})
include(CheckTagLibFileName)

macro_optional_find_package(Njb)
macro_log_feature( NJB_FOUND "libnjb" "Support Creative Labs nomad jukebox devices" "http://www.sf.net/projects/libnjb" FALSE "2.2.4" "" )

macro_optional_find_package(Ifp)
macro_log_feature( IFP_FOUND "libifp" "Support iRiver iFP audio devices" "http://ifp-driver.sourceforge.net/libifp/" FALSE "1.0.0.2" "")

macro_optional_find_package(MySQL)
macro_log_feature( MYSQL_FOUND "mysql" "Store Collection in a mysql database instead of a sqlite one" "http://www.mysql.com" FALSE "4.1.0" "" )

macro_optional_find_package(PostgreSQL)
macro_log_feature( POSTGRESQL_FOUND "postgresql" "Store Collection in a postgresql database instead of a sqlite one" "http://www.postgresql.org" FALSE "7.4" "" )

macro_optional_find_package(Libvisual)
macro_log_feature( LIBVISUAL_FOUND "libvisual" "Enable support for visualizations" "http://localhost.nl/~synap/libvisual/jj" FALSE "0.4.0" "")

macro_optional_find_package(Mp4v2)
macro_log_feature( MP4V2_FOUND "libmp4v2" "Enable tagging of mp4 files from within amarok" "http://www.sf.net/projects/mpeg4ip" FALSE "" "" )

macro_optional_find_package(Mtp)
macro_log_feature( MTP_FOUND "libmtp" "Enable Support for portable media devices that use the media transfer protocol" "http://libmtp.sourceforge.net/" FALSE "0.1.1" "")

macro_optional_find_package(OpenGL)
macro_log_feature( OPENGL_FOUND "opengl" "Support for gl-accellerated visualization rendering" "" FALSE "" "")

find_package(KdeMultimedia)
macro_log_feature( KDEMULTIMEDIA_FOUND "kdemultimedia" "Support for playing audio cds" "http://www.kde.org" FALSE "4.0" "")


macro_bool_to_01(LIBVISUAL_FOUND HAVE_LIBVISUAL)

check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)

check_taglib_filename(COMPLEX_TAGLIB_FILENAME)

configure_file (config-amarok.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-amarok.h )

add_subdirectory( src )

macro_display_feature_log()
