from typing import Any, ClassVar, Dict, Sequence, List, Iterator, Optional
from typing import overload
import klayout.tl as tl
import klayout.db as db
import klayout.rdb as rdb
class AbstractMenu:
    r"""
    @brief An abstraction for the application menus

    The abstract menu is a class that stores a main menu and several popup menus
    in a generic form such that they can be manipulated and converted into GUI objects.

    Each item can be associated with a Action, which delivers a title, enabled/disable state etc.
    The Action is either provided when new entries are inserted or created upon initialisation.

    The abstract menu class provides methods to manipulate the menu structure (the state of the
    menu items, their title and shortcut key is provided and manipulated through the Action object). 

    Menu items and submenus are referred to by a "path". The path is a string with this interpretation:

    @<table>
      @<tr>@<td>""                 @</td>@<td>is the root@</td>@</tr> 
      @<tr>@<td>"[<path>.]<name>"  @</td>@<td>is an element of the submenu given by <path>. If <path> is omitted, this refers to an element in the root@</td>@</tr> 
      @<tr>@<td>"[<path>.]end"     @</td>@<td>refers to the item past the last item of the submenu given by <path> or root@</td>@</tr>
      @<tr>@<td>"[<path>.]begin"   @</td>@<td>refers to the first item of the submenu given by <path> or root@</td>@</tr>
      @<tr>@<td>"[<path>.]#<n>"    @</td>@<td>refers to the nth item of the submenu given by <path> or root (n is an integer number)@</td>@</tr>
    @</table>

    Menu items can be put into groups. The path strings of each group can be obtained with the 
    "group" method. An item is put into a group by appending ":<group-name>" to the item's name.
    This specification can be used several times.

    Detached menus (i.e. for use in context menus) can be created as virtual top-level submenus
    with a name of the form "@@<name>". A special detached menu is "@toolbar" which represents the tool bar of the main window. 
    Menus are closely related to the \Action class. Actions are used to represent selectable items inside menus, provide the title and other configuration settings. Actions also link the menu items with code. See the \Action class description for further details.
    """
    @classmethod
    def new(cls) -> AbstractMenu:
        r"""
        @hide
        """
    @classmethod
    def pack_key_binding(cls, path_to_keys: Dict[str, str]) -> str:
        r"""
        @brief Serializes a key binding definition into a single string
        The serialized format is used by the 'key-bindings' config key. This method will take an array of path/key definitions (including the \Action#NoKeyBound option) and convert it to a single string suitable for assigning to the config key.

        This method has been introduced in version 0.26.
        """
    @classmethod
    def pack_menu_items_hidden(cls, path_to_visibility: Dict[str, bool]) -> str:
        r"""
        @brief Serializes a menu item visibility definition into a single string
        The serialized format is used by the 'menu-items-hidden' config key. This method will take an array of path/visibility flag definitions and convert it to a single string suitable for assigning to the config key.

        This method has been introduced in version 0.26.
        """
    @classmethod
    def unpack_key_binding(cls, s: str) -> Dict[str, str]:
        r"""
        @brief Deserializes a key binding definition
        This method is the reverse of \pack_key_binding.

        This method has been introduced in version 0.26.
        """
    @classmethod
    def unpack_menu_items_hidden(cls, s: str) -> Dict[str, bool]:
        r"""
        @brief Deserializes a menu item visibility definition
        This method is the reverse of \pack_menu_items_hidden.

        This method has been introduced in version 0.26.
        """
    def __copy__(self) -> AbstractMenu:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> AbstractMenu:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @hide
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
    def action(self, path: str) -> ActionBase:
        r"""
        @brief Gets the reference to a Action object associated with the given path

        @param path The path to the item.
        @return A reference to a Action object associated with this path or nil if the path is not valid
        """
    def assign(self, other: AbstractMenu) -> None:
        r"""
        @brief Assigns another object to self
        """
    def clear_menu(self, path: str) -> None:
        r"""
        @brief Deletes the children of the item given by the path

        @param path The path to the item whose children to delete

        This method has been introduced in version 0.28.
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def delete_item(self, path: str) -> None:
        r"""
        @brief Deletes the item given by the path

        @param path The path to the item to delete

        This method will also delete all children of the given item. To clear the children only, use \clear_menu.
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
    def dup(self) -> AbstractMenu:
        r"""
        @brief Creates a copy of self
        """
    def group(self, group: str) -> List[str]:
        r"""
        @brief Gets the group members

        @param group The group name
        @param A vector of all members (by path) of the group
        """
    def insert_item(self, path: str, name: str, action: ActionBase) -> None:
        r"""
        @brief Inserts a new item before the one given by the path

        The Action object passed as the third parameter references the handler which both implements the action to perform and the menu item's appearance such as title, icon and keyboard shortcut.

        @param path The path to the item before which to insert the new item
        @param name The name of the item to insert 
        @param action The Action object to insert
        """
    @overload
    def insert_menu(self, path: str, name: str, action: ActionBase) -> None:
        r"""
        @brief Inserts a new submenu before the item given by the path

        @param path The path to the item before which to insert the submenu
        @param name The name of the submenu to insert 
        @param action The action object of the submenu to insert

        This method variant has been added in version 0.28.
        """
    @overload
    def insert_menu(self, path: str, name: str, title: str) -> None:
        r"""
        @brief Inserts a new submenu before the item given by the path

        The title string optionally encodes the key shortcut and icon resource
        in the form <text>["("<shortcut>")"]["<"<icon-resource>">"].

        @param path The path to the item before which to insert the submenu
        @param name The name of the submenu to insert 
        @param title The title of the submenu to insert
        """
    def insert_separator(self, path: str, name: str) -> None:
        r"""
        @brief Inserts a new separator before the item given by the path

        @param path The path to the item before which to insert the separator
        @param name The name of the separator to insert 
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_menu(self, path: str) -> bool:
        r"""
        @brief Returns true if the item is a menu

        @param path The path to the item
        @return false if the path is not valid or is not a menu
        """
    def is_separator(self, path: str) -> bool:
        r"""
        @brief Returns true if the item is a separator

        @param path The path to the item
        @return false if the path is not valid or is not a separator

        This method has been introduced in version 0.19.
        """
    def is_valid(self, path: str) -> bool:
        r"""
        @brief Returns true if the path is a valid one

        @param path The path to check
        @return false if the path is not a valid path to an item
        """
    def items(self, path: str) -> List[str]:
        r"""
        @brief Gets the subitems for a given submenu

        @param path The path to the submenu
        @return A vector or path strings for the child items or an empty vector if the path is not valid or the item does not have children
        """

class Action(ActionBase):
    r"""
    @brief The abstraction for an action (i.e. used inside menus)

    Actions act as a generalization of menu entries. The action provides the appearance of a menu entry such as title, key shortcut etc. and dispatches the menu events. The action can be manipulated to change to appearance of a menu entry and can be attached an observer that receives the events when the menu item is selected.

    Multiple action objects can refer to the same action internally, in which case the information and event handler is copied between the incarnations. This way, a single implementation can be provided for multiple places where an action appears, for example inside the toolbar and in addition as a menu entry. Both actions will shared the same icon, text, shortcut etc.

    Actions are mainly used for providing new menu items inside the \AbstractMenu class. This is some sample Ruby code for that case:

    @code
    a = RBA::Action.new
    a.title = "Push Me!"
    a.on_triggered do 
      puts "I was pushed!"
    end

    app = RBA::Application.instance
    mw = app.main_window

    menu = mw.menu
    menu.insert_separator("@toolbar.end", "name")
    menu.insert_item("@toolbar.end", "my_action", a)
    @/code

    This code will register a custom action in the toolbar. When the toolbar button is pushed a message is printed. The toolbar is addressed by a path starting with the pseudo root "@toolbar".

    In Version 0.23, the Action class has been merged with the ActionBase class.
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

