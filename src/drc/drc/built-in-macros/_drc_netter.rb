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
      @name_prefix = "l"
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

        _register_layer(a.data, "connect")
        _register_layer(b.data, "connect")
        a.data.is_a?(RBA::Region) && @l2n.connect(a.data)
        b.data.is_a?(RBA::Region) && @l2n.connect(b.data)
        @l2n.connect(a.data, b.data)

      end

    end

    # %DRC%
    # @name soft_connect
    # @brief Specifies a soft connection between two layers
    # @synopsis soft_connect(a, b)
    # a and b must be polygon or text layers. After calling this function, the
    # Netter considers shapes from layer a and b connected in "soft mode".
    # Typically, b is a high-ohmic layer such as diffusion, implant for substate
    # material, also called the "lower" layer. 
    #
    # A soft connection between shapes from layer a and b forms a directional 
    # connection like an ideal diode: current can flow down, but now up 
    # (not meant in the physical sense, this is a concept). 
    # 
    # Hence, two nets are disconnected, if they both connect to the same lower layer, 
    # but do not have a connection between them. 
    #
    # The netlist extractor will use this scheme to identify nets that are 
    # connected only via such a high-ohmic region. Such a case is typically
    # bad for the functionality of a device and reported as an error.
    # Once, the check has been made and no error is found, soft-connected
    # nets are joined the same way than hard connections are made.
    #
    # Beside this, soft connections follow the same rules than hard connections
    # (see \connect).

    def soft_connect(a, b)

      @engine._context("soft_connect") do

        a.is_a?(DRC::DRCLayer) || raise("First argument must be a layer")
        b.is_a?(DRC::DRCLayer) || raise("Second argument must be a layer")
        a.requires_texts_or_region
        b.requires_texts_or_region

        _register_layer(a.data, "soft_connect")
        _register_layer(b.data, "soft_connect")
        # soft connections imply hard intra-layer connections
        a.data.is_a?(RBA::Region) && @l2n.connect(a.data)
        b.data.is_a?(RBA::Region) && @l2n.connect(b.data)
        @l2n.soft_connect(a.data, b.data)

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

        _register_layer(l.data, "connect_global")
        l.data.is_a?(RBA::Region) && @l2n.connect(l.data)
        @l2n.connect_global(l.data, name)

      end

    end
    
    # %DRC%
    # @name soft_connect_global
    # @brief Soft-connects a layer with a global net
    # @synopsis soft-connect_global(l, name)
    # Connects the shapes from the given layer l to a global net with the given name
    # in "soft mode".
    #
    # See \connect_global for details about the concepts of global nets.
    # See \soft_connect for details about the concept of soft connections.
    # In global net soft connections, the global net (typically a substrate)
    # is always the "lower" layer.
    
    def soft_connect_global(l, name)

      @engine._context("soft_connect_global") do

        l.is_a?(DRC::DRCLayer) || raise("Layer argument must be a layer")
        l.requires_texts_or_region

        _register_layer(l.data, "soft_connect_global")
        l.data.is_a?(RBA::Region) && @l2n.connect(l.data)
        @l2n.soft_connect_global(l.data, name)

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
          _register_layer(l.data, "extract_devices")
          ls[n.to_s] = l.data
        end

        @engine._cmd(@l2n, :extract_devices, devex, ls) 

      end

      devex.device_class

    end

    # %DRC%
    # @name name
    # @brief Assigns a name to a layer
    # @synopsis name(layer, name)
    # @synopsis name(layer, name, layer_number, datatype_number)
    # @synopsis name(layer, name, layer_info)
    # Layer names are listed in the LayoutToNetlist (L2N) or LVS database. They
    # are used to identify the layers, the net or device terminal geometries are
    # on. It is usual to have computed layers, so it is necessary to indicate the
    # purpose of the layer for later reuse of the geometries.
    #
    # It is a good practice to assign names to computed and original layers, 
    # for example:
    #
    # @code
    # poly = input(...)
    # poly_resistor = input(...)
    #
    # poly_wiring = poly - poly_resistor
    # name(poly_wiring, "poly_wiring")
    # @/code
    #
    # Names must be assigned before the layers are used for the first time
    # in \connect, \soft_connect, \connect_global, \soft_connect_global and 
    # \extract_devices statements. 
    # 
    # If layers are not named, they will be given a name made from the 
    # \name_prefix and an incremental number when the layer is used for the
    # first time. 
    #
    # \name can only be used once on a layer and the layer names must be 
    # unique (not taken by another layer).
    #
    # The layer/datatype or LayerInfo specification is optional and will
    # be used to configure the internal layout. This information is also
    # persisted inside database files. Specifying a layer/datatype information
    # is useful, if a layer is not an original layer, but is to be restored
    # to an actual layout layer later.

    # %DRC%
    # @name name_prefix
    # @brief Specifies the name prefix for auto-generated layer names
    # @synopsis name_prefix(prefix)
    # See \\name for details. The default prefix is "l".

    def name(*args)

      @engine._context("name") do

        if args.size < 2 || args.size > 4
          raise("Two to four arguments expected (layer, name, [layer, datatype / layer properties])")
        end

        l = args[0]
        l.is_a?(DRC::DRCLayer) || raise("First argument must be a layer")

        name = args[1]
        (name.is_a?(String) && name != "") || raise("Second argument must be a non-empty string")

        lp = args[2]
        if lp
          if !lp.is_a?(RBA::LayerInfo)
            lnum = args[2]
            dnum = args[3] || 0
            lnum.is_a?(1.class) || raise("Layer argument needs to be a RBA::LayerInfo object or a layer number")
            dnum.is_a?(1.class) || raise("Datatype argument needs to be an integer")
            lp = RBA::LayerInfo::new(lnum, dnum)
          elsif args[3]
            raise("Three arguments are enough with RBA::LayerInfo")
          end
        end

        id = l.data.data_id 

        if @layers && @layers[id]
          # already registered
          if @layers[id][1] != name
            raise("Layer already registered with name #{@layers[id][1]} in context: #{@layers[id][2]}")
          end
          return
        end
        
        self._register_layer(l.data, "name", name, lp)

      end

    end

    def name_prefix(prefix)
      @name_prefix = prefix.to_s
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
          arg1.is_a?(Array) || raise("The argument has to be an array of strings")
          arg1.find { |a| !a.is_a?(String) } && raise("The argument has to be an array of strings")
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
    # @name evaluate_nets
    # @brief Evaluates net properties and annotates shapes from a given layer on the nets
    # @synopsis evaluate_nets(primary_layer, secondary_layers, expression [, variables])
    #
    # The function takes a primary layer and a number of secondary layers, each of
    # them given a variable name.
    # It visits each net and evaluates the given expression on the net.
    # The expression needs to be written in KLayout expression notations.
    #
    # The default action is to copy the shapes of the primary layer to the
    # output. It is possible to customize the output further: you can 
    # conditionally skip the output or copy all shapes of the net from
    # all layers the output. You can choose to emit individual polygons
    # or merge all polygons from a net (all layers or a subset) into
    # a single polygon. The latter is the default.
    # 
    # You can also choose to emit the bounding box of the net if the number of polygons
    # on the net exceeds a certain limit.
    # 
    # Using the "put" function inside the expression, properties can be 
    # attached to the output shapes. The properties can be computed using
    # a number of net attributes - area and perimeter for example.
    #
    # Also the RBA::Net object representing the net is available through the
    # 'net' function. This allows implementing a more elaborate
    # antenna check for example.
    #
    # Arbitrary values can be passed as variables, which removes the need
    # to encode variable values into the expression. For this, use the 
    # 'variables' argument and pass a hash with names and values. Each of
    # those values becomes available as a variable with the given name
    # inside the expression.
    # 
    # The following functions are available inside the expressions:
    #
    # @ul
    # @li "net" - the RBA::Net object of the current net @/li
    # @li "db" - the RBA::LayoutToNetlist object the netlist lives in @/li
    # @li "skip" or "skip(flag)" - if called with a 'true' argument (the default), the primary layer's shapes are not copied for this net @/li
    # @li "copy(...)" - configures polygon output in a more elaborate way than "skip" (see below) @/li
    # @li "put(name, value)" - places the value as a property with name 'name' (this must be a string) on the output shapes @/li
    # @li "area" - the combined area of the primary layer's shapes on the net in square micrometer units @/li
    # @li "area(symbol)" - the combined area of the secondary layer's shapes on the net in square micrometer units @/li
    # @li "perimeter" - the perimeter of the primary layer's shapes on the net in micrometer units @/li
    # @li "perimeter(symbol)" - the perimeter of the secondary layer's shapes on the net in micrometer units @/li
    # @/ul
    #
    # Here, 'symbol' is the name given to the secondary layer in the secondary layer
    # dictionary.
    #
    # "copy" and "skip" control the polygon output. Here are the options:
    #
    # @ul
    # @li "skip" or "skip(true): skip output, identical to "copy(layers=[])" @/li
    # @li "skip(false)": copy the shapes from the primary layer, identical to "copy(layer=0)" @/li
    # @li "copy" or "copy(true)": copy all shapes from the net, merged into a single polygon.
    #     Note: this is not equivalent to "skip(false)", as in the latter case, only the primary layer's
    #     shapes are copied @/li
    # @li "copy(false)": equivalent to "skip(true)" @/li
    # @li "copy(merged=false)": copies all shapes from all layers of the net, without merging.
    #     "merged" is a keyword argument that can be combined with other arguments. @/li
    # @li "copy(limit=number)": if the net has less than "number" polygons on the selected layers, 
    #     copy them to the output. For more polygons, emit the bounding box of the net for the 
    #     given layers.
    #     "limit" is a keyword argument that can be combined with other arguments. @/li
    # @li "copy(layer=symbol)": copies all shapes from the layer denoted by the symbol.
    #     The primary layer has value zero (0), so "copy(layer=0)" copies the shapes from the primary layer.
    #     "layer" is a keyword argument that can be combined with other arguments, except "layers". @/li
    # @li "copy(layers=[symbol, symbol, ...])": copies all shapes from the layers denoted by the symbols.
    #     "layers" is a keyword argument that can be combined with other arguments, except "layer". @/li
    # @/ul
    #
    # When mixing "skip" and "copy", the last active specification controls the output. The following
    # expressions are equivalent:
    #
    # @code
    # copy(net.name == "VDD")
    # @/code
    #
    # and
    #
    # @code
    # skip ; net.name == "VDD" && copy
    # @/code
    # 
    # where the second expression establishes "skip" as the default and conditionally executes "copy", 
    # overriding "skip".
    #
    # @h4 Antenna check example @/h4
    #
    # The following example emulates an antenna check. It computes the area ratio of metal vs. gate area and 
    # attaches the value as a property with name 'AR' to the shapes, copied from the 'gate' layer:
    #
    # @code
    # gate = ...   # a layer with the gate shapes
    # metal = ...  # a layer with metal shapes
    # 
    # # NOTE: 'MET' is the name we are going to use in the expression
    # antenna_errors = evaluate_nets(gate, { "MET" => metal }, "put('AR',area(MET)/area)")
    # @/code
    #
    # This other example also computes the area ratio of metal vs. gate area, and 
    # outputs the gate shapes of all nets whose metal to gate area ratio is bigger than
    # 500. The area ratio is output to a property with name 'AR':
    #
    # @code
    # gate = ...   # a layer with the gate shapes
    # metal = ...  # a layer with metal shapes
    # 
    # variables = { "thr" => 500.0 }
    # expression = "var ar=area(MET)/area; put('AR',ar); skip(ar<thr)"
    #
    # antenna_errors = evaluate_nets(gate, { "MET" => metal }, expression, variables)
    # @/code
    #
    # NOTE: GDS does not support properties with string names, so 
    # either save to OASIS, or use integer numbers for the property names.

    def evaluate_nets(primary, secondary, expression, variables = {})

      primary.is_a?(DRC::DRCLayer) || raise("First argument must be a layer")
      primary.requires_region("'primary'")

      secondary_data = {}
      secondary.is_a?(Hash) || raise("Second argument must be a hash of names and layers")
      secondary.each do |k,v|
        v.is_a?(DRC::DRCLayer) || raise("Second argument must be a hash of names and polygon")
        v.requires_region("Secondary '#{k}'")
        secondary_data[k.to_s] = v.data
      end

      expression.is_a?(String) || raise("'expression' argument must be a string")

      DRC::DRCLayer::new(@engine, @engine._cmd(l2n_data, :evaluate_nets, primary.data, secondary_data, expression, variables, nil))

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

    def _l2n_object
      @l2n
    end

    def _make_soft_connection_diodes(f)
      @l2n.make_soft_connection_diodes = f
    end

  protected

    def _register_layer(data, context, name = nil, lp = nil)

      id = data.data_id 
      ensure_data

      if @layers && @layers[id]
        # already registered
        return
      end

      if !name
        @lnum += 1
        name = @name_prefix + @lnum.to_s
      end

      @layers[id] = [ data, name, context ]

      # every layer gets registered and intra-layer connections are made
      index = @l2n.register(data, name)

      # set the layer properties if requested
      if lp
        @l2n.internal_layout.set_info(index, lp)
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

  end

end

