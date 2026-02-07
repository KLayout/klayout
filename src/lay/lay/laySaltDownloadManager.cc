
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

#include "laySaltDownloadManager.h"
#include "laySaltManagerDialog.h"
#include "laySalt.h"
#include "tlProgress.h"
#include "tlFileUtils.h"
#include "tlWebDAV.h"
#include "tlLog.h"
#include "tlEnv.h"

#include <memory>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QVariant>

namespace lay
{

// ----------------------------------------------------------------------------------

ConfirmationDialog::ConfirmationDialog (QWidget *parent)
  : QDialog (parent), m_confirmed (false), m_cancelled (false), m_aborted (false), m_file (50000, true)
{
  Ui::SaltManagerInstallConfirmationDialog::setupUi (this);

  connect (ok_button, SIGNAL (clicked ()), this, SLOT (confirm_pressed ()));
  connect (cancel_button, SIGNAL (clicked ()), this, SLOT (cancel_pressed ()));
  connect (close_button, SIGNAL (clicked ()), this, SLOT (close_pressed ()));
  connect (abort_button, SIGNAL (clicked ()), this, SLOT (abort_pressed ()));

  log_panel->hide ();
  attn_frame->hide ();
  abort_button->hide ();
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
    item->setData (column, Qt::ForegroundRole, QVariant (QBrush (update ? QColor (Qt::blue) : QColor (Qt::black))));
  }
}

void
ConfirmationDialog::separator ()
{
  m_file.separator ();
}

void
ConfirmationDialog::mark_fetching (const std::string &name)
{
  std::map<std::string, QTreeWidgetItem *>::const_iterator i = m_items_by_name.find (name);
  if (i != m_items_by_name.end ()) {
    list->scrollToItem (i->second);
    for (int c = 0; c < list->columnCount (); ++c) {
      i->second->setData (c, Qt::BackgroundRole, QColor (224, 244, 244));
      i->second->setData (c, Qt::ForegroundRole, QColor (Qt::blue));
    }
    i->second->setData (1, Qt::DisplayRole, tr ("FETCHING"));
  }
}

void
ConfirmationDialog::mark_error (const std::string &name)
{
  set_icon_for_name (name, QIcon (QString::fromUtf8 (":/error_16px.png")));

  std::map<std::string, QTreeWidgetItem *>::const_iterator i = m_items_by_name.find (name);
  if (i != m_items_by_name.end ()) {
    list->scrollToItem (i->second);
    for (int c = 0; c < list->columnCount (); ++c) {
      i->second->setData (c, Qt::BackgroundRole, QColor (255, 224, 244));
      i->second->setData (c, Qt::ForegroundRole, QColor (Qt::black));
    }
    i->second->setData (1, Qt::DisplayRole, tr ("ERROR"));
  }
}

void
ConfirmationDialog::mark_success (const std::string &name)
{
  set_icon_for_name (name, QIcon (QString::fromUtf8 (":/marked_16px.png")));

  std::map<std::string, QTreeWidgetItem *>::const_iterator i = m_items_by_name.find (name);
  if (i != m_items_by_name.end ()) {
    list->scrollToItem (i->second);
    for (int c = 0; c < list->columnCount (); ++c) {
      i->second->setData (c, Qt::BackgroundRole, QColor (160, 255, 160));
      i->second->setData (c, Qt::ForegroundRole, QColor (Qt::black));
    }
    i->second->setData (1, Qt::DisplayRole, tr ("INSTALLED"));
  }
}

void
ConfirmationDialog::set_progress (const std::string &name, double progress)
{
  std::map<std::string, QTreeWidgetItem *>::const_iterator i = m_items_by_name.find (name);
  if (i != m_items_by_name.end ()) {
    i->second->setData (1, Qt::DisplayRole, tl::to_qstring (tl::sprintf ("%.1f%%", progress)));
  }
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
  close_button->hide ();
  abort_button->show ();
}

void
ConfirmationDialog::finish ()
{
  close_button->show ();
  abort_button->hide ();
}

// ----------------------------------------------------------------------------------

SaltDownloadManager::SaltDownloadManager ()
{
  m_always_download_package_information = false;
}

