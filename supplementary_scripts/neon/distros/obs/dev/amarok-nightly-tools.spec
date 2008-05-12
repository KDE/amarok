#
# spec file for package amarok-nightly-tools
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

# norootforbuild


Name:           amarok-nightly-tools
#BuildRequires:  
License:        GPL
Group:          Productivity/Other
Summary:        Tools for Project Neon (Amarok Nightly)
Version:        1.2
Release:        2
Source0:        %name.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
Project Neon tools

%package -n amarok-nightly-deps
Group:          Development/Tools/Other
Summary:        Development dependencies for Project Neon (Amarok Nightly)

%description -n amarok-nightly-deps
Project Neon development dependencies

%prep
%setup -q -n dev

%build
pwd

%install
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/opt/amarok-nightly/neon/data/

cp -r scripts/* $RPM_BUILD_ROOT/usr/bin/
cp data/* $RPM_BUILD_ROOT/opt/amarok-nightly/neon/data/

%clean
rm -rf $RPM_BUILD_ROOT

%files -n amarok-nightly-tools
%defattr(-,root,root)
/usr/bin/*
/opt/amarok-nightly/neon/data/*

%files -n amarok-nightly-deps
%defattr(-,root,root)

%changelog
* Sun May 11 2008 nightly@getamarok.com
- initial build
