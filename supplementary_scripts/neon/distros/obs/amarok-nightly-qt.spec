#
# spec file for package amarok-nightly-qt
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

# norootforbuild


Name:           amarok-nightly-qt
BuildRequires:  Mesa-devel cups-devel freetype2-devel gcc-c++ libjpeg-devel libmng-devel
BuildRequires:  libpng-devel libtiff-devel pkgconfig sqlite-devel
BuildRequires:  update-desktop-files
BuildRequires:  dbus-1-devel openssl-devel xorg-x11-devel
%if %suse_version > 1020
BuildRequires:  clucene-core-devel fdupes
%endif  
License:        GPL v2 only; GPL v3 only
Group:          System/Libraries
Summary:        Qt Library for Project Neon (Amarok Nightly)
Version:        0x1344224
Release:        1
Source0:        %name.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
Qt is a set of libraries for developing applications.

This package contains base tools, like string, xml, and network
handling.

%package -n amarok-nightly-deps
Group:          Development/Tools/Other
Summary:        Development dependencies for Project Neon (Amarok Nightly)

%description -n amarok-nightly-deps
Project Neon development dependencies

%debug_package

%prep
%setup -q -n qt-copy-804416

%build
  export QTDIR=$PWD
  export PATH=$PWD/bin:$PATH
  export LD_LIBRARY_PATH=$PWD/lib/
  sed -i -e "s,^\(CXXFLAGS[ \t]*=.*\),\1 $RPM_OPT_FLAGS," qmake/Makefile.unix
  export CXXFLAGS="$CXXFLAGS $RPM_OPT_FLAGS"
  export CFLAGS="$CFLAGS $RPM_OPT_FLAGS"
  export MAKEFLAGS="%{?jobs:-j %jobs}"
  ./configure -confirm-license -prefix "/opt/amarok-nightly" -fast -nomake "examples" -nomake "demos" -I/usr/include/freetype2 -lfontconfig -no-exceptions -debug -qdbus -pch
  make %{?jobs:-j %jobs}

%install
  make INSTALL_ROOT=$RPM_BUILD_ROOT install

%clean
  rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/opt/amarok-nightly/*

%changelog
* Mon May 12 2008 nightly@getamarok.com
- initial build (trial)
