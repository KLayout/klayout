from typing import Any, ClassVar, Dict, Sequence, List, Iterator, Optional
from typing import overload
class AbsoluteProgress(Progress):
    r"""
    @brief A progress reporter counting progress in absolute units

    An absolute progress reporter counts from 0 upwards without a known limit. A unit value is used to convert the value to a bar value. One unit corresponds to 1% on the bar.
    For formatted output, a format string can be specified as well as a unit value by which the current value is divided before it is formatted.

    The progress can be configured to have a description text, a title and a format.
    The "inc" method increments the value, the "set" or "value=" methods set the value to a specific value.

    While one of these three methods is called, they will run the event loop in regular intervals. That makes the application respond to mouse clicks, specifically the Cancel button on the progress bar. If that button is clicked, an exception will be raised by these methods.

    The progress object must be destroyed explicitly in order to remove the progress status bar.

    The following sample code creates a progress bar which displays the current count as "Megabytes".
    For the progress bar, one percent corresponds to 16 kByte:

    @code
    p = RBA::AbsoluteProgress::new("test")
    p.format = "%.2f MBytes"
    p.unit = 1024*16
    p.format_unit = 1024*1024
    begin
      10000000.times { p.inc }
    ensure
      p.destroy
    end
    @/code

    This class has been introduced in version 0.23.
    """
    @property
    def format(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief sets the output format (sprintf notation) for the progress text
        """
    @property
    def format_unit(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the format unit

        This is the unit used for formatted output.
        The current count is divided by the format unit to render
        the value passed to the format string.
        """
    @property
    def unit(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the unit

        Specifies the count value corresponding to 1 percent on the progress bar. By default, the current value divided by the unit is used to create the formatted value from the output string. Another attribute is provided (\format_unit=) to specify a separate unit for that purpose.
        """
    @property
    def value(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the progress value
        """
    @overload
    @classmethod
    def new(cls, desc: str) -> AbsoluteProgress:
        r"""
        @brief Creates an absolute progress reporter with the given description
        """
    @overload
    @classmethod
    def new(cls, desc: str, yield_interval: int) -> AbsoluteProgress:
        r"""
        @brief Creates an absolute progress reporter with the given description

        The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, the event loop will be executed every tenth call of \inc or \set.
        """
    def __copy__(self) -> AbsoluteProgress:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> AbsoluteProgress:
        r"""
        @brief Creates a copy of self
        """
    @overload
    def __init__(self, desc: str) -> None:
        r"""
        @brief Creates an absolute progress reporter with the given description
        """
    @overload
    def __init__(self, desc: str, yield_interval: int) -> None:
        r"""
        @brief Creates an absolute progress reporter with the given description

        The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, the event loop will be executed every tenth call of \inc or \set.
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: Progress) -> None:
        r"""
        @brief Assigns another object to self
        """
    def dup(self) -> AbsoluteProgress:
        r"""
        @brief Creates a copy of self
        """
    def inc(self) -> AbsoluteProgress:
        r"""
        @brief Increments the progress value
        """
    def set(self, value: int, force_yield: bool) -> None:
        r"""
        @brief Sets the progress value

        This method is equivalent to \value=, but it allows forcing the event loop to be triggered.
        If "force_yield" is true, the event loop will be triggered always, irregardless of the yield interval specified in the constructor.
        """

class AbstractProgress(Progress):
    r"""
    @brief The abstract progress reporter

    The abstract progress reporter acts as a 'bracket' for a sequence of operations which are connected logically. For example, a DRC script consists of multiple operations. An abstract progress reportert is instantiated during the run time of the DRC script. This way, the application leaves the UI open while the DRC executes and log messages can be collected.

    The abstract progress does not have a value.

    This class has been introduced in version 0.27.
    """
    @classmethod
    def new(cls, desc: str) -> AbstractProgress:
        r"""
        @brief Creates an abstract progress reporter with the given description
        """
    def __copy__(self) -> AbstractProgress:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> AbstractProgress:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self, desc: str) -> None:
        r"""
        @brief Creates an abstract progress reporter with the given description
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: Progress) -> None:
        r"""
        @brief Assigns another object to self
        """
    def dup(self) -> AbstractProgress:
        r"""
        @brief Creates a copy of self
        """

class ArgType:
    r"""
    @hide
    """
    TypeBool: ClassVar[int]
    r"""
    """
    TypeByteArray: ClassVar[int]
    r"""
    """
    TypeChar: ClassVar[int]
    r"""
    """
    TypeDouble: ClassVar[int]
    r"""
    """
    TypeFloat: ClassVar[int]
    r"""
    """
    TypeInt: ClassVar[int]
    r"""
    """
    TypeLong: ClassVar[int]
    r"""
    """
    TypeLongLong: ClassVar[int]
    r"""
    """
    TypeMap: ClassVar[int]
    r"""
    """
    TypeObject: ClassVar[int]
    r"""
    """
    TypeSChar: ClassVar[int]
    r"""
    """
    TypeShort: ClassVar[int]
    r"""
    """
    TypeString: ClassVar[int]
    r"""
    """
    TypeUChar: ClassVar[int]
    r"""
    """
    TypeUInt: ClassVar[int]
    r"""
    """
    TypeULong: ClassVar[int]
    r"""
    """
    TypeULongLong: ClassVar[int]
    r"""
    """
    TypeUShort: ClassVar[int]
    r"""
    """
    TypeVar: ClassVar[int]
    r"""
    """
    TypeVector: ClassVar[int]
    r"""
    """
    TypeVoid: ClassVar[int]
    r"""
    """
    TypeVoidPtr: ClassVar[int]
    r"""
    """
    @classmethod
    def new(cls) -> ArgType:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> ArgType:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> ArgType:
        r"""
        @brief Creates a copy of self
        """
    def __eq__(self, arg0: object) -> bool:
        r"""
        @brief Equality of two types
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def __ne__(self, arg0: object) -> bool:
        r"""
        @brief Inequality of two types
        """
    def __repr__(self) -> str:
        r"""
        @brief Convert to a string
        """
    def __str__(self) -> str:
        r"""
        @brief Convert to a string
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: ArgType) -> None:
        r"""
        @brief Assigns another object to self
        """
    def cls(self) -> Class:
        r"""
        @brief Specifies the class for t_object.. types
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def default(self) -> Any:
        r"""
        @brief Returns the default value or nil is there is no default value
        Applies to arguments only. This method has been introduced in version 0.24.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> ArgType:
        r"""
        @brief Creates a copy of self
        """
    def has_default(self) -> bool:
        r"""
        @brief Returns true, if a default value is specified for this argument
        Applies to arguments only. This method has been introduced in version 0.24.
        """
    def inner(self) -> ArgType:
        r"""
        @brief Returns the inner ArgType object (i.e. value of a vector/map)
        Starting with version 0.22, this method replaces the is_vector method.
        """
    def inner_k(self) -> ArgType:
        r"""
        @brief Returns the inner ArgType object (i.e. key of a map)
        This method has been introduced in version 0.27.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_cptr(self) -> bool:
        r"""
        @brief True, if the type is a const pointer to the given type
        This property indicates that the argument is a const pointer (in C++: 'const X *').
        """
    def is_cref(self) -> bool:
        r"""
        @brief True, if the type is a const reference to the given type
        This property indicates that the argument is a const reference (in C++: 'const X &').
        """
    def is_iter(self) -> bool:
        r"""
        @brief (Return value only) True, if the return value is an iterator rendering the given type
        """
    def is_ptr(self) -> bool:
        r"""
        @brief True, if the type is a non-const pointer to the given type
        This property indicates that the argument is a non-const pointer (in C++: 'X *').
        """
    def is_ref(self) -> bool:
        r"""
        @brief True, if the type is a reference to the given type
        Starting with version 0.22 there are more methods that describe the type of reference and is_ref? only applies to non-const reference (in C++: 'X &').
        """
    def name(self) -> str:
        r"""
        @brief Returns the name for this argument or an empty string if the argument is not named
        Applies to arguments only. This method has been introduced in version 0.24.
        """
    def pass_obj(self) -> bool:
        r"""
        @brief True, if the ownership over an object represented by this type is passed to the receiver
        In case of the return type, a value of true indicates, that the object is a freshly created one and the receiver has to take ownership of the object.

        This method has been introduced in version 0.24.
        """
    def to_s(self) -> str:
        r"""
        @brief Convert to a string
        """
    def type(self) -> int:
        r"""
        @brief Return the basic type (see t_.. constants)
        """

class Class:
    r"""
    @hide
    """
    @classmethod
    def each_class(cls) -> Iterator[Class]:
        r"""
        @brief Iterate over all classes
        """
    @classmethod
    def new(cls) -> Class:
        r"""
        @brief Creates a new object of this class
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def base(self) -> Class:
        r"""
        @brief The base class or nil if the class does not have a base class

        This method has been introduced in version 0.22.
        """
    def can_copy(self) -> bool:
        r"""
        @brief True if the class offers assignment
        """
    def can_destroy(self) -> bool:
        r"""
        @brief True if the class offers a destroy method

        This method has been introduced in version 0.22.
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def doc(self) -> str:
        r"""
        @brief The documentation string for this class
        """
    def each_child_class(self) -> Iterator[Class]:
        r"""
        @brief Iterate over all child classes defined within this class
        """
    def each_method(self) -> Iterator[Method]:
        r"""
        @brief Iterate over all methods of this class
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def module(self) -> str:
        r"""
        @brief The name of module where the class lives
        """
    def name(self) -> str:
        r"""
        @brief The name of the class
        """
    def parent(self) -> Class:
        r"""
        @brief The parent of the class
        """
    def python_methods(self, static: bool) -> List[PythonFunction]:
        r"""
        @brief Gets the Python methods (static or non-static)
        """
    def python_properties(self, static: bool) -> List[PythonGetterSetterPair]:
        r"""
        @brief Gets the Python properties (static or non-static) as a list of getter/setter pairs
        Note that if a getter or setter is not available the list of Python functions for this part is empty.
        """

