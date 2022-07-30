from typing import Any, ClassVar, Dict, Optional

from typing import overload

class AbsoluteProgress(Progress):
    """
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


    __gsi_id__: ClassVar[int] = ...
    format: None
    """
    format(format: str) -> None
    @brief sets the output format (sprintf notation) for the progress text
    """
    format_unit: None
    """
    format_unit(unit: float) -> None
    @brief Sets the format unit
    
    This is the unit used for formatted output.
    The current count is divided by the format unit to render
    the value passed to the format string.
    """
    unit: None
    """
    unit(unit: float) -> None
    @brief Sets the unit
    
    Specifies the count value corresponding to 1 percent on the progress bar. By default, the current value divided by the unit is used to create the formatted value from the output string. Another attribute is provided (\format_unit=) to specify a separate unit for that purpose.
    """
    value: None
    """
    value(value: int) -> None
    @brief Sets the progress value
    """
    @overload
    def __init__(self, desc: str) -> AbsoluteProgress:
        """
        __init__(desc: str) -> AbsoluteProgress
        @brief Creates an absolute progress reporter with the given description
        """
    @overload
    def __init__(self, desc: str, yield_interval: int) -> AbsoluteProgress:
        """
        __init__(desc: str, yield_interval: int) -> AbsoluteProgress
        @brief Creates an absolute progress reporter with the given description
        
        The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, the event loop will be executed every tenth call of \inc or \set.
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def inc(self) -> AbsoluteProgress:
        """
        inc() -> AbsoluteProgress
        @brief Increments the progress value
        """
    @classmethod
    @overload
    def new(cls, desc: str) -> AbsoluteProgress:
        """
        new(desc: str) -> AbsoluteProgress
        @brief Creates an absolute progress reporter with the given description
        """
    @overload
    def new(cls, desc: str, yield_interval: int) -> AbsoluteProgress:
        """
        new(desc: str, yield_interval: int) -> AbsoluteProgress
        @brief Creates an absolute progress reporter with the given description
        
        The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, the event loop will be executed every tenth call of \inc or \set.
        """
    def set(self, value: int, force_yield: bool) -> None:
        """
        set(value: int, force_yield: bool) -> None
        @brief Sets the progress value
        
        This method is equivalent to \value=, but it allows forcing the event loop to be triggered.
        If "force_yield" is true, the event loop will be triggered always, irregardless of the yield interval specified in the constructor.
        """

