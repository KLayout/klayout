
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "laySaltDownloadManager.h"
#include "laySalt.h"
#include "tlFileUtils.h"
#include "tlWebDAV.h"

#include "ui_SaltManagerInstallConfirmationDialog.h"

#include <QTreeWidgetItem>

namespace lay
{

// ----------------------------------------------------------------------------------

class ConfirmationDialog
  : public QDialog, private Ui::SaltManagerInstallConfirmationDialog
{
public:
  ConfirmationDialog (QWidget *parent)
    : QDialog (parent)
  {
    Ui::SaltManagerInstallConfirmationDialog::setupUi (this);
  }

  void add_info (const std::string &name, bool update, const std::string &version, const std::string &url)
  {
    QTreeWidgetItem *item = new QTreeWidgetItem (list);
    item->setText (0, tl::to_qstring (name));
    item->setText (1, update ? tr ("UPDATE") : tr ("INSTALL"));
    item->setText (2, tl::to_qstring (version));
    item->setText (3, tl::to_qstring (url));
  }
};

// ----------------------------------------------------------------------------------

SaltDownloadManager::SaltDownloadManager ()
{
  //  .. nothing yet ..
}

void
SaltDownloadManager::register_download (const std::string &name, const std::string &url, const std::string &version)
{
  m_registry.insert (std::make_pair (name, Descriptor (url, version)));
}

void
SaltDownloadManager::compute_dependencies (const lay::Salt &salt, const lay::Salt &salt_mine)
{
  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Computing package dependencies ..")));

  while (needs_iteration ()) {

    fetch_missing (salt_mine, progress);

    std::map<std::string, Descriptor> registry = m_registry;
    for (std::map<std::string, Descriptor>::const_iterator p = registry.begin (); p != registry.end (); ++p) {

      for (std::vector<SaltGrain::Dependency>::const_iterator d = p->second.grain.dependencies ().begin (); d != p->second.grain.dependencies ().end (); ++d) {

        std::map<std::string, Descriptor>::iterator r = m_registry.find (d->name);
        if (r != m_registry.end ()) {

          if (SaltGrain::compare_versions (r->second.version, d->version) < 0) {

            //  Grain is present, but too old -> update version and reload in the next iteration
            r->second.downloaded = false;
            r->second.version = d->version;
            r->second.url = d->url;
            r->second.downloaded = false;

          }

        } else {

          const SaltGrain *g = salt.grain_by_name (d->name);
          if (g) {

            //  Grain is installed already, but too old -> register for update
            if (SaltGrain::compare_versions (g->version (), d->version) < 0) {
              register_download (d->name, d->url, d->version);
            }

          } else {
            register_download (d->name, d->url, d->version);
          }

        }

      }

    }

  }
}

bool
SaltDownloadManager::needs_iteration ()
{
  for (std::map<std::string, Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
    if (! p->second.downloaded) {
      return true;
    }
  }
  return false;
}

void
SaltDownloadManager::fetch_missing (const lay::Salt &salt_mine, tl::AbsoluteProgress &progress)
{
  for (std::map<std::string, Descriptor>::iterator p = m_registry.begin (); p != m_registry.end (); ++p) {

    if (! p->second.downloaded) {

      ++progress;

      //  If no URL is given, utilize the salt mine to fetch it
      if (p->second.url.empty ()) {
        const lay::SaltGrain *g = salt_mine.grain_by_name (p->first);
        if (SaltGrain::compare_versions (g->version (), p->second.version) < 0) {
          throw tl::Exception (tl::to_string (QObject::tr ("Package '%1': package in repository is too old (%2) to satisfy requirements (%3)").arg (tl::to_qstring (p->first)).arg (tl::to_qstring (g->version ())).arg (tl::to_qstring (p->second.version))));
        }
        p->second.version = g->version ();
        p->second.url = g->url ();
      }

      p->second.grain = SaltGrain::from_url (p->second.url);
      p->second.downloaded = true;

    }

  }
}

bool
SaltDownloadManager::show_confirmation_dialog (QWidget *parent, const lay::Salt &salt)
{
  lay::ConfirmationDialog dialog (parent);

  //  First the packages to update
  for (std::map<std::string, Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
    const lay::SaltGrain *g = salt.grain_by_name (p->first);
    if (g) {
      dialog.add_info (p->first, true, g->version () + "->" + p->second.version, p->second.url);
    }
  }

  //  Then the packages to install
  for (std::map<std::string, Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
    const lay::SaltGrain *g = salt.grain_by_name (p->first);
    if (!g) {
      dialog.add_info (p->first, false, p->second.version, p->second.url);
    }
  }

  return dialog.exec ();
}

bool
SaltDownloadManager::execute (lay::Salt &salt)
{
  bool result = true;

  tl::RelativeProgress progress (tl::to_string (QObject::tr ("Downloading packages")), m_registry.size (), 1);

  for (std::map<std::string, Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {

    lay::SaltGrain target;
    target.set_name (p->first);
    lay::SaltGrain *g = salt.grain_by_name (p->first);
    if (g) {
      target.set_path (g->path ());
    }

    if (! salt.create_grain (p->second.grain, target)) {
      result = false;
    }

    ++progress;

  }

  return result;
}

}
