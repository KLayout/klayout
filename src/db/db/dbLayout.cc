
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


#include "dbLayout.h"
#include "dbMemStatistics.h"
#include "dbTrans.h"
#include "dbTechnology.h"
#include "dbShapeRepository.h"
#include "dbPCellHeader.h"
#include "dbPCellVariant.h"
#include "dbPCellDeclaration.h"
#include "dbLibraryProxy.h"
#include "dbColdProxy.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbCellVariants.h"
#include "dbRegion.h"
#include "dbEdgePairs.h"
#include "dbEdges.h"
#include "dbTexts.h"
#include "dbCellMapping.h"
#include "dbLayerMapping.h"
#include "dbLayoutUtils.h"
#include "dbCellVariants.h"
#include "tlTimer.h"
#include "tlLog.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlAssert.h"


namespace db
{

static const int layout_base_verbosity = 30;

// -----------------------------------------------------------------
//  The undo/redo operations

struct LayoutOp
  : public db::Op
{
  LayoutOp () { }
  virtual ~LayoutOp () { }

  virtual void redo (db::Layout *) const = 0;
  virtual void undo (db::Layout *) const = 0;
};

struct SetLayoutPropId
  : public LayoutOp
{
  SetLayoutPropId (db::properties_id_type f, db::properties_id_type t)
    : m_from (f), m_to (t)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->prop_id (m_to);
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->prop_id (m_from);
  }

private:
  db::properties_id_type m_from, m_to;
};

struct SetLayoutTechName
  : public LayoutOp
{
  SetLayoutTechName (const std::string &from, const std::string &to)
    : m_from (from), m_to (to)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->set_technology_name_without_update (m_to);
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->set_technology_name_without_update (m_from);
  }

private:
  std::string m_from, m_to;
};

struct SetLayoutDBU
  : public LayoutOp
{
  SetLayoutDBU (double f, double t)
    : m_from (f), m_to (t)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->dbu (m_to);
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->dbu (m_from);
  }

private:
  double m_from, m_to;
};

struct RenameCellOp
  : public LayoutOp
{
  RenameCellOp (db::cell_index_type i, const std::string &f, const std::string &t)
    : m_cell_index (i), m_from (f), m_to (t)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->rename_cell (m_cell_index, m_to.c_str ());
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->rename_cell (m_cell_index, m_from.c_str ());
  }

private:
  db::cell_index_type m_cell_index;
  std::string m_from, m_to;
};

struct NewRemoveCellOp
  : public LayoutOp
{
  NewRemoveCellOp (db::cell_index_type i, const std::string &name, bool remove, db::Cell *cell)
    : m_cell_index (i), m_name (name), m_remove (remove), mp_cell (cell)
  { }

  ~NewRemoveCellOp ()
  {
    if (mp_cell) {
      delete mp_cell;
    }
  }

  virtual void redo (db::Layout *layout) const
  {
    if (m_remove) {
      remove_cell (layout);
    } else {
      new_cell (layout);
    }
  }

  virtual void undo (db::Layout *layout) const
  {
    if (m_remove) {
      new_cell (layout);
    } else {
      remove_cell (layout);
    }
  }

private:
  db::cell_index_type m_cell_index;
  std::string m_name;
  bool m_remove;
  mutable db::Cell *mp_cell;

  virtual void new_cell (db::Layout *layout) const
  {
    tl_assert (mp_cell != 0);
    layout->insert_cell (m_cell_index, m_name, mp_cell);
    mp_cell = 0; // now it belongs to the layout
  }

  virtual void remove_cell (db::Layout *layout) const
  {
    tl_assert (mp_cell == 0);
    mp_cell = layout->take_cell (m_cell_index);
  }
};

struct SetLayerPropertiesOp
  : public LayoutOp
{
  SetLayerPropertiesOp (unsigned int l, const db::LayerProperties &new_props, const db::LayerProperties &old_props)
    : m_layer_index (l), m_new_props (new_props), m_old_props (old_props)
  { }

  virtual void redo (db::Layout *layout) const
  {
    layout->set_properties (m_layer_index, m_new_props);
  }

  virtual void undo (db::Layout *layout) const
  {
    layout->set_properties (m_layer_index, m_old_props);
  }

private:
  unsigned int m_layer_index;
  db::LayerProperties m_new_props, m_old_props;
};

struct InsertRemoveLayerOp
  : public LayoutOp
{
  InsertRemoveLayerOp (unsigned int l, const db::LayerProperties &props, bool insert)
    : m_layer_index (l), m_props (props), m_insert (insert)
  { }

  virtual void redo (db::Layout *layout) const
  {
    if (m_insert) {
      layout->insert_layer (m_layer_index, m_props);
    } else {
      layout->delete_layer (m_layer_index);
    }
  }

  virtual void undo (db::Layout *layout) const
  {
    if (! m_insert) {
      layout->insert_layer (m_layer_index, m_props);
    } else {
      layout->delete_layer (m_layer_index);
    }
  }

private:
  unsigned int m_layer_index;
  db::LayerProperties m_props;
  bool m_insert;
};

// -----------------------------------------------------------------
//  Implementation of the ProxyContextInfo class

LayoutOrCellContextInfo
LayoutOrCellContextInfo::deserialize (std::vector<std::string>::const_iterator from, std::vector<std::string>::const_iterator to)
{
  LayoutOrCellContextInfo info;

  for (auto i = from; i != to; ++i) {

    tl::Extractor ex (i->c_str ());

    if (ex.test ("LIB=")) {

      info.lib_name = ex.skip ();

    } else if (ex.test ("P(")) {

      std::pair<std::string, tl::Variant> vv;

      ex.read_word_or_quoted (vv.first);
      ex.test (")");
      ex.test ("=");
      ex.read (vv.second);

      info.pcell_parameters.insert (vv);

    } else if (ex.test ("PCELL=")) {

      info.pcell_name = ex.skip ();

    } else if (ex.test ("CELL=")) {

      info.cell_name = ex.skip ();

    } else if (ex.test ("META(")) {

      std::pair<std::string, std::pair<tl::Variant, std::string> > vv;

      ex.read_word_or_quoted (vv.first);
      if (ex.test (",")) {
        ex.read_word_or_quoted (vv.second.second);
      }
      ex.test (")");
      ex.test ("=");
      ex.read (vv.second.first);

      info.meta_info.insert(vv);

    }

  }

  return info;
}

void
LayoutOrCellContextInfo::serialize (std::vector<std::string> &strings)
{
  if (! lib_name.empty ()) {
    strings.push_back ("LIB=" + lib_name);
  }
  for (auto p = pcell_parameters.begin (); p != pcell_parameters.end (); ++p) {
    strings.push_back ("P(" + tl::to_word_or_quoted_string (p->first) + ")=" + p->second.to_parsable_string ());
  }
  if (! pcell_name.empty ()) {
    strings.push_back ("PCELL=" + pcell_name);
  }
  if (! cell_name.empty ()) {
    strings.push_back ("CELL=" + cell_name);
  }

  std::string mv;
  for (auto m = meta_info.begin (); m != meta_info.end (); ++m) {
    mv.clear ();
    mv += "META(";
    mv += tl::to_word_or_quoted_string (m->first);
    if (! m->second.second.empty ()) {
      mv += ",";
      mv += tl::to_word_or_quoted_string (m->second.second);
    }
    mv += ")=";
    mv += m->second.first.to_parsable_string ();
    strings.push_back (mv);
  }
}

bool
LayoutOrCellContextInfo::has_proxy_info () const
{
  return !pcell_name.empty () || !lib_name.empty ();
}

bool
LayoutOrCellContextInfo::has_meta_info () const
{
  return !meta_info.empty ();
}

// -----------------------------------------------------------------
//  Implementation of the Layout class

Layout::Layout (db::Manager *manager)
  : db::Object (manager),
    mp_library (0),
    m_cells_size (0),
    m_invalid (0),
    m_top_cells (0),
    m_dbu (0.001),
    m_prop_id (0),
    m_properties_repository (this),
    m_do_cleanup (false),
    m_editable (db::default_editable_mode ())
{
  // .. nothing yet ..
}

Layout::Layout (bool editable, db::Manager *manager)
  : db::Object (manager),
    mp_library (0),
    m_cells_size (0),
    m_invalid (0),
    m_top_cells (0),
    m_dbu (0.001),
    m_prop_id (0),
    m_properties_repository (this),
    m_do_cleanup (false),
    m_editable (editable)
{
  // .. nothing yet ..
}

Layout::Layout (const db::Layout &layout)
  : db::Object (layout),
    db::LayoutStateModel (),
    gsi::ObjectBase (),
    tl::Object (),
    tl::UniqueId (),
    mp_library (0),
    m_cells_size (0),
    m_invalid (0),
    m_top_cells (0),
    m_dbu (0.001),
    m_prop_id (0),
    m_properties_repository (this),
    m_do_cleanup (false),
    m_editable (layout.m_editable)
{
  *this = layout;
}

Layout::~Layout ()
{
  //  since it the cell graph (or the derived layout) might produce some transactions that refer to 
  //  this object, we need to clear the manager's transaction list before the cell graph is deleted.
  if (manager ()) {
    manager ()->clear ();
  }

  clear ();
}

void
Layout::dbu (double d)
{
  if (fabs (d - m_dbu)) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SetLayoutDBU (m_dbu, d));
    }
    m_dbu = d;
    dbu_changed ();
  }
}

void
Layout::clear ()
{
  invalidate_hier ();

  m_free_cell_indices.clear ();
  m_cells.clear ();
  m_cells_size = 0;
  m_cell_ptrs.clear ();

  m_top_down_list.clear ();

  m_layers.clear ();

  for (std::vector<const char *>::const_iterator p = m_cell_names.begin (); p != m_cell_names.end (); ++p) {
    if (*p) {
      delete [] *p;
    }
  }
  m_cell_names.clear ();
  m_cell_map.clear ();

  m_shape_repository = db::GenericRepository ();
  db::PropertiesRepository empty_pr (this);
  m_properties_repository = empty_pr;
  m_array_repository = db::ArrayRepository ();

  for (std::vector<pcell_header_type *>::const_iterator pc = m_pcells.begin (); pc != m_pcells.end (); ++pc) {
    delete *pc;
  }
  m_pcells.clear ();
  m_pcell_ids.clear ();

  m_lib_proxy_map.clear ();
  m_meta_info.clear ();
}

