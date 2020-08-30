# $autorun-early

module DRC

  # A single DRC layer which is either 
  # an edge pair, edge or region layer
  
  # %DRC%
  # @scope
  # @name Layer
  # @brief DRC Reference: Layer Object
  # The layer object represents a collection of polygons, edges or edge pairs.
  
  class DRCLayer
    
    def initialize(engine, data)
      @engine = engine
      @data = data
    end
    
    def data
      @data
    end

    # %DRC%
    # @name insert
    # @brief Inserts one or many objects into the layer
    # @synopsis insert(object, object ...)
    # 
    # Objects that can be inserted are RBA::Edge objects (into edge layers) or 
    # RBA::DPolygon, RBA::DSimplePolygon, RBA::Path, RBA::DBox (into polygon layers).
    # Convenience methods exist to create such objects (\global#edge, \global#polygon, \global#box and \#global#path).
    # However, RBA constructors can used as well.
    # 
    # The insert method is useful in combination with the \global#polygon_layer or \global#edge_layer functions: 
    #
    # @code
    # el = edge_layer
    # el.insert(edge(0.0, 0.0, 100.0, 0.0)
    #
    # pl = polygon_layer
    # pl.insert(box(0.0, 0.0, 100.0, 200.0)
    # @/code
    
    def insert(*args)
      requires_edges_or_region("insert")
      args.each do |a|
        if a.is_a?(RBA::DBox) 
          @data.insert(RBA::Box::from_dbox(a * (1.0 / @engine.dbu)))
        elsif a.is_a?(RBA::DPolygon) 
          @data.insert(RBA::Polygon::from_dpoly(a * (1.0 / @engine.dbu)))
        elsif a.is_a?(RBA::DSimplePolygon) 
          @data.insert(RBA::SimplePolygon::from_dpoly(a * (1.0 / @engine.dbu)))
        elsif a.is_a?(RBA::DPath) 
          @data.insert(RBA::Path::from_dpath(a * (1.0 / @engine.dbu)))
        elsif a.is_a?(RBA::DEdge) 
          @data.insert(RBA::Edge::from_dedge(a * (1.0 / @engine.dbu)))
        elsif a.is_a?(Array)
          insert(*a)
        else
          raise("Invalid argument type #{a.class.to_s} for 'insert' method")
        end
      end
      self
    end
    
    # %DRC%
    # @name strict
    # @brief Marks a layer for strict handling
    # @synopsis layer.strict
    # If a layer is marked for strict handling, some optimizations 
    # are disabled. Specifically for boolean operations, the results
    # will also be merged if one input is empty. 
    # For boolean operations, strict handling should be enabled for both inputs.
    # Strict handling is disabled by default.
    #
    # See \non_strict about how to reset this mode.
    # 
    # This feature has been introduced in version 0.23.2.
    
    def strict
      requires_region("strict")
      @data.strict_handling = true
      self
    end
    
    # %DRC%
    # @name non_strict
    # @brief Marks a layer for non-strict handling
    # @synopsis layer.non_strict
    #
    # See \strict for details about this option.
    # 
    # This feature has been introduced in version 0.23.2.
    
    def non_strict
      requires_region("non_strict")
      @data.strict_handling = false
      self
    end
    
    # %DRC% 
    # @name strict?
    # @brief Returns true, if strict handling is enabled for this layer
    # @synopsis layer.is_strict?
    #
    # See \strict for a discussion of strict handling.
    # 
    # This feature has been introduced in version 0.23.2.
    
    def is_strict?
      requires_region("is_strict?")
      @data.strict_handling?
    end
    
    # %DRC%
    # @name clean
    # @brief Marks a layer as clean
    # @synopsis layer.clean
    # A layer marked as clean will provide "merged" semantics, i.e.
    # overlapping or touching polygons are considered as single
    # polygons. Inner edges are removed and collinear edges are 
    # connected. 
    # Clean state is the default.
    #
    # See \raw for some remarks about how this state is 
    # propagated.
    
    def clean
      requires_edges_or_region("clean")
      @data.merged_semantics = true
      self
    end
    
    # %DRC%
    # @name raw
    # @brief Marks a layer as raw
    # @synopsis layer.raw
    # 
    # A raw layer basically is the opposite of a "clean" layer
    # (see \clean). Polygons on a raw layer are considered "as is", i.e.
    # overlapping polygons are not connected and inner edges may occur
    # due to cut lines. Holes may not exists if the polygons are derived
    # from a representation that does not allow holes (i.e. GDS2 files).
    # 
    # Note that this method will set the state of the layer. In combination
    # with the fact, that copied layers are references to the original layer,
    # this may lead to unexpected results:
    # 
    # @code
    #   l = ...
    #   l2 = l1
    #   ... do something
    #   l.raw
    #   # now l2 is also a raw layer
    # @/code
    #
    # To avoid that, use the \dup method to create a real (deep) copy.
    
    def raw
      requires_edges_or_region("raw")
      @data.merged_semantics = false
      self
    end
    
    # %DRC% 
    # @name is_clean?
    # @brief Returns true, if the layer is clean state
    # @synopsis layer.is_clean?
    #
    # See \clean for a discussion of the clean state.
    
    def is_clean?
      requires_edges_or_region("is_clean?")
      @data.merged_semantics?
    end
    
    # %DRC% 
    # @name is_raw?
    # @brief Returns true, if the layer is raw state
    # @synopsis layer.is_raw?
    #
    # See \clean for a discussion of the raw state.
    
    def is_raw?
      requires_edges_or_region("is_raw?")
      !@data.merged_semantics?
    end
    
    # %DRC%
    # @name size 
    # @brief Returns the number of objects on the layer
    # @synopsis layer.size
    #
    # The number of objects is the number of raw objects, not merged
    # regions or edges. It is more efficent to call this method on output layers than
    # on input layers.

    def size
      @data.size
    end
    
    # %DRC%
    # @name dup
    # @brief Duplicates a layer
    # @synopsis layer.dup
    # 
    # Duplicates the layer. This basically will create a copy and
    # modifications of the original layer will not affect the duplicate.
    # Please note that just assigning the layer to another variable will
    # not create a copy but rather a pointer to the original layer. Hence
    # modifications will then be visible on the original and derived 
    # layer. Using the dup method will avoid that.
    # 
    # However, dup will double the memory required to hold the data 
    # and performing the deep copy may be expensive in terms of CPU time.
    
    def dup
      DRCLayer::new(@engine, @data.dup)
    end

    # %DRC%
    # @name with_area
    # @brief Selects polygons by area
    # @synopsis layer.with_area(min .. max)
    # @synopsis layer.with_area(value)
    # @synopsis layer.with_area(min, max)
    # The first form will select all polygons with an area larger or
    # equal to min and less (but not equal to) max. The second form
    # will select the polygons with exactly the given area.
    # The third form basically is equivalent to the first form, but
    # allows specification of nil for min or max indicating no lower or 
    # upper limit.
    
    # %DRC%
    # @name without_area
    # @brief Selects polygons by area
    # @synopsis layer.without_area(min .. max)
    # @synopsis layer.without_area(value)
    # @synopsis layer.without_area(min, max)
    # This method is the inverse of "with_area". It will select 
    # polygons without an area equal to the given one or outside
    # the given interval.
    #
    # This method is available for polygon layers only.
    
    %w(area).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)
          requires_region("#{f}")
          if args.size == 1
            a = args[0]
            if a.is_a?(Range)
              DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :with_#{f}, @engine._prep_value_area(a.first), @engine._prep_value_area(a.last), #{inv.inspect}))
            else
              DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :with_#{f}, @engine._prep_value_area(a), #{inv.inspect}))
            end
          elsif args.size == 2
            DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :with_#{f}, @engine._prep_value_area(args[0]), @engine._prep_value_area(args[1]), #{inv.inspect}))
          else
            raise("Invalid number of arguments for method '#{mn}'")
          end
        end
CODE
      end
    end
    
    # %DRC%
    # @name with_perimeter
    # @brief Selects polygons by perimeter
    # @synopsis layer.with_perimeter(min .. max)
    # @synopsis layer.with_perimeter(value)
    # @synopsis layer.with_perimeter(min, max)
    # The first form will select all polygons with an perimeter larger or
    # equal to min and less (but not equal to) max. The second form
    # will select the polygons with exactly the given perimeter.
    # The third form basically is equivalent to the first form, but
    # allows specification of nil for min or max indicating no lower or 
    # upper limit.
    #
    # This method is available for polygon layers only.
    
    # %DRC%
    # @name without_perimeter
    # @brief Selects polygons by perimeter
    # @synopsis layer.without_perimeter(min .. max)
    # @synopsis layer.without_perimeter(value)
    # @synopsis layer.without_perimeter(min, max)
    # This method is the inverse of "with_perimeter". It will select 
    # polygons without a perimeter equal to the given one or outside
    # the given interval.
    #
    # This method is available for polygon layers only.
    
    # %DRC%
    # @name with_bbox_min
    # @brief Selects polygons by the minimum dimension of the bounding box
    # @synopsis layer.with_bbox_min(min .. max)
    # @synopsis layer.with_bbox_min(value)
    # @synopsis layer.with_bbox_min(min, max)
    # The method selects polygons similar to \with_area or \with_perimeter.
    # However, the measured dimension is the minimum dimension of the
    # bounding box. The minimum dimension is either the width or height of 
    # the bounding box, whichever is smaller.
    #
    # This method is available for polygon layers only.

    # %DRC%
    # @name without_bbox_min
    # @brief Selects polygons by the minimum dimension of the bounding box
    # @synopsis layer.without_bbox_min(min .. max)
    # @synopsis layer.without_bbox_min(value)
    # @synopsis layer.without_bbox_min(min, max)
    # The method selects polygons similar to \without_area or \without_perimeter.
    # However, the measured dimension is the minimum dimension of the
    # bounding box. The minimum dimension is either the width or height of 
    # the bounding box, whichever is smaller.
    #
    # This method is available for polygon layers only.
    
    # %DRC%
    # @name with_bbox_max
    # @brief Selects polygons by the maximum dimension of the bounding box
    # @synopsis layer.with_bbox_max(min .. max)
    # @synopsis layer.with_bbox_max(value)
    # @synopsis layer.with_bbox_max(min, max)
    # The method selects polygons similar to \with_area or \with_perimeter.
    # However, the measured dimension is the maximum dimension of the
    # bounding box. The maximum dimension is either the width or height of 
    # the bounding box, whichever is larger.
    #
    # This method is available for polygon layers only.

    # %DRC%
    # @name without_bbox_max
    # @brief Selects polygons by the maximum dimension of the bounding box
    # @synopsis layer.without_bbox_max(min .. max)
    # @synopsis layer.without_bbox_max(value)
    # @synopsis layer.without_bbox_max(min, max)
    # The method selects polygons similar to \without_area or \without_perimeter.
    # However, the measured dimension is the maximum dimension of the
    # bounding box. The minimum dimension is either the width or height of 
    # the bounding box, whichever is larger.
    #
    # This method is available for polygon layers only.
    
    # %DRC%
    # @name with_bbox_width
    # @brief Selects polygons by the width of the bounding box
    # @synopsis layer.with_bbox_width(min .. max)
    # @synopsis layer.with_bbox_width(value)
    # @synopsis layer.with_bbox_width(min, max)
    # The method selects polygons similar to \with_area or \with_perimeter.
    # However, the measured dimension is the width of the
    # bounding box. 
    #
    # This method is available for polygon layers only.

    # %DRC%
    # @name without_bbox_width
    # @brief Selects polygons by the width of the bounding box
    # @synopsis layer.without_bbox_width(min .. max)
    # @synopsis layer.without_bbox_width(value)
    # @synopsis layer.without_bbox_width(min, max)
    # The method selects polygons similar to \without_area or \without_perimeter.
    # However, the measured dimension is the width of the
    # bounding box.
    #
    # This method is available for polygon layers only.
    
    # %DRC%
    # @name with_bbox_height
    # @brief Selects polygons by the height of the bounding box
    # @synopsis layer.with_bbox_height(min .. max)
    # @synopsis layer.with_bbox_height(value)
    # @synopsis layer.with_bbox_height(min, max)
    # The method selects polygons similar to \with_area or \with_perimeter.
    # However, the measured dimension is the width of the
    # bounding box. 
    #
    # This method is available for polygon layers only.

    # %DRC%
    # @name without_bbox_height
    # @brief Selects polygons by the height of the bounding box
    # @synopsis layer.without_bbox_height(min .. max)
    # @synopsis layer.without_bbox_height(value)
    # @synopsis layer.without_bbox_height(min, max)
    # The method selects polygons similar to \without_area or \without_perimeter.
    # However, the measured dimension is the width of the
    # bounding box.
    #
    # This method is available for polygon layers only.
    
    %w(bbox_height bbox_max bbox_min bbox_width perimeter).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)
          requires_region("#{mn}")
          if args.size == 1
            a = args[0]
            if a.is_a?(Range)
              DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :with_#{f}, @engine._prep_value(a.first), @engine._prep_value(a.last), #{inv.inspect}))
            else
              DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :with_#{f}, @engine._prep_value(a), #{inv.inspect}))
            end
          elsif args.size == 2
            DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :with_#{f}, @engine._prep_value(args[0]), @engine._prep_value(args[1]), #{inv.inspect}))
          else
            raise("Invalid number of arguments for method '#{mn}'")
          end
        end
