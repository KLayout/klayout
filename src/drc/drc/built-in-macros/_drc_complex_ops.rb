# $autorun-early

module DRC

class DRCOpNode

  attr_accessor :description
  attr_accessor :engine
  
  def initialize(engine, node = nil)
    @node = node
    self.engine = engine
    self.description = "Basic"
  end
  
  def create_node(cache)
    n = cache[self.object_id]
    if !n
      n = self.do_create_node(cache)
      cache[self.object_id] = n
    end
    n
  end
  
  def do_create_node(cache)
    @node
  end
  
  def dump(indent)
    return indent + self.description
  end
  
  def _build_geo_bool_node(other, op)
    if ! other.is_a?(DRCOpNode)
      raise("Second argument to #{op.to_s} must be a DRC expression")
    end
    DRCOpNodeBool::new(@engine, op, self, other)
  end
  
  %w(& - ^ | +).each do |f|
    eval <<"CODE"
      def #{f}(other)
        self.engine._context("#{f}") do
          self._build_geo_bool_node(other, :#{f})
        end
      end
CODE
  end
  
  def !()
    self.engine._context("!") do
      if self.respond_to?(:inverted)
        return self.inverted
      else
        empty = RBA::CompoundRegionOperationNode::new_empty(RBA::CompoundRegionOperationNode::ResultType::Region)
        DRCOpNodeCase::new(@engine, [ self, DRCOpNode::new(@engine, empty), @engine.primary ])
      end
    end
  end

  def _check_numeric(v, symbol)
    if ! v.is_a?(Float) && ! v.is_a?(1.class)
      if symbol
        raise("Argument '#{symbol}' (#{v.inspect}) isn't numeric in operation '#{self.description}'")
      else
        raise("Argument (#{v.inspect}) isn't numeric in operation '#{self.description}'")
      end
    end
  end
  
  def _make_value(v, symbol)
    self._check_numeric(v, symbol)
    @engine._prep_value(v)
  end
  
  def _make_area_value(v, symbol)
    self._check_numeric(v, symbol)
    @engine._prep_area_value(v)
  end
  
  def area
    DRCOpNodeAreaFilter::new(@engine, self)
  end
  
  def perimeter
    DRCOpNodePerimeterFilter::new(@engine, self)
  end
  
  def bbox_min
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::BoxMinDim, self)
  end
  
  def bbox_max
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::BoxMaxDim, self)
  end
  
  def bbox_width
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::BoxWidth, self)
  end
  
  def bbox_height
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::BoxHeight, self)
  end
  
  def length
    DRCOpNodeEdgeLengthFilter::new(@engine, self)
  end
  
  def angle
    DRCOpNodeEdgeOrientationFilter::new(@engine, self)
  end
  
  def rounded_corners(inner, outer, n)
    self.engine._context("rounded_corners") do
      self._check_numeric(n, :n)
      DRCOpNodeFilter::new(@engine, self, :new_rounded_corners, "rounded_corners", self.make_value(inner, :inner), self.make_value(outer, :outer), n)
    end
  end
  
  def smoothed(d)
    self.engine._context("smoothed") do
      DRCOpNodeFilter::new(@engine, self, :new_smoothed, "smoothed", self.make_value(d, :d))
    end
  end
  
  def corners(as_dots = DRCAsDots::new(false))
    self.engine._context("corners") do
      if as_dots.is_a?(DRCAsDots)
        as_dots = as_dots.value
      else
        raise("Invalid argument (#{as_dots.inspect}) for 'corners' method")
      end
      DRCOpNodeCornersFilter::new(@engine, self, as_dots)
    end
  end
  
  %w(middle extent_refs).each do |f| 
    eval <<"CODE"
    def #{f}(*args)

      self.engine._context("#{f}") do

        f = []
        as_edges = false

        @@std_refs ||= { 
          :center => [0.5] * 4,
          :c => [0.5] * 4,
          :bottom_center => [ 0.5, 0.0, 0.5, 0.0 ],
          :bc => [ 0.5, 0.0, 0.5, 0.0 ],
          :bottom_left => [ 0.0, 0.0, 0.0, 0.0 ],
          :bl => [ 0.0, 0.0, 0.0, 0.0 ],
          :bottom_right => [ 1.0, 0.0, 1.0, 0.0 ],
          :br => [ 1.0, 0.0, 1.0, 0.0 ],
          :top_center => [ 0.5, 1.0, 0.5, 1.0 ],
          :tc => [ 0.5, 1.0, 0.5, 1.0 ],
          :top_left => [ 0.0, 1.0, 0.0, 1.0 ],
          :tl => [ 0.0, 1.0, 0.0, 1.0 ],
          :top_right => [ 1.0, 1.0, 1.0, 1.0 ],
          :tr => [ 1.0, 1.0, 1.0, 1.0 ],
          :left_center => [ 0.0, 0.5, 0.0, 0.5 ],
          :lc => [ 0.0, 0.5, 0.0, 0.5 ],
          :right_center => [ 1.0, 0.5, 1.0, 0.5 ],
          :rc => [ 1.0, 0.5, 1.0, 0.5 ],
          :south => [ 0.5, 0.0, 0.5, 0.0 ],
          :s => [ 0.5, 0.0, 0.5, 0.0 ],
          :left => [ 0.0, 0.0, 0.0, 1.0 ],
          :l => [ 0.0, 0.0, 0.0, 1.0 ],
          :bottom => [ 1.0, 0.0, 0.0, 0.0 ],
          :b => [ 1.0, 0.0, 0.0, 0.0 ],
          :right => [ 1.0, 1.0, 1.0, 0.0 ],
          :r => [ 1.0, 1.0, 1.0, 0.0 ],
          :top => [ 0.0, 1.0, 1.0, 1.0 ],
          :t => [ 0.0, 1.0, 1.0, 1.0 ]
        }

        args.each_with_index do |a,ia|
          if a.is_a?(1.0.class) && :#{f} != :middle
            f << a 
          elsif a.is_a?(DRCAsDots)
            as_edges = a.value
          elsif @@std_refs[a] && :#{f} != :middle
            f = @@std_refs[a]
          else
            raise("Invalid argument #" + (ia + 1).to_s + " (" + a.inspect + ") for '#{f}' method on operation '" + self.description + "' - needs to be numeric or 'as_dots/as_edges'")
          end
        end

        if f.size == 2
          f = f + f
        else
          f = (f + [0.5] * 4)[0..3]
        end
            
        if as_edges
          return DRCOpNodeRelativeExtents::new(self, true, *f)
        else
          # add oversize for point- and edge-like regions
          zero_area = (f[0] - f[2]).abs < 1e-7 || (f[1] - f[3]).abs < 1e-7
          f += [ zero_area ? 1 : 0 ] * 2
          return DRCOpNodeRelativeExtents::new(self, false, *f)
        end

      end

    end
