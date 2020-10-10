# $autorun-early

module DRC

  # A layout source representative object.
  # This object describes an input. It consists of a layout reference plus 
  # some attributes describing how input is to be gathered. 
  
  # %DRC%
  # @scope
  # @name Source
  # @brief DRC Reference: Source Object
  # The layer object represents a collection of polygons, edges or edge pairs.
  # A source specifies where to take layout from. That includes the actual layout,
  # the top cell and options such as clip/query boxes, cell filters etc.
  
  class DRCSource
  
    def initialize(engine, layout, layout_var, cell, path)
      @engine = engine
      @layout = layout
      @layout_var = layout_var
      @path = path
      @cell = cell
      @box = nil
      @sel = []
      @clip = false
      @overlapping = false
      @tmp_layers = []
    end

    # Conceptual deep copy (not including the temp layers)
    def dup
      d = DRCSource::new(@engine, @layout, @layout_var, @cell, @path)
      d._init_internal(@box ? @box.dup : nil, @sel.dup, @clip, @overlapping)
      d
    end

    # internal copy initialization
    def _init_internal(box, sel, clip, overlapping)
      @box = box
      @sel = sel
      @clip = clip
      @overlapping = overlapping
    end
        
    # %DRC%
    # @name layout
    # @brief Returns the RBA::Layout object associated with this source
    # @synopsis layout
    
    def layout
      @layout
    end
    
    # %DRC%
    # @name cell_name
    # @brief Returns the name of the currently selected cell
    # @synopsis cell_name
    
    def cell_name
      @cell.name
    end
    
    # %DRC%
    # @name cell_obj
    # @brief Returns the RBA::Cell object of the currently selected cell
    # @synopsis cell_obj
    
    def cell_obj
      @cell
    end
    
    def finish
      @tmp_layers.each do |li|
        @layout.delete_layer(li)
      end
    end

    def set_box(method, *args)
      box = nil
      if args.size == 1
        box = args[0]
        box.is_a?(RBA::DBox) || raise("'#{method}' method requires a box specification")
      elsif args.size == 2
        (args[0].is_a?(RBA::DPoint) && args[1].is_a?(RBA::DPoint)) || raise("'#{method}' method requires a box specification with two points")
        box = RBA::DBox::new(args[0], args[1])
      elsif args.size == 4
        box = RBA::DBox::new(*args)
      else 
        raise("Invalid number of arguments for '#{method}' method")
      end
      @box = RBA::Box::from_dbox(box * (1.0 / @layout.dbu))
      self
    end
    
    def inplace_clip(*args)
      set_box("clip", *args)
      @clip = true
      @overlapping = true
    end
            
    def inplace_touching(*args)
      set_box("touching", *args)
      @clip = false
      @overlapping = false
    end
    
    def inplace_overlapping(*args)
      set_box("overlapping", *args)
      @clip = false
      @overlapping = true
    end
    
    def inplace_cell(arg)
      @cell = layout.cell(arg)
      @cell ||= layout.create_cell(arg)
      self  
    end
    
    def inplace_select(*args)
      args.each do |a|
        a.is_a?(String) || raise("Invalid arguments to 'select' method - must be strings")
        @sel.push(a)
      end
      self
    end
    
    # %DRC%
    # @name select
    # @brief Adds cell name expressions to the cell filters
    # @synopsis source.select(filter1, filter2, ...)
    # This method will construct a new source object with the given cell filters 
    # applied.
    # Cell filters will enable or disable cells plus their subtree.
    # Cells can be switched on and off, which makes the hierarchy traversal
    # stop or begin delivering shapes at the given cell. The arguments of 
    # the select method form a sequence of enabling or disabling instructions
    # using cell name pattern in the glob notation ("*" as the wildcard, like shell).
    # Disabling instructions start with a "-", enabling instructions with a "+" or
    # no specification.
    #
    # The following options are available:
    # 
    # @ul
    # @li @tt+@/tt @i name_filter @/i: Cells matching the name filter will be enabled @/li
    # @li @i name_filter @/i: Same as "+name_filter" @/li
    # @li @tt-@/tt @i name_filter @/i: Cells matching the name filter will be disabled @/li
    # @/ul
    #
    # To disable the TOP cell but enabled a hypothetical cell B below the top cell, use that
    # code:
    #
    # @code
    # layout_with_selection = source.select("-TOP", "+B")
    # l1 = source.input(1, 0)
    # ...
    # @/code
    #
    # Please note that the sample above will deliver the children of "B" because there is 
    # nothing said about how to proceed with cells other than "TOP" or "B". Conceptually,
    # the instantiation path of a cell will be matched against the different filters in the
    # order they are given.
    # A matching negative expression will disable the cell, a matching positive expression
    # will enable the cell. Hence, every cell that has a "B" in the instantiation path 
    # is enabled.
    #
    # The following code will just select "B" without its children, because in the 
    # first "-*" selection, all cells including the children of "B" are disabled:
    #
    # @code
    # layout_with_selection = source.select("-*", "+B")
    # l1 = source.input(1, 0)
    # ...
    # @/code
    # 
    # The short form "-" will disable the top cell. This code is identical to the first example
    # and will start with a disabled top cell regardless of its name:
    # 
    # @code
    # layout_with_selection = source.select("-", "+B")
    # l1 = source.input(1, 0)
    # ...
    # @/code
    
    # %DRC%
    # @name cell
    # @brief Specifies input from a specific cell
    # @synopsis source.cell(name)
    # This method will create a new source that delivers shapes from the 
    # specified cell. 
    
    # %DRC%
    # @name clip
    # @brief Specifies clipped input
    # @synopsis source.clip(box)
    # @synopsis source.clip(p1, p2)
    # @synopsis source.clip(l, b, r, t)
    # Creates a source which represents a rectangular part of the 
    # original input. Three ways are provided to specify the rectangular
    # region: a single RBA::DBox object (micron units), two RBA::DPoint
    # objects (lower/left and upper/right coordinate in micron units)
    # or four coordinates: left, bottom, right and top coordinate.
    # 
    # This method will create a new source which delivers the shapes
    # from that region clipped to the rectangle. A method doing the 
    # same but without clipping is \touching or \overlapping.
    
    # %DRC%
    # @name touching
    # @brief Specifies input selected from a region in touching mode
    # @synopsis source.touching(box)
    # @synopsis source.touching(p1, p2)
    # @synopsis source.touching(l, b, r, t)
    # Like \clip, this method will create a new source delivering shapes
    # from a specified rectangular region. In contrast to clip, all shapes
    # touching the region with their bounding boxes are delivered as a whole
    # and are not clipped. Hence shapes may extent beyond the limits of
    # the specified rectangle.
    # 
    # \overlapping is a similar method which delivers shapes overlapping
    # the search region with their bounding box (and not just touching)
    
    # %DRC%
    # @name overlapping
    # @brief Specifies input selected from a region in overlapping mode
    # @synopsis source.overlapping(...)
    # Like \clip, this method will create a new source delivering shapes
    # from a specified rectangular region. In contrast to clip, all shapes
    # overlapping the region with their bounding boxes are delivered as a whole
    # and are not clipped. Hence shapes may extent beyond the limits of
    # the specified rectangle.
    # 
    # \touching is a similar method which delivers shapes touching
    # the search region with their bounding box (without the requirement to overlap)
    
    # export inplace_* as * out-of-place
    %w(select cell clip touching overlapping).each do |f|
      eval <<"CODE"
        def #{f}(*args)
          s = self.dup
          s.inplace_#{f}(*args)
          s
        end
