@0xbfd8ee64184d5def;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::cell");

# The Cell header

struct Cell
{
  # Enumerates the views the cell provides. These are base-0 indexes
  # into the Library::viewSpecsTable::viewSpecs list. The corresponding
  # view messages follow this Cell message.
  # 
  # In KLayout, a cell can have the following views:
  #   "layout": the shapes and instances (LayoutView message)
  #   "metaData": the cell's meta data (MetaDataView message)
  # 
  # If "layout" is missing, the cell is a "ghost cell" - i.e. one
  # which does not have a content yet. Such cells are treated specially,
  # e.g. they do not appear as structures in GDS files.
  # 
  # "metaData" is an optional view that holds arbitrary meta data
  # (key/value pairs) per cell.

  viewIds @0 :List(UInt16);
}

# Followed by "viewIds.size" messages with the view data

