# $autorun-early

module DRC

  class DRCLayer

    # %DRC%
    # @name drc
    # @brief Universal DRC function
    # @synopsis drc(...)
    #
    # TODO: add doc

    def drc(op)
      @engine._context("drc") do
        requires_region
        return DRCLayer::new(@engine, self.data.complex_op(op.create_node({})))
      end
    end
    
  end

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
    # This function will evaluate c1 which is a universal DRC expression (see \drc).
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
    # To supply additional input for the universal DRC expressions (see \drc), use
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
    # The primary input of the universal DRC function is the layer the \drc function
    # is called on.
    
    def primary
      res = DRCOpNode::new(self, RBA::CompoundRegionOperationNode::new_primary)
      res.description = "primary"
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
    # The condition expressions can be of any type (edges, edge pairs and polygons).
     
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
    # This method creates a universal DRC expression (see \drc) to select primary shapes whose
    # bounding box height satisfies the condition. Conditions can be written as arithmetic comparisons
    # against numeric values. For example, "bbox_height < 2.0" will select all primary shapes whose
    # bounding box height is less than 2 micrometers. See \drc for more details about comparison 
    # specs. Plain "bbox_min" is equivalent to "primary.bbox_min" - i.e. it is used on the primary
    # shape. Also see \DRC#bbox_min.
    
    # %DRC%
    # @name bbox_width
    # @brief Selects primary shapes based on their bounding box width
    # @synopsis bbox_max (in condition)
    #
    # See \drc, \bbox_height and \DRC#bbox_height for more details.
    
    # %DRC%
    # @name bbox_max
    # @brief Selects primary shapes based on their bounding box height or width, whichever is larger
    # @synopsis bbox_max (in condition)
    #
    # See \drc, \bbox_max and \DRC#bbox_max for more details.
    
    # %DRC%
    # @name bbox_min
    # @brief Selects primary shapes based on their bounding box height or width, whichever is smaller
    # @synopsis bbox_max (in condition)
    #
    # See \drc, \bbox_min and \DRC#bbox_min for more details.
    
    %w(
      bbox_height
      bbox_max
      bbox_min
      bbox_width
    ).each do |f|
      eval <<"CODE"
        def #{f}
          self._context("#{f}") do
            primary.#{f}
          end
        end
CODE
    end
    
    %w(
      area
      holes
      hulls
      odd_polygons
      perimeter
      rectangles
      rectilinear
    ).each do |f|
      # NOTE: these methods are fallback for the respective global ones which route to DRCLayer or here.
      eval <<"CODE"
        def _cop_#{f}
          primary.#{f}
        end
CODE
    end
    
    def _cop_corners(as_dots = DRCAsDots::new(false))
      # NOTE: this method is a fallback for the respective global ones which route to DRCLayer or here.
      return primary.corners(as_dots)
    end
    
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
    
    %w(
      covering
      inside
      interacting
      outside
      overlapping
    ).each do |f|
      # NOTE: these methods are fallback for the respective global ones which route to DRCLayer or here.
      eval <<"CODE"
        def _cop_#{f}(other)
          primary.#{f}(other)
        end
CODE
    end
    
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
          elsif a.is_a?(DRCProjectionLimits)
            minp = self._make_value(a.min)
            maxp = self._make_value(a.max)
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
          if other
            raise("No other layer must be specified for a single-layer check")
          end
        else
          if !other
            raise("The other layer must be specified for a two-layer check")
          end
        end
        
        DRCOpNodeCheck::new(self, :#{f}, other, *args)
        
      end  
CODE
    end
    
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
      _cop_separation(*args)
    end

  end

end

