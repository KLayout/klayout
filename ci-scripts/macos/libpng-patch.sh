# Solves issue #1346
# Deprecate patch when Github Actions provides Apple Silicon runners.
# WARNING: Only run this in a CI runner

# Issue: klayout depends on libpng, which is loaded by homebrew.
# But M1 wheels are cross-compiled in an intel host, so by default
# libpng.dylib has an x86 architecture. This prevents linking the library
# in the tl library.

# Patch solution: manually download libpng arm64 pre-built library from homebrew

# Reading
# https://stackoverflow.com/questions/55110564/how-can-i-install-a-homebrew-bottle-designed-for-a-different-previous-version
# https://github.com/Homebrew/homebrew-core/blob/6abea16e917b12d6af4912736f66eb8d42bf089b/Formula/libpng.rb

export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1

# Check if we're about to build an arm64 wheel. WARNING universal wheels not supported
if [ "$_PYTHON_HOST_PLATFORM" == "macosx-11.0-arm64" ]
then
    # Exit early if done
    test -f /tmp/libpng-apple-silicon-installed && exit 0
    curl -L -H "Authorization: Bearer QQ==" -o /tmp/libpng-1.6.39.arm64_big_sur.bottle.1.tar.gz https://ghcr.io/v2/homebrew/core/libpng/blobs/sha256:cf59cedc91afc6f2f3377567ba82b99b97744c60925a5d1df6ecf923fdb2f234
    brew reinstall -f /tmp/libpng-1.6.39.arm64_big_sur.bottle.1.tar.gz || exit 1
    touch /tmp/libpng-apple-silicon-installed
elif [ -f "/tmp/libpng-apple-silicon-installed" ]; then
    brew reinstall -f libpng && rm -f /tmp/libpng-apple-silicon-installed
fi