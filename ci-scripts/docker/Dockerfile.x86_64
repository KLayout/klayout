FROM quay.io/pypa/manylinux1_x86_64
MAINTAINER Thomas Ferreira de Lima (thomas@tlima.me)

# Install a system package required by our library
RUN yum install -y zlib-devel
RUN yum install -y ccache
RUN ln -s /usr/bin/ccache /usr/lib64/ccache/c++
RUN ln -s /usr/bin/ccache /usr/lib64/ccache/cc
RUN ln -s /usr/bin/ccache /usr/lib64/ccache/gcc
RUN ln -s /usr/bin/ccache /usr/lib64/ccache/g++

# Add ccache to PATH
RUN mkdir -p /persist/.ccache
ENV CCACHE_DIR="/persist/.ccache"

# Need zip to fix wheel
RUN yum install -y zip
