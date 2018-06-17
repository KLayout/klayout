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
  depends="libqt4-designer (>= 4.8.6), libqt4-xml (>= 4.8.6), libqt4-sql (>= 4.8.6), libqt4-network (>= 4.8.6), libqtcore4 (>= 4.8.6), libqtgui4 (>= 4.8.6), zlib1g (>= 1.2.8), libruby2.3 (>= 2.3.1), libpython3.5 (>= 3.5.1), libstdc++6 (>= 4.6.3), libc6 (>= 2.15)"
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

bindir="bin.linux.release" 
builddir="build.linux.release" 
libdir="/usr/lib/klayout"

./build.sh -j1 \
           -bin $bindir \
           -build $builddir \
           -rpath $libdir 

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

mkdir -p makedeb-tmp/usr/share/doc/klayout
mkdir -p makedeb-tmp/usr/share/applications
mkdir -p makedeb-tmp/usr/share/pixmaps
mkdir -p makedeb-tmp/usr/lib/klayout
mkdir -p makedeb-tmp/usr/bin

cp etc/klayout.desktop makedeb-tmp/usr/share/applications
cp etc/logo.png makedeb-tmp/usr/share/pixmaps
cp Changelog makedeb-tmp/usr/share/doc/klayout/changelog
cp Changelog.Debian makedeb-tmp/usr/share/doc/klayout/changelog.Debian
cp COPYRIGHT makedeb-tmp/usr/share/doc/klayout/copyright

cp -pd $bindir/strm* makedeb-tmp/usr/bin
cp -pd $bindir/klayout makedeb-tmp/usr/bin
cp -pd $bindir/lib*so* makedeb-tmp/usr/lib/klayout

cd makedeb-tmp

echo "Checking files .."

grep -q $version usr/share/doc/klayout/copyright || (
  echo "*** ERROR: version $version not found in copyright file"
  exit 1
)

grep -q $version usr/share/doc/klayout/changelog || (
  echo "*** ERROR: version $version not found in changelog file"
  exit 1
)

grep -q $version usr/share/doc/klayout/changelog.Debian || (
  echo "*** ERROR: version $version not found in changelog.Debian file"
  exit 1
)

echo "Modifying control file .."

strip usr/bin/*

size=`du -ck usr | grep total | sed "s/ *total//"`

mv control control.org
cat control.org | sed "s/%ARCH%/$arch/g" | sed "s/%VERSION%/$version/g" | sed "s/%SIZE%/$size/g" | sed "s/%DEPENDS%/$depends/g" >control
rm -f control.org
cat control

echo "Building .deb package .."

gzip -n --best usr/share/doc/klayout/changelog
gzip -n --best usr/share/doc/klayout/changelog.Debian

# lintian complains about exec bits set
find ./usr -name "lib*.so.*" -exec chmod 644 "{}" ";"

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