class ActionBase:
    r"""
    @hide
    @alias Action
    """
    NoKeyBound: ClassVar[str]
    r"""
    @brief Gets a shortcut value indicating that no shortcut shall be assigned
    This method has been introduced in version 0.26.
    """
    checkable: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the item is checkable

    Setter:
    @brief Makes the item(s) checkable or not

    @param checkable true to make the item checkable
    """
    checked: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the item is checked

    Setter:
    @brief Checks or unchecks the item

    @param checked true to make the item checked
    """
    @property
    def icon(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the icon to the given image file

        @param file The image file to load for the icon

        Passing an empty string will reset the icon.
        """
    default_shortcut: str
    r"""
    Getter:
    @brief Gets the default keyboard shortcut
    @return The default keyboard shortcut as a string

    This attribute has been introduced in version 0.25.

    Setter:
    @brief Sets the default keyboard shortcut

    The default shortcut is used, if \shortcut is empty.

    This attribute has been introduced in version 0.25.
    """
    enabled: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the item is enabled

    Setter:
    @brief Enables or disables the action

    @param enabled true to enable the item
    """
    hidden: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the item is hidden
    If an item is hidden, it's always hidden and \is_visible? does not have an effect.
    This attribute has been introduced in version 0.25.

    Setter:
    @brief Sets a value that makes the item hidden always
    See \is_hidden? for details.

    This attribute has been introduced in version 0.25
    """
    icon_text: str
    r"""
    Getter:
    @brief Gets the icon's text

    Setter:
    @brief Sets the icon's text

    If an icon text is set, this will be used for the text below the icon.
    If no icon text is set, the normal text will be used for the icon.
    Passing an empty string will reset the icon's text.
    """
    on_menu_opening: None
    r"""
    Getter:
    @brief This event is called if the menu item is a sub-menu and before the menu is opened.

    This event provides an opportunity to populate the menu before it is opened.

    This event has been introduced in version 0.28.

    Setter:
    @brief This event is called if the menu item is a sub-menu and before the menu is opened.

    This event provides an opportunity to populate the menu before it is opened.

    This event has been introduced in version 0.28.
    """
    on_triggered: None
    r"""
    Getter:
    @brief This event is called if the menu item is selected.

    This event has been introduced in version 0.21 and moved to the ActionBase class in 0.28.

    Setter:
    @brief This event is called if the menu item is selected.

    This event has been introduced in version 0.21 and moved to the ActionBase class in 0.28.
    """
    separator: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the item is a separator
    This method has been introduced in version 0.25.

    Setter:
    @brief Makes an item a separator or not

    @param separator true to make the item a separator
    This method has been introduced in version 0.25.
    """
    shortcut: str
    r"""
    Getter:
    @brief Gets the keyboard shortcut
    @return The keyboard shortcut as a string

    Setter:
    @brief Sets the keyboard shortcut
    If the shortcut string is empty, the default shortcut will be used. If the string is equal to \Action#NoKeyBound, no keyboard shortcut will be assigned.

    @param shortcut The keyboard shortcut in Qt notation (i.e. "Ctrl+C")

    The NoKeyBound option has been added in version 0.26.
    """
    title: str
    r"""
    Getter:
    @brief Gets the title

    @return The current title string

    Setter:
    @brief Sets the title

    @param title The title string to set (just the title)
    """
    tool_tip: str
    r"""
    Getter:
    @brief Gets the tool tip text.

    This method has been added in version 0.22.

    Setter:
    @brief Sets the tool tip text

    The tool tip text is displayed in the tool tip window of the menu entry.
    This is in particular useful for entries in the tool bar.
    This method has been added in version 0.22.
    """
    visible: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the item is visible
    The visibility combines with \is_hidden?. To get the true visiblity, use \is_effective_visible?.
    Setter:
    @brief Sets the item's visibility

    @param visible true to make the item visible
    """
    @classmethod
    def new(cls) -> ActionBase:
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
    def effective_shortcut(self) -> str:
        r"""
        @brief Gets the effective keyboard shortcut
        @return The effective keyboard shortcut as a string

        The effective shortcut is the one that is taken. It's either \shortcut or \default_shortcut.

        This attribute has been introduced in version 0.25.
        """
    def is_checkable(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item is checkable
        """
    def is_checked(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item is checked
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_effective_enabled(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item is really enabled
        This is the combined value from \is_enabled? and dynamic value (\wants_enabled).
        This attribute has been introduced in version 0.28.
        """
    def is_effective_visible(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item is really visible
        This is the combined visibility from \is_visible? and \is_hidden? and dynamic visibility (\wants_visible).
        This attribute has been introduced in version 0.25.
        """
    def is_enabled(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item is enabled
        """
    def is_hidden(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item is hidden
        If an item is hidden, it's always hidden and \is_visible? does not have an effect.
        This attribute has been introduced in version 0.25.
        """
    def is_separator(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item is a separator
        This method has been introduced in version 0.25.
        """
    def is_visible(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item is visible
        The visibility combines with \is_hidden?. To get the true visiblity, use \is_effective_visible?.
        """
    def trigger(self) -> None:
        r"""
        @brief Triggers the action programmatically
        """

class Annotation(BasicAnnotation):
    r"""
    @brief A layout annotation (i.e. ruler)

    Annotation objects provide a way to attach measurements or descriptive information to a layout view. Annotation objects can appear as rulers for example. Annotation objects can be configured in different ways using the styles provided. By configuring an annotation object properly, it can appear as a rectangle or a plain line for example.
    See @<a href="/manual/ruler_properties.xml">Ruler properties@</a> for more details about the appearance options.

    Annotations are inserted into a layout view using \LayoutView#insert_annotation. Here is some sample code in Ruby:

    @code
    app = RBA::Application.instance
    mw = app.main_window
    view = mw.current_view

    ant = RBA::Annotation::new
    ant.p1 = RBA::DPoint::new(0, 0)
    ant.p2 = RBA::DPoint::new(100, 0)
    ant.style = RBA::Annotation::StyleRuler
    view.insert_annotation(ant)
    @/code

    Annotations can be retrieved from a view with \LayoutView#each_annotation and all annotations can be cleared with \LayoutView#clear_annotations.

    Starting with version 0.25, annotations are 'live' objects once they are inserted into the view. Changing properties of annotations will automatically update the view (however, that is not true the other way round).

    Here is some sample code of changing the style of all rulers to two-sided arrows:

    @code
    view = RBA::LayoutView::current

    begin

      view.transaction("Restyle annotations")

      view.each_annotation do |a|
        a.style = RBA::Annotation::StyleArrowBoth
      end
  
    ensure
      view.commit
    end
    @/code
    """
    AlignAuto: ClassVar[int]
    r"""
    @brief This code indicates automatic alignment.
    This code makes the annotation align the label the way it thinks is best.

    This constant has been introduced in version 0.25.
    """
    AlignBottom: ClassVar[int]
    r"""
    @brief This code indicates bottom alignment.
    If used in a vertical context, this alignment code makes the label aligned at the bottom side - i.e. it will appear top of the reference point.

    This constant has been introduced in version 0.25.
    """
    AlignCenter: ClassVar[int]
    r"""
    @brief This code indicates automatic alignment.
    This code makes the annotation align the label centered. When used in a horizontal context, centering is in horizontal direction. If used in a vertical context, centering is in vertical direction.

    This constant has been introduced in version 0.25.
    """
    AlignDown: ClassVar[int]
    r"""
    @brief This code indicates left or bottom alignment, depending on the context.
    This code is equivalent to \AlignLeft and \AlignBottom.

    This constant has been introduced in version 0.25.
    """
    AlignLeft: ClassVar[int]
    r"""
    @brief This code indicates left alignment.
    If used in a horizontal context, this alignment code makes the label aligned at the left side - i.e. it will appear right of the reference point.

    This constant has been introduced in version 0.25.
    """
    AlignRight: ClassVar[int]
    r"""
    @brief This code indicates right alignment.
    If used in a horizontal context, this alignment code makes the label aligned at the right side - i.e. it will appear left of the reference point.

    This constant has been introduced in version 0.25.
    """
    AlignTop: ClassVar[int]
    r"""
    @brief This code indicates top alignment.
    If used in a vertical context, this alignment code makes the label aligned at the top side - i.e. it will appear bottom of the reference point.

    This constant has been introduced in version 0.25.
    """
    AlignUp: ClassVar[int]
    r"""
    @brief This code indicates right or top alignment, depending on the context.
    This code is equivalent to \AlignRight and \AlignTop.

    This constant has been introduced in version 0.25.
    """
    AngleAny: ClassVar[int]
    r"""
    @brief Gets the any angle code for use with the \angle_constraint method
    If this value is specified for the angle constraint, all angles will be allowed.
    """
    AngleDiagonal: ClassVar[int]
    r"""
    @brief Gets the diagonal angle code for use with the \angle_constraint method
    If this value is specified for the angle constraint, only multiples of 45 degree are allowed.
    """
    AngleGlobal: ClassVar[int]
    r"""
    @brief Gets the global angle code for use with the \angle_constraint method.
    This code will tell the ruler or marker to use the angle constraint defined globally.
    """
    AngleHorizontal: ClassVar[int]
    r"""
    @brief Gets the horizontal angle code for use with the \angle_constraint method
    If this value is specified for the angle constraint, only horizontal rulers are allowed.
    """
    AngleOrtho: ClassVar[int]
    r"""
    @brief Gets the ortho angle code for use with the \angle_constraint method
    If this value is specified for the angle constraint, only multiples of 90 degree are allowed.
    """
    AngleVertical: ClassVar[int]
    r"""
    @brief Gets the vertical angle code for use with the \angle_constraint method
    If this value is specified for the angle constraint, only vertical rulers are allowed.
    """
    OutlineAngle: ClassVar[int]
    r"""
    @brief Gets the angle measurement ruler outline code for use with the \outline method
    When this outline style is specified, the ruler is drawn to indicate the angle between the first and last segment.

    This constant has been introduced in version 0.28.
    """
    OutlineBox: ClassVar[int]
    r"""
    @brief Gets the box outline code for use with the \outline method
    When this outline style is specified, a box is drawn with the corners specified by the start and end point. All box edges are drawn in the style specified with the \style attribute.
    """
    OutlineDiag: ClassVar[int]
    r"""
    @brief Gets the diagonal output code for use with the \outline method
    When this outline style is specified, a line connecting start and end points in the given style (ruler, arrow or plain line) is drawn.
    """
    OutlineDiagXY: ClassVar[int]
    r"""
    @brief Gets the xy plus diagonal outline code for use with the \outline method
    @brief outline_xy code used by the \outline method
    When this outline style is specified, three lines are drawn: one horizontal from left to right and attached to the end of that a line from the bottom to the top. Another line is drawn connecting the start and end points directly. The lines are drawn in the specified style (see \style method).
    """
    OutlineDiagYX: ClassVar[int]
    r"""
    @brief Gets the yx plus diagonal outline code for use with the \outline method
    When this outline style is specified, three lines are drawn: one vertical from bottom to top and attached to the end of that a line from the left to the right. Another line is drawn connecting the start and end points directly. The lines are drawn in the specified style (see \style method).
    """
    OutlineEllipse: ClassVar[int]
    r"""
    @brief Gets the ellipse outline code for use with the \outline method
    When this outline style is specified, an ellipse is drawn with the extensions specified by the start and end point. The contour drawn as a line.

    This constant has been introduced in version 0.26.
    """
    OutlineRadius: ClassVar[int]
    r"""
    @brief Gets the radius measurement ruler outline code for use with the \outline method
    When this outline style is specified, the ruler is drawn to indicate a radius defined by at least three points of the ruler.

    This constant has been introduced in version 0.28.
    """
    OutlineXY: ClassVar[int]
    r"""
    @brief Gets the xy outline code for use with the \outline method
    When this outline style is specified, two lines are drawn: one horizontal from left to right and attached to the end of that a line from the bottom to the top. The lines are drawn in the specified style (see \style method).
    """
    OutlineYX: ClassVar[int]
    r"""
    @brief Gets the yx outline code for use with the \outline method
    When this outline style is specified, two lines are drawn: one vertical from bottom to top and attached to the end of that a line from the left to the right. The lines are drawn in the specified style (see \style method).
    """
    PositionAuto: ClassVar[int]
    r"""
    @brief This code indicates automatic positioning.
    The main label will be put either to p1 or p2, whichever the annotation considers best.

    This constant has been introduced in version 0.25.
    """
    PositionCenter: ClassVar[int]
    r"""
    @brief This code indicates positioning of the main label at the mid point between p1 and p2.
    The main label will be put to the center point.

    This constant has been introduced in version 0.25.
    """
    PositionP1: ClassVar[int]
    r"""
    @brief This code indicates positioning of the main label at p1.
    The main label will be put to p1.

    This constant has been introduced in version 0.25.
    """
    PositionP2: ClassVar[int]
    r"""
    @brief This code indicates positioning of the main label at p2.
    The main label will be put to p2.

    This constant has been introduced in version 0.25.
    """
    RulerModeAutoMetric: ClassVar[int]
    r"""
    @brief Specifies auto-metric ruler mode for the \register_template method
    In auto-metric mode, a ruler can be placed with a single click and p1/p2 will be determined from the neighborhood.

    This constant has been introduced in version 0.25
    """
    RulerModeNormal: ClassVar[int]
    r"""
    @brief Specifies normal ruler mode for the \register_template method

    This constant has been introduced in version 0.25
    """
    RulerModeSingleClick: ClassVar[int]
    r"""
    @brief Specifies single-click ruler mode for the \register_template method
    In single click-mode, a ruler can be placed with a single click and p1 will be == p2.

    This constant has been introduced in version 0.25
    """
    RulerMultiSegment: ClassVar[int]
    r"""
    @brief Specifies multi-segment mode
    In multi-segment mode, multiple segments can be created. The ruler is finished with a double click.

    This constant has been introduced in version 0.28
    """
    RulerThreeClicks: ClassVar[int]
    r"""
    @brief Specifies three-click ruler mode for the \register_template method
    In this ruler mode, two segments are created for angle and circle radius measurements. Three mouse clicks are required.

    This constant has been introduced in version 0.28
    """
    StyleArrowBoth: ClassVar[int]
    r"""
    @brief Gets the both arrow ends style code for use the \style method
    When this style is specified, a two-headed arrow is drawn.
    """
    StyleArrowEnd: ClassVar[int]
    r"""
    @brief Gets the end arrow style code for use the \style method
    When this style is specified, an arrow is drawn pointing from the start to the end point.
    """
    StyleArrowStart: ClassVar[int]
    r"""
    @brief Gets the start arrow style code for use the \style method
    When this style is specified, an arrow is drawn pointing from the end to the start point.
    """
    StyleCrossBoth: ClassVar[int]
    r"""
    @brief Gets the line style code for use with the \style method
    When this style is specified, a cross is drawn at both points.

    This constant has been added in version 0.26.
    """
    StyleCrossEnd: ClassVar[int]
    r"""
    @brief Gets the line style code for use with the \style method
    When this style is specified, a cross is drawn at the end point.

    This constant has been added in version 0.26.
    """
    StyleCrossStart: ClassVar[int]
    r"""
    @brief Gets the line style code for use with the \style method
    When this style is specified, a cross is drawn at the start point.

    This constant has been added in version 0.26.
    """
    StyleLine: ClassVar[int]
    r"""
    @brief Gets the line style code for use with the \style method
    When this style is specified, a plain line is drawn.
    """
    StyleRuler: ClassVar[int]
    r"""
    @brief Gets the ruler style code for use the \style method
    When this style is specified, the annotation will show a ruler with some ticks at distances indicating a decade of units and a suitable subdivision into minor ticks at intervals of 1, 2 or 5 units.
    """
    angle_constraint: int
    r"""
    Getter:
    @brief Returns the angle constraint attribute
    See \angle_constraint= for a more detailed description.
    Setter:
    @brief Sets the angle constraint attribute
    This attribute controls if an angle constraint is applied when moving one of the ruler's points. The Angle... values can be used for this purpose.
    """
    category: str
    r"""
    Getter:
    @brief Gets the category string
    See \category= for details.

    This method has been introduced in version 0.25
    Setter:
    @brief Sets the category string of the annotation
    The category string is an arbitrary string that can be used by various consumers or generators to mark 'their' annotation.

    This method has been introduced in version 0.25
    """
    fmt: str
    r"""
    Getter:
    @brief Returns the format used for the label
    @return The format string
    Format strings can contain placeholders for values and formulas for computing derived values. See @<a href="/manual/ruler_properties.xml">Ruler properties@</a> for more details.
    Setter:
    @brief Sets the format used for the label
    @param format The format string
    Format strings can contain placeholders for values and formulas for computing derived values. See @<a href="/manual/ruler_properties.xml">Ruler properties@</a> for more details.
    """
    fmt_x: str
    r"""
    Getter:
    @brief Returns the format used for the x-axis label
    @return The format string
    Format strings can contain placeholders for values and formulas for computing derived values. See @<a href="/manual/ruler_properties.xml">Ruler properties@</a> for more details.
    Setter:
    @brief Sets the format used for the x-axis label
    X-axis labels are only used for styles that have a horizontal component. @param format The format string
    Format strings can contain placeholders for values and formulas for computing derived values. See @<a href="/manual/ruler_properties.xml">Ruler properties@</a> for more details.
    """
    fmt_y: str
    r"""
    Getter:
    @brief Returns the format used for the y-axis label
    @return The format string
    Format strings can contain placeholders for values and formulas for computing derived values. See @<a href="/manual/ruler_properties.xml">Ruler properties@</a> for more details.
    Setter:
    @brief Sets the format used for the y-axis label
    Y-axis labels are only used for styles that have a vertical component. @param format The format string
    Format strings can contain placeholders for values and formulas for computing derived values. See @<a href="/manual/ruler_properties.xml">Ruler properties@</a> for more details.
    """
    main_position: int
    r"""
    Getter:
    @brief Gets the position of the main label
    See \main_position= for details.

    This method has been introduced in version 0.25
    Setter:
    @brief Sets the position of the main label
    This method accepts one of the Position... constants.

    This method has been introduced in version 0.25
    """
    main_xalign: int
    r"""
    Getter:
    @brief Gets the horizontal alignment type of the main label
    See \main_xalign= for details.

    This method has been introduced in version 0.25
    Setter:
    @brief Sets the horizontal alignment type of the main label
    This method accepts one of the Align... constants.

    This method has been introduced in version 0.25
    """
    main_yalign: int
    r"""
    Getter:
    @brief Gets the vertical alignment type of the main label
    See \main_yalign= for details.

    This method has been introduced in version 0.25
    Setter:
    @brief Sets the vertical alignment type of the main label
    This method accepts one of the Align... constants.

    This method has been introduced in version 0.25
    """
    outline: int
    r"""
    Getter:
    @brief Returns the outline style of the annotation object

    Setter:
    @brief Sets the outline style used for drawing the annotation object
    The Outline... values can be used for defining the annotation object's outline. The outline style determines what components are drawn. 
    """
    p1: db.DPoint
    r"""
    Getter:
    @brief Gets the first point of the ruler or marker
    The points of the ruler or marker are always given in micron units in floating-point coordinates.

    This method is provided for backward compatibility. Starting with version 0.28, rulers can be multi-segmented. Use \points or \seg_p1 to retrieve the points of the ruler segments.

    @return The first point

    Setter:
    @brief Sets the first point of the ruler or marker
    The points of the ruler or marker are always given in micron units in floating-point coordinates.

    This method is provided for backward compatibility. Starting with version 0.28, rulers can be multi-segmented. Use \points= to specify the ruler segments.
    """
    p2: db.DPoint
    r"""
    Getter:
    @brief Gets the second point of the ruler or marker
    The points of the ruler or marker are always given in micron units in floating-point coordinates.

    This method is provided for backward compatibility. Starting with version 0.28, rulers can be multi-segmented. Use \points or \seg_p1 to retrieve the points of the ruler segments.

    @return The second point

    Setter:
    @brief Sets the second point of the ruler or marker
    The points of the ruler or marker are always given in micron units in floating-point coordinates.

    This method is provided for backward compatibility. Starting with version 0.28, rulers can be multi-segmented. Use \points= to specify the ruler segments.
    """
    points: List[db.DPoint]
    r"""
    Getter:
    @brief Gets the points of the ruler
    A single-segmented ruler has two points. Rulers with more points have more segments correspondingly. Note that the point list may have one point only (single-point ruler) or may even be empty.

    Use \points= to set the segment points. Use \segments to get the number of segments and \seg_p1 and \seg_p2 to get the first and second point of one segment.

    Multi-segmented rulers have been introduced in version 0.28
    Setter:
    @brief Sets the points for a (potentially) multi-segmented ruler
    See \points for a description of multi-segmented rulers. The list of points passed to this method is cleaned from duplicates before being stored inside the ruler.

    This method has been introduced in version 0.28.
    """
    snap: bool
    r"""
    Getter:
    @brief Returns the 'snap to objects' attribute

    Setter:
    @brief Sets the 'snap to objects' attribute
    If this attribute is set to true, the ruler or marker snaps to other objects when moved. 
    """
    style: int
    r"""
    Getter:
    @brief Returns the style of the annotation object

    Setter:
    @brief Sets the style used for drawing the annotation object
    The Style... values can be used for defining the annotation object's style. The style determines if ticks or arrows are drawn.
    """
    xlabel_xalign: int
    r"""
    Getter:
    @brief Gets the horizontal alignment type of the x axis label
    See \xlabel_xalign= for details.

    This method has been introduced in version 0.25
    Setter:
    @brief Sets the horizontal alignment type of the x axis label
    This method accepts one of the Align... constants.

    This method has been introduced in version 0.25
    """
    xlabel_yalign: int
    r"""
    Getter:
    @brief Gets the vertical alignment type of the x axis label
    See \xlabel_yalign= for details.

    This method has been introduced in version 0.25
    Setter:
    @brief Sets the vertical alignment type of the x axis label
    This method accepts one of the Align... constants.

    This method has been introduced in version 0.25
    """
    ylabel_xalign: int
    r"""
    Getter:
    @brief Gets the horizontal alignment type of the y axis label
    See \ylabel_xalign= for details.

    This method has been introduced in version 0.25
    Setter:
    @brief Sets the horizontal alignment type of the y axis label
    This method accepts one of the Align... constants.

    This method has been introduced in version 0.25
    """
    ylabel_yalign: int
    r"""
    Getter:
    @brief Gets the vertical alignment type of the y axis label
    See \ylabel_yalign= for details.

    This method has been introduced in version 0.25
    Setter:
    @brief Sets the vertical alignment type of the y axis label
    This method accepts one of the Align... constants.

    This method has been introduced in version 0.25
    """
    @classmethod
    def from_s(cls, s: str) -> Annotation:
        r"""
        @brief Creates a ruler from a string representation
        This function creates a ruler from the string returned by \to_s.

        This method was introduced in version 0.28.
        """
    @classmethod
    def register_template(cls, annotation: BasicAnnotation, title: str, mode: Optional[int] = ...) -> None:
        r"""
        @brief Registers the given annotation as a template globally
        @annotation The annotation to use for the template (positions are ignored)
        @param title The title to use for the ruler template
        @param mode The mode the ruler will be created in (see Ruler... constants)

        In order to register a system template, the category string of the annotation has to be a unique and non-empty string. The annotation is added to the list of annotation templates and becomes available as a new template in the ruler drop-down menu.

        The new annotation template is registered on all views.

        NOTE: this setting is persisted and the the application configuration is updated.

        This method has been added in version 0.25.
        """
    @classmethod
    def unregister_templates(cls, category: str) -> None:
        r"""
        @brief Unregisters the template or templates with the given category string globally

        This method will remove all templates with the given category string. If the category string is empty, all templates are removed.

        NOTE: this setting is persisted and the the application configuration is updated.

        This method has been added in version 0.28.
        """
    def __eq__(self, other: object) -> bool:
        r"""
        @brief Equality operator
        """
    def __ne__(self, other: object) -> bool:
        r"""
        @brief Inequality operator
        """
    def __repr__(self) -> str:
        r"""
        @brief Returns the string representation of the ruler
        This method was introduced in version 0.19.
        """
    def __str__(self) -> str:
        r"""
        @brief Returns the string representation of the ruler
        This method was introduced in version 0.19.
        """
    def _assign(self, other: BasicAnnotation) -> None:
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
    def _dup(self) -> Annotation:
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
    def box(self) -> db.DBox:
        r"""
        @brief Gets the bounding box of the object (not including text)
        @return The bounding box
        """
    def delete(self) -> None:
        r"""
        @brief Deletes this annotation from the view
        If the annotation is an "active" one, this method will remove it from the view. This object will become detached and can still be manipulated, but without having an effect on the view.
        This method has been introduced in version 0.25.
        """
    def detach(self) -> None:
        r"""
        @brief Detaches the annotation object from the view
        If the annotation object was inserted into the view, property changes will be reflected in the view. To disable this feature, 'detach' can be called after which the annotation object becomes inactive and changes will no longer be reflected in the view.

        This method has been introduced in version 0.25.
        """
    def id(self) -> int:
        r"""
        @brief Returns the annotation's ID
        The annotation ID is an integer that uniquely identifies an annotation inside a view.
        The ID is used for replacing an annotation (see \LayoutView#replace_annotation).

        This method was introduced in version 0.24.
        """
    def is_valid(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object is a valid reference.
        If this value is true, the object represents an annotation on the screen. Otherwise, the object is a 'detached' annotation which does not have a representation on the screen.

        This method was introduced in version 0.25.
        """
    def seg_p1(self, segment_index: int) -> db.DPoint:
        r"""
        @brief Gets the first point of the given segment.
        The segment is indicated by the segment index which is a number between 0 and \segments-1.

        This method has been introduced in version 0.28.
        """
    def seg_p2(self, segment_index: int) -> db.DPoint:
        r"""
        @brief Gets the second point of the given segment.
        The segment is indicated by the segment index which is a number between 0 and \segments-1.
        The second point of a segment is also the first point of the following segment if there is one.

        This method has been introduced in version 0.28.
        """
    def segments(self) -> int:
        r"""
        @brief Gets the number of segments.
        This method returns the number of segments the ruler is made up. Even though the ruler can be one or even zero points, the number of segments is at least 1.

        This method has been introduced in version 0.28.
        """
    def text(self, index: Optional[int] = ...) -> str:
        r"""
        @brief Returns the formatted text for the main label
        The index parameter indicates which segment to use (0 is the first one). It has been added in version 0.28.
        """
    def text_x(self, index: Optional[int] = ...) -> str:
        r"""
        @brief Returns the formatted text for the x-axis label
        The index parameter indicates which segment to use (0 is the first one). It has been added in version 0.28.
        """
    def text_y(self, index: Optional[int] = ...) -> str:
        r"""
        @brief Returns the formatted text for the y-axis label
        The index parameter indicates which segment to use (0 is the first one). It has been added in version 0.28.
        """
    def to_s(self) -> str:
        r"""
        @brief Returns the string representation of the ruler
        This method was introduced in version 0.19.
        """
    @overload
    def transformed(self, t: db.DCplxTrans) -> Annotation:
        r"""
        @brief Transforms the ruler or marker with the given complex transformation
        @param t The magnifying transformation to apply
        @return The transformed object

        Starting with version 0.25, all overloads all available as 'transform'.
        """
    @overload
    def transformed(self, t: db.DTrans) -> Annotation:
        r"""
        @brief Transforms the ruler or marker with the given simple transformation
        @param t The transformation to apply
        @return The transformed object
        """
    @overload
    def transformed(self, t: db.ICplxTrans) -> Annotation:
        r"""
        @brief Transforms the ruler or marker with the given complex transformation
        @param t The magnifying transformation to apply
        @return The transformed object (in this case an integer coordinate object)

        This method has been introduced in version 0.18.

        Starting with version 0.25, all overloads all available as 'transform'.
        """
    @overload
    def transformed_cplx(self, t: db.DCplxTrans) -> Annotation:
        r"""
        @brief Transforms the ruler or marker with the given complex transformation
        @param t The magnifying transformation to apply
        @return The transformed object

        Starting with version 0.25, all overloads all available as 'transform'.
        """
    @overload
    def transformed_cplx(self, t: db.ICplxTrans) -> Annotation:
        r"""
        @brief Transforms the ruler or marker with the given complex transformation
        @param t The magnifying transformation to apply
        @return The transformed object (in this case an integer coordinate object)

        This method has been introduced in version 0.18.

        Starting with version 0.25, all overloads all available as 'transform'.
        """

class BasicAnnotation:
    r"""
    @hide
    @alias Annotation
    """
    @classmethod
    def new(cls) -> BasicAnnotation:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> BasicAnnotation:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> BasicAnnotation:
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
    def assign(self, other: BasicAnnotation) -> None:
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
    def dup(self) -> BasicAnnotation:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class BasicImage:
    r"""
    @hide
    @alias Image
    """
    @classmethod
    def new(cls) -> BasicImage:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> BasicImage:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> BasicImage:
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
    def assign(self, other: BasicImage) -> None:
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
    def dup(self) -> BasicImage:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class BitmapBuffer:
    r"""
    @brief A simplistic pixel buffer representing monochrome image

    This object is mainly provided for offline rendering of layouts in Qt-less environments.
    It supports a rectangular pixel space with color values encoded in single bits.

    This class supports basic operations such as initialization, single-pixel access and I/O to PNG.

    This class has been introduced in version 0.28.
    """
    @classmethod
    def from_png_data(cls, data: bytes) -> BitmapBuffer:
        r"""
        @brief Reads the pixel buffer from a PNG byte stream
        This method may not be available if PNG support is not compiled into KLayout.
        """
    @classmethod
    def new(cls, width: int, height: int) -> BitmapBuffer:
        r"""
        @brief Creates a pixel buffer object

        @param width The width in pixels
        @param height The height in pixels

        The pixels are basically uninitialized. You will need to use \fill to initialize them to a certain value.
        """
    @classmethod
    def read_png(cls, file: str) -> BitmapBuffer:
        r"""
        @brief Reads the pixel buffer from a PNG file
        This method may not be available if PNG support is not compiled into KLayout.
        """
    def __copy__(self) -> BitmapBuffer:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> BitmapBuffer:
        r"""
        @brief Creates a copy of self
        """
    def __eq__(self, other: object) -> bool:
        r"""
        @brief Returns a value indicating whether self is identical to the other image
        """
    def __init__(self, width: int, height: int) -> None:
        r"""
        @brief Creates a pixel buffer object

        @param width The width in pixels
        @param height The height in pixels

        The pixels are basically uninitialized. You will need to use \fill to initialize them to a certain value.
        """
    def __ne__(self, other: object) -> bool:
        r"""
        @brief Returns a value indicating whether self is not identical to the other image
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
    def assign(self, other: BitmapBuffer) -> None:
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
    def dup(self) -> BitmapBuffer:
        r"""
        @brief Creates a copy of self
        """
    def fill(self, color: bool) -> None:
        r"""
        @brief Fills the pixel buffer with the given pixel value
        """
    def height(self) -> int:
        r"""
        @brief Gets the height of the pixel buffer in pixels
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def pixel(self, x: int, y: int) -> bool:
        r"""
        @brief Gets the value of the pixel at position x, y
        """
    def set_pixel(self, x: int, y: int, c: bool) -> None:
        r"""
        @brief Sets the value of the pixel at position x, y
        """
    def swap(self, other: BitmapBuffer) -> None:
        r"""
        @brief Swaps data with another BitmapBuffer object
        """
    def to_png_data(self) -> bytes:
        r"""
        @brief Converts the pixel buffer to a PNG byte stream
        This method may not be available if PNG support is not compiled into KLayout.
        """
    def width(self) -> int:
        r"""
        @brief Gets the width of the pixel buffer in pixels
        """
    def write_png(self, file: str) -> None:
        r"""
        @brief Writes the pixel buffer to a PNG file
        This method may not be available if PNG support is not compiled into KLayout.
        """

class ButtonState:
    r"""
    @brief The namespace for the button state flags in the mouse events of the Plugin class.
    This class defines the constants for the button state. In the event handler, the button state is indicated by a bitwise combination of these constants. See \Plugin for further details.
    This class has been introduced in version 0.22.
    """
    AltKey: ClassVar[int]
    r"""
    @brief Indicates that the Alt key is pressed
    This constant is combined with other constants within \ButtonState
    """
    ControlKey: ClassVar[int]
    r"""
    @brief Indicates that the Control key is pressed
    This constant is combined with other constants within \ButtonState
    """
    LeftButton: ClassVar[int]
    r"""
    @brief Indicates that the left mouse button is pressed
    This constant is combined with other constants within \ButtonState
    """
    MidButton: ClassVar[int]
    r"""
    @brief Indicates that the middle mouse button is pressed
    This constant is combined with other constants within \ButtonState
    """
    RightButton: ClassVar[int]
    r"""
    @brief Indicates that the right mouse button is pressed
    This constant is combined with other constants within \ButtonState
    """
    ShiftKey: ClassVar[int]
    r"""
    @brief Indicates that the Shift key is pressed
    This constant is combined with other constants within \ButtonState
    """
    @classmethod
    def new(cls) -> ButtonState:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> ButtonState:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> ButtonState:
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
    def assign(self, other: ButtonState) -> None:
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
    def dup(self) -> ButtonState:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class CellView:
    r"""
    @brief A class describing what is shown inside a layout view

    The cell view points to a specific cell within a certain layout and a hierarchical context.
    For that, first of all a layout pointer is provided. The cell itself
    is addressed by an cell_index or a cell object reference.
    The layout pointer can be nil, indicating that the cell view is invalid.

    The cell is not only identified by its index or object but also 
    by the path leading to that cell. This path indicates how to find the
    cell in the hierarchical context of its parent cells. 

    The path is in fact composed of two parts: first in an unspecific fashion,
    just describing which parent cells are used. The target of this path
    is called the "context cell". It is accessible by the \ctx_cell_index
    or \ctx_cell methods. In the viewer, the unspecific part of the path is
    the location of the cell in the cell tree.

    Additionally the path's second part may further identify a specific instance of a certain
    subcell in the context cell. This is done through a set of \InstElement
    objects. The target of this specific path is the actual cell addressed by the
    cellview. This target cell is accessible by the \cell_index or \cell methods.
    In the viewer, the target cell is shown in the context of the context cell.
    The hierarchy levels are counted from the context cell, which is on level 0.
    If the context path is empty, the context cell is identical with the target cell.

    Starting with version 0.25, the cellview can be modified directly. This will have an immediate effect on the display. For example, the following code will select a different cell:

    @code
    cv = RBA::CellView::active
    cv.cell_name = "TOP2"
    @/code

    See @<a href="/programming/application_api.xml">The Application API@</a> for more details about the cellview objects.
    """
    cell: db.Cell
    r"""
    Getter:
    @brief Gets the reference to the target cell currently addressed

    Setter:
    @brief Sets the cell by reference to a Cell object
    Setting the cell reference to nil invalidates the cellview. This method will construct any path to this cell, not a 
    particular one. It will clear the context path
    and update the context and target cell.
    """
    cell_index: int
    r"""
    Getter:
    @brief Gets the target cell's index

    Setter:
    @brief Sets the path to the given cell

    This method will construct any path to this cell, not a 
    particular one. It will clear the context path
    and update the context and target cell. Note that the cell is specified by its index.
    """
    cell_name: str
    r"""
    Getter:
    @brief Gets the name of the target cell currently addressed

    Setter:
    @brief Sets the cell by name

    If the name is not a valid one, the cellview will become
    invalid.
    This method will construct any path to this cell, not a 
    particular one. It will clear the context path
    and update the context and target cell.
    """
    context_path: List[db.InstElement]
    r"""
    Getter:
    @brief Gets the cell's context path
    The context path leads from the context cell to the target cell in a specific fashion, i.e. describing each instance in detail, not just by cell indexes. If the context and target cell are identical, the context path is empty.
    Setter:
    @brief Sets the context path explicitly

    This method assumes that the unspecific part of the path 
    is established already and that the context path starts
    from the context cell.
    """
    name: str
    r"""
    Getter:
    @brief Gets the unique name associated with the layout behind the cellview

    Setter:
    @brief sets the unique name associated with the layout behind the cellview

    this method has been introduced in version 0.25.
    """
    on_technology_changed: None
    r"""
    Getter:
    @brief An event indicating that the technology has changed
    This event is triggered when the CellView is attached to a different technology.

    This event has been introduced in version 0.27.

    Setter:
    @brief An event indicating that the technology has changed
    This event is triggered when the CellView is attached to a different technology.

    This event has been introduced in version 0.27.
    """
    path: List[int]
    r"""
    Getter:
    @brief Gets the cell's unspecific part of the path leading to the context cell

    Setter:
    @brief Sets the unspecific part of the path explicitly

    Setting the unspecific part of the path will clear the context path component and
    update the context and target cell.
    """
    technology: str
    r"""
    Getter:
    @brief Returns the technology name for the layout behind the given cell view
    This method has been added in version 0.23.

    Setter:
    @brief Sets the technology for the layout behind the given cell view
    According to the specification of the technology, new layer properties may be loaded or the net tracer may be reconfigured. If the layout is shown in multiple views, the technology is updated for all views.
    This method has been added in version 0.22.
    """
    @classmethod
    def active(cls) -> CellView:
        r"""
        @brief Gets the active CellView
        The active CellView is the one that is selected in the current layout view. This method is equivalent to
        @code
        RBA::LayoutView::current.active_cellview
        @/code
        If no CellView is active, this method returns nil.

        This method has been introduced in version 0.23.
        """
    @classmethod
    def new(cls) -> CellView:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> CellView:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> CellView:
        r"""
        @brief Creates a copy of self
        """
    def __eq__(self, other: object) -> bool:
        r"""
        @brief Equality: indicates whether the cellviews refer to the same one
        In version 0.25, the definition of the equality operator has been changed to reflect identity of the cellview. Before that version, identity of the cell shown was implied.
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
    def ascend(self) -> None:
        r"""
        @brief Ascends upwards in the hierarchy.
        Removes one element from the specific path of the cellview with the given index. Returns the element removed.
        This method has been added in version 0.25.
        """
    def assign(self, other: CellView) -> None:
        r"""
        @brief Assigns another object to self
        """
    def close(self) -> None:
        r"""
        @brief Closes this cell view

        This method will close the cellview - remove it from the layout view. After this method was called, the cellview will become invalid (see \is_valid?).

        This method was introduced in version 0.25.
        """
    def context_dtrans(self) -> db.DCplxTrans:
        r"""
        @brief Gets the accumulated transformation of the context path in micron unit space
        This is the transformation applied to the target cell before it is shown in the context cell
        Technically this is the product of all transformations over the context path.
        See \context_trans for a version delivering an integer-unit space transformation.

        This method has been introduced in version 0.27.3.
        """
    def context_trans(self) -> db.ICplxTrans:
        r"""
        @brief Gets the accumulated transformation of the context path
        This is the transformation applied to the target cell before it is shown in the context cell
        Technically this is the product of all transformations over the context path.
        See \context_dtrans for a version delivering a micron-unit space transformation.

        This method has been introduced in version 0.27.3.
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def ctx_cell(self) -> db.Cell:
        r"""
        @brief Gets the reference to the context cell currently addressed
        """
    def ctx_cell_index(self) -> int:
        r"""
        @brief Gets the context cell's index
        """
    def descend(self, path: Sequence[db.InstElement]) -> None:
        r"""
        @brief Descends further into the hierarchy.
        Adds the given path (given as an array of InstElement objects) to the specific path of the cellview with the given index. In effect, the cell addressed by the terminal of the new path components can be shown in the context of the upper cells, if the minimum hierarchy level is set to a negative value.
        The path is assumed to originate from the current cell and contain specific instances sorted from top to bottom.
        This method has been added in version 0.25.
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
    def dup(self) -> CellView:
        r"""
        @brief Creates a copy of self
        """
    def filename(self) -> str:
        r"""
        @brief Gets filename associated with the layout behind the cellview
        """
    def hide_cell(self, cell: db.Cell) -> None:
        r"""
        @brief Hides the given cell

        This method has been added in version 0.25.
        """
    def index(self) -> int:
        r"""
        @brief Gets the index of this cellview in the layout view
        The index will be negative if the cellview is not a valid one.
        This method has been added in version 0.25.
        """
    def is_cell_hidden(self, cell: db.Cell) -> bool:
        r"""
        @brief Returns true, if the given cell is hidden

        This method has been added in version 0.25.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_dirty(self) -> bool:
        r"""
        @brief Gets a flag indicating whether the layout needs saving
        A layout is 'dirty' if it is modified and needs saving. This method returns true in this case.

        This method has been introduced in version 0.24.10.
        """
    def is_valid(self) -> bool:
        r"""
        @brief Returns true, if the cellview is valid
        A cellview may become invalid if the corresponding tab is closed for example.
        """
    def layout(self) -> db.Layout:
        r"""
        @brief Gets the reference to the layout object addressed by this view
        """
    def reset_cell(self) -> None:
        r"""
        @brief Resets the cell 

        The cellview will become invalid. The layout object will
        still be attached to the cellview, but no cell will be selected.
        """
    def set_cell(self, cell_index: int) -> None:
        r"""
        @brief Sets the path to the given cell

        This method will construct any path to this cell, not a 
        particular one. It will clear the context path
        and update the context and target cell. Note that the cell is specified by its index.
        """
    def set_cell_name(self, cell_name: str) -> None:
        r"""
        @brief Sets the cell by name

        If the name is not a valid one, the cellview will become
        invalid.
        This method will construct any path to this cell, not a 
        particular one. It will clear the context path
        and update the context and target cell.
        """
    def set_context_path(self, path: Sequence[db.InstElement]) -> None:
        r"""
        @brief Sets the context path explicitly

        This method assumes that the unspecific part of the path 
        is established already and that the context path starts
        from the context cell.
        """
    def set_path(self, path: Sequence[int]) -> None:
        r"""
        @brief Sets the unspecific part of the path explicitly

        Setting the unspecific part of the path will clear the context path component and
        update the context and target cell.
        """
    def show_all_cells(self) -> None:
        r"""
        @brief Makes all cells shown (cancel effects of \hide_cell) for the specified cell view

        This method has been added in version 0.25.
        """
    def show_cell(self, cell: db.Cell) -> None:
        r"""
        @brief Shows the given cell (cancels the effect of \hide_cell)

        This method has been added in version 0.25.
        """
    def view(self) -> LayoutView:
        r"""
        @brief Gets the view the cellview resides in
        This reference will be nil if the cellview is not a valid one.
        This method has been added in version 0.25.
        """

class Cursor:
    r"""
    @brief The namespace for the cursor constants
    This class defines the constants for the cursor setting (for example for class \Plugin, method set_cursor).
    This class has been introduced in version 0.22.
    """
    Arrow: ClassVar[int]
    r"""
    @brief 'Arrow cursor' constant
    """
    Blank: ClassVar[int]
    r"""
    @brief 'Blank cursor' constant
    """
    Busy: ClassVar[int]
    r"""
    @brief 'Busy state cursor' constant
    """
    ClosedHand: ClassVar[int]
    r"""
    @brief 'Closed hand cursor' constant
    """
    Cross: ClassVar[int]
    r"""
    @brief 'Cross cursor' constant
    """
    Forbidden: ClassVar[int]
    r"""
    @brief 'Forbidden area cursor' constant
    """
    IBeam: ClassVar[int]
    r"""
    @brief 'I beam (text insert) cursor' constant
    """
    None_: ClassVar[int]
    r"""
    @brief 'No cursor (default)' constant for \set_cursor (resets cursor to default)
    """
    OpenHand: ClassVar[int]
    r"""
    @brief 'Open hand cursor' constant
    """
    PointingHand: ClassVar[int]
    r"""
    @brief 'Pointing hand cursor' constant
    """
    SizeAll: ClassVar[int]
    r"""
    @brief 'Size all directions cursor' constant
    """
    SizeBDiag: ClassVar[int]
    r"""
    @brief 'Backward diagonal resize cursor' constant
    """
    SizeFDiag: ClassVar[int]
    r"""
    @brief 'Forward diagonal resize cursor' constant
    """
    SizeHor: ClassVar[int]
    r"""
    @brief 'Horizontal resize cursor' constant
    """
    SizeVer: ClassVar[int]
    r"""
    @brief 'Vertical resize cursor' constant
    """
    SplitH: ClassVar[int]
    r"""
    @brief 'split_horizontal cursor' constant
    """
    SplitV: ClassVar[int]
    r"""
    @brief 'Split vertical cursor' constant
    """
    UpArrow: ClassVar[int]
    r"""
    @brief 'Upward arrow cursor' constant
    """
    Wait: ClassVar[int]
    r"""
    @brief 'Waiting cursor' constant
    """
    WhatsThis: ClassVar[int]
    r"""
    @brief 'Question mark cursor' constant
    """
    @classmethod
    def new(cls) -> Cursor:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> Cursor:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> Cursor:
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
    def assign(self, other: Cursor) -> None:
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
    def dup(self) -> Cursor:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class Dispatcher:
    r"""
    @brief Root of the configuration space in the plugin context and menu dispatcher

    This class provides access to the root configuration space in the context of plugin programming. You can use this class to obtain configuration parameters from the configuration tree during plugin initialization. However, the preferred way of plugin configuration is through \Plugin#configure.

    Currently, the application object provides an identical entry point for configuration modification. For example, "Application::instance.set_config" is identical to "Dispatcher::instance.set_config". Hence there is little motivation for the Dispatcher class currently and this interface may be modified or removed in the future.
    This class has been introduced in version 0.25 as 'PluginRoot'.
    It is renamed and enhanced as 'Dispatcher' in 0.27.
    """
    @classmethod
    def instance(cls) -> Dispatcher:
        r"""
        @brief Gets the singleton instance of the Dispatcher object

        @return The instance
        """
    @classmethod
    def new(cls) -> Dispatcher:
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
    def clear_config(self) -> None:
        r"""
        @brief Clears the configuration parameters
        """
    def commit_config(self) -> None:
        r"""
        @brief Commits the configuration settings

        Some configuration options are queued for performance reasons and become active only after 'commit_config' has been called. After a sequence of \set_config calls, this method should be called to activate the settings made by these calls.
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
    def get_config(self, name: str) -> Any:
        r"""
        @brief Gets the value of a local configuration parameter

        @param name The name of the configuration parameter whose value shall be obtained (a string)

        @return The value of the parameter or nil if there is no such parameter
        """
    def get_config_names(self) -> List[str]:
        r"""
        @brief Gets the configuration parameter names

        @return A list of configuration parameter names

        This method returns the names of all known configuration parameters. These names can be used to get and set configuration parameter values.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def read_config(self, file_name: str) -> bool:
        r"""
        @brief Reads the configuration from a file
        @return A value indicating whether the operation was successful

        This method silently does nothing, if the config file does not
        exist. If it does and an error occurred, the error message is printed
        on stderr. In both cases, false is returned.
        """
    def set_config(self, name: str, value: str) -> None:
        r"""
        @brief Set a local configuration parameter with the given name to the given value

        @param name The name of the configuration parameter to set
        @param value The value to which to set the configuration parameter

        This method sets a configuration parameter with the given name to the given value. Values can only be strings. Numerical values have to be converted into strings first. Local configuration parameters override global configurations for this specific view. This allows for example to override global settings of background colors. Any local settings are not written to the configuration file. 
        """
    def write_config(self, file_name: str) -> bool:
        r"""
        @brief Writes configuration to a file
        @return A value indicating whether the operation was successful

        If the configuration file cannot be written, false 
        is returned but no exception is thrown.
        """

class Image(BasicImage):
    r"""
    @brief An image to be stored as a layout annotation

    Images can be put onto the layout canvas as annotations, along with rulers and markers.
    Images can be monochrome (represent scalar data) as well as color (represent color images).
    The display of images can be adjusted in various ways, i.e. color mapping (translation of scalar values to
    colors), geometrical transformations (including rotation by arbitrary angles) and similar.
    Images are always based on floating point data. The actual data range is not fixed and can be adjusted to the data set (i.e. 0..255 or -1..1). This gives a great flexibility when displaying data which is the result of some measurement or calculation for example.
    The basic parameters of an image are the width and height of the data set, the width and height of one pixel, the geometrical transformation to be applied, the data range (min_value to max_value) and the data mapping which is described by an own class, \ImageDataMapping.

    Starting with version 0.22, the basic transformation is a 3x3 matrix rather than the simple affine transformation. This matrix includes the pixel dimensions as well. One consequence of that is that the magnification part of the matrix and the pixel dimensions are no longer separated. That has certain consequences, i.e. setting an affine transformation with a magnification scales the pixel sizes as before but an affine transformation returned will no longer contain the pixel dimensions as magnification because it only supports isotropic scaling. For backward compatibility, the rotation center for the affine transformations while the default center and the center for matrix transformations is the image center.

    As with version 0.25, images become 'live' objects. Changes to image properties will be reflected in the view automatically once the image object has been inserted into a view. Note that changes are not immediately reflected in the view, but are delayed until the view is refreshed. Hence, iterating the view's images will not render the same results than the image objects attached to the view. To ensure synchronization, call \Image#update.
    """
    data_mapping: ImageDataMapping
    r"""
    Getter:
    @brief Gets the data mapping
    @return The data mapping object

    The data mapping describes the transformation of a pixel value (any double value) into pixel data which can be sent to the graphics cards for display. See \ImageDataMapping for a more detailed description.

    Setter:
    @brief Sets the data mapping object

    The data mapping describes the transformation of a pixel value (any double value) into pixel data which can be sent to the graphics cards for display. See \ImageDataMapping for a more detailed description.
    """
    mask_data: List[bool]
    r"""
    Getter:
    @brief Gets the mask from a array of boolean values
    See \set_mask_data for a description of the data field.

    This method has been introduced in version 0.27.

    Setter:
    @brief Sets the mask from a array of boolean values
    The order of the boolean values is line first, from bottom to top and left to right and is the same as the order in the data array.

    This method has been introduced in version 0.27.
    """
    matrix: db.Matrix3d
    r"""
    Getter:
    @brief Returns the pixel-to-micron transformation matrix

    This transformation matrix converts pixel coordinates (0,0 being the center and each pixel having the dimension of pixel_width and pixel_height)
    to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.

    The matrix is more general than the transformation used before and supports shear and perspective transformation. This property replaces the \trans property which is still functional, but deprecated.

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the transformation matrix

    This transformation matrix converts pixel coordinates (0,0 being the center and each pixel having the dimension of pixel_width and pixel_height)
    to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.

    The matrix is more general than the transformation used before and supports shear and perspective transformation. This property replaces the \trans property which is still functional, but deprecated.

    This method has been introduced in version 0.22.
    """
    max_value: float
    r"""
    Getter:
    @brief Sets the maximum value

    See the \max_value method for the description of the maximum value property.

    Setter:
    @brief Gets the upper limit of the values in the data set

    This value determines the upper end of the data mapping (i.e. white value etc.).
    It does not necessarily correspond to the maximum value of the data set but it must be
    larger than that.
    """
    min_value: float
    r"""
    Getter:
    @brief Gets the upper limit of the values in the data set

    This value determines the upper end of the data mapping (i.e. white value etc.).
    It does not necessarily correspond to the minimum value of the data set but it must be
    larger than that.

    Setter:
    @brief Sets the minimum value

    See \min_value for the description of the minimum value property.
    """
    pixel_height: float
    r"""
    Getter:
    @brief Gets the pixel height

    See \pixel_height= for a description of that property.

    Starting with version 0.22, this property is incorporated into the transformation matrix.
    This property is provided for convenience only.
    Setter:
    @brief Sets the pixel height

    The pixel height determines the height of on pixel in the original space which is transformed to
    micron space with the transformation.

    Starting with version 0.22, this property is incorporated into the transformation matrix.
    This property is provided for convenience only.
    """
    pixel_width: float
    r"""
    Getter:
    @brief Gets the pixel width

    See \pixel_width= for a description of that property.

    Starting with version 0.22, this property is incorporated into the transformation matrix.
    This property is provided for convenience only.
    Setter:
    @brief Sets the pixel width

    The pixel width determines the width of on pixel in the original space which is transformed to
    micron space with the transformation.

    Starting with version 0.22, this property is incorporated into the transformation matrix.
    This property is provided for convenience only.
    """
    trans: db.DCplxTrans
    r"""
    Getter:
    @brief Returns the pixel-to-micron transformation

    This transformation converts pixel coordinates (0,0 being the lower left corner and each pixel having the dimension of pixel_width and pixel_height)
    to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.

    The general property is \matrix which also allows perspective and shear transformation. This property will only work, if the transformation does not include perspective or shear components. Therefore this property is deprecated.
    Please note that for backward compatibility, the rotation center is pixel 0,0 (lowest left one), while it is the image center for the matrix transformation.
    Setter:
    @brief Sets the transformation

    This transformation converts pixel coordinates (0,0 being the lower left corner and each pixel having the dimension of pixel_width and pixel_height)
    to micron coordinates. The coordinate of the pixel is the lower left corner of the pixel.

    The general property is \matrix which also allows perspective and shear transformation.
    Please note that for backward compatibility, the rotation center is pixel 0,0 (lowest left one), while it is the image center for the matrix transformation.
    """
    visible: bool
    r"""
    Getter:
    @brief Gets a flag indicating whether the image object is visible

    An image object can be made invisible by setting the visible property to false.

    This method has been introduced in version 0.20.

    Setter:
    @brief Sets the visibility

    See the \is_visible? method for a description of this property.

    This method has been introduced in version 0.20.
    """
    z_position: int
    r"""
    Getter:
    @brief Gets the z position of the image
    Images with a higher z position are painted in front of images with lower z position.
    The z value is an integer that controls the position relative to other images.

    This method was introduced in version 0.25.
    Setter:
    @brief Sets the z position of the image

    See \z_position for details about the z position attribute.

    This method was introduced in version 0.25.
    """
    @classmethod
    def from_s(cls, s: str) -> Image:
        r"""
        @brief Creates an image from the string returned by \to_s.
        This method has been introduced in version 0.27.
        """
    @overload
    @classmethod
    def new(cls) -> Image:
        r"""
        @brief Create a new image with the default attributes
        This will create an empty image without data and no particular pixel width or related.
        Use the \read_file or \set_data methods to set image properties and pixel values.
        """
    @overload
    @classmethod
    def new(cls, filename: str, trans: Optional[db.DCplxTrans] = ...) -> Image:
        r"""
        @brief Constructor from a image file

        This constructor creates an image object from a file (which can have any format supported by Qt) and 
        a transformation. The image will originally be put to position 0,0 (lower left corner) and each pixel
        will have a size of 1. The transformation describes how to transform this image into micron space.

        @param filename The path to the image file to load.
        @param trans The transformation to apply to the image when displaying it.
        """
    @overload
    @classmethod
    def new(cls, pixels: PixelBuffer, trans: Optional[db.DCplxTrans] = ...) -> Image:
        r"""
        @brief Constructor from a image pixel buffer

        This constructor creates an image object from a pixel buffer object. This object holds RGB or mono image data similar to QImage, except it is available also when Qt is not available (e.g. inside the Python module).

        The image will originally be put to position 0,0 (lower left corner) and each pixel
        will have a size of 1. The transformation describes how to transform this image into micron space.

        @param filename The path to the image file to load.
        @param trans The transformation to apply to the image when displaying it.
        """
    @overload
    @classmethod
    def new(cls, w: int, h: int, data: Sequence[float]) -> Image:
        r"""
        @brief Constructor for a monochrome image with the given pixel values

        This constructor creates an image from the given pixel values. The values have to be organized
        line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
        Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
        the common convention for image data.
        Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). 
        To adjust the data range use the \min_value and \max_value properties.

        @param w The width of the image
        @param h The height of the image
        @param d The data (see method description)
        """
    @overload
    @classmethod
    def new(cls, w: int, h: int, red: Sequence[float], green: Sequence[float], blue: Sequence[float]) -> Image:
        r"""
        @brief Constructor for a color image with the given pixel values

        This constructor creates an image from the given pixel values. The values have to be organized
        line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
        Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
        the common convention for image data.
        Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). 
        To adjust the data range use the \min_value and \max_value properties.

        @param w The width of the image
        @param h The height of the image
        @param red The red channel data set which will become owned by the image
        @param green The green channel data set which will become owned by the image
        @param blue The blue channel data set which will become owned by the image
        """
    @overload
    @classmethod
    def new(cls, w: int, h: int, trans: db.DCplxTrans, data: Sequence[float]) -> Image:
        r"""
        @brief Constructor for a monochrome image with the given pixel values

        This constructor creates an image from the given pixel values. The values have to be organized
        line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
        Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
        the common convention for image data.
        Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). 
        To adjust the data range use the \min_value and \max_value properties.

        @param w The width of the image
        @param h The height of the image
        @param trans The transformation from pixel space to micron space
        @param d The data (see method description)
        """
    @overload
    @classmethod
    def new(cls, w: int, h: int, trans: db.DCplxTrans, red: Sequence[float], green: Sequence[float], blue: Sequence[float]) -> Image:
        r"""
        @brief Constructor for a color image with the given pixel values

        This constructor creates an image from the given pixel values. The values have to be organized
        line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
        Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
        the common convention for image data.
        Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). 
        To adjust the data range use the \min_value and \max_value properties.

        @param w The width of the image
        @param h The height of the image
        @param trans The transformation from pixel space to micron space
        @param red The red channel data set which will become owned by the image
        @param green The green channel data set which will become owned by the image
        @param blue The blue channel data set which will become owned by the image
        """
    @classmethod
    def read(cls, path: str) -> Image:
        r"""
        @brief Loads the image from the given path.

        This method expects the image file as a KLayout image format file (.lyimg). This is a XML-based format containing the image data plus placement and transformation information for the image placement. In addition, image manipulation parameters for false color display and color channel enhancement are embedded.

        This method has been introduced in version 0.27.
        """
    @overload
    def __init__(self) -> None:
        r"""
        @brief Create a new image with the default attributes
        This will create an empty image without data and no particular pixel width or related.
        Use the \read_file or \set_data methods to set image properties and pixel values.
        """
    @overload
    def __init__(self, filename: str, trans: Optional[db.DCplxTrans] = ...) -> None:
        r"""
        @brief Constructor from a image file

        This constructor creates an image object from a file (which can have any format supported by Qt) and 
        a transformation. The image will originally be put to position 0,0 (lower left corner) and each pixel
        will have a size of 1. The transformation describes how to transform this image into micron space.

        @param filename The path to the image file to load.
        @param trans The transformation to apply to the image when displaying it.
        """
    @overload
    def __init__(self, pixels: PixelBuffer, trans: Optional[db.DCplxTrans] = ...) -> None:
        r"""
        @brief Constructor from a image pixel buffer

        This constructor creates an image object from a pixel buffer object. This object holds RGB or mono image data similar to QImage, except it is available also when Qt is not available (e.g. inside the Python module).

        The image will originally be put to position 0,0 (lower left corner) and each pixel
        will have a size of 1. The transformation describes how to transform this image into micron space.

        @param filename The path to the image file to load.
        @param trans The transformation to apply to the image when displaying it.
        """
    @overload
    def __init__(self, w: int, h: int, data: Sequence[float]) -> None:
        r"""
        @brief Constructor for a monochrome image with the given pixel values

        This constructor creates an image from the given pixel values. The values have to be organized
        line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
        Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
        the common convention for image data.
        Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). 
        To adjust the data range use the \min_value and \max_value properties.

        @param w The width of the image
        @param h The height of the image
        @param d The data (see method description)
        """
    @overload
    def __init__(self, w: int, h: int, red: Sequence[float], green: Sequence[float], blue: Sequence[float]) -> None:
        r"""
        @brief Constructor for a color image with the given pixel values

        This constructor creates an image from the given pixel values. The values have to be organized
        line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
        Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
        the common convention for image data.
        Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). 
        To adjust the data range use the \min_value and \max_value properties.

        @param w The width of the image
        @param h The height of the image
        @param red The red channel data set which will become owned by the image
        @param green The green channel data set which will become owned by the image
        @param blue The blue channel data set which will become owned by the image
        """
    @overload
    def __init__(self, w: int, h: int, trans: db.DCplxTrans, data: Sequence[float]) -> None:
        r"""
        @brief Constructor for a monochrome image with the given pixel values

        This constructor creates an image from the given pixel values. The values have to be organized
        line by line. Each line must consist of "w" values where the first value is the leftmost pixel.
        Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
        the common convention for image data.
        Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). 
        To adjust the data range use the \min_value and \max_value properties.

        @param w The width of the image
        @param h The height of the image
        @param trans The transformation from pixel space to micron space
        @param d The data (see method description)
        """
    @overload
    def __init__(self, w: int, h: int, trans: db.DCplxTrans, red: Sequence[float], green: Sequence[float], blue: Sequence[float]) -> None:
        r"""
        @brief Constructor for a color image with the given pixel values

        This constructor creates an image from the given pixel values. The values have to be organized
        line by line and separated by color channel. Each line must consist of "w" values where the first value is the leftmost pixel.
        Note, that the rows are oriented in the mathematical sense (first one is the lowest) contrary to 
        the common convention for image data.
        Initially the pixel width and height will be 1 micron and the data range will be 0 to 1.0 (black to white level). 
        To adjust the data range use the \min_value and \max_value properties.

        @param w The width of the image
        @param h The height of the image
        @param trans The transformation from pixel space to micron space
        @param red The red channel data set which will become owned by the image
        @param green The green channel data set which will become owned by the image
        @param blue The blue channel data set which will become owned by the image
        """
    def __repr__(self) -> str:
        r"""
        @brief Converts the image to a string
        The string returned can be used to create an image object using \from_s.
        @return The string
        """
    def __str__(self) -> str:
        r"""
        @brief Converts the image to a string
        The string returned can be used to create an image object using \from_s.
        @return The string
        """
    def _assign(self, other: BasicImage) -> None:
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
    def _dup(self) -> Image:
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
    def box(self) -> db.DBox:
        r"""
        @brief Gets the bounding box of the image
        @return The bounding box
        """
    def clear(self) -> None:
        r"""
        @brief Clears the image data (sets to 0 or black).
        This method has been introduced in version 0.27.
        """
    def data(self, channel: Optional[int] = ...) -> List[float]:
        r"""
        @brief Gets the data array for a specific color channel
        Returns an array of pixel values for the given channel. For a color image, channel 0 is green, channel 1 is red and channel 2 is blue. For a monochrome image, the channel is ignored.

        For the format of the data see the constructor description.

        This method has been introduced in version 0.27.
        """
    def delete(self) -> None:
        r"""
        @brief Deletes this image from the view
        If the image is an "active" one, this method will remove it from the view. This object will become detached and can still be manipulated, but without having an effect on the view.
        This method has been introduced in version 0.25.
        """
    def detach(self) -> None:
        r"""
        @brief Detaches the image object from the view
        If the image object was inserted into the view, property changes will be reflected in the view. To disable this feature, 'detach'' can be called after which the image object becomes inactive and changes will no longer be reflected in the view.

        This method has been introduced in version 0.25.
        """
    def filename(self) -> str:
        r"""
        @brief Gets the name of the file loaded of an empty string if not file is loaded
        @return The file name (path)
        """
    @overload
    def get_pixel(self, x: int, y: int) -> float:
        r"""
        @brief Gets one pixel (monochrome only)

        @param x The x coordinate of the pixel (0..width()-1)
        @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)

        If x or y value exceeds the image bounds, this method 
        returns 0.0. This method is valid for monochrome images only. For color images it will return 0.0 always.
        Use \is_color? to decide whether the image is a color image or monochrome one.
        """
    @overload
    def get_pixel(self, x: int, y: int, component: int) -> float:
        r"""
        @brief Gets one pixel (monochrome and color)

        @param x The x coordinate of the pixel (0..width()-1)
        @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
        @param component 0 for red, 1 for green, 2 for blue.

        If the component index, x or y value exceeds the image bounds, this method 
        returns 0.0. For monochrome images, the component index is ignored.
        """
    def height(self) -> int:
        r"""
        @brief Gets the height of the image in pixels
        @return The height in pixels
        """
    def id(self) -> int:
        r"""
        @brief Gets the Id

        The Id is an arbitrary integer that can be used to track the evolution of an
        image object. The Id is not changed when the object is edited.
        On initialization, a unique Id is given to the object. The Id cannot be changed. This behaviour has been modified in version 0.20.
        """
    def is_color(self) -> bool:
        r"""
        @brief Returns true, if the image is a color image
        @return True, if the image is a color image
        """
    def is_empty(self) -> bool:
        r"""
        @brief Returns true, if the image does not contain any data (i.e. is default constructed)
        @return True, if the image is empty
        """
    def is_valid(self) -> bool:
        r"""
        @brief Returns a value indicating whether the object is a valid reference.
        If this value is true, the object represents an image on the screen. Otherwise, the object is a 'detached' image which does not have a representation on the screen.

        This method was introduced in version 0.25.
        """
    def is_visible(self) -> bool:
        r"""
        @brief Gets a flag indicating whether the image object is visible

        An image object can be made invisible by setting the visible property to false.

        This method has been introduced in version 0.20.
        """
    def mask(self, x: int, y: int) -> bool:
        r"""
        @brief Gets the mask for one pixel

        @param x The x coordinate of the pixel (0..width()-1)
        @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
        @return false if the pixel is not drawn.

        See \set_mask for details about the mask.

        This method has been introduced in version 0.23.
        """
    @overload
    def set_data(self, w: int, h: int, d: Sequence[float]) -> None:
        r"""
        @brief Writes the image data field (monochrome)
        @param w The width of the new data
        @param h The height of the new data
        @param d The (monochrome) data to load into the image

        See the constructor description for the data organisation in that field.
        """
    @overload
    def set_data(self, w: int, h: int, r: Sequence[float], g: Sequence[float], b: Sequence[float]) -> None:
        r"""
        @brief Writes the image data field (color)
        @param w The width of the new data
        @param h The height of the new data
        @param r The red channel data to load into the image
        @param g The green channel data to load into the image
        @param b The blue channel data to load into the image

        See the constructor description for the data organisation in that field.
        """
    def set_mask(self, x: int, y: int, m: bool) -> None:
        r"""
        @brief Sets the mask for a pixel

        @param x The x coordinate of the pixel (0..width()-1)
        @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
        @param m The mask

        If the mask of a pixel is set to false, the pixel is not drawn. The default is true for all pixels.

        This method has been introduced in version 0.23.
        """
    @overload
    def set_pixel(self, x: int, y: int, r: float, g: float, b: float) -> None:
        r"""
        @brief Sets one pixel (color)

        @param x The x coordinate of the pixel (0..width()-1)
        @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
        @param red The red component
        @param green The green component
        @param blue The blue component

        If the component index, x or y value exceeds the image bounds of the image is not a color image,
        this method does nothing.
        """
    @overload
    def set_pixel(self, x: int, y: int, v: float) -> None:
        r"""
        @brief Sets one pixel (monochrome)

        @param x The x coordinate of the pixel (0..width()-1)
        @param y The y coordinate of the pixel (mathematical order: 0 is the lowest, 0..height()-1)
        @param v The value

        If the component index, x or y value exceeds the image bounds of the image is a color image,
        this method does nothing.
        """
    def to_s(self) -> str:
        r"""
        @brief Converts the image to a string
        The string returned can be used to create an image object using \from_s.
        @return The string
        """
    @overload
    def transformed(self, t: db.DCplxTrans) -> Image:
        r"""
        @brief Transforms the image with the given complex transformation
        @param t The magnifying transformation to apply
        @return The transformed object
        """
    @overload
    def transformed(self, t: db.DTrans) -> Image:
        r"""
        @brief Transforms the image with the given simple transformation
        @param t The transformation to apply
        @return The transformed object
        """
    @overload
    def transformed(self, t: db.Matrix3d) -> Image:
        r"""
        @brief Transforms the image with the given matrix transformation
        @param t The transformation to apply (a matrix)
        @return The transformed object
        This method has been introduced in version 0.22.
        """
    def transformed_cplx(self, t: db.DCplxTrans) -> Image:
        r"""
        @brief Transforms the image with the given complex transformation
        @param t The magnifying transformation to apply
        @return The transformed object
        """
    def transformed_matrix(self, t: db.Matrix3d) -> Image:
        r"""
        @brief Transforms the image with the given matrix transformation
        @param t The transformation to apply (a matrix)
        @return The transformed object
        This method has been introduced in version 0.22.
        """
    def update(self) -> None:
        r"""
        @brief Forces an update of the view
        Usually it is not required to call this method. The image object is automatically synchronized with the view's image objects. For performance reasons this update is delayed to collect multiple update requests. Calling 'update' will ensure immediate updates.

        This method has been introduced in version 0.25.
        """
    def width(self) -> int:
        r"""
        @brief Gets the width of the image in pixels
        @return The width in pixels
        """
    def write(self, path: str) -> None:
        r"""
        @brief Saves the image to KLayout's image format (.lyimg)
        This method has been introduced in version 0.27.
        """

class ImageDataMapping:
    r"""
    @brief A structure describing the data mapping of an image object

    Data mapping is the process of transforming the data into RGB pixel values.
    This implementation provides four adjustment steps: first, in the case of monochrome
    data, the data is converted to a RGB triplet using the color map. The default color map
    will copy the value to all channels rendering a gray scale. After having normalized the data 
    to 0..1 cooresponding to the min_value and max_value settings of the image, a color channel-independent
    brightness and contrast adjustment is applied. Then, a per-channel multiplier (red_gain, green_gain,
    blue_gain) is applied. Finally, the gamma function is applied and the result converted into a 0..255 
    pixel value range and clipped.
    """
    blue_gain: float
    r"""
    Getter:
    @brief The blue channel gain

    This value is the multiplier by which the blue channel is scaled after applying 
    false color transformation and contrast/brightness/gamma.

    1.0 is a neutral value. The gain should be >=0.0.

    Setter:
    @brief Set the blue_gain
    See \blue_gain for a description of this property.
    """
    brightness: float
    r"""
    Getter:
    @brief The brightness value

    The brightness is a double value between roughly -1.0 and 1.0. 
    Neutral (original) brightness is 0.0.

    Setter:
    @brief Set the brightness
    See \brightness for a description of this property.
    """
    contrast: float
    r"""
    Getter:
    @brief The contrast value

    The contrast is a double value between roughly -1.0 and 1.0. 
    Neutral (original) contrast is 0.0.

    Setter:
    @brief Set the contrast
    See \contrast for a description of this property.
    """
    gamma: float
    r"""
    Getter:
    @brief The gamma value

    The gamma value allows one to adjust for non-linearities in the display chain and to enhance contrast.
    A value for linear intensity reproduction on the screen is roughly 0.5. The exact value depends on the 
    monitor calibration. Values below 1.0 give a "softer" appearance while values above 1.0 give a "harder" appearance.

    Setter:
    @brief Set the gamma
    See \gamma for a description of this property.
    """
    green_gain: float
    r"""
    Getter:
    @brief The green channel gain

    This value is the multiplier by which the green channel is scaled after applying 
    false color transformation and contrast/brightness/gamma.

    1.0 is a neutral value. The gain should be >=0.0.

    Setter:
    @brief Set the green_gain
    See \green_gain for a description of this property.
    """
    red_gain: float
    r"""
    Getter:
    @brief The red channel gain

    This value is the multiplier by which the red channel is scaled after applying 
    false color transformation and contrast/brightness/gamma.

    1.0 is a neutral value. The gain should be >=0.0.

    Setter:
    @brief Set the red_gain
    See \red_gain for a description of this property.
    """
    @classmethod
    def new(cls) -> ImageDataMapping:
        r"""
        @brief Create a new data mapping object with default settings
        """
    def __copy__(self) -> ImageDataMapping:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> ImageDataMapping:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self) -> None:
        r"""
        @brief Create a new data mapping object with default settings
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
    @overload
    def add_colormap_entry(self, value: float, color: int) -> None:
        r"""
        @brief Add a colormap entry for this data mapping object.
        @param value The value at which the given color should be applied.
        @param color The color to apply (a 32 bit RGB value).

        This settings establishes a color mapping for a given value in the monochrome channel. The color must be given as a 32 bit integer, where the lowest order byte describes the blue component (0 to 255), the second byte the green component and the third byte the red component, i.e. 0xff0000 is red and 0x0000ff is blue. 
        """
    @overload
    def add_colormap_entry(self, value: float, lcolor: int, rcolor: int) -> None:
        r"""
        @brief Add a colormap entry for this data mapping object.
        @param value The value at which the given color should be applied.
        @param lcolor The color to apply left of the value (a 32 bit RGB value).
        @param rcolor The color to apply right of the value (a 32 bit RGB value).

        This settings establishes a color mapping for a given value in the monochrome channel. The colors must be given as a 32 bit integer, where the lowest order byte describes the blue component (0 to 255), the second byte the green component and the third byte the red component, i.e. 0xff0000 is red and 0x0000ff is blue.

        In contrast to the version with one color, this version allows specifying a color left and right of the value - i.e. a discontinuous step.

        This variant has been introduced in version 0.27.
        """
    def assign(self, other: ImageDataMapping) -> None:
        r"""
        @brief Assigns another object to self
        """
    def clear_colormap(self) -> None:
        r"""
        @brief The the color map of this data mapping object.
        """
    def colormap_color(self, n: int) -> int:
        r"""
        @brief Returns the color for a given color map entry.
        @param n The index of the entry (0..\num_colormap_entries-1)
        @return The color (see \add_colormap_entry for a description).

        NOTE: this version is deprecated and provided for backward compatibility. For discontinuous nodes this method delivers the left-sided color.
        """
    def colormap_lcolor(self, n: int) -> int:
        r"""
        @brief Returns the left-side color for a given color map entry.
        @param n The index of the entry (0..\num_colormap_entries-1)
        @return The color (see \add_colormap_entry for a description).

        This method has been introduced in version 0.27.
        """
    def colormap_rcolor(self, n: int) -> int:
        r"""
        @brief Returns the right-side color for a given color map entry.
        @param n The index of the entry (0..\num_colormap_entries-1)
        @return The color (see \add_colormap_entry for a description).

        This method has been introduced in version 0.27.
        """
    def colormap_value(self, n: int) -> float:
        r"""
        @brief Returns the value for a given color map entry.
        @param n The index of the entry (0..\num_colormap_entries-1)
        @return The value (see \add_colormap_entry for a description).
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
    def dup(self) -> ImageDataMapping:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def num_colormap_entries(self) -> int:
        r"""
        @brief Returns the current number of color map entries.
        @return The number of entries.
        """

class KeyCode:
    r"""
    @brief The namespace for the some key codes.
    This namespace defines some key codes understood by built-in \LayoutView components. When compiling with Qt, these codes are compatible with Qt's key codes.
    The key codes are intended to be used when directly interfacing with \LayoutView in non-Qt-based environments.

    This class has been introduced in version 0.28.
    """
    Backspace: ClassVar[int]
    r"""
    @brief Indicates the Backspace key
    """
    Backtab: ClassVar[int]
    r"""
    @brief Indicates the Backtab key
    """
    Delete: ClassVar[int]
    r"""
    @brief Indicates the Delete key
    """
    Down: ClassVar[int]
    r"""
    @brief Indicates the Down key
    """
    End: ClassVar[int]
    r"""
    @brief Indicates the End key
    """
    Enter: ClassVar[int]
    r"""
    @brief Indicates the Enter key
    """
    Escape: ClassVar[int]
    r"""
    @brief Indicates the Escape key
    """
    Home: ClassVar[int]
    r"""
    @brief Indicates the Home key
    """
    Insert: ClassVar[int]
    r"""
    @brief Indicates the Insert key
    """
    Left: ClassVar[int]
    r"""
    @brief Indicates the Left key
    """
    PageDown: ClassVar[int]
    r"""
    @brief Indicates the PageDown key
    """
    PageUp: ClassVar[int]
    r"""
    @brief Indicates the PageUp key
    """
    Return: ClassVar[int]
    r"""
    @brief Indicates the Return key
    """
    Right: ClassVar[int]
    r"""
    @brief Indicates the Right key
    """
    Tab: ClassVar[int]
    r"""
    @brief Indicates the Tab key
    """
    Up: ClassVar[int]
    r"""
    @brief Indicates the Up key
    """
    @classmethod
    def new(cls) -> KeyCode:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> KeyCode:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> KeyCode:
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
    def assign(self, other: KeyCode) -> None:
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
    def dup(self) -> KeyCode:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class LayerProperties:
    r"""
    @brief The layer properties structure

    The layer properties encapsulate the settings relevant for
    the display and source of a layer.

    Each attribute is present in two incarnations: local and real.
    "real" refers to the effective attribute after collecting the 
    attributes from the parents to the leaf property node.
    In the spirit of this distinction, all read accessors
    are present in "local" and "real" form. The read accessors take
    a boolean parameter "real" that must be set to true, if the real
    value shall be returned.

    "brightness" is a index that indicates how much to make the
    color brighter to darker rendering the effective color 
    (\eff_frame_color, \eff_fill_color). It's value is roughly between
    -255 and 255.
    """
    animation: int
    r"""
    Getter:
    @brief Gets the animation state

    This method is a convenience method for "animation(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the animation state

    See the description of the \animation method for details about the animation state
    """
    dither_pattern: int
    r"""
    Getter:
    @brief Gets the dither pattern index

    This method is a convenience method for "dither_pattern(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the dither pattern index

    The dither pattern index must be one of the valid indices.
    The first indices are reserved for built-in pattern, the following ones are custom pattern.
    Index 0 is always solid filled and 1 is always the hollow filled pattern.
    For custom pattern see \LayoutView#add_stipple.
    """
    fill_brightness: int
    r"""
    Getter:
    @brief Gets the fill brightness value

    This method is a convenience method for "fill_brightness(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the fill brightness

    For neutral brightness set this value to 0. For darker colors set it to a negative value (down to -255), for brighter colors to a positive value (up to 255)
    """
    fill_color: int
    r"""
    Getter:
    @brief Gets the fill color

    This method is a convenience method for "fill_color(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the fill color to the given value

    The color is a 32bit value encoding the blue value in the lower 8 bits, the green value in the next 8 bits and the red value in the 8 bits above that.
    """
    frame_brightness: int
    r"""
    Getter:
    @brief Gets the frame brightness value

    This method is a convenience method for "frame_brightness(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the frame brightness

    For neutral brightness set this value to 0. For darker colors set it to a negative value (down to -255), for brighter colors to a positive value (up to 255)
    """
    frame_color: int
    r"""
    Getter:
    @brief Gets the frame color

    This method is a convenience method for "frame_color(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the frame color to the given value

    The color is a 32bit value encoding the blue value in the lower 8 bits, the green value in the next 8 bits and the red value in the 8 bits above that.
    """
    line_style: int
    r"""
    Getter:
    @brief Gets the line style index

    This method is a convenience method for "line_style(true)"

    This method has been introduced in version 0.25.
    Setter:
    @brief Sets the line style index

    The line style index must be one of the valid indices.
    The first indices are reserved for built-in pattern, the following ones are custom pattern.
    Index 0 is always solid filled.
    For custom line styles see \LayoutView#add_line_style.

    This method has been introduced in version 0.25.
    """
    lower_hier_level: int
    r"""
    Getter:
    @brief Gets the lower hierarchy level shown

    This method is a convenience method for "lower_hier_level(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the lower hierarchy level

    If this method is called, the lower hierarchy level is enabled. See \lower_hier_level for a description of this property.
    """
    marked: bool
    r"""
    Getter:
    @brief Gets the marked state

    This method is a convenience method for "marked?(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the marked state
    """
    name: str
    r"""
    Getter:
    @brief Gets the name

    Setter:
    @brief Sets the name to the given string
    """
    source: str
    r"""
    Getter:
    @brief Gets the source specification 

    This method is a convenience method for "source(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Loads the source specification from a string

    Sets the source specification to the given string. The source specification may contain the cellview index, the source layer (given by layer/datatype or layer name), transformation, property selector etc.
    This method throws an exception if the specification is not valid. 
    """
    source_cellview: int
    r"""
    Getter:
    @brief Gets the cellview index that this layer refers to

    This method is a convenience method for "source_cellview(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the cellview index that this layer refers to

    See \cellview for a description of the transformations.
    """
    source_datatype: int
    r"""
    Getter:
    @brief Gets the stream datatype that the shapes are taken from

    This method is a convenience method for "source_datatype(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the stream datatype that the shapes are taken from

    See \datatype for a description of this property
    """
    source_layer: int
    r"""
    Getter:
    @brief Gets the stream layer that the shapes are taken from

    This method is a convenience method for "source_layer(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the stream layer that the shapes are taken from

    See \source_layer for a description of this property
    """
    source_layer_index: int
    r"""
    Getter:
    @brief Gets the stream layer that the shapes are taken from

    This method is a convenience method for "source_layer_index(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the layer index specification that the shapes are taken from

    See \source_layer_index for a description of this property.
    """
    source_name: str
    r"""
    Getter:
    @brief Gets the stream name that the shapes are taken from

    This method is a convenience method for "source_name(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the stream layer name that the shapes are taken from

    See \name for a description of this property
    """
    trans: List[db.DCplxTrans]
    r"""
    Getter:
    @brief Gets the transformations that the layer is transformed with

    This method is a convenience method for "trans(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the transformations that the layer is transformed with

    See \trans for a description of the transformations.
    """
    transparent: bool
    r"""
    Getter:
    @brief Gets the transparency state

    This method is a convenience method for "transparent?(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the transparency state
    """
    upper_hier_level: int
    r"""
    Getter:
    @brief Gets the upper hierarchy level shown

    This method is a convenience method for "upper_hier_level(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets a upper hierarchy level

    If this method is called, the upper hierarchy level is enabled. See \upper_hier_level for a description of this property.
    """
    valid: bool
    r"""
    Getter:
    @brief Gets the validity state

    This method is a convenience method for "valid?(true)"

    This method has been introduced in version 0.23.
    Setter:
    @brief Sets the validity state
    """
    visible: bool
    r"""
    Getter:
    @brief Gets the visibility state

    This method is a convenience method for "visible?(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the visibility state
    """
    width: int
    r"""
    Getter:
    @brief Gets the line width

    This method is a convenience method for "width(true)"

    This method has been introduced in version 0.22.
    Setter:
    @brief Sets the line width to the given width
    """
    xfill: bool
    r"""
    Getter:
    @brief Gets a value indicating whether shapes are drawn with a cross

    This method is a convenience method for "xfill?(true)"

    This attribute has been introduced in version 0.25.

    Setter:
    @brief Sets a value indicating whether shapes are drawn with a cross

    This attribute has been introduced in version 0.25.
    """
    @classmethod
    def new(cls) -> LayerProperties:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> LayerProperties:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> LayerProperties:
        r"""
        @brief Creates a copy of self
        """
    def __eq__(self, other: object) -> bool:
        r"""
        @brief Equality 

        @param other The other object to compare against
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def __ne__(self, other: object) -> bool:
        r"""
        @brief Inequality 

        @param other The other object to compare against
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
    def assign(self, other: LayerProperties) -> None:
        r"""
        @brief Assigns another object to self
        """
    def cellview(self) -> int:
        r"""
        @brief Gets the the cellview index

        This is the index of the actual cellview to use. Basically, this method returns \source_cellview in "real" mode. The result may be different, if the cellview is not valid for example. In this case, a negative value is returned. 
        """
    def clear_dither_pattern(self) -> None:
        r"""
        @brief Clears the dither pattern
        """
    def clear_fill_color(self) -> None:
        r"""
        @brief Resets the fill color
        """
    def clear_frame_color(self) -> None:
        r"""
        @brief Resets the frame color 
        """
    def clear_line_style(self) -> None:
        r"""
        @brief Clears the line style

        This method has been introduced in version 0.25.
        """
    def clear_lower_hier_level(self) -> None:
        r"""
        @brief Clears the lower hierarchy level specification

        See \has_lower_hier_level for a description of this property
        """
    def clear_source_name(self) -> None:
        r"""
        @brief Removes any stream layer name specification from this layer
        """
    def clear_upper_hier_level(self) -> None:
        r"""
        @brief Clears the upper hierarchy level specification

        See \has_upper_hier_level for a description of this property
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
    def dup(self) -> LayerProperties:
        r"""
        @brief Creates a copy of self
        """
    @overload
    def eff_dither_pattern(self) -> int:
        r"""
        @brief Gets the effective dither pattern index

        This method is a convenience method for "eff_dither_pattern(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def eff_dither_pattern(self, real: bool) -> int:
        r"""
        @brief Gets the effective dither pattern index

        The effective dither pattern index is always a valid index, even if no dither pattern is set.
        @param real Set to true to return the real instead of local value
        """
    @overload
    def eff_fill_color(self) -> int:
        r"""
        @brief Gets the effective fill color

        This method is a convenience method for "eff_fill_color(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def eff_fill_color(self, real: bool) -> int:
        r"""
        @brief Gets the effective fill color

        The effective fill color is computed from the frame color brightness and the
        frame color.

        @param real Set to true to return the real instead of local value
        """
    @overload
    def eff_frame_color(self) -> int:
        r"""
        @brief Gets the effective frame color

        This method is a convenience method for "eff_frame_color(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def eff_frame_color(self, real: bool) -> int:
        r"""
        @brief Gets the effective frame color 

        The effective frame color is computed from the frame color brightness and the
        frame color.

        @param real Set to true to return the real instead of local value
        """
    @overload
    def eff_line_style(self) -> int:
        r"""
        @brief Gets the line style index

        This method is a convenience method for "eff_line_style(true)"

        This method has been introduced in version 0.25.
        """
    @overload
    def eff_line_style(self, real: bool) -> int:
        r"""
        @brief Gets the effective line style index

        The effective line style index is always a valid index, even if no line style is set. In that case, a default style index will be returned.

        @param real Set to true to return the real instead of local value

        This method has been introduced in version 0.25.
        """
    def flat(self) -> LayerProperties:
        r"""
        @brief Returns the "flattened" (effective) layer properties entry for this node

        This method returns a \LayerProperties object that is not embedded into a hierarchy.
        This object represents the effective layer properties for the given node. In particular, all 'local' properties are identical to the 'real' properties. Such an object can be used as a basis for manipulations.
        This method has been introduced in version 0.22.
        """
    @overload
    def has_dither_pattern(self) -> bool:
        r"""
        @brief True, if the dither pattern is set

        This method is a convenience method for "has_dither_pattern?(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def has_dither_pattern(self, real: bool) -> bool:
        r"""
        @brief True, if the dither pattern is set
        """
    @overload
    def has_fill_color(self) -> bool:
        r"""
        @brief True, if the fill color is set

        This method is a convenience method for "has_fill_color?(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def has_fill_color(self, real: bool) -> bool:
        r"""
        @brief True, if the fill color is set
        """
    @overload
    def has_frame_color(self) -> bool:
        r"""
        @brief True, if the frame color is set

        This method is a convenience method for "has_frame_color?(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def has_frame_color(self, real: bool) -> bool:
        r"""
        @brief True, if the frame color is set
        """
    @overload
    def has_line_style(self) -> bool:
        r"""
        @brief True, if the line style is set

        This method is a convenience method for "has_line_style?(true)"

        This method has been introduced in version 0.25.
        """
    @overload
    def has_line_style(self, real: bool) -> bool:
        r"""
        @brief Gets a value indicating whether the line style is set

        This method has been introduced in version 0.25.
        """
    @overload
    def has_lower_hier_level(self) -> bool:
        r"""
        @brief Gets a value indicating whether a lower hierarchy level is explicitly specified

        This method is a convenience method for "has_lower_hier_level?(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def has_lower_hier_level(self, real: bool) -> bool:
        r"""
        @brief Gets a value indicating whether a lower hierarchy level is explicitly specified

        If "real" is true, the effective value is returned.
        """
    @overload
    def has_source_name(self) -> bool:
        r"""
        @brief Gets a value indicating whether a stream layer name is specified for this layer

        This method is a convenience method for "has_source_name?(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def has_source_name(self, real: bool) -> bool:
        r"""
        @brief Gets a value indicating whether a stream layer name is specified for this layer

        If "real" is true, the effective value is returned.
        """
    @overload
    def has_upper_hier_level(self) -> bool:
        r"""
        @brief Gets a value indicating whether an upper hierarchy level is explicitly specified

        This method is a convenience method for "has_upper_hier_level?(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def has_upper_hier_level(self, real: bool) -> bool:
        r"""
        @brief Gets a value indicating whether an upper hierarchy level is explicitly specified

        If "real" is true, the effective value is returned.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def layer_index(self) -> int:
        r"""
        @brief Gets the the layer index

        This is the index of the actual layer used. The source specification given by \source_layer, \source_datatype, \source_name is evaluated and the corresponding layer is looked up in the layout object. If a \source_layer_index is specified, this layer index is taken as the layer index to use.
        """
    @overload
    def lower_hier_level_mode(self) -> int:
        r"""
        @brief Gets the mode for the lower hierarchy level.

        This method is a convenience method for "lower_hier_level_mode(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def lower_hier_level_mode(self, arg0: bool) -> int:
        r"""
        @brief Gets the mode for the lower hierarchy level.
        @param real If true, the computed value is returned, otherwise the local node value

        The mode value can be 0 (value is given by \lower_hier_level), 1 for "minimum value" and 2 for "maximum value".

        This method has been introduced in version 0.20.
        """
    @overload
    def lower_hier_level_relative(self) -> bool:
        r"""
        @brief Gets a value indicating whether the upper hierarchy level is relative.

        This method is a convenience method for "lower_hier_level_relative(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def lower_hier_level_relative(self, real: bool) -> bool:
        r"""
        @brief Gets a value indicating whether the lower hierarchy level is relative.

        See \lower_hier_level for a description of this property.

        This method has been introduced in version 0.19.
        """
    @overload
    def set_lower_hier_level(self, level: int, relative: bool) -> None:
        r"""
        @brief Sets the lower hierarchy level and if it is relative to the context cell

        If this method is called, the lower hierarchy level is enabled. See \lower_hier_level for a description of this property.

        This method has been introduced in version 0.19.
        """
    @overload
    def set_lower_hier_level(self, level: int, relative: bool, mode: int) -> None:
        r"""
        @brief Sets the lower hierarchy level, whether it is relative to the context cell and the mode

        If this method is called, the lower hierarchy level is enabled. See \lower_hier_level for a description of this property.

        This method has been introduced in version 0.20.
        """
    @overload
    def set_upper_hier_level(self, level: int, relative: bool) -> None:
        r"""
        @brief Sets the upper hierarchy level and if it is relative to the context cell

        If this method is called, the upper hierarchy level is enabled. See \upper_hier_level for a description of this property.

        This method has been introduced in version 0.19.
        """
    @overload
    def set_upper_hier_level(self, level: int, relative: bool, mode: int) -> None:
        r"""
        @brief Sets the upper hierarchy level, if it is relative to the context cell and the mode

        If this method is called, the upper hierarchy level is enabled. See \upper_hier_level for a description of this property.

        This method has been introduced in version 0.20.
        """
    @overload
    def upper_hier_level_mode(self) -> int:
        r"""
        @brief Gets the mode for the upper hierarchy level.

        This method is a convenience method for "upper_hier_level_mode(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def upper_hier_level_mode(self, real: bool) -> int:
        r"""
        @brief Gets the mode for the upper hierarchy level.
        @param real If true, the computed value is returned, otherwise the local node value

        The mode value can be 0 (value is given by \upper_hier_level), 1 for "minimum value" and 2 for "maximum value".

        This method has been introduced in version 0.20.
        """
    @overload
    def upper_hier_level_relative(self) -> bool:
        r"""
        @brief Gets a value indicating whether the upper hierarchy level is relative.

        This method is a convenience method for "upper_hier_level_relative(true)"

        This method has been introduced in version 0.22.
        """
    @overload
    def upper_hier_level_relative(self, real: bool) -> bool:
        r"""
        @brief Gets a value indicating whether if the upper hierarchy level is relative.

        See \upper_hier_level for a description of this property.

        This method has been introduced in version 0.19.
        """

class LayerPropertiesIterator:
    r"""
    @brief Layer properties iterator

    This iterator provides a flat view for the layers in the layer tree if used with the next method. In this mode it will descend into the hierarchy and deliver node by node as a linear (flat) sequence.

    The iterator can also be used to navigate through the node hierarchy using \next_sibling, \down_first_child, \parent etc.

    The iterator also plays an important role for manipulating the layer properties tree, i.e. by specifying insertion points in the tree for the \LayoutView class.
    """
    @classmethod
    def new(cls) -> LayerPropertiesIterator:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> LayerPropertiesIterator:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> LayerPropertiesIterator:
        r"""
        @brief Creates a copy of self
        """
    def __eq__(self, other: object) -> bool:
        r"""
        @brief Equality

        @param other The other object to compare against
        Returns true, if self and other point to the same layer properties node. Caution: this does not imply that both layer properties nodes sit in the same tab. Just their position in the tree is compared.
        """
    def __init__(self) -> None:
        r"""
        @brief Creates a new object of this class
        """
    def __lt__(self, other: LayerPropertiesIterator) -> bool:
        r"""
        @brief Comparison

        @param other The other object to compare against

        @return true, if self points to an object that comes before other
        """
    def __ne__(self, other: object) -> bool:
        r"""
        @brief Inequality

        @param other The other object to compare against
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
    def assign(self, other: LayerPropertiesIterator) -> None:
        r"""
        @brief Assigns another object to self
        """
    def at_end(self) -> bool:
        r"""
        @brief At-the-end property

        This predicate is true if the iterator is at the end of either all elements or
        at the end of the child list (if \down_last_child or \down_first_child is used to iterate).
        """
    def at_top(self) -> bool:
        r"""
        @brief At-the-top property

        This predicate is true if there is no parent node above the node addressed by self.
        """
    def child_index(self) -> int:
        r"""
        @brief Returns the index of the child within the parent

        This method returns the index of that the properties node the iterator points to in the list
        of children of its parent. If the element does not have a parent, the 
        index of the element in the global list is returned.
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def current(self) -> LayerPropertiesNodeRef:
        r"""
        @brief Returns a reference to the layer properties node that the iterator points to

        Starting with version 0.25, the returned object can be manipulated and the changes will be reflected in the view immediately.
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
    def down_first_child(self) -> LayerPropertiesIterator:
        r"""
        @brief Move to the first child

        This method moves to the first child of the current element. If there is
        no child, \at_end? will be true. Even then, the iterator is sitting at the 
        the child level and \up can be used to move back.
        """
    def down_last_child(self) -> LayerPropertiesIterator:
        r"""
        @brief Move to the last child

        This method moves behind the last child of the current element. \at_end? will be
        true then. Even then, the iterator points to the child level and \up 
        can be used to move back.

        Despite the name, the iterator does not address the last child, but the position after that child. To actually get the iterator for the last child, use down_last_child and next_sibling(-1).
        """
    def dup(self) -> LayerPropertiesIterator:
        r"""
        @brief Creates a copy of self
        """
    def first_child(self) -> LayerPropertiesIterator:
        r"""
        @brief Returns the iterator pointing to the first child

        If there is no children, the iterator will be a valid insert point but not
        point to any valid element. It will report \at_end? = true.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_null(self) -> bool:
        r"""
        @brief "is null" predicate

        This predicate is true if the iterator is "null". Such an iterator can be
        created with the default constructor or by moving a top-level iterator up.
        """
    def last_child(self) -> LayerPropertiesIterator:
        r"""
        @brief Returns the iterator pointing behind the last child

        The iterator will be a valid insert point but not
        point to any valid element. It will report \at_end? = true.

        Despite the name, the iterator does not address the last child, but the position after that child. To actually get the iterator for the last child, use last_child and call next_sibling(-1) on that iterator.
        """
    def next(self) -> LayerPropertiesIterator:
        r"""
        @brief Increment operator

        The iterator will be incremented to point to the next layer entry. It will descend into the hierarchy to address child nodes if there are any.
        """
    def next_sibling(self, n: int) -> LayerPropertiesIterator:
        r"""
        @brief Move to the next sibling by a given distance


        The iterator is moved to the nth next sibling of the current element. Use negative distances to move backward.
        """
    def num_siblings(self) -> int:
        r"""
        @brief Return the number of siblings

        The count includes the current element. More precisely, this property delivers the number of children of the current node's parent.
        """
    def parent(self) -> LayerPropertiesIterator:
        r"""
        @brief Returns the iterator pointing to the parent node

        This method will return an iterator pointing to the parent element.
        If there is no parent, the returned iterator will be a null iterator.
        """
    def to_sibling(self, n: int) -> LayerPropertiesIterator:
        r"""
        @brief Move to the sibling with the given index


        The iterator is moved to the nth sibling by selecting the nth child in the current node's parent.
        """
    def up(self) -> LayerPropertiesIterator:
        r"""
        @brief Move up

        The iterator is moved to point to the current element's parent.
        If the current element does not have a parent, the iterator will
        become a null iterator.
        """

class LayerPropertiesNode(LayerProperties):
    r"""
    @brief A layer properties node structure

    This class is derived from \LayerProperties. Objects of this class are used
    in the hierarchy of layer views that are arranged in a tree while the \LayerProperties
    object reflects the properties of a single node.
    """
    expanded: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the layer tree node is expanded.
    This predicate has been introduced in version 0.28.6.
    Setter:
    @brief Set a value indicating whether the layer tree node is expanded.
    Setting this value to 'true' will expand (open) the tree node. Setting it to 'false' will collapse the node.

    This predicate has been introduced in version 0.28.6.
    """
    def __eq__(self, other: object) -> bool:
        r"""
        @brief Equality 

        @param other The other object to compare against
        """
    def __ne__(self, other: object) -> bool:
        r"""
        @brief Inequality 

        @param other The other object to compare against
        """
    def _assign(self, other: LayerProperties) -> None:
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
    def _dup(self) -> LayerPropertiesNode:
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
    @overload
    def add_child(self) -> LayerPropertiesNodeRef:
        r"""
        @brief Add a child entry
        @return A reference to the node created
        This method allows building a layer properties tree by adding children to node objects. It returns a reference to the node object created which is a freshly initialized one.

        The parameterless version of this method was introduced in version 0.25.
        """
    @overload
    def add_child(self, child: LayerProperties) -> LayerPropertiesNodeRef:
        r"""
        @brief Add a child entry
        @return A reference to the node created
        This method allows building a layer properties tree by adding children to node objects. It returns a reference to the node object created.

        This method was introduced in version 0.22.
        """
    def bbox(self) -> db.DBox:
        r"""
        @brief Compute the bbox of this layer

        This takes the layout and path definition (supported by the
        given default layout or path, if no specific is given).
        The node must have been attached to a view to make this
        operation possible.

        @return A bbox in micron units
        """
    def clear_children(self) -> None:
        r"""
        @brief Clears all children
        This method was introduced in version 0.22.
        """
    def flat(self) -> LayerPropertiesNode:
        r"""
        @brief return the "flattened" (effective) layer properties node for this node

        This method returns a \LayerPropertiesNode object that is not embedded into a hierarchy.
        This object represents the effective layer properties for the given node. In particular, all 'local' properties are identical to the 'real' properties. Such an object can be used as a basis for manipulations.

        Unlike the name suggests, this node will still contain a hierarchy of nodes below if the original node did so.
        """
    def has_children(self) -> bool:
        r"""
        @brief Test, if there are children
        """
    def id(self) -> int:
        r"""
        @brief Obtain the unique ID

        Each layer properties node object has a unique ID that is created 
        when a new LayerPropertiesNode object is instantiated. The ID is
        copied when the object is copied. The ID can be used to identify the
        object irregardless of its content.
        """
    def is_expanded(self) -> bool:
        r"""
        @brief Gets a value indicating whether the layer tree node is expanded.
        This predicate has been introduced in version 0.28.6.
        """
    def list_index(self) -> int:
        r"""
        @brief Gets the index of the layer properties list that the node lives in
        """
    def view(self) -> LayoutView:
        r"""
        @brief Gets the view this node lives in

        This reference can be nil if the node is a orphan node that lives outside a view.
        """