class EmptyClass:
    r"""
    """
    @classmethod
    def new(cls) -> EmptyClass:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> EmptyClass:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> EmptyClass:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: EmptyClass) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> EmptyClass:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class Executable(ExecutableBase):
    r"""
    @brief A generic executable object
    This object is a delegate for implementing the actual function of some generic executable function. In addition to the plain execution, if offers a post-mortem cleanup callback which is always executed, even if execute's implementation is cancelled in the debugger.

    Parameters are kept as a generic key/value map.

    This class has been introduced in version 0.27.
    """
    def _assign(self, other: ExecutableBase) -> None:
        r"""
        @brief Assigns another object to self
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _dup(self) -> Executable:
        r"""
        @brief Creates a copy of self
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """

class ExecutableBase:
    r"""
    @hide
    @alias Executable
    """
    @classmethod
    def new(cls) -> ExecutableBase:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> ExecutableBase:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> ExecutableBase:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: ExecutableBase) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> ExecutableBase:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class Expression(ExpressionContext):
    r"""
    @brief Evaluation of Expressions

    This class allows evaluation of expressions. Expressions are used in many places throughout KLayout and provide computation features for various applications. Having a script language, there is no real use for expressions inside a script client. This class is provided mainly for testing purposes.

    An expression is 'compiled' into an Expression object and can be evaluated multiple times.

    This class has been introduced in version 0.25. In version 0.26 it was separated into execution and context.
    """
    @property
    def text(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the given text as the expression.
        """
    @classmethod
    def _class_eval(cls, expr: str) -> Any:
        r"""
        @brief A convience function to evaluate the given expression and directly return the result
        This is a static method that does not require instantiation of the expression object first.
        """
    @classmethod
    def eval(cls, expr: str) -> Any:
        r"""
        @brief A convience function to evaluate the given expression and directly return the result
        This is a static method that does not require instantiation of the expression object first.
        """
    @overload
    @classmethod
    def new(cls, expr: str) -> Expression:
        r"""
        @brief Creates an expression evaluator
        """
    @overload
    @classmethod
    def new(cls, expr: str, variables: Dict[str, Any]) -> Expression:
        r"""
        @brief Creates an expression evaluator
        This version of the constructor takes a hash of variables available to the expressions.
        """
    @overload
    def __init__(self, expr: str) -> None:
        r"""
        @brief Creates an expression evaluator
        """
    @overload
    def __init__(self, expr: str, variables: Dict[str, Any]) -> None:
        r"""
        @brief Creates an expression evaluator
        This version of the constructor takes a hash of variables available to the expressions.
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _inst_eval(self) -> Any:
        r"""
        @brief Evaluates the current expression and returns the result
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def eval(self) -> Any:
        r"""
        @brief Evaluates the current expression and returns the result
        """

