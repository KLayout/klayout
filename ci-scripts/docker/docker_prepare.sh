#!/usr/bin/env bash
set -xe

# manylinux prep
if [[ -f "/etc/centos-release" ]]; then
    # sometimes the epel server is down. retry 5 times
    for i in $(seq 1 5); do 
        yum install -y zlib-devel curl-devel expat-devel libpng-devel ccache && s=0 && break || s=$? && sleep 15; 
    done

    [ $s -eq 0 ] || exit $s

    if [[ -d "/usr/lib64/ccache" ]]; then
        for comp in c++ cc gcc g++; do
            if ! [ -e /usr/lib64/ccache/$comp ]; then
                ln -s /usr/bin/ccache /usr/lib64/ccache/$comp
            fi
        done
        export PATH="/usr/lib64/ccache:$PATH"
    elif [[ -d "/usr/lib/ccache" ]]; then
        for comp in c++ cc gcc g++; do
            if ! [ -e /usr/lib/ccache/$comp ]; then
                ln -s /usr/bin/ccache /usr/lib/ccache/$comp
            fi
        done
        export PATH="/usr/lib/ccache:$PATH"
    fi

elif [[ -f "/etc/alpine-release" ]]; then
    # musllinux prep
    # ccache already present
    apk add curl-dev expat-dev zlib-dev libpng-dev ccache
    export PATH="/usr/lib/ccache/bin:$PATH"
fi

# hack until https://github.com/pypa/cibuildwheel/issues/1030 is fixed
# Place ccache folder in /outputs
HOST_CCACHE_DIR="/host${HOST_CCACHE_DIR:-/home/runner/work/klayout/klayout/.ccache}"
if [ -d $HOST_CCACHE_DIR ]; then
    mkdir -p /output
    cp -R $HOST_CCACHE_DIR /output/.ccache
fi

ls -la /output/

ccache -o cache_dir="/output/.ccache"
# export CCACHE_DIR="/host/home/runner/work/klayout/klayout/.ccache"
ccache -M 5 G  # set cache size to 5 G

# Show ccache stats
echo "Cache stats:"
ccache -s