class LayerPropertiesNodeRef(LayerPropertiesNode):
    r"""
    @brief A class representing a reference to a layer properties node

    This object is returned by the layer properties iterator's current method (\LayerPropertiesIterator#current). A reference behaves like a layer properties node, but changes in the node are reflected in the view it is attached to.

    A typical use case for references is this:

    @code
    # Hides a layers of a view
    view = RBA::LayoutView::current
    view.each_layer do |lref|
      # lref is a LayerPropertiesNodeRef object
      lref.visible = false
    end
    @/code

    This class has been introduced in version 0.25.
    """
    def __copy__(self) -> LayerPropertiesNode:
        r"""
        @brief Creates a \LayerPropertiesNode object as a copy of the content of this node.
        This method is mainly provided for backward compatibility with 0.24 and before.
        """
    def __deepcopy__(self) -> LayerPropertiesNode:
        r"""
        @brief Creates a \LayerPropertiesNode object as a copy of the content of this node.
        This method is mainly provided for backward compatibility with 0.24 and before.
        """
    def _assign(self, other: LayerProperties) -> None:
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
    def _dup(self) -> LayerPropertiesNodeRef:
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
    @overload
    def assign(self, other: LayerProperties) -> None:
        r"""
        @brief Assigns the contents of the 'other' object to self.

        This version accepts a \LayerProperties object. Assignment will change the properties of the layer in the view.
        """
    @overload
    def assign(self, other: LayerProperties) -> None:
        r"""
        @brief Assigns the contents of the 'other' object to self.

        This version accepts a \LayerPropertiesNode object and allows modification of the layer node's hierarchy. Assignment will reconfigure the layer node in the view.
        """
    def delete(self) -> None:
        r"""
        @brief Erases the current node and all child nodes

        After erasing the node, the reference will become invalid.
        """
    def dup(self) -> LayerPropertiesNode:
        r"""
        @brief Creates a \LayerPropertiesNode object as a copy of the content of this node.
        This method is mainly provided for backward compatibility with 0.24 and before.
        """
    def is_valid(self) -> bool:
        r"""
        @brief Returns true, if the reference points to a valid layer properties node

        Invalid references behave like ordinary \LayerPropertiesNode objects but without the ability to update the view upon changes of attributes.
        """

