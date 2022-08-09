""" Stub file generation routines.

This module contains routines to generate stub files from klayout's python API.
This uses the `tl` module of the API, which offers an introspection layer to
the C-extension modules.
"""

from copy import copy
from dataclasses import dataclass, field
from textwrap import indent
from typing import Dict, List, Set, Tuple
from klayout.tlcore import ArgType, Method
import pya  # initialize all modules
import klayout.tl as ktl

def qualified_name(_class: ktl.Class) -> str:
    base = _class.base()
    name = _class.name()
    if base is None:
        return name
    else:
        return f"{qualified_name(base)}.{name}"

def is_reserved_word (name: str) -> bool:
    wordlist = [
        "and",
        "del",
        "from",
        "not",
        "while",
        "as",
        "elif",
        "global",
        "or",
        "with",
        "assert",
        "else",
        "if",
        "pass",
        "yield",
        "break",
        "except",
        "import",
        "print",
        "class",
        "exec",
        "in",
        "raise",
        "continue",
        "finally",
        "is",
        "return",
        "def",
        "for",
        "lambda",
        "try",
        "None",
    ]
    return name in wordlist

def translate_methodname(name: str) -> str:
    """
        Should be the same as pyaModule.cc:extract_python_name function
        *  The name string encodes some additional information, specifically:
        *    "*..."      The method is protected
        *    "x|y"       Aliases (synonyms)
        *    "x|#y"      y is deprecated
        *    "x="        x is a setter
        *    ":x"        x is a getter
        *    "x?"        x is a predicate
        *  Backslashes can be used to escape the special characters, like "*" and "|".
    """
    if name == "new":
        new_name = "__init__"
    elif name == "++":
        new_name = "inc"
    elif name == "--":
        new_name = "dec"
    elif name == "()":
        new_name = "call"
    elif name == "!":
        new_name = "not"
    elif name == "==":
        new_name = "__eq__"
    elif name == "!=":
        new_name = "__ne__"
    elif name == "<":
        new_name = "__lt__"
    elif name == "<=":
        new_name = "__le__"
    elif name == ">":
        new_name = "__gt__"
    elif name == ">=":
        new_name = "__ge__"
    elif name == "<=>":
        new_name = "__cmp__"
    elif name == "+":
        new_name = "__add__"
    elif name == "+@":
        new_name = "__pos__"
    elif name == "-":
        new_name = "__sub__"
    elif name == "-@":
        new_name = "__neg__"
    elif name == "/":
        new_name = "__truediv__"
    elif name == "*":
        new_name = "__mul__"
    elif name == "%":
        new_name = "__mod__"
    elif name == "<<":
        new_name = "__lshift__"
    elif name == ">>":
        new_name = "__rshift__"
    elif name == "~":
        new_name = "__invert__"
    elif name == "&":
        new_name = "__and__"
    elif name == "|":
        new_name = "__or__"
    elif name == "^":
        new_name = "__xor__"
    elif name == "+=":
        new_name = "__iadd__"
    elif name == "-=":
        new_name = "__isub__"
    elif name == "/=":
        new_name = "__itruediv__"
    elif name == "*=":
        new_name = "__imul__"
    elif name == "%=":
        new_name = "__imod__"
    elif name == "<<=":
        new_name = "__ilshift__"
    elif name == ">>=":
        new_name = "__irshift__"
    elif name == "&=":
        new_name = "__iand__"
    elif name == "|=":
        new_name = "__ior__"
    elif name == "^=":
        new_name = "__ixor__"
    elif name == "[]":
        new_name = "__getitem__"
    elif name == "[]=":
        new_name = "__setitem__"
    else:
        # Ignore other conversions for now.
        if name.startswith("*"):
            print(name)
        new_name = name
    if is_reserved_word(new_name):
        new_name = new_name + "_"

    return new_name

def translate_type(arg_type: ktl.ArgType, module: str) -> str:
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
        if module and arg_type.cls().module() != module:
            py_str = arg_type.cls().module() + "." + arg_type.cls().name()
        else:
            py_str = arg_type.cls().name()
    elif arg_type.type() == ktl.ArgType.TypeMap:
        inner_key = translate_type(arg_type.inner_k(), module)
        inner_val = translate_type(arg_type.inner(), module)
        py_str = f"Dict[{inner_key}, {inner_val}]"
    elif arg_type.type() == ktl.ArgType.TypeVector:
        py_str = f"Iterable[{translate_type(arg_type.inner(), module)}]"
    else:
        py_str = _type_dict[arg_type.type()]

    if arg_type.has_default():
        py_str = f"Optional[{py_str}] = ..."
    return py_str

