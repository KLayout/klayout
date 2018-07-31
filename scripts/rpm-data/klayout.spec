#
# spec file for package klayout
#
# Copyright (c) 2017 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

Name:           klayout
Version:        %{git_version}
Release:        0
Summary:        KLayout, viewer and editor for mask layouts
License:        GPL-2.0+
Group:          Productivity/Scientific/Electronics
Url:            http://www.klayout.de
%if "%{git_source}" == ""
Source0:        http://www.klayout.de/downloads/%{name}-%{version}.tar.gz
%endif
# BuildRoot:      %{_tmppath}/%{name}-%{version}-build

# Disable auto-detection of dependencies (to prevent including the
# so's of klayout itself)
AutoReqProv: 	no

# CentOS7 requirements
%if "%{target_system}" == "centos7"
Requires:	ruby >= 2.0.0
Requires:	python >= 2.7.5
Requires: qt-x11 >= 4.8.5
%define buildopt -j2
%endif

%if "%{target_system}" == "centos6"
# CentOS6 requirements
Requires: libcurl >= 7.19.7
Requires: ruby >= 1.8.7
Requires: python >= 2.6.6
Requires: qt-x11 >= 4.6.2
%define buildopt -libcurl -j2
%endif

%if "%{target_system}" == "opensuse42_2"
# OpenSuSE 42.2 requirements
Requires:	ruby2.3 >= 2.3.1
Requires:	python3 >= 3.4.6
Requires: libqt4-x11 >= 4.8.6
%define buildopt -j2
%endif

%if "%{target_system}" == "opensuse42_3"
# OpenSuSE 42.3 requirements
Requires:	ruby2.3 >= 2.3.1
Requires:	python3 >= 3.4.6
Requires: libqt4-x11 >= 4.8.6
%define buildopt -j2
%endif

%if "%{target_system}" == "opensuse15"
# OpenSuSE Leap 15 requirements
Requires:	ruby >= 2.5
Requires:	python3 >= 3.6
Requires: libqt4-x11 >= 4.8.7
%define buildopt -j2
%endif

%description
Mask layout viewer and editor for the chip design engineer.

For details see README.md

%prep

%if "%{git_source}" != ""
  rm -rf %{_sourcedir}
  ln -s %{git_source} %{_sourcedir}
%else
  %setup -q	
%endif

%build

TARGET="linux-release"

# TODO: remove -without-qtbinding
%if "%{git_source}" != ""
# build from git sources if possible
cd %{git_source}
%else
cd %{_sourcedir}
%endif
./build.sh -rpath %{_libdir}/klayout \
           -bin %{_builddir}/bin.$TARGET \
           -build %{_builddir}/build.$TARGET \
           %{buildopt} 

cp -p LICENSE Changelog CONTRIB %{_builddir}
strip %{_builddir}/bin.$TARGET/*

%install

TARGET="linux-release"

mkdir -p %{buildroot}%{_libdir}/klayout
mkdir -p %{buildroot}%{_bindir}
cp -pd %{_builddir}/bin.$TARGET/lib*.so* %{buildroot}%{_libdir}/klayout
chmod 644 %{buildroot}%{_libdir}/klayout/*
cp -pd %{_builddir}/bin.$TARGET/klayout %{_builddir}/bin.$TARGET/strm* %{buildroot}%{_bindir}
chmod 755 %{buildroot}%{_bindir}/*
install -Dm644 %{_sourcedir}/etc/%{name}.desktop %{buildroot}%{_datadir}/applications/%{name}.desktop
install -Dm644 %{_sourcedir}/etc/logo.png %{buildroot}%{_datadir}/pixmaps/%{name}.png

# TODO: remove this? This macro does not expand to anything in SuSE 42.x
#%if 0%{?suse_version}%{?sles_version}
#%suse_update_desktop_file -n %{name}
#%endif

%files
%defattr(-,root,root)
%doc LICENSE
%doc Changelog
%doc CONTRIB
%{_bindir}/klayout
%{_bindir}/strm*
%{_libdir}/klayout/*
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png

%changelog
