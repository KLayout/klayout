
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


#include "layCellView.h"
#include "layLayoutViewBase.h"
#if defined(HAVE_QT)
#  include "layStream.h"
#endif
#include "dbLayout.h"
#include "dbWriter.h"
#include "dbReader.h"
#include "tlLog.h"
#include "tlStaticObjects.h"

#include <algorithm>

namespace lay
{

// -------------------------------------------------------------

static std::string 
filename_for_caption (const std::string &fn)
{
  const char *cp = fn.c_str ();
  const char *cpp = cp + fn.size ();
  while (cpp > cp && cpp [-1] != '\\' && cpp [-1] != '/') {
    --cpp;
  }
  return cpp;
}

// -------------------------------------------------------------
//  LayoutHandle implementation


LayoutHandle::LayoutHandle (db::Layout *layout, const std::string &filename)
  : mp_layout (layout),
    m_ref_count (0),
    m_filename (filename),
    m_dirty (false),
    m_save_options_valid (false)
{
  layout->technology_changed_event.add (this, &LayoutHandle::on_technology_changed);

  //  layouts in the managed layouts space participate in spare proxy cleanup
  layout->do_cleanup (true);

  add_file_to_watcher (m_filename);

  if (! m_filename.empty ()) {
    rename (filename_for_caption (m_filename));
  } else {

    //  create a unique new name 
    static int nn = 0;

    std::string n;
    do {
      n = tl::sprintf ("L%d", ++nn);
    } while (find (n) != 0);

    m_name = n;
    ms_dict.insert (std::make_pair (n, this));

  }

  mp_layout->hier_changed_event.add (this, &LayoutHandle::layout_changed);
  mp_layout->bboxes_changed_any_event.add (this, &LayoutHandle::layout_changed);
  mp_layout->cell_name_changed_event.add (this, &LayoutHandle::layout_changed);
  mp_layout->prop_ids_changed_event.add (this, &LayoutHandle::layout_changed);
  mp_layout->layer_properties_changed_event.add (this, &LayoutHandle::layout_changed);

  if (tl::verbosity () >= 30) {
    tl::info << "Created layout " << name ();
  }
}

LayoutHandle::~LayoutHandle ()
{
  if (tl::verbosity () >= 30) {
    tl::info << "Deleted layout " << name ();
  }

  delete mp_layout;
  mp_layout = 0;

  if (find (m_name) == this) {
    ms_dict.erase (m_name);
  }

  remove_file_from_watcher (filename ());
}

void
LayoutHandle::remove_file_from_watcher (const std::string &path)
{
#if defined(HAVE_QT)
  file_watcher ().remove_file (path);
#endif
}

void
LayoutHandle::add_file_to_watcher (const std::string &path)
{
#if defined(HAVE_QT)
  file_watcher ().add_file (path);
#endif
}

void
LayoutHandle::on_technology_changed ()
{
  technology_changed_event ();
}

void 
LayoutHandle::layout_changed ()
{
  m_dirty = true;
}

void 
LayoutHandle::rename (const std::string &name, bool force)
{
  std::string n (name);

  if (n != m_name) {

    if (force || find (n) == 0) {
      ms_dict.erase (m_name);
      if (tl::verbosity () >= 40) {
        tl::info << "Renamed layout from " << m_name << " to " << n;
      }
      m_name = n;
      ms_dict.insert (std::make_pair (n, this));
      return;
    }

    //  rename using suffix "[u]" where u is a unique index
    int nn = 0;
    int ns = 0x40000000;
    do {
      n = name + tl::sprintf ("[%d]", nn + ns);
      if (find (n) != 0) {
        nn += ns;
      }
      ns /= 2;
    } while (ns > 0);

    n = name + tl::sprintf ("[%d]", nn + 1);

    if (tl::verbosity () >= 40) {
      tl::info << "Renamed layout from " << m_name << " to " << n;
    }

    if (find (m_name) == this) {
      ms_dict.erase (m_name);
    }

    m_name = n;
    ms_dict.insert (std::make_pair (n, this));
    return;

  }
}

db::Layout &
LayoutHandle::layout () const
{
  return *mp_layout;
}

void
LayoutHandle::set_filename (const std::string &fn) 
{
  remove_file_from_watcher (m_filename);
  m_filename = fn;
  add_file_to_watcher (m_filename);
}

const std::string &
LayoutHandle::filename () const
{
  return m_filename;
}

const std::string &
LayoutHandle::name () const
{
  return m_name;
}

void 
LayoutHandle::add_ref ()
{
  if (tl::verbosity () >= 50) {
    tl::info << "Add reference to " << m_name;
  }
  ++m_ref_count;
}

void 
LayoutHandle::remove_ref ()
{
  if (tl::verbosity () >= 50) {
    tl::info << "Remove reference from " << m_name;
  }
  if (--m_ref_count <= 0) {
    //  not nice, but hopefully we can do so:
    delete this;
  }
}

const std::string &
LayoutHandle::tech_name () const
{
  static std::string s_empty;
  return mp_layout ? mp_layout->technology_name () : s_empty;
}

const db::Technology *
LayoutHandle::technology () const
{
  return mp_layout ? mp_layout->technology () : 0;
}

void
LayoutHandle::apply_technology (const std::string &tn)
{
  set_tech_name (tn);
  apply_technology_event ();
  apply_technology_with_sender_event (this);
}

void 
LayoutHandle::set_tech_name (const std::string &tn)
{
  if (mp_layout && tn != tech_name ()) {
    mp_layout->set_technology_name (tn);
  }
}

LayoutHandle *
LayoutHandle::find (const std::string &name)
{
  std::map <std::string, LayoutHandle *>::const_iterator h = ms_dict.find (name);
  if (h == ms_dict.end ()) {
    return 0;
  } else {
    return h->second;
  }
}

LayoutHandle *
LayoutHandle::find_layout (const db::Layout *layout)
{
  for (auto h = ms_dict.begin (); h != ms_dict.end (); ++h) {
    if (h->second->mp_layout == layout) {
      return h->second;
    }
  }
  return 0;
}

void 
LayoutHandle::get_names (std::vector <std::string> &names)
{
  names.clear ();
  names.reserve (ms_dict.size ());
  for (std::map <std::string, LayoutHandle *>::const_iterator h = ms_dict.begin (); h != ms_dict.end (); ++h) {
    names.push_back (h->first);
  }
}

void
LayoutHandle::set_save_options (const db::SaveLayoutOptions &options, bool valid)
{
  m_save_options = options;
  m_save_options_valid = valid;
}

void
LayoutHandle::update_save_options (db::SaveLayoutOptions &options)
{
#if defined(HAVE_QT)
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {

    const lay::StreamWriterPluginDeclaration *decl = dynamic_cast <const lay::StreamWriterPluginDeclaration *> (&*cls);
    if (! decl || decl->options_alias ()) {
      continue;
    }

    std::unique_ptr<db::FormatSpecificWriterOptions> specific_options;
    if (options.get_options (decl->format_name ())) {
      specific_options.reset (options.get_options (decl->format_name ())->clone ());
    } else {
      specific_options.reset (decl->create_specific_options ());
    }

    if (specific_options.get ()) {
      decl->initialize_options_from_layout_handle (specific_options.get (), *this);
      options.set_options (specific_options.release ());
    }

  }
#endif
}

void 
LayoutHandle::save_as (const std::string &fn, tl::OutputStream::OutputStreamMode om, const db::SaveLayoutOptions &options, bool update, int keep_backups)
{
  if (update) {

    m_save_options = options;
    m_save_options_valid = true;
    //  We must not load with the original options after we have saved the file - hence we reset the
    //  reader options.
    m_load_options = db::LoadLayoutOptions ();

    remove_file_from_watcher (filename ());

    rename (filename_for_caption (fn));

    //  NOTE: we don't use set_filename since this would re-attach the file watcher
    m_filename = fn;

  }

  try {

    {
      //  The write needs to be finished before the file watcher gets the new modification time
      db::Writer writer (options);
      tl::OutputStream stream (fn, om, false, keep_backups);
      try {
        writer.write (*mp_layout, stream);
      } catch (...) {
        stream.reject ();
        throw;
      }
    }

    if (update) {
      add_file_to_watcher (filename ());
      m_dirty = false;
    }

  } catch (...) {

    if (update) {
      add_file_to_watcher (filename ());
    }

    throw;

  }
}

db::LayerMap 
LayoutHandle::load (const db::LoadLayoutOptions &options, const std::string &technology)
{
  m_load_options = options;
  m_save_options = db::SaveLayoutOptions ();
  m_save_options_valid = false;

  set_tech_name (technology);

  tl::InputStream stream (m_filename);
  db::Reader reader (stream);
  db::LayerMap new_lmap = reader.read (layout (), m_load_options);

  //  If there is no technology given and the reader reports one, use this one
  if (technology.empty ()) {
    std::string tech_from_reader = layout ().technology_name ();
    if (! tech_from_reader.empty ()) {
      set_tech_name (tech_from_reader);
    }
  }

  //  Update the file's data:
  remove_file_from_watcher (filename ());
  add_file_to_watcher (filename ());

  m_save_options.set_format (reader.format ());
  m_dirty = false;
  return new_lmap;
}

db::LayerMap 
LayoutHandle::load ()
{
  m_load_options = db::LoadLayoutOptions ();
  m_save_options = db::SaveLayoutOptions ();
  m_save_options_valid = false;

  set_tech_name (std::string ());

  tl::InputStream stream (m_filename);
  db::Reader reader (stream);
  db::LayerMap new_lmap = reader.read (layout (), m_load_options);

  //  Attach the technology from the reader if it reports one
  std::string tech_from_reader = layout ().technology_name ();
  if (! tech_from_reader.empty ()) {
    set_tech_name (tech_from_reader);
  }

  //  Update the file's data:
  remove_file_from_watcher (filename ());
  add_file_to_watcher (filename ());

  m_save_options.set_format (reader.format ());
  m_dirty = false;
  return new_lmap;
}

#if defined(HAVE_QT)
tl::FileSystemWatcher &
LayoutHandle::file_watcher ()
{
  if (! mp_file_watcher) {
    mp_file_watcher = new tl::FileSystemWatcher ();
    tl::StaticObjects::reg (&mp_file_watcher);
  }
  return *mp_file_watcher;
}

tl::FileSystemWatcher *LayoutHandle::mp_file_watcher = 0;
#endif

std::map <std::string, LayoutHandle *> LayoutHandle::ms_dict;

// -------------------------------------------------------------
//  LayoutHandleRef implementation

LayoutHandleRef::LayoutHandleRef ()
  : mp_handle (0)
{
  // .. nothing yet ..
}

LayoutHandleRef::LayoutHandleRef (LayoutHandle *h)
  : mp_handle (0)
{
  set (h);
}

LayoutHandleRef::LayoutHandleRef (const LayoutHandleRef &r)
  : mp_handle (0)
{
  set (r.mp_handle);
}

LayoutHandleRef::~LayoutHandleRef ()
{
  set (0);
}

bool 
LayoutHandleRef::operator== (const LayoutHandleRef &r) const
{
  return mp_handle == r.mp_handle;
}

LayoutHandleRef &
LayoutHandleRef::operator= (const LayoutHandleRef &r)
{
  if (&r != this) {
    set (r.mp_handle);
  }
  return *this;
}

void
LayoutHandleRef::set (LayoutHandle *h)
{
  if (mp_handle == h) {
    return;
  }

  if (mp_handle) {
    mp_handle->remove_ref ();
    mp_handle = 0;
  } 
  mp_handle = h;
  if (mp_handle) {
    mp_handle->add_ref ();
  }
}

LayoutHandle *
LayoutHandleRef::operator-> () const
{
  return mp_handle;
}

LayoutHandle *
LayoutHandleRef::get () const
{
  return mp_handle;
}

// -------------------------------------------------------------
//  CellView implementation

CellView::CellView () 
  : mp_ctx_cell (0), m_ctx_cell_index (0), mp_cell (0), m_cell_index (cell_index_type (-1))
{ }

bool 
CellView::operator== (const CellView &cv) const
{
  return m_layout_href == cv.m_layout_href 
        && mp_ctx_cell == cv.mp_ctx_cell && m_ctx_cell_index == cv.m_ctx_cell_index 
        && mp_cell == cv.mp_cell && m_cell_index == cv.m_cell_index 
        && m_unspecific_path == cv.m_unspecific_path && m_specific_path == cv.m_specific_path;
}

bool 
CellView::is_valid () const
{
  if (m_layout_href.get () == 0 || mp_cell == 0) {
    return false;
  }

  //  check, if the path references valid cell indices.
  for (unspecific_cell_path_type::const_iterator pp = m_unspecific_path.begin (); pp != m_unspecific_path.end (); ++pp) {
    if (! m_layout_href.get ()->layout ().is_valid_cell_index (*pp)) {
      return false;
    }
  }
  for (specific_cell_path_type::const_iterator pp = m_specific_path.begin (); pp != m_specific_path.end (); ++pp) {
    if (! pp->inst_ptr.instances () || ! pp->inst_ptr.instances ()->is_valid (pp->inst_ptr) || ! m_layout_href.get ()->layout ().is_valid_cell_index (pp->inst_ptr.cell_index ())) {
      return false;
    }
  }

  return true;
}

void 
CellView::set_unspecific_path (const unspecific_cell_path_type &p)
{
  tl_assert (m_layout_href.get () != 0);

  mp_cell = 0;
  m_cell_index = 0;
  m_unspecific_path = p;
  m_specific_path.clear ();

  if (p.size () > 0 && m_layout_href.get () && p.back () < m_layout_href->layout ().cells ()) {
    m_cell_index = p.back ();
    mp_cell = &m_layout_href->layout ().cell (p.back ());
  }

  mp_ctx_cell = mp_cell;
  m_ctx_cell_index = m_cell_index;
}

void 
CellView::set_specific_path (const specific_cell_path_type &p)
{
  tl_assert (m_layout_href.get () != 0);

  m_specific_path = p;
  for (specific_cell_path_type::iterator pp = m_specific_path.begin (); pp != m_specific_path.end (); ++pp) {
    //  fix elements of the path not associated with a certain array instance (this may happen if 
    //  unspecific selections are put into the path)
    if (pp->array_inst.at_end ()) {
      pp->array_inst = pp->inst_ptr.begin ();
    }
  }

  if (p.empty ()) {
    m_cell_index = m_ctx_cell_index;
    mp_cell = mp_ctx_cell;
  } else if (m_layout_href.get () && p.back ().inst_ptr.cell_index () < m_layout_href->layout ().cells ()) {
    m_cell_index = p.back ().inst_ptr.cell_index ();
    mp_cell = &m_layout_href->layout ().cell (m_cell_index);
  } else {
    reset_cell ();
  }
}

CellView::unspecific_cell_path_type 
CellView::combined_unspecific_path () const
{
  CellView::unspecific_cell_path_type path;
  path.reserve (m_unspecific_path.size () + m_specific_path.size ());
  path.insert (path.end (), m_unspecific_path.begin (), m_unspecific_path.end ());
  for (CellView::specific_cell_path_type::const_iterator p = m_specific_path.begin (); p != m_specific_path.end (); ++p) {
    path.push_back (p->inst_ptr.cell_index ());
  }
  return path;
}

void
CellView::set_cell (cell_index_type index)
{
  tl_assert (m_layout_href.get () != 0);

  db::Layout &layout = m_layout_href->layout ();
  
  if (! layout.is_valid_cell_index (index)) {

    reset_cell ();

  } else {

    m_cell_index = index;
    mp_cell = &layout.cell (m_cell_index);

    m_unspecific_path.clear ();
    m_specific_path.clear ();
    m_unspecific_path.push_back (index);

    while (! layout.cell (index).is_top ()) {
      index = *layout.cell (index).begin_parent_cells ();
      m_unspecific_path.push_back (index);
    }

    std::reverse (m_unspecific_path.begin (), m_unspecific_path.end ());

    mp_ctx_cell = mp_cell;
    m_ctx_cell_index = m_cell_index;

  }
}

void 
CellView::set_cell (const std::string &name)
{
  tl_assert (m_layout_href.get () != 0);

  std::pair<bool, db::cell_index_type> cp = m_layout_href->layout ().cell_by_name (name.c_str ());
  if (cp.first) {
    set_cell (cp.second);
  } else {
    reset_cell ();
  }
}

void 
CellView::reset_cell ()
{
  mp_cell = 0;
  m_cell_index = cell_index_type (-1);
  mp_ctx_cell = 0;
  m_ctx_cell_index = 0;
  m_unspecific_path.clear ();
  m_specific_path.clear ();
}

void 
CellView::set (lay::LayoutHandle *handle)
{
  reset_cell ();
  m_layout_href.set (handle);
}

CellView
CellView::deep_copy (db::Manager *manager) const
{
  CellView r;
  r.set (new lay::LayoutHandle (new db::Layout (manager), ""));
  r->layout () = (*this)->layout ();
  r.set_unspecific_path (unspecific_path ());
  r.set_specific_path (specific_path ());
  return r;
}

db::ICplxTrans
CellView::context_trans () const
{
  db::ICplxTrans trans;
  for (std::vector <db::InstElement>::const_iterator p = specific_path ().begin (); p != specific_path ().end (); ++p) {
    trans = trans * p->complex_trans ();
  }
  return trans;
}

db::DCplxTrans
CellView::context_dtrans () const
{
  tl_assert (m_layout_href.get () != 0);

  db::CplxTrans dbu_trans (m_layout_href->layout ().dbu ());
  return dbu_trans * context_trans () * dbu_trans.inverted ();
}


// -------------------------------------------------------------
//  CellView implementation

CellViewRef::CellViewRef ()
{
  // .. nothing yet ..
}

CellViewRef::CellViewRef (lay::CellView *cv, lay::LayoutViewBase *view)
  : mp_cv (cv), mp_view (view)
{
  // .. nothing yet ..
}

bool
CellViewRef::operator== (const CellView &cv) const
{
  if (! is_valid ()) {
    return false;
  } else {
    return mp_cv->operator== (cv);
  }
}

bool
CellViewRef::is_valid () const
{
  return mp_view && mp_cv;
}

int
CellViewRef::index () const
{
  if (!is_valid ()) {
    return -1;
  } else {
    return mp_view->index_of_cellview (mp_cv.get ());
  }
}

lay::LayoutViewBase *
CellViewRef::view ()
{
  return mp_view.get ();
}

lay::LayoutHandle *
CellViewRef::operator-> () const
{
  if (mp_cv) {
    return mp_cv->handle ();
  } else {
    return 0;
  }
}

void
CellViewRef::set_name (const std::string &name)
{
  if (is_valid ()) {
    mp_view->rename_cellview (name, mp_view->index_of_cellview (mp_cv.get ()));
  }
}

void
CellViewRef::set_unspecific_path (const CellViewRef::unspecific_cell_path_type &p)
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.set_unspecific_path (p);
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

void
CellViewRef::set_specific_path (const CellViewRef::specific_cell_path_type &p)
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.set_specific_path (p);
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

void
CellViewRef::set_cell (cell_index_type ci)
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.set_cell (ci);
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

void
CellViewRef::set_cell (const std::string &name)
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.set_cell (name);
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

void
CellViewRef::reset_cell ()
{
  if (is_valid ()) {
    lay::CellView cv = *mp_cv;
    cv.reset_cell ();
    mp_view->select_cellview (mp_view->index_of_cellview (mp_cv.get ()), cv);
  }
}

db::Cell *
CellViewRef::ctx_cell () const
{
  return is_valid () ? mp_cv->ctx_cell () : 0;
}

db::Cell *
CellViewRef::cell () const
{
  return is_valid () ? mp_cv->cell () : 0;
}

CellViewRef::unspecific_cell_path_type
CellViewRef::combined_unspecific_path () const
{
  if (is_valid ()) {
    return mp_cv->combined_unspecific_path ();
  } else {
    return CellViewRef::unspecific_cell_path_type ();
  }
}

const CellViewRef::unspecific_cell_path_type &
CellViewRef::unspecific_path () const
{
  if (is_valid ()) {
    return mp_cv->unspecific_path ();
  } else {
    static CellViewRef::unspecific_cell_path_type empty_path;
    return empty_path;
  }
}

const CellViewRef::specific_cell_path_type &
CellViewRef::specific_path () const
{
  if (is_valid ()) {
    return mp_cv->specific_path ();
  } else {
    static CellViewRef::specific_cell_path_type empty_path;
    return empty_path;
  }
}

db::ICplxTrans
CellViewRef::context_trans () const
{
  if (is_valid ()) {
    return mp_cv->context_trans ();
  } else {
    return db::ICplxTrans ();
  }
}

db::DCplxTrans
CellViewRef::context_dtrans () const
{
  if (is_valid ()) {
    return mp_cv->context_dtrans ();
  } else {
    return db::DCplxTrans ();
  }
}

}

