@0x91d51203f4528da2;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::metaData");

using Variant = import "variant.capnp".Variant;

# An entry for meta data
struct MetaDataEntry
{
  # The key
  name @0 :Text;

  # Some optional description text
  description @1 :Text;

  # The value
  value @2 :Variant;
}

# The MetaData View class:
# This message represents the meta data view of cell.
# Meta data is a generic concept - basically a list of
# key/value pairs for custom payload, with not specific 
# implied interpretation by the application.
struct MetaData
{
  entries @0 :List(MetaDataEntry);
}

