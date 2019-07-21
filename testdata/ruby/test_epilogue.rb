
# In the test environment, we cannot make sure that we destroy the ruby interpreter before the RBA
# environment is shut down. Therefore we must release all RBA objects by explicitly calling the GC
# and start the test suite manually.

begin

  err = 0
  any = nil
  repeat = (ENV["TESTREPEAT"] || "1").to_i

  class MyTestRunner < Test::Unit::UI::Console::TestRunner
    def initialize(suite, *args)
      super(suite, *args)
    end
    def test_started(name)
      super
    end
  end

  Object.constants.each do |c|
    if c.to_s =~ /_TestClass$/
      repeat.times do
        r = MyTestRunner::new(Object.const_get(c)).start
        err += r.error_count + r.failure_count
      end
      any = true
    end
  end

  if !any
    raise("No test class defined (any ending with _TestClass)")
  end

  if err > 0
    raise("Tests failed (#{err} Errors + Failures)")
  end

ensure
  GC.start
end

