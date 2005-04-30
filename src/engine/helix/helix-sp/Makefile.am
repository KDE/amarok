noinst_LTLIBRARIES = libhelix-sp.la

# can also be pentiumpro,pentium,i486,i586,i686,amd64,x86_64,alpha,sparc,powerpc
# need to figure out how to get automake to set this instead...
arch=pentium

AM_CPPFLAGS = \
    -D_UNIX \
    -Wall -Wreturn-type -fno-exceptions -march=$(arch) \
    -I$(top_srcdir)/amarok/src/engine \
    -I$(top_srcdir)/amarok/src/ \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/runtime \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/include \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/client/include \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/container \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/system \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/dbgtool \
    -I$(top_srcdir)/amarok/src/engine/helix/helix-sp/helix-include/common/util \
    -I. \
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

