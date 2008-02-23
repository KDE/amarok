INCLUDES = -I$(top_builddir)/amarok/src/amarokcore     -I$(top_srcdir)/amarok/src/amarokcore     -I$(top_srcdir)/amarok/src/analyzers     -I$(top_srcdir)/amarok/src/plugin     -I$(top_srcdir)/amarok/src/statusbar     -I$(top_srcdir)/amarok/src/mediadevice     -I$(top_srcdir)/amarok/src/device     -I$(top_srcdir)/amarok/src     -I$(kde_includes)/arts     $(TAGLIB_CFLAGS)     $(sqlite_includes)     $(mysql_includes)     $(postgresql_includes)     $(EXSCALIBAR_CFLAGS)     $(all_includes)
METASOURCES = AUTO


noinst_LTLIBRARIES = libmagnatunebrowser.la
libmagnatunebrowser_la_SOURCES = magnatuneartistinfobox.cpp \
	magnatunebrowser.cpp magnatunedownloaddialogbase.ui magnatunedownloaddialog.cpp \
	magnatunepurchasedialogbase.ui magnatunepurchasedialog.cpp magnatunepurchasehandler.cpp \
	magnatunetypes.cpp magnatunexmlparser.cpp magnatunedatabasehandler.cpp \
	magnatunelistviewitems.cpp magnatunelistview.cpp magnatuneredownloaddialog.cpp \
	magnatuneredownloadhandler.cpp magnatunedownloadinfo.cpp magnatunealbumdownloader.cpp \
	magnatuneredownloaddialogbase.ui
noinst_HEADERS = magnatunedatabasehandler.h magnatunelistviewitems.h \
	magnatunelistview.h magnatuneartistinfobox.h magnatunebrowser.h magnatunedownloaddialog.h \
	magnatunepurchasedialog.h magnatunepurchasehandler.h magnatunetypes.h magnatunexmlparser.h \
	magnatunedownloadinfo.h magnatunealbumdownloader.h
