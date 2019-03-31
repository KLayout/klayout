
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "gsiDecl.h"
#include "dbNetlistDeviceExtractor.h"
#include "dbNetlistDeviceExtractorClasses.h"

namespace {

/**
 *  @brief A NetlistDeviceExtractor implementation that allows reimplementation of the virtual methods
 */
class GenericDeviceExtractor
  : public db::NetlistDeviceExtractor
{
public:
  GenericDeviceExtractor ()
    : db::NetlistDeviceExtractor (std::string ())
  {
    //  .. nothing yet ..
  }

  void register_device_class (db::DeviceClass *device_class)
  {
    //  the class is owned by the extractor
    device_class->keep ();
    db::NetlistDeviceExtractor::register_device_class (device_class);
  }

  void setup_fb ()
  {
    return db::NetlistDeviceExtractor::setup ();
  }

  virtual void setup ()
  {
    if (cb_setup.can_issue ()) {
      cb_setup.issue<GenericDeviceExtractor> (&GenericDeviceExtractor::setup_fb);
    } else {
      db::NetlistDeviceExtractor::setup ();
    }
  }

  db::Connectivity get_connectivity_fb (const db::Layout &layout, const std::vector<unsigned int> &layers) const
  {
    return db::NetlistDeviceExtractor::get_connectivity (layout, layers);
  }

  virtual db::Connectivity get_connectivity (const db::Layout &layout, const std::vector<unsigned int> &layers) const
  {
    if (cb_get_connectivity.can_issue ()) {
      return cb_get_connectivity.issue<GenericDeviceExtractor, db::Connectivity, const db::Layout &, const std::vector<unsigned int> &> (&GenericDeviceExtractor::get_connectivity_fb, layout, layers);
    } else {
      return db::NetlistDeviceExtractor::get_connectivity (layout, layers);
    }
  }

  void extract_devices_fb (const std::vector<db::Region> &layer_geometry)
  {
    return db::NetlistDeviceExtractor::extract_devices (layer_geometry);
  }

  virtual void extract_devices (const std::vector<db::Region> &layer_geometry)
  {
    if (cb_extract_devices.can_issue ()) {
      cb_extract_devices.issue<GenericDeviceExtractor, const std::vector<db::Region> &> (&GenericDeviceExtractor::extract_devices_fb, layer_geometry);
    } else {
      db::NetlistDeviceExtractor::extract_devices (layer_geometry);
    }
  }

  gsi::Callback cb_setup;
  gsi::Callback cb_get_connectivity;
  gsi::Callback cb_extract_devices;
};

}

namespace tl
{

template<> struct type_traits<GenericDeviceExtractor> : public tl::type_traits<void>
{
  //  mark "NetlistDeviceExtractor" as having a default ctor and no copy ctor
  typedef tl::false_tag has_copy_constructor;
  typedef tl::true_tag has_default_constructor;
};

}

