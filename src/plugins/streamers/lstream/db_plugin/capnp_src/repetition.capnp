@0x9c4548253282e237;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::repetition");

using Vector = import "geometry.capnp".Vector;

# A regular array with x and y spacing
# The displacements of an object are:
#   disp = (ix*dx, iy*dy)
# where ix=0..nx-1 and iy=0..ny-1
struct RegularOrthoRepetition
{
  dx @0 :Int64;
  dy @1 :Int64;
  nx @2 :UInt64;
  ny @3 :UInt64;
}

# A regular array with free step vectors which do not need to be orthogonal
# The displacements of an object are:
#   disp = ia*a+ib*b
# where ia=0..na-1 and ib=0..nb-1
struct RegularRepetition
{
  a @0 :Vector;
  b @1 :Vector;
  na @2 :UInt64;
  nb @3 :UInt64;
}

# A free repetition given by a set of displacements
struct EnumeratedRepetition
{
  # The number of instances represented by this repetition type is one 
  # more than the length of the list. The first entry is implicitly
  # assumed to be (0,0) and is dropped.
  # The list is given in incremental offsets.
  # So the offsets are:
  #   offsets = { (0,0), delta[0], delta[0]+delta[1] ... }
  deltas @0 :List(Vector);
}

# A generic repetition object
# This union describes a set of object "placements".
# Placements are basically shifts of some original
# object by a given vector.
struct Repetition
{
  types :union {
    single @0 :Vector;
    regular @1 :RegularRepetition;
    regularOrtho @2 :RegularOrthoRepetition;
    enumerated @3 :EnumeratedRepetition;
  }
}

