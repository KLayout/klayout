# $autorun-early

module DRC

# %DRC%
# @scope
# @name DRC
# @brief DRC Reference: DRC expressions
#
# DRC expression objects represent abstract recipes for the \Layer#drc universal DRC function.
# For example, when using a universal DRC expression like this:
#
# @code
# out = in.drc(width < 2.0)
# @/code
#
# "width < 2.0" forms a DRC expression object. DRC expression objects have methods which
# manipulate or evaluate the results of this expression. In addition, DRC expressions have a
# result type, which is either polygon, edge or edge pair. The result type is defined by the
# expression generating it. In the example above, "width < 2.0" is a DRC width check which
# renders edge pairs. To obtain polygons from these edge pairs, use the "polygons" method:
#
# @code
# out = in.drc((width < 2.0).polygons)
# @/code
#
# The following global functions are relevant for the DRC expressions:
# 
# @ul
# @li \global#angle @/li
# @li \global#area @/li
# @li \global#area_ratio @/li
# @li \global#bbox_aspect_ratio @/li
# @li \global#bbox_height @/li
# @li \global#bbox_max @/li
# @li \global#bbox_min @/li
# @li \global#bbox_width @/li
# @li \global#corners @/li
# @li \global#covering @/li
# @li \global#enc @/li
# @li \global#enclosing @/li
# @li \global#extent_refs @/li
# @li \global#extents @/li
# @li \global#foreign @/li
# @li \global#holes @/li
# @li \global#hulls @/li
# @li \global#if_all @/li
# @li \global#if_any @/li
# @li \global#if_none @/li
# @li \global#inside @/li
# @li \global#interacting @/li
# @li \global#iso @/li
# @li \global#length @/li
# @li \global#middle @/li
# @li \global#notch @/li
# @li \global#outside @/li
# @li \global#overlap @/li
# @li \global#overlapping @/li
# @li \global#perimeter @/li
# @li \global#primary @/li
# @li \global#rectangles @/li
# @li \global#rectilinear @/li
# @li \global#relative_height @/li
# @li \global#rounded_corners @/li
# @li \global#secondary @/li
# @li \global#separation @/li
# @li \global#sep @/li
# @li \global#sized @/li
# @li \global#smoothed @/li
# @li \global#space @/li
# @li \global#squares @/li
# @li \global#switch @/li
# @li \global#width @/li
# @li \global#with_holes @/li
# @/ul
# 
# The following documentation will list the methods available for DRC expression objects.
  
# A base class for implementing ranges that can be put into a condition
module DRCComparable
  
  attr_accessor :reverse
  attr_accessor :original
  attr_accessor :lt, :le, :gt, :ge
  attr_accessor :description
  attr_accessor :mode_or_supported
  attr_accessor :mode_or
  
  def _init_comparable
    self.reverse = false
    self.original = nil
    self.le = nil
    self.ge = nil
    self.lt = nil
    self.gt = nil
    self.gt = nil
    self.description = ""
    self.mode_or_supported = false
    self.mode_or = false
  end
  
  def _check_bounds
    if ! self.mode_or && (self.lt || self.le) && (self.gt || self.ge)
      epsilon = 1e-10
      lower = self.ge ? self.ge - epsilon : self.gt + epsilon
      upper = self.le ? self.le + epsilon : self.lt - epsilon
      if lower > upper - epsilon
        raise("'" + self.description + "': lower bound is larger than upper bound")
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
    reversed = self.dup
    reversed.reverse = true
    reversed.original = self
    [ reversed, something ]
  end
  
  def _self_or_original
    return (self.original || self).dup
  end
  
  def !=(other)
    if self.respond_to?(:inverted)
      res = self.==(other).inverted
    else
      if !self.mode_or_supported
        raise("!= operator is not allowed for '" + self.description + "'")
      end
      if !(other.is_a?(Float) || other.is_a?(Integer))
        raise("!= operator needs a numerical argument for '" + self.description + "' argument")
      end
      res = self._self_or_original
      res.mode_or = true
      res.set_lt(other)
      res.set_gt(other)
    end
    res
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

