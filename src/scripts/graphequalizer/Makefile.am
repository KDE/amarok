graphequalizerdir = $(kde_datadir)/amarok/scripts/graphequalizer

graphequalizer_PROGRAMS = graphequalizer

INCLUDES = \
        $(all_includes)

# eqdialog.ui.h removed because of automake turning it into just '.h' for some reason
# and then complaining about its inexistence

graphequalizer_SOURCES = \
        equalizercanvasview.cpp \
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