class ExpressionContext:
    r"""
    @brief Represents the context of an expression evaluation

    The context provides a variable namespace for the expression evaluation.

    This class has been introduced in version 0.26 when \Expression was separated into the execution and context part.
    """
    @classmethod
    def global_var(cls, name: str, value: Any) -> None:
        r"""
        @brief Defines a global variable with the given name and value
        """
    @classmethod
    def new(cls) -> ExpressionContext:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> ExpressionContext:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> ExpressionContext:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: ExpressionContext) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> ExpressionContext:
        r"""
        @brief Creates a copy of self
        """
    def eval(self, expr: str) -> Any:
        r"""
        @brief Compiles and evaluates the given expression in this context
        This method has been introduced in version 0.26.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def var(self, name: str, value: Any) -> None:
        r"""
        @brief Defines a variable with the given name and value
        """

class GlobPattern:
    r"""
    @brief A glob pattern matcher
    This class is provided to make KLayout's glob pattern matching available to scripts too. The intention is to provide an implementation which is compatible with KLayout's pattern syntax.

    This class has been introduced in version 0.26.
    """
    case_sensitive: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the glob pattern match is case sensitive.
    Setter:
    @brief Sets a value indicating whether the glob pattern match is case sensitive.
    """
    head_match: bool
    r"""
    Getter:
    @brief Gets a value indicating whether trailing characters are allowed.

    Setter:
    @brief Sets a value indicating whether trailing characters are allowed.
    If this predicate is false, the glob pattern needs to match the full subject string. If true, the match function will ignore trailing characters and return true if the front part of the subject string matches.
    """
    @classmethod
    def new(cls, pattern: str) -> GlobPattern:
        r"""
        @brief Creates a new glob pattern match object
        """
    def __copy__(self) -> GlobPattern:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> GlobPattern:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self, pattern: str) -> None:
        r"""
        @brief Creates a new glob pattern match object
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: GlobPattern) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> GlobPattern:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def match(self, subject: str) -> Any:
        r"""
        @brief Matches the subject string against the pattern.
        Returns nil if the subject string does not match the pattern. Otherwise returns a list with the substrings captured in round brackets.
        """

class Interpreter:
    r"""
    @brief A generalization of script interpreters
    The main purpose of this class is to provide cross-language call options. Using the Python interpreter, it is possible to execute Python code from Ruby for example.

    The following example shows how to use the interpreter class to execute Python code from Ruby and how to pass values from Ruby to Python and back using the \Value wrapper object:

    @code
    pya = RBA::Interpreter::python_interpreter
    out_param = RBA::Value::new(17)
    pya.define_variable("out_param", out_param)
    pya.eval_string(<<END)
    print("This is Python now!")
    out_param.value = out_param.value + 25
    END
    puts out_param.value  # gives '42'@/code

    This class was introduced in version 0.27.5.
    """
    @classmethod
    def new(cls) -> Interpreter:
        r"""
        @brief Creates a new object of this class
        """
    @classmethod
    def python_interpreter(cls) -> Interpreter:
        r"""
        @brief Gets the instance of the Python interpreter
        """
    @classmethod
    def ruby_interpreter(cls) -> Interpreter:
        r"""
        @brief Gets the instance of the Ruby interpreter
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def define_variable(self, name: str, value: Any) -> None:
        r"""
        @brief Defines a (global) variable with the given name and value
        You can use the \Value class to provide 'out' or 'inout' parameters which can be modified by code executed inside the interpreter and read back by the caller.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def eval_expr(self, string: str, filename: Optional[str] = ..., line: Optional[int] = ...) -> Any:
        r"""
        @brief Executes the expression inside the given string and returns the result value
        Use 'filename' and 'line' to indicate the original source for the error messages.
        """
    def eval_string(self, string: str, filename: Optional[str] = ..., line: Optional[int] = ...) -> None:
        r"""
        @brief Executes the code inside the given string
        Use 'filename' and 'line' to indicate the original source for the error messages.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def load_file(self, path: str) -> None:
        r"""
        @brief Loads the given file into the interpreter
        This will execute the code inside the file.
        """

