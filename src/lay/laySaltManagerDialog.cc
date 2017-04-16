
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

#include "laySaltManagerDialog.h"
#include "laySaltModel.h"
#include "laySaltGrainPropertiesDialog.h"
#include "laySaltDownloadManager.h"
#include "laySalt.h"
#include "ui_SaltGrainTemplateSelectionDialog.h"
#include "tlString.h"
#include "tlExceptions.h"
#include "tlHttpStream.h"

#include <QTextDocument>
#include <QPainter>
#include <QDir>
#include <QTextStream>
#include <QBuffer>
#include <QResource>
#include <QMessageBox>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>

namespace lay
{

// --------------------------------------------------------------------------------------

/**
 *  @brief A tiny dialog to select a template and a name for the grain
 */
class SaltGrainTemplateSelectionDialog
  : public QDialog, private Ui::SaltGrainTemplateSelectionDialog
{
public:
  SaltGrainTemplateSelectionDialog (QWidget *parent, lay::Salt *salt)
    : QDialog (parent), mp_salt (salt)
  {
    Ui::SaltGrainTemplateSelectionDialog::setupUi (this);

    m_salt_templates.add_location (":/salt_templates");
    salt_view->setModel (new SaltModel (this, &m_salt_templates));
    salt_view->setItemDelegate (new SaltItemDelegate (this));
    salt_view->setCurrentIndex (salt_view->model ()->index (0, 0, QModelIndex ()));
  }

  lay::SaltGrain templ () const
  {
    SaltModel *model = dynamic_cast<SaltModel *> (salt_view->model ());
    tl_assert (model != 0);

    SaltGrain *g = model->grain_from_index (salt_view->currentIndex ());
    tl_assert (g != 0);

    return *g;
  }

  std::string name () const
  {
    return tl::to_string (name_edit->text ());
  }

  void accept ()
  {
    name_alert->clear ();
    std::string name = tl::to_string (name_edit->text ().simplified ());
    if (name.empty ()) {
      name_alert->error () << tr ("Name must not be empty");
    } else if (! SaltGrain::valid_name (name)) {
      name_alert->error () << tr ("Name is not valid (must be composed of letters, digits or underscores.\nGroups and names need to be separated with slashes.");
    } else {

      //  check, if this name does not exist yet
      for (Salt::flat_iterator g = mp_salt->begin_flat (); g != mp_salt->end_flat (); ++g) {
        if ((*g)->name () == name) {
          name_alert->error () << tr ("A package with this name already exists");
          return;
        }
      }

      QDialog::accept ();

    }
  }

private:
  lay::Salt m_salt_templates;
  lay::Salt *mp_salt;
};

// --------------------------------------------------------------------------------------
//  SaltManager implementation

// @@@
lay::Salt salt;
static bool salt_initialized = false;
void make_salt ()
{
  if (!salt_initialized) {
    salt_initialized = true;
    salt.add_location (tl::to_string (QDir::homePath () + QString::fromUtf8("/.klayout/salt")));
  }
}
lay::Salt *get_salt ()
{
  salt = lay::Salt (); salt_initialized = false;
  make_salt ();
  return &salt;
}
// @@@

// @@@
lay::Salt salt_mine;
void make_salt_mine ()
{
  salt_mine = lay::Salt ();
  salt_mine.load ("/home/matthias/salt.mine");
}
lay::Salt *get_salt_mine ()
{
  make_salt_mine();
  return &salt_mine;
}
// @@@

SaltManagerDialog::SaltManagerDialog (QWidget *parent)
  : QDialog (parent),
    m_current_changed_enabled (true), dm_update_models (this, &SaltManagerDialog::update_models)
{
  Ui::SaltManagerDialog::setupUi (this);
  mp_properties_dialog = new lay::SaltGrainPropertiesDialog (this);

  connect (edit_button, SIGNAL (clicked ()), this, SLOT (edit_properties ()));
  connect (create_button, SIGNAL (clicked ()), this, SLOT (create_grain ()));
  connect (delete_button, SIGNAL (clicked ()), this, SLOT (delete_grain ()));
  connect (apply_button, SIGNAL (clicked ()), this, SLOT (apply ()));

  mp_salt = get_salt ();
  mp_salt_mine = get_salt_mine ();

  SaltModel *model = new SaltModel (this, mp_salt);
  salt_view->setModel (model);
  salt_view->setItemDelegate (new SaltItemDelegate (this));

  SaltModel *mine_model = new SaltModel (this, mp_salt_mine);
  salt_mine_view->setModel (mine_model);
  salt_mine_view->setItemDelegate (new SaltItemDelegate (this));

  mode_tab->setCurrentIndex (mp_salt->is_empty () ? 1 : 0);

  connect (mode_tab, SIGNAL (currentChanged (int)), this, SLOT (mode_changed ()));

  connect (mp_salt, SIGNAL (collections_changed ()), this, SLOT (salt_changed ()));
  connect (mp_salt_mine, SIGNAL (collections_changed ()), this, SLOT (salt_mine_changed ()));

  update_models ();

  connect (salt_view->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (current_changed ()));
  connect (salt_view, SIGNAL (doubleClicked (const QModelIndex &)), this, SLOT (edit_properties ()));
  connect (salt_mine_view->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (mine_current_changed ()), Qt::QueuedConnection);
  connect (salt_mine_view, SIGNAL (doubleClicked (const QModelIndex &)), this, SLOT (mark_clicked ()));

  search_installed_edit->set_clear_button_enabled (true);
  search_new_edit->set_clear_button_enabled (true);
  connect (search_installed_edit, SIGNAL (textChanged (const QString &)), this, SLOT (search_text_changed (const QString &)));
  connect (search_new_edit, SIGNAL (textChanged (const QString &)), this, SLOT (search_text_changed (const QString &)));

  connect (mark_button, SIGNAL (clicked ()), this, SLOT (mark_clicked ()));

  salt_mine_view->addAction (actionUnmarkAll);
  QAction *a = new QAction (this);
  a->setSeparator (true);
  salt_mine_view->addAction (a);
  salt_mine_view->addAction (actionShowMarkedOnly);
  salt_mine_view->addAction (actionShowAll);
  salt_mine_view->setContextMenuPolicy (Qt::ActionsContextMenu);

  connect (actionUnmarkAll, SIGNAL (triggered ()), this, SLOT (unmark_all ()));
  connect (actionShowMarkedOnly, SIGNAL (triggered ()), this, SLOT (show_marked_only ()));
  connect (actionShowAll, SIGNAL (triggered ()), this, SLOT (show_all ()));
}

void
SaltManagerDialog::mode_changed ()
{
  //  keeps the splitters in sync
  if (mode_tab->currentIndex () == 1) {
    splitter_new->setSizes (splitter->sizes ());
    show_all ();
  } else if (mode_tab->currentIndex () == 0) {
    splitter->setSizes (splitter_new->sizes ());
  }
}

void
SaltManagerDialog::show_all ()
{
  search_new_edit->clear ();

  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view->model ());
  if (! model) {
    return;
  }

