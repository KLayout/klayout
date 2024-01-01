
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "laySalt.h"
#include "laySaltParsedURL.h"

#include "tlString.h"
#include "tlFileUtils.h"
#include "tlLog.h"
#include "tlInternational.h"
#include "tlWebDAV.h"
#include "tlEnv.h"
#if defined(HAVE_GIT2)
#  include "tlGit.h"
#endif

#include "lymMacro.h"

#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QResource>

namespace lay
{

Salt::Salt ()
{
  //  .. nothing yet ..
}

Salt::Salt (const Salt &other)
  : QObject ()
{
  operator= (other);
}

Salt &Salt::operator= (const Salt &other)
{
  if (this != &other) {
    emit collections_about_to_change ();
    m_root = other.m_root;
    invalidate ();
  }
  return *this;
}

SaltGrains &
Salt::root ()
{
  return m_root;
}

bool
Salt::download_package_information () const
{
  //  $KLAYOUT_ALWAYS_DOWNLOAD_PACKAGE_INFO
  return tl::app_flag ("always-download-package-info") || m_root.sparse ();
}

Salt::flat_iterator
Salt::begin_flat ()
{
  validate ();
  return mp_flat_grains.begin ();
}

Salt::flat_iterator
Salt::end_flat ()
{
  validate ();
  return mp_flat_grains.end ();
}

SaltGrain *
Salt::grain_by_name (const std::string &name)
{
  validate ();
  std::map<std::string, SaltGrain *>::const_iterator g = m_grains_by_name.find (name);
  if (g != m_grains_by_name.end ()) {
    return g->second;
  } else {
    return 0;
  }
}

void
Salt::add_location (const std::string &path)
{
  tl_assert (! path.empty ());

  if (path[0] != ':') {
    //  do nothing if the collection is already there
    QFileInfo fi (tl::to_qstring (path));
    for (lay::SaltGrains::collection_iterator g = m_root.begin_collections (); g != m_root.end_collections (); ++g) {
      if (QFileInfo (tl::to_qstring (g->path ())) == fi) {
        return;
      }
    }
  }

  lay::SaltGrains gg = lay::SaltGrains::from_path (path);
  emit collections_about_to_change ();
  m_root.add_collection (gg);
  invalidate ();
}

void
Salt::remove_location (const std::string &path)
{
  QFileInfo fi (tl::to_qstring (path));
  for (lay::SaltGrains::collection_iterator g = m_root.begin_collections (); g != m_root.end_collections (); ++g) {
    if (QFileInfo (tl::to_qstring (g->path ())) == fi) {
      emit collections_about_to_change ();
      m_root.remove_collection (g, false);
      invalidate ();
      return;
    }
  }
}

void
Salt::refresh ()
{
  lay::SaltGrains new_root;
  for (lay::SaltGrains::collection_iterator g = m_root.begin_collections (); g != m_root.end_collections (); ++g) {
    new_root.add_collection (lay::SaltGrains::from_path (g->path ()));
  }
  if (new_root != m_root) {
    emit collections_about_to_change ();
    m_root = new_root;
    invalidate ();
  }
}

void
Salt::add_collection_to_flat (SaltGrains &gg)
{
  for (lay::SaltGrains::grain_iterator g = gg.begin_grains (); g != gg.end_grains (); ++g) {
    //  TODO: get rid of the const cast - would require a non-const grain iterator
    mp_flat_grains.push_back (const_cast <SaltGrain *> (g.operator-> ()));
  }
  for (lay::SaltGrains::collection_iterator g = gg.begin_collections (); g != gg.end_collections (); ++g) {
    //  TODO: get rid of the const cast - would require a non-const grain collection iterator
    add_collection_to_flat (const_cast <SaltGrains &> (*g));
  }
}

namespace {

struct NameAndTopoIndexCompare
{
  NameAndTopoIndexCompare (const std::map<std::string, int> &topo_index)
    : mp_topo_index (&topo_index)
  {
    //  .. nothing yet ..
  }