Layout &
Layout::operator= (const Layout &d)
{
  if (&d != this) {

    db::LayoutStateModel::operator= (d);

    clear ();

    m_layers = d.m_layers;

    m_editable = d.m_editable;

    m_pcell_ids = d.m_pcell_ids;
    m_pcells.reserve (d.m_pcells.size ());

    for (std::vector<pcell_header_type *>::const_iterator pc = d.m_pcells.begin (); pc != d.m_pcells.end (); ++pc) {
      if (*pc) {
        m_pcells.push_back (new pcell_header_type (**pc));
      } else {
        m_pcells.push_back (0);
      }
    }

    m_lib_proxy_map = d.m_lib_proxy_map;

    m_cell_ptrs.resize (d.m_cell_ptrs.size (), 0);

    for (const_iterator c = d.begin (); c != d.end (); ++c) {
      cell_type *new_cell = (*c).clone (*this);
      m_cells.push_back_ptr (new_cell);
      ++m_cells_size;
      m_cell_ptrs [new_cell->cell_index ()] = new_cell;
    }

    m_properties_repository = d.m_properties_repository; // because the cell assign operator does not map property ID's ..
    m_top_down_list = d.m_top_down_list;
    m_top_cells = d.m_top_cells;

    m_cell_names.reserve (d.m_cell_names.size ());

    cell_index_type i = 0;
    for (std::vector<const char *>::const_iterator p = d.m_cell_names.begin (); p != d.m_cell_names.end (); ++p) {
      if (*p) {
        char *pp = new char [strlen (*p) + 1];
        strcpy (pp, *p);
        m_cell_names.push_back (pp);
        m_cell_map.insert (std::make_pair (pp, i));
      } else {
        m_cell_names.push_back (0);
      }
      ++i;
    }

    m_dbu = d.m_dbu;

    m_meta_info = d.m_meta_info;
    m_meta_info_by_cell = d.m_meta_info_by_cell;
    m_meta_info_names = d.m_meta_info_names;
    m_meta_info_name_map = d.m_meta_info_name_map;

    m_tech_name = d.m_tech_name;

    m_prop_id = d.m_prop_id;

  }
  return *this;
}

const db::Technology *
Layout::technology () const
{
  return db::Technologies::instance ()->has_technology (m_tech_name) ? db::Technologies::instance ()->technology_by_name (m_tech_name) : 0;
}

void
Layout::set_technology_name_without_update (const std::string &tech)
{
  if (tech != m_tech_name) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SetLayoutTechName (m_tech_name, tech));
    }
    m_tech_name = tech;
    technology_changed_event ();
  }
}

void
Layout::set_technology_name (const std::string &tech)
{
  if (tech == m_tech_name) {
    return;
  }

  //  determine which library to map to what
  std::map<db::lib_id_type, db::lib_id_type> mapping;
  std::set<db::lib_id_type> seen;
  std::set<db::lib_id_type> lost;

  for (db::Layout::iterator c = begin (); c != end (); ++c) {

    db::LibraryProxy *lib_proxy = dynamic_cast<db::LibraryProxy *> (&*c);
    if (lib_proxy && seen.find (lib_proxy->lib_id ()) == seen.end ()) {

      seen.insert (lib_proxy->lib_id ());

      std::pair<bool, db::lib_id_type> new_id (false, 0);
      const db::Library *l = db::LibraryManager::instance ().lib (lib_proxy->lib_id ());
      if (l) {
        new_id = db::LibraryManager::instance ().lib_by_name (l->get_name (), tech);
      }

      if (new_id.first && new_id.second != l->get_id ()) {
        mapping.insert (std::make_pair (l->get_id (), new_id.second));
      } else if (! new_id.first) {
        lost.insert (lib_proxy->lib_id ());
      }

    }

  }

  if (! mapping.empty () || ! lost.empty ()) {

    bool needs_cleanup = false;

    std::vector<std::pair<db::LibraryProxy *, db::PCellVariant *> > pcells_to_map;
    std::vector<db::LibraryProxy *> lib_cells_to_map;
    std::vector<db::LibraryProxy *> lib_cells_lost;

    for (db::Layout::iterator c = begin (); c != end (); ++c) {

      std::map<db::lib_id_type, db::lib_id_type>::const_iterator m;

      db::LibraryProxy *lib_proxy = dynamic_cast<db::LibraryProxy *> (&*c);
      if (! lib_proxy) {
        continue;
      }

      if ((m = mapping.find (lib_proxy->lib_id ())) != mapping.end ()) {

        db::Library *lib = db::LibraryManager::instance ().lib (lib_proxy->lib_id ());
        db::Cell *lib_cell = &lib->layout ().cell (lib_proxy->library_cell_index ());
        db::PCellVariant *lib_pcell = dynamic_cast <db::PCellVariant *> (lib_cell);
        if (lib_pcell) {
          pcells_to_map.push_back (std::make_pair (lib_proxy, lib_pcell));
        } else {
          lib_cells_to_map.push_back (lib_proxy);
        }

        needs_cleanup = true;

      } else if (lost.find (lib_proxy->lib_id ()) != lost.end ()) {

        lib_cells_lost.push_back (lib_proxy);

        needs_cleanup = true;

      }

    }

    //  We do PCell resolution before the library proxy resolution. The reason is that
    //  PCells may generate library proxies in their instantiation. Hence we must instantiate
    //  the PCells before we can resolve them.
    for (std::vector<std::pair<db::LibraryProxy *, db::PCellVariant *> >::const_iterator lp = pcells_to_map.begin (); lp != pcells_to_map.end (); ++lp) {

      db::cell_index_type ci = lp->first->Cell::cell_index ();
      db::PCellVariant *lib_pcell = lp->second;

      std::pair<bool, pcell_id_type> pn = lib_pcell->layout ()->pcell_by_name (lp->first->get_basic_name ().c_str ());

      if (! pn.first) {

        //  substitute by a cold proxy
        db::LayoutOrCellContextInfo info;
        get_context_info (ci, info);
        create_cold_proxy_as (info, ci);

      } else {

        db::Library *new_lib = db::LibraryManager::instance ().lib (mapping [lp->first->lib_id ()]);

        const db::PCellDeclaration *old_pcell_decl = lib_pcell->layout ()->pcell_declaration (lib_pcell->pcell_id ());
        const db::PCellDeclaration *new_pcell_decl = new_lib->layout ().pcell_declaration (pn.second);
        if (! old_pcell_decl || ! new_pcell_decl) {

          //  substitute by a cold proxy
          db::LayoutOrCellContextInfo info;
          get_context_info (ci, info);
          create_cold_proxy_as (info, ci);

        } else {

          //  map pcell parameters by name
          std::map<std::string, tl::Variant> param_by_name = lib_pcell->parameters_by_name ();
          lp->first->remap (new_lib->get_id (), new_lib->layout ().get_pcell_variant (pn.second, new_pcell_decl->map_parameters (param_by_name)));

        }

      }

    }

    for (std::vector<db::LibraryProxy *>::const_iterator lp = lib_cells_to_map.begin (); lp != lib_cells_to_map.end (); ++lp) {

      db::Library *new_lib = db::LibraryManager::instance ().lib (mapping [(*lp)->lib_id ()]);

      db::cell_index_type ci = (*lp)->Cell::cell_index ();

      std::pair<bool, cell_index_type> cn = new_lib->layout ().cell_by_name ((*lp)->get_basic_name ().c_str ());

      if (! cn.first) {

        //  unlink this proxy: substitute by a cold proxy
        db::LayoutOrCellContextInfo info;
        get_context_info (ci, info);
        create_cold_proxy_as (info, ci);

      } else {

        (*lp)->remap (new_lib->get_id (), cn.second);

      }

    }

    for (std::vector<db::LibraryProxy *>::const_iterator lp = lib_cells_lost.begin (); lp != lib_cells_lost.end (); ++lp) {

      db::cell_index_type ci = (*lp)->Cell::cell_index ();

      //  substitute by a cold proxy
      db::LayoutOrCellContextInfo info;
      get_context_info (ci, info);
      create_cold_proxy_as (info, ci);

    }

    if (needs_cleanup) {
      cleanup ();
    }

  }

  set_technology_name_without_update (tech);

  //  we may have re-established a connection for pending ("cold") proxies so we can try to restore them
  restore_proxies ();
}

void
Layout::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (!no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  m_layers.mem_stat (stat, purpose, cat, true, (void *) this);

  db::mem_stat (stat, purpose, cat, m_cell_ptrs, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_free_cell_indices, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_top_down_list, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_cell_names, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_cell_map, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_pcells, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_pcell_ids, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_lib_proxy_map, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_meta_info, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_string_repository, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_shape_repository, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_properties_repository, true, (void *) this);
  db::mem_stat (stat, purpose, cat, m_array_repository, true, (void *) this);

  for (std::vector<const char *>::const_iterator i = m_cell_names.begin (); i != m_cell_names.end (); ++i) {
    stat->add (typeid (char []), (void *) *i, *i ? (strlen (*i) + 1) : 0, *i ? (strlen (*i) + 1) : 0, (void *) this, purpose, cat);
  }
  for (cell_list::const_iterator i = m_cells.begin (); i != m_cells.end (); ++i) {
    db::mem_stat (stat, MemStatistics::CellInfo, int (i->id ()), *i, false, (void *) this);
  }
  for (std::vector<pcell_header_type *>::const_iterator i = m_pcells.begin (); i != m_pcells.end (); ++i) {
    db::mem_stat (stat, MemStatistics::CellInfo, 0, **i, false, (void *) this);
  }
}

void
Layout::prop_id (db::properties_id_type id) 
{
  if (m_prop_id != id) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SetLayoutPropId (m_prop_id, id));
    }
    m_prop_id = id;
  }
}

bool 
Layout::has_cell (const char *name)
{
  return m_cell_map.find (name) != m_cell_map.end ();
}

std::pair<bool, cell_index_type> 
Layout::cell_by_name (const char *name) const
{
  cell_map_type::const_iterator c = m_cell_map.find (name);
  if (c != m_cell_map.end ()) {
    return std::make_pair (true, c->second);
  } else {
    return std::make_pair (false, 0);
  }
}

const char *
Layout::cell_name (cell_index_type index) const
{
  tl_assert (index < m_cell_names.size ());
  return m_cell_names [index];
}

