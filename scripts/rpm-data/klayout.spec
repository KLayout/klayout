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

%ifarch %ix86
TARGET="linux-32-gcc-release"
%else
TARGET="linux-64-gcc-release"
%endif
VERSION="%VERSION%"

Name:           klayout
Version:        $VERSION
Release:        0
Summary:        KLayout, viewer and editor for mask layouts
License:        GPL-2.0+
Group:          Productivity/Scientific/Electronics
Url:            http://www.klayout.de
Source0:        http://www.klayout.de/downloads/%{name}-%{version}.tar.gz
Source1:        %{name}.desktop

BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
Mask layout viewer and editor for the chip design engineer.

Highlights are:
* Efficient viewing in viewer mode
* Advanced editing features in editor mode (such as 
  parametrized cells)
* Scripting capability (Ruby and Python)
* Net tracing
* DRC functionality
* Clip feature
* XOR feature

%prep
%setup -q

%build

./build.sh -rpath ${_libdir}/klayout \
           -bin `pwd`/bin.$TARGET \
           -build `pwd`/build.$TARGET \
           -j4 \

%install
install -Dm644 `pwd`/bin.$TARGET/lib*.so* %{buildroot}%{_libdir}/klayout
install -Dm644 `pwd`/bin.$TARGET/klayout `pwd`/bin.$TARGET/strm* %{buildroot}%{_bindir}
install -Dm644 %{SOURCE1} %{buildroot}%{_datadir}/applications/%{name}.desktop
install -Dm644 src/images/logo.png %{buildroot}%{_datadir}/pixmaps/%{name}.png
%if 0%{?suse_version}%{?sles_version}
%suse_update_desktop_file -n %{name}
%endif

%files
%defattr(-,root,root)
%doc LICENSE Changelog
%{_bindir}/klayout
%{_bindir}/strm*
%{_libdir}/klayout/*
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png

%changelog
See Changelog file
