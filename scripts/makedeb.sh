#!/bin/sh -e

# Run this script with 
#
#   makedeb.sh <system>
#
# Currently system can be:
#   ubuntu16    - Ubuntu 16.10LTS
#   ...


target="$1"

if ! [ -e version.sh ]; then
  echo "*** ERROR: no version information found (no version.sh). Call this script from the root directory."
  exit 1
fi

# TODO: derive this list automatically?
case $target in
ubuntu16)
  depends="libqt4-designer (>= 4.8.6), libqt4-xml (>= 4.8.6), libqt4-sql (>= 4.8.6), libqt4-network (>= 4.8.6), libqtcore4 (>= 4.8.6), libqtgui4 (>= 4.8.6), zlib1g (>= 1.2.8), libruby2.3 (>= 2.3.1), python3 (>= 3.5.1), libpython3.5 (>= 3.5.1), libstdc++6 (>= 4.6.3), libc6 (>= 2.15)"
  ;;
ubuntu18)
  depends="libqt4-designer (>= 4.8.7), libqt4-xml (>= 4.8.7), libqt4-sql (>= 4.8.7), libqt4-network (>= 4.8.7), libqtcore4 (>= 4.8.7), libqtgui4 (>= 4.8.7), zlib1g (>= 1.2.11), libruby2.5 (>= 2.5.1), python3 (>= 3.6.5), libpython3.6 (>= 3.6.5), libstdc++6 (>= 8), libc6 (>= 2.27)"
  ;;
*)
  echo "Unknown target '$target' (given as first argument)"
  exit 1
  ;;
esac

. ./version.sh

version="$KLAYOUT_VERSION"
exe_name="klayout"
bits=64


umask 0022

echo "Checking $bits installation"
echo "----------------------------------------"

bininstdir="bin.linux.release" 
builddir="build.linux.release" 

# destination folders
sharedir="usr/share"
bindir="usr/bin"
libdir="usr/lib/klayout"

# TODO: is there a better way to produce this path?
pylibdir="usr/lib/python3/dist-packages/klayout"

# clean bin directory
rm -rf $bininstdir

# do the actual build
./build.sh -j2 \
           -bin $bininstdir \
           -build $builddir \
           -rpath /$libdir 

if [ "$bits" = "32" ]; then
  arch="i386"
else
  arch="amd64"
fi

rm -rf makedeb-tmp
mkdir makedeb-tmp
 
pkgname=${exe_name}_${version}-1_$arch

echo "INFO: package name is $pkgname"

echo "Copying files .."
cd scripts/deb-data
tar --exclude=".svn" -cf data.tar * 
mv data.tar ../..
cd ../..
cd makedeb-tmp
mv ../data.tar .
tar xf data.tar
rm data.tar
cd ..

mkdir -p makedeb-tmp/${sharedir}/doc/klayout
mkdir -p makedeb-tmp/${sharedir}/applications
mkdir -p makedeb-tmp/${sharedir}/pixmaps
mkdir -p makedeb-tmp/${libdir}/db_plugins
mkdir -p makedeb-tmp/${libdir}/lay_plugins
mkdir -p makedeb-tmp/${pylibdir}
mkdir -p makedeb-tmp/${bindir}

cp etc/klayout.desktop makedeb-tmp/${sharedir}/applications
cp etc/logo.png makedeb-tmp/${sharedir}/pixmaps/klayout.png
cp Changelog makedeb-tmp/${sharedir}/doc/klayout/changelog
cp Changelog.Debian makedeb-tmp/${sharedir}/doc/klayout/changelog.Debian
cp COPYRIGHT makedeb-tmp/${sharedir}/doc/klayout/copyright

cp -pd $bininstdir/strm* makedeb-tmp/${bindir}
cp -pd $bininstdir/klayout makedeb-tmp/${bindir}
cp -pd $bininstdir/lib*so* makedeb-tmp/${libdir}
cp -pd $bininstdir/db_plugins/lib*so* makedeb-tmp/${libdir}/db_plugins
cp -pd $bininstdir/lay_plugins/lib*so* makedeb-tmp/${libdir}/lay_plugins
cp -pd $bininstdir/pymod/klayout/*so makedeb-tmp/${pylibdir}
cp -pd $bininstdir/pymod/klayout/*py makedeb-tmp/${pylibdir}
for d in db tl rdb; do
  mkdir -p makedeb-tmp/${pylibdir}/$d
  cp -pd $bininstdir/pymod/klayout/$d/*py makedeb-tmp/${pylibdir}/$d
done

cd makedeb-tmp

echo "Checking files .."

grep -q $version ${sharedir}/doc/klayout/copyright || (
  echo "*** ERROR: version $version not found in copyright file"
  exit 1
)

grep -q $version ${sharedir}/doc/klayout/changelog || (
  echo "*** ERROR: version $version not found in changelog file"
  exit 1
)

grep -q $version ${sharedir}/doc/klayout/changelog.Debian || (
  echo "*** ERROR: version $version not found in changelog.Debian file"
  exit 1
)

echo "Modifying control file .."

strip ${bindir}/*
strip ${pylibdir}/*.so
strip ${libdir}/db_plugins/*.so*
strip ${libdir}/lay_plugins/*.so*

size=`du -ck usr | grep total | sed "s/ *total//"`

mv control control.org
cat control.org | sed "s/%ARCH%/$arch/g" | sed "s/%VERSION%/$version/g" | sed "s/%SIZE%/$size/g" | sed "s/%DEPENDS%/$depends/g" >control
rm -f control.org
cat control

echo "Building .deb package .."

gzip -n --best ${sharedir}/doc/klayout/changelog
gzip -n --best ${sharedir}/doc/klayout/changelog.Debian

# lintian complains about exec bits set
find ./usr -name "*.so.*" -exec chmod 644 "{}" ";"
find ./usr -name "*.so" -exec chmod 644 "{}" ";"

find ./usr -type f -exec md5sum "{}" ";" >md5sums
chmod 644 md5sums

fakeroot tar -cvf data.tar ./usr
gzip data.tar

fakeroot tar -cvf control.tar control md5sums postinst postrm
gzip control.tar

fakeroot ar cr $pkgname.deb debian-binary control.tar.gz data.tar.gz

echo "Finish making $pkgname.deb .."
mv $pkgname.deb ..
cd ..
rm -rf makedeb-tmp

echo "Running lintian $pkgname.deb .."
lintian $pkgname.deb --suppress-tags binary-without-manpage --no-tag-display-limit

