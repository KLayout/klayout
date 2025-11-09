
# LStream plugin sources

This plugin employs Cap'n'Proto (https://capnproto.org/) to implement 
the serialization layer. 

The schema files are provided in an already compiled form.
The provided sources use Cap'n'Proto compiler version 1.0.1.

Use the `capnp_compile.sh` script to compile the schema files
into C++. It requires "capnp" version 1.0.1.

The original LStream sources are kept somewhere else.
Use `fetch.sh` to sync these external sources with the local
ones.

