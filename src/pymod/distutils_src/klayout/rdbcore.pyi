from typing import Any, ClassVar, Dict, Sequence, List, Iterator, Optional
from typing import overload
import klayout.tl as tl
import klayout.db as db
class RdbCategory:
    r"""
    @brief A category inside the report database
    Every item in the report database is assigned to a category. A category is a DRC rule check for example. Categories can be organized hierarchically, i.e. a category may have sub-categories. Item counts are summarized for categories and items belonging to sub-categories of one category can be browsed together for example. As a general rule, categories not being leaf categories (having child categories) may not have items. 
    """
    description: str
    r"""
    Getter:
    @brief Gets the category description
    @return The description string

    Setter:
    @brief Sets the category description
    @param description The description string
    """
    @classmethod
    def new(cls) -> RdbCategory:
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
    def database(self) -> ReportDatabase:
        r"""
        @brief Gets the database object that category is associated with

        This method has been introduced in version 0.23.
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
    def each_item(self) -> Iterator[RdbItem]:
        r"""
        @brief Iterates over all items inside the database which are associated with this category

        This method has been introduced in version 0.23.
        """
    def each_sub_category(self) -> Iterator[RdbCategory]:
        r"""
        @brief Iterates over all sub-categories
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def name(self) -> str:
        r"""
        @brief Gets the category name
        The category name is an string that identifies the category in the context of a parent category or inside the database when it is a top level category. The name is not the path name which is a path to a child category and incorporates all names of parent categories.
        @return The category name
        """
    def num_items(self) -> int:
        r"""
        @brief Gets the number of items in this category
        The number of items includes the items in sub-categories of this category.
        """
    def num_items_visited(self) -> int:
        r"""
        @brief Gets the number of visited items in this category
        The number of items includes the items in sub-categories of this category.
        """
    def parent(self) -> RdbCategory:
        r"""
        @brief Gets the parent category of this category
        @return The parent category or nil if this category is a top-level category
        """
    def path(self) -> str:
        r"""
        @brief Gets the category path
        The category path is the category name for top level categories. For child categories, the path contains the names of all parent categories separated by a dot.
        @return The path for this category
        """
    def rdb_id(self) -> int:
        r"""
        @brief Gets the category ID
        The category ID is an integer that uniquely identifies the category. It is used for referring to a category in \RdbItem for example.
        @return The category ID
        """
    @overload
    def scan_collection(self, cell: RdbCell, trans: db.CplxTrans, edge_pairs: db.EdgePairs, flat: Optional[bool] = ..., with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Turns the given edge pair collection into a hierarchical or flat report database
        This a another flavour of \scan_collection accepting an edge pair collection.

        This method has been introduced in version 0.26. The 'with_properties' argument has been added in version 0.28.
        """
    @overload
    def scan_collection(self, cell: RdbCell, trans: db.CplxTrans, edges: db.Edges, flat: Optional[bool] = ..., with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Turns the given edge collection into a hierarchical or flat report database
        This a another flavour of \scan_collection accepting an edge collection.

        This method has been introduced in version 0.26. The 'with_properties' argument has been added in version 0.28.
        """
    @overload
    def scan_collection(self, cell: RdbCell, trans: db.CplxTrans, region: db.Region, flat: Optional[bool] = ..., with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Turns the given region into a hierarchical or flat report database
        The exact behavior depends on the nature of the region. If the region is a hierarchical (original or deep) region and the 'flat' argument is false, this method will produce a hierarchical report database in the given category. The 'cell_id' parameter is ignored in this case. Sample references will be produced to supply minimal instantiation information.

        If the region is a flat one or the 'flat' argument is true, the region's polygons will be produced as report database items in this category and in the cell given by 'cell_id'.

        The transformation argument needs to supply the dbu-to-micron transformation.

        If 'with_properties' is true, user properties will be turned into tagged values as well.

        This method has been introduced in version 0.26. The 'with_properties' argument has been added in version 0.28.
        """
    @overload
    def scan_collection(self, cell: RdbCell, trans: db.CplxTrans, texts: db.Texts, flat: Optional[bool] = ..., with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Turns the given edge pair collection into a hierarchical or flat report database
        This a another flavour of \scan_collection accepting a text collection.

        This method has been introduced in version 0.28.
        """
    def scan_layer(self, layout: db.Layout, layer: int, cell: Optional[db.Cell] = ..., levels: Optional[int] = ..., with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Scans a layer from a layout into this category, starting with a given cell and a depth specification
        Creates RDB items for each polygon or edge shape read from the cell and its children in the layout on the given layer and puts them into this category.
        New cells will be generated when required.
        "levels" is the number of hierarchy levels to take the child cells from. 0 means to use only "cell" and don't descend, -1 means "all levels".
        Other settings like database unit, description, top cell etc. are not made in the RDB.

        If 'with_properties' is true, user properties will be turned into tagged values as well.

        This method has been introduced in version 0.23. The 'with_properties' argument has been added in version 0.28.
        """
    def scan_shapes(self, iter: db.RecursiveShapeIterator, flat: Optional[bool] = ..., with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Scans the polygon or edge shapes from the shape iterator into the category
        Creates RDB items for each polygon or edge shape read from the iterator and puts them into this category.
        A similar, but lower-level method is \ReportDatabase#create_items with a \RecursiveShapeIterator argument.
        In contrast to \ReportDatabase#create_items, 'scan_shapes' can also produce hierarchical databases if the \flat argument is false. In this case, the hierarchy the recursive shape iterator traverses is copied into the report database using sample references.

        If 'with_properties' is true, user properties will be turned into tagged values as well.

        This method has been introduced in version 0.23. The flat mode argument has been added in version 0.26. The 'with_properties' argument has been added in version 0.28.
        """

class RdbCell:
    r"""
    @brief A cell inside the report database
    This class represents a cell in the report database. There is not necessarily a 1:1 correspondence of RDB cells and layout database cells. Cells have an ID, a name, optionally a variant name and a set of references which describe at least one example instantiation in some parent cell. The references do not necessarily map to references or cover all references in the layout database.
    """
    @classmethod
    def new(cls) -> RdbCell:
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
    def add_reference(self, ref: RdbReference) -> None:
        r"""
        @brief Adds a reference to the references of this cell
        @param ref The reference to add.
        """
    def clear_references(self) -> None:
        r"""
        @brief Removes all references from this cell
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def database(self) -> ReportDatabase:
        r"""
        @brief Gets the database object that category is associated with

        This method has been introduced in version 0.23.
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
    def each_item(self) -> Iterator[RdbItem]:
        r"""
        @brief Iterates over all items inside the database which are associated with this cell

        This method has been introduced in version 0.23.
        """
    def each_reference(self) -> Iterator[RdbReference]:
        r"""
        @brief Iterates over all references
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def name(self) -> str:
        r"""
        @brief Gets the cell name
        The cell name is an string that identifies the category in the database. Additionally, a cell may carry a variant identifier which is a string that uniquely identifies a cell in the context of its variants. The "qualified name" contains both the cell name and the variant name. Cell names are also used to identify report database cell's with layout cells. @return The cell name
        """
    def num_items(self) -> int:
        r"""
        @brief Gets the number of items for this cell
        """
    def num_items_visited(self) -> int:
        r"""
        @brief Gets the number of visited items for this cell
        """
    def qname(self) -> str:
        r"""
        @brief Gets the cell's qualified name
        The qualified name is a combination of the cell name and optionally the variant name. It is used to identify the cell by name in a unique way.
        @return The qualified name
        """
    def rdb_id(self) -> int:
        r"""
        @brief Gets the cell ID
        The cell ID is an integer that uniquely identifies the cell. It is used for referring to a cell in \RdbItem for example.
        @return The cell ID
        """
    def variant(self) -> str:
        r"""
        @brief Gets the cell variant name
        A variant name additionally identifies the cell when multiple cells with the same name are present. A variant name is either assigned automatically or set when creating a cell. @return The cell variant name
        """

class RdbItem:
    r"""
    @brief An item inside the report database
    An item is the basic information entity in the RDB. It is associated with a cell and a category. It can be assigned values which encapsulate other objects such as strings and geometrical objects. In addition, items can be assigned an image (i.e. a screenshot image) and tags which are basically boolean flags that can be defined freely.
    """
    @property
    def image(self) -> None:
        r"""
        WARNING: This variable can only be set, not retrieved.
        @brief Sets the attached image from a PixelBuffer object

        This method has been added in version 0.28.
        """
    image_str: str
    r"""
    Getter:
    @brief Gets the image associated with this item as a string
    @return A base64-encoded image file (in PNG format)

    Setter:
    @brief Sets the image from a string
    @param image A base64-encoded image file (preferably in PNG format)
    """
    tags_str: str
    r"""
    Getter:
    @brief Returns a string listing all tags of this item
    @return A comma-separated list of tags

    Setter:
    @brief Sets the tags from a string
    @param tags A comma-separated list of tags
    """
    @classmethod
    def new(cls) -> RdbItem:
        r"""
        @brief Creates a new object of this class
        """
    def __copy__(self) -> RdbItem:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> RdbItem:
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
    def add_tag(self, tag_id: int) -> None:
        r"""
        @brief Adds a tag with the given id to the item
        Each tag can be added once to the item. The tags of an item thus form a set. If a tag with that ID already exists, this method does nothing.
        """
    @overload
    def add_value(self, shape: db.Shape, trans: db.CplxTrans) -> None:
        r"""
        @brief Adds a geometrical value object from a shape
        @param value The shape object from which to take the geometrical object.
        @param trans The transformation to apply.

        The transformation can be used to convert database units to micron units.

        This method has been introduced in version 0.25.3.
        """
    @overload
    def add_value(self, value: RdbItemValue) -> None:
        r"""
        @brief Adds a value object to the values of this item
        @param value The value to add.
        """
    @overload
    def add_value(self, value: db.DBox) -> None:
        r"""
        @brief Adds a box object to the values of this item
        @param value The box to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: db.DEdge) -> None:
        r"""
        @brief Adds an edge object to the values of this item
        @param value The edge to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: db.DEdgePair) -> None:
        r"""
        @brief Adds an edge pair object to the values of this item
        @param value The edge pair to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: db.DPolygon) -> None:
        r"""
        @brief Adds a polygon object to the values of this item
        @param value The polygon to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: float) -> None:
        r"""
        @brief Adds a numeric value to the values of this item
        @param value The value to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: str) -> None:
        r"""
        @brief Adds a string object to the values of this item
        @param value The string to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    def assign(self, other: RdbItem) -> None:
        r"""
        @brief Assigns another object to self
        """
    def category_id(self) -> int:
        r"""
        @brief Gets the category ID
        Returns the ID of the category that this item is associated with.
        @return The category ID
        """
    def cell_id(self) -> int:
        r"""
        @brief Gets the cell ID
        Returns the ID of the cell that this item is associated with.
        @return The cell ID
        """
    def clear_values(self) -> None:
        r"""
        @brief Removes all values from this item
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def database(self) -> ReportDatabase:
        r"""
        @brief Gets the database object that item is associated with

        This method has been introduced in version 0.23.
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
    def dup(self) -> RdbItem:
        r"""
        @brief Creates a copy of self
        """
    def each_value(self) -> Iterator[RdbItemValue]:
        r"""
        @brief Iterates over all values
        """
    def has_image(self) -> bool:
        r"""
        @brief Gets a value indicating that the item has an image attached
        See \image_str how to obtain the image.

        This method has been introduced in version 0.28.
        """
    def has_tag(self, tag_id: int) -> bool:
        r"""
        @brief Returns a value indicating whether the item has a tag with the given ID
        @return True, if the item has a tag with the given ID
        """
    def image_pixels(self) -> lay.PixelBuffer:
        r"""
        @brief Gets the attached image as a PixelBuffer object

        This method has been added in version 0.28.
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_visited(self) -> bool:
        r"""
        @brief Gets a value indicating whether the item was already visited
        @return True, if the item has been visited already
        """
    def remove_tag(self, tag_id: int) -> None:
        r"""
        @brief Remove the tag with the given id from the item
        If a tag with that ID does not exists on this item, this method does nothing.
        """

