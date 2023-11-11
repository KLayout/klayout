
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
#include "gsiDeclDbMetaInfo.h"
#include "dbLayout.h"
#include "dbClip.h"
#include "dbRecursiveShapeIterator.h"
#include "dbLayoutUtils.h"
#include "dbWriter.h"
#include "dbSaveLayoutOptions.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbPCellDeclaration.h"
#include "dbHash.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbTexts.h"
#include "dbLayoutUtils.h"
#include "dbLayerMapping.h"
#include "dbCellMapping.h"
#include "dbTechnology.h"
#include "tlStream.h"
#include "tlGlobPattern.h"

namespace gsi
{

// ---------------------------------------------------------------
//  db::LayerProperties binding

static const std::string &lp_get_name (const db::LayerProperties *lp)
{
  return lp->name;
}

static void lp_set_name (db::LayerProperties *lp, const std::string &s)
{
  lp->name = s;
}

static int lp_get_layer (const db::LayerProperties *lp)
{
  return lp->layer;
}

static void lp_set_layer (db::LayerProperties *lp, int n)
{
  lp->layer = n;
}

static int lp_get_datatype (const db::LayerProperties *lp)
{
  return lp->datatype;
}

static void lp_set_datatype (db::LayerProperties *lp, int n)
{
  lp->datatype = n;
}

static 
db::LayerProperties *ctor_layer_info_default ()
{
  return new db::LayerProperties ();
}

static 
db::LayerProperties *ctor_layer_info_ld (int layer, int datatype)
{
   db::LayerProperties *ret = new db::LayerProperties ();
   ret->layer = layer;
   ret->datatype = datatype;
   return ret;
}

static 
db::LayerProperties *ctor_layer_info_ldn (int layer, int datatype, const std::string &name)
{
   db::LayerProperties *ret = new db::LayerProperties ();
   ret->layer = layer;
   ret->datatype = datatype;
   ret->name = name;
   return ret;
}

static 
db::LayerProperties *ctor_layer_info_name (const std::string &name)
{
   db::LayerProperties *ret = new db::LayerProperties ();
   ret->name = name;
   return ret;
}

static
db::LayerProperties li_from_string (const char *s, bool as_target)
{
  tl::Extractor ex (s);
  db::LayerProperties lp;
  lp.read (ex, as_target);
  return lp;
}

static
size_t hash_value (const db::LayerProperties *l)
{
  return std::hfunc (*l);
}

static bool log_equal_ext (const db::LayerProperties *lp1, const db::LayerProperties &lp2)
{
  if (lp1->log_equal (lp2)) {
    return true;
  }
  //  compare by name as fallback if one argument is named and the other is not
  //  (this gives a way to look up
  if ((lp1->is_named () || lp2.is_named()) && ! lp1->name.empty () && ! lp2.name.empty ()) {
    return lp1->name == lp2.name;
  }
  return false;
}

//  since there already exists a "LayerProperties" object, we call this one "LayerInfo"
Class<db::LayerProperties> decl_LayerInfo ("db", "LayerInfo",
  gsi::constructor ("new", &ctor_layer_info_default, 
    "@brief The default constructor.\n"
    "Creates a default \\LayerInfo object.\n"
    "\n"
    "This method was added in version 0.18.\n"
  ) +
  gsi::constructor ("new", &ctor_layer_info_ld, gsi::arg ("layer"), gsi::arg ("datatype"),
    "@brief The constructor for a layer/datatype pair.\n"
    "Creates a \\LayerInfo object representing a layer and datatype.\n"
    "@param layer The layer number\n"
    "@param datatype The datatype number\n"
    "\n"
    "This method was added in version 0.18.\n"
  ) +
  gsi::constructor ("new", &ctor_layer_info_name, gsi::arg ("name"),
    "@brief The constructor for a named layer.\n"
    "Creates a \\LayerInfo object representing a named layer.\n"
    "@param name The name\n"
    "\n"
    "This method was added in version 0.18.\n"
  ) +
  gsi::constructor ("new", &ctor_layer_info_ldn, gsi::arg ("layer"), gsi::arg ("datatype"), gsi::arg ("name"),
    "@brief The constructor for a named layer with layer and datatype.\n"
    "Creates a \\LayerInfo object representing a named layer with layer and datatype.\n"
    "@param layer The layer number\n"
    "@param datatype The datatype number\n"
    "@param name The name\n"
    "\n"
    "This method was added in version 0.18.\n"
  ) +
  gsi::method ("from_string", &li_from_string, gsi::arg ("s"), gsi::arg ("as_target", false),
    "@brief Create a layer info object from a string\n"
    "@param The string\n"
    "@return The LayerInfo object\n"
    "\n"
    "If 'as_target' is true, relative specifications such as '*+1' for layer or datatype are permitted.\n"
    "\n"
    "This method will take strings as produced by \\to_s and create a \\LayerInfo object from them. "
    "The format is either \"layer\", \"layer/datatype\", \"name\" or \"name (layer/datatype)\".\n"
    "\n"
    "This method was added in version 0.23.\n"
    "The 'as_target' argument has been added in version 0.26.5.\n"
  ) +
  gsi::method ("to_s", &db::LayerProperties::to_string, gsi::arg ("as_target", false),
    "@brief Convert the layer info object to a string\n"
    "@return The string\n"
    "\n"
    "If 'as_target' is true, wildcard and relative specifications are formatted such such.\n"
    "\n"
    "This method was added in version 0.18.\n"
    "The 'as_target' argument has been added in version 0.26.5.\n"
  ) +
  gsi::method ("==", &db::LayerProperties::operator==, gsi::arg ("b"),
    "@brief Compares two layer info objects\n"
    "@return True, if both are equal\n"
    "\n"
    "This method was added in version 0.18.\n"
  ) +
  gsi::method ("!=", &db::LayerProperties::operator!=, gsi::arg ("b"),
    "@brief Compares two layer info objects\n"
    "@return True, if both are not equal\n"
    "\n"
    "This method was added in version 0.18.\n"
  ) +
  gsi::method_ext ("is_equivalent?", &log_equal_ext, gsi::arg ("b"),
    "@brief Equivalence of two layer info objects\n"
    "@return True, if both are equivalent\n"
    "\n"
    "First, layer and datatype are compared. The name is of second order and used only if no layer or datatype is given "
    "for one of the operands.\n"
    "This is basically a weak comparison that reflects the search preferences. It is the basis for \\Layout#find_layer.\n"
    "Here are some examples:\n"
    "\n"
    "@code\n"
    "# no match as layer/datatypes or names differ:\n"
    "RBA::LayerInfo::new(1, 17).is_equivalent?(RBA::LayerInfo::new(1, 18)) -> false\n"
    "RBA::LayerInfo::new('metal1').is_equivalent?(RBA::LayerInfo::new('m1')) -> false\n"
    "# exact match for numbered or named layers:\n"
    "RBA::LayerInfo::new(1, 17).is_equivalent?(RBA::LayerInfo::new(1, 17)) -> true\n"
    "RBA::LayerInfo::new('metal1').is_equivalent?(RBA::LayerInfo::new('metal1')) -> true\n"
    "# match as names are second priority over layer/datatypes:\n"
    "RBA::LayerInfo::new(1, 17, 'metal1').is_equivalent?(RBA::LayerInfo::new(1, 17, 'm1')) -> true\n"
    "# match as name matching is fallback:\n"
    "RBA::LayerInfo::new(1, 17, 'metal1').is_equivalent?(RBA::LayerInfo::new('metal1')) -> true\n"
    "# no match as neither names or layer/datatypes match:\n"
    "RBA::LayerInfo::new(1, 17, 'metal1').is_equivalent?(RBA::LayerInfo::new('m1')) -> false\n"
    "@/code\n"
    "\n"
    "This method was added in version 0.18 and modified to compare non-named vs. named layers in version 0.28.11.\n"
  ) +
  gsi::method_ext ("hash", &hash_value,
    "@brief Computes a hash value\n"
    "Returns a hash value for the given layer info object. This method enables layer info objects as hash keys.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("anonymous?", &db::LayerProperties::is_null,
    "@brief Returns true, if the layer has no specification (i.e. is created by the default constructor).\n"
    "@return True, if the layer does not have any specification.\n"
    "\n"
    "This method was added in version 0.23.\n"
  ) +
  gsi::method ("is_named?", &db::LayerProperties::is_named, 
    "@brief Returns true, if the layer is purely specified by name.\n"
    "@return True, if no layer or datatype is given.\n"
    "\n"
    "This method was added in version 0.18.\n"
  ) +
  gsi::method_ext ("name=", &lp_set_name, gsi::arg ("name"),
    "@brief Set the layer name\n"
    "The name is set on OASIS input for example, if the layer has a name.\n"
  ) +
  gsi::method_ext ("name", &lp_get_name, 
    "@brief Gets the layer name\n"
  ) +
  gsi::method_ext ("layer=", &lp_set_layer, gsi::arg ("layer"),
    "@brief Sets the layer number\n"
  ) +
  gsi::method_ext ("layer", &lp_get_layer, 
    "@brief Gets the layer number\n"
  ) +
  gsi::method_ext ("datatype=", &lp_set_datatype, gsi::arg ("datatype"),
    "@brief Set the datatype\n"
  ) +
  gsi::method_ext ("datatype", &lp_get_datatype, 
    "@brief Gets the datatype\n"
  ),
  "@brief A structure encapsulating the layer properties\n"
  "\n"
  "The layer properties describe how a layer is stored in a GDS2 or OASIS file for example. "
  "The \\LayerInfo object represents the storage properties that are attached to a layer in the database.\n"
  "\n"
  "In general, a layer has either a layer and a datatype number (in GDS2), a name (for example in DXF or CIF) "
  "or both (in OASIS). In the latter case, the primary identification is through layer and datatype number and "
  "the name is some annotation attached to it. A \\LayerInfo object which specifies just a name returns true on \\is_named?.\n"
  "The \\LayerInfo object can also specify an anonymous layer (use \\LayerInfo#new without arguments). Such "
  "a layer will not be stored when saving the layout. They can be employed for temporary layers for example. Use \\LayerInfo#anonymous? to test whether a layer does not have a specification.\n"
  "\n"
  "The \\LayerInfo is used for example in \\Layout#insert_layer to specify the properties of the new layer that "
  "will be created. The \\is_equivalent? method compares two \\LayerInfo objects using the layer and datatype "
  "numbers with a higher priority over the name.\n"
);

// ---------------------------------------------------------------
//  db::Layout binding

static void dump_mem_statistics (const db::Layout *layout, bool detailed)
{
  db::MemStatisticsCollector ms (detailed);
  layout->mem_stat (&ms, db::MemStatistics::LayoutInfo, 0 /*cat*/);
  ms.print ();
}

static bool layout_has_prop_id (const db::Layout *l)
{
  return l->prop_id () != 0;
}

static void delete_layout_property (db::Layout *l, const tl::Variant &key)
{
  //  TODO: check if is editable

  db::properties_id_type id = l->prop_id ();
  if (id == 0) {
    return;
  }

  std::pair<bool, db::property_names_id_type> nid = l->properties_repository ().get_id_of_name (key);
  if (! nid.first) {
    return;
  }

  db::PropertiesRepository::properties_set props = l->properties_repository ().properties (id);
  db::PropertiesRepository::properties_set::iterator p = props.find (nid.second);
  if (p != props.end ()) {
    props.erase (p);
  }

  l->prop_id (l->properties_repository ().properties_id (props));
}

static void set_layout_property (db::Layout *l, const tl::Variant &key, const tl::Variant &value)
{
  //  TODO: check if is editable

  db::properties_id_type id = l->prop_id ();

  db::property_names_id_type nid = l->properties_repository ().prop_name_id (key);

  db::PropertiesRepository::properties_set props = l->properties_repository ().properties (id);
  db::PropertiesRepository::properties_set::iterator p = props.find (nid);
  if (p != props.end ()) {
    p->second = value;
  } else {
    props.insert (std::make_pair (nid, value));
  }

  l->prop_id (l->properties_repository ().properties_id (props));
}

static tl::Variant get_layout_property (db::Layout *l, const tl::Variant &key)
{
  //  TODO: check if is editable
  
  db::properties_id_type id = l->prop_id ();
  if (id == 0) {
    return tl::Variant ();
  }

  std::pair<bool, db::property_names_id_type> nid = l->properties_repository ().get_id_of_name (key);
  if (! nid.first) {
    return tl::Variant ();
  }

  const db::PropertiesRepository::properties_set &props = l->properties_repository ().properties (id);
  db::PropertiesRepository::properties_set::const_iterator p = props.find (nid.second);
  if (p != props.end ()) {
    return p->second;
  } else {
    return tl::Variant ();
  }
}

static db::cell_index_type cell_by_name (db::Layout *l, const char *name)
{
  std::pair<bool, db::cell_index_type> c = l->cell_by_name (name);
  if (! c.first) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("No such cell: '%s'")), name));
  }
  return c.second;
}

static db::cell_index_type clip (db::Layout *l, db::cell_index_type c, const db::Box &box)
{
  std::vector <db::Box> boxes;
  boxes.push_back (box);
  std::vector <db::cell_index_type> cc = db::clip_layout(*l, *l, c, boxes, true);
  tl_assert (! cc.empty ());
  return cc [0];
}

static db::cell_index_type clip_dbox (db::Layout *l, db::cell_index_type c, const db::DBox &box)
{
  std::vector <db::Box> boxes;
  boxes.push_back (db::CplxTrans (l->dbu ()).inverted () * box);
  std::vector <db::cell_index_type> cc = db::clip_layout(*l, *l, c, boxes, true);
  tl_assert (! cc.empty ());
  return cc [0];
}

static db::Cell *clip_cell_dbox (db::Layout *l, const db::Cell &c, const db::DBox &box)
{
  std::vector <db::Box> boxes;
  boxes.push_back (db::CplxTrans (l->dbu ()).inverted () * box);
  std::vector <db::cell_index_type> cc = db::clip_layout(*l, *l, c.cell_index (), boxes, true);
  tl_assert (! cc.empty ());
  return &l->cell (cc [0]);
}

static db::Cell *clip_cell (db::Layout *l, const db::Cell &c, const db::Box &box)
{
  std::vector <db::Box> boxes;
  boxes.push_back (box);
  std::vector <db::cell_index_type> cc = db::clip_layout(*l, *l, c.cell_index (), boxes, true);
  tl_assert (! cc.empty ());
  return &l->cell (cc [0]);
}

