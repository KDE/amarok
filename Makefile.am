## kdenonbeta/Makefile.am
## (C) 1997 Stephan Kulow

AUTOMAKE_OPTIONS = foreign 
# COMPILE_AFTER_kdedb = kcontrol kdbbrowser
# COMPILE_AFTER_libsearch = caitoo columbo
DISTCLEANFILES = inst-apps


install-data-local:
	@echo ""
	@echo ""
	@echo "*************** Important *************************"
	@echo ""
	@echo "This module contains unreleased software."
	@echo ""
	@echo "The software may compile and work, but it may just"
	@echo "as well neither compile nor work."
	@echo ""
	@echo "****************************************************"
	@echo ""

$(top_srcdir)/acinclude.m4: $(top_srcdir)/admin/acinclude.m4.in $(top_srcdir)/admin/libtool.m4.in
	@cd $(top_srcdir) && cat admin/acinclude.m4.in admin/libtool.m4.in > acinclude.m4 

$(top_srcdir)/configure.in: $(top_srcdir)/configure.files $(shell test -f $(top_srcdir)/configure.files && cat $(top_srcdir)/configure.files)
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common configure.in
$(top_srcdir)/subdirs:
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common subdirs
$(top_srcdir)/configure.files:
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common configure.files

MAINTAINERCLEANFILES = subdirs configure.in acinclude.m4 SUBDIRS configure.files

SUBDIRS=$(TOPSUBDIRS)
