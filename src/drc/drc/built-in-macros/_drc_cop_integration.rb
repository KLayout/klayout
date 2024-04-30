# $autorun-early

module DRC

  # %DRC%
  # @scope
  # @name Layer

  class DRCLayer

    # %DRC%
    # @name drc
    # @brief Provides a generic DRC function for use with \DRC# expressions
    # @synopsis layer.drc(expression [, prop_constraint ])
    #
    # This method implements the universal DRC which offers enhanced abilities,
    # improved performance in some applications and better readability.
    #
    # The key concept for this method are DRC expressions. DRC expressions
    # are formed by using predefined keywords like "width", operators like "&" 
    # and methods to build an abstract definition of the operations to perform
    # within the DRC.
    #
    # When the DRC function is executed, it will basically visit all shapes
    # from the input layer (the layer, the "drc" method is called on)).
    # While it does, it collects the neighbor shapes from all involved other inputs 
    # and runs the requested operations on each cluster. 
    # Currently, "drc" is only available for polygon layers.
    #
    # This way, the nature of the "drc" operation is that of the loop over all (merged) input
    # polygons. Within the operation executed on each shape, it's possible to make
    # decisions such as "if the shape has an area larger than something, apply this
    # operation" or similar. This often can be achieved with conventional DRC functions too,
    # but involves potentially complex and heavy operations such as booleans, interact
    # etc. For this reason, the "drc" function may provide a better performance.
    #
    # In addition, within the loop a single shape from the input layer is presented to the
    # execution engine which runs the operations.
    # This allows using operations such as "size" without having to consider 
    # neighbor polygons growing into the area of the initial shape. In this sense,
    # the "drc" function sees the layer as individual polygons rather than
    # a global "sea of polygons". This enables new applications which are otherwise
    # difficult to implement.
    #
    # @h3 Primaries and secondaries @/h3
    #
    # An important concept in "drc" expressions is the "primary".
    # The primary represents a single shape from the input layer. "Secondaries" are shapes
    # from other inputs. Primaries guide the operation - secondaries without
    # primaries are not seen. The "drc" operation will look for secondaries within
    # a certain distance which is determined from the operations from the 
    # expression to execute. The secondaries collected in this step will not be
    # merged, so the secondary polygons may be partial. This is important when
    # using measurement operations like "area" on secondary polygons.
    #
    # @h3 Checks @/h3
    #
    # Here is an example for a generic DRC operation which performs a width
    # check for less than 0.5.um on the primary shapes. It uses the "\global#width" operator:
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
    # To specify the second input for a two-layer check, add it to 
    # the check function. This example shows how to use a two-layer separation check ("\global#separation"):
    #
    # @code
    # l1 = input(1, 0)
    # l2 = input(2, 0)
    # out = l1.drc(separation(l2) < 0.5.um)
    # @/code
    #
    # The second input of this check function can be a computed expression. In this
    # case the local loop will first evaluate the expression for the second input and
    # then use the result as second input in the check. Note that this computation is
    # performed locally and separately for each primary and its context.
    #
    # Options for the checks are also specified inside the brackets. For example,
    # to select projection metrics ("projection") for the "width" check use:
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
    # For example, all checks produce edge pairs which can be converted into polygons
    # using the "polygons" method:
    #
    # @code
    # out = in.drc((width(projection) < 0.5.um).polygons)
    # @/code
    # 
    # Note a subtle detail: when putting the "polygons" method inside the "drc"
    # brackets, it is executed locally on every visited primary polygon. The result
    # in this case is identical to the global conversion:
    #
    # @code
    # # same, but with "global" conversion:
    # out = in.drc(width(projection) < 0.5.um).polygons
    # @/code
    #
    # But having the check polygons inside the loop opens new opportunities and 
    # is more efficient in general. In the previous example, the local conversion
    # will keep a few edge pairs after having converted them to polygons. In
    # the global case, all edge pairs are collected first and then converted.
    # If there are many edge pairs, this requires more memory and a larger computing
    # overhead for managing the bigger number of shapes.
    #
    # For the conversion of edges, edge pairs and polygons into other types, these
    # methods are provided:
    #
    # @ul
    # @li "\DRC#polygons": converts edge pairs to polygons @/li
    # @li "\DRC#extended", "\DRC#extended_in", "\DRC#extended_out": converts edges to polygons @/li
    # @li "\DRC#first_edges", \DRC#second_edges": extracts edges from edge pairs @/li
    # @li "\DRC#edges": decomposes edge pairs and polygons into edges @/li
    # @li "\DRC#corners": can extract corners from polygons @/li
    # @/ul
    #
    # The following example decomposes the primary polygons into edges:
    # 
    # @code
    # out = in.drc(primary.edges)
    # @/code
    #
    # (for backward compatibility you cannot abbreviate "primary.edges" simply as "edges" like
    # other functions). 
    #
    # The previous example isn't quite exciting as it is equivalent to 
    #
    # @code
    # # Same as above
    # out = in.edges
    # @/code
    #
    # But it gets more interesting, as within the loop, "edges" delivers the edge set for
    # each individual polygon. It's possible to work with this distinct set, so for example
    # this will give you the edges of polygons with more than four corners:
    #
    # @code
    # out = in.drc(primary.edges.count > 4)
    # @/code
    #
    # Explanation: "count" is a "quantifier" which takes any kind of set (edges, edge pairs, polygons)
    # and returns the set if the number of inhabitants meets the given condition. Otherwise the set
    # is skipped. So it will look at the edges and if there are more than four (per primary shape),
    # it will forward this set. 
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
    # @li Edge vs. polygon interactions ("\DRC#interacting", "\DRC#overlapping") @/li
    # @li Edge sampling ("\DRC#start_segments", "\DRC#centers", "\DRC#end_segments") @/li
    # @/ul
    #
    # @h3 Filters @/h3
    #
    # Filter operators select input polygons or edges based on their properties. These filters are provided:
    #
    # @ul
    # @li "\DRC#area": selects polygons based on their area @/li
    # @li "\DRC#perimeter": selects polygons based on their perimeter @/li
    # @li "\DRC#area_ratio": selects polygons based on their bounding box to polygon area ratio @/li
    # @li "\DRC#bbox_aspect_ratio": selects polygons based on their bounding box aspect ratio @/li
    # @li "\DRC#relative_height": selects polygons based on their relative height @/li
    # @li "\DRC#bbox_min", "\DRC#bbox_max", "\DRC#bbox_width", "\DRC#bbox_height": selects polygons based on their bounding box properties @/li
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
    # Note that it's basically possible to use the polygon filters on any input - computed and secondaries.
    # In fact, plain "area" for example is a shortcut for "\global#primary.area" indicating that
    # the area of primary shapes are supposed to be computed.
    # However, any input other than the primary is not necessarily complete or it may 
    # consist of multiple polygons. Hence the computed values may be too big or too small.
    # It's recommended therefore to use the measurement functions on primary polygons
    # unless you know what you're doing.
    #
    # @h3 Filter predicates @/h3
    #
    # The "drc" feature also supports some predicates. "predicates" are boolean values
    # indicating a certain condition. A predicate filter works in a way that it only
    # passes the polygons if the condition is met.
    #
    # The predicates available currently are:
    #
    # @ul
    # @li "\DRC#rectangles": Filters rectangles @/li
    # @li "\DRC#squares": Filters squares @/li
    # @li "\DRC#rectilinear": Filters rectilinear ("Manhattan") polygons @/li
    # @/ul
    #
    # For the same reason as explained above, it's recommended to use these predicates
    # standalone, so they act on primary shapes. It's possible to use the predicates
    # on computed shapes or secondaries, but that may not render the desired results.
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
    # The logical "if_any" or "if_all" functions allow connecting multiple
    # conditions and evaluate to "true" (means: a non-empty shape set) if either
    # one input is a non-empty shape set ("if_any") or if all inputs are non-empty
    # ("if_all"). 
    #
    # For example, this will select all polygons which are rectangles
    # and whose area is larger than 20 square micrometers:
    #
    # @code
    # out = in.drc(if_all(rectangles, area > 20.0))
    # @/code
    #
    # "if_all" delivers the primary shape if all of the input expressions
    # render a non-empty result.
    #
    # In contrast to this, the "if_any" operation will deliver the primary shape
    # if one of the input expressions renders a non-empty result.
    #
    # The "\global#switch" function allows selecting one input based on the results of an
    # expression. In the two-input form it's equivalent to "if". The first expression
    # is the condition. If it evaluates to a non-empty shape set, the result of the
    # second expression is taken. Otherwise, the result is empty. 
    #
    # Hence the following code delivers all rectangles sized by 100 nm. All
    # other shapes are skipped:
    #
    # @code
    # out = in.drc(switch(rectangles, primary.sized(100.nm)))
    # @/code
    #
    # A third expression will be considered the "else" branch: the result of
    # this expression will be taken if the first one is not taken. So this
    # example will size all rectangles and leave other shapes untouched:
    #
    # @code
    # out = in.drc(switch(rectangles, primary.sized(100.nm), primary))
    # @/code
    #
    # If more expressions are given, they are considered as a sequence of condition/result
    # chain (c1, e1, c2, e2, ...) in the sense of "if(c1) return(e1) else if(c2) return(e2) ...".
    # So the e1 is taken if c1 is met, e2 is taken when c1 is not met, but c2 is and so forth.
    # If there is an odd number of expressions, the last one will be the default expression
    # which is taken if none of the conditions is met.
    #
    # @h3 Polygon manipulations @/h3
    #
    # The "drc" operations feature polygon manipulations where the input is
    # either the primary, secondaries or derived shapes.
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
    # The "\global#secondary" operator indicates that "l2" is to be used as secondary input to the "drc" function. Only
    # in this form, the operators of the boolean AND can be reversed:
    # 
    # @code
    # out = l1.drc((secondary(l2) & primary).area > 1.0)
    # @/code
    #
    # @h3 Quantifiers @/h3
    #
    # Some filters operate on properties of the full, local, per-primary shape set. 
    # While the loop is executed, the DRC expressions will collect shapes, either
    # from the primary, its neighborhood (secondaries) or from deriving shape sets.
    #
    # Obviously the primary is a simple one: it consists of a single shape, because
    # this is how the loop operates. Derived shape sets however can be more complex.
    # "Quantifiers" allow assessing properties of the complete, per-primary shape
    # set. A simple one is "\DRC#count" which checks if the number of shapes within
    # a shape set is within a given range.
    #
    # Obviously, "primary.count == 1" is always true. So using "count" primaries isn't
    # much fun. So it's better to use it on derived sets.
    # The following condition will select all primary shapes which have more than 13 corners:
    #
    # @code
    # out = in.drc(if_any(primary.corners.count > 13))
    # @/code
    #
    # Note an important detail here: the "if_any" function will make this statement render primary
    # polygons, if the expression inside gives a non-empty result. Without
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
    # or the boolean operation. But when the "drc" function executes the loop over the primaries it will 
    # only compute the area once per primary as it is represented by the same Ruby object.
    #
    # @h3 Properties constraints @/h3
    #
    # The method can be given a properties constraint so that it is only performed
    # between shapes with the same or different user properties. Note that properties
    # have to be enabled or generated (e.g. through the \DRCLayer#nets method) before they can
    # be used.
    #
    # Example:
    #
    # @code
    # connect(metal1, via1)
    # ... 
    # 
    # space_not_connected = metal1.nets.drc(space < 0.4.um, props_ne)
    # @/code
    #
    # See \global#props_eq, \global#props_ne and \global#props_copy for details.
    #
    # @h3 Outlook @/h3
    #
    # DRC expressions are quite rich and powerful. They provide a more intuitive way of
    # writing DRC expressions, are more efficient and open new opportunities. DRC
    # development is likely to focus on this scheme in the future.
    #
    # More formal details about the bits and pieces can be found in the "\DRC" class documentation.

    def drc(op, prop_constraint = DRCPropertiesConstraint::new(RBA::Region::IgnoreProperties))
      @engine._context("drc") do
        requires_region
        op.is_a?(DRCOpNode) || raise("A DRC expression is required for the argument (got #{op.inspect})")
        prop_constraint.is_a?(DRCPropertiesConstraint) || raise("A properties constraint is required for the second argument (got #{prop_constraint.inspect})")
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
          DRCLayer::new(@engine, @engine._tcmd(self.data, node.distance, result_cls, :complex_op, node, prop_constraint.value))
        end
      end
    end
    
  end

  # %DRC%
  # @scope 
  # @name global 

  class DRCEngine

    # %DRC%
    # @name switch
    # @brief A conditional selector for the "drc" universal DRC function
    # @synopsis switch(...)
    #
    # This function provides a conditional selector for the "drc" function.
    # It is used this way:
    #
    # @code
    #   out = in.drc(switch(c1, r1, c2, r2, ..., cn, rn)
    #   out = in.drc(switch(c1, r1, c2, r2, ..., cn, rn, rdef)
    # @/code
    # 
    # This function will evaluate c1 which is a universal DRC expression (see \Layer#drc).
    # If the result is not empty, "switch" will evaluate and return r1. Otherwise it
    # will continue with c2 and the result of this expression is not empty it will
    # return r2. Otherwise it will continue with c3/r3 etc. 
    #
    # If an odd number of arguments is given, the last expression is evaluated if
    # none of the conditions c1..cn gives a non-empty result.
    #
    # As a requirement, the result types of all r1..rn expressions and the rdef
    # needs to be the same - i.e. all need to render polygons or edges or edge pairs.

    def switch(*args)

      self._context("switch") do

        args = args.collect { |a| self._make_node(a) }

        args.each_with_index do |a,index|
          if !a.is_a?(DRCOpNode)
            raise("All inputs need to be valid compound operation expressions (argument ##{index + 1} isn't)")
          end
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

        res = DRCOpNode::new(self) { RBA::CompoundRegionOperationNode::new_secondary(layer.data) }
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
      res = DRCOpNode::new(self) { RBA::CompoundRegionOperationNode::new_primary }
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
    # are always symmetric: a polygon cannot be considered separated from its neighbors
    # on the same layer.
    # 
    # The following example computes every part of the input which is closer than
    # 0.5 micrometers to other (disconnected) polygons on the same layer:
    #
    # @code
    # out = in.drc(primary & foreign.sized(0.5.um))
    # @/code
    
    def foreign
      res = DRCOpNode::new(self) { RBA::CompoundRegionOperationNode::new_foreign }
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
    # out = in.drc(if_all(area > 0.5, rectangles))
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
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.area" (see \Layer#area) and returns the total area of the 
    # polygons in the layer. 
    #
    # Without a layer argument, "area" represents an area filter for primary shapes in 
    # \global# expressions (see \Layer#drc and \DRC#area for more details).
    
    # %DRC%
    # @name hulls
    # @brief Selects all hulls from the input polygons
    # @synopsis hulls
    # @synopsis hulls(layer)
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.hulls" (see \Layer#hulls). Without a layer
    # argument, "hulls" represents a hull contour extractor for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \global#hulls for more details).
    
    # %DRC%
    # @name holes
    # @brief Selects all holes from the input polygons
    # @synopsis holes
    # @synopsis holes(layer)
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.holes" (see \Layer#hulls). Without a layer
    # argument, "holes" represents a hole extractor for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \DRC#hulls for more details).
    
    # %DRC%
    # @name perimeter
    # @brief Computes the total perimeter or in universal DRC context: selects the primary shape if the perimeter is meeting the condition
    # @synopsis perimeter (in condition)
    # @synopsis perimeter(layer)
    #
    # This function can be used with a layer argument. In this case it
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
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.rectangles" (see \Layer#rectangles). Without a layer
    # argument, "rectangles" represents the rectangles filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \global#rectangles for more details).

    # %DRC%
    # @name squares
    # @brief Selects all polygons which are squares
    # @synopsis squares
    # @synopsis squares(layer)
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.squares" (see \Layer#squares). Without a layer
    # argument, "squares" represents the rectangles filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \global#squares for more details).

    # %DRC%
    # @name rectilinear
    # @brief Selects all polygons which are rectilinear
    # @synopsis rectilinear
    # @synopsis rectilinear(layer)
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.rectilinear" (see \Layer#rectilinear). Without a layer
    # argument, "rectilinear" represents the rectilinear polygons filter for primary shapes in 
    # \DRC# expressions (see \Layer#drc and \global#rectilinear for more details).

    # %DRC%
    # @name length
    # @brief Computes the total edge length of an edge layer or in universal DRC context: selects edges based on a length condition
    # @synopsis length (in condition)
    # @synopsis length(layer)
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.length" (see \Layer#length). Without a layer
    # argument, "length" represents the edge length filter on the primary shape edges in 
    # \DRC# expressions (see \Layer#drc and \DRC#length for more details). In this context,
    # the operation acts similar to \Layer#with_length.

    %w(
      area
      holes
      hulls
      length
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
    # @name angle
    # @brief In universal DRC context: selects edges based on their orientation
    # @synopsis angle (in condition)
    #
    # "angle" represents the edge orientation filter on the primary shape edges in
    # \DRC# expressions (see \Layer#drc and \DRC#angle for more details). In this context,
    # the operation acts similar to \Layer#with_angle.

    # %DRC%
    # @name corners
    # @brief Selects corners of polygons
    # @synopsis corners([ options ]) (in condition)
    # @synopsis corners(layer [, options ])
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.corners" (see \Layer#corners). Without a layer
    # argument, "corners" represents the corner generator/filter in primary shapes for
    # \DRC# expressions (see \Layer#drc and \global#corners for more details).
    #
    # Like the layer-based version, the "corners" operator accepts the 
    # output type option: "as_dots" for dot-like edges, "as_boxes" for
    # small (2x2 DBU) box markers and "as_edge_pairs" for edge pairs.
    # The default output type is "as_boxes".
    # 
    # The "corners" operator can be put into a condition which means it's
    # applied to corners meeting a particular angle constraint.

    # %DRC%
    # @name extent_refs
    # @brief Returns partial references to the boundings boxes of the polygons
    # @synopsis extent_refs([ options ])
    # @synopsis extent_refs(layer, [ options ])
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.extent_refs" (see \Layer#extent_refs). Without a layer
    # argument, "extent_refs" represents the partial extents extractor on primary shapes within
    # \DRC# expressions (see \Layer#drc and \global#extent_refs for more details).

    # %DRC%
    # @name extents
    # @brief Returns the bounding box of each input object
    # @synopsis extents([ enlargement ])
    # @synopsis extents(layer, [ enlargement ])
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.extents" (see \Layer#extents). Without a layer
    # argument, "extents" represents the extents generator on primary shapes within
    # \DRC# expressions (see \Layer#drc and \global#extents for more details).

    # %DRC%
    # @name middle
    # @brief Returns the centers of polygon bounding boxes
    # @synopsis middle([ options ])
    # @synopsis middle(layer, [ options ])
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.middle" (see \Layer#middle). Without a layer
    # argument, "middle" represents the bounding box center marker generator on primary shapes within
    # \DRC# expressions (see \Layer#drc and \global#middle for more details).

    # %DRC%
    # @name rounded_corners
    # @brief Applies corner rounding
    # @synopsis rounded_corners(inner, outer, n)
    # @synopsis rounded_corners(layer, inner, outer, n)
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.rounded_corners" (see \Layer#rounded_corners). Without a layer
    # argument, "rounded_corners" represents the corner rounding algorithm on primary shapes within
    # \DRC# expressions (see \Layer#drc and \global#rounded_corners for more details).
      
    # %DRC%
    # @name sized
    # @brief Returns the sized version of the input
    # @synopsis sized(d [, mode])
    # @synopsis sized(dx, dy [, mode]))
    # @synopsis sized(layer, d [, mode])
    # @synopsis sized(layer, dx, dy [, mode]))
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.sized" (see \Layer#sized). Without a layer
    # argument, "sized" represents the polygon sizer on primary shapes within
    # \DRC# expressions (see \Layer#drc and \global#sized for more details).

    # %DRC%
    # @name smoothed
    # @brief Applies smoothing
    # @synopsis smoothed(d)
    # @synopsis smoothed(layer, d)
    #
    # This function can be used with a layer argument. In this case it
    # is equivalent to "layer.smoothed" (see \Layer#smoothed). Without a layer
    # argument, "smoothed" represents the polygon smoother on primary shapes within
    # \DRC# expressions (see \Layer#drc and \global#smoothed for more details).
    
    %w(
      extent_refs
      extents
      middle
      rounded_corners
      sized
      smoothed
      corners
      angle
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
    # @synopsis covering(other) (optionally in condition)
    #
    # This operator represents the selector of primary shapes
    # which entirely cover shapes from the other layer. This version can be put into
    # a condition indicating how many shapes of the other layer need to be covered.
    # Use this operator within \DRC# expressions (also see \Layer#drc). If can be used
    # as method to an expression. See there for more details: \global#covering.

    # %DRC%
    # @name interacting
    # @brief Selects shapes interacting with other shapes
    # @synopsis interacting(other) (optionally in condition)
    #
    # This operator represents the selector of primary shapes
    # which interact with shapes from the other layer. This version can be put into
    # a condition indicating how many shapes of the other layer need to be covered.
    # Use this operator within \DRC# expressions (also see \Layer#drc). If can be used
    # as method to an expression. See there for more details: \global#interacting.
    
    # %DRC%
    # @name overlapping
    # @brief Selects shapes overlapping with other shapes
    # @synopsis overlapping(other) (optionally in condition)
    #
    # This operator represents the selector of primary shapes
    # which overlap shapes from the other layer. This version can be put into
    # a condition indicating how many shapes of the other layer need to be covered.
    # Use this operator within \DRC# expressions (also see \Layer#drc). If can be used
    # as method to an expression. See there for more details: \global#overlapping.
    
    # %DRC%
    # @name inside
    # @brief Selects shapes entirely inside other shapes
    # @synopsis inside(other)
    #
    # This operator represents the selector of primary shapes
    # which are inside shapes from the other layer. 
    # Use this operator within \DRC# expressions (also see \Layer#drc). If can be used
    # as method to an expression. See there for more details: \global#inside.
    
    # %DRC%
    # @name outside
    # @brief Selects shapes entirely outside other shapes
    # @synopsis outside(other)
    #
    # This operator represents the selector of primary shapes
    # which are outside shapes from the other layer. 
    # Use this operator within \DRC# expressions (also see \Layer#drc). If can be used
    # as method to an expression. See there for more details: \global#outside.
    
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
    # @name with_holes
    # @brief Selects all input polygons according to their number of holes in DRC expressions
    # @synopsis with_holes (in condition)
    #
    # "with_holes" represents a polygon selector for
    # \DRC# expressions selecting polygons of the primary by their number of holes
    # (see \Layer#drc and \global#with_holes for more details).

    def with_holes
      primary.with_holes
    end
    
    # %DRC%
    # @name enclosing
    # @brief Performs an enclosing check
    # @synopsis enclosing(other [, options ]) (in conditions)
    # @synopsis enclosing(layer, other [, options ])
    #
    # This check verifies if the polygons of the input layer are enclosing the shapes
    # of the other input layer by a certain distance.
    # It has manifold options. See \Layer#width for the basic options such
    # as metrics, projection and angle constraints etc. This check also features
    # opposite and rectangle filtering. See \Layer#separation for details about opposite and
    # rectangle error filtering.
    #
    # @h3 Classic mode @/h3
    #
    # This function can be used in classic mode with a layer argument. In this case it
    # is equivalent to "layer.enclosing" (see \Layer#enclosing). 
    #
    # @code
    # # classic "enclosing" check for < 0.2 um
    # in = layer(1, 0)
    # other = layer(2, 0)
    # errors = enclosing(in, other, 0.2.um)
    # @/code
    #
    # @h3 Universal DRC @/h3
    #
    # The version without a first layer is intended for use within \DRC# expressions
    # together with the "universal DRC" method \Layer#drc. In this case, this function needs to be 
    # put into a condition to specify the check constraints. The other options
    # of \Layer#enclosing (e.g. metrics, projection constraints, angle limits etc.)
    # apply to this version too:
    #
    # @code
    # # universal DRC "enclosing" check for < 0.2 um
    # in = layer(1, 0)
    # other = layer(2, 0)
    # errors = in.drc(enclosing(other) < 0.2.um)
    # @/code
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
    # With a lower limit alone, the results are edge pairs which are formed by two identical, but opposite edges attached to 
    # the primary shape. Without an upper limit only, the first edge of the marker is attached to the 
    # primary shape while the second edge is attached to the shape of the "other" layer.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_enc1u.png) @/td
    #     @td @img(/images/drc_enc2u.png) @/td
    #   @/tr
    # @/table
    #
    # When "larger than" constraints are used, this function will produce the edges from the
    # first layer only. The result will still be edge pairs for consistency, but each edge pair holds one edge from
    # the original polygon plus a reverse copy of that edge in the second member. Use "first_edges" to extract the 
    # actual edges from the first input (see \separation for an example).
    #
    
    # %DRC%
    # @name enclosed
    # @brief Performs an enclosing check (other enclosing layer)
    # @synopsis enclosed(other [, options ]) (in conditions)
    # @synopsis enclosed(layer, other [, options ])
    #
    # This check verifies if the polygons of the input layer are enclosed by shapes
    # of the other input layer by a certain distance.
    # It has manifold options. See \Layer#width for the basic options such
    # as metrics, projection and angle constraints etc. This check also features
    # opposite and rectangle filtering. See \Layer#separation for details about opposite and
    # rectangle error filtering.
    #
    # This function is essentially the reverse of \enclosing. In case of
    # "enclosed", the other layer must be bigger than the primary layer.
    #
    # @h3 Classic mode @/h3
    #
    # This function can be used in classic mode with a layer argument. In this case it
    # is equivalent to "layer.enclosed" (see \Layer#enclosed). 
    #
    # @code
    # # classic "enclosed" check for < 0.2 um
    # in = layer(1, 0)
    # other = layer(2, 0)
    # errors = enclosed(in, other, 0.2.um)
    # @/code
    #
    # @h3 Universal DRC @/h3
    #
    # The version without a first layer is intended for use within \DRC# expressions
    # together with the "universal DRC" method \Layer#drc. In this case, this function needs to be 
    # put into a condition to specify the check constraints. The other options
    # of \Layer#enclosed (e.g. metrics, projection constraints, angle limits etc.)
    # apply to this version too:
    #
    # @code
    # # universal DRC "enclosed" check for < 0.2 um
    # in = layer(1, 0)
    # other = layer(2, 0)
    # errors = in.drc(enclosed(other) < 0.2.um)
    # @/code
    #
    # The conditions may involve an upper and lower limit. The following examples
    # illustrate the use of this function with conditions:
    #
    # @code
    # out = in.drc(enclosed(other) < 0.2.um)
    # out = in.drc(enclosed(other) <= 0.2.um)
    # out = in.drc(enclosed(other) > 0.2.um)
    # out = in.drc(enclosed(other) >= 0.2.um)
    # out = in.drc(enclosed(other) == 0.2.um)
    # out = in.drc(enclosed(other) != 0.2.um)
    # out = in.drc(0.1.um <= enclosed(other) < 0.2.um)
    # @/code
    #
    # The result of the enclosed check are edges or edge pairs forming the markers.
    # These markers indicate the presence of the specified condition.
    # 
    # With a lower and upper limit, the results are edges marking the positions on the 
    # primary shape where the condition is met.
    # With a lower limit alone, the results are edge pairs which are formed by two identical, but opposite edges attached to 
    # the primary shape. Without an upper limit only, the first edge of the marker is attached to the 
    # primary shape while the second edge is attached to the shape of the "other" layer.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_encd1u.png) @/td
    #     @td @img(/images/drc_encd2u.png) @/td
    #   @/tr
    # @/table
    #
    # When "larger than" constraints are used, this function will produce the edges from the
    # first layer only. The result will still be edge pairs for consistency, but each edge pair holds one edge from
    # the original polygon plus a reverse copy of that edge in the second member. Use "first_edges" to extract the 
    # actual edges from the first input (see \separation for an example).
    #
    
    # %DRC%
    # @name separation
    # @brief Performs a separation check
    # @synopsis separation(other [, options ]) (in conditions)
    # @synopsis separation(layer, other [, options ])
    #
    # Provides a separation check (primary layer vs. another layer). Like \enclosing this 
    # function provides a two-layer check, but checking the distance rather than the 
    # overlap. 
    # This check has manifold options. See \Layer#width for the basic options such
    # as metrics, projection and angle constraints etc. This check also features
    # opposite and rectangle filtering. See \Layer#separation for details about opposite and
    # rectangle error filtering.
    # 
    # @h3 Classic mode @/h3
    #
    # Like \enclosing, this function is available as a classic DRC function with a layer as the first
    # argument and as an \DRC expression operator for use with \Layer#drc.
    # 
    # @code
    # # classic "separation" check for distance < 0.2 um
    # in = layer(1, 0)
    # other = layer(2, 0)
    # errors = separation(in, other, 0.2.um)
    # @/code
    #
    # @h3 Universal DRC @/h3
    #
    # For use with the "universal DRC" put the separation expression into the "drc"
    # function call and use a condition to specify the constraint:
    #
    # @code
    # # universal DRC "separation" check for distance < 0.2 um
    # in = layer(1, 0)
    # other = layer(2, 0)
    # errors = in.drc(separation(other) < 0.2.um)
    # @/code
    #
    # \enclosing explains the constraints and how the
    # work in generating error markers.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_separation1u.png) @/td
    #   @/tr
    # @/table
    #
    # When "larger than" constraints are used, this function will produce the edges from the
    # first layer only. The result will still be edge pairs for consistency, but each edge pair holds one edge from
    # the original polygon plus a reverse copy of that edge in the second member. Use "first_edges" to extract the 
    # actual edges from the first input:
    #
    # @code
    # l1_edges_without_l2 = l1.drc((separation(l2) >= 1.0).first_edges)
    # @/code
    #
    # The following image shows the effect of such a negative-output separation check:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_separation1un.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name overlap
    # @brief Performs an overlap check
    # @synopsis overlap(other [, options ]) (in conditions)
    # @synopsis overlap(layer, other [, options ])
    #
    # Provides an overlap check (primary layer vs. another layer). 
    # This check has manifold options. See \Layer#width for the basic options such
    # as metrics, projection and angle constraints etc. This check also features
    # opposite and rectangle filtering. See \Layer#separation for details about opposite and
    # rectangle error filtering.
    #
    # @h3 Classic mode @/h3
    #
    # Like other checks, this function is available as a classic DRC function with a layer as the first
    # argument and as an \DRC expression operator for use with \Layer#drc.
    # 
    # @code
    # # classic "overlap" check for < 0.2 um
    # in = layer(1, 0)
    # other = layer(2, 0)
    # errors = overlap(in, other, 0.2.um)
    # @/code
    #
    # @h3 Universal DRC @/h3
    #
    # For use with the "unversal DRC" put the separation expression into the "drc"
    # function call and use a condition to specify the constraint:
    #
    # @code
    # # universal DRC "overlap" check for < 0.2 um
    # in = layer(1, 0)
    # other = layer(2, 0)
    # errors = in.drc(overlap(other) < 0.2.um)
    # @/code
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_overlap1u.png) @/td
    #     @td @img(/images/drc_overlap2u.png) @/td
    #   @/tr
    # @/table
    #
    # When "larger than" constraints are used, this function will produce the edges from the
    # first layer only. The result will still be edge pairs for consistency, but each edge pair holds one edge from
    # the original polygon plus a reverse copy of that edge in the second member. Use "first_edges" to extract the 
    # actual edges from the first input (see \separation for an example).
    #
    
    # %DRC%
    # @name width
    # @brief Performs a width check
    # @synopsis width([ options ]) (in conditions)
    # @synopsis width(layer [, options ])
    #
    # A width check is a check for the distance of edges of the same polygon.
    #
    # @h3 Classic mode @/h3
    #
    # This function can be used in classic mode with a layer argument. In this case it
    # is equivalent to "layer.width" (see \Layer#width). 
    #
    # @code
    # # classic "width" check for width < 2 um
    # in = layer(1, 0)
    # errors = width(in, 0.2.um)
    # @/code
    #
    # @h3 Universal DRC @/h3
    #
    # The version without a layer is intended for use within \DRC# expressions
    # together with the "universal DRC" method \Layer#drc. In this case, this function needs to be 
    # put into a condition to specify the check constraints. The other options
    # of \Layer#width (e.g. metrics, projection constraints, angle limits etc.)
    # apply to this version too:
    #
    # @code
    # # universal DRC check for width < 2 um
    # in = layer(1, 0)
    # errors = in.drc(width < 0.2.um)
    # @/code
    #
    # The conditions may involve an upper and lower limit. The following examples
    # illustrate the use of this function with conditions:
    #
    # @code
    # errors = in.drc(width < 0.2.um)
    # errors = in.drc(width <= 0.2.um)
    # errors = in.drc(width > 0.2.um)
    # errors = in.drc(width >= 0.2.um)
    # errors = in.drc(width == 0.2.um)
    # errors = in.drc(width != 0.2.um)
    # errors = in.drc(0.1.um <= width < 0.2.um)
    # @/code
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_width1u.png) @/td
    #     @td @img(/images/drc_width2u.png) @/td
    #   @/tr
    # @/table
    #
    # With a lower and upper limit or with the "equal" condition, the results are edges marking the positions on the 
    # primary shape where the condition is met.
    # With a lower limit alone, the results are edge pairs which are formed by two identical, but opposite edges attached to 
    # the primary shape. Without an upper limit only, both edges are attached to different sides of the primary
    # shape.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_width3u.png) @/td
    #     @td @img(/images/drc_width4u.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_width5u.png) @/td
    #     @td @img(/images/drc_width6u.png) @/td
    #   @/tr
    # @/table
    #
    
    # %DRC%
    # @name space
    # @brief Performs a space check
    # @synopsis space([ options ]) (in conditions)
    # @synopsis space(layer [, options ])
    #
    # "space" looks for spacing violations between edges of the same polygon (intra-polygon checks)
    # and between different polygons (inter-polygon checks).
    # \notch is similar function that provides only intra-polygon space checks. \isolated
    # is the version checking inter-polygon distance only.
    # The check has manifold options. See \Layer#width for the basic options such
    # as metrics, projection and angle constraints etc.
    # 
    # @h3 Classic mode @/h3
    #
    # This function can be used in classic mode with a layer argument. In this case it
    # is equivalent to "layer.space" (see \Layer#space). In this mode, "space" is applicable to edge 
    # layers too. 
    #
    # @code
    # # classic "space" check for space < 0.2 um
    # in = layer(1, 0)
    # errors = space(in, 0.2.um)
    # @/code
    #
    # @h3 Universal DRC @/h3
    #
    # The version without a layer is intended for use within \DRC# expressions
    # together with the "universal DRC" method \Layer#drc. In this case, this function needs to be 
    # put into a condition to specify the check constraints. The other options
    # of \Layer#space (e.g. metrics, projection constraints, angle limits etc.)
    # apply to this version too:
    #
    # @code
    # # universal DRC check for space < 0.2.um
    # in = layer(1, 0)
    # errors = in.drc(space < 0.2.um)
    # @/code
    #
    # See \enclosing for more details about the various ways to specify conditions.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_space1u.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name notch
    # @brief Performs a notch (intra-polygon space) check
    # @synopsis notch([ options ]) (in conditions)
    # @synopsis notch(layer [, options ])
    #
    # Provides a intra-polygon space check for polygons. It is similar to
    # \space, but checks intra-polygon space only.
    # It has manifold options. See \Layer#width for the basic options such
    # as metrics, projection and angle constraints etc.
    # 
    # @h3 Classic mode @/h3
    #
    # This function can be used in classic mode with a layer argument. In this case it
    # is equivalent to "layer.notch" (see \Layer#notch). 
    #
    # @code
    # # classic "notch" check for space < 1.2 um
    # in = layer(1, 0)
    # errors = notch(in, 1.2.um)
    # @/code
    #
    # @h3 Universal DRC @/h3
    #
    # The version without a layer is intended for use within \DRC# expressions
    # together with the "universal DRC" method \Layer#drc. In this case, this function needs to be 
    # put into a condition to specify the check constraints. The other options
    # of \Layer#notch (e.g. metrics, projection constraints, angle limits etc.)
    # apply to this version too:
    #
    # @code
    # # universal DRC "notch" check for space < 1.2.um
    # in = layer(1, 0)
    # errors = in.drc(notch < 1.2.um)
    # @/code
    #
    # See \enclosing for more details about the various ways to specify conditions.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_space2u.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name isolated
    # @brief Performs an isolation (inter-polygon space) check
    # @synopsis isolated([ options ]) (in conditions)
    # @synopsis iso([ options ]) (in conditions)
    # @synopsis isolated(layer [, options ])
    # @synopsis iso(layer [, options ])
    #
    # Provides a intra-polygon space check for polygons. It is similar to
    # \space, but checks inter-polygon space only. "iso" is a synonym for "isolated".
    # This check has manifold options. See \Layer#width for the basic options such
    # as metrics, projection and angle constraints etc. This check also features
    # opposite and rectangle filtering. See \Layer#separation for details about opposite and
    # rectangle error filtering.
    # 
    # @h3 Classic mode @/h3
    #
    # This function can be used in classic mode with a layer argument. In this case it
    # is equivalent to "layer.isolated" (see \Layer#isolated). 
    #
    # @code
    # # classic "isolated" check for space < 1.2 um
    # in = layer(1, 0)
    # errors = isolated(in, 1.2.um)
    # @/code
    #
    # @h3 Universal DRC @/h3
    #
    # The version without a layer is intended for use within \DRC# expressions
    # together with the "universal DRC" method \Layer#drc. In this case, this function needs to be 
    # put into a condition to specify the check constraints. The other options
    # of \Layer#isolated (e.g. metrics, projection constraints, angle limits etc.)
    # apply to this version too:
    #
    # @code
    # # universal DRC "isolated" check for space < 1.2.um
    # in = layer(1, 0)
    # errors = in.drc(isolated < 1.2.um)
    # @/code
    #
    # See \enclosing for more details about the various ways to specify conditions.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_space3u.png) @/td
    #   @/tr
    # @/table
    
    %w(
      enclosing
      enclosed
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
        zd_mode = RBA::Region::IncludeZeroDistanceWhenTouching
    
        n = 1
        args.each do |a|
          if a.is_a?(DRCMetrics)
            metrics = a.value
          elsif a.is_a?(DRCWholeEdges)
            whole_edges = a.value
          elsif a.is_a?(DRCPropertiesConstraint)
            prop_constraint = a.value
          elsif a.is_a?(DRCZeroDistanceMode)
            zd_mode = a.value
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

        args << zd_mode
        
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
        node = DRCOpNode::new(self) { RBA::CompoundRegionOperationNode::new_secondary(arg.data) }
        node.description = "secondary"
        return node
      else
        return arg
      end
    end

  end

end

