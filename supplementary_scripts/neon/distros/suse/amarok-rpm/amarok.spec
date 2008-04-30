#
# spec file for package amarok-nightly
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

# norootforbuild


Name:           amarok-nightly
BuildRequires:  amarok-nightly-taglib amarok-nightly-kdebase-runtime
BuildRequires:  curl-devel ruby-devel xine-devel xine-lib
BuildRequires:  libgpod-devel libnjb-devel libmtp-devel libusb-devel
Url:            http://amarok.kde.org
License:        GPL v2 or later
Group:          Productivity/Multimedia/Sound/Players
Summary:        Nightly builds of the Amarok music player
Version:        20080430
Release:        1
Source0:        %name-%version.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
%kde4_runtime_requires 
%define prefix /opt/amarok-nightly

%description
Amarok is a music player for all kinds of media. This includes MP3, Ogg
Vorbis, audio CDs, podcasts and streams.

%debug_package

%prep
%setup -q -n %name-%version

%build
  %cmake_kde4 -d build
  %make_jobs

%install
  cd build
  %create_subdir_filelist -d amarok
  cd ..
  %create_exclude_filelist
  %suse_update_desktop_file amarok AudioVideo Player

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf filelists

#%post
#%run_ldconfig
#
#%postun
#%run_ldconfig

%files -f filelists/exclude
%defattr(-,root,root)

%files -n kde4-amarok -f filelists/amarok
%defattr(-,root,root)

%changelog
* Wed Apr 30 2008 nightly@getamarok.com
- initial build
