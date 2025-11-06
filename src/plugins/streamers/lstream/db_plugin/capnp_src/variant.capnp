@0x92e3d852e5f6a442;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::variant");

# A helper struct to build a dictionary of key/value pairs
struct ArrayEntry
{
  key @0 :Variant;
  value @1 :Variant;
}

# A generic type representing a number of C++ types
struct Variant {
  value :union {
    nil @0 :Void;
    bool @1 :Bool;
    uint64 @2 :UInt64;
    int64 @3 :Int64;
    double @4 :Float64;
    text @5 :Text;
    list @6 :List(Variant);
    array @7 :List(ArrayEntry);
    object @8 :Text;
  }
}

