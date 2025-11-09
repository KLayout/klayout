
@0xc5f5e75ece54ff24;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::header");

using MetaData = import "metaData.capnp".MetaData;

# A library dictionary entry
# These entries are used later to form the library list
struct LibrarySpec
{
  # The name of the library
  name @0 :Text;

  # The type of library: 
  # The type carries information about the usage of the library.
  # Types are: 
  #   "layout": traverses "layout" views
  #   "schematic": traverses "schematic" views through "symbol"
  #   more t.b.d
  type @1 :Text;
}

# The global header
struct Header
{
  # File-global meta data
  metaData @0 :MetaData;

  # The name of the tool that generated this file
  generator @1 :Text;

  # The technology this stream is intended for
  # This is an arbitrary string as of now.
  technology @2 :Text;

  # The list of libraries contained in this stream
  libraries @3 :List(LibrarySpec);
}