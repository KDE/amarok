####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = startupTip.txt 

install-data-local:
	$(mkinstalldirs) $(kde_datadir)/amarok/data/
	$(INSTALL_DATA) $(srcdir)/startupTip.txt $(kde_datadir)/amarok/data/startupTip.txt

uninstall-local:
	-rm -f $(kde_datadir)/amarok/data/startupTip.txt

####### kdevelop will overwrite this part!!! (end)############
datadir = $(kde_datadir)/amarok/data