CODE
  end
  
  def odd_polygons
    return DRCOpNodeFilter::new(@engine, self, :new_strange_polygons_filter, "odd_polygon")
  end
  
  def rectangles
    return DRCOpNodeFilter::new(@engine, self, :new_rectangle_filter, "rectangle")
  end
  
  def rectilinear
    return DRCOpNodeFilter::new(@engine, self, :new_rectilinear_filter, "rectilinear")
  end
  
  def holes
    return DRCOpNodeFilter::new(@engine, self, :new_holes, "holes")
  end
  
  def hulls
    return DRCOpNodeFilter::new(@engine, self, :new_hulls, "hull")
  end
  
  def edges
    return DRCOpNodeFilter::new(@engine, self, :new_edges, "edges")
  end
  
  def sized(*args)

    self.engine._context("sized") do

      dist = 0
      
      mode = 2
      values = []
      args.each_with_index do |a,ia|
        if a.is_a?(1.class) || a.is_a?(Float)
          v = self._make_value(a, "argument ##{ia + 1}")
          v.abs > dist && dist = v.abs 
          values.push(v)
        elsif a.is_a?(DRCSizingMode)
          mode = a.value
        end
      end
      
      args = []
      if values.size < 1
        raise("sized: Method requires one or two sizing values")
      elsif values.size > 2
        raise("sized: Method must not have more than two values")
      else
        args << values[0]
        args << values[-1]
      end
      args << mode
      
      DRCOpNodeFilter::new(@engine, self, :new_sized, "sized", *args)

    end

  end
  
  def extents(e = 0)
    self.engine._context("extents") do
      DRCOpNodeFilter::new(@engine, self, :new_extents, "extents", self._make_value(e, :e))
    end
  end
  
  def first_edges
    DRCOpNodeFilter::new(@engine, self, :new_edge_pair_to_first_edges, "first_edges")
  end
  
  def second_edges
    DRCOpNodeFilter::new(@engine, self, :new_edge_pair_to_second_edges, "second_edges")
  end

  def end_segments(length, fraction = 0.0)
    self.engine._context("end_segments") do
      self._check_numeric(fraction, :fraction)
      DRCOpNodeFilter::new(@engine, self, :new_end_segments, "end_segments", self._make_value(length, :length), fraction)
    end
  end
  
  def start_segments(length, fraction = 0.0)
    self.engine._context("start_segments") do
      self._check_numeric(fraction, :fraction)
      DRCOpNodeFilter::new(@engine, self, :new_start_segments, "start_segments", self._make_value(length, :length), fraction)
    end
  end
  
  def centers(length, fraction = 0.0)
    self.engine._context("centers") do
      self._check_numeric(fraction, :fraction)
      DRCOpNodeFilter::new(@engine, self, :new_centers, "centers", self._make_value(length, :length), fraction)
    end
  end
  
  def extended(*args)
  
    self.engine._context("extended") do

      av = [ 0, 0, 0, 0 ]
      args.each_with_index do |a,i|
        if a.is_a?(Hash)
          a[:begin]  && av[0] = self._make_value(a[:begin], :begin)
          a[:end]    && av[1] = self._make_value(a[:end], :end)
          a[:out]    && av[2] = self._make_value(a[:out], :out)
          a[:in]     && av[3] = self._make_value(a[:in], :in)
          a[:joined] && av[4] = true
        elsif i < 4
          av[i] = self._make_value(a, "argument " + (i+1).to_s)
        else
          raise("Too many arguments for method '#{f}' (1 to 5 expected)")
        end
      end

      DRCOpNodeFilter::new(@engine, self, :new_extended, "extended", *args)

    end
    
  end
  
  def extended_in(e)
    self.engine._context("extended_in") do
      DRCOpNodeFilter::new(@engine, self, :new_extended_in, "extended_in", self._make_value(e))
    end
  end
  
  def extended_out(e)
    self.engine._context("extended_out") do
      DRCOpNodeFilter::new(@engine, self, :new_extended_out, "extended_out", self._make_value(e))
    end
  end
  
  def polygons
    self.engine._context("polygons") do
      DRCOpNodeFilter::new(@engine, self, :new_polygons, "polygons")
    end
  end
  