  for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
    --i;
    salt_mine_view->setRowHidden (i, false);
  }
}

void
SaltManagerDialog::show_marked_only ()
{
  search_new_edit->clear ();

  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view->model ());
  if (! model) {
    return;
  }

  for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
    --i;
    SaltGrain *g = model->grain_from_index (model->index (i, 0, QModelIndex ()));
    salt_mine_view->setRowHidden (i, !(g && model->is_marked (g->name ())));
  }
}

void
SaltManagerDialog::unmark_all ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view->model ());
  if (model) {
    model->clear_marked ();
    update_apply_state ();
  }
}

void
SaltManagerDialog::search_text_changed (const QString &text)
{
  QListView *view = 0;
  if (sender () == search_installed_edit) {
    view = salt_view;
  } else if (sender () == search_new_edit) {
    view = salt_mine_view;
  } else {
    return;
  }

  SaltModel *model = dynamic_cast <SaltModel *> (view->model ());
  if (! model) {
    return;
  }

  if (text.isEmpty ()) {

    for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
      --i;
      view->setRowHidden (i, false);
    }

  } else {

    QRegExp re (text, Qt::CaseInsensitive);

    for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
      --i;
      QModelIndex index = model->index (i, 0, QModelIndex ());
      SaltGrain *g = model->grain_from_index (index);
      bool hidden = (!g || re.indexIn (tl::to_qstring (g->name ())) < 0);
      view->setRowHidden (i, hidden);
    }

  }
}

void
SaltManagerDialog::mark_clicked ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view->model ());
  if (! model) {
    return;
  }
  SaltGrain *g = mine_current_grain ();
  if (! g) {
    return;
  }

  model->set_marked (g->name (), !model->is_marked (g->name ()));
  update_apply_state ();
}