void
SaltDownloadManager::register_download (const std::string &name, const std::string &token, const std::string &url, const std::string &version)
{
  m_registry.push_back (Descriptor (name, token, url, version));
}

void
SaltDownloadManager::compute_dependencies (const lay::Salt &salt, const lay::Salt &salt_mine)
{
  compute_list (salt, salt_mine, true);
}

void
SaltDownloadManager::compute_packages (const lay::Salt &salt, const lay::Salt &salt_mine)
{
  compute_list (salt, salt_mine, false);
}

void
SaltDownloadManager::compute_list (const lay::Salt &salt, const lay::Salt &salt_mine, bool with_dep)
{
  tl::AbsoluteProgress progress (tl::to_string (QObject::tr ("Computing package dependencies ..")));

  //  add further entries as derived from the dependencies

  while (needs_iteration ()) {

    fetch_missing (salt, salt_mine, progress);

    if (! with_dep) {
      break;
    }

    std::map<std::string, size_t> reg_by_name;
    for (std::vector<Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
      reg_by_name.insert (std::make_pair (p->name, p - m_registry.begin ()));
    }

    size_t n = m_registry.size ();
    for (size_t i = 0; i < n; ++i) {

      Descriptor p = m_registry [i];

      for (std::vector<SaltGrainDependency>::const_iterator d = p.grain.dependencies ().begin (); d != p.grain.dependencies ().end (); ++d) {

        std::map<std::string, size_t>::iterator r = reg_by_name.find (d->name);
        if (r != reg_by_name.end ()) {

          //  Dependency is already scheduled for installation - check if we need a newer package

          Descriptor &pd = m_registry [r->second];

          if (SaltGrain::compare_versions (pd.version, d->version) < 0) {

            //  Grain is present, but too old -> update version and reload in the next iteration
            if (tl::verbosity() >= 20) {
              tl::log << "Upgrading installation request as required by package " << p.name << ": " << d->name << " (" << d->version << ") with URL " << d->url;
            }

            pd.downloaded = false;
            pd.version = d->version;
            pd.url = d->url;

          }

        } else {

          const SaltGrain *g = salt.grain_by_name (d->name);
          if (g) {

            //  Grain is installed already, but too old -> register for update
            if (SaltGrain::compare_versions (g->version (), d->version) < 0) {

              if (tl::verbosity() >= 20) {
                tl::log << "Considering for update as dependency: " << d->name << " (" << d->version << ") with URL " << d->url;
              }
              m_registry.push_back (Descriptor (d->name, std::string (), d->url, d->version));

            } else {
              if (tl::verbosity() >= 20) {
                tl::log << "Dependency already satisfied: " << d->name << "(" << d->version << ")";
              }
            }

          } else {

            if (tl::verbosity() >= 20) {
              tl::log << "Considering for download as dependency: " << d->name << " (" << d->version << ") with URL " << d->url;
            }
            m_registry.push_back (Descriptor (d->name, std::string (), d->url, d->version));

          }

        }

      }

    }

  }
}

bool
SaltDownloadManager::needs_iteration ()
{
  for (std::vector<Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
    if (! p->downloaded) {
      return true;
    }
  }
  return false;
}