void 
Layout::delete_cells (const std::set<cell_index_type> &cells_to_delete)
{
  //  Collect parent cells
  std::set <cell_index_type> pcs;
  for (std::set<cell_index_type>::const_iterator c = cells_to_delete.begin (); c != cells_to_delete.end (); ++c) {
    const db::Cell &cref = cell (*c);
    for (db::Cell::parent_cell_iterator pc = cref.begin_parent_cells (); pc != cref.end_parent_cells (); ++pc) {
      pcs.insert (*pc);
    }
  }

  db::LayoutLocker locker (this);

  //  Clear all instances
  for (std::set<cell_index_type>::const_iterator c = cells_to_delete.begin (); c != cells_to_delete.end (); ++c) {

    db::Cell &cref = cell (*c);

    cref.clear_insts ();

    //  If transacting, do not use clear_shapes here. This will delete all the shapes containers will
    //  will disable us saving undo data with reference to them.
    if (manager () && manager ()->transacting ()) {
      for (unsigned int i = 0; i < layers (); ++i) {
        if (is_valid_layer (i)) {
          cref.clear (i);
        }
      }
    } else {
      cref.clear_shapes ();
    }

  }

  //  delete all instances of this cell

  std::vector <db::Instance> insts_to_delete;
  for (std::set <cell_index_type>::const_iterator pc = pcs.begin (); pc != pcs.end (); ++pc) {

    db::Cell &parent_cref = cell (*pc);

    insts_to_delete.clear ();
    for (db::Cell::const_iterator pci = parent_cref.begin (); ! pci.at_end (); ++pci) {
      if (cells_to_delete.find (pci->cell_index ()) != cells_to_delete.end ()) {
        insts_to_delete.push_back (*pci);
      }
    }

    std::sort (insts_to_delete.begin (), insts_to_delete.end ());

    parent_cref.erase_insts (insts_to_delete);

  }

  //  erase the cells themselves
  //  If transacting, the cell is not deleted yet. Instead, the transaction object acts as
  //  a backup container for the cell. This is necessary since the ID's within manager are given to
  //  cell child objects that must remain.
  for (std::set<cell_index_type>::const_iterator c = cells_to_delete.begin (); c != cells_to_delete.end (); ++c) {

    if (manager () && manager ()->transacting ()) {
       
      //  note the "take" method - this takes out the cell
      std::string cn (cell_name (*c));
      manager ()->queue (this, new NewRemoveCellOp (*c, cn, true /*remove*/, take_cell (*c)));

    } else {

      //  remove the cell - we use take_cell and delete to avoid recursion issues
      delete take_cell (*c);

    }

  }
}

void 
Layout::delete_cell (cell_index_type id)
{
  db::Cell &cref = cell (id);

  std::vector <cell_index_type> pcs;
  for (db::Cell::parent_cell_iterator pc = cref.begin_parent_cells (); pc != cref.end_parent_cells (); ++pc) {
    pcs.push_back (*pc);
  }

  cref.clear_insts ();

  //  If transacting, do not use clear_shapes here. This will delete all the shapes containers will
  //  will disable us saving undo data with reference to them.
  if (manager () && manager ()->transacting ()) {
    for (unsigned int i = 0; i < layers (); ++i) {
      if (is_valid_layer (i)) {
        cref.clear (i);
      }
    }
  } else {
    cref.clear_shapes ();
  }

  //  delete all instances of this cell

  std::vector <db::Instance> insts_to_delete;
  for (std::vector <cell_index_type>::const_iterator pc = pcs.begin (); pc != pcs.end (); ++pc) {

    if (is_valid_cell_index (*pc)) {

      db::Cell &parent_cref = cell (*pc);

      insts_to_delete.clear ();
      for (db::Cell::const_iterator pci = parent_cref.begin (); ! pci.at_end (); ++pci) {
        if (pci->cell_index () == id) {
          insts_to_delete.push_back (*pci);
        }
      }

      std::sort (insts_to_delete.begin (), insts_to_delete.end ());

      parent_cref.erase_insts (insts_to_delete);

    }

  }

  //  erase the cell itself
  //  If transacting, the cell is not deleted yet. Instead, the transaction object acts as
  //  a backup container for the cell. This is necessary since the ID's within manager are given to
  //  cell child objects that must remain.

  if (manager () && manager ()->transacting ()) {
     
    //  not the "take" method - this takes out the cell
    std::string cn (cell_name (id));
    manager ()->queue (this, new NewRemoveCellOp (id, cn, true /*remove*/, take_cell (id)));

  } else {

    //  remove the cell - we use take_cell and delete to avoid recursion issues
    delete take_cell (id);

  }
}

void
Layout::insert (db::cell_index_type cell, int layer, const db::Region &region)
{
  region.insert_into (this, cell, layer);
}

void
Layout::insert (db::cell_index_type cell, int layer, const db::Edges &edges)
{
  edges.insert_into (this, cell, layer);
}

void
Layout::insert (db::cell_index_type cell, int layer, const db::EdgePairs &edge_pairs)
{
  edge_pairs.insert_into (this, cell, layer);
}

void
Layout::insert (db::cell_index_type cell, int layer, const db::Texts &texts)
{
  texts.insert_into (this, cell, layer);
}

void
Layout::flatten (const db::Cell &source_cell, db::Cell &target_cell, const db::ICplxTrans &t, int levels)
{
  db::ICplxTrans tt = t;

  if (&source_cell != &target_cell) {

    unsigned int nlayers = layers ();
    for (unsigned int l = 0; l < nlayers; ++l) {

      if (is_valid_layer (l)) {

        db::Shapes &target_shapes = target_cell.shapes (l);
        const db::Shapes &source_shapes = source_cell.shapes (l);

        tl::ident_map<db::Layout::properties_id_type> pm1;
        for (db::Shapes::shape_iterator sh = source_shapes.begin (db::ShapeIterator::All); ! sh.at_end (); ++sh) {
          target_shapes.insert (*sh, tt, pm1);
        }

      }

    }

  }

  if (levels == 0) {

    if (&source_cell != &target_cell) {
      for (db::Cell::const_iterator inst = source_cell.begin (); ! inst.at_end (); ++inst) {
        db::Instance new_inst = target_cell.insert (*inst);
        target_cell.transform (new_inst, tt);
      }
    }

  } else if (&target_cell == &source_cell) {

    update ();

    try {

      //  Note: suppressing the update speeds up the flatten process considerably since 
      //  even an iteration of the instances requires an update.
      start_changes ();

      db::Instances old_instances (&target_cell);
      old_instances = target_cell.instances ();
      target_cell.clear_insts ();

      for (db::Cell::const_iterator inst = old_instances.begin (); ! inst.at_end (); ++inst) {

        db::CellInstArray cell_inst = inst->cell_inst ();

        for (db::CellInstArray::iterator a = cell_inst.begin (); ! a.at_end (); ++a) {
          db::ICplxTrans tinst = t * cell_inst.complex_trans (*a);
          flatten (cell (cell_inst.object ().cell_index ()), target_cell, tinst, levels < 0 ? levels : levels - 1);
        }

      }

      end_changes ();

    } catch (...) {
      end_changes ();
      throw;
    }

  } else {
    
    try {

      //  Note: suppressing the update speeds up the flatten process considerably since 
      //  even an iteration of the instances requires an update.
      start_changes ();

      for (db::Cell::const_iterator inst = source_cell.begin (); ! inst.at_end (); ++inst) {

        db::CellInstArray cell_inst = inst->cell_inst ();

        for (db::CellInstArray::iterator a = cell_inst.begin (); ! a.at_end (); ++a) {
          db::ICplxTrans tinst = t * cell_inst.complex_trans (*a);
          flatten (cell (cell_inst.object ().cell_index ()), target_cell, tinst, levels < 0 ? levels : levels - 1);
        }

      }

      end_changes ();

    } catch (...) {
      end_changes ();
      throw;
    }

  }

}

void 
Layout::flatten (db::Cell &cell_to_flatten, int levels, bool prune) 
{
  std::set<db::cell_index_type> direct_children;
  if (prune) {
    //  save direct children
    cell_to_flatten.collect_called_cells (direct_children, 1);
  }

  flatten (cell_to_flatten, cell_to_flatten, db::ICplxTrans (), levels);

  if (prune) {

    //  determine all direct children that are orphans now.
    for (std::set<db::cell_index_type>::iterator dc = direct_children.begin (); dc != direct_children.end (); ) {
      std::set<db::cell_index_type>::iterator dc_next = dc;
      ++dc_next;
      if (cell (*dc).parent_cells () != 0) {
        direct_children.erase (dc);
      }
      dc = dc_next;
    }

    //  and prune them
    prune_cells (direct_children.begin (), direct_children.end (), levels - 1);

  }
}

void 
Layout::prune_cell (cell_index_type id, int levels)
{
  do_prune_cell_or_subcell (id, levels, false);
}

void 
Layout::prune_subcells (cell_index_type id, int levels)
{
  do_prune_cell_or_subcell (id, levels, true);
}

void 
Layout::do_prune_cell_or_subcell (cell_index_type id, int levels, bool subcells)
{
  db::Cell &cref = cell (id);

  //  collect the called cells
  std::set <cell_index_type> called;
  cref.collect_called_cells (called, levels);
  if (! subcells) {
    called.insert (id);
  }

  //  From these cells erase all cells that have parents outside the subtree of our cell.
  //  Make sure this is done recursively by doing this top-down.
  for (top_down_iterator c = begin_top_down (); c != end_top_down (); ++c) {
    if (*c != id && called.find (*c) != called.end ()) {
      db::Cell &ccref = cell (*c);
      for (db::Cell::parent_cell_iterator pc = ccref.begin_parent_cells (); pc != ccref.end_parent_cells (); ++pc) {
        if (*pc != id && called.find (*pc) == called.end ()) {
          //  we have a parent outside the subset considered currently (either the cell was never in or
          //  it was removed itself already): remove this cell from the set of valid subcells.
          called.erase (*c);
          break;
        }
      }
    }
  }

  //  and delete the cells
  delete_cells (called);

  //  erase all instances in the subcells case (because, by definition we don't have any more instances)
  if (subcells) {
    cref.clear_insts ();
  }
}

void 
Layout::prune_cells (const std::set<cell_index_type> &ids, int levels)
{
  do_prune_cells_or_subcells (ids, levels, false);
}

void 
Layout::prune_subcells (const std::set<cell_index_type> &ids, int levels)
{
  do_prune_cells_or_subcells (ids, levels, true);
}

