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

      @engine._context("insert") do

        requires_edges_or_region
        args.each do |a|
          if a.is_a?(RBA::DBox) 
            self.data.insert(RBA::Box::from_dbox(a * (1.0 / @engine.dbu)))
          elsif a.is_a?(RBA::DPolygon) 
            self.data.insert(RBA::Polygon::from_dpoly(a * (1.0 / @engine.dbu)))
          elsif a.is_a?(RBA::DSimplePolygon) 
            self.data.insert(RBA::SimplePolygon::from_dpoly(a * (1.0 / @engine.dbu)))
          elsif a.is_a?(RBA::DPath) 
            self.data.insert(RBA::Path::from_dpath(a * (1.0 / @engine.dbu)))
          elsif a.is_a?(RBA::DEdge) 
            self.data.insert(RBA::Edge::from_dedge(a * (1.0 / @engine.dbu)))
          elsif a.is_a?(Array)
            insert(*a)
          else
            raise("Invalid argument type for #{a.inspect}")
          end
        end

        self

      end

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

      @engine._context("strict") do

        requires_region
        self.data.strict_handling = true
        self

      end

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

      @engine._context("non_strict") do

        requires_region
        self.data.strict_handling = false
        self

      end

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

      @engine._context("is_strict") do

        requires_region
        self.data.strict_handling?

      end

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

      @engine._context("clean") do

        requires_edges_or_region
        self.data.merged_semantics = true
        self

      end

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

      @engine._context("raw") do

        requires_edges_or_region
        self.data.merged_semantics = false
        self

      end

    end
    
    # %DRC% 
    # @name is_clean?
    # @brief Returns true, if the layer is clean state
    # @synopsis layer.is_clean?
    #
    # See \clean for a discussion of the clean state.
    
    def is_clean?

      @engine._context("is_clean?") do

        requires_edges_or_region
        self.data.merged_semantics?

      end

    end
    
    # %DRC% 
    # @name is_raw?
    # @brief Returns true, if the layer is raw state
    # @synopsis layer.is_raw?
    #
    # See \clean for a discussion of the raw state.
    
    def is_raw?

      @engine._context("is_raw?") do

        requires_edges_or_region
        !self.data.merged_semantics?

      end

    end
    
    # %DRC%
    # @name forget
    # @brief Cleans up memory for this layer
    # @synopsis forget
    #
    # KLayout's DRC engine is imperative. This means, every command is executed immediately
    # rather than being compiled and executed later. The advantage of this approach is that
    # it allows decisions to be taken depending on the content of a layer and to code 
    # functions that operate directly on the layer's content.
    #
    # However, one drawback is that the engine cannot decide when a layer is no longer 
    # required - it may still be used later in the script. So a layer's data is not cleaned
    # up automatically.
    #
    # In order to save memory for DRC scripts intended for bigger layouts, the DRC script
    # should clean up layers as soon as they are no longer required. The "forget" method
    # will free the memory used for the layer's information.
    #
    # The recommended approach is:
    #
    # @code
    # l = ... # compute some layer
    # ...
    # # once you're done with l:
    # l.forget
    # l = nil
    # @/code
    #
    # By setting the layer to nil, it is ensured that it can no longer be accessed.

    def forget

      @engine._context("forget") do

        if @data
          @data._destroy
          @data = nil
        end

      end

    end
    
    # %DRC%
    # @name count 
    # @brief Returns the number of objects on the layer
    # @synopsis layer.count
    #
    # The count is the number of raw objects, not merged
    # regions or edges. This is the flat count - the number of polygons,
    # edges or edge pairs seen from the top cell.
    # "count" can be computationally expensive for original layers with
    # clip regions or cell tree filters.
    #
    # See \hier_count for a hierarchical (each cell counts once) count.

    def count
      self.data.count
    end
    
    # %DRC%
    # @name hier_count 
    # @brief Returns the hierarchical number of objects on the layer
    # @synopsis layer.hier_count
    #
    # The hier_count is the number of raw objects, not merged
    # regions or edges, with each cell counting once. 
    # A high \count to hier_count (flat to hierarchical) ratio is an indication
    # of a good hierarchical compression.
    # "hier_count" applies only to original layers without clip regions or
    # cell filters and to layers in \deep mode. Otherwise, hier_count gives 
    # the same value than \count.

    def hier_count
      self.data.hier_count
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
      DRCLayer::new(@engine, self.data.dup)
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

          @engine._context("#{mn}") do

            requires_region
            if args.size == 1
              a = args[0]
              if a.is_a?(Range)
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_area_value_with_nil(a.begin), @engine._make_area_value_with_nil(a.end), #{inv.inspect}))
              else
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_area_value(a), #{inv.inspect}))
              end
            elsif args.size == 2
              DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_area_value_with_nil(args[0]), @engine._make_area_value_with_nil(args[1]), #{inv.inspect}))
            else
              raise("Invalid number of arguments (1 or 2 expected)")
            end

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
    
    %w(bbox_height bbox_max bbox_min bbox_width perimeter holes).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)

          @engine._context("#{mn}") do

            requires_region
            if args.size == 1
              a = args[0]
              if a.is_a?(Range)
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_value_with_nil(a.begin), @engine._make_value_with_nil(a.end), #{inv.inspect}))
              else
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_value(a), #{inv.inspect}))
              end
            elsif args.size == 2
              DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_value_with_nil(args[0]), @engine._make_value_with_nil(args[1]), #{inv.inspect}))
            else
              raise("Invalid number of arguments (1 or 2 expected)")
            end

          end

        end
CODE
      end
    end
    
    # %DRC%
    # @name with_holes
    # @brief Selects all polygons with the specified number of holes 
    # @synopsis layer.with_holes(count)
    # @synopsis layer.with_holes(min_count, max_count)
    # @synopsis layer.with_holes(min_count .. max_count)
    #
    # This method is available for polygon layers. It will select all polygons from the input layer
    # which have the specified number of holes.

    # %DRC%
    # @name without_holes
    # @brief Selects all polygons with the specified number of holes 
    # @synopsis layer.without_holes(count)
    # @synopsis layer.without_holes(min_count, max_count)
    # @synopsis layer.without_holes(min_count .. max_count)
    #
    # This method is available for polygon layers. It will select all polygons from the input layer
    # which do not have the specified number of holes.

    %w(holes).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)

          @engine._context("#{mn}") do

            requires_region
            if args.size == 1
              a = args[0]
              if a.is_a?(Range)
                min = @engine._make_numeric_value_with_nil(a.begin)
                max = @engine._make_numeric_value_with_nil(a.end)
                max && (max += 1)
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, min, max, #{inv.inspect}))
              else
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_value(a), #{inv.inspect}))
              end
            elsif args.size == 2
              min = @engine._make_numeric_value_with_nil(args[0])
              max = @engine._make_numeric_value_with_nil(args[1])
              max && (max += 1)
              DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, min, max, #{inv.inspect}))
            else
              raise("Invalid number of arguments (1 or 2 expected)")
            end

          end

        end
CODE
      end
    end
    
    # %DRC%
    # @name with_bbox_aspect_ratio
    # @brief Selects polygons by the aspect ratio of their bounding box
    # @synopsis layer.with_bbox_aspect_ratio(min .. max)
    # @synopsis layer.with_bbox_aspect_ratio(value)
    # @synopsis layer.with_bbox_aspect_ratio(min, max)
    # The method selects polygons similar to \with_area or \with_perimeter.
    # However, the measured value is the aspect ratio of the bounding 
    # box. It is the larger dimensions divided by the smaller one.
    # The "thinner" the polygon, the larger the aspect ratio. A square
    # bounding box gives an aspect ratio of 1. 
    #
    # This method is available for polygon layers only.

    # %DRC%
    # @name without_bbox_height
    # @brief Selects polygons by the aspect ratio of their bounding box
    # @synopsis layer.without_bbox_aspect_ratio(min .. max)
    # @synopsis layer.without_bbox_aspect_ratio(value)
    # @synopsis layer.without_bbox_aspect_ratio(min, max)
    # The method provides the opposite filter for \with_bbox_aspect_ratio.
    #
    # This method is available for polygon layers only.
    
    # %DRC%
    # @name with_area_ratio
    # @brief Selects polygons by the ratio of the bounding box area vs. polygon area
    # @synopsis layer.with_area_ratio(min .. max)
    # @synopsis layer.with_area_ratio(value)
    # @synopsis layer.with_area_ratio(min, max)
    # The area ratio is a measure how far a polygon is approximated by it's
    # bounding box. The value is always larger or equal to 1. Boxes have a 
    # area ratio of 1. Larger values mean more empty area inside the bounding box.
    # 
    # This method is available for polygon layers only.

    # %DRC%
    # @name without_area_ratio
    # @brief Selects polygons by the aspect ratio of their bounding box
    # @synopsis layer.without_area_ratio(min .. max)
    # @synopsis layer.without_area_ratio(value)
    # @synopsis layer.without_area_ratio(min, max)
    # The method provides the opposite filter for \with_area_ratio.
    #
    # This method is available for polygon layers only.
    
    # %DRC%
    # @name with_relative_height
    # @brief Selects polygons by the ratio of the height vs. width of it's bounding box
    # @synopsis layer.with_relative_height(min .. max)
    # @synopsis layer.with_relative_height(value)
    # @synopsis layer.with_relative_height(min, max)
    # The relative height is a measure how tall a polygon is. Tall polygons
    # have values larger than 1, wide polygons have a value smaller than 1.
    # Squares have a value of 1.
    #
    # Don't use this method when you can use \with_area_ratio, which provides a 
    # similar measure but is isotropic. 
    # 
    # This method is available for polygon layers only.

    # %DRC%
    # @name without_relative_height
    # @brief Selects polygons by the ratio of the height vs. width
    # @synopsis layer.without_relative_height(min .. max)
    # @synopsis layer.without_relative_height(value)
    # @synopsis layer.without_relative_height(min, max)
    # The method provides the opposite filter for \with_relative_height.
    #
    # This method is available for polygon layers only.
    
    %w(area_ratio bbox_aspect_ratio relative_height).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)

          @engine._context("#{mn}") do

            requires_region
            if args.size == 1
              a = args[0]
              if a.is_a?(Range)
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_numeric_value_with_nil(a.begin), @engine._make_numeric_value_with_nil(a.end), #{inv.inspect}))
              else
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_numeric_value(a), #{inv.inspect}))
              end
            elsif args.size == 2
              DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :with_#{f}, @engine._make_numeric_value(args[0]), @engine._make_numeric_value(args[1]), #{inv.inspect}))
            else
              raise("Invalid number of arguments (1 or 2 expected)")
            end

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
    # @synopsis edge_pairlayer.with_length(min, max [, both])
    # The method selects edges by their length. The first version selects
    # edges with a length larger or equal to min and less than max (but not equal).
    # The second version selects edges with exactly the given length. The third
    # version is similar to the first one, but allows specification of nil for min or
    # max indicating that there is no lower or upper limit. 
    #
    # This method is available for edge and edge pair layers.
    #
    # When called on an edge pair layer, this method will select edge pairs if one 
    # or both of the edges meet the length criterion. Use the additional argument 
    # and pass "both" (plain word) to specify that both edges need to be within the given interval.
    # By default, it's sufficient for one edge to meet the criterion.
    # 
    # Here are examples for "with_length" on edge pair layers:
    #
    # @code
    # # at least one edge needs to have a length of 1.0 <= l < 2.0
    # ep1 = edge_pairs.with_length(1.um .. 2.um)
    # # both edges need to have a length of exactly 2 um
    # ep2 = edge_pairs.with_length(2.um, both)
    # @/code

    # %DRC%
    # @name without_length
    # @brief Selects edges by the their length
    # @synopsis layer.without_length(min .. max)
    # @synopsis layer.without_length(value)
    # @synopsis layer.without_length(min, max)
    # @synopsis edge_pairlayer.with_length(min, max [, both])
    # The method basically is the inverse of \with_length. It selects all edges
    # of the edge layer which do not have the given length (second form) or are
    # not inside the given interval (first and third form).
    #
    # This method is available for edge and edge pair layers.
    #
    # A note on the "both" modifier (without_length called on edge pairs): "both" means that
    # both edges need to be "without_length". For example
    #
    # @code
    # # both edges are not exactly 1 um in length, or:
    # # the edge pair is skipped if one edge has a length of exactly 1 um
    # ep = edge_pairs.without_length(1.um, both)
    # @/code
    
    %w(length).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)

          @engine._context("#{mn}") do

            requires_edges_or_edge_pairs

            result_class = self.data.class

            f = :with_#{f} 
            args = args.select do |a|
              if a.is_a?(DRCBothEdges)
                if !self.data.is_a?(RBA::EdgePairs)
                  raise("'both' keyword only available for edge pair layers")
                end
                f = :with_#{f}_both
                false
              else
                true
              end
            end

            if args.size == 1
              a = args[0]
              if a.is_a?(Range)
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, f, @engine._make_value_with_nil(a.begin), @engine._make_value_with_nil(a.end), #{inv.inspect}))
              else
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, f, @engine._make_value(a), #{inv.inspect}))
              end
            elsif args.size == 2
              DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, f, @engine._make_value_with_nil(args[0]), @engine._make_value_with_nil(args[1]), #{inv.inspect}))
            else
              raise("Invalid number of range arguments (1 or 2 expected)")
            end

          end

        end