CODE
      end
    end
    
    # %DRC%
    # @name with_length
    # @brief Selects edges by their length
    # @synopsis layer.with_length(min .. max)
    # @synopsis layer.with_length(value)
    # @synopsis layer.with_length(min, max)
    # The method selects edges by their length. The first version selected
    # edges with a length larger or equal to min and less than max (but not equal).
    # The second version selects edges with exactly the given length. The third
    # version is similar to the first one, but allows specification of nil for min or
    # max indicating that there is no lower or upper limit. 
    #
    # This method is available for edge layers only.

    # %DRC%
    # @name without_length
    # @brief Selects edges by the their length
    # @synopsis layer.without_length(min .. max)
    # @synopsis layer.without_length(value)
    # @synopsis layer.without_length(min, max)
    # The method basically is the inverse of \with_length. It selects all edges
    # of the edge layer which do not have the given length (second form) or are
    # not inside the given interval (first and third form).
    #
    # This method is available for edge layers only.
    
    %w(length).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)
          requires_edges("#{mn}")
          if args.size == 1
            a = args[0]
            if a.is_a?(Range)
              DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Edges, :with_#{f}, @engine._prep_value(a.first), @engine._prep_value(a.last), #{inv.inspect}))
            else
              DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Edges, :with_#{f}, @engine._prep_value(a), #{inv.inspect}))
            end
          elsif args.size == 2
            DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Edges, :with_#{f}, @engine._prep_value(args[0]), @engine._prep_value(args[1]), #{inv.inspect}))
          else
            raise("Invalid number of arguments for method '#{mn}'")
          end
        end
CODE
      end
    end
    
    # %DRC%
    # @name with_angle
    # @brief Selects edges by their angle
    # @synopsis layer.with_angle(min .. max)
    # @synopsis layer.with_angle(value)
    # @synopsis layer.with_angle(min, max)
    #
    # When called on an edge layer, the method selects edges by their angle, 
    # measured against the horizontal axis in the mathematical sense. 
    # The first version selects
    # edges with a angle larger or equal to min and less than max (but not equal).
    # The second version selects edges with exactly the given angle. The third
    # version is identical to the first one. 
    #
    # When called on a polygon layer, this method selects corners which match the 
    # given angle or is within the given angle interval. The angle is measured between the edges forming the corner.
    # For each corner, an edge pair containing the edges forming in the angle is returned.
    #
    # A method delivering all objects not matching the angle criterion is \without_angle.
    #
    # The following images demonstrate some use cases of \with_angle and \without_angle:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_with_angle1.png) @/td
    #     @td @img(/images/drc_with_angle2.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_with_angle3.png) @/td
    #     @td @img(/images/drc_with_angle4.png) @/td
    #   @/tr
    # @/table


    # %DRC%
    # @name without_angle
    # @brief Selects edges by the their angle
    # @synopsis layer.without_angle(min .. max)
    # @synopsis layer.without_angle(value)
    # @synopsis layer.without_angle(min, max)
    #
    # The method basically is the inverse of \with_angle. It selects all edges
    # of the edge layer or corners of the polygons which do not have the given angle (second form) or whose angle
    # is not inside the given interval (first and third form).
    
    [true, false].each do |inv|
      mn = (inv ? "without" : "with") + "_angle"
      eval <<"CODE"
      def #{mn}(*args)
        requires_edges_or_region("#{mn}")
        result_class = @data.is_a?(RBA::Region) ? RBA::EdgePairs : RBA::Edges
        if args.size == 1
          a = args[0]
          if a.is_a?(Range)
            DRCLayer::new(@engine, @engine._tcmd(@data, 0, result_class, :with_angle, a.first, a.last, #{inv.inspect}))
          else
            DRCLayer::new(@engine, @engine._tcmd(@data, 0, result_class, :with_angle, a, #{inv.inspect}))
          end
        elsif args.size == 2
          DRCLayer::new(@engine, @engine._tcmd(@data, 0, result_class, :with_angle, args[0], args[1], #{inv.inspect}))
        else
          raise("Invalid number of arguments for method '#{mn}'")
        end
      end
CODE
    end
    
    # %DRC%
    # @name rounded_corners
    # @brief Applies corner rounding to each corner of the polygon
    # @synopsis layer.rounded_corners(inner, outer, n)
    #
    # Inner (concave) corners are replaced by circle segments with a radius given by the 
    # "inner" parameter. Outer (convex) corners are relaced by circle segments with a radius
    # given by the "outer" parameter. 
    #
    # The circles are approximated by polygons. "n" segments are used to approximate a full circle.
    #
    # This method return a layer wit the modified polygons. Merged semantics applies for this
    # method (see \raw and \clean).
    # If used with tiling, the rounded_corners function may render invalid results because
    # in tiling mode, not the whole merged region may be captured. In that case, inner
    # edges may appear as outer ones and their corners will receive rounding.
    #
    # The following image shows the effect of the "rounded_corners" method. The upper ends of 
    # the vertical bars are rounded with a smaller radius automatically because their width does not allow
    # a larger radius.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_rounded_corners.png) @/td
    #   @/tr
    # @/table
    
    def rounded_corners(inner, outer, n)
      requires_region("rounded_corners")
      DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :rounded_corners, @engine._prep_value(inner), @engine._prep_value(outer), n))
    end
    
    # %DRC%
    # @name smoothed
    # @brief Smoothes the polygons of the region
    # @synopsis layer.smoothed(d)
    #
    # "Smoothing" returns a simplified version of the polygons. Simplification is 
    # achieved by removing vertices unless the resulting polygon deviates by more
    # than the given distance d from the original polygon.
    #
    # This method return a layer wit the modified polygons. Merged semantics applies for this
    # method (see \raw and \clean).
    
    def smoothed(d)
      requires_region("smoothed")
      DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :smoothed, @engine._prep_value(d)))
    end
    
    # %DRC%
    # @name texts
    # @brief Selects texts from an original layer
    # @synopsis layer.texts
    # @synopsis layer.texts(p)
    # @synopsis layer.texts([ options ])
    # This method can be applied to original layers - i.e. ones that have
    # been created with \input. By default, a small box (2x2 DBU) will be produced on each
    # selected text. By using the "as_dots" option, degenerated point-like edges will be
    # produced.
    #
    # The preferred method however is to use true text layers created with \labels.
    # In this case, without specifying "as_dots" or "as_boxes" retains the text
    # objects as such a text filtering is applied. In contrast to this, layers generated
    # with \input cannot maintain the text nature of the selected objects and 
    # produce dots or small polygon boxes in the \texts method.
    #
    # Texts can be selected either by exact match string or a pattern match with a 
    # glob-style pattern. By default, glob-style pattern are used. 
    # The options available are:
    #
    # @ul
    #   @li @b pattern(p) @/b: Use a pattern to match the string (this is the default) @/li  
    #   @li @b text(s) @/b: Select the texts that exactly match the given string @/li  
    #   @li @b as_boxes @/b: with this option, small boxes will be produced as markers @/li  
    #   @li @b as_dots @/b: with this option, point-like edges will be produced instead of small boxes @/li  
    # @/ul
    #
    # Here are some examples:
    #
    # @code
    #   # Selects all texts
    #   t = labels(1, 0).texts
    #   # Selects all texts beginning with an "A"
    #   t = labels(1, 0).texts("A*")
    #   t = labels(1, 0).texts(pattern("A*"))
    #   # Selects all texts whose string is "ABC"
    #   t = labels(1, 0).texts(text("ABC"))
    # @/code
    #
    # The effect of the operation is shown in these examples:
    #  
    # @table
    #   @tr 
    #     @td @img(/images/drc_texts1.png) @/td
    #     @td @img(/images/drc_texts2.png) @/td
    #   @/tr
    # @/table
     
    def texts(*args)
      requires_texts_or_region("texts")
      self._texts_impl(false, *args)
    end

    # %DRC%
    # @name texts_not
    # @brief Selects texts from an original layer not matching a specific selection
    # @synopsis layer.texts_not
    # @synopsis layer.texts_not(p)
    # @synopsis layer.texts_not([ options ])
    #
    # This method can be applied to true text layers obtained with \labels.
    # In this case, without specifying "as_dots" or "as_boxes" retains the text
    # objects as such. Only text filtering is applied.
    #
    # Beside that this method acts like \texts, but will select the text objects
    # not matching the filter.

    def texts_not(*args)
      requires_texts("texts_not")
      self._texts_impl(true, *args)
    end

    # Implementation of texts or texts_not

    def _texts_impl(invert, *args)

      as_pattern = true
      pattern = "*"
      as_dots = nil

      args.each do |a|
        if a.is_a?(String)
          as_pattern = true
          pattern = a
        elsif a.is_a?(DRCPattern)
          as_pattern = a.as_pattern
          pattern = a.pattern
        elsif a.is_a?(DRCAsDots)
          as_dots = a.value
        else
          raise("Invalid argument for 'texts' method")
        end
      end

      if @data.is_a?(RBA::Texts)
        if as_pattern
          result = @engine._tcmd(@data, 0, RBA::Texts, :with_match, pattern, invert)
        else
          result = @engine._tcmd(@data, 0, RBA::Texts, :with_text, pattern, invert)
        end
        if as_dots
          return DRCLayer::new(@engine, @engine._tcmd(result, 0, RBA::Region, :edges))
        elsif as_dots == false
          return DRCLayer::new(@engine, @engine._tcmd(result, 0, RBA::Region, :polygons))
        else
          return DRCLayer::new(@engine, result)
        end
      else    
        if as_dots
          return DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :texts_dots, pattern, as_pattern))
        else
          return DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :texts, pattern, as_pattern))
        end
      end

    end
     
    # %DRC%
    # @name corners
    # @brief Selects corners of polygons
    # @synopsis layer.corners([ options ])
    # @synopsis layer.corners(angle, [ options ])
    # @synopsis layer.corners(amin .. amax, [ options ])
    #
    # This method produces markers on the corners of the polygons. An angle criterion can be given which
    # selects corners based on the angle of the connecting edges. Positive angles indicate a left turn
    # while negative angles indicate a right turn. Since polygons are oriented clockwise, positive angles
    # indicate concave corners while negative ones indicate convex corners.
    # 
    # The markers generated can be point-like edges or small 2x2 DBU boxes. The latter is the default.
    # 
    # The options available are:
    #
    # @ul
    #   @li @b as_boxes @/b: with this option, small boxes will be produced as markers @/li  
    #   @li @b as_dots @/b: with this option, point-like edges will be produced instead of small boxes @/li  
    # @/ul
    #
    # The following images show the effect of this method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_corners1.png) @/td
    #     @td @img(/images/drc_corners2.png) @/td
    #     @td @img(/images/drc_corners3.png) @/td
    #   @/tr
    # @/table

    def corners(*args)

      requires_region("corners")

      as_dots = false
      amin = -180.0
      amax = 180.0

      args.each do |a|
        if a.is_a?(Range)
          if (!a.min.is_a?(1.0.class) && !a.min.is_a?(1.class)) || (!a.max.is_a?(1.0.class) && !a.max.is_a?(1.class))
            raise("An angle limit requires an interval of two angles")
          end
          amin = a.min.to_f
          amax = a.max.to_f
        elsif a.is_a?(1.0.class) || a.is_a?(1.class)
          amin = a.to_f
          amax = a.to_f
        elsif a.is_a?(DRCAsDots)
          as_dots = a.value
        else
          raise("Invalid argument for 'corners' method")
        end
      end

      DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, as_dots ? :corners_dots : :corners, amin, amax))

    end

    # %DRC%
    # @name middle
    # @brief Returns the center points of the bounding boxes of the polygons
    # @synopsis layer.middle([ options ])
    #
    # This method produces markers on the centers of the polygon's bounding box centers. 
    # These markers can be point-like edges or small 2x2 DBU boxes. The latter is the default.
    # A more generic function is \extent_refs. "middle" is basically a synonym for "extent_refs(:center)".
    # 
    # The options available are:
    #
    # @ul
    #   @li @b as_boxes @/b: with this option, small boxes will be produced as markers @/li  
    #   @li @b as_dots @/b: with this option, point-like edges will be produced instead of small boxes @/li  
    # @/ul
    #
    # The following image shows the effect of this method
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_middle1.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name extent_refs
    # @brief Returns partial references to the boundings boxes of the polygons
    # @synopsis layer.extent_refs(fx, fy [, options ])
    # @synopsis layer.extent_refs(fx1, fy1, fx2, fx2 [, options ])
    # @synopsis layer.extent_refs(ref_spec [, options ])
    #
    # This method produces parts of the bounding box of the polygons. It can select 
    # either edges, certain points or partial boxes. It can be used the following
    # ways:
    #  
    # @ul
    #   @li @b With a formal specification @/b: This is an identifier like
    #     ":center" or ":left" to indicate which part will be produced. @/li
    #   @li @b With two floating-point arguments @/b: These arguments specify
    #     a point relative to the bounding box. The first argument is a relative
    #     x coordinate where 0.0 means "left side of the bounding box" and 1.0
    #     is the right side. The second argument is a relative y coordinate where
    #     0.0 means "bottom" and 1.0 means "top". The results will be small 
    #     (2x2 DBU) boxes or point-like edges for edge output @/li
    #   @li @b With four floating-point arguments @/b: These arguments specify
    #     a box in relative coordinates: a pair of x/y relative coordinate for
    #     the first point and another pair for the second point. The results will
    #     be boxes or a tilted edge in case of edge output. If the range specifies
    #     a finite-area box (height and width are not zero), no adjustment of 
    #     the boxes will happen for polygon output - i.e. the additional enlargement 
    #     by 1 DBU which is applied for zero-area boxes does not happen.@/li
    # @/ul
    # 
    # The formal specifiers are for points:
    # 
    # @ul
    #   @li @b :center @/b or @b :c @/b: the center point @/li
    #   @li @b :bottom_center @/b or @b :bc @/b: the bottom center point @/li
    #   @li @b :bottom_left @/b or @b :bl @/b: the bottom left point @/li
    #   @li @b :bottom_right @/b or @b :br @/b: the bottom right point @/li
    #   @li @b :left @/b or @b :l @/b: the left point @/li
    #   @li @b :right @/b or @b :r @/b: the right point @/li
    #   @li @b :top_center @/b or @b :tc @/b: the top center point @/li
    #   @li @b :top_left @/b or @b :tl @/b: the top left point @/li
    #   @li @b :top_right @/b or @b :tr @/b: the top right point @/li
    # @/ul
    #
    # The formal specifiers for lines are:
    # 
    # @ul
    #   @li @b :bottom @/b or @b :b @/b: the bottom line @/li
    #   @li @b :top @/b or @b :t @/b: the top line @/li
    #   @li @b :left @/b or @b :l @/b: the left line @/li
    #   @li @b :right @/b or @b :r @/b: the right line @/li
    # @/ul
    #
    # Dots are represented by small (2x2 DBU) boxes or point-like
    # edges with edge output. Lines are represented by narrow or 
    # flat (2 DBU) boxes or edges for edge output. Edges will follow
    # the orientation convention for the corresponding edges - i.e.
    # "inside" of the bounding box is on the right side of the edge.
    #
    # The following additional option controls the output format:
    #
    # @ul
    #   @li @b as_boxes @/b: with this option, small boxes will be produced as markers @/li  
    #   @li @b as_dots @/b or @b as_edges @/b: with this option, point-like edges will be produced for dots
    #                         and edges will be produced for line-like selections @/li  
    # @/ul
    #
    # The following table shows a few applications:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_extent_refs1.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_extent_refs10.png) @/td
    #     @td @img(/images/drc_extent_refs11.png) @/td
    #     @td @img(/images/drc_extent_refs12.png) @/td
    #     @td @img(/images/drc_extent_refs13.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_extent_refs20.png) @/td
    #     @td @img(/images/drc_extent_refs21.png) @/td
    #     @td @img(/images/drc_extent_refs22.png) @/td
    #     @td @img(/images/drc_extent_refs23.png) @/td
    #     @td @img(/images/drc_extent_refs24.png) @/td
    #     @td @img(/images/drc_extent_refs25.png) @/td
    #     @td @img(/images/drc_extent_refs26.png) @/td
    #     @td @img(/images/drc_extent_refs27.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_extent_refs30.png) @/td
    #     @td @img(/images/drc_extent_refs31.png) @/td
    #   @/tr
    # @/table

    %w(middle extent_refs).each do |f| 
      eval <<"CODE"
      def #{f}(*args)

        requires_region("#{f}")

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

        args.each do |a|
          if a.is_a?(1.0.class) && :#{f} != :middle
            f << a 
          elsif a.is_a?(DRCAsDots)
            as_edges = a.value
          elsif @@std_refs[a] && :#{f} != :middle
            f = @@std_refs[a]
          else
            raise("Invalid argument for '#{f}' method")
          end
        end

        if f.size == 2
          f = f + f
        else
          f = (f + [0.5] * 4)[0..3]
        end
            
        if as_edges
          DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :extent_refs_edges, *f))
        else
          # add oversize for point- and edge-like regions
          zero_area = (f[0] - f[2]).abs < 1e-7 || (f[1] - f[3]).abs < 1e-7
          f += [ zero_area ? 1 : 0 ] * 2
          DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :extent_refs, *f))
        end

      end