CODE
    end

    # %DRC%
    # @name extent
    # @brief Returns a layer with the bounding box of the selected layout
    # @synopsis source.extent
    # The extent function is useful to invert a layer:
    # 
    # @code
    # inverse_1 = extent.sized(100.0) - input(1, 0)
    # @/code
    
    def extent
      layer = input
      if @box
        layer.insert(RBA::DBox::from_ibox(@box) * @layout.dbu)
      else
        layer.insert(RBA::DBox::from_ibox(@cell.bbox) * @layout.dbu)
      end
      layer
    end
          
    # %DRC%
    # @name input
    # @brief Specifies input from a source
    # @synopsis source.input(layer)
    # @synopsis source.input(layer, datatype)
    # @synopsis source.input(layer_into)
    # @synopsis source.input(filter, ...)
    # Creates a layer with the shapes from the given layer of the source.
    # The layer can be specified by layer and optionally datatype, by a RBA::LayerInfo
    # object or by a sequence of filters. 
    # Filters are expressions describing ranges
    # of layers and/or datatype numbers or layer names. Multiple filters
    # can be given and all layers matching at least one of these filter
    # expressions are joined to render the input layer for the DRC engine.
    #
    # Some filter expressions are:
    #
    # @ul
    # @li @tt 1/0-255 @/tt: Datatypes 0 to 255 for layer 1 @/li
    # @li @tt 1-10 @/tt: Layers 1 to 10, datatype 0 @/li
    # @li @tt METAL @/tt: A layer named "METAL" @/li
    # @li @tt METAL (17/0) @/tt: A layer named "METAL" or layer 17, datatype 0 (for GDS, which does
    #           not have names)@/li
    # @/ul
    #
    # Layers created with "input" contain both texts and polygons. There is a subtle
    # difference between flat and deep mode: in flat mode, texts are not visible in polygon
    # operations. In deep mode, texts appear as small 2x2 DBU rectangles. In flat mode, 
    # some operations such as clipping are not fully supported for texts. Also, texts will
    # vanish in most polygon operations such as booleans etc.
    #
    # Texts can later be selected on the layer returned by "input" with the \Layer#texts method.
    #
    # If you don't want to see texts, use \polygons to create an input layer with polygon data
    # only. If you only want to see texts, use \labels to create an input layer with texts only.
    #
    # Use the global version of "input" without a source object to address the default source.
    
    def input(*args)
      layers = parse_input_layers(*args)
      DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::SAll))
    end

    # %DRC%
    # @name labels
    # @brief Gets the labels (texts) from an input layer
    # @synopsis source.labels(layer)
    # @synopsis source.labels(layer, datatype)
    # @synopsis source.labels(layer_into)
    # @synopsis source.labels(filter, ...)
    #
    # Creates a layer with the labels from the given layer of the source.
    # 
    # This method is identical to \input, but takes only texts from the given input
    # layer.
    #
    # Use the global version of "labels" without a source object to address the default source.
    
    def labels(*args)
      layers = parse_input_layers(*args)
      DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::STexts))
    end

    # %DRC%
    # @name polygons
    # @brief Gets the polygon shapes (or shapes that can be converted polygons) from an input layer
    # @synopsis source.polygons(layer)
    # @synopsis source.polygons(layer, datatype)
    # @synopsis source.polygons(layer_into)
    # @synopsis source.polygons(filter, ...)
    #
    # Creates a layer with the polygon shapes from the given layer of the source.
    # With "polygon shapes" we mean all kind of shapes that can be converted to polygons.
    # Those are boxes, paths and real polygons.
    # 
    # This method is identical to \input with respect to the options supported.
    #
    # Use the global version of "polygons" without a source object to address the default source.
    
    def polygons(*args)
      layers = parse_input_layers(*args)
      DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::SBoxes | RBA::Shapes::SPaths | RBA::Shapes::SPolygons | RBA::Shapes::SEdgePairs))
    end

    # %DRC%
    # @name make_layer
    # @brief Creates an empty polygon layer based on the hierarchy of the layout
    # @synopsis make_layer
    # This method delivers a new empty original layer.

    def make_layer
      layers = []
      DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::SAll))
    end

    # %DRC%
    # @name layers
    # @brief Gets the layers the source contains
    # @synopsis source.layers
    # Delivers a list of RBA::LayerInfo objects representing the layers
    # inside the source.
    #
    # One application is to read all layers from a source. In the following
    # example, the "and" operation is used to perform a clip with the given
    # rectangle. Note that this solution is not efficient - it's provided
    # as an example only:
    # 
    # @code
    # output_cell("Clipped")
    # 
    # clip_box = polygon_layer
    # clip_box.insert(box(0.um, -4.um, 4.um, 0.um))
    # 
    # layers.each { |l| (input(l) & clip_box).output(l) }
    # @/code
    
    def layers
      @layout.layer_indices.collect { |li| @layout.get_info(li) }
    end

    # %DRC%
    # @name path
    # @brief Gets the path of the corresponding layout file or nil if there is no path
    # @synopsis path

    def path
      @path
    end
     
  private

    def parse_input_layers(*args)

      layers = []
     
      if args.size == 0
      
        li = @layout.insert_layer(RBA::LayerInfo::new)
        li && layers.push(li)
        li && @tmp_layers.push(li)
      
      elsif (args.size == 1 && args[0].is_a?(RBA::LayerInfo))

        li = @layout.find_layer(args[0])
        li && layers.push(li)

      elsif (args.size == 1 || args.size == 2) && args[0].is_a?(1.class)

        li = @layout.find_layer(args[0], args[1] || 0)
        li && layers.push(li)
        
      else
      
        args.each do |a|
          if a.is_a?(String)
            # use the LayerMap class to fetch the matching layers
            lm = RBA::LayerMap::new
            lm.map(a, 0)
            @layout.layer_indices.each do |li|
              if lm.is_mapped?(@layout.get_info(li))
                layers.push(li)
              end
            end
          end
        end
        
      end

      layers

    end

  end

end

