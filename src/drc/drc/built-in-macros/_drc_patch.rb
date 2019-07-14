# $autorun-early

# Extend the Float class by methods which convert
# values with units, i.e. 1.3nm gives 0.0013

1.0.class.class_eval do

  class << self
    @dbu = nil
    def _dbu=(dbu)
      @dbu = dbu
    end
    def _dbu
      @dbu
    end
  end

  def um
    self
  end
  def micron
    self
  end
  def degree
    self
  end
  def nm
    self*0.001
  end
  def mm
    self*1000.0
  end
  def m
    self*1000000.0
  end
  def nm2
    self*1e-6
  end
  def um2
    self
  end
  def micron2
    self
  end
  def mm2
    self*1.0e6
  end
  def m2
    self*1.0e12
  end
  def dbu
    self.class._dbu || raise("No layout loaded - cannot determine database unit")
    self*self.class._dbu
  end

end

# Extend the Fixnum class, so it is possible to 
# convert a value to Float with a unit spec, i.e.
# 5.nm -> 0.005. A spec with ".dbu" gives the 
# Fixnum value itself. This indicates a value in 
# database units for most methods of the DRC 
# framework.

1.class.class_eval do

  class << self
    @dbu = nil
    def _dbu=(dbu)
      @dbu = dbu
    end
    def _dbu
      @dbu
    end
  end

  def um
    self.to_f
  end
  def micron
    self.to_f
  end
  def degree
    self.to_f
  end
  def nm
    self*0.001
  end
  def mm
    self*1000.0
  end
  def m
    self*1000000.0
  end
  def nm2
    self*1.0e-6
  end
  def um2
    self.to_f
  end
  def micron2
    self.to_f
  end
  def mm2
    self*1.0e6
  end
  def m2
    self*1.0e12
  end
  def dbu
    self.class._dbu || raise("No layout loaded - cannot determine database unit")
    self*self.class._dbu
  end

end

