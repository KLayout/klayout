""" Stub file generation routines.

This module contains routines to generate stub files from klayout's python API.
This uses the `tl` module of the API, which offers an introspection layer to
the C-extension modules.
"""

from collections import Counter
from copy import copy
from dataclasses import dataclass, field
from functools import wraps
import functools
from sys import argv
from textwrap import indent
from typing import Any, Dict, List, Optional, Set, Tuple, Union
from klayout.tlcore import ArgType, Method
import pya  # initialize all modules
import klayout.tl as ktl


def qualified_name(_class: ktl.Class) -> str:
    name = _class.name()
    if _class.parent():
        return f"{qualified_name(_class.parent())}.{name}"
    else:
        return name


def superclass(_class: ktl.Class) -> str:
    if _class.base():
        return superclass(_class.base())
    else:
        return _class.name()


def is_reserved_word(name: str) -> bool:
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


_type_dict = dict()

_type_dict[ktl.ArgType.TypeBool] = "bool"
_type_dict[ktl.ArgType.TypeChar] = "str"
_type_dict[ktl.ArgType.TypeDouble] = "float"
_type_dict[ktl.ArgType.TypeFloat] = "float"
_type_dict[ktl.ArgType.TypeInt] = "int"
_type_dict[ktl.ArgType.TypeLong] = "int"
_type_dict[ktl.ArgType.TypeLongLong] = "int"
# _type_dict[ktl.ArgType.TypeMap] = None
# _type_dict[ktl.ArgType.TypeObject] = None
_type_dict[ktl.ArgType.TypeSChar] = "str"
_type_dict[ktl.ArgType.TypeShort] = "int"
_type_dict[ktl.ArgType.TypeString] = "str"
_type_dict[ktl.ArgType.TypeUChar] = "str"
_type_dict[ktl.ArgType.TypeUInt] = "int"
_type_dict[ktl.ArgType.TypeULong] = "int"
_type_dict[ktl.ArgType.TypeULongLong] = "int"
_type_dict[ktl.ArgType.TypeUShort] = "int"
_type_dict[ktl.ArgType.TypeVar] = "Any"
# _type_dict[ktl.ArgType.TypeVector] = None
_type_dict[ktl.ArgType.TypeVoid] = "None"
_type_dict[ktl.ArgType.TypeVoidPtr] = "None"


def _translate_type(arg_type: ktl.ArgType, within_class: ktl.Class) -> str:
    """Translates klayout's C-type to a type in Python.

    This function is equivalent to the `type_to_s` in `pyaModule.cc`.
    See also `type_to_s` in `layGSIHelpProvider.cc`"""

    py_str: str = ""
    if arg_type.type() == ktl.ArgType.TypeObject:
        if within_class.module() and arg_type.cls().module() != within_class.module():
            py_str = arg_type.cls().module() + "." + qualified_name(arg_type.cls())
        else:
            py_str = qualified_name(arg_type.cls())
    elif arg_type.type() == ktl.ArgType.TypeMap:
        inner_key = _translate_type(arg_type.inner_k(), within_class)
        inner_val = _translate_type(arg_type.inner(), within_class)
        py_str = f"Dict[{inner_key}, {inner_val}]"
    elif arg_type.type() == ktl.ArgType.TypeVector:
        py_str = f"Iterable[{_translate_type(arg_type.inner(), within_class)}]"
    else:
        py_str = _type_dict[arg_type.type()]

    if arg_type.is_iter():
        py_str = f"Iterable[{py_str}]"
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
    name: Any
    docstring: str
    indent_docstring: bool = True
    child_stubs: List["Stub"] = field(default_factory=list)
    decorator: str = ""

    def __eq__(self, __o: object) -> bool:
        if not isinstance(__o, Stub):
            return False
        return (
            self.signature == __o.signature
            and self.child_stubs == __o.child_stubs
            and self.decorator == __o.decorator
        )

    def __lt__(self, other: "Stub") -> bool:
        # mypy complains if an overload with float happens before int
        self_signature = self.signature.replace(": int", "0").replace(": float", "1")
        other_signature = other.signature.replace(": int", "0").replace(": float", "1")

        self_sortkey = self.name, len(self.signature.split(",")), self_signature
        other_sortkey = other.name, len(other.signature.split(",")), other_signature
        return self_sortkey < other_sortkey

    def __hash__(self):
        return hash(self.format_stub(include_docstring=False))

    def format_stub(self, include_docstring=True):
        lines = []
        lines.extend(self.decorator.splitlines())
        if self.indent_docstring:  # all but properties
            if include_docstring or len(self.child_stubs) > 0:
                lines.append(self.signature + ":")
            else:
                lines.append(self.signature + ": ...")
        else:
            lines.append(self.signature)

        stub_str = "\n".join(lines)

        lines = []
        lines.append('r"""')
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
            stub_str += indent(
                stub.format_stub(include_docstring=include_docstring), " " * 4
            )

        return stub_str


@dataclass(eq=False)
class MethodStub(Stub):
    indent_docstring: bool = True