class LayoutView(LayoutViewBase):
    r"""
    @brief The view object presenting one or more layout objects

    The visual part of the view is the tab panel in the main window. The non-visual part are the redraw thread, the layout handles, cell lists, layer view lists etc. This object controls these aspects of the view and controls the appearance of the data. 
    """
    current: ClassVar[LayoutView]
    r"""
    @brief Returns the current view
    The current view is the one that is made current by using \current=.

    This variation has been introduced for the non-Qt case in version 0.28.
    """
    on_drawing_finished_event: None
    r"""
    Getter:
    @brief An event indicating that the image is fully drawn

    This event is triggered when calling \timer. Before this event is issue, a final \on_image_updated_event may be issued.

    This event has been introduced in version 0.28.
    Setter:
    @brief An event indicating that the image is fully drawn

    This event is triggered when calling \timer. Before this event is issue, a final \on_image_updated_event may be issued.

    This event has been introduced in version 0.28.
    """
    on_image_updated_event: None
    r"""
    Getter:
    @brief An event indicating that the image ("screenshot") was updated

    This event is triggered when calling \timer.
    This event has been introduced in version 0.28.
    Setter:
    @brief An event indicating that the image ("screenshot") was updated

    This event is triggered when calling \timer.
    This event has been introduced in version 0.28.
    """
    @classmethod
    def new(cls, editable: Optional[bool] = ..., manager: Optional[db.Manager] = ..., options: Optional[int] = ...) -> LayoutView:
        r"""
        @brief Creates a standalone view

        This constructor is for special purposes only. To create a view in the context of a main window, use \MainWindow#create_view and related methods.

        @param editable True to make the view editable
        @param manager The \Manager object to enable undo/redo
        @param options A combination of the values in the LV_... constants

        This constructor has been introduced in version 0.25.
        It has been enhanced with the arguments in version 0.27.
        """
    def __init__(self, editable: Optional[bool] = ..., manager: Optional[db.Manager] = ..., options: Optional[int] = ...) -> None:
        r"""
        @brief Creates a standalone view

        This constructor is for special purposes only. To create a view in the context of a main window, use \MainWindow#create_view and related methods.

        @param editable True to make the view editable
        @param manager The \Manager object to enable undo/redo
        @param options A combination of the values in the LV_... constants

        This constructor has been introduced in version 0.25.
        It has been enhanced with the arguments in version 0.27.
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
    def timer(self) -> None:
        r"""
        @brief A callback required to be called regularily in the non-Qt case.

        This callback eventually implements the event loop in the non-Qt case. The main task is to indicate new versions of the layout image while it is drawn. When a new image has arrived, this method will issue an \on_image_updated_event. In the implementation of the latter, "screenshot" may be called to retrieve the current image.
        When drawing has finished, the \on_drawing_finished_event will be triggered.

        This method has been introduced in version 0.28.
        """

class LayoutViewBase:
    r"""
    @hide
    @alias LayoutView
    """
    class SelectionMode:
        r"""
        @brief Specifies how selected objects interact with already selected ones.

        This enum was introduced in version 0.27.
        """
        Add: ClassVar[LayoutViewBase.SelectionMode]
        r"""
        @brief Adds to any existing selection
        """
        Invert: ClassVar[LayoutViewBase.SelectionMode]
        r"""
        @brief Adds to any existing selection, if it's not there yet or removes it from the selection if it's already selected
        """
        Replace: ClassVar[LayoutViewBase.SelectionMode]
        r"""
        @brief Replaces the existing selection
        """
        Reset: ClassVar[LayoutViewBase.SelectionMode]
        r"""
        @brief Removes from any existing selection
        """
        @overload
        @classmethod
        def new(cls, i: int) -> LayoutViewBase.SelectionMode:
            r"""
            @brief Creates an enum from an integer value
            """
        @overload
        @classmethod
        def new(cls, s: str) -> LayoutViewBase.SelectionMode:
            r"""
            @brief Creates an enum from a string value
            """
        @overload
        def __eq__(self, other: object) -> bool:
            r"""
            @brief Compares an enum with an integer value
            """
        @overload
        def __eq__(self, other: object) -> bool:
            r"""
            @brief Compares two enums
            """
        @overload
        def __init__(self, i: int) -> None:
            r"""
            @brief Creates an enum from an integer value
            """
        @overload
        def __init__(self, s: str) -> None:
            r"""
            @brief Creates an enum from a string value
            """
        @overload
        def __lt__(self, other: LayoutViewBase.SelectionMode) -> bool:
            r"""
            @brief Returns true if the first enum is less (in the enum symbol order) than the second
            """
        @overload
        def __lt__(self, other: int) -> bool:
            r"""
            @brief Returns true if the enum is less (in the enum symbol order) than the integer value
            """
        @overload
        def __ne__(self, other: object) -> bool:
            r"""
            @brief Compares an enum with an integer for inequality
            """
        @overload
        def __ne__(self, other: object) -> bool:
            r"""
            @brief Compares two enums for inequality
            """
        def __repr__(self) -> str:
            r"""
            @brief Converts an enum to a visual string
            """
        def __str__(self) -> str:
            r"""
            @brief Gets the symbolic string from an enum
            """
        def inspect(self) -> str:
            r"""
            @brief Converts an enum to a visual string
            """
        def to_i(self) -> int:
            r"""
            @brief Gets the integer value from the enum
            """
        def to_s(self) -> str:
            r"""
            @brief Gets the symbolic string from an enum
            """
    Add: ClassVar[LayoutViewBase.SelectionMode]
    r"""
    @brief Adds to any existing selection
    """
    Invert: ClassVar[LayoutViewBase.SelectionMode]
    r"""
    @brief Adds to any existing selection, if it's not there yet or removes it from the selection if it's already selected
    """
    LV_Naked: ClassVar[int]
    r"""
    @brief With this option, no separate views will be provided
    Use this value with the constructor's 'options' argument.
    This option is basically equivalent to using \LV_NoLayers+\LV_NoHierarchyPanel+\LV_NoLibrariesView+\LV_NoBookmarksView

    This constant has been introduced in version 0.27.
    """
    LV_NoBookmarksView: ClassVar[int]
    r"""
    @brief With this option, no bookmarks view will be provided (see \bookmarks_frame)
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoEditorOptionsPanel: ClassVar[int]
    r"""
    @brief With this option, no editor options panel will be provided (see \editor_options_frame)
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoGrid: ClassVar[int]
    r"""
    @brief With this option, the grid background is not shown
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoHierarchyPanel: ClassVar[int]
    r"""
    @brief With this option, no cell hierarchy view will be provided (see \hierarchy_control_frame)
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoLayers: ClassVar[int]
    r"""
    @brief With this option, no layers view will be provided (see \layer_control_frame)
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoLibrariesView: ClassVar[int]
    r"""
    @brief With this option, no library view will be provided (see \libraries_frame)
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoMove: ClassVar[int]
    r"""
    @brief With this option, move operations are not supported
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoPlugins: ClassVar[int]
    r"""
    @brief With this option, all plugins are disabled
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoPropertiesPopup: ClassVar[int]
    r"""
    @brief This option disables the properties popup on double click
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.28.
    """
    LV_NoSelection: ClassVar[int]
    r"""
    @brief With this option, objects cannot be selected
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    LV_NoServices: ClassVar[int]
    r"""
    @brief This option disables all services except the ones for pure viewing
    Use this value with the constructor's 'options' argument.
    With this option, all manipulation features are disabled, except zooming.
    It is equivalent to \LV_NoMove + \LV_NoTracker + \LV_NoSelection + \LV_NoPlugins.

    This constant has been introduced in version 0.27.
    """
    LV_NoTracker: ClassVar[int]
    r"""
    @brief With this option, mouse position tracking is not supported
    Use this value with the constructor's 'options' argument.
    This option is not useful currently as no mouse tracking support is provided.

    This constant has been introduced in version 0.27.
    """
    LV_NoZoom: ClassVar[int]
    r"""
    @brief With this option, zooming is disabled
    Use this value with the constructor's 'options' argument.

    This constant has been introduced in version 0.27.
    """
    Replace: ClassVar[LayoutViewBase.SelectionMode]
    r"""
    @brief Replaces the existing selection
    """
    Reset: ClassVar[LayoutViewBase.SelectionMode]
    r"""
    @brief Removes from any existing selection
    """
    current_layer: LayerPropertiesIterator
    r"""
    Getter:
    @brief Gets the current layer view

    Returns the \LayerPropertiesIterator pointing to the current layer view (the one that has the focus). If no layer view is active currently, a null iterator is returned.

    Setter:
    @brief Sets the current layer view

    Specifies an \LayerPropertiesIterator pointing to the new current layer view.

    This method has been introduced in version 0.23.
    """
    current_layer_list: int
    r"""
    Getter:
    @brief Gets the index of the currently selected layer properties tab
    This method has been introduced in version 0.21.

    Setter:
    @brief Sets the index of the currently selected layer properties tab
    This method has been introduced in version 0.21.
    """
    @property
    def active_setview_index(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Makes the cellview with the given index the active one (shown in hierarchy browser)
        See \active_cellview_index.

        This method has been renamed from set_active_cellview_index to active_cellview_index= in version 0.25. The original name is still available, but is deprecated.
        """
    max_hier_levels: int
    r"""
    Getter:
    @brief Returns the maximum hierarchy level up to which to display geometries

    @return The maximum level up to which to display geometries
    Setter:
    @brief Sets the maximum hierarchy level up to which to display geometries

    @param level The maximum level below which to display something

    This methods allows setting the maximum hierarchy below which to display geometries.This method may cause a redraw if required.
    """
    min_hier_levels: int
    r"""
    Getter:
    @brief Returns the minimum hierarchy level at which to display geometries

    @return The minimum level at which to display geometries
    Setter:
    @brief Sets the minimum hierarchy level at which to display geometries

    @param level The minimum level above which to display something

    This methods allows setting the minimum hierarchy level above which to display geometries.This method may cause a redraw if required.
    """
    object_selection: List[ObjectInstPath]
    r"""
    Getter:
    @brief Returns a list of selected objects
    This method will deliver an array of \ObjectInstPath objects listing the selected geometrical objects. Other selected objects such as annotations and images will not be contained in that list.

    The list returned is an array of copies of \ObjectInstPath objects. They can be modified, but they will become a new selection only after re-introducing them into the view through \object_selection= or \select_object.

    Another way of obtaining the selected objects is \each_object_selected.

    This method has been introduced in version 0.24.

    Setter:
    @brief Sets the list of selected objects

    This method will set the selection of geometrical objects such as shapes and instances. It is the setter which complements the \object_selection method.

    Another way of setting the selection is through \clear_object_selection and \select_object.

    This method has been introduced in version 0.24.
    """
    on_active_cellview_changed: None
    r"""
    Getter:
    @brief An event indicating that the active cellview has changed

    If the active cellview is changed by selecting a new one from the drop-down list, this event is triggered.
    When this event is triggered, the cellview has already been changed.
    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_active_cellview_changed/remove_active_cellview_changed) have been removed in 0.25.

    Setter:
    @brief An event indicating that the active cellview has changed

    If the active cellview is changed by selecting a new one from the drop-down list, this event is triggered.
    When this event is triggered, the cellview has already been changed.
    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_active_cellview_changed/remove_active_cellview_changed) have been removed in 0.25.
    """
    on_annotation_changed: None
    r"""
    Getter:
    @brief A event indicating that an annotation has been modified
    The argument of the event is the ID of the annotation that was changed.
    This event has been added in version 0.25.

    Setter:
    @brief A event indicating that an annotation has been modified
    The argument of the event is the ID of the annotation that was changed.
    This event has been added in version 0.25.
    """
    on_annotation_selection_changed: None
    r"""
    Getter:
    @brief A event indicating that the annotation selection has changed
    This event has been added in version 0.25.

    Setter:
    @brief A event indicating that the annotation selection has changed
    This event has been added in version 0.25.
    """
    on_annotations_changed: None
    r"""
    Getter:
    @brief A event indicating that annotations have been added or removed
    This event has been added in version 0.25.

    Setter:
    @brief A event indicating that annotations have been added or removed
    This event has been added in version 0.25.
    """
    on_apply_technology: None
    r"""
    Getter:
    @brief An event indicating that a cellview has requested a new technology

    If the technology of a cellview is changed, this event is triggered.
    The integer parameter of this event will indicate the cellview that has changed.

    This event has been introduced in version 0.28.

    Setter:
    @brief An event indicating that a cellview has requested a new technology

    If the technology of a cellview is changed, this event is triggered.
    The integer parameter of this event will indicate the cellview that has changed.

    This event has been introduced in version 0.28.
    """
    on_cell_visibility_changed: None
    r"""
    Getter:
    @brief An event indicating that the visibility of one or more cells has changed

    This event is triggered after the visibility of one or more cells has changed.

    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_cell_visibility_observer/remove_cell_visibility_observer) have been removed in 0.25.

    Setter:
    @brief An event indicating that the visibility of one or more cells has changed

    This event is triggered after the visibility of one or more cells has changed.

    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_cell_visibility_observer/remove_cell_visibility_observer) have been removed in 0.25.
    """
    on_cellview_changed: None
    r"""
    Getter:
    @brief An event indicating that a cellview has changed

    If a cellview is modified, this event is triggered.
    When this event is triggered, the cellview have already been changed.
    The integer parameter of this event will indicate the cellview that has changed.

    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_cellview_observer/remove_cellview_observer) have been removed in 0.25.

    Setter:
    @brief An event indicating that a cellview has changed

    If a cellview is modified, this event is triggered.
    When this event is triggered, the cellview have already been changed.
    The integer parameter of this event will indicate the cellview that has changed.

    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_cellview_observer/remove_cellview_observer) have been removed in 0.25.
    """
    on_cellviews_changed: None
    r"""
    Getter:
    @brief An event indicating that the cellview collection has changed

    If new cellviews are added or cellviews are removed, this event is triggered.
    When this event is triggered, the cellviews have already been changed.
    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_cellview_list_observer/remove_cellview_list_observer) have been removed in 0.25.

    Setter:
    @brief An event indicating that the cellview collection has changed

    If new cellviews are added or cellviews are removed, this event is triggered.
    When this event is triggered, the cellviews have already been changed.
    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_cellview_list_observer/remove_cellview_list_observer) have been removed in 0.25.
    """
    on_current_layer_list_changed: None
    r"""
    Getter:
    @brief An event indicating the current layer list (the selected tab) has changed
    @param index The index of the new current layer list

    This event is triggered after the current layer list was changed - i.e. a new tab was selected.

    This event was introduced in version 0.25.

    Setter:
    @brief An event indicating the current layer list (the selected tab) has changed
    @param index The index of the new current layer list

    This event is triggered after the current layer list was changed - i.e. a new tab was selected.

    This event was introduced in version 0.25.
    """
    on_file_open: None
    r"""
    Getter:
    @brief An event indicating that a file was opened

    If a file is loaded, this event is triggered.
    When this event is triggered, the file was already loaded and the new file is the new active cellview.
    Despite its name, this event is also triggered if a layout object is loaded into the view.

    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_file_open_observer/remove_file_open_observer) have been removed in 0.25.

    Setter:
    @brief An event indicating that a file was opened

    If a file is loaded, this event is triggered.
    When this event is triggered, the file was already loaded and the new file is the new active cellview.
    Despite its name, this event is also triggered if a layout object is loaded into the view.

    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_file_open_observer/remove_file_open_observer) have been removed in 0.25.
    """
    on_image_changed: None
    r"""
    Getter:
    @brief A event indicating that an image has been modified
    The argument of the event is the ID of the image that was changed.
    This event has been added in version 0.25.

    Setter:
    @brief A event indicating that an image has been modified
    The argument of the event is the ID of the image that was changed.
    This event has been added in version 0.25.
    """
    on_image_selection_changed: None
    r"""
    Getter:
    @brief A event indicating that the image selection has changed
    This event has been added in version 0.25.

    Setter:
    @brief A event indicating that the image selection has changed
    This event has been added in version 0.25.
    """
    on_images_changed: None
    r"""
    Getter:
    @brief A event indicating that images have been added or removed
    This event has been added in version 0.25.

    Setter:
    @brief A event indicating that images have been added or removed
    This event has been added in version 0.25.
    """
    on_l2ndb_list_changed: None
    r"""
    Getter:
    @brief An event that is triggered the list of netlist databases is changed

    If a netlist database is added or removed, this event is triggered.

    This method has been added in version 0.26.
    Setter:
    @brief An event that is triggered the list of netlist databases is changed

    If a netlist database is added or removed, this event is triggered.

    This method has been added in version 0.26.
    """
    on_layer_list_changed: None
    r"""
    Getter:
    @brief An event indicating that the layer list has changed

    This event is triggered after the layer list has changed its configuration.
    The integer argument gives a hint about the nature of the changed:
    Bit 0 is set, if the properties (visibility, color etc.) of one or more layers have changed. Bit 1 is
    set if the hierarchy has changed. Bit 2 is set, if layer names have changed.
    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_layer_list_observer/remove_layer_list_observer) have been removed in 0.25.

    Setter:
    @brief An event indicating that the layer list has changed

    This event is triggered after the layer list has changed its configuration.
    The integer argument gives a hint about the nature of the changed:
    Bit 0 is set, if the properties (visibility, color etc.) of one or more layers have changed. Bit 1 is
    set if the hierarchy has changed. Bit 2 is set, if layer names have changed.
    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_layer_list_observer/remove_layer_list_observer) have been removed in 0.25.
    """
    on_layer_list_deleted: None
    r"""
    Getter:
    @brief An event indicating that a layer list (a tab) has been removed
    @param index The index of the layer list that was removed

    This event is triggered after the layer list has been removed - i.e. a tab was deleted.

    This event was introduced in version 0.25.

    Setter:
    @brief An event indicating that a layer list (a tab) has been removed
    @param index The index of the layer list that was removed

    This event is triggered after the layer list has been removed - i.e. a tab was deleted.

    This event was introduced in version 0.25.
    """
    on_layer_list_inserted: None
    r"""
    Getter:
    @brief An event indicating that a layer list (a tab) has been inserted
    @param index The index of the layer list that was inserted

    This event is triggered after the layer list has been inserted - i.e. a new tab was created.

    This event was introduced in version 0.25.

    Setter:
    @brief An event indicating that a layer list (a tab) has been inserted
    @param index The index of the layer list that was inserted

    This event is triggered after the layer list has been inserted - i.e. a new tab was created.

    This event was introduced in version 0.25.
    """
    on_rdb_list_changed: None
    r"""
    Getter:
    @brief An event that is triggered the list of report databases is changed

    If a report database is added or removed, this event is triggered.

    This event was translated from the Observer pattern to an event in version 0.25.
    Setter:
    @brief An event that is triggered the list of report databases is changed

    If a report database is added or removed, this event is triggered.

    This event was translated from the Observer pattern to an event in version 0.25.
    """
    on_selection_changed: None
    r"""
    Getter:
    @brief An event that is triggered if the selection is changed

    If the selection changed, this event is triggered.

    This event was translated from the Observer pattern to an event in version 0.25.
    Setter:
    @brief An event that is triggered if the selection is changed

    If the selection changed, this event is triggered.

    This event was translated from the Observer pattern to an event in version 0.25.
    """
    on_transient_selection_changed: None
    r"""
    Getter:
    @brief An event that is triggered if the transient selection is changed

    If the transient selection is changed, this event is triggered.
    The transient selection is the highlighted selection when the mouse hovers over some object(s).
    This event was translated from the Observer pattern to an event in version 0.25.
    Setter:
    @brief An event that is triggered if the transient selection is changed

    If the transient selection is changed, this event is triggered.
    The transient selection is the highlighted selection when the mouse hovers over some object(s).
    This event was translated from the Observer pattern to an event in version 0.25.
    """
    on_viewport_changed: None
    r"""
    Getter:
    @brief An event indicating that the viewport (the visible rectangle) has changed

    This event is triggered after a new display rectangle was chosen - for example, because the user zoomed into the layout.

    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_viewport_changed_observer/remove_viewport_changed_observer) have been removed in 0.25.

    Setter:
    @brief An event indicating that the viewport (the visible rectangle) has changed

    This event is triggered after a new display rectangle was chosen - for example, because the user zoomed into the layout.

    Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods (add_viewport_changed_observer/remove_viewport_changed_observer) have been removed in 0.25.
    """
    title: str
    r"""
    Getter:
    @brief Returns the view's title string

    @return The title string

    The title string is either a string composed of the file names loaded (in some "readable" manner) or a customized title string set by \set_title.
    Setter:
    @brief Sets the title of the view

    @param title The title string to use

    Override the standard title of the view indicating the file names loaded by the specified title string. The title string can be reset with \reset_title to the standard title again.
    """
    @classmethod
    def menu_symbols(cls) -> List[str]:
        r"""
        @brief Gets all available menu symbols (see \call_menu).
        NOTE: currently this method delivers a superset of all available symbols. Depending on the context, no all symbols may trigger actual functionality.

        This method has been introduced in version 0.27.
        """
    @classmethod
    def new(cls) -> LayoutViewBase:
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
    def active_cellview(self) -> CellView:
        r"""
        @brief Gets the active cellview (shown in hierarchy browser)

        This is a convenience method which is equivalent to cellview(active_cellview_index()).

        This method has been introduced in version 0.19.
        Starting from version 0.25, the returned object can be manipulated which will have an immediate effect on the display.
        """
    def active_cellview_index(self) -> int:
        r"""
        @brief Gets the index of the active cellview (shown in hierarchy browser)
        """
    def add_l2ndb(self, db: db.LayoutToNetlist) -> int:
        r"""
        @brief Adds the given netlist database to the view

        This method will add an existing database to the view. It will then appear in the netlist database browser.
        A similar method is \create_l2ndb which will create a new database within the view.

        @return The index of the database within the view (see \l2ndb)

        This method has been added in version 0.26.
        """
    @overload
    def add_line_style(self, name: str, data: int, bits: int) -> int:
        r"""
        @brief Adds a custom line style

        @param name The name under which this pattern will appear in the style editor
        @param data A bit set with the new line style pattern (bit 0 is the leftmost pixel)
        @param bits The number of bits to be used
        @return The index of the newly created style, which can be used as the line style index of \LayerProperties.
        This method has been introduced in version 0.25.
        """
    @overload
    def add_line_style(self, name: str, string: str) -> int:
        r"""
        @brief Adds a custom line style from a string

        @param name The name under which this pattern will appear in the style editor
        @param string A string describing the bits of the pattern ('.' for missing pixel, '*' for a set pixel)
        @return The index of the newly created style, which can be used as the line style index of \LayerProperties.
        This method has been introduced in version 0.25.
        """
    def add_lvsdb(self, db: db.LayoutVsSchematic) -> int:
        r"""
        @brief Adds the given database to the view

        This method will add an existing database to the view. It will then appear in the netlist database browser.
        A similar method is \create_lvsdb which will create a new database within the view.

        @return The index of the database within the view (see \lvsdb)

        This method has been added in version 0.26.
        """
    def add_missing_layers(self) -> None:
        r"""
        @brief Adds new layers to layer list
        This method was introduced in version 0.19.
        """
    def add_rdb(self, db: rdb.ReportDatabase) -> int:
        r"""
        @brief Adds the given report database to the view

        This method will add an existing database to the view. It will then appear in the marker database browser.
        A similar method is \create_rdb which will create a new database within the view.

        @return The index of the database within the view (see \rdb)

        This method has been added in version 0.26.
        """
    @overload
    def add_stipple(self, name: str, data: Sequence[int], bits: int) -> int:
        r"""
        @brief Adds a stipple pattern

        'data' is an array of unsigned integers describing the bits that make up the stipple pattern. If the array has less than 32 entries, the pattern will be repeated vertically. The number of bits used can be less than 32 bit which can be specified by the 'bits' parameter. Logically, the pattern will be put at the end of the list.

        @param name The name under which this pattern will appear in the stipple editor
        @param data See above
        @param bits See above
        @return The index of the newly created stipple pattern, which can be used as the dither pattern index of \LayerProperties.
        """
    @overload
    def add_stipple(self, name: str, string: str) -> int:
        r"""
        @brief Adds a stipple pattern given by a string

        'string' is a string describing the pattern. It consists of one or more lines composed of '.' or '*' characters and separated by newline characters. A '.' is for a missing pixel and '*' for a set pixel. The length of each line must be the same. Blanks before or after each line are ignored.

        @param name The name under which this pattern will appear in the stipple editor
        @param string See above
        @return The index of the newly created stipple pattern, which can be used as the dither pattern index of \LayerProperties.
        This method has been introduced in version 0.25.
        """
    def annotation(self, id: int) -> Annotation:
        r"""
        @brief Gets the annotation given by an ID
        Returns a reference to the annotation given by the respective ID or an invalid annotation if the ID is not valid.
        Use \Annotation#is_valid? to determine whether the returned annotation is valid or not.

        The returned annotation is a 'live' object and changing it will update the view.

        This method has been introduced in version 0.25.
        """
    def annotation_templates(self) -> List[List[Any]]:
        r"""
        @brief Gets a list of \Annotation objects representing the annotation templates.

        Annotation templates are the rulers available in the ruler drop-down (preset ruler types). This method will fetch the templates available. This method returns triplets '(annotation, title, mode)'. The first member of the triplet is the annotation object representing the template. The second member is the title string displayed in the menu for this templates. The third member is the mode value (one of the RulerMode... constants - e.g \RulerModeNormal).

        The positions of the returned annotation objects are undefined.

        This method has been introduced in version 0.28.
        """
    def ascend(self, index: int) -> db.InstElement:
        r"""
        @brief Ascends upwards in the hierarchy.

        Removes one element from the specific path of the cellview with the given index. Returns the element removed.
        """
    @overload
    def begin_layers(self) -> LayerPropertiesIterator:
        r"""
        @brief Begin iterator for the layers

        This iterator delivers the layers of this view, either in a recursive or non-recursive
        fashion, depending which iterator increment methods are used.
        The iterator delivered by \end_layers is the past-the-end iterator. It can be compared
        against a current iterator to check, if there are no further elements.

        Starting from version 0.25, an alternative solution is provided with 'each_layer' which is based on the \LayerPropertiesNodeRef class.
        """
    @overload
    def begin_layers(self, index: int) -> LayerPropertiesIterator:
        r"""
        @brief Begin iterator for the layers

        This iterator delivers the layers of this view, either in a recursive or non-recursive
        fashion, depending which iterator increment methods are used.
        The iterator delivered by \end_layers is the past-the-end iterator. It can be compared
        against a current iterator to check, if there are no further elements.
        This version addresses a specific list in a multi-tab layer properties arrangement with the "index" parameter. This method has been introduced in version 0.21.
        """
    def box(self) -> db.DBox:
        r"""
        @brief Returns the displayed box in micron space
        """
    def call_menu(self, symbol: str) -> None:
        r"""
        @brief Calls the menu item with the provided symbol.
        To obtain all symbols, use \menu_symbols.

        This method has been introduced in version 0.27.
        """
    def cancel(self) -> None:
        r"""
        @brief Cancels all edit operations

        This method will stop all pending edit operations (i.e. drag and drop) and cancel the current selection. Calling this method is useful to ensure there are no potential interactions with the script's functionality.
        """
    def cellview(self, cv_index: int) -> CellView:
        r"""
        @brief Gets the cellview object for a given index

        @param cv_index The cellview index for which to get the object for

        Starting with version 0.25, this method returns a \CellView object that can be manipulated to directly reflect any changes in the display.
        """
    def cellviews(self) -> int:
        r"""
        @brief Gets the number of cellviews
        """
    def clear_annotations(self) -> None:
        r"""
        @brief Clears all annotations on this view
        """
    def clear_config(self) -> None:
        r"""
        @brief Clears the local configuration parameters

        See \set_config for a description of the local configuration parameters.
        """
    def clear_images(self) -> None:
        r"""
        @brief Clear all images on this view
        """
    @overload
    def clear_layers(self) -> None:
        r"""
        @brief Clears all layers
        """
    @overload
    def clear_layers(self, index: int) -> None:
        r"""
        @brief Clears all layers for the given layer properties list
        This version addresses a specific list in a multi-tab layer properties arrangement with the "index" parameter. This method has been introduced in version 0.21.
        """
    def clear_line_styles(self) -> None:
        r"""
        @brief Removes all custom line styles
        All line styles except the fixed ones are removed. If any of the custom styles is still used by the layers displayed, the results will be undefined.
        This method has been introduced in version 0.25.
        """
    def clear_object_selection(self) -> None:
        r"""
        @brief Clears the selection of geometrical objects (shapes or cell instances)
        The selection of other objects (such as annotations and images) will not be affected.

        This method has been introduced in version 0.24
        """
    def clear_selection(self) -> None:
        r"""
        @brief Clears the selection of all objects (shapes, annotations, images ...)

        This method has been introduced in version 0.26.2
        """
    def clear_stipples(self) -> None:
        r"""
        @brief Removes all custom line styles
        All stipple pattern except the fixed ones are removed. If any of the custom stipple pattern is still used by the layers displayed, the results will be undefined.
        """
    def clear_transactions(self) -> None:
        r"""
        @brief Clears all transactions

        Discard all actions in the undo buffer. After clearing that buffer, no undo is available. It is important to clear the buffer when making database modifications outside transactions, i.e after that modifications have been done. If failing to do so, 'undo' operations are likely to produce invalid results.
        This method was introduced in version 0.16.
        """
    def clear_transient_selection(self) -> None:
        r"""
        @brief Clears the transient selection (mouse-over hightlights) of all objects (shapes, annotations, images ...)

        This method has been introduced in version 0.26.2
        """
    def commit(self) -> None:
        r"""
        @brief Ends a transaction

        See \transaction for a detailed description of transactions. 
        This method was introduced in version 0.16.
        """
    def commit_config(self) -> None:
        r"""
        @brief Commits the configuration settings

        Some configuration options are queued for performance reasons and become active only after 'commit_config' has been called. After a sequence of \set_config calls, this method should be called to activate the settings made by these calls.

        This method has been introduced in version 0.25.
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def create_l2ndb(self, name: str) -> int:
        r"""
        @brief Creates a new netlist database and returns the index of the new database
        @param name The name of the new netlist database
        @return The index of the new database
        This method returns an index of the new netlist database. Use \l2ndb to get the actual object. If a netlist database with the given name already exists, a unique name will be created.
        The name will be replaced by the file name when a file is loaded into the netlist database.

        This method has been added in version 0.26.
        """
    @overload
    def create_layout(self, add_cellview: bool) -> int:
        r"""
        @brief Creates a new, empty layout

        The add_cellview parameter controls whether to create a new cellview (true)
        or clear all cellviews before (false).

        This version will associate the new layout with the default technology.

        @return The index of the cellview created.
        """
    @overload
    def create_layout(self, tech: str, add_cellview: bool) -> int:
        r"""
        @brief Create a new, empty layout and associate it with the given technology

        The add_cellview parameter controls whether to create a new cellview (true)
        or clear all cellviews before (false).

        @return The index of the cellview created.

        This variant has been introduced in version 0.22.
        """
    @overload
    def create_layout(self, tech: str, add_cellview: bool, init_layers: bool) -> int:
        r"""
        @brief Create a new, empty layout and associate it with the given technology

        The add_cellview parameter controls whether to create a new cellview (true)
        or clear all cellviews before (false). This variant also allows one to control whether the layer properties are
        initialized (init_layers = true) or not (init_layers = false).

        @return The index of the cellview created.

        This variant has been introduced in version 0.22.
        """
    def create_lvsdb(self, name: str) -> int:
        r"""
        @brief Creates a new netlist database and returns the index of the new database
        @param name The name of the new netlist database
        @return The index of the new database
        This method returns an index of the new netlist database. Use \lvsdb to get the actual object. If a netlist database with the given name already exists, a unique name will be created.
        The name will be replaced by the file name when a file is loaded into the netlist database.

        This method has been added in version 0.26.
        """
    def create_measure_ruler(self, point: db.DPoint, ac: Optional[int] = ...) -> Annotation:
        r"""
        @brief Createas an auto-measure ruler at the given point.

        @param point The seed point where to create the auto-measure ruler
        @param ac The orientation constraints (determines the search direction too)

        The \ac parameters takes one of the Angle... constants from \Annotation.

        This method will create a ruler with a measurement, looking to the sides of the seed point for visible layout in the vicinity. The angle constraint determines the main directions where to look. If suitable edges are found, the method will pull a line between the closest edges. The ruler's endpoints will sit on these lines and the ruler's length will be the distance.
        Only visible layers will participate in the measurement.

        The new ruler is inserted into the view already. It is created with the default style of rulers.
        If the measurement fails because there is no layout in the vicinity, a ruler with identical start and end points will be created.

        @return The new ruler object

        This method was introduced in version 0.26.
        """
    def create_rdb(self, name: str) -> int:
        r"""
        @brief Creates a new report database and returns the index of the new database
        @param name The name of the new report database
        @return The index of the new database
        This method returns an index of the new report database. Use \rdb to get the actual object. If a report database with the given name already exists, a unique name will be created.
        The name will be replaced by the file name when a file is loaded into the report database.
        """
    @overload
    def delete_layer(self, index: int, iter: LayerPropertiesIterator) -> None:
        r"""
        @brief Deletes the layer properties node specified by the iterator

        This method deletes the object that the iterator points to and invalidates
        the iterator since the object that the iterator points to is no longer valid.
        This version addresses a specific list in a multi-tab layer properties arrangement with the "index" parameter. This method has been introduced in version 0.21.
        """
    @overload
    def delete_layer(self, iter: LayerPropertiesIterator) -> None:
        r"""
        @brief Deletes the layer properties node specified by the iterator

        This method deletes the object that the iterator points to and invalidates
        the iterator since the object that the iterator points to is no longer valid.
        """
    def delete_layer_list(self, index: int) -> None:
        r"""
        @brief Deletes the given properties list
        At least one layer properties list must remain. This method may change the current properties list.
        This method has been introduced in version 0.21.
        """
    @overload
    def delete_layers(self, index: int, iterators: Sequence[LayerPropertiesIterator]) -> None:
        r"""
        @brief Deletes the layer properties nodes specified by the iterator

        This method deletes the nodes specifies by the iterators. This method is the most convenient way to delete multiple entries.
        This version addresses a specific list in a multi-tab layer properties arrangement with the "index" parameter. This method has been introduced in version 0.22.
        """
    @overload
    def delete_layers(self, iterators: Sequence[LayerPropertiesIterator]) -> None:
        r"""
        @brief Deletes the layer properties nodes specified by the iterator

        This method deletes the nodes specifies by the iterators. This method is the most convenient way to delete multiple entries.

        This method has been added in version 0.22.
        """
    def descend(self, path: Sequence[db.InstElement], index: int) -> None:
        r"""
        @brief Descends further into the hierarchy.

        Adds the given path (given as an array of InstElement objects) to the specific path of the cellview with the given index. In effect, the cell addressed by the terminal of the new path components can be shown in the context of the upper cells, if the minimum hierarchy level is set to a negative value.
        The path is assumed to originate from the current cell and contain specific instances sorted from top to bottom.
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
    def each_annotation(self) -> Iterator[Annotation]:
        r"""
        @brief Iterates over all annotations attached to this view
        """
    def each_annotation_selected(self) -> Iterator[Annotation]:
        r"""
        @brief Iterate over each selected annotation objects, yielding a \Annotation object for each of them
        This method was introduced in version 0.19.
        """
    def each_image(self) -> Iterator[Image]:
        r"""
        @brief Iterate over all images attached to this view

        With version 0.25, the objects returned by the iterator are references and can be manipulated to change their appearance.
        """
    def each_image_selected(self) -> Iterator[Image]:
        r"""
        @brief Iterate over each selected image object, yielding a \Image object for each of them
        This method was introduced in version 0.19.
        """
    @overload
    def each_layer(self) -> Iterator[LayerPropertiesNodeRef]:
        r"""
        @brief Hierarchically iterates over the layers in the first layer list

        This iterator will recursively deliver the layers in the first layer list of the view. The objects presented by the iterator are \LayerPropertiesNodeRef objects. They can be manipulated to apply changes to the layer settings or even the hierarchy of layers:

        @code
        RBA::LayoutViewBase::current.each_layer do |lref|
          # lref is a RBA::LayerPropertiesNodeRef object
          lref.visible = false
        end
        @/code

        This method was introduced in version 0.25.
        """
    @overload
    def each_layer(self, layer_list: int) -> Iterator[LayerPropertiesNodeRef]:
        r"""
        @brief Hierarchically iterates over the layers in the given layer list

        This version of this method allows specification of the layer list to be iterated over. The layer list is specified by its index which is a value between 0 and \num_layer_lists-1.For details see the parameter-less version of this method.

        This method was introduced in version 0.25.
        """
    def each_object_selected(self) -> Iterator[ObjectInstPath]:
        r"""
        @brief Iterates over each selected geometrical object, yielding a \ObjectInstPath object for each of them

        This iterator will deliver const objects - they cannot be modified. In order to modify the selection, create a copy of the \ObjectInstPath objects, modify them and install the new selection using \select_object or \object_selection=.

        Another way of obtaining the selection is \object_selection, which returns an array of \ObjectInstPath objects.
        """
    def each_object_selected_transient(self) -> Iterator[ObjectInstPath]:
        r"""
        @brief Iterates over each geometrical objects in the transient selection, yielding a \ObjectInstPath object for each of them

        This method was introduced in version 0.18.
        """
    def enable_edits(self, enable: bool) -> None:
        r"""
        @brief Enables or disables edits

        @param enable Enable edits if set to true

        This method allows putting the view into read-only mode by disabling all edit functions. For doing so, this method has to be called with a 'false' argument. Calling it with a 'true' parameter enables all edits again. This method must not be confused with the edit/viewer mode. The LayoutView's enable_edits method is intended to temporarily disable all menu entries and functions which could allow the user to alter the database.
        In 0.25, this method has been moved from MainWindow to LayoutView.
        """
    @overload
    def end_layers(self) -> LayerPropertiesIterator:
        r"""
        @brief End iterator for the layers
        See \begin_layers for a description about this iterator
        """
    @overload
    def end_layers(self, index: int) -> LayerPropertiesIterator:
        r"""
        @brief End iterator for the layers
        See \begin_layers for a description about this iterator
        This version addresses a specific list in a multi-tab layer properties arrangement with the "index" parameter. This method has been introduced in version 0.21.
        """
    def erase_annotation(self, id: int) -> None:
        r"""
        @brief Erases the annotation given by the id
        Deletes an existing annotation given by the id parameter. The id of an annotation can be obtained through \Annotation#id.

        This method has been introduced in version 0.24.
        Starting with version 0.25, the annotation's \Annotation#delete method can also be used to delete an annotation.
        """
    def erase_cellview(self, index: int) -> None:
        r"""
        @brief Erases the cellview with the given index

        This closes the given cellview and unloads the layout associated with it, unless referred to by another cellview.
        """
    def erase_image(self, id: int) -> None:
        r"""
        @brief Erase the given image
        @param id The id of the object to erase

        Erases the image with the given Id. The Id can be obtained with if "id" method of the image object.

        This method has been introduced in version 0.20.

        With version 0.25, \Image#delete can be used to achieve the same results.
        """
    @overload
    def expand_layer_properties(self) -> None:
        r"""
        @brief Expands the layer properties for all tabs

        This method will expand all wildcard specifications in the layer properties by iterating over the specified objects (i.e. layers, cellviews) and by replacing default colors and stipples by the ones specified with the palettes.

        This method was introduced in version 0.21.
        """
    @overload
    def expand_layer_properties(self, index: int) -> None:
        r"""
        @brief Expands the layer properties for the given tab

        This method will expand all wildcard specifications in the layer properties by iterating over the specified objects (i.e. layers, cellviews) and by replacing default colors and stipples by the ones specified with the palettes.

        This method was introduced in version 0.21.
        """
    def get_config(self, name: str) -> str:
        r"""
        @brief Gets the value of a local configuration parameter

        @param name The name of the configuration parameter whose value shall be obtained (a string)

        @return The value of the parameter

        See \set_config for a description of the local configuration parameters.
        """
    def get_config_names(self) -> List[str]:
        r"""
        @brief Gets the configuration parameter names

        @return A list of configuration parameter names

        This method returns the names of all known configuration parameters. These names can be used to get and set configuration parameter values.

        This method was introduced in version 0.25.
        """
    def get_current_cell_path(self, cv_index: int) -> List[int]:
        r"""
        @brief Gets the cell path of the current cell

        The current cell is the one highlighted in the browser with the focus rectangle. The 
        current path is returned for the cellview given by cv_index.
        The cell path is a list of cell indices from the top cell to the current cell.

        @param cv_index The cellview index for which to get the current path from (usually this will be the active cellview index)
        This method is was deprecated in version 0.25 since from then, the \CellView object can be used to obtain an manipulate the selected cell.
        """
    def get_line_style(self, index: int) -> str:
        r"""
        @brief Gets the line style string for the style with the given index

        This method will return the line style string for the style with the given index.
        The format of the string is the same than the string accepted by \add_line_style.
        An empty string corresponds to 'solid line'.

        This method has been introduced in version 0.25.
        """
    def get_pixels(self, width: int, height: int) -> PixelBuffer:
        r"""
        @brief Gets the layout image as a \PixelBuffer

        @param width The width of the image to render in pixel.
        @param height The height of the image to render in pixel.

        The image contains the current scene (layout, annotations etc.).
        The image is drawn synchronously with the given width and height. Drawing may take some time. 
        This method has been introduced in 0.28.
        """
    def get_pixels_with_options(self, width: int, height: int, linewidth: Optional[int] = ..., oversampling: Optional[int] = ..., resolution: Optional[float] = ..., target: Optional[db.DBox] = ...) -> PixelBuffer:
        r"""
        @brief Gets the layout image as a \PixelBuffer (with options)

        @param width The width of the image to render in pixel.
        @param height The height of the image to render in pixel.
        @param linewidth The width of a line in pixels (usually 1) or 0 for default.
        @param oversampling The oversampling factor (1..3) or 0 for default.
        @param resolution The resolution (pixel size compared to a screen pixel size, i.e 1/oversampling) or 0 for default.
        @param target_box The box to draw or an empty box for default.

        The image contains the current scene (layout, annotations etc.).
        The image is drawn synchronously with the given width and height. Drawing may take some time. 
        This method has been introduced in 0.28.
        """
    def get_pixels_with_options_mono(self, width: int, height: int, linewidth: Optional[int] = ..., target: Optional[db.DBox] = ...) -> BitmapBuffer:
        r"""
        @brief Gets the layout image as a \PixelBuffer (with options)

        @param width The width of the image to render in pixel.
        @param height The height of the image to render in pixel.
        @param linewidth The width of a line in pixels (usually 1) or 0 for default.
        @param target_box The box to draw or an empty box for default.

        The image contains the current scene (layout, annotations etc.).
        The image is drawn synchronously with the given width and height. Drawing may take some time. Monochrome images don't have background or annotation objects currently.

        This method has been introduced in 0.28.
        """
    def get_screenshot_pixels(self) -> PixelBuffer:
        r"""
        @brief Gets a screenshot as a \PixelBuffer

        Getting the image requires the drawing to be complete. Ideally, synchronous mode is switched on for the application to guarantee this condition. The image will have the size of the viewport showing the current layout.
        This method has been introduced in 0.28.
        """
    def get_stipple(self, index: int) -> str:
        r"""
        @brief Gets the stipple pattern string for the pattern with the given index

        This method will return the stipple pattern string for the pattern with the given index.
        The format of the string is the same than the string accepted by \add_stipple.

        This method has been introduced in version 0.25.
        """
    def has_annotation_selection(self) -> bool:
        r"""
        @brief Returns true, if annotations (rulers) are selected in this view
        This method was introduced in version 0.19.
        """
    def has_image_selection(self) -> bool:
        r"""
        @brief Returns true, if images are selected in this view
        This method was introduced in version 0.19.
        """
    def has_object_selection(self) -> bool:
        r"""
        @brief Returns true, if geometrical objects (shapes or cell instances) are selected in this view
        """
    def has_selection(self) -> bool:
        r"""
        @brief Indicates whether any objects are selected

        This method has been introduced in version 0.27
        """
    def has_transient_object_selection(self) -> bool:
        r"""
        @brief Returns true, if geometrical objects (shapes or cell instances) are selected in this view in the transient selection

        The transient selection represents the objects selected when the mouse hovers over the layout windows. This selection is not used for operations but rather to indicate which object would be selected if the mouse is clicked.

        This method was introduced in version 0.18.
        """
    def hide_cell(self, cell_index: int, cv_index: int) -> None:
        r"""
        @brief Hides the given cell for the given cellview
        """
    def icon_for_layer(self, iter: LayerPropertiesIterator, w: int, h: int, dpr: float, di_off: Optional[int] = ..., no_state: Optional[bool] = ...) -> PixelBuffer:
        r"""
        @brief Creates an icon pixmap for the given layer.

        The icon will have size w times h pixels multiplied by the device pixel ratio (dpr). The dpr is The number of physical pixels per logical pixels on high-DPI displays.

        'di_off' will shift the dither pattern by the given number of (physical) pixels. If 'no_state' is true, the icon will not reflect visibility or validity states but rather the display style.

        This method has been introduced in version 0.28.
        """
    def image(self, id: int) -> Image:
        r"""
        @brief Gets the image given by an ID
        Returns a reference to the image given by the respective ID or an invalid image if the ID is not valid.
        Use \Image#is_valid? to determine whether the returned image is valid or not.

        The returned image is a 'live' object and changing it will update the view.

        This method has been introduced in version 0.25.
        """
    def init_layer_properties(self, props: LayerProperties) -> None:
        r"""
        @brief Fills the layer properties for a new layer

        This method initializes a layer properties object's color and stipples according to the defaults for the given layer source specification. The layer's source must be set already on the layer properties object.

        This method was introduced in version 0.19.

        @param props The layer properties object to initialize.
        """
    def insert_annotation(self, obj: Annotation) -> None:
        r"""
        @brief Inserts an annotation object into the given view
        Inserts a new annotation into the view. Existing annotation will remain. Use \clear_annotations to delete them before inserting new ones. Use \replace_annotation to replace an existing one with a new one. 
        Starting with version 0.25 this method modifies self's ID to reflect the ID of the ruler created. After an annotation is inserted into the view, it can be modified and the changes of properties will become reflected immediately in the view.
        """
    def insert_image(self, obj: Image) -> None:
        r"""
        @brief Insert an image object into the given view
        Insert the image object given by obj into the view.

        With version 0.25, this method will attach the image object to the view and the image object will become a 'live' object - i.e. changes to the object will change the appearance of the image on the screen.
        """
    @overload
    def insert_layer(self, index: int, iter: LayerPropertiesIterator, node: Optional[LayerProperties] = ...) -> LayerPropertiesNodeRef:
        r"""
        @brief Inserts the given layer properties node into the list before the given position

        This version addresses a specific list in a multi-tab layer properties arrangement with the "index" parameter. This method inserts the new properties node before the position given by "iter" and returns a const reference to the element created. The iterator that specified the position will remain valid after the node was inserted and will point to the newly created node. It can be used to add further nodes. 
        This method has been introduced in version 0.21.
        Since version 0.22, this method accepts LayerProperties and LayerPropertiesNode objects. A LayerPropertiesNode object can contain a hierarchy of further nodes.
        Since version 0.26 the node parameter is optional and the reference returned by this method can be used to set the properties of the new node.
        """
    @overload
    def insert_layer(self, iter: LayerPropertiesIterator, node: Optional[LayerProperties] = ...) -> LayerPropertiesNodeRef:
        r"""
        @brief Inserts the given layer properties node into the list before the given position

        This method inserts the new properties node before the position given by "iter" and returns a const reference to the element created. The iterator that specified the position will remain valid after the node was inserted and will point to the newly created node. It can be used to add further nodes. To add children to the node inserted, use iter.last_child as insertion point for the next insert operations.

        Since version 0.22, this method accepts LayerProperties and LayerPropertiesNode objects. A LayerPropertiesNode object can contain a hierarchy of further nodes.
        Since version 0.26 the node parameter is optional and the reference returned by this method can be used to set the properties of the new node.
        """
    def insert_layer_list(self, index: int) -> None:
        r"""
        @brief Inserts a new layer properties list at the given index
        This method inserts a new tab at the given position. The current layer properties list will be changed to the new list.
        This method has been introduced in version 0.21.
        """
    def is_cell_hidden(self, cell_index: int, cv_index: int) -> bool:
        r"""
        @brief Returns true, if the cell is hidden

        @return True, if the cell with "cell_index" is hidden for the cellview "cv_index"
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_editable(self) -> bool:
        r"""
        @brief Returns true if the view is in editable mode

        This read-only attribute has been added in version 0.27.5.
        """
    def is_transacting(self) -> bool:
        r"""
        @brief Indicates if a transaction is ongoing

        See \transaction for a detailed description of transactions. 
        This method was introduced in version 0.16.
        """
    def l2ndb(self, index: int) -> db.LayoutToNetlist:
        r"""
        @brief Gets the netlist database with the given index
        @return The \LayoutToNetlist object or nil if the index is not valid
        This method has been added in version 0.26.
        """
    @overload
    def load_layer_props(self, fn: str) -> None:
        r"""
        @brief Loads the layer properties

        @param fn The file name of the .lyp file to load

        Load the layer properties from the file given in "fn"
        """
    @overload
    def load_layer_props(self, fn: str, add_default: bool) -> None:
        r"""
        @brief Loads the layer properties with options

        @param fn The file name of the .lyp file to load
        @param add_default If true, default layers will be added for each other layer in the layout

        Load the layer properties from the file given in "fn".
        This version allows one to specify whether defaults should be used for all other layers by setting "add_default" to true.

        This variant has been added on version 0.21.
        """
    @overload
    def load_layer_props(self, fn: str, cv_index: int, add_default: bool) -> None:
        r"""
        @brief Loads the layer properties with options

        @param fn The file name of the .lyp file to load
        @param cv_index See description text
        @param add_default If true, default layers will be added for each other layer in the layout

        Load the layer properties from the file given in "fn".
        This version allows one to specify whether defaults should be used for all other layers by setting "add_default" to true. It can be used to load the layer properties for a specific cellview by setting "cv_index" to the index for which the layer properties file should be applied. All present definitions for this layout will be removed before the properties file is loaded. "cv_index" can be set to -1. In that case, the layer properties file is applied to each of the layouts individually.

        Note that this version will override all cellview index definitions in the layer properties file.

        This variant has been added on version 0.21.
        """
    @overload
    def load_layout(self, filename: str, add_cellview: Optional[bool] = ...) -> int:
        r"""
        @brief Loads a (new) file into the layout view

        Loads the file given by the "filename" parameter.
        The add_cellview param controls whether to create a new cellview (true)
        or clear all cellviews before (false).

        @return The index of the cellview loaded. The 'add_cellview' argument has been made optional in version 0.28.
        """
    @overload
    def load_layout(self, filename: str, options: db.LoadLayoutOptions, add_cellview: Optional[bool] = ...) -> int:
        r"""
        @brief Loads a (new) file into the layout view

        Loads the file given by the "filename" parameter.
        The options specify various options for reading the file.
        The add_cellview param controls whether to create a new cellview (true)
        or clear all cellviews before (false).

        @return The index of the cellview loaded.

        This method has been introduced in version 0.18. The 'add_cellview' argument has been made optional in version 0.28.
        """
    @overload
    def load_layout(self, filename: str, options: db.LoadLayoutOptions, technology: str, add_cellview: Optional[bool] = ...) -> int:
        r"""
        @brief Loads a (new) file into the layout view with the given technology

        Loads the file given by the "filename" parameter and associates it with the given technology.
        The options specify various options for reading the file.
        The add_cellview param controls whether to create a new cellview (true)
        or clear all cellviews before (false).

        @return The index of the cellview loaded.

        This version has been introduced in version 0.22. The 'add_cellview' argument has been made optional in version 0.28.
        """
    @overload
    def load_layout(self, filename: str, technology: str, add_cellview: Optional[bool] = ...) -> int:
        r"""
        @brief Loads a (new) file into the layout view with the given technology

        Loads the file given by the "filename" parameter and associates it with the given technology.
        The add_cellview param controls whether to create a new cellview (true)
        or clear all cellviews before (false).

        @return The index of the cellview loaded.

        This version has been introduced in version 0.22. The 'add_cellview' argument has been made optional in version 0.28.
        """
    def lvsdb(self, index: int) -> db.LayoutVsSchematic:
        r"""
        @brief Gets the netlist database with the given index
        @return The \LayoutVsSchematic object or nil if the index is not valid
        This method has been added in version 0.26.
        """
    def max_hier(self) -> None:
        r"""
        @brief Selects all hierarchy levels available

        Show the layout in full depth down to the deepest level of hierarchy. This method may cause a redraw.
        """
    def menu(self) -> AbstractMenu:
        r"""
        @brief Gets the \AbstractMenu associated with this view.

        In normal UI application mode this is the main window's view. For a detached view or in non-UI applications this is the view's private menu.

        This method has been introduced in version 0.28.
        """
    def mode_name(self) -> str:
        r"""
        @brief Gets the name of the current mode.

        See \switch_mode about a method to change the mode and \mode_names for a method to retrieve all available mode names.

        This method has been introduced in version 0.28.
        """
    def mode_names(self) -> List[str]:
        r"""
        @brief Gets the names of the available modes.

        This method allows asking the view for the available mode names for \switch_mode and for the value returned by \mode.

        This method has been introduced in version 0.28.
        """
    def num_l2ndbs(self) -> int:
        r"""
        @brief Gets the number of netlist databases loaded into this view
        @return The number of \LayoutToNetlist objects present in this view

        This method has been added in version 0.26.
        """
    def num_layer_lists(self) -> int:
        r"""
        @brief Gets the number of layer properties tabs present
        This method has been introduced in version 0.23.
        """
    def num_rdbs(self) -> int:
        r"""
        @brief Gets the number of report databases loaded into this view
        @return The number of \ReportDatabase objects present in this view
        """
    def pan_center(self, p: db.DPoint) -> None:
        r"""
        @brief Pans to the given point

        The window is positioned such that "p" becomes the new center
        """
    def pan_down(self) -> None:
        r"""
        @brief Pans down
        """
    def pan_left(self) -> None:
        r"""
        @brief Pans to the left
        """
    def pan_right(self) -> None:
        r"""
        @brief Pans to the right
        """
    def pan_up(self) -> None:
        r"""
        @brief Pans upward
        """
    def rdb(self, index: int) -> rdb.ReportDatabase:
        r"""
        @brief Gets the report database with the given index
        @return The \ReportDatabase object or nil if the index is not valid
        """
    def register_annotation_template(self, annotation: BasicAnnotation, title: str, mode: Optional[int] = ...) -> None:
        r"""
        @brief Registers the given annotation as a template for this particular view
        @annotation The annotation to use for the template (positions are ignored)
        @param title The title to use for the ruler template
        @param mode The mode the ruler will be created in (see Ruler... constants)

        See \Annotation#register_template for a method doing the same on application level. This method is hardly useful normally, but can be used when customizing layout views as individual widgets.

        This method has been added in version 0.28.
        """
    def reload_layout(self, cv: int) -> None:
        r"""
        @brief Reloads the given cellview

        @param cv The index of the cellview to reload
        """
    def remove_l2ndb(self, index: int) -> None:
        r"""
        @brief Removes a netlist database with the given index
        @param The index of the netlist database to remove from this view
        This method has been added in version 0.26.
        """
    def remove_line_style(self, index: int) -> None:
        r"""
        @brief Removes the line style with the given index
        The line styles with an index less than the first custom style. If a style is removed that is still used, the results are undefined.

        This method has been introduced in version 0.25.
        """
    def remove_rdb(self, index: int) -> None:
        r"""
        @brief Removes a report database with the given index
        @param The index of the report database to remove from this view
        """
    def remove_stipple(self, index: int) -> None:
        r"""
        @brief Removes the stipple pattern with the given index
        The pattern with an index less than the first custom pattern cannot be removed. If a stipple pattern is removed that is still used, the results are undefined. 
        """
    def remove_unused_layers(self) -> None:
        r"""
        @brief Removes unused layers from layer list
        This method was introduced in version 0.19.
        """
    def rename_cellview(self, name: str, index: int) -> None:
        r"""
        @brief Renames the cellview with the given index

        If the name is not unique, a unique name will be constructed from the name given.
        The name may be different from the filename but is associated with the layout object.
        If a layout is shared between multiple cellviews (which may happen due to a clone of the layout view
        for example), all cellviews are renamed.
        """
    def rename_layer_list(self, index: int, name: str) -> None:
        r"""
        @brief Sets the title of the given layer properties tab
        This method has been introduced in version 0.21.
        """
    def replace_annotation(self, id: int, obj: Annotation) -> None:
        r"""
        @brief Replaces the annotation given by the id with the new one
        Replaces an existing annotation given by the id parameter with the new one. The id of an annotation can be obtained through \Annotation#id.

        This method has been introduced in version 0.24.
        """
    def replace_image(self, id: int, new_obj: Image) -> None:
        r"""
        @brief Replace an image object with the new image

        @param id The id of the object to replace
        @param new_obj The new object to replace the old one

        Replaces  the image with the given Id with the new object. The Id can be obtained with if "id" method of the image object.

        This method has been introduced in version 0.20.
        """
    def replace_l2ndb(self, db_index: int, db: db.LayoutToNetlist) -> int:
        r"""
        @brief Replaces the netlist database with the given index

        If the index is not valid, the database will be added to the view (see \add_lvsdb).

        @return The index of the database within the view (see \lvsdb)

        This method has been added in version 0.26.
        """
    @overload
    def replace_layer_node(self, index: int, iter: LayerPropertiesIterator, node: LayerProperties) -> None:
        r"""
        @brief Replaces the layer node at the position given by "iter" with a new one
        This version addresses a specific list in a multi-tab layer properties arrangement with the "index" parameter. 
        This method has been introduced in version 0.21.
        Since version 0.22, this method accepts LayerProperties and LayerPropertiesNode objects. A LayerPropertiesNode object can contain a hierarchy of further nodes.
        """
    @overload
    def replace_layer_node(self, iter: LayerPropertiesIterator, node: LayerProperties) -> None:
        r"""
        @brief Replaces the layer node at the position given by "iter" with a new one

        Since version 0.22, this method accepts LayerProperties and LayerPropertiesNode objects. A LayerPropertiesNode object can contain a hierarchy of further nodes.
        """
    def replace_lvsdb(self, db_index: int, db: db.LayoutVsSchematic) -> int:
        r"""
        @brief Replaces the database with the given index

        If the index is not valid, the database will be added to the view (see \add_lvsdb).

        @return The index of the database within the view (see \lvsdb)

        This method has been added in version 0.26.
        """
    def replace_rdb(self, db_index: int, db: rdb.ReportDatabase) -> int:
        r"""
        @brief Replaces the report database with the given index

        If the index is not valid, the database will be added to the view (see \add_rdb).

        @return The index of the database within the view (see \rdb)

        This method has been added in version 0.26.
        """
    def reset_title(self) -> None:
        r"""
        @brief Resets the title to the standard title

        See \set_title and \title for a description about how titles are handled.
        """
    def resize(self, arg0: int, arg1: int) -> None:
        r"""
        @brief Resizes the layout view to the given dimension

        This method has been made available in all builds in 0.28.
        """
    @overload
    def save_as(self, index: int, filename: str, gzip: bool, options: db.SaveLayoutOptions) -> None:
        r"""
        @brief Saves a layout to the given stream file

        @param index The cellview index of the layout to save.
        @param filename The file to write.
        @param gzip Ignored.
        @param options Writer options.

        The layout with the given index is written to the stream file with the given options. 'options' is a \SaveLayoutOptions object that specifies which format to write and further options such as scaling factor etc.
        Calling this method is equivalent to calling 'write' on the respective layout object.

        This method is deprecated starting from version 0.23. The compression mode is determined from the file name automatically and the \gzip parameter is ignored.
        """
    @overload
    def save_as(self, index: int, filename: str, options: db.SaveLayoutOptions) -> None:
        r"""
        @brief Saves a layout to the given stream file

        @param index The cellview index of the layout to save.
        @param filename The file to write.
        @param options Writer options.

        The layout with the given index is written to the stream file with the given options. 'options' is a \SaveLayoutOptions object that specifies which format to write and further options such as scaling factor etc.
        Calling this method is equivalent to calling 'write' on the respective layout object.

        If the file name ends with a suffix ".gz" or ".gzip", the file is compressed with the zlib algorithm.
        """
    def save_image(self, filename: str, width: int, height: int) -> None:
        r"""
        @brief Saves the layout as an image to the given file

        @param filename The file to which to write the screenshot to.
        @param width The width of the image to render in pixel.
        @param height The height of the image to render in pixel.

        The image contains the current scene (layout, annotations etc.).
        The image is written as a PNG file to the given file. The image is drawn synchronously with the given width and height. Drawing may take some time. 
        """
    def save_image_with_options(self, filename: str, width: int, height: int, linewidth: Optional[int] = ..., oversampling: Optional[int] = ..., resolution: Optional[float] = ..., target: Optional[db.DBox] = ..., monochrome: Optional[bool] = ...) -> None:
        r"""
        @brief Saves the layout as an image to the given file (with options)

        @param filename The file to which to write the screenshot to.
        @param width The width of the image to render in pixel.
        @param height The height of the image to render in pixel.
        @param linewidth The line width scale factor (usually 1) or 0 for 1/resolution.
        @param oversampling The oversampling factor (1..3) or 0 for the oversampling the view was configured with.
        @param resolution The resolution (pixel size compared to a screen pixel) or 0 for 1/oversampling.
        @param target_box The box to draw or an empty box for default.
        @param monochrome If true, monochrome images will be produced.

        The image contains the current scene (layout, annotations etc.).
        The image is written as a PNG file to the given file. The image is drawn synchronously with the given width and height. Drawing may take some time. Monochrome images don't have background or annotation objects currently.

        The 'linewidth' factor scales the layout style line widths.

        The 'oversampling' factor will use multiple passes passes to create a single image pixels. An oversampling factor of 2 uses 2x2 virtual pixels to generate an output pixel. This results in a smoother image. This however comes with a corresponding memory and run time penalty. When using oversampling, you can set linewidth and resolution to 0. This way, line widths and stipple pattern are scaled such that the resulting image is equivalent to the standard image.

        The 'resolution' is the pixel size used to translate font sizes and stipple pattern. A resolution of 0.5 renders twice as large fonts and stipple pattern. When combining this value with an oversampling factor of 2 and a line width factor of 2, the resulting image is an oversampled version of the standard image.

        Examples:

        @code
        # standard image 500x500 pixels (oversampling as configured in the view)
        layout_view.save_image_with_options("image.png", 500, 500)

        # 2x oversampled image with 500x500 pixels
        layout_view.save_image_with_options("image.png", 500, 500, 0, 2, 0)

        # 2x scaled image with 1000x1000 pixels
        layout_view.save_image_with_options("image.png", 1000, 1000, 2, 1, 0.5)
        @/code

        This method has been introduced in 0.23.10.
        """
    def save_layer_props(self, fn: str) -> None:
        r"""
        @brief Saves the layer properties

        Save the layer properties to the file given in "fn"
        """
    def save_screenshot(self, filename: str) -> None:
        r"""
        @brief Saves a screenshot to the given file

        @param filename The file to which to write the screenshot to.

        The screenshot is written as a PNG file to the given file. This requires the drawing to be complete. Ideally, synchronous mode is switched on for the application to guarantee this condition. The image will have the size of the viewport showing the current layout.
        """
    def select_all(self) -> None:
        r"""
        @brief Selects all objects from the view

        This method has been introduced in version 0.27
        """
    def select_cell(self, cell_index: int, cv_index: int) -> None:
        r"""
        @brief Selects a cell by index for a certain cell view

        Select the current (top) cell by specifying a path (a list of cell indices from top to the actual cell) and the cellview index for which this cell should become the currently shown one.
        This method selects the cell to be drawn. In constrast, the \set_current_cell_path method selects the cell that is highlighted in the cell tree (but not necessarily drawn).
        This method is was deprecated in version 0.25 since from then, the \CellView object can be used to obtain an manipulate the selected cell.
        """
    def select_cell_path(self, cell_index: Sequence[int], cv_index: int) -> None:
        r"""
        @brief Selects a cell by cell index for a certain cell view

        Select the current (top) cell by specifying a cell indexand the cellview index for which this cell should become the currently shown one. The path to the cell is constructed by selecting one that leads to a top cell.
        This method selects the cell to be drawn. In constrast, the \set_current_cell_path method selects the cell that is highlighted in the cell tree (but not necessarily drawn).
        This method is was deprecated in version 0.25 since from then, the \CellView object can be used to obtain an manipulate the selected cell.
        """
    @overload
    def select_from(self, box: db.DBox, mode: Optional[LayoutViewBase.SelectionMode] = ...) -> None:
        r"""
        @brief Selects the objects from a given box

        The mode indicates whether to add to the selection, replace the selection, remove from selection or invert the selected status of the objects found inside the given box.

        This method has been introduced in version 0.27
        """
    @overload
    def select_from(self, point: db.DPoint, mode: Optional[LayoutViewBase.SelectionMode] = ...) -> None:
        r"""
        @brief Selects the objects from a given point

        The mode indicates whether to add to the selection, replace the selection, remove from selection or invert the selected status of the objects found around the given point.

        This method has been introduced in version 0.27
        """
    def select_object(self, obj: ObjectInstPath) -> None:
        r"""
        @brief Adds the given selection to the list of selected objects

        The selection provided by the \ObjectInstPath descriptor is added to the list of selected objects.
        To clear the previous selection, use \clear_object_selection.

        The selection of other objects (such as annotations and images) will not be affected.

        Another way of selecting objects is \object_selection=.

        This method has been introduced in version 0.24
        """
    def selected_cells_paths(self, cv_index: int) -> List[List[int]]:
        r"""
        @brief Gets the paths of the selected cells

        Gets a list of cell paths to the cells selected in the cellview given by \cv_index. The "selected cells" are the ones selected in the cell list or cell tree. This is not the "current cell" which is the one that is shown in the layout window.

        The cell paths are arrays of cell indexes where the last element is the actual cell selected.

        This method has be introduced in version 0.25.
        """
    def selected_layers(self) -> List[LayerPropertiesIterator]:
        r"""
        @brief Gets the selected layers

        Returns an array of \LayerPropertiesIterator objects pointing to the currently selected layers. If no layer view is selected currently, an empty array is returned.
        """
    def selection_bbox(self) -> db.DBox:
        r"""
        @brief Returns the bounding box of the current selection

        This method has been introduced in version 0.26.2
        """
    def selection_size(self) -> int:
        r"""
        @brief Returns the number of selected objects

        This method has been introduced in version 0.27
        """
    def send_enter_event(self) -> None:
        r"""
        @brief Sends a mouse window leave event

        This method is intended to emulate the mouse mouse window leave events sent by Qt normally in environments where Qt is not present. 
        This method was introduced in version 0.28.
        """
    def send_key_press_event(self, key: int, buttons: int) -> None:
        r"""
        @brief Sends a key press event

        This method is intended to emulate the key press events sent by Qt normally in environments where Qt is not present. The arguments follow the conventions used within \Plugin#key_event for example.

        This method was introduced in version 0.28.
        """
    def send_leave_event(self) -> None:
        r"""
        @brief Sends a mouse window leave event

        This method is intended to emulate the mouse mouse window leave events sent by Qt normally in environments where Qt is not present. 
        This method was introduced in version 0.28.
        """
    def send_mouse_double_clicked_event(self, pt: db.DPoint, buttons: int) -> None:
        r"""
        @brief Sends a mouse button double-click event

        This method is intended to emulate the mouse button double-click events sent by Qt normally in environments where Qt is not present. The arguments follow the conventions used within \Plugin#mouse_moved_event for example.

        This method was introduced in version 0.28.
        """
    def send_mouse_move_event(self, pt: db.DPoint, buttons: int) -> None:
        r"""
        @brief Sends a mouse move event

        This method is intended to emulate the mouse move events sent by Qt normally in environments where Qt is not present. The arguments follow the conventions used within \Plugin#mouse_moved_event for example.

        This method was introduced in version 0.28.
        """
    def send_mouse_press_event(self, pt: db.DPoint, buttons: int) -> None:
        r"""
        @brief Sends a mouse button press event

        This method is intended to emulate the mouse button press events sent by Qt normally in environments where Qt is not present. The arguments follow the conventions used within \Plugin#mouse_moved_event for example.

        This method was introduced in version 0.28.
        """
    def send_mouse_release_event(self, pt: db.DPoint, buttons: int) -> None:
        r"""
        @brief Sends a mouse button release event

        This method is intended to emulate the mouse button release events sent by Qt normally in environments where Qt is not present. The arguments follow the conventions used within \Plugin#mouse_moved_event for example.

        This method was introduced in version 0.28.
        """
    def send_wheel_event(self, delta: int, horizontal: bool, pt: db.DPoint, buttons: int) -> None:
        r"""
        @brief Sends a mouse wheel event

        This method is intended to emulate the mouse wheel events sent by Qt normally in environments where Qt is not present. The arguments follow the conventions used within \Plugin#wheel_event for example.

        This method was introduced in version 0.28.
        """
    def set_active_cellview_index(self, index: int) -> None:
        r"""
        @brief Makes the cellview with the given index the active one (shown in hierarchy browser)
        See \active_cellview_index.

        This method has been renamed from set_active_cellview_index to active_cellview_index= in version 0.25. The original name is still available, but is deprecated.
        """
    def set_config(self, name: str, value: str) -> None:
        r"""
        @brief Sets a local configuration parameter with the given name to the given value

        @param name The name of the configuration parameter to set
        @param value The value to which to set the configuration parameter

        This method sets a local configuration parameter with the given name to the given value. Values can only be strings. Numerical values have to be converted into strings first. Local configuration parameters override global configurations for this specific view. This allows for example to override global settings of background colors. Any local settings are not written to the configuration file. 
        """
    def set_current_cell_path(self, cv_index: int, cell_path: Sequence[int]) -> None:
        r"""
        @brief Sets the path to the current cell

        The current cell is the one highlighted in the browser with the focus rectangle. The
        cell given by the path is highlighted and scrolled into view.
        To select the cell to be drawn, use the \select_cell or \select_cell_path method.

        @param cv_index The cellview index for which to set the current path for (usually this will be the active cellview index)
        @param path The path to the current cell

        This method is was deprecated in version 0.25 since from then, the \CellView object can be used to obtain an manipulate the selected cell.
        """
    def set_current_layer_list(self, index: int) -> None:
        r"""
        @brief Sets the index of the currently selected layer properties tab
        This method has been introduced in version 0.21.
        """
    @overload
    def set_layer_properties(self, index: int, iter: LayerPropertiesIterator, props: LayerProperties) -> None:
        r"""
        @brief Sets the layer properties of the layer pointed to by the iterator

        This method replaces the layer properties of the element pointed to by "iter" by the properties given by "props" in the tab given by "index". It will not change the hierarchy but just the properties of the given node.This version addresses a specific list in a multi-tab layer properties arrangement with the "index" parameter. This method has been introduced in version 0.21.
        """
    @overload
    def set_layer_properties(self, iter: LayerPropertiesIterator, props: LayerProperties) -> None:
        r"""
        @brief Sets the layer properties of the layer pointed to by the iterator

        This method replaces the layer properties of the element pointed to by "iter" by the properties given by "props". It will not change the hierarchy but just the properties of the given node.
        """
    def set_title(self, title: str) -> None:
        r"""
        @brief Sets the title of the view

        @param title The title string to use

        Override the standard title of the view indicating the file names loaded by the specified title string. The title string can be reset with \reset_title to the standard title again.
        """
    @overload
    def show_all_cells(self) -> None:
        r"""
        @brief Makes all cells shown (cancel effects of \hide_cell)
        """
    @overload
    def show_all_cells(self, cv_index: int) -> None:
        r"""
        @brief Makes all cells shown (cancel effects of \hide_cell) for the specified cell view
        Unlike \show_all_cells, this method will only clear the hidden flag on the cell view selected by \cv_index.

        This variant has been added in version 0.25.
        """
    def show_cell(self, cell_index: int, cv_index: int) -> None:
        r"""
        @brief Shows the given cell for the given cellview (cancel effect of \hide_cell)
        """
    def show_image(self, id: int, visible: bool) -> None:
        r"""
        @brief Shows or hides the given image
        @param id The id of the object to show or hide
        @param visible True, if the image should be shown

        Sets the visibility of the image with the given Id. The Id can be obtained with if "id" method of the image object.

        This method has been introduced in version 0.20.

        With version 0.25, \Image#visible= can be used to achieve the same results.
        """
    @overload
    def show_layout(self, layout: db.Layout, add_cellview: bool) -> int:
        r"""
        @brief Shows an existing layout in the view

        Shows the given layout in the view. If add_cellview is true, the new layout is added to the list of cellviews in the view.

        Note: once a layout is passed to the view with show_layout, it is owned by the view and must not be destroyed with the 'destroy' method.

        @return The index of the cellview created.

        This method has been introduced in version 0.22.
        """
    @overload
    def show_layout(self, layout: db.Layout, tech: str, add_cellview: bool) -> int:
        r"""
        @brief Shows an existing layout in the view

        Shows the given layout in the view. If add_cellview is true, the new layout is added to the list of cellviews in the view.
        The technology to use for that layout can be specified as well with the 'tech' parameter. Depending on the definition of the technology, layer properties may be loaded for example.
        The technology string can be empty for the default technology.

        Note: once a layout is passed to the view with show_layout, it is owned by the view and must not be destroyed with the 'destroy' method.

        @return The index of the cellview created.

        This method has been introduced in version 0.22.
        """
    @overload
    def show_layout(self, layout: db.Layout, tech: str, add_cellview: bool, init_layers: bool) -> int:
        r"""
        @brief Shows an existing layout in the view

        Shows the given layout in the view. If add_cellview is true, the new layout is added to the list of cellviews in the view.
        The technology to use for that layout can be specified as well with the 'tech' parameter. Depending on the definition of the technology, layer properties may be loaded for example.
        The technology string can be empty for the default technology.
        This variant also allows one to control whether the layer properties are
        initialized (init_layers = true) or not (init_layers = false).

        Note: once a layout is passed to the view with show_layout, it is owned by the view and must not be destroyed with the 'destroy' method.

        @return The index of the cellview created.

        This method has been introduced in version 0.22.
        """
    def stop(self) -> None:
        r"""
        @brief Stops redraw thread and close any browsers
        This method usually does not need to be called explicitly. The redraw thread is stopped automatically.
        """
    def stop_redraw(self) -> None:
        r"""
        @brief Stops the redraw thread

        It is very important to stop the redraw thread before applying changes to the layout or the cell views and the LayoutView configuration. This is usually done automatically. For rare cases, where this is not the case, this method is provided.
        """
    def switch_mode(self, arg0: str) -> None:
        r"""
        @brief Switches the mode.

        See \mode_name about a method to get the name of the current mode and \mode_names for a method to retrieve all available mode names.

        This method has been introduced in version 0.28.
        """
    def transaction(self, description: str) -> None:
        r"""
        @brief Begins a transaction

        @param description A text that appears in the 'undo' description

        A transaction brackets a sequence of database modifications that appear as a single undo action. Only modifications that are wrapped inside a transaction..commit call pair can be undone.
        Each transaction must be terminated with a \commit method call, even if some error occurred. It is advisable therefore to catch errors and issue a commit call in this case.

        This method was introduced in version 0.16.
        """
    def transient_to_selection(self) -> None:
        r"""
        @brief Turns the transient selection into the actual selection

        The current selection is cleared before. All highlighted objects under the mouse will become selected. This applies to all types of objects (rulers, shapes, images ...).

        This method has been introduced in version 0.26.2
        """
    def unregister_annotation_templates(self, category: str) -> None:
        r"""
        @brief Unregisters the template or templates with the given category string on this particular view

        See \Annotation#unregister_templates for a method doing the same on application level.This method is hardly useful normally, but can be used when customizing layout views as individual widgets.

        This method has been added in version 0.28.
        """
    def unselect_object(self, obj: ObjectInstPath) -> None:
        r"""
        @brief Removes the given selection from the list of selected objects

        The selection provided by the \ObjectInstPath descriptor is removed from the list of selected objects.
        If the given object was not part of the selection, nothing will be changed.
        The selection of other objects (such as annotations and images) will not be affected.

        This method has been introduced in version 0.24
        """
    def update_content(self) -> None:
        r"""
        @brief Updates the layout view to the current state

        This method triggers an update of the hierarchy tree and layer view tree. Usually, this method does not need to be called. The widgets are updated automatically in most cases.

        Currently, this method should be called however, after the layer view tree has been changed by the \insert_layer, \replace_layer_node or \delete_layer methods.
        """
    def viewport_height(self) -> int:
        r"""
        @brief Return the viewport height in pixels
        This method was introduced in version 0.18.
        """
    def viewport_trans(self) -> db.DCplxTrans:
        r"""
        @brief Returns the transformation that converts micron coordinates to pixels
        Hint: the transformation returned will convert any point in micron coordinate space into a pixel coordinate. Contrary to usual convention, the y pixel coordinate is given in a mathematically oriented space - which means the bottom coordinate is 0.
        This method was introduced in version 0.18.
        """
    def viewport_width(self) -> int:
        r"""
        @brief Returns the viewport width in pixels
        This method was introduced in version 0.18.
        """
    def zoom_box(self, box: db.DBox) -> None:
        r"""
        @brief Sets the viewport to the given box

        @param box The box to which to set the view in micron coordinates
        """
    def zoom_fit(self) -> None:
        r"""
        @brief Fits the contents of the current view into the window
        """
    def zoom_fit_sel(self) -> None:
        r"""
        @brief Fits the contents of the current selection into the window

        This method has been introduced in version 0.25.
        """
    def zoom_in(self) -> None:
        r"""
        @brief Zooms in somewhat
        """
    def zoom_out(self) -> None:
        r"""
        @brief Zooms out somewhat
        """

