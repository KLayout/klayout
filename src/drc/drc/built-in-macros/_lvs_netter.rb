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
  # need to be kept for multiple extractions or when different configurations
  # need to be used in the same script. If you really want
  # a Netter object, use the global \netter function:
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
  #
  # # build connectivity
  # nx.connect(poly, contact)
  # ...
  #
  # # read the reference netlist
  # nx.schematic("reference.cir")
  #
  # # configure the netlist compare
  # nx.same_circuits("A", "B")
  # ...
  #
  # # runs the compare
  # if ! nx.compare
  #   puts("no equivalence!")
  # end
  # @/code

  class LVSNetter < DRCNetter

    def initialize(engine)
      super
    end

    def _make_data

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

    # %LVS%
    # @name lvs_data
    # @brief Gets the internal RBA::LayoutVsSchematic object
    # @synopsis lvs_data
    # The RBA::LayoutVsSchematic object provides access to the internal details of
    # the netter object.

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

    # %LVS%
    # @name compare
    # @brief Compares the extracted netlist vs. the schematic
    # @synopsis compare
    # Before using this method, a schematic netlist has to be loaded with \schematic.
    # The compare can be configured in more details using \same_nets, \same_circuits,
    # \same_device_classes and \equivalent_pins.
    #
    # This method will return true, if the netlists are equivalent and false
    # otherwise.

    def compare
      @lvs.compare(@comparer)
    end

    def _ensure_two_netlists

      netlist || raise("No netlist present (not extracted?)")
      lvs_data.reference || raise("No reference schematic present (no set with 'schematic'?)")

      [ netlist, lvs_data.reference ]

    end
      
    # %LVS%
    # @name same_nets
    # @brief Establishes an equivalence between the nets
    # @synopsis same_nets(circuit, net_a, net_b)
    # @synopsis same_nets(circuit_a, net_a, circuit_b, net_b)
    # This method will force an equivalence between the net_a and net_b from circuit_a
    # and circuit_b (circuit in the three-argument form is for both circuit_a and circuit_b).
    # Circuit and nets are string giving a circuit and net by name.
    # After using this function, the compare algorithm will consider these nets equivalent.
    # Use this method to provide hints for the comparer in cases which are difficult to
    # resolve otherwise.
    #
    # Before this method can be used, a schematic netlist needs to be loaded with
    # \schematic.

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

    # %LVS%
    # @name same_circuits
    # @brief Establishes an equivalence between the circuits
    # @synopsis same_circuits(circuit_a, circuit_b)
    # This method will force an equivalence between the two circuits.
    # By default, circuits are identified by name. If names are different, this
    # method allows establishing an explicit correspondence.
    #
    # Before this method can be used, a schematic netlist needs to be loaded with
    # \schematic.

    def same_circuits(a, b)

      a.is_a?(String) || b.is_a?(String) || raise("Both arguments of 'same_circuits' need to be strings")

      ( nl_a, nl_b ) = _ensure_two_netlists

      circuit_a = nl_a.circuit_by_name(a) || raise("Not a valid circuit name in extracted netlist: #{a}")
      circuit_b = nl_b.circuit_by_name(b) || raise("Not a valid circuit name in reference netlist: #{b}")

      @comparer.same_circuits(circuit_a, circuit_b)
      
    end

    # %LVS%
    # @name same_device_classes
    # @brief Establishes an equivalence between the device classes
    # @synopsis same_device_classes(class_a, class_b)
    # This method will force an equivalence between the two device classes.
    # Device classes are also known as "models".
    # By default, device classes are identified by name. If names are different, this
    # method allows establishing an explicit correspondence.
    #
    # Before this method can be used, a schematic netlist needs to be loaded with
    # \schematic.

    def same_device_classes(a, b)

      a.is_a?(String) || b.is_a?(String) || raise("Both arguments of 'same_device_classes' need to be strings")

      ( nl_a, nl_b ) = _ensure_two_netlists

      dc_a = nl_a.device_class_by_name(a) || raise("Not a valid device class in extracted netlist: #{a}")
      dc_b = nl_b.device_class_by_name(b) || raise("Not a valid device class in reference netlist: #{b}")

      @comparer.same_device_classes(dc_a, dc_b)
      
    end

    # %LVS%
    # @name equivalent_pins
    # @brief Marks pins as equivalent
    # @synopsis equivalent_pins(circuit, pins ...)
    # This method will mark the given pins as equivalent. This gives the compare algorithm
    # more degrees of freedom when establishing net correspondence. Typically this method
    # is used to declare inputs from gates are equivalent where are are logically, but not
    # physically (e.g. in a CMOS NAND gate):
    #
    # @code
    # netter.equivalent_pins("NAND2", "A", "B")
    # @/code
    #
    # Before this method can be used, a schematic netlist needs to be loaded with
    # \schematic.

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

    # %LVS%
    # @name schematic
    # @brief Reads the reference netlist
    # @synopsis schematic(filename)
    # @synopsis schematic(filename, reader)
    # @synopsis schematic(netlist)
    # If a filename is given (first two forms), the netlist is read from the given file.
    # If no reader is provided, Spice format will be assumed. The reader object is a
    # RBA::NetlistReader object and allows detailed customization of the reader process.
    #
    # Alternatively, a RBA::Netlist object can be given which is obtained from any other
    # source.
      
    def schematic(schematic, reader = nil)

      if schematic.is_a?(RBA::Netlist)
        lvs_data.reference = netlist
      else

        schematic.is_a?(String) || raise("First argument must be string or netlist in 'schematic'")

        if reader
          reader.is_a?(RBA::NetlistReader) || raise("Second argument must be netlist reader object in 'schematic'")
        else
          reader = RBA::NetlistSpiceReader::new
        end

        netlist_file = @engine._make_path(schematic)
        @engine.info("Reading netlist: #{netlist_file} ..")

        netlist = RBA::Netlist::new
        netlist.read(netlist_file, reader)

        lvs_data.reference = netlist 

      end

    end

    # %LVS%
    # @name min_caps
    # @brief Ignores capacitors with a capacitance below a certain value
    # @synopsis min_caps(threshold)
    # After using this method, the netlist compare will ignore capacitance devices
    # with a capacitance values below the given threshold (in Farad).

    def min_caps(value)
      lvs_data
      @comparer.min_capacitance = value.to_f
    end
      
    # %LVS%
    # @name max_res
    # @brief Ignores resistors with a resistance above a certain value
    # @synopsis max_res(threshold)
    # After using this method, the netlist compare will ignore resistor devices
    # with a resistance value above the given threshold (in Farad).

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