class DRCOpNode

  attr_accessor :description
  attr_accessor :engine
  
  def initialize(engine, &factory)
    @factory = factory
    self.engine = engine
    self.description = "Basic"
  end
  
  def create_node(cache)
    n = cache[self.object_id]
    if !n || n.destroyed?
      n = self.do_create_node(cache)
      cache[self.object_id] = n
    end
    n
  end
  
  def do_create_node(cache)
    @factory.call
  end
  
  def dump(indent)
    return indent + self.description
  end
  
  def _build_geo_bool_node(other, op)
    other = @engine._make_node(other)
    if ! other.is_a?(DRCOpNode)
      raise("Second argument to #{op.to_s} must be a DRC expression")
    end
    if op == :+
      a = self.is_a?(DRCOpNodeJoin) ? self.children : [ self ]
      b = other.is_a?(DRCOpNodeJoin) ? other.children : [ other ]
      return DRCOpNodeJoin::new(@engine, a + b)
    else
      return DRCOpNodeBool::new(@engine, op, self, other)
    end
  end

  # %DRC%
  # @name &
  # @brief Boolean AND between the results of two expressions
  # @synopsis expression & expression
  # 
  # The & operator will compute the boolean AND between the results
  # of two expressions. The expression types need to be edge or polygon.
  # 
  # The following example computes the partial edges where width is less than 
  # 0.3 micrometers and space is less than 0.2 micrometers:
  #
  # @code
  # out = in.drc((width < 0.3).edges & (space < 0.2).edges)
  # @/code
  
  # %DRC%
  # @name |
  # @brief Boolean OR between the results of two expressions
  # @synopsis expression | expression
  #
  # The | operator will compute the boolean OR between the results of 
  # two expressions. '+' is basically a synonym. Both expressions
  # must render the same type.

  # %DRC%
  # @name +
  # @brief Boolean OR between the results of two expressions
  # @synopsis expression + expression
  #
  # The + operator will join the results of two expressions.
   
  # %DRC%
  # @name -
  # @brief Boolean NOT between the results of two expressions
  # @synopsis expression - expression
  #
  # The - operator will compute the difference between the results
  # of two expressions. The NOT operation is defined for polygons,
  # edges and polygons subtracted from edges (first argument edge,
  # second argument polygon).
  #
  # CAUTION: be careful not to take secondary input for the 
  # first argument. This will not render the desired results.
  # Remember that the "drc" function will walk over all primary
  # shapes and present single primaries to the NOT operation together
  # with the secondaries of that single shape. So when you use
  # secondary shapes as the first argument, they will not see all
  # all the primaries required to compute the correct result.
  # That's also why a XOR operation cannot be provided in the 
  # context of a generic DRC function.
  # 
  # The following example will produce edge markers where the 
  # width of is less then 0.3 micron but not inside polygons on 
  # the "waive" layer:
  #  
  # @code
  # out = in.drc((width < 0.3).edges - secondary(waive))
  # @/code
  
  %w(& - | +).each do |f|
    eval <<"CODE"
      def #{f}(other)
        @engine._context("#{f}") do
          self._build_geo_bool_node(other, :#{f})
        end
      end
CODE
  end

  # %DRC%
  # @name !
  # @brief Logical not
  # @synopsis ! expression
  #
  # This operator will evaluate the expression after. If this expression renders
  # an empty result, the operator will return the primary shape. Otherwise it will
  # return an empty result.
  # 
  # This operator can be used together with predicates such a "rectangles" to 
  # invert their meaning. For example, this code selects all primary shapes which
  # are not rectangles:
  #
  # @code
  # out = in.drc(! rectangles)
  # out = in.drc(! primary.rectangles)   # equivalent
  # @/code
  
  def !()
    @engine._context("!") do
      if self.respond_to?(:inverted)
        return self.inverted
      else
        # TODO: what if the expression isn't region?
        empty = DRCOpNode::new(@engine) { RBA::CompoundRegionOperationNode::new_empty(RBA::CompoundRegionOperationNode::ResultType::Region) }
        DRCOpNodeCase::new(@engine, [ self, empty, @engine.primary ])
      end
    end
  end

  # %DRC%
  # @name area
  # @brief Selects the primary shape if the area is meeting the condition
  # @synopsis expression.area (in condition)
  #
  # This operation is used in conditions to select shapes based on their area.
  # It is applicable on polygon expressions. The result will be the input 
  # polygons if the area condition is met. 
  #
  # See \Layer#drc for more details about comparison specs.
  #
  # The following example will select all polygons with an area less than 2.0 square micrometers:
  #
  # @code
  # out = in.drc(area < 2.0)
  # out = in.drc(primary.area < 2.0)   # equivalent
  # @/code
  #
  # The area method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.area".
  
  def area
    DRCOpNodeAreaFilter::new(@engine, self, false)
  end
  
  # %DRC%
  # @name area_sum
  # @brief Selects the input polygons if the sum of all areas meets the condition
  # @synopsis expression.area_sum (in condition)
  #
  # Returns the input polygons if the sum of their areas meets the specified
  # condition. This condition is evaluated on the total of all shapes generated in one step of the 
  # "drc" loop. As there is a single primary in each loop iteration, "primary.area_sum" is
  # equivalent to "primary.area".
  #
  # See \Layer#drc for more details about comparison specs.
  
  def area_sum
    DRCOpNodeAreaFilter::new(@engine, self, true)
  end
  
  # %DRC%
  # @name count
  # @brief Selects a expression result based on the number of (local) shapes
  # @synopsis expression.count (in condition)
  #
  # This operation is used in conditions to select expression results based on their
  # count. "count" is used as a method on a expression. It will evaluate the expression locally
  # and return the original result if the shape count in the result is matching the condition.
  #
  # See \Layer#drc for more details about comparison specs.
  #
  # Note that the expression is evaluated locally: for each primary shape, the expression is
  # evaluated and the count of the resulting edge, edge pair or polygon set is taken.
  # As the primary input will always have a single item (the local shape), using "count" on
  # primary does not really make sense. It can be used on derived expressions however.
  #
  # The following example selects triangles:
  # 
  # @code
  # out = in.drc(if_any(corners.count == 3))
  # @/code
  #
  # Note "if_any" which selects the primary shape if the argument evaluates to a non-empty
  # result. Without "if_any" three corners are returned for each triangle.
  
  def count
    DRCOpNodeCountFilter::new(@engine, self, :new_count_filter, "count")
  end
  
  # %DRC%
  # @name perimeter
  # @brief Selects the input polygon if the perimeter is meeting the condition
  # @synopsis expression.perimeter (in condition)
  #
  # This operation is used in conditions to select shapes based on their perimeter.
  # It is applicable on polygon expressions. The result will be the input 
  # polygons if the perimeter condition is met. 
  #
  # See \Layer#drc for more details about comparison specs.
  #
  # The following example will select all polygons with a perimeter less than 10 micrometers:
  #
  # @code
  # out = in.drc(perimeter < 10.0)
  # out = in.drc(primary.perimeter < 10.0)   # equivalent
  # @/code
  #
  # The perimeter method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.perimeter".
  
  def perimeter
    DRCOpNodePerimeterFilter::new(@engine, self, false)
  end
  
  # %DRC%
  # @name perimeter_sum
  # @brief Selects the input polygons if the sum of all perimeters meets the condition
  # @synopsis expression.perimeter_sum (in condition)
  #
  # Returns the input polygons if the sum of their perimeters meets the specified
  # condition. This condition is evaluated on the total of all shapes generated in one step of the 
  # "drc" loop. As there is a single primary in each loop iteration, "primary.perimeter_sum" is
  # equivalent to "primary.perimeter".
  #
  # See \Layer#drc for more details about comparison specs.
  
  def perimeter_sum
    DRCOpNodePerimeterFilter::new(@engine, self, true)
  end
  
  # %DRC%
  # @name bbox_min
  # @brief Selects the input polygon if its bounding box smaller dimension is meeting the condition
  # @synopsis expression.bbox_min (in condition)
  #
  # This operation is used in conditions to select shapes based on smaller dimension of their bounding boxes.
  # It is applicable on polygon expressions. The result will be the input 
  # polygons if the bounding box condition is met. 
  #
  # See \Layer#drc for more details about comparison specs.
  #
  # The following example will select all polygons whose bounding box smaller dimension is larger
  # than 200 nm:
  #
  # @code
  # out = in.drc(bbox_min > 200.nm)
  # out = in.drc(primary.bbox_min > 200.nm)   # equivalent
  # @/code
  #
  # The "bbox_min" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.bbox_min".
  
  def bbox_min
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::ParameterType::BoxMinDim, self)
  end
  
  # %DRC%
  # @name bbox_max
  # @brief Selects the input polygon if its bounding box larger dimension is meeting the condition
  # @synopsis expression.bbox_max (in condition)
  # 
  # This operation acts similar to \DRC#bbox_min, but takes the larger dimension of the shape's 
  # bounding box.
  #
  # The "bbox_max" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.bbox_max".

  def bbox_max
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::ParameterType::BoxMaxDim, self)
  end
  
  # %DRC%
  # @name bbox_width
  # @brief Selects the input polygon if its bounding box width is meeting the condition
  # @synopsis expression.bbox_width (in condition)
  # 
  # This operation acts similar to \DRC#bbox_min, but takes the width of the shape's 
  # bounding box. In general, it's more advisable to use \DRC#bbox_min or \DRC#bbox_max
  # because bbox_width implies a certain orientation. This can imply variant formation in 
  # hierarchical contexts: cells rotated by 90 degree have to be treated differently from
  # ones not rotated. This usually results in a larger computation effort and larger result files.
  #
  # The "bbox_width" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.bbox_width".

  def bbox_width
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::ParameterType::BoxWidth, self)
  end
  
  # %DRC%
  # @name bbox_height
  # @brief Selects the input polygon if its bounding box height is meeting the condition
  # @synopsis expression.bbox_height (in condition)
  # 
  # This operation acts similar to \DRC#bbox_min, but takes the height of the shape's 
  # bounding box. In general, it's more advisable to use \DRC#bbox_min or \DRC#bbox_max
  # because bbox_height implies a certain orientation. This can imply variant formation in 
  # hierarchical contexts: cells rotated by 90 degree have to be treated differently from
  # ones not rotated. This usually results in a larger computation effort and larger result files.
  #
  # The "bbox_height" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.bbox_height".

  def bbox_height
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::ParameterType::BoxHeight, self)
  end
  
  # %DRC%
  # @name bbox_aspect_ratio
  # @brief Selects the input polygon according to the aspect ratio of the bounding box
  # @synopsis expression.bbox_aspect_ratio (in condition)
  #
  # This operation is used in conditions to select shapes based on aspect ratios of their bounding boxes.
  # The aspect ratio is computed by dividing the larger of width and height by the smaller of both.
  # The aspect ratio is always larger or equal to 1. Square or square-boxed shapes have a
  # bounding box aspect ratio of 1.
  #
  # This filter is applicable on polygon expressions. The result will be the input 
  # polygon if the bounding box condition is met. 
  #
  # See \Layer#drc for more details about comparison specs.
  #
  # The following example will select all polygons whose bounding box aspect ratio is larger than 3:
  #
  # @code
  # out = in.drc(bbox_aspect_ratio > 3)
  # out = in.drc(primary.bbox_aspect_ratio > 3)   # equivalent
  # @/code
  #
  # The "bbox_aspect_ratio" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.bbox_aspect_ratio".
  
  def bbox_aspect_ratio
    DRCOpNodeRatioParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::RatioParameterType::AspectRatio, self)
  end
  
  # %DRC%
  # @name relative_height
  # @brief Selects the input polygon according to the height vs. width of the bounding box
  # @synopsis expression.relative_height (in condition)
  #
  # This operation is used in conditions to select shapes based on the ratio of bounding box
  # height vs. width. The taller the shape, the larger the value. Wide polygons have a value
  # below 1. A square has a relative height of 1.
  #
  # This filter is applicable on polygon expressions. The result will be the input 
  # polygon if the condition is met.
  #
  # Don't use this method if you can use \bbox_aspect_ratio, because the latter is
  # isotropic and can be used hierarchically without generating rotation variants.
  #
  # See \Layer#drc for more details about comparison specs.
  #
  # The following example will select all polygons whose relative height is larger than 3:
  #
  # @code
  # out = in.drc(relative_height > 3)
  # out = in.drc(primary.relative_height > 3)   # equivalent
  # @/code
  #
  # The "relative_height" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.bbox_aspect_ratio".
  
  def relative_height
    DRCOpNodeRatioParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::RatioParameterType::RelativeHeight, self)
  end
  
  # %DRC%
  # @name area_ratio
  # @brief Selects the input polygon according to its area ratio (bounding box area by polygon area)
  # @synopsis expression.area_ratio (in condition)
  #
  # This operation is used in conditions to select shapes based on their area ratio.
  # The area ratio is the ratio of bounding box vs. polygon area. It's a measure how
  # "sparse" the polygons are and how good an approximation the bounding box is. 
  # The value is always larger or equal than 1. Boxes have a value of 1.
  #
  # This filter is applicable on polygon expressions. The result will be the input 
  # polygon if the condition is met.
  #
  # See \Layer#drc for more details about comparison specs.
  #
  # The following example will select all polygons whose area ratio is larger than 3:
  #
  # @code
  # out = in.drc(area_ratio > 3)
  # out = in.drc(primary.area_ratio > 3)   # equivalent
  # @/code
  #
  # The "area_ratio" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.area_ratio".
  
  def area_ratio
    DRCOpNodeRatioParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::RatioParameterType::AreaRatio, self)
  end
  
  # %DRC%
  # @name length
  # @brief Selects edges based on their length
  # @synopsis expression.length (in condition)
  # 
  # This operation will select those edges which are meeting the length
  # criterion. Non-edge shapes (polygons, edge pairs) will be converted to edges before.
  #
  # For example, this code selects all edges from the primary shape which are longer or
  # equal than 1 micrometer:
  #
  # @code
  # out = in.drc(length >= 1.um) 
  # out = in.drc(primary.length >= 1.um)   # equivalent
  # @/code
  #
  # The "length" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.length".

  def length
    DRCOpNodeEdgeLengthFilter::new(@engine, self, false)
  end
  
  # %DRC%
  # @name length_sum
  # @brief Selects the input edges if the sum of their lengths meets the condition
  # @synopsis expression.length_sum (in condition)
  #
  # Returns the input edges if the sum of their lengths meets the specified
  # condition. This condition is evaluated on the total of all edges generated in one step of the 
  # "drc" loop.
  #
  # See \Layer#drc for more details about comparison specs.
  
  def length_sum
    DRCOpNodeEdgeLengthFilter::new(@engine, self, true)
  end
  
  # %DRC%
  # @name angle
  # @brief Selects edges based on their angle
  # @synopsis expression.angle (in condition)
  # 
  # This operation selects edges by their angle, measured against the horizontal 
  # axis in the mathematical sense. 
  #
  # For this measurement edges are considered without their direction.
  # A horizontal edge has an angle of zero degree. A vertical one has
  # an angle of 90 degrees. The angle range is from -90 (exclusive) to 90 degree (inclusive).
  #
  # If the input shapes are not polygons or edge pairs, they are converted to edges 
  # before the angle test is made.
  # 
  # For example, the following code selects all edges from the primary shape which are 45 degree
  # (up) or -45 degree (down). The "+" operator will join the results:
  #
  # @code
  # out = in.drc((angle == 45) + (angle == -45)) 
  # out = in.drc((primary.angle == 45) + (primary.angle == -45))    # equivalent
  # @/code
  #
  # You can avoid using both 45 and -45 degree checks with the 'absolute' option.
  # With this option, angles are not signed and the value is the absolute angle 
  # the edge encloses with the x axis:
  #
  # @code
  # out = in.drc(angle(absolute) == 45)
  # @/code
  #
  # Note that angle checks usually imply the need for rotation variant formation as cells which
  # are placed non-rotated and rotated by 90 degree cannot be considered identical. This imposes
  # a performance penalty in hierarchical mode. If possible, consider using \DRC#rectilinear for
  # example to detect shapes with non-manhattan geometry instead of using angle checks.
  #
  # The "angle" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to the method call "primary.angle".

  def angle(*args)

    filter = DRCOpNodeEdgeOrientationFilter::new(@engine, self)

    args.each do |a|
      if a.is_a?(DRCAbsoluteMode)
        filter.absolute = a.value
      else
        raise("Invalid argument (#{a.inspect}) for 'angle' method")
      end
    end
    
    filter
    
  end
  
  # %DRC%
  # @name rounded_corners
  # @brief Applies corner rounding
  # @synopsis expression.rounded_corners(inner, outer, n)
  #
  # This operation acts on polygons and applies corner rounding to the given inner
  # and outer corner radius and the number of points n per full circle. See \Layer#rounded_corners for more details.
  #
  # The "rounded_corners" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.rounded_corners".

  def rounded_corners(inner, outer, n)
    @engine._context("rounded_corners") do
      @engine._check_numeric(n)
      DRCOpNodeFilter::new(@engine, self, :new_rounded_corners, "rounded_corners", @engine._make_value(inner), @engine._make_value(outer), n)
    end
  end
  
  # %DRC%
  # @name smoothed
  # @brief Applies smoothing
  # @synopsis expression.smoothed(d [, keep_hv ])
  #
  # This operation acts on polygons and applies polygon smoothing with the tolerance d. 'keep_hv' indicates
  # whether horizontal and vertical edges are maintained. Default is 'no' which means such edges may be distorted.
  # See \Layer#smoothed for more details.
  #
  # The "smoothed" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.smoothed".

  def smoothed(d, keep_hv = false)
    @engine._context("smoothed") do
      DRCOpNodeFilter::new(@engine, self, :new_smoothed, "smoothed", @engine._make_value(d), keep_hv)
    end
  end
  
  # %DRC%
  # @name corners
  # @brief Selects corners of polygons
  # @synopsis expression.corners
  # @synopsis expression.corners([ options ])
  #
  # This operation acts on polygons and selects the corners of the polygons.
  # It can be put into a condition to select corners by their angles. The angle of
  # a corner is positive for a turn to the left if walking a polygon clockwise
  # and negative for the turn to the right. Hence positive angles indicate concave
  # (inner) corners, negative ones indicate convex (outer) corners.
  # Angles take values between -180 and 180 degree.
  #
  # When using "as_dots" for an option, the operation will return single-point edges at
  # the selected corners. With "as_boxes" (the default), small (2x2 DBU) rectangles will be
  # produced at each selected corner.
  # 
  # Another option is "absolute" which selects the corners by absolute angle - i.e left
  # and right turns will both be considered positive angles.
  #
  # Examples for use of the options are:
  #
  # @code
  # corners(as_dots)
  # corners(as_boxes, absolute)
  # @/code
  #
  # The following example selects all corners:
  #
  # @code
  # out = in.drc(corners)
  # out = in.drc(primary.corners)    # equivalent
  # @/code
  #  
  # The following example selects all inner corners:
  #
  # @code
  # out = in.drc(corners > 0)
  # out = in.drc(primary.corners > 0)    # equivalent
  # @/code
  #
  # The "corners" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.corners".

  def corners(*args)
    @engine._context("corners") do
      output_mode = :as_boxes
      absolute = false
      args.each do |a|
        if a.is_a?(DRCOutputMode)
          output_mode = a.value
        elsif a.is_a?(DRCAbsoluteMode)
          absolute = a.value
        else
          raise("Invalid argument (#{a.inspect}) for 'corners' method")
        end
      end
      DRCOpNodeCornersFilter::new(@engine, output_mode, absolute, self)
    end
  end
  
  # %DRC%
  # @name middle
  # @brief Returns the centers of polygon bounding boxes
  # @synopsis expression.middle([ options ])
  #
  # The middle operation acts on polygons and has the same effect than \Layer#middle.
  # It takes the same arguments. It is available as a method on \DRC# expressions or
  # as plain function, in which case it acts on the primary shapes.

  # %DRC%
  # @name extent_refs
  # @brief Returns partial references to the bounding boxes of the polygons
  # @synopsis expression.extent_refs([ options ])
  #
  # The extent_refs operation acts on polygons and has the same effect than \Layer#extent_refs.
  # It takes the same arguments. It is available as a method on \DRC# expressions or
  # as plain function, in which case it acts on the primary shapes.

  %w(middle extent_refs).each do |f| 
    eval <<"CODE"
    def #{f}(*args)

      @engine._context("#{f}") do

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
          elsif a.is_a?(DRCOutputMode)
            as_edges = (a.value == :edges || a.value == :dots)
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
          return DRCOpNodeRelativeExtents::new(@engine, true, self, *f)
        else
          # add oversize for point- and edge-like regions
          zero_area = (f[0] - f[2]).abs < 1e-7 || (f[1] - f[3]).abs < 1e-7
          f += [ zero_area ? 1 : 0 ] * 2
          return DRCOpNodeRelativeExtents::new(@engine, false, self, *f)
        end

      end

    end
