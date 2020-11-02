
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
//  Common reader implementation

static const size_t null_id = std::numeric_limits<size_t>::max ();

CommonReader::CommonReader ()
  : m_cc_resolution (AddToCell)
{
  //  .. nothing yet ..
}

db::cell_index_type
CommonReader::make_cell (db::Layout &layout, const std::string &cn)
{
  tl_assert (! cn.empty ());

  std::map<std::string, std::pair<size_t, db::cell_index_type> >::iterator iname = m_name_map.find (cn);
  if (iname != m_name_map.end ()) {

    db::Cell &cell = layout.cell (iname->second.second);

    if (! cell.is_ghost_cell ()) {
      common_reader_error (tl::sprintf (tl::to_string (tr ("A cell with name %s already exists")), cn));
    }

    cell.set_ghost_cell (false);
    return cell.cell_index ();

  } else {

    db::cell_index_type ci = layout.add_cell ();

    m_name_map [cn] = std::make_pair (null_id, ci);
    return ci;

  }
}

bool
CommonReader::has_cell (const std::string &cn) const
{
  return m_name_map.find (cn) != m_name_map.end ();
}

std::pair<bool, db::cell_index_type>
CommonReader::cell_by_name (const std::string &cn) const
{
  std::map<std::string, std::pair<size_t, db::cell_index_type> >::const_iterator iname = m_name_map.find (cn);
  if (iname != m_name_map.end ()) {
    return std::make_pair (true, iname->second.second);
  } else {
    return std::make_pair (false, size_t (0));
  }
}

db::cell_index_type
CommonReader::make_cell (db::Layout &layout, size_t id)
{
  tl_assert (id != null_id);

  std::map<size_t, std::pair<std::string, db::cell_index_type> >::iterator iid = m_id_map.find (id);
  if (iid != m_id_map.end ()) {

    db::Cell &cell = layout.cell (iid->second.second);

    if (! cell.is_ghost_cell ()) {
      common_reader_error (tl::sprintf (tl::to_string (tr ("A cell with ID %ld already exists")), id));
    }

    cell.set_ghost_cell (false);
    return cell.cell_index ();

  } else {

    db::cell_index_type ci = layout.add_cell ();

    m_id_map [id] = std::make_pair (std::string (), ci);
    return ci;

  }
}

bool
CommonReader::has_cell (size_t id) const
{
  return m_id_map.find (id) != m_id_map.end ();
}

std::pair<bool, db::cell_index_type>
CommonReader::cell_by_id (size_t id) const
{
  std::map<size_t, std::pair<std::string, db::cell_index_type> >::const_iterator iid = m_id_map.find (id);
  if (iid != m_id_map.end ()) {
    return std::make_pair (true, iid->second.second);
  } else {
    return std::make_pair (false, size_t (0));
  }
}

void
CommonReader::rename_cell (db::Layout &layout, size_t id, const std::string &cn)
{
  std::map<size_t, std::pair<std::string, db::cell_index_type> >::iterator iid = m_id_map.find (id);
  std::map<std::string, std::pair<size_t, db::cell_index_type> >::iterator iname = m_name_map.find (cn);

  if (iid != m_id_map.end () && iname != m_name_map.end ()) {

    if (iid->second.first != cn) {
      common_reader_error (tl::sprintf (tl::to_string (tr ("Cell named %s with ID %ld was already given name %s")), cn, id, iid->second.first));
    }

    if (iname->second.second != iid->second.second) {

      //  Both cells already exist and are not identical: merge ID-declared cell into the name-declared one
      layout.force_update ();
      merge_cell (layout, iname->second.second, iid->second.second);
      iid->second.second = iname->second.second;

    }

  } else if (iid != m_id_map.end ()) {

    m_name_map [cn] = std::make_pair (id, iid->second.second);

  } else if (iname != m_name_map.end ()) {

    m_id_map [id] = std::make_pair (cn, iname->second.second);

  } else {

    db::cell_index_type ci = layout.add_cell ();

    m_id_map [id] = std::make_pair (std::string (), ci);
    m_name_map [cn] = std::make_pair (null_id, ci);

  }
}