@dataclass
class _Method:
    name: str
    is_setter: bool
    is_getter: bool
    is_classvar: bool
    is_classmethod: bool
    doc: str
    m: ktl.Method


@dataclass
class Stub:
    signature: str
    docstring: str
    indent_docstring: bool = True
    child_stubs: List['Stub'] = field(default_factory=list)
    decorator: str = ""

    def format_stub(self, include_docstring=True):
        lines = []
        lines.extend(self.decorator.splitlines())
        if self.indent_docstring: # all but properties
            if include_docstring or len(self.child_stubs) > 0:
                lines.append(self.signature + ":")
            else:
                lines.append(self.signature + ": ...")
        else:
            lines.append(self.signature)

        stub_str = "\n".join(lines)

        lines = []
        lines.append('"""')
        lines.extend(self.docstring.splitlines())
        lines.append('"""')
        doc_str = "\n".join(lines)

        # indent only if it is required (methods, not properties)
        if self.indent_docstring:
            doc_str = indent(doc_str, " " * 4)

        if include_docstring:
            stub_str += "\n"
            stub_str += doc_str

        for stub in self.child_stubs:
            stub_str += "\n"
            stub_str += indent(stub.format_stub(include_docstring=include_docstring), " " * 4)

        return stub_str
@dataclass
class MethodStub(Stub):
    indent_docstring: bool = True
@dataclass
class PropertyStub(Stub):
    indent_docstring: bool = False
@dataclass
class ClassStub(Stub):
    indent_docstring: bool = True

def get_c_methods(c: ktl.Class) -> List[_Method]:
    """
    Iterates over all methods defined in the C API, sorting
    properties, class methods and bound methods.
    """
    method_list: List[_Method] = list()
    setters = set()

    def primary_synonym(m: ktl.Method) -> ktl.MethodOverload:
        for ms in m.each_overload():
            if ms.name() == m.primary_name():
                return ms
        raise ("Primary synonym not found for method " + m.name())

    for m in c.each_method():
        if m.is_signal():
            # ignore signals as they do not have arguments and are neither setters nor getters.
            continue

        method_def = primary_synonym(m)
        if method_def.is_setter():
            setters.add(method_def.name())

    for m in c.each_method():
        num_args = len([True for a in m.each_argument()])
        method_def = primary_synonym(m)

        # extended definition of "getter" for Python
        is_getter = (num_args == 0) and (
            method_def.is_getter()
            or (not method_def.is_setter() and method_def.name() in setters)
        )
        is_setter = (num_args == 1) and method_def.is_setter()
        is_classvar = (num_args == 0) and (
            m.is_static() and not m.is_constructor()
        )
        method_list.append(
            _Method(
                name=method_def.name(),
                is_setter=is_setter,
                is_getter=is_getter or is_classvar,
                is_classmethod=m.is_constructor(),
                is_classvar=is_classvar,
                doc=m.doc(),
                m=m,
            )
        )

        # print(f"{m.name()}: {m.is_static()=}, {m.is_constructor()=}, {m.is_const_object()=}")
    return method_list