CODE
      end
    end
    
    # %DRC%
    # @name with_distance
    # @brief Selects edge pairs by the distance of the edges
    # @synopsis layer.with_distance(min .. max)
    # @synopsis layer.with_distance(value)
    # @synopsis layer.with_distance(min, max)
    # The method selects edge pairs by the distance of their edges. The first version selects
    # edge pairs with a distance larger or equal to min and less than max (but not equal).
    # The second version selects edge pairs with exactly the given distance. The third
    # version is similar to the first one, but allows specification of nil for min or
    # max indicating that there is no lower or upper limit. 
    # 
    # The distance of the edges is defined by the minimum distance of all points from the 
    # edges involved. For edge pairs generated in geometrical checks this is equivalent
    # to the actual distance of the original edges.
    #
    # This method is available for edge pair layers only.

    # %DRC%
    # @name without_distance
    # @brief Selects edge pairs by the distance of the edges
    # @synopsis layer.without_distance(min .. max)
    # @synopsis layer.without_distance(value)
    # @synopsis layer.without_distance(min, max)
    # The method basically is the inverse of \with_distance. It selects all edge pairs
    # of the edge pair layer which do not have the given distance (second form) or are
    # not inside the given interval (first and third form).
    #
    # This method is available for edge pair layers only.
    
    %w(distance).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)

          @engine._context("#{mn}") do

            requires_edge_pairs

            result_class = RBA::EdgePairs

            if args.size == 1
              a = args[0]
              if a.is_a?(Range)
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, :with_#{f}, @engine._make_value_with_nil(a.begin), @engine._make_value_with_nil(a.end), #{inv.inspect}))
              else
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, :with_#{f}, @engine._make_value(a), #{inv.inspect}))
              end
            elsif args.size == 2
              DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, :with_#{f}, @engine._make_value_with_nil(args[0]), @engine._make_value_with_nil(args[1]), #{inv.inspect}))
            else
              raise("Invalid number of range arguments (1 or 2 expected)")
            end

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
    # @synopsis edge_pairlayer.with_angle(min, max [, both])
    #
    # When called on an edge layer, the method selects edges by their angle, 
    # measured against the horizontal axis in the mathematical sense. 
    #
    # For this measurement edges are considered without their direction and straight lines. 
    # A horizontal edge has an angle of zero degree. A vertical one has
    # an angle of 90 degee. The angle range is from -90 (exclusive) to 90 degree (inclusive).
    #
    # The first version of this method selects
    # edges with a angle larger or equal to min and less than max (but not equal).
    # The second version selects edges with exactly the given angle. The third
    # version is identical to the first one. 
    #
    # When called on a polygon layer, this method selects corners which match the 
    # given angle or is within the given angle interval. The angle is measured between the edges forming the corner.
    # For each corner, an edge pair containing the edges forming in the angle is returned.
    #
    # When called on an edge pair layer, this method selects edge pairs with one or both edges
    # meeting the angle criterion. In this case an additional argument is accepted which can be
    # either "both" (plain word) to indicate that both edges have to be within the given interval.
    # Without this argument, it is sufficient for one edge to meet the criterion.
    #
    # Here are examples for "with_angle" on edge pair layers:
    #
    # @code
    # # at least one edge needs to be horizontal
    # ep1 = edge_pairs.with_angle(0)
    # # both edges need to vertical
    # ep2 = edge_pairs.with_angle(90, both)
    # @/code
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
    # @synopsis edge_pairlayer.without_angle(min, max [, both])
    #
    # The method basically is the inverse of \with_angle. It selects all edges
    # of the edge layer or corners of the polygons which do not have the given angle (second form) or whose angle
    # is not inside the given interval (first and third form).
    #
    # A note on the "both" modifier (without_angle called on edge pairs): "both" means that
    # both edges need to be "without_angle". For example
    #
    # @code
    # # both edges are not horizontal or:
    # # the edge pair is skipped if one edge is horizontal
    # ep = edge_pairs.without_angle(0, both)
    # @/code
    # 
    
    %w(angle).each do |f|
      [true, false].each do |inv|
        mn = (inv ? "without" : "with") + "_" + f
        eval <<"CODE"
        def #{mn}(*args)

          @engine._context("#{mn}") do

            f = :with_#{f}
            args = args.select do |a|
              if a.is_a?(DRCBothEdges)
                if !self.data.is_a?(RBA::EdgePairs)
                  raise("'both' keyword only available for edge pair layers")
                end
                f = :with_#{f}_both
                false
              else
                true
              end
            end

            result_class = self.data.is_a?(RBA::Edges) ? RBA::Edges : RBA::EdgePairs
            if args.size == 1
              a = args[0]
              if a.is_a?(Range)
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, f, a.begin, a.end, #{inv.inspect}))
              else
                DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, f, a, #{inv.inspect}))
              end
            elsif args.size == 2
              DRCLayer::new(@engine, @engine._tcmd(self.data, 0, result_class, f, args[0], args[1], #{inv.inspect}))
            else
              raise("Invalid number of range arguments (1 or 2 expected)")
            end

          end

        end
CODE
      end
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
      @engine._context("rounded_corners") do
        requires_region
        DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :rounded_corners, @engine._make_value(inner), @engine._make_value(outer), n))
      end
    end
    
    # %DRC%
    # @name smoothed
    # @brief Smoothes the polygons of the region
    # @synopsis layer.smoothed(d)
    # @synopsis layer.smoothed(d, hv_keep)
    #
    # "Smoothing" returns a simplified version of the polygons. Simplification is 
    # achieved by removing vertices unless the resulting polygon deviates by more
    # than the given distance d from the original polygon.
    #
    # "hv_keep" is a boolean parameter which makes the smoothing function maintain
    # horizontal or vertical edges. The default is false, meaning horizontal or
    # vertical edges may be changed into tilted ones.
    #
    # This method return a layer wit the modified polygons. Merged semantics applies for this
    # method (see \raw and \clean).
    
    def smoothed(d, hv_keep = false)
      @engine._context("smoothed") do
        requires_region
        DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :smoothed, @engine._make_value(d), hv_keep))
      end
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
      @engine._context("texts") do
        requires_texts_or_region
        self._texts_impl(false, *args)
      end
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
      @engine._context("texts_not") do
        requires_texts
        self._texts_impl(true, *args)
      end
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
        elsif a.is_a?(DRCOutputMode)
          as_dots = (a.value == :edges || a.value == :dots)
        else
          raise("Invalid argument type #{a.inspect}")
        end
      end

      if self.data.is_a?(RBA::Texts)
        if as_pattern
          result = @engine._tcmd(self.data, 0, RBA::Texts, :with_match, pattern, invert)
        else
          result = @engine._tcmd(self.data, 0, RBA::Texts, :with_text, pattern, invert)
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
          return DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :texts_dots, pattern, as_pattern))
        else
          return DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :texts, pattern, as_pattern))
        end
      end

    end
     
    # %DRC%
    # @name corners
    # @brief Selects corners of polygons
    # @synopsis layer.corners([ options ])
    # @synopsis layer.corners(angle [, options ])
    # @synopsis layer.corners(amin .. amax [, options ])
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
    #   @li @b as_edge_pairs @/b: with this option, an edge pair is produced for each corner selected. The first edge 
    #                             is the incoming edge to the corner, the second edge the outgoing edge. @/li  
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

      @engine._context("corners") do

        requires_region

        output_mode = :boxes
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
          elsif a.is_a?(DRCOutputMode)
            output_mode = a.value
          else
            raise("Invalid argument #{a.inspect}")
          end
        end

        f = :corners
        cls = RBA::Region
        if output_mode == :edges || output_mode == :dots
          f = :corners_dots
          cls = RBA::Edges
        elsif output_mode == :edge_pairs
          f = :corners_edge_pairs
          cls = RBA::EdgePairs
        end
        DRCLayer::new(@engine, @engine._tcmd(self.data, 0, cls, f, amin, amax))

      end

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

        @engine._context("#{f}") do

          requires_region

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
            elsif a.is_a?(DRCOutputMode)
              as_edges = (a.value == :edges || a.value == :dots)
            elsif @@std_refs[a] && :#{f} != :middle
              f = @@std_refs[a]
            else
              raise("Invalid argument: " + a.inspect)
            end
          end

          if f.size == 2
            f = f + f
          else
            f = (f + [0.5] * 4)[0..3]
          end
              
          if as_edges
            DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :extent_refs_edges, *f))
          else
            # add oversize for point- and edge-like regions
            zero_area = (f[0] - f[2]).abs < 1e-7 || (f[1] - f[3]).abs < 1e-7
            f += [ zero_area ? 1 : 0 ] * 2
            DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :extent_refs, *f))
          end

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

      @engine._wrapper_context("select") do

        new_data = self.data.class.new
        t = RBA::CplxTrans::new(@engine.dbu)
        @engine.run_timed("\"select\" in: #{@engine.src_line}", self.data) do
          self.data.send(new_data.is_a?(RBA::EdgePairs) ? :each : :each_merged) do |object| 
            block.call(object.transformed(t)) && new_data.insert(object)
          end
          new_data
        end
        DRCLayer::new(@engine, new_data)

      end

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

      @engine._wrapper_context("each") do

        t = RBA::CplxTrans::new(@engine.dbu)
        @engine.run_timed("\"each\" in: #{@engine.src_line}", self.data) do
          self.data.send(self.data.is_a?(RBA::EdgePairs) ? :each : :each_merged) do |object| 
            block.call(object.transformed(t))
          end
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
    # @synopsis layer.collect_to_region { |object| ... }
    # This method is similar to \collect, but creates a polygon layer. It expects the block to 
    # deliver objects that can be converted into polygons. Such objects are of class RBA::DPolygon,
    # RBA::DBox, RBA::DPath, RBA::Polygon, RBA::Path, RBA::Box and RBA::Region.
    
    # %DRC%
    # @name collect_to_edges
    # @brief Transforms a layer into edge objects
    # @synopsis layer.collect_to_edges { |object| ... }
    # This method is similar to \collect, but creates an edge layer. It expects the block to 
    # deliver objects that can be converted into edges. If polygon-like objects are returned, their
    # contours will be turned into edge sequences.
    
    # %DRC%
    # @name collect_to_edge_pairs
    # @brief Transforms a layer into edge pair objects
    # @synopsis layer.collect_to_edge_pairs { |object| ... }
    # This method is similar to \collect, but creates an edge pair layer. It expects the block to 
    # deliver RBA::EdgePair, RBA::DEdgePair or RBA::EdgePairs objects.
    
    %w(collect collect_to_region collect_to_edges collect_to_edge_pairs).each do |f| 
      eval <<"CODE"
      def #{f}(&block)

        @engine._wrapper_context("#{f}") do

          if :#{f} == :collect
            new_data = self.data.class.new
          elsif :#{f} == :collect_to_region
            new_data = RBA::Region.new
          elsif :#{f} == :collect_to_edges
            new_data = RBA::Edges.new
          elsif :#{f} == :collect_to_edge_pairs
            new_data = RBA::EdgePairs.new
          end

          t = RBA::CplxTrans::new(@engine.dbu)
          dbu_trans = RBA::VCplxTrans::new(1.0 / @engine.dbu)

          @engine.run_timed("\\"#{f}\\" in: " + @engine.src_line, self.data) do
            self.data.send(new_data.is_a?(RBA::EdgePairs) ? :each : :each_merged) do |object| 
              insert_object_into(new_data, block.call(object.transformed(t)), dbu_trans)
            end
            new_data
          end

          DRCLayer::new(@engine, new_data)

        end

      end
CODE
    end
    
    # %DRC%
    # @name odd_polygons
    # @brief Checks for odd polygons (self-overlapping, non-orientable)
    # @synopsis layer.odd_polygons
    # Returns the parts of the polygons which are not orientable (i.e. "8" configuration) or self-overlapping.
    # Merged semantics does not apply for this method. Always the raw polygons are taken (see \raw).
    #
    # The odd_polygons check is not available in deep mode currently. See \deep_reject_odd_polygons for
    # an alternative.
    
    def odd_polygons
      @engine._context("odd_polygons") do
        if is_deep?
          @engine.error("'odd_polygons' is not performing any check in deep mode - use 'deep_reject_odd_polygons' instead")
          return @engine.polygons
        else
          requires_region
          return DRCLayer::new(@engine, @engine._vcmd(self.data, :strange_polygon_check))
        end
      end
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
      @engine._context("ongrid") do
        requires_region
        if args.size == 1
          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::EdgePairs, :grid_check, @engine._make_value(args[0]), @engine._make_value(args[0])))
        elsif args.size == 2
          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::EdgePairs, :grid_check, @engine._make_value(args[0]), @engine._make_value(args[1])))
        else
          raise("Invalid number of arguments (1 or 2 expected)")
        end
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

        @engine._context("#{f}") do

          requires_region
          gx = gy = 0
          if args.size == 1
            gx = gy = @engine._make_value(args[0])
          elsif args.size == 2
            gx = @engine._make_value(args[0])
            gy = @engine._make_value(args[1])
          else
            raise("Invalid number of arguments (1 or 2 expected)")
          end
          if :#{f} == :snap && @engine.is_tiled?
            # in tiled mode, no modifying versions are available
            self.data = @engine._tcmd(self.data, 0, self.data.class, :snapped, gx, gy)
            self
          elsif :#{f} == :snap
            @engine._tcmd(self.data, 0, self.data.class, :#{f}, gx, gy)
            self
          else
            DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, gx, gy))
          end

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
      @engine._context("and") do
        self & other    
      end
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
      @engine._context("not") do
        self - other    
      end
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
      @engine._context("xor") do
        self ^ other    
      end
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
      @engine._context("or") do
        self | other    
      end
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
      @engine._context("join") do
        self + other
      end
    end
    
    # %DRC%
    # @name andnot
    # @brief Computes Boolean AND and NOT results at the same time
    # @synopsis layer.andnot(other)
    # This method returns a two-element array containing one layer for the
    # AND result and one for the NOT result.
    #
    # This method is available for polygon layers.
    #
    # It can be used to initialize two variables with the AND and NOT results:
    #
    # @code
    # (and_result, not_result) = l1.andnot(l2)
    # @/code
    #
    # As the AND and NOT results are computed in the same sweep, calling this
    # method is faster than calling AND and NOT separately.
   
    def andnot(other)

      @engine._context("andnot") do

        check_is_layer(other)
        requires_region
        other.requires_region

        res = @engine._tcmd_a2(self.data, 0, self.data.class, self.data.class, :andnot, other.data)

        [ DRCLayer::new(@engine, res[0]), DRCLayer::new(@engine, res[1]) ]

      end

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
    # @name covering
    # @brief Selects shapes or regions of self which completely cover (enclose) one or more shapes from the other region
    # @synopsis layer.covering(other)
    # @synopsis layer.covering(other, min_count)
    # @synopsis layer.covering(other, min_count, max_count)
    # @synopsis layer.covering(other, min_count .. max_count)
    # This method selects all shapes or regions from self which completly cover shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_covering.
    #
    # This method is available for polygons only.
    #
    # The following image shows the effect of the "covering" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_covering.png) @/td
    #   @/tr
    # @/table
    #
    # A range of counts can be specified. If so, the shape from the primary layer is 
    # only selected when covering a given number of shapes from the other layer.
    # For the interpretation of the count see \interacting.
    #
    # The "covering" attribute is sometimes called "enclosing", but this name is
    # used for the respective DRC function (see \enclosing).
    
    # %DRC%
    # @name not_covering
    # @brief Selects shapes or regions of self which do not cover (enclose) one or more shapes from the other region
    # @synopsis layer.not_covering(other)
    # @synopsis layer.not_covering(other, min_count)
    # @synopsis layer.not_covering(other, min_count, max_count)
    # @synopsis layer.not_covering(other, min_count .. max_count)
    # This method selects all shapes or regions from self which do not cover shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected. This method returns the inverse of \covering
    # and provides the same options.
    #
    # The following image shows the effect of the "not_covering" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_not_covering.png) @/td
    #   @/tr
    # @/table
    #
    # This method is available for polygons only.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_not_covering.
    
    # %DRC%
    # @name split_covering
    # @brief Returns the results of \covering and \not_covering at the same time
    # @synopsis (a, b) = layer.split_covering(other [, options ])
    #
    # This method returns the polygons covering polygons from the other layer in 
    # one layer and all others in a second layer. This method is equivalent to calling 
    # \covering and \not_covering, but is faster than doing this in separate steps:
    #
    # @code
    # (covering, not_covering) = l1.split_covering(l2)
    # @/code
    #
    # The options of this method are the same than \covering.
    
    # %DRC%
    # @name select_covering
    # @brief Selects shapes or regions of self which completely cover (enclose) one or more shapes from the other region
    # @synopsis layer.select_covering(other)
    # @synopsis layer.select_covering(other, min_count)
    # @synopsis layer.select_covering(other, min_count, max_count)
    # @synopsis layer.select_covering(other, min_count .. max_count)
    # This method selects all shapes or regions from self which cover shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \covering.
    #
    # This method is available for polygons only.
    
    # %DRC%
    # @name select_not_covering
    # @brief Selects shapes or regions of self which do not cover (enclose) one or more shapes from the other region
    # @synopsis layer.select_not_covering(other)
    # @synopsis layer.select_not_covering(other, min_count)
    # @synopsis layer.select_not_covering(other, min_count, max_count)
    # @synopsis layer.select_not_covering(other, min_count .. max_count)
    # This method selects all shapes or regions from self which do not cover shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected. 
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \not_covering.
    #
    # This method is available for polygons only.

    # %DRC%
    # @name overlapping
    # @brief Selects shapes or regions of self which overlap shapes from the other region
    # @synopsis layer.overlapping(other)
    # @synopsis layer.overlapping(other, min_count)
    # @synopsis layer.overlapping(other, min_count, max_count)
    # @synopsis layer.overlapping(other, min_count .. max_count)
    # This method selects all shapes or regions from self which overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_overlapping.
    #
    # This method is available for polygons only.
    #
    # The following image shows the effect of the "overlapping" method:
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_overlapping.png) @/td
    #   @/tr
    # @/table
    #
    # A range of counts can be specified. If so, the shape from the primary layer is 
    # only selected when overlapping a given number of shapes from the other layer.
    # For the interpretation of the count see \interacting.
    
    # %DRC%
    # @name not_overlapping
    # @brief Selects shapes or regions of self which do not overlap shapes from the other region
    # @synopsis layer.not_overlapping(other)
    # @synopsis layer.not_overlapping(other, min_count)
    # @synopsis layer.not_overlapping(other, min_count, max_count)
    # @synopsis layer.not_overlapping(other, min_count .. max_count)
    # This method selects all shapes or regions from self which do not overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected. This method will return the inverse of \overlapping
    # and provides the same options.
    #
    # The "not_overlapping" method is similar to the \outside method. However, "outside" does 
    # not provide the option to specify counts.
    #
    # This method is available for polygons only.
    # It returns a new layer containing the selected shapes. A version which modifies self
    # is \select_not_overlapping.
    
    # %DRC%
    # @name split_overlapping
    # @brief Returns the results of \overlapping and \not_overlapping at the same time
    # @synopsis (a, b) = layer.split_overlapping(other [, options ])
    #
    # This method returns the polygons overlapping polygons from the other layer in 
    # one layer and all others in a second layer. This method is equivalent to calling 
    # \overlapping and \not_overlapping, but is faster than doing this in separate steps:
    #
    # @code
    # (overlapping, not_overlapping) = l1.split_overlapping(l2)
    # @/code
    #
    # The options of this method are the same than \overlapping.
    
    # %DRC%
    # @name select_overlapping
    # @brief Selects shapes or regions of self which overlap shapes from the other region
    # @synopsis layer.select_overlapping(other)
    # @synopsis layer.select_overlapping(other, min_count)
    # @synopsis layer.select_overlapping(other, min_count, max_count)
    # @synopsis layer.select_overlapping(other, min_count .. max_count)
    # This method selects all shapes or regions from self which overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \overlapping.
    #
    # This method is available for polygons only.
    
    # %DRC%
    # @name select_not_overlapping
    # @brief Selects shapes or regions of self which do not overlap shapes from the other region
    # @synopsis layer.select_not_overlapping(other)
    # @synopsis layer.select_not_overlapping(other, min_count)
    # @synopsis layer.select_not_overlapping(other, min_count, max_count)
    # @synopsis layer.select_not_overlapping(other, min_count .. max_count)
    # This method selects all shapes or regions from self which do not overlap shapes from the other
    # region. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected. 
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \not_overlapping.
    #
    # This method is available for polygons only.
    
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
    # @name split_inside
    # @brief Returns the results of \inside and \not_inside at the same time
    # @synopsis (a, b) = layer.split_inside(other)
    #
    # This method returns the polygons inside of polygons from the other layer in 
    # one layer and all others in a second layer. This method is equivalent to calling 
    # \inside and \not_inside, but is faster than doing this in separate steps:
    #
    # @code
    # (inside, not_inside) = l1.split_inside(l2)
    # @/code
    
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
    # @name split_outside
    # @brief Returns the results of \outside and \not_outside at the same time
    # @synopsis (a, b) = layer.split_outside(other)
    #
    # This method returns the polygons outside of polygons from the other layer in 
    # one layer and all others in a second layer. This method is equivalent to calling 
    # \outside and \not_outside, but is faster than doing this in separate steps:
    #
    # @code
    # (outside, not_outside) = l1.split_outside(l2)
    # @/code
    
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
    # @synopsis layer.interacting(other, min_count)
    # @synopsis layer.interacting(other, min_count, max_count)
    # @synopsis layer.interacting(other, min_count .. max_count)
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
    #
    # If a single count is given, shapes from self are selected only if they do interact at least with the given
    # number of (different) shapes from the other layer. If a min and max count is given, shapes from  
    # self are selected only if they interact with min_count or more, but a maximum of max_count different shapes
    # from the other layer. Two polygons overlapping or touching at two locations are counted as single interactions.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_interacting2.png) @/td
    #     @td @img(/images/drc_interacting3.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_interacting4.png) @/td
    #     @td @img(/images/drc_interacting5.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name not_interacting
    # @brief Selects shapes or regions of self which do not touch or overlap shapes from the other region
    # @synopsis layer.not_interacting(other)
    # @synopsis layer.not_interacting(other, min_count)
    # @synopsis layer.not_interacting(other, min_count, max_count)
    # @synopsis layer.not_interacting(other, min_count .. max_count)
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
    #
    # If a single count is given, shapes from self are selected only if they interact with less than the given
    # number of (different) shapes from the other layer. If a min and max count is given, shapes from  
    # self are selected only if they interact with less than min_count or more than max_count different shapes
    # from the other layer. Two polygons overlapping or touching at two locations are counted as single interactions.
    #
    # @table
    #   @tr 
    #     @td @img(/images/drc_not_interacting2.png) @/td
    #     @td @img(/images/drc_not_interacting3.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_not_interacting4.png) @/td
    #     @td @img(/images/drc_not_interacting5.png) @/td
    #   @/tr
    # @/table
    
    # %DRC%
    # @name split_interacting
    # @brief Returns the results of \interacting and \not_interacting at the same time
    # @synopsis (a, b) = layer.split_interacting(other [, options ])
    #
    # This method returns the polygons interacting with objects from the other container in 
    # one layer and all others in a second layer. This method is equivalent to calling 
    # \interacting and \not_interacting, but is faster than doing this in separate steps:
    #
    # @code
    # (interacting, not_interacting) = l1.split_interacting(l2)
    # @/code
    #
    # The options of this method are the same than \interacting.
    
    # %DRC%
    # @name select_interacting
    # @brief Selects shapes or regions of self which touch or overlap shapes from the other region
    # @synopsis layer.select_interacting(other)
    # @synopsis layer.select_interacting(other, min_count)
    # @synopsis layer.select_interacting(other, min_count, max_count)
    # @synopsis layer.select_interacting(other, min_count .. max_count)
    # This method selects all shapes or regions from self which touch or overlap shapes from the other
    # layer. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \interacting.
    #
    # This method is available for polygon, text and edge layers. Edges can be selected
    # with respect to other edges or polygons. Texts can be selected with respect to 
    # polygons. Polygons can be selected with respect to edges, texts and other polygons.
    #
    # If a single count is given, shapes from self are selected only if they do interact at least with the given
    # number of (different) shapes from the other layer. If a min and max count is given, shapes from  
    # self are selected only if they interact with min_count or more, but a maximum of max_count different shapes
    # from the other layer. Two polygons overlapping or touching at two locations are counted as single interactions.
    
    # %DRC%
    # @name select_not_interacting
    # @brief Selects shapes or regions of self which do not touch or overlap shapes from the other region
    # @synopsis layer.select_not_interacting(other)
    # @synopsis layer.select_not_interacting(other, min_count)
    # @synopsis layer.select_not_interacting(other, min_count, max_count)
    # @synopsis layer.select_not_interacting(other, min_count .. max_count)
    # This method selects all shapes or regions from self which do not touch or overlap shapes from the other
    # layer. Unless self is in raw mode (see \raw), coherent regions are selected from self, 
    # otherwise individual shapes are selected.
    # It modifies self to contain the selected shapes. A version which does not modify self
    # is \not_interacting.
    #
    # This method is available for polygon, text and edge layers. Edges can be selected
    # with respect to other edges or polygons. Texts can be selected with respect to 
    # polygons. Polygons can be selected with respect to edges, texts and other polygons.
    #
    # If a single count is given, shapes from self are selected only if they interact with less than the given
    # number of (different) shapes from the other layer. If a min and max count is given, shapes from  
    # self are selected only if they interact with less than min_count or more than max_count different shapes
    # from the other layer. Two polygons overlapping or touching at two locations are counted as single interactions.

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
 
        @engine._context("#{f}") do

          check_is_layer(other)
          if :#{f} != :pull_interacting 
            requires_region
            other.requires_region
          else
            requires_edges_texts_or_region
            if self.data.is_a?(RBA::Text)
              other.requires_region
            elsif self.data.is_a?(RBA::Region)
              other.requires_edges_texts_or_region
            else
              other.requires_edges_or_region
            end
          end

          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, other.data.class, :#{f}, other.data))

        end

      end
