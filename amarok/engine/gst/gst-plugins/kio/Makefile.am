# location where plug-in will be installed
plugindir= $(libdir)/gstreamer-@GST_MAJORMINOR@

# change libgstplugin.la to something more suitable
plugin_LTLIBRARIES = libgstkio.la

# sources used to compile this plug-in
libgstkio_la_SOURCES = gstkiosrc.cc

# flags used to compile this plugin
# we use the GST_LIBS flags because we might be using plug-in libs
libgstkio_la_CFLAGS = $(GST_LIBS_CFLAGS)
libgstkio_la_LDFLAGS = $(GST_LIBS_LIBS)

# headers we need but don't want installed
noinst_HEADERS = gstkio.h
