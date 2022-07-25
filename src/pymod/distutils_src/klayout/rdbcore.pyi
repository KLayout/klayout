from typing import Any, ClassVar, Iterable, Optional

from typing import overload

from klayout.dbcore import *

class RdbCategory:
    """
    @brief A category inside the report database
    Every item in the report database is assigned to a category. A category is a DRC rule check for example. Categories can be organized hierarchically, i.e. a category may have sub-categories. Item counts are summarized for categories and items belonging to sub-categories of one category can be browsed together for example. As a general rule, categories not being leaf categories (having child categories) may not have items.
    """


    __gsi_id__: ClassVar[int] = ...
    description: str
    """
    description() -> str
    @brief Gets the category description
    @return The description string

    ------
    description(description: str) -> None
    @brief Sets the category description
    @param description The description string
    """
    def __init__(self) -> RdbCategory:
        """
        __init__() -> RdbCategory
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
    def database(self) -> ReportDatabase:
        """
        database() -> ReportDatabase
        @brief Gets the database object that category is associated with

        This method has been introduced in version 0.23.
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
    def each_item(self) -> RdbItem:
        """
        each_item() -> RdbItem
        @brief Iterates over all items inside the database which are associated with this category

        This method has been introduced in version 0.23.
        """
    def each_sub_category(self) -> RdbCategory:
        """
        each_sub_category() -> RdbCategory
        @brief Iterates over all sub-categories
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def name(self) -> str:
        """
        name() -> str
        @brief Gets the category name
        The category name is an string that identifies the category in the context of a parent category or inside the database when it is a top level category. The name is not the path name which is a path to a child category and incorporates all names of parent categories.
        @return The category name
        """
    @classmethod
    def new(cls) -> RdbCategory:
        """
        new() -> RdbCategory
        @brief Creates a new object of this class
        """
    def num_items(self) -> int:
        """
        num_items() -> int
        @brief Gets the number of items in this category
        The number of items includes the items in sub-categories of this category.
        """
    def num_items_visited(self) -> int:
        """
        num_items_visited() -> int
        @brief Gets the number of visited items in this category
        The number of items includes the items in sub-categories of this category.
        """
    def parent(self) -> RdbCategory:
        """
        parent() -> RdbCategory
        @brief Gets the parent category of this category
        @return The parent category or nil if this category is a top-level category
        """
    def path(self) -> str:
        """
        path() -> str
        @brief Gets the category path
        The category path is the category name for top level categories. For child categories, the path contains the names of all parent categories separated by a dot.
        @return The path for this category
        """
    def rdb_id(self) -> int:
        """
        rdb_id() -> int
        @brief Gets the category ID
        The category ID is an integer that uniquely identifies the category. It is used for referring to a category in \RdbItem for example.
        @return The category ID
        """
    @overload
    def scan_collection(self, cell: RdbCell, trans: CplxTrans, region: Region, flat: Optional[bool] = ...) -> None:
        """
        scan_collection(cell: RdbCell, trans: CplxTrans, region: Region, flat: Optional[bool] = None) -> None
        @brief Turns the given region into a hierarchical or flat report database
        The exact behavior depends on the nature of the region. If the region is a hierarchical (original or deep) region and the 'flat' argument is false, this method will produce a hierarchical report database in the given category. The 'cell_id' parameter is ignored in this case. Sample references will be produced to supply minimal instantiation information.

        If the region is a flat one or the 'flat' argument is true, the region's polygons will be produced as report database items in this category and in the cell given by 'cell_id'.

        The transformation argument needs to supply the dbu-to-micron transformation.

        This method has been introduced in version 0.26.
        """
    @overload
    def scan_collection(self, cell: RdbCell, trans: CplxTrans, edges: Edges, flat: Optional[bool] = ...) -> None:
        """
        scan_collection(cell: RdbCell, trans: CplxTrans, edges: Edges, flat: Optional[bool] = None) -> None
        @brief Turns the given edge collection into a hierarchical or flat report database
        This a another flavour of \scan_collection accepting an edge collection.

        This method has been introduced in version 0.26.
        """
    @overload
    def scan_collection(self, cell: RdbCell, trans: CplxTrans, edge_pairs: EdgePairs, flat: Optional[bool] = ...) -> None:
        """
        scan_collection(cell: RdbCell, trans: CplxTrans, edge_pairs: EdgePairs, flat: Optional[bool] = None) -> None
        @brief Turns the given edge pair collection into a hierarchical or flat report database
        This a another flavour of \scan_collection accepting an edge pair collection.

        This method has been introduced in version 0.26.
        """
    @overload
    def scan_layer(self, layout: Layout, layer: int) -> None:
        """
        scan_layer(layout: Layout, layer: int) -> None
        @brief Scans a layer from a layout into this category
        Creates RDB items for each polygon or edge shape read from the each cell in the layout on the given layer and puts them into this category.
        New cells will be generated for every cell encountered in the layout.
        Other settings like database unit, description, top cell etc. are not made in the RDB.

        This method has been introduced in version 0.23.
        """
    @overload
    def scan_layer(self, layout: Layout, layer: int, cell: Cell) -> None:
        """
        scan_layer(layout: Layout, layer: int, cell: Cell) -> None
        @brief Scans a layer from a layout into this category, starting with a given cell
        Creates RDB items for each polygon or edge shape read from the cell and it's children in the layout on the given layer and puts them into this category.
        New cells will be generated when required.
        Other settings like database unit, description, top cell etc. are not made in the RDB.

        This method has been introduced in version 0.23.
        """
    @overload
    def scan_layer(self, layout: Layout, layer: int, cell: Cell, levels: int) -> None:
        """
        scan_layer(layout: Layout, layer: int, cell: Cell, levels: int) -> None
        @brief Scans a layer from a layout into this category, starting with a given cell and a depth specification
        Creates RDB items for each polygon or edge shape read from the cell and it's children in the layout on the given layer and puts them into this category.
        New cells will be generated when required.
        "levels" is the number of hierarchy levels to take the child cells from. 0 means to use only "cell" and don't descend, -1 means "all levels".
        Other settings like database unit, description, top cell etc. are not made in the RDB.

        This method has been introduced in version 0.23.
        """
    def scan_shapes(self, iter: RecursiveShapeIterator, flat: Optional[bool] = ...) -> None:
        """
        scan_shapes(iter: RecursiveShapeIterator, flat: Optional[bool] = None) -> None
        @brief Scans the polygon or edge shapes from the shape iterator into the category
        Creates RDB items for each polygon or edge shape read from the iterator and puts them into this category.
        A similar, but lower-level method is \ReportDatabase#create_items with a \RecursiveShapeIterator argument.
        In contrast to \ReportDatabase#create_items, 'scan_shapes' can also produce hierarchical databases if the \flat argument is false. In this case, the hierarchy the recursive shape iterator traverses is copied into the report database using sample references.

        This method has been introduced in version 0.23. The flat mode argument has been added in version 0.26.
        """

class RdbCell:
    """
    @brief A cell inside the report database
    This class represents a cell in the report database. There is not necessarily a 1:1 correspondence of RDB cells and layout database cells. Cells have an ID, a name, optionally a variant name and a set of references which describe at least one example instantiation in some parent cell. The references do not necessarily map to references or cover all references in the layout database.
    """


    __gsi_id__: ClassVar[int] = ...
    def __init__(self) -> RdbCell:
        """
        __init__() -> RdbCell
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
    def add_reference(self, ref: RdbReference) -> None:
        """
        add_reference(ref: RdbReference) -> None
        @brief Adds a reference to the references of this cell
        @param ref The reference to add.
        """
    def clear_references(self) -> None:
        """
        clear_references() -> None
        @brief Removes all references from this cell
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def database(self) -> ReportDatabase:
        """
        database() -> ReportDatabase
        @brief Gets the database object that category is associated with

        This method has been introduced in version 0.23.
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
    def each_item(self) -> RdbItem:
        """
        each_item() -> RdbItem
        @brief Iterates over all items inside the database which are associated with this cell

        This method has been introduced in version 0.23.
        """
    def each_reference(self) -> RdbReference:
        """
        each_reference() -> RdbReference
        @brief Iterates over all references
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def name(self) -> str:
        """
        name() -> str
        @brief Gets the cell name
        The cell name is an string that identifies the category in the database. Additionally, a cell may carry a variant identifier which is a string that uniquely identifies a cell in the context of it's variants. The "qualified name" contains both the cell name and the variant name. Cell names are also used to identify report database cell's with layout cells. @return The cell name
        """
    @classmethod
    def new(cls) -> RdbCell:
        """
        new() -> RdbCell
        @brief Creates a new object of this class
        """
    def num_items(self) -> int:
        """
        num_items() -> int
        @brief Gets the number of items for this cell
        """
    def num_items_visited(self) -> int:
        """
        num_items_visited() -> int
        @brief Gets the number of visited items for this cell
        """
    def qname(self) -> str:
        """
        qname() -> str
        @brief Gets the cell's qualified name
        The qualified name is a combination of the cell name and optionally the variant name. It is used to identify the cell by name in a unique way.
        @return The qualified name
        """
    def rdb_id(self) -> int:
        """
        rdb_id() -> int
        @brief Gets the cell ID
        The cell ID is an integer that uniquely identifies the cell. It is used for referring to a cell in \RdbItem for example.
        @return The cell ID
        """
    def variant(self) -> str:
        """
        variant() -> str
        @brief Gets the cell variant name
        A variant name additionally identifies the cell when multiple cells with the same name are present. A variant name is either assigned automatically or set when creating a cell. @return The cell variant name
        """

class RdbItem:
    """
    @brief An item inside the report database
    An item is the basic information entity in the RDB. It is associated with a cell and a category. It can be assigned values which encapsulate other objects such as strings and geometrical objects. In addition, items can be assigned an image (i.e. a screenshot image) and tags which are basically boolean flags that can be defined freely.
    """


    __gsi_id__: ClassVar[int] = ...
    tags_str: str
    """
    tags_str() -> str
    @brief Returns a string listing all tags of this item
    @return A comma-separated list of tags

    ------
    tags_str(tags: str) -> None
    @brief Sets the tags from a string
    @param tags A comma-separated list of tags
    """
    def __init__(self) -> RdbItem:
        """
        __init__() -> RdbItem
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
    def add_tag(self, tag_id: int) -> None:
        """
        add_tag(tag_id: int) -> None
        @brief Adds a tag with the given id to the item
        Each tag can be added once to the item. The tags of an item thus form a set. If a tag with that ID already exists, this method does nothing.
        """
    @overload
    def add_value(self, value: RdbItemValue) -> None:
        """
        add_value(value: RdbItemValue) -> None
        @brief Adds a value object to the values of this item
        @param value The value to add.
        """
    @overload
    def add_value(self, value: DPolygon) -> None:
        """
        add_value(value: DPolygon) -> None
        @brief Adds a polygon object to the values of this item
        @param value The polygon to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: DBox) -> None:
        """
        add_value(value: DBox) -> None
        @brief Adds a box object to the values of this item
        @param value The box to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: DEdge) -> None:
        """
        add_value(value: DEdge) -> None
        @brief Adds an edge object to the values of this item
        @param value The edge to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: DEdgePair) -> None:
        """
        add_value(value: DEdgePair) -> None
        @brief Adds an edge pair object to the values of this item
        @param value The edge pair to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: str) -> None:
        """
        add_value(value: str) -> None
        @brief Adds a string object to the values of this item
        @param value The string to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, value: float) -> None:
        """
        add_value(value: float) -> None
        @brief Adds a numeric value to the values of this item
        @param value The value to add.
        This method has been introduced in version 0.25 as a convenience method.
        """
    @overload
    def add_value(self, shape: Shape, trans: CplxTrans) -> None:
        """
        add_value(shape: Shape, trans: CplxTrans) -> None
        @brief Adds a geometrical value object from a shape
        @param value The shape object from which to take the geometrical object.
        @param trans The transformation to apply.

        The transformation can be used to convert database units to micron units.

        This method has been introduced in version 0.25.3.
        """
    def category_id(self) -> int:
        """
        category_id() -> int
        @brief Gets the category ID
        Returns the ID of the category that this item is associated with.
        @return The category ID
        """
    def cell_id(self) -> int:
        """
        cell_id() -> int
        @brief Gets the cell ID
        Returns the ID of the cell that this item is associated with.
        @return The cell ID
        """
    def clear_values(self) -> None:
        """
        clear_values() -> None
        @brief Removes all values from this item
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def database(self) -> ReportDatabase:
        """
        database() -> ReportDatabase
        @brief Gets the database object that item is associated with

        This method has been introduced in version 0.23.
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
    def each_value(self) -> RdbItemValue:
        """
        each_value() -> RdbItemValue
        @brief Iterates over all values
        """
    def has_tag(self, tag_id: int) -> bool:
        """
        has_tag(tag_id: int) -> bool
        @brief Returns a value indicating whether the item has a tag with the given ID
        @return True, if the item has a tag with the given ID
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_visited(self) -> bool:
        """
        is_visited() -> bool
        @brief Gets a value indicating whether the item was already visited
        @return True, if the item has been visited already
        """
    @classmethod
    def new(cls) -> RdbItem:
        """
        new() -> RdbItem
        @brief Creates a new object of this class
        """
    def remove_tag(self, tag_id: int) -> None:
        """
        remove_tag(tag_id: int) -> None
        @brief Remove the tag with the given id from the item
        If a tag with that ID does not exists on this item, this method does nothing.
        """

