@0x973a841036ae34e0;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::layoutView");

using Box = import "geometry.capnp".Box;
using Polygon = import "geometry.capnp".Polygon;
using SimplePolygon = import "geometry.capnp".SimplePolygon;
using Edge = import "geometry.capnp".Edge;
using EdgePair = import "geometry.capnp".EdgePair;
using Path = import "geometry.capnp".Path;
using Point = import "geometry.capnp".Point;
using Vector = import "geometry.capnp".Vector;
using Label = import "geometry.capnp".Label;
using FixPointTransformation = import "geometry.capnp".FixPointTransformation;
using Repetition = import "repetition.capnp".Repetition;

struct SingleObject(Object)
{
  basic @0 :Object;
}

struct ObjectArray(Object)
{
  basic @0 :Object;

  # "repetitionId" is 0 for single instance, otherwise it's a base-1 index in the
  # "Layer::repetitions" list
  repetitionId @1 :UInt64;
}

struct ObjectWithProperties(Object)
{
  basic @0 :Object;

  # "propertySetId" is 0 for "no properties", otherwise it's a base-1 index in the 
  # "Header::propertiesTable::propertySets" list.
  propertySetId @1 :UInt64;
}

struct ObjectContainerForType(Object)
{
  basic @0 :List(SingleObject(Object));  # NOTE: can't be "List(Object)" simply
  withProperties @1 :List(ObjectWithProperties(Object));
  arrays @2 :List(ObjectArray(Object));
  arraysWithProperties @3 :List(ObjectWithProperties(ObjectArray(Object)));
}

# A layer
# A layer is a collection of objects - with repetitions and/or properties.
# "objects" can be geometrical objects such as boxes and others.
struct Layer
{
  # The layer index - this is a base-0 index into Library::layerTable::layerEntries
  layerId @0 :UInt64;

  # The repetitions
  # The "repetitionId" specified above is a base-1 index in that this.
  repetitions @1 :List(Repetition);

  # Containers for the different objects the layer can be composed of.
  # Every container can hold objects with properties and/or repetitions.
  boxes @2 :ObjectContainerForType(Box);
  polygons @3 :ObjectContainerForType(Polygon);
  simplePolygons @4 :ObjectContainerForType(SimplePolygon);
  paths @5 :ObjectContainerForType(Path);
  labels @6 :ObjectContainerForType(Label);
  edges @7 :ObjectContainerForType(Edge);
  edgePairs @8 :ObjectContainerForType(EdgePair);
  points @9 :ObjectContainerForType(Point);
}

# Specifies a transformation for a cell
# Transformations can be "simple": such transformations
# do not allow arbitrary angle rotations or scaling.
# "complex" transformations on the other hand, support
# arbitrary angle rotations and scaling but come with 
# potential issues with off-grid vertexes rendered
# by their resulting geometries.
# 
# The order of the transformations in the simple cases is:
#  1. Apply simple.orientation
#  2. Apply displacement (shift)
# 
# In the complex case the order is:
#  1. Multiplication by the "scale" factor
#  2. Mirror at x axis if "mirror" is true
#  3. Rotate by "angle" degrees counter-clockwise
#  4. Apply displacement (shift)

struct CellTransformation
{
  displacement @0 :Vector;

  transformation :union {
    simple :group {
      orientation @1 : FixPointTransformation;
    }
    complex :group {
      angle @2 :Float64;  # rotation angle in degree
      mirror @3 :Bool;    # at x axis before rotation
      scale @4 :Float64;  # before rotation, mirror and displacement
    }
  }
}

# A cell placement
struct CellInstance
{
  # This is a base-0 index into Library::cellSpecsTable::cellSpecs
  cellId @0 :UInt64;

  # The transformation describes how the cell is placed
  transformation @1 :CellTransformation;
}

# The LayoutView class:
# This message represents the entire layout view of a cell
struct LayoutView
{
  # The cell bounding box
  # This bounding box includes all geometries, including child cells
  # but maybe larger than the actual size due to shapes not written 
  # to the stream.
  boundingBox @0 :Box;

  # The layers present in the layout view
  layers @1 :List(Layer);

  # The repetitions used for the cell instances
  instanceRepetitions @2 :List(Repetition);

  # The cell instance container: lists cell instances with properties
  # and/or repetitions.
  # Regular array repetitions play a special role as they are preserved
  # in edit mode.
  instances @3 :ObjectContainerForType(CellInstance);
}