end

class DRCOpNodeLogicalBool < DRCOpNode

  attr_accessor :children
  attr_accessor :op
  
  def initialize(engine, op)
    super(engine)
    self.children = []
    self.op = op
    self.description = op.to_s
  end

  def dump(indent)
    return indent + self.description + "\n" + self.children.collect { |c| c.dump("  " + indent) }.join("\n")
  end

  def do_create_node(cache)
    log_op = { 
      :if_all => RBA::CompoundRegionOperationNode::LogicalOp::LogAnd, 
      :if_any => RBA::CompoundRegionOperationNode::LogicalOp::LogOr,
      :if_none => RBA::CompoundRegionOperationNode::LogicalOp::LogOr 
    } [self.op]
    invert = { 
      :if_all => false,
      :if_any => false,
      :if_none => true
    } [self.op]
    RBA::CompoundRegionOperationNode::new_logical_boolean(log_op, invert, self.children.collect { |c| c.create_node(cache) })
  end
  
end

class DRCOpNodeBool < DRCOpNode

  attr_accessor :children
  attr_accessor :op
  
  def initialize(engine, op, a, b)
    super(engine)
    self.children = [a, b]
    self.op = op
    self.description = "Geometrical #{op.to_s}"
  end

  def dump(indent)
    return indent + self.description + "\n" + self.children.collect { |c| c.dump("  " + indent) }.join("\n")
  end

  def do_create_node(cache)
    bool_op = { :& => RBA::CompoundRegionOperationNode::GeometricalOp::And, 
                :+ => RBA::CompoundRegionOperationNode::GeometricalOp::Or,
                :| => RBA::CompoundRegionOperationNode::GeometricalOp::Or,
                :- => RBA::CompoundRegionOperationNode::GeometricalOp::Not,
                :^ => RBA::CompoundRegionOperationNode::GeometricalOp::Xor }[self.op]
    nodes = self.children.collect do |c| 
      n = c.create_node(cache)
      if n.result_type == RBA::CompoundRegionOperationNode::ResultType::EdgePairs
        n = RBA::CompoundRegionOperationNode::new_edge_pair_to_first_edges(n)
      end
      n
    end
    RBA::CompoundRegionOperationNode::new_geometrical_boolean(bool_op, *nodes)
  end
  
