FROM quay.io/pypa/manylinux1_i686
MAINTAINER Thomas Ferreira de Lima (thomas@tlima.me)

# Install a system package required by our library
RUN linux32 yum install -y zlib-devel
RUN linux32 yum install -y ccache
RUN ln -s /usr/bin/ccache /usr/lib/ccache/c++
RUN ln -s /usr/bin/ccache /usr/lib/ccache/cc
RUN ln -s /usr/bin/ccache /usr/lib/ccache/gcc
RUN ln -s /usr/bin/ccache /usr/lib/ccache/g++

# Add ccache to PATH
RUN mkdir -p /persist/.ccache
ENV CCACHE_DIR="/persist/.ccache"

# Need zip to fix wheel
RUN linux32 yum install -y zip