void 
Layout::do_prune_cells_or_subcells (const std::set<cell_index_type> &ids, int levels, bool subcells)
{
  //  collect the called cells
  std::set <cell_index_type> called;
  for (std::set<cell_index_type>::const_iterator id = ids.begin (); id != ids.end (); ++id) {
    db::Cell &cref = cell (*id);
    cref.collect_called_cells (called, levels);
  }
  called.insert (ids.begin (), ids.end ());

  //  From these cells erase all cells that have parents outside the subtree of our cell.
  //  Make sure this is done recursively by doing this top-down.
  for (top_down_iterator c = begin_top_down (); c != end_top_down (); ++c) {
    if (called.find (*c) != called.end () && ids.find (*c) == ids.end ()) {
      db::Cell &ccref = cell (*c);
      for (db::Cell::parent_cell_iterator pc = ccref.begin_parent_cells (); pc != ccref.end_parent_cells (); ++pc) {
        if (called.find (*pc) == called.end ()) {
          //  we have a parent outside the subset considered currently (either the cell was never in or
          //  it was removed itself already): remove this cell from the set of valid subcells.
          called.erase (*c);
          break;
        }
      }
    }
  }

  //  order the called cells bottom-up 
  std::vector <cell_index_type> cells_to_delete;
  cells_to_delete.reserve (called.size ());
  for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
    if (called.find (*c) != called.end () && (!subcells || ids.find (*c) == ids.end ())) {
      cells_to_delete.push_back (*c);
    }
  }

  //  and delete these cells
  delete_cells (cells_to_delete.begin (), cells_to_delete.end ());

  //  erase all instances in the subcells case (because, by definition we don't have any more instances)
  if (subcells) {
    for (std::set<cell_index_type>::const_iterator id = ids.begin (); id != ids.end (); ++id) {
      db::Cell &cref = cell (*id);
      cref.clear_insts ();
    }
  }
}

void 
Layout::delete_cell_rec (cell_index_type id)
{
  db::Cell &cref = cell (id);

  //  collect the called cells
  std::set <cell_index_type> called;
  cref.collect_called_cells (called);
  called.insert (id);

  //  order the called cells bottom-up
  std::vector <cell_index_type> cells_to_delete;
  cells_to_delete.reserve (called.size ());
  for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
    if (called.find (*c) != called.end ()) {
      cells_to_delete.push_back (*c);
    }
  }

  //  and delete these cells
  delete_cells (cells_to_delete.begin (), cells_to_delete.end ());
}

void
Layout::insert_cell (cell_index_type ci, const std::string &name, db::Cell *cell)
{
  //  this method is supposed to restore a cell deleted before
  tl_assert (m_cell_names.size () > ci);
  tl_assert (m_cell_names [ci] == 0);

  char *cp = new char [name.size () + 1];
  m_cell_names [ci] = cp;
  strcpy (cp, name.c_str ());

  invalidate_hier ();

  m_cells.push_back_ptr (cell);
  m_cell_ptrs [ci] = cell;
  m_cell_map.insert (std::make_pair (cp, ci));

  cell->reregister ();
  ++m_cells_size;
}

db::Cell *
Layout::take_cell (cell_index_type ci)
{
  tl_assert (m_cell_ptrs [ci] != 0);

  invalidate_hier ();

  cell_type *cell = m_cells.take (iterator (m_cell_ptrs [ci]));
  cell->unregister ();
  --m_cells_size;

  m_cell_ptrs [ci] = 0;

  auto mi = m_meta_info_by_cell.find (ci);
  if (mi != m_meta_info_by_cell.end ()) {
    m_meta_info_by_cell.erase (mi);
  }

  //  Using free cell indices does have one significant drawback:
  //  The cellview references cannot be uniquely classified as being invalid - because the
  //  ID might be reused. This causes problems, when a cell is being deleted and subsequently a
  //  cell is being created - a crash occurs. Therefore the free index feature is disabled.
  //  If this causes memory consumption problems, it should be considered to use a map and
  //  an arbitrary ID.
  // m_free_cell_indices.push_back (ci);

  if (m_cell_names [ci] != 0) {

    cell_map_type::iterator cm = m_cell_map.find (m_cell_names [ci]);
    if (cm != m_cell_map.end ()) {
      m_cell_map.erase (cm);
    }

    delete [] m_cell_names [ci];
    m_cell_names [ci] = 0;

  }

  return cell;
}

std::string 
Layout::uniquify_cell_name (const char *name) const
{
  if (name != 0 && m_cell_map.find (name) == m_cell_map.end ()) {
    return std::string (name);
  } else {

    std::string b;

    //  if the cell does not have a valid name yet, create a unique one.
    unsigned int j = 0;
    for (unsigned int m = 0x40000000; m > 0; m >>= 1) {
      j += m;
      b = std::string (name ? name : "") + "$" + tl::to_string (j);
      if (m_cell_map.find (b.c_str ()) == m_cell_map.end ()) {
        j -= m;
      }
    } 

    b = std::string (name ? name : "") + "$" + tl::to_string (j + 1);
    return b;

  }
}

cell_index_type
Layout::add_cell (const db::Layout &other, db::cell_index_type ci)
{
  cell_index_type ci_new = add_cell (other.cell_name (ci));
  cell (ci_new).set_ghost_cell (other.cell (ci).is_ghost_cell ());

  if (&other == this) {
    add_meta_info (ci_new, other.begin_meta (ci), other.end_meta (ci));
  } else {
    for (auto m = other.begin_meta (ci); m != other.end_meta (ci); ++m) {
      add_meta_info (ci_new, meta_info_name_id (other.meta_info_name (m->first)), m->second);
    }
  }

  return ci_new;
}

cell_index_type
Layout::add_cell (const char *name)
{
  std::string b;

  if (name == 0) {

    //  0 name means: create a new one
    b = uniquify_cell_name (0);
    name = b.c_str ();

  } else {

    cell_map_type::const_iterator cm = m_cell_map.find (name);
    if (cm != m_cell_map.end ()) {

      const db::Cell &c= cell (cm->second);
      if (c.is_ghost_cell () && c.empty ()) {
        //  ghost cells are available as new cells - the idea is to 
        //  treat them as non-existing.
        return cm->second;
      } else {
        //  create a unique name
        b = uniquify_cell_name (name);
        name = b.c_str ();
      }

    }

  }

  //  create a new cell 
  cell_index_type new_index = allocate_new_cell ();

  cell_type *new_cell = new cell_type (new_index, *this);
  m_cells.push_back_ptr (new_cell);
  m_cell_ptrs [new_index] = new_cell;

  //  enter its index and cell_name
  register_cell_name (name, new_index);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
  }

  return new_index;
}

cell_index_type
Layout::add_anonymous_cell ()
{
  std::string b;

  //  create a new cell
  cell_index_type new_index = allocate_new_cell ();

  cell_type *new_cell = new cell_type (new_index, *this);
  m_cells.push_back_ptr (new_cell);
  m_cell_ptrs [new_index] = new_cell;

  //  enter its index and cell_name
  register_cell_name (0, new_index);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
  }

  return new_index;
}

void 
Layout::register_cell_name (const char *name, cell_index_type ci)
{
  //  enter its index and cell_name
  char *cp;

  if (name == 0) {
    cp = new char [1];
    *cp = 0;
  } else {
    cp = new char [strlen (name) + 1];
    strcpy (cp, name);
  }

  while (m_cell_names.size () < ci) {
    char *e = new char [1];
    *e = 0;
    m_cell_names.push_back (e);
  }

  if (m_cell_names.size () > ci) {
    delete [] m_cell_names [ci];
    m_cell_names [ci] = cp;
  } else {
    m_cell_names.push_back (cp);
  }

  if (name) {
    m_cell_map.insert (std::make_pair (cp, ci));
  }
}

void
Layout::rename_cell (cell_index_type id, const char *name)
{
  tl_assert (id < m_cell_names.size ());

  if (strcmp (m_cell_names [id], name) != 0) {

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new RenameCellOp (id, m_cell_names [id], name));
    }

    m_cell_map.erase (m_cell_names [id]);

    char *cp = new char [strlen (name) + 1];
    strcpy (cp, name);

    delete [] m_cell_names [id];
    m_cell_names [id] = cp;

    m_cell_map.insert (std::make_pair (cp, id));

    //  to enforce a redraw and a rebuild
    cell_name_changed ();

  }
}

bool 
Layout::topological_sort ()
{
  m_top_cells = 0;
  m_top_down_list.clear ();

  //  NOTE: we explicitly count the cells here and do not rely on "m_cell_size".
  //  Reason is that this is somewhat safer, specifically directly after take() when
  //  the cell list is already reduced, but the cell pointers are still containing the cell
  //  (issue #905)
  size_t ncells = 0;
  for (const_iterator c = begin (); c != end (); ++c) {
    ++ncells;
  }
  m_top_down_list.reserve (ncells);

  std::vector<size_t> num_parents (m_cell_ptrs.size (), 0);

  //  while there are cells to treat ..
  while (m_top_down_list.size () != ncells) {

    size_t n_top_down_cells = m_top_down_list.size ();

    //  Treat all cells that do not have all parents reported.
    //  For all such a cells, disable the parent counting, 
    //  add the cell's index to the top-down sorted list and
    //  increment the reported parent instance count in all the
    //  child cells.

    for (const_iterator c = begin (); c != end (); ++c) {
      if (c->parent_cells () == num_parents [c->cell_index ()]) {
        m_top_down_list.push_back (c->cell_index ());
        num_parents [c->cell_index ()] = std::numeric_limits<cell_index_type>::max ();
      }
    }

    //  For all these a cells, increment the reported parent instance 
    //  count in all the child cells.
    for (cell_index_vector::const_iterator ii = m_top_down_list.begin () + n_top_down_cells; ii != m_top_down_list.end (); ++ii) {
      for (cell_type::child_cell_iterator cc = cell (*ii).begin_child_cells (); ! cc.at_end (); ++cc) {
        tl_assert (num_parents [*cc] != std::numeric_limits<cell_index_type>::max ());
        num_parents [*cc] += 1;
      }
    }

    //  If no new cells have been reported this is basically a 
    //  sign of recursion in the graph.
    if (n_top_down_cells == m_top_down_list.size ()) {
      return false;
    }
 
  }

  //  Determine the number of top cells
  for (top_down_iterator e = m_top_down_list.begin (); e != m_top_down_list.end () && cell (*e).is_top (); ++e) {
    ++m_top_cells;
  }

  //  The cell graph is fine.
  return true;

}

