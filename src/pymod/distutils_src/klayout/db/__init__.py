import functools
from typing import Type
import klayout.dbcore
from klayout.dbcore import *

from klayout.db.pcell_declaration_helper import PCellDeclarationHelper

__all__ = klayout.dbcore.__all__ + ["PCellDeclarationHelper"]  # type: ignore

# If class has from_s, to_s, and assign, use them to
# enable serialization.
for name, cls in klayout.dbcore.__dict__.items():
    if not isinstance(cls, type):
        continue
    if hasattr(cls, 'from_s') and hasattr(cls, 'to_s') and hasattr(cls, 'assign'):
        cls.__getstate__ = cls.to_s  # type: ignore
        def _setstate(self, str):
            cls = self.__class__
            self.assign(cls.from_s(str))
        cls.__setstate__ = _setstate  # type: ignore
