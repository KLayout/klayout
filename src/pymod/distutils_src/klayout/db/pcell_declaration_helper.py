from klayout.db import Trans, PCellDeclaration, PCellParameterDeclaration


class _PCellDeclarationHelperLayerDescriptor(object):
    """
    A descriptor object which translates the PCell parameters into class attributes
    """

    def __init__(self, param_index):
        self.param_index = param_index

    def __get__(self, obj, type=None):
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

    def __init__(self, param_index):
        self.param_index = param_index
        self.value = None

    def __get__(self, obj, type=None):
        if obj._param_values:
            return obj._param_values[self.param_index]
        else:
            return self.value

    def __set__(self, obj, value):
        if obj._param_values:
            obj._param_values[self.param_index] = value
        else:
            self.value = value


class _PCellDeclarationHelper(PCellDeclaration):
    """
    A helper class that somewhat simplifies the implementation
    of a PCell
    """

    def __init__(self):
        """
        initialize this instance
        """
        # "private" attributes
        self._param_decls = []
        self._param_values = None
        self._layer_param_index = []
        self._layers = []
        # public attributes
        self.layout = None
        self.shape = None
        self.layer = None
        self.cell = None

    def param(self, name, value_type, description, hidden=False, readonly=False, unit=None, default=None, choices=None):
        """
        Defines a parameter
          name         -> the short name of the parameter
          type         -> the type of the parameter
          description  -> the description text
        named parameters
          hidden      -> (boolean) true, if the parameter is not shown in the dialog
          readonly    -> (boolean) true, if the parameter cannot be edited
          unit        -> the unit string
          default     -> the default value
          choices     -> ([ [ d, v ], ...) choice descriptions/value for choice type
        this method defines accessor methods for the parameters
          {name}       -> read accessor
          set_{name}   -> write accessor ({name}= does not work because the
                          Ruby confuses that method with variables)
          {name}_layer -> read accessor for the layer index for TypeLayer parameters
        """

        # create accessor methods for the parameters
        param_index = len(self._param_decls)
        setattr(type(self), name, _PCellDeclarationHelperParameterDescriptor(param_index))

        if value_type == type(self).TypeLayer:
            setattr(type(self), name + "_layer",
                    _PCellDeclarationHelperLayerDescriptor(len(self._layer_param_index)))
            self._layer_param_index.append(param_index)

        # store the parameter declarations
        pdecl = PCellParameterDeclaration(name, value_type, description)
        self._param_decls.append(pdecl)

        # set additional attributes of the parameters
        pdecl.hidden = hidden
        pdecl.readonly = readonly
        if not (default is None):
            pdecl.default = default
        if not (unit is None):
            pdecl.unit = unit
        if not (choices is None):
            if not isinstance(choices, list) and not isinstance(choices, tuple):
                raise "choices value must be an list/tuple of two-element arrays (description, value)"
            for c in choices:
                if (not isinstance(choices, list) and not isinstance(choices, tuple)) or len(c) != 2:
                    raise "choices value must be an list/tuple of two-element arrays (description, value)"
                pdecl.add_choice(c[0], c[1])

        # return the declaration object for further operations
        return pdecl

    def display_text(self, parameters):
        """
        implementation of display_text
        """
        self._param_values = parameters
        text = self.display_text_impl()
        self._param_values = None
        return text

    def get_parameters(self):
        """
        gets the parameters
        """
        return self._param_decls

    def get_values(self):
        """
        gets the temporary parameter values
        """
        v = self._param_values
        self._param_values = None
        return v

    def init_values(self, values=None, layers=None):
        """
        initializes the temporary parameter values
        "values" are the original values. If "None" is given, the
        default values will be used.
        "layers" are the layer indexes corresponding to the layer
        parameters.
        """
        if not values:
            self._param_values = []
            for pd in self._param_decls:
                self._param_values.append(pd.default)
        else:
            self._param_values = values
        self._layers = layers

    def finish(self):
        """
        Needs to be called at the end of produce() after init_values was used
        """
        self._param_values = None
        self._layers = None

    def get_layers(self, parameters):
        """
        get the layer definitions
        """
        layers = []
        for i in self._layer_param_index:
            layers.append(parameters[i])
        return layers

    def coerce_parameters(self, layout, parameters):
        """
        coerce parameters (make consistent)
        """
        self.init_values(parameters)
        self.layout = layout
        self.coerce_parameters_impl()
        self.layout = None
        return self.get_values()

    def produce(self, layout, layers, parameters, cell):
        """
        coerce parameters (make consistent)
        """
        self.init_values(parameters, layers)
        self.cell = cell
        self.layout = layout
        self.produce_impl()
        self.cell = None
        self.layout = None
        self.finish()

    def can_create_from_shape(self, layout, shape, layer):
        """
        produce a helper for can_create_from_shape
        """
        self.layout = layout
        self.shape = shape
        self.layer = layer
        ret = self.can_create_from_shape_impl()
        self.layout = None
        self.shape = None
        self.layer = None
        return ret

    def transformation_from_shape(self, layout, shape, layer):
        """
        produce a helper for parameters_from_shape
        """
        self.layout = layout
        self.shape = shape
        self.layer = layer
        t = self.transformation_from_shape_impl()
        self.layout = None
        self.shape = None
        self.layer = None
        return t

    def parameters_from_shape(self, layout, shape, layer):
        """
        produce a helper for parameters_from_shape
        with this helper, the implementation can use the parameter setters
        """
        self.init_values()
        self.layout = layout
        self.shape = shape
        self.layer = layer
        self.parameters_from_shape_impl()
        param = self.get_values()
        self.layout = None
        self.shape = None
        self.layer = None
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
        return Trans()


# import the Type... constants from PCellParameterDeclaration
for k in dir(PCellParameterDeclaration):
    if k.startswith("Type"):
        setattr(_PCellDeclarationHelper, k, getattr(PCellParameterDeclaration, k))

# Inject the PCellDeclarationHelper into pya module for consistency:
PCellDeclarationHelper = _PCellDeclarationHelper
