noinst_LTLIBRARIES = libstatusbar.la

libstatusbar_la_METASOURCES = popupMessage.moc statusbar.moc statusBarBase.moc

INCLUDES = \
    -I$(top_srcdir)/amarok/src/ \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_builddir)/amarok/src/ \
    $(all_includes)

libstatusbar_la_SOURCES = \
    statusBarBase.cpp \
    statusbar.cpp \
    overlayWidget.cpp \
    progressBar.cpp