class AbstractProgress(Progress):
    """
    @brief The abstract progress reporter
    
    The abstract progress reporter acts as a 'bracket' for a sequence of operations which are connected logically. For example, a DRC script consists of multiple operations. An abstract progress reportert is instantiated during the run time of the DRC script. This way, the application leaves the UI open while the DRC executes and log messages can be collected.
    
    The abstract progress does not have a value.
    
    This class has been introduced in version 0.27.
    """


    __gsi_id__: ClassVar[int] = ...
    def __init__(self, desc: str) -> AbstractProgress:
        """
        __init__(desc: str) -> AbstractProgress
        @brief Creates an abstract progress reporter with the given description
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    @classmethod
    def new(cls, desc: str) -> AbstractProgress:
        """
        new(desc: str) -> AbstractProgress
        @brief Creates an abstract progress reporter with the given description
        """

class ArgType:
    """
    @hide
    """


    __gsi_id__: ClassVar[int] = ...
    TypeBool: Any
    TypeChar: Any
    TypeDouble: Any
    TypeFloat: Any
    TypeInt: Any
    TypeLong: Any
    TypeLongLong: Any
    TypeMap: Any
    TypeObject: Any
    TypeSChar: Any
    TypeShort: Any
    TypeString: Any
    TypeUChar: Any
    TypeUInt: Any
    TypeULong: Any
    TypeULongLong: Any
    TypeUShort: Any
    TypeVar: Any
    TypeVector: Any
    TypeVoid: Any
    TypeVoidPtr: Any
    def __init__(self) -> ArgType:
        """
        __init__() -> ArgType
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: ArgType) -> None:
        """
        assign(other: ArgType) -> None
        @brief Assigns another object to self
        """
    def cls(self) -> Class:
        """
        cls() -> Class
        @brief Specifies the class for t_object.. types
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def default(self) -> Any:
        """
        default() -> Any
        @brief Returns the default value or nil is there is no default value
        Applies to arguments only. This method has been introduced in version 0.24.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> ArgType:
        """
        dup() -> ArgType
        @brief Creates a copy of self
        """
    def has_default(self) -> bool:
        """
        has_default() -> bool
        @brief Returns true, if a default value is specified for this argument
        Applies to arguments only. This method has been introduced in version 0.24.
        """
    def inner(self) -> ArgType:
        """
        inner() -> ArgType
        @brief Returns the inner ArgType object (i.e. value of a vector/map)
        Starting with version 0.22, this method replaces the is_vector method.
        """
    def inner_k(self) -> ArgType:
        """
        inner_k() -> ArgType
        @brief Returns the inner ArgType object (i.e. key of a map)
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_cptr(self) -> bool:
        """
        is_cptr() -> bool
        @brief True, if the type is a const pointer to the given type
        This property indicates that the argument is a const pointer (in C++: 'const X *').
        """
    def is_cref(self) -> bool:
        """
        is_cref() -> bool
        @brief True, if the type is a const reference to the given type
        This property indicates that the argument is a const reference (in C++: 'const X &').
        """
    def is_iter(self) -> bool:
        """
        is_iter() -> bool
        @brief (Return value only) True, if the return value is an iterator rendering the given type
        """
    def is_ptr(self) -> bool:
        """
        is_ptr() -> bool
        @brief True, if the type is a non-const pointer to the given type
        This property indicates that the argument is a non-const pointer (in C++: 'X *').
        """
    def is_ref(self) -> bool:
        """
        is_ref() -> bool
        @brief True, if the type is a reference to the given type
        Starting with version 0.22 there are more methods that describe the type of reference and is_ref? only applies to non-const reference (in C++: 'X &').
        """
    def name(self) -> str:
        """
        name() -> str
        @brief Returns the name for this argument or an empty string if the argument is not named
        Applies to arguments only. This method has been introduced in version 0.24.
        """
    @classmethod
    def new(cls) -> ArgType:
        """
        new() -> ArgType
        @brief Creates a new object of this class
        """
    def pass_obj(self) -> bool:
        """
        pass_obj() -> bool
        @brief True, if the ownership over an object represented by this type is passed to the receiver
        In case of the return type, a value of true indicates, that the object is a freshly created one and the receiver has to take ownership of the object.
        
        This method has been introduced in version 0.24.
        """
    @classmethod
    def t_bool(cls) -> int:
        """
        t_bool() -> int
        """
    @classmethod
    def t_char(cls) -> int:
        """
        t_char() -> int
        """
    @classmethod
    def t_double(cls) -> int:
        """
        t_double() -> int
        """
    @classmethod
    def t_float(cls) -> int:
        """
        t_float() -> int
        """
    @classmethod
    def t_int(cls) -> int:
        """
        t_int() -> int
        """
    @classmethod
    def t_long(cls) -> int:
        """
        t_long() -> int
        """
    @classmethod
    def t_longlong(cls) -> int:
        """
        t_longlong() -> int
        """
    @classmethod
    def t_map(cls) -> int:
        """
        t_map() -> int
        """
    @classmethod
    def t_object(cls) -> int:
        """
        t_object() -> int
        """
    @classmethod
    def t_schar(cls) -> int:
        """
        t_schar() -> int
        """
    @classmethod
    def t_short(cls) -> int:
        """
        t_short() -> int
        """
    @classmethod
    def t_string(cls) -> int:
        """
        t_string() -> int
        """
    @classmethod
    def t_uchar(cls) -> int:
        """
        t_uchar() -> int
        """
    @classmethod
    def t_uint(cls) -> int:
        """
        t_uint() -> int
        """
    @classmethod
    def t_ulong(cls) -> int:
        """
        t_ulong() -> int
        """
    @classmethod
    def t_ulonglong(cls) -> int:
        """
        t_ulonglong() -> int
        """
    @classmethod
    def t_ushort(cls) -> int:
        """
        t_ushort() -> int
        """
    @classmethod
    def t_var(cls) -> int:
        """
        t_var() -> int
        """
    @classmethod
    def t_vector(cls) -> int:
        """
        t_vector() -> int
        """
    @classmethod
    def t_void(cls) -> int:
        """
        t_void() -> int
        """
    @classmethod
    def t_void_ptr(cls) -> int:
        """
        t_void_ptr() -> int
        """
    def to_s(self) -> str:
        """
        to_s() -> str
        @brief Convert to a string
        """
    def type(self) -> int:
        """
        type() -> int
        @brief Return the basic type (see t_.. constants)
        """
    def __eq__(self, arg1: ArgType) -> bool:
        """
        __eq__(arg1: ArgType) -> bool
        @brief Equality of two types
        """
    def __ne__(self, arg1: ArgType) -> bool:
        """
        __ne__(arg1: ArgType) -> bool
        @brief Inequality of two types
        """

class Class:
    """
    @hide
    """


    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> Class:
        """
        __init__() -> Class
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def base(self) -> Class:
        """
        base() -> Class
        @brief The base class or nil if the class does not have a base class
        
        This method has been introduced in version 0.22.
        """
    def can_copy(self) -> bool:
        """
        can_copy() -> bool
        @brief True if the class offers assignment
        """
    def can_destroy(self) -> bool:
        """
        can_destroy() -> bool
        @brief True if the class offers a destroy method
        
        This method has been introduced in version 0.22.
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def doc(self) -> str:
        """
        doc() -> str
        @brief The documentation string for this class
        """
    @classmethod
    def each_class(cls) -> Class:
        """
        each_class() -> Class
        @brief Iterate over all classes
        """
    def each_method(self) -> Method:
        """
        each_method() -> Method
        @brief Iterate over all methods of this class
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def module(self) -> str:
        """
        module() -> str
        @brief The name of module where the class lives
        """
    def name(self) -> str:
        """
        name() -> str
        @brief The name of the class
        """
    @classmethod
    def new(cls) -> Class:
        """
        new() -> Class
        @brief Creates a new object of this class
        """

