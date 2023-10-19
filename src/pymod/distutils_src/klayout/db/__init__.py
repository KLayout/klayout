
import sys
from ..dbcore import __all__
from ..dbcore import *

from .pcell_declaration_helper import *

# establish the PCellDeclarationHelper using the mixin provided by _pcell_declaration_helper
class PCellDeclarationHelper(_PCellDeclarationHelperMixin, PCellDeclaration):
  def __init__(self):
    super().__init__()
  def _make_parameter_declaration(self, name, value_type, description):
    return PCellParameterDeclaration(name, value_type, description)
  def _make_default_trans(self):
    return Trans()

# import the Type... constants from PCellParameterDeclaration
for k in dir(PCellParameterDeclaration):
  if k.startswith("Type"):
    setattr(PCellDeclarationHelper, k, getattr(PCellParameterDeclaration, k))

# If class has from_s, to_s, and assign, use them to
# enable serialization.
for name in __all__:
  cls = globals()[name]
  if hasattr(cls, 'from_s') and hasattr(cls, 'to_s') and hasattr(cls, 'assign'):
    cls.__getstate__ = cls.to_s  # type: ignore
    def _setstate(self, str):
      cls = self.__class__
      self.assign(cls.from_s(str))
    cls.__setstate__ = _setstate  # type: ignore