static db::cell_index_type clip_into (const db::Layout *l, db::cell_index_type c, db::Layout *t, const db::Box &box)
{
  std::vector <db::Box> boxes;
  boxes.push_back (box);
  std::vector <db::cell_index_type> cc = db::clip_layout(*l, *t, c, boxes, true);
  tl_assert (! cc.empty ());
  return cc [0];
}

static db::cell_index_type clip_into_dbox (const db::Layout *l, db::cell_index_type c, db::Layout *t, const db::DBox &box)
{
  std::vector <db::Box> boxes;
  boxes.push_back (db::CplxTrans (l->dbu ()).inverted () * box);
  std::vector <db::cell_index_type> cc = db::clip_layout(*l, *t, c, boxes, true);
  tl_assert (! cc.empty ());
  return cc [0];
}

static db::Cell *clip_into_cell (const db::Layout *l, const db::Cell &c, db::Layout *t, const db::Box &box)
{
  std::vector <db::Box> boxes;
  boxes.push_back (box);
  std::vector <db::cell_index_type> cc = db::clip_layout(*l, *t, c.cell_index (), boxes, true);
  tl_assert (! cc.empty ());
  return &t->cell (cc [0]);
}

static db::Cell *clip_into_cell_dbox (const db::Layout *l, const db::Cell &c, db::Layout *t, const db::DBox &box)
{
  std::vector <db::Box> boxes;
  boxes.push_back (db::CplxTrans (l->dbu ()).inverted () * box);
  std::vector <db::cell_index_type> cc = db::clip_layout(*l, *t, c.cell_index (), boxes, true);
  tl_assert (! cc.empty ());
  return &t->cell (cc [0]);
}

static std::vector<db::Box> transform_boxes (const db::Layout *l, const std::vector<db::DBox> &boxes)
{
  std::vector<db::Box> result;
  result.reserve (boxes.size ());
  db::VCplxTrans t = db::CplxTrans (l->dbu ()).inverted ();
  for (std::vector<db::DBox>::const_iterator i = boxes.begin (); i != boxes.end (); ++i) {
    result.push_back (t * *i);
  }
  return result;
}

static std::vector<db::Cell *> to_cell_refs (db::Layout *l, const std::vector<db::cell_index_type> &cell_indexes)
{
  std::vector<db::Cell *> result;
  result.reserve (cell_indexes.size ());
  for (std::vector<db::cell_index_type>::const_iterator i = cell_indexes.begin (); i != cell_indexes.end (); ++i) {
    result.push_back (&l->cell (*i));
  }
  return result;
}

static std::vector<db::cell_index_type> multi_clip (db::Layout *l, db::cell_index_type c, const std::vector<db::Box> &boxes)
{
  return db::clip_layout (*l, *l, c, boxes, true);
}

static std::vector<db::Cell *> multi_clip_cells (db::Layout *l, const db::Cell &c, const std::vector<db::Box> &boxes)
{
  return to_cell_refs (l, db::clip_layout (*l, *l, c.cell_index (), boxes, true));
}

static std::vector<db::cell_index_type> multi_clip_dboxes (db::Layout *l, db::cell_index_type c, const std::vector<db::DBox> &boxes)
{
  return db::clip_layout (*l, *l, c, transform_boxes (l, boxes), true);
}

static std::vector<db::Cell *> multi_clip_cells_dboxes (db::Layout *l, const db::Cell &c, const std::vector<db::DBox> &boxes)
{
  return to_cell_refs (l, db::clip_layout (*l, *l, c.cell_index (), transform_boxes (l, boxes), true));
}

static std::vector<db::cell_index_type> multi_clip_into (db::Layout *l, db::cell_index_type c, db::Layout *t, const std::vector<db::Box> &boxes)
{
  return db::clip_layout (*l, *t, c, boxes, true);
}

static std::vector<db::Cell *> multi_clip_into_cells (db::Layout *l, const db::Cell &c, db::Layout *t, const std::vector<db::Box> &boxes)
{
  return to_cell_refs (l, db::clip_layout (*l, *t, c.cell_index (), boxes, true));
}

static std::vector<db::cell_index_type> multi_clip_into_dboxes (db::Layout *l, db::cell_index_type c, db::Layout *t, const std::vector<db::DBox> &boxes)
{
  return db::clip_layout (*l, *t, c, transform_boxes (l, boxes), true);
}

static std::vector<db::Cell *> multi_clip_into_cells_dboxes (db::Layout *l, const db::Cell &c, db::Layout *t, const std::vector<db::DBox> &boxes)
{
  return to_cell_refs (l, db::clip_layout (*l, *t, c.cell_index (), transform_boxes (l, boxes), true));
}

static unsigned int get_layer0 (db::Layout *l)
{
  return l->get_layer (db::LayerProperties ());
}

static unsigned int get_layer1 (db::Layout *l, const std::string &name)
{
  return l->get_layer (db::LayerProperties (name));
}

static unsigned int get_layer2 (db::Layout *l, int ln, int dn)
{
  return l->get_layer (db::LayerProperties (ln, dn));
}

static unsigned int get_layer3 (db::Layout *l, int ln, int dn, const std::string &name)
{
  return l->get_layer (db::LayerProperties (ln, dn, name));
}

static tl::Variant find_layer (db::Layout *l, const db::LayerProperties &lp)
{
  if (lp.is_null ()) {
    //  for a null layer info always return nil
    return tl::Variant ();
  } else {
    //  if we have a layer with the requested properties already, return this one.
    //  first pass: exact match
    int index = l->get_layer_maybe (lp);
    if (index >= 0) {
      return tl::Variant ((unsigned int) index);
    }
    //  second pass: relaxed match
    for (db::Layout::layer_iterator li = l->begin_layers (); li != l->end_layers (); ++li) {
      if (log_equal_ext ((*li).second, lp)) {
        return tl::Variant ((*li).first);
      }
    }
    //  otherwise return nil
    return tl::Variant ();
  }
}

static tl::Variant find_layer1 (db::Layout *l, const std::string &name)
{
  return find_layer (l, db::LayerProperties (name));
}

static tl::Variant find_layer2 (db::Layout *l, int ln, int dn)
{
  return find_layer (l, db::LayerProperties (ln, dn));
}

static tl::Variant find_layer3 (db::Layout *l, int ln, int dn, const std::string &name)
{
  return find_layer (l, db::LayerProperties (ln, dn, name));
}

static std::vector<unsigned int> layer_indexes (const db::Layout *l)
{
  std::vector<unsigned int> layers;
  for (unsigned int i = 0; i < l->layers (); ++i) {
    if (l->is_valid_layer (i)) {
      layers.push_back (i);
    }
  }
  return layers;
}

static std::vector<db::LayerProperties> layer_infos (const db::Layout *l)
{
  std::vector<db::LayerProperties> layers;
  for (unsigned int i = 0; i < l->layers (); ++i) {
    if (l->is_valid_layer (i)) {
      layers.push_back (l->get_properties (i));
    }
  }
  return layers;
}

static db::properties_id_type properties_id (db::Layout *layout, const std::vector<tl::Variant> &properties)
{
  db::PropertiesRepository::properties_set props;

  for (std::vector<tl::Variant>::const_iterator v = properties.begin (); v != properties.end (); ++v) {
    if (! v->is_list () || v->get_list ().size () != 2) {
      throw tl::Exception (tl::to_string (tr ("Expected a list of pairs of variants (found at least one that is not a pair)")));
    }
    db::property_names_id_type name_id = layout->properties_repository ().prop_name_id (v->get_list ()[0]);
    props.insert (std::make_pair (name_id, v->get_list () [1]));
  }

  return layout->properties_repository ().properties_id (props);
}

static std::vector<tl::Variant> properties (const db::Layout *layout, db::properties_id_type id)
{
  std::vector<tl::Variant> ret;

  if (layout->properties_repository ().is_valid_properties_id (id)) {

    const db::PropertiesRepository::properties_set &props = layout->properties_repository ().properties (id);
    ret.reserve (props.size ());

    for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {
      ret.push_back (tl::Variant::empty_list ());
      ret.back ().get_list ().reserve (2);
      ret.back ().get_list ().push_back (layout->properties_repository ().prop_name (p->first));
      ret.back ().get_list ().push_back (p->second);
    }

  }

  return ret;
}

static void 
delete_cells (db::Layout *layout, const std::vector<db::cell_index_type> &cell_indices)
{
  layout->delete_cells (cell_indices.begin (), cell_indices.end ());
}

static void 
prune_cell (db::Layout *layout, db::cell_index_type cell_index, int levels)
{
  layout->prune_cell (cell_index, levels);
}

static void 
prune_subcells (db::Layout *layout, db::cell_index_type cell_index, int levels)
{
  layout->prune_subcells (cell_index, levels);
}

static void 
flatten (db::Layout *layout, db::cell_index_type cell_index, int levels, bool prune)
{
  layout->flatten (layout->cell (cell_index), levels, prune);
}

static void 
flatten_into (db::Layout *layout, db::cell_index_type cell_index, db::cell_index_type target_cell_index, const db::ICplxTrans &t, int levels)
{
  layout->flatten (layout->cell (cell_index), layout->cell (target_cell_index), t, levels);
}

static void 
write_simple (db::Layout *layout, const std::string &filename)
{
  db::SaveLayoutOptions options;
  if (! options.set_format_from_filename (filename)) {
    throw tl::Exception (tl::to_string (tr ("Cannot determine format from filename")));
  }

  db::Writer writer (options);
  tl::OutputStream stream (filename);
  writer.write (*layout, stream);
}

static void 
write_options1 (db::Layout *layout, const std::string &filename, const db::SaveLayoutOptions &options)
{
  db::Writer writer (options);
  tl::OutputStream stream (filename);
  writer.write (*layout, stream);
}

static void 
write_options2 (db::Layout *layout, const std::string &filename, bool /*gzip*/, const db::SaveLayoutOptions &options)
{
  write_options1 (layout, filename, options);
}

static db::RecursiveShapeIterator 
begin_shapes (const db::Layout *layout, db::cell_index_type starting_cell, unsigned int layer)
{
  if (! layout->is_valid_layer (layer)) {
    throw tl::Exception (tl::to_string (tr ("Invalid layer index")));
  }
  if (! layout->is_valid_cell_index (starting_cell)) {
    throw tl::Exception (tl::to_string (tr ("Invalid cell index")));
  }
  return db::RecursiveShapeIterator (*layout, layout->cell (starting_cell), layer);
}

static db::RecursiveShapeIterator 
begin_shapes2 (const db::Layout *layout, const db::Cell *cell, unsigned int layer)
{
  return begin_shapes (layout, cell->cell_index (), layer);
}

static db::RecursiveShapeIterator 
begin_shapes_touching (const db::Layout *layout, db::cell_index_type starting_cell, unsigned int layer, db::Box region)
{
  if (! layout->is_valid_layer (layer)) {
    throw tl::Exception (tl::to_string (tr ("Invalid layer index")));
  }
  if (! layout->is_valid_cell_index (starting_cell)) {
    throw tl::Exception (tl::to_string (tr ("Invalid cell index")));
  }
  return db::RecursiveShapeIterator (*layout, layout->cell (starting_cell), layer, region, false);
}

static db::RecursiveShapeIterator 
begin_shapes_touching2 (const db::Layout *layout, const db::Cell *cell, unsigned int layer, db::Box region)
{
  return begin_shapes_touching (layout, cell->cell_index (), layer, region);
}

static db::RecursiveShapeIterator 
begin_shapes_overlapping (const db::Layout *layout, db::cell_index_type starting_cell, unsigned int layer, db::Box region)
{
  if (! layout->is_valid_layer (layer)) {
    throw tl::Exception (tl::to_string (tr ("Invalid layer index")));
  }
  if (! layout->is_valid_cell_index (starting_cell)) {
    throw tl::Exception (tl::to_string (tr ("Invalid cell index")));
  }
  return db::RecursiveShapeIterator (*layout, layout->cell (starting_cell), layer, region, true);
}

static db::RecursiveShapeIterator 
begin_shapes_overlapping2 (const db::Layout *layout, const db::Cell *cell, unsigned int layer, db::Box region)
{
  return begin_shapes_overlapping (layout, cell->cell_index (), layer, region);
}

static db::RecursiveShapeIterator
begin_shapes_touching_um (const db::Layout *layout, db::cell_index_type starting_cell, unsigned int layer, db::DBox region)
{
  if (! layout->is_valid_layer (layer)) {
    throw tl::Exception (tl::to_string (tr ("Invalid layer index")));
  }
  if (! layout->is_valid_cell_index (starting_cell)) {
    throw tl::Exception (tl::to_string (tr ("Invalid cell index")));
  }
  return db::RecursiveShapeIterator (*layout, layout->cell (starting_cell), layer, db::CplxTrans (layout->dbu ()).inverted () * region, false);
}

static db::RecursiveShapeIterator
begin_shapes_touching2_um (const db::Layout *layout, const db::Cell *cell, unsigned int layer, db::DBox region)
{
  return begin_shapes_touching_um (layout, cell->cell_index (), layer, region);
}

static db::RecursiveShapeIterator
begin_shapes_overlapping_um (const db::Layout *layout, db::cell_index_type starting_cell, unsigned int layer, db::DBox region)
{
  if (! layout->is_valid_layer (layer)) {
    throw tl::Exception (tl::to_string (tr ("Invalid layer index")));
  }
  if (! layout->is_valid_cell_index (starting_cell)) {
    throw tl::Exception (tl::to_string (tr ("Invalid cell index")));
  }
  return db::RecursiveShapeIterator (*layout, layout->cell (starting_cell), layer, db::CplxTrans (layout->dbu ()).inverted () * region, true);
}

static db::RecursiveShapeIterator
begin_shapes_overlapping2_um (const db::Layout *layout, const db::Cell *cell, unsigned int layer, db::DBox region)
{
  return begin_shapes_overlapping_um (layout, cell->cell_index (), layer, region);
}

static db::Layout *layout_ctor_with_manager(db::Manager &manager)
{
  return new db::Layout (true, &manager);
}

static db::Layout *layout_default_ctor()
{
  return new db::Layout (true);
}

static db::Layout *editable_layout_ctor_with_manager(bool editable, db::Manager &manager)
{
  return new db::Layout (editable, &manager);
}

static db::Layout *editable_layout_default_ctor(bool editable)
{
  return new db::Layout (editable);
}

static db::cell_index_type add_lib_pcell_variant (db::Layout *layout, db::Library *lib, db::pcell_id_type pcell_id, const std::vector<tl::Variant> &parameters)
{
  db::cell_index_type lib_cell = lib->layout ().get_pcell_variant (pcell_id, parameters);
  return layout->get_lib_proxy (lib, lib_cell);
}

