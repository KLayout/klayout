
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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
#include "gsiEnums.h"
#include "dbLayoutToNetlist.h"
#include "tlStream.h"
#include "tlVariant.h"

namespace gsi
{

static db::LayoutToNetlist *make_l2n (const db::RecursiveShapeIterator &iter)
{
  return new db::LayoutToNetlist (iter);
}

static db::LayoutToNetlist *make_l2n_default ()
{
  return new db::LayoutToNetlist ();
}

static db::LayoutToNetlist *make_l2n_from_existing_dss_with_layout (db::DeepShapeStore *dss, unsigned int layout_index)
{
  return new db::LayoutToNetlist (dss, layout_index);
}

static db::LayoutToNetlist *make_l2n_flat (const std::string &topcell_name, double dbu)
{
  return new db::LayoutToNetlist (topcell_name, dbu);
}

static db::Layout *l2n_internal_layout (db::LayoutToNetlist *l2n)
{
  return l2n->internal_layout ();
}

static db::Cell *l2n_internal_top_cell (db::LayoutToNetlist *l2n)
{
  return l2n->internal_top_cell ();
}

static db::Layout *l2n_original_layout (db::LayoutToNetlist *l2n)
{
  return l2n->original_layout ();
}

static db::Cell *l2n_original_top_cell (db::LayoutToNetlist *l2n)
{
  return l2n->original_top_cell ();
}

static tl::Variant l2n_layer_index_by_name (const db::LayoutToNetlist *l2n, const std::string &name)
{
  tl::optional<unsigned int> index = l2n->layer_index_by_name (name);
  if (index.has_value ()) {
    return tl::Variant (index.value ());
  } else {
    return tl::Variant ();
  }
}

static std::map<unsigned int, unsigned int> layer_map_from_var (const db::LayoutToNetlist *l2n, db::Layout &target, const tl::Variant &lmap)
{
  if (lmap.is_nil ()) {

    return l2n->create_layermap (target);

  } else if (! lmap.is_array ()) {

    throw tl::Exception (tl::to_string (tr ("'lmap' argument needs to be nil or a hash")));

  } else {

    std::map<unsigned int, unsigned int> res;

    for (auto kv = lmap.begin_array (); kv != lmap.end_array (); ++kv) {

      const tl::Variant &k = kv->first;
      const tl::Variant &v = kv->second;

      unsigned int ki = k.to_uint ();
      if (v.is_user ()) {

        unsigned int vi = 0;
        if (dynamic_cast<const tl::VariantUserClass<db::Region> *> (v.user_cls ()) != 0) {
          vi = l2n->layer_of<db::ShapeCollection> (v.to_user<db::Region> ());
        } else if (dynamic_cast<const tl::VariantUserClass<db::Texts> *> (v.user_cls ()) != 0) {
          vi = l2n->layer_of<db::ShapeCollection> (v.to_user<db::Texts> ());
        } else {
          throw tl::Exception (tl::to_string (tr ("'lmap' argument hash values need to be ints, Region or Texts objects")));
        }
        res.insert (std::make_pair (ki, vi));

      } else if (! v.is_nil ()) {

        res.insert (std::make_pair (ki, v.to_uint ()));

      }

    }

    return res;

  }
}

static void build_net (const db::LayoutToNetlist *l2n, const db::Net &net, db::Layout &target, db::Cell &target_cell, const tl::Variant &lmap, const tl::Variant &netname_prop, db::BuildNetHierarchyMode hier_mode, const tl::Variant &circuit_cell_name_prefix, const tl::Variant &device_cell_name_prefix)
{
  std::string p = circuit_cell_name_prefix.to_string ();
  std::string dp = device_cell_name_prefix.to_string ();
  l2n->build_net (net, target, target_cell, layer_map_from_var (l2n, target, lmap), db::NPM_AllProperties, netname_prop, hier_mode, circuit_cell_name_prefix.is_nil () ? 0 : p.c_str (), device_cell_name_prefix.is_nil () ? 0 : dp.c_str ());
}

static void build_all_nets (const db::LayoutToNetlist *l2n, const db::CellMapping &cmap, db::Layout &target, const tl::Variant &lmap, const tl::Variant &net_cell_name_prefix, const tl::Variant &netname_prop, db::BuildNetHierarchyMode hier_mode, const tl::Variant &circuit_cell_name_prefix, const tl::Variant &device_cell_name_prefix)
{
  std::string cp = circuit_cell_name_prefix.to_string ();
  std::string np = net_cell_name_prefix.to_string ();
  std::string dp = device_cell_name_prefix.to_string ();
  l2n->build_all_nets (cmap, target, layer_map_from_var (l2n, target, lmap), net_cell_name_prefix.is_nil () ? 0 : np.c_str (), db::NPM_AllProperties, netname_prop, hier_mode, circuit_cell_name_prefix.is_nil () ? 0 : cp.c_str (), device_cell_name_prefix.is_nil () ? 0 : dp.c_str ());
}

static void build_nets (const db::LayoutToNetlist *l2n, const std::vector<const db::Net *> &nets, const db::CellMapping &cmap, db::Layout &target, const tl::Variant &lmap, const tl::Variant &net_cell_name_prefix, const tl::Variant &netname_prop, db::BuildNetHierarchyMode hier_mode, const tl::Variant &circuit_cell_name_prefix, const tl::Variant &device_cell_name_prefix)
{
  std::string cp = circuit_cell_name_prefix.to_string ();
  std::string np = net_cell_name_prefix.to_string ();
  std::string dp = device_cell_name_prefix.to_string ();
  l2n->build_nets (&nets, cmap, target, layer_map_from_var (l2n, target, lmap), net_cell_name_prefix.is_nil () ? 0 : np.c_str (), db::NPM_AllProperties, netname_prop, hier_mode, circuit_cell_name_prefix.is_nil () ? 0 : cp.c_str (), device_cell_name_prefix.is_nil () ? 0 : dp.c_str ());
}

static std::vector<std::string> l2n_layer_names (const db::LayoutToNetlist *l2n)
{
  std::vector<std::string> ln;
  for (db::LayoutToNetlist::layer_iterator l = l2n->begin_layers (); l != l2n->end_layers (); ++l) {
    ln.push_back (l->second);
  }
  return ln;
}

static std::vector<unsigned int> l2n_layer_indexes (const db::LayoutToNetlist *l2n)
{
  std::vector<unsigned int> li;
  for (db::LayoutToNetlist::layer_iterator l = l2n->begin_layers (); l != l2n->end_layers (); ++l) {
    li.push_back (l->first);
  }
  return li;
}

static db::LayerProperties l2n_layer_info (const db::LayoutToNetlist *l2n, unsigned int layer)
{
  if (! l2n->internal_layout () || ! l2n->internal_layout ()->is_valid_layer (layer)) {
    return db::LayerProperties ();
  } else {
    return l2n->internal_layout ()->get_properties (layer);
  }
}

static db::Region antenna_check3 (db::LayoutToNetlist *l2n, const db::Region &poly, double poly_area_factor, double poly_perimeter_factor, const db::Region &metal, double metal_area_factor, double metal_perimeter_factor, double ratio, const std::vector<tl::Variant> &diodes, db::Texts *texts)
{
  std::vector<std::pair<const db::Region *, double> > diode_pairs;

  for (std::vector<tl::Variant>::const_iterator d = diodes.begin (); d != diodes.end (); ++d) {

    if (d->is_user<db::Region> ()) {

      diode_pairs.push_back (std::make_pair (& d->to_user<db::Region> (), 0.0));

    } else if (d->is_list ()) {

      const std::vector<tl::Variant> &list = d->get_list ();
      if (list.size () != 2) {
        throw tl::Exception (tl::to_string (tr ("Diode layer specifications of 'antenna' method require list of diode layer/ratio pairs (e.g. '[ [ diode_layer, 10.0 ], ... ]')")));
      }
      if (! list [0].is_user<db::Region> ()) {
        throw tl::Exception (tl::to_string (tr ("Diode layer specifications of 'antenna' method require list of diode layer/ratio pairs (e.g. '[ [ diode_layer, 10.0 ], ... ]') - first element isn't a Region object")));
      }
      if (! list [1].can_convert_to_double ()) {
        throw tl::Exception (tl::to_string (tr ("Diode layer specifications of 'antenna' method require list of diode layer/ratio pairs (e.g. '[ [ diode_layer, 10.0 ], ... ]') - second element isn't a number")));
      }

      diode_pairs.push_back (std::make_pair (& list [0].to_user<db::Region> (), list [1].to_double ()));

    }

  }

  return l2n->antenna_check (poly, poly_area_factor, poly_perimeter_factor, metal, metal_area_factor, metal_perimeter_factor, ratio, diode_pairs, texts);
}

static db::Region antenna_check2 (db::LayoutToNetlist *l2n, const db::Region &poly, double poly_perimeter_factor, const db::Region &metal, double metal_perimeter_factor, double ratio, const std::vector<tl::Variant> &diodes, db::Texts *texts)
{
  return antenna_check3 (l2n, poly, 1, poly_perimeter_factor, metal, 1, metal_perimeter_factor, ratio, diodes, texts);
}

static db::Region antenna_check (db::LayoutToNetlist *l2n, const db::Region &poly, const db::Region &metal, double ratio, const std::vector<tl::Variant> &diodes, db::Texts *texts)
{
  return antenna_check3 (l2n, poly, 1, 0, metal, 1, 0, ratio, diodes, texts);
}

static db::Region evaluate_nets (db::LayoutToNetlist *l2n, const db::Region &primary, const std::map<std::string, const db::Region *> &secondary, const std::string &expression, const tl::Variant &variables, const tl::Variant &dbu)
{
  if (! variables.is_array ()) {
    throw tl::Exception (tl::to_string (tr ("'variables' argument needs to a hash")));
  }

  std::map<std::string, tl::Variant> variables_map;
  for (auto v = variables.begin_array (); v != variables.end_array (); ++v) {
    variables_map [v->first.to_string ()] = v->second;
  }

  return l2n->measure_net (primary, secondary, expression, variables_map, dbu.is_nil () ? -1.0 : dbu.to_float ());
}

static void join_net_names (db::LayoutToNetlist *l2n, const std::string &s)
{
  l2n->join_net_names (tl::GlobPattern (s));
}

static std::string dump_joined_net_names (const db::LayoutToNetlist *l2n)
{
  const std::list<tl::GlobPattern> &jn = l2n->joined_net_names ();
  std::vector<std::string> s;
  for (std::list<tl::GlobPattern>::const_iterator j = jn.begin (); j != jn.end (); ++j) {
    s.push_back (j->pattern ());
  }
  return tl::join (s, ",");
}

static void join_net_names2 (db::LayoutToNetlist *l2n, const std::string &c, const std::string &s)
{
  l2n->join_net_names (tl::GlobPattern (c), tl::GlobPattern (s));
}

static std::string dump_joined_net_names_per_cell (const db::LayoutToNetlist *l2n)
{
  const std::list<std::pair<tl::GlobPattern, tl::GlobPattern> > &jn = l2n->joined_net_names_per_cell ();
  std::vector<std::string> s;
  for (std::list<std::pair<tl::GlobPattern, tl::GlobPattern> >::const_iterator i = jn.begin (); i != jn.end (); ++i) {
    s.push_back (i->first.pattern () + ":" + i->second.pattern ());
  }
  return tl::join (s, ",");
}

static void join_nets (db::LayoutToNetlist *l2n, const std::set<std::string> &s)
{
  l2n->join_nets (s);
}

static std::string dump_joined_nets (const db::LayoutToNetlist *l2n)
{
  const std::list<std::set<std::string> > &jn = l2n->joined_nets ();
  std::vector<std::string> s;
  for (std::list<std::set<std::string> >::const_iterator j = jn.begin (); j != jn.end (); ++j) {
    std::vector<std::string> t (j->begin (), j->end ());
    s.push_back (tl::join (t, "+"));
  }
  return tl::join (s, ",");
}

static void join_nets2 (db::LayoutToNetlist *l2n, const std::string &c, const std::set<std::string> &s)
{
  l2n->join_nets (tl::GlobPattern (c), s);
}

static std::string dump_joined_nets_per_cell (const db::LayoutToNetlist *l2n)
{
  const std::list<std::pair<tl::GlobPattern, std::set<std::string> > > &jn = l2n->joined_nets_per_cell ();
  std::vector<std::string> s;
  for (std::list<std::pair<tl::GlobPattern, std::set<std::string> > >::const_iterator i = jn.begin (); i != jn.end (); ++i) {
    std::vector<std::string> t (i->second.begin (), i->second.end ());
    s.push_back (i->first.pattern () + ":" + tl::join (t, "+"));
  }
  return tl::join (s, ",");
}

Class<db::LayoutToNetlist> decl_dbLayoutToNetlist ("db", "LayoutToNetlist",
  gsi::constructor ("new", &make_l2n, gsi::arg ("iter"),
    "@brief Creates a new extractor connected to an original layout\n"
    "This constructor will attach the extractor to an original layout through the "
    "shape iterator.\n"
    "\n"
    "The shape iterator does not need to be an actual shape iterator. It is merely used to identify the original "
    "layout and provides additional parameters such as the top cell to use and advanced options such as subtree pruning.\n"
    "\n"
    "You can construct a dummy iterator usable for this purpose without layers using an empty layer set:\n"
    "\n"
    "@code\n"
    "ly = ...  # external layout\n"
    "l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))"
    "..."
    "@/code\n"
  ) +
  gsi::constructor ("new", &make_l2n_default,
    "@brief Creates a new and empty extractor object\n"
    "The main objective for this constructor is to create an object suitable for reading an annotated netlist.\n"
    "A default-created LayoutToNetlist object is not very useful for other purposes."
  ) +
  gsi::constructor ("new", &make_l2n_from_existing_dss_with_layout, gsi::arg ("dss"), gsi::arg ("layout_index", 0),
    "@brief Creates a new extractor object reusing an existing \\DeepShapeStore object\n"
    "This constructor can be used if there is a DSS object already from which the "
    "shapes can be taken. This version can only be used with \\register to "
    "add layers (regions) inside the 'dss' object.\n"
    "\n"
    "The make_... methods will not create new layers as there is no particular place "
    "defined where to create the layers.\n"
    "\n"
    "The extractor will not take ownership of the dss object unless you call \\keep_dss."
  ) +
  gsi::constructor ("new", &make_l2n_flat, gsi::arg ("topcell_name"), gsi::arg ("dbu"),
    "@brief Creates a new, detached extractor object with a flat DSS\n"
    "@param topcell_name The name of the top cell of the internal flat layout\n"
    "@param dbu The database unit to use for the internal flat layout\n"
    "\n"
    "This constructor will create an extractor for flat extraction. Layers registered "
    "with \\register will be flattened. New layers created with make_... will be flat "
    "layers.\n"
    "\n"
    "The database unit is mandatory because the physical parameter extraction "
    "for devices requires this unit for translation of layout to physical dimensions.\n"
    "\n"
    "Example:\n"
    "\n"
    "@code\n"
    "rmetal1 = ...  # an external Region object representing 'metal1' layer\n"
    "rvia11  = ...  # an external Region object representing 'via1' layer\n"
    "rmetal2 = ...  # an external Region object representing 'metal2' layer\n"
    "\n"
    "l2n = RBA::LayoutToNetlist::new(\"TOP_CELL\", 0.001)\n"
    "# imports the external Regions as flat ones and assigns proper names\n"
    "l2n.register(rmetal1, \"metal1\")\n"
    "l2n.register(rvia1, \"via1\")\n"
    "l2n.register(rmetal2, \"metal2\")\n"
    "\n"
    "# intra- and inter-layer connects:\n"
    "l2n.connect(metal1)\n"
    "l2n.connect(metal1, via1)\n"
    "l2n.connect(metal2)\n"
    "@/code\n"
  ) +
  gsi::method ("generator", &db::LayoutToNetlist::generator,
    "@brief Gets the generator string.\n"
    "The generator is the script that created this database.\n"
  ) +
  gsi::method ("generator=", &db::LayoutToNetlist::set_generator, gsi::arg ("generator"),
    "@brief Sets the generator string.\n"
  ) +
  gsi::method ("dss", (db::DeepShapeStore &(db::LayoutToNetlist::*) ()) &db::LayoutToNetlist::dss,
    "@brief Gets a reference to the internal DSS object.\n"
    "See the class description for details about the DSS object used inside the LayoutToNetlist object."
  ) +
  gsi::method ("keep_dss", &db::LayoutToNetlist::keep_dss,
    "@brief Takes ownership of the DSS object if the LayoutToNetlist object was created with an external one.\n"
    "See the class description for details about the DSS object used inside the LayoutToNetlist object."
  ) +
  gsi::method ("threads=", &db::LayoutToNetlist::set_threads, gsi::arg ("n"),
    "@brief Sets the number of threads to use for operations which support multiple threads\n"
  ) +
  gsi::method ("threads", &db::LayoutToNetlist::threads,
    "@brief Gets the number of threads to use for operations which support multiple threads\n"
  ) +
  gsi::method ("area_ratio=", &db::LayoutToNetlist::set_area_ratio, gsi::arg ("r"),
    "@brief Sets the area_ratio parameter for the hierarchical network processor\n"
    "This parameter controls splitting of large polygons in order to reduce the\n"
    "error made by the bounding box approximation.\n"
  ) +
  gsi::method ("area_ratio", &db::LayoutToNetlist::area_ratio,
    "@brief Gets the area_ratio parameter for the hierarchical network processor\n"
    "See \\area_ratio= for details about this attribute."
  ) +
  gsi::method ("max_vertex_count=", &db::LayoutToNetlist::set_max_vertex_count, gsi::arg ("n"),
    "@brief Sets the max_vertex_count parameter for the hierarchical network processor\n"
    "This parameter controls splitting of large polygons in order to enhance performance\n"
    "for very big polygons.\n"
  ) +
  gsi::method ("max_vertex_count", &db::LayoutToNetlist::max_vertex_count,
    "See \\max_vertex_count= for details about this attribute."
  ) +
  gsi::method ("device_scaling=", &db::LayoutToNetlist::set_device_scaling, gsi::arg ("f"),
   "@brief Sets the device scaling factor\n"
   "This factor will scale the physical properties of the extracted devices\n"
   "accordingly. The scale factor applies an isotropic shrink (<1) or expansion (>1).\n"
  ) +
  gsi::method ("device_scaling", &db::LayoutToNetlist::device_scaling,
    "@brief Gets the device scaling factor\n"
    "See \\device_scaling= for details about this attribute."
  ) +
  gsi::method ("name", (const std::string &(db::LayoutToNetlist::*) () const) &db::LayoutToNetlist::name,
    "@brief Gets the name of the database\n"
  ) +
  gsi::method ("name=", &db::LayoutToNetlist::set_name, gsi::arg ("name"),
    "@brief Sets the name of the database\n"
  ) +
  gsi::method ("description", (const std::string &(db::LayoutToNetlist::*) () const) &db::LayoutToNetlist::name,
    "@brief Gets the description of the database\n"
  ) +
  gsi::method ("description=", &db::LayoutToNetlist::set_name, gsi::arg ("description"),
    "@brief Sets the description of the database\n"
  ) +
  gsi::method ("filename", &db::LayoutToNetlist::filename,
    "@brief Gets the file name of the database\n"
    "The filename is the name under which the database is stored or empty if it is not associated with a file."
  ) +
  gsi::method ("original_file", &db::LayoutToNetlist::original_file,
    "@brief Gets the original file name of the database\n"
    "The original filename is the layout file from which the netlist DB was created."
  ) +
  gsi::method ("original_file=", &db::LayoutToNetlist::set_original_file, gsi::arg ("path"),
    "@brief Sets the original file name of the database\n"
  ) +
  gsi::method ("layer_name", (std::string (db::LayoutToNetlist::*) (const db::ShapeCollection &region) const) &db::LayoutToNetlist::name, gsi::arg ("l"),
    "@brief Gets the name of the given layer\n"
    "The layer is given by a \\ShapeCollection object which is either a \\Region or \\Texts object.\n"
    "This \\Region or \\Texts object needs to be either one that was created by make_... or one that\n"
    "was registered through \\register. This function will return the name that was given to that \\Region or \\Texts "
    "during those operations."
    "\n"
    "You can use \\layer_by_name to retrieve the \\Region object of a layer from the layer index.\n"
  ) +
  gsi::method ("layer_index", (unsigned int (db::LayoutToNetlist::*) (const db::ShapeCollection &region) const) &db::LayoutToNetlist::layer_of<db::ShapeCollection>, gsi::arg ("l"),
    "@brief Gets the layer index for the given data object\n"
    "This method is essentially identical to \\layer_of, but uses \\ShapeCollection, which is a polymorphic base class "
    "for a variety of shape containers.\n"
    "\n"
    "The layer index returned is index of the corresponding layer inside the internal layout (see \\internal_layout).\n"
    "The layer index is more handy to use for identifying a layer than a \\Region of \\Texts object, for example when using it as a key in hashes.\n"
    "\n"
    "You can use \\layer_by_index to retrieve the \\Region object of a layer from the layer index.\n"
    "\n"
    "This method has been introduced in version 0.29.3.\n"
  ) +
  gsi::method_ext ("layer_index", &l2n_layer_index_by_name, gsi::arg ("name"),
    "@brief Gets the layer index for a given name or nil if the name is not valid.\n"
    "\n"
    "This method has been introduced in version 0.30."
  ) +
  gsi::method ("layer_name", (std::string (db::LayoutToNetlist::*) (unsigned int) const) &db::LayoutToNetlist::name, gsi::arg ("l"),
    "@brief Gets the name of the given layer (by index)\n"
    "See \\layer_index for a description of the layer index.\n"
  ) +
  gsi::method ("register", (unsigned int (db::LayoutToNetlist::*) (const db::ShapeCollection &collection, const std::string &)) &db::LayoutToNetlist::register_layer, gsi::arg ("l"), gsi::arg ("n", std::string (), "\"\""),
    "@brief Names the given layer\n"
    "@return The index of the layer registered\n"
    "'l' must be a \\Region or \\Texts object.\n"
    "Flat regions or text collections must be registered with this function, before they can be used in \\connect. "
    "Registering will copy the shapes into the LayoutToNetlist object in this step to enable "
    "netlist extraction.\n"
    "\n"
    "External \\Region or \\Texts objects that are registered are persisted. This means "
    "the LayoutToNetlist object becomes owner of them and they are not discarded when the "
    "Region or Text object is destroyed.\n"
    "\n"
    "Naming a layer allows allows retrieving the layer later, for example after the LayoutToNetlist object\n"
    "has been written to a file and restored from that file (during this process, the layer indexes will change).\n"
    "\n"
    "If no name is given, the system will assign a name automatically.\n"
    "It is recommended to specify a name if it is required to identify the layer later - for example for "
    "retrieving shapes from it.\n"
    "\n"
    "This method has been generalized in version 0.27. Starting with version 0.29.3, the index of the layer is returned.\n"
  ) +
  gsi::method_ext ("layer_names", &l2n_layer_names,
    "@brief Returns a list of names of the layers kept inside the LayoutToNetlist object."
  ) +
  gsi::method_ext ("layer_indexes", &l2n_layer_indexes,
    "@brief Returns a list of indexes of the layers kept inside the LayoutToNetlist object.\n"
    "You can use \\layer_name to get the name from a layer index. You can use \\layer_info to get "
    "the \\LayerInfo object attached to a layer - if the layer is an original layer.\n"
    "You can use \\layer_by_index to get the \\Region object for the layer by index.\n"
    "\n"
    "This method has been introduced in version 0.29.2.\n"
  ) +
  gsi::method_ext ("layer_info", &l2n_layer_info, gsi::arg ("index"),
    "@brief Returns the LayerInfo object attached to a layer (by index).\n"
    "If the layer is an original layer and not a derived one, this method will return the "
    "stream layer information where the original layer was taken from. Otherwise an empty \\LayerInfo object "
    "is returned.\n"
    "\n"
    "The LayerInfo object is usually empty for derived layers - i.e. those which are computed through "
    "boolean operations for example. It is recommended to assign names to such layers for easy identification later.\n"
    "\n"
    "This method has been introduced in version 0.29.2.\n"
  ) +
  gsi::factory ("polygons_by_name|#layer_by_name", &db::LayoutToNetlist::layer_by_name, gsi::arg ("name"),
    "@brief Gets a layer object for the given name.\n"
    "The returned object is a new Region object representing the named layer. It will refer to a layer inside the "
    "internal layout, or more specifically inside the \\DeepShapeStorage object (see \\dss and \\internal_layout).\n"
    "The method returns 'nil' if the name is not a valid layer name.\n"
    "See \\register and the make_... methods for a description of layer naming.\n"
    "\n"
    "It is in the responsibility of the user to use \\texts_by_index or \\polygons_by_index on the right layers. "
    "A layer created for text purpose should not be used with \\polygons_by_index or vice versa.\n"
    "\n"
    "Starting with version 0.30, the preferred name for this method is \\polygons_by_index to "
    "differentiate from \\texts_by_index."
  ) +
  gsi::factory ("texts_by_name", &db::LayoutToNetlist::texts_by_name, gsi::arg ("name"),
    "@brief Gets a layer object for the given name.\n"
    "The returned object is a new Texts object representing the named layer. It will refer to a layer inside the "
    "internal layout, or more specifically inside the \\DeepShapeStorage object (see \\dss and \\internal_layout).\n"
    "The method returns 'nil' if the name is not a valid layer name.\n"
    "See \\register and the make_... methods for a description of layer naming.\n"
    "\n"
    "It is in the responsibility of the user to use \\texts_by_name or \\polygons_by_name on the right layers. "
    "A layer created for text purpose should not be used with \\polygons_by_name or vice versa.\n"
    "\n"
    "This method has been introduced in version 0.30."
  ) +
  gsi::factory ("polygons_by_index|#layer_by_index", &db::LayoutToNetlist::layer_by_index, gsi::arg ("index"),
    "@brief Gets a \\Region object for the given index.\n"
    "The returned object is a new Region object representing the layer with the given index. It will refer to a layer inside the "
    "internal layout, or more specifically inside the \\DeepShapeStorage object (see \\dss and \\internal_layout).\n"
    "The method returns 'nil' if the index is not a valid layer index.\n"
    "\n"
    "It is in the responsibility of the user to use \\texts_by_index or \\polygons_by_index on the right layers. "
    "A layer created for text purpose should not be used with \\polygons_by_index or vice versa.\n"
    "\n"
    "Starting with version 0.30, the preferred name for this method is \\polygons_by_index to "
    "differentiate from \\texts_by_index."
  ) +
  gsi::factory ("texts_by_index", &db::LayoutToNetlist::texts_by_index, gsi::arg ("index"),
    "@brief Gets a \\Texts object for the given index.\n"
    "The returned object is a new Texts object representing the layer with the given index. It will refer to a layer inside the "
    "internal layout, or more specifically inside the \\DeepShapeStorage object (see \\dss and \\internal_layout).\n"
    "The method returns 'nil' if the index is not a valid layer index.\n"
    "\n"
    "It is in the responsibility of the user to use \\texts_by_index or \\polygons_by_index on the right layers. "
    "A layer created for text purpose should not be used with \\polygons_by_index or vice versa.\n"
    "\n"
    "This method has been introduced in version 0.30."
  ) +
  gsi::method ("is_persisted?", &db::LayoutToNetlist::is_persisted<db::Region>, gsi::arg ("layer"),
    "@brief Returns true, if the given layer is a persisted region.\n"
    "Persisted layers are kept inside the LayoutToNetlist object and are not released "
    "if their object is destroyed. Layers created with make_... or registered through \\register are persisted.\n"
    "This basically applies to all layers, except intermediate layers that are potentially created as results of "
    "operations between layers and which are not registered.\n"
  ) +
  gsi::method ("is_persisted?", &db::LayoutToNetlist::is_persisted<db::Texts>, gsi::arg ("layer"),
    "@brief Returns true, if the given layer is a persisted texts collection.\n"
    "Persisted layers are kept inside the LayoutToNetlist object and are not released "
    "if their object is destroyed. Layers created with make_... or registered through \\register are persisted.\n"
    "This basically applies to all layers, except intermediate layers that are potentially created as results of "
    "operations between layers and which are not registered.\n"
    "\n"
    "The variant for Texts collections has been added in version 0.27."
  ) +
  gsi::factory ("make_layer", (db::Region *(db::LayoutToNetlist::*) (const std::string &)) &db::LayoutToNetlist::make_layer, gsi::arg ("name", std::string (), "\"\""),
    "@brief Creates a new, empty hierarchical region\n"
    "\n"
    "This method will create a new, empty layer inside the internal layout and register it.\n"
    "It returns a new Region object that represents the new layer. See the class description for more details.\n"
    "The name is optional. If given, the layer will already be named accordingly (see \\register).\n"
  ) +
  gsi::factory ("make_layer", (db::Region *(db::LayoutToNetlist::*) (unsigned int, const std::string &)) &db::LayoutToNetlist::make_layer, gsi::arg ("layer_index"), gsi::arg ("name", std::string (), "\"\""),
    "@brief Creates a new hierarchical region representing an original layer\n"
    "'layer_index' is the layer index of the desired layer in the original layout.\n"
    "This variant produces polygons and takes texts for net name annotation as special, property-annotated polygons.\n"
    "A variant not taking texts is \\make_polygon_layer. A Variant only taking\n"
    "texts is \\make_text_layer.\n"
    "\n"
    "This method will basically create a copy of the original layer inside the internal layout - more specifically inside "
    "the DSS (see \\dss and \\internal_layout). It returns a new Region object that represents this layer copy. "
    "The new layer is already registered with the given name and can be used for \\connect for example.\n"
  ) +
  gsi::factory ("make_text_layer", (db::Texts *(db::LayoutToNetlist::*) (const std::string &)) &db::LayoutToNetlist::make_text_layer, gsi::arg ("name", std::string (), "\"\""),
    "@brief Creates a new, empty hierarchical text collection\n"
    "See \\make_layer for details.\n"
    "\n"
    "Starting with version 0.30, the original layer index is optional, allowing to create empty, internal text layers."
  ) +
  gsi::factory ("make_text_layer", (db::Texts *(db::LayoutToNetlist::*) (unsigned int, const std::string &)) &db::LayoutToNetlist::make_text_layer, gsi::arg ("layer_index"), gsi::arg ("name", std::string (), "\"\""),
    "@brief Creates a new text collection representing an original layer taking texts only\n"
    "See \\make_layer for details.\n"
    "\n"
    "Starting with version 0.27, this method returns a \\Texts object."
  ) +
  gsi::factory ("make_polygon_layer", (db::Region *(db::LayoutToNetlist::*) (const std::string &)) &db::LayoutToNetlist::make_layer, gsi::arg ("name", std::string (), "\"\""),
    "@brief Creates a new, empty hierarchical region\n"
    "See \\make_layer for details.\n"
    "\n"
    "Starting with version 0.30, the original layer index is optional for consistency with \\make_layer."
  ) +
  gsi::factory ("make_polygon_layer", (db::Region *(db::LayoutToNetlist::*) (unsigned int, const std::string &)) &db::LayoutToNetlist::make_polygon_layer, gsi::arg ("layer_index"), gsi::arg ("name", std::string (), "\"\""),
    "@brief Creates a new region representing an original layer taking polygons only\n"
    "See \\make_layer for details.\n"
  ) +
  gsi::method ("extract_devices", &db::LayoutToNetlist::extract_devices, gsi::arg ("extractor"), gsi::arg ("layers"),
    "@brief Extracts devices\n"
    "See the class description for more details.\n"
    "This method will run device extraction for the given extractor. The layer map is specific\n"
    "for the extractor and uses the Region objects derived with \\make_layer and its variants or Region "
    "objects registered through \\register. The layer map keys are the inputs layers defined for the\n"
    "specific extractor, but also the output layers where the extractor places markers for the device terminals.\n"
    "\n"
    "In addition, derived regions can also be passed to the device extractor inside the layer map.\n"
    "Certain limitations apply. It is usually safe to use\n"
    "boolean operations for deriving layers. Other operations are applicable as long as they are\n"
    "capable of delivering hierarchical layers.\n"
    "\n"
    "Example:\n"
    "\n"
    "@code\n"
    "ly = ... # original Layout\n"
    "\n"
    "l2n = RBA::LayoutToNetlist::new(RBA::RecursiveShapeIterator::new(ly, ly.top_cell, []))\n"
    "rnwell   = l2n.make_layer(ly.layer(1, 0), \"nwell\" )\n"
    "ractive  = l2n.make_layer(ly.layer(2, 0), \"active\" )\n"
    "rpoly    = l2n.make_layer(ly.layer(3, 0), \"poly\" )\n"
    "\n"
    "rpactive = ractive & rnwell\n"
    "rpgate   = rpactive & rpoly\n"
    "rpsd     = rpactive - rpgate\n"
    "\n"
    "rnactive = ractive - rnwell\n"
    "rngate   = rnactive & rpoly\n"
    "rnsd     = rnactive - rngate\n"
    "\n"
    "# PMOS transistor device extraction\n"
    "pmos_ex = RBA::DeviceExtractorMOS3Transistor::new(\"PMOS\")\n"
    "l2n.extract_devices(pmos_ex, { \"SD\" => rpsd, \"G\" => rpgate, \"P\" => rpoly })\n"
    "\n"
    "# NMOS transistor device extraction\n"
    "nmos_ex = RBA::DeviceExtractorMOS3Transistor::new(\"NMOS\")\n"
    "l2n.extract_devices(nmos_ex, { \"SD\" => rnsd, \"G\" => rngate, \"P\" => rpoly })\n"
    "@/code\n"
    "\n"
    "If errors occur, they will be logged inside the device extractor object and copied to the log of this LayoutToNetlist object (self).\n"
  ) +
  gsi::method ("reset_extracted", &db::LayoutToNetlist::reset_extracted,
    "@brief Resets the extracted netlist and enables re-extraction\n"
    "This method is implicitly called when using \\connect or \\connect_global after a netlist has been extracted.\n"
    "This enables incremental connect with re-extraction.\n"
    "\n"
    "This method has been introduced in version 0.27.1.\n"
  ) +
  gsi::method ("is_extracted?", &db::LayoutToNetlist::is_netlist_extracted,
    "@brief Gets a value indicating whether the netlist has been extracted\n"
    "\n"
    "This method has been introduced in version 0.27.1.\n"
  ) +
  gsi::method ("connect", (void (db::LayoutToNetlist::*) (const db::Region &)) &db::LayoutToNetlist::connect, gsi::arg ("l"),
    "@brief Defines an intra-layer connection for the given layer.\n"
    "The layer as a Region object, representing either an original layer created with \\make_layer and its variants or\n"
    "a derived layer which was registered using \\register. Certain limitations apply. It's safe to use\n"
    "boolean operations for deriving layers. Other operations are applicable as long as they are\n"
    "capable of delivering hierarchical layers. Operations that introduce flat layers will create additonal pins\n"
    "as connections need to be made from a subcell to the top cell. Hence, flat layers - or rather some with a bad hierarchy - should\n"
    "be avoided in \\connect.\n"
  ) +
  gsi::method ("connect", (void (db::LayoutToNetlist::*) (const db::Region &, const db::Region &)) &db::LayoutToNetlist::connect, gsi::arg ("a"), gsi::arg ("b"),
    "@brief Defines an inter-layer connection for the given layers.\n"
    "The conditions mentioned with intra-layer \\connect apply for this method too.\n"
  ) +
  gsi::method ("connect", (void (db::LayoutToNetlist::*) (const db::Region &, const db::Texts &)) &db::LayoutToNetlist::connect, gsi::arg ("a"), gsi::arg ("b"),
    "@brief Defines an inter-layer connection for the given layers.\n"
    "The conditions mentioned with intra-layer \\connect apply for this method too.\n"
    "As one argument is a (hierarchical) text collection, this method is used to attach net labels to polygons.\n"
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  gsi::method ("connect", (void (db::LayoutToNetlist::*) (const db::Texts &, const db::Region &)) &db::LayoutToNetlist::connect, gsi::arg ("a"), gsi::arg ("b"),
    "@brief Defines an inter-layer connection for the given layers.\n"
    "The conditions mentioned with intra-layer \\connect apply for this method too.\n"
    "As one argument is a (hierarchical) text collection, this method is used to attach net labels to polygons.\n"
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  gsi::method ("soft_connect", (void (db::LayoutToNetlist::*) (const db::Region &, const db::Region &)) &db::LayoutToNetlist::soft_connect, gsi::arg ("a"), gsi::arg ("b"),
    "@brief Defines an inter-layer connection for the given layers in soft mode.\n"
    "Connects two layers through a soft connection.\n"
    "Soft connections cannot make connections between two different nets.\n"
    "These are directional connections where 'b' is the 'lower' layer (typically high-ohmic substrate or diffusion).\n"
    "\n"
    "Soft connections have been introduced in version 0.29.\n"
  ) +
  gsi::method ("soft_connect", (void (db::LayoutToNetlist::*) (const db::Region &, const db::Texts &)) &db::LayoutToNetlist::soft_connect, gsi::arg ("a"), gsi::arg ("b"),
    "@brief Defines an inter-layer connection for the given layers in soft mode.\n"
    "Connects two layers through a soft connection.\n"
    "Soft connections cannot make connections between two different nets.\n"
    "These are directional connections where 'b' is the 'lower' layer (typically high-ohmic substrate or diffusion).\n"
    "As one argument is a (hierarchical) text collection, this method is used to attach net labels to polygons.\n"
    "\n"
    "Soft connections have been introduced in version 0.29.\n"
  ) +
  gsi::method ("soft_connect", (void (db::LayoutToNetlist::*) (const db::Texts &, const db::Region &)) &db::LayoutToNetlist::soft_connect, gsi::arg ("a"), gsi::arg ("b"),
    "@brief Defines an inter-layer connection for the given layers in soft mode.\n"
    "Connects two layers through a soft connection.\n"
    "Soft connections cannot make connections between two different nets.\n"
    "These are directional connections where 'b' is the 'lower' layer (typically high-ohmic substrate or diffusion).\n"
    "As one argument is a (hierarchical) text collection, this method is used to attach net labels to polygons.\n"
    "\n"
    "Soft connections have been introduced in version 0.29.\n"
  ) +
  gsi::method ("connect_global", (size_t (db::LayoutToNetlist::*) (const db::Region &, const std::string &)) &db::LayoutToNetlist::connect_global, gsi::arg ("l"), gsi::arg ("global_net_name"),
    "@brief Defines a connection of the given layer with a global net.\n"
    "This method returns the ID of the global net. Use \\global_net_name to get "
    "the name back from the ID."
  ) +
  gsi::method ("connect_global", (size_t (db::LayoutToNetlist::*) (const db::Texts &, const std::string &)) &db::LayoutToNetlist::connect_global, gsi::arg ("l"), gsi::arg ("global_net_name"),
    "@brief Defines a connection of the given text layer with a global net.\n"
    "This method returns the ID of the global net. Use \\global_net_name to get "
    "the name back from the ID.\n"
    "\n"
    "This variant has been introduced in version 0.27.\n"
  ) +
  gsi::method ("soft_connect_global", (size_t (db::LayoutToNetlist::*) (const db::Region &, const std::string &)) &db::LayoutToNetlist::soft_connect_global, gsi::arg ("l"), gsi::arg ("global_net_name"),
    "@brief Defines a connection of the given layer with a global net in soft mode.\n"
    "This method returns the ID of the global net. Use \\global_net_name to get "
    "the name back from the ID.\n"
    "Soft connections are directional, where the global net is the 'lower' layer (typically high-ohmic substrate or diffusion).\n"
    "\n"
    "Soft connections have been introduced in version 0.29.\n"
  ) +
  gsi::method ("soft_connect_global", (size_t (db::LayoutToNetlist::*) (const db::Texts &, const std::string &)) &db::LayoutToNetlist::soft_connect_global, gsi::arg ("l"), gsi::arg ("global_net_name"),
    "@brief Defines a connection of the given text layer with a global net in soft mode.\n"
    "This method returns the ID of the global net. Use \\global_net_name to get "
    "the name back from the ID.\n"
    "Soft connections are directional, where the global net is the 'lower' layer (typically high-ohmic substrate or diffusion).\n"
    "\n"
    "Soft connections have been introduced in version 0.29.\n"
  ) +
  gsi::method ("global_net_name", &db::LayoutToNetlist::global_net_name, gsi::arg ("global_net_id"),
    "@brief Gets the global net name for the given global net ID."
  ) +
  gsi::method ("include_floating_subcircuits=", &db::LayoutToNetlist::set_include_floating_subcircuits, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether to include floating subcircuits in the netlist.\n"
    "\n"
    "With 'include_floating_subcircuits' set to true, subcircuits with no connection to their parent "
    "circuit are still included in the circuit as floating subcircuits. Specifically on flattening this "
    "means that these subcircuits are properly propagated to their parent instead of appearing as "
    "additional top circuits.\n"
    "\n"
    "This attribute has been introduced in version 0.27 and replaces the arguments of \\extract_netlist."
  ) +
  gsi::method ("include_floating_subcircuits", &db::LayoutToNetlist::include_floating_subcircuits,
    "@brief Gets a flag indicating whether to include floating subcircuits in the netlist.\n"
    "See \\include_floating_subcircuits= for details.\n"
    "\n"
    "This attribute has been introduced in version 0.27.\n"
  ) +
  gsi::method ("top_level_mode=", &db::LayoutToNetlist::set_top_level_mode, gsi::arg ("flag"),
    "@brief Sets a flag indicating whether top level mode is enabled.\n"
    "\n"
    "In top level mode, must-connect warnings are turned into errors for example.\n"
    "To enable top level mode, set this attribute to true. By default, top-level mode is turned off.\n"
    "\n"
    "This attribute has been introduced in version 0.28.13."
  ) +
  gsi::method ("top_level_mode", &db::LayoutToNetlist::top_level_mode,
    "@brief Gets a flag indicating whether top level mode is enabled.\n"
    "See \\top_level_mode= for details.\n"
    "\n"
    "This attribute has been introduced in version 0.28.13.\n"
  ) +
  gsi::method ("clear_join_net_names", &db::LayoutToNetlist::clear_join_net_names,
    "@brief Clears all implicit net joining expressions.\n"
    "See \\extract_netlist for more details about this feature.\n"
    "\n"
    "This method has been introduced in version 0.27 and replaces the arguments of \\extract_netlist."
  ) +
  gsi::method_ext ("join_net_names", &join_net_names, gsi::arg ("pattern"),
    "@brief Specifies another pattern for implicit joining of nets for the top level cell.\n"
    "Use this method to register a pattern for net labels considered in implicit net joining. Implicit net joining "
    "allows connecting multiple parts of the same nets (e.g. supply rails) without need for a physical connection. "
    "The pattern specifies labels to look for. When parts are labelled with a name matching the expression, "
    "the parts carrying the same name are joined.\n"
    "\n"
    "This method adds a new pattern. Use \\clear_join_net_names to clear the registered pattern.\n"
    "\n"
    "Each pattern is a glob expression. Valid glob expressions are:\n"
    "@ul\n"
    "@li \"\" no implicit connections.@/li\n"
    "@li \"*\" to make all labels candidates for implicit connections.@/li\n"
    "@li \"VDD\" to make all 'VDD' nets candidates for implicit connections.@/li\n"
    "@li \"VDD\" to make all 'VDD'+suffix nets candidates for implicit connections.@/li\n"
    "@li \"{VDD,VSS}\" to all VDD and VSS nets candidates for implicit connections.@/li\n"
    "@/ul\n"
    "\n"
    "Label matching is case sensitive.\n"
    "\n"
    "This method has been introduced in version 0.27 and replaces the arguments of \\extract_netlist."
  ) +
  gsi::method_ext ("join_net_names", &join_net_names2, gsi::arg ("cell_pattern"), gsi::arg ("pattern"),
    "@brief Specifies another pattern for implicit joining of nets for the cells from the given cell pattern.\n"
    "This method allows applying implicit net joining for specific cells, not only for the top cell.\n"
    "\n"
    "This method adds a new pattern. Use \\clear_join_net_names to clear the registered pattern.\n"
    "\n"
    "This method has been introduced in version 0.27 and replaces the arguments of \\extract_netlist."
  ) +
  gsi::method ("clear_join_nets", &db::LayoutToNetlist::clear_join_nets,
    "@brief Clears all explicit net joining expressions.\n"
    "See \\extract_netlist for more details about this feature.\n"
    "\n"
    "Explicit net joining has been introduced in version 0.27."
  ) +
  gsi::method_ext ("join_nets", &join_nets, gsi::arg ("net_names"),
    "@brief Specifies another name list for explicit joining of nets for the top level cell.\n"
    "Use this method to join nets from the set of net names. All these nets will be connected together forming a single net.\n"
    "Explicit joining will imply implicit joining for the involved nets - partial nets involved will be connected too (intra-net joining).\n"
    "\n"
    "This method adds a new name list. Use \\clear_join_nets to clear the registered pattern.\n"
    "\n"
    "Explicit net joining has been introduced in version 0.27."
  ) +
  gsi::method_ext ("join_nets", &join_nets2, gsi::arg ("cell_pattern"), gsi::arg ("net_names"),
    "@brief Specifies another name list for explicit joining of nets for the cells from the given cell pattern.\n"
    "This method allows applying explicit net joining for specific cells, not only for the top cell.\n"
    "\n"
    "This method adds a new name list. Use \\clear_join_nets to clear the registered pattern.\n"
    "\n"
    "Explicit net joining has been introduced in version 0.27."
  ) +
  gsi::method ("extract_netlist", &db::LayoutToNetlist::extract_netlist,
    "@brief Runs the netlist extraction\n"
    "\n"
    "See the class description for more details.\n"
    "\n"
    "This method has been made parameter-less in version 0.27. Use \\include_floating_subcircuits= and \\join_net_names as substitutes for the arguments of previous versions."
  ) +
  gsi::method ("check_extraction_errors", &db::LayoutToNetlist::check_extraction_errors,
    "@brief Raises an exception if extraction errors are present\n"
    "\n"
    "This method has been introduced in version 0.28.13."
  ) +
  gsi::method_ext ("original_layout", &l2n_original_layout,
    "@brief Gets the original layout or nil if the LayoutToNetlist object is not attached to one.\n"
    "\n"
    "This method has been introduced in version 0.30."
  ) +
  gsi::method_ext ("original_top_cell", &l2n_original_top_cell,
    "@brief Gets the original top cell or nil if the LayoutToNetlist object is not attached to an original layout.\n"
    "\n"
    "This method has been introduced in version 0.30."
  ) +
  gsi::method_ext ("internal_layout", &l2n_internal_layout,
    "@brief Gets the internal layout\n"
    "The internal layout is where the LayoutToNetlist database stores the shapes for the nets. "
    "Usually you do not need to access this object - you must use \\build_net or \\shapes_of_net to "
    "retrieve the per-net shape information. If you access the internal layout, make sure you do not "
    "modify it.\n"
    "\n"
    "See the class description for details about the internal layout object."
  ) +
  gsi::method_ext ("internal_top_cell", &l2n_internal_top_cell,
    "@brief Gets the internal top cell\n"
    "Usually it should not be required to obtain the internal cell. If you need to do so, make sure not to modify the cell as\n"
    "the functionality of the netlist extractor depends on it.\n"
    "\n"
    "See the class description for details about the internal layout object."
  ) +
  gsi::method ("#layer_of", &db::LayoutToNetlist::layer_of<db::Region>, gsi::arg ("l"),
    "@brief Gets the internal layer for a given extraction layer\n"
    "This method is required to derive the internal layer index - for example for\n"
    "investigating the cluster tree.\n"
    "\n"
    "A generalized version of this method is \\layer_index."
  ) +
  gsi::method ("#layer_of", &db::LayoutToNetlist::layer_of<db::Texts>, gsi::arg ("l"),
    "@brief Gets the internal layer for a given text collection\n"
    "This method is required to derive the internal layer index - for example for\n"
    "investigating the cluster tree.\n"
    "\n"
    "The variant for Texts collections has been added in version 0.27.\n"
    "A generalized version of this method is \\layer_index."
  ) +
  gsi::method ("cell_mapping_into", (db::CellMapping (db::LayoutToNetlist::*) (db::Layout &, db::Cell &, bool)) &db::LayoutToNetlist::cell_mapping_into, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("with_device_cells", false),
    "@brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.\n"
    "If 'with_device_cells' is true, cells will be produced for devices. These are cells not corresponding to circuits, so they are disabled normally.\n"
    "Use this option, if you want to access device terminal shapes per device.\n"
    "\n"
    "CAUTION: this function may create new cells in 'layout'. Use \\const_cell_mapping_into if you want to use the target layout's hierarchy and not modify it.\n"
  ) +
  gsi::method ("cell_mapping_into", (db::CellMapping (db::LayoutToNetlist::*) (db::Layout &, db::Cell &, const std::vector<const db::Net *> &, bool)) &db::LayoutToNetlist::cell_mapping_into, gsi::arg ("layout"), gsi::arg ("cell"), gsi::arg ("nets"), gsi::arg ("with_device_cells", false),
    "@brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.\n"
    "This version will only create cells which are required to represent the nets from the 'nets' argument.\n"
    "\n"
    "If 'with_device_cells' is true, cells will be produced for devices. These are cells not corresponding to circuits, so they are disabled normally.\n"
    "Use this option, if you want to access device terminal shapes per device.\n"
    "\n"
    "CAUTION: this function may create new cells in 'layout'. Use \\const_cell_mapping_into if you want to use the target layout's hierarchy and not modify it.\n"
  ) +
  gsi::method ("const_cell_mapping_into", &db::LayoutToNetlist::const_cell_mapping_into, gsi::arg ("layout"), gsi::arg ("cell"),
    "@brief Creates a cell mapping for copying shapes from the internal layout to the given target layout.\n"
    "This version will not create new cells in the target layout.\n"
    "If some required cells do not exist there, they will be flattened into the first existing parent.\n"
  ) +
  gsi::method ("netlist", &db::LayoutToNetlist::netlist,
    "@brief gets the netlist extracted (0 if no extraction happened yet)\n"
  ) +
  gsi::method ("shapes_of_pin", &db::LayoutToNetlist::shapes_of_pin, gsi::arg ("pin"), gsi::arg ("trans", db::ICplxTrans (), "unity"),
    "@brief Returns all shapes of the given subcircuit pin that make a connection to the net the pin lives in.\n"
    "This will return all shapes from the subcircuit attached by the given pin that interact with the net the pin lives in.\n"
    "This method returns a \\Region object with the shapes per layer where interactions are found.\n"
    "The layers are given as layer indexes.\n"
    "\n"
    "The returned shapes are already transformed into the coordinate system of the net (see \\shapes_of_net for example).\n"
    "An additional transformation can be applied using the optional \\trans argument.\n"
    "\n"
    "Note, that this method only considers interations between net shapes and subcircuits on every level below, "
    "but not between subcircuits.\n"
    "It can be used for example for digital nets connecting gate cells. In the general case however, nets may be formed\n"
    "also by touching subcircuits. In that case, the nets do not have shapes of their own and this function cannot detect\n"
    "the pin shapes.\n"
    "\n"
    "The call of this method may not be cheap, specificially if large nets are involved.\n"
    "\n"
    "This method has been introduced in version 0.29.2."
  ) +
  gsi::method ("shapes_of_terminal", &db::LayoutToNetlist::shapes_of_terminal, gsi::arg ("terminal"), gsi::arg ("trans", db::ICplxTrans (), "unity"),
    "@brief Returns all shapes of the given device terminal that make a connection to the net the terminal lives in.\n"
    "This will return all shapes from the device attached by the given terminal that interact with the net the terminal lives in.\n"
    "This method returns a \\Region object with the shapes per layer where interactions are found.\n"
    "The layers are given as layer indexes.\n"
    "\n"
    "The returned shapes are already transformed into the coordinate system of the net (see \\shapes_of_net for example).\n"
    "An additional transformation can be applied using the optional \\trans argument.\n"
    "\n"
    "Note, that this method only considers interations between net shapes and the device connected by the terminal, "
    "but not between subcircuits on the net and the device.\n"
    "It can be used for example for flat-extracted, transistor-level netlists. In the general case however, nets may be formed\n"
    "also by subcircuits touching devices. In that case, the nets do not have shapes of their own and this function cannot detect\n"
    "the terminal shapes.\n"
    "\n"
    "The call of this method may not be cheap, specificially if large nets are involved.\n"
    "\n"
    "This method has been introduced in version 0.29.2."
  ) +
  gsi::factory ("shapes_of_net", (db::Region *(db::LayoutToNetlist::*) (const db::Net &, const db::Region &, bool, const db::ICplxTrans &) const) &db::LayoutToNetlist::shapes_of_net, gsi::arg ("net"), gsi::arg ("of_layer"), gsi::arg ("recursive", true), gsi::arg ("trans", db::ICplxTrans (), "unity"),
    "@brief Returns all polygons of a specific net and layer.\n"
    "If 'recursive' is true, the returned region will contain the shapes of\n"
    "all subcircuits too.\n"
    "\n"
    "The optional 'trans' parameter allows applying a transformation to all shapes. It has been introduced in version 0.28.4."
  ) +
  gsi::factory ("shapes_of_net", (db::Texts *(db::LayoutToNetlist::*) (const db::Net &, const db::Texts &, bool, const db::ICplxTrans &) const) &db::LayoutToNetlist::shapes_of_net, gsi::arg ("net"), gsi::arg ("of_layer"), gsi::arg ("recursive", true), gsi::arg ("trans", db::ICplxTrans (), "unity"),
    "@brief Returns all texts of a specific net and layer.\n"
    "If 'recursive' is true, the returned text collection will contain the shapes of\n"
    "all subcircuits too.\n"
    "\n"
    "The optional 'trans' parameter allows applying a transformation to all shapes.\n"
    "\n"
    "This variant has been introduced in version 0.30."
  ) +
  gsi::factory ("polygons_of_net", (db::Region *(db::LayoutToNetlist::*) (const db::Net &, unsigned int, bool, const db::ICplxTrans &) const) &db::LayoutToNetlist::shapes_of_net_with_layer_index<db::Region>, gsi::arg ("net"), gsi::arg ("of_layer"), gsi::arg ("recursive", true), gsi::arg ("trans", db::ICplxTrans (), "unity"),
    "@brief Returns all polygons of a specific net and layer.\n"
    "If 'recursive' is true, the returned region will contain the shapes of\n"
    "all subcircuits too.\n"
    "\n"
    "The optional 'trans' parameter allows applying a transformation to all shapes.\n"
    "\n"
    "This method is similar to \\shapes_of_net, but takes a layer index for the layer. It was introduced in version 0.30.\n"
  ) +
  gsi::factory ("texts_of_net", (db::Texts *(db::LayoutToNetlist::*) (const db::Net &, unsigned int, bool, const db::ICplxTrans &) const) &db::LayoutToNetlist::shapes_of_net_with_layer_index<db::Texts>, gsi::arg ("net"), gsi::arg ("of_layer"), gsi::arg ("recursive", true), gsi::arg ("trans", db::ICplxTrans (), "unity"),
    "@brief Returns all texts of a specific net and layer.\n"
    "If 'recursive' is true, the returned region will contain the shapes of\n"
    "all subcircuits too.\n"
    "\n"
    "The optional 'trans' parameter allows applying a transformation to all shapes.\n"
    "\n"
    "This method is similar to \\shapes_of_net, but takes a layer index for the layer. It was introduced in version 0.30.\n"
  ) +
  gsi::method ("shapes_of_net", (void (db::LayoutToNetlist::*) (const db::Net &, const db::ShapeCollection &, bool, db::Shapes &, db::properties_id_type, const db::ICplxTrans &) const) &db::LayoutToNetlist::shapes_of_net, gsi::arg ("net"), gsi::arg ("of_layer"), gsi::arg ("recursive"), gsi::arg ("to"), gsi::arg ("propid", db::properties_id_type (0), "0"), gsi::arg ("trans", db::ICplxTrans (), "unity"),
    "@brief Sends all shapes of a specific net and layer to the given Shapes container.\n"
    "If 'recursive' is true, the returned region will contain the shapes of\n"
    "all subcircuits too.\n"
    "\n"
    "'prop_id' is an optional properties ID. If given, this property set will be attached to the shapes."
    "\n"
    "The optional 'trans' parameter allows applying a transformation to all shapes. It has been introduced in version 0.28.4."
    "\n"
    "The 'of_layer' argument has been generalized in version 0.30 and can be a layer index, a \\Region layer or a \\Texts layer."
  ) +
  gsi::method ("shapes_of_net", (void (db::LayoutToNetlist::*) (const db::Net &, unsigned int, bool, db::Shapes &, db::properties_id_type, const db::ICplxTrans &) const) &db::LayoutToNetlist::shapes_of_net, gsi::arg ("net"), gsi::arg ("of_layer"), gsi::arg ("recursive"), gsi::arg ("to"), gsi::arg ("propid", db::properties_id_type (0), "0"), gsi::arg ("trans", db::ICplxTrans (), "unity"),
    "@brief Sends all shapes of a specific net and layer to the given Shapes container.\n"
    "If 'recursive' is true, the returned region will contain the shapes of\n"
    "all subcircuits too.\n"
    "\n"
    "'prop_id' is an optional properties ID. If given, this property set will be attached to the shapes."
    "\n"
    "The optional 'trans' parameter allows applying a transformation to all shapes. It has been introduced in version 0.28.4."
    "\n"
    "The 'of_layer' argument has been generalized in version 0.30 and can be a layer index, a \\Region layer or a \\Texts layer."
  ) +
  gsi::method_ext ("build_net", &build_net, gsi::arg ("net"), gsi::arg ("target"), gsi::arg ("target_cell"), gsi::arg ("lmap", tl::Variant (), "nil"), gsi::arg ("netname_prop", tl::Variant (), "nil"), gsi::arg ("hier_mode", db::BNH_Flatten, "BNH_Flatten"), gsi::arg ("circuit_cell_name_prefix", tl::Variant (), "nil"), gsi::arg ("device_cell_name_prefix", tl::Variant (), "nil"),
    "@brief Builds a net representation in the given layout and cell\n"
    "\n"
    "This method puts the shapes of a net into the given target cell using a variety of options\n"
    "to represent the net name and the hierarchy of the net.\n"
    "\n"
    "If 'netname_prop' is not nil, a property with the given name is created and attached to shapes. The value "
    "of the property is the net name.\n"
    "\n"
    "'lmap' defines which layers are to be produced. It is map, where the keys are layer indexes in the "
    "target layout and the values are Region or Texts objects or layer indexes, indicating the layer where shapes are to be taken from. "
    "'lmap' can also be left nil, in which case, a layer mapping will be provided based on the layer info attributes of "
    "the layers (see \\layer_info).\n"
    "\n"
    "Also see \\build_all_nets for a description of the 'hier_mode' argument.\n"
    "\n"
    "If a device cell name prefix is given, cells will be produced for each device abstract\n"
    "using a name like device_cell_name_prefix + device name. Otherwise the device shapes are\n"
    "treated as part of the net.\n"
    "\n"
    "@param target The target layout\n"
    "@param target_cell The target cell\n"
    "@param lmap The layer mapping (see description of this method)\n"
    "@param hier_mode See description of this method\n"
    "@param netname_prop An (optional) property name to which to attach the net name\n"
    "@param cell_name_prefix Chooses recursive mode if non-null\n"
    "@param device_cell_name_prefix See above\n"
    "\n"
    "The 'lmap' argument has been generalized in version 0.30 and became optional."
  ) +
  gsi::method_ext ("build_all_nets", &build_all_nets, gsi::arg ("cmap"), gsi::arg ("target"), gsi::arg ("lmap", tl::Variant (), "nil"), gsi::arg ("net_cell_name_prefix", tl::Variant (), "nil"), gsi::arg ("netname_prop", tl::Variant (), "nil"), gsi::arg ("hier_mode", db::BNH_Flatten, "BNH_Flatten"), gsi::arg ("circuit_cell_name_prefix", tl::Variant (), "nil"), gsi::arg ("device_cell_name_prefix", tl::Variant (), "nil"),
    "@brief Builds a full hierarchical representation of the nets\n"
    "\n"
    "This method copies all nets into cells corresponding to the circuits. It uses the 'cmap'\n"
    "object to determine the target cell (create it with \"cell_mapping_into\" or \"const_cell_mapping_into\").\n"
    "If no mapping is provided for a specific circuit cell, the nets are copied into the next mapped parent as "
    "many times as the circuit cell appears there (circuit flattening).\n"
    "\n"
    "If 'netname_prop' is not nil, a property with the given name is created and attached to shapes. The value "
    "of the property is the net name.\n"
    "\n"
    "'lmap' defines which layers are to be produced. It is map, where the keys are layer indexes in the "
    "target layout and the values are Region or Texts objects or layer indexes, indicating the layer where shapes are to be taken from. "
    "'lmap' can also be left nil, in which case, a layer mapping will be provided based on the layer info attributes of "
    "the layers (see \\layer_info).\n"
    "\n"
    "'cmap' specifies the cell mapping. Use \\cell_mapping_into or \\const_cell_mapping_into to "
    "define the target cells in the target layout and to derive a cell mapping.\n"
    "\n"
    "The method has three net annotation modes:\n"
    "@ul\n"
    " @li No annotation (net_cell_name_prefix == nil and netname_prop == nil): the shapes will be put\n"
    "     into the target cell simply. @/li\n"
    " @li Net name property (net_cell_name_prefix == nil and netname_prop != nil): the shapes will be\n"
    "     annotated with a property named with netname_prop and containing the net name string. @/li\n"
    " @li Individual subcells per net (net_cell_name_prefix != 0): for each net, a subcell is created\n"
    "     and the net shapes will be put there (name of the subcell = net_cell_name_prefix + net name).\n"
    "     (this mode can be combined with netname_prop too). @/li\n"
    "@/ul\n"
    "\n"
    "In addition, net hierarchy is covered in three ways:\n"
    "@ul\n"
    " @li No connection indicated (hier_mode == \\BNH_Disconnected: the net shapes are simply put into their\n"
    "     respective circuits. The connections are not indicated. @/li\n"
    " @li Subnet hierarchy (hier_mode == \\BNH_SubcircuitCells): for each root net, a full hierarchy is built\n"
    "     to accommodate the subnets (see build_net in recursive mode). @/li\n"
    " @li Flat (hier_mode == \\BNH_Flatten): each net is flattened and put into the circuit it\n"
    "     belongs to. @/li\n"
    "@/ul\n"
    "\n"
    "If a device cell name prefix is given, cells will be produced for each device abstract\n"
    "using a name like device_cell_name_prefix + device name. Otherwise the device shapes are\n"
    "treated as part of the net.\n"
    "\n"
    "@param cmap The cell mapping (see description of this method)\n"
    "@param target The target layout\n"
    "@param lmap The layer mapping (see description of this method)\n"
    "@param hier_mode See description of this method\n"
    "@param netname_prop An (optional) property name to which to attach the net name\n"
    "@param circuit_cell_name_prefix See method description\n"
    "@param net_cell_name_prefix See method description\n"
    "@param device_cell_name_prefix See above\n"
    "\n"
    "The 'lmap' argument has been generalized in version 0.30 and became optional."
  ) +
  gsi::method_ext ("build_nets", &build_nets, gsi::arg ("nets"), gsi::arg ("cmap"), gsi::arg ("target"), gsi::arg ("lmap", tl::Variant (), "nil"), gsi::arg ("net_cell_name_prefix", tl::Variant (), "nil"), gsi::arg ("netname_prop", tl::Variant (), "nil"), gsi::arg ("hier_mode", db::BNH_Flatten, "BNH_Flatten"), gsi::arg ("circuit_cell_name_prefix", tl::Variant (), "nil"), gsi::arg ("device_cell_name_prefix", tl::Variant (), "nil"),
    "@brief Like \\build_all_nets, but with the ability to select some nets."
    "\n"
    "The 'lmap' argument has been generalized in version 0.30 and became optional."
  ) +
  gsi::method ("probe_net", (db::Net *(db::LayoutToNetlist::*) (const db::Region &, const db::DPoint &, std::vector<db::SubCircuit *> *, db::Circuit *)) &db::LayoutToNetlist::probe_net, gsi::arg ("of_layer"), gsi::arg ("point"), gsi::arg ("sc_path_out", (std::vector<db::SubCircuit *> *) 0, "nil"), gsi::arg ("initial_circuit", (db::Circuit *) 0, "nil"),
    "@brief Finds the net by probing a specific location on the given layer\n"
    "\n"
    "This method will find a net looking at the given layer at the specific position.\n"
    "It will traverse the hierarchy below if no shape in the requested layer is found\n"
    "in the specified location. The function will report the topmost net from far above the\n"
    "hierarchy of circuits as possible.\n"
    "\n"
    "If \\initial_circuit is given, the probing will start from this circuit and from the "
    "cell this circuit represents. By default, the probing will start from the top circuit.\n"
    "\n"
    "If no net is found at all, 0 is returned.\n"
    "\n"
    "It is recommended to use \\probe_net on the netlist right after extraction.\n"
    "Optimization functions such as \\Netlist#purge will remove parts of the net which means\n"
    "shape to net probing may no longer work for these nets.\n"
    "\n"
    "If non-null and an array, 'sc_path_out' will receive a list of \\SubCircuits objects which lead to the "
    "net from the top circuit of the database.\n"
    "\n"
    "This variant accepts a micrometer-unit location. The location is given in the\n"
    "coordinate space of the initial cell.\n"
    "\n"
    "The \\sc_path_out and \\initial_circuit parameters have been added in version 0.27.\n"
  ) +
  gsi::method ("probe_net", (db::Net *(db::LayoutToNetlist::*) (const db::Region &, const db::Point &, std::vector<db::SubCircuit *> *, db::Circuit *)) &db::LayoutToNetlist::probe_net, gsi::arg ("of_layer"), gsi::arg ("point"), gsi::arg ("sc_path_out", (std::vector<db::SubCircuit *> *) 0, "nil"), gsi::arg ("initial_circuit", (db::Circuit *) 0, "nil"),
    "@brief Finds the net by probing a specific location on the given layer\n"
    "See the description of the other \\probe_net variant.\n"
    "This variant accepts a database-unit location. The location is given in the\n"
    "coordinate space of the initial cell.\n"
    "\n"
    "The \\sc_path_out and \\initial_circuit parameters have been added in version 0.27.\n"
  ) +
  gsi::method ("write|write_l2n", &db::LayoutToNetlist::save, gsi::arg ("path"), gsi::arg ("short_format", false),
    "@brief Writes the extracted netlist to a file.\n"
    "This method employs the native format of KLayout.\n"
  ) +
  gsi::method ("read|read_l2n", &db::LayoutToNetlist::load, gsi::arg ("path"),
    "@brief Reads the extracted netlist from the file.\n"
    "This method employs the native format of KLayout.\n"
  ) +
  gsi::method ("clear_log_entries", &db::LayoutToNetlist::clear_log_entries,
    "@brief Clears the log entries.\n"
    "This method has been introduced in version 0.30.2"
  ) +
  gsi::method ("add_log_entry", &db::LayoutToNetlist::log_entry, gsi::arg ("entry"),
    "@brief Adds a log entry.\n"
    "This method has been introduced in version 0.30.2"
  ) +
  gsi::iterator ("each_log_entry|#each_error", &db::LayoutToNetlist::begin_log_entries, &db::LayoutToNetlist::end_log_entries,
    "@brief Iterates over all log entries collected during device and netlist extraction.\n"
    "This method has been introduced in version 0.28.13."
  ) +
  gsi::method_ext ("antenna_check", &antenna_check, gsi::arg ("gate"), gsi::arg ("metal"), gsi::arg ("ratio"), gsi::arg ("diodes", std::vector<tl::Variant> (), "[]"), gsi::arg ("texts", (db::Texts *) 0, "nil"),
    "@brief Runs an antenna check on the extracted clusters\n"
    "\n"
    "The antenna check will traverse all clusters and run an antenna check\n"
    "for all root clusters. The antenna ratio is defined by the total\n"
    "area of all \"metal\" shapes divided by the total area of all \"gate\" shapes\n"
    "on the cluster. Of all clusters where the antenna ratio is larger than\n"
    "the limit ratio all metal shapes are copied to the output region as\n"
    "error markers.\n"
    "\n"
    "The simple call is:\n"
    "\n"
    "@code\n"
    "l2n = ... # a LayoutToNetlist object\n"
    "l2n.extract_netlist\n"
    "# check for antenna ratio 10.0 of metal vs. poly:\n"
    "errors = l2n.antenna(poly, metal, 10.0)\n"
    "@/code\n"
    "\n"
    "You can include diodes which rectify the antenna effect. "
    "Provide recognition layers for theses diodes and include them "
    "in the connections. Then specify the diode layers in the antenna call:\n"
    "\n"
    "@code\n"
    "...\n"
    "# include diode_layer1:\n"
    "errors = l2n.antenna(poly, metal, 10.0, [ diode_layer1 ])\n"
    "# include diode_layer1 and diode_layer2:"
    "errors = l2n.antenna(poly, metal, 10.0, [ diode_layer1, diode_layer2 ])\n"
    "@/code\n"
    "\n"
    "Diodes can be configured to partially reduce the antenna effect depending "
    "on their area. This will make the diode_layer1 increase the ratio by 50.0 "
    "per square micrometer area of the diode:\n"
    "\n"
    "@code\n"
    "...\n"
    "# diode_layer1 increases the ratio by 50 per square micrometer area:\n"
    "errors = l2n.antenna(poly, metal, 10.0 [ [ diode_layer, 50.0 ] ])\n"
    "@/code\n"
    "\n"
    "If 'texts' is non-nil, this text collection will receive labels explaining the error in "
    "terms of area values and relevant ratio.\n"
    "\n"
    "The 'texts' parameter has been added in version 0.27.11."
  ) +
  gsi::method_ext ("antenna_check", &antenna_check2, gsi::arg ("gate"), gsi::arg ("gate_perimeter_factor"), gsi::arg ("metal"), gsi::arg ("metal_perimeter_factor"), gsi::arg ("ratio"), gsi::arg ("diodes", std::vector<tl::Variant> (), "[]"), gsi::arg ("texts", (db::Texts *) 0, "nil"),
    "@brief Runs an antenna check on the extracted clusters taking the perimeter into account\n"
    "\n"
    "This version of the \\antenna_check method allows taking the perimeter of gate or metal into account. "
    "The effective area is computed using:\n"
    "\n"
    "@code\n"
    "Aeff = A + P * t\n"
    "@/code\n"
    "\n"
    "Here Aeff is the area used in the check, A is the polygon area, P the perimeter and t the perimeter factor. "
    "This formula applies to gate polygon area/perimeter with 'gate_perimeter_factor' for t and metal polygon area/perimeter "
    "with 'metal_perimeter_factor'. The perimeter_factor has the dimension of micrometers and can be thought of as the width "
    "of the material. Essentially the side walls of the material are taking into account for the surface area as well.\n"
    "\n"
    "This variant has been introduced in version 0.26.6.\n"
  ) +
  gsi::method_ext ("antenna_check", &antenna_check3, gsi::arg ("gate"), gsi::arg ("gate_area_factor"), gsi::arg ("gate_perimeter_factor"), gsi::arg ("metal"), gsi::arg ("metal_area_factor"), gsi::arg ("metal_perimeter_factor"), gsi::arg ("ratio"), gsi::arg ("diodes", std::vector<tl::Variant> (), "[]"), gsi::arg ("texts", (db::Texts *) 0, "nil"),
    "@brief Runs an antenna check on the extracted clusters taking the perimeter into account and providing an area factor\n"
    "\n"
    "This (most generic) version of the \\antenna_check method allows taking the perimeter of gate or metal into account and also "
    "provides a scaling factor for the area part.\n"
    "The effective area is computed using:\n"
    "\n"
    "@code\n"
    "Aeff = A * f + P * t\n"
    "@/code\n"
    "\n"
    "Here f is the area factor and t the perimeter factor. A is the polygon area and P the polygon perimeter. "
    "A use case for this variant is to set the area factor to zero. This way, only perimeter contributions are "
    "considered.\n"
    "\n"
    "This variant has been introduced in version 0.26.6.\n"
  ) +
  gsi::method_ext ("evaluate_nets", &evaluate_nets, gsi::arg ("primary"), gsi::arg ("secondary"), gsi::arg ("expression"), gsi::arg ("variables", std::map<std::string, tl::Variant> (), "{}"), gsi::arg ("dbu", tl::Variant (), "auto"),
    "@brief Runs a generic net measurement function\n"
    "\n"
    "This method accepts some primary layer, a number of secondary layers with names and an expression.\n"
    "It also accepts variables which become available as variables inside the expression. This allows passing "
    "arbitrary values without having to encode them into the expression string.\n"
    "\n"
    "It will look at nets connecting to shapes on the primary layer and execute the expression for each\n"
    "of those nets. After that it will copy the primary shapes of the net to the output with the properties\n"
    "placed by 'put' attached to them.\n"
    "\n"
    "It is possible to skip primary shapes of a specific net by calling the 'skip' function with a 'true'\n"
    "value.\n"
    "\n"
    "The expression may use the following functions:\n"
    "\n"
    "@ul\n"
    "@li 'area': the area of all primary-layer shapes on the net in square um @/li\n"
    "@li 'area(name)': the area of all secondary-layer shapes. 'name' is a symbol with the name given in the secondary-layer map @/li\n"
    "@li 'perimeter': the perimeter of the primary-layer shapes on the net in um @/li\n"
    "@li 'perimeter(name)': the perimeter of the secondary-layer shapes. 'name' is a symbol with the name given in the secondary-layer map @/li\n"
    "@li 'put(name, value)': places the value as property 'name' on the output shapes @/li\n"
    "@li 'skip(flag)': will skip the primary shapes of that net when called with a true value @/li\n"
    "@li 'net': the \\Net object of the current net @/li\n"
    "@/ul\n"
    "\n"
    "If given, the 'dbu' argument gives the database unit to use for converting shape dimensions into micrometer units. "
    "If this value is 0, the area and perimeters are calculated in database units. If no DBU is specified, the value is determined automatically."
  ) +
  //  test API
  gsi::method ("make_soft_connection_diodes=", &db::LayoutToNetlist::set_make_soft_connection_diodes, gsi::arg ("flag"), "@hide") +
  gsi::method ("make_soft_connection_diodes", &db::LayoutToNetlist::make_soft_connection_diodes, "@hide") +
  gsi::method_ext ("dump_joined_net_names", &dump_joined_net_names, "@hide") +
  gsi::method_ext ("dump_joined_net_names_per_cell", &dump_joined_net_names_per_cell, "@hide") +
  gsi::method_ext ("dump_joined_nets", &dump_joined_nets, "@hide") +
  gsi::method_ext ("dump_joined_nets_per_cell", &dump_joined_nets_per_cell, "@hide")
  ,
  "@brief A framework for extracting netlists from layouts\n"
  "\n"
  "This class provides a framework for extracting a netlist from a layout.\n"
  "\n"
  "A LayoutToNetlist object extracts a netlist from an external \\Layout. To do so, it keeps "
  "an internal copy with an optimized representation of the original layout. When a netlist is extracted "
  "the net geometries can be recovered from that internal layout. In addition to that layout, it keeps "
  "the extracted netlist. Netlist and internal layout form a pair and there are references between them. "
  "For example, the \\Circuit objects from the netlist have an attribute \\cell_index, which tells what cell "
  "from the internal layout the circuit was derived from. In the same way, subcircuit references refer to "
  "cell instances and nets keep a reference to the shapes they were derived from.\n"
  "\n"
  "LayoutToNetlist can also operate in detached mode, when there is no external layout. In this mode, "
  "layers are created inside the internal layout only. As there is no input hierarchy, operation is "
  "necessarily flat in that case. Single \\Region and \\Texts shape collections can be introduced into "
  "the LayoutToNetlist objects from external sources to populate the layers from the internal layout.\n"
  "For detached mode, use the 'LayoutToNetlist(topcell, dbu)' constructor.\n"
  "\n"
  "Usually, the internal layout is stored inside an internal \\DeepShapeStore object, which supplies "
  "additional services such as layer lifetime management and maintains the connection to and from "
  "the external layout.\n"
  "However, you can also use the extractor with an existing \\DeepShapeStore object.\n"
  "In that case, this external \\DeepShapeStore object is used instead of the internal one.\n"
  "\n"
  "The LayoutToNetlist object can be persisted into a 'Layout to netlist database' file. This database "
  "is a storage for both the netlist and the net or circuit geometries. When reading such file into "
  "a new LayoutToNetlist object, there will be no connection to any external layout, but all the "
  "essential netlist and geometry information will be available.\n"
  "\n"
  "The \\LayoutToNetlist object is also the entry point for netlist-driven algorithms such as antenna checks.\n"
  "\n"
  "The use model of the LayoutToNetlist object consists of five steps which need to be executed in this order.\n"
  "\n"
  "@ul\n"
  "@li @b Configuration: @/b\n"
  "    In this step, the LayoutToNetlist object is created and\n"
  "    if required, configured. Methods to be used in this step are \\threads=,\n"
  "    \\area_ratio= or \\max_vertex_count=. The constructor for the LayoutToNetlist\n"
  "    object receives a \\RecursiveShapeIterator object which basically supplies the\n"
  "    hierarchy and the external layout taken as input. The constructor will initialize\n"
  "    the internal layout and connect it to the external one.\n"
  "@/li\n"
  "@li @b Preparation: @/b\n"
  "    In this step, the device recognition and extraction layers are drawn from\n"
  "    the framework. Derived layers can now be computed using boolean operations.\n"
  "    Methods to use in this step are \\make_layer and its variants. \\make_layer will either create\n"
  "    a new, empty layer or pull a layer from the external layout into the internal layout.\n"
  "    Derived layers are computed using the \\Region or \\Texts objects representing\n"
  "    existing layers. If derived layers are to be used in connectivity, they\n"
  "    need to be registered using \\register. This makes the LayoutToNetlist object the owner of "
  "    the layer (the layer is said to be persisted then). Registered layers can or should be given a "
  "    name. That helps indentifying them later.\n"
  "    Layer preparation is not necessarily required to happen before all\n"
  "    other steps. Layers can be computed shortly before they are required.\n"
  "@/li\n"
  "@li @b Device extraction: @/b\n"
  "    Following the preparation, the devices can be extracted using \\extract_devices.\n"
  "    This method needs to be called for each device extractor required. Each time,\n"
  "    a device extractor needs to be given, plus a map of device layers. The device\n"
  "    layers are device extractor specific. Either original or derived layers\n"
  "    may be specified here. Layer preparation may happen between calls to \\extract_devices.\n"
  "@/li\n"
  "@li @b Connectivity definition: @/b\n"
  "    Once the devices are derived, the netlist connectivity can be defined and the\n"
  "    netlist extracted. The connectivity is defined with \\connect and its\n"
  "    flavours. The actual netlist extraction happens with \\extract_netlist.\n"
  "@/li\n"
  "@li @b Netlist extraction: @/b\n"
  "    After netlist extraction, the information is ready to be retrieved.\n"
  "    The produced netlist is available with \\netlist. The Shapes of a\n"
  "    specific net are available with \\shapes_of_net. \\probe_net allows\n"
  "    finding a net by probing a specific location.\n"
  "@/li\n"
  "@/ul\n"
  "\n"
  "Once the extraction is done, you can persist the \\LayoutToNetlist object "
  "using \\write and restore it using \\read. You can use the query API (see below) to "
  "analyze the LayoutToNetlist database.\n"
  "\n"
  "The query API of the \\LayoutToNetlist object consists of the following parts:\n"
  "\n"
  "@ul\n"
  "@li Net shape retrieval: \\build_all_nets, \\build_nets, \\build_net and \\shapes_of_net @/li\n"
  "@li Layers: \\layer_by_index, \\layer_by_name, \\layer_indexes, \\layer_names, \\layer_info, \\layer_name @/li\n"
  "@li Log entries: \\each_log_entry @/li\n"
  "@li Probing (get net from position): \\probe_net @/li\n"
  "@li Netlist: \\netlist @/li\n"
  "@li Internal shape storage: \\internal_layout, \\internal_top_cell @/li\n"
  "@li Helper functions: \\cell_mapping_into, \\const_cell_mapping_into @/li\n"
  "@/ul\n"
  "\n"
  "Layers stored inside the LayoutToNetlist object are addressed in three ways:\n"
  "\n"
  "@ul\n"
  "@li Through a layer index: this is an integer number that addresses the layer. Technically this is "
  "    the layer index inside the internal layout (see \\internal_layout). To get a layer index from "
  "    a shape collection or name use \\layer_index. To get all layer indexes available, use \\layer_indexes. "
  "    Note, that \\make_layer, \\make_polygon_layer and \\make_text_layer also take a layer index, but "
  "    this the layer index of layer in the \\original_layout. @/li\n"
  "@li Through a name: Alternatively to the layer index, a layer can be addressed by name, provided one was "
  "    assigned. To assign a name, specify one when creating a layer with \\register, \\make_layer, \\make_polygon_layer "
  "    or \\make_text_layer. To get the layer index of a named layer, use \\layer_index. To get the name "
  "    of a layer use \\layer_name. To get the names of all layers, use \\layer_names. @/li\n"
  "@li As a shape collection: a layer can also be represented by a shape collection. A shape collection is "
  "    either a \\Region or a \\Texts object. These objects do not only represent a layer, but can be used "
  "    to derive new layers or do computations on layers. "
  "    In order to assign names to derived shape collections, use \\register. To create a new layer and a "
  "    corresponding shape collection object, use \\make_layer, \\make_polygon_layer or \\make_text_layer. "
  "    To get the shape collection for a given layer index, use \\layer_index, to get layer name from "
  "    a shape collection, use \\layer_name. To get a shape collection from a layer index, use "
  "    \\layer_by_index, \\polygons_by_index or \\texts_by_index. @/li\n"
  "@/ul\n"
  "\n"
  "This class has been introduced in version 0.26."
);

gsi::EnumIn<db::LayoutToNetlist, db::BuildNetHierarchyMode> decl_dbLayoutToNetlist_BuildNetHierarchyMode ("db", "BuildNetHierarchyMode",
  gsi::enum_const ("BNH_Flatten", db::BNH_Flatten,
    "@brief This constant tells \\build_net and \\build_all_nets to flatten the nets (used for the \"hier_mode\" parameter)."
  ) +
  gsi::enum_const ("BNH_Disconnected", db::BNH_Disconnected,
    "@brief This constant tells \\build_net and \\build_all_nets to produce local nets without connections to subcircuits (used for the \"hier_mode\" parameter)."
  ) +
  gsi::enum_const ("BNH_SubcircuitCells", db::BNH_SubcircuitCells,
    "@brief This constant tells \\build_net and \\build_all_nets to produce a hierarchy of subcircuit cells per net (used for the \"hier_mode\" parameter)."
  ),
  "@brief This class represents the LayoutToNetlist::BuildNetHierarchyMode enum\n"
  "This enum is used for \\LayoutToNetlist#build_all_nets and \\LayoutToNetlist#build_net."
);

//  Inject the NetlistCrossReference::BuildNetHierarchyMode declarations into NetlistCrossReference:
gsi::ClassExt<db::LayoutToNetlist> inject_dbLayoutToNetlist_BuildNetHierarchyMode_in_parent (decl_dbLayoutToNetlist_BuildNetHierarchyMode.defs ());

}