CODE
    end

    %w(| ^ inside not_inside outside not_outside in not_in).each do |f| 
      eval <<"CODE"
      def #{f}(other)

        @engine._context("#{f}") do

          requires_same_type(other)
          requires_edges_or_region

          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data))

        end

      end
CODE
    end

    %w(& -).each do |f| 
      eval <<"CODE"
      def #{f}(other)

        @engine._context("#{f}") do

          check_is_layer(other)
          other.requires_edges_texts_or_region
          if self.data.is_a?(RBA::Texts)
            other.requires_region
          else
            other.requires_edges_or_region
          end

          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data))

        end

      end
CODE
    end

    %w(+).each do |f| 
      eval <<"CODE"
      def #{f}(other)

        @engine._context("#{f}") do

          requires_same_type(other)
          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data))

        end

      end
CODE
    end

    %w(interacting not_interacting).each do |f| 
      eval <<"CODE"
      def #{f}(other, *args)

        @engine._context("#{f}") do

          check_is_layer(other)
          if self.data.is_a?(RBA::Text)
            other.requires_region
          elsif self.data.is_a?(RBA::Region)
            other.requires_edges_texts_or_region
          else
            other.requires_edges_or_region
          end

          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data, *minmax_count(*args)))

        end

      end
CODE
    end

    %w(interacting not_interacting).each do |fi|
      f = "select_" + fi
      # In tiled mode, there are no modifying versions. Emulate using the non-modifying one.
      eval <<"CODE"
      def #{f}(other, *args)

        @engine._context("#{f}") do

          check_is_layer(other)
          requires_edges_texts_or_region
          if self.data.is_a?(RBA::Text)
            other.requires_region
          elsif self.data.is_a?(RBA::Region)
            other.requires_edges_texts_or_region
          else
            other.requires_edges_or_region
          end

          if @engine.is_tiled?
            self.data = @engine._tcmd(self.data, 0, self.data.class, :#{fi}, other.data, *minmax_count(*args))
            DRCLayer::new(@engine, self.data)
          else
            DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data, *minmax_count(*args)))
          end

        end

      end
