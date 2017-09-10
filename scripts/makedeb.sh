#!/bin/sh -e

# See blog.peterscholtens.net/?p=210
#
# This scrips needs builds in:
#   bin.linux-32-gcc-release    (32 bit)
# and/or:
#   bin.linux-64-gcc-release    (64 bit)
#
# Create these builds with (-rpath and -bin are important):
#   ./build.sh -release -rpath /usr/lib/klayout -bin bin.linux-64-gcc-release -build build.linux-64-gcc-release
#   ./build.sh -release -rpath /usr/lib/klayout -bin bin.linux-32-gcc-release -build build.linux-32-gcc-release
#
# TODO:
# - The dependency list needs to be updated manually currently

if ! [ -e version.sh ]; then
  echo "*** ERROR: no version information found (no version.sh). Call this script from the root directory."
  exit 1
fi

. ./version.sh

version="$KLAYOUT_VERSION"
exe_name="klayout"

# TODO: derive this list automatically?
depends="libqt4-designer (>= 4.8.1), libqt4-xml (>= 4.8.1), libqt4-sql (>= 4.8.1), libqt4-network (>= 4.8.1), libqtcore4 (>= 4.8.1), libqtgui4 (>= 4.8.1), zlib1g (>= 1.2.3.4), libruby2.3 (>= 2.3.1), libpython2.7 (>= 2.7.12), libstdc++6 (>= 4.6.3), libc6 (>= 2.15)"

umask 0022

for bits in 32 64; do

  echo "Checking $bits installation"
  echo "----------------------------------------"

  if [ "$bits" = "32" ]; then
    arch="i386"
    bindir="bin.linux-32-gcc-release" 
  else
    arch="amd64"
    bindir="bin.linux-64-gcc-release"
  fi

  if [ -d $bindir ]; then
  
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
    cp -pd $bindir/strm* makedeb-tmp/usr/bin
    cp -pd $bindir/klayout makedeb-tmp/usr/bin
    cp -pd $bindir/lib* makedeb-tmp/usr/lib/klayout
    
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

  fi
      
done
