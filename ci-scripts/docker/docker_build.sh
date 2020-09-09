#!/usr/bin/env bash


if [[ -z $PY_VERSION ]]; then
    echo '$PY_VERSION is not set'
    exit 1
fi

if [[ -z $DOCKER_IMAGE ]]; then
    echo '$DOCKER_IMAGE is not set'
    exit 1
fi

echo PY_VERSION=$PY_VERSION
echo DOCKER_IMAGE=$DOCKER_IMAGE

# sometimes the epel server is down. retry 5 times
for i in $(seq 1 5); do 
    yum install -y zlib-devel ccache zip git && s=0 && break || s=$? && sleep 15; 
done

[ $s -eq 0 ] || exit $s

if [[ $DOCKER_IMAGE == "quay.io/pypa/manylinux1_x86_64" ]]; then
    ln -s /usr/bin/ccache /usr/lib64/ccache/c++
    ln -s /usr/bin/ccache /usr/lib64/ccache/cc
    ln -s /usr/bin/ccache /usr/lib64/ccache/gcc
    ln -s /usr/bin/ccache /usr/lib64/ccache/g++
    export PATH="/usr/lib64/ccache/:$PATH"
elif [[ $DOCKER_IMAGE == "quay.io/pypa/manylinux1_i686" ]]; then
    ln -s /usr/bin/ccache /usr/lib/ccache/c++
    ln -s /usr/bin/ccache /usr/lib/ccache/cc
    ln -s /usr/bin/ccache /usr/lib/ccache/gcc
    ln -s /usr/bin/ccache /usr/lib/ccache/g++
    export PATH="/usr/lib/ccache/:$PATH"
fi
echo $PATH
export CCACHE_DIR="/io/ccache"
ccache -M 5 G  # set cache size to 5 G

# Download proper auditwheel program
git clone https://github.com/thomaslima/auditwheel.git /tmp/auditwheel
cd /tmp/auditwheel
git checkout 87f5306ec02cc68020afaa9933543c898b1d47c1  # patched version
AUDITWHEEL_PYTHON=$(cat `which auditwheel` | head -1 | sed -e 's/#!\(.*\)/\1/')
# Install auditwheel, replacing the system's auditwheel binary
$AUDITWHEEL_PYTHON -m pip install .

# Show ccache stats
echo "Cache stats:"
ccache -s

# Compile wheel and build source distribution
cd /io
"/opt/python/$PY_VERSION/bin/python" setup.py bdist_wheel -d /io/wheelhouse/ || exit 1
"/opt/python/$PY_VERSION/bin/python" setup.py sdist --formats=zip -d /io/wheelhouse || exit 1

# Show ccache stats
echo "Cache stats:"
ccache -s

# Bundle external shared libraries into the wheels via auditwheel
for whl in /io/wheelhouse/*linux_*.whl; do
    auditwheel repair "$whl" -w /io/wheelhouse/ || exit 1
done

# Install packages and test
TEST_HOME=/io/testdata
"/opt/python/$PY_VERSION/bin/pip" install klayout --no-index -f /io/wheelhouse || exit 1
"/opt/python/$PY_VERSION/bin/python" $TEST_HOME/pymod/import_db.py || exit 1
"/opt/python/$PY_VERSION/bin/python" $TEST_HOME/pymod/import_rdb.py || exit 1
"/opt/python/$PY_VERSION/bin/python" $TEST_HOME/pymod/import_tl.py || exit 1
"/opt/python/$PY_VERSION/bin/python" $TEST_HOME/pymod/pya_tests.py || exit 1

