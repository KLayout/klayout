
from klayout.db import LayerInfo

class _PCellDeclarationHelperLayerDescriptor(object):
  """
  A descriptor object which translates the PCell parameters into class attributes
  """
  
  def __init__(self, param_index):
    self.param_index = param_index
  
  def __get__(self, obj, type = None):
    return obj._layers[self.param_index]
    
  def __set__(self, obj, value):
    raise AttributeError("can't change layer attribute")

class _PCellDeclarationHelperParameterDescriptor(object):
  """
  A descriptor object which translates the PCell parameters into class attributes

  In some cases (i.e. can_convert_from_shape), these placeholders are not 
  connected to real parameters (obj._param_values is None). In this case,
  the descriptor acts as a value holder (self.value)
  """
  
  def __init__(self, param_index, param_name):
    self.param_index = param_index
    self.param_name = param_name
    self.value = None
  
  def __get__(self, obj, type = None):
    if obj._param_values:
      return obj._param_values[self.param_index]
    elif obj._param_states:
      return obj._param_states.parameter(self.param_name)
    else:
      return self.value
    
  def __set__(self, obj, value):
    if obj._param_values:
      obj._param_values[self.param_index] = value
    else:
      self.value = value

class _PCellDeclarationHelperMixin:
  """  
  A mixin class that somewhat simplifies the implementation of a PCell
  Needed to build PCellDeclarationHelper
  """

  def __init__(self, *args, **kwargs):
    """
    initializes this instance
    """
    super().__init__(*args, **kwargs)
    # "private" attributes
    self._param_decls = []
    self._param_values = None
    self._param_states = None
    self._layer_param_index = []
    self._layers = []
    self._state_stack = []
    # public attributes
    self.layout = None
    self.shape = None
    self.layer = None
    self.cell = None

  def param(self, name, value_type, description, hidden = False, readonly = False, unit = None, default = None, choices = None, min_value = None, max_value = None, tooltip = None):
    """
    Defines a parameter
      name         -> the short name of the parameter
      type         -> the type of the parameter
      description  -> the description text
    named parameters
      hidden      -> (boolean) true, if the parameter is not shown in the dialog
      readonly    -> (boolean) true, if the parameter cannot be edited
      unit        -> the unit string
      min_value   -> the minimum value (only effective for numerical types and if no choices are present)
      max_value   -> the maximum value (only effective for numerical types and if no choices are present)
      default     -> the default value
      tooltip     -> tool tip text
      choices     -> ([ [ d, v ], ...) choice descriptions/value for choice type
    this method defines accessor methods for the parameters
      {name}       -> read accessor
      set_{name}   -> write accessor ({name}= does not work because the
                      Ruby confuses that method with variables)
      {name}_layer -> read accessor for the layer index for TypeLayer parameters
    """
  
    # create accessor methods for the parameters
    param_index = len(self._param_decls)
    setattr(type(self), name, _PCellDeclarationHelperParameterDescriptor(param_index, name))

    if value_type == type(self).TypeLayer:
      setattr(type(self), name + "_layer", _PCellDeclarationHelperLayerDescriptor(len(self._layer_param_index)))
      self._layer_param_index.append(param_index)
      
    # store the parameter declarations
    pdecl = self._make_parameter_declaration(name, value_type, description)
    self._param_decls.append(pdecl)    
    
    # set additional attributes of the parameters
    pdecl.hidden = hidden
    pdecl.readonly = readonly
    if not (default is None):
      pdecl.default = default
    if not (tooltip is None):
      pdecl.tooltip = tooltip
    pdecl.min_value = min_value
    pdecl.max_value = max_value
    if not (unit is None):
      pdecl.unit = unit
    if not (choices is None):
      if not isinstance(choices, list) and not isinstance(choices, tuple):
        raise TypeError("choices value must be an list/tuple of two-element arrays (description, value)")
      for c in choices:
        if (not isinstance(choices, list) and not isinstance(choices, tuple)) or len(c) != 2:
          raise TypeError("choices value must be an list/tuple of two-element arrays (description, value)")
        pdecl.add_choice(c[0],c[1])
    
    # return the declaration object for further operations
    return pdecl
  
  def display_text(self, parameters):
    """
    Reimplementation of PCellDeclaration.display_text

    This function delegates the implementation to self.display_text_impl
    after configuring the PCellDeclaration object.
    """
    self._start()
    self._param_values = parameters
    try:
      text = self.display_text_impl()
    finally:
      self._finish()
    return text
  
  def get_parameters(self):
    """
    Reimplementation of PCellDeclaration.get_parameters

    This function uses the collected parameters to feed the 
    PCell declaration.
    """
    return self._param_decls

  def _get_values(self):
    """
    Gets the temporary parameter values used for the current evaluation

    Call this function to get the a current parameter values. This 
    is an array of variants in the order the parameters are declared.
    """
    v = self._param_values
    self._param_values = None
    return v
  
  def _init_values(self, values = None, layers = None, states = None):
    """
    initializes the temporary parameter values for the current evaluation

    "values" are the original values. If "None" is given, the
    default values will be used. 
    "layers" are the layer indexes corresponding to the layer
    parameters.
    """
    self._start()
    self._param_values = None
    self._param_states = None
    if states:
      self._param_states = states
    elif not values:
      self._param_values = []
      for pd in self._param_decls:
        self._param_values.append(pd.default)
    else:
      self._param_values = values
    self._layers = layers

  def _start(self):
    """ 
    Is called to prepare the environment for an operation
    After the operation, "_finish" must be called.
    This method will push the state onto a stack, hence implementing
    reentrant implementation methods.
    """
    self._state_stack.append( (self._param_values, self._param_states, self._layers, self.cell, self.layout, self.layer, self.shape) )
    self._reset_state()

  def _finish(self):
    """
    Is called at the end of an implementation of a PCellDeclaration method
    """
    if len(self._state_stack) > 0:
      self._param_values, self._param_states, self._layers, self.cell, self.layout, self.layer, self.shape = self._state_stack.pop()
    else:
      self._reset_state()

  def _reset_state(self):
    """
    Resets the internal state
    """
    self._param_values = None
    self._param_states = None
    self._layers = None
    self.layout = super(_PCellDeclarationHelperMixin, self).layout()
    # This should be here:
    #  self.cell = None
    #  self.layer = None
    #  self.shape = None
    # but this would break backward compatibility of "display_text" (actually
    # exploiting this bug) - fix this in the next major release.
    
  def get_layers(self, parameters):
    """
    Reimplements PCellDeclaration.get_layers.

    Gets the layer definitions from all layer parameters.
    """
    layers = []
    for i in self._layer_param_index:
      if parameters[i] is not None:
        layers.append(parameters[i])
      else:
        layers.append(LayerInfo())
    return layers
    
  def callback(self, layout, name, states):
    """
    Reimplements PCellDeclaration.callback (change state on parameter change)

    The function delegates the implementation to callback_impl
    after updating the state of this object with the current parameters.
    """
    self._init_values(states = states)
    self.layout = layout
    try:
      self.callback_impl(name)
    finally:
      self._finish()

  def coerce_parameters(self, layout, parameters):
    """
    Reimplements PCellDeclaration.coerce parameters (make consistent)

    The function delegates the implementation to coerce_parameters_impl
    after updating the state of this object with the current parameters.
    """
    self._init_values(parameters)
    self.layout = layout
    try:
      self.coerce_parameters_impl()
      parameters = self._get_values()
    finally:
      self._finish()
    return parameters

  def produce(self, layout, layers, parameters, cell):
    """
    Reimplements PCellDeclaration.produce (produces the layout)

    The function delegates the implementation to produce_impl
    after updating the state of this object with the current parameters.
    """
    self._init_values(parameters, layers)
    self.cell = cell
    self.layout = layout
    try:
      self.produce_impl()
    finally:
      self._finish()

  def can_create_from_shape(self, layout, shape, layer):
    """
    Reimplements PCellDeclaration.can_create_from_shape

    The function delegates the implementation to can_create_from_shape_impl
    after updating the state of this object with the current parameters.
    """
    self._start()
    self.layout = layout
    self.shape = shape
    self.layer = layer
    try:
      ret = self.can_create_from_shape_impl()
    finally:
      self._finish()
    return ret
  
  def transformation_from_shape(self, layout, shape, layer):
    """
    Reimplements PCellDeclaration.transformation_from_shape

    The function delegates the implementation to transformation_from_shape_impl
    after updating the state of this object with the current parameters.
    """
    self._start()
    self.layout = layout
    self.shape = shape
    self.layer = layer
    try:
      t = self.transformation_from_shape_impl()
      if t is None:
        t = self._make_default_trans()
    finally:
      self._finish()
    return t
  
  def parameters_from_shape(self, layout, shape, layer):
    """
    Reimplements PCellDeclaration.parameters_from_shape

    The function delegates the implementation to parameters_from_shape_impl
    after updating the state of this object with the current parameters.
    """
    self._init_values()
    self.layout = layout
    self.shape = shape
    self.layer = layer
    try:
      self.parameters_from_shape_impl()
      param = self._get_values()
    finally:
      self._finish()
    return param
  
  def display_text_impl(self):
    """
    default implementation
    """
    return ""
 
  def coerce_parameters_impl(self):
    """
    default implementation 
    """
    pass
  
  def callback_impl(self, name):
    """
    default implementation 
    """
    pass
  
  def produce_impl(self):
    """
    default implementation
    """
    pass

  def can_create_from_shape_impl(self):
    """
    default implementation
    """
    return False
  
  def parameters_from_shape_impl(self):
    """
    default implementation
    """
    pass
  
  def transformation_from_shape_impl(self):
    """
    default implementation
    """
    return None
  

__all__ = [ "_PCellDeclarationHelperLayerDescriptor", 
            "_PCellDeclarationHelperParameterDescriptor", 
            "_PCellDeclarationHelperMixin" ]

