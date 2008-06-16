# -*- mode: makefile; coding: utf-8 -*-
# Copyright (C) 2006 Peter Rockai <me@mornfall.net>
# Copyright (C) 2006 Fathi Boudra <fboudra@free.fr>
# Description: A class for cmake packages
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

ifndef _cdbs_bootstrap
_cdbs_scripts_path ?= /usr/lib/cdbs
_cdbs_rules_path ?= /usr/share/cdbs/1/rules
_cdbs_class_path ?= /usr/share/cdbs/1/class
endif

ifndef _cdbs_class_cmake
_cdbs_class_cmake := 1

include $(_cdbs_rules_path)/buildcore.mk$(_cdbs_makefile_suffix)

ifdef _cdbs_tarball_dir
DEB_BUILDDIR = $(_cdbs_tarball_dir)/obj-$(DEB_BUILD_GNU_TYPE)
else
DEB_BUILDDIR = obj-$(DEB_BUILD_GNU_TYPE)
endif

DEB_MAKE_INSTALL_TARGET = install DESTDIR=$(DEB_DESTDIR)
DEB_CMAKE_PREFIX =/usr

# Overriden from makefile-vars.mk
# We pass CFLAGS and friends to ./configure, so no need to pass them to make
DEB_MAKE_INVOKE = $(DEB_MAKE_ENVVARS) $(MAKE) -C $(DEB_BUILDDIR)

include $(_cdbs_class_path)/makefile.mk$(_cdbs_makefile_suffix)

common-configure-arch common-configure-indep:: common-configure-impl
common-configure-impl:: $(DEB_BUILDDIR)/CMakeCache.txt
$(DEB_BUILDDIR)/CMakeCache.txt:
	cd $(DEB_BUILDDIR) && cmake $(CURDIR)/$(DEB_SRCDIR) \
	-DCMAKE_INSTALL_PREFIX="$(DEB_CMAKE_PREFIX)" \
	$(DEB_CMAKE_EXTRA_FLAGS) -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
	-DCMAKE_C_FLAGS="$(CFLAGS)" -DCMAKE_VERBOSE_MAKEFILE=ON
	mkdir -p $(DEB_DESTDIR)

cleanbuilddir::
	-if test "$(DEB_BUILDDIR)" != "$(DEB_SRCDIR)"; then rm -rf $(DEB_BUILDDIR); fi

endif

