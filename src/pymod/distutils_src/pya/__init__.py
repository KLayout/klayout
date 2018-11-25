
import klayout
import importlib

__all__ = []
for m in klayout.__all__:
  mod = importlib.import_module("klayout." + m)
  for mm in mod.__all__:
    globals()[mm] = getattr(mod, mm)