class Macro:
    r"""
    @brief A macro class

    This class is provided mainly to support generation of template macros in the DSL interpreter framework provided by \MacroInterpreter. The implementation may be enhanced in future versions and provide access to macros stored inside KLayout's macro repository.
    But it can be used to execute macro code in a consistent way:

    @code
    path = "path-to-macro.lym"
    RBA::Macro::new(path).run()
    @/code

    Using the Macro class with \run for executing code will chose the right interpreter and is able to execute DRC and LVS scripts in the proper environment. This also provides an option to execute Ruby code from Python and vice versa.

    In this scenario you can pass values to the script using \Interpreter#define_variable. The interpreter to choose for DRC and LVS scripts is \Interpreter#ruby_interpreter. For passing values back from the script, wrap the variable value into a \Value object which can be modified by the called script and read back by the caller.
    """
    class Format:
        r"""
        @brief Specifies the format of a macro
        This enum has been introduced in version 0.27.5.
        """
        MacroFormat: ClassVar[Macro.Format]
        r"""
        @brief The macro has macro (XML) format
        """
        PlainTextFormat: ClassVar[Macro.Format]
        r"""
        @brief The macro has plain text format
        """
        PlainTextWithHashAnnotationsFormat: ClassVar[Macro.Format]
        r"""
        @brief The macro has plain text format with special pseudo-comment annotations
        """
        @overload
        @classmethod
        def new(cls, i: int) -> Macro.Format:
            r"""
            @brief Creates an enum from an integer value
            """
        @overload
        @classmethod
        def new(cls, s: str) -> Macro.Format:
            r"""
            @brief Creates an enum from a string value
            """
        @overload
        def __eq__(self, other: object) -> bool:
            r"""
            @brief Compares an enum with an integer value
            """
        @overload
        def __eq__(self, other: object) -> bool:
            r"""
            @brief Compares two enums
            """
        @overload
        def __init__(self, i: int) -> None:
            r"""
            @brief Creates an enum from an integer value
            """
        @overload
        def __init__(self, s: str) -> None:
            r"""
            @brief Creates an enum from a string value
            """
        @overload
        def __lt__(self, other: Macro.Format) -> bool:
            r"""
            @brief Returns true if the first enum is less (in the enum symbol order) than the second
            """
        @overload
        def __lt__(self, other: int) -> bool:
            r"""
            @brief Returns true if the enum is less (in the enum symbol order) than the integer value
            """
        @overload
        def __ne__(self, other: object) -> bool:
            r"""
            @brief Compares two enums for inequality
            """
        @overload
        def __ne__(self, other: object) -> bool:
            r"""
            @brief Compares an enum with an integer for inequality
            """
        def __repr__(self) -> str:
            r"""
            @brief Converts an enum to a visual string
            """
        def __str__(self) -> str:
            r"""
            @brief Gets the symbolic string from an enum
            """
        def inspect(self) -> str:
            r"""
            @brief Converts an enum to a visual string
            """
        def to_i(self) -> int:
            r"""
            @brief Gets the integer value from the enum
            """
        def to_s(self) -> str:
            r"""
            @brief Gets the symbolic string from an enum
            """
    class Interpreter:
        r"""
        @brief Specifies the interpreter used for executing a macro
        This enum has been introduced in version 0.27.5.
        """
        DSLInterpreter: ClassVar[Macro.Interpreter]
        r"""
        @brief A domain-specific interpreter (DSL)
        """
        None_: ClassVar[Macro.Interpreter]
        r"""
        @brief No specific interpreter
        """
        Python: ClassVar[Macro.Interpreter]
        r"""
        @brief The interpreter is Python
        """
        Ruby: ClassVar[Macro.Interpreter]
        r"""
        @brief The interpreter is Ruby
        """
        Text: ClassVar[Macro.Interpreter]
        r"""
        @brief Plain text
        """
        @overload
        @classmethod
        def new(cls, i: int) -> Macro.Interpreter:
            r"""
            @brief Creates an enum from an integer value
            """
        @overload
        @classmethod
        def new(cls, s: str) -> Macro.Interpreter:
            r"""
            @brief Creates an enum from a string value
            """
        @overload
        def __eq__(self, other: object) -> bool:
            r"""
            @brief Compares an enum with an integer value
            """
        @overload
        def __eq__(self, other: object) -> bool:
            r"""
            @brief Compares two enums
            """
        @overload
        def __init__(self, i: int) -> None:
            r"""
            @brief Creates an enum from an integer value
            """
        @overload
        def __init__(self, s: str) -> None:
            r"""
            @brief Creates an enum from a string value
            """
        @overload
        def __lt__(self, other: Macro.Interpreter) -> bool:
            r"""
            @brief Returns true if the first enum is less (in the enum symbol order) than the second
            """
        @overload
        def __lt__(self, other: int) -> bool:
            r"""
            @brief Returns true if the enum is less (in the enum symbol order) than the integer value
            """
        @overload
        def __ne__(self, other: object) -> bool:
            r"""
            @brief Compares an enum with an integer for inequality
            """
        @overload
        def __ne__(self, other: object) -> bool:
            r"""
            @brief Compares two enums for inequality
            """
        def __repr__(self) -> str:
            r"""
            @brief Converts an enum to a visual string
            """
        def __str__(self) -> str:
            r"""
            @brief Gets the symbolic string from an enum
            """
        def inspect(self) -> str:
            r"""
            @brief Converts an enum to a visual string
            """
        def to_i(self) -> int:
            r"""
            @brief Gets the integer value from the enum
            """
        def to_s(self) -> str:
            r"""
            @brief Gets the symbolic string from an enum
            """
    DSLInterpreter: ClassVar[Macro.Interpreter]
    r"""
    @brief A domain-specific interpreter (DSL)
    """
    MacroFormat: ClassVar[Macro.Format]
    r"""
    @brief The macro has macro (XML) format
    """
    None_: ClassVar[Macro.Interpreter]
    r"""
    @brief No specific interpreter
    """
    PlainTextFormat: ClassVar[Macro.Format]
    r"""
    @brief The macro has plain text format
    """
    PlainTextWithHashAnnotationsFormat: ClassVar[Macro.Format]
    r"""
    @brief The macro has plain text format with special pseudo-comment annotations
    """
    Python: ClassVar[Macro.Interpreter]
    r"""
    @brief The interpreter is Python
    """
    Ruby: ClassVar[Macro.Interpreter]
    r"""
    @brief The interpreter is Ruby
    """
    Text: ClassVar[Macro.Interpreter]
    r"""
    @brief Plain text
    """
    category: str
    r"""
    Getter:
    @brief Gets the category tags

    The category tags string indicates to which categories a macro will belong to. This string is only used for templates currently and is a comma-separated list of category names.
    Setter:
    @brief Sets the category tags string
    See \category for details.
    """
    description: str
    r"""
    Getter:
    @brief Gets the description text

    The description text of a macro will appear in the macro list. If used as a macro template, the description text can have the format "Group;;Description". In that case, the macro will appear in a group with title "Group".
    Setter:
    @brief Sets the description text
    @param description The description text.
    See \description for details.
    """
    doc: str
    r"""
    Getter:
    @brief Gets the macro's documentation string

    This method has been introduced in version 0.27.5.

    Setter:
    @brief Sets the macro's documentation string

    This method has been introduced in version 0.27.5.
    """
    dsl_interpreter: str
    r"""
    Getter:
    @brief Gets the macro's DSL interpreter name (if interpreter is DSLInterpreter)

    This method has been introduced in version 0.27.5.

    Setter:
    @brief Sets the macro's DSL interpreter name (if interpreter is DSLInterpreter)

    This method has been introduced in version 0.27.5.
    """
    epilog: str
    r"""
    Getter:
    @brief Gets the epilog code

    The epilog is executed after the actual code is executed. Interpretation depends on the implementation of the DSL interpreter for DSL macros.
    Setter:
    @brief Sets the epilog
    See \epilog for details.
    """
    format: Macro.Format
    r"""
    Getter:
    @brief Gets the macro's storage format

    This method has been introduced in version 0.27.5.

    Setter:
    @brief Sets the macro's storage format

    This method has been introduced in version 0.27.5.
    """
    group_name: str
    r"""
    Getter:
    @brief Gets the menu group name

    If a group name is specified and \show_in_menu? is true, the macro will appear in a separate group (separated by a separator) together with other macros sharing the same group.
    Setter:
    @brief Sets the menu group name
    See \group_name for details.
    """
    interpreter: Macro.Interpreter
    r"""
    Getter:
    @brief Gets the macro's interpreter

    This method has been introduced in version 0.27.5.

    Setter:
    @brief Sets the macro's interpreter

    This method has been introduced in version 0.27.5.
    """
    is_autorun: bool
    r"""
    Getter:
    @brief Gets a flag indicating whether the macro is automatically executed on startup

    This method has been introduced in version 0.27.5.

    Setter:
    @brief Sets a flag indicating whether the macro is automatically executed on startup

    This method has been introduced in version 0.27.5.
    """
    is_autorun_early: bool
    r"""
    Getter:
    @brief Gets a flag indicating whether the macro is automatically executed early on startup

    This method has been introduced in version 0.27.5.

    Setter:
    @brief Sets a flag indicating whether the macro is automatically executed early on startup

    This method has been introduced in version 0.27.5.
    """
    menu_path: str
    r"""
    Getter:
    @brief Gets the menu path

    If a menu path is specified and \show_in_menu? is true, the macro will appear in the menu at the specified position.
    Setter:
    @brief Sets the menu path
    See \menu_path for details.
    """
    prolog: str
    r"""
    Getter:
    @brief Gets the prolog code

    The prolog is executed before the actual code is executed. Interpretation depends on the implementation of the DSL interpreter for DSL macros.
    Setter:
    @brief Sets the prolog
    See \prolog for details.
    """
    shortcut: str
    r"""
    Getter:
    @brief Gets the macro's keyboard shortcut

    This method has been introduced in version 0.27.5.

    Setter:
    @brief Sets the macro's keyboard shortcut

    This method has been introduced in version 0.27.5.
    """
    show_in_menu: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the macro shall be shown in the menu

    Setter:
    @brief Sets a value indicating whether the macro shall be shown in the menu
    """
    text: str
    r"""
    Getter:
    @brief Gets the macro text

    The text is the code executed by the macro interpreter. Depending on the DSL interpreter, the text can be any kind of code.
    Setter:
    @brief Sets the macro text
    See \text for details.
    """
    version: str
    r"""
    Getter:
    @brief Gets the macro's version

    This method has been introduced in version 0.27.5.

    Setter:
    @brief Sets the macro's version

    This method has been introduced in version 0.27.5.
    """
    @classmethod
    def macro_by_path(cls, path: str) -> Macro:
        r"""
        @brief Finds the macro by installation path

        Returns nil if no macro with this path can be found.

        This method has been added in version 0.26.
        """
    @classmethod
    def new(cls, path: str) -> Macro:
        r"""
        @brief Loads the macro from the given file path

        This constructor has been introduced in version 0.27.5.
        """
    @classmethod
    def real_line(cls, path: str, line: int) -> int:
        r"""
        @brief Gets the real line number for an include-encoded path and line number

        When using KLayout's include scheme based on '# %include ...', __FILE__ and __LINE__ (Ruby) will not have the proper values but encoded file names. This method allows retrieving the real line number by using

        @code
        # Ruby
        real_line = RBA::Macro::real_line(__FILE__, __LINE__)

        # Python
        real_line = pya::Macro::real_line(__file__, __line__)
        @/code

        This substitution is not required for top-level macros as KLayout's interpreter will automatically use this function instead of __FILE__. Call this function when you need __FILE__ from files included through the languages mechanisms such as 'require' or 'load' where this substitution does not happen.

        For Python there is no equivalent for __LINE__, so you always have to use:

        @code
        # Pythonimport inspect
        real_line = pya.Macro.real_line(__file__, inspect.currentframe().f_back.f_lineno)
        @/code

        This feature has been introduced in version 0.27.
        """
    @classmethod
    def real_path(cls, path: str, line: int) -> str:
        r"""
        @brief Gets the real path for an include-encoded path and line number

        When using KLayout's include scheme based on '# %include ...', __FILE__ and __LINE__ (Ruby) will not have the proper values but encoded file names. This method allows retrieving the real file by using

        @code
        # Ruby
        real_file = RBA::Macro::real_path(__FILE__, __LINE__)
        @/code

        This substitution is not required for top-level macros as KLayout's interpreter will automatically use this function instead of __FILE__. Call this function when you need __FILE__ from files included through the languages mechanisms such as 'require' or 'load' where this substitution does not happen.

        For Python there is no equivalent for __LINE__, so you always have to use:

        @code
        # Pythonimport inspect
        real_file = pya.Macro.real_path(__file__, inspect.currentframe().f_back.f_lineno)
        @/code

        This feature has been introduced in version 0.27.
        """
    def __init__(self, path: str) -> None:
        r"""
        @brief Loads the macro from the given file path

        This constructor has been introduced in version 0.27.5.
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
    def interpreter_name(self) -> str:
        r"""
        @brief Gets the macro interpreter name
        This is the string version of \interpreter.

        This method has been introduced in version 0.27.5.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def name(self) -> str:
        r"""
        @brief Gets the name of the macro

        This attribute has been added in version 0.25.
        """
    def path(self) -> str:
        r"""
        @brief Gets the path of the macro

        The path is the path where the macro is stored, starting with an abstract group identifier. The path is used to identify the macro in the debugger for example.
        """
    def run(self) -> int:
        r"""
        @brief Executes the macro

        This method has been introduced in version 0.27.5.
        """
    def save_to(self, path: str) -> None:
        r"""
        @brief Saves the macro to the given file

        This method has been introduced in version 0.27.5.
        """
    def sync_properties_with_text(self) -> None:
        r"""
        @brief Synchronizes the macro properties with the text

        This method performs the reverse process of \sync_text_with_properties.

        This method has been introduced in version 0.27.5.
        """
    def sync_text_with_properties(self) -> None:
        r"""
        @brief Synchronizes the macro text with the properties

        This method applies to PlainTextWithHashAnnotationsFormat format. The macro text will be enhanced with pseudo-comments reflecting the macro properties. This way, the macro properties can be stored in plain files.

        This method has been introduced in version 0.27.5.
        """

class MacroExecutionContext:
    r"""
    @brief Support for various debugger features

    This class implements some features that allow customization of the debugger behavior, specifically the generation of back traces and the handling of exception. These functions are particular useful for implementing DSL interpreters and providing proper error locations in the back traces or to suppress exceptions when re-raising them.
    """
    @classmethod
    def ignore_next_exception(cls) -> None:
        r"""
        @brief Ignores the next exception in the debugger
        The next exception thrown will be ignored in the debugger. That feature is useful when re-raising exceptions if those new exception shall not appear in the debugger.
        """
    @classmethod
    def new(cls) -> MacroExecutionContext:
        r"""
        @brief Creates a new object of this class
        """
    @classmethod
    def remove_debugger_scope(cls) -> None:
        r"""
        @brief Removes a debugger scope previously set with \set_debugger_scope
        """
    @classmethod
    def set_debugger_scope(cls, filename: str) -> None:
        r"""
        @brief Sets a debugger scope (file level which shall appear in the debugger)
        If a debugger scope is set, back traces will be produced starting from that scope. Setting a scope is useful for implementing DSL interpreters and giving a proper hint about the original location of an error.
        """
    def __copy__(self) -> MacroExecutionContext:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> MacroExecutionContext:
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
    def assign(self, other: MacroExecutionContext) -> None:
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
    def dup(self) -> MacroExecutionContext:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class MacroInterpreter:
    r"""
    @brief A custom interpreter for a DSL (domain specific language)

    DSL interpreters are a way to provide macros written in a language specific for the application. One example are DRC scripts which are written in some special language optimized for DRC ruledecks. Interpreters for such languages can be built using scripts itself by providing the interpreter implementation through this object.

    An interpreter implementation involves at least these steps:

    @ul
    @li Derive a new object from RBA::MacroInterpreter @/li
    @li Reimplement the \execute method for the actual execution of the code @/li
    @li In the initialize method configure the object using the attribute setters like \suffix= and register the object as DSL interpreter (in that order) @/li
    @li Create at least one template macro in the initialize method @/li
    @/ul

    Template macros provide a way for the macro editor to present macros for the new interpreter in the list of templates. Template macros can provide menu bindings, shortcuts and some initial text for example

    The simple implementation can be enhanced by providing more information, i.e. syntax highlighter information, the debugger to use etc. This involves reimplementing further methods, i.e. "syntax_scheme".

    This is a simple example for an interpreter in Ruby. Is is registered under the name 'simple-dsl' and just evaluates the script text:

    @code
    class SimpleExecutable < RBA::Excutable

      # Constructor
      def initialize(macro)
        \@macro = macro
      end
  
      # Implements the execute method
      def execute
        eval(\@macro.text, nil, \@macro.path)
        nil
      end

    end

    class SimpleInterpreter < RBA::MacroInterpreter

      # Constructor
      def initialize
        self.description = "A test interpreter"
        # Registers the new interpreter
        register("simple-dsl")
        # create a template for the macro editor:
        # Name is "new_simple", the description will be "Simple interpreter macro"
        # in the "Special" group.
        mt = create_template("new_simple")
        mt.description = "Special;;Simple interpreter macro"
      end
  
      # Creates the executable delegate
      def executable(macro)
        SimpleExecutable::new(macro)
      end

    end

    # Register the new interpreter
    SimpleInterpreter::new

    @/code

    Please note that such an implementation is dangerous because the evaluation of the script happens in the context of the interpreter object. In this implementation the script could redefine the execute method for example. This implementation is provided as an example only.
    A real implementation should add execution of prolog and epilog code inside the execute method and proper error handling.

    In order to make the above code effective, store the code in an macro, set "early auto-run" and restart KLayout.

    This class has been introduced in version 0.23 and modified in 0.27.
    """
    MacroFormat: ClassVar[Macro.Format]
    r"""
    @brief The macro has macro (XML) format
    """
    NoDebugger: ClassVar[Macro.Interpreter]
    r"""
    @brief Indicates no debugging for \debugger_scheme
    """
    PlainTextFormat: ClassVar[Macro.Format]
    r"""
    @brief The macro has plain text format
    """
    PlainTextWithHashAnnotationsFormat: ClassVar[Macro.Format]
    r"""
    @brief The macro has plain text format with special pseudo-comment annotations
    """
    RubyDebugger: ClassVar[Macro.Interpreter]
    r"""
    @brief Indicates Ruby debugger for \debugger_scheme
    """
    @property
    def debugger_scheme(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the debugger scheme (which debugger to use for the DSL macro)

        The value can be one of the constants \RubyDebugger or \NoDebugger.

        Use this attribute setter in the initializer before registering the interpreter.

        Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for performance reasons in version 0.25.
        """
    @property
    def description(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets a description string

        This string is used for showing the type of DSL macro in the file selection box together with the suffix for example. 
        Use this attribute setter in the initializer before registering the interpreter.

        Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for performance reasons in version 0.25.
        """
    @property
    def storage_scheme(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the storage scheme (the format as which the macro is stored)

        This value indicates how files for this DSL macro type shall be stored. The value can be one of the constants \PlainTextFormat, \PlainTextWithHashAnnotationsFormat and \MacroFormat.

        Use this attribute setter in the initializer before registering the interpreter.

        Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for performance reasons in version 0.25.
        """
    @property
    def suffix(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the file suffix

        This string defines which file suffix to associate with the DSL macro. If an empty string is given (the default) no particular suffix is assciated with that macro type and "lym" is assumed. 
        Use this attribute setter in the initializer before registering the interpreter.

        Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for performance reasons in version 0.25.
        """
    @property
    def supports_include_expansion(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets a value indicating whether this interpreter supports the default include file expansion scheme.
        If this value is set to true (the default), lines like '# %include ...' will be substituted by the content of the file following the '%include' keyword.
        Set this value to false if you don't want to support this feature.

        This attribute has been introduced in version 0.27.
        """
    @property
    def syntax_scheme(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets a string indicating the syntax highlighter scheme

        The scheme string can be empty (indicating no syntax highlighting), "ruby" for the Ruby syntax highlighter or another string. In that case, the highlighter will look for a syntax definition under the resource path ":/syntax/<scheme>.xml".

        Use this attribute setter in the initializer before registering the interpreter.

        Before version 0.25 this attribute was a re-implementable method. It has been turned into an attribute for performance reasons in version 0.25.
        """
    @classmethod
    def new(cls) -> MacroInterpreter:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> MacroInterpreter:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> MacroInterpreter:
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
    def assign(self, other: MacroInterpreter) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def create_template(self, url: str) -> Macro:
        r"""
        @brief Creates a new macro template
        @param url The template will be initialized from that URL.

        This method will create a register a new macro template. It returns a \Macro object which can be modified in order to adjust the template (for example to set description, add a content, menu binding, autorun flags etc.)

        This method must be called after \register has called.
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
    def dup(self) -> MacroInterpreter:
        r"""
        @brief Creates a copy of self
        """
    def include_expansion(self, macro: Macro) -> List[str]:
        r"""
        @brief Provides include expansion as defined by the interpreter
        The return value will be a two-element array with the encoded file path and the include-expanded text.

        This method has been introduced in version 0.28.12.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def register(self, name: str) -> None:
        r"""
        @brief Registers the macro interpreter
        @param name The interpreter name. This is an arbitrary string which should be unique.

        Registration of the interpreter makes the object known to the system. After registration, macros whose interpreter is set to 'dsl' can use this object to run the script. For executing a script, the system will call the interpreter's \execute method.
        """

class Marker:
    r"""
    @brief The floating-point coordinate marker object

    The marker is a visual object that "marks" (highlights) a 
    certain area of the layout, given by a database object. This object accepts database objects with floating-point coordinates in micron values.
    """
    color: int
    r"""
    Getter:
    @brief Gets the color of the marker
    This value is valid only if \has_color? is true.
    Setter:
    @brief Sets the color of the marker
    The color is a 32bit unsigned integer encoding the RGB values in the lower 3 bytes (blue in the lowest significant byte). The color can be reset with \reset_color, in which case, the default foreground color is used.
    """
    dismissable: bool
    r"""
    Getter:
    @brief Gets a value indicating whether the marker can be hidden
    See \dismissable= for a description of this predicate.
    Setter:
    @brief Sets a value indicating whether the marker can be hidden
    Dismissable markers can be hidden setting "View/Show Markers" to "off". The default setting is "false" meaning the marker can't be hidden.

    This attribute has been introduced in version 0.25.4.
    """
    dither_pattern: int
    r"""
    Getter:
    @brief Gets the stipple pattern index
    See \dither_pattern= for a description of the stipple pattern index.
    Setter:
    @brief Sets the stipple pattern index
    A value of -1 or less than zero indicates that the marker is not filled. Otherwise, the value indicates which pattern to use for filling the marker.
    """
    frame_color: int
    r"""
    Getter:
    @brief Gets the frame color of the marker
    This value is valid only if \has_frame_color? is true.The set method has been added in version 0.20.

    Setter:
    @brief Sets the frame color of the marker
    The color is a 32bit unsigned integer encoding the RGB values in the lower 3 bytes (blue in the lowest significant byte). The color can be reset with \reset_frame_color, in which case the fill color is used.
    The set method has been added in version 0.20.
    """
    halo: int
    r"""
    Getter:
    @brief Gets the halo flag
    See \halo= for a description of the halo flag.
    Setter:
    @brief Sets the halo flag
    The halo flag is either -1 (for taking the default), 0 to disable the halo or 1 to enable it. If the halo is enabled, a pixel border with the background color is drawn around the marker, the vertices and texts.
    """
    line_style: int
    r"""
    Getter:
    @brief Get the line style
    See \line_style= for a description of the line style index.
    This method has been introduced in version 0.25.
    Setter:
    @brief Sets the line style
    The line style is given by an index. 0 is solid, 1 is dashed and so forth.

    This method has been introduced in version 0.25.
    """
    line_width: int
    r"""
    Getter:
    @brief Gets the line width of the marker
    See \line_width= for a description of the line width.
    Setter:
    @brief Sets the line width of the marker
    This is the width of the line drawn for the outline of the marker.
    """
    vertex_size: int
    r"""
    Getter:
    @brief Gets the vertex size of the marker
    See \vertex_size= for a description.
    Setter:
    @brief Sets the vertex size of the marker
    This is the size of the rectangles drawn for the vertices object.
    """
    @classmethod
    def new(cls, view: LayoutViewBase) -> Marker:
        r"""
        @brief Creates a marker

        A marker is always associated with a view, in which it is shown. The view this marker is associated with must be passed to the constructor.
        """
    def __init__(self, view: LayoutViewBase) -> None:
        r"""
        @brief Creates a marker

        A marker is always associated with a view, in which it is shown. The view this marker is associated with must be passed to the constructor.
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
    def has_color(self) -> bool:
        r"""
        @brief Returns a value indicating whether the marker has a specific color
        """
    def has_frame_color(self) -> bool:
        r"""
        @brief Returns a value indicating whether the marker has a specific frame color
        The set method has been added in version 0.20.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def reset_color(self) -> None:
        r"""
        @brief Resets the color of the marker
        See \set_color for a description of the color property of the marker.
        """
    def reset_frame_color(self) -> None:
        r"""
        @brief Resets the frame color of the marker
        See \set_frame_color for a description of the frame color property of the marker.The set method has been added in version 0.20.
        """
    @overload
    def set(self, box: db.DBox) -> None:
        r"""
        @brief Sets the box the marker is to display

        Makes the marker show a box. The box must be given in micron units.
        If the box is empty, no marker is drawn.
        The set method has been added in version 0.20.
        """
    @overload
    def set(self, edge: db.DEdge) -> None:
        r"""
        @brief Sets the edge the marker is to display

        Makes the marker show a edge. The edge must be given in micron units.
        The set method has been added in version 0.20.
        """
    @overload
    def set(self, path: db.DPath) -> None:
        r"""
        @brief Sets the path the marker is to display

        Makes the marker show a path. The path must be given in micron units.
        The set method has been added in version 0.20.
        """
    @overload
    def set(self, polygon: db.DPolygon) -> None:
        r"""
        @brief Sets the polygon the marker is to display

        Makes the marker show a polygon. The polygon must be given in micron units.
        The set method has been added in version 0.20.
        """
    @overload
    def set(self, text: db.DText) -> None:
        r"""
        @brief Sets the text the marker is to display

        Makes the marker show a text. The text must be given in micron units.
        The set method has been added in version 0.20.
        """
    def set_box(self, box: db.DBox) -> None:
        r"""
        @brief Sets the box the marker is to display

        Makes the marker show a box. The box must be given in micron units.
        If the box is empty, no marker is drawn.
        The set method has been added in version 0.20.
        """
    def set_edge(self, edge: db.DEdge) -> None:
        r"""
        @brief Sets the edge the marker is to display

        Makes the marker show a edge. The edge must be given in micron units.
        The set method has been added in version 0.20.
        """
    def set_path(self, path: db.DPath) -> None:
        r"""
        @brief Sets the path the marker is to display

        Makes the marker show a path. The path must be given in micron units.
        The set method has been added in version 0.20.
        """
    def set_polygon(self, polygon: db.DPolygon) -> None:
        r"""
        @brief Sets the polygon the marker is to display

        Makes the marker show a polygon. The polygon must be given in micron units.
        The set method has been added in version 0.20.
        """
    def set_text(self, text: db.DText) -> None:
        r"""
        @brief Sets the text the marker is to display

        Makes the marker show a text. The text must be given in micron units.
        The set method has been added in version 0.20.
        """

class ObjectInstPath:
    r"""
    @brief A class describing a selected shape or instance

    A shape or instance is addressed by a path which describes all instances leading to the specified
    object. These instances are described through \InstElement objects, which specify the instance and, in case of array instances, the specific array member.
    For shapes, additionally the layer and the shape itself is specified. The ObjectInstPath objects
    encapsulates both forms, which can be distinguished with the \is_cell_inst? predicate.

    An instantiation path leads from a top cell down to the container cell which either holds the shape or the instance.
    The top cell can be obtained through the \top attribute, the container cell through the \source attribute. Both are cell indexes which can be converted to \Cell objects through the \Layout#cell. In case of objects located in the top cell, \top and \source refer to the same cell.
    The first element of the instantiation path is the instance located within the top cell leading to the first child cell. The second element leads to the next child cell and so forth. \path_nth can be used to obtain a specific element of the path.

    The \cv_index attribute specifies the cellview the selection applies to. Use \LayoutView#cellview to obtain the \CellView object from the index.

    The shape or instance the selection refers to can be obtained with \shape and \inst respectively. Use \is_cell_inst? to decide whether the selection refers to an instance or not.

    The ObjectInstPath class plays a role when retrieving and modifying the selection of shapes and instances through \LayoutView#object_selection, \LayoutView#object_selection=, \LayoutView#select_object and \LayoutView#unselect_object. \ObjectInstPath objects can be modified to reflect a new selection, but the new selection becomes active only after it is installed in the view. The following sample demonstrates that. It implements a function to convert all shapes to polygons:

    @code
    mw = RBA::Application::instance::main_window
    view = mw.current_view

    begin

      view.transaction("Convert selected shapes to polygons")

      sel = view.object_selection

      sel.each do |s|
        if !s.is_cell_inst? && !s.shape.is_text?
          ly = view.cellview(s.cv_index).layout
          # convert to polygon
          s.shape.polygon = s.shape.polygon
        end
      end
  
      view.object_selection = sel

    ensure
      view.commit
    end
    @/code

    Note, that without resetting the selection in the above example, the application might raise errors because after modifying the selected objects, the current selection will no longer be valid. Establishing a new valid selection in the way shown above will help avoiding this issue.
    """
    cv_index: int
    r"""
    Getter:
    @brief Gets the cellview index that describes which cell view the shape or instance is located in

    Setter:
    @brief Sets the cellview index that describes which cell view the shape or instance is located in

    This method has been introduced in version 0.24.
    """
    layer: Any
    r"""
    Getter:
    @brief Gets the layer index that describes which layer the selected shape is on

    Starting with version 0.27, this method returns nil for this property if \is_cell_inst? is false - i.e. the selection does not represent a shape.
    Setter:
    @brief Sets to the layer index that describes which layer the selected shape is on

    Setting the layer property to a valid layer index makes the path a shape selection path.
    Setting the layer property to a negative layer index makes the selection an instance selection.

    This method has been introduced in version 0.24.
    """
    path: List[db.InstElement]
    r"""
    Getter:
    @brief Gets the instantiation path
    The path is a sequence of \InstElement objects leading to the target object.

    This method was introduced in version 0.26.

    Setter:
    @brief Sets the instantiation path

    This method was introduced in version 0.26.
    """
    seq: int
    r"""
    Getter:
    @brief Gets the sequence number

    The sequence number describes when the item was selected.
    A sequence number of 0 indicates that the item was selected in the first selection action (without 'Shift' pressed).

    Setter:
    @brief Sets the sequence number

    See \seq for a description of this property.

    This method was introduced in version 0.24.
    """
    shape: Any
    r"""
    Getter:
    @brief Gets the selected shape

    The shape object may be modified. This does not have an immediate effect on the selection. Instead, the selection must be set in the view using \LayoutView#object_selection= or \LayoutView#select_object.

    This method delivers valid results only for object selections that represent shapes. Starting with version 0.27, this method returns nil for this property if \is_cell_inst? is false.
    Setter:
    @brief Sets the shape object that describes the selected shape geometrically

    When using this setter, the layer index must be set to a valid layout layer (see \layer=).
    Setting both properties makes the selection a shape selection.

    This method has been introduced in version 0.24.
    """
    top: int
    r"""
    Getter:
    @brief Gets the cell index of the top cell the selection applies to

    The top cell is identical to the current cell provided by the cell view.
    It is the cell from which is instantiation path originates and the container cell if not instantiation path is set.

    This method has been introduced in version 0.24.
    Setter:
    @brief Sets the cell index of the top cell the selection applies to

    See \top_cell for a description of this property.

    This method has been introduced in version 0.24.
    """
    @classmethod
    def new(cls, si: db.RecursiveShapeIterator, cv_index: int) -> ObjectInstPath:
        r"""
        @brief Creates a new path object from a \RecursiveShapeIterator
        Use this constructor to quickly turn a recursive shape iterator delivery into a shape selection.
        """
    def __copy__(self) -> ObjectInstPath:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> ObjectInstPath:
        r"""
        @brief Creates a copy of self
        """
    def __eq__(self, b: object) -> bool:
        r"""
        @brief Equality of two ObjectInstPath objects
        Note: this operator returns true if both instance paths refer to the same object, not just identical ones.

        This method has been introduced with version 0.24.
        """
    def __init__(self, si: db.RecursiveShapeIterator, cv_index: int) -> None:
        r"""
        @brief Creates a new path object from a \RecursiveShapeIterator
        Use this constructor to quickly turn a recursive shape iterator delivery into a shape selection.
        """
    def __lt__(self, b: ObjectInstPath) -> bool:
        r"""
        @brief Provides an order criterion for two ObjectInstPath objects
        Note: this operator is just provided to establish any order, not a particular one.

        This method has been introduced with version 0.24.
        """
    def __ne__(self, b: object) -> bool:
        r"""
        @brief Inequality of two ObjectInstPath objects
        See the comments on the == operator.

        This method has been introduced with version 0.24.
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
    def append_path(self, element: db.InstElement) -> None:
        r"""
        @brief Appends an element to the instantiation path

        This method allows building of an instantiation path pointing to the selected object.
        For an instance selection, the last component added is the instance which is selected.
        For a shape selection, the path points to the cell containing the selected shape.

        This method was introduced in version 0.24.
        """
    def assign(self, other: ObjectInstPath) -> None:
        r"""
        @brief Assigns another object to self
        """
    def cell_index(self) -> int:
        r"""
        @brief Gets the cell index of the cell that the selection applies to.
        This method returns the cell index that describes which cell the selected shape is located in or the cell whose instance is selected if \is_cell_inst? is true.
        This property is set implicitly by setting the top cell and adding elements to the instantiation path.
        To obtain the index of the container cell, use \source.
        """
    def clear_path(self) -> None:
        r"""
        @brief Clears the instantiation path

        This method was introduced in version 0.24.
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
    def dtrans(self) -> db.DCplxTrans:
        r"""
        @brief Gets the transformation applicable for the shape in micron space.

        This method returns the same transformation than \trans, but applicable to objects in micrometer units:

        @code
        # renders the micrometer-unit polygon in top cell coordinates:
        dpolygon_in_top = sel.dtrans * sel.shape.dpolygon
        @/code

        This method is not applicable to instance selections. A more generic attribute is \source_dtrans.

        The method has been introduced in version 0.25.
        """
    def dup(self) -> ObjectInstPath:
        r"""
        @brief Creates a copy of self
        """
    def each_inst(self) -> Iterator[db.InstElement]:
        r"""
        @brief Yields the instantiation path

        The instantiation path describes by an sequence of \InstElement objects the path by which the cell containing the selected shape is found from the cell view's current cell.
        If this object represents an instance, the path will contain the selected instance as the last element.
        The elements are delivered top down.
        """
    def inst(self) -> db.Instance:
        r"""
        @brief Deliver the instance represented by this selection

        This method delivers valid results only if \is_cell_inst? is true.
        It returns the instance reference (an \Instance object) that this selection represents.

        This property is set implicitly by adding instance elements to the instantiation path.

        This method has been added in version 0.16.
        """
    def is_cell_inst(self) -> bool:
        r"""
        @brief True, if this selection represents a cell instance

        If this attribute is true, the shape reference and layer are not valid.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_valid(self, view: LayoutViewBase) -> bool:
        r"""
        @brief Gets a value indicating whether the instance path refers to a valid object in the context of the given view

        This predicate has been introduced in version 0.27.12.
        """
    def layout(self) -> db.Layout:
        r"""
        @brief Gets the Layout object the selected object lives in.

        This method returns the \Layout object that the selected object lives in. This method may return nil, if the selection does not point to a valid object.

        This method has been introduced in version 0.25.
        """
    def path_length(self) -> int:
        r"""
        @brief Returns the length of the path (number of elements delivered by \each_inst)

        This method has been added in version 0.16.
        """
    def path_nth(self, n: int) -> db.InstElement:
        r"""
        @brief Returns the nth element of the path (similar to \each_inst but with direct access through the index)

        @param n The index of the element to retrieve (0..\path_length-1)
        This method has been added in version 0.16.
        """
    def source(self) -> int:
        r"""
        @brief Returns to the cell index of the cell that the selected element resides inside.

        If this reference represents a cell instance, this method delivers the index of the cell in which the cell instance resides. Otherwise, this method returns the same value than \cell_index.

        This property is set implicitly by setting the top cell and adding elements to the instantiation path.

        This method has been added in version 0.16.
        """
    def source_dtrans(self) -> db.DCplxTrans:
        r"""
        @brief Gets the transformation applicable for an instance and shape in micron space.

        This method returns the same transformation than \source_trans, but applicable to objects in micrometer units:

        @code
        # renders the cell instance as seen from top level:
        dcell_inst_in_top = sel.source_dtrans * sel.inst.dcell_inst
        @/code

        The method has been introduced in version 0.25.
        """
    def source_trans(self) -> db.ICplxTrans:
        r"""
        @brief Gets the transformation applicable for an instance and shape.

        If this object represents a shape, this transformation describes how the selected shape is transformed into the current cell of the cell view.
        If this object represents an instance, this transformation describes how the selected instance is transformed into the current cell of the cell view.
        This method is similar to \trans, except that the resulting transformation does not include the instance transformation if the object represents an instance.

        This property is set implicitly by setting the top cell and adding elements to the instantiation path.

        This method has been added in version 0.16.
        """
    def trans(self) -> db.ICplxTrans:
        r"""
        @brief Gets the transformation applicable for the shape.

        If this object represents a shape, this transformation describes how the selected shape is transformed into the current cell of the cell view.
        Basically, this transformation is the accumulated transformation over the instantiation path. If the ObjectInstPath represents a cell instance, this includes the transformation of the selected instance as well.

        This property is set implicitly by setting the top cell and adding elements to the instantiation path.
        This method is not applicable for instance selections. A more generic attribute is \source_trans.
        """

class PixelBuffer:
    r"""
    @brief A simplistic pixel buffer representing an image of ARGB32 or RGB32 values

    This object is mainly provided for offline rendering of layouts in Qt-less environments.
    It supports a rectangular pixel space with color values encoded in 32bit integers. It supports transparency through an optional alpha channel. The color format for a pixel is "0xAARRGGBB" where 'AA' is the alpha value which is ignored in non-transparent mode.

    This class supports basic operations such as initialization, single-pixel access and I/O to PNG.

    This class has been introduced in version 0.28.
    """
    transparent: bool
    r"""
    Getter:
    @brief Gets a flag indicating whether the pixel buffer supports an alpha channel

    Setter:
    @brief Sets a flag indicating whether the pixel buffer supports an alpha channel

    By default, the pixel buffer does not support an alpha channel.
    """
    @classmethod
    def from_png_data(cls, data: bytes) -> PixelBuffer:
        r"""
        @brief Reads the pixel buffer from a PNG byte stream
        This method may not be available if PNG support is not compiled into KLayout.
        """
    @classmethod
    def new(cls, width: int, height: int) -> PixelBuffer:
        r"""
        @brief Creates a pixel buffer object

        @param width The width in pixels
        @param height The height in pixels

        The pixels are basically uninitialized. You will need to use \fill to initialize them to a certain value.
        """
    @classmethod
    def read_png(cls, file: str) -> PixelBuffer:
        r"""
        @brief Reads the pixel buffer from a PNG file
        This method may not be available if PNG support is not compiled into KLayout.
        """
    def __copy__(self) -> PixelBuffer:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> PixelBuffer:
        r"""
        @brief Creates a copy of self
        """
    def __eq__(self, other: object) -> bool:
        r"""
        @brief Returns a value indicating whether self is identical to the other image
        """
    def __init__(self, width: int, height: int) -> None:
        r"""
        @brief Creates a pixel buffer object

        @param width The width in pixels
        @param height The height in pixels

        The pixels are basically uninitialized. You will need to use \fill to initialize them to a certain value.
        """
    def __ne__(self, other: object) -> bool:
        r"""
        @brief Returns a value indicating whether self is not identical to the other image
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
    def assign(self, other: PixelBuffer) -> None:
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
    def diff(self, other: PixelBuffer) -> PixelBuffer:
        r"""
        @brief Creates a difference image

        This method is provided to support transfer of image differences - i.e. small updates instead of full images. It works for non-transparent images only and generates an image with transpareny enabled and with the new pixel values for pixels that have changed. The alpha value will be 0 for identical images and 255 for pixels with different values. This way, the difference image can be painted over the original image to generate the new image.
        """
    def dup(self) -> PixelBuffer:
        r"""
        @brief Creates a copy of self
        """
    def fill(self, color: int) -> None:
        r"""
        @brief Fills the pixel buffer with the given pixel value
        """
    def height(self) -> int:
        r"""
        @brief Gets the height of the pixel buffer in pixels
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def patch(self, other: PixelBuffer) -> None:
        r"""
        @brief Patches another pixel buffer into this one

        This method is the inverse of \diff - it will patch the difference image created by diff into this pixel buffer. Note that this method will not do true alpha blending and requires the other pixel buffer to have the same format than self. Self will be modified by this operation.
        """
    def pixel(self, x: int, y: int) -> int:
        r"""
        @brief Gets the value of the pixel at position x, y
        """
    def set_pixel(self, x: int, y: int, c: int) -> None:
        r"""
        @brief Sets the value of the pixel at position x, y
        """
    def swap(self, other: PixelBuffer) -> None:
        r"""
        @brief Swaps data with another PixelBuffer object
        """
    def to_png_data(self) -> bytes:
        r"""
        @brief Converts the pixel buffer to a PNG byte stream
        This method may not be available if PNG support is not compiled into KLayout.
        """
    def width(self) -> int:
        r"""
        @brief Gets the width of the pixel buffer in pixels
        """
    def write_png(self, file: str) -> None:
        r"""
        @brief Writes the pixel buffer to a PNG file
        This method may not be available if PNG support is not compiled into KLayout.
        """

class Plugin:
    r"""
    @brief The plugin object

    This class provides the actual plugin implementation. Each view gets its own instance of the plugin class. The plugin factory \PluginFactory class must be specialized to provide a factory for new objects of the Plugin class. See the documentation there for details about the plugin mechanism and the basic concepts.

    This class has been introduced in version 0.22.
    """
    @classmethod
    def new(cls) -> Plugin:
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
    def grab_mouse(self) -> None:
        r"""
        @brief Redirects mouse events to this plugin, even if the plugin is not active.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def set_cursor(self, cursor_type: int) -> None:
        r"""
        @brief Sets the cursor in the view area to the given type
        Setting the cursor has an effect only inside event handlers, i.e. mouse_press_event. The cursor is not set permanently. Is is reset in the mouse move handler unless a button is pressed or the cursor is explicitly set again in the mouse_move_event.

        The cursor type is one of the cursor constants in the \Cursor class, i.e. 'CursorArrow' for the normal cursor.
        """
    def ungrab_mouse(self) -> None:
        r"""
        @brief Removes a mouse grab registered with \grab_mouse.
        """

class PluginFactory:
    r"""
    @brief The plugin framework's plugin factory object

    Plugins are components that extend KLayout's functionality in various aspects. Scripting support exists currently for providing mouse mode handlers and general on-demand functionality connected with a menu entry.

    Plugins are objects that implement the \Plugin interface. Each layout view is associated with one instance of such an object. The PluginFactory is a singleton which is responsible for creating \Plugin objects and providing certain configuration information such as where to put the menu items connected to this plugin and what configuration keys are used.

    An implementation of PluginFactory must at least provide an implementation of \create_plugin. This method must instantiate a new object of the specific plugin.

    After the factory has been created, it must be registered in the system using one of the \register methods. It is therefore recommended to put the call to \register at the end of the "initialize" method. For the registration to work properly, the menu items must be defined before \register is called.

    The following features can also be implemented:

    @<ul>
      @<li>Reserve keys in the configuration file using \add_option in the constructor@</li>
      @<li>Create menu items by using \add_menu_entry in the constructor@</li>
      @<li>Set the title for the mode entry that appears in the tool bar using the \register argument@</li>
      @<li>Provide global functionality (independent from the layout view) using \configure or \menu_activated@</li>
    @</ul>

    This is a simple example for a plugin in Ruby. It switches the mouse cursor to a 'cross' cursor when it is active:

    @code
    class PluginTestFactory < RBA::PluginFactory

      # Constructor
      def initialize
        # registers the new plugin class at position 100000 (at the end), with name
        # "my_plugin_test" and title "My plugin test"
        register(100000, "my_plugin_test", "My plugin test")
      end
  
      # Create a new plugin instance of the custom type
      def create_plugin(manager, dispatcher, view)
        return PluginTest.new
      end

    end

    # The plugin class
    class PluginTest < RBA::Plugin
      def mouse_moved_event(p, buttons, prio)
        if prio
          # Set the cursor to cross if our plugin is active.
          set_cursor(RBA::Cursor::Cross)
        end
        # Returning false indicates that we don't want to consume the event.
        # This way for example the cursor position tracker still works.
        false
      end
      def mouse_click_event(p, buttons, prio)
        if prio
          puts "mouse button clicked."
          # This indicates we want to consume the event and others don't receive the mouse click
          # with prio = false.
          return true
        end
        # don't consume the event if we are not active.
        false
      end
    end

    # Instantiate the new plugin factory.
    PluginTestFactory.new
    @/code

    This class has been introduced in version 0.22.
    """
    @property
    def has_tool_entry(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Enables or disables the tool bar entry
        Initially this property is set to true. This means that the plugin will have a visible entry in the toolbar. This property can be set to false to disable this feature. In that case, the title and icon given on registration will be ignored. 
        """
    @classmethod
    def new(cls) -> PluginFactory:
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
    def add_config_menu_item(self, menu_name: str, insert_pos: str, title: str, cname: str, cvalue: str) -> None:
        r"""
        @brief Adds a configuration menu item

        Menu items created this way will send a configuration request with 'cname' as the configuration parameter name and 'cvalue' as the configuration parameter value.

        This method has been introduced in version 0.27.
        """
    @overload
    def add_menu_entry(self, menu_name: str, insert_pos: str) -> None:
        r"""
        @brief Specifies a separator
        Call this method in the factory constructor to build the menu items that this plugin shall create.
        This specific call inserts a separator at the given position (insert_pos). The position uses abstract menu item paths and "menu_name" names the component that will be created. See \AbstractMenu for a description of the path.
        """
    @overload
    def add_menu_entry(self, symbol: str, menu_name: str, insert_pos: str, title: str) -> None:
        r"""
        @brief Specifies a menu item
        Call this method in the factory constructor to build the menu items that this plugin shall create.
        This specific call inserts a menu item at the specified position (insert_pos). The position uses abstract menu item paths and "menu_name" names the component that will be created. See \AbstractMenu for a description of the path.
        When the menu item is selected "symbol" is the string that is sent to the \menu_activated callback (either the global one for the factory ot the one of the per-view plugin instance).

        @param symbol The string to send to the plugin if the menu is triggered
        @param menu_name The name of entry to create at the given position
        @param insert_pos The position where to create the entry
        @param title The title string for the item. The title can contain a keyboard shortcut in round braces after the title text, i.e. "My Menu Item(F12)"
        """
    @overload
    def add_menu_entry(self, symbol: str, menu_name: str, insert_pos: str, title: str, sub_menu: bool) -> None:
        r"""
        @brief Specifies a menu item or sub-menu
        Similar to the previous form of "add_menu_entry", but this version allows also to create sub-menus by setting the last parameter to "true".

        With version 0.27 it's more convenient to use \add_submenu.
        """
    def add_menu_item_clone(self, symbol: str, menu_name: str, insert_pos: str, copy_from: str) -> None:
        r"""
        @brief Specifies a menu item as a clone of another one
        Using this method, a menu item can be made a clone of another entry (given as path by 'copy_from').
        The new item will share the \Action object with the original one, so manipulating the action will change both the original entry and the new entry.

        This method has been introduced in version 0.27.
        """
    def add_option(self, name: str, default_value: str) -> None:
        r"""
        @brief Specifies configuration variables.
        Call this method in the factory constructor to add configuration key/value pairs to the configuration repository. Without specifying configuration variables, the status of a plugin cannot be persisted. 

        Once the configuration variables are known, they can be retrieved on demand using "get_config" from \MainWindow or listening to \configure callbacks (either in the factory or the plugin instance). Configuration variables can be set using "set_config" from \MainWindow. This scheme also works without registering the configuration options, but doing so has the advantage that it is guaranteed that a variable with this keys exists and has the given default value initially.

        """
    def add_submenu(self, menu_name: str, insert_pos: str, title: str) -> None:
        r"""
        @brief Specifies a menu item or sub-menu

        This method has been introduced in version 0.27.
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
    @overload
    def register(self, position: int, name: str, title: str) -> None:
        r"""
        @brief Registers the plugin factory
        @param position An integer that determines the order in which the plugins are created. The internal plugins use the values from 1000 to 50000.
        @param name The plugin name. This is an arbitrary string which should be unique. Hence it is recommended to use a unique prefix, i.e. "myplugin::ThePluginClass".
        @param title The title string which is supposed to appear in the tool bar and menu related to this plugin.

        Registration of the plugin factory makes the object known to the system. Registration requires that the menu items have been set already. Hence it is recommended to put the registration at the end of the initialization method of the factory class.
        """
    @overload
    def register(self, position: int, name: str, title: str, icon: str) -> None:
        r"""
        @brief Registers the plugin factory
        @param position An integer that determines the order in which the plugins are created. The internal plugins use the values from 1000 to 50000.
        @param name The plugin name. This is an arbitrary string which should be unique. Hence it is recommended to use a unique prefix, i.e. "myplugin::ThePluginClass".
        @param title The title string which is supposed to appear in the tool bar and menu related to this plugin.
        @param icon The path to the icon that appears in the tool bar and menu related to this plugin.

        This version also allows registering an icon for the tool bar.

        Registration of the plugin factory makes the object known to the system. Registration requires that the menu items have been set already. Hence it is recommended to put the registration at the end of the initialization method of the factory class.
        """