CODE
  end
  
  # %DRC%
  # @name rectangles
  # @brief Selects all polygons which are rectangles
  # @synopsis expression.rectangles
  #
  # This operation can be used as a plain function in which case it acts on primary
  # shapes or can be used as method on another DRC expression.
  # The following example selects all rectangles:
  #
  # @code
  # out = in.drc(rectangles)
  # out = in.drc(primary.rectangles)    # equivalent
  # @/code
  
  def rectangles
    return DRCOpNodeFilter::new(@engine, self, :new_rectangle_filter, "rectangle", false)
  end
  
  # %DRC%
  # @name squares
  # @brief Selects all polygons which are squares
  # @synopsis expression.squares
  #
  # This operation can be used as a plain function in which case it acts on primary
  # shapes or can be used as method on another DRC expression.
  # The following example selects all squares:
  #
  # @code
  # out = in.drc(squares)
  # out = in.drc(primary.squares)    # equivalent
  # @/code
  
  def squares
    return DRCOpNodeFilter::new(@engine, self, :new_rectangle_filter, "rectangle", true)
  end
  
  # %DRC%
  # @name rectilinear
  # @brief Selects all polygons which are rectilinear
  # @synopsis expression.rectilinear
  #
  # Rectilinear polygons only have vertical and horizontal edges. Such polygons are also
  # called manhattan polygons.
  #
  # This operation can be used as a plain function in which case it acts on primary
  # shapes or can be used as method on another DRC expression.
  # The following example selects all manhattan polygons:
  #
  # @code
  # out = in.drc(rectilinear)
  # out = in.drc(primary.rectilinear)    # equivalent
  # @/code
  
  def rectilinear
    return DRCOpNodeFilter::new(@engine, self, :new_rectilinear_filter, "rectilinear")
  end
  
  # %DRC%
  # @name holes
  # @brief Selects all holes from the input polygons
  # @synopsis expression.holes
  #
  # This operation can be used as a plain function in which case it acts on primary
  # shapes or can be used as method on another DRC expression.
  # The following example selects all holes with an area larger than 2 square micrometers:
  #
  # @code
  # out = in.drc(holes.area > 2.um)
  # out = in.drc(primary.holes.area > 2.um)   # equivalent
  # @/code
  
  def holes
    return DRCOpNodeFilter::new(@engine, self, :new_holes, "holes")
  end
  
  # %DRC%
  # @name hulls
  # @brief Selects all hulls from the input polygons
  # @synopsis expression.hulls
  #
  # The hulls are the outer contours of the input polygons. By selecting hulls only,
  # all holes will be closed.
  #
  # This operation can be used as a plain function in which case it acts on primary
  # shapes or can be used as method on another DRC expression.
  # The following example closes all holes:
  #
  # @code
  # out = in.drc(hulls)
  # out = in.drc(primary.hulls)   # equivalent
  # @/code
  
  def hulls
    return DRCOpNodeFilter::new(@engine, self, :new_hulls, "hull")
  end
  
  # %DRC%
  # @name edges
  # @brief Converts the input shapes into edges
  # @synopsis expression.edges
  # @synopsis expression.edges(mode)
  #
  # Polygons will be separated into edges forming their contours. Edge pairs will be 
  # decomposed into individual edges.
  #
  # Contrary most other operations, "edges" does not have a plain function equivalent
  # as this is reserved for the function generating an edges layer.
  # To generate the edges of the primary shapes, use "primary" explicit as the source
  # for the edges:
  #
  # @code
  # out = in.drc(primary.edges)
  # @/code
  #
  # The "mode" argument allows selecting specific edges from polygons.
  # Allowed values are: "convex", "concave", "step", "step_in" and "step_out".
  # "step" generates edges only if they provide a step between two other
  # edges. "step_in" creates edges that make a step towards the inside of
  # the polygon and "step_out" creates edges that make a step towards the
  # outside:
  #
  # @code
  # out = in.drc(primary.edges(convex))
  # @/code
  #
  # In addition, "not_.." variants are available which selects edges
  # not qualifying for the specific mode:
  #
  # @code
  # out = in.drc(primary.edges(not_convex))
  # @/code
  #
  # The mode argument is ignored when translating other objects than
  # polygons.
  
  def edges(mode = nil)
    if mode 
      if ! mode.is_a?(DRC::DRCEdgeMode)
        raise "The mode argument needs to be a mode type (convex, concave, step, step_in or step_out)"
      end
      mode = mode.value
    else
      mode = RBA::EdgeMode::All
    end
    return DRCOpNodeFilter::new(@engine, self, :new_edges, "edges", mode)
  end
  
  # %DRC%
  # @name with_holes
  # @brief Selects all input polygons with the specified number of holes
  # @synopsis expression.with_holes (in condition)
  #
  # This operation can be used as a plain function in which case it acts on primary
  # shapes or can be used as method on another DRC expression.
  # The following example selects all polygons with more than 2 holes:
  #
  # @code
  # out = in.drc(with_holes > 2)
  # out = in.drc(primary.with_holes > 2)   # equivalent
  # @/code
  
  def with_holes
    return DRCOpNodeCountFilter::new(@engine, self, :new_hole_count_filter, "with_holes")
  end
  
  # %DRC%
  # @name merged
  # @brief Returns the merged input polygons, optionally selecting multi-overlap
  # @synopsis expression.merged
  # @synopsis expression.merged(min_count)
  #
  # This operation will act on polygons. Without a min_count argument, the merged
  # polygons will be returned.
  #
  # With a min_count argument, the result will include only those parts where more
  # than the given number of polygons overlap. As the primary input is merged already,
  # it will always contribute as one.
  #
  # The "merged" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.merged".
  
  def merged(*args)

    @engine._context("merged") do

      min_wc = 0
      if args.size > 1
        raise("merged: Method requires no or one value")
      end
      if args.size == 1
        min_wc = @engine._make_numeric_value(args[0])
        min_wc = [ 0, (min_wc - 1).to_i ].max
      end

      min_coherence = true
        
      DRCOpNodeFilter::new(@engine, self, :new_merged, "merged", min_coherence, min_wc)

    end

  end
  
  # %DRC%
  # @name sized
  # @brief Returns the sized version of the input
  # @synopsis expression.sized(d [, mode])
  # @synopsis expression.sized(dx, dy [, mode]))
  #
  # This method provides the same functionality as \Layer#sized and takes the same
  # arguments. It acts on polygon expressions.
  #
  # The "sized" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.sized".

  def sized(*args)

    @engine._context("sized") do

      dist = 0
      
      mode = 2
      values = []
      args.each_with_index do |a,ia|
        if a.is_a?(1.class) || a.is_a?(Float)
          v = @engine._make_value(a)
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
  
  # %DRC%
  # @name extents
  # @brief Returns the bounding box of each input object
  # @synopsis expression.extents([ enlargement ])
  # 
  # This method provides the same functionality as \Layer#extents and takes the same
  # arguments. It returns the bounding boxes of the input objects. It acts on edge
  # edge pair and polygon expressions.
  #
  # The "extents" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.extents".

  def extents(e = 0)
    @engine._context("extents") do
      DRCOpNodeFilter::new(@engine, self, :new_extents, "extents", @engine._make_value(e))
    end
  end
  
  # %DRC%
  # @name first_edges
  # @brief Returns the first edges of edge pairs
  # @synopsis expression.first_edges
  # 
  # This method acts on edge pair expressions and returns the first edges of the
  # edge pairs delivered by the expression.
  #
  # Some checks deliver symmetric edge pairs (e.g. space, width, etc.) for which the
  # edges are commutable. "first_edges" will deliver both edges for such edge pairs.

  # %DRC%
  # @name second_edges
  # @brief Returns the second edges of edge pairs
  # @synopsis expression.second_edges
  # 
  # This method acts on edge pair expressions and returns the second edges of the
  # edge pairs delivered by the expression.
  #
  # Some checks deliver symmetric edge pairs (e.g. space, width, etc.) for which the
  # edges are commutable. "second_edges" will not deliver edges for such edge pairs.
  # Instead, "first_edges" will deliver both.

  def first_edges
    DRCOpNodeFilter::new(@engine, self, :new_edge_pair_to_first_edges, "first_edges")
  end
  
  def second_edges
    DRCOpNodeFilter::new(@engine, self, :new_edge_pair_to_second_edges, "second_edges")
  end

  # %DRC%
  # @name end_segments 
  # @brief Returns the part at the end of each edge of the input
  # @synopsis expression.end_segments(length)
  # @synopsis expression.end_segments(length, fraction)
  #
  # This method acts on edge expressions and delivers a specific part of each edge.
  # See \layer#end_segments for details about this functionality.
  
  # %DRC%
  # @name start_segments 
  # @brief Returns the part at the beginning of each edge of the input
  # @synopsis expression.start_segments(length)
  # @synopsis expression.start_segments(length, fraction)
  #
  # This method acts on edge expressions and delivers a specific part of each edge.
  # See \layer#start_segments for details about this functionality.
  
  # %DRC%
  # @name centers 
  # @brief Returns the part at the center of each edge of the input
  # @synopsis expression.centers(length)
  # @synopsis expression.end_segments(length, fraction)
  #
  # This method acts on edge expressions and delivers a specific part of each edge.
  # See \layer#centers for details about this functionality.
  
  def end_segments(length, fraction = 0.0)
    @engine._context("end_segments") do
      @engine._check_numeric(fraction)
      DRCOpNodeFilter::new(@engine, self, :new_end_segments, "end_segments", @engine._make_value(length), fraction)
    end
  end
  
  def start_segments(length, fraction = 0.0)
    @engine._context("start_segments") do
      @engine._check_numeric(fraction)
      DRCOpNodeFilter::new(@engine, self, :new_start_segments, "start_segments", @engine._make_value(length), fraction)
    end
  end
  
  def centers(length, fraction = 0.0)
    @engine._context("centers") do
      @engine._check_numeric(fraction)
      DRCOpNodeFilter::new(@engine, self, :new_centers, "centers", @engine._make_value(length), fraction)
    end
  end

  # %DRC%
  # @name extended
  # @brief Returns polygons describing an area along the edges of the input
  # @synopsis expression.extended([:begin => b,] [:end => e,] [:out => o,] [:in => i], [:joined => true])
  # @synopsis expression.extended(b, e, o, i)
  # 
  # This method acts on edge expressions.
  # It will create a polygon for each edge
  # tracing the edge with certain offsets to the edge. "o" is the offset applied to the 
  # outer side of the edge, "i" is the offset applied to the inner side of the edge.
  # "b" is the offset applied at the beginning and "e" is the offset applied at the end.
  
  def extended(*args)
  
    @engine._context("extended") do

      av = [ 0, 0, 0, 0 ]
      args.each_with_index do |a,i|
        if a.is_a?(Hash)
          a[:begin]  && av[0] = @engine._make_value(a[:begin])
          a[:end]    && av[1] = @engine._make_value(a[:end])
          a[:out]    && av[2] = @engine._make_value(a[:out])
          a[:in]     && av[3] = @engine._make_value(a[:in])
          a[:joined] && av[4] = true
        elsif i < 4
          av[i] = @engine._make_value(a).to_s
        else
          raise("Too many arguments for method '#{f}' (1 to 5 expected)")
        end
      end

      DRCOpNodeFilter::new(@engine, self, :new_extended, "extended", *args)

    end
    
  end
  
  # %DRC%
  # @name extended_in
  # @brief Returns polygons describing an area along the edges of the input
  # @synopsis expression.extended_in(d)
  #
  # This method acts on edge expressions. Polygons are generated for 
  # each edge describing the edge drawn with a certain width extending into
  # the "inside" (the right side when looking from start to end).
  # This method is basically equivalent to the \extended method:
  # "extended(0, 0, 0, dist)".
  # A version extending to the outside is \extended_out.
  
  # %DRC%
  # @name extended_out
  # @brief Returns polygons describing an area along the edges of the input
  # @synopsis expression.extended_out(d)
  #
  # This method acts on edge expressions. Polygons are generated for 
  # each edge describing the edge drawn with a certain width extending into
  # the "outside" (the left side when looking from start to end).
  # This method is basically equivalent to the \extended method:
  # "extended(0, 0, dist, 0)".
  # A version extending to the inside is \extended_in.

  def extended_in(e)
    @engine._context("extended_in") do
      DRCOpNodeFilter::new(@engine, self, :new_extended_in, "extended_in", @engine._make_value(e))
    end
  end
  
  def extended_out(e)
    @engine._context("extended_out") do
      DRCOpNodeFilter::new(@engine, self, :new_extended_out, "extended_out", @engine._make_value(e))
    end
  end
  
  # %DRC%
  # @name polygons
  # @brief Converts the input shapes into polygons
  # @synopsis expression.polygons([ enlargement ])
  #
  # Generates polygons from the input shapes. Polygons stay polygons. Edges and edge pairs
  # are converted to polygons. For this, the enlargement parameter will specify the 
  # edge thickness or augmentation applied to edge pairs. With the default enlargement
  # of zero edges will not be converted to valid polygons and degenerated edge pairs
  # will not become valid polygons as well.
  #
  # Contrary most other operations, "polygons" does not have a plain function equivalent
  # as this is reserved for the function generating a polygon layer.
  #
  # This method is useful for generating polygons from DRC violation markers as shown in
  # the following example:
  #
  # @code
  # out = in.drc((width < 0.5.um).polygons)
  # @/code
  
  def polygons(e = 0)
    @engine._context("polygons") do
      DRCOpNodeFilter::new(@engine, self, :new_polygons, "polygons", @engine._make_value(e))
    end
  end
  
  # %DRC%
  # @name covering
  # @brief Selects shapes entirely covering other shapes
  # @synopsis expression.covering(other) (optionally in conditions)
  # @synopsis covering(other) (optionally in conditions)
  #
  # This method represents the selector of primary shapes
  # which entirely cover shapes from the other layer. This version can be put into
  # a condition indicating how many shapes of the other layer need to be covered.
  # Use this variant within \DRC# expressions (also see \Layer#drc).
  #
  # For example, the following statement selects all input shapes which entirely 
  # cover shapes from the "other" layer:
  #
  # @code
  # out = in.drc(covering(other))
  # @/code
  #
  # This example selects all input shapes which entire cover shapes from
  # the other layer and there are more than two shapes from "other" inside
  # primary shapes:
  #
  # @code
  # out = in.drc(covering(other) > 2)
  # @/code

  # %DRC%
  # @name interacting
  # @brief Selects shapes interacting with other shapes
  # @synopsis expression.interacting(other) (optionally in conditions)
  # @synopsis interacting(other) (optionally in conditions)
  #
  # See \covering for a description of the use cases for this function. 
  # When using "interacting", shapes are selected when the interact (overlap, touch)
  # shapes from the other layer.
  # 
  # When using this method with a count, the operation may not render 
  # the correct results if the other input is not merged. By nature of the
  # generic DRC feature, only those shapes that interact with the primary shape
  # will be selected. If the other input is split into multiple polygons,
  # not all components may be captured and the computed interaction count
  # may be incorrect.
  
  # %DRC%
  # @name overlapping
  # @brief Selects shapes overlapping with other shapes
  # @synopsis expression.overlapping(other) (optionally in conditions)
  # @synopsis overlapping(other) (optionally in conditions)
  #
  # See \covering for a description of the use cases for this function. 
  # When using "overlapping", shapes are selected when the overlap
  # shapes from the other layer.
  #
  # When using this method with a count, the operation may not render 
  # the correct results if the other input is not merged. By nature of the
  # generic DRC feature, only those shapes that interact with the primary shape
  # will be selected. If the other input is split into multiple polygons,
  # not all components may be captured and the computed interaction count
  # may be incorrect.
  
  # %DRC%
  # @name inside
  # @brief Selects shapes entirely inside other shapes
  # @synopsis expression.inside(other)
  # @synopsis inside(other)
  #
  # This method represents the selector of primary shapes
  # which are entirely inside shapes from the other layer. 
  # Use this variant within \DRC# expressions (also see \Layer#drc).
  
  # %DRC%
  # @name outside
  # @brief Selects shapes entirely outside other shapes
  # @synopsis expression.outside(other)
  # @synopsis outside(other)
  #
  # This method represents the selector of primary shapes
  # which are entirely outside shapes from the other layer. 
  # Use this variant within \DRC# expressions (also see \Layer#drc).
    
  %w(covering overlapping interacting).each do |f|
    eval <<"CODE"
      def #{f}(other)
        @engine._context("#{f}") do
          other = @engine._make_node(other)
          if ! other.is_a?(DRCOpNode)
            raise("Argument " + other.to_s + " to #{f} must be a DRC expression")
          end
          DRCOpNodeInteractingWithCount::new(@engine, self, other, :#{f})
        end
      end
CODE
  end
  
  %w(inside outside).each do |f|
    eval <<"CODE"
      def #{f}(other)
        @engine._context("#{f}") do
          other = @engine._make_node(other)
          if ! other.is_a?(DRCOpNode)
            raise("Argument " + other.to_s + " to #{f} must be a DRC expression")
          end
          DRCOpNodeInteracting::new(@engine, self, other, :#{f})
        end
      end
CODE
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

class DRCOpNodeJoin < DRCOpNode

  attr_accessor :children
  
  def initialize(engine, cc)
    super(engine)
    self.children = cc
    self.description = "Join"
  end

  def dump(indent)
    return indent + self.description + "\n" + self.children.collect { |c| c.dump("  " + indent) }.join("\n")
  end

  def do_create_node(cache)
    nodes = self.children.collect { |c| c.create_node(cache) }
    if nodes.collect { |n| n.result_type.to_i }.sort.uniq.size > 1
      raise("All inputs to the + operator need to have the same type")
    end
    RBA::CompoundRegionOperationNode::new_join(nodes)
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

    nodes = self.children.collect { |c| c.create_node(cache) }
    types = []
    nodes.each_with_index do |a,index|
      if index % 2 == 1
        types << a.result_type
      end
    end
    if types.uniq { |a,b| a.to_i <=> b.to_i }.size > 1
      raise("All result arguments of 'switch' need to render the same type (got '" + types.map!(&:to_s).join(",") + "')")
    end

    RBA::CompoundRegionOperationNode::new_case(nodes)

  end

end

class DRCOpNodeWithCompare < DRCOpNode

  include DRCComparable
  
  attr_accessor :reverse
  
  def initialize(engine)
    super(engine)
    self.description = "BasicWithCompare"
    self.mode_or = false
    self.mode_or_supported = false
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
      return indent + self.description + " " + cmp.join(" ") + (self.mode_or ? " [or]" : "")
    end
  end

end

class DRCOpNodeCountFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverse
  attr_accessor :method
  attr_accessor :name
  
  def initialize(engine, input, method, name)
    super(engine)
    self.input = input
    self.inverse = false
    self.description = name
    self.method = method
  end

  def _description_for_dump
    self.inverse ? name : "not_" + name
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    args << (self.gt ? @engine._make_numeric_value(self.gt) + 1 : (self.ge ? @engine._make_numeric_value(self.ge) : 0))
    if self.lt || self.le
      args << (self.lt ? @engine._make_numeric_value(self.lt) : @engine._make_numeric_value(self.le) + 1)
    end
    RBA::CompoundRegionOperationNode::send(self.method, *args)
  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end
  
end

class DRCOpNodeAreaFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverse
  attr_accessor :sum
  
  def initialize(engine, input, sum)
    super(engine)
    self.input = input
    self.inverse = false
    self.description = "area"
    self.sum = sum
  end

  def _description_for_dump
    (self.inverse ? "area" : "not_area") + (self.sum ? "_sum" : "")
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    args << (self.gt ? @engine._make_area_value(self.gt) + 1 : (self.ge ? @engine._make_area_value(self.ge) : 0))
    if self.lt || self.le
      args << (self.lt ? @engine._make_area_value(self.lt) : @engine._make_area_value(self.le) + 1)
    end
    RBA::CompoundRegionOperationNode::send(self.sum ? :new_area_sum_filter : :new_area_filter, *args)
  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end
  
end

class DRCOpNodeEdgeLengthFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverse
  attr_accessor :sum
  
  def initialize(engine, input, sum)
    super(engine)
    self.input = input
    self.inverse = false
    self.description = "length"
    self.sum = sum
  end
  
  def _description_for_dump
    (self.inverse ? "length" : "not_length") + (self.sum ? "_sum" : "")
  end
  
  def do_create_node(cache)

    n = self.input.create_node(cache)

    # insert an edge conversion node if required
    if n.result_type != RBA::CompoundRegionOperationNode::ResultType::Edges
      n = RBA::CompoundRegionOperationNode::new_edges(n)
    end

    args = [ n, self.inverse ]
    args << (self.gt ? @engine._make_value(self.gt) + 1 : (self.ge ? @engine._make_value(self.ge) : 0))
    if self.lt || self.le
      args << (self.lt ? @engine._make_value(self.lt) : @engine._make_value(self.le) + 1)
    end

    RBA::CompoundRegionOperationNode::send(self.sum ? :new_edge_length_sum_filter : :new_edge_length_filter, *args)

  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end
  
end

class DRCOpNodeEdgeOrientationFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverse
  attr_accessor :absolute
  
  def initialize(engine, input)
    super(engine)
    self.input = input
    self.inverse = false
    self.description = "angle"
  end
  
  def _description_for_dump
    self.inverse ? "angle" : "not_angle"
  end
  
  def do_create_node(cache)

    n = self.input.create_node(cache)

    # insert an edge conversion node if required
    if n.result_type != RBA::CompoundRegionOperationNode::ResultType::Edges
      n = RBA::CompoundRegionOperationNode::new_edges(n)
    end

    args = [ n, self.inverse ]
    angle_delta = 1e-6
    args << (self.gt ? self.gt + angle_delta : (self.ge ? self.ge : -180.0))
    args << (self.gt ? false : true)
    args << (self.lt ? self.lt : (self.le ? self.le + angle_delta : 180.0))
    args << (self.lt ? false : true)
    args << self.absolute

    RBA::CompoundRegionOperationNode::new_edge_orientation_filter(*args)

  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end
  
end

class DRCOpNodePerimeterFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverse
  attr_accessor :sum
  
  def initialize(engine, input, sum)
    super(engine)
    self.input = input
    self.inverse = false
    self.description = "perimeter"
    self.sum = sum
  end

  def _description_for_dump
    (self.inverse ? "perimeter" : "not_perimeter") + (self.sum ? "_sum" : "")
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    args << (self.gt ? @engine._make_value(self.gt) + 1 : (self.ge ? @engine._make_value(self.ge) : 0))
    if self.lt || self.le
      args << (self.lt ? @engine._make_value(self.lt) : @engine._make_value(self.le) + 1)
    end
    RBA::CompoundRegionOperationNode::send(self.sum ? :new_perimeter_sum_filter : :new_perimeter_filter, *args)
  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end
  
end

class DRCOpNodeInteractingWithCount < DRCOpNodeWithCompare

  attr_accessor :a, :b
  attr_accessor :inverse
  attr_accessor :op
  
  def initialize(engine, a, b, op)
    super(engine)
    self.a = a
    self.b = b
    self.op = op
    self.inverse = false
    self.description = (self.inverse ? "" : "not_") + self.op.to_s
  end
  
  def do_create_node(cache)
    args = [ self.a.create_node(cache), self.b.create_node(cache), self.inverse ]
    args << (self.gt ? self.gt + 1 : (self.ge ? self.ge : 0))
    if self.lt || self.le
      args << (self.lt ? self.lt - 1 : self.le)
    end
    factory = { :covering => :new_enclosing,
                :overlapping => :new_overlapping,
                :interacting => :new_interacting }[self.op]
    RBA::CompoundRegionOperationNode::send(factory, *args)
  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end
  
end

class DRCOpNodeInteracting < DRCOpNode

  attr_accessor :a, :b
  attr_accessor :inverse
  attr_accessor :op
  
  def initialize(engine, a, b, op)
    super(engine)
    self.a = a
    self.b = b
    self.op = op
    self.inverse = false
    self.description = (self.inverse ? "" : "not_") + self.op.to_s
  end
  
  def do_create_node(cache)
    factory = { :inside => :new_inside,
                :outside => :new_outside }[self.op]
    RBA::CompoundRegionOperationNode::send(factory, self.a.create_node(cache), self.b.create_node(cache), self.inverse)
  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
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
    self.mode_or_supported = true
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
                :enclosing => :new_enclosing_check, :enclosed => :new_enclosed_check }[self.check]

    oargs = []
    if self.other
      oargs << self.other.create_node(cache)
    end

    if self.lt || self.le
      dmin = self.le ? @engine._make_value(self.le) + 1 : @engine._make_value(self.lt)
      res = RBA::CompoundRegionOperationNode::send(factory, *(oargs + [ dmin ] + self.args))
    else
      res = nil
    end
      
    if self.gt || self.ge
      if ! res
        dmax = self.ge ? @engine._make_value(self.ge) : @engine._make_value(self.gt) + 1
        res = RBA::CompoundRegionOperationNode::send(factory, *(oargs + [ dmax ] + self.args + [ true ]))
      elsif self.mode_or
        dmax = self.ge ? @engine._make_value(self.ge) : @engine._make_value(self.gt) + 1
        max_check = RBA::CompoundRegionOperationNode::send(factory, *(oargs + [ dmax ] + self.args + [ true ]))
        if self.check == :width || self.check == :notch
          # Same polygon check - we need to take both edges of the result
          other = RBA::CompoundRegionOperationNode::new_edges(res)
        else
          other = RBA::CompoundRegionOperationNode::new_edge_pair_to_first_edges(res)
        end
        res_max = RBA::CompoundRegionOperationNode::new_edge_pair_to_first_edges(max_check)
        res = RBA::CompoundRegionOperationNode::new_join([ other, res_max ])
      else
        dmax = self.ge ? @engine._make_value(self.ge) : @engine._make_value(self.gt) + 1
        max_check_for_not = RBA::CompoundRegionOperationNode::send(factory, *(oargs + [ dmax ] + self.args))
        if self.check == :width || self.check == :notch
          # Same polygon check - we need to take both edges of the result
          other = RBA::CompoundRegionOperationNode::new_edges(res)
        else
          other = RBA::CompoundRegionOperationNode::new_edge_pair_to_first_edges(res)
        end
        res_max_for_not = RBA::CompoundRegionOperationNode::new_edges(max_check_for_not)
        res = RBA::CompoundRegionOperationNode::new_geometrical_boolean(RBA::CompoundRegionOperationNode::GeometricalOp::Not, other, res_max_for_not)
      end
    end
    
    return res

  end

end

class DRCOpNodeBBoxParameterFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :parameter
  attr_accessor :inverse
  
  def initialize(engine, parameter, input)
    super(engine)
    self.parameter = parameter
    self.input = input
    self.inverse = false
    self.description = parameter.to_s
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.parameter, self.inverse ]
    args << (self.gt ? @engine._make_value(self.gt) + 1 : (self.ge ? @engine._make_value(self.ge) : 0))
    if self.lt || self.le
      args << (self.lt ? @engine._make_value(self.lt) : @engine._make_value(self.le) + 1)
    end
    RBA::CompoundRegionOperationNode::new_bbox_filter(*args)
  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end
  