class Logger:
    r"""
    @brief A logger

    The logger outputs messages to the log channels. If the log viewer is open, the log messages will be shown in the logger view. Otherwise they will be printed to the terminal on Linux for example.

    A code example:

    @code
    RBA::Logger::error("An error message")
    RBA::Logger::warn("A warning")
    @/code

    This class has been introduced in version 0.23.
    """
    verbosity: ClassVar[int]
    r"""
    @brief Returns the verbosity level

    The verbosity level is defined by the application (see -d command line option for example). Level 0 is silent, levels 10, 20, 30 etc. denote levels with increasing verbosity. 11, 21, 31 .. are sublevels which also enable timing logs in addition to messages.
    """
    @classmethod
    def error(cls, msg: str) -> None:
        r"""
        @brief Writes the given string to the error channel

        The error channel is formatted as an error (i.e. red in the logger window) and output unconditionally.
        """
    @classmethod
    def info(cls, msg: str) -> None:
        r"""
        @brief Writes the given string to the info channel

        The info channel is printed as neutral messages unconditionally.
        """
    @classmethod
    def log(cls, msg: str) -> None:
        r"""
        @brief Writes the given string to the log channel

        Log messages are printed as neutral messages and are output only if the verbosity is above 0.
        """
    @classmethod
    def new(cls) -> Logger:
        r"""
        @brief Creates a new object of this class
        """
    @classmethod
    def warn(cls, msg: str) -> None:
        r"""
        @brief Writes the given string to the warning channel

        The warning channel is formatted as a warning (i.e. blue in the logger window) and output unconditionally.
        """
    def __copy__(self) -> Logger:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> Logger:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: Logger) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> Logger:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class Method:
    r"""
    @hide
    """
    @classmethod
    def new(cls) -> Method:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> Method:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> Method:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def accepts_num_args(self, arg0: int) -> bool:
        r"""
        @brief True, if this method is compatible with the given number of arguments

        This method has been introduced in version 0.24.
        """
    def assign(self, other: Method) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def doc(self) -> str:
        r"""
        @brief The documentation string for this method
        """
    def dup(self) -> Method:
        r"""
        @brief Creates a copy of self
        """
    def each_argument(self) -> Iterator[ArgType]:
        r"""
        @brief Iterate over all arguments of this method
        """
    def each_overload(self) -> Iterator[MethodOverload]:
        r"""
        @brief This iterator delivers the synonyms (overloads).

        This method has been introduced in version 0.24.
        """
    def is_const(self) -> bool:
        r"""
        @brief True, if this method does not alter the object
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_constructor(self) -> bool:
        r"""
        @brief True, if this method is a constructor
        Static methods that return new objects are constructors.
        This method has been introduced in version 0.25.
        """
    def is_protected(self) -> bool:
        r"""
        @brief True, if this method is protected

        This method has been introduced in version 0.24.
        """
    def is_signal(self) -> bool:
        r"""
        @brief True, if this method is a signal

        Signals replace events for version 0.25. is_event? is no longer available.
        """
    def is_static(self) -> bool:
        r"""
        @brief True, if this method is static (a class method)
        """
    def name(self) -> str:
        r"""
        @brief The name string of the method
        A method may have multiple names (aliases). The name string delivers all of them in a combined way.

        The names are separated by pipe characters (|). A trailing star (*) indicates that the method is protected.

        Names may be prefixed by a colon (:) to indicate a property getter. This colon does not appear in the method name.

        A hash prefix indicates that a specific alias is deprecated.

        Names may be suffixed by a question mark (?) to indicate a predicate or a equal character (=) to indicate a property setter. Depending on the preferences of the language, these characters may appear in the method names of not - in Python they don't, in Ruby they will be part of the method name.

        The backslash character is used inside the names to escape these special characters.

        The preferred method of deriving the overload is to iterate then using \each_overload.
        """
    def primary_name(self) -> str:
        r"""
        @brief The primary name of the method
        The primary name is the first name of a sequence of aliases.

        This method has been introduced in version 0.24.
        """
    def python_methods(self) -> str:
        r"""
        @brief Gets the Python specific documentation
        """
    def ret_type(self) -> ArgType:
        r"""
        @brief The return type of this method
        """

class MethodOverload:
    r"""
    @hide
    """
    @classmethod
    def new(cls) -> MethodOverload:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> MethodOverload:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> MethodOverload:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: MethodOverload) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def deprecated(self) -> bool:
        r"""
        @brief A value indicating that this overload is deprecated
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> MethodOverload:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_getter(self) -> bool:
        r"""
        @brief A value indicating that this overload is a property getter
        """
    def is_predicate(self) -> bool:
        r"""
        @brief A value indicating that this overload is a predicate
        """
    def is_setter(self) -> bool:
        r"""
        @brief A value indicating that this overload is a property setter
        """
    def name(self) -> str:
        r"""
        @brief The name of this overload
        This is the raw, unadorned name. I.e. no question mark suffix for predicates, no equal character suffix for setters etc.
        """