CODE
    end
    
    %w(split_interacting).each do |f|
      eval <<"CODE"
      def #{f}(other, *args)

        @engine._context("#{f}") do

          check_is_layer(other)
          requires_region
          other.requires_edges_texts_or_region

          res = @engine._tcmd_a2(self.data, 0, self.data.class, self.data.class, :#{f}, other.data, *minmax_count(*args))
          [ DRCLayer::new(@engine, res[0]), DRCLayer::new(@engine, res[1]) ]

        end

      end
CODE
    end
    
    %w(overlapping not_overlapping covering not_covering).each do |f| 
      eval <<"CODE"

      def #{f}(other, *args)

        @engine._context("#{f}") do

          requires_same_type(other)
          requires_region

          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data, *minmax_count(*args)))

        end

      end
CODE
    end

    %w(overlapping not_overlapping covering not_covering).each do |fi|
      f = "select_" + fi
      # In tiled mode, there are no modifying versions. Emulate using the non-modifying one.
      eval <<"CODE"
      def #{f}(other, *args)

        @engine._context("#{f}") do

          requires_region
          requires_same_type(other)

          if @engine.is_tiled?
            self.data = @engine._tcmd(self.data, 0, self.data.class, :#{fi}, other.data, *minmax_count(*args))
            DRCLayer::new(@engine, self.data)
          else
            DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data, *minmax_count(*args)))
          end

        end

      end