void
SaltManagerDialog::update_apply_state ()
{
  int marked = 0;

  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view->model ());
  if (! model) {
    return;
  }

  for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
    --i;
    QModelIndex index = model->index (i, 0, QModelIndex ());
    SaltGrain *g = model->grain_from_index (index);
    if (g && model->is_marked (g->name ())) {
      marked += 1;
    }
  }

  apply_button->setEnabled (marked > 0);
  if (marked == 0) {
    apply_label->setText (QString ());
  } else if (marked == 1) {
    apply_label->setText (tr ("One package selected"));
  } else if (marked > 1) {
    apply_label->setText (tr ("%1 packages selected").arg (marked));
  }
}

void
SaltManagerDialog::apply ()
{
BEGIN_PROTECTED

  lay::SaltDownloadManager manager;

  bool any = false;

  //  fetch all marked grains and register for download
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view->model ());
  if (model) {
    for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
      --i;
      QModelIndex index = model->index (i, 0, QModelIndex ());
      SaltGrain *g = model->grain_from_index (index);
      if (g && model->is_marked (g->name ())) {
        manager.register_download (g->name (), g->url (), g->version ());
        any = true;
      }
    }
  }

  if (! any) {
    throw tl::Exception (tl::to_string (tr ("No packages marked for installation or update")));
  }

  manager.compute_dependencies (*mp_salt, *mp_salt_mine);

  if (manager.show_confirmation_dialog (this, *mp_salt)) {
    unmark_all ();
    manager.execute (*mp_salt);
  }

END_PROTECTED
}

void
SaltManagerDialog::edit_properties ()
{
  SaltGrain *g = current_grain ();
  if (g) {
    if (mp_properties_dialog->exec_dialog (g, mp_salt)) {
      current_changed ();
    }
  }
}

void
SaltManagerDialog::create_grain ()
{
BEGIN_PROTECTED

  SaltGrainTemplateSelectionDialog temp_dialog (this, mp_salt);
  if (temp_dialog.exec ()) {

    SaltGrain target;
    target.set_name (temp_dialog.name ());

    if (mp_salt->create_grain (temp_dialog.templ (), target)) {

      //  select the new one
      SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());
      if (model) {
        for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
          --i;
          QModelIndex index = model->index (i, 0, QModelIndex ());
          SaltGrain *g = model->grain_from_index (index);
          if (g && g->name () == target.name ()) {
            salt_view->setCurrentIndex (index);
            break;
          }
        }

      }

    } else {
      throw tl::Exception (tl::to_string (tr ("Initialization of new package failed - see log window (File/Log Viewer) for details")));
    }

  }

END_PROTECTED
}

void
SaltManagerDialog::delete_grain ()
{
BEGIN_PROTECTED

  SaltGrain *g = current_grain ();
  if (! g) {
    throw tl::Exception (tl::to_string (tr ("No package selected to delete")));
  }

  if (QMessageBox::question (this, tr ("Delete Package"), tr ("Are you sure to delete package '%1'?").arg (tl::to_qstring (g->name ())), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
    mp_salt->remove_grain (*g);
  }

END_PROTECTED
}

void
SaltManagerDialog::salt_changed ()
{
  dm_update_models ();
}

void
SaltManagerDialog::salt_mine_changed ()
{
  dm_update_models ();
}

void
SaltManagerDialog::update_models ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());
  tl_assert (model != 0);

  //  NOTE: the disabling of the event handler prevents us from
  //  letting the model connect to the salt's signal directly.
  m_current_changed_enabled = false;

  model->clear_messages ();

  //  Establish a message saying that an update is available
  for (Salt::flat_iterator g = mp_salt->begin_flat (); g != mp_salt->end_flat (); ++g) {
    SaltGrain *gm = mp_salt_mine->grain_by_name ((*g)->name ());
    if (gm && SaltGrain::compare_versions (gm->version (), (*g)->version ()) > 0) {
      model->set_message ((*g)->name (), SaltModel::Warning, tl::to_string (tr ("An update to version %1 is available").arg (tl::to_qstring (gm->version ()))));
    }
  }

  model->update ();

  m_current_changed_enabled = true;

  if (mp_salt->is_empty ()) {

    list_stack->setCurrentIndex (1);
    details_frame->hide ();

  } else {

    list_stack->setCurrentIndex (0);
    details_frame->show ();

    //  select the first grain
    if (model->rowCount (QModelIndex ()) > 0) {
      salt_view->setCurrentIndex (model->index (0, 0, QModelIndex ()));
    }

  }

  SaltModel *mine_model = dynamic_cast <SaltModel *> (salt_mine_view->model ());
  tl_assert (mine_model != 0);

  //  NOTE: the disabling of the event handler prevents us from
  //  letting the model connect to the salt's signal directly.
  m_current_changed_enabled = false;

  mine_model->clear_order ();
  mine_model->clear_messages ();
  mine_model->enable_all ();

  //  Establish a message saying that an update is available
  for (Salt::flat_iterator g = mp_salt->begin_flat (); g != mp_salt->end_flat (); ++g) {
    SaltGrain *gm = mp_salt_mine->grain_by_name ((*g)->name ());
    if (gm && SaltGrain::compare_versions (gm->version (), (*g)->version ()) > 0) {
      mine_model->set_message ((*g)->name (), SaltModel::Warning, tl::to_string (tr ("The installed version is outdated (%1)").arg (tl::to_qstring ((*g)->version ()))));
      mine_model->set_order ((*g)->name (), -1);
    } else if (gm) {
      mine_model->set_message ((*g)->name (), SaltModel::None, tl::to_string (tr ("This package is already installed and up to date")));
      mine_model->set_order ((*g)->name (), 1);
      mine_model->set_enabled ((*g)->name (), false);
    }
  }

  mine_model->update ();

  m_current_changed_enabled = true;

  //  select the first grain
  if (mine_model->rowCount (QModelIndex ()) > 0) {
    salt_mine_view->setCurrentIndex (mine_model->index (0, 0, QModelIndex ()));
  }

  mine_current_changed ();
  current_changed ();
  update_apply_state ();
}

