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
      @comparer_miniconfig = []
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
    #
    # Some device parameters - like the resistor's "L" and "W" parameters - are not compared by default.
    # These are "secondary" device parameters. Using a tolerance on such parameters will make these parameters
    # being compared even if they are secondary ones.
    #
    # A function is skip a parameter during the device compare is "ignore_parameter".
    #
    # "tolerance" and "ignore_parameter" only have an effect with the default device comparer. Using a custom device comparer
    # will override the definitions by "ignore_parameter" or "tolerance".

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

      if self._l2n_data
        # already extracted
        self._tolerance(self._l2n_data, device_class_name, parameter_name, abs_tol, rel_tol)
      else
        @post_extract_config << lambda { |l2n| self._tolerance(l2n, device_class_name, parameter_name, abs_tol, rel_tol) }
      end

    end

    def _tolerance(l2n, device_class_name, parameter_name, abs_tol, rel_tol)

      dc = l2n.netlist.device_class_by_name(device_class_name)
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
    # @name ignore_parameter
    # @brief Skip a specific parameter for a given device class name during device compare
    # @synopsis ignore_parameter(device_class_name, parameter_name)
    # 
    # Use this function is ignore a parameter for a particular device class during the netlist compare.
    # Some parameters - for example "L" and "W" parameters of the resistor device - are "secondary" parameters
    # which are not ignored by default. Using "ignore_parameter" on such devices does not have an effect.
    #
    # "ignore_parameter" and "tolerance" only have an effect with the default device comparer. Using a custom device comparer
    # will override the definitions by "ignore_parameter" or "tolerance".

    def ignore_parameter(device_class_name, parameter_name)

      device_class_name.is_a?(String) || raise("Device class argument of 'ignore_parameter' must be a string")
      parameter_name.is_a?(String) || raise("Parameter name argument of 'ignore_parameter' must be a string")

      if self._l2n_data
        # already extracted
        self._ignore_parameter(self._l2n_data, device_class_name, parameter_name)
      else
        @post_extract_config << lambda { |l2n| self._ignore_parameter(l2n, device_class_name, parameter_name) }
      end

    end

    def _ignore_parameter(l2n, device_class_name, parameter_name)

      dc = l2n.netlist.device_class_by_name(device_class_name)
      if dc && dc.has_parameter?(parameter_name)
        ep = RBA::EqualDeviceParameters::ignore(dc.parameter_id(parameter_name))
        if dc.equal_parameters == nil
          dc.equal_parameters = ep
        else
          dc.equal_parameters += ep
        end
      end

    end

    # %LVS%
    # @name enable_parameter
    # @brief Indicates whether to enable a specific parameter for a given device
    # @synopsis enable_parameter(device_class_name, parameter_name)
    # The parameter is made "primary" which enables further applications - e.g. it is netlisted
    # for some elements which normally would not print that parameter, and the parameter
    # is compared in the default device compare scheme during netlist matching.
    #
    # Enabling a parameter is rather a hint for the system and the effects can be controlled
    # by other means, so this is not a strong concept. For example, once a \tolerance is 
    # specified for a parameter, the "primary" flag of the parameter is not considered anymore.
    # The inverse the this function is \disable_parameter.

    # %LVS%
    # @name disable_parameter
    # @brief Indicates whether to disable a specific parameter for a given device
    # @synopsis disable_parameter(device_class_name, parameter_name)
    # Disabling a parameter is the inverse of \enable_parameter. Disabling a parameter will
    # reset the "primary" flag of the parameter. This has several effects - e.g. the parameter will not be 
    # used in device compare during netlist matching by default. 
    #
    # This is not a strong concept but rather
    # a hint for the system. Disabling a parameter for netlist compare without side effects
    # is possible with the \ignore_parameter function. In the same way, \tolerance will enable a parameter for
    # netlist compare regardless of the "primary" status of the parameter.

    [ :enable_parameter, :disable_parameter ].each do |mn|
      eval <<"CODE"
      def #{mn}(device_class_name, parameter_name)

        device_class_name.is_a?(String) || raise("Device class argument of '#{mn}' must be a string")
        parameter_name.is_a?(String) || raise("Parameter name argument of '#{mn}' must be a string")

        if self._l2n_data
          # already extracted
          self._enable_parameter(self._l2n_data, device_class_name, parameter_name, :#{mn} == :enable_parameter)
        else
          @post_extract_config << lambda { |l2n| self._enable_parameter(l2n, device_class_name, parameter_name, :#{mn} == :enable_parameter) }
        end

      end
CODE
    end

    def _enable_parameter(l2n, device_class_name, parameter_name, enable)

      dc = l2n.netlist.device_class_by_name(device_class_name)
      if dc && dc.has_parameter?(parameter_name)
        dc.enable_parameter(parameter_name, enable)
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

      if self._l2n_data
        # already extracted
        self._join_symmetric_nets(self._l2n_data, circuit_pattern)
      else
        @post_extract_config << lambda { |l2n| self._join_symmetric_nets(l2n, circuit_pattern) }
      end

    end

    def _join_symmetric_nets(l2n, circuit_pattern)

      comparer = self._comparer_mini

      l2n.netlist.circuits_by_name(circuit_pattern).each do |c|
        comparer.join_symmetric_nets(c)
      end

      comparer._destroy

    end

    # %LVS%
    # @name split_gates
    # @brief Implements the "split gates" feature
    # @synopsis split_gates(device_name)
    # @synopsis split_gates(device_name, circuit_filter)
    # Multi-fingered, multi-gate MOS transistors can be built without connecting
    # the source/drain internal nets between the fingers. This will prevent 
    # "combine_devices" from combining the single gate transistors of the
    # different fingers into single ones.
    #
    # "split_gates" now marks the devices of the given class so that they will
    # receive a special treatment which joins the internl source/drain nodes.
    #
    # By default, this method is applied to all circuits. You can specify
    # a circuit pattern to apply it to certain circuits only.
    #
    # "device_name" must be a valid device name and denote a MOS3, MOS4, DMOS3
    # or DMOS4 device.

    def split_gates(device_name, circuit_pattern = "*")

      device_name.is_a?(String) || raise("Device name argument of 'split_gates' must be a string")
      circuit_pattern.is_a?(String) || raise("Circuit pattern argument of 'split_gates' must be a string")

      if self._l2n_data
        # already extracted
        self._split_gates(self._l2n_data, device_name, circuit_pattern)
      else
        @post_extract_config << lambda { |l2n| self._split_gates(l2n, device_name, circuit_pattern) }
      end

    end

    def _split_gates(l2n, device_name, circuit_pattern)

      dc = l2n.netlist.device_class_by_name(device_name)
      if ! dc
        raise("'#{device_name}' is not a valid device name") 
      end
      if ! dc.respond_to?(:join_split_gates)
        raise("Device '#{device_name}' is not a kind supporting 'split_gates'") 
      end

      l2n.netlist.circuits_by_name(circuit_pattern).each do |c|
        dc.join_split_gates(c)
      end

    end

    # %LVS%
    # @name blank_circuit
    # @brief Removes the content from the given circuits (blackboxing)
    # @synopsis blank_circuit(circuit_filter)
    # This method will erase all content from the circuits matching the filter.
    # The filter is a glob expression.
    #
    # This has the following effects:
    #
    # @ul
    # @li The circuits are no longer compared (netlist vs. schematic) @/li
    # @li Named pins are required to match (use labels on the nets to name pins in the layout) @/li
    # @li Unnamed pins are treated as equivalent and can be swapped @/li
    # @li The selected circuits will not be purged on netlist simplification @/li
    # @/ul
    #
    # Using this method can be useful to reduce the verification overhead for 
    # blocks which are already verifified by other ways or for which no schematic
    # is available - e.g. hard macros.
    #
    # Example:
    # 
    # @code
    # # skips all MEMORY* circuits from compare
    # blank_circuit("MEMORY*")
    # @/code

    def blank_circuit(circuit_pattern)

      circuit_pattern.is_a?(String) || raise("Circuit pattern argument of 'blank_circuit' must be a string")

      if self._l2n_data
        # already extracted
        self._blank_circuit(self._l2n_data, circuit_pattern)
      else
        @post_extract_config << lambda { |l2n| self._blank_circuit(l2n, circuit_pattern) }
      end

    end

    def _blank_circuit(l2n, circuit_pattern)

      (n, s) = _ensure_two_netlists

      n.blank_circuit(circuit_pattern)
      s.blank_circuit(circuit_pattern)

    end

    def _comparer

      comparer = RBA::NetlistComparer::new

      # execute the configuration commands
      @comparer_config.each do |cc|
        cc.call(comparer)
      end

      return comparer

    end

    def _comparer_mini

      comparer = RBA::NetlistComparer::new
      comparer.with_log = false

      # execute the configuration commands
      @comparer_miniconfig.each do |cc|
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
    # @name no_lvs_hints
    # @brief Disables LVS hints
    # @synopsis no_lvs_hints
    # LVS hints may be expensive to compute. Use this function to disable
    # generation of LVS hints

    def no_lvs_hints
      @comparer_config << lambda { |comparer| comparer.with_log = false }
    end

    # %LVS%
    # @name flag_missing_ports
    # @brief Flags inconsistently labelled or missing ports in the current top circuit
    # @synopsis flag_missing_ports
    # This method must be called after "compare" was executed successfully and will 
    # report errors if pins in the current top circuit's schematic are not labelled 
    # correspondingly in the layout. This prevents swapping of port labels or 
    # pads.
    #
    # @code
    # success = compare
    # success && flag_missing_ports
    # @/code
    #
    # Note that in order to use this method, the top circuit from the schematic netlist
    # needs to have pins. This may not be always the case - for example, if the top 
    # level circuit is not a subcircuit in a Spice netlist.

    def flag_missing_ports

      lvs_data.netlist || raise("Netlist not extracted yet")
      lvs_data.xref || raise("Compare step was not executed yet")

      lvs_data.flag_missing_ports(lvs_data.netlist.top_circuit)

    end

    # %LVS%
    # @name same_nets
    # @brief Establishes an equivalence between the nets
    # @synopsis same_nets(circuit_pattern, net_pattern)
    # @synopsis same_nets(circuit_pattern, net_a, net_b)
    # @synopsis same_nets(circuit_a, net_a, circuit_b, net_b)
    # This method will force an equivalence between the net_a and net_b from circuit_a
    # and circuit_b (circuit in the three-argument form is for both circuit_a and circuit_b).
    # 
    # In the four-argument form, the circuits can be either given by name or as Circuit
    # objects. In the three-argument form, the circuits have to be given by name pattern. 
    # Nets can be either given by name or as Net objects.
    # In the two-argument form, the circuits and nets have to be given as name pattern.
    #
    # "name pattern" are glob-style pattern - e.g. the following will identify the 
    # all nets starting with "A" from the extracted netlist with the same net from 
    # the schematic netlist for all circuits starting with "INV":
    #
    # @code
    # same_nets("INV*", "A*")
    # @/code
    #
    # A plain "*" for the net pattern forces all (named) nets to be equivalent between layout and schematic. 
    # Unnamed nets from the extracted netlist are not considered - i.e. nets without a label.
    #
    # After using this function, the compare algorithm will consider these nets equivalent.
    # Use this method to provide hints for the comparer in cases which are difficult to
    # resolve otherwise.
    #
    # circuit_a and net_a are for the layout netlist, circuit_b and net_b for the schematic netlist.
    # Names are case sensitive for layout-derived netlists and case-insensitive for SPICE schematic netlists.
    #
    # Use this method andwhere in the script before the \compare call.
    #
    # Multiple calls of "same_nets" can be used. The calls are effective in the order
    # the are given. For example, the following sequence specifies equivalence of all
    # equally named nets, with the exception of "A" and "B" which are equivalent to each other
    # inside cell "ND2", despite their different name:
    #
    # @code
    # same_nets("*", "*")
    # same_nets("ND2", "A", "B")
    # @/code

    def same_nets(*args)
      _same_nets_impl(false, *args)
    end

    # %LVS%
    # @name same_nets!
    # @brief Establishes an equivalence between the nets with matching requirement
    # @synopsis same_nets!(circuit_pattern, net_pattern)
    # @synopsis same_nets!(circuit_pattern, net_a, net_b)
    # @synopsis same_nets!(circuit_a, net_a, circuit_b, net_b)
    # This method is equivalent to \same_nets, but requires identity of the given nets.
    # If the specified nets do not match, an error is reported.
    #
    # For example, this global specification requires all named nets from the
    # layout to have an equivalent net in the schematic and those nets need to be 
    # identical for all circuits:
    #
    # @code
    # same_nets!("*", "*")
    # @/code
    #
    # The following specification requires "A" and "B" to be identical in
    # circuit "ND2". It is not an error if either "A" does not exist in the
    # layout or "B" does not exist in the schematic:
    #
    # @code
    # same_nets!("ND2", "A", "B")
    # @/code
    
    def same_nets!(*args)
      _same_nets_impl(true, *args)
    end

    def _same_nets_impl(force, *args)

      if args.size < 2 
        raise("Too few arguments to 'same_nets' (need at least 2)")
      end
      if args.size > 4 
        raise("Too many arguments to 'same_nets' (need max 4)")
      end

      if args.size == 2
        ( ca, a ) = args
        cb = nil
        ca.is_a?(String) || raise("Circuit argument of 'same_nets' must be a string")
        b = nil
        a.is_a?(String) || raise("Net argument of 'same_nets' must be a string")
      elsif args.size == 3
        ( ca, a, b ) = args
        cb = nil
        ca.is_a?(String) || raise("Circuit argument of 'same_nets' must be a string")
        [ a, b ].each do |n|
          n.is_a?(String) || n.is_a?(RBA::Net) || raise("Net arguments of 'same_nets' must be strings or Net objects")
        end
      else
        ( ca, a, cb, b ) = args
        [ ca, cb ].each do |n|
          n.is_a?(String) || n.is_a?(RBA::Circuit) || raise("Circuit arguments of 'same_nets' must be strings or Circuit objects")
        end
        [ a, b ].each do |n|
          n.is_a?(String) || n.is_a?(RBA::Net) || raise("Net arguments of 'same_nets' must be strings or Net objects")
        end
      end

      @comparer_config << lambda { |comparer| self._same_nets(comparer, ca, a, cb, b, force) }

    end

    def _same_nets(comparer, ca, a, cb, b, force)

      ( nl_a, nl_b ) = _ensure_two_netlists

      cs = !(nl_a.is_case_sensitive? && nl_b.is_case_sensitive?)

      if ca.is_a?(String) && (!cb || cb == "*")

        n2c = {}
        nl_a.circuits_by_name(ca, cs).each { |c| name = cs ? c.name.upcase : c.name; n2c[name] ||= [ nil, nil ]; n2c[name][0] = c }
        nl_b.circuits_by_name(ca, cs).each { |c| name = cs ? c.name.upcase : c.name; n2c[name] ||= [ nil, nil ]; n2c[name][1] = c }

        circuits = []
        n2c.keys.sort.each do |n|
          if n2c[n][0] && n2c[n][1]
            circuits << n2c[n]
          end
        end
          
      else 

        circuit_a = ca.is_a?(String) ? nl_a.circuit_by_name(ca) : ca
        circuit_b = cb.is_a?(String) ? nl_b.circuit_by_name(cb) : cb

        circuits = []
        if circuit_a && circuit_b
          circuits << [ circuit_a, circuit_b ]
        end

      end

      circuits.each do |circuit_a, circuit_b|

        if a.is_a?(String) && (!b || b == "*")

          n2n = {}
          circuit_a.nets_by_name(a, cs).each { |n| name = cs ? n.name.upcase : n.name; n2n[name] ||= [ nil, nil ]; n2n[name][0] = n }
          circuit_b.nets_by_name(a, cs).each { |n| name = cs ? n.name.upcase : n.name; n2n[name] ||= [ nil, nil ]; n2n[name][1] = n }

          nets = []
          n2n.keys.sort.each do |n|
            if n2n[n][0] && (force || n2n[n][1])
              nets << n2n[n]
            end
          end

        else

          if a.is_a?(String)
            net_a = circuit_a.net_by_name(a)
          else
            net_a = a
          end

          if b.is_a?(String)
            net_b = circuit_b.net_by_name(b)
          else
            net_b = b
          end
          
          nets = []
          if net_a && net_b
            nets << [ net_a, net_b ]
          end

        end

        nets.each do |net_a, net_b|
          comparer.same_nets(circuit_a, circuit_b, net_a, net_b, force)
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
    # circuit_a is for the layout netlist, circuit_b for the schematic netlist.
    # Names are case sensitive for layout-derived netlists and case-insensitive for SPICE schematic netlists.
    # 
    # One of the circuits may be nil. In this case, the corresponding
    # other circuit is mapped to "nothing", i.e. ignored.
    #
    # Use this method andwhere in the script before the \compare call.

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
    # class_a is for the layout netlist, class_b for the schematic netlist.
    # Names are case sensitive for layout-derived netlists and case-insensitive for SPICE schematic netlists.
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
    # "A" is no longer equivalent to "A" on the other side. If you want "A" to 
    # stay equivalent to "A" too, you need to use 'same_device_classes("A", "A")' 
    # in addition.
    #
    # Use this method andwhere in the script before the \compare call.

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
    # Names are case sensitive for layout-derived netlists and case-insensitive for SPICE schematic netlists.
    #
    # The pin arguments are zero-based pin numbers, where 0 is the first number, 1 the second etc.
    # If the netlist provides named pins, names can be used instead of numbers. Again, use upper
    # case pin names for SPICE netlists.
    #
    # Use this method andwhere in the script before the \compare call.

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
      @comparer_miniconfig << lambda { |comparer| comparer.min_capacitance = v }
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
      @comparer_miniconfig << lambda { |comparer| comparer.max_resistance = v }
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
    # 
    # By default, the depth is unlimited, but it may
    # be reduced in order to limit the compare runtimes at the cost
    # of a less elaborate compare attempt. The preferred solution 
    # however is to use labels for net name hints which also reduces
    # the branch complexity.

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
    # 
    # By default, the branch complexity is unlimited, but it may
    # be reduced in order to limit the compare runtimes at the cost
    # of a less elaborate compare attempt. The preferred solution 
    # however is to use labels for net name hints which also reduces
    # the depth.
 
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