void
SaltDownloadManager::fetch_missing (const lay::Salt &salt, const lay::Salt &salt_mine, tl::AbsoluteProgress &progress)
{
  std::vector<Descriptor> registry;

  //  drop entries with same name but lower version

  registry.swap (m_registry);
  std::sort (registry.begin (), registry.end ());

  for (std::vector<Descriptor>::const_iterator p = registry.begin (); p != registry.end (); ++p) {
    if (p + 1 != registry.end ()) {
      if (p->name != p[1].name) {
        m_registry.push_back (*p);
      }
    } else {
      m_registry.push_back (*p);
    }
  }

  //  download the items that need to be downloaded

  for (std::vector<Descriptor>::iterator p = m_registry.begin (); p != m_registry.end (); ++p) {

    if (! p->downloaded) {

      ++progress;

      //  Add URL and token from the package index
      //
      //  In order to do so, we try to use the information from that package index as far as possible.
      //  Downloading a package definition from the original package URL may be expensive in case of
      //  large GIT repositories.
      //
      //  Downloading is required if:
      //  - A package download is requested without a name (package can't be looked up in the package index)
      //  - Or a name is given, but not found in the package index
      //
      //  Downloading can be bypassed if the package index (salt mine) specifies "sparse=false".
      //  In that case, the package index will have all information about the package.

      if (! p->name.empty ()) {

        const lay::SaltGrain *g = salt_mine.grain_by_name (p->name);
        if (! g) {
          if (p->url.empty ()) {
            throw tl::Exception (tl::to_string (tr ("Package '%s' not found in index - cannot resolve download URL")), p->name);
          }
        } else {
          if (p->url.empty ()) {
            if (tl::verbosity() >= 20) {
              tl::log << tr ("Resolved package URL for package") << " '" << p->name << "': " << g->url ();
            }
            p->url = g->url ();
          }
          p->token = g->token ();
          p->grain = *g;
          p->downloaded = true;
        }

      }

      if (! p->downloaded && (m_always_download_package_information || salt_mine.download_package_information ())) {

        //  If requested, download package information to complete information from index or dependencies
        if (tl::verbosity() >= 10) {
          tl::log << tl::sprintf (tl::to_string (tr ("Reading package description for package '%s' from: '%s")), p->name, p->url);
        }
        try {
          p->grain = SaltGrain::from_url (p->url);
          p->downloaded = true;
        } catch (tl::Exception &ex) {
          throw tl::Exception (tl::to_string (tr ("Error fetching spec file for package from '%s': %s")), p->url, ex.msg ());
        }

      }

      if (! p->downloaded) {

        if (p->name.empty ()) {
          throw tl::Exception (tl::to_string (tr ("No name given for package from '%s' (from dependencies or command line installation request)")), p->url);
        }

        if (tl::verbosity() >= 10) {
          tl::warn << tl::sprintf (tl::to_string (tr ("Package '%s' not downloaded from: %s. Dependencies may not be resolved.")), p->name, p->url);
        }

      } else {

        if (p->version.empty ()) {
          p->version = p->grain.version ();
        }
        if (p->name.empty ()) {
          p->name = p->grain.name ();
        }

        if (SaltGrain::compare_versions (p->grain.version (), p->version) < 0) {
          throw tl::Exception (tl::to_string (tr ("Package '%s': package in repository is too old (%s) to satisfy requirements (%s)")), p->name, p->grain.version (), p->version);
        }

      }

      p->downloaded = true;

    }

  }

  //  remove those registered entries which don't need to be updated (we do this after download since now the
  //  names should be known when only the URL is given)

  registry.clear ();
  registry.swap (m_registry);

  for (std::vector<Descriptor>::const_iterator p = registry.begin (); p != registry.end (); ++p) {
    const SaltGrain *g = salt.grain_by_name (p->name);
    if (g) {
      if (SaltGrain::compare_versions (p->version, g->version ()) <= 0 && p->url == g->url ()) {
        if (tl::verbosity() >= 20) {
          tl::log << "Package already present with sufficient version - not installed again: " << p->name << " (" << p->version << ")";
        }
      } else {
        if (tl::verbosity() >= 20) {
          tl::log << "Considering package for upgrade or URL switch: " << p->name << ", from " << g->url () << "(" << g->version () << ") to " << p->url << "(" << p->version << ")";
        }
        m_registry.push_back (*p);
      }
    } else {
      m_registry.push_back (*p);
    }
  }
}