end

class DRCOpNodeRatioParameterFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :parameter
  attr_accessor :inverse
  
  def initialize(engine, parameter, input)
    super(engine)
    self.parameter = parameter
    self.input = input
    self.inverse = false
    self.description = parameter.to_s
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.parameter, self.inverse ]
    args << (self.gt ? self.gt : (self.ge ? self.ge : 0.0))
    args << (self.gt ? false : true)
    if self.lt || self.le
      args << (self.lt ? self.lt : self.le)
      args << (self.lt ? false : true)
    end
    RBA::CompoundRegionOperationNode::new_ratio_filter(*args)
  end

  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end
  
end

class DRCOpNodeCornersFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :output_mode
  attr_accessor :inverse
  attr_accessor :absolute
  
  def initialize(engine, output_mode, absolute, input)
    super(engine)
    self.output_mode = output_mode
    self.input = input
    self.description = "corners"
    self.inverse = false
    self.absolute = absolute
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache) ]
    args << (self.gt ? self.gt : (self.ge ? self.ge : -180.0))
    args << (self.gt ? false : true)
    args << (self.lt ? self.lt : (self.le ? self.le : 180.0))
    args << (self.lt ? false : true)
    if self.output_mode == :dots || self.output_mode == :edges
      args << self.inverse
      args << self.absolute
      RBA::CompoundRegionOperationNode::new_corners_as_dots(*args)
    elsif self.output_mode == :edge_pairs
      args << self.inverse
      args << self.absolute
      RBA::CompoundRegionOperationNode::new_corners_as_edge_pairs(*args)
    else
      args << 2 # dimension is 2x2 DBU
      args << self.inverse
      args << self.absolute
      RBA::CompoundRegionOperationNode::new_corners_as_rectangles(*args)
    end
  end
  
  def inverted
    res = self.dup
    res.inverse = !res.inverse
    return res
  end