static db::cell_index_type add_lib_pcell_variant_dict (db::Layout *layout, db::Library *lib, db::pcell_id_type pcell_id, const std::map<std::string, tl::Variant> &parameters)
{
  db::cell_index_type lib_cell = lib->layout ().get_pcell_variant_dict (pcell_id, parameters);
  return layout->get_lib_proxy (lib, lib_cell);
}

static db::pcell_id_type pcell_id (const db::Layout *layout, const char *name)
{
  std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (name);
  return pc.second;
}

static const db::PCellDeclaration *pcell_declaration (const db::Layout *layout, const char *name)
{
  std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (name);
  return pc.first ? layout->pcell_declaration (pc.second) : 0;
}

static std::vector<db::pcell_id_type> pcell_ids (const db::Layout *layout)
{
  std::vector<db::pcell_id_type> res;
  for (db::Layout::pcell_iterator pc = layout->begin_pcells (); pc != layout->end_pcells (); ++pc) {
    res.push_back (pc->second);
  }
  return res;
}

static std::vector<std::string> pcell_names (const db::Layout *layout)
{
  std::vector<std::string> res;
  for (db::Layout::pcell_iterator pc = layout->begin_pcells (); pc != layout->end_pcells (); ++pc) {
    res.push_back (pc->first);
  }
  return res;
}

static db::Cell *cell_from_index (db::Layout *ly, db::cell_index_type ci)
{
  if (! ly->is_valid_cell_index (ci)) {
    throw tl::Exception (tl::to_string (tr ("Not a valid cell index: ")) + tl::to_string (ci));
  }
  return &ly->cell (ci);
}

static db::Cell *cell_from_name (db::Layout *ly, const std::string &name)
{
  std::pair<bool, db::cell_index_type> cn = ly->cell_by_name (name.c_str ());
  if (cn.first) {
    return &ly->cell (cn.second);
  } else {
    return 0;
  }
}

static std::vector<db::Cell *> cells_from_name (db::Layout *layout, const std::string &filter)
{
  tl::GlobPattern gp (filter);

  std::vector<db::Cell *> result;
  db::Layout::top_down_iterator td = layout->begin_top_down ();
  while (td != layout->end_top_down ()) {
    if (gp.match (layout->cell_name (*td))) {
      result.push_back (&layout->cell (*td));
    }
    ++td;
  }

  return result;
}

static std::vector<db::Cell *> top_cells (db::Layout *layout)
{
  std::vector<db::Cell *> tc;
  db::Layout::top_down_iterator td = layout->begin_top_down ();
  while (td != layout->end_top_cells ()) {
    tc.push_back (&layout->cell (*td));
    ++td;
  }
  return tc;
}

static db::Cell *top_cell (db::Layout *layout)
{
  db::Cell *tc = 0;
  db::Layout::top_down_iterator td = layout->begin_top_down ();
  while (td != layout->end_top_cells ()) {
    if (! tc) {
      tc = &layout->cell (*td);
    } else {
      throw tl::Exception (tl::to_string (tr ("The layout has multiple top cells")));
    }
    ++td;
  }
  return tc;
}

static db::Cell *create_cell (db::Layout *layout, const std::string &name)
{
  return &layout->cell (layout->add_cell (name.c_str ()));
}

static db::Cell *create_cell2 (db::Layout *layout, const std::string &name, const std::map<std::string, tl::Variant> &params)
{
  std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (name.c_str ());
  if (! pc.first) {
    return 0;
  }

  return &layout->cell (layout->get_pcell_variant_dict (pc.second, params));
}

static db::Cell *create_cell3 (db::Layout *layout, const std::string &name, const std::string &libname)
{
  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (libname, layout->technology_name ());
  if (! lib) {
    return 0;
  }

  std::pair<bool, db::cell_index_type> lc = lib->layout ().cell_by_name (name.c_str ());
  if (! lc.first) {
    return 0;
  }

  return &layout->cell (layout->get_lib_proxy (lib, lc.second));
}

static db::Cell *create_cell4 (db::Layout *layout, const std::string &name, const std::string &libname, const std::map<std::string, tl::Variant> &params)
{
  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (libname, layout->technology_name ());
  if (! lib) {
    return 0;
  }

  std::pair<bool, db::pcell_id_type> pc = lib->layout ().pcell_by_name (name.c_str ());
  if (! pc.first) {
    return 0;
  }

  db::cell_index_type lib_cell = lib->layout ().get_pcell_variant_dict (pc.second, params);
  return &layout->cell (layout->get_lib_proxy (lib, lib_cell));
}

static void layout_add_meta_info (db::Layout *layout, const MetaInfo &mi)
{
  layout->add_meta_info (mi.name, db::MetaInfo (mi.description, mi.value, mi.persisted));
}

static MetaInfo *layout_get_meta_info (db::Layout *layout, const std::string &name)
{
  if (layout->has_meta_info (name)) {
    const db::MetaInfo &value = layout->meta_info (name);
    return new MetaInfo (name, value);
  } else {
    return 0;
  }
}

static const tl::Variant &layout_get_meta_info_value (db::Layout *layout, const std::string &name)
{
  const db::MetaInfo &value = layout->meta_info (name);
  return value.value;
}

static MetaInfoIterator layout_each_meta_info (const db::Layout *layout)
{
  return MetaInfoIterator (layout, layout->begin_meta (), layout->end_meta ());
}

static void scale_and_snap1 (db::Layout *layout, db::Cell &cell, db::Coord g, db::Coord m, db::Coord d)
{
  scale_and_snap (*layout, cell, g, m, d);
}

static void scale_and_snap2 (db::Layout *layout, db::cell_index_type ci, db::Coord g, db::Coord m, db::Coord d)
{
  scale_and_snap (*layout, layout->cell (ci), g, m, d);
}

static void copy_tree_shapes2 (db::Layout *layout, const db::Layout &source_layout, const db::CellMapping &cm)
{
  if (layout == &source_layout) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same layout")));
  }

  db::ICplxTrans trans (source_layout.dbu () / layout->dbu ());

  db::LayerMapping lm;
  lm.create_full (*layout, source_layout);

  db::copy_shapes (*layout, source_layout, trans, cm.source_cells (), cm.table (), lm.table ());
}

static void copy_tree_shapes3 (db::Layout *layout, const db::Layout &source_layout, const db::CellMapping &cm, const db::LayerMapping &lm)
{
  if (layout == &source_layout) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same layout")));
  }

  db::ICplxTrans trans (source_layout.dbu () / layout->dbu ());

  db::copy_shapes (*layout, source_layout, trans, cm.source_cells (), cm.table (), lm.table ());
}

static void move_tree_shapes2 (db::Layout *layout, db::Layout &source_layout, const db::CellMapping &cm)
{
  if (layout == &source_layout) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same layout")));
  }

  db::ICplxTrans trans (source_layout.dbu () / layout->dbu ());

  db::LayerMapping lm;
  lm.create_full (*layout, source_layout);

  db::move_shapes (*layout, source_layout, trans, cm.source_cells (), cm.table (), lm.table ());
}

static void move_tree_shapes3 (db::Layout *layout, db::Layout &source_layout, const db::CellMapping &cm, const db::LayerMapping &lm)
{
  if (layout == &source_layout) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same layout")));
  }

  db::ICplxTrans trans (source_layout.dbu () / layout->dbu ());

  db::move_shapes (*layout, source_layout, trans, cm.source_cells (), cm.table (), lm.table ());
}

static void dtransform (db::Layout *layout, const db::DTrans &trans)
{
  db::CplxTrans dbu_trans (layout->dbu ());
  layout->transform (db::Trans (dbu_trans.inverted () * db::DCplxTrans (trans) * dbu_trans));
}

static void dtransform_cplx (db::Layout *layout, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (layout->dbu ());
  layout->transform (dbu_trans.inverted () * trans * dbu_trans);
}

static db::LayerProperties get_properties (const db::Layout *layout, unsigned int index)
{
  if (layout->is_valid_layer (index)) {
    return layout->get_properties (index);
  } else {
    return db::LayerProperties ();
  }
}

static void set_properties (db::Layout *layout, unsigned int index, const db::LayerProperties &props)
{
  if (layout->is_valid_layer (index)) {
    layout->set_properties (index, props);
  }
}

