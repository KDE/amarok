# -*- mode: makefile; coding: utf-8 -*-
# Copyright © 2006 Peter Rockai <me@mornfall.net>
# Copyright © 2006 Fathi Boudra <fboudra@free.fr>
# Copyright © 2007 Peter Eisentraut <petere@debian.org>
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

_cdbs_scripts_path ?= /usr/lib/cdbs
_cdbs_rules_path ?= /usr/share/cdbs/1/rules
_cdbs_class_path ?= /usr/share/cdbs/1/class

ifndef _cdbs_class_cmake
_cdbs_class_cmake = 1

include $(_cdbs_class_path)/makefile.mk$(_cdbs_makefile_suffix)

ifdef _cdbs_tarball_dir
DEB_BUILDDIR = $(_cdbs_tarball_dir)/obj-$(DEB_BUILD_GNU_TYPE)
else
DEB_BUILDDIR = obj-$(DEB_BUILD_GNU_TYPE)
endif

# Overriden from makefile-vars.mk
# We pass CFLAGS and friends to cmake, so no need to pass them to make
DEB_MAKE_INVOKE = $(DEB_MAKE_ENVVARS) $(MAKE) -C $(DEB_BUILDDIR)

DEB_MAKE_INSTALL_TARGET = install DESTDIR=$(DEB_DESTDIR)

CMAKE = cmake
DEB_CMAKE_INSTALL_PREFIX = /usr
DEB_CMAKE_NORMAL_ARGS = -DCMAKE_INSTALL_PREFIX="$(DEB_CMAKE_INSTALL_PREFIX)" -DCMAKE_C_COMPILER="$(CC)" -DCMAKE_CXX_COMPILER="$(CXX)" -DCMAKE_C_FLAGS="$(CFLAGS)" -DCMAKE_CXX_FLAGS="$(CXXFLAGS)" -DCMAKE_SKIP_RPATH=ON -DCMAKE_VERBOSE_MAKEFILE=ON

common-configure-arch common-configure-indep:: common-configure-impl
common-configure-impl:: $(DEB_BUILDDIR)/CMakeCache.txt
$(DEB_BUILDDIR)/CMakeCache.txt:
	cd $(DEB_BUILDDIR) && $(CMAKE) $(CURDIR)/$(DEB_SRCDIR) $(DEB_CMAKE_NORMAL_ARGS) $(DEB_CMAKE_EXTRA_FLAGS)

cleanbuilddir::
	-$(if $(call cdbs_streq,$(DEB_BUILDDIR),$(DEB_SRCDIR)),,rm -rf $(DEB_BUILDDIR))

endif
