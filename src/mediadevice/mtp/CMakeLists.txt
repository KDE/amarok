
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../klegendary
    ${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
    ${CMAKE_CURRENT_SOURCE_DIR}/../../engine
    ${CMAKE_CURRENT_SOURCE_DIR}/../../mediadevice
    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(amarok_mtp-mediadevice_PART_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/mtpmediadevice.cpp )

kde4_add_plugin(amarok_mtp-mediadevice WITH_PREFIX ${amarok_mtp-mediadevice_PART_SRCS})

target_link_libraries(amarok_mtp-mediadevice  ${KDE4_KDECORE_LIBS} )

install(TARGETS amarok_mtp-mediadevice DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES amarok_mtp-mediadevice.desktop DESTINATION ${SERVICES_INSTALL_DIR})


#original Makefile.am contents follow:

#kde_services_DATA = amarok_mtp-mediadevice.desktop
#
#
#INCLUDES = \
#	-I$(top_srcdir)/amarok/src \
#        -I$(top_srcdir)/amarok/src/klegendary \
#	-I$(top_builddir)/amarok/src \
#	-I$(top_srcdir)/amarok/src/amarokcore \
#	-I$(top_builddir)/amarok/src/amarokcore \
#	-I$(top_srcdir)/amarok/src/engine \
#	-I$(top_builddir)/amarok/src/engine \
#	-I$(top_srcdir)/amarok/src/mediadevice \
#	$(taglib_includes) \
#	$(all_includes)
#
#METASOURCES = AUTO
#kde_module_LTLIBRARIES = libamarok_mtp-mediadevice.la
#
#libamarok_mtp_mediadevice_la_LIBADD = $(top_builddir)/amarok/src/libamarok.la \
#	-lkdeui -lkdecore -lmtp
#
#libamarok_mtp_mediadevice_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
#
#noinst_HEADERS = mtpmediadevice.h playlist.h track.h
#
#libamarok_mtp_mediadevice_la_SOURCES = mtpmediadevice.cpp playlist.cpp \
#	track.cpp
