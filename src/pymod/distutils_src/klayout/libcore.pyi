from typing import Any

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
