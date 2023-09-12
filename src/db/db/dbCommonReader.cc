
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



#include "dbCommonReader.h"
#include "dbStream.h"
#include "tlXMLParser.h"

namespace db
{

// ---------------------------------------------------------------
//  Common reader basic feature implementation

static const size_t null_id = std::numeric_limits<size_t>::max ();

CommonReaderBase::CommonReaderBase ()
  : m_cc_resolution (AddToCell), m_create_layers (false)
{
  //  .. nothing yet ..
}

db::cell_index_type
CommonReaderBase::make_cell (db::Layout &layout, const std::string &cn)
{
  tl_assert (! cn.empty ());

  std::map<std::string, std::pair<size_t, db::cell_index_type> >::iterator iname = m_name_map.find (cn);
  if (iname != m_name_map.end ()) {

    db::Cell &cell = layout.cell (iname->second.second);

    if (! cell.is_ghost_cell ()) {
      common_reader_error (tl::sprintf (tl::to_string (tr ("A cell with name %s already exists")), cn));
    }

    m_temp_cells.erase (cell.cell_index ());
    cell.set_ghost_cell (false);
    return cell.cell_index ();

  } else {

    db::cell_index_type ci = layout.add_anonymous_cell ();

    m_name_map [cn] = std::make_pair (null_id, ci);
    return ci;

  }
}

bool
CommonReaderBase::has_cell (const std::string &cn) const
{
  return m_name_map.find (cn) != m_name_map.end ();
}

std::pair<bool, db::cell_index_type>
CommonReaderBase::cell_by_name (const std::string &cn) const
{
  std::map<std::string, std::pair<size_t, db::cell_index_type> >::const_iterator iname = m_name_map.find (cn);
  if (iname != m_name_map.end ()) {
    return std::make_pair (true, iname->second.second);
  } else {
    return std::make_pair (false, db::cell_index_type (0));
  }
}

db::cell_index_type
CommonReaderBase::make_cell (db::Layout &layout, size_t id)
{
  tl_assert (id != null_id);

  std::map<size_t, std::pair<std::string, db::cell_index_type> >::iterator iid = m_id_map.find (id);
  if (iid != m_id_map.end ()) {

    db::Cell &cell = layout.cell (iid->second.second);

    if (! cell.is_ghost_cell ()) {
      common_reader_error (tl::sprintf (tl::to_string (tr ("A cell with ID %ld already exists")), id));
    }

    m_temp_cells.erase (cell.cell_index ());
    cell.set_ghost_cell (false);
    return cell.cell_index ();

  } else {

    db::cell_index_type ci = layout.add_anonymous_cell ();

    m_id_map [id] = std::make_pair (std::string (), ci);
    return ci;

  }
}

bool
CommonReaderBase::has_cell (size_t id) const
{
  return m_id_map.find (id) != m_id_map.end ();
}

std::pair<bool, db::cell_index_type>
CommonReaderBase::cell_by_id (size_t id) const
{
  std::map<size_t, std::pair<std::string, db::cell_index_type> >::const_iterator iid = m_id_map.find (id);
  if (iid != m_id_map.end ()) {
    return std::make_pair (true, iid->second.second);
  } else {
    return std::make_pair (false, db::cell_index_type (0));
  }
}

const std::string &
CommonReaderBase::name_for_id (size_t id) const
{
  std::map<size_t, std::string>::const_iterator n = m_name_for_id.find (id);
  if (n != m_name_for_id.end ()) {
    return n->second;
  } else {
    static std::string empty;
    return empty;
  }
}

void
CommonReaderBase::rename_cell (db::Layout &layout, size_t id, const std::string &cn)
{
  m_name_for_id.insert (std::make_pair (id, cn));

  std::map<size_t, std::pair<std::string, db::cell_index_type> >::iterator iid = m_id_map.find (id);
  std::map<std::string, std::pair<size_t, db::cell_index_type> >::iterator iname = m_name_map.find (cn);

  if (iid != m_id_map.end () && ! iid->second.first.empty () && iid->second.first != cn) {
    common_reader_error (tl::sprintf (tl::to_string (tr ("Cell named %s with ID %ld was already given name %s")), cn, id, iid->second.first));
  }

  if (iid != m_id_map.end () && iname != m_name_map.end ()) {

    if (iname->second.second != iid->second.second) {

      //  Both cells already exist and are not identical: merge ID-declared cell into the name-declared one
      layout.force_update ();
      merge_cell (layout, iname->second.second, iid->second.second);
      iid->second.second = iname->second.second;

    }

    iid->second.first = cn;
    iname->second.first = id;

  } else if (iid != m_id_map.end ()) {

    m_name_map [cn] = std::make_pair (id, iid->second.second);
    iid->second.first = cn;

  } else if (iname != m_name_map.end ()) {

    m_id_map [id] = std::make_pair (cn, iname->second.second);
    iname->second.first = id;

  } else {

    db::cell_index_type ci = layout.add_anonymous_cell ();
    layout.cell (ci).set_ghost_cell (true);
    m_temp_cells.insert (ci);

    m_id_map [id] = std::make_pair (cn, ci);
    m_name_map [cn] = std::make_pair (id, ci);

  }
}

db::cell_index_type
CommonReaderBase::cell_for_instance (db::Layout &layout, size_t id)
{
  tl_assert (id != null_id);

  std::map<size_t, std::pair<std::string, db::cell_index_type> >::iterator iid = m_id_map.find (id);
  if (iid != m_id_map.end ()) {

    m_temp_cells.erase (iid->second.second);
    return iid->second.second;

  } else {

    db::cell_index_type ci = layout.add_anonymous_cell ();
    layout.cell (ci).set_ghost_cell (true);

    m_id_map [id] = std::make_pair (std::string (), ci);
    return ci;

  }
}

db::cell_index_type
CommonReaderBase::cell_for_instance (db::Layout &layout, const std::string &cn)
{
  tl_assert (! cn.empty ());

  std::map<std::string, std::pair<size_t, db::cell_index_type> >::iterator iname = m_name_map.find (cn);
  if (iname != m_name_map.end ()) {

    m_temp_cells.erase (iname->second.second);
    return iname->second.second;

  } else {

    db::cell_index_type ci = layout.add_anonymous_cell ();
    layout.cell (ci).set_ghost_cell (true);

    m_name_map [cn] = std::make_pair (null_id, ci);
    return ci;

  }
}

void
CommonReaderBase::merge_cell (db::Layout &layout, db::cell_index_type target_cell_index, db::cell_index_type src_cell_index) const
{
  const db::Cell &src_cell = layout.cell (src_cell_index);
  db::Cell &target_cell = layout.cell (target_cell_index);
  target_cell.set_ghost_cell (src_cell.is_ghost_cell () && target_cell.is_ghost_cell ());

  //  copy over the instances
  for (db::Cell::const_iterator i = src_cell.begin (); ! i.at_end (); ++i) {
    //  NOTE: cell indexed may be invalid because we delete subcells without update()
    if (layout.is_valid_cell_index (i->cell_index ())) {
      target_cell.insert (*i);
    }
  }

  merge_cell_without_instances (layout, target_cell_index, src_cell_index);
}

void
CommonReaderBase::merge_cell_without_instances (db::Layout &layout, db::cell_index_type target_cell_index, db::cell_index_type src_cell_index) const
{
  const db::Cell &src_cell = layout.cell (src_cell_index);
  db::Cell &target_cell = layout.cell (target_cell_index);

  //  copy over the shapes
  for (unsigned int l = 0; l < layout.layers (); ++l) {
    if (layout.is_valid_layer (l) && ! src_cell.shapes (l).empty ()) {
      target_cell.shapes (l).insert (src_cell.shapes (l));
    }
  }

  //  replace all instances of the new cell with the original one
  layout.replace_instances_of (src_cell.cell_index (), target_cell.cell_index ());

  //  finally delete the new cell
  layout.delete_cell (src_cell.cell_index ());
}

void
CommonReaderBase::init ()
{
  m_layer_map_out.clear ();
  m_multi_mapping_placeholders.clear ();
  m_layer_cache.clear ();
  m_layers_created.clear ();
  m_layer_names.clear ();
}

void
CommonReaderBase::finish (db::Layout &layout)
{
  bool any_missing = false;

  for (std::map<size_t, std::pair<std::string, db::cell_index_type> >::const_iterator i = m_id_map.begin (); i != m_id_map.end (); ++i) {
    if (i->second.first.empty ()) {
      common_reader_warn (tl::sprintf (tl::to_string (tr ("No cellname defined for cell name id %ld")), i->first));
      any_missing = true;
    }
  }

  if (any_missing) {
    common_reader_error (tl::to_string (tr ("Some cell IDs don't have a name (see previous warnings)")));
  }

  //  check if we need to resolve conflicts

  bool has_conflict = false;
  for (std::map<std::string, std::pair<size_t, db::cell_index_type> >::const_iterator i = m_name_map.begin (); i != m_name_map.end () && ! has_conflict; ++i) {
    has_conflict = layout.cell_by_name (i->first.c_str ()).first;
  }

  if (! has_conflict) {

    //  no conflict - plain rename

    for (std::map<std::string, std::pair<size_t, db::cell_index_type> >::const_iterator i = m_name_map.begin (); i != m_name_map.end (); ++i) {
      layout.rename_cell (i->second.second, i->first.c_str ());
    }

  } else {

    //  elaborate conflict resolution

    layout.force_update ();

    std::map<db::cell_index_type, std::string> new_cells;
    for (std::map<std::string, std::pair<size_t, db::cell_index_type> >::const_iterator i = m_name_map.begin (); i != m_name_map.end (); ++i) {
      new_cells.insert (std::make_pair (i->second.second, i->first));
    }

    std::vector<std::pair<db::cell_index_type, db::cell_index_type> > cells_with_conflict;

    //  First treat all the cells without conflict
    for (db::Layout::bottom_up_iterator bu = layout.begin_bottom_up (); bu != layout.end_bottom_up (); ++bu) {

      db::cell_index_type ci_new = *bu;
      std::map<db::cell_index_type, std::string>::const_iterator i = new_cells.find (ci_new);

      if (i == new_cells.end ()) {
        //  not a new cell
        continue;
      }

      std::pair<bool, db::cell_index_type> c2n = layout.cell_by_name (i->second.c_str ());
      db::cell_index_type ci_org = c2n.second;

      //  NOTE: proxy cells are never resolved. "RenameCell" is a plain and simple case.
      //  Ghost cells are merged rendering the new cell a non-ghost cell.
      if (c2n.first && (m_cc_resolution != RenameCell || layout.cell (ci_org).is_ghost_cell () || layout.cell (ci_new).is_ghost_cell ()) && ! layout.cell (ci_org).is_proxy ()) {
        cells_with_conflict.push_back (std::make_pair (ci_new, ci_org));
      } else {
        layout.rename_cell (ci_new, layout.uniquify_cell_name (i->second.c_str ()).c_str ());
      }

    }

    //  Then treat all the cells with conflict
    for (std::vector<std::pair<db::cell_index_type, db::cell_index_type> >::const_iterator cc = cells_with_conflict.begin (); cc != cells_with_conflict.end (); ++cc) {

      db::cell_index_type ci_new = cc->first;
      db::cell_index_type ci_org = cc->second;

      //  we have a cell conflict

      if (m_cc_resolution == OverwriteCell && ! layout.cell (ci_new).is_ghost_cell ()) {

        if (! layout.cell (ci_org).begin ().at_end ()) {

          //  NOTE: because prune_subcells needs the parents for sub cells and we are going do delete
          //  the current cell, we cannot save the "update()" just by traversing bottom-up.
          layout.force_update ();
          layout.prune_subcells (ci_org);

        }

        layout.cell (ci_org).clear_shapes ();

        merge_cell (layout, ci_org, ci_new);

      } else if (m_cc_resolution == SkipNewCell && ! layout.cell (ci_org).is_ghost_cell ()) {

        layout.prune_subcells (ci_new);
        layout.cell (ci_new).clear_shapes ();

        //  NOTE: ignore instances -> this saves us a layout update
        merge_cell_without_instances (layout, ci_org, ci_new);

      } else {

        merge_cell (layout, ci_org, ci_new);

      }

    }

  }

  //  remove temporary cells (some that were "declared" by "rename_cell" but not used by cell_for_instance)

  for (std::set<db::cell_index_type>::const_iterator ci = m_temp_cells.begin (); ci != m_temp_cells.end (); ++ci) {
    layout.delete_cell (*ci);
  }

  //  resolve layer multi-mapping

  for (std::map<std::set<unsigned int>, unsigned int>::const_iterator i = m_multi_mapping_placeholders.begin (); i != m_multi_mapping_placeholders.end (); ++i) {

    if (i->first.size () > 1) {

      bool discard_layer = i->first.find (i->second) == i->first.end ();

      for (std::set<unsigned int>::const_iterator l = i->first.begin (); l != i->first.end (); ++l) {

        //  last one? this one will get a "move"
        std::set<unsigned int>::const_iterator ll = l;
        if (discard_layer && ++ll == i->first.end ()) {
          layout.move_layer (i->second, *l);
          layout.delete_layer (i->second);
        } else {
          layout.copy_layer (i->second, *l);
        }

      }

    }

  }

  //  rename layers created before if required

  for (std::set<unsigned int>::const_iterator i = m_layers_created.begin (); i != m_layers_created.end (); ++i) {

    const db::LayerProperties &lp = layout.get_properties (*i);

    const tl::interval_map <db::ld_type, std::string> *dtmap = layer_names ().mapped (lp.layer);
    const std::string *name = 0;
    if (dtmap) {
      name = dtmap->mapped (lp.datatype);
    }

    if (name) {
      //  need to rename: add a new madding to m_layer_map_out and adjust the layout's layer properties
      db::LayerProperties lpp = lp;
      join_layer_names (lpp.name, *name);
      layout.set_properties (*i, lpp);
      m_layer_map_out.map (LDPair (lp.layer, lp.datatype), *i, lpp);
    }

  }
}

std::pair <bool, unsigned int>
CommonReaderBase::open_dl (db::Layout &layout, const LDPair &dl)
{
  std::map<db::LDPair, std::pair <bool, unsigned int> >::const_iterator lc = m_layer_cache.find (dl);
  if (lc != m_layer_cache.end ()) {
    return lc->second;
  } else {
    std::pair <bool, unsigned int> res = open_dl_uncached (layout, dl);
    m_layer_cache.insert (std::make_pair (dl, res));
    return res;
  }
}

std::pair <bool, unsigned int>
CommonReaderBase::open_dl_uncached (db::Layout &layout, const LDPair &dl)
{
  std::set<unsigned int> li = m_layer_map.logical (dl, layout);
  if (li.empty ()) {

    if (! m_create_layers) {
      return std::make_pair (false, (unsigned int) 0);
    }

    //  and create the layer
    db::LayerProperties lp;
    lp.layer = dl.layer;
    lp.datatype = dl.datatype;

    //  resolve OASIS name if possible
    const tl::interval_map <db::ld_type, std::string> *names_dmap = m_layer_names.mapped (dl.layer);
    if (names_dmap != 0) {
      const std::string *name = names_dmap->mapped (dl.datatype);
      if (name != 0) {
        lp.name = *name;
      }
    }

    unsigned int nl = layout.insert_layer (lp);
    m_layer_map_out.map (dl, nl, lp);

    m_layers_created.insert (nl);

    return std::make_pair (true, nl);

  } else if (li.size () == 1) {

    m_layer_map_out.map (dl, *li.begin (), layout.get_properties (*li.begin ()));

    return std::make_pair (true, *li.begin ());

  } else {

    for (std::set<unsigned int>::const_iterator i = li.begin (); i != li.end (); ++i) {
      m_layer_map_out.mmap (dl, *i, layout.get_properties (*i));
    }

    std::map<std::set<unsigned int>, unsigned int>::iterator mmp = m_multi_mapping_placeholders.find (li);
    if (mmp == m_multi_mapping_placeholders.end ()) {
      //  create a placeholder layer
      mmp = m_multi_mapping_placeholders.insert (std::make_pair (li, layout.insert_layer ())).first;
    }

    return std::make_pair (true, mmp->second);

  }
}

// ---------------------------------------------------------------
//  Common reader implementation

CommonReader::CommonReader ()
{
  //  .. nothing yet ..
}

const db::LayerMap &
CommonReader::read (db::Layout &layout, const db::LoadLayoutOptions &options)
{
  init (options);

  tl_assert (!layout.under_construction ());

  layer_map ().prepare (layout);

  layout.start_changes ();
  try {
    do_read (layout);
    finish (layout);
    layout.end_changes ();
  } catch (...) {
    layout.end_changes ();
    throw;
  }

  //  A cleanup may be necessary because of the following scenario: if library proxies contain subcells
  //  which are proxies itself, the proxy update may make them orphans (the proxies are regenerated).
  //  The cleanup will removed these.
  layout.cleanup ();

  return layer_map_out ();
}

const db::LayerMap &
CommonReader::read (db::Layout &layout)
{
  return read (layout, db::LoadLayoutOptions ());
}

void
CommonReader::init (const LoadLayoutOptions &options)
{
  ReaderBase::init (options);
  CommonReaderBase::init ();

  db::CommonReaderOptions common_options = options.get_options<db::CommonReaderOptions> ();
  set_conflict_resolution_mode (common_options.cell_conflict_resolution);
  set_create_layers (common_options.create_other_layers);
  set_layer_map (common_options.layer_map);
}

// ---------------------------------------------------------------
//  Common format declaration

/**
 *  @brief A declaration for the common reader options
 *  This is a dummy declaration that provides common specifications for both GDS and OASIS readers.
 */
class CommonFormatDeclaration
  : public db::StreamFormatDeclaration
{
public:
  CommonFormatDeclaration ()
  {
    //  .. nothing yet ..
  }

  virtual std::string format_name () const { return "Common"; }
  virtual std::string format_desc () const { return "GDS2+OASIS"; }
  virtual std::string format_title () const { return "Common GDS2+OASIS"; }
  virtual std::string file_format () const { return std::string (); }

  virtual bool detect (tl::InputStream & /*s*/) const
  {
    return false;
  }

  virtual ReaderBase *create_reader (tl::InputStream & /*s*/) const
  {
    return 0;
  }

  virtual WriterBase *create_writer () const
  {
    return 0;
  }

  virtual bool can_read () const
  {
    return false;
  }

  virtual bool can_write () const
  {
    return false;
  }

  virtual tl::XMLElementBase *xml_reader_options_element () const
  {
    return new db::ReaderOptionsXMLElement<db::CommonReaderOptions> ("common",
      tl::make_member (&db::CommonReaderOptions::create_other_layers, "create-other-layers") +
      tl::make_member (&db::CommonReaderOptions::layer_map, "layer-map") +
      tl::make_member (&db::CommonReaderOptions::enable_properties, "enable-properties") +
      tl::make_member (&db::CommonReaderOptions::enable_text_objects, "enable-text-objects")
    );
  }
};

static tl::RegisteredClass<db::StreamFormatDeclaration> reader_decl (new CommonFormatDeclaration (), 20, "Common");

}

