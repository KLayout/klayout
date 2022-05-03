#!/usr/bin/env bash
set -xe

# sometimes the epel server is down. retry 5 times
for i in $(seq 1 5); do 
    yum install -y zlib-devel curl-devel expat-devel ccache && s=0 && break || s=$? && sleep 15; 
done

[ $s -eq 0 ] || exit $s

if [[ -d "/usr/lib64/ccache" ]]; then
    ln -s /usr/bin/ccache /usr/lib64/ccache/c++
    ln -s /usr/bin/ccache /usr/lib64/ccache/cc
    ln -s /usr/bin/ccache /usr/lib64/ccache/gcc
    ln -s /usr/bin/ccache /usr/lib64/ccache/g++
    export PATH="/usr/lib64/ccache:$PATH"
elif [[ -d "/usr/lib/ccache" ]]; then
    ln -s /usr/bin/ccache /usr/lib/ccache/c++
    ln -s /usr/bin/ccache /usr/lib/ccache/cc
    ln -s /usr/bin/ccache /usr/lib/ccache/gcc
    ln -s /usr/bin/ccache /usr/lib/ccache/g++
    export PATH="/usr/lib/ccache:$PATH"
fi

echo $PATH
# export CCACHE_DIR="/host/home/runner/work/klayout/klayout/.ccache"
ccache -M 5 G  # set cache size to 5 G

# Show ccache stats
echo "Cache stats:"
ccache -s
