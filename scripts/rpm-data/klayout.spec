#
# spec file for package klayout
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

# RockyLinux9 requirements
%if "%{target_system}" == "rockylinux9"
Requires: ruby >= 3.0.0
Requires: python3 >= 3.9.0
Requires: qt5-qtbase >= 5.15.9
Requires: qt5-qtmultimedia >= 5.15.9
Requires: qt5-qtxmlpatterns >= 5.15.9
Requires: qt5-qtsvg >= 5.15.9
Requires: qt5-qttools >= 5.15.9
# NOTE: this package is required for libQt5Designer and pulls in a lot of devel stuff.
# Maybe it's worth considering to drop designer support and replace by QUiLoader.
Requires: qt5-qttools-devel >= 5.15.9
# Needed by something else (still?)
Requires: http-parser >= 2.9.4

%define buildopt -j2
# libgit2 is not available as standard package, but through EPEL
# So we include it explicitly
%define copylibs /usr/lib64/libgit2.so*
%define __python /usr/bin/python3
%endif

# CentOS8 requirements
%if "%{target_system}" == "centos8"
Requires: ruby >= 2.5.5
Requires: python3 >= 3.6.0
Requires: libgit2 >= 0.26.8
Requires: qt5-qtbase >= 5.11.1
Requires: qt5-qtmultimedia >= 5.11.1
Requires: qt5-qtxmlpatterns >= 5.11.1
Requires: qt5-qtsvg >= 5.11.1
Requires: qt5-qttools >= 5.11.1
# NOTE: this package is required for libQt5Designer and pulls in a lot of devel stuff.
# Maybe it's worth considering to drop designer support and replace by QUiLoader.
Requires: qt5-qttools-devel >= 5.11.1

%define buildopt -j2
%define __python /usr/bin/python3
%endif

# CentOS7 requirements
%if "%{target_system}" == "centos7"
Requires: ruby >= 2.0.0
Requires: python3 >= 3.6.0
Requires: qt-x11 >= 4.8.5
Requires: libgit2 >= 0.26.8
%define buildopt -j2
%endif

%if "%{target_system}" == "centos6"
# CentOS6 requirements
Requires: libcurl >= 7.19.7
Requires: ruby >= 1.8.7
Requires: python >= 2.6.6
Requires: qt-x11 >= 4.6.2
%define buildopt -libcurl -j2 -nolibgit2
%endif

%if "%{target_system}" == "opensuse42_2"
# OpenSuSE 42.2 requirements
Requires: ruby2.3 >= 2.3.1
Requires: python3 >= 3.4.6
Requires: libqt4-x11 >= 4.8.6
%define buildopt -j2 -nolibgit2
%endif

%if "%{target_system}" == "opensuse42_3"
# OpenSuSE 42.3 requirements
Requires: ruby2.3 >= 2.3.1
Requires: python3 >= 3.4.6
Requires: libqt4-x11 >= 4.8.6
%define buildopt -j2 -nolibgit2
%endif

%if "%{target_system}" == "opensuse15"
# OpenSuSE Leap 15 requirements
Requires: ruby >= 2.5
Requires: python3 >= 3.6
Requires: libgit2-1_3 >= 1.3.0
Requires: libqt5-qtbase >= 5.15.2
Requires: libQt5PrintSupport5 >= 5.15.2
Requires: libQt5Designer5 >= 5.15.2
Requires: libQt5Multimedia5 >= 5.15.2
Requires: libQt5Svg5 >= 5.15.2
Requires: libQt5XmlPatterns5 >= 5.15.2
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

# create and populate libdir
mkdir -p %{buildroot}%{_libdir}/klayout
mkdir -p %{buildroot}%{_libdir}/klayout/db_plugins
mkdir -p %{buildroot}%{_libdir}/klayout/lay_plugins
mkdir -p %{buildroot}%{_libdir}/klayout/pymod
cp -pd %{_builddir}/bin.$TARGET/lib*.so* %{buildroot}%{_libdir}/klayout
cp -pd %{_builddir}/bin.$TARGET/db_plugins/lib*.so* %{buildroot}%{_libdir}/klayout/db_plugins
cp -pd %{_builddir}/bin.$TARGET/lay_plugins/lib*.so* %{buildroot}%{_libdir}/klayout/lay_plugins
cp -rpd %{_builddir}/bin.$TARGET/pymod/* %{buildroot}%{_libdir}/klayout/pymod
%if %{defined copylibs}
  cp -pd %{copylibs} %{buildroot}%{_libdir}/klayout
%endif
chmod 644 %{buildroot}%{_libdir}/klayout/*.so*
chmod 644 %{buildroot}%{_libdir}/klayout/db_plugins/*.so*
chmod 644 %{buildroot}%{_libdir}/klayout/lay_plugins/*.so*
find %{buildroot}%{_libdir}/klayout/pymod -type f -exec chmod 644 {} +
find %{buildroot}%{_libdir}/klayout/pymod -type d -exec chmod 755 {} +

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
%{_libdir}/klayout/*
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png

%changelog
