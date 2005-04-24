noinst_LTLIBRARIES = libhelix-sp.la

INCLUDES = \
	-I$(top_srcdir)/amarok/src/engine \
	-I$(top_srcdir)/amarok/src/ \
	-I$(top_srcdir)/amarok/src/engine/helix-sp/helix-include/runtime \
	-I$(top_srcdir)/amarok/src/engine/helix-sp/helix-include/common/include \
	-I$(top_srcdir)/amarok/src/engine/helix-sp/helix-include/client/include \
	-I$(top_srcdir)/amarok/src/engine/helix-sp/helix-include/common/container \
	-I$(top_srcdir)/amarok/src/engine/helix-sp/helix-include/common/system \
	-I$(top_srcdir)/amarok/src/engine/helix-sp/helix-include/common/dbgtool \
	-I$(top_srcdir)/amarok/src/engine/helix-sp/helix-include/common/util \
	-include helixdefines.h \
    $(all_includes)

libhelix_sp_la_SOURCES = \
	iids.cpp \
    hspcontext.cpp \
    hspadvisesink.cpp \
    hsperror.cpp \
    hspauthmgr.cpp \
    print.cpp \
    utils.cpp \
    helix-sp.cpp

METASOURCES = \
    AUTO