void
Layout::copy_tree_shapes (const db::Layout &source_layout, const db::CellMapping &cm)
{
  if (this == &source_layout) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same layout")));
  }

  db::ICplxTrans trans (source_layout.dbu () / dbu ());

  db::LayerMapping lm;
  lm.create_full (*this, source_layout);

  db::copy_shapes (*this, source_layout, trans, cm.source_cells (), cm.table (), lm.table ());
}

void
Layout::copy_tree_shapes (const db::Layout &source_layout, const db::CellMapping &cm, const db::LayerMapping &lm)
{
  if (this == &source_layout) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same layout")));
  }

  db::ICplxTrans trans (source_layout.dbu () / dbu ());

  db::copy_shapes (*this, source_layout, trans, cm.source_cells (), cm.table (), lm.table ());
}

void
Layout::move_tree_shapes (db::Layout &source_layout, const db::CellMapping &cm)
{
  if (this == &source_layout) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same layout")));
  }

  db::ICplxTrans trans (source_layout.dbu () / dbu ());

  db::LayerMapping lm;
  lm.create_full (*this, source_layout);

  db::move_shapes (*this, source_layout, trans, cm.source_cells (), cm.table (), lm.table ());
}

void
Layout::move_tree_shapes (db::Layout &source_layout, const db::CellMapping &cm, const db::LayerMapping &lm)
{
  if (this == &source_layout) {
    throw tl::Exception (tl::to_string (tr ("Cannot copy shapes within the same layout")));
  }

  db::ICplxTrans trans (source_layout.dbu () / dbu ());

  db::move_shapes (*this, source_layout, trans, cm.source_cells (), cm.table (), lm.table ());
}

bool 
Layout::is_valid_cell_index (cell_index_type ci) const
{
  return ci < m_cell_ptrs.size () && m_cell_ptrs [ci] != 0;
}

cell_index_type
Layout::allocate_new_cell ()
{
  invalidate_hier ();

  cell_index_type new_index;
  if (m_free_cell_indices.empty ()) {
    new_index = cell_index_type (m_cell_ptrs.size ());
    m_cell_ptrs.push_back (0);
  } else {
    new_index = m_free_cell_indices.back ();
    m_free_cell_indices.pop_back ();
  }

  ++m_cells_size;

  return new_index;
}

void
Layout::refresh ()
{
  for (iterator c = begin (); c != end (); ++c) {
    c->update ();
  }
}

void
Layout::cleanup (const std::set<db::cell_index_type> &keep)
{
  //  only managed layouts will receive cleanup requests. Never library container layouts - these
  //  cannot know if their proxies are not referenced by other proxies.
  if (! m_do_cleanup) {
    return;
  }

  //  deleting cells may create new top cells which need to be deleted as well, hence we iterate
  //  until there are no more cells to delete
  while (true) {

    //  delete all cells that are top cells and are proxies. Those cells are proxies no longer required.
    std::set<cell_index_type> cells_to_delete;
    for (top_down_iterator c = begin_top_down (); c != end_top_cells (); ++c) {
      if (cell (*c).is_proxy ()) {
        cells_to_delete.insert (*c);
      }
    }

    for (std::set<db::cell_index_type>::const_iterator k = keep.begin (); k != keep.end (); ++k) {
      cells_to_delete.erase (*k);
    }

    if (cells_to_delete.empty ()) {
      break;
    }

    delete_cells (cells_to_delete);

  }
}

void 
Layout::update_relations ()
{
  for (iterator c = begin (); c != end (); ++c) {
    c->sort_child_insts ();
  }

  std::vector <size_t> parent_insts (cells (), 0);
  for (const_iterator c = begin (); c != end (); ++c) {
    c->count_parent_insts (parent_insts);
  }
  std::vector <size_t>::const_iterator n = parent_insts.begin ();
  for (iterator c = begin (); c != end (); ++c, ++n) {
    c->clear_parent_insts (*n);
  }
  for (iterator c = begin (); c != end (); ++c) {
    c->update_relations ();
  }
}

Layout::top_down_iterator 
Layout::end_top_cells () 
{
  update ();
  return m_top_down_list.begin () + m_top_cells;
}

Layout::top_down_const_iterator 
Layout::end_top_cells () const
{
  update ();
  return m_top_down_list.begin () + m_top_cells;
}

void 
Layout::force_update () 
{
  if (hier_dirty () || bboxes_dirty ()) {

    unsigned int invalid = m_invalid;

    try {

      m_invalid = std::numeric_limits<unsigned int>::max ();   //  prevent recursion

      db::LayoutStateModel *state_model = const_cast<db::LayoutStateModel *> ((const db::LayoutStateModel *) this);
      state_model->update ();

      m_invalid = invalid;

    } catch (...) {
      m_invalid = invalid;
      throw;
    }

  }
}

void 
Layout::update () const
{
  if (! under_construction () && (hier_dirty () || bboxes_dirty ())) {

    try {

      m_invalid = std::numeric_limits<unsigned int>::max ();   //  prevent recursion

      db::LayoutStateModel *state_model = const_cast<db::LayoutStateModel *> ((const db::LayoutStateModel *) this);
      state_model->update ();

      m_invalid = 0;

    } catch (...) {
      m_invalid = 0;
      throw;
    }

  }
}

void 
Layout::do_update ()
{
  tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity, tl::to_string (tr ("Sorting")));

  //  establish a progress report since this operation can take some time.
  //  HINT: because of some gcc bug, automatic destruction of the tl::Progress
  //  object does not work. We overcome this problem by creating the object with new 
  //  and catching exceptions.
  //  As this operation is critical we don't want to have it cancelled. Plus: do_update is called during ~LayoutLocker and
  //  if we throw exceptions then, we'll get a runtime assertion.
  tl::RelativeProgress *pr = new tl::RelativeProgress (tl::to_string (tr ("Sorting layout")), m_cells_size, 0, false /*can't cancel*/);
  pr->set_desc ("");

  try {

    //  if the hierarchy has been changed so far, update
    //  the hierarchy management information
    if (hier_dirty ()) {
      {
        tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Updating relations");
        pr->set_desc (tl::to_string (tr ("Updating relations")));
        update_relations ();
      } 
      {
        tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Topological sort");
        pr->set_desc (tl::to_string (tr ("Topological sorting")));
        tl_assert (topological_sort ());
      }
    }

    //  KLUDGE: a boolean vector (with size as determined by number of cells)
    //  would probably be much faster!
    std::set<cell_index_type> dirty_parents;

    //  if something on the bboxes (either on shape level or on 
    //  cell bbox level - i.e. by child instances) has been changed,
    //  update the bbox information. In addition sort the shapes
    //  lists of region queries, since they might have changed once
    //  the bboxes are dirty.
    if (bboxes_dirty ()) {

      {
        tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Updating bounding boxes");
        unsigned int layers = 0;
        pr->set (0);
        pr->set_desc (tl::to_string (tr ("Updating bounding boxes")));
        for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
          ++*pr;
          cell_type &cp (cell (*c));
          if (cp.is_shape_bbox_dirty () || dirty_parents.find (*c) != dirty_parents.end ()) {
            if (cp.update_bbox (layers)) {
              //  the bounding box has changed - need to insert parents into "dirty parents" list
              for (cell_type::parent_cell_iterator p = cp.begin_parent_cells (); p != cp.end_parent_cells (); ++p) {
                dirty_parents.insert (*p);
              }
            } 
          }
          if (cp.layers () > layers) {
            layers = cp.layers ();
          }
        }
      }

      {
        tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Sorting shapes");
        pr->set (0);
        pr->set_desc (tl::to_string (tr ("Sorting shapes")));
        for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
          ++*pr;
          cell_type &cp (cell (*c));
          cp.sort_shapes ();
        }
      }

    }

    //  sort the instance trees now, since we have computed the bboxes
    if (hier_dirty () || ! dirty_parents.empty ()) {
      tl::SelfTimer timer (tl::verbosity () > layout_base_verbosity + 10, "Sorting instances");
      size_t layers = 0;
      pr->set (0);
      pr->set_desc (tl::to_string (tr ("Sorting instances")));
      for (bottom_up_iterator c = begin_bottom_up (); c != end_bottom_up (); ++c) {
        ++*pr;
        cell_type &cp (cell (*c));
        bool force_sort_inst_tree = dirty_parents.find (*c) != dirty_parents.end ();
        if (hier_dirty () || force_sort_inst_tree) {
          cp.sort_inst_tree (force_sort_inst_tree);
        }
        if (cp.layers () > layers) {
          layers = cp.layers ();
        }
      }
    }

  } catch (...) {
    delete pr;
    throw;
  }

  delete pr;
}

static Layout::meta_info_map s_empty_meta;

Layout::meta_info_iterator
Layout::begin_meta (db::cell_index_type ci) const
{
  auto m = m_meta_info_by_cell.find (ci);
  if (m != m_meta_info_by_cell.end ()) {
    return m->second.begin ();
  } else {
    return s_empty_meta.begin ();
  }
}

Layout::meta_info_iterator
Layout::end_meta (db::cell_index_type ci) const
{
  auto m = m_meta_info_by_cell.find (ci);
  if (m != m_meta_info_by_cell.end ()) {
    return m->second.end ();
  } else {
    return s_empty_meta.end ();
  }
}

const std::string &
Layout::meta_info_name (Layout::meta_info_name_id_type name_id) const
{
  static std::string empty;
  return name_id < m_meta_info_names.size () ? m_meta_info_names[name_id] : empty;
}

Layout::meta_info_name_id_type
Layout::meta_info_name_id (const std::string &name)
{
  auto n = m_meta_info_name_map.find (name);
  if (n != m_meta_info_name_map.end ()) {
    return n->second;
  } else {
    size_t id = m_meta_info_names.size ();
    m_meta_info_names.push_back (name);
    m_meta_info_name_map.insert (std::make_pair (name, id));
    return id;
  }
}

Layout::meta_info_name_id_type
Layout::meta_info_name_id (const std::string &name) const
{
  auto n = m_meta_info_name_map.find (name);
  return n != m_meta_info_name_map.end () ? n->second : std::numeric_limits<meta_info_name_id_type>::max ();
}

void
Layout::clear_meta ()
{
  m_meta_info.clear ();
}

void
Layout::add_meta_info (meta_info_name_id_type name_id, const MetaInfo &i)
{
  m_meta_info[name_id] = i;
}