namespace gsi
{

Class<db::NetlistDeviceExtractorError> decl_dbNetlistDeviceExtractorError ("db", "NetlistDeviceExtractorError",
  gsi::method ("message", &db::NetlistDeviceExtractorError::message,
    "@brief Gets the message text.\n"
  ) +
  gsi::method ("message=", &db::NetlistDeviceExtractorError::set_message, gsi::arg ("message"),
    "@brief Sets the message text.\n"
  ) +
  gsi::method ("cell_name", &db::NetlistDeviceExtractorError::cell_name,
    "@brief Gets the cell name.\n"
    "See \\cell_name= for details about this attribute."
  ) +
  gsi::method ("cell_name=", &db::NetlistDeviceExtractorError::set_cell_name, gsi::arg ("cell_name"),
    "@brief Sets the cell name.\n"
    "The cell name is the name of the layout cell which was treated. This is "
    "also the name of the circuit the device should have appeared in (it may be dropped because of this error). "
    "If netlist hierarchy manipulation happens however, the circuit may not exist "
    "any longer or may be renamed."
  ) +
  gsi::method ("geometry", &db::NetlistDeviceExtractorError::geometry,
    "@brief Gets the geometry.\n"
    "See \\geometry= for more details."
  ) +
  gsi::method ("geometry=", &db::NetlistDeviceExtractorError::set_geometry, gsi::arg ("polygon"),
    "@brief Sets the geometry.\n"
    "The geometry is optional. If given, a marker will be shown when selecting this error."
  ) +
  gsi::method ("category_name", &db::NetlistDeviceExtractorError::category_name,
    "@brief Gets the category name.\n"
    "See \\category_name= for more details."
  ) +
  gsi::method ("category_name=", &db::NetlistDeviceExtractorError::set_category_name, gsi::arg ("name"),
    "@brief Sets the category name.\n"
    "The category name is optional. If given, it specifies a formal category name. Errors with the same "
    "category name are shown in that category. If in addition a category description is specified "
    "(see \\category_description), this description will be displayed as the title of."
  ) +
  gsi::method ("category_description", &db::NetlistDeviceExtractorError::category_description,
    "@brief Gets the category description.\n"
    "See \\category_name= for details about categories."
  ) +
  gsi::method ("category_description=", &db::NetlistDeviceExtractorError::set_category_description, gsi::arg ("description"),
    "@brief Sets the category description.\n"
    "See \\category_name= for details about categories."
  ),
  "@brief An error that occurred during device extraction\n"
  "The device extractor will keep errors that occurred during extraction of the devices. "
  "It does not by using this error class.\n"
  "\n"
  "An error is basically described by the cell/circuit it occures in and the message. "
  "In addition, a geometry may be attached forming a marker that can be shown when the error is selected. "
  "The geometry is given as a \\DPolygon object. If no geometry is specified, this polygon is empty.\n"
  "\n"
  "For categorization of the errors, a category name and description may be specified. If given, the "
  "errors will be shown in the specified category. The category description is optional.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

static const std::string &ld_name (const db::NetlistDeviceExtractorLayerDefinition *ld)
{
  return ld->name;
}

static const std::string &ld_description (const db::NetlistDeviceExtractorLayerDefinition *ld)
{
  return ld->description;
}

static size_t ld_index (const db::NetlistDeviceExtractorLayerDefinition *ld)
{
  return ld->index;
}

Class<db::NetlistDeviceExtractorLayerDefinition> decl_dbNetlistDeviceExtractorLayerDefinition ("db", "NetlistDeviceExtractorLayerDefinition",
  gsi::method_ext ("name", &ld_name,
    "@brief Gets the name of the layer.\n"
  ) +
  gsi::method_ext ("description", &ld_description,
    "@brief Gets the description of the layer.\n"
  ) +
  gsi::method_ext ("index", &ld_index,
    "@brief Gets the index of the layer.\n"
  ),
  "@brief Describes a layer used in the device extraction\n"
  "This read-only structure is used to describe a layer in the device extraction.\n"
  "Every device has specific layers used in the device extraction process.\n"
  "Layer definitions can be retrieved using \\NetlistDeviceExtractor#each_layer.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<db::NetlistDeviceExtractor> decl_dbNetlistDeviceExtractor ("db", "DeviceExtractorBase",
  gsi::method ("name", &db::NetlistDeviceExtractor::name,
    "@brief Gets the name of the device extractor and the device class."
  ) +
  gsi::iterator ("each_layer_definition", &db::NetlistDeviceExtractor::begin_layer_definitions, &db::NetlistDeviceExtractor::end_layer_definitions,
    "@brief Iterates over all layer definitions."
  ) +
  gsi::iterator ("each_error", &db::NetlistDeviceExtractor::begin_errors, &db::NetlistDeviceExtractor::end_errors,
    "@brief Iterates over all errors collected in the device extractor."
  ),
  "@brief The base class for all device extractors.\n"
  "This is an abstract base class for device extractors. See \\GenericDeviceExtractor for a generic "
  "class which you can reimplement to supply your own customized device extractor. "
  "In many cases using one of the preconfigured specific device extractors may be useful already and "
  "it's not required to implement a custom one. For an example about a preconfigured device extractor see "
  "\\DeviceExtractorMOS3Transistor.\n"
  "\n"
  "This class cannot and should not be instantiated explicitly. Use one of the subclasses instead.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

Class<GenericDeviceExtractor> decl_GenericDeviceExtractor (decl_dbNetlistDeviceExtractor, "db", "GenericDeviceExtractor",
  gsi::method ("name=", &GenericDeviceExtractor::set_name,
    "@brief Sets the name of the device extractor and the device class."
  ) +
  gsi::callback ("setup", &GenericDeviceExtractor::setup, &GenericDeviceExtractor::cb_setup,
    "@brief Sets up the extractor.\n"
    "This method is supposed to set up the device extractor. This involves three basic steps:\n"
    "defining the name, the device classe and setting up the device layers.\n"
    "\n"
    "Use \\name= to give the extractor and it's device class a name.\n"
    "Use \\register_device_class to register the device class you need.\n"
    "Defined the layers by calling \\define_layer once or several times.\n"
  ) +
  gsi::callback ("get_connectivity", &GenericDeviceExtractor::get_connectivity, &GenericDeviceExtractor::cb_get_connectivity,
    gsi::arg ("layout"), gsi::arg ("layers"),
    "@brief Gets the connectivity object used to extract the device geometry.\n"
    "This method shall raise an error, if the input layer are not properly defined (e.g.\n"
    "too few etc.)\n"
    "\n"
    "The 'layers' argument specifies the actual layer layouts for the logical device layers (see \\define_layer). "
    "The list of layers corresponds to the number of layers defined. Use the layer indexes from this list "
    "to build the connectivity with \\Connectivity#connect."
  ) +
  gsi::callback ("extract_devices", &GenericDeviceExtractor::extract_devices, &GenericDeviceExtractor::cb_extract_devices,
    gsi::arg ("layer_geometry"),
    "@brief Extracts the devices from the given shape cluster.\n"
    "\n"
    "The shape cluster is a set of geometries belonging together in terms of the\n"
    "connectivity defined by \"get_connectivity\". The cluster might cover multiple devices,\n"
    "so the implementation needs to consider this case. The geometries are already merged.\n"
    "\n"
    "The implementation of this method shall use \"create_device\" to create new\n"
    "devices based on the geometry found. It shall use \"define_terminal\" to define\n"
    "terminals by which the nets extracted in the network extraction step connect\n"
    "to the new devices.\n"
  ) +
  gsi::method ("register_device_class", &GenericDeviceExtractor::register_device_class, gsi::arg ("device_class"),
   "@brief Registers a device class.\n"
   "The device class object will become owned by the netlist and must not be deleted by\n"
   "the caller. The name of the device class will be changed to the name given to\n"
   "the device extractor.\n"
   "This method shall be used inside the implementation of \\setup to register\n"
   "the device classes.\n"
  ) +
  gsi::method ("define_layer", &GenericDeviceExtractor::define_layer, gsi::arg ("name"), gsi::arg ("description"),
   "@brief Defines a layer.\n"
   "Each call will define one more layer for the device extraction.\n"
   "This method shall be used inside the implementation of \\setup to define\n"
   "the device layers. The actual geometries are later available to \\extract_devices\n"
   "in the order the layers are defined.\n"
  ) +
  gsi::method ("create_device", &GenericDeviceExtractor::create_device,
   "@brief Creates a device.\n"
   "The device object returned can be configured by the caller, e.g. set parameters.\n"
   "It will be owned by the netlist and must not be deleted by the caller.\n"
  ) +
  gsi::method ("define_terminal", (void (GenericDeviceExtractor::*) (db::Device *, size_t, size_t, const db::Polygon &)) &GenericDeviceExtractor::define_terminal,
    gsi::arg ("device"), gsi::arg ("terminal_id"), gsi::arg ("layer_index"), gsi::arg ("shape"),
   "@brief Defines a device terminal.\n"
   "This method will define a terminal to the given device and the given terminal ID. \n"
   "The terminal will be placed on the layer given by \"layer_index\". The layer index \n"
   "is the index of the layer during layer definition. The first layer is 0, the second layer 1 etc.\n"
   "\n"
   "This version produces a terminal with a shape given by the polygon. Note that the polygon is\n"
   "specified in database units.\n"
  ) +
  gsi::method ("define_terminal", (void (GenericDeviceExtractor::*) (db::Device *, size_t, size_t, const db::Box &)) &GenericDeviceExtractor::define_terminal,
    gsi::arg ("device"), gsi::arg ("terminal_id"), gsi::arg ("layer_index"), gsi::arg ("shape"),
   "@brief Defines a device terminal.\n"
   "This method will define a terminal to the given device and the given terminal ID. \n"
   "The terminal will be placed on the layer given by \"layer_index\". The layer index \n"
   "is the index of the layer during layer definition. The first layer is 0, the second layer 1 etc.\n"
   "\n"
   "This version produces a terminal with a shape given by the box. Note that the box is\n"
   "specified in database units.\n"
  ) +
  gsi::method ("define_terminal", (void (GenericDeviceExtractor::*) (db::Device *, size_t, size_t, const db::Point &)) &GenericDeviceExtractor::define_terminal,
    gsi::arg ("device"), gsi::arg ("terminal_id"), gsi::arg ("layer_index"), gsi::arg ("point"),
   "@brief Defines a device terminal.\n"
   "This method will define a terminal to the given device and the given terminal ID. \n"
   "The terminal will be placed on the layer given by \"layer_index\". The layer index \n"
   "is the index of the layer during layer definition. The first layer is 0, the second layer 1 etc.\n"
   "\n"
   "This version produces a point-like terminal. Note that the point is\n"
   "specified in database units.\n"
  ) +
  gsi::method ("dbu", &GenericDeviceExtractor::dbu,
   "@brief Gets the database unit\n"
  ) +
  gsi::method ("error", (void (GenericDeviceExtractor::*) (const std::string &)) &GenericDeviceExtractor::error,
    gsi::arg ("message"),
   "@brief Issues an error with the given message\n"
  ) +
  gsi::method ("error", (void (GenericDeviceExtractor::*) (const std::string &, const db::DPolygon &)) &GenericDeviceExtractor::error,
    gsi::arg ("message"), gsi::arg ("geometry"),
   "@brief Issues an error with the given message and micrometer-units polygon geometry\n"
  ) +
  gsi::method ("error", (void (GenericDeviceExtractor::*) (const std::string &, const db::Polygon &)) &GenericDeviceExtractor::error,
    gsi::arg ("message"), gsi::arg ("geometry"),
   "@brief Issues an error with the given message and databse-unit polygon geometry\n"
  ) +
  gsi::method ("error", (void (GenericDeviceExtractor::*) (const std::string &, const std::string &, const std::string &)) &GenericDeviceExtractor::error,
    gsi::arg ("category_name"), gsi::arg ("category_description"), gsi::arg ("message"),
   "@brief Issues an error with the given category name and description, message\n"
  ) +
  gsi::method ("error", (void (GenericDeviceExtractor::*) (const std::string &, const std::string &, const std::string &, const db::DPolygon &)) &GenericDeviceExtractor::error,
    gsi::arg ("category_name"), gsi::arg ("category_description"), gsi::arg ("message"), gsi::arg ("geometry"),
   "@brief Issues an error with the given category name and description, message and micrometer-units polygon geometry\n"
  ) +
  gsi::method ("error", (void (GenericDeviceExtractor::*) (const std::string &, const std::string &, const std::string &, const db::Polygon &)) &GenericDeviceExtractor::error,
    gsi::arg ("category_name"), gsi::arg ("category_description"), gsi::arg ("message"), gsi::arg ("geometry"),
   "@brief Issues an error with the given category name and description, message and databse-unit polygon geometry\n"
  ),
  "@brief The basic class for implementing custom device extractors.\n"
  "\n"
  "This class serves as a base class for implementing customized device extractors. "
  "This class does not provide any extraction functionality, so you have to "
  "implement every detail.\n"
  "\n"
  "Device extraction requires a few definitions. The definitions are made in the reimplementation of the \\setup\n"
  "method. Required definitions to be made are:\n"
  "\n"
  "@ul\n"
  "  @li The name of the extractor. This will also be the name of the device class produced by the extractor. "
  "      The name is set using \\name=. @/li\n"
  "  @li The device class of the devices to produce. The device class is registered using \\register_device_class. @/li\n"
  "  @li The layers used for the device extraction. These are input layers for the extraction as well as "
  "      output layers for defining the terminals. Terminals are the poins at which the nets connect to the devices.\n"
  "      Layers are defined using \\define_layer. Initially, layers are abstract definitions with a name and a description.\n"
  "      Concrete layers will be given when defining the connectivitiy. @/li\n"
  "@/ul\n"
  "\n"
  "When the device extraction is started, the device extraction algorithm will first ask the device extractor "
  "for the 'connectivity'. This is not a connectivity in a sense of electrical connections. The connectivity defines are "
  "logical compound that makes up the device. 'Connected' shapes are collected and presented to the device extractor.\n"
  "The connectivity is obtained by calling \\get_connectivity. This method must be "
  "implemented to produce the connectivity.\n"
  "\n"
  "Finally, the individual devices need to be extracted. Each cluster of connected shapes is presented to the "
  "device extractor. A cluster may include more than one device. It's the device extractor's responsibilty to "
  "extract the devices from this cluster and deliver the devices through \\create_device. In addition, terminals "
  "have to be defined, so the net extractor can connect to the devices. Terminal definitions are made through "
  "\\define_terminal. The device extraction is implemented in the \\extract_devices method.\n"
  "\n"
  "If errors occur during device extraction, the \\error method may be used to issue such errors. Errors "
  "reported this way are kept in the error log.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorMOS3Transistor *make_mos3_extractor (const std::string &name)
{
  return new db::NetlistDeviceExtractorMOS3Transistor (name);
}

Class<db::NetlistDeviceExtractorMOS3Transistor> decl_NetlistDeviceExtractorMOS3Transistor (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorMOS3Transistor",
  gsi::constructor ("new", &make_mos3_extractor, gsi::arg ("name"),
    "@brief Creates a new device extractor with the given name."
  ),
  "@brief A device extractor for a three-terminal MOS transistor\n"
  "\n"
  "This class supplies the generic extractor for a MOS device.\n"
  "The device is defined by two basic input layers: the diffusion area\n"
  "(source and drain) and the gate area. It requires a third layer\n"
  "(poly) to put the gate terminals on. The separation between poly\n"
  "and allows separating the device recognition layer (gate) from the\n"
  "conductive layer.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassMOS3Transistor.\n"
  "The extractor extracts the four parameters of this class: L, W, AS and AD.\n"
  "\n"
  "The diffusion area is distributed on the number of gates connecting to\n"
  "the particular source or drain area.\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorMOS4Transistor *make_mos4_extractor (const std::string &name)
{
  return new db::NetlistDeviceExtractorMOS4Transistor (name);
}

Class<db::NetlistDeviceExtractorMOS4Transistor> decl_NetlistDeviceExtractorMOS4Transistor (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorMOS4Transistor",
  gsi::constructor ("new", &make_mos4_extractor, gsi::arg ("name"),
    "@brief Creates a new device extractor with the given name."
  ),
  "@brief A device extractor for a four-terminal MOS transistor\n"
  "\n"
  "This class supplies the generic extractor for a MOS device.\n"
  "The device is defined by two basic input layers: the diffusion area\n"
  "(source and drain) and the gate area. It requires a third layer\n"
  "(poly) to put the gate terminals on and a forth layer to put the bulk\n"
  "terminal an. The separation between poly\n"
  "and allows separating the device recognition layer (gate) from the\n"
  "conductive layer.\n"
  "\n"
  "The bulk terminal layer can be an empty layer representing the substrate.\n"
  "In this use mode the bulk terminal shapes will be produced there. This\n"
  "layer then needs to be connected to a global net to establish the net.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassMOS4Transistor.\n"
  "The extractor extracts the four parameters of this class: L, W, AS and AD.\n"
  "\n"
  "The diffusion area is distributed on the number of gates connecting to\n"
  "the particular source or drain area.\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}