class Progress:
    r"""
    @brief A progress reporter

    This is the base class for all progress reporter objects. Progress reporter objects are used to report the progress of some operation and to allow aborting an operation. Progress reporter objects must be triggered periodically, i.e. a value must be set. On the display side, a progress bar usually is used to represent the progress of an operation.

    Actual implementations of the progress reporter class are \RelativeProgress and \AbsoluteProgress.

    This class has been introduced in version 0.23.
    """
    @property
    def title(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the title text of the progress object

        Initially the title is equal to the description.
        """
    desc: str
    r"""
    Getter:
    @brief Gets the description text of the progress object

    Setter:
    @brief Sets the description text of the progress object
    """
    @classmethod
    def new(cls) -> Progress:
        r"""
        @brief Creates a new object of this class
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class PythonFunction:
    r"""
    @hide
    """
    @classmethod
    def new(cls) -> PythonFunction:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> PythonFunction:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> PythonFunction:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: PythonFunction) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> PythonFunction:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_protected(self) -> bool:
        r"""
        @brief Gets a value indicating whether this function is protected
        """
    def is_static(self) -> bool:
        r"""
        @brief Gets the value indicating whether this Python function is 'static' (class function)
        """
    def methods(self) -> List[Method]:
        r"""
        @brief Gets the list of methods bound to this Python function
        """
    def name(self) -> str:
        r"""
        @brief Gets the name of this Python function
        """

