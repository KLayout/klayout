# $autorun-early

module DRC

  # %DRC%
  # @scope
  # @name Layer

  class DRCLayer

    # %DRC%
    # @name drc
    # @brief Provides a generic DRC function for use with \DRC# expressions
    # @synopsis layer.drc(expression)
    #
    # This method implement the universal DRC which offers enhanced abilities,
    # improved performance in some applications and better readability.
    #
    # The key concept for this method are DRC expressions. DRC expressions
    # are formed by using predefined keywords like "width", operators like "&" 
    # and method to build an abstract definition of the operations to perform
    # within the DRC.
    #
    # When the DRC function is executed, it will basically visit all shapes
    # from the input layer (the one, the "drc" method is called on), collect
    # the neighbor shapes from all involved other inputs and run the requested
    # operations on each cluster. Currently, "drc" is only available for polygon
    # layers.
    #
    # The nature of the "drc" operation is that of the loop over all (merged) input
    # polygons. Within the operation executed on each shape, it's possible to make
    # decisions such as "if the shape has an area larger than something, apply this
    # operation" etc. This can be achieved with conventional DRC functions too,
    # but involves potentially complex and heavy operations such as booleans, interact
    # etc. For this reason, the "drc" function may provide a performance benefit.
    #
    # In addition, within the loop, a single shape from the input layer is presented to
    # execution engine which runs the operations.
    # This allows using operations such as "size" without having to consider 
    # neigbor polygons growing into the area of the initial shape. In this sense,
    # the "drc" function allows seeing the layer as individual polygons rather than
    # a global "sea of polygons". This enables new applications which are otherwise
    # difficult to implement.
    #
    # An important concept in the context of "drc" expressions is the "primary".
    # This expression represents a single primary shape. "Secondaries" are shapes
    # from other inputs. Primary shapes guide the operation - secondaries without
    # primaries are not seen. The "drc" operation will look for secondaries within
    # a certain distance which is determined from the operations within the 
    # expression to execute. The secondaries collected in this step will not be
    # merged, so the secondary polygons may be partial. This is important when
    # using measurement operations like "area" on secondary polygons.
    #
    # @h3 Checks @/h3
    #
    # Here is an example for a generic DRC operation which performs a width
    # check for less than 0.5.um on the primary shapes. It uses the \global#width operator:
    #
    # @code
    # out = in.drc(width < 0.5.um)
    # @/code
    #
    # Other single or double-bounded conditions are available too, for example:
    #
    # @code
    # out = in.drc(width <= 0.5.um)
    # out = in.drc(width > 0.5.um)
    # out = in.drc(width == 0.5.um)
    # out = in.drc(width != 0.5.um)
    # out = in.drc(0.2.um < width < 0.5.um)
    # @/code
    #
    # To specify the second input for a two-layer check, specify the second input
    # with the check function. Here a two-layer separation check is used (\global#separation):
    #
    # @code
    # l1 = input(1, 0)
    # l2 = input(2, 0)
    # out = l1.drc(separation(l2) < 0.5.um)
    # @/code
    #
    # The second input of this check function can be a computed expression. In this
    # case the local loop will first evaluate the expression for the second input and
    # then use it inside the check.  
    #
    # Options for the checks are also specified inside the brackets. For example,
    # to specify a projection metrics for width use:
    #
    # @code 
    # out = in.drc(width(projection) < 0.5.um)
    # @/code
    #
    #
    # @h3 Edges and edge pairs @/h3
    #
    # Although the "drc" function operates on polygon layers, internally it is 
    # able to handle edge and edge pair types too. Some operations generate edge pairs,
    # some other generate edges. As results from one operation can be processed further
    # in the DRC expressions, methods are available to filter, process and convert
    # these types.
    #
    # For example, the check produces edge pairs which can be converted into polygons
    # using the "polygons" method:
    #
    # @code
    # out = in.drc((width(projection) < 0.5.um).polygons)
    # @/code
    # 
    # Note the subtle difference: when putting the "polygons" method inside the "drc"
    # brackets, it is executed locally on every checked primary polygon. The result
    # may be identical to the global conversion:
    #
    # @code
    # # same, but with "global" conversion:
    # out = in.drc(width(projection) < 0.5.um).polygons
    # @/code
    #
    # but having the check polygons inside the loop opens new opportunities and 
    # is more efficient in general.
    #
    # Conversion methods are:
    #
    # @ul
    # @li \DRC#polygons: converts edge pairs to polygons @/li
    # @li \DRC#extended, \DRC#extended_in, \DRC#extended_out: converts edges to polygons @/li
    # @li \DRC#first_edges, \DRC#second_edges: extracts edges from edge pairs @/li
    # @li \DRC#edges: decomposes edge pairs and polygons into edges @/li
    # @li \DRC#corners: can extract corners from polygons @/li
    # @/ul
    #
    # This example decomposes the primary polygons into edges:
    # 
    # @code
    # out = in.drc(primary.edges)
    # @/code
    #
    # (for backward compatibility you cannot abbreviate "primary.edges" simply as "edges" like
    # other functions). 
    #
    # The previous isn't quite exciting as it is equivalent to 
    #
    # @code
    # # Same as above
    # out = in.edges
    # @/code
    #
    # But it gets more interesting as within the loop, "edges" delivers the edge set for
    # each individual polygon. So this will give you the edges of polygons with more than four corners:
    #
    # @code
    # out = in.drc(primary.edges.count > 4)
    # @/code
    # 
    # The same result can be achieved with classic DRC with "interact" and a figure count, but
    # at a much higher computation cost.
    #
    # @h3 Edge and edge/polygon operations @/h3
    #
    # The "drc" framework supports the following edge and edge/polygon operations:
    #
    # @ul
    # @li Edge vs. edge and edge vs. polygon booleans @/li
    # @li Edge vs. polygon interactions (\DRC#interacting, \DRC#overlapping) @/li
    # @li Edge sampling (\DRC#start_segments, \DRC#centers, \DRC#end_segments) @/li
    # @/ul
    #
    # @h3 Filters @/h3
    #
    # Filter operators select input polygons or edges based on their properties. These filters are:
    #
    # @ul
    # @li "\DRC#area": selects polygons based on their area @/li
    # @li "\DRC#perimeter": selects polygons based on their perimeter @/li
    # @li "\DRC#bbox_min", "\global#bbox_max", "\global#bbox_width", "\global#bbox_height": selects polygons based on their bounding box properties @/li
    # @li "\DRC#length": selects edges based on their length @/li
    # @li "\DRC#angle": selects edges based on their orientation @/li
    # @/ul
    # 
    # For example, to select polygons with an area larger than one square micrometer, use:
    #
    # @code
    # out = in.drc(area > 1.0)
    # @/code
    #
    # For the condition, use the usual numerical bounds like:
    #
    # @code
    # out = in.drc(area == 1.0)
    # out = in.drc(area <= 1.0)
    # out = in.drc(0.2 < area < 1.0)
    # @/code
    #
    # The result of the area operation is the input polygon if the area condition is met.
    # 
    # In the same fashion, "perimeter" applies to the perimeter of the polygon.
    # "bbox_min" etc. will evaluate a particular dimensions of the polygon's bounding box and
    # use the respective dimension for filtering the polygon.
    #
    # Note that it's basically possible to use the polygon filters on any kind of input.
    # In fact, plain "area" for example is a shortcut for "\global#primary.area" indicating that
    # the area of primary shapes are supposed to be computed.
    # However, any input other than the primary is not necessarily complete or it may 
    # consist of multiple polygons. Hence the computed values may be too big or too small.
    # It's recommended therefore to use the measurement functions on primary polygons
    # only.
    #
    # @h3 Filter predicates @/h3
    #
    # The "drc" feature also supports some predicates. "predicates" are boolean values
    # indicating a certain condition. A predicate filter works in a way that it only
    # passes the polygons 
    # The predicates available currently are:
    #
    # @ul
    # @li "\global#rectangles": Filters rectangles @/li
    # @li "\global#squares": Filters squares @/li
    # @li "\global#rectilinear": Filters rectilinear ("Manhattan") polygons @/li
    # @/ul
    #
    # For the same reason as explained above, it's recommended to use these predicates
    # standalone, so they act on primary shapes. It's possible to use the predicates
    # on computed shapes or secondary input, but that may not render the desired results.
    #
    # @h3 Logical NOT operator @/h3
    #
    # The "!" operator will evaluate the expression behind it and return the 
    # current primary shape if the input is empty and return an empty polygon set 
    # if not. Hence the following filter will deliver all polygons which are 
    # not rectangles:
    #
    # @code
    # out = in.drc(! rectangles)
    # @/code
    #
    # @h3 Logical combination operators @/h3
    #
    # The logical "if_any" or "if_all" statements allow connecting multiple
    # conditions and evaluate to "true" (means: a non-empty shape set) if either
    # on input is a non-empty shape set ("if_any") or if all inputs are non-empty
    # ("if_all"). For example, this will select all polygons which are rectangles
    # and whose area is larger than 20 quare micrometers:
    #
    # @code
    # out = in.drc(if_all(rectangles, area > 20.0))
    # @/code
    #
    # In fact, "if_all" renders the result of the last expression, provided all
    # previous ones are non-empty. So this operation will render rectangles
    # sized by 100 nm and skip all other types of polygons:
    #
    # @code
    # out = in.drc(if_all(rectangles, sized(100.nm)))
    # @/code
    #
    # Contrary to this, the "if_any" operation will render the first non-empty
    # expression result and skip the following ones. So this example will
    # size all rectangles by 100 nm and leave all other types of polygons
    # untouched:
    #
    # @code
    # out = in.drc(if_any(rectangles.sized(100.nm), primary))
    # @/code
    #
    # @h3 Polygon manipulations @/h3
    #
    # The "drc" operations feature polygon manipulations where the input is
    # either the primary polygon or derived shapes.
    # Manipulations include sizing ("\global#sized"), corner rounding ("\global#rounded_corners"), smoothing ("\global#smoothed")
    # and boolean operations.
    #
    # This example computes a boolean AND between two layers before selecting
    # the result polygons with an area larger than 1 square micrometer. Note that
    # "primary" is a placeholder for the primary shape:
    #
    # @code
    # l1 = input(1, 0)
    # l2 = input(2, 0)
    # out = l1.drc((primary & l2).area > 1.0)
    # @/code
    #
    # This example demonstrates how the "drc" operation can improve performance: as the
    # boolean operation is computed locally and the result is discarded when no longer required,
    # less shapes need to be stored hence reducing the memory overhead and CPU time required
    # to manage these shapes.
    #
    # Note that the precise form of the example above is
    #
    # @code
    # out = l1.drc((primary & secondary(l2)).area > 1.0)
    # @/code
    #
    # The "\global#secondar" operator indicates that "l2" is to be used as secondary input to the "drc" function. Only
    # in this form, the operators of the boolean AND can be reversed:
    # 
    # @code
    # out = l1.drc((secondary(l2) & primary).area > 1.0)
    # @/code
    #
    # @h3 Quantifiers @/h3
    #
    # Some filters operate on properties of the full, local shape set. 
    # While the loop is executed, the DRC expressions will collect shapes, either
    # from the primary, it's neighborhood (secondary) or by deriving shape sets.
    #
    # Obviously the primary is a simple one: it consists of a single shape, because
    # this is how the loop operates. Derived shape sets however can be more complex.
    # "Quantifiers" allow to assess properties of the complete, per-primary shape
    # set. A simple one is "count" which checks if the number of shapes within
    # a shape set is within a given range.
    #
    # Obviously, "primary.count == 1" is always true. The following condition will
    # select all primary shapes which have more than 13 corners:
    #
    # @code
    # out = in.drc(if_any(primary.corners.count > 13))
    # @/code
    #
    # Note an important detail here: the "if_any" function will render primary
    # @b polygons @/b, if the expression inside gives a non-empty result. Without
    # "if_any", the result would be the output of "count" which is the set of all
    # corners where the corner count is larger than 13.
    #
    # @h3 Expressions as objects @/h3
    #
    # The expression inside the "drc" function is a Ruby object and can be 
    # stored in variables. If you need the same expression multiple times, it can be 
    # more efficient to use the same Ruby object. In this example, the same expression
    # is used two times. Hence it's computed two times:
    #
    # @code
    # out = l1.drc(((primary & l2).area == 1.0) + ((primary & l2).area == 2.0))
    # @/code
    #
    # A more efficient version is:
    #
    # @code
    # overlap_area = (primary & l2).area
    # out = l1.drc((overlap_area == 1.0) + (overlap_area == 2.0))
    # @/code
    #
    # Note that the first line prepares the operation, but does not execute the area computation
    # or the boolean operation. But when the "drc" function executes the operation it will 
    # only compute the area once as it is represented by the same Ruby object.
    #
    # @h3 Summary @/h3
    #
    # The bottom line is: DRC expressions are quite rich and there is a lot more to be said and written.
    # More formal details about the bits and pieces can be found in the \DRC# class documentation.

    def drc(op)
      @engine._context("drc") do
        requires_region
        op.is_a?(DRCOpNode) || raise("A DRC expression is required for the argument (got #{op.inspect})")
        node = op.create_node({})
        result_cls = nil
        if node.result_type == RBA::CompoundRegionOperationNode::ResultType::Region
          result_cls = RBA::Region
        elsif node.result_type == RBA::CompoundRegionOperationNode::ResultType::Edges
          result_cls = RBA::Edges
        elsif node.result_type == RBA::CompoundRegionOperationNode::ResultType::EdgePairs
          result_cls = RBA::EdgePairs
        end
        if result_cls
          DRCLayer::new(@engine, @engine._tcmd(self.data, node.distance, result_cls, :complex_op, node))
        end
      end
    end
    
  end

  # %DRC%
  # @scope 
  # @name global 

  class DRCEngine

    # %DRC%
    # @name case
    # @brief A conditional selector for the "drc" universal DRC function
    # @synopsis case(...)
    #
    # This function provides a conditional selector for the "drc" function.
    # It is used this way:
    #
    # @code
    #   out = in.drc(case(c1, r1, c2, r2, ..., cn, rn)
    #   out = in.drc(case(c1, r1, c2, r2, ..., cn, rn, rdef)
    # @/code
    # 
    # This function will evaluate c1 which is a universal DRC expression (see \Layer#drc).
    # If the result is not empty, "case" will evaluate and return r1. Otherwise it
    # will continue with c2 and the result of this expression is not empty it will
    # return r2. Otherwise it will continue with c3/r3 etc. 
    #
    # If an odd number of arguments is given, the last expression is evaluated if
    # none of the conditions c1..cn gives a non-empty result.
    #
    # As a requirement, the result types of all r1..rn expressions and the rdef
    # needs to be the same - i.e. all need to render polygons or edges or edge pairs.

    def case(*args)

      self._context("case") do

        anum = 1

        args = args.collect { |a| self._make_node(a) }

        types = []
        args.each do |a|
          if !a.is_a?(DRCOpNode)
            raise("All inputs need to be valid compound operation expressions (argument ##{anum} isn't)")
          end
          if a % 2 == 0
            types << a.result_type
          end
          anum += 1
        end

        if types.sort.uniq.size > 1
          raise("All result arguments need to have the same type (we got '" + types.collect(:to_s).join(",") + "')")
        end

        DRCOpNodeCase::new(self, args)

      end

    end
    
    # %DRC%
    # @name secondary
    # @brief Provides secondary input for the "drc" universal DRC function
    # @synopsis secondary(layer)
    #
    # To supply additional input for the universal DRC expressions (see \Layer#drc), use
    # "secondary" with a layer argument. This example provides a boolean AND
    # between l1 and l2:
    #
    # @code
    #   l1 = layer(1, 0)
    #   l2 = layer(2, 0)
    #   out = l1.drc(primary & secondary(l2))
    # @/code
     
    def secondary(layer)

      self._context("secondary") do

        layer.requires_region

        res = DRCOpNode::new(self, RBA::CompoundRegionOperationNode::new_secondary(layer.data))
        res.description = "secondary"
        return res

      end

    end
    
    # %DRC%
    # @name primary
    # @brief Represents the primary input of the universal DRC function
    # @synopsis primary
    #
    # The primary input of the universal DRC function is the layer the \Layer#drc function
    # is called on.
    
    def primary
      res = DRCOpNode::new(self, RBA::CompoundRegionOperationNode::new_primary)
      res.description = "primary"
      return res
    end
    
    # %DRC%
    # @name foreign
    # @brief Represents all other polygons from primary except the current one
    # @synopsis foreign
    #
    # The primary input of the universal DRC function is the layer the \Layer#drc function
    # is called on. This operation represents all "other" primary polygons while
    # \primary represents the current polygon.
    #
    # This feature opens new options for processing layouts beyond the 
    # abilities of the classical DRC concept. For classic DRC, intra-layer interactions
    # are always symmetric: a polygon cannot be considered separated from it's neighbors
    # on the same layer.
    # 
    # The following example computes every part of the input which is closer than
    # 0.5 micrometers to other (disconnected) polygons on the same layer:
    #
    # @code
    # out = in.drc(primary & foreign.sized(0.5.um))
    # @/code
    
    def foreign
      res = DRCOpNode::new(self, RBA::CompoundRegionOperationNode::new_foreign)
      res.description = "foreign"
      return res
    end
    
    # %DRC%
    # @name if_all
    # @brief Evaluates to the primary shape when all condition expression results are non-empty
    # @synopsis if_all(c1, ... cn)
    #
    # This function will evaluate the conditions c1 to cn and return the 
    # current primary shape if all conditions render a non-empty result.
    # The following example selects all shapes which are rectangles and 
    # whose area is larger than 0.5 square micrometers:
    # 
    # @code
    # out = in.drc(if_all(area > 0.5, rectangle))
    # @/code
    #
    # The condition expressions may be of any type (edges, edge pairs and polygons).
     
    # %DRC%
    # @name if_any
    # @brief Evaluates to the primary shape when any condition expression results is non-empty
    # @synopsis if_any(c1, ... cn)
    # 
    # This function will evaluate the conditions c1 to cn and return the 
    # current primary shape if at least one condition renders a non-empty result.
    # See \if_all for an example how to use the if_... functions.
      
    # %DRC%
    # @name if_none
    # @brief Evaluates to the primary shape when all of the condition expression results are empty
    # @synopsis if_none(c1, ... cn)
    # 
    # This function will evaluate the conditions c1 to cn and return the 
    # current primary shape if all conditions renders an empty result.
    # See \if_all for an example how to use the if_... functions.

    %w(
      if_all 
      if_any
      if_none
    ).each do |f|
      eval <<"CODE"
        def #{f}(*args)

          self._context("#{f}") do

            args = args.collect { |a| self._make_node(a) }

            args.each_with_index do |a,ia|
              if ! a.is_a?(DRCOpNode)
                raise("Argument #" + (ia + 1).to_s + " to #{f} must be a DRC expression")
              end
            end
            res = DRCOpNodeLogicalBool::new(self, :#{f})
            res.children = args
            res

          end

        end
CODE
    end
    
    # %DRC%
    # @name bbox_height
    # @brief Selects primary shapes based on their bounding box height
    # @synopsis bbox_height (in condition)
    #
    # This method creates a universal DRC expression (see \Layer#drc) to select primary shapes whose
    # bounding box height satisfies the condition. Conditions may be written as arithmetic comparisons
    # against numeric values. For example, "bbox_height < 2.0" will select all primary shapes whose
    # bounding box height is less than 2 micrometers. See \Layer#drc for more details about comparison 
    # specs. Plain "bbox_min" is equivalent to "primary.bbox_min" - i.e. it is used on the primary
    # shape. Also see \DRC#bbox_min.
    
    # %DRC%
    # @name bbox_width
    # @brief Selects primary shapes based on their bounding box width
    # @synopsis bbox_max (in condition)
    #
    # See \Layer#drc, \bbox_height and \DRC#bbox_height for more details.
    
    # %DRC%
    # @name bbox_max
    # @brief Selects primary shapes based on their bounding box height or width, whichever is larger
    # @synopsis bbox_max (in condition)
    #
    # See \Layer#drc, \bbox_max and \DRC#bbox_max for more details.
    
    # %DRC%
    # @name bbox_min
    # @brief Selects primary shapes based on their bounding box height or width, whichever is smaller
    # @synopsis bbox_max (in condition)
    #
    # See \Layer#drc, \bbox_min and \DRC#bbox_min for more details.
    
    # %DRC%
    # @name bbox_aspect_ratio
    # @brief Selects primary shapes based on the aspect ratio of their bounding boxes
    # @synopsis bbox_aspect_ratio (in condition)
    #
    # See \Layer#drc, \bbox_aspect_ratio and \DRC#bbox_aspect_ratio for more details.
    
    # %DRC%
    # @name relative_height
    # @brief Selects primary shapes based on the ratio of height and width of their bounding boxes
    # @synopsis relative_height (in condition)
    #
    # See \Layer#drc, \relative_height and \DRC#relative_height for more details.
    
    # %DRC%
    # @name area_ratio
    # @brief Selects primary shapes based on the ratio of bounding box and polygon area
    # @synopsis area_ratio (in condition)
    #
    # See \Layer#drc, \area_ratio and \DRC#area_ratio for more details.
    
    %w(
      bbox_height
      bbox_max
      bbox_min
      bbox_width
      bbox_aspect_ratio
      area_ratio
      relative_height
    ).each do |f|
      eval <<"CODE"
        def #{f}
          self._context("#{f}") do
            primary.#{f}
          end
        end
CODE
    end
    
    # %DRC%
    # @name area
    # @brief Computes the total area or in universal DRC context: selects the primary shape if the area is meeting the condition
    # @synopsis area (in condition)
    # @synopsis area(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.area" (see \Layer#area) and returns the total area of the 
    # polygons in the layer. 
    #
    # Without a layer argument, "area" represents an area filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#area for more details).
    
    # %DRC%
    # @name hulls
    # @brief Selects all hulls from the input polygons
    # @synopsis hulls
    # @synopsis hulls(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.hulls" (see \Layer#hulls). Without a layer
    # argument, "hulls" represents a hull contour extractor for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#hulls for more details).
    
    # %DRC%
    # @name holes
    # @brief Selects all holes from the input polygons
    # @synopsis holes
    # @synopsis holes(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.holes" (see \Layer#hulls). Without a layer
    # argument, "holes" represents a hole extractor for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#hulls for more details).
    
    # %DRC%
    # @name odd_polygons
    # @brief Selects all polygons which are non-orientable
    # @synopsis odd_polygons
    # @synopsis odd_polygons(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.odd_polygons" (see \Layer#odd_polygons). Without a layer
    # argument, "odd_polygons" represents an odd polygon filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#odd_polygons for more details).

    # %DRC%
    # @name perimeter
    # @brief Computes the total perimeter or in universal DRC context: selects the primary shape if the perimeter is meeting the condition
    # @synopsis perimeter (in condition)
    # @synopsis perimeter(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.perimeter" (see \Layer#perimeter) and returns the
    # total perimeter of all polygons in the layer.
    #
    # Without a layer argument, "perimeter" represents a perimeter filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#perimeter for more details).

    # %DRC%
    # @name rectangles
    # @brief Selects all polygons which are rectangles
    # @synopsis rectangles
    # @synopsis rectangles(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.rectangles" (see \Layer#rectangles). Without a layer
    # argument, "rectangles" represents the rectangles filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#rectangles for more details).

    # %DRC%
    # @name squares
    # @brief Selects all polygons which are squares
    # @synopsis squares
    # @synopsis squares(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.squares" (see \Layer#squares). Without a layer
    # argument, "squares" represents the rectangles filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#squares for more details).

    # %DRC%
    # @name rectilinear
    # @brief Selects all polygons which are rectilinear
    # @synopsis rectilinear
    # @synopsis rectilinear(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.rectilinear" (see \Layer#rectilinear). Without a layer
    # argument, "rectilinear" represents the rectilinear polygons filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#rectilinear for more details).

    # %DRC%
    # @name length (in condition)
    # @brief Computes the total edge length of an edge layer or in universal DRC context: selects edges based on a length condition
    # @synopsis length (in condition)
    # @synopsis length(layer)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.length" (see \Layer#length). Without a layer
    # argument, "length" represents the edge length filter on the primary shape edges in 
    # \DRC# expressions (see \Layer#drc and \DRC#length for more details). In this context,
    # the operation acts similar to \Layer#with_length.

    # %DRC%
    # @name angle (in condition)
    # @brief In universal DRC context: selects edges based on their orientation
    # @synopsis angle (in condition)
    #
    # "angle" represents the edge orientation filter on the primary shape edges in
    # \DRC# expressions (see \Layer#drc and \DRC#angle for more details). In this context,
    # the operation acts similar to \Layer#with_angle.

    %w(
      angle
      area
      holes
      hulls
      length
      odd_polygons
      perimeter
      rectangles
      rectilinear
      squares
    ).each do |f|
      # NOTE: these methods are fallback for the respective global ones which route to DRCLayer or here.
      eval <<"CODE"
        def _cop_#{f}
          primary.#{f}
        end
CODE
    end
    
    # %DRC%
    # @name corners
    # @brief Selects all polygons which are rectilinear
    # @synopsis corners([ options ]) (in condition)
    # @synopsis corners(layer [, options ])
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.corners" (see \Layer#corners). Without a layer
    # argument, "corners" represents the corner generator/filter in primary shapes for
    # \DRC# expressions (see \Layer#drc and \DRC#corners for more details).
    #
    # Like the layer-based version, the "corners" operator accepts the 
    # output type option: "as_dots" for dot-like edges and "as_boxes" for
    # small (2x2 DBU) box markers.
    # 
    # The "corners" operator can be put into a condition which means it's
    # applied to coners meeting a particular angle constraint.

    def _cop_corners(as_dots = DRCAsDots::new(false))
      # NOTE: this method is a fallback for the respective global ones which route to DRCLayer or here.
      return primary.corners(as_dots)
    end
    
    # %DRC%
    # @name extent_refs
    # @brief Returns partial references to the boundings boxes of the polygons
    # @synopsis extent_refs([ options ])
    # @synopsis extent_refs(layer, [ options ])
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.extent_refs" (see \Layer#extent_refs). Without a layer
    # argument, "extent_refs" represents the partial extents extractor on primary shapes within
    # \DRC# expressions (see \Layer#drc and \DRC#extent_refs for more details).

    # %DRC%
    # @name extents
    # @brief Returns the bounding box of each input object
    # @synopsis extents([ enlargement ])
    # @synopsis extents(layer, [ enlargement ])
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.extents" (see \Layer#extents). Without a layer
    # argument, "extents" represents the extents generator on primary shapes within
    # \DRC# expressions (see \Layer#drc and \DRC#extents for more details).

    # %DRC%
    # @name middle
    # @brief Returns the centers of polygon bounding boxes
    # @synopsis middle([ options ])
    # @synopsis middle(layer, [ options ])
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.middle" (see \Layer#middle). Without a layer
    # argument, "middle" represents the bounding box center marker generator on primary shapes within
    # \DRC# expressions (see \Layer#drc and \DRC#middle for more details).

    # %DRC%
    # @name rounded_corners
    # @brief Applies corner rounding
    # @synopsis rounded_corners(inner, outer, n)
    # @synopsis rounded_corners(layer, inner, outer, n)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.rounded_corners" (see \Layer#rounded_corners). Without a layer
    # argument, "rounded_corners" represents the corner rounding algorithm on primary shapes within
    # \DRC# expressions (see \Layer#drc and \DRC#rounded_corners for more details).
      
    # %DRC%
    # @name sized
    # @brief Returns the sized version of the input
    # @synopsis sized(d [, mode])
    # @synopsis sized(dx, dy [, mode]))
    # @synopsis sized(layer, d [, mode])
    # @synopsis sized(layer, dx, dy [, mode]))
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.sized" (see \Layer#sized). Without a layer
    # argument, "sized" represents the polygon sizer on primary shapes within
    # \DRC# expressions (see \Layer#drc and \DRC#sized for more details).

    # %DRC%
    # @name smoothed
    # @brief Applies smoothing
    # @synopsis smoothed(d)
    # @synopsis smoothed(layer, d)
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.smoothed" (see \Layer#smoothed). Without a layer
    # argument, "smoothed" represents the polygon smoother on primary shapes within
    # \DRC# expressions (see \Layer#drc and \DRC#smoothed for more details).
    
    %w(
      extent_refs
      extents
      middle
      rounded_corners
      sized
      smoothed
    ).each do |f|
      # NOTE: these methods are fallback for the respective global ones which route to DRCLayer or here.
      eval <<"CODE"
        def _cop_#{f}(*args)
          primary.#{f}(*args)
        end
CODE
    end
    
    # %DRC%
    # @name covering
    # @brief Selects shapes entirely covering other shapes
    # @synopsis covering(other) (in conditions)
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
    # @synopsis interacting(other) (in conditions)
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
    # @synopsis overlapping(other) (in conditions)
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
    # @synopsis inside(other)
    #
    # This method represents the selector of primary shapes
    # which are entirely inside shapes from the other layer. 
    # Use this variant within \DRC# expressions (also see \Layer#drc).
    
    # %DRC%
    # @name outside
    # @brief Selects shapes entirely outside other shapes
    # @synopsis outside(other)
    #
    # This method represents the selector of primary shapes
    # which are entirely outside shapes from the other layer. 
    # Use this variant within \DRC# expressions (also see \Layer#drc).
    
    %w(
      covering
      inside
      interacting
      outside
      overlapping
    ).each do |f|
      eval <<"CODE"
        def #{f}(other)
          primary.#{f}(other)
        end
CODE
    end
    
    # %DRC%
    # @name enclosing
    # @brief Performs an enclosing check
    # @synopsis enclosing(other [, options ]) (in conditions)
    # @synopsis enclosing(layer, other [, options ])
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.enclosing" (see \Layer#enclosing). 
    #
    # The version without the first layer is intended for use within \DRC# expressions
    # together with the \Layer#drc method. In this case, this function needs to be 
    # put into a condition to specify the check constraints. The other options
    # of \Layer#enclosing (e.g. metrics, projection constraints, angle limits etc.)
    # apply to this version too. 
    #
    # The conditions may involve an upper and lower limit. The following examples
    # illustrate the use of this function with conditions:
    #
    # @code
    # out = in.drc(enclosing(other) < 0.2.um)
    # out = in.drc(enclosing(other) <= 0.2.um)
    # out = in.drc(enclosing(other) > 0.2.um)
    # out = in.drc(enclosing(other) >= 0.2.um)
    # out = in.drc(enclosing(other) == 0.2.um)
    # out = in.drc(enclosing(other) != 0.2.um)
    # out = in.drc(0.1.um <= enclosing(other) < 0.2.um)
    # @/code
    #
    # The result of the enclosing check are edges or edge pairs forming the markers.
    # These markers indicate the presence of the specified condition.
    # 
    # With a lower and upper limit, the results are edges marking the positions on the 
    # primary shape where the condition is met.
    # With a lower limit alone, these markers are formed by two, identical but opposite edges attached to 
    # the primary shape. Without an upper limit only, the first edge of the marker is attached to the 
    # primary shape while the second edge is attached to the shape of the "other" layer.
    
    # %DRC%
    # @name separation
    # @brief Performs a separation check
    # @synopsis separation(other [, options ]) (in conditions)
    # @synopsis separation(layer, other [, options ])
    #
    # Provides a separation check (primary layer vs. another layer). Like \enclosing this 
    # function provides a two-layer check. See there for details how to use this function.
    
    # %DRC%
    # @name overlap
    # @brief Performs an overlap check
    # @synopsis overlap(other [, options ]) (in conditions)
    # @synopsis overlap(layer, other [, options ])
    #
    # Provides an overlap check (primary layer vs. another layer). Like \enclosing this 
    # function provides a two-layer check. See there for details how to use this function.
    
    # %DRC%
    # @name width
    # @brief Performs a width check
    # @synopsis width([ options ]) (in conditions)
    # @synopsis width(layer [, options ])
    #
    # This function can be used with a layer argument in which case it
    # is equivalent to "layer.width" (see \Layer#width). 
    #
    # The version without a layer is intended for use within \DRC# expressions
    # together with the \Layer#drc method. In this case, this function needs to be 
    # put into a condition to specify the check constraints. The other options
    # of \Layer#width (e.g. metrics, projection constraints, angle limits etc.)
    # apply to this version too. 
    #
    # The conditions may involve an upper and lower limit. The following examples
    # illustrate the use of this function with conditions:
    #
    # @code
    # out = in.drc(width < 0.2.um)
    # out = in.drc(width <= 0.2.um)
    # out = in.drc(width > 0.2.um)
    # out = in.drc(width >= 0.2.um)
    # out = in.drc(width == 0.2.um)
    # out = in.drc(width != 0.2.um)
    # out = in.drc(0.1.um <= width < 0.2.um)
    # @/code
    #
    # With a lower and upper limit, the results are edges marking the positions on the 
    # primary shape where the condition is met.
    # With a lower limit alone, these markers are formed by two, identical but opposite edges attached to 
    # the primary shape. Without an upper limit only, both edges are attached to different sides of the primary
    # shape.
    
    # %DRC%
    # @name space
    # @brief Performs a space check
    # @synopsis space([ options ]) (in conditions)
    # @synopsis space(layer [, options ])
    #
    # Provides a space check on the primary layer. Like \width this 
    # function provides a single-layer check. See there for details how to use this function.
    
    # %DRC%
    # @name notch
    # @brief Performs a notch (intra-polygon space) check
    # @synopsis notch([ options ]) (in conditions)
    # @synopsis notch(layer [, options ])
    #
    # Provides a intra-polygon space check for polygons from the primary layer. Like \width this 
    # function provides a single-layer check. See there for details how to use this function.
    
    %w(
      enclosing
      isolated
      notch
      overlap
      separation
      space
      width
    ).each do |f|
      # NOTE: these methods are fallback for the respective global ones which route to DRCLayer or here.
      eval <<"CODE"
      def _cop_#{f}(*args)
    
        metrics = RBA::Region::Euclidian
        minp = nil
        maxp = nil
        alim = nil
        whole_edges = false
        other = nil
        shielded = nil
        opposite_filter = RBA::Region::NoOppositeFilter
        rect_filter = RBA::Region::NoRectFilter
    
        n = 1
        args.each do |a|
          if a.is_a?(DRCMetrics)
            metrics = a.value
          elsif a.is_a?(DRCWholeEdges)
            whole_edges = a.value
          elsif a.is_a?(DRCOppositeErrorFilter)
            opposite_filter = a.value
          elsif a.is_a?(DRCRectangleErrorFilter)
            rect_filter = RBA::Region::RectFilter::new(a.value.to_i | rect_filter.to_i)
          elsif a.is_a?(DRCAngleLimit)
            alim = a.value
          elsif a.is_a?(DRCOpNode)
            other = a
          elsif a.is_a?(DRCLayer)
            other = self._make_node(a)
          elsif a.is_a?(DRCProjectionLimits)
            (minp, maxp) = a.get_limits(self)
          elsif a.is_a?(DRCShielded)
            shielded = a.value
          else
            raise("Parameter #" + n.to_s + " does not have an expected type (is " + a.inspect + ")")
          end
          n += 1
        end
    
        args = [ whole_edges, metrics, alim, minp, maxp ]
    
        args << (shielded == nil ? true : shielded)
        if :#{f} != :width && :#{f} != :notch
          args << opposite_filter
          args << rect_filter
        elsif opposite_filter != RBA::Region::NoOppositeFilter
          raise("An opposite error filter cannot be used with this check")
        elsif rect_filter != RBA::Region::NoRectFilter
          raise("A rectangle error filter cannot be used with this check")
        end
        
        if :#{f} == :width || :#{f} == :space || :#{f} == :notch || :#{f} == :isolated
          other && raise("No other layer must be specified for a single-layer check")
        else
          other || raise("The other layer must be specified for a two-layer check")
        end
        
        DRCOpNodeCheck::new(self, :#{f}, other, *args)
        
      end  
CODE
    end
    
    # %DRC%
    # @name iso
    # @brief Synonym for "isolated"
    # @synopsis iso(...)
    #
    # "iso" is the short form for \isolated.

    # %DRC%
    # @name sep
    # @brief Synonym for "separation"
    # @synopsis sep(...)
    #
    # "sep" is the short form for \separation.

    # %DRC%
    # @name enc
    # @brief Synonym for "enclosing"
    # @synopsis enc(...)
    #
    # "enc" is the short form for \enclosing.

    def _cop_iso(*args)
      # NOTE: this method is a fallback for the respective global ones which route to DRCLayer or here.
      _cop_isolated(*args)
    end
    
    def _cop_sep(*args)
      # NOTE: this method is a fallback for the respective global ones which route to DRCLayer or here.
      _cop_separation(*args)
    end
    
    def _cop_enc(*args)
      # NOTE: this method is a fallback for the respective global ones which route to DRCLayer or here.
      _cop_enclosing(*args)
    end

    def _make_node(arg)
      if arg.is_a?(DRCLayer)
        arg = DRCOpNode::new(self, RBA::CompoundRegionOperationNode::new_secondary(arg.data))
        arg.description = "secondary"
      end
      arg
    end

  end

end

