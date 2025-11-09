@0x86931e1824fec888;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::library");

using MetaData = import "metaData.capnp".MetaData;
using PropertySet = import "propertySet.capnp".PropertySet;
using PropertyName = import "propertySet.capnp".PropertyName;
using NamedValue = import "propertySet.capnp".NamedValue;
using Box = import "geometry.capnp".Box;

using Variant = import "variant.capnp".Variant;

# A table listing the names of the properties used in this library
struct PropertyNamesTable
{
  # Namespaces are intended to separate property names which are otherwise
  # identical. Different applications or system can define their own namespace
  # which acts as a prefix to the property names.
  # The "namespaceId" used in PropertyName is a base-1 index in this table
  namespaces @0 :List(Text);

  # Names with namespace Ids:
  # The "nameId" used in PropertySet is a base-0 index in this table
  names @1 :List(PropertyName);
}

# A table of all property/name combinations used in this library:
# Shapes or other objects specifying properties do not carry a dictionary,
# but just state the "Id" of the property set, which is a pointer into
# this table. This serves the compactness goal of the format as properties
# are often repeated.
struct PropertiesTable
{
  # NOTE: the propertyId used in many places is 0 for "no property"
  # otherwise "propertyId" is the base-1 index of the property set in this list
  propertySets @0 :List(PropertySet);
}

# A table of all text (label) strings used:
# A label object does not directly specify the text, but stores a
# base-0 index into this table. This also serves the compactness goal.
struct TextStringsTable
{
  textStrings @0 :List(Text);
}

# A reference to a library:
# This is merely a pointer to an external entity. It is up to the
# application to resolve this reference.
struct LibraryRef
{
  libraryName @0 :Text;
}

# A list of all library references
struct LibraryRefs
{
  refs @0 :List(LibraryRef);
}

# A structure describing the parameters of a cell
# Cell parameters are name/value pairs. It is up to the application
# to provide a cell implementation and identify the parameters to use.
struct CellParameters
{
  values @0 :List(NamedValue);
}

# A cell header:
# This structure describes a cell inside a library. A cell is primarily
# given by a name which must be unique inside the library. A cell can refer
# to a separate library and/or a parameterized cell.
# an any case, the cell's actual content is copied into the stream, so a
# consumer of a stream can obtain the actual geometry of a cell, instead
# of having to find the provider to build it.
struct CellSpec
{
  # The name of the cell in the current stream. This needs to be a unique name.
  # This attribute is mandatory. If the cell refers to a library cell, the cell
  # name inside the library can be different from the cell name.
  name @0 :Text;

  # Specifies, whether the cell refers to a library.
  # "libraryNameId" is 0 for a local cell (no library reference), otherwise it's a base-1 
  # index into "Header::libraryRefs".
  libraryRefId @1 :UInt64;
  
  # The name of the cell inside the library, if one is specified
  libraryCellName @2 :Text;

  # The cell parameters, if the cell is a parameterized cell.
  parameters @3 :CellParameters;

  # "propertySetId" is 0 for "no properties", otherwise it's a base-1 index in the 
  # "Header::propertiesTable::propertySets" list.
  propertySetId @4 :UInt64;
}

# A table of all cells
struct CellSpecsTable
{
  cellSpecs @0 :List(CellSpec);
}

# A node in the hierarchy tree:
# The hierarchy tree lists all cells with their children.
struct CellHierarchyTreeNode
{
  # The cellId is a base-0 index in CellSpecsTable::cellSpecs
  cellId @0 :UInt64;

  # Same for the child cell Ids
  childCellIds @1 :List(UInt64);
}

# The full hierarchy tree
struct CellHierarchyTree
{
  # The number of top cells (cells without a parent)
  # As the "nodes" list is sorted top-down, the first
  # "numberOfTopCells" cells are actually top cells.
  numberOfTopCells @0 :UInt64;

  # top-down sorted cells
  nodes @1 :List(CellHierarchyTreeNode);
}

