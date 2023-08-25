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
      @global_trans = RBA::DCplxTrans::new
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

    def inplace_global_transform(*args)
      gt = RBA::DCplxTrans::new
      args.each do |a|
        if a.is_a?(RBA::DVector) || a.is_a?(RBA::DTrans)
          gt = RBA::DCplxTrans::new(a) * gt
        elsif a.is_a?(RBA::DCplxTrans)
          gt = a * gt
        else
          raise("Expected a transformation spec instead of #{a.inspect}")
        end
      end
      @global_trans = gt
    end

    def global_transformation
      @global_trans
    end
    
    def finish
      @tmp_layers.each do |layout,li|
        layout.delete_layer(li)
      end
      @tmp_layers = []
    end

    def set_box(method, *args)

      @engine._context(method) do

        box = nil
        if args.size == 0
          # unclip
        elsif args.size == 1
          box = args[0]
          box.is_a?(RBA::DBox) || raise("Method requires a box specification")
        elsif args.size == 2
          (args[0].is_a?(RBA::DPoint) && args[1].is_a?(RBA::DPoint)) || raise("Method requires a box specification with two points")
          box = RBA::DBox::new(args[0], args[1])
        elsif args.size == 4
          box = RBA::DBox::new(*args)
        else 
          raise("Invalid number of arguments (1, 2 or 4 expected)")
        end
        @box = box && RBA::Box::from_dbox(box * (1.0 / @layout.dbu))

        self

      end

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
      @engine._context("inplace_cell") do
        @cell = layout.cell(arg)
        @cell ||= layout.create_cell(arg)
        self  
      end
    end
    
    def inplace_select(*args)
      @engine._context("inplace_select") do
        args.each do |a|
          a.is_a?(String) || raise("Invalid arguments - must be strings")
          @sel.push(a)
        end
        self
      end
    end
    
    # %DRC%
    # @name select
    # @brief Adds cell name expressions to the cell filters
    # @synopsis new_source = source.select(filter1, filter2, ...)
    # This method will construct a new source object with the given cell filters 
    # applied. Note that there is a global version of "select" which does not 
    # create a new source, but acts on the default source.
    #
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
    # source_with_selection = source.select("-TOP", "+B")
    # l1 = source_with_selection.input(1, 0)
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
    # source_with_selection = source.select("-*", "+B")
    # l1 = source_with_selection.input(1, 0)
    # ...
    # @/code
    # 
    # The short form "-" will disable the top cell. This code is identical to the first example
    # and will start with a disabled top cell regardless of its name:
    # 
    # @code
    # source_with_selection = source.select("-", "+B")
    # l1 = source_with_selection.input(1, 0)
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
    
    # %DRC%
    # @name global_transform
    # @brief Gets or sets a global transformation
    # @synopsis global_transform
    # @synopsis global_transform([ transformations ])
    #
    # This method returns a new source representing the transformed layout. It is provided in the spritit of 
    # \Source#clip and similar methods.
    #
    # The transformation
    # is either given as a RBA::DTrans, RBA::DVector or RBA::DCplxTrans object or as one of the 
    # following specifications:
    #
    # @ul
    # @li "shift(x, y)": shifts the input layout horizontally by x and vertically by y micrometers @/li
    # @li "rotate(a)": rotates the input layout by a degree counter-clockwise @/li
    # @li "magnify(m)": magnifies the input layout by the factor m (NOTE: using fractional scale factors may result in small gaps due to grid snapping) @/li
    # @li "mirror_x": mirrors the input layout at the x axis @/li
    # @li "mirror_y": mirrors the input layout at the y axis @/li
    # @/ul
    #
    # Multiple transformation specs can be given. In that case the transformations are applied right to left.
    # Using "global_transform" will reset any global transformation present already.
    # Without an argument, the global transformation is reset.
    #
    # The following example rotates the layout by 90 degree at the origin (0, 0) and then shifts it up by 
    # 100 micrometers:
    #
    # @code
    # source.global_transform(shift(0, 100.um), rotate(90.0))
    # @/code

    # export inplace_* as * out-of-place
    %w(select cell clip touching overlapping global_transform).each do |f|
      eval <<"CODE"
        def #{f}(*args)
          @engine._context("#{f}") do
            s = self.dup
            s.inplace_#{f}(*args)
            s
          end
        end