class PythonGetterSetterPair:
    r"""
    @hide
    """
    @classmethod
    def new(cls) -> PythonGetterSetterPair:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> PythonGetterSetterPair:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> PythonGetterSetterPair:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: PythonGetterSetterPair) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> PythonGetterSetterPair:
        r"""
        @brief Creates a copy of self
        """
    def getter(self) -> PythonFunction:
        r"""
        @brief Gets the getter function
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def setter(self) -> PythonFunction:
        r"""
        @brief Gets the setter function
        """

class Recipe:
    r"""
    @brief A facility for providing reproducible recipes
    The idea of this facility is to provide a service by which an object
    can be reproduced in a parametrized way. The intended use case is a 
    DRC report for example, where the DRC script is the generator.

    In this use case, the DRC engine will register a recipe. It will 
    put the serialized version of the recipe into the DRC report. If the 
    user requests a re-run of the DRC, the recipe will be called and 
    the implementation is supposed to deliver a new database.

    To register a recipe, reimplement the Recipe class and create an
    instance. To serialize a recipe, use "generator", to execute the
    recipe, use "make".

    Parameters are kept as a generic key/value map.

    This class has been introduced in version 0.26.
    """
    @classmethod
    def make(cls, generator: str, add_params: Optional[Dict[str, Any]] = ...) -> Any:
        r"""
        @brief Executes the recipe given by the generator string.
        The generator string is the one delivered with \generator.
        Additional parameters can be passed in "add_params". They have lower priority than the parameters kept inside the generator string.
        """
    @classmethod
    def new(cls, name: str, description: Optional[str] = ...) -> Recipe:
        r"""
        @brief Creates a new recipe object with the given name and (optional) description
        """
    def __init__(self, name: str, description: Optional[str] = ...) -> None:
        r"""
        @brief Creates a new recipe object with the given name and (optional) description
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def description(self) -> str:
        r"""
        @brief Gets the description of the recipe.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def generator(self, params: Dict[str, Any]) -> str:
        r"""
        @brief Delivers the generator string from the given parameters.
        The generator string can be used with \make to re-run the recipe.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def name(self) -> str:
        r"""
        @brief Gets the name of the recipe.
        """

