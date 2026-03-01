
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "layLayoutHandle.h"

#include "dbWriter.h"
#include "dbReader.h"
#include "tlStaticObjects.h"

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
  //  the layout gets held by the LayoutHandle object
  layout->keep ();

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
LayoutHandle::save_as (const std::string &fn, tl::OutputStream::OutputStreamMode om, const db::SaveLayoutOptions &options, bool update, int keep_backups)
{
  if (update) {

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

}
