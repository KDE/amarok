INCLUDES = $(all_includes) -I$(kde_includes)/arts
KDE_OPTIONS = nofinal
METASOURCES = AUTO

lib_LTLIBRARIES = libamarokarts.la

libamarokarts_la_SOURCES = winSkinFFT_impl.cpp visQueue.cpp realFFTFilter.cpp realFFT.cpp amarokarts.cc synth_stereo_xfade_impl.cpp

libnoatunarts_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
libnoatunarts_la_LIBADD =  -lkmedia2_idl -lsoundserver_idl -lartsflow

# in case somebody wants to install headers
#include_HEADERS = amarokarts.h

EXTRA_DIST = amarokarts.h realFFT.h realFFTFilter.h visQueue.h winSkinFFT_impl.h

amarokarts.cc amarokarts.h amarokarts.mcoptype amarokarts.mcopclass : $(srcdir)/amarokarts.idl $(MCOPIDL)
	$(MCOPIDL) -t $(INCLUDES) $(srcdir)/amarokarts.idl

mcoptypedir = $(libdir)/mcop
mcoptype_DATA = amarokarts.mcoptype amarokarts.mcopclass

amarokmcopdir = $(libdir)/mcop/Amarok
amarokmcop_DATA = WinSkinFFT.mcopclass Synth_STEREO_XFADE.mcopclass
