#!/bin/bash -e

# This script regenerates the Python stubs from the sources

# clean up
rm -rf build dist
rm -rf python3-venv-make_stubs

python3 setup.py build
python3 setup.py bdist_wheel
python3 -m venv create python3-venv-make_stubs

. python3-venv-make_stubs/bin/activate

pip3 install ./dist/*.whl

./scripts/make_stubs.sh

