INCLUDES = $(all_includes) -I$(kde_includes)/arts
KDE_OPTIONS = nofinal
METASOURCES = AUTO

lib_LTLIBRARIES = libamarokarts.la

libamarokarts_la_SOURCES = amarokarts.cc synth_stereo_xfade_impl.cpp rawscope_impl.cpp

libamarokarts_la_LDFLAGS = $(all_libraries) -avoid-version -no-undefined
libamarokarts_la_LIBADD =  -lkmedia2_idl -lsoundserver_idl -lartsflow

# in case somebody wants to install headers
#include_HEADERS = amarokarts.h

EXTRA_DIST = amarokarts.h

amarokarts.cc amarokarts.h amarokarts.mcoptype amarokarts.mcopclass : $(srcdir)/amarokarts.idl $(MCOPIDL)
	$(MCOPIDL) -t $(INCLUDES) $(srcdir)/amarokarts.idl

mcoptypedir = $(libdir)/mcop
mcoptype_DATA = amarokarts.mcoptype amarokarts.mcopclass

amarokmcopdir = $(libdir)/mcop/Amarok
amarokmcop_DATA = Synth_STEREO_XFADE.mcopclass RawScope.mcopclass

CLEANFILES = amarokarts.cc amarokarts.h amarokarts.mcopclass amarokarts.mcoptype
#noinst_HEADERS =  amarokconfig.h

rawscope_impl.lo : amarokarts.h
