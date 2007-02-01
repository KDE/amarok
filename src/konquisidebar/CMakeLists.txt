
include_directories( ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(konqsidebar_universalamarok_PART_SRCS universalamarok.cpp )

kde4_automoc(${konqsidebar_universalamarok_PART_SRCS})

kde4_add_dcop_stubs(konqsidebar_universalamarok_PART_SRCS ${CMAKE_SOURCE_DIR}/amarok/src/amarokcore/amarokdcopiface.h )


kde4_add_plugin(konqsidebar_universalamarok ${konqsidebar_universalamarok_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} konqsidebar_universalamarok )

target_link_libraries(konqsidebar_universalamarok  ${KDE4_KDECORE_LIBS} konqsidebarplugin ${KDE4_KHTML_LIBS} ${KDE4_KPARTS_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} konqsidebar_universalamarok )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/konqsidebartng/entries FILES  amarok.desktop )
install_files( ${DATA_INSTALL_DIR}/konqsidebartng/kicker_entries FILES  amarok.desktop )
install_files( ${DATA_INSTALL_DIR}/konqsidebartng/add FILES  amarok.desktop )



#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/amarok/src/amarokcore $(all_includes)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = konqsidebar_universalamarok.la
#
#konqsidebar_universalamarok_la_SOURCES = universalamarok.cpp amarokdcopiface.stub
#konqsidebar_universalamarok_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries) -lkonqsidebarplugin
#konqsidebar_universalamarok_la_LIBADD = $(LIB_KHTML) $(LIB_KPARTS) $(LIB_KFILE)
#
#
#amarokdcopiface_DIR=$(top_srcdir)/amarok/src/amarokcore
#universalamarok_add_DATA = amarok.desktop
#universalamarok_adddir = $(kde_datadir)/konqsidebartng/add
#
#universalamarok_kickerentries_DATA = amarok.desktop
#universalamarok_kickerentriesdir = $(kde_datadir)/konqsidebartng/kicker_entries
#
#universalamarok_entries_DATA = amarok.desktop
#universalamarok_entriesdir = $(kde_datadir)/konqsidebartng/entries
#
#noinst_HEADERS = universalamarok.h 
