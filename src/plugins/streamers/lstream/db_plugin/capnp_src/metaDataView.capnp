@0xf401d688f5f6eb25;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stream::metaDataView");

using MetaData = import "metaData.capnp".MetaData;

# A meta data view for a cell:
# A cell can add arbirary key/value information through this view.
struct MetaDataView
{
  data @0 :MetaData;
}

