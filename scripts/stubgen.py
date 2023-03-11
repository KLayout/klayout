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
from typing import Any, List, Optional, Tuple, Union
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
_type_dict[ktl.ArgType.TypeByteArray] = "bytes"
_type_dict[ktl.ArgType.TypeUChar] = "str"
_type_dict[ktl.ArgType.TypeUInt] = "int"
_type_dict[ktl.ArgType.TypeULong] = "int"
_type_dict[ktl.ArgType.TypeULongLong] = "int"
_type_dict[ktl.ArgType.TypeUShort] = "int"
_type_dict[ktl.ArgType.TypeVar] = "Any"
# _type_dict[ktl.ArgType.TypeVector] = None
_type_dict[ktl.ArgType.TypeVoid] = "None"
_type_dict[ktl.ArgType.TypeVoidPtr] = "None"


def _translate_type(
    arg_type: ktl.ArgType, within_class: ktl.Class, is_return=False
) -> str:
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
        inner_key = _translate_type(arg_type.inner_k(), within_class, is_return)
        inner_val = _translate_type(arg_type.inner(), within_class, is_return)
        py_str = f"Dict[{inner_key}, {inner_val}]"
    elif arg_type.type() == ktl.ArgType.TypeVector:
        if is_return:
            py_str = f"List[{_translate_type(arg_type.inner(), within_class, is_return)}]"
        else:
            py_str = f"Sequence[{_translate_type(arg_type.inner(), within_class, is_return)}]"
    else:
        py_str = _type_dict[arg_type.type()]

    if arg_type.is_iter():
        py_str = f"Iterator[{py_str}]"
    if arg_type.has_default():
        py_str = f"Optional[{py_str}] = ..."
    return py_str


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

def get_child_classes(c: ktl.Class):
    child_classes = []
    for c_child in c.each_child_class():
        child_classes.append(c_child)
    return sorted(child_classes, key=lambda cls: cls.name())

def get_py_child_classes(c: ktl.Class):
    for c_child in get_child_classes(c):
        yield c_child


def get_py_methods(
    c: ktl.Class,
) -> List[Stub]:

    translate_arg_type = functools.partial(_translate_type, within_class=c, is_return=False)
    translate_ret_type = functools.partial(_translate_type, within_class=c, is_return=True)

    def _get_arglist(
        m: ktl.Method, self_str: str
    ) -> List[Tuple[str, Optional[ktl.ArgType]]]:
        args: List[Tuple[str, Optional[ktl.ArgType]]] = [(self_str, None)]
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
                new_arglist.append((argname, translate_arg_type(a)))
            else:
                new_arglist.append((argname, None))
        return _format_args(new_arglist)

    # Collect all properties here
    properties: List[Stub] = list()

    # Extract all instance properties
    for f in c.python_properties(False): 
        name = f.getter().name()
        getter = None
        if len(f.getter().methods()) > 0:
            getter = f.getter().methods()[0]
        setter = None
        if len(f.setter().methods()) > 0:
            setter = f.setter().methods()[0]
        if getter and setter: 
            # Full property
            ret_type = translate_ret_type(getter.ret_type())
            doc = "Getter:\n" + getter.doc() + "\nSetter:\n" + setter.doc()
            properties.append(
                PropertyStub(
                    decorator="",
                    signature=f"{name}: {ret_type}",
                    name=name,
                    docstring=doc,
                )
            )
        elif getter:
            # Only getter
            ret_type = translate_ret_type(getter.ret_type())
            doc = getter.doc()
            properties.append(
                MethodStub(
                    decorator="@property",
                    signature=f"def {name}(self) -> {ret_type}",
                    name=name,
                    docstring=doc,
                )
            )
        elif setter:
            # Only setter
            doc = "WARNING: This variable can only be set, not retrieved.\n" + setter.doc()
            properties.append(
                MethodStub(
                    decorator="@property",
                    signature=f"def {name}(self) -> None",
                    name=name,
                    docstring=doc,
                )
            )

    # Extract all class properties (TODO: setters not supported currently)
    for f in c.python_properties(True): 
        name = f.getter().name()
        if len(f.getter().methods()) > 0:
            getter = f.getter().methods()[0]
            ret_type = translate_ret_type(getter.ret_type())
            doc = getter.doc()
            properties.append(
                PropertyStub(
                    decorator="",
                    signature=f"{name}: ClassVar[{ret_type}]",
                    name=name,
                    docstring=doc,
                )
            )

    # Extract all classmethods
    classmethods: List[Stub] = list()

    for f in c.python_methods(True): 

        name = f.name()

        decorator = ""
        if len(f.methods()) > 1:
            decorator = "@overload\n"
        decorator += "@classmethod"

        for m in f.methods():
            ret_type = translate_ret_type(m.ret_type())
            classmethods.append(
                MethodStub(
                    decorator=decorator,
                    signature=f"def {name}({format_args(m, 'cls')}) -> {ret_type}",
                    name=name,
                    docstring=m.doc(),
                )
            )

    # Extract bound methods
    boundmethods: List[Stub] = list()

    for f in c.python_methods(False): 

        name = f.name()

        decorator = ""
        if len(f.methods()) > 1:
            decorator = "@overload\n"

        for m in f.methods():

            if name == "__init__":
                ret_type = "None"
            else:
                ret_type = translate_ret_type(m.ret_type())

            arg_list = _get_arglist(m, "self")
            # TODO: fix type errors
            # Exceptions:
            # For X.__eq__(self, a:X), treat second argument as type object instead of X
            if name in ("__eq__", "__ne__"):
                arg_list[1] = arg_list[1][0], "object"
            # X._assign(self, other:X), mypy complains if _assign is defined at base class.
            # We can't specialize other in this case.
            elif name in ("_assign", "assign"):
                assert arg_list[1][1] is not None
                assert arg_list[1][1].type() == ktl.ArgType.TypeObject
                arg_list[1] = arg_list[1][0], superclass(arg_list[1][1].cls())
            else:
                new_arg_list = []
                for argname, a in arg_list:
                    if a:
                        new_arg_list.append((argname, translate_arg_type(a)))
                    else:
                        new_arg_list.append((argname, a))
                arg_list = new_arg_list
            formatted_args = _format_args(arg_list)

            boundmethods.append(
                MethodStub(
                    decorator=decorator,
                    signature=f"def {name}({formatted_args}) -> {ret_type}",
                    name=name,
                    docstring=m.doc(),
                )
            )

    boundmethods = sorted(boundmethods, key=lambda m: m.signature)
    properties = sorted(properties, key=lambda m: m.signature)
    classmethods = sorted(classmethods, key=lambda m: m.signature)

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
    for child_c in get_child_classes(c):
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
    return sorted(_classes, key=lambda cls: cls.name())


def get_module_stubs(module: str) -> List[ClassStub]:
    _stubs = []
    _classes = get_classes(module)
    for c in _classes:
        _cstub = get_class_stub(c, ignore=_classes, module=module)
        _stubs.append(_cstub)
    return _stubs


def print_mod(module, dependencies):
    print("from typing import Any, ClassVar, Dict, Sequence, List, Iterator, Optional")
    print("from typing import overload")
    for dep in dependencies:
      print(f"import klayout.{dep} as {dep}")
    for stub in get_module_stubs(module):
        print(stub.format_stub(include_docstring=True) + "\n")


if __name__ == "__main__":
    if len(argv) < 2:
        print("Specify module in argument")
        exit(1)
    if len(argv) == 2:
        print_mod(argv[1], [])
    else:
        print_mod(argv[1], argv[2].split(","))
