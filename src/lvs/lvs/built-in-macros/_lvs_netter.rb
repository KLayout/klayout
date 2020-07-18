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
  # Similar to the DRC \DRC::Netter (which lacks the compare ability), the
  # relevant method of this object are available as global functions too
  # where they act on a default incarnation. Usually it's not required
  # to instantiate a Netter object explicitly. 
  #
  # The LVS Netter object inherits all methods of the \DRC::Netter.
  #
  # An individual netter object can be created, if the netter results
  # need to be kept for multiple extractions. If you really need
  # a Netter object, use the global \global#netter function:
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
      @comparer_config = []
    end

    def _make_data

      if @engine._dss
        @engine._dss.is_singular? || raise("The LVS script features more than one or no layout source - network extraction cannot be performed in such configurations")
        @lvs = RBA::LayoutVsSchematic::new(@engine._dss)
      else
        layout = @engine.source.layout
        cell = @engine.source.cell_obj
        @lvs = RBA::LayoutVsSchematic::new(cell.name, layout.dbu)
      end

      @lvs.name = "LVS"
      @lvs.generator = @engine._generator

      @l2n = @lvs

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
      @schematic = nil
    end

    def _take_data
      data = super
      @lvs = nil
      @schematic = nil
      data
    end

    def _lvs_data
      _l2n_data && @lvs
    end

    # %LVS%
    # @name tolerance
    # @brief Specifies compare tolerances for certain device parameters
    # @synopsis tolerance(device_class_name, parameter_name, absolute_tolerance [, relative_tolerance])
    # @synopsis tolerance(device_class_name, parameter_name [, :absolute => absolute_tolerance] [, :relative => relative_tolerance])
    # Specifies a compare tolerance for a specific parameter on a given device class.
    # The device class is the name of a device class in the extracted netlist.
    # Tolerances can be given in absolute units or relative or both. 
    # The relative tolerance is given as a factor, so 0.1 is a 10% tolerance.
    # Absolute and relative tolerances add, so specifying both allows for a larger
    # deviation.

    def tolerance(device_class_name, parameter_name, *args)

      device_class_name.is_a?(String) || raise("Device class argument of 'tolerance' must be a string")
      parameter_name.is_a?(String) || raise("Parameter name argument of 'tolerance' must be a string")

      abs_tol = nil
      rel_tol = nil
      args.each do |a|
        if a.is_a?(Hash)
          a.keys.each do |k|
            if k == :absolute
              abs_tol = a[k].to_f
            elsif k == :relative
              rel_tol = a[k].to_f
            else
              raise("Unknown option #{k.to_s} in 'tolerance' (allowed options are: absolute, relative)")
            end
          end
        elsif abs_tol == nil
          abs_tol = a.to_f
        elsif rel_tol == nil
          rel_tol = a.to_f
        else
          raise("Too many arguments in 'tolerance' (max. 4)")
        end
      end

      abs_tol ||= 0.0
      rel_tol ||= 0.0

      dc = netlist.device_class_by_name(device_class_name)
      if dc && dc.has_parameter?(parameter_name)
        ep = RBA::EqualDeviceParameters::new(dc.parameter_id(parameter_name), abs_tol, rel_tol)
        if dc.equal_parameters == nil
          dc.equal_parameters = ep
        else
          dc.equal_parameters += ep
        end
      end

    end

    # %LVS%
    # @name align
    # @brief Aligns the extracted netlist vs. the schematic
    # @synopsis align
    # The align method will modify the netlists in case of missing 
    # corresponding circuits. It will flatten these circuits, thus 
    # improving the equivalence between the netlists. Top level circuits
    # are not flattened.
    #
    # This feature is in particular useful to remove structural cells
    # like device PCells, reuse blocks etc.
    # 
    # This method will also remove schematic circuits for which there is
    # no corresponding layout cell. In the extreme case of flat layout this
    # will result in a flat vs. flat compare.
    # 
    # "netlist.flatten_circuit(...)" or "schematic.flatten_circuit(...)"
    # are other (explicit) ways to flatten circuits.
    #
    # Please note that flattening circuits has some side effects such 
    # as loss of details in the cross reference and net layout.

    def align

      nl = _ensure_two_netlists

      comparer = self._comparer 

      unmatched_a = comparer.unmatched_circuits_a(*nl)

      # check whether we're about to flatten away the internal top cell - that's bad
      top_cell = l2n_data.internal_top_cell.name
      if unmatched_a.find { |c| c.name == top_cell }
        raise("Can't find a schematic counterpart for the top cell #{top_cell} - use 'same_circuit' to establish a correspondence")
      end

      # flatten layout cells for which there is no corresponding schematic circuit
      unmatched_a.each do |c|
        @engine.info("Flatten layout cell (no schematic): #{c.name}")
      end
      nl[0].flatten_circuits(unmatched_a)

      # flatten schematic circuits for which there is no corresponding layout cell
      unmatched_b = comparer.unmatched_circuits_b(*nl)
      unmatched_b.each do |c|
        @engine.info("Flatten schematic circuit (no layout): #{c.name}")
      end
      nl[1].flatten_circuits(unmatched_b)

    end

    # %LVS%
    # @name compare
    # @brief Compares the extracted netlist vs. the schematic
    # @synopsis compare
    # Before using this method, a schematic netlist has to be loaded with \schematic.
    # The compare can be configured in more details using \same_nets, \same_circuits,
    # \same_device_classes and \equivalent_pins.
    #
    # The compare method will also modify the netlists in case of missing 
    # corresponding circuits: the unpaired circuit will be flattened then.
    #
    # This method will return true, if the netlists are equivalent and false
    # otherwise.

    def compare

      nl = _ensure_two_netlists
      lvs_data.reference = nl[1]

      lvs_data.compare(self._comparer)

    end

    # %LVS%
    # @name join_symmetric_nets
    # @brief Joins symmetric nets of selected circuits on the extracted netlist
    # @synopsis join_symmetric_nets(circuit_filter)
    # Nets are symmetrical if swapping them would not modify the circuit.
    # Hence they will carry the same potential and can be connected (joined).
    # This will simplify the circuit and can be applied before device combination
    # (e.g. through "netlist.simplify") to render a schematic-equivalent netlist in some 
    # cases where symmetric nodes are split (i.e. "split gate" configuration).
    #
    # This method operates on the extracted netlist (layout). The circuit filter
    # specifies the circuits to which to apply this operation. The filter is a
    # glob-style pattern. Using "*" for all circuits is possible, but it's 
    # discouraged currenty until the reliability of the symmetry detection 
    # algorithm is established. Currently it is recommended to apply it only to 
    # those circuits for which this feature is required.
    #
    # For the symmetry detection, the specified constraints (e.g. tolerances,
    # device filters etc.) apply.

    def join_symmetric_nets(circuit_pattern)

      circuit_pattern.is_a?(String) || raise("Circuit pattern argument of 'join_symmetric_nets' must be a string")

      comparer = self._comparer 

      netlist || raise("No netlist present (not extracted?)")
      netlist.circuits_by_name(circuit_pattern).each do |c|
        comparer.join_symmetric_nets(c)
      end

    end

    def _comparer

      comparer = RBA::NetlistComparer::new

      # execute the configuration commands
      @comparer_config.each do |cc|
        cc.call(comparer)
      end

      return comparer

    end

    def _ensure_two_netlists

      netlist || raise("No netlist present (not extracted?)")
      schematic || raise("No reference schematic present (not set with 'schematic'?)")

      [ netlist, schematic ]

    end
      
    # %LVS%
    # @name same_nets
    # @brief Establishes an equivalence between the nets
    # @synopsis same_nets(circuit, net_a, net_b)
    # @synopsis same_nets(circuit_a, net_a, circuit_b, net_b)
    # This method will force an equivalence between the net_a and net_b from circuit_a
    # and circuit_b (circuit in the three-argument form is for both circuit_a and circuit_b).
    #
    # In the four-argument form, the circuits can be either given by name or as Circuit
    # objects. In the three-argument form, the circuit has to be given by name. 
    # Nets can be either given by name or as Net objects.
    #
    # After using this function, the compare algorithm will consider these nets equivalent.
    # Use this method to provide hints for the comparer in cases which are difficult to
    # resolve otherwise.
    #
    # Before this method can be used, a schematic netlist needs to be loaded with
    # \schematic.

    def same_nets(*args)

      if args.size < 3 
        raise("Too few arguments to 'same_nets' (need at least 3)")
      end
      if args.size > 4 
        raise("Too many arguments to 'same_nets' (need max 4)")
      end

      if args.size == 3
        ( ca, a, b ) = args
        cb = ca
        ca.is_a?(String) || raise("Circuit argument of 'same_nets' must be a string")
      else
        ( ca, a, cb, b ) = args
        [ ca, cb ].each do |n|
          n.is_a?(String) || n.is_a?(RBA::Net) || raise("Circuit arguments of 'same_nets' must be strings or Net objects")
        end
      end

      [ a, b ].each do |n|
        n.is_a?(String) || n.is_a?(RBA::Net) || raise("Net arguments of 'same_nets' must be strings or Net objects")
      end

      @comparer_config << lambda { |comparer| self._same_nets(comparer, ca, a, cb, b) }

    end

    def _same_nets(comparer, ca, a, cb, b)

      ( nl_a, nl_b ) = _ensure_two_netlists

      if ca.is_a?(String)
        circuit_a = nl_a.circuit_by_name(ca)
      else 
        circuit_a = ca
      end

      if cb.is_a?(String)
        circuit_b = nl_b.circuit_by_name(cb)
      else
        circuit_b = cb
      end

      if circuit_a && circuit_b

        if a.is_a?(String)
          net_a = circuit_a.net_by_name(a) || raise("Not a valid net name in extracted netlist in 'same_nets': #{a} (for circuit #{circuit_a})")
        else
          net_a = a
        end

        if b.is_a?(String)
          net_b = circuit_b.net_by_name(b) || raise("Not a valid net name in extracted netlist in 'same_nets': #{b} (for circuit #{circuit_b})")
        else
          net_b = b
        end

        if net_a && net_b
          comparer.same_nets(net_a, net_b)
        end

      end
      
    end

    # %LVS%
    # @name same_circuits
    # @brief Establishes an equivalence between the circuits
    # @synopsis same_circuits(circuit_a, circuit_b)
    # This method will force an equivalence between the two circuits.
    # By default, circuits are identified by name. If names are different, this
    # method allows establishing an explicit correspondence.
    # 
    # One of the circuits may be nil. In this case, the corresponding
    # other circuit is mapped to "nothing", i.e. ignored.
    #
    # Before this method can be used, a schematic netlist needs to be loaded with
    # \schematic.

    def same_circuits(a, b)

      a.is_a?(String) || a == nil || b.is_a?(String) || b == nil || raise("Both arguments of 'same_circuits' need to be strings or nil")

      @comparer_config << lambda { |comparer| self._same_circuits(comparer, a, b) }

    end

    def _same_circuits(comparer, a, b)

      ( nl_a, nl_b ) = _ensure_two_netlists

      circuit_a = a && nl_a.circuit_by_name(a)
      circuit_b = b && nl_b.circuit_by_name(b)

      if circuit_a && circuit_b
        comparer.same_circuits(circuit_a, circuit_b)
      end
      
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
    #
    # One of the device classes may be "nil". In this case, the corresponding
    # other device class is mapped to "nothing", i.e. ignored.
    #
    # A device class on one side can be mapped to multiple other device
    # classes on the other side by using this function multiple times, e.g.
    #
    # @code
    # same_device_classes("POLYRES", "RES")
    # same_device_classes("WELLRES", "RES")
    # @/code
    #
    # will match both "POLYRES" and "WELLRES" on the layout side to "RES" on the 
    # schematic side.
    #
    # Once a device class is mentioned with "same_device_classes", matching by
    # name is disabled for this class. So after using 'same_device_classes("A", "B")'
    # "A" is no longer equivalent to "A" on the other side.

    def same_device_classes(a, b)

      a.is_a?(String) || a == nil || b.is_a?(String) || b == nil || raise("Both arguments of 'same_device_classes' need to be strings or nil")

      @comparer_config << lambda { |comparer| self._same_device_classes(comparer, a, b) }

    end

    def _same_device_classes(comparer, a, b)

      ( nl_a, nl_b ) = _ensure_two_netlists

      dc_a = a && (nl_a.device_class_by_name(a) || raise("Not a valid device class in extracted netlist in 'same_device_class': #{a}"))
      dc_b = b && nl_b.device_class_by_name(b)

      # NOTE: a device class is allowed to be missing in the reference netlist because the
      # device may simply not be used there.
      if dc_b
        comparer.same_device_classes(dc_a, dc_b)
      end
      
    end

    # %LVS%
    # @name equivalent_pins
    # @brief Marks pins as equivalent
    # @synopsis equivalent_pins(circuit, pin ...)
    # This method will mark the given pins as equivalent. This gives the compare algorithm
    # more degrees of freedom when establishing net correspondence. Typically this method
    # is used to declare inputs from gates are equivalent where are are logically, but not
    # physically (e.g. in a CMOS NAND gate):
    #
    # @code
    # netter.equivalent_pins("NAND2", 0, 1)
    # @/code
    #
    # The circuit argument is either a circuit name (a string) or a Circuit object
    # from the schematic netlist.
    #
    # The pin arguments are zero-based pin numbers, where 0 is the first number, 1 the second etc.
    # If the netlist provides named pins, names can be used instead of numbers.
    #
    # Before this method can be used, a schematic netlist needs to be loaded with
    # \schematic.

    def equivalent_pins(circuit, *pins)

      circuit.is_a?(String) || 
        raise("Circuit argument of 'equivalent_pins' needs to be a string")

      pins.each do |a|
        a.is_a?(String) || 
          a.respond_to?(:to_i) ||
          raise("All pin arguments of 'equivalent_pins' need to be strings or numbers")
      end

      @comparer_config << lambda { |comparer| self._equivalent_pins(comparer, circuit, *pins) }

    end

    def _equivalent_pins(comparer, circuit, *pins)

      ( nl_a, nl_b ) = _ensure_two_netlists

      circuit_b = nl_b.circuit_by_name(circuit)
      if circuit_b

        pins_by_index = []
        circuit_b.each_pin { |p| pins_by_index << p }

        pin_ids_b = pins.collect do |p|
          if p.is_a?(String)
            pin = circuit_b.pin_by_name(p) || raise("Not a valid pin name in circuit '#{circuit}' in 'equivalent_pins': #{p}")
          else
            pin = pins_by_index[p.to_i] || raise("Not a valid pin index in circuit '#{circuit}' in 'equivalent_pins': #{p}")
          end
          pin.id
        end

        comparer.equivalent_pins(circuit_b, pin_ids_b)

      end
      
    end

    # %LVS%
    # @name schematic
    # @brief Gets, sets or reads the reference netlist
    # @synopsis schematic(filename)
    # @synopsis schematic(filename, reader)
    # @synopsis schematic(netlist)
    # @synopsis schematic
    # If no argument is given, the current schematic netlist is returned. nil is 
    # returned if no schematic netlist is set yet.
    #
    # If a filename is given (first two forms), the netlist is read from the given file.
    # If no reader is provided, Spice format will be assumed. The reader object is a
    # RBA::NetlistReader object and allows detailed customization of the reader process.
    #
    # Alternatively, a RBA::Netlist object can be given which is obtained from any other
    # source.
      
    def schematic(schematic = nil, reader = nil)

      if !schematic

        # without arguments: return current schematic
        @schematic

      elsif schematic.is_a?(RBA::Netlist)

        @schematic = netlist

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

        @schematic = netlist 

      end

    end

    # %LVS%
    # @name min_caps
    # @brief Ignores capacitors with a capacitance below a certain value
    # @synopsis min_caps(threshold)
    # After using this method, the netlist compare will ignore capacitance devices
    # with a capacitance values below the given threshold (in Farad).

    def min_caps(value)
      v = value.to_f
      @comparer_config << lambda { |comparer| comparer.min_capacitance = v }
    end
      
    # %LVS%
    # @name max_res
    # @brief Ignores resistors with a resistance above a certain value
    # @synopsis max_res(threshold)
    # After using this method, the netlist compare will ignore resistor devices
    # with a resistance value above the given threshold (in Farad).

    def max_res(value)
      v = value.to_f
      @comparer_config << lambda { |comparer| comparer.max_resistance = v }
    end

    # %LVS%
    # @name max_depth
    # @brief Configures the maximum search depth for net match deduction
    # @synopsis max_depth(n)
    # The netlist compare algorithm works recursively: once a net
    # equivalence is established, additional matches are derived from
    # this equivalence. Such equivalences in turn are used to derive
    # new equivalences and so on. The maximum depth parameter configures
    # the number of recursions the algorithm performs before picking
    # the next net. With higher values for the depth, the algorithm
    # pursues this "deduction path" in greater depth while with 
    # smaller values, the algorithm prefers picking nets in a random fashion
    # as the seeds for this deduction path. The default value is 8. 

    def max_depth(value)
      v = value.to_i
      @comparer_config << lambda { |comparer| comparer.max_depth = v }
    end

    # %LVS%
    # @name max_branch_complexity
    # @brief Configures the maximum branch complexity for ambiguous net matching
    # @synopsis max_branch_complexity(n)
    # The netlist compare algorithm is basically a backtracing algorithm.
    # With ambiguous nets, the algorithm picks possible net pairs and
    # tries whether they will make a good match. Following the deduction
    # path for this nets may lead to further branches if more ambiguous
    # nets are encountered. To avoid combinational explosion, the maximum
    # branch complexity is limited to the value configured with this 
    # function. The default value is 500 which means not more than
    # 500 combinations are tried for a single seed pair. For networks
    # with inherent ambiguity such as decoders, the complexity
    # can be increased at the expense of potentially larger runtimes.
    # The runtime penality is roughly proportional to the branch
    # complexity.
 
    def max_branch_complexity(value)
      v = value.to_i
      @comparer_config << lambda { |comparer| comparer.max_branch_complexity = v }
    end

    # %LVS%
    # @name consider_net_names
    # @brief Indicates whether the netlist comparer shall use net names
    # @synopsis consider_net_names(f)
    # If this value is set to true (the default), the netlist comparer
    # will employ net names to resolve ambiguities. If set to false,
    # ambiguities will be resolved based on the topology alone. Topology
    # resolution is more expensive.
 
    def consider_net_names(value)
      v = ! value
      @comparer_config << lambda { |comparer| comparer.dont_consider_net_names = v }
    end

  end
  
end

