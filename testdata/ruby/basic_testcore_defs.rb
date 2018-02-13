
# extend A
class RBA::A 

  alias_method :org_initialize, :initialize

  def initialize(*args)
    org_initialize(*args)
    @offset = nil
  end
  def s( o )
    @offset = o
  end
  def g
    return @offset
  end
  def m
    return @offset+a1
  end
  def call_a10_prot(f)
    a10_prot(f)
  end
  def inspect
    if @offset 
      @offset.to_s
    else
      "a1=" + self.a1.to_s
    end 
  end

private
  @offset

end

class MyException < RuntimeError
  def initialize(s)
    super(s)
  end
end
    
class XEdge < RBA::Edge
  def initialize
    super(RBA::Point.new(1,2), RBA::Point.new(3,4))
  end
end

class RBA::E
  def m 
    @m
  end
  def m=(x)
    @m = x
  end
  @m = nil
end

class RBAGObject < RBA::GObject
  def initialize(z)
    super()
    @z = z
  end
  # reimplementation of "virtual int g()"
  def g
    return @z*2
  end
end

class RBAGFactory < RBA::GFactory
  def initialize
    super()
  end
  # reimplementation of "virtual GObject *f(int)"
  def f(z)
    return RBAGObject::new(z)
  end
end

