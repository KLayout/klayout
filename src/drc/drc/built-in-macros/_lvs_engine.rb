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

  class LVSEngine < DRCEngine

    def initialize
      super
    end

    def _netter
      @netter ||= LVS::LVSNetter::new(self)
    end
    
    def _before_cleanup

      # save the netlist database if requested
      if @output_lvsdb_file && @netter && @netter.lvs_data && @netter.lvs_data.xref

        lvsdb_file = _make_path(@output_lvsdb_file)
        info("Writing LVS database: #{lvsdb_file} ..")
        @netter.lvs_data.write(lvsdb_file)

      end

    end

    # %DRC%
    # @name report_lvs
    # @brief Specifies an LVS report for output
    # @synopsis report_lvs([ filename ])
    # After the comparison step, the LVS database will be shown 
    # in the netlist database browser in a cross-reference view.
    # If a filename is given, the LVS database is also written to
    # this file.
    #
    # If this method is called together with report_netlist and two files each, two
    # files can be generated - one for the extracted netlist (L2N database) and one for the
    # LVS database. However, report_netlist will only write the extracted netlist
    # while report_lvs will write the LVS database which also includes the 
    # extracted netlist.
    # 
    # report_lvs is only effective if a comparison step is included.
    
    def report_lvs(filename = nil)
      @show_l2ndb = true
      if filename
        filename.is_a?(String) || raise("Argument must be string in report_lvs")
      end
      @output_lvsdb_file = filename
    end

    # ...

    %w(schematic compare same_nets same_circuits same_device_classes equivalent_pins min_caps max_res max_depth max_branch_complexity).each do |f|
      eval <<"CODE"
        def #{f}(*args)
          _netter.#{f}(*args)
        end
CODE
    end

  end

end

