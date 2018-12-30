Author: Thomas Ferreira de Lima
email: thomas@tlima.me

This folder contains scripts to be run inside docker images. See instructions on how to test this yourself in ci-scripts/docker/development_notes.

## docker_build.sh

We need two environment variables to get going:

```bash
DOCKER_IMAGE="quay.io/pypa/manylinux1_x86_64"
PY_VERSION="cp37-cp37m"
```

The script must be run inside an image pulled from $DOCKER_IMAGE and with klayout's git repo cloned in /io. Inside the git clone folder, run:

```bash
docker run --rm -e DOCKER_IMAGE -e PY_VERSION -v `pwd`:/io $DOCKER_IMAGE $PRE_CMD "/io/ci-scripts/docker/docker_build.sh";
# $PRE_CMD is empty for now (useless currently).
```

This command will generate a wheel and place it in `wheelhouse/klayout-*manylinux1*.whl`. This is the wheel that needs to be uploaded to PyPI via twine. See ci-scripts/twine/README.md.
