INCLUDES = -I$(top_srcdir)/amarok/src/amarokcore $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = konqsidebar_universalamarok.la

konqsidebar_universalamarok_la_SOURCES = universalamarok.cpp amarokdcopiface.stub
konqsidebar_universalamarok_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries) -lkonqsidebarplugin
konqsidebar_universalamarok_la_LIBADD = $(LIB_KHTML) $(LIB_KPARTS) $(LIB_KFILE)


amarokdcopiface_DIR=$(top_srcdir)/amarok/src/amarokcore
universalamarok_add_DATA = amarok.desktop
universalamarok_adddir = $(kde_datadir)/konqsidebartng/add

universalamarok_kickerentries_DATA = amarok.desktop
universalamarok_kickerentriesdir = $(kde_datadir)/konqsidebartng/kicker_entries

universalamarok_entries_DATA = amarok.desktop
universalamarok_entriesdir = $(kde_datadir)/konqsidebartng/entries

noinst_HEADERS = universalamarok.h 
