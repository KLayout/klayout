# $autorun-early

module DRC

  # A wrapper for a named value which is stored in
  # a variable for delayed execution 
  class DRCVar
    def initialize(name)
      @name = name
    end
    def inspect
      @name
    end
    def to_s
      @name
    end
  end
  
  # A wrapper for the sizing mode value
  class DRCSizingMode
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
 
  # A wrapper for the join flag for extended
  class DRCJoinFlag
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for the angle limit
  # The purpose of this wrapper is to identify the
  # angle limit specification
  class DRCAngleLimit
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for a metrics constant
  # The purpose of this wrapper is to identify the 
  # metrics constant by the class.
  class DRCMetrics
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for the "whole edges" flag for
  # the DRC functions. The purpose of this class
  # is to identify the value by the class.
  class DRCWholeEdges
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for the "as_dots" or "as_boxes" flag for
  # some DRC functions. The purpose of this class
  # is to identify the value by the class.
  class DRCAsDots
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for a rectangle error filter mode 
  # The purpose of this wrapper is to identify the error filter mode
  class DRCRectangleErrorFilter
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for a opposite error filter mode 
  # The purpose of this wrapper is to identify the error filter mode
  class DRCOppositeErrorFilter
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for a glob-pattern style text selection for
  # some DRC functions. The purpose of this class
  # is to identify the value by the class.
  class DRCPattern
    attr_accessor :as_pattern
    attr_accessor :pattern
    def initialize(f, p)
      self.as_pattern = f
      self.pattern = p
    end
  end
  
  # A wrapper for a pair of limit values
  # This class is used to identify projection limits for DRC
  # functions
  class DRCProjectionLimits

    include DRCComparable

    def initialize(*a)

      _init_comparable

      if a.size > 2

        raise("A projection limits specification requires a maximum of two values")

      elsif a.size == 1

        if !a[0].is_a?(Range) || (!a[0].min.is_a?(Float) && !a[0].min.is_a?(1.class))
          raise("A projection limit requires an interval of two length values or two individual length values")
        end

        self.set_ge(a[0].min)
        self.set_lt(a[0].max)

      elsif a.size == 2

        if a[0] && !a[0].is_a?(Float) && !a[0].is_a?(1.class)
          raise("First argument to a projection limit must be either nil or a length value")
        end
        if a[1] && !a[1].is_a?(Float) && !a[1].is_a?(1.class)
          raise("Second argument to a projection limit must be either nil or a length value")
        end

        self.set_ge(a[0])
        self.set_lt(a[1])

      end

    end

    def get_limits(engine)

      if ! self.le && ! self.ge && ! self.lt && ! self.gt 
        raise("No constraint given for projection limits (supply values or place inside a condition)")
      end

      min = self.ge ? engine._make_value(self.ge) : (self.gt ? engine._make_value(self.gt) + 1 : 0)
      max = self.le ? engine._make_value(self.le) + 1 : (self.lt ? engine._make_value(self.lt) : nil)

      [ min, max ]

    end

  end

  # A wrapper for an input for the antenna check
  # This class is used to identify a region plus an
  # optional perimeter factor
  class DRCAreaAndPerimeter
    attr_accessor :region
    attr_accessor :area_factor
    attr_accessor :perimeter_factor
    def initialize(r, f, t)
      self.region = r
      self.area_factor = f
      self.perimeter_factor = t
    end
  end

  # A wrapper for specifying shielded mode
  class DRCShielded
    def initialize(f)
      @value = f
    end
    def value
      @value
    end
  end
 
end

