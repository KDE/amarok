INCLUDES = $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = konqsidebar_universalamarok.la

konqsidebar_universalamarok_la_SOURCES = universalamarok.cpp
konqsidebar_universalamarok_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries) -lkonqsidebarplugin
konqsidebar_universalamarok_la_LIBADD = $(LIB_KHTML) $(LIB_KPARTS) $(LIB_KFILE)



universalamarok_entry_DATA = amarok.desktop
universalamarok_entrydir = $(kde_datadir)/konqsidebartng/add
noinst_HEADERS = universalamarok.h
