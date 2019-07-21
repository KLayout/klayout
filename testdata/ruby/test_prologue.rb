
# in MSVC environment:
if ENV["RUBY"]
  ruby_libs = "#{ENV["RUBY"]}/lib/ruby/#{RUBY_VERSION}"
  if !$:.member?(ruby_libs)
    $:.push(ruby_libs)
  end
end

# Set this to true to disable some tests involving exceptions
$leak_check = ENV["TEST_LEAK_CHECK"]

# Fetch location of source files and the temp files
$ut_testsrc = ENV["TESTSRC"] || raise("Environment variable $TESTSRC not set")
$ut_testtmp = ENV["TESTTMP_WITH_NAME"] || ENV["TESTTMP"] || raise("Environment variable $TESTTMP not set")

# Pull packages from vendor drop-in
vendor = File.join($ut_testsrc, "testdata", "vendor", "ruby")
if !$:.member?(vendor)
  $:.unshift(vendor)
end

# Require Test::Unit
require 'test/unit/ui/console/testrunner'

# TestBase is an alias for "TestCase"
Object.const_defined?(:TestBase) && Object.send(:remove_const, :TestBase)
TestBase = Test::Unit::TestCase

# undefine existing classes

Object.constants.each do |c|
  if c.to_s =~ /_TestClass$/
    Object.send(:remove_const, c)
  end
end

# Some cleanup ahead
GC.start