end

class DRCOpNodeCase < DRCOpNode

  attr_accessor :children
  
  def initialize(engine, children)
    super(engine)
    self.children = children
    self.description = "switch"
  end

  def dump(indent)
    return indent + self.description + "\n" + self.children.collect { |c| c.dump("  " + indent) }.join("\n")
  end
  
  def do_create_node(cache)
    RBA::CompoundRegionOperationNode::new_case(self.children.collect { |c| c.create_node(cache) })
  end

end

class DRCOpNodeWithCompare < DRCOpNode
  
  attr_accessor :reverse
  attr_accessor :original
  attr_accessor :lt, :le, :gt, :ge, :arg
  
  def initialize(engine, original = nil, reverse = false)
    super(engine)
    self.reverse = reverse
    self.original = original
    self.description = original ? original.description : "BasicWithCompare"
  end
  
  def _description_for_dump
    self.description
  end
  
  def dump(indent = "")
    if self.original
      return "@temp (should not happen)"
    else
      cmp = []
      self.lt && (cmp << ("<%.12g" % self.lt))
      self.le && (cmp << ("<=%.12g" % self.le))
      self.gt && (cmp << (">%.12g" % self.gt))
      self.ge && (cmp << (">=%.12g" % self.ge))
      return indent + self.description + " " + cmp.join(" ")
    end
  end

  def _check_bounds
    if (self.lt || self.le) && (self.gt || self.ge)
      epsilon = 1e-10
      lower = self.ge ? self.ge - epsilon : self.gt + epsilon
      upper = self.le ? self.le + epsilon : self.lt - epsilon
      if lower > upper - epsilon
        raise("Lower bound is larger than upper bound")
      end 
    end  
  end
    
  def set_lt(value)
    (self.lt || self.le) && raise("'" + self.description + "' already has an upper bound of " + ("%.12g" % (self.lt || self.le)))
    self.lt = value
    self._check_bounds
  end    

  def set_le(value)
    (self.lt || self.le) && raise("'" + self.description + "' already has an upper bound of " + ("%.12g" % (self.lt || self.le)))
    self.le = value
    self._check_bounds
  end    
      
  def set_gt(value)
    (self.gt || self.ge) && raise("'" + self.description + "' already has an lower bound of " + ("%.12g" % (self.gt || self.ge)))
    self.gt = value
    self._check_bounds
  end    

  def set_ge(value)
    (self.gt || self.ge) && raise("'" + self.description + "' already has an lower bound of " + ("%.12g" % (self.gt || self.ge)))
    self.ge = value
    self._check_bounds
  end
  
  def coerce(something)
    [ DRCOpNodeWithCompare::new(self.engine, self, true), something ]
  end
  
  def _self_or_original
    return self.original || self
  end
  
  def ==(other)
    if !(other.is_a?(Float) || other.is_a?(Integer))
      raise("== operator needs a numerical argument for '" + self.description + "' argument")
    end
    res = self._self_or_original
    res.set_le(other)
    res.set_ge(other)
    return res
  end
  
  def <(other)
    if !(other.is_a?(Float) || other.is_a?(Integer))
      raise("< operator needs a numerical argument for '" + self.description + "' argument")
    end
    res = self._self_or_original
    if reverse
      res.set_gt(other)
    else
      res.set_lt(other)
    end
    return res
  end
  
  def <=(other)
    if !(other.is_a?(Float) || other.is_a?(Integer))
      raise("<= operator needs a numerical argument for '" + self.description + "' argument")
    end
    res = self._self_or_original
    if reverse
      res.set_ge(other)
    else
      res.set_le(other)
    end
    return res
  end

  def >(other)
    if !(other.is_a?(Float) || other.is_a?(Integer))
      raise("> operator needs a numerical argument for '" + self.description + "' argument")
    end
    res = self._self_or_original
    if reverse
      res.set_lt(other)
    else
      res.set_gt(other)
    end
    return res
  end
  
  def >=(other)
    if !(other.is_a?(Float) || other.is_a?(Integer))
      raise(">= operator needs a numerical argument for '" + self.description + "' argument")
    end
    res = self._self_or_original
    if reverse
      res.set_le(other)
    else
      res.set_ge(other)
    end
    return res
  end
  