def get_py_methods(c: ktl.Class) -> Tuple[List[Stub],Dict[str, ktl.Class]]:
    c_methods = get_c_methods(c)
    unknown_classes: Dict[str, ktl.Class] = dict()

    # extract properties
    _c_methods = copy(c_methods)

    # Helper functions
    def find_setter(c_methods: List[_Method], name: str):
        for m in c_methods:
            if m.name == name and m.is_setter:
                return m
        return None

    def process_argtype(arg: ktl.ArgType):
        # Collects all used classes into a set
        # this is required because some class variables are defined within the class.
        if arg.type() == ktl.ArgType.TypeObject:
            unknown_classes[arg.cls().name()] = arg.cls()

    def format_args(m: ktl.Method, self_str="self"):
        args = [self_str]
        for i, a in enumerate(m.each_argument()):
            process_argtype(a)
            argname = a.name()
            if is_reserved_word(argname):
                argname += "_"
            elif not argname:
                argname = f"arg{i}"
            args.append(f"{argname}: {translate_type(a, c.module())}")
        return ", ".join(args)

    # Extract all properties (methods that have getters and/or setters)
    properties = list()
    for m in copy(_c_methods):
        if m.is_getter:
            m_setter = find_setter(c_methods, m.name)
            process_argtype(m.m.ret_type())
            ret_type = translate_type(m.m.ret_type(), c.module())
            if m_setter is not None:  # full property
                doc = m.doc + m_setter.doc
                properties.append(
                    PropertyStub(
                        decorator="",
                        signature=f"{translate_methodname(m.name)}: {ret_type}",
                        docstring=doc,
                    )
                )
                # _c_methods.remove(m_setter)
            elif m.is_classvar:
                properties.append(
                    PropertyStub(
                        decorator="",
                        signature=f"{translate_methodname(m.name)}: ClassVar[{ret_type}]",
                        docstring=m.doc,
                    )
                )
            else:  # only getter
                properties.append(
                    MethodStub(
                        decorator="@property",
                        signature=f"def {translate_methodname(m.name)}(self) -> {ret_type}",
                        docstring=m.doc,
                    )
                )
            _c_methods.remove(m)

    for m in copy(_c_methods):
        if m.is_setter:
            _c_methods.remove(m)


    # Extract all classmethods
    classmethods = list()
    for m in copy(_c_methods):
        if m.is_classmethod:
            decorator = "@classmethod"
            is_duplicate = len([_m.name for _m in c_methods if _m.name == m.name]) > 1
            if is_duplicate:
                decorator = "@overload\n" + decorator
            process_argtype(m.m.ret_type())
            ret_type = translate_type(m.m.ret_type(), c.module())
            classmethods.append(
                MethodStub(
                    decorator=decorator,
                    signature=f"def {translate_methodname(m.name)}({format_args(m.m, 'cls')}) -> {ret_type}",
                    docstring=m.doc,
                )
            )
            _c_methods.remove(m)

    # Extract bound methods
    boundmethods = list()
    for m in copy(_c_methods):
        decorator = ""
        is_duplicate = len([_m.name for _m in c_methods if _m.name == m.name]) > 1
        if is_duplicate:
            decorator = "@overload\n" + decorator
        process_argtype(m.m.ret_type())
        ret_type = translate_type(m.m.ret_type(), c.module())
        boundmethods.append(
            MethodStub(
                decorator=decorator,
                signature=f"def {translate_methodname(m.name)}({format_args(m.m, 'self')}) -> {ret_type}",
                docstring=m.doc,
            )
        )
        _c_methods.remove(m)

    return properties + classmethods + boundmethods, unknown_classes

def get_class_stub(c: ktl.Class, ignore: List[ktl.Class] = None, module: str = ""):
    base = ""
    if c.base():
        base = f"({c.base().name()})"
    if c.module() != module:
        full_name = c.module() + "." + c.name()
    else:
        full_name = c.name()
    _cstub = ClassStub(signature = "class " + full_name + base, docstring=c.doc())
    child_attributes, child_classes = get_py_methods(c)
    if ignore is None:
        ignore = []
    for ignore_c in ignore + [c]:
        child_classes.pop(ignore_c.name(), None)
    for child_c in child_classes.values():
        if child_c.module():
        # if child class has a module, no need to include it.
            continue
        _cstub.child_stubs.append(get_class_stub(child_c, ignore=ignore, module=c.module()))
    for stub in child_attributes:
        _cstub.child_stubs.append(stub)
    return _cstub

def get_db_classes() -> List[ktl.Class]:
    _classes = []
    for c in ktl.Class.each_class():
        if c.module() != 'db':
            continue
        _classes.append(c)
    return _classes

def get_db_stubs() -> List[ClassStub]:
    _stubs = []
    db_classes = get_db_classes()
    for c in db_classes:
        _cstub = get_class_stub(c, ignore=db_classes, module="db")
        _stubs.append(_cstub)
    return _stubs

def print_db():
    print("from typing import Any, ClassVar, Dict, Iterable, Optional")
    print("from typing import overload")
    for stub in get_db_stubs():
        print(stub.format_stub(include_docstring=False) + "\n")

def test_v1():
    for c in ktl.Class.each_class():
        base = ""
        if c.base():
            base = f"({c.base().name()})"
        if c.name() != 'Region':
            continue
        print(c.module() + "." + c.name() + base + ":")
        for stub in get_py_methods(c):
            print(stub.format_stub(include_docstring=False) + "\n")

def test_v2():
    db_classes = get_db_classes()
    for c in db_classes:
        if c.name() != 'DPoint':
            continue
        print(get_class_stub(c, ignore=db_classes, module="db").format_stub(include_docstring=True))
def test_v3():
    db_classes = get_db_classes()
    for c in db_classes:
        if c.name() != 'Instance':
            continue
        print(get_class_stub(c, ignore=db_classes, module="db").format_stub(include_docstring=False))

if __name__ == "__main__":
    print_db()
    # test_v3()