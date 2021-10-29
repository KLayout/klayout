#!/usr/bin/env bash


if [[ -z $PY_VERSION ]]; then
    echo '$PY_VERSION is not set'
    exit 1
fi

echo PY_VERSION=$PY_VERSION

# Use single cores only so we do not overload the Jenkins host
export KLAYOUT_SETUP_MULTICORE=1

# Compile wheel and build source distribution
cd /io
"/opt/python/$PY_VERSION/bin/python" setup.py bdist_wheel -d /io/wheelhouse/ || exit 1
"/opt/python/$PY_VERSION/bin/python" setup.py sdist --formats=zip -d /io/wheelhouse || exit 1

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

