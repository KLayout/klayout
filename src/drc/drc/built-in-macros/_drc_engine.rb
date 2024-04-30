# $autorun-early

require 'pathname'

module DRC

  class CustomDeviceClassFactory < RBA::DeviceClassFactory
    def initialize(cls)
      @cls = cls
    end
    def create_class
      @cls.new
    end
  end

  class OutputChannel
    def initialize(engine)
      @engine = engine
    end
    def is_rdb?
      false
    end
    def cellname=(cellname)
      # reimplement
    end
    def cell
      # reimplement
    end
    def write
      # reimplement
    end
    def finish(final)
      # reimplement
    end
    def write
      # reimplement
    end
    def layout
      nil
    end
    def rdb
      nil
    end
  end

  class LayoutOutputChannel < OutputChannel

    def initialize(engine, layout, cell, file_name)
      super(engine)
      @layout = layout
      @cell = cell
      @file_name = file_name
    end

    def cellname=(cellname)
      @cell = @layout.cell(cellname) || @layout.create_cell(cellname)
    end

    def is_rdb?
      false
    end

    def finish(final, view)
      write
    end

    def write
      if @file_name
        opt = RBA::SaveLayoutOptions::new
        gzip = opt.set_format_from_filename(@file_name)
        @engine.info("Writing layout file: #{@file_name} ..")
        @layout.write(@file_name, gzip, opt)
      end
    end

    def layout
      @layout
    end

    def cell
      @cell
    end

  end

  class RDBOutputChannel < OutputChannel

    def initialize(engine, rdb, rdb_index, cellname, file_name)
      super(engine)
      @rdb = rdb
      @rdb_index = rdb_index
      @cell = cellname && rdb.create_cell(cellname)
      @file_name = file_name
    end

    def cellname=(cellname)
      @cell = nil
      @rdb.each_cell do |c|
        if c.name == cellname
          @cell = c
        end
      end
      @cell ||= @rdb.create_cell(cellname)
    end

    def is_rdb?
      true
    end

    def finish(final, view)
      write
      if final && view
        view.show_rdb(@rdb_index, view.active_cellview_index)
      end
    end

    def write

      if @file_name
      
        rdb_file = @engine._make_path(@file_name)

        #  Apply waive DB if possible
        wdb_file = rdb_file + ".w"
        if File.exists?(wdb_file)
          begin
            wdb = RBA::ReportDatabase::new
            wdb.load(wdb_file)
            @engine.info("Applying waive database: #{wdb_file} ..")
            @rdb.apply(wdb)
            wdb._destroy
          rescue
          end
        end

        @engine.info("Writing report database: #{rdb_file} ..")
        @rdb.save(rdb_file)

      end

    end

    def rdb
      @rdb
    end

    def cell
      @cell
    end

  end

  # The DRC engine
  
  # %DRC%
  # @scope 
  # @name global 
  # @brief DRC Reference: Global Functions
  # Some functions are available on global level and can be used without any object.
  # Most of them are convenience functions that basically act on some default object
  # or provide function-like alternatives for the methods.

  class DRCEngine
  
    def initialize

      cv = RBA::CellView::active

      @time = Time::now
      @force_gc = ($drc_force_gc == true)   # for testing, $drc_force_gc can be set to true
      @generator = ""
      @rdb_index = nil
      @l2ndb_index = nil
      @def_layout = cv && cv.layout
      @def_cell = cv && cv.cell
      @def_path = cv && cv.filename
      @def_source = nil
      @dbu_read = false
      use_dbu(@def_layout && @def_layout.dbu)
      @show_l2ndb = nil
      @def_output = nil
      @other_outputs = []
      @output_l2ndb_file = nil
      @target_netlist_file = nil
      @target_netlist_format = nil
      @target_netlist_comment = nil
      @used_output_layers = {}
      @output_layers = []
      @vnum = 1
      @layout_sources = {}
      @lnum = 1
      @log_file = nil
      @dss = nil
      @deep = false
      @netter = nil
      @netter_data = nil
      @total_timer = nil
      @drc_progress = nil
      
      # initialize the defaults for max_area_ratio, max_vertex_count
      dss = RBA::DeepShapeStore::new
      @max_area_ratio = dss.max_area_ratio
      @max_vertex_count = dss.max_vertex_count
      @deep_reject_odd_polygons = dss.reject_odd_polygons
      dss._destroy

      @verbose = false
      @profile = false
      @profile_n = 0
      @profile_info = {}

      @in_context = nil

    end

    # avoids lengthy error messages
    def inspect
      "DRCEngine(...)"
    end

    # for testing
    def force_gc(f)
      @force_gc = f
    end

    def both
      DRCBothEdges::new
    end
    
    def ortho
      DRCOrthoEdges::new
    end
    
    def diagonal_only
      DRCDiagonalOnlyEdges::new
    end
    
    def diagonal
      DRCDiagonalEdges::new
    end
    
    def shift(x, y)
      self._context("shift") do
        RBA::DCplxTrans::new(RBA::DVector::new(_make_value(x) * self.dbu, _make_value(y) * self.dbu))
      end
    end
    
    def magnify(m)
      self._context("magnify") do
        RBA::DCplxTrans::new(_make_numeric_value(m))
      end
    end
    
    def rotate(a)
      self._context("rotate") do
        RBA::DCplxTrans::new(1.0, _make_numeric_value(a), false, RBA::DVector::new)
      end
    end
    
    def mirror_x
      self._context("mirror_x") do
        RBA::DCplxTrans::new(1.0, 0.0, true, RBA::DVector::new)
      end
    end
    
    def mirror_y
      self._context("mirror_y") do
        RBA::DCplxTrans::new(1.0, 180.0, true, RBA::DVector::new)
      end
    end
    
    def joined
      DRCJoinFlag::new(true)
    end
    
    def convex
      DRCEdgeMode::new(RBA::EdgeMode::Convex)
    end
    
    def not_convex
      DRCEdgeMode::new(RBA::EdgeMode::NotConvex)
    end
    
    def concave
      DRCEdgeMode::new(RBA::EdgeMode::Concave)
    end
    
    def not_concave
      DRCEdgeMode::new(RBA::EdgeMode::NotConcave)
    end
    
    def step_in
      DRCEdgeMode::new(RBA::EdgeMode::StepIn)
    end
    
    def not_step_in
      DRCEdgeMode::new(RBA::EdgeMode::NotStepIn)
    end
    
    def step_out
      DRCEdgeMode::new(RBA::EdgeMode::StepOut)
    end
    
    def not_step_out
      DRCEdgeMode::new(RBA::EdgeMode::NotStepOut)
    end
    
    def step
      DRCEdgeMode::new(RBA::EdgeMode::Step)
    end
    
    def not_step
      DRCEdgeMode::new(RBA::EdgeMode::NotStep)
    end
    
    def padding_zero
      DRCDensityPadding::new(:zero)
    end

    def padding_ignore
      DRCDensityPadding::new(:ignore)
    end

    def diamond_limit
      DRCSizingMode::new(0)
    end
    
    def octagon_limit
      DRCSizingMode::new(1)
    end
    
    def square_limit
      DRCSizingMode::new(2)
    end
    
    def acute_limit
      DRCSizingMode::new(3)
    end
    
    def no_limit
      DRCSizingMode::new(4)
    end
    
    def shielded
      DRCShielded::new(true)
    end
    
    def transparent
      DRCShielded::new(false)
    end

    def projection_limits(*args)
      self._context("projection_limits") do
        if args.size == 0
          raise("At least one argument is required")
        end
        res = DRCProjectionLimits::new(*args)
        res.description = "projection_limits"
        res
      end
    end
    
    def projecting
      self._context("projecting") do
        res = DRCProjectionLimits::new
        res.description = "projecting"
        res
      end
    end
    
    def angle_limit(a)
      self._context("angle_limit") do
        DRCAngleLimit::new(a)
      end
    end
    
    def whole_edges(f = true)
      self._context("whole_edges") do
        DRCWholeEdges::new(f)
      end
    end
    
    def without_touching_corners(f = true)
      self._context("without_touching_corners") do
        DRCZeroDistanceMode::new(f ? RBA::Region::IncludeZeroDistanceWhenOverlapping : RBA::Region::IncludeZeroDistanceWhenTouching)
      end
    end
    
    def without_touching_edges(f = true)
      self._context("without_touching_edges") do
        DRCZeroDistanceMode::new(f ? RBA::Region::NeverIncludeZeroDistance : RBA::Region::IncludeZeroDistanceWhenTouching)
      end
    end
    
    def euclidian
      DRCMetrics::new(RBA::Region::Euclidian)
    end
    
    def square
      DRCMetrics::new(RBA::Region::Square)
    end
    
    def projection
      DRCMetrics::new(RBA::Region::Projection)
    end
    
    def not_opposite
      DRCOppositeErrorFilter::new(RBA::Region::NotOpposite)
    end
    
    def only_opposite
      DRCOppositeErrorFilter::new(RBA::Region::OnlyOpposite)
    end
    
    def one_side_allowed
      DRCRectangleErrorFilter::new(RBA::Region::OneSideAllowed)
    end
    
    def two_sides_allowed
      DRCRectangleErrorFilter::new(RBA::Region::TwoSidesAllowed)
    end
    
    def two_connected_sides_allowed
      DRCRectangleErrorFilter::new(RBA::Region::TwoConnectedSidesAllowed)
    end
    
    def two_opposite_sides_allowed
      DRCRectangleErrorFilter::new(RBA::Region::TwoOppositeSidesAllowed)
    end
    
    def three_sides_allowed
      DRCRectangleErrorFilter::new(RBA::Region::ThreeSidesAllowed)
    end
    
    def four_sides_allowed
      DRCRectangleErrorFilter::new(RBA::Region::FourSidesAllowed)
    end
    
    def prop(k)
      self._context("prop") do
        DRCPropertyName::new(k)
      end
    end
    
    # %DRC%
    # @brief Specifies "same properties" for operations supporting user properties constraints
    # @name props_eq
    # 
    # Some operations such as boolean AND support properties constraints. By giving 
    # a "props_eq" constraint, the operation is performed only on shapes with the same
    # properties, where "properties" stands for the full set of key/value pairs.
    # 
    # Note that you have to enable properties explicitly or generate properties (e.g. 
    # with the \DRCLayer#nets method).
    #
    # Example:
    # 
    # @code
    # connect(metal1, via1)
    # connect(via1, metal2)
    # ... further connect statements
    # 
    # m1m2_overlap_connected = metal1.nets.and(metal2, props_eq) 
    # @/code
    # 
    # "props_eq" can be combined with \props_copy. In this case, properties
    # are transferred to the output shapes and can be used in further processing:
    #
    # @code
    # m1m2_overlap_connected = metal1.nets.and(metal2, props_eq + props_copy) 
    # @/code
    #
    # See also \props_ne.

    # %DRC%
    # @brief Specifies "different properties" for operations supporting user properties constraints
    # @name props_ne
    # 
    # Some operations such as boolean AND support properties constraints. By giving 
    # a "props_ne" constraint, the operation is performed only on shapes with different
    # properties, where "properties" stands for the full set of key/value pairs.
    # 
    # Note that you have to enable properties explicitly or generate properties (e.g. 
    # with the \DRCLayer#nets method).
    #
    # Example:
    # 
    # @code
    # connect(metal1, via1)
    # connect(via1, metal2)
    # ... further connect statements
    # 
    # m1m2_overlap_not_connected = metal1.nets.and(metal2, props_ne)
    # @/code
    #
    # "props_ne" can be combined with \props_copy. In this case, properties
    # are transferred to the output shapes and can be used in further processing:
    #
    # @code
    # m1m2_overlap_connected = metal1.nets.and(metal2, props_ne + props_copy) 
    # @/code
    #
    # See also \props_eq.

    # %DRC%
    # @brief Specifies "copy properties" on operations supporting user properties constraints
    # @name props_copy
    #
    # This properties constraint does not constrain the operation, but instructs it to 
    # attach the properties from the primary input to the output objects.
    # 
    # See also \props_ne and \props_eq.
     
    def props_eq
      self._context("props_eq") do
        DRCPropertiesConstraint::new(RBA::Region::SamePropertiesConstraintDrop)
      end
    end
    
    def props_ne
      self._context("props_ne") do
        DRCPropertiesConstraint::new(RBA::Region::DifferentPropertiesConstraintDrop)
      end
    end
    
    def props_copy
      self._context("props_copy") do
        DRCPropertiesConstraint::new(RBA::Region::NoPropertyConstraint)
      end
    end

    def negative
      DRCNegative::new
    end

    def enable_props
      DRCPropertySelector::new(:enable_properties)
    end
    
    def remove_props
      DRCPropertySelector::new(:remove_properties)
    end
    
    def select_props(*keys)
      self._context("select_props") do
        keys.each do |k|
          k.is_a?(String) || k.is_a?(1.class) || raise("Key values need to be integers or strings (got '#{k.inspect}')")
        end
        DRCPropertySelector::new(:filter_properties, keys)
      end
    end
    
    def map_props(hash)
      self._context("map_props") do
        hash.is_a?(Hash) || raise("Argument needs to be a hash (got '#{hash.inspect}')")
        DRCPropertySelector::new(:map_properties, hash)
      end
    end
    
    def pattern(p)
      self._context("pattern") do
        DRCPattern::new(true, p)
      end
    end
    
    def text(p)
      self._context("text") do
        DRCPattern::new(false, p)
      end
    end

    def absolute
      DRCAbsoluteMode::new(true)
    end
    
    def as_dots
      DRCOutputMode::new(:dots)
    end
    
    def as_edges
      DRCOutputMode::new(:edges)
    end
    
    def as_boxes
      DRCOutputMode::new(:boxes)
    end
    
    def as_edge_pairs
      DRCOutputMode::new(:edge_pairs)
    end
    
    def area_only(r)
      self._context("area_only") do
        DRCAreaAndPerimeter::new(r, 1.0, 0.0)
      end
    end
    
    def perimeter_only(r, f)
      self._context("perimeter_only") do
        DRCAreaAndPerimeter::new(r, 0.0, f)
      end
    end
    
    def area_and_perimeter(r, f)
      self._context("area_and_perimeter") do
        DRCAreaAndPerimeter::new(r, 1.0, f)
      end
    end

    def fill_pattern(name)
      DRCFillCell::new(name)
    end

    def hstep(x, y = nil)
      DRCFillStep::new(true, x, y)
    end
    
    def vstep(x, y = nil)
      DRCFillStep::new(false, x, y)
    end

    def auto_origin
      DRCFillOrigin::new
    end

    def multi_origin
      DRCFillOrigin::new(nil, nil, true)
    end

    def origin(x, y)
      DRCFillOrigin::new(x, y)
    end
    
    def tile_size(x, y = nil)
      DRCTileSize::new(_make_value(x) * self.dbu, _make_value(y || x) * self.dbu)
    end
    
    def tile_step(x, y = nil)
      DRCTileStep::new(_make_value(x) * self.dbu, _make_value(y || x) * self.dbu)
    end
    
    def tile_origin(x, y)
      DRCTileOrigin::new(_make_value(x) * self.dbu, _make_value(y) * self.dbu)
    end
    
    def tile_count(x, y)
      DRCTileCount::new(_make_numeric_value(x), _make_numeric_value(y))
    end
    
    def tile_boundary(b)
      b.is_a?(DRCLayer) || raise("'tile_boundary' requires a layer argument")
      DRCTileBoundary::new(b)
    end
    
    # %DRC%
    # @brief Defines SPICE output format (with options) 
    # @name write_spice
    # @synopsis write_spice([ use_net_names [, with_comments ] ])
    # @synopsis write_spice(writer_delegate [, use_net_names [, with_comments ] ])
    # Use this option in \target_netlist for the format parameter to 
    # specify SPICE format.
    # "use_net_names" and "with_comments" are boolean parameters indicating
    # whether to use named nets (numbers if false) and whether to add 
    # information comments such as instance coordinates or pin names.
    #
    # "writer_delegate" allows using a RBA::NetlistSpiceWriterDelegate object to 
    # control the actual writing.

    def write_spice(*args)

      self._context("write_spice") do

        delegate = nil
        use_net_names = nil
        with_comments = nil
        args.each do |a|
          if (a == false || a == true) && (use_net_names == nil || with_comments == nil)
            if use_net_names == nil
              use_net_names = a
            else
              with_comments = a
            end
          elsif a.is_a?(RBA::NetlistSpiceWriterDelegate)
            delegate = a
          else
            raise("Too many arguments specified or argument is of wrong type: " + a.inspect)
          end
        end
            
        writer = RBA::NetlistSpiceWriter::new(delegate)
        if use_net_names != nil
          writer.use_net_names = use_net_names
        end
        if with_comments != nil
          writer.with_comments = with_comments
        end

        writer

      end
    end

    def _make_factory(cls)
      if !cls
        return nil
      elsif !cls.is_a?(Class)
        raise("Expected a class object for the 'class' argument of device extractors")
      else
        CustomDeviceClassFactory::new(cls)
      end
    end

    # %DRC%
    # @brief Supplies the MOS3 transistor extractor class
    # @name mos3
    # @synopsis mos3(name)
    # @synopsis mos3(name, class)
    # Use this class with \extract_devices to specify extraction of a 
    # three-terminal MOS transistor.
    #
    # See RBA::DeviceExtractorMOS3Transistor for more details
    # about this extractor (non-strict mode applies for 'mos3').

    def mos3(name, cls = nil)
      self._context("mos3") do
        RBA::DeviceExtractorMOS3Transistor::new(name, false, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the MOS4 transistor extractor class
    # @name mos4
    # @synopsis mos4(name)
    # @synopsis mos4(name, class)
    # Use this class with \extract_devices to specify extraction of a 
    # four-terminal MOS transistor.
    #
    # See RBA::DeviceExtractorMOS4Transistor for more details
    # about this extractor (non-strict mode applies for 'mos4').

    def mos4(name, cls = nil)
      self._context("mos4") do
        RBA::DeviceExtractorMOS4Transistor::new(name, false, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the DMOS3 transistor extractor class
    # @name dmos3
    # @synopsis dmos3(name)
    # @synopsis dmos3(name, class)
    # Use this class with \extract_devices to specify extraction of a 
    # three-terminal DMOS transistor. A DMOS transistor is essentially
    # the same than a MOS transistor, but source and drain are 
    # separated.
    #
    # See RBA::DeviceExtractorMOS3Transistor for more details
    # about this extractor (strict mode applies for 'dmos3').

    def dmos3(name, cls = nil)
      self._context("dmos3") do
        RBA::DeviceExtractorMOS3Transistor::new(name, true, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the MOS4 transistor extractor class
    # @name dmos4
    # @synopsis dmos4(name)
    # @synopsis dmos4(name, class)
    # Use this class with \extract_devices to specify extraction of a 
    # four-terminal DMOS transistor. A DMOS transistor is essentially
    # the same than a MOS transistor, but source and drain are 
    # separated.
    #
    # See RBA::DeviceExtractorMOS4Transistor for more details
    # about this extractor (strict mode applies for 'dmos4').

    def dmos4(name, cls = nil)
      self._context("dmos4") do
        RBA::DeviceExtractorMOS4Transistor::new(name, true, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the BJT3 transistor extractor class
    # @name bjt3
    # @synopsis bjt3(name)
    # @synopsis bjt3(name, class)
    # Use this class with \extract_devices to specify extraction of a 
    # bipolar junction transistor
    #
    # See RBA::DeviceExtractorBJT3Transistor for more details
    # about this extractor.

    def bjt3(name, cls = nil)
      self._context("bjt3") do
        RBA::DeviceExtractorBJT3Transistor::new(name, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the BJT4 transistor extractor class
    # @name bjt4
    # @synopsis bjt4(name)
    # @synopsis bjt4(name, class)
    # Use this class with \extract_devices to specify extraction of a 
    # bipolar junction transistor with a substrate terminal
    #
    # See RBA::DeviceExtractorBJT4Transistor for more details
    # about this extractor.

    def bjt4(name, cls = nil)
      self._context("bjt4") do
        RBA::DeviceExtractorBJT4Transistor::new(name, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the diode extractor class
    # @name diode
    # @synopsis diode(name)
    # @synopsis diode(name, class)
    # Use this class with \extract_devices to specify extraction of a 
    # planar diode 
    #
    # See RBA::DeviceExtractorDiode for more details
    # about this extractor.

    def diode(name, cls = nil)
      self._context("diode") do
        RBA::DeviceExtractorDiode::new(name, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the resistor extractor class
    # @name resistor
    # @synopsis resistor(name, sheet_rho)
    # @synopsis resistor(name, sheet_rho, class)
    # Use this class with \extract_devices to specify extraction of a resistor.
    #
    # The sheet_rho value is the sheet resistance in ohms/square. It is used
    # to compute the resistance from the geometry.
    #
    # See RBA::DeviceExtractorResistor for more details
    # about this extractor.

    def resistor(name, sheet_rho, cls = nil)
      self._context("resistor") do
        RBA::DeviceExtractorResistor::new(name, sheet_rho, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the resistor extractor class that includes a bulk terminal
    # @name resistor_with_bulk
    # @synopsis resistor_with_bulk(name, sheet_rho)
    # @synopsis resistor_with_bulk(name, sheet_rho, class)
    # Use this class with \extract_devices to specify extraction of a resistor 
    # with a bulk terminal.
    # The sheet_rho value is the sheet resistance in ohms/square.
    #
    # See RBA::DeviceExtractorResistorWithBulk for more details
    # about this extractor.

    def resistor_with_bulk(name, sheet_rho, cls = nil)
      self._context("resistor_with_bulk") do
        RBA::DeviceExtractorResistorWithBulk::new(name, sheet_rho, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the capacitor extractor class
    # @name capacitor
    # @synopsis capacitor(name, area_cap)
    # @synopsis capacitor(name, area_cap, class)
    # Use this class with \extract_devices to specify extraction of a capacitor.
    # The area_cap argument is the capacitance in Farad per square micrometer.
    #
    # See RBA::DeviceExtractorCapacitor for more details
    # about this extractor.

    def capacitor(name, area_cap, cls = nil)
      self._context("capacitor") do
        RBA::DeviceExtractorCapacitor::new(name, area_cap, _make_factory(cls))
      end
    end

    # %DRC%
    # @brief Supplies the capacitor extractor class that includes a bulk terminal
    # @name capacitor_with_bulk
    # @synopsis capacitor_with_bulk(name, area_cap)
    # @synopsis capacitor_with_bulk(name, area_cap, class)
    # Use this class with \extract_devices to specify extraction of a capacitor 
    # with a bulk terminal.
    # The area_cap argument is the capacitance in Farad per square micrometer.
    #
    # See RBA::DeviceExtractorCapacitorWithBulk for more details
    # about this extractor.

    def capacitor_with_bulk(name, area_cap, cls = nil)
      self._context("capacitor_with_bulk") do
        RBA::DeviceExtractorCapacitorWithBulk::new(name, area_cap, _make_factory(cls))
      end
    end

    # %DRC%
    # @name profile
    # @brief Profiles the script and provides a runtime + memory statistics
    # @synopsis profile
    # @synopsis profile(n)
    # Turns profiling on or off (default). In profiling mode, the 
    # system will collect statistics about rules executed, their execution time
    # and memory information. The argument specifies how many operations to 
    # print at the end of the run. Without an argument, all operations are
    # printed. Passing "false" for the argument will disable profiling. This is the
    # default.

    def profile(n = 0)
      if !n.is_a?(1.class) && n != nil && n != false
        raise("Argument to 'profile' must be either an integer number or nil")
      end
      @profile = !!n
      @profile_n = [n || 0, 0].max
    end

    # %DRC%
    # @name verbose?
    # @brief Returns true, if verbose mode is enabled
    # @synopsis verbose?
    # In verbose mode, more output is generated in the log file
    
    def verbose?
      @verbose
    end
    
    # %DRC%
    # @name verbose
    # @brief Sets or resets verbose mode
    # @synopsis verbose
    # @synopsis verbose(m)
    # In verbose mode, more output is generated in the log file
    
    def verbose(f = true)
      self.verbose = f
    end
    
    def verbose=(f)
      @verbose = f
    end
    
    # %DRC%
    # @name silent
    # @brief Resets verbose mode
    # @synopsis silent
    # This function is equivalent to "verbose(false)" (see \verbose)
    
    def silent(f = true)
      self.verbose = !f
    end
    
    # %DRC%
    # @name info 
    # @brief Outputs as message to the logger or progress window
    # @synopsis info(message)
    # @synopsis info(message, indent)
    # Prints the message to the log window in verbose mode.
    # In non-verbose more, nothing is printed but a statement is put into the progress window.
    # \log is a function that always prints a message.
    
    def info(arg, indent = 0)
      if @verbose
        log(arg, indent)
      else
        str = ("    " * indent) + arg
        RBA::Logger::log(str)
        self._process_events
      end
    end
    
    # %DRC%
    # @name log 
    # @brief Outputs as message to the logger window
    # @synopsis log(message)
    # @synopsis log(message, indent)
    # Prints the message to the log window.
    # \info is a function that prints a message only if 
    # verbose mode is enabled.
    
    def log(arg, indent = 0)
      str = ("    " * indent) + arg
      if @log_file
        @log_file.puts(str)
      else
        RBA::Logger::info(str)
        self._process_events
      end
    end
    
    # %DRC%
    # @name error
    # @brief Prints an error
    # @synopsis error(message)
    # Similar to \log, but the message is printed formatted as an error
    
    def error(arg)
      if @log_file
        @log_file.puts("ERROR: " + arg)
      else
        RBA::Logger::error(arg)
        self._process_events
      end
    end
    
    # %DRC%
    # @name warn
    # @brief Prints a warning
    # @synopsis warn(message)
    # Similar to \log, but the message is printed formatted as a warning
    
    def warn(arg)
      if @log_file
        @log_file.puts("WARNING: " + arg)
      else
        RBA::Logger::warn(arg)
        self._process_events
      end
    end
    
    # %DRC%
    # @name log_file
    # @brief Specify the log file where to send to log to
    # @synopsis log_file(filename)
    # After using that method, the log output is sent to the 
    # given file instead of the logger window or the terminal.
    
    def log_file(arg)
      @log_file && @log_file.close
      @log_file = File.open(arg, "w")
    end
    
    # Specify the database unit to use
    
    def use_dbu(d)
      if @dbu_read 
        raise("Cannot change the database unit at this point")
      end
      # Should have a "context", but no such thing for Float or Fixnum
      1.0.class._dbu = d
      1.class._dbu = d
      @dbu = d
    end
    
    # %DRC%
    # @name dbu
    # @brief Gets or sets the database unit to use
    # @synopsis dbu(dbu_value)
    # @synopsis dbu
    # Without any argument, this method gets the database unit
    # used inside the DRC engine. 
    #
    # With an argument, sets the database unit used internally in the DRC engine.
    # Without using that method, the database unit is automatically
    # taken as the database unit of the last input. 
    # A specific database unit can be set in order to optimize
    # for two layouts (i.e. take the largest common denominator).
    # When the database unit is set, it must be set at the beginning
    # of the script and before any operation that uses it.
    
    def dbu(d = nil)
      if !d
        @dbu || raise("No database unit specified")
      else
        use_dbu(d.to_f)
      end
      @dbu_read = true
      @dbu
    end

    def dbu=(d)
      self.dbu(d)
    end
    
    # %DRC%
    # @name tiles
    # @brief Specifies tiling 
    # @synopsis tiles(t)
    # @synopsis tiles(w, h)
    # Specifies tiling mode. In tiling mode, the DRC operations are evaluated in tiles
    # with width w and height h. With one argument, square tiles with width and height
    # t are used.
    # 
    # Special care must be taken when using tiling mode, since some operations may not
    # behave as expected at the borders of the tile. Tiles can be made overlapping by
    # specifying a tile border dimension with \tile_borders. Some operations like sizing,
    # the DRC functions specify a tile border implicitly. Other operations without a
    # defined range won't do so and the consequences of tiling mode can be difficult to
    # predict.
    #
    # In tiling mode, the memory requirements are usually smaller (depending on the 
    # choice of the tile size) and multi-CPU support is enabled (see \threads).
    # To disable tiling mode use \flat or \deep. 
    #
    # Tiling mode will disable deep mode (see \deep).
    
    def tiles(tx, ty = nil)
      @tx = tx.to_f
      @ty = (ty || tx).to_f
      @deep = false
    end

    def tiles=(t)
      self.tiles(t)
    end
    
    # %DRC%
    # @name tile_borders
    # @brief Specifies a minimum tile border
    # @synopsis tile_borders(b)
    # @synopsis tile_borders(bx, by)
    # The tile border specifies the distance to which shapes are collected into the 
    # tile. In order words, when processing a tile, shapes within the border distance
    # participate in the operations.
    #
    # For some operations such as booleans (\Layer#and, \Layer#or, ...), \Layer#size and the DRC functions (\Layer#width, \Layer#space, ...)
    # a tile border is automatically established. For other operations such as \Layer#with_area
    # or \Layer#edges, the exact distance is unknown, because such operations may have a long range.
    # In that cases, no border is used. The tile_borders function may be used to specify a minimum border
    # which is used in that case. That allows taking into account at least shapes within the 
    # given range, although not necessarily all.
    # 
    # To reset the tile borders, use \no_borders or "tile_borders(nil)".
    
    def tile_borders(bx, by = nil)
      @bx = bx.to_f
      @by = (by || bx).to_f
    end
    
    def tile_borders=(b)
      self.tile_borders(b)
    end
    
    # %DRC%
    # @name no_borders
    # @brief Reset the tile borders
    # @synopsis no_borders
    # Resets the tile borders - see \tile_borders for a description of tile borders.
    
    def no_borders
      @bx = @by = nil
    end
    
    # %DRC%
    # @name is_tiled?
    # @brief Returns true, if in tiled mode
    # @synopsis is_tiled?
    
    def is_tiled?
      @tx != nil
    end
    
    # %DRC%
    # @name deep
    # @brief Enters deep (hierarchical) mode
    # @synopsis deep
    #
    # In deep mode, the operations will be performed in a hierarchical fashion. 
    # Sometimes this reduces the time and memory required for an operation, but this
    # will also add some overhead for the hierarchical analysis.
    #
    # "deepness" is a property of layers. Layers created with "input" while in 
    # deep mode carry hierarchy. Operations involving such layers at the only
    # or the first argument are carried out in hierarchical mode. 
    # 
    # Hierarchical mode has some more implications, like "merged_semantics" being
    # implied always. Sometimes cell variants will be created.
    #
    # Deep mode can be cancelled with \tiles or \flat.
    
    def deep
      @deep = true
      @tx = @ty = nil
    end
    
    # %DRC%
    # @name is_deep?
    # @brief Returns true, if in deep mode
    # @synopsis is_deep?
    
    def is_deep?
      @deep
    end
    
    # %DRC%
    # @name flat
    # @brief Disables tiling mode 
    # @synopsis flat
    # Disables tiling mode. Tiling mode can be enabled again with \tiles later.
    
    def flat
      @tx = @ty = nil
      @deep = false
    end
    
    # %DRC%
    # @name threads
    # @brief Specifies the number of CPU cores to use in tiling mode
    # @synopsis threads(n)
    # @synopsis threads
    # If using threads, tiles are distributed on multiple CPU cores for
    # parallelization. Still, all tiles must be processed before the 
    # operation proceeds with the next statement. 
    #
    # Without an argument, "threads" will return the current number of 
    # threads
    
    def threads(n = nil)
      if n
        @tt = n.to_i
      end
      @tt
    end

    def threads=(n)
      self.threads(n)
    end
    
    # %DRC%
    # @name deep_reject_odd_polygons
    # @brief Gets or sets a value indicating whether the reject odd polygons in deep mode
    # @synopsis deep_reject_odd_polygons(flag)
    # @synopsis deep_reject_odd_polygons
    #
    # In deep mode, non-orientable (e.g. "8"-shaped) polygons may not be resolved properly.
    # By default the interpretation of such polygons is undefined - they may even vanish entirely.
    # By setting this flag to true, the deep mode layout processor will reject such polygons with 
    # an error. 
 
    def deep_reject_odd_polygons(*args)
      if args.size > 0 
        @deep_reject_odd_polygons = args[0] ? true : false
      end
      @deep_reject_odd_polygons
    end

    def deep_reject_odd_polygons=(flag)
      self.deep_reject_odd_polygons(flag)
    end

    # %DRC%
    # @name max_vertex_count
    # @brief Gets or sets the maximum vertex count for deep mode fragmentation
    # @synopsis max_vertex_count(count)
    # @synopsis max_vertex_count
    #
    # In deep mode, polygons with more than the given number of vertexes will be split into
    # smaller chunks to optimize performance (which is better or less complex polygons).
    # The default threshold is 16 vertexes. Use this method with a vertex count to set the
    # value and without an argument to get the current maximum vertex count.
    # Set the value to zero to disable splitting by vertex count.
    # 
    # See also \max_area_ratio for the other option affecting polygon splitting.
 
    def max_vertex_count(count = nil)
      if count
        if count.is_a?(1.class)
          @max_vertex_count = count.to_i
        else
          raise("Argument is not an integer number in max_vertex_count")
        end
      end
      @max_vertex_count
    end

    def max_vertex_count=(count)
      self.max_vertex_count(count)
    end

    # %DRC%
    # @name max_area_ratio
    # @brief Gets or sets the maximum bounding box to polygon area ratio for deep mode fragmentation
    # @synopsis max_area_ratio(ratio)
    # @synopsis max_area_ratio
    #
    # In deep mode, polygons with a bounding box to polygon area ratio bigger than the given number
    # will be split into smaller chunks to optimize performance (which gets better if the polygon's
    # bounding boxes do not cover a lot of empty space).
    # The default threshold is 3.0 which means fairly compact polygons. Use this method with a numeric 
    # argument to set the value and without an argument to get the current maximum area ratio.
    # Set the value to zero to disable splitting by area ratio.
    # 
    # See also \max_vertex_count for the other option affecting polygon splitting.
 
    def max_area_ratio(ratio = nil)
      if ratio
        if ratio.is_a?(1.0.class) || ratio.is_a?(1.class)
          @max_area_ratio = ratio.to_f
        else
          raise("Argument is not a number in max_area_ratio")
        end
      end
      @max_area_ratio
    end

    def max_area_ratio=(ratio)
      self.max_area_ratio(ratio)
    end

    # %DRC%
    # @name make_layer
    # @brief Creates an empty polygon layer based on the hierarchical scheme selected
    # @synopsis make_layer
    # The intention of this method is to provide an empty polygon layer based on the
    # hierarchical scheme selected. This will create a new layer with the hierarchy
    # of the current layout in deep mode and a flat layer in flat mode.
    # This method is similar to \polygon_layer, but the latter does not create
    # a hierarchical layer. Hence the layer created by \make_layer is suitable
    # for use in device extraction for example, while the one
    # delivered by \polygon_layer is not.
    #
    # On the other hand, a layer created by the \make_layer method is not intended to be
    # filled with \Layer#insert.
    
    def make_layer
      layout.make_layer
    end
      
    # %DRC%
    # @name polygon_layer
    # @brief Creates an empty polygon layer
    # @synopsis polygon_layer
    # The intention of that method is to create an empty layer which can be 
    # filled with polygon-like objects using \Layer#insert.
    # A similar method which creates a hierarchical layer in deep mode is 
    # \make_layer. This other layer is better suited for use with device extraction.
    
    def polygon_layer
      DRCLayer::new(self, RBA::Region::new)
    end
      
    # %DRC%
    # @name edge_layer
    # @brief Creates an empty edge layer
    # @synopsis edge_layer
    # The intention of that method is to create an empty layer which can be 
    # filled with edge objects using \Layer#insert.
    
    def edge_layer
      DRCLayer::new(self, RBA::Edges::new)
    end
      
    # %DRC%
    # @name source
    # @brief Specifies a source layout
    # @synopsis source
    # @synopsis source(what)
    # This function replaces the default source layout by the specified
    # file. If this function is not used, the currently active layout 
    # is used as input. 
    #
    # \layout is a similar method which specifies @i an additional @/i input layout.
    # 
    # "what" specifies what input to use. "what" be either
    #
    # @ul
    # @li A string "\@n" specifying input from a layout in the current panel @/li
    # @li A layout filename plus an optional cell name@/li
    # @li A RBA::Layout object plus an optional cell name@/li
    # @li A RBA::Cell object @/li
    # @/ul
    # 
    # Without any arguments the default layout is returned. If a filename is given, a cell name
    # can be specified as the second argument. If none is specified, the top cell is taken which
    # must be unique in that case.
    #
    # @code
    # # XOR between layers 1 of "first_layout.gds" and "second_layout.gds" and sends the results to "xor_layout.gds":
    # target("xor_layout.gds")
    # source("first_layout.gds")
    # l2 = layout("second_layout.gds")
    # (input(1, 0) ^ l2.input(1, 0)).output(100, 0)
    # @/code
    # 
    # For further methods on the source object see \Source.
    
    def source(arg = nil, arg2 = nil)
    
      self._context("source") do

        if arg
        
          if arg.is_a?(String)
          
            if arg =~ /^@(\d+)/
              n = $1.to_i - 1
              view = RBA::LayoutView::current
              view || raise("No view open")
              (n >= 0 && view.cellviews > n) || raise("Invalid layout index @#{n + 1}")
              cv = view.cellview(n)
              cv.is_valid? || raise("Invalid layout @#{n + 1}")
              @def_source = make_source(cv.layout, cv.cell, cv.filename)
            else
              layout = RBA::Layout::new
              info("Reading #{arg} ..")
              layout.read(arg)
              cell = nil 
              if arg2
                arg2.is_a?(String) || raise("Second argument must be a string")
                cell = layout.cell(arg2)
                cell || raise("Cell name #{arg2} not found in input layout")
              end
              @def_source = make_source(layout, cell, arg)
            end
            
          elsif arg.is_a?(RBA::Layout)

            cell = arg2
            if cell.is_a?(String)
              cell = arg.cell(cell)
              cell || raise("Cell name #{cell} not found in input layout")
            elsif !cell.is_a?(RBA::Cell)
              raise("Second argument must be a string or RBA::Cell object")
            end
            @def_source = make_source(arg, cell)

          elsif arg.is_a?(RBA::Cell)
            @def_source = make_source(arg.layout, arg)
          else
            raise("Invalid argument '" + arg.inspect + "'")
          end
        
        else
          @def_source || @def_layout || raise("No layout loaded - no default layout. Use 'layout' or 'source' to explicitly specify a layout.")
          @def_source ||= make_source(@def_layout, @def_cell, @def_path)
        end

        # make default input also default output if none is set yet.
        @def_layout ||= @def_source.layout
        @def_cell ||= @def_source.cell_obj
        @def_path ||= @def_source.path
            
        # use the DBU of the new input as DBU reference
        @dbu_read || use_dbu(@def_source.layout.dbu)

        @def_source

      end

    end

    # %DRC%
    # @name layout
    # @brief Specifies an additional layout for the input source.
    # @synopsis layout
    # @synopsis layout(what)
    # This function can be used to specify a new layout for input.
    # It returns an Source object representing that layout. The "input" method
    # of that object can be used to get input layers for that layout.
    # 
    # "what" specifies what input to use. "what" be either
    #
    # @ul
    # @li A string "\@n" specifying input from a cellview in the current view @/li
    # @li A layout filename plus an optional cell name @/li
    # @li A RBA::Layout object @/li
    # @li A RBA::Cell object @/li
    # @/ul
    # 
    # Without any arguments the default layout is returned.
    #
    # If a file name is given, a cell name can be specified as the second argument.
    # If not, the top cell is taken which must be unique in that case.
    #
    # Having specified a layout for input enables to use the input method
    # for getting input:
    #
    # @code
    # # XOR between layers 1 or the default input and "second_layout.gds":
    # l2 = layout("second_layout.gds")
    # (input(1, 0) ^ l2.input(1, 0)).output(100, 0)
    # @/code
    # 
    # For further methods on the source object see \Source.
    
    def layout(arg = nil, arg2 = nil)
    
      self._context("layout") do

        if arg
        
          if arg.is_a?(String)
          
            if arg =~ /^@(\d+)/
              n = $1.to_i - 1
              view = RBA::LayoutView::current
              view || raise("No view open")
              (n >= 0 && view.cellviews > n) || raise("Invalid layout index @#{n + 1}")
              cv = view.cellview(n)
              cv.is_valid? || raise("Invalid layout @#{n + 1}")
              return make_source(cv.layout, cv.cell, cv.filename)
            else
              layout = RBA::Layout::new
              info("Reading #{arg} ..")
              layout.read(arg)
              cell = nil 
              if arg2
                arg2.is_a?(String) || raise("Second argument of 'source' must be a string")
                cell = layout.cell(arg2)
                cell || raise("Cell name #{arg2} not found in input layout")
              end
              return make_source(layout, cell, arg)
            end
            
          elsif arg.is_a?(RBA::Layout)
            return make_source(layout)
          elsif arg.is_a?(RBA::Cell)
            return make_source(arg.layout, arg)
          else
            raise("Invalid argument for 'layout' method")
          end
        
        else
          @def_source || @def_layout || raise("No layout loaded - no default layout. Use 'layout' or 'source' to explicitly specify a layout.")
          @def_source ||= make_source(@def_layout, @def_cell, @def_path)
          @def_source
        end

      end
          
    end

    # %DRC%
    # @name report
    # @brief Specifies a report database for output
    # @synopsis report(description [, filename [, cellname ] ])
    # After specifying a report database for output, \output method calls are redirected to
    # the report database. The format of the \output calls changes and a category name plus
    # description can be specified rather than a layer/datatype number of layer name.
    # See the description of the output method for details.
    #
    # If a filename is given, the report database will be written to the specified file name.
    # Otherwise it will be shown but not written.
    #
    # If external input is specified with \source, 
    # "report" must be called after "source".
    #
    # The cellname specifies the top cell used for the report file.
    # By default this is the cell name of the default source. If there
    # is no source layout you'll need to give the cell name in the 
    # third parameter.
      
    def report(description, filename = nil, cellname = nil)

      self._context("report") do

        # finish what we got so far
        _finish(false)

        @def_output = nil
        @def_output = _make_report(description, filename, cellname)

      end
      
    end

    # %DRC%
    # @name new_report
    # @brief Creates a new report database object for use in "output"
    # @synopsis new_report(description [, filename [, cellname ] ])
    # 
    # This method creates an independent report object. This object
    # can be used in "output" to send a layer to a different report than
    # the default report or target.
    #
    # Arguments are the same than for \global#report.
    #
    # See \Layer#output for details about this feature.

    def new_report(description, filename = nil, cellname = nil)

      output = nil

      self._context("new_report") do
        output = _make_report(description, filename, cellname)
        @other_outputs << output
      end

      output
      
    end

    # %DRC%
    # @name report_netlist
    # @brief Specifies an extracted netlist report for output
    # @synopsis report_netlist([ filename [, long ] ])
    # This method applies to runsets creating a netlist through
    # extraction. Extraction happens when connections and/or device
    # extractions are made. If this statement is used, the extracted
    # netlist plus the net and device shapes are turned into a 
    # layout-to-netlist report (L2N database) and shown in the 
    # netlist browser window. If a file name is given, the report
    # will also be written to the given file.
    # If a file name is given and "long" is true, a verbose 
    # version of the L2N DB format will be used.
    
    def report_netlist(filename = nil, long = nil)

      self._context("report_netlist") do

        @show_l2ndb = true
        if filename
          filename.is_a?(String) || raise("Argument must be string")
        end
        @output_l2ndb_file = filename
        @output_l2ndb_long = long

      end

    end

    # %DRC%
    # @name target_netlist
    # @brief With this statement, an extracted netlist is finally written to a file
    # @synopsis target_netlist(filename [, format [, comment ] ])
    # This method applies to runsets creating a netlist through
    # extraction. Extraction happens when connections and/or device
    # extractions are made. If this statement is used, the extracted
    # netlist is written to the given file.
    #
    # The format parameter specifies the writer to use. You can use nil
    # to use the standard format or produce a SPICE writer with \write_spice.
    # See \write_spice for more details.
    
    def target_netlist(filename, format = nil, comment = nil)

      self._context("target_netlist") do

        filename.is_a?(String) || raise("First argument must be string")
        @target_netlist_file = filename
        if format
          format.is_a?(RBA::NetlistWriter) || raise("Second argument must be netlist writer object")
        end
        @target_netlist_format = format
        if comment
          comment.is_a?(String) || raise("Third argument must be string")
        end
        @target_netlist_comment = comment

      end

    end

    # %DRC%
    # @name output_cell
    # @brief Specifies a target cell, but does not change the target layout
    # @synopsis output_cell(cellname)
    # This method switches output to the specified cell, but does not
    # change the target layout nor does it switch the output channel to
    # layout if is report database. 
    
    def output_cell(cellname)

      self._context("output_cell") do

        # finish what we got so far
        _flush
        
        if ! @def_output
          if @def_layout
            # establish a new default output from the default layout on this occasion
            @def_output = LayoutOutputChannel::new(self, @def_layout, @def_layout.cell(cellname.to_s), nil)
          end
        else
          @def_output.cellname = cellname.to_s
        end

      end
          
    end

    # %DRC%
    # @name target
    # @brief Specify the target layout
    # @synopsis target(what [, cellname])
    # This function can be used to specify a target layout for output.
    # Subsequent calls of "output" will send their results to that target
    # layout. Using "target" will disable output to a report database.
    # If any target was specified before, that target will be closed and 
    # a new target will be set up.
    # 
    # "what" specifies what input to use. "what" be either
    #
    # @ul
    # @li A string "\@n" (n is an integer) specifying output to a layout in the current panel @/li
    # @li A string "\@+" specifying output to a new layout in the current panel @/li
    # @li A layout filename @/li
    # @li A RBA::Layout object @/li
    # @li A RBA::Cell object @/li
    # @/ul
    # 
    # Except if the argument is a RBA::Cell object, a cellname can be specified 
    # stating the cell name under which the results are saved. If no cellname is 
    # specified, either the current cell or "TOP" is used.
    # 
    
    def target(arg, cellname = nil)

      self._context("target") do
    
        # finish what we got so far
        _finish(false)

        @def_output = nil
        @def_output = _make_target(arg, cellname)

      end

    end
            
    # %DRC%
    # @name new_target
    # @brief Creates a new layout target object for use in "output"
    # @synopsis new_target(what [, cellname])
    # 
    # This method creates an independent target object. This object
    # can be used in "output" to send a layer to a different layout file than
    # the default report or target.
    #
    # Arguments are the same than for \global#target.
    #
    # See \Layer#output for details about this feature.

    def new_target(arg, cellname = nil)

      output = nil

      self._context("new_target") do
        output = _make_target(arg, cellname)
        @other_outputs << output
      end

      output
      
    end

    # %DRC%
    # @name box 
    # @brief Creates a box object
    # @synopsis box(...)
    # This function creates a box object. The arguments are the same than for the 
    # RBA::DBox constructors.
 
    def box(*args)
      self._context("box") do
        RBA::DBox::new(*args)
      end
    end
    
    # %DRC%
    # @name path 
    # @brief Creates a path object
    # @synopsis path(...)
    # This function creates a path object. The arguments are the same than for the 
    # RBA::DPath constructors.
 
    def path(*args)
      self._context("path") do
        RBA::DPath::new(*args)
      end
    end
    
    # %DRC%
    # @name polygon 
    # @brief Creates a polygon object
    # @synopsis polygon(...)
    # This function creates a polygon object. The arguments are the same than for the 
    # RBA::DPolygon constructors.
 
    def polygon(*args)
      self._context("polygon") do
        RBA::DPolygon::new(*args)
      end
    end
    
    # %DRC%
    # @name p
    # @brief Creates a point object
    # @synopsis p(x, y)
    # A point is not a valid object by itself, but it is useful for creating 
    # paths for polygons:
    #
    # @code
    # x = polygon_layer
    # x.insert(polygon([ p(0, 0), p(16.0, 0), p(8.0, 8.0) ]))
    # @/code
    
    def p(x, y)
      self._context("p") do
        RBA::DPoint::new(x, y)
      end
    end
    
    # %DRC%
    # @name edge 
    # @brief Creates an edge object
    # @synopsis edge(...)
    # This function creates an edge object. The arguments are the same than for the 
    # RBA::DEdge constructors.
 
    def edge(*args)
      self._context("edge") do
        RBA::DEdge::new(*args)
      end
    end
    
    # %DRC%
    # @name extent 
    # @brief Creates a new layer with the bounding box of the default source or cell bounding boxes
    # @synopsis extent
    # @synopsis extent(cell_filter)
    # See \Source#extent for a description of that function.
 
    def extent(cell_filter = nil)
      self._context("extent") do
        layout.extent(cell_filter)
      end
    end
    
    # %DRC%
    # @name input 
    # @brief Fetches the shapes from the specified input from the default source
    # @synopsis input(args)
    # See \Source#input for a description of that function. This method will fetch
    # polygons and labels. See \polygons and \labels for more specific versions of
    # this method.
 
    # %DRC%
    # @name polygons 
    # @brief Fetches the polygons (or shapes that can be converted to polygons) from the specified input from the default source
    # @synopsis polygons(args)
    # See \Source#polygons for a description of that function.
 
    # %DRC%
    # @name labels 
    # @brief Gets the labels (text) from an original layer
    # @synopsis labels(args)
    # See \Source#labels for a description of that function.
 
    # %DRC%
    # @name edges 
    # @brief Gets the edges from an original layer
    # @synopsis edges(args)
    # See \Source#edges for a description of that function.
 
    # %DRC%
    # @name edge_pairs 
    # @brief Gets the edges from an original layer
    # @synopsis edge_pairs(args)
    # See \Source#edge_pairs for a description of that function.
 
    %w(
      edge_pairs
      edges
      input
      labels
      polygons
    ).each do |f|
      eval <<"CODE"
      def #{f}(*args)
        self._context("#{f}") do
          layout.#{f}(*args)
        end
      end
CODE
    end

    # %DRC%
    # @name output
    # @brief Outputs a layer to the report database or output layout
    # @synopsis output(layer, args)
    # This function is equivalent to "layer.output(args)". See \Layer#output for details about this function.
    
    def output(layer, *args)
      self._context("output") do
        layer.output(*args)
      end
    end
    
    # %DRC%
    # @name layers 
    # @brief Gets the layers contained in the default source
    # @synopsis layers
    # See \Source#layers for a description of that function.
 
    def layers
      layout.layers
    end
    
    # %DRC%
    # @name cell 
    # @brief Selects a cell for input on the default source
    # @synopsis cell(args)
    # See \Source#cell for a description of that function.
    # In addition to the functionality described there, the global function will also send the output
    # to the specified cell.
    # 
    # The following code will select cell "MACRO" from the input layout:
    # 
    # @code
    # cell("MACRO")
    # # shapes now will be taken from cell "MACRO"
    # l1 = input(1, 0)
    # @/code
 
    def cell(*args)
      self._context("cell") do
        @def_source = layout.cell(*args)
        output_cell(*args)
      end
      nil
    end
    
    # %DRC%
    # @name select 
    # @brief Specifies cell filters on the default source
    # @synopsis select(args)
    # See \Source#select for a description of that function.
    # Using the global version does not create a new source, but
    # modifies the default source.
    #
    # @code
    # # Selects only B cell instances below the top cell
    # select("-", "+B*")
    # l1 = input(1, 0)
    # @/code
 
    def select(*args)
      self._context("select") do
        @def_source = layout.select(*args)
      end
      nil
    end
    
    # %DRC%
    # @name clip 
    # @brief Specifies clipped input on the default source
    # @synopsis clip(args)
    # See \Source#clip for a description of that function.
    #
    # The following code will select shapes within a 500x600 micron rectangle (lower left corner at 0,0) 
    # from the input layout. The shapes will be clipped to that rectangle:
    # 
    # @code
    # clip(0.mm, 0.mm, 0.5.mm, 0.6.mm)
    # # shapes now will be taken from the given rectangle and clipped to it
    # l1 = input(1, 0)
    # @/code
    # 
    # To remove the clip condition, call "clip" without any arguments.
 
    def clip(*args)
      self._context("clip") do
        @def_source = layout.clip(*args)
      end
      nil
    end

    # %DRC%
    # @name region_touch 
    # @brief Specifies region selected input in "touch mode"
    # @synopsis region_touch(args)
    # See \Source#touching for a description of that function.
    #
    # The following code will select shapes touching a 500x600 micron rectangle (lower left corner at 0,0) 
    # from the input layout. The shapes will not be clipped:
    # 
    # @code
    # region_touch(0.mm, 0.mm, 0.5.mm, 0.6.mm)
    # # shapes will now be the ones touching the rectangular region
    # l1 = input(1, 0)
    # @/code
    # 
    # To remove this condition, call "region_touch" without any arguments.
 
    def region_touch(*args)
      self._context("region_touch") do
        @def_source = layout.touching(*args)
      end
      nil
    end

    # %DRC%
    # @name region_overlap 
    # @brief Specifies region selected input in "overlap mode"
    # @synopsis region_overlap(args)
    # See \Source#overlapping for a description of that function.
    #
    # The following code will select shapes overlapping a 500x600 micron rectangle (lower left corner at 0,0) 
    # from the input layout. The shapes will not be clipped:
    # 
    # @code
    # region_overlapping(0.mm, 0.mm, 0.5.mm, 0.6.mm)
    # # shapes will now be the ones overlapping the rectangular region
    # l1 = input(1, 0)
    # @/code
    # 
    # To remove this condition, call "region_overlapping" without any arguments.
 
    def region_overlap(*args)
      self._context("region_overlap") do
        @def_source = layout.overlapping(*args)
      end
      nil
    end

    # %DRC%
    # @name global_transform
    # @brief Gets or sets a global transformation
    # @synopsis global_transform
    # @synopsis global_transform([ transformations ])
    #
    # Applies a global transformation to the default source layout.
    # See \Source#global_transform for a description of this feature.
  
    def global_transform(*args)
      self._context("global_transform") do
        @def_source = layout.global_transform(*args)
      end
      nil
    end

    # %DRC%
    # @name cheat 
    # @brief Hierarchy cheats
    # @synopsis cheat(args) { block }
    #
    # Hierarchy cheats can be used in deep mode to shortcut hierarchy evaluation
    # for certain cells and consider their local configuration only.
    # Cheats are useful for example when dealing with memory arrays. Often
    # such arrays are build from unit cells and those often overlap with their
    # neighbors. Now, if the hierarchical engine encounters such a situation, it
    # will first analyse all these interactions (which can be expensive) and then
    # it may come to the conclusion that boundary instances need to be handled
    # differently than inside instances. This in turn might lead to propagation of
    # shapes and in an LVS context to device externalisation: because some devices
    # might have different parameters for boundary cells than for inside cells, the
    # device instances can no longer be kept inside the unit cell. Specifically for
    # memory arrays, this is not desired as eventually this leads to flattening
    # of the whole array. 
    #
    # The solution is to cheat: provided the unit cell is fully fledged and neighbors
    # do not disturb the unit cell's configuration in critical ways, the unit cell 
    # can be treated as being isolated and results are put together in the usual way.
    #
    # Cheats can be applied on layout operations - specifically booleans - and device
    # extraction operations. Cheats are only effective in \deep mode.
    #
    # For booleans, a cheat means that the cheating cell's boolean results are computed
    # locally and are combined afterwards. A cheat is introduced this way:
    #
    # @code
    # deep
    # 
    # l1 = input(1, 0)
    # l2 = input(2, 0)
    # 
    # # usual booleans
    # l1and2 = l1 & l2
    #
    # # will compute "UNIT_CELL" isolated and everything else in normal hierarchical mode:
    # l1minus2 = cheat("UNIT_CELL) { l1 - l2 }
    # @/code
    #
    # The cheat block can also be wrapped in do .. end statements and can return multiple
    # layer objects:
    #
    # @code
    # deep
    # 
    # l1 = input(1, 0)
    # l2 = input(2, 0)
    # 
    # # computes both AND and NOT of l1 and l2 with cheating for "UNIT_CELL"
    # l1and2, l1minus2 = cheat("UNIT_CELL) do
    #   [ l1 & l2, l1 - l2 ]
    # end
    # @/code
    #
    # (Technically, the cheat code block is a Ruby Proc and cannot create variables
    # outside its scope. Hence the results of this code block have to be passed
    # through the "cheat" method).
    # 
    # To apply cheats for device extraction, use the following scheme:
    #
    # @code
    # deep
    # 
    # poly = input(1, 0)
    # active = input(2, 0)
    #
    # sd = active - poly
    # gate = active & poly
    # 
    # # device extraction with cheating for "UNIT_CELL":
    # cheat("UNIT_CELL") do
    #   extract_devices(mos3("NMOS"), { "SD" => sd, "G" => gate, "tS" => sd, "tD" => sd, "tG" => poly }
    # end
    # @/code
    #
    # The argument to the cheat method is a list of cell name pattern (glob-style
    # pattern). For example:
    #
    # @code
    # cheat("UNIT_CELL*") { ... }
    # cheat("UNIT_CELL1", "UNIT_CELL2") { ... }
    # cheat("UNIT_CELL{1,2}") { ... }
    # @/code
    #
    # For LVS applications, it's usually sufficient to cheat in the device extraction step. 
    # Cheats have been introduced in version 0.26.1.

    def cheat(*args, &block)
      self._wrapper_context("cheat") do
        if _dss
          _dss.push_state
          args.flatten.each { |a| _dss.add_breakout_cells(a.to_s) }
          ret = block.call
          _dss.pop_state
        else
          ret = block.call
        end
        ret
      end
    end
    
    # make some DRCLayer methods available as functions
    # for the engine
    %w(
      enc
      enclosing
      enclosed
      overlap
      sep
      separation
    ).each do |f|
      eval <<"CODE"
        def #{f}(*args)
          self._context("#{f}") do
            if args[0].is_a?(DRCLayer) && args[1].is_a?(DRCLayer)
              obj = args.shift
              return obj.#{f}(*args)
            elsif self.respond_to?(:_cop_#{f})
              # forward to _cop_ implementation for complex DRC operations
              return self._cop_#{f}(*args)
            else
              raise("Function requires at a layer expression for the first two arguments")
            end
          end
        end
CODE
    end
    
    # make some DRCLayer methods available as functions
    # for the engine
    %w(
      and
      andnot
      angle
      area
      bbox 
      centers
      corners
      end_segments
      extended
      extended_in
      extended_out
      extent_refs
      extents
      first_edges
      flatten
      holes
      hulls
      in
      inside_part
      intersections
      iso
      isolated
      join
      length
      merge
      merged
      middle
      move
      moved
      non_rectangles
      non_rectilinear
      non_squares
      non_strict
      not 
      notch
      not_covering
      not_in
      not_inside
      not_interacting
      not_outside
      not_overlapping
      odd_polygons
      ongrid
      or
      output
      outside_part
      perimeter 
      pull_inside 
      pull_interacting 
      pull_overlapping 
      rectangles
      rectilinear
      rounded_corners
      scale
      scaled
      second_edges
      select_covering
      select_inside
      select_interacting
      select_not_covering
      select_not_inside
      select_not_interacting
      select_not_outside
      select_not_overlapping
      select_outside
      select_overlapping
      size
      sized 
      smoothed
      snap 
      snapped
      space
      squares
      start_segments
      strict
      texts
      texts_not
      touching
      transform
      transformed
      width
      with_angle
      with_area
      with_area_ratio
      with_bbox_area
      with_bbox_aspect_ratio
      with_bbox_height
      with_bbox_max
      with_bbox_min
      with_bbox_width
      with_length
      with_distance
      without_angle
      without_area
      without_area_ratio
      without_bbox
      without_bbox_aspect_ratio
      without_bbox_height
      without_bbox_max
      without_bbox_min
      without_length
      without_perimeter
      without_relative_height
      with_perimeter
      with_relative_height
      xor
    ).each do |f|
      eval <<"CODE"
        def #{f}(*args)
          self._context("#{f}") do
            if args[0].is_a?(DRCLayer)
              obj = args.shift
              return obj.#{f}(*args)
            elsif self.respond_to?(:_cop_#{f})
              # forward to _cop_ implementation for complex DRC operations
              return self._cop_#{f}(*args)
            else
              raise("Function requires at a layer expression for the first argument")
            end
          end
        end
CODE
    end
    
    # %DRC%
    # @name netter
    # @brief Creates a new netter object
    # @synopsis netter
    # See \Netter# for more details
 
    def netter
      self._context("netter") do
        DRC::DRCNetter::new
      end
    end

    # %DRC%
    # @name connect
    # @brief Specifies a connection between two layers
    # @synopsis connect(a, b)
    # See \Netter#connect for a description of that function.
 
    # %DRC%
    # @name soft_connect
    # @brief Specifies a soft connection between two layers
    # @synopsis soft_connect(a, b)
    # A "soft connection" is made between two layers and
    # is a directional connection (like an ideal diode).
    # Soft connections allow detecting if nets are connected
    # via a high-ohmic substrate or diffusion layer (the
    # "lower" layer).
    # "b" is the "lower" and "a" the upper layer.
    #
    # See \Netter#connect for a more detailed description of that function.
 
    # %DRC%
    # @name connect_global
    # @brief Specifies a connection to a global net
    # @synopsis connect_global(l, name)
    # See \Netter#connect_global for a description of that function.
 
    # %DRC%
    # @name soft_connect_global
    # @brief Specifies a soft connection to a global net
    # @synopsis soft_connect_global(l, name)
    # Like \soft_connect, a soft connection is made between 
    # a layer and a global net (e.g. substrate). The global net
    # is always the "lower" net of the soft connection.
    #
    # See \Netter#soft_connect_global for a more detailed 
    # description of that function.
 
    # %DRC%
    # @name clear_connections
    # @brief Clears all connections stored so far
    # @synopsis clear_connections
    # See \Netter#clear_connections for a description of that function.

    # %DRC%
    # @name connect_implicit
    # @brief Specifies a label pattern for implicit net connections
    # @synopsis connect_implicit(label_pattern)
    # @synopsis connect_implicit(cell_pattern, label_pattern)
    # See \Netter#connect_implicit for a description of that function.

    # %DRC%
    # @name connect_explicit
    # @brief Specifies explicit net connections
    # @synopsis connect_explicit(net_names)
    # @synopsis connect_explicit(cell_pattern, net_names)
    # See \Netter#connect_explicit for a description of that function.
    # Net names is an array (use square brackets to list the net names).

    # %DRC%
    # @name antenna_check
    # @brief Performs an antenna check
    # @synopsis antenna_check(gate, metal, ratio, [ diode_specs ... ])
    # See \Netter#antenna_check for a description of that function.
 
    # %DRC%
    # @name l2n_data
    # @brief Gets the internal RBA::LayoutToNetlist object for the default \Netter
    # @synopsis l2n_data
    # See \Netter#l2n_data for a description of that function.
 
    # %DRC%
    # @name device_scaling
    # @brief Specifies a dimension scale factor for the geometrical device properties
    # @synopsis device_scaling(factor)
    # See \Netter#device_scaling for a description of that function.
 
    # %DRC%
    # @name top_level
    # @brief Specifies that the circuit is a chip top level circuit
    # @synopsis top_level(flag)
    # See \Netter#top_level for a description of that function.
 
    # %DRC%
    # @name ignore_extraction_errors
    # @brief Specifies whether to ignore extraction errors
    # @synopsis ignore_extraction_errors(value)
    # See \Netter#ignore_extraction_errors for a description of that function.
 
    # %DRC%
    # @name extract_devices
    # @brief Extracts devices for a given device extractor and device layer selection
    # @synopsis extract_devices(extractor, layer_hash)
    # @synopsis extract_devices(extractor_class, name, layer_hash)
    # See \Netter#extract_devices for a description of that function.
 
    # %DRC%
    # @name netlist
    # @brief Obtains the extracted netlist from the default \Netter
    # The netlist is a RBA::Netlist object. If no netlist is extracted 
    # yet, this method will trigger the extraction process.
    # See \Netter#netlist for a description of this function.
 
    %w(
      antenna_check
      clear_connections
      connect
      connect_global
      soft_connect
      soft_connect_global
      connect_implicit
      connect_explicit
      device_scaling
      top_level
      ignore_extraction_errors
      extract_devices
      netlist
      l2n_data
      _l2n_object
      _make_soft_connection_diodes
    ).each do |f|
      eval <<"CODE"
        def #{f}(*args)
          self._context("#{f}") do
            _netter.#{f}(*args)
          end
        end
CODE
    end
    
    def src_line
      cc = caller.find do |c|
        c !~ /drc.lym:/ && c !~ /_drc_\w+\.rb:/ && c !~ /\(eval\)/
      end
      if cc =~ /(.*)\s*:\s*(\d+)\s*:\s*in.*$/
        path = $1
        line = $2.to_i
        real_path = RBA::Macro::real_path(path, line)
        real_line = RBA::Macro::real_line(path, line)
        return File::basename(real_path) + ":" + real_line.to_s
      else
        return cc
      end
    end

    def _wrapper_context(func, *args, &proc)
      in_context_outer = @in_context
      begin
        @in_context = func
        return yield(*args)
      rescue => ex
        RBA::MacroExecutionContext::ignore_next_exception
        raise("'" + func + "': " + ex.to_s)
      ensure 
        @in_context = in_context_outer
      end
    end
    
    def _context(func, *args, &proc)
      if @in_context
        return yield(*args)
      else
        begin
          @in_context = func
          return yield(*args)
        rescue => ex
          RBA::MacroExecutionContext::ignore_next_exception
          raise("'" + func + "': " + ex.to_s)
        ensure
          @in_context = nil
        end
      end
    end
    
    def _result_info(res, indent, prefix = "")
      if res.is_a?(Array)
        res.each_with_index do |a, index|
          _result_info(a, indent, "[#{index + 1}] ")
        end
      elsif res.is_a?(RBA::Region)
        info(prefix + "Polygons (raw): #{res.count} (flat)  #{res.hier_count} (hierarchical)", indent)
      elsif res.is_a?(RBA::Edges)
        info(prefix + "Edges: #{res.count} (flat)  #{res.hier_count} (hierarchical)", indent)
      elsif res.is_a?(RBA::EdgePairs)
        info(prefix + "Edge pairs: #{res.count} (flat)  #{res.hier_count} (hierarchical)", indent)
      elsif res.is_a?(RBA::Texts)
        info(prefix + "Texts: #{res.count} (flat)  #{res.hier_count} (hierarchical)", indent)
      end
    end

    def run_timed(desc, obj)

      info(desc)

      # enable progress
      disable_progress = false
      if obj.is_a?(RBA::Region) || obj.is_a?(RBA::Edges) || obj.is_a?(RBA::EdgePairs) || obj.is_a?(RBA::Texts)
        disable_progress = true
        obj.enable_progress(desc)
      end
      
      t = RBA::Timer::new

      t.start
      self._process_events
      if @force_gc || Time::now - @time > 0.5 || @profile
        GC.start    # force a garbage collection before the operation to free unused memory
        @time = Time::now
      end
      mem_before = RBA::Timer::memory_size
      res = yield
      mem_after = RBA::Timer::memory_size
      t.stop

      begin

        if @verbose

          # Report result statistics
          _result_info(res, 1)

          if mem_after > 0
            info("Elapsed: #{'%.3f'%(t.sys+t.user)}s  Memory: #{'%.2f'%(mem_after/(1024*1024))}M", 1)
          else
            info("Elapsed: #{'%.3f'%(t.sys+t.user)}s", 1)
          end

        end

        if @profile

          # calls, sys time (in sec), user time (in sec), memory added (in bytes)
          info = (@profile_info[desc] ||= [ 0, 0.0, 0.0, 0 ])
          info[0] += 1
          info[1] += t.sys
          info[2] += t.user
          info[3] += mem_after - mem_before

        end

      ensure

        # disable progress again
        if disable_progress
          obj.disable_progress
        end

      end
          
      res

    end
    
    def _cmd(obj, method, *args)
      run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
        obj.send(method, *args)
      end
    end
    
    def _tcmd(obj, border, result_cls, method, *args)
    
      if @tx && @ty
      
        tp = RBA::TilingProcessor::new
        tp.dbu = self.dbu
        tp.scale_to_dbu = false
        tp.tile_size(@tx, @ty)
        bx = [ @bx || 0.0, border * self.dbu ].max
        by = [ @by || 0.0, border * self.dbu ].max
        tp.tile_border(bx, by)

        res = result_cls.new      
        tp.output("res", res)
        tp.input("self", obj)
        tp.threads = (@tt || 1)
        args.each_with_index do |a,i|
          if a.is_a?(RBA::Edges) || a.is_a?(RBA::Region) || a.is_a?(RBA::EdgePairs) || a.is_a?(RBA::Texts)
            tp.input("a#{i}", a)
          else
            tp.var("a#{i}", a)
          end
        end
        av = args.size.times.collect { |i| "a#{i}" }.join(", ")
        tp.queue("_output(res, self.#{method}(#{av}))")
        run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
          tp.execute("Tiled \"#{method}\" in: #{src_line}")
          res
        end
        
      else

        if @dss
          @dss.threads = (@tt || 1)
        end

        res = nil
        run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
          res = obj.send(method, *args)
        end

      end
      
      res
      
    end

    # used for two-element array output methods (e.g. andnot)
    def _tcmd_a2(obj, border, result_cls1, result_cls2, method, *args)
    
      if @tx && @ty
      
        tp = RBA::TilingProcessor::new
        tp.dbu = self.dbu
        tp.scale_to_dbu = false
        tp.tile_size(@tx, @ty)
        bx = [ @bx || 0.0, border * self.dbu ].max
        by = [ @by || 0.0, border * self.dbu ].max
        tp.tile_border(bx, by)

        res1 = result_cls1.new
        tp.output("res1", res1)
        res2 = result_cls2.new
        tp.output("res2", res2)
        res = [ res1, res2 ]
        tp.input("self", obj)
        tp.threads = (@tt || 1)
        args.each_with_index do |a,i|
          if a.is_a?(RBA::Edges) || a.is_a?(RBA::Region) || a.is_a?(RBA::EdgePairs) || a.is_a?(RBA::Texts)
            tp.input("a#{i}", a)
          else
            tp.var("a#{i}", a)
          end
        end
        av = args.size.times.collect { |i| "a#{i}" }.join(", ")
        tp.queue("var rr = self.#{method}(#{av}); _output(res1, rr[0]); _output(res2, rr[1])")
        run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
          tp.execute("Tiled \"#{method}\" in: #{src_line}")
          res
        end
        
      else

        if @dss
          @dss.threads = (@tt || 1)
        end

        res = nil
        run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
          res = obj.send(method, *args)
        end

      end
      
      res
      
    end

    # used for area and perimeter only    
    def _tdcmd(obj, border, method)
    
      if @tx && @ty
      
        tp = RBA::TilingProcessor::new
        tp.tile_size(@tx, @ty)
        tp.tile_border(border * self.dbu, border * self.dbu)

        res = RBA::Value::new
        res.value = 0.0
        tp.output("res", res)
        tp.input("self", obj)
        tp.threads = (@tt || 1)
        tp.queue("_output(res, _tile ? self.#{method}(_tile.bbox) : self.#{method})")
        run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
          tp.execute("Tiled \"#{method}\" in: #{src_line}")
          res
        end
        
        res = res.value
        
      else

        if @dss
          @dss.threads = (@tt || 1)
        end

        res = nil
        run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
          res = obj.send(method)
        end

      end
      
      res
      
    end
    
    def _rcmd(obj, method, *args)
      run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
        RBA::Region::new(obj.send(method, *args))
      end
    end
    
    def _vcmd(obj, method, *args)
      run_timed("\"#{@in_context || method}\" in: #{src_line}", obj) do
        obj.send(method, *args)
      end
    end

    def _bx
      @bx
    end
    
    def _by
      @by
    end
    
    def _tx
      @tx
    end
    
    def _ty
      @ty
    end

    def _output_layout
      if @def_output && @def_output.layout
        output = @def_output.layout
      else
        output = @def_layout
        output || raise("No output layout specified")
      end
      output
    end
    
    def _output_cell
      if @def_output && @def_output.layout 
        if @def_output.cell
          output_cell = @def_output.cell
        elsif @def_cell
          output_layout = @def_output.layout
          output_cell = output_layout.cell(@def_cell.name) || output_layout.create_cell(@def_cell.name)
        end
        output_cell || raise("No output cell specified (see 'target' instruction)")
      else
        output_cell = @def_cell
        output_cell || raise("No output cell specified")
      end
      output_cell
    end

    def _dump_profile

      if @profile_info.empty?
        return
      end

      desc_title = "Operation"
      calls_title = "# calls"
      time_title = "Time (s)"
      memory_title = "Memory (k)"
      titles = [ desc_title, calls_title, time_title, memory_title ]

      max_len_desc  = [ @profile_info.keys.collect { |s| s.size }.max, desc_title.size ].max
      max_len_calls = [ @profile_info.values.collect { |v| v[0].to_s.size }.max, calls_title.size ].max
      max_len_time  = [ @profile_info.values.collect { |v| ("%.3f" % (v[1] + v[2])).to_s.size }.max, time_title.size ].max
      max_len_mem   = [ @profile_info.values.collect { |v| v[3].to_s.size }.max, memory_title.size ].max

      fmt = "%-" + max_len_desc.to_s + "s  " +
            "%-" + max_len_calls.to_s + "d  " +
            "%-" + max_len_time.to_s + ".3f  " + 
            "%-" + max_len_mem.to_s + ".0f" 

      fmt_title = "%-" + max_len_desc.to_s + "s  " +
                  "%-" + max_len_calls.to_s + "s  " +
                  "%-" + max_len_time.to_s + "s  " + 
                  "%-" + max_len_mem.to_s + "s" 

      pi = @profile_info.keys.collect { |s| [s] + @profile_info[s] }.collect do |desc,calls,sys_time,user_time,memory|
        [ desc, calls, sys_time + user_time, memory.to_f / 1024.0 ]
      end

      self.log("")
      self.log("Operations by execution time\n")
      self.log(fmt_title % titles)
      n = 1
      pi.sort { |a,b| b[2] <=> a[2] }.each do |info|
        self.log(fmt % info)
        n += 1
        if @profile_n > 0 && n > @profile_n
          self.log("... (%d entries skipped)" % (pi.size - @profile_n))
          break
        end
      end

      self.log("")
      self.log("Operations by memory adder\n")
      self.log(fmt_title % titles)
      n = 1
      pi.sort { |a,b| b[3] <=> a[3] }.each do |info|
        self.log(fmt % info)
        n += 1
        if @profile_n > 0 && n > @profile_n
          self.log("... (%d entries skipped)" % (pi.size - @profile_n))
          break
        end
      end

    end
    
    def _start(job_description)
    
      # clearing the selection avoids some nasty problems
      view = RBA::LayoutView::current
      view && view.cancel
      
      @total_timer = RBA::Timer::new
      @total_timer.start

      @drc_progress = RBA::AbstractProgress::new(job_description)

    end
    
    def _flush
    
      # clean up resources (i.e. temp layers)
      @layout_sources.each do |n,l|
        l.finish
      end
      
    end
    
    def _finish(final = true)

      begin

        _flush    

        view = RBA::LayoutView::current

        @def_output && @def_output.finish(final, view)

        if final
          @other_outputs.each do |output|
            output.finish(final, view)
          end
        end

        # dump the profile information
        if @profile && final
          _dump_profile
        end
        
        # create the new layers as visual layers if necessary
        if view
        
          output = ( @def_output && @def_output.layout ) || @def_layout
          cv_index = nil
          view.cellviews.times do |cvi|
            view.cellview(cvi).layout == output && cv_index = cvi
          end
          if cv_index
          
            # clear selection
            view.cancel 
      
            # create layer views for those layers which are not present yet
                  
            present_layers = {}
            l = view.begin_layers
            while !l.at_end?
              if l.current.cellview == cv_index
                present_layers[l.current.layer_index] = true
              end
              l.next
            end
            
            @output_layers.each do |li|
              if !present_layers[li]
                info = output.get_info(li)
                lp = RBA::LayerProperties::new
                lp.source_layer = info.layer
                lp.source_datatype = info.datatype
                lp.source_name = info.name
                lp.source_cellview = cv_index
                view.init_layer_properties(lp)
                view.insert_layer(view.end_layers, lp)     
              end
            end
            
            view.update_content
      
          end
          
        end

        # save the netlist if required
        if final && @target_netlist_file && @netter && @netter._l2n_data

          writer = @target_netlist_format || RBA::NetlistSpiceWriter::new

          netlist_file = _make_path(@target_netlist_file)
          info("Writing netlist: #{netlist_file} ..")
          netlist.write(netlist_file, writer, @target_netlist_comment || "")

        end
      
        # save the netlist database if requested
        if final && @output_l2ndb_file && @netter && @netter._l2n_data

          l2ndb_file = _make_path(@output_l2ndb_file)
          info("Writing netlist database: #{l2ndb_file} ..")
          @netter._l2n_data.write_l2n(l2ndb_file, !@output_l2ndb_long)

        end

        # give derived classes a change to perform their actions
        _before_cleanup
      
        # show the data in the browser
        if final && view && @show_l2ndb && @netter && @netter._l2n_data

          # NOTE: to prevent the netter destroying the database, we need to take it
          l2ndb = _take_data
          if self._l2ndb_index
            l2ndb_index = view.replace_l2ndb(self._l2ndb_index, l2ndb)
          else
            l2ndb_index = view.add_l2ndb(l2ndb)
          end
          view.show_l2ndb(l2ndb_index, view.active_cellview_index)

        end

      ensure

        @output_layers = []
        @def_output = nil
        if final
          @other_outputs = []
        end
        @show_l2ndb = nil
        @output_l2ndb_file = nil

        # clean up temp data
        @dss && @dss._destroy
        @dss = nil
        @netter && @netter._finish
        @netter = nil
        @netter_data = nil
        
        if final

          @total_timer.stop
          if @verbose
            mem = RBA::Timer::memory_size
            if mem > 0
              info("Total elapsed: #{'%.3f'%(@total_timer.sys+@total_timer.user)}s  Memory: #{'%.2f'%(mem/(1024*1024))}M")
            else
              info("Total elapsed: #{'%.3f'%(@total_timer.sys+@total_timer.user)}s")
            end
          end

          _cleanup

        end

        # force garbage collection
        GC.start

      end

    end

    def _cleanup

      if @log_file
        @log_file.close
        @log_file = nil
      end

      # unlocks the UI
      if @drc_progress
        @drc_progress._destroy
        @drc_progress = nil
      end

      GC.start

    end

    def _take_data

      if ! @netter
        return nil
      end

      if ! @netter_data

        @netter_data = @netter._take_data

        # we also need to make the extractor take over ownership over the DSS
        # because otherwise we can't free the resources.
        if @netter_data.dss == @dss
          @netter_data.keep_dss
          @dss = nil
        end

      end

      @netter_data

    end

    def _before_cleanup
      # nothing yet
    end

    def _dss
      @dss
    end

    def _default_netter
      @netter 
    end

    def _netter
      @netter ||= DRC::DRCNetter::new(self)
    end

    def _make_path(file)
      # resolves the file path relative to the source's path
      sp = (@def_source || @def_layout) && self.source.path
      if sp
        if File.respond_to?(:absolute_path)
          return File::absolute_path(file, File::dirname(sp))
        else
          return (Pathname::new(File::dirname(sp)) + Pathname.new(file)).to_s
        end
      else
        return file
      end
    end

    def _generator
      @generator
    end

    def _generator=(g)
      @generator = g
    end

    def _rdb_index
      @rdb_index
    end

    def _rdb_index=(i)
      @rdb_index = i
    end
    
    def _l2ndb_index
      @l2ndb_index
    end

    def _l2ndb_index=(i)
      @l2ndb_index = i
    end

    def _prep_value(a)
      if a.is_a?(RBA::DPoint)
        RBA::Point::from_dpoint(a * (1.0 / self.dbu.to_f))
      elsif a.is_a?(RBA::DCplxTrans)
        RBA::ICplxTrans::from_dtrans(RBA::DCplxTrans::new(1.0 / self.dbu.to_f) * a * RBA::DCplxTrans::new(self.dbu.to_f))
      elsif a.is_a?(RBA::DTrans)
        RBA::ICplxTrans::from_dtrans(RBA::DCplxTrans::new(1.0 / self.dbu.to_f) * RBA::DCplxTrans::new(a) * RBA::DCplxTrans::new(self.dbu.to_f))
      elsif a.is_a?(Float)
        (0.5 + a / self.dbu).floor.to_i
      else
        a
      end
    end
    
    def _prep_value_area(a)
      dbu2 = self.dbu.to_f * self.dbu.to_f
      if a.is_a?(Float)
        (0.5 + a / dbu2).floor.to_i
      else
        a
      end
    end
    
    def _check_numeric(v)
      if ! v.is_a?(Float) && ! v.is_a?(1.class)
        raise("Argument (#{v.inspect}) isn't numeric")
      end
    end
    
    def _make_value(v)
      self._check_numeric(v)
      self._prep_value(v)
    end
    
    def _make_area_value(v)
      self._check_numeric(v)
      self._prep_value_area(v)
    end
  
    def _make_numeric_value(v)
      self._check_numeric(v)
      v
    end
  
    def _make_value_with_nil(v)
      if v == nil
        return v
      end
      self._check_numeric(v)
      self._prep_value(v)
    end
    
    def _make_area_value_with_nil(v)
      if v == nil
        return v
      end
      self._check_numeric(v)
      self._prep_value_area(v)
    end
  
    def _make_numeric_value_with_nil(v)
      if v == nil
        return v
      end
      self._check_numeric(v)
      v
    end
  
    def _use_output_layer(li)
      if !@used_output_layers[li]
        @output_layers.push(li)
        @used_output_layers[li] = true
      end
    end

    def _process_events
      if RBA.constants.member?(:Application) && RBA::Application.instance
        RBA::Application.instance.process_events
      end
    end
    
    def _input(layout, cell_index, layers, sel, box, clip, overlapping, shape_flags, global_trans, prop_sel, cls)
    
      if layers.empty? && ! @deep

        r = cls.new
       
      else
    
        if box
          iter = RBA::RecursiveShapeIterator::new(layout, layout.cell(cell_index), layers, box, overlapping)
        else
          iter = RBA::RecursiveShapeIterator::new(layout, layout.cell(cell_index), layers)
        end
        prop_sel.each { |p| p.apply_to(iter) }
        iter.shape_flags = shape_flags
        iter.global_dtrans = global_trans
        
        sel.each do |s|
          if s == "-"
            iter.unselect_cells([cell_index])
          elsif s == "-*"
            iter.unselect_all_cells
          elsif s == "+*"
            iter.select_all_cells
          elsif s =~ /^-(.*)/
            iter.unselect_cells($1)
          elsif s =~ /^\+(.*)/
            iter.select_cells($1)
          else
            iter.select_cells(s)
          end
        end

        sf = layout.dbu / self.dbu
        if @deep

          @dss ||= RBA::DeepShapeStore::new

          # TODO: align with LayoutToNetlist by using a "master" L2N
          # object which keeps the DSS.
          @dss.text_property_name = "LABEL"
          @dss.text_enlargement = 0
          @dss.reject_odd_polygons = @deep_reject_odd_polygons
          @dss.max_vertex_count = @max_vertex_count
          @dss.max_area_ratio = @max_area_ratio

          r = cls.new(iter, @dss, RBA::ICplxTrans::new(sf.to_f))

        else
          r = cls.new(iter, RBA::ICplxTrans::new(sf.to_f))
        end
        
        # clip if a box is specified
        # TODO: the whole clip thing could be a part of the Region constructor
        if cls == RBA::Region && clip && box
          # HACK: deep regions will always clip in the constructor, so skip this
          if ! @deep 
            # NOTE: NoPropertyConstraint will copy the original properties
            r.and_with(RBA::Region::new(box), RBA::Region::NoPropertyConstraint)
          end
        end
      
      end
      
      r

    end
    
  private

    def _make_string(v)
      if v.class.respond_to?(:from_s)
        v.class.to_s + "::from_s(" + v.to_s.inspect + ")"
      else
        v.inspect
      end
    end

    def _layout(name)
      @layout_sources[name].layout
    end
    
    def _output(data, *args)

      channel = args.find { |a| a.is_a?(OutputChannel) }
      if ! channel
        if ! @def_output
          @def_output = LayoutOutputChannel::new(self, self._output_layout, self._output_cell, nil)
        end
        channel = @def_output
      end

      args = args.select { |a| !a.is_a?(OutputChannel) }

      if channel.rdb
        
        if args.size < 1
          raise("Invalid number of arguments - category name and optional description expected")
        end

        output_rdb = channel.rdb
        output_cell = channel.cell

        categories = args[0]
        if !categories.is_a?(Array)
          categories = [ categories.to_s ]
        end

        cat = nil
        categories.each do |c|
          ccat = nil
          if cat
            cat.each_sub_category do |i|
              if i.name == c
                ccat = i
                break
              end
            end
          else
            output_rdb.each_category do |i|
              if i.name == c
                ccat = i
                break
              end
            end
          end
          cat = ccat ? ccat : output_rdb.create_category(cat, c)
        end
        cat ||= output_rdb.create_category("default")

        args[1] && cat.description = args[1]

        cat.scan_collection(output_cell, RBA::CplxTrans::new(self.dbu), data)
      
      end

      if channel.layout

        output = channel.layout
        output_cell = channel.cell

        info = nil
        if args.size == 1
          if args[0].is_a?(1.class)
            info = RBA::LayerInfo::new(args[0], 0)
          elsif args[0].is_a?(RBA::LayerInfo)
            info = args[0]
          elsif args[0].is_a?(String)
            info = RBA::LayerInfo::from_string(args[0])
          else
            raise("Invalid parameter type - must be string or number")
          end
        elsif args.size == 2 || args.size == 3
          info = RBA::LayerInfo::new(*args)
        else
          raise("Invalid number of arguments - one, two or three arguments expected")
        end
        li = output.find_layer(info)
        if !li
          li = output.insert_layer(info)
        end

        # make sure the output has the right database unit
        output.dbu = self.dbu

        tmp = li

        begin

          if channel == @def_output
            if !@used_output_layers[li]
              @output_layers.push(li)
              # Note: to avoid issues with output onto the input layer, we
              # output to a temp layer and later swap both. The simple implementation
              # did a clear here and the effect of that was that the data potentially
              # got invalidated.
              tmp = output.insert_layer(RBA::LayerInfo::new)
              @used_output_layers[li] = true
            end
          end

          # insert the data into the output layer
          if data.is_a?(RBA::EdgePairs)
            data.insert_into_as_polygons(output, output_cell.cell_index, tmp, 1)
          else
            data.insert_into(output, output_cell.cell_index, tmp)
          end

          #  make the temp layer the output layer
          if tmp != li
            output.swap_layers(tmp, li)
          end

        ensure
          #  clean up the original layer if requested
          if tmp != li
            output.delete_layer(tmp)
          end
        end

      end        

      data 

    end

    def make_source(layout, cell = nil, path = nil)
      name = "layout" + @lnum.to_s
      @lnum += 1
      @dbu ||= layout.dbu
      src = DRCSource::new(self, layout, layout, cell || layout.top_cell, path)
      @layout_sources[name] = src
      src
    end
    
    def _make_report(description, filename, cellname)

      output_rdb_file = filename

      name = filename && File::basename(filename)
      name ||= "DRC"
      
      output_rdb_index = nil

      view = RBA::LayoutView::current
      if view
        if self._rdb_index
          output_rdb = RBA::ReportDatabase::new("")   # reuse existing name
          output_rdb_index = view.replace_rdb(self._rdb_index, output_rdb)
        else
          output_rdb = RBA::ReportDatabase::new(name)
          output_rdb_index = view.add_rdb(output_rdb)
        end
      else
        output_rdb = RBA::ReportDatabase::new(name)
      end

      cn = cellname && cellname.to_s
      cn ||= @def_cell && @def_cell.name
      cn ||= @def_source && @def_source.cell_obj && @def_source.cell_obj.name

      cn || raise("No cell name specified - either the source was not specified before 'report' or there is no default source. In the latter case, specify a cell name as the third parameter")

      output_rdb.generator = self._generator
      output_rdb.top_cell_name = cn
      output_rdb.description = description
      output_rdb.original_file = @def_path
      
      RDBOutputChannel::new(self, output_rdb, output_rdb_index, cn, output_rdb_file)
      
    end

    def _make_target(arg, cellname = nil)

      if arg.is_a?(String)
      
        if arg =~ /^@(\d+|\+)/

          view = RBA::LayoutView::current
          view || raise("No view open")
          if $1 == "+"
            n = view.create_layout(true)
            cellname ||= (@def_cell ? @def_cell.name : "TOP")
          else
            n = $1.to_i - 1
          end

          (n >= 0 && view.cellviews > n) || raise("Invalid layout index @#{n + 1}")

          cv = view.cellview(n)
          cv.is_valid? || raise("Invalid layout @#{n + 1}")

          output_layout = cv.layout
          output_cell = cellname ? (output_layout.cell(cellname.to_s) || output_layout.create_cell(cellname.to_s)) : cv.cell
          cv.cell = output_cell
          output_layout_file = nil

        else

          cn = cellname && cellname.to_s
          cn ||= @def_cell && @def_cell.name
          cn ||= @def_source && @def_source.cell_obj && @def_source.cell_obj.name

          cn || raise("No cell name specified - either the source was not specified before 'report' or there is no default source. In the latter case, specify a cell name as the second parameter")

          output_layout = RBA::Layout::new
          output_cell = output_layout.create_cell(cn)
          output_layout_file = arg

        end
        
      elsif arg.is_a?(RBA::Layout)
      
        output_layout = arg

        output_cell = cellname && (output_layout.cell(cellname.to_s) || output_layout.create_cell(cellname.to_s))
        if ! output_cell
          begin
            output_cell = output_layout.top_cell
          rescue 
            raise("No cell name specified and the layout does not have a unique top cell - specify the name of the top cell to write the output to")
          end
        end

        output_layout_file = nil
        
      elsif arg.is_a?(RBA::Cell)
      
        output_layout = arg.layout
        output_cell = arg
        output_layout_file = nil

      else
        raise("Invalid argument '" + arg.inspect + "'")
      end
    
      output = LayoutOutputChannel::new(self, output_layout, output_cell, output_layout_file)

    end
    
  end
 
end

