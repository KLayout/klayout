
# pull everything from the generic klayout.pya package into pya
from klayout.pya import __all__
from klayout.pya import *

from klayout.db.pcell_declaration_helper import *
from klayout.db.pcell_declaration_helper import __all__ as _all_added1
__all__ += _all_added1

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

__all__ += [ "PCellDeclarationHelper" ]