class EmptyClass:
    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> EmptyClass:
        """
        __init__() -> EmptyClass
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: EmptyClass) -> None:
        """
        assign(other: EmptyClass) -> None
        @brief Assigns another object to self
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> EmptyClass:
        """
        dup() -> EmptyClass
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    @classmethod
    def new(cls) -> EmptyClass:
        """
        new() -> EmptyClass
        @brief Creates a new object of this class
        """

class Executable(ExecutableBase):
    """
    @brief A generic executable object
    This object is a delegate for implementing the actual function of some generic executable function. In addition to the plain execution, if offers a post-mortem cleanup callback which is always executed, even if execute's implementation is cancelled in the debugger.
    
    Parameters are kept as a generic key/value map.
    
    This class has been introduced in version 0.27.
    """


    __gsi_id__: ClassVar[int] = ...
    def _assign(self, other: Executable) -> None:
        """
        _assign(other: Executable) -> None
        @brief Assigns another object to self
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _dup(self) -> Executable:
        """
        _dup() -> Executable
        @brief Creates a copy of self
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """

class ExecutableBase:
    """
    @hide
    @alias Executable
    """


    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> ExecutableBase:
        """
        __init__() -> ExecutableBase
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: ExecutableBase) -> None:
        """
        assign(other: ExecutableBase) -> None
        @brief Assigns another object to self
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> ExecutableBase:
        """
        dup() -> ExecutableBase
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    @classmethod
    def new(cls) -> ExecutableBase:
        """
        new() -> ExecutableBase
        @brief Creates a new object of this class
        """

class Expression(ExpressionContext):
    """
    @brief Evaluation of Expressions
    
    This class allows evaluation of expressions. Expressions are used in many places throughout KLayout and provide computation features for various applications. Having a script language, there is no real use for expressions inside a script client. This class is provided mainly for testing purposes.
    
    An expression is 'compiled' into an Expression object and can be evaluated multiple times.
    
    This class has been introduced in version 0.25. In version 0.26 it was separated into execution and context.
    """


    __gsi_id__: ClassVar[int] = ...
    eval: Any
    text: None
    """
    text(expr: str) -> None
    @brief Sets the given text as the expression.
    """
    @overload
    def __init__(self, expr: str) -> Expression:
        """
        __init__(expr: str) -> Expression
        @brief Creates an expression evaluator
        """
    @overload
    def __init__(self, expr: str, variables: Dict[str,Any]) -> Expression:
        """
        __init__(expr: str, variables: Dict[str, Any]) -> Expression
        @brief Creates an expression evaluator
        This version of the constructor takes a hash of variables available to the expressions.
        """
    @classmethod
    def _class_eval(cls, expr: str) -> Any:
        """
        _class_eval(expr: str) -> Any
        @brief A convience function to evaluate the given expression and directly return the result
        This is a static method that does not require instantiation of the expression object first.
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _inst_eval(self) -> Any:
        """
        _inst_eval() -> Any
        @brief Evaluates the current expression and returns the result
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    @classmethod
    @overload
    def new(cls, expr: str) -> Expression:
        """
        new(expr: str) -> Expression
        @brief Creates an expression evaluator
        """
    @overload
    def new(cls, expr: str, variables: Dict[str,Any]) -> Expression:
        """
        new(expr: str, variables: Dict[str, Any]) -> Expression
        @brief Creates an expression evaluator
        This version of the constructor takes a hash of variables available to the expressions.
        """

class ExpressionContext:
    """
    @brief Represents the context of an expression evaluation
    
    The context provides a variable namespace for the expression evaluation.
    
    This class has been introduced in version 0.26 when \Expression was separated into the execution and context part.
    """


    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> ExpressionContext:
        """
        __init__() -> ExpressionContext
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: ExpressionContext) -> None:
        """
        assign(other: ExpressionContext) -> None
        @brief Assigns another object to self
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> ExpressionContext:
        """
        dup() -> ExpressionContext
        @brief Creates a copy of self
        """
    def eval(self, expr: str) -> Any:
        """
        eval(expr: str) -> Any
        @brief Compiles and evaluates the given expression in this context
        This method has been introduced in version 0.26.
        """
    @classmethod
    def global_var(cls, name: str, value: Any) -> None:
        """
        global_var(name: str, value: Any) -> None
        @brief Defines a global variable with the given name and value
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    @classmethod
    def new(cls) -> ExpressionContext:
        """
        new() -> ExpressionContext
        @brief Creates a new object of this class
        """
    def var(self, name: str, value: Any) -> None:
        """
        var(name: str, value: Any) -> None
        @brief Defines a variable with the given name and value
        """

