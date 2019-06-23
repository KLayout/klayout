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
    
  end

end

