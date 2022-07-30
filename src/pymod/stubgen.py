""" Stub file generation routines.

This module contains routines to generate stub files from klayout's python API.
This uses the `tl` module of the API, which offers an introspection layer to
the C-extension modules.
"""

import pya  # initialize all modules
import klayout.tl as ktl

def qualified_name(_class: ktl.Class) -> str:
    base = _class.base()
    name = _class.name()
    if base is None:
        return name
    else:
        return f"{qualified_name(base)}.{name}"

def translate_type(arg_type: ktl.ArgType) -> str:
    """ Translates klayout's C-type to a type in Python.

    This function is equivalent to the `type_to_s` in `pyaModule.cc`.
    See also `type_to_s` in `layGSIHelpProvider.cc`"""
    _type_dict = dict()

    _type_dict[ktl.ArgType.TypeBool] = "bool"
    _type_dict[ktl.ArgType.TypeChar] = "str"
    _type_dict[ktl.ArgType.TypeDouble] = "float"
    _type_dict[ktl.ArgType.TypeFloat] = "float"
    _type_dict[ktl.ArgType.TypeInt] = "int"
    _type_dict[ktl.ArgType.TypeLong] = "int"
    _type_dict[ktl.ArgType.TypeLongLong] = "int"
    _type_dict[ktl.ArgType.TypeMap] = None
    _type_dict[ktl.ArgType.TypeObject] = None
    _type_dict[ktl.ArgType.TypeSChar] = "str"
    _type_dict[ktl.ArgType.TypeShort] = "int"
    _type_dict[ktl.ArgType.TypeString] = "str"
    _type_dict[ktl.ArgType.TypeUChar] = "str"
    _type_dict[ktl.ArgType.TypeUInt] = "int"
    _type_dict[ktl.ArgType.TypeULong] = "int"
    _type_dict[ktl.ArgType.TypeULongLong] = "int"
    _type_dict[ktl.ArgType.TypeUShort] = "int"
    _type_dict[ktl.ArgType.TypeVar] = "Any"
    _type_dict[ktl.ArgType.TypeVector] = None
    _type_dict[ktl.ArgType.TypeVoid] = "None"
    _type_dict[ktl.ArgType.TypeVoidPtr] = "None"

    py_str:str = ""
    if arg_type.type() == ktl.ArgType.TypeObject:
        py_str = qualified_name(arg_type.cls())
    elif arg_type.type() == ktl.ArgType.TypeMap:
        inner_key = translate_type(arg_type.inner_k())
        inner_val = translate_type(arg_type.inner())
        py_str = f"Dict[{inner_key}, {inner_val}]"
    elif arg_type.type() == ktl.ArgType.TypeVector:
        py_str = f"Iterable[{translate_type(arg_type.inner())}]"
    else:
        py_str = _type_dict[arg_type.type()]

    if arg_type.has_default():
        py_str = f"Optional[{py_str}] = ..."
    return py_str

def traverse_methods(_class = ktl.Class):
    """
    *  The name string encodes some additional information, specifically:
        *    "*..."      The method is protected
        *    "x|y"       Aliases (synonyms)
        *    "x|#y"      y is deprecated
        *    "x="        x is a setter
        *    ":x"        x is a getter
        *    "x?"        x is a predicate
        *  Backslashes can be used to escape the special characters, like "*" and "|".
    # """
    # getter_list = [m.name() for m in c.each_method() if m.is_getter()]
    # setter_list = [m.name() for m in c.each_method() if m.is_setter() and m.name() not in getter_list]
    # method_list = [m.name() for m in c.each_method() if m.name() not in set(getter_list + setter_list)]
    # print(getter_list)
    # print(setter_list)
    # print(method_list)
    for m in c.each_method():
        ov = list(m.each_overload())
        print(m.name(), m.is_signal(), [(_ov.name(), _ov.is_getter(), _ov.is_setter()) for _ov in ov])


for c in ktl.Class.each_class():
    base = ""
    if c.base():
        base = f"({c.base().name()})"
    if 'Path' not in c.name():
        continue
    print(c.module() + "." + c.name() + base + ":")
    for m in c.each_method():
        args = ["self"] + [f"{a.name()}: {translate_type(a)}" for a in m.each_argument()]
        print("  " + m.name() + "(" + ", ".join(args) +") -> " + translate_type(m.ret_type()))