class GlobPattern:
    """
    @brief A glob pattern matcher
    This class is provided to make KLayout's glob pattern matching available to scripts too. The intention is to provide an implementation which is compatible with KLayout's pattern syntax.
    
    This class has been introduced in version 0.26.
    """


    __gsi_id__: ClassVar[int] = ...
    case_sensitive: bool
    """
    case_sensitive() -> bool
    @brief Gets a value indicating whether the glob pattern match is case sensitive.
    ------
    case_sensitive(case_sensitive: bool) -> None
    @brief Sets a value indicating whether the glob pattern match is case sensitive.
    """
    head_match: bool
    """
    head_match() -> bool
    @brief Gets a value indicating whether trailing characters are allowed.
    
    ------
    head_match(head_match: bool) -> None
    @brief Sets a value indicating whether trailing characters are allowed.
    If this predicate is false, the glob pattern needs to match the full subject string. If true, the match function will ignore trailing characters and return true if the front part of the subject string matches.
    """
    def __init__(self, pattern: str) -> GlobPattern:
        """
        __init__(pattern: str) -> GlobPattern
        @brief Creates a new glob pattern match object
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: GlobPattern) -> None:
        """
        assign(other: GlobPattern) -> None
        @brief Assigns another object to self
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> GlobPattern:
        """
        dup() -> GlobPattern
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def match(self, subject: str) -> Any:
        """
        match(subject: str) -> Any
        @brief Matches the subject string against the pattern.
        Returns nil if the subject string does not match the pattern. Otherwise returns a list with the substrings captured in round brackets.
        """
    @classmethod
    def new(cls, pattern: str) -> GlobPattern:
        """
        new(pattern: str) -> GlobPattern
        @brief Creates a new glob pattern match object
        """

class Interpreter:
    """
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


    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> Interpreter:
        """
        __init__() -> Interpreter
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def define_variable(self, name: str, value: Any) -> None:
        """
        define_variable(name: str, value: Any) -> None
        @brief Defines a (global) variable with the given name and value
        You can use the \Value class to provide 'out' or 'inout' parameters which can be modified by code executed inside the interpreter and read back by the caller.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def eval_expr(self, string: str, filename: Optional[str] = ..., line: Optional[int] = ...) -> Any:
        """
        eval_expr(string: str, filename: Optional[str] = None, line: Optional[int] = None) -> Any
        @brief Executes the expression inside the given string and returns the result value
        Use 'filename' and 'line' to indicate the original source for the error messages.
        """
    def eval_string(self, string: str, filename: Optional[str] = ..., line: Optional[int] = ...) -> None:
        """
        eval_string(string: str, filename: Optional[str] = None, line: Optional[int] = None) -> None
        @brief Executes the code inside the given string
        Use 'filename' and 'line' to indicate the original source for the error messages.
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def load_file(self, path: str) -> None:
        """
        load_file(path: str) -> None
        @brief Loads the given file into the interpreter
        This will execute the code inside the file.
        """
    @classmethod
    def new(cls) -> Interpreter:
        """
        new() -> Interpreter
        @brief Creates a new object of this class
        """
    @classmethod
    def python_interpreter(cls) -> Interpreter:
        """
        python_interpreter() -> Interpreter
        @brief Gets the instance of the Python interpreter
        """
    @classmethod
    def ruby_interpreter(cls) -> Interpreter:
        """
        ruby_interpreter() -> Interpreter
        @brief Gets the instance of the Ruby interpreter
        """

class Logger:
    """
    @brief A logger
    
    The logger outputs messages to the log channels. If the log viewer is open, the log messages will be shown in the logger view. Otherwise they will be printed to the terminal on Linux for example.
    
    A code example:
    
    @code
    RBA::Logger::error("An error message")
    RBA::Logger::warn("A warning")
    @/code
    
    This class has been introduced in version 0.23.
    """


    __gsi_id__: ClassVar[int] = ...
    verbosity: Any
    def __init__(self) -> Logger:
        """
        __init__() -> Logger
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    @classmethod
    def error(cls, msg: str) -> None:
        """
        error(msg: str) -> None
        @brief Writes the given string to the error channel
        
        The error channel is formatted as an error (i.e. red in the logger window) and output unconditionally.
        """
    @classmethod
    def info(cls, msg: str) -> None:
        """
        info(msg: str) -> None
        @brief Writes the given string to the info channel
        
        The info channel is printed as neutral messages unconditionally.
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    @classmethod
    def log(cls, msg: str) -> None:
        """
        log(msg: str) -> None
        @brief Writes the given string to the log channel
        
        Log messages are printed as neutral messages and are output only if the verbosity is above 0.
        """
    @classmethod
    def new(cls) -> Logger:
        """
        new() -> Logger
        @brief Creates a new object of this class
        """
    @classmethod
    def warn(cls, msg: str) -> None:
        """
        warn(msg: str) -> None
        @brief Writes the given string to the warning channel
        
        The warning channel is formatted as a warning (i.e. blue in the logger window) and output unconditionally.
        """

class Method:
    """
    @hide
    """


    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> Method:
        """
        __init__() -> Method
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def accepts_num_args(self, arg1: int) -> bool:
        """
        accepts_num_args(arg1: int) -> bool
        @brief True, if this method is compatible with the given number of arguments
        
        This method has been introduced in version 0.24.
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def doc(self) -> str:
        """
        doc() -> str
        @brief The documentation string for this method
        """
    def each_argument(self) -> ArgType:
        """
        each_argument() -> ArgType
        @brief Iterate over all arguments of this method
        """
    def each_overload(self) -> MethodOverload:
        """
        each_overload() -> MethodOverload
        @brief This iterator delivers the synonyms (overloads).
        
        This method has been introduced in version 0.24.
        """
    def is_const(self) -> bool:
        """
        is_const() -> bool
        @brief True, if this method does not alter the object
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_constructor(self) -> bool:
        """
        is_constructor() -> bool
        @brief True, if this method is a constructor
        Static methods that return new objects are constructors.
        This method has been introduced in version 0.25.
        """
    def is_protected(self) -> bool:
        """
        is_protected() -> bool
        @brief True, if this method is protected
        
        This method has been introduced in version 0.24.
        """
    def is_signal(self) -> bool:
        """
        is_signal() -> bool
        @brief True, if this method is a signal
        
        Signals replace events for version 0.25. is_event? is no longer available.
        """
    def is_static(self) -> bool:
        """
        is_static() -> bool
        @brief True, if this method is static (a class method)
        """
    def name(self) -> str:
        """
        name() -> str
        @brief The name string of the method
        A method may have multiple names (aliases). The name string delivers all of them in a combined way.
        
        The names are separated by pipe characters (|). A trailing star (*) indicates that the method is protected.
        
        Names may be prefixed by a colon (:) to indicate a property getter. This colon does not appear in the method name.
        
        A hash prefix indicates that a specific alias is deprecated.
        
        Names may be suffixed by a question mark (?) to indicate a predicate or a equal character (=) to indicate a property setter. Depending on the preferences of the language, these characters may appear in the method names of not - in Python they don't, in Ruby they will be part of the method name.
        
        The backslash character is used inside the names to escape these special characters.
        
        The preferred method of deriving the overload is to iterate then using \each_overload.
        """
    @classmethod
    def new(cls) -> Method:
        """
        new() -> Method
        @brief Creates a new object of this class
        """
    def primary_name(self) -> str:
        """
        primary_name() -> str
        @brief The primary name of the method
        The primary name is the first name of a sequence of aliases.
        
        This method has been introduced in version 0.24.
        """
    def ret_type(self) -> ArgType:
        """
        ret_type() -> ArgType
        @brief The return type of this method
        """

