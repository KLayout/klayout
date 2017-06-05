
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

#include <memory>
#include <QTreeWidgetItem>
#include <QMessageBox>

namespace lay
{

// ----------------------------------------------------------------------------------

ConfirmationDialog::ConfirmationDialog (QWidget *parent)
  : QDialog (parent), m_confirmed (false), m_cancelled (false), m_closed (false), m_file (50000, true)
{
  Ui::SaltManagerInstallConfirmationDialog::setupUi (this);

  connect (ok_button, SIGNAL (clicked ()), this, SLOT (confirm_pressed ()));
  connect (cancel_button, SIGNAL (clicked ()), this, SLOT (cancel_pressed ()));
  connect (close_button, SIGNAL (clicked ()), this, SLOT (close_pressed ()));

  log_panel->hide ();
  attn_frame->hide ();
  log_view->setModel (&m_file);

  connect (&m_file, SIGNAL (layoutChanged ()), log_view, SLOT (scrollToBottom ()));
  connect (&m_file, SIGNAL (attention_changed (bool)), attn_frame, SLOT (setVisible (bool)));
}

void
ConfirmationDialog::add_info (const std::string &name, bool update, const std::string &version, const std::string &url)
{
  QTreeWidgetItem *item = new QTreeWidgetItem (list);
  m_items_by_name.insert (std::make_pair (name, item));

  item->setFlags (item->flags () & ~Qt::ItemIsSelectable);

  item->setText (0, tl::to_qstring (name));
  item->setText (1, update ? tr ("UPDATE") : tr ("INSTALL"));
  item->setText (2, tl::to_qstring (version));
  item->setText (3, tl::to_qstring (url));

  for (int column = 0; column < list->colorCount (); ++column) {
    item->setData (column, Qt::ForegroundRole, update ? Qt::blue : Qt::black);
  }
}

void
ConfirmationDialog::separator ()
{
  m_file.separator ();
}

void
ConfirmationDialog::mark_error (const std::string &name)
{
  set_icon_for_name (name, QIcon (QString::fromUtf8 (":/error_16.png")));
}

void
ConfirmationDialog::mark_success (const std::string &name)
{
  set_icon_for_name (name, QIcon (QString::fromUtf8 (":/marked_16.png")));
}

void
ConfirmationDialog::set_icon_for_name (const std::string &name, const QIcon &icon)
{
  std::map<std::string, QTreeWidgetItem *>::const_iterator i = m_items_by_name.find (name);
  if (i != m_items_by_name.end ()) {
    i->second->setData (0, Qt::DecorationRole, icon);
  }
}

void
ConfirmationDialog::start ()
{
  confirm_panel->hide ();
  log_panel->show ();
  close_button->setEnabled (false);
}

void
ConfirmationDialog::finish ()
{
  close_button->setEnabled (true);
}

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

  std::map<std::string, Descriptor> registry;

  //  remove those registered entries which don't need to be updated

  registry = m_registry;
  for (std::map<std::string, Descriptor>::const_iterator p = registry.begin (); p != registry.end (); ++p) {
    const SaltGrain *g = salt.grain_by_name (p->first);
    if (g && SaltGrain::compare_versions (p->second.version, g->version ()) == 0 && p->second.url == g->url ()) {
      m_registry.erase (p->first);
    }
  }

  //  add further entries as derived from the dependencies

  while (needs_iteration ()) {

    fetch_missing (salt_mine, progress);

    registry = m_registry;
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

      try {
        p->second.grain = SaltGrain::from_url (p->second.url);
      } catch (tl::Exception &ex) {
        throw tl::Exception (tl::to_string (QObject::tr ("Error fetching spec file for package '%1': %2").arg (tl::to_qstring (p->first)).arg (tl::to_qstring (ex.msg ()))));
      }

      p->second.downloaded = true;

    }

  }
}

lay::ConfirmationDialog *
SaltDownloadManager::make_confirmation_dialog (QWidget *parent, const lay::Salt &salt)
{
  lay::ConfirmationDialog *dialog = new lay::ConfirmationDialog (parent);

  //  First the packages to update
  for (std::map<std::string, Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
    const lay::SaltGrain *g = salt.grain_by_name (p->first);
    if (g) {
      //  \342\206\222 is UTF-8 "right arrow"
      dialog->add_info (p->first, true, g->version () + " \342\206\222 " + p->second.version, p->second.url);
    }
  }

  //  Then the packages to install
  for (std::map<std::string, Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
    const lay::SaltGrain *g = salt.grain_by_name (p->first);
    if (!g) {
      dialog->add_info (p->first, false, p->second.version, p->second.url);
    }
  }

  return dialog;
}

bool
SaltDownloadManager::execute (QWidget *parent, lay::Salt &salt)
{
  //  Stop with a warning if there is nothing to do
  if (m_registry.empty()) {
    QMessageBox::warning (parent, tr ("Nothing to do"), tr ("No packages need update or are marked for installation"));
    return true;
  }

  std::auto_ptr<lay::ConfirmationDialog> dialog (make_confirmation_dialog (parent, salt));

  dialog->setModal (true);
  dialog->show ();

  while (! dialog->is_confirmed ()) {
    QCoreApplication::processEvents (QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 100);
    if (dialog->is_cancelled () || ! dialog->isVisible ()) {
      return false;
    }
  }

  dialog->start ();

  bool result = true;

  for (std::map<std::string, Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {

    lay::SaltGrain target;
    target.set_name (p->first);
    lay::SaltGrain *g = salt.grain_by_name (p->first);
    if (g) {
      target.set_path (g->path ());
    }

    if (! salt.create_grain (p->second.grain, target)) {
      dialog->mark_error (p->first);
      result = false;
    } else {
      dialog->mark_success (p->first);
    }

    dialog->separator ();

  }

  dialog->finish ();

  while (! dialog->is_closed () && dialog->isVisible ()) {
    QCoreApplication::processEvents (QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 100);
  }

  return result;
}

}
