@0x814220fba761a890;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::geometry");

# Enumerates the possible orthogonal fixpoint transformations
enum FixPointTransformation {
  r0   @0;   # no rotation
  r90  @1;   # rotation by 90 degree counterclockwise
  r180 @2;   # rotation by 180 degree counterclockwise
  r270 @3;   # rotation by 270 degree counterclockwise
  m0   @4;   # mirror at x axis ("0 degree axis")
  m45  @5;   # mirror at positive x/y diagonal ("45 degree axis")
  m90  @6;   # mirror at y axis ("90 degree axis")
  m135 @7;   # mirror at negative x/y diagonal ("135 degree axis")
}

# Implements a Vector
# A vector is a displacement or differences between points.
# In contrast to that, a point is a fixed position in 2d space.
# Coordinates are always expressed as 64 bit coordinates.
struct Vector
{
  dx @0 :Int64;
  dy @1 :Int64;
}

# A point
# A point describes a position. In constrast to the vector,
# the position is not the distance between two points.
# Coordinates are always expressed as 64 bit coordinates.
struct Point
{
  x @0 :Int64;
  y @1 :Int64;
}

# A rectangle
# Rectangles are implemented by a lower-left point and a vector
# connecting the upper-right point with the lower left one.
struct Box
{
  # delta.x or .y is >= 0 for normal boxes
  # delta.x < 0 or delta.y indicates an empty box
  p1 @0 :Point;
  delta @1 :Vector;
}

# An edge
# An edge is a connection between two points.
# Edges are implemented by on point and a vector connecting a 
# second point with the first one.
struct Edge
{
  p1 @0 :Point;
  delta @1 :Vector;
}

# An edge pair
# An edge pair is the combination of two edges. Edge pairs are 
# useful objects to describe DRC errors.
struct EdgePair
{
  e1 @0 :Edge;
  e2 @1 :Edge;
}

# A contour
# A contour is a sequence of points. Here, the contour is described
# by a first point and vectors connecting every point to it's 
# successor.
struct Contour
{
  # p1 is the first point, deltas are the differences
  # to the next point. I.e.
  #   points = { p1, p1+delta[0], p1+delta[0]+delta[1] ... }
  p1 @0 :Point;
  deltas @1 :List(Vector);
}

# A simple polygon
# A simple polygon is made from one contour. The contour forms
# the hull of the polygon. First and last point are implicitly 
# connected.
# The contour shall be orientated clockwise and should not
# be self-intersecting. If can be self-touching to make polygons
# with holes.
# The hull must have three points at least.
# No repeated points must be present and degenerated sequences
# such as reflecting edges or collinear edge pairs should be avoided,
# as they are not guaranteed to be preserved.
struct SimplePolygon
{
  hull @0 :Contour;
}

# A polygon with holes
# Like the simple polygon, but adds holes to the polygon.
# The holes are described by contours as well as the hull.
# Hole contours must not be overlapping and shall be 
# oriented counterclockwise. 
# The same restrictions apply for the holes as for the hull
# contour.
struct Polygon
{
  hull @0 :Contour;
  holes @1 :List(Contour);
}

# A path 
# A path is a list of points in form of a contour which
# are connected by a wide line. The width of the line
# is specified as the half-width to enforce on-gridness
# of the boundary of the path. The point sequence is 
# called the path's "spine". 
# The line ends can be configured to be flat, square,
# circular or round.
# The spine needs to have at least one point.
# Paths with single points are allowed. With square
# ends, this represents a square, with round ends this
# represents a circle.
struct Path
{
  # The spine
  spine @0 :Contour;

  # The path's half width
  halfWidth @1 :UInt64;

  # Extension at the start and end -
  # only valid with extensionType "variable":
  beginExtension @2 :Int64;
  endExtension @3 :Int64;

  # The type of extension
  extensionType @4 :ExtensionType;

  enum ExtensionType
  {
    flush @0;      # flat
    square @1;     # square
    round @2;      # circular
    variable @3;   # variable (given by "beginExtension" and "endExtension")
  }
}

# A text
# "texts" are not called "Text" as not to collide with Cap'n'Proto's
# built-in "Text" type.
# A text is a label at a specific position.
struct Label
{
  # The position of the label
  position @0 :Point;
  
  # The orientation of the label
  orientation @1 :FixPointTransformation;

  # base-0 index in the Header::TextStringsTable list
  stringId @2 :UInt64;

  # Alignment
  horizontalAlign @3 :HAlignment;
  verticalAlign @4 :VAlignment;

  # The offset by which the text is displayed relative to the 
  # position (this feature is ignored as of now)
  displayOffset @5 :Vector;

  # Text height in DBU
  size @6 :UInt64;

  # Horizontal alignment flags
  enum HAlignment
  {
    left @0;
    center @1;
    right @2;
  }

  # Vertical alignment flags
  enum VAlignment
  {
    bottom @0;
    center @1;
    top @2;
  }
}