class MethodOverload:
    """
    @hide
    """


    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> MethodOverload:
        """
        __init__() -> MethodOverload
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: MethodOverload) -> None:
        """
        assign(other: MethodOverload) -> None
        @brief Assigns another object to self
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def deprecated(self) -> bool:
        """
        deprecated() -> bool
        @brief A value indicating that this overload is deprecated
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> MethodOverload:
        """
        dup() -> MethodOverload
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_getter(self) -> bool:
        """
        is_getter() -> bool
        @brief A value indicating that this overload is a property getter
        """
    def is_predicate(self) -> bool:
        """
        is_predicate() -> bool
        @brief A value indicating that this overload is a predicate
        """
    def is_setter(self) -> bool:
        """
        is_setter() -> bool
        @brief A value indicating that this overload is a property setter
        """
    def name(self) -> str:
        """
        name() -> str
        @brief The name of this overload
        This is the raw, unadorned name. I.e. no question mark suffix for predicates, no equal character suffix for setters etc.
        """
    @classmethod
    def new(cls) -> MethodOverload:
        """
        new() -> MethodOverload
        @brief Creates a new object of this class
        """

class Progress:
    """
    @brief A progress reporter
    
    This is the base class for all progress reporter objects. Progress reporter objects are used to report the progress of some operation and to allow aborting an operation. Progress reporter objects must be triggered periodically, i.e. a value must be set. On the display side, a progress bar usually is used to represent the progress of an operation.
    
    Actual implementations of the progress reporter class are \RelativeProgress and \AbsoluteProgress.
    
    This class has been introduced in version 0.23.
    """


    __gsi_id__: ClassVar[int] = ...
    desc: str
    """
    desc() -> str
    @brief Gets the description text of the progress object
    
    ------
    desc(desc: str) -> None
    @brief Sets the description text of the progress object
    """
    title: None
    """
    title(title: str) -> None
    @brief Sets the title text of the progress object
    
    Initially the title is equal to the description.
    """
    def __init__(self) -> Progress:
        """
        __init__() -> Progress
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    @classmethod
    def new(cls) -> Progress:
        """
        new() -> Progress
        @brief Creates a new object of this class
        """

class Recipe:
    """
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


    __gsi_id__: ClassVar[int] = ...
    def __init__(self, name: str, description: Optional[str] = ...) -> Recipe:
        """
        __init__(name: str, description: Optional[str] = None) -> Recipe
        @brief Creates a new recipe object with the given name and (optional) description
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def description(self) -> str:
        """
        description() -> str
        @brief Gets the description of the recipe.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def generator(self, params: Dict[str,Any]) -> str:
        """
        generator(params: Dict[str, Any]) -> str
        @brief Delivers the generator string from the given parameters.
        The generator string can be used with \make to re-run the recipe.
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    @classmethod
    def make(cls, generator: str, add_params: Optional[Dict[str,Any]] = ...) -> Any:
        """
        make(generator: str, add_params: Optional[Dict[str, Any]] = None) -> Any
        @brief Executes the recipe given by the generator string.
        The generator string is the one delivered with \generator.
        Additional parameters can be passed in "add_params". They have lower priority than the parameters kept inside the generator string.
        """
    def name(self) -> str:
        """
        name() -> str
        @brief Gets the name of the recipe.
        """
    @classmethod
    def new(cls, name: str, description: Optional[str] = ...) -> Recipe:
        """
        new(name: str, description: Optional[str] = None) -> Recipe
        @brief Creates a new recipe object with the given name and (optional) description
        """

class RelativeProgress(Progress):
    """
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


    __gsi_id__: ClassVar[int] = ...
    format: None
    """
    format(format: str) -> None
    @brief sets the output format (sprintf notation) for the progress text
    """
    value: None
    """
    value(value: int) -> None
    @brief Sets the progress value
    """
    @overload
    def __init__(self, desc: str, max_value: int) -> RelativeProgress:
        """
        __init__(desc: str, max_value: int) -> RelativeProgress
        @brief Creates a relative progress reporter with the given description and maximum value
        
        The reported progress will be 0 to 100% for values between 0 and the maximum value.
        The values are always integers. Double values cannot be used property.
        """
    @overload
    def __init__(self, desc: str, max_value: int, yield_interval: int) -> RelativeProgress:
        """
        __init__(desc: str, max_value: int, yield_interval: int) -> RelativeProgress
        @brief Creates a relative progress reporter with the given description and maximum value
        
        The reported progress will be 0 to 100% for values between 0 and the maximum value.
        The values are always integers. Double values cannot be used property.
        
        The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, the event loop will be executed every tenth call of \inc or \set.
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def inc(self) -> RelativeProgress:
        """
        inc() -> RelativeProgress
        @brief Increments the progress value
        """
    @classmethod
    @overload
    def new(cls, desc: str, max_value: int) -> RelativeProgress:
        """
        new(desc: str, max_value: int) -> RelativeProgress
        @brief Creates a relative progress reporter with the given description and maximum value
        
        The reported progress will be 0 to 100% for values between 0 and the maximum value.
        The values are always integers. Double values cannot be used property.
        """
    @overload
    def new(cls, desc: str, max_value: int, yield_interval: int) -> RelativeProgress:
        """
        new(desc: str, max_value: int, yield_interval: int) -> RelativeProgress
        @brief Creates a relative progress reporter with the given description and maximum value
        
        The reported progress will be 0 to 100% for values between 0 and the maximum value.
        The values are always integers. Double values cannot be used property.
        
        The yield interval specifies, how often the event loop will be triggered. When the yield interval is 10 for example, the event loop will be executed every tenth call of \inc or \set.
        """
    def set(self, value: int, force_yield: bool) -> None:
        """
        set(value: int, force_yield: bool) -> None
        @brief Sets the progress value
        
        This method is equivalent to \value=, but it allows forcing the event loop to be triggered.
        If "force_yield" is true, the event loop will be triggered always, irregardless of the yield interval specified in the constructor.
        """

class Timer:
    """
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


    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> Timer:
        """
        __init__() -> Timer
        @brief Creates a new object of this class
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: Timer) -> None:
        """
        assign(other: Timer) -> None
        @brief Assigns another object to self
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> Timer:
        """
        dup() -> Timer
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    @classmethod
    def memory_size(cls) -> int:
        """
        memory_size() -> int
        @brief Gets the current memory usage of the process in Bytes
        
        This method has been introduced in version 0.27.
        """
    @classmethod
    def new(cls) -> Timer:
        """
        new() -> Timer
        @brief Creates a new object of this class
        """
    def start(self) -> None:
        """
        start() -> None
        @brief Starts the timer
        """
    def stop(self) -> None:
        """
        stop() -> None
        @brief Stops the timer
        """
    def sys(self) -> float:
        """
        sys() -> float
        @brief Returns the elapsed CPU time in kernel mode from start to stop in seconds
        """
    def to_s(self) -> str:
        """
        to_s() -> str
        @brief Produces a string with the currently elapsed times
        """
    def user(self) -> float:
        """
        user() -> float
        @brief Returns the elapsed CPU time in user mode from start to stop in seconds
        """
    def wall(self) -> float:
        """
        wall() -> float
        @brief Returns the elapsed real time from start to stop in seconds
        This method has been introduced in version 0.26.
        """

class Value:
    """
    @brief Encapsulates a value (preferably a plain data type) in an object
    This class is provided to 'box' a value (encapsulate the value in an object). This class is required to interface to pointer or reference types in a method call. By using that class, the method can alter the value and thus implement 'out parameter' semantics. The value may be 'nil' which acts as a null pointer in pointer type arguments.
    This class has been introduced in version 0.22.
    """


    __gsi_id__: ClassVar[int] = ...
    value: Any
    """
    value() -> Any
    @brief Gets the actual value.
    
    ------
    value(value: Any) -> None
    @brief Set the actual value.
    """
    @overload
    def __init__(self) -> Value:
        """
        __init__() -> Value
        @brief Constructs a nil object.
        """
    @overload
    def __init__(self, value: Any) -> Value:
        """
        __init__(value: Any) -> Value
        @brief Constructs a non-nil object with the given value.
        This constructor has been introduced in version 0.22.
        """
    def _create(self) -> None:
        """
        _create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def _destroy(self) -> None:
        """
        _destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def _destroyed(self) -> bool:
        """
        _destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def _is_const_object(self) -> bool:
        """
        _is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def _manage(self) -> None:
        """
        _manage() -> None
        @brief Marks the object as managed by the script side.
        After calling this method on an object, the script side will be responsible for the management of the object. This method may be called if an object is returned from a C++ function and the object is known not to be owned by any C++ instance. If necessary, the script side may delete the object if the script's reference is no longer required.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def _unmanage(self) -> None:
        """
        _unmanage() -> None
        @brief Marks the object as no longer owned by the script side.
        Calling this method will make this object no longer owned by the script's memory management. Instead, the object must be managed in some other way. Usually this method may be called if it is known that some C++ object holds and manages this object. Technically speaking, this method will turn the script's reference into a weak reference. After the script engine decides to delete the reference, the object itself will still exist. If the object is not managed otherwise, memory leaks will occur.
        
        Usually it's not required to call this method. It has been introduced in version 0.24.
        """
    def assign(self, other: Value) -> None:
        """
        assign(other: Value) -> None
        @brief Assigns another object to self
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def destroy(self) -> None:
        """
        destroy() -> None
        @brief Explicitly destroys the object
        Explicitly destroys the object on C++ side if it was owned by the script interpreter. Subsequent access to this object will throw an exception.
        If the object is not owned by the script, this method will do nothing.
        """
    def destroyed(self) -> bool:
        """
        destroyed() -> bool
        @brief Returns a value indicating whether the object was already destroyed
        This method returns true, if the object was destroyed, either explicitly or by the C++ side.
        The latter may happen, if the object is owned by a C++ object which got destroyed itself.
        """
    def dup(self) -> Value:
        """
        dup() -> Value
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    @classmethod
    @overload
    def new(cls) -> Value:
        """
        new() -> Value
        @brief Constructs a nil object.
        """
    @overload
    def new(cls, value: Any) -> Value:
        """
        new(value: Any) -> Value
        @brief Constructs a non-nil object with the given value.
        This constructor has been introduced in version 0.22.
        """
    def to_s(self) -> str:
        """
        to_s() -> str
        @brief Convert this object to a string
        """

