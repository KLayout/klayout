#!/bin/bash
# run with docker run --rm -v `pwd`:/io $DOCKER_IMAGE $PRE_CMD /io/ci-scripts/manylinux-docker.sh
# see https://github.com/pypa/python-manylinux-demo/blob/master/.travis.yml
# cache using https://github.com/travis-ci/travis-ci/issues/5358
set -e -x

# Install a system package required by our library
yum install -y zlib-devel
yum install -y ccache
ln -s /usr/bin/ccache /usr/lib64/ccachec++
ln -s /usr/bin/ccache /usr/lib64/ccachecc
ln -s /usr/bin/ccache /usr/lib64/ccachegcc
ln -s /usr/bin/ccache /usr/lib64/ccacheg++
export PATH=/usr/lib64/ccache:$PATH

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    cd /io; mkdir -p /io/wheelhouse
    "${PYBIN}/python" setup.py bdist_wheel -d wheelhouse/
done

# Bundle external shared libraries into the wheels
for whl in wheelhouse/*.whl; do
    auditwheel repair "$whl" -w /io/wheelhouse/
done

# Install packages and test
for PYBIN in /opt/python/*/bin/; do
    "${PYBIN}/pip" install klayout --no-index -f /io/wheelhouse
    "${PYBIN}" -m unittest /io/testdata/pymod/import_db.py  testdata/pymod/import_rdb.py testdata/pymod/import_tl.py
done