Class<db::Layout> decl_Layout ("db", "Layout",
  gsi::constructor ("new", &layout_ctor_with_manager, gsi::arg ("manager"),
    "@brief Creates a layout object attached to a manager\n"
    "\n"
    "This constructor specifies a manager object which is used to "
    "store undo information for example.\n"
    "\n"
    "Starting with version 0.25, layouts created with the default constructor are "
    "always editable. Before that version, they inherited the editable flag from "
    "the application.\n"
  ) +
  gsi::constructor ("new", &layout_default_ctor,
    "@brief Creates a layout object\n"
    "\n"
    "Starting with version 0.25, layouts created with the default constructor are "
    "always editable. Before that version, they inherited the editable flag from "
    "the application."
  ) +
  gsi::constructor ("new", &editable_layout_ctor_with_manager, gsi::arg ("editable"), gsi::arg ("manager"),
    "@brief Creates a layout object attached to a manager\n"
    "\n"
    "This constructor specifies a manager object which is used to "
    "store undo information for example. It also allows one to specify whether "
    "the layout is editable. In editable mode, some optimizations are disabled "
    "and the layout can be manipulated through a variety of methods.\n"
    "\n"
    "This method was introduced in version 0.22.\n"
  ) +
  gsi::constructor ("new", &editable_layout_default_ctor, gsi::arg ("editable"),
    "@brief Creates a layout object\n"
    "\n"
    "This constructor specifies whether "
    "the layout is editable. In editable mode, some optimizations are disabled "
    "and the layout can be manipulated through a variety of methods.\n"
    "\n"
    "This method was introduced in version 0.22.\n"
  ) +
  gsi::method ("library", &db::Layout::library,
    "@brief Gets the library this layout lives in or nil if the layout is not part of a library\n"
    "This attribute has been introduced in version 0.27.5."
  ) +
  gsi::method ("refresh", &db::Layout::refresh,
    "@brief Calls \\Cell#refresh on all cells inside this layout\n"
    "This method is useful to recompute all PCells from a layout. Note that this does not "
    "update PCells which are linked from a library. To recompute PCells from a library, you need "
    "to use \\Library#refresh on the library object from which the PCells are imported.\n"
    "\n"
    "This method has been introduced in version 0.27.9."
  ) +
  gsi::method_ext ("add_meta_info", &layout_add_meta_info, gsi::arg ("info"),
    "@brief Adds meta information to the layout\n"
    "See \\LayoutMetaInfo for details about layouts and meta information."
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("clear_meta_info", static_cast<void (db::Layout::*) ()> (&db::Layout::clear_meta),
    "@brief Clears the meta information of the layout\n"
    "See \\LayoutMetaInfo for details about layouts and meta information."
    "\n"
    "This method has been introduced in version 0.28.8."
  ) +
  gsi::method ("remove_meta_info", static_cast<void (db::Layout::*) (const std::string &name)> (&db::Layout::remove_meta_info), gsi::arg ("name"),
    "@brief Removes meta information from the layout\n"
    "See \\LayoutMetaInfo for details about layouts and meta information."
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("meta_info_value", &layout_get_meta_info_value, gsi::arg ("name"),
    "@brief Gets the meta information value for a given name\n"
    "See \\LayoutMetaInfo for details about layouts and meta information.\n"
    "\n"
    "If no meta information with the given name exists, a nil value will be returned.\n"
    "A more generic version that delivers all fields of the meta information is \\meta_info.\n"
    "\n"
    "This method has been introduced in version 0.25. Starting with version 0.28.8, the value is of variant type instead of string only.\n"
  ) +
  gsi::factory_ext ("meta_info", &layout_get_meta_info, gsi::arg ("name"),
    "@brief Gets the meta information for a given name\n"
    "See \\LayoutMetaInfo for details about layouts and meta information.\n"
    "\n"
    "If no meta information with the given name exists, nil is returned.\n"
    "\n"
    "This method has been introduced in version 0.28.8.\n"
  ) +
  gsi::iterator_ext ("each_meta_info", &layout_each_meta_info,
    "@brief Iterates over the meta information of the layout\n"
    "See \\LayoutMetaInfo for details about layouts and meta information.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("technology_name", &db::Layout::technology_name,
    "@brief Gets the name of the technology this layout is associated with\n"
    "This method has been introduced in version 0.27. Before that, the technology has been kept in the 'technology' meta data element."
  ) +
  gsi::method ("technology", &db::Layout::technology,
    "@brief Gets the \\Technology object of the technology this layout is associated with or nil if the layout is not associated with a technology\n"
    "This method has been introduced in version 0.27. Before that, the technology has been kept in the 'technology' meta data element."
  ) +
  gsi::method ("technology_name=", &db::Layout::set_technology_name, gsi::arg ("name"),
    "@brief Sets the name of the technology this layout is associated with\n"
    "Changing the technology name will re-assess all library references because libraries can be technology specified. "
    "Cell layouts may be substituted during this re-assessment.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::method ("is_editable?", &db::Layout::is_editable,
    "@brief Returns a value indicating whether the layout is editable.\n"
    "@return True, if the layout is editable.\n"
    "If a layout is editable, in general manipulation methods are enabled and "
    "some optimizations are disabled (i.e. shape arrays are expanded).\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method ("clear", &db::Layout::clear, 
    "@brief Clears the layout\n"
    "\n"
    "Clears the layout completely."
  ) +
  method ("prop_id", (db::properties_id_type (db::Layout::*) () const) &db::Layout::prop_id,
    "@brief Gets the properties ID associated with the layout\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  method ("prop_id=", (void (db::Layout::*) (db::properties_id_type)) &db::Layout::prop_id, gsi::arg ("id"),
    "@brief Sets the properties ID associated with the layout\n"
    "This method is provided, if a properties ID has been derived already. Usually it's more convenient "
    "to use \\delete_property, \\set_property or \\property.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method_ext ("has_prop_id?", &layout_has_prop_id,
    "@brief Returns true, if the layout has user properties\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) +
  gsi::method_ext ("delete_property", &delete_layout_property, gsi::arg ("key"),
    "@brief Deletes the user property with the given key\n"
    "This method is a convenience method that deletes the property with the given key. "
    "It does nothing if no property with that key exists. Using that method is more "
    "convenient than creating a new property set with a new ID and assigning that properties ID.\n"
    "This method may change the properties ID.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  gsi::method_ext ("set_property", &set_layout_property, gsi::arg ("key"), gsi::arg ("value"),
    "@brief Sets the user property with the given key to the given value\n"
    "This method is a convenience method that sets the property with the given key to the given value. "
    "If no property with that key exists, it will create one. Using that method is more "
    "convenient than creating a new property set with a new ID and assigning that properties ID.\n"
    "This method may change the properties ID. "
    "Note: GDS only supports integer keys. OASIS supports numeric and string keys. "
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  gsi::method_ext ("property", &get_layout_property, gsi::arg ("key"),
    "@brief Gets the user property with the given key\n"
    "This method is a convenience method that gets the property with the given key. "
    "If no property with that key exists, it will return nil. Using that method is more "
    "convenient than using the properties ID to retrieve the property value. "
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  gsi::method_ext ("properties_id", &properties_id, gsi::arg ("properties"),
    "@brief Gets the properties ID for a given properties set\n"
    "\n"
    "Before a set of properties can be attached to a shape, it must be converted into an ID that "
    "is unique for that set. The properties set must be given as a list of pairs of variants, "
    "each pair describing a name and a value. The name acts as the key for the property and does not need to be a string (it can be an integer or double value as well).\n"
    "The backward conversion can be performed with the 'properties' method.\n"
    "\n"
    "@param properties The array of pairs of variants (both elements can be integer, double or string)\n"
    "@return The unique properties ID for that set"
  ) +
  gsi::method_ext ("properties", &properties, gsi::arg ("properties_id"),
    "@brief Gets the properties set for a given properties ID\n"
    "\n"
    "Basically performs the backward conversion of the 'properties_id' method. "
    "Given a properties ID, returns the properties set as an array of pairs of "
    "variants. In this array, each key and the value are stored as pairs (arrays with two elements).\n"
    "If the properties ID is not valid, an empty array is returned.\n"
    "\n"
    "@param properties_id The properties ID to get the properties for\n"
    "@return The array of variants (see \\properties_id)\n"
  ) +
  gsi::method ("unique_cell_name", &db::Layout::uniquify_cell_name, gsi::arg ("name"),
    "@brief Creates a new unique cell name from the given name\n"
    "@return A unique name derived from the argument\n"
    "\n"
    "If a cell with the given name exists, a suffix will be added to make the name unique. "
    "Otherwise, the argument will be returned unchanged.\n"
    "\n"
    "The returned name can be used to rename cells without risk of creating name clashes.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::method_ext ("top_cell", &top_cell,
    "@brief Returns the top cell object\n"
    "@return The \\Cell object of the top cell\n"
    "If the layout has a single top cell, this method returns the top cell's \\Cell object.\n"
    "If the layout does not have a top cell, this method returns \"nil\". If the layout has multiple\n"
    "top cells, this method raises an error.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) + 
  gsi::method_ext ("top_cells", &top_cells, 
    "@brief Returns the top cell objects\n"
    "@return The \\Cell objects of the top cells\n"
    "This method returns and array of \\Cell objects representing the top cells of the layout.\n"
    "This array can be empty, if the layout does not have a top cell (i.e. no cell at all).\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) + 
  gsi::method ("has_cell?", &db::Layout::has_cell, gsi::arg ("name"),
    "@brief Returns true if a cell with a given name exists\n"
    "Returns true, if the layout has a cell with the given name"
  ) +
  gsi::method_ext ("#cell_by_name", &cell_by_name, gsi::arg ("name"),
    "@brief Gets the cell index for a given name\n"
    "Returns the cell index for the cell with the given name. If no cell with this "
    "name exists, an exception is thrown."
    "\n"
    "From version 0.23 on, a version of the \\cell method is provided which returns a \\Cell object for the cell with the given name "
    "or \"nil\" if the name is not valid. This method replaces \\cell_by_name and \\has_cell?\n"
  ) +
  gsi::method ("cell_name", &db::Layout::cell_name, gsi::arg ("index"),
    "@brief Gets the name for a cell with the given index\n"
  ) +
  gsi::method_ext ("create_cell", &create_cell, gsi::arg ("name"),
    "@brief Creates a cell with the given name\n"
    "@param name The name of the cell to create\n"
    "@return The \\Cell object of the newly created cell.\n"
    "\n"
    "If a cell with that name already exists, the unique name will be chosen for the new cell "
    "consisting of the given name plus a suitable suffix.\n"
    "\n"
    "This method has been introduce in version 0.23 and replaces \\add_cell.\n"
  ) +
  gsi::method_ext ("create_cell", &create_cell2, gsi::arg ("pcell_name"), gsi::arg ("params"),
    "@brief Creates a cell as a PCell variant for the PCell with the given name\n"
    "@param pcell_name The name of the PCell and also the name of the cell to create\n"
    "@param params The PCell parameters (key/value dictionary)\n"
    "@return The \\Cell object of the newly created cell or an existing cell if the PCell has already been used with these parameters.\n"
    "\n"
    "PCells are instantiated by creating a PCell variant. A PCell variant is linked to the PCell and represents "
    "this PCell with a particular parameter set.\n"
    "\n"
    "This method will look up the PCell by the PCell name and create a new PCell variant "
    "for the given parameters. If the PCell has already been instantiated with the same parameters, the "
    "original variant will be returned. Hence this method is not strictly creating a cell - only if the required variant has "
    "not been created yet.\n"
    "\n"
    "The parameters are specified as a key/value dictionary with "
    "the names being the ones from the PCell declaration.\n"
    "\n"
    "If no PCell with the given name exists, nil is returned.\n"
    "\n"
    "This method has been introduce in version 0.24.\n"
  ) +
  gsi::method_ext ("create_cell", &create_cell3, gsi::arg ("name"), gsi::arg ("lib_name"),
    "@brief Creates a cell with the given name\n"
    "@param name The name of the library cell and the name of the cell to create\n"
    "@param lib_name The name of the library where to take the cell from\n"
    "@return The \\Cell object of the newly created cell or an existing cell if the library cell has already been used in this layout.\n"
    "\n"
    "Library cells are imported by creating a 'library proxy'. This is a cell which represents "
    "the library cell in the framework of the current layout. The library proxy is linked to the "
    "library and will be updated if the library cell is changed.\n"
    "\n"
    "This method will look up the cell by the given name in the specified library and create a new library proxy for this cell.\n"
    "If the same library cell has already been used, the original library proxy is returned. Hence, strictly speaking this "
    "method does not always create a new cell but may return a reference to an existing cell.\n"
    "\n"
    "If the library name is not valid, nil is returned.\n"
    "\n"
    "This method has been introduce in version 0.24.\n"
  ) +
  gsi::method_ext ("create_cell", &create_cell4, gsi::arg ("pcell_name"), gsi::arg ("lib_name"), gsi::arg ("params"),
    "@brief Creates a cell for a PCell with the given PCell name from the given library\n"
    "@param pcell_name The name of the PCell and also the name of the cell to create\n"
    "@param lib_name The name of the library where to take the PCell from\n"
    "@param params The PCell parameters (key/value dictionary)\n"
    "@return The \\Cell object of the newly created cell or an existing cell if this PCell has already been used with the given parameters\n"
    "\n"
    "This method will look up the PCell by the PCell name in the specified library and create a new PCell variant "
    "for the given parameters plus the library proxy. The parameters must be specified as a key/value dictionary with "
    "the names being the ones from the PCell declaration.\n"
    "\n"
    "If no PCell with the given name exists or the library name is not valid, nil is returned. Note that "
    "this function - despite the name - may not always create a new cell, but return an existing cell if the "
    "PCell from the library has already been used with the given parameters.\n"
    "\n"
    "This method has been introduce in version 0.24.\n"
  ) +
  gsi::method ("#add_cell", &db::Layout::add_cell, gsi::arg ("name"),
    "@brief Adds a cell with the given name\n"
    "@return The index of the newly created cell.\n"
    "\n"
    "From version 0.23 on this method is deprecated because another method exists which is more convenient because "
    "is returns a \\Cell object (\\create_cell).\n"
  ) +
  gsi::method ("rename_cell", &db::Layout::rename_cell, gsi::arg ("index"), gsi::arg ("name"),
    "@brief Renames the cell with given index\n"
    "The cell with the given index is renamed to the given name. NOTE: it is not ensured that the name is unique. "
    "This method allows assigning identical names to different cells which usually breaks things.\n"
    "Consider using \\unique_cell_name to generate truely unique names.\n"
  ) +
  gsi::method ("delete_cell", &db::Layout::delete_cell, gsi::arg ("cell_index"),
    "@brief Deletes a cell \n"
    "\n"
    "This deletes a cell but not the sub cells of the cell.\n"
    "These subcells will likely become new top cells unless they are used\n"
    "otherwise.\n"
    "All instances of this cell are deleted as well.\n"
    "Hint: to delete multiple cells, use \"delete_cells\" which is \n"
    "far more efficient in this case.\n"
    "\n"
    "@param cell_index The index of the cell to delete\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method_ext ("delete_cells", &delete_cells, gsi::arg ("cell_index_list"),
    "@brief Deletes multiple cells\n"
    "\n"
    "This deletes the cells but not the sub cells of these cells.\n"
    "These subcells will likely become new top cells unless they are used\n"
    "otherwise.\n"
    "All instances of these cells are deleted as well.\n"
    "\n"
    "@param cell_index_list An array of cell indices of the cells to delete\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method_ext ("prune_subcells", &prune_subcells, gsi::arg ("cell_index"), gsi::arg ("levels"),
    "@brief Deletes all sub cells of the cell which are not used otherwise down to the specified level of hierarchy\n"
    "\n"
    "This deletes all sub cells of the cell which are not used otherwise.\n"
    "All instances of the deleted cells are deleted as well.\n"
    "It is possible to specify how many levels of hierarchy below the given root cell are considered.\n"
    "\n"
    "@param cell_index The root cell from which to delete a sub cells\n"
    "@param levels The number of hierarchy levels to consider (-1: all, 0: none, 1: one level etc.)\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method_ext ("prune_cell", &prune_cell, gsi::arg ("cell_index"), gsi::arg ("levels"),
    "@brief Deletes a cell plus subcells not used otherwise\n"
    "\n"
    "This deletes a cell and also all sub cells of the cell which are not used otherwise.\n"
    "The number of hierarchy levels to consider can be specified as well. One level of hierarchy means that "
    "only the direct children of the cell are deleted with the cell itself.\n"
    "All instances of this cell are deleted as well.\n"
    "\n"
    "@param cell_index The index of the cell to delete\n"
    "@param levels The number of hierarchy levels to consider (-1: all, 0: none, 1: one level etc.)\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method ("delete_cell_rec", &db::Layout::delete_cell_rec, gsi::arg ("cell_index"),
    "@brief Deletes a cell plus all subcells\n"
    "\n"
    "This deletes a cell and also all sub cells of the cell.\n"
    "In contrast to \\prune_cell, all cells are deleted together with their instances even if they are used otherwise.\n"
    "\n"
    "@param cell_index The index of the cell to delete\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method ("insert", (void (db::Layout::*) (db::cell_index_type, int, const db::Region &)) &db::Layout::insert,
    gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Inserts a region into the given cell and layer\n"
    "If the region is (conceptionally) a flat region, it will be inserted into the cell's shapes "
    "list as a flat sequence of polygons.\n"
    "If the region is a deep (hierarchical) region, it will create a subhierarchy below the given "
    "cell and its shapes will be put into the respective cells. Suitable subcells will be picked "
    "for inserting the shapes. If a hierarchy already exists below the given cell, the algorithm will "
    "try to reuse this hierarchy.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method ("insert", (void (db::Layout::*) (db::cell_index_type, int, const db::Edges &)) &db::Layout::insert,
    gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("edges"),
    "@brief Inserts an edge collection into the given cell and layer\n"
    "If the edge collection is (conceptionally) flat, it will be inserted into the cell's shapes "
    "list as a flat sequence of edges.\n"
    "If the edge collection is deep (hierarchical), it will create a subhierarchy below the given "
    "cell and its edges will be put into the respective cells. Suitable subcells will be picked "
    "for inserting the edges. If a hierarchy already exists below the given cell, the algorithm will "
    "try to reuse this hierarchy.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method ("insert", (void (db::Layout::*) (db::cell_index_type, int, const db::EdgePairs &)) &db::Layout::insert,
    gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("edge_pairs"),
    "@brief Inserts an edge pair collection into the given cell and layer\n"
    "If the edge pair collection is (conceptionally) flat, it will be inserted into the cell's shapes "
    "list as a flat sequence of edge pairs.\n"
    "If the edge pair collection is deep (hierarchical), it will create a subhierarchy below the given "
    "cell and its edge pairs will be put into the respective cells. Suitable subcells will be picked "
    "for inserting the edge pairs. If a hierarchy already exists below the given cell, the algorithm will "
    "try to reuse this hierarchy.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method ("insert", (void (db::Layout::*) (db::cell_index_type, int, const db::Texts &)) &db::Layout::insert,
    gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("texts"),
    "@brief Inserts an text collection into the given cell and layer\n"
    "If the text collection is (conceptionally) flat, it will be inserted into the cell's shapes "
    "list as a flat sequence of texts.\n"
    "If the text collection is deep (hierarchical), it will create a subhierarchy below the given "
    "cell and its texts will be put into the respective cells. Suitable subcells will be picked "
    "for inserting the texts. If a hierarchy already exists below the given cell, the algorithm will "
    "try to reuse this hierarchy.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("flatten", &flatten, gsi::arg ("cell_index"), gsi::arg ("levels"), gsi::arg ("prune"),
    "@brief Flattens the given cell\n"
    "\n"
    "This method propagates all shapes and instances from the specified number of hierarchy levels below into the given cell.\n"
    "It also removes the instances of the cells from which the shapes came from, but does not remove the cells themselves if prune is set to false.\n"
    "If prune is set to true, these cells are removed if not used otherwise.\n"
    "\n"
    "@param cell_index The cell which should be flattened\n"
    "@param levels The number of hierarchy levels to flatten (-1: all, 0: none, 1: one level etc.)\n"
    "@param prune Set to true to remove orphan cells.\n"
    "\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method_ext ("flatten_into", &flatten_into, gsi::arg ("source_cell_index"), gsi::arg ("target_cell_index"), gsi::arg ("trans"), gsi::arg ("levels"),
    "@brief Flattens the given cell into another cell\n"
    "\n"
    "This method works like 'flatten', but allows specification of a target cell which can be different from the source cell plus "
    "a transformation which is applied for all shapes and instances in the target cell.\n"
    "\n"
    "In contrast to the 'flatten' method, the source cell is not modified.\n"
    "\n"
    "@param source_cell_index The source cell which should be flattened\n"
    "@param target_cell_index The target cell into which the resulting objects are written\n"
    "@param trans The transformation to apply on the output shapes and instances\n"
    "@param levels The number of hierarchy levels to flatten (-1: all, 0: none, 1: one level etc.)\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
  ) +
  gsi::method ("start_changes", &db::Layout::start_changes,
    "@brief Signals the start of an operation bringing the layout into invalid state\n"
    "\n"
    "This method should be called whenever the layout is\n"
    "about to be brought into an invalid state. After calling\n"
    "this method, \\under_construction? returns true which \n"
    "tells foreign code (i.e. the asynchronous painter or the cell tree view)\n"
    "not to use this layout object.\n\n"
    "This state is cancelled by the \\end_changes method.\n"
    "The start_changes method can be called multiple times\n"
    "and must be cancelled the same number of times.\n\n"
    "This method can be used to speed up certain operations. For example "
    "iterating over the layout with a \\RecursiveShapeIterator while modifying other layers of the layout "
    "can be very inefficient, because inside the loop the layout's state is invalidate and updated frequently.\n"
    "Putting a update and start_changes sequence before the loop (use both methods in that order!) and a "
    "end_changes call after the loop can improve the performance dramatically.\n\n"
    "In addition, it can be necessary to prevent redraw operations in certain cases by using start_changes .. "
    "end_changes, in particular when it is possible to put a layout object into an invalid state temporarily.\n\n"
    "While the layout is under construction \\update can be called to update the internal state explicitly if required.\n"
    "This for example might be necessary to update the cell bounding boxes or to redo the sorting for region queries.\n"
  ) +
  gsi::method ("end_changes", &db::Layout::end_changes,
    "@brief Cancels the \"in changes\" state (see \"start_changes\")\n"
  ) +
  gsi::method ("under_construction?", &db::Layout::under_construction,
    "@brief Returns true if the layout object is under construction\n"
    "\n"
    "A layout object is either under construction if a transaction\n"
    "is ongoing or the layout is brought into invalid state by\n"
    "\"start_changes\".\n"
  ) +
  gsi::method ("update", (void (db::Layout::*) ()) &db::Layout::force_update,
    "@brief Updates the internals of the layout\n"
    "This method updates the internal state of the layout. Usually this is done automatically\n"
    "This method is provided to ensure this explicitly. This can be useful while using \\start_changes and \\end_changes to wrap a performance-critical operation. "
    "See \\start_changes for more details."
  ) +
  gsi::method ("cleanup", &db::Layout::cleanup, gsi::arg ("cell_indexes_to_keep", std::set<db::cell_index_type> (), "[]"),
    "@brief Cleans up the layout\n"
    "This method will remove proxy objects that are no longer in use. After changing PCell parameters such "
    "proxy objects may still be present in the layout and are cached for later reuse. Usually they are cleaned up automatically, "
    "but in a scripting context it may be useful to clean up these cells explicitly.\n"
    "\n"
    "Use 'cell_indexes_to_keep' for specifying a list of cell indexes of PCell variants or library proxies you don't want to be cleaned up.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("dbu=", (void (db::Layout::*) (double)) &db::Layout::dbu, gsi::arg ("dbu"),
    "@brief Sets the database unit\n"
    "\n"
    "See \\dbu for a description of the database unit.\n"
  ) +
  gsi::method ("dbu", (double (db::Layout::*) () const) &db::Layout::dbu,
    "@brief Gets the database unit\n"
    "\n"
    "The database unit is the value of one units distance in micrometers.\n"
    "For numerical reasons and to be compliant with the GDS2 format, the database objects "
    "use integer coordinates. The basic unit of these coordinates is the database unit.\n" 
    "You can convert coordinates to micrometers by multiplying the integer value with the database unit.\n"
    "Typical values for the database unit are 0.001 micrometer (one nanometer).\n"
  ) +
  gsi::method_ext ("layer", &get_layer0,
    "@brief Creates a new internal layer\n"
    "\n"
    "This method will create a new internal layer and return the layer index for this layer.\n"
    "The layer does not have any properties attached to it. That means, it is not going to be saved "
    "to a layout file unless it is given database properties with \\set_info.\n"
    "\n"
    "This method is equivalent to \"layer(RBA::LayerInfo::new())\".\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("layer", &db::Layout::get_layer, gsi::arg ("info"),
    "@brief Finds or creates a layer with the given properties\n"
    "\n"
    "If a layer with the given properties already exists, this method will return the index of that layer."
    "If no such layer exists, a new one with these properties will be created and its index will be returned. "
    "If \"info\" is anonymous (info.anonymous? is true), a new layer will always be created.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("layer", &get_layer1, gsi::arg ("name"),
    "@brief Finds or creates a layer with the given name\n"
    "\n"
    "If a layer with the given name already exists, this method will return the index of that layer."
    "If no such layer exists, a new one with this name will be created and its index will be returned.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("layer", &get_layer2, gsi::arg ("layer"), gsi::arg ("datatype"),
    "@brief Finds or creates a layer with the given layer and datatype number\n"
    "\n"
    "If a layer with the given layer/datatype already exists, this method will return the index of that layer."
    "If no such layer exists, a new one with these properties will be created and its index will be returned.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("layer", &get_layer3, gsi::arg ("layer"), gsi::arg ("datatype"), gsi::arg ("name"),
    "@brief Finds or creates a layer with the given layer and datatype number and name\n"
    "\n"
    "If a layer with the given layer/datatype/name already exists, this method will return the index of that layer."
    "If no such layer exists, a new one with these properties will be created and its index will be returned.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("find_layer", &find_layer, gsi::arg ("info"),
    "@brief Finds a layer with the given properties\n"
    "\n"
    "If a layer with the given properties already exists, this method will return the index of that layer."
    "If no such layer exists, it will return nil.\n"
    "\n"
    "In contrast to \\layer, this method will also find layers matching by name only. "
    "For example:\n"
    "\n"
    "@code\n"
    "# finds layer '17/0' and 'name (17/0)':\n"
    "index = layout.find_layer(RBA::LayerInfo::new(17, 0))\n"
    "# finds layer 'name' (first priority), but also 'name (17/0)' (second priority):\n"
    "index = layout.find_layer(RBA::LayerInfo::new('name'))\n"
    "# note that this will not match layer 'name (17/0)' and create a new name-only layer:\n"
    "index = layout.layer(RBA::LayerInfo::new('name'))\n"
    "@/code\n"
    "\n"
    "This method has been introduced in version 0.23 and has been extended to name queries in version 0.28.11.\n"
  ) +
  gsi::method_ext ("find_layer", &find_layer1, gsi::arg ("name"),
    "@brief Finds a layer with the given name\n"
    "\n"
    "If a layer with the given name already exists, this method will return the index of that layer."
    "If no such layer exists, it will return nil.\n"
    "\n"
    "In contrast to \\layer, this method will also find numbered layers if the name matches. "
    "For example:\n"
    "\n"
    "@code\n"
    "# finds layer 'name' (first priority), but also 'name (17/0)' (second priority):\n"
    "index = layout.find_layer('name')\n"
    "# note that this will not match layer 'name (17/0)' and create a new name-only layer:\n"
    "index = layout.layer('name')\n"
    "@/code\n"
    "\n"
    "This method has been introduced in version 0.23 and has been extended to name queries in version 0.28.11.\n"
  ) +
  gsi::method_ext ("find_layer", &find_layer2, gsi::arg ("layer"), gsi::arg ("datatype"),
    "@brief Finds a layer with the given layer and datatype number\n"
    "\n"
    "If a layer with the given layer/datatype already exists, this method will return the index of that layer."
    "If no such layer exists, it will return nil.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("find_layer", &find_layer3, gsi::arg ("layer"), gsi::arg ("datatype"), gsi::arg ("name"),
    "@brief Finds a layer with the given layer and datatype number and name\n"
    "\n"
    "If a layer with the given layer/datatype/name already exists, this method will return the index of that layer."
    "If no such layer exists, it will return nil.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("insert_layer", (unsigned int (db::Layout::*) (const db::LayerProperties &)) &db::Layout::insert_layer, gsi::arg ("props"),
    "@brief Inserts a new layer with the given properties\n"
    "@return The index of the newly created layer\n"
  ) +
  gsi::method ("insert_layer_at", (void (db::Layout::*) (unsigned int, const db::LayerProperties &)) &db::Layout::insert_layer, gsi::arg ("index"), gsi::arg ("props"),
    "@brief Inserts a new layer with the given properties at the given index\n"
    "This method will associate the given layer info with the given layer index. If a layer with that index already "
    "exists, this method will change the properties of the layer with that index. Otherwise a new layer is created."
  ) +
  gsi::method ("guiding_shape_layer", &db::Layout::guiding_shape_layer,
    "@brief Returns the index of the guiding shape layer\n"
    "The guiding shape layer is used to store guiding shapes for PCells.\n"
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  gsi::method ("error_layer", &db::Layout::error_layer,
    "@brief Returns the index of the error layer\n"
    "The error layer is used to place error texts on it, for example when a PCell evaluation fails.\n"
    "\n"
    "This method has been added in version 0.23.13.\n"
  ) +
  gsi::method ("insert_special_layer", (unsigned int (db::Layout::*) (const db::LayerProperties &)) &db::Layout::insert_special_layer, gsi::arg ("props"),
    "@brief Inserts a new special layer with the given properties\n"
    "\n"
    "Special layers can be used to represent objects that should not participate in normal viewing or other "
    "related operations. Special layers are not reported as valid layers.\n"
    "\n"
    "@return The index of the newly created layer\n"
  ) +
  gsi::method ("insert_special_layer_at", (void (db::Layout::*) (unsigned int, const db::LayerProperties &)) &db::Layout::insert_special_layer, gsi::arg ("index"), gsi::arg ("props"),
    "@brief Inserts a new special layer with the given properties at the given index\n"
    "\n"
    "See \\insert_special_layer for a description of special layers."
  ) +
  gsi::method_ext ("set_info", &set_properties, gsi::arg ("index"), gsi::arg ("props"),
    "@brief Sets the info structure for a specified layer\n"
  ) +
  gsi::method_ext ("get_info", &get_properties, gsi::arg ("index"),
    "@brief Gets the info structure for a specified layer\n"
    "If the layer index is not a valid layer index, an empty LayerProperties object will be returned."
  ) +
  gsi::method ("cells", &db::Layout::cells,
    "@brief Returns the number of cells\n"
    "\n"
    "@return The number of cells (the maximum cell index)\n"
  ) +
  gsi::method_ext ("cells", &cells_from_name, gsi::arg ("name_filter"),
    "@brief Gets the cell objects for a given name filter\n"
    "\n"
    "@param name_filter The cell name filter (glob pattern)\n"
    "@return A list of \\Cell object of the cells matching the pattern\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  gsi::method_ext ("cell", &cell_from_name, gsi::arg ("name"),
    "@brief Gets a cell object from the cell name\n"
    "\n"
    "@param name The cell name\n"
    "@return A reference to the cell (a \\Cell object)\n"
    "\n"
    "If name is not a valid cell name, this method will return \"nil\".\n"
    "This method has been introduced in version 0.23 and replaces \\cell_by_name.\n"
  ) +
  gsi::method_ext ("cell", &cell_from_index, gsi::arg ("i"),
    "@brief Gets a cell object from the cell index\n"
    "\n"
    "@param i The cell index\n"
    "@return A reference to the cell (a \\Cell object)\n"
    "\n"
    "If the cell index is not a valid cell index, this method will raise an error. "
    "Use \\is_valid_cell_index? to test whether a given cell index is valid.\n"
  ) +
  gsi::iterator ("each_cell", (db::Layout::iterator (db::Layout::*) ()) &db::Layout::begin, (db::Layout::iterator (db::Layout::*) ()) &db::Layout::end,
    "@brief Iterates the unsorted cell list\n"
  ) +
  gsi::iterator ("each_cell_bottom_up", (db::Layout::bottom_up_iterator (db::Layout::*) ()) &db::Layout::begin_bottom_up, (db::Layout::bottom_up_iterator (db::Layout::*) ()) &db::Layout::end_bottom_up,
    "@brief Iterates the bottom-up sorted cell list\n"
    "\n"
    "In bottom-up traversal a cell is not delivered before\n"
    "the last child cell of this cell has been delivered.\n"
    "The bottom-up iterator does not deliver cells but cell\n"
    "indices actually.\n"
  ) +
  gsi::iterator ("each_cell_top_down", (db::Layout::top_down_iterator (db::Layout::*) ()) &db::Layout::begin_top_down, (db::Layout::top_down_iterator (db::Layout::*) ()) &db::Layout::end_top_down,
    "@brief Iterates the top-down sorted cell list\n"
    "\n"
    "The top-down cell list has the property of delivering all\n"
    "cells before they are instantiated. In addition the first\n"
    "cells are all top cells. There is at least one top cell.\n"
    "The top-down iterator does not deliver cells but cell\n"
    "indices actually.\n"
    "@brief begin iterator of the top-down sorted cell list\n"
  ) +
  gsi::iterator ("each_top_cell", (db::Layout::top_down_iterator (db::Layout::*) ()) &db::Layout::begin_top_down, (db::Layout::top_down_iterator (db::Layout::*) ()) &db::Layout::end_top_cells,
    "@brief Iterates the top cells\n"
    "A layout may have an arbitrary number of top cells. The usual case however is that there is one "
    "top cell."
  ) +
  gsi::method ("swap_layers", &db::Layout::swap_layers, gsi::arg ("a"), gsi::arg ("b"),
    "@brief Swap two layers\n"
    "\n"
    "Swaps the shapes of both layers.\n"
    "\n"
    "This method was introduced in version 0.19.\n"
    "\n"
    "@param a The first of the layers to swap.\n"
    "@param b The second of the layers to swap.\n"
  ) +
  gsi::method ("move_layer", static_cast<void (db::Layout::*) (unsigned int, unsigned int)> (&db::Layout::move_layer), gsi::arg ("src"), gsi::arg ("dest"),
    "@brief Moves a layer\n"
    "\n"
    "Moves a layer from the source to the destination layer. The target is not cleared before, so that this method \n"
    "merges shapes from the source with the destination layer. The source layer is empty after that operation.\n"
    "\n"
    "@param src The layer index of the source layer.\n"
    "@param dest The layer index of the destination layer.\n"
    "\n"
    "This method was introduced in version 0.19.\n"
  ) +
  gsi::method ("move_layer", static_cast<void (db::Layout::*) (unsigned int, unsigned int, unsigned int)> (&db::Layout::move_layer), gsi::arg ("src"), gsi::arg ("dest"), gsi::arg ("flags"),
    "@brief Moves a layer (selected shape types only)\n"
    "\n"
    "Moves a layer from the source to the destination layer. The target is not cleared before, so that this method \n"
    "merges shapes from the source with the destination layer. The copied shapes are removed from the source layer.\n"
    "\n"
    "@param src The layer index of the source layer.\n"
    "@param dest The layer index of the destination layer.\n"
    "@param flags A combination of the shape type flags from \\Shapes, S... constants\n"
    "\n"
    "This method variant has been introduced in version 0.28.9.\n"
  ) +
  gsi::method ("copy_layer", static_cast<void (db::Layout::*) (unsigned int, unsigned int)> (&db::Layout::copy_layer), gsi::arg ("src"), gsi::arg ("dest"),
    "@brief Copies a layer\n"
    "\n"
    "Copies a layer from the source to the destination layer. The destination layer is not cleared before, so that this method \n"
    "merges shapes from the source with the destination layer.\n"
    "\n"
    "@param src The layer index of the source layer.\n"
    "@param dest The layer index of the destination layer.\n"
    "\n"
    "This method was introduced in version 0.19.\n"
  ) +
  gsi::method ("copy_layer", static_cast<void (db::Layout::*) (unsigned int, unsigned int, unsigned int)> (&db::Layout::copy_layer), gsi::arg ("src"), gsi::arg ("dest"), gsi::arg ("flags"),
    "@brief Copies a layer (selected shape types only)\n"
    "\n"
    "Copies a layer from the source to the destination layer. The destination layer is not cleared before, so that this method \n"
    "merges shapes from the source with the destination layer.\n"
    "\n"
    "@param src The layer index of the source layer.\n"
    "@param dest The layer index of the destination layer.\n"
    "@param flags A combination of the shape type flags from \\Shapes, S... constants\n"
    "\n"
    "This method variant has been introduced in version 0.28.9.\n"
  ) +
  gsi::method ("clear_layer", static_cast<void (db::Layout::*) (unsigned int)> (&db::Layout::clear_layer), gsi::arg ("layer_index"),
    "@brief Clears a layer\n"
    "\n"
    "Clears the layer: removes all shapes.\n"
    "\n"
    "This method was introduced in version 0.19.\n"
    "\n"
    "@param layer_index The index of the layer to delete.\n"
  ) +                               
  gsi::method ("clear_layer", static_cast<void (db::Layout::*) (unsigned int, unsigned int)> (&db::Layout::clear_layer), gsi::arg ("layer_index"), gsi::arg ("flags"),
    "@brief Clears a layer (given shape types only)\n"
    "\n"
    "Clears the layer: removes all shapes for the given shape types.\n"
    "\n"
    "This method was introduced in version 0.28.9.\n"
    "\n"
    "@param layer_index The index of the layer to delete.\n"
    "@param flags The type selector for the shapes to delete (see \\Shapes class, S... constants).\n"
  ) +
  gsi::method ("delete_layer", &db::Layout::delete_layer, gsi::arg ("layer_index"),
    "@brief Deletes a layer\n"
    "\n"
    "This method frees the memory allocated for the shapes of this layer and remembers the\n"
    "layer's index for reuse when the next layer is allocated.\n"
    "\n"
    "@param layer_index The index of the layer to delete.\n"
  ) +
  gsi::method_ext ("layer_indexes|#layer_indices", &layer_indexes,
    "@brief Gets a list of valid layer's indices\n"
    "This method returns an array with layer indices representing valid layers.\n"
    "\n"
    "This method has been introduced in version 0.19.\n"
  ) +
  gsi::method_ext ("layer_infos", &layer_infos,
    "@brief Gets a list of valid layer's properties\n"
    "The method returns an array with layer properties representing valid layers.\n"
    "The sequence and length of this list corresponds to that of \\layer_indexes.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("layers", &db::Layout::layers,
    "@brief Returns the number of layers\n"
    "The number of layers reports the maximum (plus 1) layer index used so far. Not all of the layers with "
    "an index in the range of 0 to layers-1 needs to be a valid layer. These layers can be either valid, "
    "special or unused. Use \\is_valid_layer? and \\is_special_layer? to test for the first two states.\n"
  ) +
  gsi::method_ext ("copy_tree_shapes", &copy_tree_shapes2, gsi::arg ("source_layout"), gsi::arg ("cell_mapping"),
    "@brief Copies the shapes for all given mappings in the \\CellMapping object\n"
    "@param source_layout The layout where to take the shapes from\n"
    "@param cell_mapping The cell mapping object that determines how cells are identified between source and target layout\n"
    "\n"
    "Provide a \\CellMapping object to specify pairs of cells which are mapped from the source layout to this "
    "layout. When constructing such a cell mapping object for example with \\CellMapping#for_multi_cells_full, use self "
    "as the target layout. During the cell mapping construction, the cell mapper will usually create a suitable target "
    "hierarchy already. After having completed the cell mapping, use \\copy_tree_shapes to copy over the shapes from "
    "the source to the target layout.\n"
    "\n"
    "This method has been added in version 0.26.8.\n"
  ) +
  gsi::method_ext ("copy_tree_shapes", &copy_tree_shapes3, gsi::arg ("source_layout"), gsi::arg ("cell_mapping"), gsi::arg ("layer_mapping"),
    "@brief Copies the shapes for all given mappings in the \\CellMapping object using the given layer mapping\n"
    "@param source_layout The layout where to take the shapes from\n"
    "@param cell_mapping The cell mapping object that determines how cells are identified between source and target layout\n"
    "@param layer_mapping Specifies which layers are copied from the source layout to the target layout\n"
    "\n"
    "Provide a \\CellMapping object to specify pairs of cells which are mapped from the source layout to this "
    "layout. When constructing such a cell mapping object for example with \\CellMapping#for_multi_cells_full, use self "
    "as the target layout. During the cell mapping construction, the cell mapper will usually create a suitable target "
    "hierarchy already. After having completed the cell mapping, use \\copy_tree_shapes to copy over the shapes from "
    "the source to the target layout.\n"
    "\n"
    "This method has been added in version 0.26.8.\n"
  ) +
  gsi::method_ext ("move_tree_shapes", &move_tree_shapes2, gsi::arg ("source_layout"), gsi::arg ("cell_mapping"),
    "@brief Moves the shapes for all given mappings in the \\CellMapping object\n"
    "\n"
    "This method acts like the corresponding \\copy_tree_shapes method, but removes the shapes from the source layout "
    "after they have been copied.\n"
    "\n"
    "This method has been added in version 0.26.8.\n"
  ) +
  gsi::method_ext ("move_tree_shapes", &move_tree_shapes3, gsi::arg ("source_layout"), gsi::arg ("cell_mapping"), gsi::arg ("layer_mapping"),
    "@brief Moves the shapes for all given mappings in the \\CellMapping object using the given layer mapping\n"
    "\n"
    "This method acts like the corresponding \\copy_tree_shapes method, but removes the shapes from the source layout "
    "after they have been copied.\n"
    "\n"
    "This method has been added in version 0.26.8.\n"
  ) +
  gsi::method_ext ("scale_and_snap", &scale_and_snap1, gsi::arg ("cell"), gsi::arg ("grid"), gsi::arg ("mult"), gsi::arg ("div"),
    "@brief Scales and snaps the layout below a given cell by the given rational factor and snaps to the given grid\n"
    "\n"
    "This method is useful to scale a layout by a non-integer factor. The "
    "scale factor is given by the rational number mult / div. After scaling, the "
    "layout will be snapped to the given grid.\n"
    "\n"
    "Snapping happens 'as-if-flat' - that is, touching edges will stay touching, regardless of their "
    "hierarchy path. To achieve this, this method usually needs to produce cell variants.\n"
    "\n"
    "This method has been introduced in version 0.26.1.\n"
  ) +
  gsi::method_ext ("scale_and_snap", &scale_and_snap2, gsi::arg ("cell_index"), gsi::arg ("grid"), gsi::arg ("mult"), gsi::arg ("div"),
    "@brief Scales and snaps the layout below a given cell by the given rational factor and snaps to the given grid\n"
    "\n"
    "Like the other version of \\scale_and_snap, but taking a cell index for the argument.\n"
    "\n"
    "This method has been introduced in version 0.26.1.\n"
  ) +
  gsi::method ("transform", (void (db::Layout::*) (const db::Trans &t)) &db::Layout::transform, gsi::arg ("trans"),
    "@brief Transforms the layout with the given transformation\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("transform", (void (db::Layout::*) (const db::ICplxTrans &t)) &db::Layout::transform, gsi::arg ("trans"),
    "@brief Transforms the layout with the given complex integer transformation\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("transform", &dtransform, gsi::arg ("trans"),
    "@brief Transforms the layout with the given transformation, which is in micrometer units\n"
    "This variant will internally translate the transformation's displacement into database units. "
    "Apart from that, it behaves identical to the version with a \\Trans argument.\n"
    "\n"
    "This variant has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("transform", &dtransform_cplx, gsi::arg ("trans"),
    "@brief Transforms the layout with the given complex integer transformation, which is in micrometer units\n"
    "This variant will internally translate the transformation's displacement into database units. "
    "Apart from that, it behaves identical to the version with a \\ICplxTrans argument.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("is_valid_cell_index?", &db::Layout::is_valid_cell_index, gsi::arg ("cell_index"),
    "@brief Returns true, if a cell index is a valid index\n"
    "\n"
    "@return true, if this is the case\n"
    "This method has been added in version 0.20.\n"
  ) +
  gsi::method ("is_valid_layer?", &db::Layout::is_valid_layer, gsi::arg ("layer_index"),
    "@brief Returns true, if a layer index is a valid normal layout layer index\n"
    "\n"
    "@return true, if this is the case\n"
  ) +
  gsi::method ("is_free_layer?", &db::Layout::is_free_layer, gsi::arg ("layer_index"),
    "@brief Returns true, if a layer index is a free (unused) layer index\n"
    "\n"
    "@return true, if this is the case\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  gsi::method ("is_special_layer?", &db::Layout::is_special_layer, gsi::arg ("layer_index"),
    "@brief Returns true, if a layer index is a special layer index\n"
    "\n"
    "@return true, if this is the case\n"
  ) +
  gsi::method_ext ("begin_shapes", &begin_shapes2, gsi::arg ("cell"), gsi::arg ("layer"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer\n"
    "@param cell The cell object of the initial (top) cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version is convenience overload which takes a cell object instead of a cell index.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec instead.\n"
    "\n"
    "This method has been added in version 0.24.\n"
  ) +
  gsi::method_ext ("#begin_shapes", &begin_shapes, gsi::arg ("cell_index"), gsi::arg ("layer"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer\n"
    "@param cell_index The index of the initial (top) cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec instead.\n"
    "\n"
    "This method has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("#begin_shapes_touching", &begin_shapes_touching, gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer using a region search\n"
    "@param cell_index The index of the starting cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@param region The search region\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version gives an iterator delivering shapes whose bounding box touches the given region.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec_touching instead.\n"
    "\n"
    "This method has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("#begin_shapes_touching", &begin_shapes_touching2, gsi::arg ("cell"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer using a region search\n"
    "@param cell The cell object for the starting cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@param region The search region\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version gives an iterator delivering shapes whose bounding box touches the given region.\n"
    "It is convenience overload which takes a cell object instead of a cell index.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec_touching instead.\n"
    "\n"
    "This method has been added in version 0.24.\n"
  ) +
  gsi::method_ext ("#begin_shapes_overlapping", &begin_shapes_overlapping, gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer using a region search\n"
    "@param cell_index The index of the starting cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@param region The search region\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version gives an iterator delivering shapes whose bounding box overlaps the given region.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec_overlapping instead.\n"
    "\n"
    "This method has been added in version 0.18.\n"
  ) +
  gsi::method_ext ("#begin_shapes_overlapping", &begin_shapes_overlapping2, gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer using a region search\n"
    "@param cell The cell object for the starting cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@param region The search region\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version gives an iterator delivering shapes whose bounding box overlaps the given region.\n"
    "It is convenience overload which takes a cell object instead of a cell index.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec_overlapping instead.\n"
    "\n"
    "This method has been added in version 0.24.\n"
  ) +
  gsi::method_ext ("#begin_shapes_touching", &begin_shapes_touching_um, gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer using a region search, the region given in micrometer units\n"
    "@param cell_index The index of the starting cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@param region The search region as a \\DBox object in micrometer units\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version gives an iterator delivering shapes whose bounding box touches the given region.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec_touching instead.\n"
    "\n"
    "This variant has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("#begin_shapes_touching", &begin_shapes_touching2_um, gsi::arg ("cell"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer using a region search, the region given in micrometer units\n"
    "@param cell The cell object for the starting cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@param region The search region as a \\DBox object in micrometer units\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version gives an iterator delivering shapes whose bounding box touches the given region.\n"
    "It is convenience overload which takes a cell object instead of a cell index.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec_touching instead.\n"
    "\n"
    "This variant has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("#begin_shapes_overlapping", &begin_shapes_overlapping_um, gsi::arg ("cell_index"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer using a region search, the region given in micrometer units\n"
    "@param cell_index The index of the starting cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@param region The search region as a \\DBox object in micrometer units\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version gives an iterator delivering shapes whose bounding box overlaps the given region.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec_overlapping instead.\n"
    "\n"
    "This variant has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("#begin_shapes_overlapping", &begin_shapes_overlapping2_um, gsi::arg ("cell"), gsi::arg ("layer"), gsi::arg ("region"),
    "@brief Delivers a recursive shape iterator for the shapes below the given cell on the given layer using a region search, the region given in micrometer units\n"
    "@param cell The cell object for the starting cell\n"
    "@param layer The layer from which to get the shapes\n"
    "@param region The search region as a \\DBox object in micrometer units\n"
    "@return A suitable iterator\n"
    "\n"
    "For details see the description of the \\RecursiveShapeIterator class.\n"
    "This version gives an iterator delivering shapes whose bounding box overlaps the given region.\n"
    "It is convenience overload which takes a cell object instead of a cell index.\n"
    "\n"
    "This method is deprecated. Use \\Cell#begin_shapes_rec_overlapping instead.\n"
    "\n"
    "This variant has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("#write", &write_options2, gsi::arg ("filename"), gsi::arg ("gzip"), gsi::arg ("options"),
    "@brief Writes the layout to a stream file\n"
    "@param filename The file to which to write the layout\n"
    "@param gzip Ignored\n"
    "@param options The option set to use for writing. See \\SaveLayoutOptions for details\n"
    "\n"
    "Starting with version 0.23, this variant is deprecated since the more convenient "
    "variant with two parameters automatically determines the compression mode from the file name. "
    "The gzip parameter is ignored staring with version 0.23.\n"
  ) +
  gsi::method_ext ("write", &write_options1, gsi::arg ("filename"), gsi::arg ("options"),
    "@brief Writes the layout to a stream file\n"
    "@param filename The file to which to write the layout\n"
    "@param options The option set to use for writing. See \\SaveLayoutOptions for details\n"
    "\n"
    "This version automatically determines the compression mode from the file name. "
    "The file is written with zlib compression if the suffix is \".gz\" or \".gzip\".\n"
    "\n"
    "This variant has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("write", &write_simple, gsi::arg ("filename"),
    "@brief Writes the layout to a stream file\n"
    "@param filename The file to which to write the layout\n"
  ) + 
  gsi::method_ext ("clip", &clip, gsi::arg ("cell"), gsi::arg ("box"),
    "@brief Clips the given cell by the given rectangle and produce a new cell with the clip\n"
    "@param cell The cell index of the cell to clip\n"
    "@param box The clip box in database units\n"
    "@return The index of the new cell\n"
    "\n"
    "This method will cut a rectangular region given by the box from the given cell. The clip will "
    "be stored in a new cell whose index is returned. The clip will be performed hierarchically. The "
    "resulting cell will hold a hierarchy of child cells, which are potentially clipped versions of "
    "child cells of the original cell." 
    "\n"
    "This method has been added in version 0.21.\n"
  ) + 
  gsi::method_ext ("clip", &clip_dbox, gsi::arg ("cell"), gsi::arg ("box"),
    "@brief Clips the given cell by the given rectangle and produce a new cell with the clip\n"
    "@param cell The cell index of the cell to clip\n"
    "@param box The clip box in micrometer units\n"
    "@return The index of the new cell\n"
    "\n"
    "This variant which takes a micrometer-unit box has been added in version 0.28."
  ) +
  gsi::method_ext ("clip", &clip_cell, gsi::arg ("cell"), gsi::arg ("box"),
    "@brief Clips the given cell by the given rectangle and produce a new cell with the clip\n"
    "@param cell The cell reference of the cell to clip\n"
    "@param box The clip box in database units\n"
    "@return The reference to the new cell\n"
    "\n"
    "This variant which takes cell references instead of cell indexes has been added in version 0.28."
  ) +
  gsi::method_ext ("clip", &clip_cell_dbox, gsi::arg ("cell"), gsi::arg ("box"),
    "@brief Clips the given cell by the given rectangle and produce a new cell with the clip\n"
    "@param cell The cell reference of the cell to clip\n"
    "@param box The clip box in micrometer units\n"
    "@return The reference to the new cell\n"
    "\n"
    "This variant which takes a micrometer-unit box and cell references has been added in version 0.28."
  ) +
  gsi::method_ext ("clip_into", &clip_into, gsi::arg ("cell"), gsi::arg ("target"), gsi::arg ("box"),
    "@brief Clips the given cell by the given rectangle and produce a new cell with the clip\n"
    "@param cell The cell index of the cell to clip\n"
    "@param box The clip box in database units\n"
    "@param target The target layout\n"
    "@return The index of the new cell in the target layout\n"
    "\n"
    "This method will cut a rectangular region given by the box from the given cell. The clip will "
    "be stored in a new cell in the target layout. The clip will be performed hierarchically. The "
    "resulting cell will hold a hierarchy of child cells, which are potentially clipped versions of "
    "child cells of the original cell.\n"
    "\n"
    "Please note that it is important that the database unit of the " 
    "target layout is identical to the database unit of the source layout to achieve the desired results."
    "This method also assumes that the target layout holds the same layers than the source layout. It will "
    "copy shapes to the same layers than they have been on the original layout. "
    "\n"
    "This method has been added in version 0.21.\n"
  ) + 
  gsi::method_ext ("clip_into", &clip_into_dbox, gsi::arg ("cell"), gsi::arg ("target"), gsi::arg ("box"),
    "@brief Clips the given cell by the given rectangle and produce a new cell with the clip\n"
    "@param cell The cell index of the cell to clip\n"
    "@param box The clip box in micrometer units\n"
    "@param target The target layout\n"
    "@return The index of the new cell in the target layout\n"
    "\n"
    "This variant which takes a micrometer-unit box has been added in version 0.28."
  ) +
  gsi::method_ext ("clip_into", &clip_into_cell, gsi::arg ("cell"), gsi::arg ("target"), gsi::arg ("box"),
    "@brief Clips the given cell by the given rectangle and produce a new cell with the clip\n"
    "@param cell The reference to the cell to clip\n"
    "@param box The clip box in database units\n"
    "@param target The target layout\n"
    "@return The reference to the new cell in the target layout\n"
    "\n"
    "This variant which takes cell references instead of cell indexes has been added in version 0.28."
  ) +
  gsi::method_ext ("clip_into", &clip_into_cell_dbox, gsi::arg ("cell"), gsi::arg ("target"), gsi::arg ("box"),
    "@brief Clips the given cell by the given rectangle and produce a new cell with the clip\n"
    "@param cell The reference to the cell to clip\n"
    "@param box The clip box in micrometer units\n"
    "@param target The target layout\n"
    "@return The reference to the new cell in the target layout\n"
    "\n"
    "This variant which takes a micrometer-unit box and cell references has been added in version 0.28."
  ) +
  gsi::method_ext ("multi_clip", &multi_clip, gsi::arg ("cell"), gsi::arg ("boxes"),
    "@brief Clips the given cell by the given rectangles and produces new cells with the clips, one for each rectangle.\n"
    "@param cell The cell index of the cell to clip\n"
    "@param boxes The clip boxes in database units\n"
    "@return The indexes of the new cells\n"
    "\n"
    "This method will cut rectangular regions given by the boxes from the given cell. The clips will "
    "be stored in a new cells whose indexed are returned. The clips will be performed hierarchically. The "
    "resulting cells will hold a hierarchy of child cells, which are potentially clipped versions of "
    "child cells of the original cell. This version is somewhat more efficient than doing individual clips "
    "because the clip cells may share clipped versions of child cells.\n" 
    "\n"
    "This method has been added in version 0.21.\n"
  ) + 
  gsi::method_ext ("multi_clip", &multi_clip_dboxes, gsi::arg ("cell"), gsi::arg ("boxes"),
    "@brief Clips the given cell by the given rectangles and produces new cells with the clips, one for each rectangle.\n"
    "@param cell The cell index of the cell to clip\n"
    "@param boxes The clip boxes in micrometer units\n"
    "@return The indexes of the new cells\n"
    "\n"
    "This variant which takes micrometer-unit boxes has been added in version 0.28."
  ) +
  gsi::method_ext ("multi_clip", &multi_clip_cells, gsi::arg ("cell"), gsi::arg ("boxes"),
    "@brief Clips the given cell by the given rectangles and produces new cells with the clips, one for each rectangle.\n"
    "@param cell The reference to the cell to clip\n"
    "@param boxes The clip boxes in database units\n"
    "@return The references to the new cells\n"
    "\n"
    "This variant which takes cell references has been added in version 0.28."
  ) +
  gsi::method_ext ("multi_clip", &multi_clip_cells_dboxes, gsi::arg ("cell"), gsi::arg ("boxes"),
    "@brief Clips the given cell by the given rectangles and produces new cells with the clips, one for each rectangle.\n"
    "@param cell The reference to the cell to clip\n"
    "@param boxes The clip boxes in micrometer units\n"
    "@return The references to the new cells\n"
    "\n"
    "This variant which takes cell references and micrometer-unit boxes has been added in version 0.28."
  ) +
  gsi::method_ext ("multi_clip_into", &multi_clip_into, gsi::arg ("cell"), gsi::arg ("target"), gsi::arg ("boxes"),
    "@brief Clips the given cell by the given rectangles and produces new cells with the clips, one for each rectangle.\n"
    "@param cell The cell index of the cell to clip\n"
    "@param boxes The clip boxes in database units\n"
    "@param target The target layout\n"
    "@return The indexes of the new cells\n"
    "\n"
    "This method will cut rectangular regions given by the boxes from the given cell. The clips will "
    "be stored in a new cells in the given target layout. The clips will be performed hierarchically. The "
    "resulting cells will hold a hierarchy of child cells, which are potentially clipped versions of "
    "child cells of the original cell. This version is somewhat more efficient than doing individual clips "
    "because the clip cells may share clipped versions of child cells.\n" 
    "\n"
    "Please note that it is important that the database unit of the " 
    "target layout is identical to the database unit of the source layout to achieve the desired results. "
    "This method also assumes that the target layout holds the same layers than the source layout. It will "
    "copy shapes to the same layers than they have been on the original layout.\n"
    "\n"
    "This method has been added in version 0.21.\n"
  ) +
  gsi::method_ext ("multi_clip_into", &multi_clip_into_dboxes, gsi::arg ("cell"), gsi::arg ("target"), gsi::arg ("boxes"),
    "@brief Clips the given cell by the given rectangles and produces new cells with the clips, one for each rectangle.\n"
    "@param cell The cell index of the cell to clip\n"
    "@param boxes The clip boxes in database units\n"
    "@param target The target layout\n"
    "@return The indexes of the new cells\n"
    "\n"
    "This variant which takes micrometer-unit boxes has been added in version 0.28."
  ) +
  gsi::method_ext ("multi_clip_into", &multi_clip_into_cells, gsi::arg ("cell"), gsi::arg ("target"), gsi::arg ("boxes"),
    "@brief Clips the given cell by the given rectangles and produces new cells with the clips, one for each rectangle.\n"
    "@param cell The reference the cell to clip\n"
    "@param boxes The clip boxes in database units\n"
    "@param target The target layout\n"
    "@return The references to the new cells\n"
    "\n"
    "This variant which takes cell references boxes has been added in version 0.28."
  ) +
  gsi::method_ext ("multi_clip_into", &multi_clip_into_cells_dboxes, gsi::arg ("cell"), gsi::arg ("target"), gsi::arg ("boxes"),
    "@brief Clips the given cell by the given rectangles and produces new cells with the clips, one for each rectangle.\n"
    "@param cell The reference the cell to clip\n"
    "@param boxes The clip boxes in micrometer units\n"
    "@param target The target layout\n"
    "@return The references to the new cells\n"
    "\n"
    "This variant which takes cell references and micrometer-unit boxes has been added in version 0.28."
  ) +
  gsi::method ("convert_cell_to_static", &db::Layout::convert_cell_to_static, gsi::arg ("cell_index"),
    "@brief Converts a PCell or library cell to a usual (static) cell\n"
    "@return The index of the new cell\n"
    "This method will create a new cell which contains the static representation of the "
    "PCell or library proxy given by \"cell_index\". If that cell is not a PCell or library "
    "proxy, it won't be touched and the input cell index is returned.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) +
  gsi::method ("add_lib_cell", &db::Layout::get_lib_proxy, gsi::arg ("library"), gsi::arg ("lib_cell_index"),
    "@brief Imports a cell from the library\n"
    "@param library The reference to the library from which to import the cell\n"
    "@param lib_cell_index The index of the imported cell in the library\n"
    "@return The cell index of the new proxy cell in this layout\n"
    "This method imports the given cell from the library and creates a new proxy cell.\n"
    "The proxy cell acts as a pointer to the actual cell which still resides in the \n"
    "library (precisely: in library.layout). The name of the new cell will be the name of\n"
    "library cell. \n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method ("add_pcell_variant", &db::Layout::get_pcell_variant_dict, gsi::arg ("pcell_id"), gsi::arg ("parameters"),
    "@brief Creates a PCell variant for the given PCell ID with the parameters given as a name/value dictionary\n"
    "@return The cell index of the pcell variant proxy cell\n"
    "This method will create a PCell variant proxy for a local PCell definition.\n"
    "It will create the PCell variant for the given parameters. Note that this method \n"
    "does not allow one to create PCell instances for PCells located in a library. Use\n"
    "\\add_pcell_variant with the library parameter for that purpose.\n"
    "Unlike the variant using a list of parameters, this version allows specification\n"
    "of the parameters with a key/value dictionary. The keys are the parameter names\n"
    "as given by the PCell declaration.\n"
    "\n"
    "The parameters are a sequence of variants which correspond to the parameters declared "
    "by the \\PCellDeclaration object.\n"
    "\n"
    "The name of the new cell will be the name of the PCell. \n"
    "If a cell with that name already exists, a new unique name is generated.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +  
  gsi::method ("add_pcell_variant", &db::Layout::get_pcell_variant, gsi::arg ("pcell_id"), gsi::arg ("parameters"),
    "@brief Creates a PCell variant for the given PCell ID with the given parameters\n"
    "@return The cell index of the pcell variant proxy cell\n"
    "This method will create a PCell variant proxy for a local PCell definition.\n"
    "It will create the PCell variant for the given parameters. Note that this method \n"
    "does not allow one to create PCell instances for PCells located in a library. Use\n"
    "\\add_pcell_variant with the library parameter for that purpose.\n"
    "\n"
    "The parameters are a sequence of variants which correspond to the parameters declared "
    "by the \\PCellDeclaration object.\n"
    "\n"
    "The name of the new cell will be the name of the PCell. \n"
    "If a cell with that name already exists, a new unique name is generated.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +  
  gsi::method_ext ("add_pcell_variant", &add_lib_pcell_variant_dict, gsi::arg ("library"), gsi::arg ("pcell_id"), gsi::arg ("parameters"),
    "@brief Creates a PCell variant for a PCell located in an external library with the parameters given as a name/value dictionary\n"
    "@return The cell index of the new proxy cell in this layout\n"
    "This method will import a PCell from a library and create a variant for the \n"
    "given parameter set.\n"
    "Technically, this method creates a proxy to the library and creates the variant\n"
    "inside that library.\n"
    "Unlike the variant using a list of parameters, this version allows specification\n"
    "of the parameters with a key/value dictionary. The keys are the parameter names\n"
    "as given by the PCell declaration.\n"
    "\n"
    "The parameters are a sequence of variants which correspond to the parameters declared "
    "by the \\PCellDeclaration object.\n"
    "\n"
    "The name of the new cell will be the name of the PCell. \n"
    "If a cell with that name already exists, a new unique name is generated.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +  
  gsi::method_ext ("add_pcell_variant", &add_lib_pcell_variant, gsi::arg ("library"), gsi::arg ("pcell_id"), gsi::arg ("parameters"),
    "@brief Creates a PCell variant for a PCell located in an external library\n"
    "@return The cell index of the new proxy cell in this layout\n"
    "This method will import a PCell from a library and create a variant for the \n"
    "given parameter set.\n"
    "Technically, this method creates a proxy to the library and creates the variant\n"
    "inside that library.  \n"
    "\n"
    "The parameters are a sequence of variants which correspond to the parameters declared "
    "by the \\PCellDeclaration object.\n"
    "\n"
    "The name of the new cell will be the name of the PCell. \n"
    "If a cell with that name already exists, a new unique name is generated.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +  
  gsi::method_ext ("pcell_names", &pcell_names,
    "@brief Gets the names of the PCells registered in the layout\n"
    "Returns an array of PCell names.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
  ) +  
  gsi::method_ext ("pcell_ids", &pcell_ids,
    "@brief Gets the IDs of the PCells registered in the layout\n"
    "Returns an array of PCell IDs.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
  ) +  
  gsi::method_ext ("pcell_id", &pcell_id, gsi::arg ("name"),
    "@brief Gets the ID of the PCell with the given name\n"
    "This method is equivalent to 'pcell_declaration(name).id'.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +  
  gsi::method_ext ("pcell_declaration", &pcell_declaration, gsi::arg ("name"),
    "@brief Gets a reference to the PCell declaration for the PCell with the given name\n"
    "Returns a reference to the local PCell declaration with the given name. If the name\n"
    "is not a valid PCell name, this method returns nil.\n"
    "\n"
    "Usually this method is used on library layouts that define\n"
    "PCells. Note that this method cannot be used on the layouts using the PCell from \n"
    "a library.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +  
  gsi::method ("pcell_declaration", &db::Layout::pcell_declaration, gsi::arg ("pcell_id"),
    "@brief Gets a reference to the PCell declaration for the PCell with the given PCell ID.\n"
    "Returns a reference to the local PCell declaration with the given PCell id. If the parameter\n"
    "is not a valid PCell ID, this method returns nil. The PCell ID is the number returned \n"
    "by \\register_pcell for example.\n"
    "\n"
    "Usually this method is used on library layouts that define\n"
    "PCells. Note that this method cannot be used on the layouts using the PCell from \n"
    "a library.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +  
  gsi::method ("register_pcell", &db::Layout::register_pcell, gsi::arg ("name"), gsi::arg ("declaration"),
    "@brief Registers a PCell declaration under the given name\n"
    "Registers a local PCell in the current layout. If a declaration with that name\n"
    "already exists, it is replaced with the new declaration.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method_ext ("dump_mem_statistics", &dump_mem_statistics, gsi::arg<bool> ("detailed", false),
    "@hide"
  ),
  "@brief The layout object\n"
  "\n"
  "This object represents a layout.\n"
  "The layout object contains the cell hierarchy and\n"
  "adds functionality for managing cell names and layer names.\n"
  "The cell hierarchy can be changed by adding cells and cell instances.\n"
  "Cell instances will virtually put the content of a cell into another cell. "
  "Many cell instances can be put into a cell thus forming repetitions of the "
  "cell content. This process can be repeated over multiple levels. In effect "
  "a cell graphs is created with parent cells and child cells. The graph must not be recursive, so "
  "there is at least one top cell, which does not have a parent cell. Multiple top cells can "
  "be present.\n"
  "\n"
  "\\Layout is the very basic class of the layout database. It has a rich set of methods to "
  "manipulate and query the layout hierarchy, the geometrical objects, the meta information and "
  "other features of the layout database. For a discussion of the basic API and the related "
  "classes see @<a href=\"/programming/database_api.xml\">The Database API@</a>.\n"
  "\n"
  "Usually layout objects have already been created by KLayout's application core. You can address "
  "such a layout via the \\CellView object inside the \\LayoutView class. For example:\n"
  "\n"
  "@code\n"
  "active_layout = RBA::CellView::active.layout\n"
  "puts \"Top cell of current layout is #{active_layout.top_cell.name}\"\n"
  "@/code\n"
  "\n"
  "However, a layout can also be used standalone:\n"
  "\n"
  "@code\n"
  "layout = RBA::Layout::new\n"
  "cell = layout.create_cell(\"TOP\")\n"
  "layer = layout.layer(RBA::LayerInfo::new(1, 0))\n"
  "cell.shapes(layer).insert(RBA::Box::new(0, 0, 1000, 1000))\n"
  "layout.write(\"single_rect.gds\")\n"
  "@/code\n"
);

static db::SaveLayoutOptions *new_v ()
{
  return new db::SaveLayoutOptions ();
}

static bool set_format_from_filename (db::SaveLayoutOptions *opt, const std::string &fn)
{
  if (! opt->set_format_from_filename (fn)) {
    throw tl::Exception (tl::to_string (tr ("Cannot determine format from filename")));
  }
  return true;
}

Class<db::SaveLayoutOptions> decl_SaveLayoutOptions ("db", "SaveLayoutOptions",
  gsi::constructor ("new", &new_v,
    "@brief Default constructor\n"
    "\n"
    "This will initialize the scale factor to 1.0, the database unit is set to\n"
    "\"same as original\" and all layers are selected as well as all cells.\n"
    "The default format is GDS2."
  ) +
  gsi::method_ext ("set_format_from_filename", &set_format_from_filename, gsi::arg ("filename"),
    "@brief Select a format from the given file name\n"
    "\n"
    "This method will set the format according to the file's extension.\n"
    "\n"
    "This method has been introduced in version 0.22. "
    "Beginning with version 0.23, this method always returns true, since the "
    "only consumer for the return value, Layout#write, now ignores that "
    "parameter and automatically determines the compression mode from the file name.\n"
  ) +
  gsi::method ("format=", &db::SaveLayoutOptions::set_format, gsi::arg ("format"),
    "@brief Select a format\n"
    "The format string can be either \"GDS2\", \"OASIS\", \"CIF\" or \"DXF\". Other formats may be available if\n"
    "a suitable plugin is installed."
  ) +
  gsi::method ("format", &db::SaveLayoutOptions::format,
    "@brief Gets the format name\n"
    "\n"
    "See \\format= for a description of that method.\n"
  ) + 
  gsi::method ("add_layer", &db::SaveLayoutOptions::add_layer, gsi::arg ("layer_index"), gsi::arg ("properties"),
    "@brief Add a layer to be saved \n"
    "\n"
    "\n"
    "Adds the layer with the given index to the layer list that will be written.\n"
    "If all layers have been selected previously, all layers will \n"
    "be unselected first and only the new layer remains.\n"
    "\n"
    "The 'properties' argument can be used to assign different layer properties than the ones\n"
    "present in the layout. Pass a default \\LayerInfo object to this argument to use the\n"
    "properties from the layout object. Construct a valid \\LayerInfo object with explicit layer,\n"
    "datatype and possibly a name to override the properties stored in the layout.\n"
  ) + 
  gsi::method ("select_all_layers", &db::SaveLayoutOptions::select_all_layers,
    "@brief Select all layers to be saved\n"
    "\n"
    "This method will clear all layers selected with \\add_layer so far and set the 'select all layers' flag.\n"
    "This is the default.\n"
  ) + 
  gsi::method ("deselect_all_layers", &db::SaveLayoutOptions::deselect_all_layers,
    "@brief Unselect all layers: no layer will be saved\n"
    "\n"
    "This method will clear all layers selected with \\add_layer so far and clear the 'select all layers' flag.\n"
    "Using this method is the only way to save a layout without any layers."
  ) + 
  gsi::method ("select_cell", &db::SaveLayoutOptions::select_cell, gsi::arg ("cell_index"),
    "@brief Selects a cell to be saved (plus hierarchy below)\n"
    "\n"
    "\n"
    "This method is basically a convenience method that combines \\clear_cells and \\add_cell.\n"
    "This method clears the 'select all cells' flag.\n"
    "\n"
    "This method has been added in version 0.22.\n"
  ) + 
  gsi::method ("select_this_cell", &db::SaveLayoutOptions::select_this_cell, gsi::arg ("cell_index"),
    "@brief Selects a cell to be saved\n"
    "\n"
    "\n"
    "This method is basically a convenience method that combines \\clear_cells and \\add_this_cell.\n"
    "This method clears the 'select all cells' flag.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) + 
  gsi::method ("clear_cells", &db::SaveLayoutOptions::clear_cells,
    "@brief Clears all cells to be saved\n"
    "\n"
    "This method can be used to ensure that no cell is selected before \\add_cell is called to specify a cell.\n"
    "This method clears the 'select all cells' flag.\n"
    "\n"
    "This method has been added in version 0.22.\n"
  ) + 
  gsi::method ("add_this_cell", &db::SaveLayoutOptions::add_this_cell, gsi::arg ("cell_index"),
    "@brief Adds a cell to be saved\n"
    "\n"
    "\n"
    "The index of the cell must be a valid index in the context of the layout that will be saved.\n"
    "This method clears the 'select all cells' flag.\n"
    "Unlike \\add_cell, this method does not implicitly add all children of that cell.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) + 
  gsi::method ("add_cell", &db::SaveLayoutOptions::add_cell, gsi::arg ("cell_index"),
    "@brief Add a cell (plus hierarchy) to be saved\n"
    "\n"
    "\n"
    "The index of the cell must be a valid index in the context of the layout that will be saved.\n"
    "This method clears the 'select all cells' flag.\n"
    "\n"
    "This method also implicitly adds the children of that cell. A method that does not add the "
    "children in \\add_this_cell.\n"
  ) + 
  gsi::method ("select_all_cells", &db::SaveLayoutOptions::select_all_cells,
    "@brief Select all cells to save\n"
    "\n"
    "This method will clear all cells specified with \\add_cells so far and set the 'select all cells' flag.\n"
    "This is the default.\n"
  ) + 
  gsi::method ("write_context_info=", &db::SaveLayoutOptions::set_write_context_info, gsi::arg ("flag"),
    "@brief Enables or disables context information\n"
    "\n"
    "If this flag is set to false, no context information for PCell or library cell instances is written. "
    "Those cells will be converted to plain cells and KLayout will not be able to restore the identity of "
    "those cells. Use this option to enforce compatibility with other tools that don't understand the "
    "context information of KLayout.\n"
    "\n"
    "The default value is true (context information is stored). Not all formats support context information, hence "
    "that flag has no effect for formats like CIF or DXF.\n"
    "\n"
    "This method was introduced in version 0.23.\n"
  ) +
  gsi::method ("write_context_info?", &db::SaveLayoutOptions::write_context_info,
    "@brief Gets a flag indicating whether context information will be stored\n"
    "\n"
    "See \\write_context_info= for details about this flag.\n"
    "\n"
    "This method was introduced in version 0.23.\n"
  ) +
  gsi::method ("keep_instances=", &db::SaveLayoutOptions::set_keep_instances, gsi::arg ("flag"),
    "@brief Enables or disables instances for dropped cells\n"
    "\n"
    "If this flag is set to true, instances for cells will be written, even if the cell is dropped. "
    "That may happen, if cells are selected with \\select_this_cell or \\add_this_cell or \\no_empty_cells is used. "
    "Even if cells called by such cells are not selected, instances will be written for that "
    "cell if \"keep_instances\" is true. That feature is supported by the GDS format currently and "
    "results in \"ghost cells\" which have instances but no cell definition.\n"
    "\n"
    "The default value is false (instances of dropped cells are not written).\n"
    "\n"
    "This method was introduced in version 0.23.\n"
  ) +
  gsi::method ("keep_instances?", &db::SaveLayoutOptions::keep_instances,
    "@brief Gets a flag indicating whether instances will be kept even if the target cell is dropped\n"
    "\n"
    "See \\keep_instances= for details about this flag.\n"
    "\n"
    "This method was introduced in version 0.23.\n"
  ) +
  gsi::method ("dbu=", &db::SaveLayoutOptions::set_dbu, gsi::arg ("dbu"),
    "@brief Set the database unit to be used in the stream file\n"
    "\n"
    "By default, the database unit of the layout is used. This method allows one to explicitly use a different\n"
    "database unit. A scale factor is introduced automatically which scales all layout objects accordingly so their physical dimensions remain the same. "
    "When scaling to a larger database unit or one that is not an integer fraction of the original one, rounding errors may occur and the "
    "layout may become slightly distorted."
  ) + 
  gsi::method ("dbu", &db::SaveLayoutOptions::dbu,
    "@brief Get the explicit database unit if one is set\n"
    "\n"
    "See \\dbu= for a description of that attribute.\n"
  ) + 
  gsi::method ("no_empty_cells=", &db::SaveLayoutOptions::set_dont_write_empty_cells, gsi::arg ("flag"),
    "@brief Don't write empty cells if this flag is set\n"
    "\n"
    "By default, all cells are written (no_empty_cells is false).\n"
    "This applies to empty cells which do not contain shapes for the specified layers "
    "as well as cells which are empty because they reference empty cells only.\n"
  ) + 
  gsi::method ("no_empty_cells?", &db::SaveLayoutOptions::dont_write_empty_cells,
    "@brief Returns a flag indicating whether empty cells are not written.\n"
  ) + 
  gsi::method ("scale_factor=", &db::SaveLayoutOptions::set_scale_factor, gsi::arg ("scale_factor"),
    "@brief Set the scaling factor for the saving \n"
    "\n"
    "Using a scaling factor will scale all objects accordingly. "
    "This scale factor adds to a potential scaling implied by using an explicit database unit.\n"
    "\n"
    "Be aware that rounding effects may occur if fractional scaling factors are used.\n"
    "\n"
    "By default, no scaling is applied."
  ) + 
  gsi::method ("scale_factor", &db::SaveLayoutOptions::scale_factor,
    "@brief Gets the scaling factor currently set\n"
  ),
  "@brief Options for saving layouts\n"
  "\n"
  "This class describes the various options for saving a layout to a stream file (GDS2, OASIS and others).\n"
  "There are: layers to be saved, cell or cells to be saved, scale factor, format, database unit\n"
  "and format specific options.\n"
  "\n"
  "Usually the default constructor provides a suitable object. Please note, that the format written is \"GDS2\" by default. Either explicitly set a "
  "format using \\format= or derive the format from the file name using \\set_format_from_filename.\n"
  "\n"
  "The layers are specified by either selecting all layers or by defining layer by layer using the\n"
  "\\add_layer method. \\select_all_layers will explicitly select all layers for saving, \\deselect_all_layers will explicitly clear the list of layers.\n"
  "\n"
  "Cells are selected in a similar fashion: by default, all cells are selected. Using \\add_cell, specific\n"
  "cells can be selected for saving. All these cells plus their hierarchy will then be written to the stream file.\n"
  "\n"
);

}
