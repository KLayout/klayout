
from importlib import metadata

try:
    __version__ = metadata.version('klayout')
except Exception:
    __version__ = 'unknown'