class RdbItemValue:
    """
    @brief A value object inside the report database
    Value objects are attached to items to provide markers. An arbitrary number of such value objects can be attached to an item.
    Currently, a value can represent a box, a polygon or an edge. Geometrical objects are represented in micron units and are therefore "D" type objects (DPolygon, DEdge and DBox).
    """


    __gsi_id__: ClassVar[int] = ...
    tag_id: int
    """
    tag_id() -> int
    @brief Gets the tag ID if the value is a tagged value or 0 if not
    @return The tag ID
    See \tag_id= for details about tagged values.

    Tagged values have been added in version 0.24.

    ------
    tag_id(id: int) -> None
    @brief Sets the tag ID to make the value a tagged value or 0 to reset it
    @param id The tag ID
    To get a tag ID, use \RdbDatabase#user_tag_id (preferred) or \RdbDatabase#tag_id (for internal use).
    Tagged values have been added in version 0.24. Tags can be given to identify a value, for example to attache measurement values to an item. To attach a value for a specific measurement, a tagged value can be used where the tag ID describes the measurement made. In that way, multiple values for different measurements can be attached to an item.

    This variant has been introduced in version 0.24
    """
    @overload
    def __init__(self, f: float) -> RdbItemValue:
        """
        __init__(f: float) -> RdbItemValue
        @brief Creates a value representing a numeric value

        This variant has been introduced in version 0.24
        """
    @overload
    def __init__(self, s: str) -> RdbItemValue:
        """
        __init__(s: str) -> RdbItemValue
        @brief Creates a value representing a string
        """
    @overload
    def __init__(self, p: DPolygon) -> RdbItemValue:
        """
        __init__(p: DPolygon) -> RdbItemValue
        @brief Creates a value representing a DPolygon object
        """
    @overload
    def __init__(self, p: DPath) -> RdbItemValue:
        """
        __init__(p: DPath) -> RdbItemValue
        @brief Creates a value representing a DPath object

        This method has been introduced in version 0.22.
        """
    @overload
    def __init__(self, t: DText) -> RdbItemValue:
        """
        __init__(t: DText) -> RdbItemValue
        @brief Creates a value representing a DText object

        This method has been introduced in version 0.22.
        """
    @overload
    def __init__(self, e: DEdge) -> RdbItemValue:
        """
        __init__(e: DEdge) -> RdbItemValue
        @brief Creates a value representing a DEdge object
        """
    @overload
    def __init__(self, ee: DEdgePair) -> RdbItemValue:
        """
        __init__(ee: DEdgePair) -> RdbItemValue
        @brief Creates a value representing a DEdgePair object
        """
    @overload
    def __init__(self, b: DBox) -> RdbItemValue:
        """
        __init__(b: DBox) -> RdbItemValue
        @brief Creates a value representing a DBox object
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
    def assign(self, other: RdbItemValue) -> None:
        """
        assign(other: RdbItemValue) -> None
        @brief Assigns another object to self
        """
    def box(self) -> DBox:
        """
        box() -> DBox
        @brief Gets the box if the value represents one.
        @return The \DBox object or nil
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
    def dup(self) -> RdbItemValue:
        """
        dup() -> RdbItemValue
        @brief Creates a copy of self
        """
    def edge(self) -> DEdge:
        """
        edge() -> DEdge
        @brief Gets the edge if the value represents one.
        @return The \DEdge object or nil
        """
    def edge_pair(self) -> DEdgePair:
        """
        edge_pair() -> DEdgePair
        @brief Gets the edge pair if the value represents one.
        @return The \DEdgePair object or nil
        """
    def float(self) -> float:
        """
        float() -> float
        @brief Gets the numeric value.
        @return The numeric value or 0
        This method has been introduced in version 0.24.
        """
    @classmethod
    def from_s(cls, s: str) -> RdbItemValue:
        """
        from_s(s: str) -> RdbItemValue
        @brief Creates a value object from a string
        The string format is the same than obtained by the to_s method.
        """
    def is_box(self) -> bool:
        """
        is_box() -> bool
        @brief Returns true if the value object represents a box
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_edge(self) -> bool:
        """
        is_edge() -> bool
        @brief Returns true if the value object represents an edge
        """
    def is_edge_pair(self) -> bool:
        """
        is_edge_pair() -> bool
        @brief Returns true if the value object represents an edge pair
        """
    def is_float(self) -> bool:
        """
        is_float() -> bool
        @brief Returns true if the value object represents a numeric value
        This method has been introduced in version 0.24.
        """
    def is_path(self) -> bool:
        """
        is_path() -> bool
        @brief Returns true if the value object represents a path

        This method has been introduced in version 0.22.
        """
    def is_polygon(self) -> bool:
        """
        is_polygon() -> bool
        @brief Returns true if the value object represents a polygon
        """
    def is_string(self) -> bool:
        """
        is_string() -> bool
        @brief Returns true if the object represents a string value
        """
    def is_text(self) -> bool:
        """
        is_text() -> bool
        @brief Returns true if the value object represents a text

        This method has been introduced in version 0.22.
        """
    @classmethod
    @overload
    def new(cls, f: float) -> RdbItemValue:
        """
        new(f: float) -> RdbItemValue
        @brief Creates a value representing a numeric value

        This variant has been introduced in version 0.24
        """
    @overload
    def new(cls, s: str) -> RdbItemValue:
        """
        new(s: str) -> RdbItemValue
        @brief Creates a value representing a string
        """
    @overload
    def new(cls, p: DPolygon) -> RdbItemValue:
        """
        new(p: DPolygon) -> RdbItemValue
        @brief Creates a value representing a DPolygon object
        """
    @overload
    def new(cls, p: DPath) -> RdbItemValue:
        """
        new(p: DPath) -> RdbItemValue
        @brief Creates a value representing a DPath object

        This method has been introduced in version 0.22.
        """
    @overload
    def new(cls, t: DText) -> RdbItemValue:
        """
        new(t: DText) -> RdbItemValue
        @brief Creates a value representing a DText object

        This method has been introduced in version 0.22.
        """
    @overload
    def new(cls, e: DEdge) -> RdbItemValue:
        """
        new(e: DEdge) -> RdbItemValue
        @brief Creates a value representing a DEdge object
        """
    @overload
    def new(cls, ee: DEdgePair) -> RdbItemValue:
        """
        new(ee: DEdgePair) -> RdbItemValue
        @brief Creates a value representing a DEdgePair object
        """
    @overload
    def new(cls, b: DBox) -> RdbItemValue:
        """
        new(b: DBox) -> RdbItemValue
        @brief Creates a value representing a DBox object
        """
    def path(self) -> DPath:
        """
        path() -> DPath
        @brief Gets the path if the value represents one.
        @return The \DPath object
        This method has been introduced in version 0.22.
        """
    def polygon(self) -> DPolygon:
        """
        polygon() -> DPolygon
        @brief Gets the polygon if the value represents one.
        @return The \DPolygon object
        """
    def string(self) -> str:
        """
        string() -> str
        @brief Gets the string representation of the value.
        @return The stringThis method will always deliver a valid string, even if \is_string? is false. The objects stored in the value are converted to a string accordingly.
        """
    def text(self) -> DText:
        """
        text() -> DText
        @brief Gets the text if the value represents one.
        @return The \DText object
        This method has been introduced in version 0.22.
        """
    def to_s(self) -> str:
        """
        to_s() -> str
        @brief Converts a value to a string
        The string can be used by the string constructor to create another object from it.
        @return The string
        """

class RdbReference:
    """
    @brief A cell reference inside the report database
    This class describes a cell reference. Such reference object can be attached to cells to describe instantiations of them in parent cells. Not necessarily all instantiations of a cell in the layout database are represented by references and in some cases there might even be no references at all. The references are merely a hint how a marker must be displayed in the context of any other, potentially parent, cell in the layout database.
    """


    __gsi_id__: ClassVar[int] = ...
    parent_cell_id: int
    """
    parent_cell_id() -> int
    @brief Gets parent cell ID for this reference
    @return The parent cell ID

    ------
    parent_cell_id(id: int) -> None
    @brief Sets the parent cell ID for this reference
    """
    trans: DCplxTrans
    """
    trans() -> DCplxTrans
    @brief Gets the transformation for this reference
    The transformation describes the transformation of the child cell into the parent cell. In that sense that is the usual transformation of a cell reference.
    @return The transformation

    ------
    trans(trans: DCplxTrans) -> None
    @brief Sets the transformation for this reference
    """
    def __init__(self, trans: DCplxTrans, parent_cell_id: int) -> RdbReference:
        """
        __init__(trans: DCplxTrans, parent_cell_id: int) -> RdbReference
        @brief Creates a reference with a given transformation and parent cell ID
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
    def assign(self, other: RdbReference) -> None:
        """
        assign(other: RdbReference) -> None
        @brief Assigns another object to self
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    def database(self) -> ReportDatabase:
        """
        database() -> ReportDatabase
        @brief Gets the database object that category is associated with

        This method has been introduced in version 0.23.
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
    def dup(self) -> RdbReference:
        """
        dup() -> RdbReference
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
    def new(cls, trans: DCplxTrans, parent_cell_id: int) -> RdbReference:
        """
        new(trans: DCplxTrans, parent_cell_id: int) -> RdbReference
        @brief Creates a reference with a given transformation and parent cell ID
        """

class ReportDatabase:
    """
    @brief The report database object
    A report database is organized around a set of items which are associated with cells and categories. Categories can be organized hierarchically by created sub-categories of other categories. Cells are associated with layout database cells and can come with a example instantiation if the layout database does not allow a unique association of the cells.
    Items in the database can have a variety of attributes: values, tags and an image object. Values are geometrical objects for example. Tags are a set of boolean flags and an image can be attached to an item to provide a screenshot for visualization for example.
    This is the main report database object. The basic use case of this object is to create one inside a \LayoutView and populate it with items, cell and categories or load it from a file. Another use case is to create a standalone ReportDatabase object and use the methods provided to perform queries or to populate it.
    """


    __gsi_id__: ClassVar[int] = ...
    description: str
    """
    description() -> str
    @brief Gets the databases description
    The description is a general purpose string that is supposed to further describe the database and it's content in a human-readable form.
    @return The description string

    ------
    description(desc: str) -> None
    @brief Sets the databases description
    @param desc The description string
    """
    generator: str
    """
    generator() -> str
    @brief Gets the databases generator
    The generator string describes how the database was created, i.e. DRC tool name and tool options.
    In a later version this will allow re-running the tool that created the report.
    @return The generator string

    ------
    generator(generator: str) -> None
    @brief Sets the generator string
    @param generator The generator string
    """
    original_file: str
    """
    original_file() -> str
    @brief Gets the original file name and path
    The original file name is supposed to describe the file from which this report database was generated. @return The original file name and path

    ------
    original_file(path: str) -> None
    @brief Sets the original file name and path
    @param path The path
    """
    top_cell_name: str
    """
    top_cell_name() -> str
    @brief Gets the top cell name
    The top cell name identifies the top cell of the design for which the report was generated. This property must be set to establish a proper hierarchical context for a hierarchical report database. @return The top cell name

    ------
    top_cell_name(cell_name: str) -> None
    @brief Sets the top cell name string
    @param cell_name The top cell name
    """
    def __init__(self, name: str) -> ReportDatabase:
        """
        __init__(name: str) -> ReportDatabase
        @brief Creates a report database
        @param name The name of the database
        The name of the database will be used in the user interface to refer to a certain database.
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
    def category_by_id(self, id: int) -> RdbCategory:
        """
        category_by_id(id: int) -> RdbCategory
        @brief Gets a category by ID
        @return The (const) category object or nil if the ID is not valid
        """
    def category_by_path(self, path: str) -> RdbCategory:
        """
        category_by_path(path: str) -> RdbCategory
        @brief Gets a category by path
        @param path The full path to the category starting from the top level (subcategories separated by dots)
        @return The (const) category object or nil if the name is not valid
        """
    def cell_by_id(self, id: int) -> RdbCell:
        """
        cell_by_id(id: int) -> RdbCell
        @brief Returns the cell for a given ID
        @param id The ID of the cell
        @return The cell object or nil if no cell with that ID exists
        """
    def cell_by_qname(self, qname: str) -> RdbCell:
        """
        cell_by_qname(qname: str) -> RdbCell
        @brief Returns the cell for a given qualified name
        @param qname The qualified name of the cell (name plus variant name optionally)
        @return The cell object or nil if no such cell exists
        """
    def create(self) -> None:
        """
        create() -> None
        @brief Ensures the C++ object is created
        Use this method to ensure the C++ object is created, for example to ensure that resources are allocated. Usually C++ objects are created on demand and not necessarily when the script object is created.
        """
    @overload
    def create_category(self, name: str) -> RdbCategory:
        """
        create_category(name: str) -> RdbCategory
        @brief Creates a new top level category
        @param name The name of the category
        """
    @overload
    def create_category(self, parent: RdbCategory, name: str) -> RdbCategory:
        """
        create_category(parent: RdbCategory, name: str) -> RdbCategory
        @brief Creates a new sub-category
        @param parent The category under which the category should be created
        @param name The name of the category
        """
    @overload
    def create_cell(self, name: str) -> RdbCell:
        """
        create_cell(name: str) -> RdbCell
        @brief Creates a new cell
        @param name The name of the cell
        """
    @overload
    def create_cell(self, name: str, variant: str) -> RdbCell:
        """
        create_cell(name: str, variant: str) -> RdbCell
        @brief Creates a new cell, potentially as a variant for a cell with the same name
        @param name The name of the cell
        @param variant The variant name of the cell
        """
    @overload
    def create_item(self, cell_id: int, category_id: int) -> RdbItem:
        """
        create_item(cell_id: int, category_id: int) -> RdbItem
        @brief Creates a new item for the given cell/category combination
        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated

        A more convenient method that takes cell and category objects instead of ID's is the other version of \create_item.
        """
    @overload
    def create_item(self, cell: RdbCell, category: RdbCategory) -> RdbItem:
        """
        create_item(cell: RdbCell, category: RdbCategory) -> RdbItem
        @brief Creates a new item for the given cell/category combination
        @param cell The cell to which the item is associated
        @param category The category to which the item is associated

        This convenience method has been added in version 0.25.
        """
    @overload
    def create_item(self, cell_id: int, category_id: int, trans: CplxTrans, shape: Shape) -> None:
        """
        create_item(cell_id: int, category_id: int, trans: CplxTrans, shape: Shape) -> None
        @brief Creates a new item from a single shape
        This method produces an item from the given shape.
        It accepts various kind of shapes, such as texts, polygons, boxes and paths and converts them to a corresponding item. The transformation argument can be used to supply the transformation that applies the database unit for example.

        This method has been introduced in version 0.25.3.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param shape The shape to take the geometrical object from
        @param trans The transformation to apply
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, iter: RecursiveShapeIterator) -> None:
        """
        create_items(cell_id: int, category_id: int, iter: RecursiveShapeIterator) -> None
        @brief Creates new items from a shape iterator
        This method takes the shapes from the given iterator and produces items from them.
        It accepts various kind of shapes, such as texts, polygons, boxes and paths and converts them to corresponding items. This method will produce a flat version of the shapes iterated by the shape iterator. A similar method, which is intended for production of polygon or edge error layers and also provides hierarchical database construction is \RdbCategory#scan_shapes.

        This method has been introduced in version 0.25.3.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param iter The iterator (a \RecursiveShapeIterator object) from which to take the items
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: CplxTrans, shapes: Shapes) -> None:
        """
        create_items(cell_id: int, category_id: int, trans: CplxTrans, shapes: Shapes) -> None
        @brief Creates new items from a shape container
        This method takes the shapes from the given container and produces items from them.
        It accepts various kind of shapes, such as texts, polygons, boxes and paths and converts them to corresponding items. The transformation argument can be used to supply the transformation that applies the database unit for example.

        This method has been introduced in version 0.25.3.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param shapes The shape container from which to take the items
        @param trans The transformation to apply
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: CplxTrans, region: Region) -> None:
        """
        create_items(cell_id: int, category_id: int, trans: CplxTrans, region: Region) -> None
        @brief Creates new polygon items for the given cell/category combination
        For each polygon in the region a single item will be created. The value of the item will be this polygon.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method will also produce a flat version of the shapes inside the region. \RdbCategory#scan_collection is a similar method which also supports construction of hierarchical databases from deep regions.

        This method has been introduced in version 0.23.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param region The region (a \Region object) containing the polygons for which to create items
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: CplxTrans, edges: Edges) -> None:
        """
        create_items(cell_id: int, category_id: int, trans: CplxTrans, edges: Edges) -> None
        @brief Creates new edge items for the given cell/category combination
        For each edge a single item will be created. The value of the item will be this edge.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method will also produce a flat version of the edges inside the edge collection. \RdbCategory#scan_collection is a similar method which also supports construction of hierarchical databases from deep edge collections.

        This method has been introduced in version 0.23.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param edges The list of edges (an \Edges object) for which the items are created
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: CplxTrans, edge_pairs: EdgePairs) -> None:
        """
        create_items(cell_id: int, category_id: int, trans: CplxTrans, edge_pairs: EdgePairs) -> None
        @brief Creates new edge pair items for the given cell/category combination
        For each edge pair a single item will be created. The value of the item will be this edge pair.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method will also produce a flat version of the edge pairs inside the edge pair collection. \RdbCategory#scan_collection is a similar method which also supports construction of hierarchical databases from deep edge pair collections.

        This method has been introduced in version 0.23.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param edges The list of edge pairs (an \EdgePairs object) for which the items are created
        """
    @overload
    def create_items(self, cell_id: int, category_id: int, trans: CplxTrans, array: Iterable[Polygon]) -> None:
        """
        create_items(cell_id: int, category_id: int, trans: CplxTrans, array: Iterable[Polygon]) -> None
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
    def create_items(self, cell_id: int, category_id: int, trans: CplxTrans, array: Iterable[Edge]) -> None:
        """
        create_items(cell_id: int, category_id: int, trans: CplxTrans, array: Iterable[Edge]) -> None
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
    def create_items(self, cell_id: int, category_id: int, trans: CplxTrans, array: Iterable[EdgePair]) -> None:
        """
        create_items(cell_id: int, category_id: int, trans: CplxTrans, array: Iterable[EdgePair]) -> None
        @brief Creates new edge pair items for the given cell/category combination
        For each edge pair a single item will be created. The value of the item will be this edge pair.
        A transformation can be supplied which can be used for example to convert the object's dimensions to micron units by scaling by the database unit.

        This method has been introduced in version 0.23.

        @param cell_id The ID of the cell to which the item is associated
        @param category_id The ID of the category to which the item is associated
        @param trans The transformation to apply
        @param edge_pairs The list of edge_pairs for which the items are created
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
    def each_category(self) -> RdbCategory:
        """
        each_category() -> RdbCategory
        @brief Iterates over all top-level categories
        """
    def each_cell(self) -> RdbCell:
        """
        each_cell() -> RdbCell
        @brief Iterates over all cells
        """
    def each_item(self) -> RdbItem:
        """
        each_item() -> RdbItem
        @brief Iterates over all items inside the database
        """
    def each_item_per_category(self, category_id: int) -> RdbItem:
        """
        each_item_per_category(category_id: int) -> RdbItem
        @brief Iterates over all items inside the database which are associated with the given category
        @param category_id The ID of the category for which all associated items should be retrieved
        """
    def each_item_per_cell(self, cell_id: int) -> RdbItem:
        """
        each_item_per_cell(cell_id: int) -> RdbItem
        @brief Iterates over all items inside the database which are associated with the given cell
        @param cell_id The ID of the cell for which all associated items should be retrieved
        """
    def each_item_per_cell_and_category(self, cell_id: int, category_id: int) -> RdbItem:
        """
        each_item_per_cell_and_category(cell_id: int, category_id: int) -> RdbItem
        @brief Iterates over all items inside the database which are associated with the given cell and category
        @param cell_id The ID of the cell for which all associated items should be retrieved
        @param category_id The ID of the category for which all associated items should be retrieved
        """
    def filename(self) -> str:
        """
        filename() -> str
        @brief Gets the file name and path where the report database is stored
        This property is set when a database is saved or loaded. It cannot be set manually.
        @return The file name and path
        """
    def is_const_object(self) -> bool:
        """
        is_const_object() -> bool
        @brief Returns a value indicating whether the reference is a const reference
        This method returns true, if self is a const reference.
        In that case, only const methods may be called on self.
        """
    def is_modified(self) -> bool:
        """
        is_modified() -> bool
        @brief Returns a value indicating whether the database has been modified
        """
    def load(self, filename: str) -> None:
        """
        load(filename: str) -> None
        @brief Loads the database from the given file
        @param filename The file from which to load the database
        The reader recognizes the format automatically and will choose the appropriate decoder. 'gzip' compressed files are uncompressed automatically.
        """
    def name(self) -> str:
        """
        name() -> str
        @brief Gets the database name
        The name of the database is supposed to identify the database within a layout view context. The name is modified to be unique when a database is entered into a layout view. @return The database name
        """
    @classmethod
    def new(cls, name: str) -> ReportDatabase:
        """
        new(name: str) -> ReportDatabase
        @brief Creates a report database
        @param name The name of the database
        The name of the database will be used in the user interface to refer to a certain database.
        """
    @overload
    def num_items(self) -> int:
        """
        num_items() -> int
        @brief Returns the number of items inside the database
        @return The total number of items
        """
    @overload
    def num_items(self, cell_id: int, category_id: int) -> int:
        """
        num_items(cell_id: int, category_id: int) -> int
        @brief Returns the number of items inside the database for a given cell/category combination
        @param cell_id The ID of the cell for which to retrieve the number
        @param category_id The ID of the category for which to retrieve the number
        @return The total number of items for the given cell and the given category
        """
    @overload
    def num_items_visited(self) -> int:
        """
        num_items_visited() -> int
        @brief Returns the number of items already visited inside the database
        @return The total number of items already visited
        """
    @overload
    def num_items_visited(self, cell_id: int, category_id: int) -> int:
        """
        num_items_visited(cell_id: int, category_id: int) -> int
        @brief Returns the number of items visited already for a given cell/category combination
        @param cell_id The ID of the cell for which to retrieve the number
        @param category_id The ID of the category for which to retrieve the number
        @return The total number of items visited for the given cell and the given category
        """
    def reset_modified(self) -> None:
        """
        reset_modified() -> None
        @brief Reset the modified flag
        """
    def save(self, filename: str) -> None:
        """
        save(filename: str) -> None
        @brief Saves the database to the given file
        @param filename The file to which to save the database
        The database is always saved in KLayout's XML-based format.
        """
    def set_item_visited(self, item: RdbItem, visited: bool) -> None:
        """
        set_item_visited(item: RdbItem, visited: bool) -> None
        @brief Modifies the visited state of an item
        @param item The item to modify
        @param visited True to set the item to visited state, false otherwise
        """
    def set_tag_description(self, tag_id: int, description: str) -> None:
        """
        set_tag_description(tag_id: int, description: str) -> None
        @brief Sets the tag description for the given tag ID
        @param tag_id The ID of the tag
        @param description The description string
        See \tag_id for a details about tags.
        """
    def tag_description(self, tag_id: int) -> str:
        """
        tag_description(tag_id: int) -> str
        @brief Gets the tag description for the given tag ID
        @param tag_id The ID of the tag
        @return The description string
        See \tag_id for a details about tags.
        """
    def tag_id(self, name: str) -> int:
        """
        tag_id(name: str) -> int
        @brief Gets the tag ID for a given tag name
        @param name The tag name
        @return The corresponding tag ID
        Tags are used to tag items in the database and to specify tagged (named) values. This method will always succeed and the tag will be created if it does not exist yet. Tags are basically names. There are user tags (for free assignment) and system tags which are used within the system. Both are separated to avoid name clashes.

        \tag_id handles system tags while \user_tag_id handles user tags.
        """
    def tag_name(self, tag_id: int) -> str:
        """
        tag_name(tag_id: int) -> str
        @brief Gets the tag name for the given tag ID
        @param tag_id The ID of the tag
        @return The name of the tag
        See \tag_id for a details about tags.

        This method has been introduced in version 0.24.10.
        """
    def user_tag_id(self, name: str) -> int:
        """
        user_tag_id(name: str) -> int
        @brief Gets the tag ID for a given user tag name
        @param name The user tag name
        @return The corresponding tag ID
        This method will always succeed and the tag will be created if it does not exist yet. See \tag_id for a details about tags.

        This method has been added in version 0.24.
        """
    def variants(self, name: str) -> Iterable[int]:
        """
        variants(name: str) -> Iterable[int]
        @brief Gets the variants for a given cell name
        @param name The basic name of the cell
        @return An array of ID's representing cells that are variants for the given base name
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