void
Layout::remove_meta_info (meta_info_name_id_type name_id)
{
  m_meta_info.erase (name_id);
}

const MetaInfo &
Layout::meta_info (meta_info_name_id_type name_id) const
{
  auto n = m_meta_info.find (name_id);
  static MetaInfo null_value;
  return n != m_meta_info.end () ? n->second : null_value;
}

bool
Layout::has_meta_info (meta_info_name_id_type name_id) const
{
  return m_meta_info.find (name_id) != m_meta_info.end ();
}

void
Layout::clear_meta (db::cell_index_type ci)
{
  m_meta_info_by_cell.erase (ci);
}

void
Layout::add_meta_info (db::cell_index_type ci, meta_info_name_id_type name_id, const MetaInfo &i)
{
  m_meta_info_by_cell[ci][name_id] = i;
}

void
Layout::remove_meta_info (db::cell_index_type ci, meta_info_name_id_type name_id)
{
  auto c = m_meta_info_by_cell.find (ci);
  if (c != m_meta_info_by_cell.end ()) {
    c->second.erase (name_id);
  }
}

const MetaInfo &
Layout::meta_info (db::cell_index_type ci, meta_info_name_id_type name_id) const
{
  auto c = m_meta_info_by_cell.find (ci);
  if (c != m_meta_info_by_cell.end ()) {
    auto i = c->second.find (name_id);
    if (i != c->second.end ()) {
      return i->second;
    }
  }

  static MetaInfo null_value;
  return null_value;
}

bool
Layout::has_meta_info (db::cell_index_type ci, meta_info_name_id_type name_id) const
{
  auto c = m_meta_info_by_cell.find (ci);
  if (c != m_meta_info_by_cell.end ()) {
    return c->second.find (name_id) != c->second.end ();
  } else {
    return false;
  }
}

void
Layout::swap_layers (unsigned int a, unsigned int b)
{
  tl_assert (m_layers.layer_state (a) != LayoutLayers::Free);
  tl_assert (m_layers.layer_state (b) != LayoutLayers::Free);

  //  clear the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->swap (a, b);
  }
}

void 
Layout::move_layer (unsigned int src, unsigned int dest)
{
  tl_assert (m_layers.layer_state (src) != LayoutLayers::Free);
  tl_assert (m_layers.layer_state (dest) != LayoutLayers::Free);

  //  move the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->move (src, dest);
  }
}

void
Layout::move_layer (unsigned int src, unsigned int dest, unsigned int flags)
{
  tl_assert (m_layers.layer_state (src) != LayoutLayers::Free);
  tl_assert (m_layers.layer_state (dest) != LayoutLayers::Free);

  //  move the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->move (src, dest, flags);
  }
}

void
Layout::copy_layer (unsigned int src, unsigned int dest)
{
  tl_assert (m_layers.layer_state (src) != LayoutLayers::Free);
  tl_assert (m_layers.layer_state (dest) != LayoutLayers::Free);

  //  copy the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->copy (src, dest);
  }
}

void
Layout::copy_layer (unsigned int src, unsigned int dest, unsigned int flags)
{
  tl_assert (m_layers.layer_state (src) != LayoutLayers::Free);
  tl_assert (m_layers.layer_state (dest) != LayoutLayers::Free);

  //  copy the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->copy (src, dest, flags);
  }
}

void
Layout::clear_layer (unsigned int n)
{
  tl_assert (m_layers.layer_state (n) != LayoutLayers::Free);

  //  clear the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->clear (n);
  }
}

void
Layout::clear_layer (unsigned int n, unsigned int flags)
{
  tl_assert (m_layers.layer_state (n) != LayoutLayers::Free);

  //  clear the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->clear (n, flags);
  }
}

void
Layout::delete_layer (unsigned int n)
{
  tl_assert (m_layers.layer_state (n) != LayoutLayers::Free);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (n, m_layers.get_properties (n), false /*delete*/));
  }

  m_layers.delete_layer (n);

  //  clear the shapes
  for (iterator c = begin (); c != end (); ++c) {
    c->clear (n);
  }

  layer_properties_changed ();
}

unsigned int
Layout::get_layer (const db::LayerProperties &props)
{
  int li = get_layer_maybe (props);
  if (li >= 0) {
    return (unsigned int) li;
  }

  if (props.is_null ()) {
    //  for a null layer info always create a layer
    return insert_layer ();
  } else {
    return insert_layer (props);
  }
}

unsigned int 
Layout::insert_layer (const LayerProperties &props)
{
  unsigned int i = m_layers.insert_layer (props);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (i, props, true/*insert*/));
  }

  layer_properties_changed ();

  return i;
}

void 
Layout::insert_layer (unsigned int index, const LayerProperties &props)
{
  m_layers.insert_layer (index, props);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (index, props, true/*insert*/));
  }

  layer_properties_changed ();
}

unsigned int 
Layout::insert_special_layer (const LayerProperties &props)
{
  unsigned int i = m_layers.insert_special_layer (props);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (i, props, true/*insert*/));
  }

  return i;
}

void 
Layout::set_properties (unsigned int i, const LayerProperties &props)
{
  if (m_layers.get_properties (i) != props) {

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new SetLayerPropertiesOp (i, props, m_layers.get_properties (i)));
    }

    m_layers.set_properties (i, props);

    layer_properties_changed ();

  }
}

void 
Layout::insert_special_layer (unsigned int index, const LayerProperties &props)
{
  m_layers.insert_special_layer (index, props);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new InsertRemoveLayerOp (index, props, true/*insert*/));
  }
}

static const std::vector<tl::Variant> &gauge_parameters (const std::vector<tl::Variant> &p, const db::PCellDeclaration *pcell_decl, std::vector<tl::Variant> &buffer)
{
  const std::vector<db::PCellParameterDeclaration> &pcp = pcell_decl->parameter_declarations ();

  if (pcp.size () > p.size ()) {

    buffer.clear ();
    buffer.resize (pcp.size ());
    buffer = p;
    for (std::vector<PCellParameterDeclaration>::const_iterator i = pcp.begin () + p.size (); i != pcp.end (); ++i) {
      buffer.push_back (i->get_default ());
    }
    return buffer;

  } else if (pcp.size () < p.size ()) {

    buffer.clear ();
    buffer.insert (buffer.end (), p.begin (), p.begin () + pcp.size ());
    return buffer;

  } else {
    return p;
  }
}

void
Layout::replace_cell (cell_index_type target_cell_index, db::Cell *new_cell, bool retain_layout)
{
  invalidate_hier ();

  db::Cell *old_cell = m_cell_ptrs [target_cell_index];
  if (old_cell) {
    old_cell->unregister ();
    if (retain_layout) {
      new_cell->Cell::operator= (*old_cell);
    }
  }

  if (manager () && manager ()->transacting ()) {
    //  note the "take" method - this takes out the cell but does not delete it (we need it inside undo)
    m_cells.take (iterator (old_cell));
    manager ()->queue (this, new NewRemoveCellOp (target_cell_index, cell_name (target_cell_index), true /*remove*/, old_cell));
  } else {
    m_cells.erase (iterator (old_cell));
  }

  m_cells.push_back_ptr (new_cell);
  m_cell_ptrs [target_cell_index] = new_cell;

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new NewRemoveCellOp (target_cell_index, m_cell_names [target_cell_index], false /*new*/, 0));
  }
}

void
Layout::replace_instances_of (cell_index_type src_cell_index, cell_index_type target_cell_index)
{
  //  replace all instances of the new cell with the original one
  std::vector<std::pair<db::cell_index_type, db::Instance> > parents;
  for (db::Cell::parent_inst_iterator pi = cell (src_cell_index).begin_parent_insts (); ! pi.at_end (); ++pi) {
    parents.push_back (std::make_pair (pi->parent_cell_index (), pi->child_inst ()));
  }

  for (std::vector<std::pair<db::cell_index_type, db::Instance> >::const_iterator p = parents.begin (); p != parents.end (); ++p) {
    db::CellInstArray ia = p->second.cell_inst ();
    ia.object ().cell_index (target_cell_index);
    cell (p->first).replace (p->second, ia);
  }
}

void 
Layout::get_pcell_variant_as (pcell_id_type pcell_id, const std::vector<tl::Variant> &p, cell_index_type target_cell_index, ImportLayerMapping *layer_mapping, bool retain_layout)
{
  pcell_header_type *header = pcell_header (pcell_id);
  tl_assert (header != 0);

  std::vector<tl::Variant> buffer;
  const std::vector<tl::Variant> &parameters = gauge_parameters (p, header->declaration (), buffer);

  //  this variant must not exist yet for "get as" semantics
  tl_assert (header->get_variant (*this, parameters) == 0);

  tl_assert (m_cell_ptrs [target_cell_index] != 0);
 
  pcell_variant_type *variant = new pcell_variant_type (target_cell_index, *this, pcell_id, parameters);
  replace_cell (target_cell_index, variant, retain_layout);

  if (! retain_layout) {
    //  produce the layout unless we retained it
    variant->update (layer_mapping);
  }
}

cell_index_type 
Layout::get_pcell_variant_dict (pcell_id_type pcell_id, const std::map<std::string, tl::Variant> &p)
{
  pcell_header_type *header = pcell_header (pcell_id);
  tl_assert (header != 0);

  std::vector<tl::Variant> parameters;
  const std::vector<db::PCellParameterDeclaration> &pcp = header->declaration ()->parameter_declarations ();
  parameters.reserve (pcp.size ());
  for (std::vector<db::PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end(); ++pd) {
    std::map<std::string, tl::Variant>::const_iterator pp = p.find (pd->get_name ());
    if (pp == p.end ()) {
      parameters.push_back (pd->get_default ());
    } else {
      parameters.push_back (pp->second);
    }
  }

  pcell_variant_type *variant = header->get_variant (*this, parameters);
  if (! variant) {

    std::string b (header->get_name ());
    if (m_cell_map.find (b.c_str ()) != m_cell_map.end ()) {
      b = uniquify_cell_name (b.c_str ());
    }

    //  create a new cell 
    cell_index_type new_index = allocate_new_cell ();

    variant = new pcell_variant_type (new_index, *this, pcell_id, parameters);
    m_cells.push_back_ptr (variant);
    m_cell_ptrs [new_index] = variant;

    //  enter its index and cell_name
    register_cell_name (b.c_str (), new_index);

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
    }

    // produce the layout
    variant->update ();

  }

  return variant->cell_index ();
}