  bool operator () (lay::SaltGrain *a, lay::SaltGrain *b) const
  {
    std::map<std::string, int>::const_iterator ti_a = mp_topo_index->find (a->name ());
    std::map<std::string, int>::const_iterator ti_b = mp_topo_index->find (b->name ());

    //  Reverse sorting by topological index as highest priority
    if (ti_a != mp_topo_index->end () && ti_b != mp_topo_index->end ()) {
      if (ti_a->second != ti_b->second) {
        return ti_a->second > ti_b->second;
      }
    }

    //  The hidden after non-hidden
    if (a->is_hidden () != b->is_hidden ()) {
      return a->is_hidden () < b->is_hidden ();
    }

    //  Finally the name (TODO: UTF-8 support?)
    return a->name () < b->name ();
  }

private:
  const std::map<std::string, int> *mp_topo_index;
};

}

void
Salt::validate ()
{
  if (mp_flat_grains.empty ()) {

    add_collection_to_flat (m_root);

    m_grains_by_name.clear ();
    for (std::vector<SaltGrain *>::const_iterator i = mp_flat_grains.begin (); i != mp_flat_grains.end (); ++i) {
      m_grains_by_name.insert (std::make_pair ((*i)->name (), *i));
    }

    //  Compute a set of topological indexes. Packages which serve dependencies of other packages have a higher
    //  topological index. Later we sort the packages by descending topo index to ensure the packages which are
    //  input to others come first.

    std::map<std::string, int> topological_index;
    for (std::map<std::string, SaltGrain *>::const_iterator g = m_grains_by_name.begin (); g != m_grains_by_name.end (); ++g) {
      topological_index.insert (std::make_pair (g->first, 0));
    }

    //  NOTE: we allow max. 10 levels of dependencies. That should be sufficient. Limiting the levels of dependencies prevents
    //  infinite recursion due to faulty recursive dependencies.
    for (int n = 0; n < 10; ++n) {

      bool any_updated = false;

      for (std::map<std::string, SaltGrain *>::const_iterator g = m_grains_by_name.begin (); g != m_grains_by_name.end (); ++g) {
        int index = topological_index [g->first];
        for (std::vector<SaltGrainDependency>::const_iterator d = g->second->dependencies ().begin (); d != g->second->dependencies ().end (); ++d) {
          std::map<std::string, int>::iterator ti = topological_index.find (d->name);
          if (ti != topological_index.end () && ti->second < index + 1) {
            ti->second = index + 1;
            any_updated = true;
          }
        }
      }

      if (! any_updated) {
        break;
      }

    }

    //  NOTE: we intentionally sort after the name list has been built - this way
    //  the first entry will win in the name to grain map.
    std::sort (mp_flat_grains.begin (), mp_flat_grains.end (), NameAndTopoIndexCompare (topological_index));

  }
}

void
Salt::invalidate ()
{
  mp_flat_grains.clear ();
  emit collections_changed ();
}

void
Salt::consolidate ()
{
  m_root.consolidate ();
  invalidate ();
}

static
bool remove_from_collection (SaltGrains &collection, const std::string &name)
{
  bool res = false;

  for (SaltGrains::grain_iterator g = collection.begin_grains (); g != collection.end_grains (); ++g) {
    if (g->name () == name) {
      res = collection.remove_grain (g, true);
      break;
    }
  }

  for (SaltGrains::collection_iterator gg = collection.begin_collections (); gg != collection.end_collections (); ++gg) {
    //  TODO: remove this const_cast
    if (remove_from_collection (const_cast <SaltGrains &> (*gg), name)) {
      res = true;
    }
  }

  return res;
}

bool
Salt::remove_grain (const SaltGrain &grain)
{
  emit collections_about_to_change ();

  QString name = tl::to_qstring (grain.name ());
  tl::info << QObject::tr ("Removing package '%1' ..").arg (name);

  //  Execute "_uninstall.lym" if it exists
  try {
    QFile uninstall_lym (QDir (tl::to_qstring (grain.path ())).absoluteFilePath (tl::to_qstring ("_uninstall.lym")));
    if (uninstall_lym.exists ()) {
      lym::Macro uninstall;
      uninstall.load_from (tl::to_string (uninstall_lym.fileName ()));
      uninstall.set_file_path (tl::to_string (uninstall_lym.fileName ()));
      uninstall.run ();
    }
  } catch (tl::Exception &ex) {
    //  Errors in the uninstallation script are only logged, but do not prevent uninstallation
    tl::error << ex.msg ();
  }

  bool res = remove_from_collection (m_root, grain.name ());
  if (res) {
    tl::info << QObject::tr ("Package '%1' removed.").arg (name);
  } else {
    tl::warn << QObject::tr ("Failed to remove package '%1'.").arg (name);
  }

  invalidate ();
  return res;
}

namespace
{

/**
 *  @brief A helper class required because directory traversal is not supported by QResource directly
 *  This class supports resource file trees and extraction of a tree from the latter
 */
class ResourceDir
  : public QResource
{
public:
  using QResource::isFile;

  /**
   *  @brief Constructor
   *  Creates a resource representing a resource tree.
   */
  ResourceDir (const QString &path)
    : QResource (path)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Writes the resource tree to the target directory
   *  Returns false on error - see log in this case.
   */
  bool copy_to (const QDir &target)
  {
    if (isDir ()) {

      QStringList templ_dir = children ();
      for (QStringList::const_iterator t = templ_dir.begin (); t != templ_dir.end (); ++t) {

        ResourceDir child_res (fileName () + QString::fromUtf8 ("/") + *t);
        if (child_res.isFile ()) {

          QFile file (target.absoluteFilePath (*t));
          if (! file.open (QIODevice::WriteOnly)) {
            tl::error << QObject::tr ("Unable to open target file for writing: %1").arg (target.absoluteFilePath (*t));
            return false;
          }

          QByteArray data;
#if QT_VERSION >= 0x60000
          if (child_res.compressionAlgorithm () == QResource::ZlibCompression) {
#else
          if (child_res.isCompressed ()) {
#endif
            data = qUncompress ((const unsigned char *)child_res.data (), (int)child_res.size ());
          } else {
            data = QByteArray ((const char *)child_res.data (), (int)child_res.size ());
          }

          file.write (data);

          file.close ();

        } else {

          QFileInfo child_dir (target.absoluteFilePath (*t));
          if (! child_dir.exists ()) {
            if (! target.mkdir (*t)) {
              tl::error << QObject::tr ("Unable to create target directory: %1").arg (child_dir.path ());
              return false;
            }
          } else if (! child_dir.isDir ()) {
            tl::error << QObject::tr ("Unable to create target directory (is a file already): %1").arg (child_dir.path ());
            return false;
          }

          if (! child_res.copy_to (QDir (target.absoluteFilePath (*t)))) {
            return false;
          }

        }

      }

    }

    return true;
  }
};

}

bool
Salt::create_grain (const SaltGrain &templ, SaltGrain &target, double timeout, tl::InputHttpStreamCallback *callback)
{
  tl_assert (m_root.begin_collections () != m_root.end_collections ());

  const SaltGrains *coll = m_root.begin_collections ().operator-> ();

  if (target.name ().empty ()) {
    target.set_name (templ.name ());
  }

  if (target.path ().empty ()) {
    lay::SaltGrain *g = grain_by_name (target.name ());
    if (g) {
      target.set_path (g->path ());
    }
  }

  std::string path = target.path ();
  if (! path.empty ()) {
    coll = 0;
    for (SaltGrains::collection_iterator gg = m_root.begin_collections (); gg != m_root.end_collections (); ++gg) {
      if (tl::is_parent_path (gg->path (), path)) {
        coll = gg.operator-> ();
        break;
      }
    }
    tl_assert (coll != 0);
  }

  tl::info << QObject::tr ("Installing package '%1' ..").arg (tl::to_qstring (target.name ()));

  QDir target_dir (tl::to_qstring (coll->path ()));

  try {

    //  change down to the desired target location and create the directory structure while doing so
    std::vector<std::string> name_parts = tl::split (target.name (), "/");
    for (std::vector<std::string>::const_iterator n = name_parts.begin (); n != name_parts.end (); ++n) {

      QFileInfo subdir (target_dir.filePath (tl::to_qstring (*n)));
      if (subdir.exists () && ! subdir.isDir ()) {
        throw tl::Exception (tl::to_string (tr ("Unable to create target directory '%1' for installing package - is already a file").arg (subdir.path ())));
      } else if (! subdir.exists ()) {
        if (! target_dir.mkpath (tl::to_qstring (*n))) {
          throw tl::Exception (tl::to_string (tr ("Unable to create target directory '%1' for installing package").arg (subdir.filePath ())));
        }
        if (! target_dir.cd (tl::to_qstring (*n))) {
          throw tl::Exception (tl::to_string (tr ("Unable to change to target directory '%1' for installing package").arg (subdir.path ())));
        }
      } else {
        if (! target_dir.cd (tl::to_qstring (*n))) {
          throw tl::Exception (tl::to_string (tr ("Unable to change to target directory '%1' for installing package").arg (subdir.path ())));
        }
      }

    }

  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    return false;
  }

  bool res = true;

  std::string target_name = target.name ();
  target = templ;
  target.set_path (tl::to_string (target_dir.absolutePath ()));
  target.set_name (target_name);

  if (! templ.path ().empty ()) {

    if (templ.path ()[0] != ':') {

      //  if the template represents an actual folder, use the files from there
      tl::info << QObject::tr ("Copying package from '%1' to '%2' ..").arg (tl::to_qstring (templ.path ())).arg (tl::to_qstring (target.path ()));
      res = tl::cp_dir_recursive (templ.path (), target.path ());

    } else {

      //  if the template represents a resource path, use the files from there
      tl::info << QObject::tr ("Installing package from resource '%1' to '%2' ..").arg (tl::to_qstring (templ.path ())).arg (tl::to_qstring (target.path ()));
      res = ResourceDir (tl::to_qstring (templ.path ())).copy_to (QDir (tl::to_qstring (target.path ())));

    }

  } else if (! templ.url ().empty ()) {

    lay::SaltParsedURL purl (templ.url ());

    if (purl.url ().find ("http:") == 0 || purl.url ().find ("https:") == 0) {

      //  otherwise download from the URL using Git or SVN

      if (purl.protocol () == Git) {

#if defined(HAVE_GIT2)
        tl::info << QObject::tr ("Downloading package from '%1' to '%2' using Git protocol (ref='%3', subdir='%4') ..").arg (tl::to_qstring (purl.url ())).arg (tl::to_qstring (target.path ())).arg (tl::to_qstring (purl.branch ())).arg (tl::to_qstring (purl.subfolder ()));
        res = tl::GitObject::download (purl.url (), target.path (), purl.subfolder (), purl.branch (), timeout, callback);
#else
        throw tl::Exception (tl::to_string (QObject::tr ("Unable to install package '%1' - git protocol not compiled in").arg (tl::to_qstring (target.name ()))));
#endif

      } else if (purl.protocol () == WebDAV || purl.protocol () == DefaultProtocol) {

        tl::info << QObject::tr ("Downloading package from '%1' to '%2' using SVN/WebDAV protocol ..").arg (tl::to_qstring (purl.url ())).arg (tl::to_qstring (target.path ()));
        res = tl::WebDAVObject::download (purl.url (), target.path (), timeout, callback);

      }

    } else {

      //  or copy from a file path for "file" URL's
      std::string src = templ.url ();
      if (src.find ("file:") == 0) {
        QUrl url (tl::to_qstring (src));
        src = tl::to_string (QFileInfo (url.toLocalFile ()).absoluteFilePath ());
      }

      tl::info << QObject::tr ("Copying package from '%1' to '%2' ..").arg (tl::to_qstring (src)).arg (tl::to_qstring (target.path ()));
      res = tl::cp_dir_recursive (src, target.path ());

    }

    target.set_url (templ.url ());

  }

  if (res) {

    target.set_installed_time (QDateTime::currentDateTime ());
    target.save ();

    //  Execute "_install.lym" if it exists
    try {
      QFile install_lym (QDir (tl::to_qstring (target.path ())).absoluteFilePath (tl::to_qstring ("_install.lym")));
      if (install_lym.exists ()) {
        lym::Macro install;
        install.load_from (tl::to_string (install_lym.fileName ()));
        install.set_file_path (tl::to_string (install_lym.fileName ()));
        install.run ();
      }
    } catch (tl::Exception &ex) {
      //  Errors in the installation script are only logged, but do not prevent installation
      tl::error << ex.msg ();
    }

    tl::info << QObject::tr ("Package '%1' installed").arg (tl::to_qstring (target.name ()));

    //  NOTE: this is a bit brute force .. we could as well try to insert the new grain into the existing structure
    refresh ();

  } else {

    tl::warn << QObject::tr ("Failed to install package '%1' - removing files ..").arg (tl::to_qstring (target.name ()));
    if (! tl::rm_dir_recursive (target.path ())) {
      tl::warn << QObject::tr ("Failed to remove files").arg (tl::to_qstring (target.name ()));
    }

  }

  return res;
}

}
