# $autorun-early

module DRC

  # The netter object

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
  # An individual netter object can be created, if the netter results
  # need to be kept for multiple extractions. If you really need
  # a Netter object, use the global \netter function:
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
  # A basic Service the Netter object provides is the formation of 
  # connected networks of conductive shapes. To do so, the Netter
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
      clear_connections
    end
    
    # %DRC%
    # @name connect
    # @brief Specifies a connection between two layers
    # @synopsis connect(a, b)
    # a and b must be polygon layers. After calling this function, the
    # Netter regards all overlapping or touching shapes on these layers
    # to form an electrical connection between the materials formed by
    # these layers. This also implies intra-layer connections: shapes
    # on these layers touching or overlapping other shapes on these
    # layers will form bigger, electrically connected areas.
    #
    # Multiple connect calls must be made to form larger connectivity
    # stacks across multiple layers. Such stacks may include forks and
    # joins.
    #
    # Connections are accumulated. The connections defined so far
    # can be cleared with \clear_connections.

    def connect(a, b)
      a.is_a?(DRC::DRCLayer) || raise("First argument of Netter#connect must be a layer")
      b.is_a?(DRC::DRCLayer) || raise("Second argument of Netter#connect must be a layer")
      a.requires_region("Netter#connect (first argument)")
      b.requires_region("Netter#connect (second argument)")
      [ a, b ].each { |l| @layers[l.data.data_id] = l.data }
      @connections << [ a, b ].collect { |l| l.data.data_id }
      modified
    end

    # %DRC%
    # @name connect_global
    # @brief Connects a layer with a global net
    # @synopsis connect_global(l, name)
    # Connects the shapes from the given layer l to a global net with the given name.
    # Global nets are common to all cells. Global nets automatically connect to parent
    # cells throughs implied pins. An example is the substrate (bulk) net which connects
    # to shapes belonging to tie-down diodes.
    
    def connect_global(l, name)
      l.is_a?(DRC::DRCLayer) || raise("Layer argument of Netter#connect_global must be a layer")
      l.requires_region("Netter#connect_global (layer argument)")
      @layers[l.data.data_id] = l.data
      @global_connections << [ l.data.data_id, name.to_s ]
    end
    
    # %DRC%
    # @name extract_devices
    # @brief Extracts devices based on the given extractor class, name and device layer selection
    # @synopsis extract_devices(extractor, layer_hash)
    # Runs the device extraction for given device extractor class.
    # 
    # The device extractor is either an instance of one of the predefined extractor
    # classes (e.g. RBA::DeviceExtractorMOS4Transistor) or a custom class. It provides the
    # algorithms for deriving the device parameters from the device geometry. It needs 
    # several device recognition layers which are passed in the layer hash.
    #
    # Each device class (e.g. n-MOS/p-MOS or high Vt/low Vt) needs it's own instance
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
    # nactive = active - nwell      # active area of NMOS
    # nsd     = nactive - poly      # source/drain area
    # gate    = nactive & poly  # gate area
    #
    # mos4_ex = RBA::DeviceExtractorMOS4Transistor::new("NMOS4")
    # extract_devices(mos4_ex, { :SD => nsd, :G => gate, :P => poly, :W => bulk })
    # @/code
    
    def extract_devices(devex, layer_selection)

      devex.is_a?(RBA::DeviceExtractorBase) || raise("First argument of Netter#extract_devices must be a device extractor instance")
      layer_selection.is_a?(Hash) || raise("Second argument of Netter#extract_devices must be a hash")

      ls = {}
      layer_selection.each do |n,l|
        l.requires_region("Netter#extract_devices (#{n} layer)")
        @layers[l.data.data_id] = l.data
        ls[n.to_s] = l.data
      end

      @devices_to_extract << [ devex, ls ]
      modified

    end
    
    # %DRC%
    # @name clear_connections
    # @brief Clears all connections stored so far
    # @synopsis clear_connections
    # See \connect for more details.

    def clear_connections
      @devices_to_extract = []
      @connections = []
      @global_connections = []
      @layers = {}
      @join_nets = ""
      modified
    end
    
    # %DRC%
    # @name join_nets
    # @brief Specifies a search pattern for labels which create implicit net connections
    # @synopsis join_nets(label_pattern)
    # Use this method to supply a glob pattern for labels which create implicit net connections
    # on the top level circuit. This feature is useful to connect identically labelled nets
    # while a component isn't integrated yet. If the component is integrated, net may be connected
    # on a higher hierarchy level - e.g. by a power mesh. Inside the component this net consists
    # of individual islands. To properly perform netlist extraction and comparison, these islands
    # need to be connected even though there isn't a physical connection. "join_nets" can
    # achive this if these islands are labelled with the same text on the top level of the
    # component.
    #
    # Glob pattern are used which resemble shell file pattern: "*" is for all labels, "VDD"
    # for all "VDD" labels (pattern act case sensitive). "VDD*" is for all labels beginning
    # with "VDD" (still different labels will be connected to different nets!). "{VDD,VSS}"
    # is either "VDD" or "VSS".
    #
    # The search pattern is applied on the next net extraction. The search pattern is cleared
    # on "clear_connections".

    def join_nets(arg)
      @join_nets = arg
      modified
    end

    # %DRC%
    # @brief Performs an antenna check
    # @name antenna_check
    # @synopsis antenna_check(gate, metal, ratio, [ diode_specs ... ])
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
    # The error shapes produced by the antenna check are copies
    # of the metal shapes on the metal layers of each network 
    # violating the antenna rule.

    def antenna_check(gate, metal, ratio, *diodes)

      gate.is_a?(DRC::DRCLayer) || raise("gate argument of Netter#antenna_check must be a layer")
      gate.requires_region("Netter#antenna_check (gate argument)")

      metal.is_a?(DRC::DRCLayer) || raise("metal argument of Netter#antenna_check must be a layer")
      metal.requires_region("Netter#antenna_check (metal argument)")

      if !ratio.is_a?(1.class) && !ratio.is_a?(Float)
        raise("ratio argument Netter#antenna_check is not a number")
      end

      dl = diodes.collect do |d|
        if d.is_a?(Array)
          d.size == 2 || raise("diode specification pair expects two elements")
          d[0].requires_region("Netter#antenna_check (diode layer)")
          [ d[0].data, d[1].to_f ]
        else 
          d.requires_region("Netter#antenna_check (diode layer)")
          [ d.data, 0.0 ]
        end
      end

      @l2n || make_l2n
      DRC::DRCLayer::new(@engine, @engine._cmd(@l2n, :antenna_check, gate.data, metal.data, ratio, dl))

    end

    # %DRC%
    # @name l2n_data
    # @brief Gets the internal RBA::LayoutToNetlist object
    # @synopsis l2n_data
    # The RBA::LayoutToNetlist object provides access to the internal details of
    # the netter object.

    def l2n_data
      @l2n || make_l2n
      @l2n
    end
    
    def _finish
      clear_connections
      # cleans up the L2N object
      modified
    end

  private

    def modified
      @l2n && @l2n._destroy
      @l2n = nil
    end
    
    def make_l2n

      if @engine._dss
        # TODO: check whether all layers are deep and come from the dss and layout index,
        # then use this layout index. This will remove the need for this check:
        @engine._dss.is_singular? || raise("The DRC script features more than one or no layout source - network extraction cannot be performed in such configurations")
        @l2n = RBA::LayoutToNetlist::new(@engine._dss)
      else
        layout = @engine.source.layout
        @l2n = RBA::LayoutToNetlist::new(layout.top_cell.name, layout.dbu)
      end

      @layers.each { |id,l| @l2n.register(l, "l" + id.to_s) }

      @devices_to_extract.each do |devex,ls| 
        @engine._cmd(@l2n, :extract_devices, devex, ls) 
      end

      @layers.each { |id,l| @l2n.connect(l) }
      @connections.each { |a,b| @l2n.connect(@layers[a], @layers[b]) }
      @global_connections.each { |l,n| @l2n.connect_global(@layers[l], n) }

      # run extraction in a timed environment
      @engine._cmd(@l2n, :extract_netlist, @join_nets)
      @l2n

    end
    
  end

end