cell_index_type 
Layout::get_pcell_variant (pcell_id_type pcell_id, const std::vector<tl::Variant> &p)
{
  pcell_header_type *header = pcell_header (pcell_id);
  tl_assert (header != 0);

  std::vector<tl::Variant> buffer;
  const std::vector<tl::Variant> &parameters = gauge_parameters (p, header->declaration (), buffer);

  pcell_variant_type *variant = header->get_variant (*this, parameters);
  if (! variant) {

    std::string b (header->get_name ());
    if (m_cell_map.find (b.c_str ()) != m_cell_map.end ()) {
      b = uniquify_cell_name (b.c_str ());
    }

    //  create a new cell 
    cell_index_type new_index = allocate_new_cell ();

    variant = new pcell_variant_type (new_index, *this, pcell_id, parameters);
    m_cells.push_back_ptr (variant);
    m_cell_ptrs [new_index] = variant;

    //  enter its index and cell_name
    register_cell_name (b.c_str (), new_index);

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
    }

    // produce the layout
    variant->update ();

  }

  return variant->cell_index ();
}

const Layout::pcell_header_type *
Layout::pcell_header (pcell_id_type pcell_id) const
{
  return (const_cast<db::Layout *> (this))->pcell_header (pcell_id);
}

Layout::pcell_header_type *
Layout::pcell_header (pcell_id_type pcell_id)
{
  if (pcell_id >= m_pcells.size ()) {
    return 0;
  } else {
    return m_pcells [pcell_id];
  }
}

std::pair<bool, pcell_id_type>
Layout::pcell_by_name (const char *name) const
{
  std::map<std::string, pcell_id_type>::const_iterator pcid = m_pcell_ids.find (std::string (name));
  if (pcid != m_pcell_ids.end ()) {
    return std::make_pair (true, pcid->second);
  } else {
    return std::make_pair (false, pcell_id_type (0));
  }
}

pcell_id_type
Layout::register_pcell (const std::string &name, pcell_declaration_type *declaration)
{
  //  No undo/redo support for PCell registration. The interactions with PCell variants
  //  (for which undo/redo support is available) is too complex ...
  tl_assert (!manager () || !manager ()->transacting ());

  pcell_id_type id;

  pcell_name_map::const_iterator pcid = m_pcell_ids.find (name);
  if (pcid != m_pcell_ids.end ()) {

    //  replace any existing PCell declaration with that name.
    id = pcid->second;
    if (m_pcells [id]) {
      delete m_pcells [id];
    }

    m_pcells [id] = new pcell_header_type (id, name, declaration);

  } else {

    id = (unsigned int) m_pcells.size ();
    m_pcells.push_back (new pcell_header_type (id, name, declaration));
    m_pcell_ids.insert (std::make_pair (std::string (name), id));

  }

  declaration->m_id = id;
  declaration->m_name = name;
  declaration->mp_layout = this;

  //  marks this object being held by the layout
  declaration->keep ();

  return id;
}

const Layout::pcell_declaration_type *
Layout::pcell_declaration (pcell_id_type pcell_id) const
{
  const pcell_header_type *header = pcell_header (pcell_id);
  return header ? header->declaration () : 0;
}

db::cell_index_type 
Layout::convert_cell_to_static (db::cell_index_type ci)
{
  tl_assert (is_valid_cell_index (ci));
  db::cell_index_type ret_ci = ci;

  if (m_cell_ptrs [ci] && m_cell_ptrs [ci]->is_proxy ()) {

    invalidate_hier ();

    const cell_type &org_cell = cell (ci);

    //  Note: convert to static cell by explicitly cloning to the db::Cell class
    ret_ci = add_cell (org_cell.get_basic_name ().c_str ());
    cell_type &new_cell = cell (ret_ci);
    new_cell = org_cell;
    new_cell.set_cell_index (ret_ci);

    //  remove guiding shapes.
    if (m_layers.guiding_shape_layer_maybe () >= 0) {
      new_cell.shapes (m_layers.guiding_shape_layer_maybe ()).clear ();
    }

  }

  return ret_ci;
}

std::pair<bool, db::pcell_id_type> 
Layout::is_pcell_instance (cell_index_type cell_index) const
{
  const Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().is_pcell_instance (lib_proxy->library_cell_index ());
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (child_cell);
  if (pcell_variant) {
    return std::make_pair (true, pcell_variant->pcell_id ());
  } else {
    return std::make_pair (false, db::pcell_id_type(0));
  }
}