CODE
    end

    # %DRC%
    # @name select
    # @brief Selects edges, edge pairs or polygons based on evaluation of a block
    # @synopsis layer.select { |object| ... }
    # This method evaluates the block and returns a new container with those objects for which
    # the block evaluates to true. It is available for edge, polygon and edge pair layers.
    # The corresponding objects are RBA::DPolygon, RBA::DEdge or RBA::DEdgePair.
    #
    # Because this method executes inside the interpreter, it's inherently slow. Tiling does not
    # apply to this method.
    #
    # Here is a (slow) equivalent of the area selection method:
    #
    # @code
    # new_layer = layer.select { |polygon| polygon.area >= 10.0 }
    # @/code
  
    def select(&block)
      new_data = @data.class.new
      t = RBA::CplxTrans::new(@engine.dbu)
      @engine.run_timed("\"select\" in: #{@engine.src_line}", @data) do
        @data.send(new_data.is_a?(RBA::EdgePairs) ? :each : :each_merged) do |object| 
          block.call(object.transformed(t)) && new_data.insert(object)
        end
      end
      DRCLayer::new(@engine, new_data)
    end
    
    # %DRC%
    # @name each
    # @brief Iterates over the objects from the layer
    # @synopsis layer.each { |object| ... }
    # This method evaluates the block on each object of the layer. Depending on the
    # layer type, these objects are of RBA::DPolygon, RBA::DEdge or RBA::DEdgePair type.
    #
    # Because this method executes inside the interpreter, it's inherently slow. Tiling does not
    # apply to this method.
  
    def each(&block)
      t = RBA::CplxTrans::new(@engine.dbu)
      @engine.run_timed("\"select\" in: #{@engine.src_line}", @data) do
        @data.send(@data.is_a?(RBA::EdgePairs) ? :each : :each_merged) do |object| 
          block.call(object.transformed(t))
        end
      end
    end
    
    # %DRC%
    # @name collect
    # @brief Transforms a layer
    # @synopsis layer.collect { |object| ... }
    # This method evaluates the block for each object in the layer and returns a new layer with the objects
    # returned from the block. It is available for edge, polygon and edge pair layers.
    # The corresponding objects are RBA::DPolygon, RBA::DEdge or RBA::DEdgePair.
    #
    # If the block evaluates to nil, no object is added to the output layer. If it returns an array, each of
    # the objects in the array is added.
    # The returned layer is of the original type and will only accept objects of the respective type. Hence,
    # for polygon layers, RBA::DPolygon objects need to be returned. For edge layers those need to be RBA::DEdge
    # and for edge pair layers, they need to be RBA::DEdgePair objects. For convenience, RBA::Polygon, RBA::Edge
    # and RBA::EdgePair objects are accepted too and are scaled by the database unit to render micrometer-unit
    # objects. RBA::Region, RBA::Edges and RBA::EdgePair objects are accepted as well and the corresponding 
    # content of that collections is inserted into the output layer.
    #
    # Other versions are available that allow translation of objects into other types (\collect_to_polygons, 
    # \collect_to_edges and \collect_to_edge_pairs).
    #
    # Because this method executes inside the interpreter, it's inherently slow. Tiling does not
    # apply to this method.
    #
    # Here is a slow equivalent of the rotated method
    #
    # @code
    # # Rotates by 45 degree
    # t = RBA::DCplxTrans(1.0, 45.0, false, RBA::DVector::new)
    # new_layer = layer.collect { |polygon| polygon.transformed(t) }
    # @/code

    # %DRC%
    # @name collect_to_region
    # @brief Transforms a layer into polygon objects
    # @synopsis layer.collect { |object| ... }
    # This method is similar to \collect, but creates a polygon layer. It expects the block to 
    # deliver objects that can be converted into polygons. Such objects are of class RBA::DPolygon,
    # RBA::DBox, RBA::DPath, RBA::Polygon, RBA::Path, RBA::Box and RBA::Region.
    
    # %DRC%
    # @name collect_to_edges
    # @brief Transforms a layer into edge objects
    # @synopsis layer.collect { |object| ... }
    # This method is similar to \collect, but creates an edge layer. It expects the block to 
    # deliver objects that can be converted into edges. If polygon-like objects are returned, their
    # contours will be turned into edge sequences.
    
    # %DRC%
    # @name collect_to_edge_pairs
    # @brief Transforms a layer into edge pair objects
    # @synopsis layer.collect { |object| ... }
    # This method is similar to \collect, but creates an edge pair layer. It expects the block to 
    # deliver RBA::EdgePair, RBA::DEdgePair or RBA::EdgePairs objects.
    
    %w(collect collect_to_region collect_to_edges collect_to_edge_pairs).each do |f| 
      eval <<"CODE"
      def #{f}(&block)

        if :#{f} == :collect
          new_data = @data.class.new
        elsif :#{f} == :collect_to_region
          new_data = RBA::Region.new
        elsif :#{f} == :collect_to_edges
          new_data = RBA::Edges.new
        elsif :#{f} == :collect_to_edge_pairs
          new_data = RBA::EdgePairs.new
        end

        t = RBA::CplxTrans::new(@engine.dbu)
        dbu_trans = RBA::VCplxTrans::new(1.0 / @engine.dbu)

        @engine.run_timed("\\"select\\" in: " + @engine.src_line, @data) do
          @data.send(new_data.is_a?(RBA::EdgePairs) ? :each : :each_merged) do |object| 
            insert_object_into(new_data, block.call(object.transformed(t)), dbu_trans)
          end
        end

        DRCLayer::new(@engine, new_data)

      end