end

class DRCOpNodeAreaFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverted
  
  def initialize(engine, input)
    super(engine)
    self.input = input
    self.inverted = false
    self.description = "area"
  end

  def _description_for_dump
    self.inverted ? "area" : "not_area"
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    args << (self.gt ? make_area_value(self.gt) + 1 : (self.ge ? make_area_value(self.ge) : 0))
    if self.lt || self.le
      args << self.lt ? make_area_value(self.lt) : make_area_value(self.le) - 1
    end
    RBA::CompoundRegionOperationNode::new_area_filter(*args)
  end

  def inverted
    res = self.dup
    res.inverted = !res.inverted
    return res
  end
  
end

class DRCOpNodeEdgeLengthFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverted
  
  def initialize(engine, input)
    super(engine)
    self.input = input
    self.inverted = false
    self.description = "length"
  end
  
  def _description_for_dump
    self.inverted ? "length" : "not_length"
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    args << (self.gt ? self._make_value(self.gt, :gt) + 1 : (self.ge ? self._make_value(self.ge, :ge) : 0))
    if self.lt || self.le
      args << self.lt ? self._make_value(self.lt, :lt) : self._make_value(self.le, :le) - 1
    end
    RBA::CompoundRegionOperationNode::new_edge_length_filter(*args)
  end

  def inverted
    res = self.dup
    res.inverted = !res.inverted
    return res
  end
  
end

class DRCOpNodeEdgeOrientationFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverted
  
  def initialize(engine, input)
    super(engine)
    self.input = input
    self.inverted = false
    self.description = "angle"
  end
  
  def _description_for_dump
    self.inverted ? "angle" : "not_angle"
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    angle_delta = 1e-6
    args << (self.gt ? self.gt + angle_delta : (self.ge ? self.ge : -180.0))
    args << (self.lt ? self.lt : (self.le ? self.le - angle_delta : 180.0))
    RBA::CompoundRegionOperationNode::new_edge_orientation_filter(*args)
  end

  def inverted
    res = self.dup
    res.inverted = !res.inverted
    return res
  end
  
end

class DRCOpNodePerimeterFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverted
  
  def initialize(engine, input)
    super(engine)
    self.input = input
    self.inverted = false
    self.description = "perimeter"
  end

  def _description_for_dump
    self.inverted ? "perimeter" : "not_perimeter"
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    args << (self.gt ? self._make_value(self.gt, :gt) + 1 : (self.ge ? self._make_value(self.ge, :ge) : 0))
    if self.lt || self.le
      args << self.lt ? self._make_value(self.lt, :lt) : self._make_value(self.le, :le) - 1
    end
    RBA::CompoundRegionOperationNode::new_perimeter_filter(*args)
  end

  def inverted
    res = self.dup
    res.inverted = !res.inverted
    return res
  end
  
end

class DRCOpNodeInteractingWithCount < DRCOpNodeWithCompare

  attr_accessor :a, :b
  attr_accessor :inverted
  attr_accessor :op
  
  def initialize(engine, a, b, op)
    super(engine)
    self.a = a
    self.b = b
    self.op = op
    self.inverted = false
    self.description = (self.inverted ? "" : "not_") + self.op.to_s
  end
  
  def do_create_node(cache)
    args = [ self.a.create_node(cache), self.b.create_node(cache), self.inverse ]
    args << (self.gt ? self.gt + 1 : (self.ge ? self.ge : 0))
    if self.lt || self.le
      args << self.lt ? self.lt : self.le - 1
    end
    factory = { :covering => :new_enclosing,
                :overlapping => :new_overlapping,
                :interacting => :new_interacting }[self.op]
    RBA::CompoundRegionOperationNode::send(factory, *args)
  end

  def inverted
    res = self.dup
    res.inverted = !res.inverted
    return res
  end
  
end

class DRCOpNodeInteracting < DRCOpNode

  attr_accessor :a, :b
  attr_accessor :inverted
  attr_accessor :op
  
  def initialize(engine, a, b, op)
    super(engine)
    self.a = a
    self.b = b
    self.op = op
    self.inverted = false
    self.description = (self.inverted ? "" : "not_") + self.op.to_s
  end
  
  def do_create_node(cache)
    factory = { :inside => :new_inside,
                :outside => :new_outside }[self.op]
    RBA::CompoundRegionOperationNode::send(factory, self.a.create_node(cache), self.b.create_node(cache), self.inverse)
  end

  def inverted
    res = self.dup
    res.inverted = !res.inverted
    return res
  end
  
end

class DRCOpNodeFilter < DRCOpNode

  attr_accessor :input
  attr_accessor :factory
  attr_accessor :args
  
  def initialize(engine, input, factory, description, *args)
    super(engine)
    self.input = input
    self.factory = factory
    self.args = args
    self.description = description
  end
  
  def dump(indent)
    if self.args.size > 0
      return self.description + "(" + self.args.collect { |a| a.inspect }.join(",") + ")\n" + input.dump("  " + indent)
    else
      return self.description + "\n" + input.dump("  " + indent)
    end
  end
  
  def do_create_node(cache)
    RBA::CompoundRegionOperationNode::send(self.factory, self.input.create_node(cache), *args)
  end
  
end