const Layout::pcell_declaration_type *
Layout::pcell_declaration_for_pcell_variant (cell_index_type variant_cell_index) const
{
  const Cell *variant_cell = &cell (variant_cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (variant_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().pcell_declaration_for_pcell_variant (lib_proxy->library_cell_index ());
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (variant_cell);
  if (pcell_variant) {
    return pcell_declaration (pcell_variant->pcell_id ());
  } else {
    return 0;
  }
}

std::pair<db::Library *, db::cell_index_type>
Layout::defining_library (cell_index_type cell_index) const
{
  const db::Layout *layout = this;
  db::Library *lib = 0;

  while (true) {

    const Cell *child_cell = &layout->cell (cell_index);
    const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
    if (lib_proxy) {

      lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
      tl_assert (lib != 0);
      cell_index = lib_proxy->library_cell_index ();
      layout = &lib->layout ();

    } else {
      return std::pair<db::Library *, db::cell_index_type> (lib, cell_index);
    }

  }
}

tl::Variant
Layout::get_pcell_parameter (cell_index_type cell_index, const std::string &name) const
{
  const Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().get_pcell_parameter (lib_proxy->library_cell_index (), name);
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (child_cell);
  if (pcell_variant) {
    return pcell_variant->parameter_by_name (name);
  } else {
    return tl::Variant ();
  }
}

std::map<std::string, tl::Variant>
Layout::get_named_pcell_parameters (cell_index_type cell_index) const
{
  const Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().get_named_pcell_parameters (lib_proxy->library_cell_index ());
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (child_cell);
  if (pcell_variant) {
    return pcell_variant->parameters_by_name ();
  } else {
    return std::map<std::string, tl::Variant> ();
  }
}

const std::vector<tl::Variant> &
Layout::get_pcell_parameters (cell_index_type cell_index) const
{
  const Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {
    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    tl_assert (lib != 0);
    return lib->layout ().get_pcell_parameters (lib_proxy->library_cell_index ());
  }

  const PCellVariant *pcell_variant = dynamic_cast<const PCellVariant *> (child_cell);
  if (pcell_variant) {
    return pcell_variant->parameters ();
  } else {
    static std::vector<tl::Variant> empty;
    return empty;
  }
}

cell_index_type
Layout::get_pcell_variant_cell (cell_index_type cell_index, const std::vector<tl::Variant> &new_parameters)
{
  Cell *child_cell = &cell (cell_index);

  const LibraryProxy *lib_proxy = dynamic_cast<const LibraryProxy *> (child_cell);
  if (lib_proxy) {

    Library *lib = LibraryManager::instance ().lib (lib_proxy->lib_id ());
    cell_index_type new_lib_cell_index = lib->layout ().get_pcell_variant_cell (lib_proxy->library_cell_index (), new_parameters);

    if (new_lib_cell_index != lib_proxy->library_cell_index ()) {
      return get_lib_proxy (lib, new_lib_cell_index);
    }

  } else {

    PCellVariant *pcell_variant = dynamic_cast<PCellVariant *> (child_cell);
    if (pcell_variant) {
      return get_pcell_variant (pcell_variant->pcell_id (), new_parameters);
    } 

  }

  return cell_index;

}

bool
Layout::has_context_info () const
{
  for (auto i = m_meta_info.begin (); i != m_meta_info.end (); ++i) {
    if (i->second.persisted) {
      return true;
    }
  }

  return false;
}

bool
Layout::has_context_info (cell_index_type cell_index) const
{
  auto c = m_meta_info_by_cell.find (cell_index);
  if (c != m_meta_info_by_cell.end ()) {
    for (auto i = c->second.begin (); i != c->second.end (); ++i) {
      if (i->second.persisted) {
        return true;
      }
    }
  }

  const db::Cell &cref = cell (cell_index);
  if (cref.is_proxy () && ! cref.is_top ()) {
    return true;
  } else {
    return false;
  }
}

bool
Layout::get_context_info (std::vector <std::string> &strings) const
{
  LayoutOrCellContextInfo info;
  if (! get_context_info (info)) {
    return false;
  } else {
    info.serialize (strings);
    return true;
  }
}

bool
Layout::get_context_info (LayoutOrCellContextInfo &info) const
{
  for (auto i = m_meta_info.begin (); i != m_meta_info.end (); ++i) {
    if (i->second.persisted) {
      std::pair<tl::Variant, std::string> &mi = info.meta_info [m_meta_info_names [i->first] ];
      mi.first = i->second.value;
      mi.second = i->second.description;
    }
  }

  return true;
}

void
Layout::fill_meta_info_from_context (std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to)
{
  fill_meta_info_from_context (LayoutOrCellContextInfo::deserialize (from, to));
}

void
Layout::fill_meta_info_from_context (const LayoutOrCellContextInfo &context_info)
{
  if (! context_info.meta_info.empty ()) {
    for (auto i = context_info.meta_info.begin (); i != context_info.meta_info.end (); ++i) {
      meta_info_name_id_type name_id = meta_info_name_id (i->first);
      m_meta_info [name_id] = MetaInfo (i->second.second, i->second.first, true);
    }
  }
}

bool
Layout::get_context_info (cell_index_type cell_index, std::vector <std::string> &strings) const
{
  LayoutOrCellContextInfo info;
  if (! get_context_info (cell_index, info)) {
    return false;
  } else {
    info.serialize (strings);
    return true;
  }
}

bool
Layout::get_context_info (cell_index_type cell_index, LayoutOrCellContextInfo &info) const
{
  bool any_meta = false;

  auto cmi = m_meta_info_by_cell.find (cell_index);
  if (cmi != m_meta_info_by_cell.end ()) {
    for (auto i = cmi->second.begin (); i != cmi->second.end (); ++i) {
      if (i->second.persisted) {
        std::pair<tl::Variant, std::string> &mi = info.meta_info [m_meta_info_names [i->first] ];
        mi.first = i->second.value;
        mi.second = i->second.description;
        any_meta = true;
      }
    }
  }

  const db::Cell *cptr = &cell (cell_index);

  const db::ColdProxy *cold_proxy = dynamic_cast <const db::ColdProxy *> (cptr);
  if (cold_proxy) {
    info = cold_proxy->context_info ();
    return true;
  }

  const db::Layout *ly = this;

  const db::LibraryProxy *lib_proxy;
  while (ly != 0 && (lib_proxy = dynamic_cast <const db::LibraryProxy *> (cptr)) != 0) {

    const db::Library *lib = db::LibraryManager::instance ().lib (lib_proxy->lib_id ());
    if (! lib) {
      return any_meta; //  abort
    } else {

      //  one level of library indirection
      ly = &lib->layout ();
      cptr = &ly->cell (lib_proxy->library_cell_index ());
      info.lib_name = lib->get_name ();

    }

  }

  const db::PCellVariant *pcell_variant = dynamic_cast <const db::PCellVariant *> (cptr);
  if (pcell_variant) {
    
    const db::PCellDeclaration *pcell_decl = ly->pcell_declaration (pcell_variant->pcell_id ());

    const std::vector<db::PCellParameterDeclaration> &pcp = pcell_decl->parameter_declarations ();
    std::vector<db::PCellParameterDeclaration>::const_iterator pd = pcp.begin ();
    for (std::vector<tl::Variant>::const_iterator p = pcell_variant->parameters ().begin (); p != pcell_variant->parameters ().end () && pd != pcp.end (); ++p, ++pd) {
      info.pcell_parameters.insert (std::make_pair (pd->get_name (), *p));
    }

    const db::PCellHeader *header = ly->pcell_header (pcell_variant->pcell_id ());
    info.pcell_name = header->get_name ();

  } else if (ly != this) {
    info.cell_name = ly->cell_name (cptr->cell_index ());
  }

  return true;
}

void
Layout::fill_meta_info_from_context (cell_index_type cell_index, std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to)
{
  fill_meta_info_from_context (cell_index, LayoutOrCellContextInfo::deserialize (from, to));
}

void
Layout::fill_meta_info_from_context (cell_index_type cell_index, const LayoutOrCellContextInfo &context_info)
{
  if (! context_info.meta_info.empty ()) {

    meta_info_map &mi = m_meta_info_by_cell [cell_index];

    for (auto i = context_info.meta_info.begin (); i != context_info.meta_info.end (); ++i) {
      meta_info_name_id_type name_id = meta_info_name_id (i->first);
      mi [name_id] = MetaInfo (i->second.second, i->second.first, true);
    }

  }
}

void
Layout::restore_proxies (ImportLayerMapping *layer_mapping)
{
  std::vector<db::ColdProxy *> cold_proxies;

  for (iterator c = begin (); c != end (); ++c) {
    db::ColdProxy *proxy = dynamic_cast<db::ColdProxy *> (c.operator-> ());
    if (proxy) {
      cold_proxies.push_back (proxy);
    }
  }

  bool needs_cleanup = false;
  for (std::vector<db::ColdProxy *>::const_iterator p = cold_proxies.begin (); p != cold_proxies.end (); ++p) {
    if (recover_proxy_as ((*p)->cell_index (), (*p)->context_info (), layer_mapping)) {
      needs_cleanup = true;
    }
  }

  if (needs_cleanup) {
    cleanup ();
  }
}

bool
Layout::recover_proxy_as (cell_index_type cell_index, std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to, ImportLayerMapping *layer_mapping)
{
  if (from == to) {
    return false;
  }

  return recover_proxy_as (cell_index, LayoutOrCellContextInfo::deserialize (from, to), layer_mapping);
}

bool
Layout::recover_proxy_as (cell_index_type cell_index, const LayoutOrCellContextInfo &info, ImportLayerMapping *layer_mapping)
{
  if (! info.lib_name.empty ()) {

    db::Cell *lib_cell = 0;

    Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (info.lib_name, m_tech_name);
    if (lib) {
      lib_cell = lib->layout ().recover_proxy_no_lib (info);
    }

    if (lib_cell) {
      get_lib_proxy_as (lib, lib_cell->cell_index (), cell_index, layer_mapping);
      return true;
    }

  } else {

    if (! info.pcell_name.empty ()) {

      std::pair<bool, pcell_id_type> pc = pcell_by_name (info.pcell_name.c_str ());
      if (pc.first) {
        get_pcell_variant_as (pc.second, pcell_declaration (pc.second)->map_parameters (info.pcell_parameters), cell_index, layer_mapping);
        return true;
      }

    } else if (! info.cell_name.empty ()) {

      //  This should not happen. A cell (given by the cell name) cannot be proxy to another cell in the same layout.
      tl_assert (false);

    } 

  }

  if (! dynamic_cast<db::ColdProxy *> (m_cell_ptrs [cell_index])) {
    //  create a cold proxy representing the context information so we can restore it
    create_cold_proxy_as (info, cell_index);
  }

  return false;
}

db::Cell *
Layout::recover_proxy (std::vector <std::string>::const_iterator from, std::vector <std::string>::const_iterator to)
{
  if (from == to) {
    return 0;
  }

  return recover_proxy (LayoutOrCellContextInfo::deserialize (from, to));
}

db::Cell *
Layout::recover_proxy (const LayoutOrCellContextInfo &info)
{
  if (! info.lib_name.empty ()) {

    Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (info.lib_name, m_tech_name);

    db::Cell *lib_cell = 0;
    if (lib) {
      lib_cell = lib->layout ().recover_proxy_no_lib (info);
    }

    if (lib_cell) {
      return m_cell_ptrs [get_lib_proxy (lib, lib_cell->cell_index ())];
    }

  } else {

    db::Cell *proxy = recover_proxy_no_lib (info);
    if (proxy) {
      return proxy;
    }

  }

  return m_cell_ptrs [create_cold_proxy (info)];
}

db::Cell *
Layout::recover_proxy_no_lib (const LayoutOrCellContextInfo &info)
{
  if (! info.pcell_name.empty ()) {

    std::pair<bool, pcell_id_type> pc = pcell_by_name (info.pcell_name.c_str ());
    if (pc.first) {
      cell_index_type cell_index = get_pcell_variant (pc.second, pcell_declaration (pc.second)->map_parameters (info.pcell_parameters));
      return m_cell_ptrs [cell_index];
    }

  } else if (! info.cell_name.empty ()) {

    std::pair<bool, cell_index_type> cc = cell_by_name (info.cell_name.c_str ());
    if (cc.first) {
      return m_cell_ptrs [cc.second];
    }

  }

  return 0;
}

std::string 
Layout::display_name (cell_index_type cell_index) const
{
  return cell (cell_index).get_display_name ();
}

std::string 
Layout::basic_name (cell_index_type cell_index) const
{
  return cell (cell_index).get_basic_name ();
}

void
Layout::register_lib_proxy (db::LibraryProxy *lib_proxy)
{
  m_lib_proxy_map.insert (std::make_pair (std::make_pair (lib_proxy->lib_id (), lib_proxy->library_cell_index ()), lib_proxy->Cell::cell_index ()));
}

void
Layout::unregister_lib_proxy (db::LibraryProxy *lib_proxy)
{
  m_lib_proxy_map.erase (std::make_pair (lib_proxy->lib_id (), lib_proxy->library_cell_index ()));
}

void
Layout::get_lib_proxy_as (Library *lib, cell_index_type cell_index, cell_index_type target_cell_index, ImportLayerMapping *layer_mapping, bool retain_layout)
{
  tl_assert (m_cell_ptrs [target_cell_index] != 0);
 
  LibraryProxy *proxy = new LibraryProxy (target_cell_index, *this, lib->get_id (), cell_index);
  replace_cell (target_cell_index, proxy, retain_layout);

  if (! retain_layout) {
    //  produce the layout unless we retained it
    proxy->update (layer_mapping);
  }
}

cell_index_type
Layout::get_lib_proxy (Library *lib, cell_index_type cell_index)
{
  lib_proxy_map::const_iterator lp = m_lib_proxy_map.find (std::make_pair (lib->get_id (), cell_index));
  if (lp != m_lib_proxy_map.end ()) {
    return lp->second;
  } else {

    //  create a new unique name
    std::string b (lib->layout ().basic_name (cell_index));
    if (m_cell_map.find (b.c_str ()) != m_cell_map.end ()) {
      b = uniquify_cell_name (b.c_str ());
    }

    //  create a new cell (a LibraryProxy)
    cell_index_type new_index = allocate_new_cell ();

    LibraryProxy *proxy = new LibraryProxy (new_index, *this, lib->get_id (), cell_index);
    m_cells.push_back_ptr (proxy);
    m_cell_ptrs [new_index] = proxy;

    //  enter its index and cell_name
    register_cell_name (b.c_str (), new_index);

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
    }

    //  produce the layout
    proxy->update ();

    return new_index;

  }
}

cell_index_type
Layout::create_cold_proxy (const db::LayoutOrCellContextInfo &info)
{
  //  create a new unique name
  std::string b;
  if (! info.cell_name.empty ()) {
    b = info.cell_name;
  } else if (! info.pcell_name.empty ()) {
    b = info.pcell_name;
  }
  if (m_cell_map.find (b.c_str ()) != m_cell_map.end ()) {
    b = uniquify_cell_name (b.c_str ());
  }

  //  create a new cell (a LibraryProxy)
  cell_index_type new_index = allocate_new_cell ();

  ColdProxy *proxy = new ColdProxy (new_index, *this, info);
  m_cells.push_back_ptr (proxy);
  m_cell_ptrs [new_index] = proxy;

  //  enter its index and cell_name
  register_cell_name (b.c_str (), new_index);

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new NewRemoveCellOp (new_index, m_cell_names [new_index], false /*new*/, 0));
  }

  return new_index;
}

void
Layout::create_cold_proxy_as (const db::LayoutOrCellContextInfo &info, cell_index_type target_cell_index)
{
  tl_assert (m_cell_ptrs [target_cell_index] != 0);

  ColdProxy *proxy = new ColdProxy (target_cell_index, *this, info);
  replace_cell (target_cell_index, proxy, true);
}

void
Layout::redo (db::Op *op)
{
  const LayoutOp *layout_op = dynamic_cast <const LayoutOp *> (op);
  if (layout_op) {
    layout_op->redo (this);
  }
}

void 
Layout::undo (db::Op *op)
{
  const LayoutOp *layout_op = dynamic_cast <const LayoutOp *> (op);
  if (layout_op) {
    layout_op->undo (this);
  }
}

}

