####### kdevelop will overwrite this part!!! (begin)##########
lib_LTLIBRARIES = libamarokarts.la


INCLUDES = $(all_includes)


LDFLAGS =                     


libamarokarts_la_LDFLAGS = -avoid-version -version-info 0:0:0

libamarokarts_la_METASOURCES = AUTO

libamarokarts_la_SOURCES = winSkinFFT_impl.cpp visQueue.cpp realFFTFilter.cpp realFFT.cpp amarokarts.cc 


EXTRA_DIST = amarokarts.h amarokarts.cc realFFT.cpp realFFT.h realFFTFilter.cpp realFFTFilter.h visQueue.cpp visQueue.h winSkinFFT_impl.cpp winSkinFFT_impl.h amarokarts.mcopclass amarokarts.mcoptype WinSkinFFT.mcopclass 

install-data-local:
	$(mkinstalldirs) $(kde_includes)/amarok/
	$(INSTALL_DATA) $(srcdir)/amarokarts.h $(kde_includes)/amarok/amarokarts.h
	$(mkinstalldirs) $(kde_libraries)/mcop/
	$(INSTALL_DATA) $(srcdir)/amarokarts.mcopclass $(kde_libraries)/mcop/amarokarts.mcopclass
	$(mkinstalldirs) $(kde_libraries)/mcop/
	$(INSTALL_DATA) $(srcdir)/amarokarts.mcoptype $(kde_libraries)/mcop/amarokarts.mcoptype
	$(mkinstalldirs) $(kde_libraries)/mcop/Amarok/
	$(INSTALL_DATA) $(srcdir)/WinSkinFFT.mcopclass $(kde_libraries)/mcop/Amarok/WinSkinFFT.mcopclass

uninstall-local:
	-rm -f $(kde_includes)/amarok/amarokarts.h
	-rm -f $(kde_libraries)/mcop/amarokarts.mcopclass
	-rm -f $(kde_libraries)/mcop/amarokarts.mcoptype
	-rm -f $(kde_libraries)/mcop/Amarok/WinSkinFFT.mcopclass

####### kdevelop will overwrite this part!!! (end)############
