# $autorun-early

module DRC

  # The DRC netter object

  # %DRC%
  # @scope 
  # @name Netter 
  # @brief DRC Reference: Netter object
  # The Netter object provides services related to network extraction
  # from a layout. The relevant methods of this object are available
  # as global functions too where they act on a default incarnation
  # of the netter. Usually it's not required to instantiate a Netter
  # object, but it serves as a container for this functionality.
  #
  # @code
  # # create a new Netter object:
  # nx = netter
  # nx.connect(poly, contact)
  # ...
  # @/code
  #
  # Network formation:
  # 
  # A basic service the Netter object provides is the formation of 
  # connected networks of conductive shapes (netting). To do so, the Netter
  # must be given a connection specification. This happens by calling
  # "connect" with two polygon layers. The Netter will then regard all
  # overlaps of shapes on these layers as connections between the
  # respective materials. Networks are the basis for netlist extraction,
  # network geometry deduction and the antenna check.
  #
  # Connections can be cleared with "clear_connections". If not, 
  # connections add atop of the already defined ones. Here is an 
  # example for the antenna check:
  # 
  # @code
  # # build connction of poly+gate to metal1
  # connect(gate, poly)
  # connect(poly, contact)
  # connect(contact, metal1)
  #
  # # runs an antenna check for metal1 with a ratio of 50
  # m1_antenna_errors = antenna_check(gate, metal1, 50.0)
  #  
  # # add connections to metal2
  # connect(metal1, via1)
  # connect(via1, metal2)
  #
  # # runs an antenna check for metal2 with a ratio of 70.0
  # m2_antenna_errors = antenna_check(gate, metal2, 70.0)
  # 
  # # this will remove all connections made
  # clear_connections
  # ...
  # @/code
  #
  # Further functionality of the Netter object:
  #
  # More methods will be added in the future to support network-related features.

  class DRCNetter

    def initialize(engine)
      @engine = engine
      @pre_extract_config = []
      @post_extract_config = []
      @l2n = nil
      @lnum = 0
      @device_scaling = 1.0
      @ignore_extraction_errors = false
      @top_level = false
    end
    
    # %DRC%
    # @name connect
    # @brief Specifies a connection between two layers
    # @synopsis connect(a, b)
    # a and b must be polygon or text layers. After calling this function, the
    # Netter regards all overlapping or touching shapes on these layers
    # to form an electrical connection between the materials formed by
    # these layers. This also implies intra-layer connections: shapes
    # on these layers touching or overlapping other shapes on these
    # layers will form bigger, electrically connected areas.
    #
    # Texts will be used to assign net names to the nets. The preferred
    # method is to use \global#labels to create a text layer from a design 
    # layer. When using \global#input, text labels are carried implicitly
    # with the polygons but at the cost of small dummy shapes (2x2 DBU
    # marker polygons) and limited functionality.
    #
    # Multiple connect calls must be made to form larger connectivity
    # stacks across multiple layers. Such stacks may include forks and
    # joins.
    #
    # Connections are accumulated. The connections defined so far
    # can be cleared with \clear_connections.

    def connect(a, b)

      @engine._context("connect") do

        a.is_a?(DRC::DRCLayer) || raise("First argument must be a layer")
        b.is_a?(DRC::DRCLayer) || raise("Second argument must be a layer")
        a.requires_texts_or_region
        b.requires_texts_or_region

        register_layer(a.data)
        register_layer(b.data)
        a.data.is_a?(RBA::Region) && @l2n.connect(a.data)
        b.data.is_a?(RBA::Region) && @l2n.connect(b.data)
        @l2n.connect(a.data, b.data)

      end

    end

    # %DRC%
    # @name connect_global
    # @brief Connects a layer with a global net
    # @synopsis connect_global(l, name)
    # Connects the shapes from the given layer l to a global net with the given name.
    # Global nets are common to all cells. Global nets automatically connect to parent
    # cells throughs implied pins. An example is the substrate (bulk) net which connects
    # to shapes belonging to tie-down diodes. "l" can be a polygon or text layer.
    
    def connect_global(l, name)

      @engine._context("connect_global") do

        l.is_a?(DRC::DRCLayer) || raise("Layer argument must be a layer")
        l.requires_texts_or_region

        register_layer(l.data)
        l.data.is_a?(RBA::Region) && @l2n.connect(l.data)
        @l2n.connect_global(l.data, name)

      end

    end
    
    # %DRC%
    # @name extract_devices
    # @brief Extracts devices based on the given extractor class, name and device layer selection
    # @synopsis extract_devices(extractor, layer_hash)
    # @synopsis extract_devices(extractor_class, name, layer_hash)
    # Runs the device extraction for given device extractor class. In the first
    # form, the extractor object is given. In the second form, the extractor's
    # class object and the new extractor's name is given.
    # 
    # The device extractor is either an instance of one of the predefined extractor
    # classes (e.g. obtained from the utility methods such as \global#mos4) or a custom class. 
    # It provides the
    # algorithms for deriving the device parameters from the device geometry. It needs 
    # several device recognition layers which are passed in the layer hash.
    #
    # Predefined device extractors are:
    #
    # @ul
    # @li \global#mos3 - A three-terminal MOS transistor @/li
    # @li \global#mos4 - A four-terminal MOS transistor @/li
    # @li \global#dmos3 - A three-terminal MOS asymmetric transistor @/li
    # @li \global#dmos4 - A four-terminal MOS asymmetric transistor @/li
    # @li \global#bjt3 - A three-terminal bipolar transistor @/li
    # @li \global#bjt4 - A four-terminal bipolar transistor @/li
    # @li \global#diode - A planar diode @/li
    # @li \global#resistor - A resistor @/li
    # @li \global#resistor_with_bulk - A resistor with a separate bulk terminal @/li
    # @li \global#capacitor - A capacitor @/li
    # @li \global#capacitor_with_bulk - A capacitor with a separate bulk terminal @/li
    # @/ul
    #
    # Each device class (e.g. n-MOS/p-MOS or high Vt/low Vt) needs its own instance
    # of device extractor. The device extractor beside the algorithm and specific 
    # extraction settings defines the name of the device to be built.
    #
    # The layer hash is a map of device type specific functional names (key) and
    # polygon layers (value). Here is an example:
    #
    # @code
    # deep
    # 
    # nwell   = input(1, 0)
    # active  = input(2, 0)
    # poly    = input(3, 0)
    # bulk    = make_layer   # renders an empty layer used for putting the terminals on
    #
    # nactive = active - nwell  # active area of NMOS
    # nsd     = nactive - poly  # source/drain area
    # gate    = nactive & poly  # gate area
    #
    # extract_devices(mos4("NMOS4"), { :SD => nsd, :G => gate, :P => poly, :W => bulk })
    # @/code
    #
    # The return value of this method will be the device class of the devices
    # generated in the extraction step (see RBA::DeviceClass).
    
    def extract_devices(devex, layer_selection)
    
      @engine._context("extract_devices") do

        ensure_data

        devex.is_a?(RBA::DeviceExtractorBase) || raise("First argument of must be a device extractor instance in the two-arguments form")

        layer_selection.is_a?(Hash) || raise("Second argument must be a hash")

        ls = {}
        layer_selection.keys.sort.each do |n|
          l = layer_selection[n]
          l.requires_texts_or_region
          register_layer(l.data)
          ls[n.to_s] = l.data
        end

        @engine._cmd(@l2n, :extract_devices, devex, ls) 

      end

      devex.device_class

    end

    # %DRC%
    # @name device_scaling
    # @brief Specifies a dimension scale factor for the geometrical device properties
    # @synopsis device_scaling(factor)
    # Specifying a factor of 2 will make all devices being extracted as if the 
    # geometries were two times larger. This feature is useful when the drawn layout
    # does not correspond to the physical dimensions.
    
    def device_scaling(factor)
      @engine._context("device_scaling") do
        @device_scaling = factor
        @l2n && @l2n.device_scaling = factor
      end
    end

    # %DRC%
    # @name top_level
    # @brief Specifies top level mode
    # @synopsis top_level(value)
    # With this value set to false (the default), it is assumed that the 
    # circuit is not used as a top level chip circuit. In that case, for
    # example must-connect nets which are not connected are reported as
    # as warnings. If top level mode is set to true, such disconnected
    # nets are reported as errors as this indicates a missing physical
    # connection.
    
    def top_level(value)
      @engine._context("top_level") do
        @top_level = value
        @l2n && @l2n.top_level_mode = value
      end
    end
    
    # %DRC%
    # @name ignore_extraction_errors
    # @brief Specifies whether to ignore extraction errors
    # @synopsis ignore_extraction_errors(value)
    # With this value set to false (the default), "extract_netlist" will raise
    # an exception upon extraction errors. Otherwise, extraction errors will be logged
    # but no error is raised.
    
    def ignore_extraction_errors(value)
      @ignore_extraction_errors = value
    end
    
    # %DRC%
    # @name clear_connections
    # @brief Clears all connections stored so far
    # @synopsis clear_connections
    # See \connect for more details.

    def clear_connections
      @pre_extract_config = []
      @post_extract_config = []
      _clear_data
    end
    
    # %DRC%
    # @name connect_implicit
    # @brief Specifies a search pattern for implicit net connections ("must connect" nets)
    # @synopsis connect_implicit(label_pattern)
    # @synopsis connect_implicit(cell_pattern, label_pattern)
    # This method specifies a net name search pattern, either for all cells or for
    # certain cells, given by a name search pattern. Search pattern follow the usual glob
    # form (e.g. "A*" for all cells or nets with names starting with "A").
    # 
    # Then, for nets matching the net name pattern and for which there is more than
    # one subnet, the subnets are connected. "Subnets" are physically disconnected parts
    # of a net which carry the same name.
    #
    # This feature is useful for example for power nets which are complete in a cell,
    # but are supposed to be connected upwards in the hierarchy ("must connect" nets).
    # Physically there are multiple nets, logically - and specifically in the schematic for
    # the purpose of LVS - there is only one net. 
    # "connect_implicit" now creates a virtual, combined physical net that matches the logical net.
    # 
    # This is general bears the risk of missing a physical connection. The "connect_implicit"
    # feature therefore checks whether the connection is made physically on the next hierarchy
    # level, except for top-level cells for which it is assumed that this connection is made
    # later. A warning is raised instead for top level cells.
    #
    # The implicit connections are applied on the next net extraction and cleared
    # on "clear_connections". Another feature is \connect_explicit which allows connecting
    # differently named subnets in a similar fashion.

    def connect_implicit(arg1, arg2 = nil)

      @engine._context("connect_implicit") do

        cleanup
        if arg2
          (arg2.is_a?(String) && arg2 != "") || raise("The second argument has to be a non-empty string")
          arg1.is_a?(String) || raise("The first argument has to be a string")
          @pre_extract_config << lambda { |l2n| l2n.join_net_names(arg1, arg2) }
        else
          arg1.is_a?(String) || raise("The argument has to be a string")
          @pre_extract_config << lambda { |l2n| l2n.join_net_names(arg1) }
        end

      end

    end

    # %DRC%
    # @name connect_explicit
    # @brief Specifies a list of net names for nets to connect ("must connect" nets)
    # @synopsis connect_explicit(net_names)
    # @synopsis connect_explicit(cell_pattern, net_names)
    # Use this method to explicitly connect nets even if there is no physical connection.
    # The concept is similar to implicit connection (see \connect_implicit). The method gets
    # a list of nets which are connected virtually, even if there is no physical connection.
    # The first version applies this scheme to all cells, the second version to cells matching
    # the cell name pattern. The cell name pattern follows the usual glob style form (e.g. "A*"
    # applies the connection in all cells whose name starts with "A").
    #
    # This method is useful to establish a logical connection which is made later up on the 
    # next level of hierarchy. For example, a standard cell my not contain substrate or well
    # taps as these may be made by tap or spare cells. Logically however, the cell only has
    # one power or ground pin for the devices and substrate or well. In order to match both
    # representations - for example for the purpose of LVS - the dual power or ground pins have
    # to be connected. Assuming that there is a global net "BULK" for the substrate and a
    # net "VSS" for the sources of the NMOS devices, the following statement will create this
    # connection for all cell names beginning with "INV":
    #
    # @code
    # connect_global(bulk, "BULK")
    # ...
    # connect_explicit("INV*", [ "BULK", "VSS" ])
    # @/code
    #
    # The resulting net and pin will carry a name made from the combination of the connected
    # nets. In this case it will be "BULK,VSS".
    #
    # The virtual connection in general bears the risk of missing a physical connection. 
    # The "connect_explicit" feature therefore checks whether the connection is made physically 
    # on the next hierarchy level ("must connect" nets), except for top-level cells for which 
    # it is assumed that this connection is made later. 
    # A warning is raised instead for top level cells.
    # 
    # Explicit connections also imply implicit connections between different parts of 
    # one of the nets. In the example before, "VSS" pieces without a physical connection
    # will also be connected.
    #
    # The explicit connections are applied on the next net extraction and cleared
    # on "clear_connections".

    def connect_explicit(arg1, arg2 = nil)

      @engine._context("connect_explicit") do

        cleanup
        if arg2
          arg2.is_a?(Array) || raise("The second argument has to be an array of strings")
          arg2.find { |a| !a.is_a?(String) } && raise("The second argument has to be an array of strings")
          arg1.is_a?(String) || raise("The first argument has to be a string")
          @pre_extract_config << lambda { |l2n| l2n.join_nets(arg1, arg2) }
        else
          arg1.is_a?(String) || raise("The argument has to be a string")
          @pre_extract_config << lambda { |l2n| l2n.join_nets(arg1) }
        end

      end

    end

    # %DRC%
    # @brief Performs an antenna check
    # @name antenna_check
    # @synopsis antenna_check(gate, metal, ratio, [ diode_specs ... ] [, texts ])
    #
    # The antenna check is used to avoid plasma induced damage. Physically, 
    # the damage happes if during the manufacturing of a metal layer with
    # plasma etching charge accumulates on the metal islands. On reaching a
    # certain threshold, this charge may discarge over gate oxide attached of 
    # devices attached to such metal areas hence damaging it.
    #
    # Antenna checks are performed by collecting all connected nets up to
    # a certain metal layer and then computing the area of all metal shapes
    # and all connected gates of a certain kind (e.g. thin and thick oxide gates).
    # The ratio of metal area divided by the gate area must not exceed a certain
    # threshold.
    #
    # A simple antenna check is this:
    #
    # @code
    # poly = ... # poly layer
    # diff = ... # diffusion layer
    # contact = ... # contact layer
    # metal1 = ... # metal layer
    # 
    # # compute gate area
    # gate = poly & diff
    #
    # # note that gate and poly have to be included - gate is
    # # a subset of poly, but forms the sensitive area
    # connect(gate, poly)
    # connect(poly, contact)
    # connect(contact, metal1)
    # errors = antenna_check(gate, metal1, 50.0)
    # @/code
    #
    # Usually antenna checks apply to multiple metal layers. In this case,
    # the connectivity needs to be extended after the first check to include
    # the next metal layers. This can be achieved with incremental connects:
    #
    # @code
    # # provide connections up to metal1
    # connect(gate, poly)
    # connect(poly, contact)
    # connect(contact, metal1)
    # metal1_errors = antenna_check(gate, metal1, 50.0)
    #
    # # now *add* connections up to metal2
    # connect(metal1, via1)
    # connect(via1, metal2)
    # metal2_errors = antenna_check(gate, metal2, 50.0)
    #
    # ... continue this scheme with further metal layers ...
    # @/code 
    #
    # Plasma induced damage can be rectified by including diodes
    # which create a safe current path for discharging the metal
    # islands. Such diodes can be identified with a recognition layer
    # (usually the diffusion area of a certain kind). You can include
    # such diode recognition layers in the antenna check. If a connection
    # is detected to a diode, the respective network is skipped:
    #
    # @code
    # ...
    # diode = ... # diode recognition layer
    #
    # connect(diode, contact)
    # errors = antenna_check(gate, metal1, 50.0, diode)
    # @/code
    #
    # You can also make diode connections decreases the
    # sensitivity of the antenna check depending on the size
    # of the diode. The following specification makes 
    # diode connections increase the ratio threshold by
    # 10 per square micrometer of diode area:
    #
    # @code
    # ...
    # diode = ... # diode recognition layer
    #
    # connect(diode, contact)
    # # each square micrometer of diode area connected to a network
    # # will add 10 to the ratio:
    # errors = antenna_check(gate, metal1, 50.0, [ diode, 10.0 ])
    # @/code
    #
    # Multiple diode specifications are allowed. Just add them 
    # to the antenna_check call.
    #
    # You can include the perimeter into the area computation for
    # the gate or metal layer or both. The physical picture
    # is this: the side walls of the material contribute to the 
    # surface too. As the side wall area can be estimated by taking
    # the perimeter times some material thickness, the effective 
    # area is: 
    #
    # @code
    # A(eff) = A + P * t
    # @/code
    #
    # Here A is the area of the polygons and P is their perimeter.
    # t is the "thickness" in micrometer units. To specify such
    # a condition, use the following notation:
    #
    # @code
    # errors = antenna_check(area_and_perimeter(gate, 0.5), ...)
    # @/code
    #
    # "area_and_perimeter" takes the polygon layer and the 
    # thickness (0.5 micrometers in this case). 
    # This notation can be applied to both gate and
    # metal layers. A detailed notation for the usual,
    # area-only case is available as well for completeness:
    #
    # @code
    # errors = antenna_check(area_only(gate), ...)
    # 
    # # this is equivalent to a zero thickness:
    # errors = antenna_check(area_and_perimeter(gate, 0.0), ...)
    # # or the standard case:
    # errors = antenna_check(gate, ...)
    # @/code
    #
    # Finally there is also "perimeter_only". When using this 
    # specification with a thickness value, the area is computed
    # from the perimeter alone:
    #
    # @code
    # A(eff) = P * t
    # @/code
    #
    # @code
    # errors = antenna_check(perimeter_only(gate, 0.5), ...)
    # @/code
    #
    # The error shapes produced by the antenna check are copies
    # of the metal shapes on the metal layers of each network 
    # violating the antenna rule.
    # 
    # You can specify a text layer (use "labels" to create one). It will receive
    # error labels describing the measured values and computation parameters for debugging
    # the layout. This option has been introduced in version 0.27.11.
    # 

    def antenna_check(agate, ametal, ratio, *args)

      @engine._context("antenna_check") do

        gate_perimeter_factor = 0.0
        gate_area_factor = 1.0
        if agate.is_a?(DRC::DRCLayer)
          gate = agate
        elsif agate.is_a?(DRC::DRCAreaAndPerimeter)
          gate = agate.region
          gate_perimeter_factor = agate.perimeter_factor
          gate_area_factor = agate.area_factor
          if ! gate.is_a?(DRC::DRCLayer)
            raise("Gate with area or area_and_perimeter: input argument must be a layer")
          end
        else
          raise("Gate argument must be a layer ")
        end

        gate.requires_region

        metal_perimeter_factor = 0.0
        metal_area_factor = 1.0
        if ametal.is_a?(DRC::DRCLayer)
          metal = ametal
        elsif ametal.is_a?(DRC::DRCAreaAndPerimeter)
          metal = ametal.region
          metal_perimeter_factor = ametal.perimeter_factor
          metal_area_factor = ametal.area_factor
          if ! metal.is_a?(DRC::DRCLayer)
            raise("Metal with area or area_and_perimeter: input argument must be a layer")
          end
        else
          raise("Metal argument must be a layer")
        end

        metal.requires_region

        if !ratio.is_a?(1.class) && !ratio.is_a?(Float)
          raise("Ratio argument is not a number")
        end

        dl = []
        texts = nil
        n = 3
        args.each do |a|
          if a.is_a?(Array)
            a.size == 2 || raise("Diode specification pair expects two elements for argument #{n + 1}")
            if ! a[0].is_a?(DRC::DRCLayer)
              raise("Diode specification pair needs a layer for the first argument of argument #{n + 1}")
            end
            a[0].requires_region
            dl << [ a[0].data, a[1].to_f ]
          elsif ! a.is_a?(DRC::DRCLayer)
            raise("Argument #{n + 1} has to be a layer")
          else
            a.requires_texts_or_region
            if a.data.is_a?(RBA::Region)
              dl << [ a.data, 0.0 ]
            else
              texts = a.data
            end
          end
          n += 1
        end

        DRC::DRCLayer::new(@engine, @engine._cmd(l2n_data, :antenna_check, gate.data, gate_area_factor, gate_perimeter_factor, metal.data, metal_area_factor, metal_perimeter_factor, ratio, dl, texts))

      end

    end

    # %DRC%
    # @name l2n_data
    # @brief Gets the internal RBA::LayoutToNetlist object
    # @synopsis l2n_data
    # The RBA::LayoutToNetlist object provides access to the internal details of
    # the netter object.

    def l2n_data

      @engine._context("l2n_data") do

        ensure_data

        # run extraction in a timed environment
        if ! @l2n.is_extracted?

          @l2n.clear_join_net_names
          @l2n.clear_join_nets

          # configure the netter
          @pre_extract_config.each do |cfg|
            cfg.call(@l2n)
          end

          @engine._cmd(@l2n, :extract_netlist)

          # configure the netter, post-extraction
          @post_extract_config.each do |cfg|
            cfg.call(@l2n)
          end

          # checks for errors if needed
          if !@ignore_extraction_errors
            @l2n.check_extraction_errors
          end

        end

        @l2n

      end

    end

    # %DRC%
    # @name netlist
    # @brief Gets the extracted netlist or triggers extraction if not done yet
    # @synopsis netlist
    # If no extraction has been performed yet, this method will start the 
    # layout analysis. Hence, all \connect, \connect_global and \connect_implicit
    # calls must have been made before this method is used. Further \connect
    # statements will clear the netlist and re-extract it again.
    def netlist
      @engine._context("netlist") do
        l2n_data && @l2n.netlist
      end
    end
    
    def _finish
      clear_connections
    end

    def _clear_data
      @l2n && @l2n._destroy
      @l2n = nil
    end

    def _take_data
      l2ndb = self.l2n_data
      @l2n = nil
      l2ndb
    end

    def _l2n_data
      @l2n && @l2n.is_extracted? && self.l2n_data
    end

  private

    def cleanup
      @l2n && @l2n.is_extracted? && clear_connections
    end
    
    def ensure_data
      if !@l2n
        @layers = {}
        _make_data
        @l2n.device_scaling = @device_scaling
        @l2n.top_level_mode = @top_level
      end
    end

    def _make_data

      if @engine._dss
        @engine._dss.is_singular? || raise("The DRC script features more than one or no layout source - network extraction cannot be performed in such configurations")
        @l2n = RBA::LayoutToNetlist::new(@engine._dss)
      else
        layout = @engine.source.layout
        cell_name = @engine.source.cell_name
        @l2n = RBA::LayoutToNetlist::new(cell_name, layout.dbu)
      end

      @l2n.name = "DRC"
      @l2n.generator = @engine._generator

    end

    def register_layer(data)

      id = data.data_id 
      ensure_data

      if @layers && @layers[id]
        # already registered
        return
      end

      @layers[id] = data
      @lnum += 1

      # every layer gets registered and intra-layer connections are made
      @l2n.register(data, "l" + @lnum.to_s)

    end
    
  end

end