CODE
    end
    
    # %DRC%
    # @name odd_polygons
    # @brief Checks for odd polygons (self-overlapping, non-orientable)
    # @synopsis layer.odd_polygons
    # Returns the parts of the polygons which are not orientable (i.e. "8" configuration) or self-overlapping.
    # Merged semantics does not apply for this method. Always the raw polygons are taken (see \raw).
    
    def odd_polygons
      requires_region("ongrid")
      DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :strange_polygon_check))
    end
    
    # %DRC%
    # @name ongrid
    # @brief Checks for on-grid vertices
    # @synopsis layer.ongrid(g)
    # @synopsis layer.ongrid(gx, gy)
    # Returns a single-vertex marker for each vertex whose x coordinate is not a
    # multiple of g or gx or whose y coordinate is not a multiple of g or gy.
    # The single-vertex markers are edge pair objects which describe a single point.
    # When setting the grid to 0, no grid check is performed in that specific direction.
    # 
    # This method requires a polygon layer. Merged semantics applies (see \raw and \clean).
    
    def ongrid(*args)
      requires_region("ongrid")
      if args.size == 1
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::EdgePairs, :grid_check, @engine._prep_value(args[0]), @engine._prep_value(args[0])))
      elsif args.size == 2
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::EdgePairs, :grid_check, @engine._prep_value(args[0]), @engine._prep_value(args[1])))
      else
        raise("Invalid number of arguments for method 'ongrid'")
      end
    end
    
    # %DRC%
    # @name snap
    # @brief Brings each vertex on the given grid (g or gx/gy for x or y direction)
    # @synopsis layer.snap(g)
    # @synopsis layer.snap(gx, gy)
    # Shifts each off-grid vertex to the nearest on-grid location. If one grid is given, this
    # grid is applied to x and y coordinates. If two grids are given, gx is applied to the x
    # coordinates and gy is applied to the y coordinates. If 0 is given as a grid, no snapping
    # is performed in that direction. 
    # 
    # This method modifies the layer. A version that returns a snapped version of the layer
    # without modifying the layer is \snapped.
    # 
    # This method requires a polygon layer. Merged semantics applies (see \raw and \clean).
    
    # %DRC%
    # @name snapped
    # @brief Returns a snapped version of the layer
    # @synopsis layer.snapped(g)
    # @synopsis layer.snapped(gx, gy)
    # See \snap for a description of the functionality. In contrast to \snap, this method does
    # not modify the layer but returns a snapped copy.
    
    %w(snap snapped).each do |f| 
      eval <<"CODE"
      def #{f}(*args)
        requires_region("#{f}")
        gx = gy = 0
        if args.size == 1
          gx = gy = @engine._prep_value(args[0])
        elsif args.size == 2
          gx = @engine._prep_value(args[0])
          gy = @engine._prep_value(args[1])
        else
          raise("Invalid number of arguments for method 'ongrid'")
        end
        aa = args.collect { |a| @engine._prep_value(a) }
        if :#{f} == :snap && @engine.is_tiled?
          # in tiled mode, no modifying versions are available
          @data = @engine._tcmd(@data, 0, @data.class, :snapped, gx, gy)
          self
        elsif :#{f} == :snap
          @engine._tcmd(@data, 0, @data.class, :#{f}, gx, gy)
          self
        else
          DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :#{f}, gx, gy))
        end
      end
CODE
    end
    
    # %DRC%
    # @name and
    # @brief Boolean AND operation
    # @synopsis layer.and(other)
    # The method computes a boolean AND between self and other.
    # It is an alias for the "&" operator.
    #
    # This method is available for polygon and edge layers.
    # If the first operand is an edge layer and the second is a polygon layer, the
    # result will be the edges of the first operand which are inside or on the
    # borders of the polygons of the second operand.
    #
    # The following images show the effect of the "and" method
    # on polygons and edges (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_and1.png) @/td
    #     @td @img(/images/drc_and2.png) @/td
    #     @td @img(/images/drc_and3.png) @/td
    #   @/tr
    # @/table
    #
    # The AND operation can be applied between a text and a polygon
    # layer. In this case, the texts inside or at the border of the 
    # polygons will be written to the output (labels: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_textpoly1.png) @/td
    #   @/tr
    # @/table
    
    def and(other)
      self & other    
    end
    
    # %DRC%
    # @name not
    # @brief Boolean NOT operation
    # @synopsis layer.not(other)
    # The method computes a boolean NOT between self and other.
    # It is an alias for the "-" operator.
    #
    # This method is available for polygon and edge layers.
    # If the first operand is an edge layer and the second is an edge layer, the
    # result will be the edges of the first operand which are outside the polygons
    # of the second operand.
    #
    # The following images show the effect of the "not" method
    # on polygons and edges (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_not1.png) @/td
    #     @td @img(/images/drc_not2.png) @/td
    #     @td @img(/images/drc_not3.png) @/td
    #   @/tr
    # @/table
    #
    # The NOT operation can be applied between a text and a polygon
    # layer. In this case, the texts outside the polygons will be 
    # written to the output (labels: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_textpoly2.png) @/td
    #   @/tr
    # @/table
    
    def not(other)
      self - other    
    end
    
    # %DRC%
    # @name xor
    # @brief Boolean XOR operation
    # @synopsis layer.xor(other)
    # The method computes a boolean XOR between self and other.
    # It is an alias for the "^" operator.
    #
    # This method is available for polygon and edge layers.
    #
    # The following images show the effect of the "xor" method
    # on polygons and edges (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_xor1.png) @/td
    #     @td @img(/images/drc_xor2.png) @/td
    #   @/tr
    # @/table
    
    def xor(other)
      self ^ other    
    end
    
    # %DRC%
    # @name or
    # @brief Boolean OR operation
    # @synopsis layer.or(other)
    # The method computes a boolean OR between self and other.
    # It is an alias for the "|" operator.
    #
    # This method is available for polygon and edge layers.
    #
    # The following images show the effect of the "or" method
    # on polygons and edges (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_or1.png) @/td
    #     @td @img(/images/drc_or2.png) @/td
    #   @/tr
    # @/table
    
    def or(other)
      self | other    
    end
    
    # %DRC%
    # @name join
    # @brief Joins the layer with another layer
    # @synopsis layer.join(other)
    # The method includes the edges or polygons from the other layer into this layer.
    # It is an alias for the "+" operator.
    #
    # This method is available for polygon, edge and edge pair layers.
    #
    # The following images show the effect of the "join" method
    # on polygons and edges (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_join1.png) @/td
    #     @td @img(/images/drc_join2.png) @/td
    #   @/tr
    # @/table
    
    def join(other)
      self + other
    end
    
    # %DRC%
    # @name &
    # @brief Boolean AND operation
    # @synopsis self & other
    # The method computes a boolean AND between self and other.
    #
    # This method is available for polygon and edge layers. An alias
    # is "\and". See there for a description of the function.
    
    # %DRC%
    # @name |
    # @brief Boolean OR operation
    # @synopsis self | other
    # The method computes a boolean OR between self and other. A similar
    # operation is \join which will basically gives the same result but
    # won't merge the shapes.
    #
    # This method is available for polygon and edge layers. An alias
    # is "\or". See there for a description of the function.
    
    # %DRC%
    # @name ^
    # @brief Boolean XOR operation
    # @synopsis self ^ other
    # The method computes a boolean XOR between self and other.
    #
    # This method is available for polygon and edge layers. An alias
    # is "\xor". See there for a description of the function.
    
    # %DRC%
    # @name -
    # @brief Boolean NOT operation
    # @synopsis self - other
    # The method computes a boolean NOT between self and other.
    #
    # This method is available for polygon and edge layers. An alias
    # is "\not". See there for a description of the function.
    
    # %DRC%
    # @name +
    # @brief Join layers
    # @synopsis self + other
    # The method includes the edges or polygons from the other layer into this layer.
    # The "+" operator is an alias for the \join method.
    #
    # This method is available for polygon, edge and edge pair layers. An alias
    # is "\join". See there for a description of the function.
   
    # %DRC%
    # @name overlapping
    # @brief Selects shapes or regions of self which overlap shapes from the other region
    # @synopsis layer.overlapping(other)
    # This method selects all shapes or regions from self which overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_overlapping.
    #
    # This method is available for polygon and edge layers. Edges can be selected
    # with respect to other edges or polygons.
    #
    # The following image shows the effect of the "overlapping" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_overlapping.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name not_overlapping
    # @brief Selects shapes or regions of self which do not overlap shapes from the other region
    # @synopsis layer.not_overlapping(other)
    # This method selects all shapes or regions from self which do not overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    #
    # The "not_overlapping" method is equivalent to the \outside method. It is provided
    # as an alias for consistency.
    #
    # This method is available for polygon and edge layers. Edges can be selected
    # with respect to other edges or polygons.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_not_overlapping.
    
    # %DRC%
    # @name select_overlapping
    # @brief Selects shapes or regions of self which overlap shapes from the other region
    # @synopsis layer.select_overlapping(other)
    # This method selects all shapes or regions from self which overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \overlapping.
    #
    # This method is available for polygon and edge layers. Edges can be selected
    # with respect to other edges or polygons.
    
    # %DRC%
    # @name select_not_overlapping
    # @brief Selects shapes or regions of self which do not overlap shapes from the other region
    # @synopsis layer.select_not_overlapping(other)
    # This method selects all shapes or regions from self which do not overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected. 
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \not_overlapping.
    #
    # This method is available for polygon and edge layers. Edges can be selected
    # with respect to other edges or polygons.
    
    # %DRC%
    # @name inside
    # @brief Selects shapes or regions of self which are inside the other region
    # @synopsis layer.inside(other)
    # This method selects all shapes or regions from self which are inside the other region.
    # completely (completely covered by polygons from the other region). If self is
    # in raw mode, this method will select individual shapes. Otherwise, this method
    # will select coherent regions and no part of these regions may be outside the 
    # other region.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_inside.
    #
    # This method is available for polygon layers.
    #
    # The following image shows the effect of the "inside" method (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_inside.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name not_inside
    # @brief Selects shapes or regions of self which are not inside the other region
    # @synopsis layer.not_inside(other)
    # This method selects all shapes or regions from self which are not inside the other region.
    # completely (completely covered by polygons from the other region). If self is
    # in raw mode, this method will select individual shapes. Otherwise, this method
    # will select coherent regions and no part of these regions may be outside the 
    # other region.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_not_inside.
    #
    # This method is available for polygon layers.
    #
    # The following image shows the effect of the "not_inside" method (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_not_inside.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name select_inside
    # @brief Selects shapes or regions of self which are inside the other region
    # @synopsis layer.select_inside(other)
    # This method selects all shapes or regions from self which are inside the other region.
    # completely (completely covered by polygons from the other region). If self is
    # in raw mode, this method will select individual shapes. Otherwise, this method
    # will select coherent regions and no part of these regions may be outside the 
    # other region.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \inside.
    #
    # This method is available for polygon layers.
    
    # %DRC%
    # @name select_not_inside
    # @brief Selects shapes or regions of self which are not inside the other region
    # @synopsis layer.select_not_inside(other)
    # This method selects all shapes or regions from self which are not inside the other region.
    # completely (completely covered by polygons from the other region). If self is
    # in raw mode, this method will select individual shapes. Otherwise, this method
    # will select coherent regions and no part of these regions may be outside the 
    # other region.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \not_inside.
    #
    # This method is available for polygon layers.
    
    # %DRC%
    # @name outside
    # @brief Selects shapes or regions of self which are outside the other region
    # @synopsis layer.outside(other)
    # This method selects all shapes or regions from self which are completely outside 
    # the other region (no part of these shapes or regions may be covered by shapes from
    # the other region). If self is in raw mode, this method will select individual 
    # shapes. Otherwise, this method will select coherent regions and no part of these 
    # regions may overlap with shapes from the other region.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_outside.
    #
    # This method is available for polygon layers.
    #
    # The following image shows the effect of the "outside" method (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_outside.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name not_outside
    # @brief Selects shapes or regions of self which are not outside the other region
    # @synopsis layer.not_outside(other)
    # This method selects all shapes or regions from self which are not completely outside 
    # the other region (part of these shapes or regions may be covered by shapes from
    # the other region). If self is in raw mode, this method will select individual 
    # shapes. Otherwise, this method will select coherent regions and no part of these 
    # regions may overlap with shapes from the other region.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_not_outside.
    #
    # This method is available for polygon layers.
    #
    # The following image shows the effect of the "not_outside" method (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_not_outside.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name select_outside
    # @brief Selects shapes or regions of self which are outside the other region
    # @synopsis layer.select_outside(other)
    # This method selects all shapes or regions from self which are completely outside 
    # the other region (no part of these shapes or regions may be covered by shapes from
    # the other region). If self is in raw mode, this method will select individual 
    # shapes. Otherwise, this method will select coherent regions and no part of these 
    # regions may overlap with shapes from the other region.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \outside.
    #
    # This method is available for polygon layers.
    
    # %DRC%
    # @name select_not_outside
    # @brief Selects shapes or regions of self which are not outside the other region
    # @synopsis layer.select_not_outside(other)
    # This method selects all shapes or regions from self which are not completely outside 
    # the other region (part of these shapes or regions may be covered by shapes from
    # the other region). If self is in raw mode, this method will select individual 
    # shapes. Otherwise, this method will select coherent regions and no part of these 
    # regions may overlap with shapes from the other region.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \not_outside.
    #
    # This method is available for polygon layers.
    
    # %DRC%
    # @name in
    # @brief Selects shapes or regions of self which are contained in the other layer
    # @synopsis layer.in(other)
    # This method selects all shapes or regions from self which are contained  
    # the other region exactly. It will use individual shapes from self or other if
    # the respective region is in raw mode. If not, it will use coherent regions or combined edges from
    # self or other.
    #
    # It will return a new layer containing the selected shapes.
    # A method which selects all shapes not contained in the other layer is \not_in.
    #
    # This method is available for polygon and edge layers.
    #
    # The following image shows the effect of the "in" method (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_in.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name not_in
    # @brief Selects shapes or regions of self which are not contained in the other layer
    # @synopsis layer.not_in(other)
    # This method selects all shapes or regions from self which are not contained  
    # the other region exactly. It will use individual shapes from self or other if
    # the respective region is in raw mode. If not, it will use coherent regions or combined edges from
    # self or other.
    # 
    # It will return a new layer containing the selected shapes.
    # A method which selects all shapes contained in the other layer is \in.
    #
    # This method is available for polygon and edge layers.
    #
    # The following image shows the effect of the "not_in" method (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_not_in.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name interacting
    # @brief Selects shapes or regions of self which touch or overlap shapes from the other region
    # @synopsis layer.interacting(other)
    # This method selects all shapes or regions from self which touch or overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_interacting.
    #
    # This method is available for polygon, text and edge layers. Edges can be selected
    # with respect to other edges or polygons. Texts can be selected with respect to 
    # polygons. Polygons can be selected with respect to edges, texts and other polygons.
    #
    # The following image shows the effect of the "interacting" method (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_interacting.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name not_interacting
    # @brief Selects shapes or regions of self which do not touch or overlap shapes from the other region
    # @synopsis layer.not_interacting(other)
    # This method selects all shapes or regions from self which do not touch or overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_not_interacting.
    #
    # This method is available for polygon, text and edge layers. Edges can be selected
    # with respect to other edges or polygons. Texts can be selected with respect to 
    # polygons. Polygons can be selected with respect to edges, texts and other polygons.
    #
    # The following image shows the effect of the "not_interacting" method (input1: red, input2: blue):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_not_interacting.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name select_interacting
    # @brief Selects shapes or regions of self which touch or overlap shapes from the other region
    # @synopsis layer.select_interacting(other)
    # This method selects all shapes or regions from self which touch or overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \interacting.
    #
    # This method is available for polygon, text and edge layers. Edges can be selected
    # with respect to other edges or polygons. Texts can be selected with respect to 
    # polygons. Polygons can be selected with respect to edges, texts and other polygons.
    
    # %DRC%
    # @name select_not_interacting
    # @brief Selects shapes or regions of self which do not touch or overlap shapes from the other region
    # @synopsis layer.select_interacting(other)
    # This method selects all shapes or regions from self which do not touch or overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \not_interacting.
    #
    # This method is available for polygon, text and edge layers. Edges can be selected
    # with respect to other edges or polygons. Texts can be selected with respect to 
    # polygons. Polygons can be selected with respect to edges, texts and other polygons.
    
    # %DRC%
    # @name intersections
    # @brief Returns the intersection points of intersecting edge segments for two edge collections
    # @synopsis layer.intersections(edges)
    # This operation is similar to the "&" operator, but it does also report intersection points
    # between non-colinear, but intersection edges. Such points are reported as point-like,
    # degenerated edge objects.
    #
    # This method is available for edge layers. The argument must be an edge layer.
    
    # %DRC%
    # @name inside_part
    # @brief Returns the parts of the edges inside the given region
    # @synopsis layer.inside_part(region)
    # This method returns the parts of the edges which are inside the given region. This is similar to the
    # "&" operator, but this method does not return edges that are exactly on the boundaries
    # of the polygons of the region.
    #
    # This method is available for edge layers. The argument must be a polygon layer.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_inside_part.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name outside_part
    # @brief Returns the parts of the edges outside the given region
    # @synopsis layer.outside_part(region)
    # This method returns the parts of the edges which are outside the given region. This is similar to the
    # "&" operator, but this method does not remove edges that are exactly on the boundaries
    # of the polygons of the region.
    #
    # This method is available for edge layers. The argument must be a polygon layer.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_outside_part.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name pull_interacting
    # @brief Selects shapes or edges of other which touch or overlap shapes from the this region
    # @synopsis layer.pull_interacting(other)
    # This method selects all shapes or regions from other which touch or overlap shapes from this
    # region. Unless other is in raw mode (see \raw), coherent regions are selected from other, 
    # otherwise individual shapes are selected.
    #
    # The functionality is similar to select_interacting, but chosing shapes from other rather
    # than from self. Because in deep mode the hierarchy reference comes from self, this method
    # provides a way to pull shapes from other to the hierarchy to self.
    #
    # This method will neither modify self nor other.
    #
    # This method is available for polygon, edge and text layers, similar to interacting.
    
    # %DRC%
    # @name pull_overlapping
    # @brief Selects shapes or regions of other which overlap shapes from the this region
    # @synopsis layer.pull_overlapping(other)
    # This method selects all shapes or regions from other which overlap shapes from this
    # region. Unless other is in raw mode (see \raw), coherent regions are selected from other, 
    # otherwise individual shapes are selected.
    #
    # The functionality is similar to select_overlapping, but chosing shapes from other rather
    # than from self. Because in deep mode the hierarchy reference comes from self, this method
    # provides a way to pull shapes from other to the hierarchy to self.
    #
    # This method is available for polygon layers. Other needs to be a polygon layer too.
    
    # %DRC%
    # @name pull_inside
    # @brief Selects shapes or regions of other which are inside polygons from the this region
    # @synopsis layer.pull_inside(other)
    # This method selects all shapes or regions from other which are inside polygons from this
    # region. Unless other is in raw mode (see \raw), coherent regions are selected from other, 
    # otherwise individual shapes are selected.
    #
    # The functionality is similar to select_inside, but chosing shapes from other rather
    # than from self. Because in deep mode the hierarchy reference comes from self, this method
    # provides a way to pull shapes from other to the hierarchy to self.
    #
    # This method is available for polygon layers. Other needs to be a polygon layer too.
    
    %w(pull_interacting pull_overlapping pull_inside).each do |f| 
      eval <<"CODE"
      def #{f}(other)
        if :#{f} != :pull_interacting 
          requires_region("#{f}")
          other.requires_region("#{f}")
        else
          requires_edges_texts_or_region("#{f}")
          if @data.is_a?(RBA::Text)
            other.requires_region("#{f}")
          elsif @data.is_a?(RBA::Region)
            other.requires_edges_texts_or_region("#{f}")
          else
            other.requires_edges_or_region("#{f}")
          end
        end
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, other.data.class, :#{f}, other.data))
      end
CODE
    end

    %w(| ^ overlapping not_overlapping inside not_inside outside not_outside in not_in).each do |f| 
      eval <<"CODE"
      def #{f}(other)
        requires_same_type(other, "#{f}")
        requires_edges_or_region("#{f}")
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :#{f}, other.data))
      end
CODE
    end

    %w(& -).each do |f| 
      eval <<"CODE"
      def #{f}(other)
        other.requires_edges_texts_or_region("#{f}")
        if @data.is_a?(RBA::Texts)
          other.requires_region("#{f}")
        else
          other.requires_edges_or_region("#{f}")
        end
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :#{f}, other.data))
      end
CODE
    end

    %w(+).each do |f| 
      eval <<"CODE"
      def #{f}(other)
        requires_same_type(other, "#{f}")
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :#{f}, other.data))
      end
CODE
    end

    %w(interacting not_interacting).each do |f| 
      eval <<"CODE"
      def #{f}(other)
        other.requires_edges_texts_or_region("#{f}")
        if @data.is_a?(RBA::Text)
          other.requires_region("#{f}")
        elsif @data.is_a?(RBA::Region)
          other.requires_edges_texts_or_region("#{f}")
        else
          other.requires_edges_or_region("#{f}")
        end
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :#{f}, other.data))
      end
CODE
    end

    %w(interacting not_interacting overlapping not_overlapping inside not_inside outside not_outside).each do |fi|
      f = "select_" + fi
      # In tiled mode, there are no modifying versions. Emulate using the non-modifying one.
      eval <<"CODE"
      def #{f}(other)
        if :#{fi} != :interacting && :#{fi} != :not_interacting 
          requires_edges_or_region("#{f}")
          requires_same_type(other, "#{f}")
        else
          requires_edges_texts_or_region("#{f}")
          if @data.is_a?(RBA::Text)
            other.requires_region("#{f}")
          elsif @data.is_a?(RBA::Region)
            other.requires_edges_texts_or_region("#{f}")
          else
            other.requires_edges_or_region("#{f}")
          end
        end
        if @engine.is_tiled?
          @data = @engine._tcmd(@data, 0, @data.class, :#{fi}, other.data)
          DRCLayer::new(@engine, @data)
        else
          DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :#{f}, other.data))
        end
      end
CODE
    end
    
    %w(inside_part outside_part).each do |f|
      eval <<"CODE"
      def #{f}(other)
        other.requires_region("#{f}")
        requires_edges("#{f}")
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :#{f}, other.data))
      end
CODE
    end
    
    %w(intersections).each do |f|
      eval <<"CODE"
      def #{f}(other)
        other.requires_edges("#{f}")
        requires_edges("#{f}")
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :#{f}, other.data))
      end
CODE
    end
    
    # %DRC%
    # @name rectangles
    # @brief Selects all rectangle polygons from the input
    # @synopsis layer.rectangles
    #
    # This method is available for polygon layers. By default "merged" semantics applies, 
    # i.e. all polygons are merged before rectangles are selected (see \clean and \raw).
    # \non_rectangles will select all non-rectangles.

    # %DRC%
    # @name rectilinear
    # @brief Selects all rectilinear polygons from the input
    # @synopsis layer.rectilinear
    #
    # This method is available for polygon layers. By default "merged" semantics applies, 
    # i.e. all polygons are merged before rectilinear polygons are selected (see \clean and \raw).
    # \non_rectilinear will select all non-rectangles.
    
    # %DRC%
    # @name non_rectangles
    # @brief Selects all polygons from the input which are not rectangles
    # @synopsis layer.non_rectangles
    #
    # This method is available for polygon layers. By default "merged" semantics applies, 
    # i.e. all polygons are merged before non-rectangles are selected (see \clean and \raw).

    # %DRC%
    # @name non_rectilinear
    # @brief Selects all non-rectilinear polygons from the input
    # @synopsis layer.non_rectilinear
    #
    # This method is available for polygon layers. By default "merged" semantics applies, 
    # i.e. all polygons are merged before non-rectilinear polygons are selected (see \clean and \raw).

    # %DRC%
    # @name holes
    # @brief Selects all polygon holes from the input
    # @synopsis layer.holes
    #
    # This method is available for polygon layers. It will create polygons from all holes inside 
    # polygons of the input. Although it is possible, running this method on raw polygon layers will
    # usually not render the expected result, since raw layers do not contain polygons with holes in
    # most cases.
    #
    # The following image shows the effects of the holes method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_holes.png) @/td
    #   @/tr
    # @/table

    # %DRC%
    # @name hulls
    # @brief Selects all polygon hulls from the input
    # @synopsis layer.hulls
    #
    # This method is available for polygon layers. It will remove all holes from the input and 
    # render the hull polygons only. Although it is possible, running this method on raw polygon layers will
    # usually not render the expected result, since raw layers do not contain polygons with holes in
    # most cases.
    #
    # The following image shows the effects of the hulls method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_hulls.png) @/td
    #   @/tr
    # @/table

    %w(rectangles rectilinear non_rectangles non_rectilinear
       holes hulls).each do |f| 
      eval <<"CODE"
      def #{f}
        requires_region("#{f}")
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :#{f}))
      end
CODE
    end
    
    # %DRC%
    # @name end_segments 
    # @brief Returns the part at the end of each edge
    # @synopsis layer.end_segments(length)
    # @synopsis layer.end_segments(length, fraction)
    #
    # This method will return a partial edge for each edge in the input, 
    # located and the end of the original edge.
    # The new edges will share the end point with the original edges, but not necessarily
    # their start point. This method applies to edge layers only. 
    # The direction of edges is defined by the clockwise orientation of a polygon: the 
    # end point of the edges will be the terminal point of each edge when walking a polygon
    # in clockwise direction. Or in other words: when looking from start to the end point
    # of an edge, the filled part of the polygon is to the right.
    # 
    # The length of the new edge can be given in two ways: as a fixed length, or a fraction, or
    # both. In the latter case, the length of the resulting edge will be either the fraction or
    # the fixed length, whichever is larger.
    # To specify a length only, omit the fraction argument or leave it at 0. To specify
    # a fraction only, pass 0 to the length argument and specify the fraction in the second
    # parameter. A fraction of 0.5 will result in edges which cover the end half of the 
    # edge.
    # 
    # The following images show the effect of the method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_end_segments1.png) @/td
    #     @td @img(/images/drc_end_segments2.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name start_segments
    # @brief Returns the part at the beginning of each edge
    # @synopsis layer.start_segments(length)
    # @synopsis layer.start_segments(length, fraction)
    #
    # This method will return a partial edge for each edge in the input, 
    # located and the end of the original edge.
    # The new edges will share the start point with the original edges, but not necessarily
    # their end points. For further details about the orientation of edges and the parameters
    # of this method, see \end_segments.
    # 
    # The following images show the effect of the method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_start_segments1.png) @/td
    #     @td @img(/images/drc_start_segments2.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name centers
    # @brief Returns the center parts of the edges
    # @synopsis layer.centers(length)
    # @synopsis layer.centers(length, fraction)
    # 
    # Similar to \start_segments and \end_segments, this method will return partial
    # edges for each given edge in the input. For the description of the parameters see
    # \start_segments or \end_segments.
    # 
    # The following images show the effect of the method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_centers1.png) @/td
    #     @td @img(/images/drc_centers2.png) @/td
    #   @/tr
    # @/table
    
    %w(end_segments start_segments centers).each do |f|
      eval <<"CODE"
      def #{f}(length, fraction = 0.0)
        requires_edges("#{f}")
        length = @engine._prep_value(length)
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Edges, :#{f}, length, fraction))
      end
CODE
    end
    
    # %DRC%
    # @name extended
    # @brief Returns polygons describing an area along the edges of the input
    # @synopsis layer.extended([:begin => b,] [:end => e,] [:out => o,] [:in => i], [:joined => true])
    # @synopsis layer.extended(b, e, o, i)
    # @synopsis layer.extended(b, e, o, i, joined)
    # 
    # This method is available for edge layers only. It will create a polygon for each edge
    # tracing the edge with certain offsets to the edge. "o" is the offset applied to the 
    # outer side of the edge, "i" is the offset applied to the inner side of the edge.
    # "b" is the offset applied at the beginning and "e" is the offset applied at the end.
    # 
    # When looking from start to end point, the "inside" side is to the right, while the "outside"
    # side is to the left.
    # 
    # "joined" is a flag, which, if present, will make connected edges behave as a continuous
    # line. Start and end offsets are applied to the first and last unconnected point respectively.
    # Please note that in order to specify joined mode, you'll need to specify "joined" as 
    # a keyword in the third form of the method.
    #
    # The following images show the effects of some parameters:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_extended1.png) @/td
    #     @td @img(/images/drc_extended2.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_extended3.png) @/td
    #     @td @img(/images/drc_extended4.png) @/td
    #   @/tr
    # @/table
    
    %w(extended).each do |f| 
      eval <<"CODE"
      def #{f}(*args)
      
        requires_edges("#{f}")
        
        av = [ 0, 0, 0, 0, false ]
        args.each_with_index do |a,i|
          if a.is_a?(Hash)
            a[:begin]  && av[0] = @engine._prep_value(a[:begin])
            a[:end]    && av[1] = @engine._prep_value(a[:end])
            a[:out]    && av[2] = @engine._prep_value(a[:out])
            a[:in]     && av[3] = @engine._prep_value(a[:in])
            a[:joined] && av[4] = true
          elsif i < 4
            if !a.is_a?(1.class) && !a.is_a?(Float)
              raise("Invalid type for argument " + (i+1).to_s + " (method '#{f}')")
            end
            av[i] = @engine._prep_value(a)
          elsif i == 4
            if a.is_a?(DRCJoinFlag)
              av[i] = a.value
            else
              av[i] = (a ? true : false)
            end
          else
            raise("Too many arguments for method '#{f}' (1 to 5 expected)")
          end
        end

        DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :#{f}, *av))

      end
CODE
    end
    
    # %DRC%
    # @name extended_in
    # @brief Returns polygons describing an area along the edges of the input
    # @synopsis layer.extended_in(d)
    #
    # This method applies to edge layers only. Polygons are generated for 
    # each edge describing the edge drawn with a certain width extending into
    # the "inside" (the right side when looking from start to end).
    # This method is basically equivalent to the \extended method:
    # "extended(0, 0, 0, dist)".
    # A version extending to the outside is \extended_out.
    
    # %DRC%
    # @name extended_out
    # @brief Returns polygons describing an area along the edges of the input
    # @synopsis layer.extended_out(d)
    #
    # This method applies to edge layers only. Polygons are generated for 
    # each edge describing the edge drawn with a certain width extending into
    # the "outside" (the left side when looking from start to end).
    # This method is basically equivalent to the \extended method:
    # "extended(0, 0, dist, 0)".
    # A version extending to the inside is \extended_in.
    
    %w(extended_in extended_out).each do |f| 
      eval <<"CODE"
      def #{f}(dist)
        requires_edges("#{f}")
        DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Region, :#{f}, @engine._prep_value(dist)))
      end
CODE
    end
    
    # %DRC%
    # @name edges
    # @brief Decomposes the layer into single edges
    #
    # Edge pair collections are decomposed into the individual edges that make up
    # the edge pairs. Polygon layers are decomposed into the edges making up the 
    # polygons. This method returns an edge layer but will not modify the layer it 
    # is called on.
    #
    # Merged semantics applies, i.e. the result reflects merged polygons rather than
    # individual ones unless raw mode is chosen.
    
    %w(edges).each do |f| 
      eval <<"CODE"
      def #{f}
        if @data.is_a?(RBA::Region)
          DRCLayer::new(@engine, @engine._tcmd(@data, 0, RBA::Edges, :#{f}))
        elsif @data.is_a?(RBA::EdgePairs)
          DRCLayer::new(@engine, @engine._cmd(@data, :#{f}))
        else
          raise "#{f}: Layer must be a polygon or edge pair layer"
        end
      end
CODE
    end
    
    # %DRC%
    # @name first_edges
    # @brief Returns the first edges of an edge pair collection
    # @synopsis layer.first_edges
    # 
    # Applies to edge pair collections only.
    # Returns the first edges of the edge pairs in the collection.
    
    # %DRC%
    # @name second_edges
    # @brief Returns the second edges of an edge pair collection
    # @synopsis layer.second_edges
    # 
    # Applies to edge pair collections only.
    # Returns the second edges of the edge pairs in the collection.
    
    %w(first_edges second_edges).each do |f| 
      eval <<"CODE"
      def #{f}
        requires_edge_pairs("#{f}")
        DRCLayer::new(@engine, @engine._cmd(@data, :#{f}))
      end
CODE
    end
    
    # %DRC%
    # @name bbox
    # @brief Returns the overall bounding box of the layer
    # @synopsis layer.bbox
    # The return value is a RBA::DBox object giving the bounding box in 
    # micrometer units. 
    
    def bbox
      RBA::DBox::from_ibox(@data.bbox) * @engine.dbu.to_f
    end
    
    # %DRC%
    # @name polygons?
    # @brief Returns true, if the layer is a polygon layer
    # @synopsis layer.polygons?
    
    def polygons?
      @data.is_a?(RBA::Region)
    end
    
    # %DRC%
    # @name edges?
    # @brief Returns true, if the layer is an edge layer
    # @synopsis layer.edges?
    
    def edges?
      @data.is_a?(RBA::Edges)
    end
    
    # %DRC%
    # @name edge_pairs?
    # @brief Returns true, if the layer is an edge pair collection
    # @synopsis layer.edge_pairs?
    
    def edge_pairs?
      @data.is_a?(RBA::EdgePairs)
    end
    
    # %DRC%
    # @name is_deep?
    # @brief Returns true, if the layer is a deep (hierarchical) layer
    # @synopsis layer.is_deep?
    
    def is_deep?
      @data.respond_to?(:is_deep?) && @data.is_deep?
    end
    
    # %DRC%
    # @name area
    # @brief Returns the total area of the polygons in the region
    # @synopsis layer.area
    # 
    # This method requires a polygon layer. It returns the total
    # area of all polygons in square micron. Merged semantics applies, 
    # i.e. before computing the area, the polygons are merged unless
    # raw mode is chosen (see \raw). Hence, in clean mode, overlapping
    # polygons are not counted twice.
    # 
    # The returned value gives the area in square micrometer units.
    
    def area
      requires_region("area")
      @engine._tdcmd(@data, 0, :area) * (@engine.dbu.to_f * @engine.dbu.to_f)
    end
    
    # %DRC%
    # @name perimeter
    # @brief Returns the total perimeter of the polygons in the region
    # @synopsis layer.perimeter
    # 
    # This method requires a polygon layer. It returns the total
    # perimeter of all polygons in micron. Merged semantics applies, 
    # i.e. before computing the perimeter, the polygons are merged unless
    # raw mode is chosen (see \raw).
    # 
    # The returned value gives the perimeter in micrometer units.
    
    def perimeter
      requires_region("perimeter")
      # Note: we have to add 1 DBU border to collect the neighbors. It's important
      # to know then since they tell us whether an edge is an outside edge.
      @engine._tdcmd(@data, 1, :perimeter) * @engine.dbu.to_f
    end
    
    # %DRC%
    # @name is_box?
    # @brief Returns true, if the region contains a single box
    # @synopsis layer.is_box?
    # 
    # The method returns true, if the region consists of a single box
    # only. Merged semantics does not apply - if the region forms a box which
    # is composed of multiple pieces, this method will not return true.
    
    def is_box?
      requires_region("is_box?")
      @engine._cmd(@data, :is_box?)
    end
    
    # %DRC%
    # @name length
    # @brief Returns the total length of the edges in the edge layer
    # @synopsis layer.length
    # 
    # This method requires an edge layer. It returns the total
    # length of all edges in micron. Merged semantics applies, 
    # i.e. before computing the length, the edges are merged unless
    # raw mode is chosen (see \raw). Hence in clean mode (see \clean), overlapping
    # edges are not counted twice.
    
    def length
      requires_edges("length")
      @engine._cmd(@data, :length) * @engine.dbu.to_f
    end
    
    # %DRC%
    # @name flatten
    # @brief Flattens the layer
    # @synopsis layer.flatten
    # 
    # If the layer already is a flat one, this method does nothing.
    # If the layer is a hierarchical layer (an original layer or
    # a derived layer in deep mode), this method will convert it
    # to a flat collection of texts, polygons, edges or edge pairs.
    
    def flatten
      DRC::DRCLayer::new(@engine, @engine._cmd(@data, :flatten))
    end
    
    # %DRC%
    # @name is_merged?
    # @brief Returns true, if the polygons of the layer are merged
    # @synopsis layer.is_merged?
    #
    # This method will return true, if the polygons of this layer are
    # merged, i.e. they don't overlap and form single continuous polygons.
    # In clean mode, this is ensured implicitly. In raw mode (see \raw),
    # merging can be achieved by using the \merge method. \is_merged?
    # tells, whether calling \merge is necessary.
    
    def is_merged?
      requires_edges_or_region("is_merged?")
      @data.is_merged?
    end
    
    # %DRC%
    # @name is_empty?
    # @brief Returns true, if the layer is empty
    # @synopsis layer.is_empty?
    
    def is_empty?
      @data.is_empty?
    end
    
    # %DRC%
    # @name iso
    # @brief An alias for "isolated"
    # @synopsis layer.iso(value [, options])
    # See \isolated for a description of that method
    
    def iso(*args)
      isolated(*args)
    end
    
    # %DRC%
    # @name enc
    # @brief An alias for "enclosing"
    # @synopsis layer.enc(value [, options])
    # See \enclosing for a description of that method
    
    def enc(*args)
      enclosing(*args)
    end
    
    # %DRC%
    # @name sep
    # @brief An alias for "separation"
    # @synopsis layer.sep(value [, options])
    # See \separation for a description of that method
    
    def sep(*args)
      separation(*args)
    end
    
    # %DRC%
    # @name width
    # @brief A width check
    # @synopsis layer.width(value [, options])
    # 
    # This method performs a width check and returns a collection of edge pairs.
    # A width check can be performed on polygon and edge layers. On edge layers, all
    # edges are checked against all other edges. If two edges form a "back to back" relation
    # (i.e. their inner sides face each other) and their distance is less than the specified
    # value, an error shape is generated for that edge pair. 
    # On polygon layers, the polygons on each layer are checked for locations where their
    # width is less than the specified value. In that case, an edge pair error shape is generated.
    # 
    # The options available are:
    #
    # @ul
    #   @li @b euclidian @/b: perform the check using Euclidian metrics (this is the default) @/li  
    #   @li @b square @/b: perform the check using Square metrics @/li  
    #   @li @b projection @/b: perform the check using projection metrics @/li
    #   @li @b whole_edges @/b: With this option, the check will return all of the edges,
    #         even if the criterion is violated only over a part of the edge @/li
    #   @li @b angle_limit(a) @/b: Specifies the angle above or equal to which no 
    #         check is performed. The default value is 90, which means that for edges having 
    #         an angle of 90 degree or more, no check is performed. Setting this value to 45 will
    #         make the check only consider edges enclosing angles of less than 45 degree. @/li
    #   @li @b projection_limits(min, max) or projection_limits(min .. max) @/b:
    #         this option makes the check only consider edge pairs whose projected length on
    #         each other is more or equal than min and less than max @/li
    # @/ul
    #
    # Note that without the angle_limit, acute corners will always be reported, since two 
    # connected edges always violate the width in the corner. By adjusting the angle_limit, an 
    # acute corner check can be implemented.
    #
    # Merge semantics applies to this method, i.e. disconnected polygons are merged before the 
    # width is checked unless "raw" mode is chosen.
    # 
    # The resulting edge pairs can be converted to polygons using the \polygons method.
    # 
    # Distance values can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators, i.e.
    #
    # @code
    #   # width check for 1.5 micron:
    #   markers = in.width(1.5)
    #   # width check for 2 database units:
    #   markers = in.width(2)
    #   # width check for 2 micron:
    #   markers = in.width(2.um)
    #   # width check for 20 nanometers:
    #   markers = in.width(20.nm)
    # @/code
    #
    # The following images show the effect of various forms of the width check:
    # 
    # @table
    #   @tr 
    #     @td @img(/images/drc_width1.png) @/td
    #     @td @img(/images/drc_width2.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_width3.png) @/td
    #     @td @img(/images/drc_width4.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name space
    # @brief A space check
    # @synopsis layer.space(value [, options])
    #
    # This method performs a space check and returns a collection of edge pairs.
    # A space check can be performed on polygon and edge layers. On edge layers, all
    # edges are checked against all other edges. If two edges form a "face to face" relation
    # (i.e. their outer sides face each other) and their distance is less than the specified
    # value, an error shape is generated for that edge pair. 
    # On polygon layers, the polygons on each layer are checked for space against other polygons 
    # for locations where their space is less than the specified value. In that case, an edge 
    # pair error shape is generated.
    # The space check will also check the polygons for space violations against themselves, i.e.
    # notches violating the space condition are reported.
    #
    # The \notch method is similar, but will only report self-space violations. The \isolated
    # method will only report space violations to other polygons. \separation is a two-layer 
    # space check where space is checked against polygons of another layer.
    #
    # The options available are the same than for the \width method. Like for the \width 
    # method, merged semantics applies.
    # Distance values can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators.
    #
    # The following image shows the effect of the space check:
    # 
    # @table
    #   @tr 
    #     @td @img(/images/drc_space1.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name isolated
    # @brief An isolation check
    # @synopsis layer.isolated(value [, options])
    #
    # See \space for a description of this method. 
    # In contrast to \space, this
    # method is available for polygon layers only, since only on such layers 
    # different polygons can be identified.
    #
    # The following image shows the effect of the isolated check:
    # 
    # @table
    #   @tr 
    #     @td @img(/images/drc_space3.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name notch
    # @brief An intra-region spacing check
    # @synopsis layer.notch(value [, options])
    #
    # See \space for a description of this method.
    # In contrast to \space, this
    # method is available for polygon layers only, since only on such layers 
    # different polygons can be identified.
    #
    # The following image shows the effect of the notch check:
    # 
    # @table
    #   @tr 
    #     @td @img(/images/drc_space2.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name separation
    # @brief A two-layer spacing check
    # @synopsis layer.separation(other_layer, value [, options])
    # 
    # This method performs a two-layer spacing check. Like \space, this method
    # can be applied to edge or polygon layers. Locations where edges of the layer
    # are closer than the specified distance to the other layer are reported
    # as edge pair error markers.
    # 
    # In contrast to the \space and related methods, locations where both 
    # layers touch are also reported. More specifically, the case of zero spacing
    # will also trigger an error while for \space it will not.
    # 
    # As for the other DRC methods, merged semantics applies. The options available 
    # are the same than for \width.  
    # Distance values can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators.
    #
    # The following image shows the effect of the separation check (input1: red, input2: blue):
    # 
    # @table
    #   @tr 
    #     @td @img(/images/drc_separation1.png) @/td
    #   @/tr
    # @/table
    
    # %DRC% 
    # @name overlap 
    # @brief An overlap check
    # @synopsis layer.overlap(other_layer, value [, options])
    #
    # This method checks whether layer and other_layer overlap by at least the
    # given length. Locations, where this is not the case will be reported in form
    # of edge pair error markers.
    # Locations, where both layers touch will be reported as errors as well. Formally
    # such locations form an overlap with a value of 0. Locations, where both regions 
    # do not overlap or touch will not be reported. Such regions can be detected 
    # with \outside or by a boolean "not".
    # 
    # Formally, the overlap method is a two-layer width check. In contrast to the single-
    # layer width method (\width), the zero value also triggers an error and separate
    # polygons are checked against each other, while for the single-layer width, only 
    # single polygons are considered.
    #
    # The overlap method can be applied to both edge or polygon layers. On edge layers 
    # the orientation of the edges matters: only edges which run back to back with their
    # inside side pointing towards each other are checked for distance.
    #
    # As for the other DRC methods, merged semantics applies. The options available 
    # are the same than for \width.  
    # Distance values can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators.
    #
    # The following images show the effect of the overlap check (input1: red, input2: blue):
    # 
    # @table
    #   @tr 
    #     @td @img(/images/drc_overlap1.png) @/td
    #     @td @img(/images/drc_overlap2.png) @/td
    #   @/tr
    # @/table
    
    # %DRC% 
    # @name enclosing
    # @brief An enclosing check
    # @synopsis layer.enclosing(other_layer, value [, options])
    #
    # This method checks whether layer encloses (is bigger than) other_layer by the
    # given dimension. Locations, where this is not the case will be reported in form
    # of edge pair error markers.
    # Locations, where both edges coincide will be reported as errors as well. Formally
    # such locations form an enclosure with a distance of 0. Locations, where other_layer
    # extends outside layer will not be reported as errors. Such regions can be detected
    # by \not_inside or a boolean "not" operation.
    #
    # The enclosing method can be applied to both edge or polygon layers. On edge layers 
    # the orientation of the edges matters and only edges looking into the same direction
    # are checked.
    # 
    # As for the other DRC methods, merged semantics applies. The options available 
    # are the same than for \width.  
    # Distance values can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators.
    #
    # The following images show the effect of two enclosing checks (red: input1, blue: input2):
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_enc1.png) @/td
    #     @td @img(/images/drc_enc2.png) @/td
    #   @/tr
    # @/table
    
    %w(width space overlap enclosing separation).each do |f|
      eval <<"CODE"
      def #{f}(*args)

        requires_edges_or_region("#{f}")
        
        value = nil
        metrics = nil
        minp = nil
        maxp = nil
        alim = nil
        whole_edges = false
        other = nil

        n = 1
        args.each do |a|
          if a.is_a?(DRCMetrics)
            metrics = a.value
          elsif a.is_a?(DRCWholeEdges)
            whole_edges = a.value
          elsif a.is_a?(DRCAngleLimit)
            alim = a.value
          elsif a.is_a?(DRCLayer)
            other = a
          elsif a.is_a?(DRCProjectionLimits)
            minp = @engine._prep_value(a.min)
            maxp = @engine._prep_value(a.max)
          elsif a.is_a?(Float) || a.is_a?(1.class)
            value && raise("Value already specified")
            value = @engine._prep_value(a)
          else
            raise("#{f}: Parameter #" + n.to_s + " does not have an expected type")
          end
          n += 1
        end

        if !value
          raise("#{f}: A check value must be specified")
        end
        
        border = (metrics == RBA::Region::Square ? value * 1.5 : value)
        
        if "#{f}" == "width" || "#{f}" == "space" || "#{f}" == "notch" || "#{f}" == "isolated"
          if other
            raise("No other layer must be specified for single-layer checks (i.e. width)")
          end
          DRCLayer::new(@engine, @engine._tcmd(@data, border, RBA::EdgePairs, :#{f}_check, value, whole_edges, metrics, alim, minp, maxp))
        else
          if !other
            raise("The other layer must be specified for two-layer checks (i.e. overlap)")
          end
          requires_same_type(other, "#{f}")
          DRCLayer::new(@engine, @engine._tcmd(@data, border, RBA::EdgePairs, :#{f}_check, other.data, value, whole_edges, metrics, alim, minp, maxp))
        end
        
      end  
CODE
    end
    
    %w(isolated notch).each do |f|
      eval <<"CODE"
      def #{f}(*args)

        requires_region("#{f}")
        
        value = nil
        metrics = nil
        minp = nil
        maxp = nil
        alim = nil
        whole_edges = false
        other = nil

        n = 1
        args.each do |a|
          if a.is_a?(DRCMetrics)
            metrics = a.value
          elsif a.is_a?(DRCWholeEdges)
            whole_edges = a.value
          elsif a.is_a?(DRCAngleLimit)
            alim = a.value
          elsif a.is_a?(DRCLayer)
            other = a
          elsif a.is_a?(DRCProjectionLimits)
            minp = @engine._prep_value(a.min)
            maxp = @engine._prep_value(a.max)
          elsif a.is_a?(Float) || a.is_a?(1.class)
            value && raise("Value already specified")
            value = @engine._prep_value(a)
          else
            raise("#{f}: Parameter #" + n.to_s + " does not have an expected type")
          end
          n += 1
        end

        if !value
          raise("A check value must be specified")
        end
        
        border = (metrics == RBA::Region::Square ? value * 1.5 : value)
        
        if "#{f}" == "width" || "#{f}" == "space" || "#{f}" == "notch" || "#{f}" == "isolated"
          if other
            raise("#{f}: No other layer must be specified for single-layer checks (i.e. width)")
          end
          DRCLayer::new(@engine, @engine._tcmd(@data, border, RBA::EdgePairs, :#{f}_check, value, whole_edges, metrics, alim, minp, maxp))
        else
          if !other
            raise("#{f}: The other layer must be specified for two-layer checks (i.e. overlap)")
          end
          DRCLayer::new(@engine, @engine._tcmd(@data, border, RBA::EdgePairs, :#{f}_check, other.data, value, whole_edges, metrics, alim, minp, maxp))
        end
        
      end  
CODE
    end
    
    # %DRC%
    # @name scaled
    # @brief Scales a layer
    # @synopsis layer.scaled(f)
    # 
    # Scales the input layer and returns a new layer whose features have a f times
    # bigger dimension. The layer that this method is called upon is not modified.
    #
    # The following images shows the effect of the "scaled" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_scaled1.png) @/td
    #   @/tr
    # @/table
    
    def scaled(f)
      transformed(RBA::ICplxTrans::new(f.to_f))
    end
    
    # %DRC%
    # @name scale
    # @brief Scales a layer (modifies the layer)
    # @synopsis layer.scale(f)
    # 
    # Scales the input. After scaling, features have a f times
    # bigger dimension. The layer that this method is called upon is modified and
    # the modified version is returned for further processing.
    
    def scale(f)
      transform(RBA::ICplxTrans::new(f.to_f))
    end
    
    # %DRC%
    # @name rotated
    # @brief Rotates a layer
    # @synopsis layer.rotated(a)
    # 
    # Rotates the input layer by the given angle (in degree) and returns
    # the rotated layer. The layer that this method is called upon is not modified.
    #
    # The following image shows the effect of the "rotated" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_rotated1.png) @/td
    #   @/tr
    # @/table
    
    def rotated(a)
      transformed(RBA::ICplxTrans::new(1.0, a, false, 0, 0))
    end
    
    # %DRC%
    # @name rotate
    # @brief Rotates a layer (modifies the layer)
    # @synopsis layer.rotate(a)
    # 
    # Rotates the input by the given angle (in degree). The layer that this method is called 
    # upon is modified and the modified version is returned for further processing.
    
    def rotate(a)
      transform(RBA::ICplxTrans::new(1.0, a, false, 0, 0))
    end
    
    # %DRC%
    # @name sized
    # @brief Polygon sizing (per-edge biasing)
    # @synopsis layer.sized(d [, mode])
    # @synopsis layer.sized(dx, dy [, mode]))
    #
    # This method requires a polygon layer. It will apply a bias per edge of the polygons 
    # and return the biased layer. The layer that this method is called on is not modified.
    # 
    # In the single-value form, that bias is applied both in horizontal or vertical direction.
    # In the two-value form, the horizontal and vertical bias can be specified separately.
    # 
    # The mode defines how to handle corners. The following modes are available:
    #
    # @ul
    #   @li @b diamond_limit @/b: This mode will connect the shifted edges without corner interpolation @/li
    #   @li @b octagon_limit @/b: This mode will create octagon-shaped corners @/li
    #   @li @b square_limit @/b: This mode will leave 90 degree corners untouched but 
    #           cut off corners with a sharper angle. This is the default mode. @/li
    #   @li @b acute_limit @/b: This mode will leave 45 degree corners untouched but
    #           cut off corners with a sharper angle @/li
    #   @li @b no_limit @/b: This mode will not cut off (only at extremely sharp angles @/li
    # @/ul
    #
    # Merged semantics applies, i.e. polygons will be merged before the sizing is applied 
    # unless the layer was put into raw mode (see \raw). On output, the polygons are not
    # merged immediately, so it is possible to detect overlapping regions after a positive 
    # sizing using \raw and \merged with an overlap count, for example:
    #
    # @code
    #   layer.sized(300.nm).raw.merged(2)
    # @/code
    #
    # Bias values can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators.
    #
    # \size is working like \sized but modifies the layer it is called on.
    #
    # The following images show the effect of various forms of the "sized" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_sized1.png) @/td
    #     @td @img(/images/drc_sized2.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_sized3.png) @/td
    #     @td @img(/images/drc_sized4.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_sized5.png) @/td
    #     @td @img(/images/drc_sized6.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name size
    # @brief Polygon sizing (per-edge biasing, modifies the layer)
    # @synopsis layer.size(d [, mode])
    # @synopsis layer.size(dx, dy [, mode]))
    #
    # See \sized. The size method basically does the same but modifies the layer
    # it is called on. The input layer is returned and available for further processing.
    
    %w(size sized).each do |f|
      eval <<"CODE"
      def #{f}(*args)
      
        requires_region("#{f}")
        
        dist = 0
        
        mode = 2
        values = []
        args.each do |a|
          if a.is_a?(1.class) || a.is_a?(Float)
            v = @engine._prep_value(a)
            v.abs > dist && dist = v.abs 
            values.push(v)
          elsif a.is_a?(DRCSizingMode)
            mode = a.value
          end
        end
        
        aa = []
        if values.size < 1
          raise "#{f}: Method requires one or two sizing values"
        elsif values.size > 2
          raise "#{f}: Method must not have more than two values"
        else
          aa.push(values[0])
          aa.push(values[-1])
        end
        
        aa.push(mode)
        
        if :#{f} == :size && @engine.is_tiled?
          # in tiled mode, no modifying versions are available
          @data = @engine._tcmd(@data, dist, RBA::Region, :sized, *aa)
          self
        elsif :#{f} == :size 
          @engine._tcmd(@data, dist, RBA::Region, :#{f}, *aa)
          self
        else 
          DRCLayer::new(@engine, @engine._tcmd(@data, dist, RBA::Region, :#{f}, *aa))
        end
        
      end
CODE
    end
    
    # %DRC%
    # @name polygons
    # @brief Returns polygons from edge pairs
    # @synopsis layer.polygons([ enlargement ])
    #
    # This method applies to edge pair collections. The edge pairs will be
    # converted into polygons connecting the edges the edge pairs are made of.
    # In order to properly handle special edge pairs (coincident edges, point-like
    # edges etc.) an enlargement parameter can be specified which will make the 
    # resulting polygon somewhat larger than the original edge pair. If the enlargement
    # parameter is 0, special edge pairs with an area of 0 will be dropped.
    
    def polygons(*args)
      requires_edge_pairs("polygons")
      args.size <= 1 || raise("polygons: Method requires 0 or 1 arguments")
      aa = args.collect { |a| @engine._prep_value(a) }
      DRCLayer::new(@engine, @engine._cmd(@data, :polygons, *aa))
    end
    
    # %DRC%
    # @name extents
    # @brief Returns the bounding box of each input object
    # @synopsis layer.extents([ enlargement ])
    # 
    # Applies to edge layers, polygon layers on edge pair collections.
    # Returns a polygon layer consisting of boxes for each input object.
    # The boxes enclose the original object. 
    # 
    # Merged semantics applies, so the box encloses the merged polygons
    # or edges unless raw mode is chosen (see \raw).
    #
    # The enlargement parameter specifies an optional enlargement which 
    # will make zero width/zero height object render valid polygons 
    # (i.e. horizontal/vertical edges).
    #
    # The following images show the effect of the extents method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_extents1.png) @/td
    #     @td @img(/images/drc_extents2.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name moved
    # @brief Moves (shifts, translates) a layer
    # @synopsis layer.moved(dx, dy)
    # 
    # Moves the input layer by the given distance (x, y) and returns
    # the moved layer. The layer that this method is called upon is not modified.
    #
    # Shift distances can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators.
    #
    # The following images shows the effect of the "moved" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_moved1.png) @/td
    #   @/tr
    # @/table

    # %DRC%
    # @name move
    # @brief Moves (shifts, translates) a layer (modifies the layer)
    # @synopsis layer.move(dx, dy)
    # 
    # Moved the input by the given distance. The layer that this method is called 
    # upon is modified and the modified version is returned for further processing.
    #
    # Shift distances can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators.
    
    # %DRC%
    # @name transformed
    # @brief Transforms a layer
    # @synopsis layer.transformed(t)
    # 
    # Transforms the input layer by the given transformation and returns
    # the moved layer. The layer that this method is called upon is not modified.
    # This is the most generic method is transform a layer. The transformation
    # is a RBA::DCplxTrans object which describes many different kinds of affine transformations
    # except shear and anisotropic magnification.
    #
    # The following image shows the effect of the "moved" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_moved1.png) @/td
    #   @/tr
    # @/table

    # %DRC%
    # @name transform
    # @brief Transforms a layer (modifies the layer)
    # @synopsis layer.transform(t)
    # 
    # Like \transform, but modifies the input and returns a reference to it for
    # further processing.
    
    %w(extents moved transformed).each do |f| 
      eval <<"CODE"
      def #{f}(*args)
        aa = args.collect { |a| @engine._prep_value(a) }
        DRCLayer::new(@engine, @engine._cmd(@data, :#{f}, *aa))
      end
CODE
    end
    
    %w(move transform).each do |f| 
      eval <<"CODE"
      def #{f}(*args)
        aa = args.collect { |a| @engine._prep_value(a) }
        @engine._cmd(@data, :#{f}, *aa)
        self
      end
CODE
    end
    
    # %DRC%
    # @name merged 
    # @brief Returns the merged layer
    # @synopsis layer.merged([overlap_count])
    #
    # Returns the merged input. Usually, merging is done implicitly using the
    # \clean state (which is default). However, in raw state, merging can be 
    # enforced by using this method. In addition, this method allows specification
    # of a minimum overlap count, i.e. only where at least the given number of polygons
    # overlap, output is produced. See \sized for an application of that.
    #
    # This method works both on edge or polygon layers. Edge merging forms
    # single, continuous edges from coincident and connected individual edges.
    #
    # A version that modifies the input layer is \merge.
    #
    # The following images show the effect of various forms of the "merged" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_merged1.png) @/td
    #     @td @img(/images/drc_merged2.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_merged3.png) @/td
    #     @td @img(/images/drc_merged4.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name merge
    # @brief Merges the layer (modifies the layer)
    # @synopsis layer.merge([overlap_count])
    #
    # Like \merged, but modifies the input and returns a reference to the 
    # new layer.
    
    def merged(*args)
      requires_edges_or_region("merged")
      aa = args.collect { |a| @engine._prep_value(a) }
      DRCLayer::new(@engine, @engine._tcmd(@data, 0, @data.class, :merged, *aa))
    end
    
    def merge(*args)
      requires_edges_or_region("merge")
      aa = args.collect { |a| @engine._prep_value(a) }
      if @engine.is_tiled?
        # in tiled mode, no modifying versions are available
        @data = @engine._tcmd(@data, 0, @data.class, :merged, *aa)
      else
        @engine._tcmd(@data, 0, @data.class, :merge, *aa)
      end
      self
    end
    
    # %DRC%
    # @name output
    # @brief Outputs the content of the layer
    # @synopsis layer.output(specs)
    # 
    # This method will copy the content of the layer to the specified output.
    #
    # If a report database is selected for the output, the specification has to include a 
    # category name and optionally a category description.
    #
    # If the layout is selected for the output, the specification can consist of
    # one to three parameters: a layer number, a data type (optional, default is 0)
    # and a layer name (optional). Alternatively, the output can be specified by 
    # a single RBA::LayerInfo object.
    #
    # See \global#report and \global#target on how to configure output to a target layout
    # or report database. 
    
    def output(*args)
      @engine._vcmd(@engine, :_output, @data, *args)
    end
    
    # %DRC%
    # @name data
    # @brief Gets the low-level data object
    # @synopsis layer.data 
    #
    # This method returns a RBA::Region, RBA::Edges or RBA::EdgePairs object
    # representing the underlying RBA object for the data.
    # Access to these objects is provided to support low-level iteration and manipulation
    # of the layer's data. 
    
    def data
      @data
    end

    def requires_region(f)
      @data.is_a?(RBA::Region) || raise("#{f}: Requires a polygon layer")
    end
    
    def requires_texts_or_region(f)
      @data.is_a?(RBA::Region) || @data.is_a?(RBA::Texts) || raise("#{f}: Requires a polygon or text layer")
    end
    
    def requires_texts(f)
      @data.is_a?(RBA::Texts) || raise("#{f}: Requires a text layer")
    end
    
    def requires_edge_pairs(f)
      @data.is_a?(RBA::EdgePairs) || raise("#{f}: Requires a edge pair layer")
    end
    
    def requires_edges(f)
      @data.is_a?(RBA::Edges) || raise("#{f}: Requires an edge layer")
    end
    
    def requires_edges_or_region(f)
      @data.is_a?(RBA::Edges) || @data.is_a?(RBA::Region) || raise("#{f}: Requires an edge or polygon layer")
    end
    
    def requires_edges_texts_or_region(f)
      @data.is_a?(RBA::Edges) || @data.is_a?(RBA::Region) || @data.is_a?(RBA::Texts) || raise("#{f}: Requires an edge, text or polygon layer")
    end
    
    def requires_same_type(other, f)
      @data.class == other.data.class || raise("#{f}: Requires input of the same kind")
    end
    
  private
      
    def insert_object_into(container, object, dbu_trans)
      if object.is_a?(Array)
        object.each { |o| insert_object_into(container, o, dbu_trans) }
      elsif container.is_a?(RBA::Region)
        if object.is_a?(RBA::Region) || object.is_a?(RBA::Polygon) || object.is_a?(RBA::SimplePolygon) || object.is_a?(RBA::Box) || object.is_a?(RBA::Path)
          container.insert(object)
        elsif object.is_a?(RBA::DPolygon) || object.is_a?(RBA::DSimplePolygon) || object.is_a?(RBA::DBox) || object.is_a?(RBA::DPath)
          container.insert(object.transformed(dbu_trans))
        end
      elsif container.is_a?(RBA::Edges)
        if object.is_a?(RBA::Region) || object.is_a?(RBA::Edges) || object.is_a?(RBA::Edge) || object.is_a?(RBA::Polygon) || object.is_a?(RBA::SimplePolygon) || object.is_a?(RBA::Box) || object.is_a?(RBA::Path)
          container.insert(object)
        elsif object.is_a?(RBA::DPolygon) || object.is_a?(RBA::DSimplePolygon) || object.is_a?(RBA::DBox) || object.is_a?(RBA::DPath) || object.is_a?(RBA::DEdge)
          container.insert(object.transformed(dbu_trans))
        end
      elsif container.is_a?(RBA::EdgePairs)
        if object.is_a?(RBA::EdgePairs) || object.is_a?(RBA::EdgePair)
          container.insert(object)
        elsif object.is_a?(RBA::DEdgePair)
          container.insert(object.transformed(dbu_trans))
        end
      end
    end
  
  end
 
end

