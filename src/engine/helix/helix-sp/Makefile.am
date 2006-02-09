noinst_LTLIBRARIES = libhelix-sp.la

# must disable HX_LOG_SUBSYSTEM before commiting!!
#    -DHX_LOG_SUBSYSTEM

AM_CPPFLAGS = \
    -D_UNIX \
    -Wall -Wreturn-type -DHELIX_CONFIG_DISABLE_ATOMIC_OPERATORS \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/ \
    -I$(srcdir)/helix-include/runtime \
    -I$(srcdir)/helix-include/audio/fixptutil \
    -I$(srcdir)/helix-include/common/include \
    -I$(srcdir)/helix-include/client/include \
    -I$(srcdir)/helix-include/common/container \
    -I$(srcdir)/helix-include/common/system \
    -I$(srcdir)/helix-include/common/dbgtool \
    -I$(srcdir)/helix-include/common/util \
    -I$(srcdir)/helix-include/common/log \
    -include $(srcdir)/helixdefines.h \
    $(all_includes)

libhelix_sp_la_SOURCES = \
    hspcontext.cpp \
    hspadvisesink.cpp \
    hsperror.cpp \
    hspauthmgr.cpp \
    hsphook.cpp \
    iids.cpp \
    utils.cpp \
    gain.cpp \
    hspalsadevice.cpp \
    helix-sp.cpp

METASOURCES = \
    AUTO

KDE_OPTIONS=nofinal