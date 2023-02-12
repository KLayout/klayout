"""
This module is a standalone distribution of KLayout's Python API.
For more details see here: https://www.klayout.org/klayout-pypi

Note: Submodules are not automatically imported. You must import submodules
in order to use them. For example `import klayout.db` or `from klayout.db import Layout`.
"""
import os, re

from . import _version
__version__ = _version.get_versions()['version']
