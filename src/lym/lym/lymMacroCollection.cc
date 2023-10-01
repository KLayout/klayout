
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


#include "lymMacroCollection.h"

#include "lymMacroInterpreter.h"
#include "tlExceptions.h"
#include "gsiDecl.h"
#include "gsiInterpreter.h"

#include "tlString.h"
#include "tlStableVector.h"
#include "tlClassRegistry.h"
#include "tlLog.h"
#include "tlXMLParser.h"
#include "tlGlobPattern.h"
#include "tlInclude.h"
#include "tlProgress.h"
#include "tlFileUtils.h"
#include "tlResources.h"
#include "tlEnv.h"

#include "rba.h"
#include "pya.h"

#if defined(HAVE_QT)
#  include <QResource>
#endif

#include <fstream>
#include <memory>
#include <string>
#include <set>

namespace lym
{

// ----------------------------------------------------------------------

static MacroCollection ms_root;

MacroCollection::MacroCollection ()
  : mp_parent (0), m_virtual_mode (ProjectFolder), m_readonly (false)
{
  // .. nothing yet ..
}

MacroCollection::~MacroCollection ()
{
  do_clear ();
}

void MacroCollection::do_clear ()
{
  for (iterator m = begin (); m != end (); ++m) {
    delete m->second;
  }
  m_macros.clear ();

  for (child_iterator mm = begin_children (); mm != end_children (); ++mm) {
    delete mm->second;
  }
  m_folders.clear ();
}

void MacroCollection::begin_changes ()
{
  //  Note: it is very important that each on_changed occurs after exactly one begin_changes.
  //  (See #459 for example)
  if (mp_parent) {
    mp_parent->begin_changes ();
  } else {
#if defined(HAVE_QT)
    emit about_to_change ();
#endif
  }
}

void MacroCollection::on_menu_needs_update ()
{
#if defined(HAVE_QT)
  emit menu_needs_update ();
#endif
}

void MacroCollection::on_changed ()
{
  //  Note: it is very important that each on_changed occurs after exactly one begin_changes.
  //  (See #459 for example)
#if defined(HAVE_QT)
  emit changed ();
#endif
  on_macro_collection_changed (this);
}

void MacroCollection::on_macro_collection_changed (MacroCollection *mc)
{
  if (mp_parent) {
    mp_parent->on_macro_collection_changed (mc);
  } else {
#if defined(HAVE_QT)
    emit macro_collection_changed (mc);
#endif
  }
}

void MacroCollection::on_child_deleted (MacroCollection *mc)
{
#if defined(HAVE_QT)
  emit child_deleted (mc);
#endif
  on_macro_collection_deleted (mc);
}

void MacroCollection::on_macro_collection_deleted (MacroCollection *mc)
{
  if (mp_parent) {
    mp_parent->on_macro_collection_deleted (mc);
  } else {
#if defined(HAVE_QT)
    emit macro_collection_deleted (mc);
#endif
  }
}

void MacroCollection::on_macro_deleted_here (Macro *macro)
{
#if defined(HAVE_QT)
  emit macro_deleted_here (macro);
#endif
  on_macro_deleted (macro);
}

void MacroCollection::on_macro_deleted (Macro *macro)
{
  if (mp_parent) {
    mp_parent->on_macro_deleted (macro);
  } else {
#if defined(HAVE_QT)
    emit macro_deleted (macro);
#endif
  }
}

void MacroCollection::on_macro_changed (Macro *macro)
{
  if (mp_parent) {
    mp_parent->on_macro_changed (macro);
  } else {
#if defined(HAVE_QT)
    emit macro_changed (macro);
#endif
  }
}

void MacroCollection::collect_used_nodes (std::set <Macro *> &macros, std::set <MacroCollection *> &macro_collections)
{
  for (MacroCollection::child_iterator c = begin_children (); c != end_children (); ++c) {
    macro_collections.insert (c->second);
    c->second->collect_used_nodes (macros, macro_collections);
  }
  for (MacroCollection::iterator c = begin (); c != end (); ++c) {
    macros.insert (c->second);
  }
}

Macro *MacroCollection::macro_by_name (const std::string &name, Macro::Format format)
{
  std::multimap <std::string, Macro *>::iterator i = m_macros.find (name);
  while (i != m_macros.end () && i->first == name) {
    if (format == Macro::NoFormat || i->second->format () == format) {
      return i->second;
    }
    ++i;
  }
  return 0;
}

const Macro *MacroCollection::macro_by_name (const std::string &name, Macro::Format format) const
{
  std::multimap <std::string, Macro *>::const_iterator i = m_macros.find (name);
  while (i != m_macros.end () && i->first == name) {
    if (format == Macro::NoFormat || i->second->format () == format) {
      return i->second;
    }
    ++i;
  }
  return 0;
}

MacroCollection *MacroCollection::folder_by_name (const std::string &name)
{
  std::map <std::string, MacroCollection *>::iterator i = m_folders.find (name);
  if (i != m_folders.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

const MacroCollection *MacroCollection::folder_by_name (const std::string &name) const
{
  std::map <std::string, MacroCollection *>::const_iterator i = m_folders.find (name);
  if (i != m_folders.end ()) {
    return i->second;
  } else {
    return 0;
  }
}

std::string MacroCollection::path () const
{
  if (m_virtual_mode) {
    return m_path;
  } else if (mp_parent) {
    return tl::combine_path (mp_parent->path (), m_path);
  } else {
    return m_path;
  }
}

std::string MacroCollection::display_string () const
{
  if (m_virtual_mode) {
    return "[" + m_description + "]";
  } else {
    std::string r = name ();
    if (! m_description.empty ()) {
      r += " - " + m_description;
    }
    return r;
  }
}

void
MacroCollection::make_readonly (bool f)
{
  if (m_readonly != f) {
    begin_changes ();
    m_readonly = f;
    on_changed ();
  }
}

MacroCollection *
MacroCollection::add_folder (const std::string &description, const std::string &p, const std::string &cat, bool readonly, bool auto_create)
{
  if (! p.empty () && p[0] == ':') {

    readonly = true;

  } else {

    std::string fp = p;
    if (! tl::is_absolute (fp)) {
      fp = tl::combine_path (path (), fp);
    }

    if (! tl::file_exists (fp)) {

      //  Try to create the folder since it does not exist yet or skip that one
      if (readonly || ! auto_create) {

        if (tl::verbosity () >= 20) {
          tl::log << tl::to_string (tr ("Folder does not exist - skipping: ")) << fp;
        }
        return 0;

      } else {

        if (tl::verbosity () >= 20) {
          tl::log << tl::to_string (tr ("Folder does not exist yet - trying to create it: ")) << fp;
        }
        if (! tl::mkpath (fp)) {
          if (tl::verbosity () >= 20) {
            tl::error << tl::to_string (tr ("Unable to create folder path: ")) << fp;
          }
          return 0;
        }
      }

    }

    if (! tl::is_dir (fp)) {
      if (tl::verbosity () >= 20) {
        tl::error << tl::to_string (tr ("Folder is not a directory - skipping: ")) << fp;
      }
      return 0;
    }

    for (child_iterator f = m_folders.begin (); f != m_folders.end (); ++f) {
      //  skip, if that folder is in the collection already
      if (f->second->path () == fp) {
        return 0;
      }
    }

    if (! readonly && ! tl::is_writable (fp)) {
      readonly = true;
      if (tl::verbosity () >= 20) {
        tl::log << tl::to_string (tr ("Folder is read-only: ")) << fp;
      }
    }

  }

  begin_changes ();

  MacroCollection *mc = m_folders.insert (std::make_pair (p, new MacroCollection ())).first->second;
  mc->set_name (p);
  mc->set_description (description);
  mc->set_category (cat);
  mc->set_readonly (readonly);
  mc->scan ();
  mc->set_parent (this);

  on_changed ();
  on_macro_changed (0);

  return mc;
}

void MacroCollection::rescan ()
{
  for (std::map <std::string, MacroCollection *>::const_iterator m = m_folders.begin (); m != m_folders.end (); ++m) {
    m->second->scan ();
  }
}

#if defined(HAVE_QT)
namespace {

  /**
   *  @brief A QResource variant that allows access to the children
   */
  class ResourceWithChildren
    : public QResource
  {
  public:
    ResourceWithChildren (const QString &path) : QResource (path) { }
    using QResource::children;
  };

}
#endif

//  Some directories are ignored in addition to dotfiles
static bool dir_is_ignored (const std::string &dn)
{
  static std::set <std::string> ignored;
  static bool initialized = false;

  if (! initialized) {
    //  a colon-separated list of directory names
    std::string ign = tl::get_env ("KLAYOUT_IGNORE_MACRO_DIRS", "__pycache__");
    auto ignv = tl::split (ign, ":");
    ignored.insert (ignv.begin (), ignv.end ());
  }

  return ignored.find (dn) != ignored.end ();
}

void MacroCollection::scan ()
{
  std::string p = path ();

  if (tl::verbosity () >= 20) {
    tl::info << tl::to_string (tr ("Scanning macro path ")) << p << " (readonly=" << m_readonly << ")";
  }

  if (! p.empty () && p[0] == ':') {

#if defined(HAVE_QT)

    ResourceWithChildren res (tl::to_qstring (p));
    QStringList children = res.children ();
    children.sort ();

    for (auto c = children.begin (); c != children.end (); ++c) {

      std::string url = p + "/" + tl::to_string (*c);
      QResource res (tl::to_qstring (url));
      if (res.size () > 0) {
        create_entry (url);
      }

    }

#else

    std::vector<std::string> res = tl::find_resources (std::string (p, 1) + "/*");
    for (auto c = res.begin (); c != res.end (); ++c) {
      create_entry (":" + *c);
    }

#endif

  } else {

    std::set<std::string> suffixes;
    suffixes.insert ("lym");
    suffixes.insert ("txt");
    //  TODO: should be either *.rb or *.python, depending on the category.
    //  Right now we rely on the folders not containing foreign files.
    suffixes.insert ("rb");
    suffixes.insert ("py");

    //  add the suffixes in the DSL interpreter declarations
    for (tl::Registrar<lym::MacroInterpreter>::iterator cls = tl::Registrar<lym::MacroInterpreter>::begin (); cls != tl::Registrar<lym::MacroInterpreter>::end (); ++cls) {
      if (! cls->suffix ().empty ()) {
        suffixes.insert (cls->suffix ());
      }
    }

    std::vector<std::string> files = tl::dir_entries (p, true /*with_files*/, false /*with_dirs*/, true /*without_dotfiles*/);
    for (auto f = files.begin (); f != files.end (); ++f) {
      if (suffixes.find (tl::extension_last (*f)) != suffixes.end ()) {
        create_entry (tl::combine_path (p, *f));
      }
    }

    //  NOTE: we add files to capture symlinks to directories
    std::vector<std::string> dirs = tl::dir_entries (p, true /*with_files*/, true /*with_dirs*/, true /*without_dotfiles*/);
    for (auto f = dirs.begin (); f != dirs.end (); ++f) {

      std::string fp = tl::combine_path (p, *f);
      if (! tl::is_dir (fp)) {
        continue;
      }

      if (dir_is_ignored (*f)) {
        continue;
      }

      try {

        MacroCollection *&mc = m_folders.insert (std::make_pair (*f, (MacroCollection *) 0)).first->second;
        if (! mc) {
          mc = new MacroCollection ();
          mc->set_name (*f);
          mc->set_virtual_mode (NotVirtual);
          bool ro = (m_readonly || ! tl::is_writable (fp));
          mc->set_readonly (ro);
          mc->set_parent (this);
          mc->scan ();
        }

      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
      }

    }

  }
}

void
MacroCollection::create_entry (const std::string &path)
{
  try {

    std::string n = tl::complete_basename (path);

    Macro::Format format = Macro::NoFormat;
    Macro::Interpreter interpreter = Macro::None;
    std::string dsl_name;
    bool autorun = false;

    std::unique_ptr<lym::Macro> new_macro;

    if (Macro::format_from_suffix (path, interpreter, dsl_name, autorun, format)) {

      iterator mm = m_macros.find (n);
      bool found = false;
      while (mm != m_macros.end () && mm->first == n && ! found) {
        if ((interpreter == Macro::None || mm->second->interpreter () == interpreter) &&
            (dsl_name.empty () || mm->second->dsl_interpreter () == dsl_name) &&
            mm->second->format () == format) {
          found = true;
        }
        ++mm;
      }
      if (! found) {
        Macro *m = new Macro ();
        new_macro.reset (m);
        m->set_interpreter (interpreter);
        m->set_autorun_default (autorun);
        m->set_autorun (autorun);
        m->set_dsl_interpreter (dsl_name);
        m->set_format (format);
        m->set_name (n);
        m->load_from (path);
        m->set_readonly (m_readonly);
        m->reset_modified ();
        m->set_is_file ();
        m->set_parent (this);
      }

    }

    if (new_macro.get ()) {
      m_macros.insert (std::make_pair (n, new_macro.release ()));
    }

  } catch (tl::Exception &ex) {
    tl::error << tl::to_string (tr ("Reading ")) << path << ": " << ex.msg ();
  }
}

void MacroCollection::clear ()
{
  begin_changes ();
  do_clear ();
  on_changed ();
}

void MacroCollection::erase (lym::Macro *mp)
{
  for (iterator m = m_macros.begin (); m != m_macros.end (); ++m) {
    if (m->second == mp) {
      begin_changes ();
      on_macro_deleted_here (mp);
      m_macros.erase (m);
      delete mp;
      on_changed ();
      return;
    }
  }
}

void MacroCollection::erase (lym::MacroCollection *mp)
{
  for (child_iterator f = m_folders.begin (); f != m_folders.end (); ++f) {
    if (f->second == mp) {
      begin_changes ();
      on_child_deleted (mp);
      m_folders.erase (f);
      delete mp;
      on_changed ();
      return;
    }
  }
}

void MacroCollection::erase (iterator i)
{
  begin_changes ();
  on_macro_deleted_here (i->second);
  delete i->second;
  m_macros.erase (i);
  on_changed ();
}

void MacroCollection::erase (child_iterator i)
{
  begin_changes ();
  on_child_deleted (i->second);
  delete i->second;
  m_folders.erase (i);
  on_changed ();
}

void MacroCollection::save ()
{
  for (child_iterator f = m_folders.begin (); f != m_folders.end (); ++f) {
    f->second->save ();
  }

  for (iterator m = m_macros.begin (); m != m_macros.end (); ++m) {
    if (m->second->is_modified () && ! m->second->is_readonly () && ! m->second->path ().empty ()) {
      try {
        m->second->save ();
      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
      }
    }
  }
}

bool MacroCollection::rename (const std::string &n)
{
  if (tl::verbosity () >= 20) {
    tl::info << tl::to_string (tr ("Renaming macro folder ")) << path () << " to " << n;
  }
  begin_changes ();
  if (! tl::rename_file (path (), n)) {
    on_changed ();
    return false;
  } else {
    m_path = n;
    on_changed ();
    return true;
  }
}

lym::MacroCollection *MacroCollection::create_folder (const char *prefix, bool mkdir)
{
  std::string name;
  int n = 0;
  do {
    name = (prefix ? prefix : "new_folder");
    if (n > 0) {
      name += "_" + tl::to_string (n);
    }
    if (m_folders.find (name) == m_folders.end ()) {
      break;
    }
    ++n;
  } while (true);

  if (mkdir && ! tl::mkpath (tl::combine_path (path (), name))) {
    return 0;
  }

  begin_changes ();

  lym::MacroCollection *m = m_folders.insert (std::make_pair (name, new lym::MacroCollection ())).first->second;
  m->set_virtual_mode (NotVirtual);
  m->set_name (name);
  m->set_parent (this);

  on_changed ();

  return m;
}

lym::Macro *MacroCollection::create (const char *prefix, Macro::Format format)
{
  std::string name;
  int n = 0;
  do {
    name = (prefix ? prefix : "new_macro");
    if (n > 0) {
      name += "_" + tl::to_string (n);
    }
    if (! macro_by_name (name, format)) {
      break;
    }
    ++n;
  } while (true);

  begin_changes ();

  lym::Macro *m = m_macros.insert (std::make_pair (name, new lym::Macro ()))->second;
  m->set_name (name);
  m->set_parent (this);

  on_changed ();

  return m;
}

void MacroCollection::add_unspecific (lym::Macro *m)
{
  begin_changes ();
  m_macros.insert (std::make_pair (m->name (), m));
  m->set_parent (this);
  on_changed ();
}

bool MacroCollection::add (lym::Macro *m)
{
  std::string d = tl::normalize_path (path ());
  std::string dd = tl::normalize_path (m->dir ());

  if (d == dd) {

    begin_changes ();
    m_macros.insert (std::make_pair (m->name (), m));
    m->set_parent (this);
    on_changed ();
    return true;

  } else {

    for (child_iterator c = begin_children (); c != end_children (); ++c) {
      if (c->second->add (m)) {
        return true;
      }
    }

    //  try to detect new child folders. If that is the case, create that folder and add
    //  the macro there.
    std::string dm = tl::normalize_path (m->dir ());
    while (true) {

      std::string folder_name = tl::filename (dm);
      dm = tl::dirname (dm);
      if (dm.empty () || dm == ".") {
        break;
      }

      if (dm == d) {

        begin_changes ();
        lym::MacroCollection *mc = m_folders.insert (std::make_pair (folder_name, new MacroCollection ())).first->second;
        mc->set_virtual_mode (NotVirtual);
        mc->set_name (folder_name);
        mc->set_parent (this);
        on_changed ();

        return mc->add (m);

      }

    }

  }

  return false;
}

bool MacroCollection::del ()
{
  if (tl::verbosity () >= 20) {
    tl::info << "Deleting macro folder " << path ();
  }
  return tl::rm_dir_recursive (path ());
}

void MacroCollection::rename_macro (Macro *macro, const std::string &new_name)
{
  iterator m = m_macros.find (macro->name ());
  while (m != m_macros.end () && m->first == macro->name ()) {
    if (m->second == macro) {
      m_macros.erase (m);
      m_macros.insert (std::make_pair (new_name, macro));
      return;
    }
    ++m;
  }
}

lym::Macro *MacroCollection::find_macro (const std::string &path)
{
  for (iterator m = begin (); m != end (); ++m) {
    if (tl::is_same_file (m->second->path (), path)) {
      return m->second;
    }
  }

  for (child_iterator mc = begin_children (); mc != end_children (); ++mc) {
    lym::Macro *macro = mc->second->find_macro (path);
    if (macro) {
      return macro;
    }
  }

  return 0;
}

MacroCollection &MacroCollection::root ()
{
  return ms_root;
}

static bool sync_macros (lym::MacroCollection *current, lym::MacroCollection *actual, bool safe)
{
  bool ret = false;

  if (actual) {
    current->make_readonly (actual->is_readonly ());
  }

  std::vector<lym::MacroCollection *> folders_to_delete;

  for (lym::MacroCollection::child_iterator m = current->begin_children (); m != current->end_children (); ++m) {
    lym::MacroCollection *cm = actual ? actual->folder_by_name (m->first) : 0;
    if (! cm) {
      folders_to_delete.push_back (m->second);
    }
  }

  if (actual) {
    for (lym::MacroCollection::child_iterator m = actual->begin_children (); m != actual->end_children (); ++m) {
      lym::MacroCollection *cm = current->folder_by_name (m->first);
      if (! cm) {
        cm = current->create_folder (m->first.c_str (), false);
        ret = true;
      }
      if (sync_macros(cm, m->second, safe)) {
        ret = true;
      }
    }
  }

  //  delete folders which do no longer exist
  for (std::vector<lym::MacroCollection *>::iterator m = folders_to_delete.begin (); m != folders_to_delete.end (); ++m) {
    ret = true;
    sync_macros (*m, 0, safe);
    current->erase (*m);
  }

  std::vector<lym::Macro *> macros_to_delete;

  for (lym::MacroCollection::iterator m = current->begin (); m != current->end (); ++m) {
    lym::Macro *cm = actual ? actual->macro_by_name (m->first, m->second->format ()) : 0;
    if (! cm) {
      macros_to_delete.push_back (m->second);
    }
  }

  if (actual) {
    for (lym::MacroCollection::iterator m = actual->begin (); m != actual->end (); ++m) {
      lym::Macro *cm = current->macro_by_name (m->first, m->second->format ());
      if (cm) {
        if (*cm != *m->second && (! safe || ! cm->is_modified ())) {
          cm->assign (*m->second);
        }
        cm->set_readonly (m->second->is_readonly ());
      } else {
        cm = current->create (m->first.c_str (), m->second->format ());
        cm->assign (*m->second);
        cm->set_readonly (m->second->is_readonly ());
        ret = true;
      }
    }
  }

  //  erase macros from collection which are no longer used
  for (std::vector<lym::Macro *>::const_iterator m = macros_to_delete.begin (); m != macros_to_delete.end (); ++m) {
    current->erase (*m);
    ret = true;
  }

  return ret;
}

void MacroCollection::reload (bool safe)
{
  //  create a new collection and synchronize

  lym::MacroCollection new_collection;
  for (lym::MacroCollection::child_iterator c = begin_children (); c != end_children (); ++c) {
    new_collection.add_folder (c->second->description (), c->second->path (), c->second->category (), c->second->is_readonly (), false /* don't auto-create folder */);
  }

  //  and synchronize current with the actual one
  sync_macros (this, &new_collection, safe);
}

static bool has_autorun_for (const lym::MacroCollection &collection, bool early)
{
  for (lym::MacroCollection::const_child_iterator c = collection.begin_children (); c != collection.end_children (); ++c) {
    if (has_autorun_for (*c->second, early)) {
      return true;
    }
  }

  for (lym::MacroCollection::const_iterator c = collection.begin (); c != collection.end (); ++c) {
    if (((early && c->second->is_autorun_early ()) || (!early && c->second->is_autorun () && !c->second->is_autorun_early ())) && ! c->second->was_autorun ()) {
      return true;
    }
  }

  return false;
}

bool MacroCollection::has_autorun () const
{
  return has_autorun_for (*this, false);
}

bool MacroCollection::has_autorun_early () const
{
  return has_autorun_for (*this, true);
}

static int collect_priority (lym::MacroCollection &collection, bool early, int from_prio)
{
  int p = -1;

  for (lym::MacroCollection::child_iterator c = collection.begin_children (); c != collection.end_children (); ++c) {
    int pp = collect_priority (*c->second, early, from_prio);
    if (pp >= from_prio && (p < 0 || pp < p)) {
      p = pp;
    }
  }

  for (lym::MacroCollection::iterator c = collection.begin (); c != collection.end (); ++c) {
    if (c->second->can_run () && ((early && c->second->is_autorun_early ()) || (!early && c->second->is_autorun () && !c->second->is_autorun_early ()))) {
      int pp = c->second->priority ();
      if (pp >= from_prio && (p < 0 || pp < p)) {
        p = pp;
      }
    }
  }

  return p;
}

static void autorun_for_prio (lym::MacroCollection &collection, bool early, int prio)
{
  for (lym::MacroCollection::child_iterator c = collection.begin_children (); c != collection.end_children (); ++c) {
    autorun_for_prio (*c->second, early, prio);
  }

  for (lym::MacroCollection::iterator c = collection.begin (); c != collection.end (); ++c) {

    if (! c->second->was_autorun () && c->second->priority () == prio && c->second->can_run () && ((early && c->second->is_autorun_early ()) || (!early && c->second->is_autorun () && !c->second->is_autorun_early ()))) {

      BEGIN_PROTECTED_SILENT
        c->second->run ();
        c->second->set_was_autorun (true);
        c->second->install_doc ();
      END_PROTECTED_SILENT

    }

  }
}

static void autorun_for (lym::MacroCollection &collection, bool early)
{
  int prio = 0;
  while (true) {
    int p = collect_priority (collection, early, prio);
    if (p < prio) {
      break;
    }
    autorun_for_prio (collection, early, p);
    prio = p + 1;
  }
}

void MacroCollection::autorun ()
{
  autorun_for (*this, false);
}

void MacroCollection::autorun_early ()
{
  autorun_for (*this, true);
}

void MacroCollection::dump (int l)
{
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf ("----\n");
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf ("Collection: %s\n", name ().c_str ());
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf ("Collection-path: %s\n", path ().c_str ());
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf ("Collection-description: %s\n", description ().c_str ());
  for (int i = 0; i < l; ++i) { printf ("  "); }
  printf("Collection-readonly: %d\n", is_readonly ());
  printf ("\n");

  for (iterator m = begin (); m != end (); ++m) {
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("Name: %s%s\n", m->second->name ().c_str (), m->second->is_modified() ? "*" : "");
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Path: %s\n", m->second->path ().c_str ());
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Readonly: %d\n", m->second->is_readonly ());
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Autorun: %d\n", m->second->is_autorun ());
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Autorun-early: %d\n", m->second->is_autorun_early ());
    for (int i = 0; i < l; ++i) { printf ("  "); }
    printf("  Description: %s\n", m->second->description ().c_str ());
  }

  for (child_iterator m = begin_children (); m != end_children (); ++m) {
    m->second->dump (l + 1);
  }
}

}

