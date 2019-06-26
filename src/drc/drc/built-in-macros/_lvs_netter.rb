# $autorun-early

module LVS

  include DRC

  # The LVS netter object

  # %LVS%
  # @scope
  # @name Netter
  # @brief LVS Reference: Netter object
  # The Netter object provides services related to network extraction
  # from a layout plus comparison against a reference netlist.
  # Similar to the DRC netter (which lacks the compare ability), the
  # relevant method of this object are available as global functions too
  # where they act on a default incarnation. Usually it's not required
  # to instantiate a Netter object explicitly. 
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
  # ...

  class LVSNetter < DRCNetter

    def initialize(engine)
      super
    end

    def make_data

      if @engine._dss
        @engine._dss.is_singular? || raise("The LVS script features more than one or no layout source - network extraction cannot be performed in such configurations")
        @lvs = RBA::LayoutVsSchematic::new(@engine._dss)
      else
        layout = @engine.source.layout
        @lvs = RBA::LayoutVsSchematic::new(layout.top_cell.name, layout.dbu)
      end

      @l2n = @lvs
      @comparer = RBA::NetlistComparer::new

    end

    def lvs_data
      l2n_data
      @lvs
    end

    def _clear_data
      super
      @lvs = nil
    end

    def _take_data
      data = super
      @lvs = nil
      data
    end

    def compare
      @lvs.compare(@comparer)
    end

    def _ensure_two_netlists

      netlist || raise("No netlist present (not extracted?)")
      lvs_data.reference || raise("No reference schematic present (no set with 'schematic'?)")

      [ netlist, lvs_data.reference ]

    end
      
    def same_nets(*args)

      pins.each do |a|
        a.is_a?(String) || raise("All arguments of 'same_nets' need to be strings")
      end
      if args.size < 3 
        raise("Too few arguments to 'same_nets' (need at least 3)")
      end
      if args.size > 4 
        raise("Too many arguments to 'same_nets' (need max 4)")
      end

      if args.size == 3
        ( ca, a, b ) = args
        cb = ca
      else
        ( ca, a, cb, b ) = args
      end

      ( nl_a, nl_b ) = _ensure_two_netlists

      circuit_a = nl_a.circuit_by_name(ca) || raise("Not a valid circuit name in extracted netlist: #{ca}")
      circuit_b = nl_b.circuit_by_name(cb) || raise("Not a valid circuit name in reference netlist: #{cb}")

      net_a = circuit_a.net_by_name(a) || raise("Not a valid net name in extracted netlist: #{a} (for circuit #{circuit_a})")
      net_b = circuit_b.net_by_name(b) || raise("Not a valid net name in reference netlist: #{b} (for circuit #{circuit_b})")

      @comparer.same_nets(net_a, net_b)
      
    end

    def same_circuits(a, b)

      a.is_a?(String) || b.is_a?(String) || raise("Both arguments of 'same_circuits' need to be strings")

      ( nl_a, nl_b ) = _ensure_two_netlists

      circuit_a = nl_a.circuit_by_name(a) || raise("Not a valid circuit name in extracted netlist: #{a}")
      circuit_b = nl_b.circuit_by_name(b) || raise("Not a valid circuit name in reference netlist: #{b}")

      @comparer.same_circuits(circuit_a, circuit_b)
      
    end

    def same_device_classes(a, b)

      a.is_a?(String) || b.is_a?(String) || raise("Both arguments of 'same_device_classes' need to be strings")

      ( nl_a, nl_b ) = _ensure_two_netlists

      dc_a = nl_a.device_class_by_name(a) || raise("Not a valid device class in extracted netlist: #{a}")
      dc_b = nl_b.device_class_by_name(b) || raise("Not a valid device class in reference netlist: #{b}")

      @comparer.same_device_classes(dc_a, dc_b)
      
    end

    def equivalent_pins(circuit, *pins)

      circuit.is_a?(String) || raise("Circuit arguments of 'equivalent_pins' needs to be a string")
      pins.each do |a|
        a.is_a?(String) || raise("All pin arguments of 'equivalent_pins' need to be strings")
      end

      ( nl_a, nl_b ) = _ensure_two_netlists

      circuit_b = nl_b.circuit_by_name(circuit) || raise("Not a valid circuit name in reference netlist: #{circuit}")

      pin_ids_b = pins.collect do |p|
        pin = circuit_b.pin_by_name(p) || raise("Not a valid pin name in circuit '#{circuit}': #{p}")
        pin.id
      end

      @comparer.equivalent_pins(circuit, pin_ids_b)
      
    end

    def schematic(filename, reader = nil)

      filename.is_a?(String) || raise("First argument must be string in 'schematic'")

      if reader
        reader.is_a?(RBA::NetlistReader) || raise("Second argument must be netlist reader object in 'schematic'")
      else
        reader = RBA::NetlistSpiceReader::new
      end

      netlist_file = @engine._make_path(filename)
      @engine.info("Reading netlist: #{netlist_file} ..")

      netlist = RBA::Netlist::new
      netlist.read(netlist_file, reader)

      lvs_data.reference = netlist 

    end

    def min_caps(value)
      lvs_data
      @comparer.min_capacitance = value.to_f
    end
      
    def max_res(value)
      lvs_data
      @comparer.max_resistance = value.to_f
    end

    def max_depth(value)
      lvs_data
      @comparer.max_depth = value.to_i
    end

    def max_branch_complexity(value)
      lvs_data
      @comparer.max_branch_complexity = value.to_i
    end

  end
  
end