void
SaltManagerDialog::current_changed ()
{
  SaltGrain *g = current_grain ();
  details_text->set_grain (g);
  if (!g) {
    details_frame->setEnabled (false);
    delete_button->setEnabled (false);
  } else {
    details_frame->setEnabled (true);
    delete_button->setEnabled (true);
    edit_button->setEnabled (! g->is_readonly ());
  }
}

lay::SaltGrain *
SaltManagerDialog::current_grain ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());
  return model ? model->grain_from_index (salt_view->currentIndex ()) : 0;
}

void
SaltManagerDialog::mine_current_changed ()
{

BEGIN_PROTECTED

  SaltGrain *g = mine_current_grain ();
  details_new_frame->setEnabled (g != 0);

  if (! g) {
    details_new_text->set_grain (0);
    return;
  }

  m_remote_grain.reset (0);

  //  Download actual grain definition file
  try {

    if (g->url ().empty ()) {
      throw tl::Exception (tl::to_string (tr ("No download link available")));
    }

    QString text = tr (
      "<html>"
        "<body>"
          "<font color=\"#c0c0c0\">"
            "<h2>Fetching Package Definition ...</h2>"
            "<p><b>URL</b>: %1</p>"
          "</font>"
        "</body>"
      "</html>"
    )
    .arg (tl::to_qstring (SaltGrain::spec_url (g->url ())));

    details_new_text->setHtml (text);

    QApplication::processEvents (QEventLoop::ExcludeUserInputEvents);

    tl::InputHttpStream http (SaltGrain::spec_url (g->url ()));
    tl::InputStream stream (http);

    m_remote_grain.reset (new SaltGrain ());
    m_remote_grain->load (stream);
    m_remote_grain->set_url (g->url ());

    if (g->name () != m_remote_grain->name ()) {
      throw tl::Exception (tl::to_string (tr ("Name mismatch between repository and actual package (repository: %1, package: %2)").arg (tl::to_qstring (g->name ())).arg (tl::to_qstring (m_remote_grain->name ()))));
    }
    if (SaltGrain::compare_versions (g->version (), m_remote_grain->version ()) != 0) {
      throw tl::Exception (tl::to_string (tr ("Version mismatch between repository and actual package (repository: %1, package: %2)").arg (tl::to_qstring (g->version ())).arg (tl::to_qstring (m_remote_grain->version ()))));
    }

    details_new_text->set_grain (m_remote_grain.get ());

  } catch (tl::Exception &ex) {

    m_remote_grain.reset (0);

    QString text = tr (
      "<html>"
        "<body>"
          "<font color=\"#ff0000\">"
            "<h2>Error Fetching Package Definition</h2>"
            "<p><b>URL</b>: %1</p>"
            "<p><b>Error</b>: %2</p>"
          "</font>"
        "</body>"
      "</html>"
    )
    .arg (tl::to_qstring (SaltGrain::spec_url (g->url ())))
    .arg (tl::to_qstring (tl::escaped_to_html (ex.msg ())));

    details_new_text->setHtml (text);

  }

END_PROTECTED
}

lay::SaltGrain *
SaltManagerDialog::mine_current_grain ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view->model ());
  return model ? model->grain_from_index (salt_mine_view->currentIndex ()) : 0;
}

}