@dataclass(eq=False)
class PropertyStub(Stub):
    indent_docstring: bool = False


@dataclass(eq=False)
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
        is_classvar = (num_args == 0) and (m.is_static() and not m.is_constructor())
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


def get_py_child_classes(c: ktl.Class):
    for c_child in c.each_child_class():
        return c_child


def get_py_methods(
    c: ktl.Class,
) -> List[Stub]:
    c_methods = get_c_methods(c)

    # extract properties
    _c_methods = copy(c_methods)

    # Helper functions
    def find_setter(c_methods: List[_Method], name: str):
        """Finds a setter method in c_methods list with a given name.f"""
        for m in c_methods:
            if m.name == name and m.is_setter:
                return m
        return None

    def find_getter(c_methods: List[_Method], name: str):
        """Finds a getter method in c_methods list with a given name.f"""
        for m in c_methods:
            if m.name == name and m.is_getter:
                return m
        return None

    translate_type = functools.partial(_translate_type, within_class=c)

    def _get_arglist(m: ktl.Method, self_str) -> List[Tuple[str, ktl.ArgType]]:
        args = [(self_str, None)]
        for i, a in enumerate(m.each_argument()):
            argname = a.name()
            if is_reserved_word(argname):
                argname += "_"
            elif not argname:
                argname = f"arg{i}"
            args.append((argname, a))
        return args

    def _format_args(arglist: List[Tuple[str, Optional[str]]]):
        args = []
        for argname, argtype in arglist:
            if argtype:
                args.append(f"{argname}: {argtype}")
            else:
                args.append(argname)
        return ", ".join(args)

    def format_args(m: ktl.Method, self_str: str = "self") -> str:
        arg_list = _get_arglist(m, self_str=self_str)
        new_arglist: List[Tuple[str, Optional[str]]] = []
        for argname, a in arg_list:
            if a:
                new_arglist.append((argname, translate_type(a)))
            else:
                new_arglist.append((argname, None))
        return _format_args(new_arglist)

    # Extract all properties (methods that have getters and/or setters)
    properties: List[Stub] = list()
    for m in copy(_c_methods):
        ret_type = translate_type(m.m.ret_type())
        if m.is_getter:
            m_setter = find_setter(c_methods, m.name)
            if m_setter is not None:  # full property
                doc = m.doc + m_setter.doc
                properties.append(
                    PropertyStub(
                        decorator="",
                        signature=f"{translate_methodname(m.name)}: {ret_type}",
                        name=f"{translate_methodname(m.name)}",
                        docstring=doc,
                    )
                )
                # _c_methods.remove(m_setter)
            elif m.is_classvar:
                properties.append(
                    PropertyStub(
                        decorator="",
                        signature=f"{translate_methodname(m.name)}: ClassVar[{ret_type}]",
                        name=f"{translate_methodname(m.name)}",
                        docstring=m.doc,
                    )
                )
            else:  # only getter
                properties.append(
                    MethodStub(
                        decorator="@property",
                        signature=f"def {translate_methodname(m.name)}(self) -> {ret_type}",
                        name=f"{translate_methodname(m.name)}",
                        docstring=m.doc,
                    )
                )
            _c_methods.remove(m)
        elif m.is_setter and not find_getter(
            c_methods, m.name
        ):  # include setter-only properties as full properties
            doc = "WARNING: This variable can only be set, not retrieved.\n" + m.doc
            properties.append(
                PropertyStub(
                    decorator="",
                    signature=f"{translate_methodname(m.name)}: {ret_type}",
                    name=f"{translate_methodname(m.name)}",
                    docstring=doc,
                )
            )
            _c_methods.remove(m)

    for m in copy(_c_methods):
        if m.is_setter:
            _c_methods.remove(m)

    def get_altnames(c_name: str):
        names = [c_name]
        if c_name == "to_s":
            names.append("__str__")
        return names

    # Extract all classmethods
    classmethods: List[Stub] = list()
    for m in copy(_c_methods):
        if m.is_classmethod:
            # Exception: if it is an __init__ constructor, ignore.
            # Will be treated by the bound method logic below.
            if translate_methodname(m.name) == "__init__":
                continue
            decorator = "@classmethod"
            ret_type = translate_type(m.m.ret_type())
            for name in get_altnames(m.name):
                classmethods.append(
                    MethodStub(
                        decorator=decorator,
                        signature=f"def {translate_methodname(name)}({format_args(m.m, 'cls')}) -> {ret_type}",
                        name=f"{translate_methodname(name)}",
                        docstring=m.doc,
                    )
                )
            _c_methods.remove(m)

    # Extract bound methods
    boundmethods: List[Stub] = list()
    for m in copy(_c_methods):
        decorator = ""

        translated_name = translate_methodname(m.name)
        if translated_name in [s.name for s in properties]:
            translated_name += "_"

        if translated_name == "__init__":
            ret_type = "None"
        else:
            ret_type = translate_type(m.m.ret_type())

        arg_list = _get_arglist(m.m, "self")
        # Exceptions:
        # For X.__eq__(self, a:X), treat second argument as type object instead of X
        if translated_name in ("__eq__", "__ne__"):
            arg_list[1] = arg_list[1][0], "object"
        # X._assign(self, other:X), mypy complains if _assign is defined at base class.
        # We can't specialize other in this case.
        elif translated_name in ("_assign", "assign"):
            assert arg_list[1][1].type() == ktl.ArgType.TypeObject
            arg_list[1] = arg_list[1][0], superclass(arg_list[1][1].cls())
        else:
            new_arg_list = []
            for argname, a in arg_list:
                if a:
                    new_arg_list.append((argname, translate_type(a)))
                else:
                    new_arg_list.append((argname, a))
            arg_list = new_arg_list
        formatted_args = _format_args(arg_list)

        for name in get_altnames(translated_name):
            boundmethods.append(
                MethodStub(
                    decorator=decorator,
                    signature=f"def {name}({formatted_args}) -> {ret_type}",
                    name=f"{name}",
                    docstring=m.doc,
                )
            )
        _c_methods.remove(m)

    def add_overload_decorator(stublist: List[Stub]):
        stubnames = [stub.name for stub in stublist]
        for stub in stublist:
            has_duplicate = stubnames.count(stub.name) > 1
            if has_duplicate:
                stub.decorator = "@overload\n" + stub.decorator
        return stublist

    boundmethods = sorted(set(boundmethods))  # sometimes duplicate methods are defined.
    add_overload_decorator(boundmethods)
    properties = sorted(set(properties))
    classmethods = sorted(classmethods)
    add_overload_decorator(classmethods)

    return_list: List[Stub] = properties + classmethods + boundmethods

    return return_list


