INCLUDES = $(all_includes)
METASOURCES = AUTO

lib_LTLIBRARIES = libamarokarts.la

libamarokarts_la_LDFLAGS = -avoid-version -version-info 0:0:0
libamarokarts_la_SOURCES = winSkinFFT_impl.cpp visQueue.cpp realFFTFilter.cpp realFFT.cpp amarokarts.cc

# in case somebody wants to install headers
#include_HEADERS = amarokarts.h

EXTRA_DIST = amarokarts.h realFFT.h realFFTFilter.h visQueue.h winSkinFFT_impl.h

mcoptypedir = $(libdir)/mcop
mcoptype_DATA = amarokarts.mcoptype amarokarts.mcopclass

amarokmcopdir = $(libdir)/mcop/Amarok
amarokmcop_DATA = WinSkinFFT.mcopclass