CODE
    end
    
    %w(split_overlapping split_covering).each do |f|
      eval <<"CODE"
      def #{f}(other, *args)

        @engine._context("#{f}") do

          requires_region
          other.requires_region

          res = @engine._tcmd_a2(self.data, 0, self.data.class, self.data.class, :#{f}, other.data, *minmax_count(*args))
          [ DRCLayer::new(@engine, res[0]), DRCLayer::new(@engine, res[1]) ]

        end

      end
CODE
    end
    
    %w(inside not_inside outside not_outside).each do |fi|
      f = "select_" + fi
      # In tiled mode, there are no modifying versions. Emulate using the non-modifying one.
      eval <<"CODE"
      def #{f}(other)

        @engine._context("#{f}") do

          requires_region
          requires_same_type(other)

          if @engine.is_tiled?
            self.data = @engine._tcmd(self.data, 0, self.data.class, :#{fi}, other.data)
            DRCLayer::new(@engine, self.data)
          else
            DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data))
          end

        end

      end
CODE
    end
    
    %w(split_inside split_outside).each do |f|
      eval <<"CODE"
      def #{f}(other)

        @engine._context("#{f}") do

          check_is_layer(other)
          requires_region
          other.requires_region

          res = @engine._tcmd_a2(self.data, 0, self.data.class, self.data.class, :#{f}, other.data)
          [ DRCLayer::new(@engine, res[0]), DRCLayer::new(@engine, res[1]) ]

        end

      end
CODE
    end
    
    %w(inside_part outside_part).each do |f|
      eval <<"CODE"
      def #{f}(other)

        @engine._context("#{f}") do

          check_is_layer(other)
          other.requires_region
          requires_edges

          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data))

        end

      end
CODE
    end
    
    %w(intersections).each do |f|
      eval <<"CODE"
      def #{f}(other)

        @engine._context("#{f}") do

          check_is_layer(other)
          other.requires_edges
          requires_edges

          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :#{f}, other.data))

        end

      end
