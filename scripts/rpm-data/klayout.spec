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

# CentOS8 requirements
%if "%{target_system}" == "centos8"
Requires:	ruby >= 2.5.5
Requires:	python3 >= 3.6.0
Requires: qt5-qtbase >= 5.11.1
Requires: qt5-qtmultimedia >= 5.11.1
Requires: qt5-qtxmlpatterns >= 5.11.1
Requires: qt5-qtsvg >= 5.11.1
Requires: qt5-qttools >= 5.11.1
# NOTE: this package is required for libQt5Designer and pulls in a lot of devel stuff.
# Maybe it's worth considering to drop designer support and replace by QUiLoader.
Requires: qt5-qttools-devel >= 5.11.1

%define buildopt -j2
%define pylib %{python_sitearch}
%define __python /usr/bin/python3
%endif

# CentOS7 requirements
%if "%{target_system}" == "centos7"
Requires:	ruby >= 2.0.0
Requires:	python >= 2.7.5
Requires: qt-x11 >= 4.8.5
%define buildopt -j2
%define pylib %{python_sitearch}
%endif

%if "%{target_system}" == "centos6"
# CentOS6 requirements
Requires: libcurl >= 7.19.7
Requires: ruby >= 1.8.7
Requires: python >= 2.6.6
Requires: qt-x11 >= 4.6.2
%define buildopt -libcurl -j2
%define pylib %{python_sitearch}
%endif

%if "%{target_system}" == "opensuse42_2"
# OpenSuSE 42.2 requirements
Requires:	ruby2.3 >= 2.3.1
Requires:	python3 >= 3.4.6
Requires: libqt4-x11 >= 4.8.6
%define buildopt -j2
%define pylib %{python3_sitearch}
%endif

%if "%{target_system}" == "opensuse42_3"
# OpenSuSE 42.3 requirements
Requires:	ruby2.3 >= 2.3.1
Requires:	python3 >= 3.4.6
Requires: libqt4-x11 >= 4.8.6
%define buildopt -j2
%define pylib %{python3_sitearch}
%endif

%if "%{target_system}" == "opensuse15"
# OpenSuSE Leap 15 requirements
Requires:	ruby >= 2.5
Requires:	python3 >= 3.6
Requires: libqt4-x11 >= 4.8.7
%define buildopt -j2
%define pylib %{python3_sitearch}
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

%if "%{git_source}" != ""
# build from git sources if possible
cd %{git_source}
%else
cd %{_sourcedir}
%endif

# clean bin dir
rm -rf %{_builddir}/bin.$TARGET

# do the actual build
./build.sh -rpath %{_libdir}/klayout \
           -bin %{_builddir}/bin.$TARGET \
           -build %{_builddir}/build.$TARGET \
           %{buildopt} 

cp -p LICENSE Changelog CONTRIB %{_builddir}
strip %{_builddir}/bin.$TARGET/*.so
strip %{_builddir}/bin.$TARGET/*/*.so
strip %{_builddir}/bin.$TARGET/*/*/*.so
strip %{_builddir}/bin.$TARGET/klayout
strip %{_builddir}/bin.$TARGET/strm*

%install

TARGET="linux-release"

# create and populate pylib
mkdir -p %{buildroot}%{pylib}/klayout
cp -pd %{_builddir}/bin.$TARGET/pymod/klayout/*.so %{buildroot}%{pylib}/klayout
cp -pd %{_builddir}/bin.$TARGET/pymod/klayout/*.py %{buildroot}%{pylib}/klayout
chmod 644 %{buildroot}%{pylib}/klayout/*
for d in tl db rdb; do
  mkdir -p %{buildroot}%{pylib}/klayout/$d
  cp -pd %{_builddir}/bin.$TARGET/pymod/klayout/$d/*.py %{buildroot}%{pylib}/klayout/$d
  chmod 644 %{buildroot}%{pylib}/klayout/$d/*
done

# create and populate libdir
mkdir -p %{buildroot}%{_libdir}/klayout
mkdir -p %{buildroot}%{_libdir}/klayout/db_plugins
mkdir -p %{buildroot}%{_libdir}/klayout/lay_plugins
cp -pd %{_builddir}/bin.$TARGET/lib*.so* %{buildroot}%{_libdir}/klayout
cp -pd %{_builddir}/bin.$TARGET/db_plugins/lib*.so* %{buildroot}%{_libdir}/klayout/db_plugins
cp -pd %{_builddir}/bin.$TARGET/lay_plugins/lib*.so* %{buildroot}%{_libdir}/klayout/lay_plugins
chmod 644 %{buildroot}%{_libdir}/klayout/*.so*
chmod 644 %{buildroot}%{_libdir}/klayout/db_plugins/*.so*
chmod 644 %{buildroot}%{_libdir}/klayout/lay_plugins/*.so*

# create and populate bindir
mkdir -p %{buildroot}%{_bindir}
cp -pd %{_builddir}/bin.$TARGET/klayout %{_builddir}/bin.$TARGET/strm* %{buildroot}%{_bindir}
chmod 755 %{buildroot}%{_bindir}/*

# other files
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
%{pylib}/klayout/*
%{_libdir}/klayout/*
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png

%changelog