# A layer specification:
# A layer can be specified by a several layer numbers 
# and/or a name. If no numbers are given, the name will
# identify the layer. Otherwise the numbers will identify
# the layer.
# Layers can be unnamed and unnumbered and layer specifications
# do not need to be unique.
# The purpose is a hint and additionally identifies the
# purpose of the shapes on the layer.
struct LayerEntry 
{
  # layerNumbers = [] for no layer/datatype, 
  #                [layer] for single layer/no datatype
  #                [layer, datatype] for GDS2 layer/datatype
  layerNumbers @0 :List(UInt64);
  name @1 :Text;
  purpose @2 :Purpose;

  enum Purpose {
    drawing @0;  # default
    fill @1;
    slot @2;
    pin @3;
    boundary @4;
    text @5;
    comment @6;
    blockage @7;
    wire @8;
    errors @9;
    handles @10;
    symbol @11;
  }
}

# All layer entries
struct LayerTable
{
  # The layerId used inside the LayoutView struct is a 
  # base-0 index into this table.
  layerEntries @0 :List(LayerEntry);
}

# A specification for a view
struct ViewSpec
{
  # The view name: the view name carries information about
  # the usage - e.g. "layout", "metaData", "abstractLayout", "schematic", "symbol" ...
  name @0 :Text;

  # The class representing the view
  # For layout views, this is "LayoutView".
  class @1 :Text;

  # The resolution: this is the number of database
  # units per micrometer. This specification is favored
  # over database unit in micrometers as this is usually
  # an integer number.
  # As of now, only layout views need this resolution value as they represent
  # geometries in integer multiples of the resolution.
  # If not used, this value should be zero.
  resolution @2 :Float64;

  # The layer table:
  # All layers used in this stream are listed here.
  # Layers can be present without being used (in a sense of having shapes in them).
  # On the contrary, layers must be listed here, before they can be used for shapes.
  layerTable @3 :LayerTable;

  # View properties:
  # "propertySetId" is 0 for "no properties", otherwise it's a base-1 index in the 
  # "Header::propertiesTable::propertySets" list.
  # For layout views, this corresponds to OASIS file properties for example.
  propertySetId @4 :UInt64;

  # Meta data attached to the view type
  metaData @5 :MetaData;
}

# A table of all views
struct ViewSpecsTable
{
  # The "viewIds" used in the "Cell" structure is a base-0 index into this
  # list.
  viewSpecs @0 :List(ViewSpec);
}

# The header for a library
struct Library
{
  # Meta data:
  # Meta data is a generic container for arbitrary data attached to the stream.
  # In contrast to properties, meta data is available on cell and library level 
  # only and is intended for application specific data.
  metaData @0 :MetaData;

  # Library names:
  # The "libraryNameId" used in this scheme refers to this table
  # as a base-1 index into it. A "libraryNameId" of 0 means "no library referenced".
  libraryRefs @1 :LibraryRefs;

  # Property names:
  # The "nameId" used in a number of places in the scheme refers
  # to this table as a base-0 index into it.
  propertyNamesTable @2 :PropertyNamesTable;

  # Properties tables:
  # The "propertySetId" used in many places in this schema refers to 
  # this table as a base-1 index into it. A property set Id of 0
  # indicates "no properties".
  propertiesTable @3 :PropertiesTable;

  # Text strings:
  # This text string repository is mainly used by layout views for
  # compact representation of text strings.
  textStringsTable @4 :TextStringsTable;

  # View specs table:
  # Defines the views with some additional attributes. Not all views
  # listed here need to be used by the cells. Every cell selects
  # views from this list. However, a cell cannot select a view that is
  # not in this list.
  viewSpecsTable @5 :ViewSpecsTable;
  
  # The cell specifications:
  # The cell specifications describe the cell's origin and 
  # attributes of the cell.
  cellSpecsTable @6 :CellSpecsTable;

  # The cell hierarchy tree:
  # This tree is mandatory for pure layout streams. It provides
  # quick access to the structure, so stream processing can be planned
  # in some cases by just reading the header.
  cellHierarchyTree @7 :CellHierarchyTree;
}

# Followed by many "Cell" messages

