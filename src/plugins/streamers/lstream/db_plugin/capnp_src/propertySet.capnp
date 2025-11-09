@0xa344c0f52014bff4;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::propertySet");

using Variant = import "variant.capnp".Variant;

# The name of a shape or instance property
struct PropertyName
{
  # A "namespaceID" of 0 means "no namespace". Otherwise it's a base-1 index in the
  # property namespace table in "header::propertyNamesTable::namespaces".
  # The namespace is intended as a kind of prefix, uniquely identifying the name
  # of the property, so that different owners can use the same name as long as the
  # own the namespace.
  namespaceId @0 :UInt64;

  # The "name":
  # The property name does not need to be a string, but can be any type,
  # specifically integer values. This way, properties can map GDS user properties.
  name @1 :Variant;
}

# A pair of name and value
struct NamedValue
{
  # The namdId is a base-0 index into Library::propertyNamesTable
  nameId @0 :UInt64;

  # The value associated with the name
  value @1 :Variant;
}

# A set of name/value pairs forming the set of properties for a shape or instance
struct PropertySet
{
  properties @0 :List(NamedValue);
}