lay::ConfirmationDialog *
SaltDownloadManager::make_confirmation_dialog (QWidget *parent, const lay::Salt &salt)
{
  lay::ConfirmationDialog *dialog = new lay::ConfirmationDialog (parent);

  std::sort (m_registry.begin (), m_registry.end ());

  //  First the packages to update
  for (std::vector<Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
    const lay::SaltGrain *g = salt.grain_by_name (p->name);
    if (g) {
      //  \342\206\222 is UTF-8 "right arrow"
      dialog->add_info (p->name, true, g->version () + " \342\206\222 " + p->version, p->url);
    }
  }

  //  Then the packages to install
  for (std::vector<Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {
    const lay::SaltGrain *g = salt.grain_by_name (p->name);
    if (!g) {
      dialog->add_info (p->name, false, p->version, p->url);
    }
  }

  return dialog;
}

namespace
{
  class DownloadProgressAdaptor
    : public tl::ProgressAdaptor, public tl::InputHttpStreamCallback
  {
  public:
    DownloadProgressAdaptor (lay::ConfirmationDialog *dialog, const std::string &name)
      : mp_dialog (dialog), m_name (name), m_is_aborted (false)
    {
      mp_dialog->mark_fetching (m_name);
    }

    virtual void yield (tl::Progress * /*progress*/)
    {
      QCoreApplication::processEvents (QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 100);
      if (mp_dialog->is_aborted ()) {
        m_is_aborted = true;
        throw tl::CancelException ();
      }
    }

    virtual void wait_for_input ()
    {
      yield (0);
    }

    virtual void trigger (tl::Progress *progress)
    {
      mp_dialog->set_progress (m_name, progress->value ());
    }

    void error ()
    {
      mp_dialog->mark_error (m_name);
    }

    void success ()
    {
      mp_dialog->mark_success (m_name);
    }

    bool is_aborted () const
    {
      return m_is_aborted;
    }

  private:
    lay::ConfirmationDialog *mp_dialog;
    std::string m_name;
    bool m_is_aborted;
  };
}

bool
SaltDownloadManager::execute (lay::SaltManagerDialog *parent, lay::Salt &salt)
{
  bool result = true;

  if (parent) {

    //  Stop with a warning if there is nothing to do
    if (m_registry.empty()) {
      QMessageBox::warning (parent, tr ("Nothing to do"), tr ("No packages need update or are marked for installation"));
      return true;
    }

    std::unique_ptr<lay::ConfirmationDialog> dialog (make_confirmation_dialog (parent, salt));

    dialog->setModal (true);
    dialog->show ();

    //  TODO: should not waste CPU time in processEvents loop.
    while (! dialog->is_confirmed ()) {
      QCoreApplication::processEvents (QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 100);
      if (dialog->is_cancelled () || ! dialog->isVisible ()) {
        return false;
      }
    }

    dialog->start ();

    //  Stop other events to interfere with the download, specifically not macro controller updates
    tl::NoDeferredMethods silent_section;

    std::sort (m_registry.begin (), m_registry.end ());

    for (std::vector<Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {

      lay::SaltGrain target;
      target.set_name (p->name);
      lay::SaltGrain *g = salt.grain_by_name (p->name);
      if (g) {
        target.set_path (g->path ());
      }

      int status = 1;

      DownloadProgressAdaptor pa (dialog.get (), p->name);
      if (! salt.create_grain (p->grain, target, 0.0 /*infinite timeout*/, &pa)) {
        pa.error ();
        result = false;
        status = 0;
      } else {
        pa.success ();
      }

      if (pa.is_aborted ()) {
        break;
      }

      try {
        //  try to give feedback about successful installations
        if (! p->token.empty ()) {
          std::string fb_url = parent->salt_mine_url () + "?token=" + p->token + "&status=" + tl::to_string (status);
          if (fb_url.find ("http:") == 0 || fb_url.find ("https:") == 0) {
            tl::InputStream fb (fb_url);
            fb.read_all ();
          }
        }
      } catch (tl::Exception &ex) {
        tl::error << ex.msg ();
      }

      dialog->separator ();

    }

    dialog->finish ();

    //  Show the dialog until it's closed
    dialog->exec ();

  } else {

    for (std::vector<Descriptor>::const_iterator p = m_registry.begin (); p != m_registry.end (); ++p) {

      lay::SaltGrain target;
      target.set_name (p->name);
      lay::SaltGrain *g = salt.grain_by_name (p->name);
      if (g) {
        target.set_path (g->path ());
      }

      if (! salt.create_grain (p->grain, target, 60.0 /*timeout for offline installation*/)) {
        tl::error << tl::to_string (QObject::tr ("Installation failed for package %1").arg (tl::to_qstring (target.name ())));
        result = false;
      } else {
        tl::log << tl::to_string (QObject::tr ("Package %1 installed successfully").arg (tl::to_qstring (target.name ())));
      }

    }

  }

  return result;
}

}
