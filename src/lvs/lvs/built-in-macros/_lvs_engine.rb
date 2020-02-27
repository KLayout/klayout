# $autorun-early

module LVS

  include DRC

  # The LVS engine
  
  # %LVS%
  # @scope 
  # @name global 
  # @brief LVS Reference: Global Functions
  # Some functions are available on global level and can be used without any object.
  # Most of them are convenience functions that basically act on some default object
  # or provide function-like alternatives for the methods.
  #
  # LVS is built upon DRC. So all functions available in DRC are also available
  # in LVS. In LVS, DRC functions are used to derive functional layers from original 
  # layers or specification of the layout source.
  #
  # For more details about the DRC functions see \DRC::global.

  class LVSEngine < DRCEngine

    def initialize
      super
    end

    # %LVS%
    # @name netter
    # @brief Creates a new netter object
    # @synopsis netter
    # See \Netter# for more details
 
    def netter
      LVS::LVSNetter::new
    end

    def _netter
      @netter ||= LVS::LVSNetter::new(self)
    end
    
    def _before_cleanup

      # save the netlist database if requested
      if @output_lvsdb_file && @netter && @netter._lvs_data && @netter._lvs_data.xref

        lvsdb_file = _make_path(@output_lvsdb_file)
        info("Writing LVS database: #{lvsdb_file} ..")
        @netter._lvs_data.write(lvsdb_file, !@output_lvsdb_long)

      end

    end

    # %LVS%
    # @name report_lvs
    # @brief Specifies an LVS report for output
    # @synopsis report_lvs([ filename [, long ] ])
    # After the comparison step, the LVS database will be shown 
    # in the netlist database browser in a cross-reference view.
    # If a filename is given, the LVS database is also written to
    # this file. If a file name is given and "long" is true, a
    # verbose version of the LVS DB format will be used.
    #
    # If this method is called together with report_netlist and two files each, two
    # files can be generated - one for the extracted netlist (L2N database) and one for the
    # LVS database. However, report_netlist will only write the extracted netlist
    # while report_lvs will write the LVS database which also includes the 
    # extracted netlist.
    # 
    # report_lvs is only effective if a comparison step is included.
    
    def report_lvs(filename = nil, long = nil)
      @show_l2ndb = true
      if filename
        filename.is_a?(String) || raise("Argument must be string in report_lvs")
      end
      @output_lvsdb_file = filename
      @output_lvsdb_long = long
    end

    # %LVS%
    # @name schematic
    # @brief Reads the reference netlist
    # @synopsis schematic(filename)
    # @synopsis schematic(filename, reader)
    # @synopsis schematic(netlist)
    # See \Netter#schematic for a description of that function.
 
    # %LVS%
    # @name compare
    # @brief Compares the extracted netlist vs. the schematic
    # @synopsis compare
    # See \Netter#compare for a description of that function.
 
    # %LVS%
    # @name join_symmetric_nets
    # @brief Joins symmetric nets of selected circuits on the extracted netlist
    # @synopsis join_symmetric_nets(circuit_filter)
    # See \Netter#join_symmetric_nets for a description of that function.
 
    # %LVS%
    # @name align
    # @brief Aligns the extracted netlist vs. the schematic by flattening circuits where required
    # @synopsis align
    # See \Netter#align for a description of that function.
 
    # %LVS%
    # @name same_nets
    # @brief Establishes an equivalence between the nets
    # @synopsis same_nets(circuit, net_a, net_b)
    # @synopsis same_nets(circuit_a, net_a, circuit_b, net_b)
    # See \Netter#same_nets for a description of that function.
 
    # %LVS%
    # @name same_circuits
    # @brief Establishes an equivalence between the circuits
    # @synopsis same_circuits(circuit_a, circuit_b)
    # See \Netter#same_circuits for a description of that function.
 
    # %LVS%
    # @name same_device_classes
    # @brief Establishes an equivalence between the device_classes
    # @synopsis same_device_classes(class_a, class_b)
    # See \Netter#same_device_classes for a description of that function.
 
    # %LVS%
    # @name equivalent_pins
    # @brief Marks pins as equivalent
    # @synopsis equivalent_pins(circuit, pins ...)
    # See \Netter#equivalent_pins for a description of that function.
 
    # %LVS%
    # @name min_caps
    # @brief Ignores capacitors with a capacitance below a certain value
    # @synopsis min_caps(threshold)
    # See \Netter#min_caps for a description of that function.

    # %LVS%
    # @name max_res
    # @brief Ignores resistors with a resistance above a certain value
    # @synopsis max_res(threshold)
    # See \Netter#max_res for a description of that function.

    # %LVS%
    # @name max_branch_complexity
    # @brief Configures the maximum branch complexity for ambiguous net matching
    # @synopsis max_branch_complexity(n)
    # See \Netter#max_branch_complexity for a description of that function.

    # %LVS%
    # @name max_depth
    # @brief Configures the maximum search depth for net match deduction
    # @synopsis max_depth(n)
    # See \Netter#max_depth for a description of that function.

    # %LVS%
    # @name tolerance
    # @brief Specifies compare tolerances for certain device parameters
    # @synopsis tolerance(device_class_name, parameter_name, absolute_tolerance [, relative_tolerance])
    # @synopsis tolerance(device_class_name, parameter_name [, :absolute => absolute_tolerance] [, :relative => relative_tolerance])
    # See \Netter#tolerance for a description of that function.

    %w(schematic compare join_symmetric_nets tolerance align same_nets same_circuits same_device_classes equivalent_pins min_caps max_res max_depth max_branch_complexity).each do |f|
      eval <<"CODE"
        def #{f}(*args)
          _netter.#{f}(*args)
        end
CODE
    end

  end

end

