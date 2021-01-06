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
# The following documentation will list the methods available for DRC expression objects.
  
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
  # The following example will produce edge markers where the 
  # width of is less then 0.3 micron but not inside polygons on 
  # the "waive" layer:
  #  
  # @code
  # out = in.drc((width < 0.3).edges - secondary(waive))
  # @/code
  
  # %DRC%
  # @name ^
  # @brief Boolean XOR between the results of two expressions
  # @synopsis expression - expression
  #
  # The ^ operator will compute the XOR (symmetric difference) between the results
  # of two expressions. The XOR operation is defined for polygons and edges.
  # Both expressions must be of the same type.

  %w(& - ^ | +).each do |f|
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
        empty = RBA::CompoundRegionOperationNode::new_empty(RBA::CompoundRegionOperationNode::ResultType::Region)
        DRCOpNodeCase::new(@engine, [ self, DRCOpNode::new(@engine, empty), @engine.primary ])
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
    DRCOpNodeAreaFilter::new(@engine, self)
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
    DRCOpNodeCountFilter::new(@engine, self)
  end
  
  # %DRC%
  # @name perimeter
  # @brief Selects the primary shape if the perimeter is meeting the condition
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
    DRCOpNodePerimeterFilter::new(@engine, self)
  end
  
  # %DRC%
  # @name bbox_min
  # @brief Selects the primary shape if its bounding box smaller dimension is meeting the condition
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
  # The "bbox_min" method is available as a plain function or as a method on \DRC## expressions.
  # The plain function is equivalent to "primary.bbox_min".
  
  def bbox_min
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::BoxMinDim, self)
  end
  
  # %DRC%
  # @name bbox_max
  # @brief Selects the primary shape if its bounding box larger dimension is meeting the condition
  # @synopsis expression.bbox_max (in condition)
  # 
  # This operation acts similar to \DRC#bbox_min, but takes the larger dimension of the shape's 
  # bounding box.
  #
  # The "bbox_max" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.bbox_max".

  def bbox_max
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::BoxMaxDim, self)
  end
  
  # %DRC%
  # @name bbox_width
  # @brief Selects the primary shape if its bounding box width is meeting the condition
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
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::BoxWidth, self)
  end
  
  # %DRC%
  # @name bbox_height
  # @brief Selects the primary shape if its bounding box height is meeting the condition
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
    DRCOpNodeBBoxParameterFilter::new(@engine, RBA::CompoundRegionOperationNode::BoxHeight, self)
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
    DRCOpNodeEdgeLengthFilter::new(@engine, self)
  end
  
  # %DRC%
  # @name angle
  # @brief Selects edges based on their angle
  # @synopsis expression.angle (in condition)
  # 
  # This operation selects edges by their angle, measured against the horizontal 
  # axis in the mathematical sense. 
  #
  # For this measurement edges are considered without their direction and straight lines. 
  # A horizontal edge has an angle of zero degree. A vertical one has
  # an angle of 90 degee. The angle range is from -90 (exclusive) to 90 degree (inclusive).
  #
  # If the input shapes are not polygons or edge pairs, they are converted to edges 
  # before the angle test is made.
  # 
  # For example, the following code selects all edges from the primary shape which are 45 degree
  # (up) or 135 degree (down). The "+" will join the results:
  #
  # @code
  # out = in.drc((angle == 45) + (angle == 135)) 
  # out = in.drc((primary.angle == 45) + (primary.angle == 135))    # equivalent
  # @/code
  #
  # Note that angle checks usually imply the need to rotation variant formation as cells which
  # are placed unrotated and rotated by 90 degree cannot be considered identical. This imposes
  # a performance penalty in hierarchical mode. If possible, consider using \DRC#rectilinear for
  # example to detect shapes with non-manhattan geometry instead of using angle checks.
  #
  # The "angle" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.angle".

  def angle
    DRCOpNodeEdgeOrientationFilter::new(@engine, self)
  end
  
  # %DRC%
  # @name rounded_corners
  # @brief Applies corner rounding
  # @synopsis expression.rounded_corners(inner, outer, n)
  #
  # This operation acts on polygons and applies corner rounding the the given inner
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
  # @synopsis expression.smoothed(d)
  #
  # This operation acts on polygons and applies polygon smoothing with the tolerance d. See \Layer#smoothed for more details.
  #
  # The "smoothed" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.smoothed".

  def smoothed(d)
    @engine._context("smoothed") do
      DRCOpNodeFilter::new(@engine, self, :new_smoothed, "smoothed", @engine._make_value(d))
    end
  end
  
  # %DRC%
  # @name corners (in condition)
  # @brief Applies smoothing
  # @synopsis expression.corners
  # @synopsis expression.corners(as_dots)
  # @synopsis expression.corners(as_boxes)
  #
  # This operation acts on polygons and selects the corners of the polygons.
  # It can be put into a condition to select corners by their angles. The angle of
  # a corner is positive for a turn to the left if walking a polygon counterclockwise
  # and negative for the turn to the right. Angles take values between -180 and 180 degree.
  #
  # When using "as_dots" for the argument, the operation will return single-point edges at
  # the selected corners. With "as_boxes" (the default), small (2x2 DBU) rectangles will be
  # produced at each selected corner.
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
  # out = in.drc(corners < 0)
  # out = in.drc(primary.corners < 0)    # equivalent
  # @/code
  #
  # The "corners" method is available as a plain function or as a method on \DRC# expressions.
  # The plain function is equivalent to "primary.corners".

  def corners(as_dots = DRCAsDots::new(false))
    @engine._context("corners") do
      if as_dots.is_a?(DRCAsDots)
        as_dots = as_dots.value
      else
        raise("Invalid argument (#{as_dots.inspect}) for 'corners' method")
      end
      DRCOpNodeCornersFilter::new(@engine, self, as_dots)
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
  # @brief Returns partial references to the boundings boxes of the polygons
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
  
  # %DRC%
  # @name odd_polygons
  # @brief Selects all polygons which are non-orientable
  # @synopsis expression.odd_polygons
  #
  # Non-orientable polygons are for example "8"-shape polygons. Such polygons are
  # usually considered harmful as their definition of covered area is depending on the
  # wrap count rule in place.
  #
  # This operation can be used as a plain function in which case it acts on primary
  # shapes or can be used as method on another DRC expression.
  
  def odd_polygons
    return DRCOpNodeFilter::new(@engine, self, :new_strange_polygons_filter, "odd_polygon")
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
    return DRCOpNodeFilter::new(@engine, self, :new_rectangle_filter, "rectangle")
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
  
  def edges
    return DRCOpNodeFilter::new(@engine, self, :new_edges, "edges")
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
  # @synopsis expression.extents([ enlargement ])
  # 
  # This method acts on edge pair expressions and returns the first edges of the
  # edge pairs delivered by the expression.

  # %DRC%
  # @name second_edges
  # @brief Returns the second edges of edge pairs
  # @synopsis expression.extents([ enlargement ])
  # 
  # This method acts on edge pair expressions and returns the second edges of the
  # edge pairs delivered by the expression.

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
      DRCOpNodeFilter::new(@engine, self, :new_extended_in, "extended_in", self._make_value(e))
    end
  end
  
  def extended_out(e)
    @engine._context("extended_out") do
      DRCOpNodeFilter::new(@engine, self, :new_extended_out, "extended_out", self._make_value(e))
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
      DRCOpNodeFilter::new(@engine, self, :new_polygons, "polygons", self._make_value(e))
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