def get_class_stub(
    c: ktl.Class,
    ignore: List[ktl.Class] = None,
    module: str = "",
) -> ClassStub:
    base = ""
    if c.base():
        base = f"({c.base().name()})"
    if c.module() != module:
        full_name = c.module() + "." + c.name()
    else:
        full_name = c.name()
    _cstub = ClassStub(
        signature="class " + full_name + base, docstring=c.doc(), name=full_name
    )
    child_attributes = get_py_methods(c)
    for child_c in c.each_child_class():
        _cstub.child_stubs.append(
            get_class_stub(
                child_c,
                ignore=ignore,
                module=c.module(),
            )
        )
    for stub in child_attributes:
        _cstub.child_stubs.append(stub)
    return _cstub

def get_classes(module: str) -> List[ktl.Class]:
    _classes = []
    for c in ktl.Class.each_class():
        if c.module() != module:
            continue
        _classes.append(c)
    return _classes

def get_module_stubs(module:str) -> List[ClassStub]:
    _stubs = []
    _classes = get_classes(module)
    for c in _classes:
        _cstub = get_class_stub(c, ignore=_classes, module=module)
        _stubs.append(_cstub)
    return _stubs


def print_db():
    print("from typing import Any, ClassVar, Dict, Iterable, Optional")
    print("from typing import overload")
    print("import klayout.rdb as rdb")
    print("import klayout.tl as tl")
    for stub in get_module_stubs("db"):
        print(stub.format_stub(include_docstring=True) + "\n")


def print_rdb():
    print("from typing import Any, ClassVar, Dict, Iterable, Optional")
    print("from typing import overload")
    print("import klayout.db as db")
    for stub in get_module_stubs("rdb"):
        print(stub.format_stub(include_docstring=True) + "\n")

def print_tl():
    print("from typing import Any, ClassVar, Dict, Iterable, Optional")
    print("from typing import overload")
    for stub in get_module_stubs("tl"):
        print(stub.format_stub(include_docstring=True) + "\n")


def test_v1():
    db_classes = get_classes("db")
    for c in db_classes:
        if c.name() != "Region":
            continue
        print(
            get_class_stub(c, ignore=db_classes, module="db").format_stub(
                include_docstring=False
            )
        )


def test_v2():
    db_classes = get_classes("db")
    for c in db_classes:
        if c.name() != "DPoint":
            continue
        print(
            get_class_stub(c, ignore=db_classes, module="db").format_stub(
                include_docstring=True
            )
        )


def test_v3():
    db_classes = get_classes("db")
    for c in db_classes:
        if c.name() != "Instance":
            continue
        print(
            get_class_stub(c, ignore=db_classes, module="db").format_stub(
                include_docstring=False
            )
        )


def test_v4():
    db_classes = get_classes("db")
    for c in db_classes:
        if c.name() != "Region":
            continue
        for cclass in get_py_child_classes(c):
            print(
                get_class_stub(cclass, ignore=db_classes, module="db").format_stub(
                    include_docstring=False
                )
            )


if __name__ == "__main__":
    if len(argv) < 2:
        print("Specity module in argument: 'db', 'rdb', 'tl'")
        exit(1)
    if argv[1] == "db":
        print_db()
    elif argv[1] == "rdb":
        print_rdb()
    elif argv[1] == "tl":
        print_tl()
    else:
        # print_rdb()
        # test_v4()
        print("Wrong arguments")
