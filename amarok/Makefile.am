if with_arts
    AMAROKARTS_BUILD = amarokarts
endif

SUBDIRS = amarokcore $(AMAROKARTS_BUILD) plugin analyzers titleproxy engine images data loader

bin_PROGRAMS = amarokapp

INCLUDES = \
        -I$(top_builddir)/amarok/amarok/amarokarts \
        -I$(top_builddir)/amarok/amarok/amarokcore \
        -I$(top_srcdir)/amarok/amarok/amarokcore \
        -I$(top_srcdir)/amarok/amarok/analyzers \
        -I$(top_srcdir)/amarok/amarok/engine \
        -I$(top_srcdir)/amarok/amarok/plugin \
        -I$(top_srcdir)/amarok/amarok/titleproxy \
        -I$(kde_includes)/arts \
        $(taglib_includes) \
        $(all_includes)

amarokapp_SOURCES = \
        Options1.ui \
        Options2.ui \
        Options3.ui \
        Options4.ui \
        Options5.ui \
        amarokconfigdialog.cpp \
        amarokfilelist.cpp \
        amarokfileview.cpp \
        amarokmenu.cpp \
        amarokslider.cpp \
        amaroksystray.cpp \
        browserbar.cpp \
        browserwin.cpp \
        collectionbrowser.cpp \
        effectwidget.cpp \
        enginecontroller.cpp \
        fht.cpp \
        filebrowser.cpp \
        kbookmarkhandler.cpp \
        osd.cpp \
        playerapp.cpp \
        playerwidget.cpp \
        playlistbrowser.cpp \
        playlistitem.cpp \
        playlistloader.cpp \
        playlisttooltip.cpp \
        playlistwidget.cpp \
        pluginmanager.cpp \
        streambrowser.cpp \
        threadweaver.cpp \
        vis/socketserver.cpp


amarokapp_LDADD = \
        $(top_builddir)/amarok/amarok/amarokcore/libamarokcore.a \
        $(top_builddir)/amarok/amarok/titleproxy/libtitleproxy.a \
        $(top_builddir)/amarok/amarok/analyzers/libanalyzers.a \
        $(top_builddir)/amarok/amarok/engine/libengine.a \
        $(top_builddir)/amarok/amarok/plugin/libplugin.a \
        $(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) \
        $(taglib_libs) $(gl_libs)

amarokapp_LDFLAGS = \
    $(all_libraries) \
    $(KDE_RPATH)

METASOURCES = \
    AUTO

KDE_ICON = \
    AUTO

rcdir = \
    $(kde_datadir)/amarok
rc_DATA = \
    amarokui.rc

configdir = \
    $(kde_confdir)
config_DATA = \
    amarokrc

xdg_apps_DATA = \
    amarok.desktop

kde_servicetypes_DATA = \
    amarok_plugin.desktop

messages: rc.cpp
        LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
        if test -n "$$LIST"; then \
         $(XGETTEXT) $$LIST -o $(podir)/amarok.pot; \
        fi
