# encoding: UTF-8

$:.push(File::dirname($0))

load("test_prologue.rb")

# NOTE: for some mysterious reason the tests work better on Ruby 2.3.x if the actual test 
# implementation is in a file of it's own (maybe: VM forces a context release on "load").
# Without this, the GC-dependent tests tend to not release objects that are supposed 
# to be cleaned up.
load("basic_testcore.rb")

load("test_epilogue.rb")