CODE
    end
    
    # %DRC%
    # @name rectangles
    # @brief Selects all rectangles from the input
    # @synopsis layer.rectangles
    #
    # This method is available for polygon layers. By default "merged" semantics applies, 
    # i.e. all polygons are merged before rectangles are selected (see \clean and \raw).
    # \non_rectangles will select all non-rectangles.

    # %DRC%
    # @name squares
    # @brief Selects all squares from the input
    # @synopsis layer.squares
    #
    # This method is available for polygon layers. By default "merged" semantics applies, 
    # i.e. all polygons are merged before squares are selected (see \clean and \raw).
    # \non_squares will select all non-rectangles.

    # %DRC%
    # @name rectilinear
    # @brief Selects all rectilinear polygons from the input
    # @synopsis layer.rectilinear
    #
    # This method is available for polygon layers. By default "merged" semantics applies, 
    # i.e. all polygons are merged before rectilinear polygons are selected (see \clean and \raw).
    # \non_rectilinear will select all non-rectangles.
    
    # %DRC%
    # @name non_squares
    # @brief Selects all polygons from the input which are not squares
    # @synopsis layer.non_rectangles
    #
    # This method is available for polygon layers. By default "merged" semantics applies, 
    # i.e. all polygons are merged before non-squares are selected (see \clean and \raw).
    
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

    %w(rectangles rectilinear non_rectangles non_rectilinear squares non_squares
       holes hulls).each do |f| 
      eval <<"CODE"
      def #{f}
        @engine._context("#{f}") do
          requires_region
          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :#{f}))
        end
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
        @engine._context("#{f}") do
          requires_edges
          length = @engine._make_value(length)
          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Edges, :#{f}, length, fraction))
        end
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
      
        @engine._context("#{f}") do

          requires_edges
          
          av = [ 0, 0, 0, 0, false ]
          args.each_with_index do |a,i|
            if a.is_a?(Hash)
              a[:begin]  && av[0] = @engine._make_value(a[:begin])
              a[:end]    && av[1] = @engine._make_value(a[:end])
              a[:out]    && av[2] = @engine._make_value(a[:out])
              a[:in]     && av[3] = @engine._make_value(a[:in])
              a[:joined] && av[4] = true
            elsif i < 4
              if !a.is_a?(1.class) && !a.is_a?(Float)
                raise("Invalid type for argument #" + (i+1).to_s)
              end
              av[i] = @engine._make_value(a)
            elsif i == 4
              if a.is_a?(DRCJoinFlag)
                av[i] = a.value
              else
                av[i] = (a ? true : false)
              end
            else
              raise("Too many arguments (1 to 5 expected)")
            end
          end

          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :#{f}, *av))

        end

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
        @engine._context("#{f}") do
          requires_edges
          DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Region, :#{f}, @engine._make_value(dist)))
        end
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
        @engine._context("#{f}") do
          if self.data.is_a?(RBA::Region)
            DRCLayer::new(@engine, @engine._tcmd(self.data, 0, RBA::Edges, :#{f}))
          elsif self.data.is_a?(RBA::EdgePairs)
            DRCLayer::new(@engine, @engine._cmd(self.data, :#{f}))
          else
            raise "Layer must be a polygon or edge pair layer"
          end
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
    #
    # Some checks deliver symmetric edge pairs (e.g. space, width, etc.) for which the
    # edges are commutable. "first_edges" will deliver both edges for such edge pairs.
    
    # %DRC%
    # @name second_edges
    # @brief Returns the second edges of an edge pair collection
    # @synopsis layer.second_edges
    # 
    # Applies to edge pair collections only.
    # Returns the second edges of the edge pairs in the collection.
    #
    # Some checks deliver symmetric edge pairs (e.g. space, width, etc.) for which the
    # edges are commutable. "second_edges" will not deliver edges for such edge pairs.
    # Instead, "first_edges" will deliver both.
    
    %w(first_edges second_edges).each do |f| 
      eval <<"CODE"
      def #{f}
        @engine._context("#{f}") do
          requires_edge_pairs
          DRCLayer::new(@engine, @engine._cmd(self.data, :#{f}))
        end
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
      RBA::DBox::from_ibox(self.data.bbox) * @engine.dbu.to_f
    end
    
    # %DRC%
    # @name polygons?
    # @brief Returns true, if the layer is a polygon layer
    # @synopsis layer.polygons?
    
    def polygons?
      self.data.is_a?(RBA::Region)
    end
    
    # %DRC%
    # @name edges?
    # @brief Returns true, if the layer is an edge layer
    # @synopsis layer.edges?
    
    def edges?
      self.data.is_a?(RBA::Edges)
    end
    
    # %DRC%
    # @name edge_pairs?
    # @brief Returns true, if the layer is an edge pair collection
    # @synopsis layer.edge_pairs?
    
    def edge_pairs?
      self.data.is_a?(RBA::EdgePairs)
    end
    
    # %DRC%
    # @name is_deep?
    # @brief Returns true, if the layer is a deep (hierarchical) layer
    # @synopsis layer.is_deep?
    
    def is_deep?
      self.data.respond_to?(:is_deep?) && self.data.is_deep?
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
      @engine._context("area") do
        requires_region
        @engine._tdcmd(self.data, 0, :area) * (@engine.dbu.to_f * @engine.dbu.to_f)
      end
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
      @engine._context("perimeter") do
        requires_region
        # Note: we have to add 1 DBU border to collect the neighbors. It's important
        # to know then since they tell us whether an edge is an outside edge.
        @engine._tdcmd(self.data, 1, :perimeter) * @engine.dbu.to_f
      end
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
      @engine._context("is_box?") do
        requires_region
        @engine._cmd(self.data, :is_box?)
      end
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
      @engine._context("length") do
        requires_edges
        @engine._cmd(self.data, :length) * @engine.dbu.to_f
      end
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
      @engine._context("flatten") do
        DRC::DRCLayer::new(@engine, @engine._cmd(self.data, :flatten))
      end
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
      @engine._context("is_merged?") do
        requires_edges_or_region
        self.data.is_merged?
      end
    end
    
    # %DRC%
    # @name is_empty?
    # @brief Returns true, if the layer is empty
    # @synopsis layer.is_empty?
    
    def is_empty?
      self.data.is_empty?
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
    # @b Note: @/b "width" is available as an operator for the "universal DRC" function \Layer#drc within
    # the \DRC framework. This variant has more options and is more intuitive to use. See \global#width for more details.
    #
    # This method performs a width check and returns a collection of edge pairs.
    # A width check can be performed on polygon and edge layers. On edge layers, all
    # edges are checked against all other edges. If two edges form a "back to back" relation
    # (i.e. their inner sides face each other) and their distance is less than the specified
    # value, an error shape is generated for that edge pair. 
    # On polygon layers, the polygons on each layer are checked for locations where their
    # width is less than the specified value. In that case, an edge pair error shape is generated.
    # 
    # @h3 Options @/h3
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
    #   @li @b projecting (in condition) @/b: This specification is equivalent to "projection_limits"
    #         but is more intuitive, as "projecting" is written with a condition, like
    #         "projecting < 2.um". Available operators are: "==", "<", "<=", ">" and ">=". 
    #         Double-bounded ranges are also available, like: "0.5 <= projecting < 2.0". @/li
    #   @li @b transparent @/b: performs the check without shielding (polygon layers only) @/li
    #   @li @b shielded @/b: performs the check with shielding (polygon layers only) @/li
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
    # @h3 Examples @/h3
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
    #
    # @h3 Universal DRC function @/h3
    #
    # There is an alternative notation for the check using the "universal DRC" function ("\Layer#drc"). 
    # This notation is more intuitive and allows checking for widths bigger than a certain value 
    # or within a certain range. See "\global#width" for details.
    #
    # Apart from that it provides the same options than the plain width check. 
    # Follow this link for the documentation of this feature: \global#width.
    #
    # @h3 Shielding @/h3
    #
    # "shielding" is a concept where an internal or external distance is measured only 
    # if the opposite edge is not blocked by other edges between. Shielded mode makes 
    # a difference if very large distances are to be checked and the minimum distance
    # is much smaller: in this case, a large distance violation may be blocked by features
    # located between the edges which are checked. With shielding, large distance violations
    # are not reported in this case. Shielding is also effective at zero distance which has
    # an adverse effect: Consider a case, where one layer A is a subset of another layer B. If 
    # you try to check the distance between features of B vs. A, you cannot use shielding, 
    # because B features which are identical to A features will shield those entirely. 
    #
    # Shielding is enabled by default, but can be switched off with the "transparent" option.
    
    # %DRC%
    # @name space
    # @brief A space check
    # @synopsis layer.space(value [, options])
    #
    # @b Note: @/b "space" is available as an operator for the "universal DRC" function \Layer#drc within
    # the \DRC framework. This variant has more options and is more intuitive to use. See \global#space for more details.
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
    # Like for the \width method, merged semantics applies.
    #
    # Distance values can be given as floating-point values (in micron) or integer values (in
    # database units). To explicitly specify the unit, use the unit denominators.
    #
    # For the manifold options of this function see the \width method description.
    #
    # The following image shows the effect of the space check:
    # 
    # @table
    #   @tr 
    #     @td @img(/images/drc_space1.png) @/td
    #   @/tr
    # @/table
    #
    
    # %DRC%
    # @name isolated
    # @brief An inter-polygon isolation check
    # @synopsis layer.isolated(value [, options])
    # @synopsis layer.iso(value [, options])
    #
    # @b Note: @/b "isolated" and "iso" are available as operators for the "universal DRC" function \Layer#drc within
    # the \DRC framework. These variants have more options and are more intuitive to use. See \global#isolated for more details.
    #
    # See \space for a description of this method. "isolated" is the space check variant which checks different polygons only.
    # In contrast to \space, the "isolated"
    # method is available for polygon layers only, since only on such layers 
    # different polygons can be identified.
    #
    # "iso" is the short form of this method.
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
    # @brief An intra-polygon spacing check
    # @synopsis layer.notch(value [, options])
    #
    # @b Note: @/b "notch" is available as an operator for the "universal DRC" function \Layer#drc within
    # the \DRC framework. This variant has more options and is more intuitive to use. See \global#notch for more details.
    #
    # See \space for a description of this method. 
    # "notch" is the space check variant which finds space violations within a single polygon, but not against other polygons.
    # In contrast to \space, the "notch"
    # method is available for polygon layers only, since only on such layers 
    # different polygons can be identified. Also, opposite and rectangle error
    # filtering is not available for this method.
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
    # @synopsis layer.sep(other_layer, value [, options])
    # 
    # @b Note: @/b "separation" and "sep" are available as operators for the "universal DRC" function \drc within
    # the \DRC framework. These variants have more options and are more intuitive to use. 
    # See \global#separation for more details.
    #
    # This method performs a two-layer spacing check. Like \space, this method
    # can be applied to edge or polygon layers. Locations where edges of the layer
    # are closer than the specified distance to the other layer are reported
    # as edge pair error markers.
    #
    # "sep" is the short form of this method.
    #
    # In contrast to the \space and related methods, locations where both 
    # layers touch are also reported. More specifically, the case of zero spacing
    # will also trigger an error while for \space it will not.
    # 
    # As for the other DRC methods, merged semantics applies.
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
    #
    # @h3 opposite and rectangle error filtering @/h3
    #
    # The options for the separation check are those available for the \width or \space
    # method plus opposite and rectangle error filtering. 
    #
    # Opposite error filtering will waive errors that are on opposite sides of the original
    # figure. The inverse is selection of errors only when there is an error present on
    # the opposite side of the original figure. Opposite error waiving or selection is achieved
    # through these options inside the DRC function call:
    #
    # @ul
    #   @li @b not_opposite @/b will waive opposite errors @/li
    #   @li @b only_opposite @/b will select errors only if there is an opposite one @/li
    # @/ul
    #
    # These modes imply partial waiving or selection if "opposite" only applies to a section
    # of an error.
    #
    # The following images shows the effect of these options:
    # 
    # @table
    #   @tr 
    #     @td @img(/images/drc_separation2.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_separation3.png) @/td
    #     @td @img(/images/drc_separation4.png) @/td
    #   @/tr
    # @/table
    #
    # Rectangle error filtering allows waiving errors based on how they cover the
    # sides of an original rectangular figure. This selection only applies to errors
    # covering the full edge of the rectangle. Errors covering parts of the rectangle
    # edges are not considered in this scheme.
    #
    # The rectangle filter option is enabled by these modes:
    #
    # @ul
    #   @li @b one_side_allowed @/b will waive errors when they appear on one side of the rectangle only @/li
    #   @li @b two_sides_allowed @/b will waive errors when they appear on two sides of the rectangle @/li
    #   @li @b two_connected_sides_allowed @/b will waive errors when they appear on two connected sides of the rectangle ("L" configuration) @/li
    #   @li @b two_opposite_sides_allowed @/b will waive errors when they appear on two opposite sides of the rectangle @/li
    #   @li @b three_sides_allowed @/b will waive errors when they appear on three sides of the rectangle @/li
    #   @li @b four_sides_allowed @/b will waive errors when they appear on four sides of the rectangle @/li
    # @/ul
    #
    # Multiple of these options can be given, which will make errors waived if one of these conditions is met.
    # 
    # The following images shows the effect of some rectangle filter modes:
    # 
    # @table 
    #   @tr 
    #     @td @img(/images/drc_separation5.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_separation6.png) @/td
    #     @td @img(/images/drc_separation7.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_separation8.png) @/td
    #     @td @img(/images/drc_separation9.png) @/td
    #   @/tr
    #   @tr 
    #     @td @img(/images/drc_separation10.png) @/td
    #     @td @img(/images/drc_separation11.png) @/td
    #   @/tr
    # @/table
    
    # %DRC% 
    # @name overlap 
    # @brief An overlap check
    # @synopsis layer.overlap(other_layer, value [, options])
    #
    # @b Note: @/b "overlap" is available as an operator for the "universal DRC" function \drc within
    # the \DRC framework. This variant has more options and is more intuitive to use. 
    # See \global#overlap for more details.
    #
    # This method checks whether layer and other_layer overlap by at least the
    # given length. Locations, where this is not the case will be reported in form
    # of edge pair error markers.
    # Locations, where both layers touch will be reported as errors as well. Formally
    # such locations form an overlap with a value of 0. Locations, where both regions 
    # do not overlap or touch will not be reported. Such regions can be detected 
    # with \outside or by a boolean "not".
    #
    # The options are the same as for \separation.
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
    # @synopsis layer.enc(other_layer, value [, options])
    #
    # @b Note: @/b "enclosing" and "enc" are available as operators for the "universal DRC" function \drc within
    # the \DRC framework. These variants have more options and are more intuitive to use. 
    # See \global#enclosing for more details.
    #
    # This method checks whether layer encloses (is bigger than) other_layer by the
    # given dimension. Locations, where this is not the case will be reported in form
    # of edge pair error markers.
    # Locations, where both edges coincide will be reported as errors as well. Formally
    # such locations form an enclosure with a distance of 0. Locations, where other_layer
    # extends outside layer will not be reported as errors. Such regions can be detected
    # by \not_inside or a boolean "not" operation.
    #
    # "enc" is the short form of this method.
    #
    # The options are the same as for \separation.
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
    
    %w(width space overlap enclosing separation isolated notch).each do |f|
      eval <<"CODE"
      def #{f}(*args)

        @engine._context("#{f}") do

          if :#{f} == :width || :#{f} == :space || :#{f} == :overlap || :#{f} == :enclosing || :#{f} == :separation
            requires_edges_or_region
          else
            requires_region
          end
          
          value = nil
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
            elsif a.is_a?(DRCLayer)
              other = a
            elsif a.is_a?(DRCProjectionLimits)
              (minp, maxp) = a.get_limits(@engine)
            elsif a.is_a?(DRCShielded)
              shielded = a.value
            elsif a.is_a?(Float) || a.is_a?(1.class)
              value && raise("Value already specified")
              value = @engine._make_value(a)
            else
              raise("Parameter #" + n.to_s + " does not have an expected type")
            end
            n += 1
          end

          if !value
            raise("A check value must be specified")
          end

          args = [ value, whole_edges, metrics, alim, minp, maxp ]

          if self.data.is_a?(RBA::Region)
            args << (shielded == nil ? true : shielded)
            if :#{f} != :width && :#{f} != :notch
              args << opposite_filter
              args << rect_filter
            elsif opposite_filter != RBA::Region::NoOppositeFilter
              raise("An opposite error filter cannot be used with this check")
            elsif rect_filter != RBA::Region::NoRectFilter
              raise("A rectangle error filter cannot be used with this check")
            end
          elsif shielded != nil
            raise("Shielding can only be used for polygon layers")
          elsif opposite_filter != RBA::Region::NoOppositeFilter
            raise("An opposite error filter can only be used for polygon layers")
          elsif rect_filter != RBA::Region::NoRectFilter
            raise("A rectangle error filter can only be used for polygon layers")
          end
          
          border = (metrics == RBA::Region::Square ? value * 1.5 : value)
          
          if :#{f} == :width || :#{f} == :space || :#{f} == :notch || :#{f} == :isolated
            if other
              raise("No other layer must be specified for a single-layer check")
            end
            DRCLayer::new(@engine, @engine._tcmd(self.data, border, RBA::EdgePairs, :#{f}_check, *args))
          else
            if !other
              raise("The other layer must be specified for a two-layer check")
            end
            requires_same_type(other)
            DRCLayer::new(@engine, @engine._tcmd(self.data, border, RBA::EdgePairs, :#{f}_check, other.data, *args))
          end
        
        end

      end  
CODE
    end

    # %DRC%
    # @name with_density
    # @brief Returns tiles whose density is within a given range
    # @synopsis layer.with_density(min_value, max_value [, options ])
    # @synopsis layer.with_density(min_value .. max_value [, options ])
    # 
    # This method runs a tiled analysis over the current layout. It reports the tiles whose density
    # is between "min_value" and "max_value". "min_value" and "max_value" are given in
    # relative units, i.e. within the range of 0 to 1.0 corresponding to a density of 0 to 100%.
    #
    # "min_value" or "max_value" can be nil or omitted in the ".." range notation.
    # In this case, they are taken as "0" and "100%".
    #
    # The tile size must be specified with the "tile_size" option:
    #
    # @code
    # # reports areas where layer 1/0 density is below 10% on 20x20 um tiles
    # low_density = input(1, 0).density(0.0 .. 0.1, tile_size(20.um))
    # @/code
    #
    # Anisotropic tiles can be specified by giving two values, like "tile_size(10.um, 20.um)".
    # The first value is the horizontal tile dimension, the second value is the vertical tile
    # dimension.
    #
    # A tile overlap can be specified using "tile_step". If the tile step is less than the
    # tile size, the tiles will overlap. The layout window given by "tile_size" is moved
    # in increments of the tile step:
    #
    # @code
    # # reports areas where layer 1/0 density is below 10% on 30x30 um tiles
    # # with a tile step of 20x20 um:
    # low_density = input(1, 0).density(0.0 .. 0.1, tile_size(30.um), tile_step(20.um))
    # @/code
    #
    # For "tile_step", anisotropic values can be given as well by using two values: the first for the
    # horizontal and the second for the vertical tile step.
    #
    # Another option is "tile_origin" which specifies the location of the first tile's position. 
    # This is the lower left tile's lower left corner. If no origin is given, the tiles are centered over the 
    # area investigated.
    #
    # By default, the tiles will cover the bounding box of the input layer. A separate layer
    # can be used in addition. This way, the layout's dimensions can be derived from some 
    # drawn boundary layer. To specify a separate, additional layer included in the bounding box, use the "tile_boundary" option:
    #
    # @code
    # # reports density of layer 1/0 below 10% on 20x20 um tiles. The layout's boundary is taken from
    # # layer 0/0:
    # cell_frame = input(0, 0)
    # low_density = input(1, 0).density(0.0 .. 0.1, tile_size(20.um), tile_boundary(cell_frame))
    # @/code
    #
    # Note that the layer given in "tile_boundary" adds to the input layer for computing the bounding box.
    # The computed area is at least the area of the input layer.
    #
    # Computation of the area can be skipped by explicitly giving a tile count in horizontal and vertical
    # direction. With the "tile_origin" option this allows full control over the area covered:
    #
    # @code
    # # reports density of layer 1/0 below 10% on 20x20 um tiles in the region 0,0 .. 2000,3000
    # # (100 and 150 tiles of 20 um each are used in horizontal and vertical direction):
    # low_density = input(1, 0).density(0.0 .. 0.1, tile_size(20.um), tile_origin(0.0, 0.0), tile_count(100, 150))
    # @/code
    #
    # The "padding mode" indicates how the area outside the layout's bounding box is considered.
    # There are two modes:
    #
    # @ul
    #   @li @b padding_zero @/b: the outside area is considered zero density. This is the default mode. @/li
    #   @li @b padding_ignore @/b: the outside area is ignored for the density computation. @/li
    # @/ul
    #
    # Example:
    #
    # @code
    # low_density = input(1, 0).density(0.0 .. 0.1, tile_size(20.um), padding_ignore)
    # @/code
    # 
    # The complementary version of "with_density" is \without_density.
    
    # %DRC%
    # @name without_density
    # @brief Returns tiles whose density is not within a given range
    # @synopsis layer.without_density(min_value, max_value [, options ])
    # @synopsis layer.without_density(min_value .. max_value [, options ])
    # 
    # For details about the operations and the operation see \with_density. This version will return the
    # tiles where the density is not within the given range.

    def _with_density(method, inverse, *args)

      requires_region

      limits = [ nil, nil ]
      nlimits = 0
      tile_size = nil
      tile_step = nil
      tile_origin = nil
      tile_count = nil
      tile_boundary = nil
      padding_mode = :zero

      n = 1
      args.each do |a|
        if a.is_a?(DRCTileSize)
          tile_size = a.get
        elsif a.is_a?(DRCTileStep)
          tile_step = a.get
        elsif a.is_a?(DRCTileOrigin)
          tile_origin = a.get
        elsif a.is_a?(DRCTileCount)
          tile_count = a.get
        elsif a.is_a?(DRCTileBoundary)
          tile_boundary = a.get
        elsif a.is_a?(DRCDensityPadding)
          padding_mode = a.value
        elsif a.is_a?(Float) || a.is_a?(1.class) || a == nil
          nlimits < 2 || raise("Too many values specified")
          limits[nlimits] = @engine._make_numeric_value_with_nil(a)
          nlimits += 1
        elsif a.is_a?(Range)
          nlimits == 0 || raise("Either a range or two limits have to be specified, not both")
          limits = [ @engine._make_numeric_value_with_nil(a.begin), @engine._make_numeric_value_with_nil(a.end) ]
          nlimits = 2
        else
          raise("Parameter #" + n.to_s + " does not have an expected type")
        end
        n += 1
      end

      tile_size || raise("At least the tile_size option needs to be present")
      tile_step ||= tile_size

      tp = RBA::TilingProcessor::new
      tp.dbu = @engine.dbu
      tp.scale_to_dbu = false
      tp.tile_size(*tile_step)
      if tile_size != tile_step
        xb = 0.5 * (tile_size[0] - tile_step[0])
        yb = 0.5 * (tile_size[1] - tile_step[1])
        tp.tile_border(xb, yb)
        tp.var("xoverlap", xb / tp.dbu)
        tp.var("yoverlap", yb / tp.dbu)
      else
        tp.var("xoverlap", 0)
        tp.var("yoverlap", 0)
      end
      if tile_origin
        tp.tile_origin(*tile_origin)
      end
      if tile_count
        tp.tiles(*tile_count)
      end

      res = RBA::Region.new      
      tp.output("res", res)
      tp.input("input", self.data)
      tp.threads = (@engine.threads || 1)
      if tile_boundary
        tp.input("boundary", tile_boundary.data)
      else
        tp.input("boundary", RBA::Region::new(self.data.bbox))
      end

      tp.var("vmin", limits[0] || 0.0)
      tp.var("vmax", limits[1] || 1.0)
      tp.var("inverse", inverse)

      if padding_mode == :zero
        tp.queue(<<"TP_SCRIPT")
          _tile && (
            var bx = _tile.bbox.enlarged(xoverlap, yoverlap);
            var d = to_f(input.area(bx)) / to_f(bx.area);
            ((d > vmin - 1e-10 && d < vmax + 1e-10) != inverse) && _output(res, bx, false)
          )
TP_SCRIPT
      elsif padding_mode == :ignore
        tp.queue(<<"TP_SCRIPT")
          _tile && (
            var bx = _tile.bbox.enlarged(xoverlap, yoverlap);
            var ba = boundary.area(bx);
            ba > 0 && (
              var d = to_f(input.area(bx)) / to_f(ba);
              ((d > vmin - 1e-10 && d < vmax + 1e-10) != inverse) && _output(res, bx, false)
            )
          )
TP_SCRIPT
      end

      @engine.run_timed("\"#{method}\" in: #{@engine.src_line}", self.data) do
        tp.execute("Tiled \"#{method}\" in: #{@engine.src_line}")
        res
      end

      DRCLayer::new(@engine, res)

    end

    def with_density(*args)
      self._with_density("with_density", false, *args)
    end

    def without_density(*args)
      self._with_density("without_density", true, *args)
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
      @engine._context("scaled") do
        transformed(RBA::ICplxTrans::new(f.to_f))
      end
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
      @engine._context("scale") do
        transform(RBA::ICplxTrans::new(f.to_f))
      end
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
      @engine._context("rotated") do
        transformed(RBA::ICplxTrans::new(1.0, a, false, 0, 0))
      end
    end
    
    # %DRC%
    # @name rotate
    # @brief Rotates a layer (modifies the layer)
    # @synopsis layer.rotate(a)
    # 
    # Rotates the input by the given angle (in degree). The layer that this method is called 
    # upon is modified and the modified version is returned for further processing.
    
    def rotate(a)
      @engine._context("rotate") do
        transform(RBA::ICplxTrans::new(1.0, a, false, 0, 0))
      end
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
      
        @engine._context("#{f}") do

          requires_region
          
          dist = 0
          
          mode = 2
          values = []
          args.each do |a|
            if a.is_a?(1.class) || a.is_a?(Float)
              v = @engine._make_value(a)
              v.abs > dist && dist = v.abs 
              values.push(v)
            elsif a.is_a?(DRCSizingMode)
              mode = a.value
            end
          end
          
          aa = []
          if values.size < 1
            raise "Method requires one or two sizing values"
          elsif values.size > 2
            raise "Method must not have more than two values"
          else
            aa.push(values[0])
            aa.push(values[-1])
          end
          
          aa.push(mode)
          
          if :#{f} == :size && @engine.is_tiled?
            # in tiled mode, no modifying versions are available
            self.data = @engine._tcmd(self.data, dist, RBA::Region, :sized, *aa)
            self
          elsif :#{f} == :size 
            @engine._tcmd(self.data, dist, RBA::Region, :#{f}, *aa)
            self
          else 
            DRCLayer::new(@engine, @engine._tcmd(self.data, dist, RBA::Region, :#{f}, *aa))
          end
          
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
      @engine._context("polygons") do
        requires_edge_pairs
        args.size <= 1 || raise("Method requires zero or 1 arguments")
        aa = args.collect { |a| @engine._prep_value(a) }
        DRCLayer::new(@engine, @engine._cmd(self.data, :polygons, *aa))
      end
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
        @engine._context("#{f}") do
          aa = args.collect { |a| @engine._prep_value(a) }
          DRCLayer::new(@engine, @engine._cmd(self.data, :#{f}, *aa))
        end
      end
CODE
    end
    
    %w(move transform).each do |f| 
      eval <<"CODE"
      def #{f}(*args)
        @engine._context("#{f}") do
          aa = args.collect { |a| @engine._prep_value(a) }
          @engine._cmd(self.data, :#{f}, *aa)
          self
        end
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
      @engine._context("merged") do
        requires_edges_or_region
        aa = args.collect { |a| @engine._prep_value(a) }
        DRCLayer::new(@engine, @engine._tcmd(self.data, 0, self.data.class, :merged, *aa))
      end
    end
    
    def merge(*args)
      @engine._context("merge") do
        requires_edges_or_region
        aa = args.collect { |a| @engine._prep_value(a) }
        if @engine.is_tiled?
          # in tiled mode, no modifying versions are available
          self.data = @engine._tcmd(self.data, 0, self.data.class, :merged, *aa)
        else
          @engine._tcmd(self.data, 0, self.data.class, :merge, *aa)
        end
        self
      end
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
      @engine._context("output") do
        @engine._vcmd(@engine, :_output, self.data, *args)
      end
    end

    # %DRC%
    # @name fill
    # @brief Fills the region with regular pattern of shapes
    # @synopsis layer.fill([ options ])
    #
    # This method will attempt to fill the polygons of the layer with a regular pattern
    # of shapes.
    #  
    # The fill function currently is not available in deep mode.
    #
    # Options are:
    # @ul
    # @li @b hstep(x) @/b or @b hstep(x, y) @/b: specifies the horizontal step pitch of the pattern. x must be 
    #     a positive value. A vertical displacement component can be specified too, which results in a skewed pattern. @/li
    # @li @b vstep(y) @/b or @b vstep(x, y) @/b: specifies the vertical step pitch of the pattern. y must be 
    #     a positive value. A horizontal displacement component can be specified too, which results in a skewed pattern. @/li
    # @li @b origin(x, y) @/b: specifies a fixed point to align the pattern with. This point specifies the location
    #     of the reference point for one pattern cell. @/li
    # @li @b auto_origin @/b: lets the algorithm choose the origin. This may result is a slightly better fill coverage
    #     as the algorithm is able to determine a pattern origin per island to fill. @/li
    # @li @b multi_origin @/b: lets the algorithm choose the origin and repeats the fill with different origins
    #     until no further fill cell can be fitted. @/li
    # @li @b fill_pattern(..) @/b: specifies the fill pattern. @/li
    # @/ul
    #
    # "fill_pattern" generates a fill pattern object. This object is used for configuring the fill pattern
    # content. Fill pattern need to be named. The name will be used for generating the fill cell.
    #
    # To provide a fill pattern, create a fill pattern object and add shapes to it. The following example creates
    # a fill pattern named "FILL_CELL" and adds a 1x1 micron box on layer 1/0:
    #
    # @code
    # p = fill_pattern("FILL_CELL")
    # p.shape(1, 0, box(0.0, 0.0, 1.0, 1.0))
    # @/code
    # 
    # See \global#box for details about the box specification. You can also add paths or polygons with \global#path or \global#polygon.
    # 
    # A more compact way of writing this is:
    #
    # @code
    # p = fill_pattern("FILL_CELL").shape(1, 0, box(0.0, 0.0, 1.0, 1.0))
    # @/code
    #
    # The fill pattern can be given a reference point which is used for placing the pattern. The reference point
    # is the one which is aligned with the pattern origin. The following code will assign (-0.5, -0.5) as the reference
    # point for the 1x1 micron rectangle. Hence the reference point is a little below and left of the rectangle which 
    # in turn shifts the rectangle fill pattern to the right and up:
    #
    # @code
    # p = fill_pattern("FILL_CELL")
    # p.shape(1, 0, box(0.0, 0.0, 1.0, 1.0))
    # p.origin(-0.5, -0.5)
    # @/code
    #
    # Without a reference point given, the lower left corner of the fill pattern's bounding box will be used
    # as the reference point. The reference point will also defined the footprint of the fill cell - more precisely
    # the lower left corner. When step vectors are given, the fill cell's footprint is taken to be a rectangle
    # having the horizontal and vertical step pitch for width and height respectively. This way the fill cells 
    # will be arrange seamlessly. However, the cell's dimensions can be changed, so that the fill cells
    # can overlap or there is a space between the cells. To change the dimensions use the "dim" method.
    #
    # The following example specifies a fill cell with an active area of -0.5 .. 1.5 in both directions
    # (2 micron width and height). With these dimensions the fill cell's footprint is independent of the
    # step pitch:
    #
    # @code
    # p = fill_pattern("FILL_CELL")
    # p.shape(1, 0, box(0.0, 0.0, 1.0, 1.0))
    # p.origin(-0.5, -0.5)
    # p.dim(2.0, 2.0)
    # @/code
    #
    # With these ingredients will can use the fill function. The first example fills the polygons
    # of "to_fill" with an orthogonal pattern of 1x1 micron rectangles with a pitch of 2 microns:
    #
    # @code
    # pattern = fill_pattern("FILL_CELL").shape(1, 0, box(0.0, 0.0, 1.0, 1.0)).origin(-0.5, -0.5)
    # to_fill.fill(pattern, hstep(2.0), vstep(2.0))
    # @/code
    #
    # This second example will create a skewed fill pattern in auto-origin mode:
    #  
    # @code
    # pattern = fill_pattern("FILL_CELL").shape(1, 0, box(0.0, 0.0, 1.0, 1.0)).origin(-0.5, -0.5)
    # to_fill.fill(pattern, hstep(2.0, 1.0), vstep(-1.0, 2.0), auto_origin)
    # @/code
    #
    # The fill function can only work with a target layout for output. 
    # It will not work for report output.
    #
    # The layers generated by the fill cells is only available for input later in the 
    # script if the output layout is identical to the input layouts.
    # If you need the area missed by the fill function, try \fill_with_left.
    
    def fill(*args)
      self._fill(false, *args)
    end

    # %DRC%
    # @name fill_with_left
    # @brief Fills the region with regular pattern of shapes
    # @synopsis layer.fill_with_left([ options ])
    #
    # This method has the same call syntax and functionality than \fill. Other than this method
    # it will return the area not covered by fill cells as a DRC layer.

    def fill_with_left(*args)
      self._fill(true, *args)
    end

    def _fill(with_left, *args)

      m = with_left ? "fill_with_left" : "fill"

      # generation of new cells not tested in deep mode
      @deep && raise("#{m} command not supported in deep mode currently")

      (@engine._output_layout && @engine._output_cell) || raise("#{m} command needs an output layout and output cell")

      source = @engine.source
      row_step = nil
      column_step = nil
      pattern = nil
      origin = RBA::DPoint::new
      repeat = false

      args.each_with_index do |a,ai|
        if a.is_a?(DRCSource)
          if source
            raise("Duplicate source specification for '#{m}' at argument ##{ai+1}")
          end
          source = a
        elsif a.is_a?(DRCFillCell)
          if pattern
            raise("Duplicate fill pattern specification for '#{m}' at argument ##{ai+1}")
          end
          pattern = a
        elsif a.is_a?(DRCFillStep)
          if a.for_row
            if row_step
              raise("Duplicate hstep specification for '#{m}' at argument ##{ai+1}")
            end
            row_step = a.step
          else
            if column_step
              raise("Duplicate vstep specification for '#{m}' at argument ##{ai+1}")
            end
            column_step = a.step
          end
        elsif a.is_a?(DRCFillOrigin)
          origin = a.origin
          repeat = a.repeat
        else
          raise("Argument ##{ai+1} not understood for '#{m}'")
        end
      end

      if !pattern
        raise("No fill pattern given for '#{m}' (use 'fill_pattern')")
      end

      if !row_step
        row_step = RBA::DVector::new(pattern.default_xpitch, 0)
      end
      if !column_step
        column_step = RBA::DVector::new(0, pattern.default_ypitch)
      end

      dbu_trans = RBA::VCplxTrans::new(1.0 / @engine.dbu)

      result = nil

      fill_cell = pattern.create_cell(@engine._output_layout, @engine)
      top_cell = @engine._output_cell
      fc_box = dbu_trans * pattern.cell_box(row_step.x, column_step.y)
      rs = dbu_trans * row_step
      cs = dbu_trans * column_step
      origin = origin ? dbu_trans * origin : nil
      fc_index = fill_cell.cell_index

      if @engine._tx && @engine._ty

        tp = RBA::TilingProcessor::new
        tp.dbu = @engine.dbu
        tp.frame = RBA::CplxTrans::new(@engine.dbu) * self.data.bbox
        tp.scale_to_dbu = false
        tp.tile_size(@engine._tx, @engine._ty)
        bx = [ @engine._bx || 0.0, row_step.x ].max
        by = [ @engine._by || 0.0, column_step.y ].max
        tp.tile_border(bx, by)
        tp.threads = (@engine.threads || 1)

        result_arg = "nil"
        if with_left
          result = RBA::Region::new
          result_arg = "result"
          tp.output(result_arg, result)
        end

        tp.input("region", self.data)
        tp.var("top_cell", top_cell)
        tp.var("fc_box", fc_box)
        tp.var("rs", rs)
        tp.var("cs", cs)
        tp.var("origin", origin)
        tp.var("fc_index", fc_index)
        tp.var("repeat", repeat)
        tp.var("with_left", with_left)

        tp.queue(<<"END")
          var tc_box = _frame.bbox;
          var tile_box = _tile ? (tc_box & _tile.bbox) : tc_box;
          !tile_box.empty && (
            tile_box = tile_box.enlarged(Vector.new(max(rs.x, fc_box.width), max(cs.y, fc_box.height)));
            tile_box = tile_box & tc_box;
            var left = with_left ? Region.new : nil;
            repeat ? 
              (region & tile_box).fill_multi(top_cell, fc_index, fc_box, rs, cs, Vector.new, left, _tile.bbox) :
              (region & tile_box).fill(top_cell, fc_index, fc_box, rs, cs, origin, left, Vector.new, left, _tile.bbox);
            with_left && _output(#{result_arg}, left)
          )
END
          
        begin
          @engine._output_layout.start_changes
          @engine.run_timed("\"#{m}\" in: #{@engine.src_line}", self.data) do
            tp.execute("Tiled \"#{m}\" in: #{@engine.src_line}")
          end
        ensure
          @engine._output_layout.end_changes
        end

      else

        if with_left
          result = RBA::Region::new
        end

        @engine.run_timed("\"#{m}\" in: #{@engine.src_line}", self.data) do
          if repeat
            self.data.fill_multi(top_cell, fc_index, fc_box, rs, cs, RBA::Vector::new, result)
          else
            self.data.fill(top_cell, fc_index, fc_box, rs, cs, origin, result, RBA::Vector::new, result)
          end
        end

      end

      if fill_cell.parent_cells == 0
        # fill cell not required (not placed) -> remove
        fill_cell.delete
      end
          
      self.data.disable_progress

      return result ? DRCLayer::new(@engine, result) : nil

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
      @engine._context("data") do
        @data || raise("Trying to access an invalid layer (did you use 'forget' on it?)")
        @data
      end
    end

    def data=(d)
      @data = d
    end

    def check_is_layer(other)
      other.is_a?(DRCLayer) || raise("Argument needs to be a DRC layer")
    end

    def requires_region
      self.data.is_a?(RBA::Region) || raise("Requires a polygon layer")
    end
    
    def requires_texts_or_region
      self.data.is_a?(RBA::Region) || self.data.is_a?(RBA::Texts) || raise("Requires a polygon or text layer")
    end
    
    def requires_texts
      self.data.is_a?(RBA::Texts) || raise("Requires a text layer")
    end
    
    def requires_edge_pairs
      self.data.is_a?(RBA::EdgePairs) || raise("Requires an edge pair layer")
    end
    
    def requires_edges
      self.data.is_a?(RBA::Edges) || raise("Requires an edge layer")
    end
    
    def requires_edges_or_edge_pairs
      self.data.is_a?(RBA::Edges) || self.data.is_a?(RBA::EdgePairs) || raise("Requires an edge or edge pair layer")
    end
    
    def requires_edges_or_region
      self.data.is_a?(RBA::Edges) || self.data.is_a?(RBA::Region) || raise("Requires an edge or polygon layer")
    end
    
    def requires_edges_texts_or_region
      self.data.is_a?(RBA::Edges) || self.data.is_a?(RBA::Region) || self.data.is_a?(RBA::Texts) || raise("Requires an edge, text or polygon layer")
    end
    
    def requires_same_type(other)
      self.data.class == other.data.class || raise("Requires input of the same kind")
    end

    def minmax_count(*args)
      if args.size == 0
        return []
      elsif args.size == 1
        a = args[0]
        if a.is_a?(Range)
          if a.begin && a.begin.to_i <= 0
            raise("Lower bound of range must be a positive, non-zero number")
          end
          if a.end
            return [(a.begin || 1).to_i, a.end.to_i]
          else
            return [(a.begin || 1).to_i]
          end
        elsif !a.is_a?(1.class)
          raise("Count argument must be an integer number")
        elsif a <= 0
          raise("Count argument must be a positive, non-zero number")
        else
          return [a]
        end
      elsif args.size == 2
        amin = args[0]
        amax = args[1]
        if !amin.is_a?(1.class)
          raise("Min_count argument must be an integer number")
        elsif !amax.is_a?(1.class)
          raise("Max_count argument must be an integer number")
        elsif amin <= 0
          raise("Min_count argument must be a positive, non-zero number")
        elsif amax < amin
          raise("Max_count argument must be larger or equal to min_count")
        else
          return [amin, amax]
        end
      else
        raise("Too many arguments")
      end
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

