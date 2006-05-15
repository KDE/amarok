imagesdir = $(kde_datadir)/amarok/images
images_DATA = \
    nmm-gradient-left.png \
    nmm-gradient-right.png \
    nmm-volume-inset.png

kde_module_LTLIBRARIES = \
    libamarok_nmmengine_plugin.la

INCLUDES = \
    -I$(top_srcdir)/amarok/src \
    $(CFLAGS_NMM) \
    $(all_includes)

libamarok_nmmengine_plugin_la_LIBADD = \
    $(top_builddir)/amarok/src/libamarok.la \
    $(top_builddir)/amarok/src/plugin/libplugin.la \
    $(LIB_KDECORE) $(LIB_KFILE) \
    -lnmmbase -lnmmgraphmgr -lnmmiprogress -lnmmiaudiodevice -lnmmigeneral

libamarok_nmmengine_plugin_la_SOURCES = \
    nmm_engine.cpp \
    nmm_configdialog.cpp \
    nmm_configdialogbase.ui \
    NmmLocation.cpp \
    HostList.cpp \
    HostListItem.cpp \
    ServerregistryPing.cpp \
    nmm_kdeconfig.kcfgc

libamarok_nmmengine_plugin_la_LDFLAGS = \
    -module \
    $(KDE_PLUGIN) \
    $(LDFLAGS_NMM) \
    $(all_libraries)

METASOURCES = \
    AUTO

KDE_CXXFLAGS = $(USE_EXCEPTIONS)

kde_services_DATA = \
    amarok_nmmengine_plugin.desktop

kde_kcfg_DATA = nmm_kdeconfig.kcfg

SUBDIRS = icons