CODE
    end

    # %DRC%
    # @name extent
    # @brief Returns a layer with the bounding box of the selected layout or cells
    # @synopsis source.extent
    # @synopsis source.extent(cell_filter)
    #
    # Without an argument, the extent method returns a layer with the bounding box
    # of the top cell. With a cell filter argument, the method returns a layer
    # with the bounding boxes of the selected cells. The cell filter is a glob
    # pattern.
    # 
    # The extent function is useful to invert a layer:
    # 
    # @code
    # inverse_1 = extent.sized(100.0) - input(1, 0)
    # @/code
    #
    # The following example returns the bounding boxes of all cells whose
    # names start with "A":
    #
    # @code
    # a_cells = extent("A*")
    # @/code
    
    def extent(cell_filter = nil)

      @engine._context("extent") do

        if cell_filter
          cell_filter.is_a?(String) || raise("Invalid cell filter argument - must be a string")
        end

        if cell_filter
          tmp = @layout_var.insert_layer(RBA::LayerInfo::new)
          @tmp_layers << [ @layout_var, tmp ]
          @layout_var.cells(cell_filter).each do |cell|
            cell.shapes(tmp).insert(cell.bbox)
          end
          layer = DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, [tmp], @sel, @box, @clip, @overlapping, RBA::Shapes::SAll, @global_trans, [], RBA::Region))
        else
          layer = input
          layer.insert((RBA::DBox::from_ibox(@cell.bbox) * @layout.dbu).transformed(@global_trans))
          if @box
            layer.data &= RBA::Region::new(@box)
          end
        end

        layer

      end

    end
          
    # %DRC%
    # @name input
    # @brief Specifies input from a source
    # @synopsis source.input
    # @synopsis source.input(layer)
    # @synopsis source.input(layer, datatype)
    # @synopsis source.input(layer_into)
    # @synopsis source.input(filter, ...)
    # @synopsis source.input(props_spec, ...)
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
    # Layers created with "input" may contain both texts (labels) and polygons. There is a subtle
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
    # \labels also produces a true "text layer" which contains text objects. A variety of 
    # operations is available for these objects, such as boolean "and" and "not" with a polygon layer.
    # True text layers should be preferred over mixed polygon/text layers if text object processing
    # is required.
    #
    # "input" without any arguments will create a new, empty original layer.
    #
    # If you want to use user properties - for example with properties constraints in DRC checks -
    # you need to enable properties on input:
    #
    # @code
    # input1_with_props = input(1, 0, enable_props)
    # @/code
    #
    # You can also filter or map property keys, similar to the functions available on
    # layers (\DRCLayer#map_props, \DRCLayer#select_props). For example to select
    # property values with key 17 (numerical) only, use:
    #
    # @code
    # input1_with_props = input(1, 0, select_props(17))
    # @/code
    #
    # Use the global version of "input" without a source object to address the default source.
    
    def input(*args)
      @engine._context("input") do
        layers, prop_selectors = parse_input_layers(*args)
        DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::SAll, @global_trans, prop_selectors, RBA::Region))
      end
    end

    # %DRC%
    # @name labels
    # @brief Gets the labels (texts) from an input layer
    # @synopsis source.labels
    # @synopsis source.labels(layer)
    # @synopsis source.labels(layer, datatype)
    # @synopsis source.labels(layer_into)
    # @synopsis source.labels(filter, ...)
    #
    # Creates a true text layer with the labels from the given layer of the source.
    # 
    # This method is identical to \input, but takes only texts from the given input
    # layer. Starting with version 0.27, the result is no longer a polygon layer that tries
    # to provide text support but a layer type which is provided for carrying text objects
    # explicitly.
    #
    # "labels" without any arguments will create a new, empty original layer.
    #
    # Use the global version of "labels" without a source object to address the default source.
    
    def labels(*args)
      @engine._context("labels") do
        layers, prop_selectors = parse_input_layers(*args)
        DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::STexts, @global_trans, prop_selectors, RBA::Texts))
      end
    end

    # %DRC%
    # @name polygons
    # @brief Gets the polygon shapes (or shapes that can be converted polygons) from an input layer
    # @synopsis source.polygons
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
    # "polygons" without any arguments will create a new, empty original layer.
    #
    # Use the global version of "polygons" without a source object to address the default source.
    
    def polygons(*args)
      @engine._context("polygons") do
        layers, prop_selectors = parse_input_layers(*args)
        DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::SBoxes | RBA::Shapes::SPaths | RBA::Shapes::SPolygons | RBA::Shapes::SEdgePairs, @global_trans, prop_selectors, RBA::Region))
      end
    end

    # %DRC%
    # @name edges
    # @brief Gets the edge shapes (or shapes that can be converted edges) from an input layer
    # @synopsis source.edges
    # @synopsis source.edges(layer)
    # @synopsis source.edges(layer, datatype)
    # @synopsis source.edges(layer_into)
    # @synopsis source.edges(filter, ...)
    #
    # Creates a layer with the edges from the given layer of the source.
    # Edge layers are formed from shapes by decomposing the shapes into edges: polygons
    # for example are decomposed into their outline edges. Some file formats support egdes
    # as native objects. 
    # 
    # This method is identical to \input with respect to the options supported.
    #
    # Use the global version of "edges" without a source object to address the default source.
    # 
    # "edges" without any arguments will create a new, empty original layer.
    #
    # This method has been introduced in version 0.27.
    
    def edges(*args)
      @engine._context("edges") do
        layers, prop_selectors = parse_input_layers(*args)
        DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::SBoxes | RBA::Shapes::SPaths | RBA::Shapes::SPolygons | RBA::Shapes::SEdgePairs | RBA::Shapes::SEdges, @global_trans, prop_selectors, RBA::Edges))
      end
    end

    # %DRC%
    # @name edge_pairs
    # @brief Gets the edge pairs from an input layer
    # @synopsis source.edge_pairs
    # @synopsis source.edge_pairs(layer)
    # @synopsis source.edge_pairs(layer, datatype)
    # @synopsis source.edge_pairs(layer_into)
    # @synopsis source.edge_pairs(filter, ...)
    #
    # Creates a layer with the edge_pairs from the given layer of the source.
    # Edge pairs are not supported by layout formats so far. So except if the source is
    # a custom-built layout object, this method has little use. It is provided for future 
    # extensions which may include edge pairs in file streams.
    # 
    # This method is identical to \input with respect to the options supported.
    #
    # Use the global version of "edge_pairs" without a source object to address the default source.
    # 
    # "edge_pairs" without any arguments will create a new, empty original layer.
    #
    # This method has been introduced in version 0.27.
    
    def edge_pairs(*args)
      @engine._context("edge_pairs") do
        layers, prop_selectors = parse_input_layers(*args)
        DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::SEdgePairs, @global_trans, prop_selectors, RBA::EdgePairs))
      end
    end

    # %DRC%
    # @name make_layer
    # @brief Creates an empty polygon layer based on the hierarchy of the layout
    # @synopsis make_layer
    # This method delivers a new empty original layer. It is provided to keep old code working.
    # Use "input" without arguments instead.

    def make_layer
      layers = []
      prop_selectors = []
      DRCLayer::new(@engine, @engine._cmd(@engine, :_input, @layout_var, @cell.cell_index, layers, @sel, @box, @clip, @overlapping, RBA::Shapes::SAll, @global_trans, prop_selectors, RBA::Region))
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
      prop_selectors = args.select { |a| a.is_a?(DRCPropertySelector) }

      args = args.select { |a| !a.is_a?(DRCPropertySelector) }

      if args.size == 0
      
        li = @layout.insert_layer(RBA::LayerInfo::new)
        li && layers.push(li)
        li && @tmp_layers.push([ @layout, li ])
      
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

      [ layers, prop_selectors ]

    end

  end

end

