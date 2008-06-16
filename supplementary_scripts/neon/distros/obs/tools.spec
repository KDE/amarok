#
# spec file for package amarok-nightly-tools
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

# norootforbuild


Name:           amarok-nightly-tools
License:        GPL
Group:          Productivity/Other
Summary:        Tools for Project Neon (Amarok Nightly)
Version:        1.4
Release:        50
Source0:        %name.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
Project Neon tools

%package -n amarok-nightly-deps-base
Group:          Development/Tools/Other
Summary:        Development dependencies for Project Neon (Amarok Nightly)
Requires:       gcc-c++ cmake dbus-1-devel procps
Requires:       giflib-devel libpng-devel openssl-devel pcre-devel xorg-x11-devel
Requires:       libxslt-devel libxml2-devel libjpeg-devel alsa-devel
Requires:       libidn-devel shared-mime-info libbz2-devel xine-devel
Requires:       ruby-devel SDL-devel curl-devel libvisual-devel xorg-x11-Mesa-devel

%description -n amarok-nightly-deps-base
Project Neon development dependencies

%prep
%setup -q -n %name

%build
  pwd

%install
  mkdir -p $RPM_BUILD_ROOT/usr/bin
  mkdir -p $RPM_BUILD_ROOT/usr/share/applications/
  mkdir -p $RPM_BUILD_ROOT/opt/amarok-nightly/neon/data/

  cp -r scripts/* $RPM_BUILD_ROOT/usr/bin/
  cp -r desktopfiles/* $RPM_BUILD_ROOT/usr/share/applications/
  cp -r data/* $RPM_BUILD_ROOT/opt/amarok-nightly/neon/data/

%clean
  rm -rf $RPM_BUILD_ROOT

%files -n amarok-nightly-tools
%defattr(-,root,root)
/usr/bin/*
/usr/share/applications/*
/opt/amarok-nightly/neon/data/*

%files -n amarok-nightly-deps-base
%defattr(-,root,root)

%changelog
* Sun May 11 2008 nightly@getamarok.com
- initial build
