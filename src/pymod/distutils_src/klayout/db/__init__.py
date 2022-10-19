import functools
from typing import Type
import klayout.dbcore
from klayout.dbcore import *

from klayout.db.pcell_declaration_helper import PCellDeclarationHelper

__all__ = klayout.dbcore.__all__ + ["PCellDeclarationHelper"]  # type: ignore

# Implementing deepcopy of common objects
# Point-like classes
PointLike = (Point, DPoint, DVector, Vector)


def pyaPoint__deepcopy__(self, memo):
    return self.dup()


def convert_type_error_to_not_implemented(cls, method):
    """If cls.method exists raises a TypeError, patch it so
    it returns a NotImplemented error instead.


    """
    if not hasattr(cls, method):
        return

    old_func = getattr(cls, method)

    @functools.wraps(old_func)
    def new_func(*args, **kwargs):
        try:
            return old_func(*args, **kwargs)
        except TypeError:
            return NotImplemented
    try:
        setattr(cls, method, new_func)
    except TypeError:
        # Some classes are immutable and cannot be changed.
        # At the time of writing, this happens to (_StaticAttribute, _AmbiguousMethodDispatcher, _Iterator, _Signal).__or__
        return

for PClass in PointLike:
    PClass.__deepcopy__ = pyaPoint__deepcopy__  # type: ignore

for cls in klayout.dbcore.__dict__.values():
    if not isinstance(cls, type):  # skip if not a class
        continue
    for method in (
        "__add__",
        "__sub__",
        "__mul__",
        "__matmul__",
        "__truediv__",
        "__floordiv__",
        "__mod__",
        "__divmod__",
        "__pow__",
        "__lshift__",
        "__rshift__",
        "__and__",
        "__xor__",
        "__or__",
    ):
        # list of methods extracted from https://docs.python.org/3.7/reference/datamodel.html#emulating-numeric-types
        convert_type_error_to_not_implemented(cls, method)


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