class RelativeProgress(Progress):
    r"""
    @brief A progress reporter counting progress in relative units

    A relative progress reporter counts from 0 to some maximum value representing 0 to 100 percent completion of a task. The progress can be configured to have a description text, a title and a format.
    The "inc" method increments the value, the "set" or "value=" methods set the value to a specific value.

    While one of these three methods is called, they will run the event loop in regular intervals. That makes the application respond to mouse clicks, specifically the Cancel button on the progress bar. If that button is clicked, an exception will be raised by these methods.

    The progress object must be destroyed explicitly in order to remove the progress status bar.

    A code example:

    @code
    p = RBA::RelativeProgress::new("test", 10000000)
    begin
      10000000.times { p.inc }
    ensure
      p.destroy
    end
    @/code

    This class has been introduced in version 0.23.
    """
    @property
    def format(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief sets the output format (sprintf notation) for the progress text
        """
    @property
    def value(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the progress value
        """
    @overload
    @classmethod
    def new(cls, desc: str, max_value: int) -> RelativeProgress:
        r"""
        @brief Creates a relative progress reporter with the given description and maximum value

        The reported progress will be 0 to 100% for values between 0 and the maximum value.
        The values are always integers. Double values cannot be used property.
        """
    @overload
    @classmethod
    def new(cls, desc: str, max_value: int, yield_interval: int) -> RelativeProgress:
        r"""
        @brief Creates a relative progress reporter with the given description and maximum value

        The reported progress will be 0 to 100% for values between 0 and the maximum value.
        The values are always integers. Double values cannot be used property.

        The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, the event loop will be executed every tenth call of \inc or \set.
        """
    def __copy__(self) -> RelativeProgress:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> RelativeProgress:
        r"""
        @brief Creates a copy of self
        """
    @overload
    def __init__(self, desc: str, max_value: int) -> None:
        r"""
        @brief Creates a relative progress reporter with the given description and maximum value

        The reported progress will be 0 to 100% for values between 0 and the maximum value.
        The values are always integers. Double values cannot be used property.
        """
    @overload
    def __init__(self, desc: str, max_value: int, yield_interval: int) -> None:
        r"""
        @brief Creates a relative progress reporter with the given description and maximum value

        The reported progress will be 0 to 100% for values between 0 and the maximum value.
        The values are always integers. Double values cannot be used property.

        The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, the event loop will be executed every tenth call of \inc or \set.
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: Progress) -> None:
        r"""
        @brief Assigns another object to self
        """
    def dup(self) -> RelativeProgress:
        r"""
        @brief Creates a copy of self
        """
    def inc(self) -> RelativeProgress:
        r"""
        @brief Increments the progress value
        """
    def set(self, value: int, force_yield: bool) -> None:
        r"""
        @brief Sets the progress value

        This method is equivalent to \value=, but it allows forcing the event loop to be triggered.
        If "force_yield" is true, the event loop will be triggered always, irregardless of the yield interval specified in the constructor.
        """

class Timer:
    r"""
    @brief A timer (stop watch)

    The timer provides a way to measure CPU time. It provides two basic methods: start and stop. After it has been started and stopped again, the time can be retrieved using the user and sys attributes, i.e.:

    @code
    t = RBA::Timer::new
    t.start
    # ... do something
    t.stop
    puts "it took #{t.sys} seconds (kernel), #{t.user} seconds (user) on the CPU"
    @/code

    The time is reported in seconds.

    This class has been introduced in version 0.23.
    """
    @classmethod
    def memory_size(cls) -> int:
        r"""
        @brief Gets the current memory usage of the process in Bytes

        This method has been introduced in version 0.27.
        """
    @classmethod
    def new(cls) -> Timer:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> Timer:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> Timer:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def __repr__(self) -> str:
        r"""
        @brief Produces a string with the currently elapsed times
        """
    def __str__(self) -> str:
        r"""
        @brief Produces a string with the currently elapsed times
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: Timer) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> Timer:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def start(self) -> None:
        r"""
        @brief Starts the timer
        """
    def stop(self) -> None:
        r"""
        @brief Stops the timer
        """
    def sys(self) -> float:
        r"""
        @brief Returns the elapsed CPU time in kernel mode from start to stop in seconds
        """
    def to_s(self) -> str:
        r"""
        @brief Produces a string with the currently elapsed times
        """
    def user(self) -> float:
        r"""
        @brief Returns the elapsed CPU time in user mode from start to stop in seconds
        """
    def wall(self) -> float:
        r"""
        @brief Returns the elapsed real time from start to stop in seconds
        This method has been introduced in version 0.26.
        """

