INCLUDES = \
    -I$(top_srcdir)/amarok/amarok/engine \
    -I$(top_srcdir)/amarok/amarok \
    $(CFLAGS_GST) \
    $(all_includes)

gstengine_la_LIBADD = \
    $(top_builddir)/amarok/amarok/engine/libengine.a \
    $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
    $(LIB_GST)

kde_module_LTLIBRARIES = gstengine.la

gstengine_la_SOURCES = gstengine.cpp

gstengine_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

METASOURCES = AUTO

plugin_DATA = gstengine.plugin
plugindir = $(kde_datadir)/amarok

