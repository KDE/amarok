noinst_LTLIBRARIES = libstatusbar.la

METASOURCES = AUTO

INCLUDES = \
    -I$(top_srcdir)/amarok/src/ \
    -I$(top_builddir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/amarokcore \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_builddir)/amarok/src/ \
    $(all_includes)
    
libstatusbar_la_LIBADD = $(LIB_QT)

libstatusbar_la_SOURCES = \
    statusBarBase.cpp \
    statusbar.cpp \
    overlayWidget.cpp \
    popupMessage.cpp \
    progressBar.cpp \
    squeezedtextlabel.cpp \
    queueLabel.cpp
