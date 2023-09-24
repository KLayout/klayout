
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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
 *  @brief A NetlistDeviceExtractor implementation that allows reimplementing the virtual methods
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

/**
 *  @brief A DeviceClassFactory implementation that allows reimplementation of the virtual methods
 */
class DeviceClassFactoryImpl
  : public db::DeviceClassFactory
{
public:
  DeviceClassFactoryImpl ()
    : db::DeviceClassFactory ()
  {
    //  .. nothing yet ..
  }

  virtual db::DeviceClass *create_class () const
  {
    if (cb_create_class.can_issue ()) {
      return cb_create_class.issue<db::DeviceClassFactory, db::DeviceClass *> (&db::DeviceClassFactory::create_class);
    } else {
      return 0;
    }
  }

  gsi::Callback cb_create_class;
};

}

namespace gsi
{

Class<DeviceClassFactoryImpl> decl_dbDeviceClassFactoryBase ("db", "DeviceClassFactory",
  gsi::factory_callback ("create_class", &DeviceClassFactoryImpl::create_class, &DeviceClassFactoryImpl::cb_create_class,
    "@brief Creates the DeviceClass object\n"
    "Reimplement this method to create the desired device class."
  ),
  "@brief A factory for creating specific device classes for the standard device extractors\n"
  "Use a reimplementation of this class to provide a device class generator for built-in device extractors "
  "such as \\DeviceExtractorMOS3Transistor. The constructor of this extractor has a 'factory' parameter "
  "which takes an object of \\DeviceClassFactory type.\n"
  "\n"
  "If such an object is provided, this factory is used "
  "to create the actual device class. The following code shows an example:\n"
  "\n"
  "@code\n"
  "class MyClass < RBA::DeviceClassMOS3Transistor\n"
  "  ... overrides some methods ...\n"
  "end\n"
  "\n"
  "class MyFactory < RBA::DeviceClassFactory\n"
  "  def create_class\n"
  "    MyClass.new\n"
  "  end\n"
  "end\n"
  "\n"
  "extractor = RBA::DeviceExtractorMOS3Transistor::new(\"NMOS\", false, MyFactory.new)\n"
  "@/code\n"
  "\n"
  "When using a factory with a device extractor, make sure it creates a corresponding device class, e.g. "
  "for the \\DeviceExtractorMOS3Transistor extractor create a device class derived from \\DeviceClassMOS3Transistor.\n"
  "\n"
  "This class has been introduced in version 0.27.3.\n"
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

static size_t ld_fallback_index (const db::NetlistDeviceExtractorLayerDefinition *ld)
{
  return ld->fallback_index;
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
  ) +
  gsi::method_ext ("fallback_index", &ld_fallback_index,
    "@brief Gets the index of the fallback layer.\n"
    "This is the index of the layer to be used when this layer isn't specified for input or (more important) output.\n"
  ),
  "@brief Describes a layer used in the device extraction\n"
  "This read-only structure is used to describe a layer in the device extraction.\n"
  "Every device has specific layers used in the device extraction process.\n"
  "Layer definitions can be retrieved using \\NetlistDeviceExtractor#each_layer.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

//  for test only
static void test_initialize (db::NetlistDeviceExtractor *ex, db::Netlist *nl)
{
  ex->initialize (nl);
}

Class<db::NetlistDeviceExtractor> decl_dbNetlistDeviceExtractor ("db", "DeviceExtractorBase",
  gsi::method ("name", &db::NetlistDeviceExtractor::name,
    "@brief Gets the name of the device extractor and the device class."
  ) +
  gsi::method ("name=", &db::NetlistDeviceExtractor::set_name, gsi::arg ("name"),
    "@brief Sets the name of the device extractor and the device class."
  ) +
  gsi::method ("device_class", &db::NetlistDeviceExtractor::device_class,
    "@brief Gets the device class used during extraction\n"
    "The attribute will hold the actual device class used in the device extraction. It "
    "is valid only after 'extract_devices'.\n"
    "\n"
    "This method has been added in version 0.27.3.\n"
  ) +
  gsi::method_ext ("test_initialize", &test_initialize, gsi::arg ("netlist"), "@hide") +   //  for test only
  gsi::iterator ("each_layer_definition", &db::NetlistDeviceExtractor::begin_layer_definitions, &db::NetlistDeviceExtractor::end_layer_definitions,
    "@brief Iterates over all layer definitions."
  ) +
  gsi::iterator ("each_log_entry|#each_error", &db::NetlistDeviceExtractor::begin_log_entries, &db::NetlistDeviceExtractor::end_log_entries,
    "@brief Iterates over all log entries collected in the device extractor."
    "Starting with version 0.28.13, the preferred name of the method is 'each_log_entry' as "
    "log entries have been generalized to become warnings too."
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

template <class Shape>
static void
define_terminal_by_names (GenericDeviceExtractor *extractor, db::Device *device, const std::string &terminal_name, const std::string &layer_name, const Shape &shape)
{
  if (! extractor->device_class ()) {
    throw tl::Exception (tl::to_string (tr ("No device class registered yet")));
  }

  size_t terminal_id = extractor->device_class ()->terminal_id_for_name (terminal_name);

  size_t layer_id = std::numeric_limits<size_t>::max ();
  for (auto l = extractor->begin_layer_definitions (); l != extractor->end_layer_definitions (); ++l) {
    if (l->name == layer_name) {
      layer_id = l->index;
    }
  }

  if (layer_id == std::numeric_limits<size_t>::max ()) {
    throw tl::Exception (tl::to_string (tr ("Not a valid layer name: ")) + layer_name);
  }

  extractor->define_terminal (device, terminal_id, layer_id, shape);
}

Class<GenericDeviceExtractor> decl_GenericDeviceExtractor (decl_dbNetlistDeviceExtractor, "db", "GenericDeviceExtractor",
  gsi::callback ("setup", &GenericDeviceExtractor::setup, &GenericDeviceExtractor::cb_setup,
    "@brief Sets up the extractor.\n"
    "This method is supposed to set up the device extractor. This involves three basic steps:\n"
    "defining the name, the device class and setting up the device layers.\n"
    "\n"
    "Use \\name= to give the extractor and its device class a name.\n"
    "Use \\register_device_class to register the device class you need.\n"
    "Defined the layers by calling \\define_layer once or several times.\n"
  ) +
  gsi::callback ("get_connectivity", &GenericDeviceExtractor::get_connectivity, &GenericDeviceExtractor::cb_get_connectivity,
    gsi::arg ("layout"), gsi::arg ("layers"),
    "@brief Gets the connectivity object used to extract the device geometry.\n"
    "This method shall raise an error, if the input layer are not properly defined (e.g.\n"
    "too few etc.)\n"
    "\n"
    "This is not a connectivity definition in the electrical sense, but defines the cluster of shapes "
    "which generates a specific device. In this case, 'connectivity' means 'definition of shapes that need to touch to form the device'.\n"
    "\n"
    "The 'layers' argument specifies the actual layer layouts for the logical device layers (see \\define_layer). "
    "The list of layers corresponds to the number of layers defined. Use the layer indexes from this list "
    "to build the connectivity with \\Connectivity#connect. Note, that in order to capture a connected cluster of shapes on the "
    "same layer you'll need to include a self-connection like 'connectivity.connect(layers[0], layers[0])'."
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
  gsi::method ("define_layer", (const db::NetlistDeviceExtractorLayerDefinition &(GenericDeviceExtractor::*) (const std::string &name, const std::string &)) &GenericDeviceExtractor::define_layer, gsi::arg ("name"), gsi::arg ("description"),
   "@brief Defines a layer.\n"
   "@return The layer descriptor object created for this layer (use 'index' to get the layer's index)\n"
   "Each call will define one more layer for the device extraction.\n"
   "This method shall be used inside the implementation of \\setup to define\n"
   "the device layers. The actual geometries are later available to \\extract_devices\n"
   "in the order the layers are defined.\n"
  ) +
  gsi::method ("define_opt_layer", (const db::NetlistDeviceExtractorLayerDefinition &(GenericDeviceExtractor::*) (const std::string &name, size_t fallback, const std::string &)) &GenericDeviceExtractor::define_layer, gsi::arg ("name"), gsi::arg ("fallback"), gsi::arg ("description"),
   "@brief Defines a layer with a fallback layer.\n"
   "@return The layer descriptor object created for this layer (use 'index' to get the layer's index)\n"
   "As \\define_layer, this method allows specification of device extraction layer. In addition to \\define_layout, it features "
   "a fallback layer. If in the device extraction statement, the primary layer is not given, "
   "the fallback layer will be used. Hence, this layer is optional. The fallback layer is given by its "
   "index and must be defined before the layer using the fallback layer is defined. "
   "For the index, 0 is the first layer defined, 1 the second and so forth."
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
  gsi::method_ext ("define_terminal", &define_terminal_by_names<db::Polygon>,
    gsi::arg ("device"), gsi::arg ("terminal_name"), gsi::arg ("layer_name"), gsi::arg ("shape"),
   "@brief Defines a device terminal using names for terminal and layer.\n"
   "\n"
   "This convenience version of the ID-based \\define_terminal methods allows using names for terminal and layer.\n"
   "It has been introduced in version 0.28."
  ) +
  gsi::method_ext ("define_terminal", &define_terminal_by_names<db::Box>,
    gsi::arg ("device"), gsi::arg ("terminal_name"), gsi::arg ("layer_name"), gsi::arg ("shape"),
   "@brief Defines a device terminal using names for terminal and layer.\n"
   "\n"
   "This convenience version of the ID-based \\define_terminal methods allows using names for terminal and layer.\n"
   "It has been introduced in version 0.28."
  ) +
  gsi::method_ext ("define_terminal", &define_terminal_by_names<db::Point>,
    gsi::arg ("device"), gsi::arg ("terminal_name"), gsi::arg ("layer_name"), gsi::arg ("point"),
   "@brief Defines a device terminal using names for terminal and layer.\n"
   "\n"
   "This convenience version of the ID-based \\define_terminal methods allows using names for terminal and layer.\n"
   "It has been introduced in version 0.28."
  ) +
  gsi::method ("dbu", &GenericDeviceExtractor::dbu,
   "@brief Gets the database unit\n"
  ) +
  gsi::method ("sdbu", &GenericDeviceExtractor::sdbu,
   "@brief Gets the scaled database unit\n"
   "Use this unit to compute device properties. It is the database unit multiplied with the\n"
   "device scaling factor."
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
   "@brief Issues an error with the given message and database-unit polygon geometry\n"
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
   "@brief Issues an error with the given category name and description, message and database-unit polygon geometry\n"
  ) +
  gsi::method ("warn", (void (GenericDeviceExtractor::*) (const std::string &)) &GenericDeviceExtractor::warn,
    gsi::arg ("message"),
    "@brief Issues a warning with the given message\n"
    "Warnings have been introduced in version 0.28.13."
  ) +
  gsi::method ("warn", (void (GenericDeviceExtractor::*) (const std::string &, const db::DPolygon &)) &GenericDeviceExtractor::warn,
    gsi::arg ("message"), gsi::arg ("geometry"),
    "@brief Issues a warning with the given message and micrometer-units polygon geometry\n"
    "Warnings have been introduced in version 0.28.13."
  ) +
  gsi::method ("warn", (void (GenericDeviceExtractor::*) (const std::string &, const db::Polygon &)) &GenericDeviceExtractor::warn,
    gsi::arg ("message"), gsi::arg ("geometry"),
    "@brief Issues a warning with the given message and database-unit polygon geometry\n"
    "Warnings have been introduced in version 0.28.13."
  ) +
  gsi::method ("warn", (void (GenericDeviceExtractor::*) (const std::string &, const std::string &, const std::string &)) &GenericDeviceExtractor::warn,
    gsi::arg ("category_name"), gsi::arg ("category_description"), gsi::arg ("message"),
    "@brief Issues a warning with the given category name and description, message\n"
    "Warnings have been introduced in version 0.28.13."
  ) +
  gsi::method ("warn", (void (GenericDeviceExtractor::*) (const std::string &, const std::string &, const std::string &, const db::DPolygon &)) &GenericDeviceExtractor::warn,
    gsi::arg ("category_name"), gsi::arg ("category_description"), gsi::arg ("message"), gsi::arg ("geometry"),
    "@brief Issues a warning with the given category name and description, message and micrometer-units polygon geometry\n"
    "Warnings have been introduced in version 0.28.13."
  ) +
  gsi::method ("warn", (void (GenericDeviceExtractor::*) (const std::string &, const std::string &, const std::string &, const db::Polygon &)) &GenericDeviceExtractor::warn,
    gsi::arg ("category_name"), gsi::arg ("category_description"), gsi::arg ("message"), gsi::arg ("geometry"),
    "@brief Issues a warning with the given category name and description, message and database-unit polygon geometry\n"
    "Warnings have been introduced in version 0.28.13."
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
  "      output layers for defining the terminals. Terminals are the points at which the nets connect to the devices.\n"
  "      Layers are defined using \\define_layer. Initially, layers are abstract definitions with a name and a description.\n"
  "      Concrete layers will be given when defining the connectivity. @/li\n"
  "@/ul\n"
  "\n"
  "When the device extraction is started, the device extraction algorithm will first ask the device extractor "
  "for the 'connectivity'. This is not a connectivity in a sense of electrical connections. The connectivity defines are "
  "logical compound that makes up the device. 'Connected' shapes are collected and presented to the device extractor.\n"
  "The connectivity is obtained by calling \\get_connectivity. This method must be "
  "implemented to produce the connectivity.\n"
  "\n"
  "Finally, the individual devices need to be extracted. Each cluster of connected shapes is presented to the "
  "device extractor. A cluster may include more than one device. It's the device extractor's responsibility to "
  "extract the devices from this cluster and deliver the devices through \\create_device. In addition, terminals "
  "have to be defined, so the net extractor can connect to the devices. Terminal definitions are made through "
  "\\define_terminal. The device extraction is implemented in the \\extract_devices method.\n"
  "\n"
  "If errors occur during device extraction, the \\error method may be used to issue such errors. Errors "
  "reported this way are kept in the error log.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

static db::NetlistDeviceExtractorMOS3Transistor *make_mos3_extractor (const std::string &name, bool strict, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorMOS3Transistor (name, strict, factory);
}

Class<db::NetlistDeviceExtractorMOS3Transistor> decl_NetlistDeviceExtractorMOS3Transistor (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorMOS3Transistor",
  gsi::constructor ("new", &make_mos3_extractor, gsi::arg ("name"), gsi::arg ("strict", false), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name.\n"
    "If \\strict is true, the MOS device extraction will happen in strict mode. That is, source and drain "
    "are not interchangeable.\n"
    "\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ) +
  gsi::method ("strict?", &db::NetlistDeviceExtractorMOS3Transistor::is_strict,
    "@brief Returns a value indicating whether extraction happens in strict mode."
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
  "\n"
  "The extractor delivers six parameters:\n"
  "\n"
  "@ul\n"
  "@li 'L' - the gate length in micrometer units @/li\n"
  "@li 'W' - the gate width in micrometer units @/li\n"
  "@li 'AS' and 'AD' - the source and drain region areas in square micrometers @/li\n"
  "@li 'PS' and 'PD' - the source and drain region perimeters in micrometer units @/li\n"
  "@/ul\n"
  "\n"
  "The device layer names are:\n"
  "\n"
  "@ul\n"
  "@li In strict mode: 'S' (source), 'D' (drain) and 'G' (gate). @/li\n"
  "@li In non-strict mode: 'SD' (source and drain) and 'G' (gate). @/li\n"
  "@/ul\n"
  "\n"
  "The terminals are output on these layers:\n"
  "@ul\n"
  "@li 'tS' - source. Default output is 'S' (strict mode) or 'SD' (otherwise). @/li\n"
  "@li 'tD' - drain. Default output is 'D' (strict mode) or 'SD' (otherwise). @/li\n"
  "@li 'tG' - gate. Default output is 'G'. @/li\n"
  "@/ul\n"
  "\n"
  "The source/drain (diffusion) area is distributed on the number of gates connecting to\n"
  "the particular source or drain area.\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

static db::NetlistDeviceExtractorMOS4Transistor *make_mos4_extractor (const std::string &name, bool strict, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorMOS4Transistor (name, strict, factory);
}

Class<db::NetlistDeviceExtractorMOS4Transistor> decl_NetlistDeviceExtractorMOS4Transistor (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorMOS4Transistor",
  gsi::constructor ("new", &make_mos4_extractor, gsi::arg ("name"), gsi::arg ("strict", false), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ),
  "@brief A device extractor for a four-terminal MOS transistor\n"
  "\n"
  "This class supplies the generic extractor for a MOS device.\n"
  "It is based on the \\DeviceExtractorMOS3Transistor class with the extension of a bulk terminal "
  "and corresponding bulk terminal output (annotation) layer.\n"
  "\n"
  "The parameters of a MOS4 device are the same than for MOS3 devices. For the device "
  "layers the bulk layer is added.\n"
  "\n"
  "@ul\n"
  "@li 'B' (bulk) - currently this layer is not used and can be empty. @/li\n"
  "@/ul\n"
  "\n"
  "The bulk terminals are output on this layer:\n"
  "@ul\n"
  "@li 'tB' - bulk terminal (a copy of the gate shape). Default output is 'B'. @/li\n"
  "@/ul\n"
  "\n"
  "The bulk terminal layer can be empty. In this case, it needs \n"
  "to be connected to a global net to establish the net connection.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassMOS4Transistor.\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorResistor *make_res_extractor (const std::string &name, double sheet_rho, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorResistor (name, sheet_rho, factory);
}

Class<db::NetlistDeviceExtractorResistor> decl_NetlistDeviceExtractorResistor (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorResistor",
  gsi::constructor ("new", &make_res_extractor, gsi::arg ("name"), gsi::arg ("sheet_rho"), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ),
  "@brief A device extractor for a two-terminal resistor\n"
  "\n"
  "This class supplies the generic extractor for a resistor device.\n"
  "The device is defined by two geometry layers: the resistor 'wire' and "
  "two contacts per wire. The contacts should be attached to the ends "
  "of the wire. The wire length and width is computed from the "
  "edge lengths between the contacts and along the contacts respectively.\n"
  "\n"
  "This simple computation is precise only when the resistor shape is "
  "a rectangle.\n"
  "\n"
  "Using the given sheet resistance, the resistance value is computed by "
  "'R = L / W * sheet_rho'.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassResistor.\n"
  "The extractor produces three parameters:\n"
  "\n"
  "@ul\n"
  "@li 'R' - the resistance in Ohm @/li\n"
  "@li 'A' - the resistor's area in square micrometer units @/li\n"
  "@li 'P' - the resistor's perimeter in micrometer units @/li\n"
  "@/ul\n"
  "\n"
  "The device layer names are:\n"
  "\n"
  "@ul\n"
  "@li 'R' - resistor path. This is the geometry that defines the resistor's current path. @/li\n"
  "@li 'C' - contacts. These areas form the contact regions at the ends of the resistor path. @/li\n"
  "@/ul\n"
  "\n"
  "The terminals are output on these layers:\n"
  "@ul\n"
  "@li 'tA', 'tB' - the two terminals of the resistor. @/li\n"
  "@/ul\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorResistorWithBulk *make_res_with_bulk_extractor (const std::string &name, double sheet_rho, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorResistorWithBulk (name, sheet_rho, factory);
}

Class<db::NetlistDeviceExtractorResistorWithBulk> decl_NetlistDeviceExtractorResistorWithBulk (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorResistorWithBulk",
  gsi::constructor ("new", &make_res_with_bulk_extractor, gsi::arg ("name"), gsi::arg ("sheet_rho"), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ),
  "@brief A device extractor for a resistor with a bulk terminal\n"
  "\n"
  "This class supplies the generic extractor for a resistor device including a bulk terminal.\n"
  "The device is defined the same way than devices are defined for \\DeviceExtractorResistor.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassResistorWithBulk.\n"
  "The extractor produces three parameters:\n"
  "\n"
  "@ul\n"
  "@li 'R' - the resistance in Ohm @/li\n"
  "@li 'A' - the resistor's area in square micrometer units @/li\n"
  "@li 'P' - the resistor's perimeter in micrometer units @/li\n"
  "@/ul\n"
  "\n"
  "The device layer names are:\n"
  "\n"
  "@ul\n"
  "@li 'R' - resistor path. This is the geometry that defines the resistor's current path. @/li\n"
  "@li 'C' - contacts. These areas form the contact regions at the ends of the resistor path. @/li\n"
  "@li 'W' - well, bulk. Currently this layer is ignored for the extraction and can be empty. @/li\n"
  "@/ul\n"
  "\n"
  "The terminals are output on these layers:\n"
  "@ul\n"
  "@li 'tA', 'tB' - the two terminals of the resistor. @/li\n"
  "@li 'tW' - the bulk terminal (copy of the resistor area). @/li\n"
  "@/ul\n"
  "\n"
  "The bulk terminal layer can be an empty layer representing the substrate. In this case, it needs to be connected globally.\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorCapacitor *make_cap_extractor (const std::string &name, double area_cap, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorCapacitor (name, area_cap, factory);
}

Class<db::NetlistDeviceExtractorCapacitor> decl_NetlistDeviceExtractorCapacitor (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorCapacitor",
  gsi::constructor ("new", &make_cap_extractor, gsi::arg ("name"), gsi::arg ("area_cap"), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ),
  "@brief A device extractor for a two-terminal capacitor\n"
  "\n"
  "This class supplies the generic extractor for a capacitor device.\n"
  "The device is defined by two geometry layers forming the 'plates' of the capacitor.\n"
  "The capacitance is computed from the overlapping area of the plates "
  "using 'C = A * area_cap' (area_cap is the capacitance per square micrometer area).\n"
  "\n"
  "Although 'area_cap' can be given in any unit, Farad should be preferred as this is the "
  "convention used for output into a netlist.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassCapacitor.\n"
  "The extractor produces three parameters:\n"
  "\n"
  "@ul\n"
  "@li 'C' - the capacitance @/li\n"
  "@li 'A' - the capacitor's area in square micrometer units @/li\n"
  "@li 'P' - the capacitor's perimeter in micrometer units @/li\n"
  "@/ul\n"
  "\n"
  "The device layer names are:\n"
  "\n"
  "@ul\n"
  "@li 'P1', 'P2' - the two plates. @/li\n"
  "@/ul\n"
  "\n"
  "The terminals are output on these layers:\n"
  "@ul\n"
  "@li 'tA', 'tB' - the two terminals. Defaults to 'P1' and 'P2'. @/li\n"
  "@/ul\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorCapacitorWithBulk *make_cap_with_bulk_extractor (const std::string &name, double area_cap, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorCapacitorWithBulk (name, area_cap, factory);
}

Class<db::NetlistDeviceExtractorCapacitorWithBulk> decl_NetlistDeviceExtractorCapacitorWithBulk (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorCapacitorWithBulk",
  gsi::constructor ("new", &make_cap_with_bulk_extractor, gsi::arg ("name"), gsi::arg ("sheet_rho"), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ),
  "@brief A device extractor for a capacitor with a bulk terminal\n"
  "\n"
  "This class supplies the generic extractor for a capacitor device including a bulk terminal.\n"
  "The device is defined the same way than devices are defined for \\DeviceExtractorCapacitor.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassCapacitorWithBulk.\n"
  "The extractor produces three parameters:\n"
  "\n"
  "@ul\n"
  "@li 'C' - the capacitance @/li\n"
  "@li 'A' - the capacitor's area in square micrometer units @/li\n"
  "@li 'P' - the capacitor's perimeter in micrometer units @/li\n"
  "@/ul\n"
  "\n"
  "The device layer names are:\n"
  "\n"
  "@ul\n"
  "@li 'P1', 'P2' - the two plates. @/li\n"
  "@li 'W' - well, bulk. Currently this layer is ignored for the extraction and can be empty. @/li\n"
  "@/ul\n"
  "\n"
  "The terminals are output on these layers:\n"
  "@ul\n"
  "@li 'tA', 'tB' - the two terminals. Defaults to 'P1' and 'P2'. @/li\n"
  "@li 'tW' - the bulk terminal (copy of the resistor area). @/li\n"
  "@/ul\n"
  "\n"
  "The bulk terminal layer can be an empty layer representing the substrate. In this case, it needs to be connected globally.\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorBJT3Transistor *make_bjt3_extractor (const std::string &name, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorBJT3Transistor (name, factory);
}

Class<db::NetlistDeviceExtractorBJT3Transistor> decl_dbNetlistDeviceExtractorBJT3Transistor (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorBJT3Transistor",
  gsi::constructor ("new", &make_bjt3_extractor, gsi::arg ("name"), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ),
  "@brief A device extractor for a bipolar transistor (BJT)\n"
  "\n"
  "This class supplies the generic extractor for a bipolar transistor device.\n"
  "\n"
  "Extraction of vertical and lateral transistors is supported through a generic geometry model: "
  "The basic area is the base area. A marker shape must be provided for this area. "
  "The emitter of the transistor is defined by emitter layer shapes inside the base area. "
  "Multiple emitter shapes can be present. In this case, multiple transistor devices sharing the "
  "same base and collector are generated.\n"
  "Finally, a collector layer can be given. If non-empty, the parts inside the base region will define "
  "the collector terminals. If empty, the collector is formed by the substrate. In this case, the base "
  "region will be output to the 'tC' terminal output layer. This layer then needs to be connected to a global net "
  "to form the net connection.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassBJT3Transistor.\n"
  "The extractor delivers these parameters:\n"
  "\n"
  "@ul\n"
  "@li 'AE', 'AB' and 'AC' - the emitter, base and collector areas in square micrometer units @/li\n"
  "@li 'PE', 'PB' and 'PC' - the emitter, base and collector perimeters in micrometer units @/li\n"
  "@li 'NE' - emitter count (initially 1 but increases when devices are combined) @/li\n"
  "@/ul\n"
  "\n"
  "The device layer names are:\n"
  "\n"
  "@ul\n"
  "@li 'E' - emitter. @/li\n"
  "@li 'B' - base. @/li\n"
  "@li 'C' - collector. @/li\n"
  "@/ul\n"
  "\n"
  "The terminals are output on these layers:\n"
  "@ul\n"
  "@li 'tE' - emitter. Default output is 'E'. @/li\n"
  "@li 'tB' - base. Default output is 'B'. @/li\n"
  "@li 'tC' - collector. Default output is 'C'. @/li\n"
  "@/ul\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorBJT4Transistor *make_bjt4_extractor (const std::string &name, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorBJT4Transistor (name, factory);
}

Class<db::NetlistDeviceExtractorBJT4Transistor> decl_NetlistDeviceExtractorBJT4Transistor (decl_dbNetlistDeviceExtractorBJT3Transistor, "db", "DeviceExtractorBJT4Transistor",
  gsi::constructor ("new", &make_bjt4_extractor, gsi::arg ("name"), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ),
  "@brief A device extractor for a four-terminal bipolar transistor (BJT)\n"
  "\n"
  "This class supplies the generic extractor for a bipolar transistor device.\n"
  "It is based on the \\DeviceExtractorBJT3Transistor class with the extension of a substrate terminal "
  "and corresponding substrate terminal output (annotation) layer.\n"
  "\n"
  "Two new layers are introduced:\n"
  "\n"
  "@ul\n"
  "@li 'S' - the bulk (substrate) layer. Currently this layer is ignored and can be empty. @/li"
  "@li 'tS' - the bulk terminal output layer (defaults to 'S'). @/li"
  "@/ul\n"
  "\n"
  "The bulk terminal layer ('tS') can be an empty layer representing the wafer substrate.\n"
  "In this use mode the substrate terminal shapes will be produced on the 'tS' layer. This\n"
  "layer then needs to be connected to a global net to establish the net connection.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassBJT4Transistor.\n"
  "The "
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

db::NetlistDeviceExtractorDiode *make_diode_extractor (const std::string &name, DeviceClassFactoryImpl *factory)
{
  return new db::NetlistDeviceExtractorDiode (name, factory);
}

Class<db::NetlistDeviceExtractorDiode> decl_NetlistDeviceExtractorDiode (decl_dbNetlistDeviceExtractor, "db", "DeviceExtractorDiode",
  gsi::constructor ("new", &make_diode_extractor, gsi::arg ("name"), gsi::arg ("factory", (DeviceClassFactoryImpl *)0, "none"),
    "@brief Creates a new device extractor with the given name\n"
    "For the 'factory' parameter see \\DeviceClassFactory. It has been added in version 0.27.3.\n"
  ),
  "@brief A device extractor for a planar diode\n"
  "\n"
  "This class supplies the generic extractor for a planar diode.\n"
  "The diode is defined by two layers whose overlap area forms\n"
  "the diode. The p-type layer forms the anode, the n-type layer\n"
  "the cathode.\n"
  "\n"
  "The device class produced by this extractor is \\DeviceClassDiode.\n"
  "The extractor extracts the two parameters of this class:\n"
  "\n"
  "@ul\n"
  "@li 'A' - the diode area in square micrometer units. @/li\n"
  "@li 'P' - the diode perimeter in micrometer units. @/li\n"
  "@/ul\n"
  "\n"
  "The device layers are:\n"
  "\n"
  "@ul\n"
  "@li 'P' - the p doped area. @/li\n"
  "@li 'N' - the n doped area. @/li\n"
  "@/ul\n"
  "\n"
  "The diode region is defined by the overlap of p and n regions.\n"
  "\n"
  "The terminal output layers are:\n"
  "\n"
  "@ul\n"
  "@li 'tA' - anode. Defaults to 'P'. @/li\n"
  "@li 'tC' - cathode. Defaults to 'N'. @/li\n"
  "@/ul\n"
  "\n"
  "This class is a closed one and methods cannot be reimplemented. To reimplement "
  "specific methods, see \\DeviceExtractor.\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}
