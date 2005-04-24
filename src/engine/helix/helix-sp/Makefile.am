noinst_LTLIBRARIES = libhelix-sp.la

AM_CPPFLAGS = \
    -D_UNIX \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/ \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/runtime \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/include \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/client/include \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/container \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/system \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/dbgtool \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/util \
    -include $(top_srcdir)/amarok/src/engine/helix/helix-sp/helixdefines.h \
    $(all_includes)

libhelix_sp_la_SOURCES = \
    hspcontext.cpp \
    hspadvisesink.cpp \
    hsperror.cpp \
    hspauthmgr.cpp \
    iids.cpp \
    print.cpp \
    utils.cpp \
    helix-sp.cpp

METASOURCES = \
    AUTO