db::cell_index_type
CommonReader::cell_for_instance (db::Layout &layout, size_t id)
{
  tl_assert (id != null_id);

  std::map<size_t, std::pair<std::string, db::cell_index_type> >::iterator iid = m_id_map.find (id);
  if (iid != m_id_map.end ()) {

    return iid->second.second;

  } else {

    db::cell_index_type ci = layout.add_cell ();
    layout.cell (ci).set_ghost_cell (true);

    m_id_map [id] = std::make_pair (std::string (), ci);
    return ci;

  }
}

db::cell_index_type
CommonReader::cell_for_instance (db::Layout &layout, const std::string &cn)
{
  tl_assert (! cn.empty ());

  std::map<std::string, std::pair<size_t, db::cell_index_type> >::iterator iname = m_name_map.find (cn);
  if (iname != m_name_map.end ()) {

    return iname->second.second;

  } else {

    db::cell_index_type ci = layout.add_cell ();
    layout.cell (ci).set_ghost_cell (true);

    m_name_map [cn] = std::make_pair (null_id, ci);
    return ci;

  }
}

void
CommonReader::merge_cell (db::Layout &layout, db::cell_index_type target_cell_index, db::cell_index_type src_cell_index) const
{
  const db::Cell &src_cell = layout.cell (src_cell_index);
  db::Cell &target_cell = layout.cell (target_cell_index);

  //  copy over the instances
  for (db::Cell::const_iterator i = src_cell.begin (); ! i.at_end (); ++i) {
    target_cell.insert (*i);
  }

  //  copy over the shapes
  for (unsigned int l = 0; l < layout.layers (); ++l) {
    if (layout.is_valid_layer (l) && ! src_cell.shapes (l).empty ()) {
      target_cell.shapes (l).insert (src_cell.shapes (l));
    }
  }

  //  replace all instances of the new cell with the original one
  std::vector<std::pair<db::cell_index_type, db::Instance> > parents;
  for (db::Cell::parent_inst_iterator pi = src_cell.begin_parent_insts (); ! pi.at_end (); ++pi) {
    parents.push_back (std::make_pair (pi->parent_cell_index (), pi->child_inst ()));
  }

  for (std::vector<std::pair<db::cell_index_type, db::Instance> >::const_iterator p = parents.begin (); p != parents.end (); ++p) {
    db::CellInstArray ia = p->second.cell_inst ();
    ia.object ().cell_index (target_cell.cell_index ());
    layout.cell (p->first).replace (p->second, ia);
  }

  //  finally delete the new cell
  layout.delete_cell (src_cell.cell_index ());
}

void
CommonReader::finish (db::Layout &layout)
{
  bool any_missing = false;

  for (std::map<size_t, std::pair<std::string, db::cell_index_type> >::const_iterator i = m_id_map.begin (); i != m_id_map.end (); ++i) {
    if (i->second.first.empty ()) {
      common_reader_warn (tl::sprintf (tl::to_string (tr ("Cell name missing for ID %ld")), i->first));
      any_missing = true;
    }
  }

  if (any_missing) {
    common_reader_error (tl::to_string (tr ("Some cell IDs don't have a name (see previous warnings)")));
  }

  for (std::map<std::string, std::pair<size_t, db::cell_index_type> >::const_iterator i = m_name_map.begin (); i != m_name_map.end (); ++i) {

    if (layout.has_cell (i->first.c_str ())) {

      db::cell_index_type ci_org = layout.cell_by_name (i->first.c_str ()).second;
      db::cell_index_type ci_new = i->second.second;

      if (m_cc_resolution == RenameCell || layout.cell (ci_org).is_proxy ()) {

        //  NOTE: we never reopen proxies (they are always local to their layout). Instead we
        //  always rename for proxies
        layout.rename_cell (i->second.second, i->first.c_str ());

      } else {

        //  we have a cell conflict
        layout.force_update ();

        if (m_cc_resolution == OverwriteCell && ! layout.cell (ci_new).is_ghost_cell ()) {

          layout.prune_subcells (ci_org);
          layout.cell (ci_org).clear_shapes ();

        } else if (m_cc_resolution == SkipNewCell && ! layout.cell (ci_org).is_ghost_cell ()) {

          layout.prune_subcells (ci_new);
          layout.cell (ci_new).clear_shapes ();

        }

        merge_cell (layout, ci_org, ci_new);

      }

    } else {

      layout.rename_cell (i->second.second, i->first.c_str ());

    }

  }
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