class Value:
    r"""
    @brief Encapsulates a value (preferably a plain data type) in an object
    This class is provided to 'box' a value (encapsulate the value in an object). This class is required to interface to pointer or reference types in a method call. By using that class, the method can alter the value and thus implement 'out parameter' semantics. The value may be 'nil' which acts as a null pointer in pointer type arguments.
    This class has been introduced in version 0.22.
    """
    value: Any
    r"""
    Getter:
    @brief Gets the actual value.

    Setter:
    @brief Set the actual value.
    """
    @overload
    @classmethod
    def new(cls) -> Value:
        r"""
        @brief Constructs a nil object.
        """
    @overload
    @classmethod
    def new(cls, value: Any) -> Value:
        r"""
        @brief Constructs a non-nil object with the given value.
        This constructor has been introduced in version 0.22.
        """
    def __copy__(self) -> Value:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> Value:
        r"""
        @brief Creates a copy of self
        """
    @overload
    def __init__(self) -> None:
        r"""
        @brief Constructs a nil object.
        """
    @overload
    def __init__(self, value: Any) -> None:
        r"""
        @brief Constructs a non-nil object with the given value.
        This constructor has been introduced in version 0.22.
        """
    def __repr__(self) -> str:
        r"""
        @brief Convert this object to a string
        """
    def __str__(self) -> str:
        r"""
        @brief Convert this object to a string
        """
    def _create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        r"""
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        r"""
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.

        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: Value) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        r"""
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> Value:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def to_s(self) -> str:
        r"""
        @brief Convert this object to a string
        """