class DRCOpNodeJoin < DRCOpNode

  attr_accessor :children
  
  def initialize(engine, op, a, b)
    super(engine)
    self.children = [a, b]
    self.description = "Join #{op.to_s}"
  end

  def dump(indent)
    return indent + self.description + "\n" + self.children.collect { |c| c.dump("  " + indent) }.join("\n")
  end

  def do_create_node(cache)
    nodes = self.children.collect { |c| c.create_node(cache) }
    if nodes.collect(:result_type).sort.uniq.size > 1
      raise("All inputs to the + operator need to have the same type")
    end
    RBA::CompoundRegionOperationNode::new_join(*nodes)
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
    [ DRCOpNodeWithCompare::new(@engine, self, true), something ]
  end
  
  def _self_or_original
    return (self.original || self).dup
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

class DRCOpNodeCountFilter < DRCOpNodeWithCompare

  attr_accessor :input
  attr_accessor :inverted
  
  def initialize(engine, input)
    super(engine)
    self.input = input
    self.inverted = false
    self.description = "count"
  end

  def _description_for_dump
    self.inverted ? "count" : "not_count"
  end
  
  def do_create_node(cache)
    args = [ self.input.create_node(cache), self.inverse ]
    args << (self.gt ? @engine._make_numeric_value(self.gt) + 1 : (self.ge ? @engine._make_numeric_value(self.ge) : 0))
    if self.lt || self.le
      args << self.lt ? @engine._make_numeric_value(self.lt) : @engine._make_numeric_value(self.le) - 1
    end
    RBA::CompoundRegionOperationNode::new_count_filter(*args)
  end

  def inverted
    res = self.dup
    res.inverted = !res.inverted
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
    args << (self.gt ? @engine._make_area_value(self.gt) + 1 : (self.ge ? @engine._make_area_value(self.ge) : 0))
    if self.lt || self.le
      args << self.lt ? @engine._make_area_value(self.lt) : @engine._make_area_value(self.le) - 1
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

    n = self.input.create_node(cache)

    # insert an edge conversion node if required
    if n.result_type != RBA::CompoundRegionOperationNode::ResultType::Edges
      n = RBA::CompoundRegionOperationNode::new_edges(n)
    end

    args = [ n, self.inverse ]
    args << (self.gt ? @engine._make_value(self.gt) + 1 : (self.ge ? @engine._make_value(self.ge) : 0))
    if self.lt || self.le
      args << self.lt ? @engine._make_value(self.lt) : @engine._make_value(self.le) - 1
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

    n = self.input.create_node(cache)

    # insert an edge conversion node if required
    if n.result_type != RBA::CompoundRegionOperationNode::ResultType::Edges
      n = RBA::CompoundRegionOperationNode::new_edges(n)
    end

    args = [ n, self.inverse ]
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
    args << (self.gt ? @engine._make_value(self.gt) + 1 : (self.ge ? @engine._make_value(self.ge) : 0))
    if self.lt || self.le
      args << self.lt ? @engine._make_value(self.lt) : @engine._make_value(self.le) - 1
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
      dmax = self.ge ? @engine._make_value(self.ge) : @engine._make_value(self.gt) + 1
      max_check = RBA::CompoundRegionOperationNode::send(factory, *(oargs + [ dmax ] + self.args + [ true ]))
      if res
        if self.check == :width || self.check == :notch
          # Same polygon check - we need to take both edges of the result
          and_with = RBA::CompoundRegionOperationNode::new_edges(res)
        else
          and_with = RBA::CompoundRegionOperationNode::new_edge_pair_to_first_edges(res)
        end
        res_max = RBA::CompoundRegionOperationNode::new_edge_pair_to_first_edges(max_check)
        res = RBA::CompoundRegionOperationNode::new_geometrical_boolean(RBA::CompoundRegionOperationNode::GeometricalOp::And, and_with, res_max)
      else
        res = max_check
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
    args << (self.gt ? @engine._make_value(self.gt) + 1 : (self.ge ? @engine._make_value(self.ge) : 0))
    if self.lt || self.le
      args << self.lt ? @engine._make_value(self.lt) : @engine._make_value(self.le) - 1
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
