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
  
  # A wrapper for the "both edges" flag for EdgePairs#with_length or EdgePairs#with_angle
  class DRCBothEdges
  end
  
  # A wrapper for the "ortho edges" flag for Edges#with_angle or EdgePairs#with_angle
  class DRCOrthoEdges
    def value
      RBA::Edges::OrthoEdges
    end
  end
  
  # A wrapper for the "diagonal only edges" flag for Edges#with_angle or EdgePairs#with_angle
  class DRCDiagonalOnlyEdges
    def value
      RBA::Edges::DiagonalEdges
    end
  end
  
  # A wrapper for the "diagonal edges" flag for Edges#with_angle or EdgePairs#with_angle
  class DRCDiagonalEdges
    def value
      RBA::Edges::OrthoDiagonalEdges
    end
  end
  
  # A wrapper for the padding modes of with_density
  class DRCDensityPadding
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for the sizing mode value
  class DRCSizingMode
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
 
  # A wrapper for the edge mode value for Region#edges
  class DRCEdgeMode
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
  
  # A wrapper for the "zero distance mode" for
  # the DRC functions. The purpose of this class
  # is to identify the value by the class.
  class DRCZeroDistanceMode
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for the "as_dots" or "as_boxes" flag for
  # some DRC functions. The purpose of this class
  # is to identify the value by the class.
  class DRCOutputMode
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
  end
  
  # A wrapper for the "absolute" flag for
  # some DRC functions. The purpose of this class
  # is to identify the value by the class.
  class DRCAbsoluteMode
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

  # A wrapper for a property name
  # Use "prop(key)" to generate this tag.
  class DRCPropertyName
    attr_accessor :value
    def initialize(k)
      self.value = k
    end
  end
  
  # A wrapper for a property constraint
  # Use "props_eq", "props_ne" or "props_copy" to generate this tag.
  class DRCPropertiesConstraint
    attr_accessor :value
    def initialize(v)
      self.value = v
    end
    def is_eq?
      self.value == RBA::Region::SamePropertiesConstraint || self.value == RBA::Region::SamePropertiesConstraintDrop
    end
    def is_ne?
      self.value == RBA::Region::DifferentPropertiesConstraint || self.value == RBA::Region::DifferentPropertiesConstraintDrop
    end
    def is_copy?
      self.value == RBA::Region::NoPropertyConstraint || self.value == RBA::Region::SamePropertiesConstraint || self.value == RBA::Region::DifferentPropertiesConstraint
    end
    def +(other)
      other.is_a?(DRCPropertiesConstraint) || raise("'+' needs to be applied to two properties constraints (got #{other.inspect} for the second one)")
      is_eq = self.is_eq? || other.is_eq? 
      is_ne = self.is_ne? || other.is_ne? 
      is_copy = self.is_copy? || other.is_copy? 
      if is_eq == is_ne
        DRCPropertiesConstraint::new(is_copy ? RBA::Region::NoPropertyConstraint : RBA::Region::IgnoreProperties)
      elsif is_eq
        DRCPropertiesConstraint::new(is_copy ? RBA::Region::SamePropertiesConstraint : RBA::Region::SamePropertiesConstraintDrop)
      elsif is_ne
        DRCPropertiesConstraint::new(is_copy ? RBA::Region::DifferentPropertiesConstraint : RBA::Region::DifferentPropertiesConstraintDrop)
      else
        nil
      end
    end
  end
  
  # Negative output on checks
  class DRCNegative
    def initialize
    end
  end

  # Property selector for "input"
  class DRCPropertySelector
    attr_accessor :method
    attr_accessor :args
    def apply_to(iter)
      iter.send(self.method, *self.args)
    end
    def initialize(method, *args)
      self.method = method
      self.args = args
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

  # A wrapper for the fill cell definition
  class DRCFillCell

    def initialize(name)
      @cell_name = name
      @shapes = []
      @origin = nil
      @dim = nil
    end

    def create_cell(layout, engine)
      cell = layout.create_cell(@cell_name)
      @shapes.each do |s|
        li = layout.layer(s[0])
        engine._use_output_layer(li)
        s[1].each { |t| cell.shapes(li).insert(t) }
      end
      cell
    end

    def cell_box(def_w, def_h)
      o = @origin || self._computed_origin
      d = @dim || RBA::DVector::new(def_w, def_h)
      RBA::DBox::new(o, o + d)
    end

    def default_xpitch
      @dim ? @dim.x : self.bbox.width
    end

    def default_ypitch
      @dim ? @dim.y : self.bbox.height
    end

    def _computed_origin
      b = self.bbox
      return b.empty? ? RBA::DPoint::new : b.p1
    end

    def bbox
      box = RBA::DBox::new
      @shapes.each do |s|
        s[1].each { |t| box += t.bbox }
      end
      box
    end

    def shape(*args)

      layer = nil
      datatype = nil
      name = nil
      shapes = []

      args.each_with_index do |a,ai|
        if a.is_a?(1.class)
          if !layer
            layer = a
          elsif !datatype
            datatype = a
          else
            raise("Argument ##{ai+1} not understood for FillCell#shape")
          end
        elsif a.is_a?(RBA::LayerInfo)
          layer = a.layer
          datatype = a.datatype
          name = a.name
        elsif a.is_a?(String)
          if !name
            name = a
          else
            raise("Argument ##{ai+1} not understood for FillCell#shape")
          end
        elsif a.is_a?(RBA::DBox) || a.is_a?(RBA::DPath) || a.is_a?(RBA::DPolygon) || a.is_a?(RBA::DText)
          shapes << a
        else
          raise("Argument ##{ai+1} not understood for FillCell#shape (needs to one of: number, string or box, path, polygon or text)")
        end
      end

      if !shapes.empty?

        li = RBA::LayerInfo::new
        if layer 
          li.layer = layer
          li.datatype = datatype || 0
        end
        if name
          li.name = name
        end

        @shapes << [ li, shapes ]

      end

      self
     
    end

    def origin(x, y)

      if !x.is_a?(1.class) && !x.is_a?(1.0.class)
        raise("x argument not numeric FillCell#origin")
      end
      if !y.is_a?(1.class) && !y.is_a?(1.0.class)
        raise("y argument not numeric FillCell#origin")
      end
      @origin = RBA::DVector::new(x, y)

      self

    end

    def dim(w, h)

      if !w.is_a?(1.class) && !w.is_a?(1.0.class)
        raise("w argument not numeric FillCell#dim")
      end
      if !h.is_a?(1.class) && !h.is_a?(1.0.class)
        raise("h argument not numeric FillCell#dim")
      end
      @dim = RBA::DVector::new(w, h)

      self

    end

  end
 
  # A wrapper for the fill step definition
  class DRCFillStep
    def initialize(for_row, x, y = nil)
      @for_row = for_row
      if !x.is_a?(1.class) && !x.is_a?(1.0.class)
        raise("x argument not numeric in fill step")
      end
      if y && !y.is_a?(1.class) && !y.is_a?(1.0.class)
        raise("y argument not numeric in fill step")
      end
      if y
        @step = RBA::DVector::new(x, y)
      elsif for_row
        @step = RBA::DVector::new(x, 0)
      else
        @step = RBA::DVector::new(0, x)
      end 
    end
    def for_row
      @for_row
    end
    def step
      @step
    end
  end

  # A wrapper for the fill origin definition
  class DRCFillOrigin
    def initialize(x = nil, y = nil, repeat = false)
      @repeat = repeat
      if !x && !y
        @origin = nil
      else
        if !x.is_a?(1.class) && !x.is_a?(1.0.class)
          raise("x argument not numeric in fill origin")
        end
        if !y.is_a?(1.class) && !y.is_a?(1.0.class)
          raise("y argument not numeric in fill origin")
        end
        @origin = RBA::DVector::new(x, y)
      end
    end
    def origin
      @origin
    end
    def repeat
      @repeat
    end
  end

  # A wrapper for the tile_size option
  class DRCTileSize
    def initialize(*args)
      @xy = args
    end
    def get
      @xy
    end
  end
 
  # A wrapper for the tile_step option
  class DRCTileStep
    def initialize(*args)
      @xy = args
    end
    def get
      @xy
    end
  end
 
  # A wrapper for the tile_origin option
  class DRCTileOrigin
    def initialize(*args)
      @xy = args
    end
    def get
      @xy
    end
  end
 
  # A wrapper for the tile_count option
  class DRCTileCount
    def initialize(*args)
      @xy = args
    end
    def get
      @xy
    end
  end
 
  # A wrapper for the tile_boundary option
  class DRCTileBoundary
    def initialize(layer)
      @b = layer
    end
    def get
      @b
    end
  end

end