class _AmbiguousMethodDispatcher:
    def __delattr__(self, name) -> Any:
        """
        Implement delattr(self, name).
        """
    def __delete__(self, *args, **kwargs) -> Any:
        """
        Delete an attribute of instance.
        """
    def __get__(self, instance, owner) -> Any:
        """
        Return an attribute of instance, which is of type owner.
        """
    def __set__(self, instance, value) -> Any:
        """
        Set an attribute of instance to value.
        """
    def __setattr__(self, name, value) -> Any:
        """
        Implement setattr(self, name, value).
        """

class _Iterator:
    def __iter__(self) -> Any:
        """
        Implement iter(self).
        """
    def __next__(self) -> Any:
        """
        Implement next(self).
        """

class _Signal:
    def add(self, *args, **kwargs) -> Any:
        """
        internal signal proxy object: += operator
        """
    def clear(self, *args, **kwargs) -> Any:
        """
        internal signal proxy object: clears all receivers
        """
    def remove(self, *args, **kwargs) -> Any:
        """
        internal signal proxy object: -= operator
        """
    def set(self, *args, **kwargs) -> Any:
        """
        internal signal proxy object: assignment
        """
    def __call__(self, *args, **kwargs) -> Any:
        """
        Call self as a function.
        """
    def __iadd__(self, other) -> Any:
        """
        Return self+=value.
        """
    def __isub__(self, other) -> Any:
        """
        Return self-=value.
        """

class _StaticAttribute:
    def __init__(self, *args, **kwargs) -> None:
        """
        Initialize self.  See help(type(self)) for accurate signature.
        """
    def __delattr__(self, name) -> Any:
        """
        Implement delattr(self, name).
        """
    def __delete__(self, *args, **kwargs) -> Any:
        """
        Delete an attribute of instance.
        """
    def __get__(self, instance, owner) -> Any:
        """
        Return an attribute of instance, which is of type owner.
        """
    def __set__(self, instance, value) -> Any:
        """
        Set an attribute of instance to value.
        """
    def __setattr__(self, name, value) -> Any:
        """
        Implement setattr(self, name, value).
        """
