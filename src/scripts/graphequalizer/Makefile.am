graphequalizerdir = $(kde_datadir)/amarok/scripts/graphequalizer

graphequalizer_PROGRAMS = graphequalizer

INCLUDES = \
        $(all_includes)

graphequalizer_SOURCES = \
        equalizercanvasview.cpp \
        eqdialog.ui.h \
        eqdialog.ui \
        stdinreader.h \
        main.cpp

graphequalizer_DATA = README

graphequalizer_LDADD = \
        $(LIB_QT) \
        $(LIB_KDEUI) \
        $(LIB_KDECORE)

graphequalizer_LDFLAGS = $(all_libraries) $(KDE_RPATH)

METASOURCES = AUTO