class RdbItemValue:
    r"""
    @brief A value object inside the report database
    Value objects are attached to items to provide markers. An arbitrary number of such value objects can be attached to an item.
    Currently, a value can represent a box, a polygon or an edge. Geometrical objects are represented in micron units and are therefore "D" type objects (DPolygon, DEdge and DBox). 
    """
    tag_id: int
    r"""
    Getter:
    @brief Gets the tag ID if the value is a tagged value or 0 if not
    @return The tag ID
    See \tag_id= for details about tagged values.

    Tagged values have been added in version 0.24.

    Setter:
    @brief Sets the tag ID to make the value a tagged value or 0 to reset it
    @param id The tag ID
    To get a tag ID, use \RdbDatabase#user_tag_id (preferred) or \RdbDatabase#tag_id (for internal use).
    Tagged values have been added in version 0.24. Tags can be given to identify a value, for example to attache measurement values to an item. To attach a value for a specific measurement, a tagged value can be used where the tag ID describes the measurement made. In that way, multiple values for different measurements can be attached to an item.

    This variant has been introduced in version 0.24
    """
    @classmethod
    def from_s(cls, s: str) -> RdbItemValue:
        r"""
        @brief Creates a value object from a string
        The string format is the same than obtained by the to_s method.
        """
    @overload
    @classmethod
    def new(cls, b: db.DBox) -> RdbItemValue:
        r"""
        @brief Creates a value representing a DBox object
        """
    @overload
    @classmethod
    def new(cls, e: db.DEdge) -> RdbItemValue:
        r"""
        @brief Creates a value representing a DEdge object
        """
    @overload
    @classmethod
    def new(cls, ee: db.DEdgePair) -> RdbItemValue:
        r"""
        @brief Creates a value representing a DEdgePair object
        """
    @overload
    @classmethod
    def new(cls, f: float) -> RdbItemValue:
        r"""
        @brief Creates a value representing a numeric value

        This variant has been introduced in version 0.24
        """
    @overload
    @classmethod
    def new(cls, p: db.DPath) -> RdbItemValue:
        r"""
        @brief Creates a value representing a DPath object

        This method has been introduced in version 0.22.
        """
    @overload
    @classmethod
    def new(cls, p: db.DPolygon) -> RdbItemValue:
        r"""
        @brief Creates a value representing a DPolygon object
        """
    @overload
    @classmethod
    def new(cls, s: str) -> RdbItemValue:
        r"""
        @brief Creates a value representing a string
        """
    @overload
    @classmethod
    def new(cls, t: db.DText) -> RdbItemValue:
        r"""
        @brief Creates a value representing a DText object

        This method has been introduced in version 0.22.
        """
    def __copy__(self) -> RdbItemValue:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> RdbItemValue:
        r"""
        @brief Creates a copy of self
        """
    @overload
    def __init__(self, b: db.DBox) -> None:
        r"""
        @brief Creates a value representing a DBox object
        """
    @overload
    def __init__(self, e: db.DEdge) -> None:
        r"""
        @brief Creates a value representing a DEdge object
        """
    @overload
    def __init__(self, ee: db.DEdgePair) -> None:
        r"""
        @brief Creates a value representing a DEdgePair object
        """
    @overload
    def __init__(self, f: float) -> None:
        r"""
        @brief Creates a value representing a numeric value

        This variant has been introduced in version 0.24
        """
    @overload
    def __init__(self, p: db.DPath) -> None:
        r"""
        @brief Creates a value representing a DPath object

        This method has been introduced in version 0.22.
        """
    @overload
    def __init__(self, p: db.DPolygon) -> None:
        r"""
        @brief Creates a value representing a DPolygon object
        """
    @overload
    def __init__(self, s: str) -> None:
        r"""
        @brief Creates a value representing a string
        """
    @overload
    def __init__(self, t: db.DText) -> None:
        r"""
        @brief Creates a value representing a DText object

        This method has been introduced in version 0.22.
        """
    def __repr__(self) -> str:
        r"""
        @brief Converts a value to a string
        The string can be used by the string constructor to create another object from it.
        @return The string
        """
    def __str__(self) -> str:
        r"""
        @brief Converts a value to a string
        The string can be used by the string constructor to create another object from it.
        @return The string
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
    def assign(self, other: RdbItemValue) -> None:
        r"""
        @brief Assigns another object to self
        """
    def box(self) -> db.DBox:
        r"""
        @brief Gets the box if the value represents one.
        @return The \DBox object or nil
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
    def dup(self) -> RdbItemValue:
        r"""
        @brief Creates a copy of self
        """
    def edge(self) -> db.DEdge:
        r"""
        @brief Gets the edge if the value represents one.
        @return The \DEdge object or nil
        """
    def edge_pair(self) -> db.DEdgePair:
        r"""
        @brief Gets the edge pair if the value represents one.
        @return The \DEdgePair object or nil
        """
    def float(self) -> float:
        r"""
        @brief Gets the numeric value.
        @return The numeric value or 0
        This method has been introduced in version 0.24.
        """
    def is_box(self) -> bool:
        r"""
        @brief Returns true if the value object represents a box
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_edge(self) -> bool:
        r"""
        @brief Returns true if the value object represents an edge
        """
    def is_edge_pair(self) -> bool:
        r"""
        @brief Returns true if the value object represents an edge pair
        """
    def is_float(self) -> bool:
        r"""
        @brief Returns true if the value object represents a numeric value
        This method has been introduced in version 0.24.
        """
    def is_path(self) -> bool:
        r"""
        @brief Returns true if the value object represents a path

        This method has been introduced in version 0.22.
        """
    def is_polygon(self) -> bool:
        r"""
        @brief Returns true if the value object represents a polygon
        """
    def is_string(self) -> bool:
        r"""
        @brief Returns true if the object represents a string value
        """
    def is_text(self) -> bool:
        r"""
        @brief Returns true if the value object represents a text

        This method has been introduced in version 0.22.
        """
    def path(self) -> db.DPath:
        r"""
        @brief Gets the path if the value represents one.
        @return The \DPath object
        This method has been introduced in version 0.22.
        """
    def polygon(self) -> db.DPolygon:
        r"""
        @brief Gets the polygon if the value represents one.
        @return The \DPolygon object
        """
    def string(self) -> str:
        r"""
        @brief Gets the string representation of the value.
        @return The stringThis method will always deliver a valid string, even if \is_string? is false. The objects stored in the value are converted to a string accordingly.
        """
    def text(self) -> db.DText:
        r"""
        @brief Gets the text if the value represents one.
        @return The \DText object
        This method has been introduced in version 0.22.
        """
    def to_s(self) -> str:
        r"""
        @brief Converts a value to a string
        The string can be used by the string constructor to create another object from it.
        @return The string
        """

class RdbReference:
    r"""
    @brief A cell reference inside the report database
    This class describes a cell reference. Such reference object can be attached to cells to describe instantiations of them in parent cells. Not necessarily all instantiations of a cell in the layout database are represented by references and in some cases there might even be no references at all. The references are merely a hint how a marker must be displayed in the context of any other, potentially parent, cell in the layout database.
    """
    parent_cell_id: int
    r"""
    Getter:
    @brief Gets parent cell ID for this reference
    @return The parent cell ID

    Setter:
    @brief Sets the parent cell ID for this reference
    """
    trans: db.DCplxTrans
    r"""
    Getter:
    @brief Gets the transformation for this reference
    The transformation describes the transformation of the child cell into the parent cell. In that sense that is the usual transformation of a cell reference.
    @return The transformation

    Setter:
    @brief Sets the transformation for this reference
    """
    @classmethod
    def new(cls, trans: db.DCplxTrans, parent_cell_id: int) -> RdbReference:
        r"""
        @brief Creates a reference with a given transformation and parent cell ID
        """
    def __copy__(self) -> RdbReference:
        r"""
        @brief Creates a copy of self
        """
    def __deepcopy__(self) -> RdbReference:
        r"""
        @brief Creates a copy of self
        """
    def __init__(self, trans: db.DCplxTrans, parent_cell_id: int) -> None:
        r"""
        @brief Creates a reference with a given transformation and parent cell ID
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
    def assign(self, other: RdbReference) -> None:
        r"""
        @brief Assigns another object to self
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def database(self) -> ReportDatabase:
        r"""
        @brief Gets the database object that category is associated with

        This method has been introduced in version 0.23.
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
    def dup(self) -> RdbReference:
        r"""
        @brief Creates a copy of self
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """

class ReportDatabase:
    r"""
    @brief The report database object
    A report database is organized around a set of items which are associated with cells and categories. Categories can be organized hierarchically by created sub-categories of other categories. Cells are associated with layout database cells and can come with a example instantiation if the layout database does not allow a unique association of the cells.
    Items in the database can have a variety of attributes: values, tags and an image object. Values are geometrical objects for example. Tags are a set of boolean flags and an image can be attached to an item to provide a screenshot for visualization for example.
    This is the main report database object. The basic use case of this object is to create one inside a \LayoutView and populate it with items, cell and categories or load it from a file. Another use case is to create a standalone ReportDatabase object and use the methods provided to perform queries or to populate it.
    """
    description: str
    r"""
    Getter:
    @brief Gets the databases description
    The description is a general purpose string that is supposed to further describe the database and its content in a human-readable form.
    @return The description string

    Setter:
    @brief Sets the databases description
    @param desc The description string
    """
    generator: str
    r"""
    Getter:
    @brief Gets the databases generator
    The generator string describes how the database was created, i.e. DRC tool name and tool options.
    In a later version this will allow re-running the tool that created the report.
    @return The generator string

    Setter:
    @brief Sets the generator string
    @param generator The generator string
    """
    original_file: str
    r"""
    Getter:
    @brief Gets the original file name and path
    The original file name is supposed to describe the file from which this report database was generated. @return The original file name and path

    Setter:
    @brief Sets the original file name and path
    @param path The path
    """
    top_cell_name: str
    r"""
    Getter:
    @brief Gets the top cell name
    The top cell name identifies the top cell of the design for which the report was generated. This property must be set to establish a proper hierarchical context for a hierarchical report database. @return The top cell name

    Setter:
    @brief Sets the top cell name string
    @param cell_name The top cell name
    """
    @classmethod
    def new(cls, name: str) -> ReportDatabase:
        r"""
        @brief Creates a report database
        @param name The name of the database
        The name of the database will be used in the user interface to refer to a certain database.
        """
    def __init__(self, name: str) -> None:
        r"""
        @brief Creates a report database
        @param name The name of the database
        The name of the database will be used in the user interface to refer to a certain database.
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
    def category_by_id(self, id: int) -> RdbCategory:
        r"""
        @brief Gets a category by ID
        @return The (const) category object or nil if the ID is not valid
        """
    def category_by_path(self, path: str) -> RdbCategory:
        r"""
        @brief Gets a category by path
        @param path The full path to the category starting from the top level (subcategories separated by dots)
        @return The (const) category object or nil if the name is not valid
        """
    def cell_by_id(self, id: int) -> RdbCell:
        r"""
        @brief Returns the cell for a given ID
        @param id The ID of the cell
        @return The cell object or nil if no cell with that ID exists
        """
    def cell_by_qname(self, qname: str) -> RdbCell:
        r"""
        @brief Returns the cell for a given qualified name
        @param qname The qualified name of the cell (name plus variant name optionally)
        @return The cell object or nil if no such cell exists
        """
    def create(self) -> None:
        r"""
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    @overload
    def create_category(self, name: str) -> RdbCategory:
        r"""
        @brief Creates a new top level category
        @param name The name of the category
        """
    @overload
    def create_category(self, parent: RdbCategory, name: str) -> RdbCategory:
        r"""
        @brief Creates a new sub-category
        @param parent The category under which the category should be created
        @param name The name of the category
        """
    @overload
    def create_cell(self, name: str) -> RdbCell:
        r"""
        @brief Creates a new cell
        @param name The name of the cell
        """
    @overload
    def create_cell(self, name: str, variant: str) -> RdbCell:
        r"""
        @brief Creates a new cell, potentially as a variant for a cell with the same name
        @param name The name of the cell
        @param variant The variant name of the cell
        """
    @overload
    def create_item(self, cell: RdbCell, category: RdbCategory) -> RdbItem:
        r"""
        @brief Creates a new item for the given cell/category combination
        @param cell The cell to which the item is associated
        @param category The category to which the item is associated

        This convenience method has been added in version 0.25.
        """
    @overload
    def create_item(self, cell_id: int, category_id: int) -> RdbItem:
        r"""
        @brief Creates a new item for the given cell/category combination
        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated

        A more convenient method that takes cell and category objects instead of ID's is the other version of \create_item.
        """
    @overload
    def create_item(self, cell_id: int, category_id: int, trans: db.CplxTrans, shape: db.Shape, with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Creates a new item from a single shape
        This method produces an item from the given shape.
        It accepts various kind of shapes, such as texts, polygons, boxes and paths and converts them to a corresponding item. The transformation argument can be used to supply the transformation that applies the database unit for example.

        This method has been introduced in version 0.25.3. The 'with_properties' argument has been added in version 0.28.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param shape The shape to take the geometrical object from
        @param trans The transformation to apply
        @param with_properties If true, user properties will be turned into tagged values as well
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, iter: db.RecursiveShapeIterator, with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Creates new items from a shape iterator
        This method takes the shapes from the given iterator and produces items from them.
        It accepts various kind of shapes, such as texts, polygons, boxes and paths and converts them to corresponding items. This method will produce a flat version of the shapes iterated by the shape iterator. A similar method, which is intended for production of polygon or edge error layers and also provides hierarchical database construction is \RdbCategory#scan_shapes.

        This method has been introduced in version 0.25.3. The 'with_properties' argument has been added in version 0.28.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param iter The iterator (a \RecursiveShapeIterator object) from which to take the items
        @param with_properties If true, user properties will be turned into tagged values as well
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: db.CplxTrans, array: Sequence[db.EdgePair]) -> None:
        r"""
        @brief Creates new edge pair items for the given cell/category combination
        For each edge pair a single item will be created. The value of the item will be this edge pair.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method has been introduced in version 0.23.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param edge_pairs The list of edge_pairs for which the items are created
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: db.CplxTrans, array: Sequence[db.Edge]) -> None:
        r"""
        @brief Creates new edge items for the given cell/category combination
        For each edge a single item will be created. The value of the item will be this edge.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method has been introduced in version 0.23.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param edges The list of edges for which the items are created
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: db.CplxTrans, array: Sequence[db.Polygon]) -> None:
        r"""
        @brief Creates new polygon items for the given cell/category combination
        For each polygon a single item will be created. The value of the item will be this polygon.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method has been introduced in version 0.23.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param polygons The list of polygons for which the items are created
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: db.CplxTrans, edge_pairs: db.EdgePairs) -> None:
        r"""
        @brief Creates new edge pair items for the given cell/category combination
        For each edge pair a single item will be created. The value of the item will be this edge pair.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method will also produce a flat version of the edge pairs inside the edge pair collection. \RdbCategory#scan_collection is a similar method which also supports construction of hierarchical databases from deep edge pair collections.

        This method has been introduced in version 0.23. It has been deprecated in favor of \RdbCategory#scan_collection in version 0.28.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param edges The list of edge pairs (an \EdgePairs object) for which the items are created
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: db.CplxTrans, edges: db.Edges) -> None:
        r"""
        @brief Creates new edge items for the given cell/category combination
        For each edge a single item will be created. The value of the item will be this edge.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method will also produce a flat version of the edges inside the edge collection. \RdbCategory#scan_collection is a similar method which also supports construction of hierarchical databases from deep edge collections.

        This method has been introduced in version 0.23. It has been deprecated in favor of \RdbCategory#scan_collection in version 0.28.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param edges The list of edges (an \Edges object) for which the items are created
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: db.CplxTrans, region: db.Region) -> None:
        r"""
        @brief Creates new polygon items for the given cell/category combination
        For each polygon in the region a single item will be created. The value of the item will be this polygon.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method will also produce a flat version of the shapes inside the region. \RdbCategory#scan_collection is a similar method which also supports construction of hierarchical databases from deep regions.

        This method has been introduced in version 0.23. It has been deprecated in favor of \RdbCategory#scan_collection in version 0.28.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param region The region (a \Region object) containing the polygons for which to create items
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: db.CplxTrans, shapes: db.Shapes, with_properties: Optional[bool] = ...) -> None:
        r"""
        @brief Creates new items from a shape container
        This method takes the shapes from the given container and produces items from them.
        It accepts various kind of shapes, such as texts, polygons, boxes and paths and converts them to corresponding items. The transformation argument can be used to supply the transformation that applies the database unit for example.

        This method has been introduced in version 0.25.3. The 'with_properties' argument has been added in version 0.28.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param shapes The shape container from which to take the items
        @param trans The transformation to apply
        @param with_properties If true, user properties will be turned into tagged values as well
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
    def each_category(self) -> Iterator[RdbCategory]:
        r"""
        @brief Iterates over all top-level categories
        """
    def each_cell(self) -> Iterator[RdbCell]:
        r"""
        @brief Iterates over all cells
        """
    def each_item(self) -> Iterator[RdbItem]:
        r"""
        @brief Iterates over all items inside the database
        """
    def each_item_per_category(self, category_id: int) -> Iterator[RdbItem]:
        r"""
        @brief Iterates over all items inside the database which are associated with the given category
        @param category_id The ID of the category for which all associated items should be retrieved
        """
    def each_item_per_cell(self, cell_id: int) -> Iterator[RdbItem]:
        r"""
        @brief Iterates over all items inside the database which are associated with the given cell
        @param cell_id The ID of the cell for which all associated items should be retrieved
        """
    def each_item_per_cell_and_category(self, cell_id: int, category_id: int) -> Iterator[RdbItem]:
        r"""
        @brief Iterates over all items inside the database which are associated with the given cell and category
        @param cell_id The ID of the cell for which all associated items should be retrieved
        @param category_id The ID of the category for which all associated items should be retrieved
        """
    def filename(self) -> str:
        r"""
        @brief Gets the file name and path where the report database is stored
        This property is set when a database is saved or loaded. It cannot be set manually.
        @return The file name and path
        """
    def is_const_object(self) -> bool:
        r"""
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_modified(self) -> bool:
        r"""
        @brief Returns a value indicating whether the database has been modified
        """
    def load(self, filename: str) -> None:
        r"""
        @brief Loads the database from the given file
        @param filename The file from which to load the database
        The reader recognizes the format automatically and will choose the appropriate decoder. 'gzip' compressed files are uncompressed automatically.
        """
    def name(self) -> str:
        r"""
        @brief Gets the database name
        The name of the database is supposed to identify the database within a layout view context. The name is modified to be unique when a database is entered into a layout view. @return The database name
        """
    @overload
    def num_items(self) -> int:
        r"""
        @brief Returns the number of items inside the database
        @return The total number of items
        """
    @overload
    def num_items(self, cell_id: int, category_id: int) -> int:
        r"""
        @brief Returns the number of items inside the database for a given cell/category combination
        @param cell_id The ID of the cell for which to retrieve the number
        @param category_id The ID of the category for which to retrieve the number
        @return The total number of items for the given cell and the given category
        """
    @overload
    def num_items_visited(self) -> int:
        r"""
        @brief Returns the number of items already visited inside the database
        @return The total number of items already visited
        """
    @overload
    def num_items_visited(self, cell_id: int, category_id: int) -> int:
        r"""
        @brief Returns the number of items visited already for a given cell/category combination
        @param cell_id The ID of the cell for which to retrieve the number
        @param category_id The ID of the category for which to retrieve the number
        @return The total number of items visited for the given cell and the given category
        """
    def reset_modified(self) -> None:
        r"""
        @brief Reset the modified flag
        """
    def save(self, filename: str) -> None:
        r"""
        @brief Saves the database to the given file
        @param filename The file to which to save the database
        The database is always saved in KLayout's XML-based format.
        """
    def set_item_visited(self, item: RdbItem, visited: bool) -> None:
        r"""
        @brief Modifies the visited state of an item
        @param item The item to modify
        @param visited True to set the item to visited state, false otherwise
        """
    def set_tag_description(self, tag_id: int, description: str) -> None:
        r"""
        @brief Sets the tag description for the given tag ID
        @param tag_id The ID of the tag
        @param description The description string
        See \tag_id for a details about tags.
        """
    def tag_description(self, tag_id: int) -> str:
        r"""
        @brief Gets the tag description for the given tag ID
        @param tag_id The ID of the tag
        @return The description string
        See \tag_id for a details about tags.
        """
    def tag_id(self, name: str) -> int:
        r"""
        @brief Gets the tag ID for a given tag name
        @param name The tag name
        @return The corresponding tag ID
        Tags are used to tag items in the database and to specify tagged (named) values. This method will always succeed and the tag will be created if it does not exist yet. Tags are basically names. There are user tags (for free assignment) and system tags which are used within the system. Both are separated to avoid name clashes.

        \tag_id handles system tags while \user_tag_id handles user tags.
        """
    def tag_name(self, tag_id: int) -> str:
        r"""
        @brief Gets the tag name for the given tag ID
        @param tag_id The ID of the tag
        @return The name of the tag
        See \tag_id for a details about tags.

        This method has been introduced in version 0.24.10.
        """
    def user_tag_id(self, name: str) -> int:
        r"""
        @brief Gets the tag ID for a given user tag name
        @param name The user tag name
        @return The corresponding tag ID
        This method will always succeed and the tag will be created if it does not exist yet. See \tag_id for a details about tags.

        This method has been added in version 0.24.
        """
    def variants(self, name: str) -> List[int]:
        r"""
        @brief Gets the variants for a given cell name
        @param name The basic name of the cell
        @return An array of ID's representing cells that are variants for the given base name
        """