end

class DRCOpNodeRelativeExtents < DRCOpNode

  attr_accessor :input
  attr_accessor :as_edges, :fx1, :fx2, :fy1, :fy2, :dx, :dy
  
  def initialize(engine, as_edges, input, fx1, fx2, fy1, fy2, dx = 0, dy = 0)
    super(engine)
    self.input = input
    self.as_edges = as_edges
    self.description = "extents"
    self.fx1 = fx1
    self.fx2 = fx2
    self.fy1 = fy1
    self.fy2 = fy2
    self.dx = dx
    self.dy = dy
  end
  
  def dump(indent)
    if !self.as_edges
      return "extents(%.12g,%.12g,%.12g,%.12g,%12g,%.12g)" % [self.fx1, self.fx2, self.fy1, self.fy2, self.dx, self.dy]
    else
      return "extents_as_edges(%.12g,%.12g,%.12g,%.12g)" % [self.fx1, self.fx2, self.fy1, self.fy2]
    end
  end
      
  def do_create_node(cache)
    node = self.input.create_node(cache)
    if !self.as_edges
      RBA::CompoundRegionOperationNode::new_relative_extents(node, self.fx1, self.fx2, self.fy1, self.fy2, self.dx, self.dy)
    else
      RBA::CompoundRegionOperationNode::new_relative_extents_as_edges(node, self.fx1, self.fx2, self.fy1, self.fy2)
    end
  end

end

end