class DRCOpNodeCheck < DRCOpNodeWithCompare

  attr_accessor :other
  attr_accessor :check
  attr_accessor :args
  
  def initialize(engine, check, other, *args)
    super(engine)
    self.check = check
    self.other = other
    self.args = args
    self.description = check.to_s
  end

  def _description_for_dump
    if self.args.size > 0
      return self.description + "(" + self.args.collect { |a| a.inspect }.join(",") + ")"
    else
      return self.description
    end
  end
  
  def do_create_node(cache)
  
    if !(self.lt || self.le) && !(self.gt || self.ge)
      raise("No value given for check #{self.check}")
    end
    
    factory = { :width => :new_width_check, :space => :new_space_check,
                :notch => :new_notch_check, :separation => :new_separation_check,
                :isolated => :new_isolated_check, :overlap => :new_overlap_check, 
                :enclosing => :new_inside_check }[self.check]

    if self.lt || self.le
      dmin = self.le ? self._make_value(self.le, :le) + 1 : self._make_value(self.lt, :lt)
      res = RBA::CompoundRegionOperationNode::send(factory, dmin, *self.args)
    else
      res = nil
    end
      
    if self.gt || self.ge
      dmax = self.ge ? self._make_value(self.ge, :ge) : self._make_value(self.gt, :gt) + 1
      max_check = RBA::CompoundRegionOperationNode::send(factory, dmax, *self.args + [ true ])
      res_max = RBA::CompoundRegionOperationNode::new_edge_pair_to_first_edges(max_check)
      if res
        if self.check == :width || self.check == :notch
          # Same polygon check - we need to take both edges of the result
          and_with = RBA::CompoundRegionOperationNode::new_edges(res)
        else
          and_with = RBA::CompoundRegionOperationNode::new_edge_pair_to_first_edges(res)
        end
        res = RBA::CompoundRegionOperationNode::new_geometrical_boolean(RBA::CompoundRegionOperationNode::GeometricalOp::And, and_with, res_max)
      else
        res = res_max
      end
    end
    
    return res

  end

end

class DRCOpNodeBBoxParameterFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :parameter
  attr_accessor :inverted
  
  def initialize(engine, parameter, input, description)
    super(engine)
    self.parameter = parameter
    self.input = input
    self.inverted = false
    self.description = description
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    args << (self.gt ? self._make_value(self.gt, :gt) + 1 : (self.ge ? self._make_value(self.ge, :ge) : 0))
    if self.lt || self.le
      args << self.lt ? self._make_value(self.lt, :lt) : self._make_value(self.le, :le) - 1
    end
    RBA::CompoundRegionOperationNode::new_perimeter_filter(*args)
  end

  def inverted
    res = self.dup
    res.inverted = !res.inverted
    return res
  end
  
end

class DRCOpNodeCornersFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :parameter
  attr_accessor :inverted
  
  def initialize(engine, as_dots, input)
    super(engine)
    self.as_dots = as_dots
    self.input = input
    self.description = "corners"
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache) ]
    angle_delta = 1e-6
    args << (self.gt ? self.gt + angle_delta : (self.ge ? self.ge : -180.0))
    args << (self.lt ? self.lt : (self.le ? self.le - angle_delta : 180.0))
    if self.as_dots
      RBA::CompoundRegionOperationNode::new_corners_as_dots_node(*args)
    else
      args << 2 # dimension is 2x2 DBU
      RBA::CompoundRegionOperationNode::new_corners_as_rectangles_node(*args)
    end
  end

end

class DRCOpNodeRelativeExtents < DRCOpNode

  attr_accessor :input
  attr_accessor :as_edges, :fx1, :fx2, :fy1, :fy2, :dx, :dy
  
  def initialize(engine, input, as_edges, fx1, fx2, fy1, fy2, dx = 0, dy = 0)
    super(engine)
    self.input = input
    self.as_edges = as_edges
    self.description = "extents"
  end
  
  def dump(indent)
    if !self.as_edges
      return "extents(%.12g,%.12g,%.12g,%.12g,%12g,%.12g)" % [self.fx1, self.fx2, self.fy1, self.fy2, self.dx, self.dy]
    else
      return "extents_as_edges(%.12g,%.12g,%.12g,%.12g)" % [self.fx1, self.fx2, self.fy1, self.fy2]
    end
  end
      
  def do_create_node(cache)
    if !self.as_edges
      RBA::CompoundRegionOperationNode::new_relative_extents_as_edges(self.input, self.fx1, self.fx2, self.fy1, self.fy2, self.dx, self.dy)
    else
      RBA::CompoundRegionOperationNode::new_relative_extents_as_edges(self.input, self.fx1, self.fx2, self.fy1, self.fy2)
    end
  end

end

end
