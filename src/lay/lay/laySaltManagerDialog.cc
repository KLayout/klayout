
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

#include "laySaltManagerDialog.h"
#include "laySaltModel.h"
#include "laySaltGrainPropertiesDialog.h"
#include "laySaltDownloadManager.h"
#include "laySalt.h"
#include "layVersion.h"
#include "layItemDelegates.h"
#include "ui_SaltGrainTemplateSelectionDialog.h"
#include "tlString.h"
#include "tlExceptions.h"
#include "tlEnv.h"

#include "rba.h"
#include "pya.h"

#include <QTextDocument>
#include <QApplication>
#include <QPainter>
#include <QDir>
#include <QTextStream>
#include <QBuffer>
#include <QResource>
#include <QMessageBox>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QRegExp>

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
    salt_view->setItemDelegate (new lay::HTMLItemDelegate (this));
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
      name_alert->error () << tr ("Name is not valid (must be composed of letters, digits, dots or underscores.\nGroups and names need to be separated with slashes.");
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
//  SaltAPIVersionCheck

class SaltAPIVersionCheck
{
public:
  struct APIFeature
  {
    APIFeature (const std::string &_name, const std::string &_version, const std::string &_description)
      : name (_name), version (_version), description (_description)
    {
      //  .. nothing yet ..
    }

    std::string name, version, description;
  };

  SaltAPIVersionCheck ();
  bool check (const std::string &api_version);

  const std::string &message () const
  {
    return m_message;
  }

private:
  std::vector<APIFeature> m_features;
  std::string m_message;

  void populate_features ();
  const APIFeature *find_feature (const std::string &name) const;
  std::string feature_list () const;
};

SaltAPIVersionCheck::SaltAPIVersionCheck ()
{
  populate_features ();
}

bool
SaltAPIVersionCheck::check (const std::string &api_version)
{
  tl::Extractor ex (api_version.c_str ());

  bool any_not_available = false;
  bool good = true;
  m_message.clear ();

  while (! ex.at_end ()) {

    std::string fname;
    ex.try_read_name (fname);

    std::string v;
    while (! ex.at_end () && ! ex.test (";")) {
      int n = 0;
      if (ex.try_read (n)) {
        v += tl::to_string (n);
      } else if (ex.test (".")) {
        v += ".";
      } else {
        m_message = tl::to_string (tr ("API version string malformed - cannot check."));
        return false;
      }
    }

    const APIFeature *f = find_feature (fname);
    if (!f) {

      if (! m_message.empty ()) {
        m_message += "\n";
      }
      m_message += tl::sprintf (tl::to_string (tr ("Feature %s not available.")), fname);

      good = false;
      any_not_available = true;

    } else if (! f->version.empty () && ! v.empty () && SaltGrain::compare_versions (f->version, v) < 0) {

      //  shorten the version (Python reports "3.6.7 blabla...")
      std::vector<std::string> fv = tl::split (f->version, " ");
      tl_assert (! fv.empty ());
      std::string fv_short = fv.front ();
      if (fv.size () > 1) {
        fv_short += " ...";
      }

      if (! m_message.empty ()) {
        m_message += "\n";
      }
      m_message += tl::sprintf (tl::to_string (tr ("%s required with version %s or later (is %s).")), f->description, v, fv_short);

      good = false;

    }

  }

  if (any_not_available) {
    m_message += tl::sprintf (tl::to_string (tr ("\nAvailable features are: %s.")), feature_list ());
  }

  return good;
}

std::string
SaltAPIVersionCheck::feature_list () const
{
  std::string fl;
  for (std::vector<APIFeature>::const_iterator f = m_features.begin (); f != m_features.end (); ++f) {
    if (! fl.empty ()) {
      fl += ", ";
    }
    fl += f->name;
  }
  return fl;
}

const SaltAPIVersionCheck::APIFeature *
SaltAPIVersionCheck::find_feature (const std::string &name) const
{
  for (std::vector<APIFeature>::const_iterator f = m_features.begin (); f != m_features.end (); ++f) {
    if (f->name == name) {
      return f.operator-> ();
    }
  }
  return 0;
}

void
SaltAPIVersionCheck::populate_features ()
{
  m_features.push_back (APIFeature (std::string (), lay::Version::version (), "KLayout API"));

  if (rba::RubyInterpreter::instance () && rba::RubyInterpreter::instance ()->available ()) {
    std::string v = rba::RubyInterpreter::instance ()->version ();
    m_features.push_back (APIFeature ("ruby", v, "Ruby"));
    if (SaltGrain::compare_versions (v, "2") < 0) {
      m_features.push_back (APIFeature ("ruby1", v, "Ruby 1"));
    } else if (SaltGrain::compare_versions (v, "3") < 0) {
      m_features.push_back (APIFeature ("ruby2", v, "Ruby 2"));
    }
  }

  if (pya::PythonInterpreter::instance () && pya::PythonInterpreter::instance ()->available ()) {
    std::string v = pya::PythonInterpreter::instance ()->version ();
    m_features.push_back (APIFeature ("python", v, "Python"));
    if (SaltGrain::compare_versions (v, "3") < 0) {
      m_features.push_back (APIFeature ("python2", v, "Python 2"));
    } else if (SaltGrain::compare_versions (v, "4") < 0) {
      m_features.push_back (APIFeature ("python3", v, "Python 3"));
    }
  }

#if defined(HAVE_QTBINDINGS)
  m_features.push_back (APIFeature ("qt_binding", std::string (), "Qt Binding for RBA or PYA"));
#endif
#if defined(HAVE_QT)
#  if QT_VERSION >= 0x040000 && QT_VERSION < 0x050000
  m_features.push_back (APIFeature ("qt4", std::string (), "Qt 4"));
#  elif QT_VERSION >= 0x050000 && QT_VERSION < 0x060000
  m_features.push_back (APIFeature ("qt5", std::string (), "Qt 5"));
#  endif
#endif

#if defined(HAVE_64BIT_COORD)
  m_features.push_back (APIFeature ("wide-coords", std::string (), "64 bit coordinates"));
#endif
}

// --------------------------------------------------------------------------------------
//  SaltManager implementation

SaltManagerDialog::SaltManagerDialog (QWidget *parent, lay::Salt *salt, const std::string &salt_mine_url)
  : QDialog (parent),
    m_salt_mine_url (salt_mine_url),
    dm_update_models (this, &SaltManagerDialog::update_models), m_current_tab (-1),
    mp_downloaded_target (0),
    dm_mine_update_selected_changed (this, &SaltManagerDialog::do_mine_update_selected_changed),
    dm_mine_new_selected_changed (this, &SaltManagerDialog::do_mine_new_selected_changed)
{
  Ui::SaltManagerDialog::setupUi (this);
  mp_properties_dialog = new lay::SaltGrainPropertiesDialog (this);

  connect (edit_button, SIGNAL (clicked ()), this, SLOT (edit_properties ()));
  connect (create_button, SIGNAL (clicked ()), this, SLOT (create_grain ()));
  connect (delete_button, SIGNAL (clicked ()), this, SLOT (delete_grain ()));
  connect (apply_new_button, SIGNAL (clicked ()), this, SLOT (apply ()));
  connect (apply_update_button, SIGNAL (clicked ()), this, SLOT (apply ()));

  mp_salt = salt;

  SaltModel *model = new SaltModel (this, mp_salt);
  model->set_empty_explanation (tr ("No packages are present on this system"));
  salt_view->setModel (model);
  salt_view->setItemDelegate (new lay::HTMLItemDelegate (this));

  SaltModel *mine_model;

  //  This model will show only the grains of mp_salt_mine which are not present in mp_salt yet.
  mine_model = new SaltModel (this, &m_salt_mine, mp_salt, true);
  mine_model->set_empty_explanation (tr ("All available packages are installed"));
  salt_mine_view_new->setModel (mine_model);
  salt_mine_view_new->setItemDelegate (new lay::HTMLItemDelegate (this));

  //  This model will show only the grains of mp_salt_mine which are present in mp_salt already.
  mine_model = new SaltModel (this, &m_salt_mine, mp_salt, false);
  mine_model->set_empty_explanation (tr ("No packages are installed"));
  salt_mine_view_update->setModel (mine_model);
  salt_mine_view_update->setItemDelegate (new lay::HTMLItemDelegate (this));

  mode_tab->setCurrentIndex (0);

  connect (mode_tab, SIGNAL (currentChanged (int)), this, SLOT (mode_changed ()));
  m_current_tab = mode_tab->currentIndex ();

  connect (mp_salt, SIGNAL (collections_changed ()), this, SLOT (salt_changed ()));
  connect (mp_salt, SIGNAL (collections_about_to_change ()), this, SLOT (salt_about_to_change ()));
  connect (&m_salt_mine, SIGNAL (collections_changed ()), this, SLOT (salt_mine_changed ()));
  connect (&m_salt_mine, SIGNAL (collections_about_to_change ()), this, SLOT (salt_mine_about_to_change ()));

  connect (salt_view->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (selected_changed ()));
  connect (salt_view, SIGNAL (doubleClicked (const QModelIndex &)), this, SLOT (edit_properties ()));
  connect (salt_mine_view_new->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (mine_new_selected_changed ()));
  connect (salt_mine_view_update->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (mine_update_selected_changed ()));
  connect (salt_mine_view_new, SIGNAL (doubleClicked (const QModelIndex &)), this, SLOT (mark_clicked ()));
  connect (salt_mine_view_update, SIGNAL (doubleClicked (const QModelIndex &)), this, SLOT (mark_clicked ()));

  search_installed_edit->set_clear_button_enabled (true);
  search_new_edit->set_clear_button_enabled (true);
  search_update_edit->set_clear_button_enabled (true);
  connect (search_installed_edit, SIGNAL (textChanged (const QString &)), this, SLOT (search_text_changed (const QString &)));
  connect (search_new_edit, SIGNAL (textChanged (const QString &)), this, SLOT (search_text_changed (const QString &)));
  connect (search_update_edit, SIGNAL (textChanged (const QString &)), this, SLOT (search_text_changed (const QString &)));

  connect (mark_new_button, SIGNAL (clicked ()), this, SLOT (mark_clicked ()));
  connect (mark_update_button, SIGNAL (clicked ()), this, SLOT (mark_clicked ()));

  QAction *a;

  salt_view->addAction (actionCreatePackage);
  salt_view->addAction (actionDelete);
  salt_view->setContextMenuPolicy (Qt::ActionsContextMenu);

  salt_mine_view_new->addAction (actionMarkNew);
  salt_mine_view_new->addAction (actionMarkAllNew);
  salt_mine_view_new->addAction (actionUnmarkNew);
  salt_mine_view_new->addAction (actionUnmarkAllNew);
  a = new QAction (this);
  a->setSeparator (true);
  salt_mine_view_new->addAction (a);
  salt_mine_view_new->addAction (actionShowMarkedOnlyNew);
  actionShowMarkedOnlyNew->setCheckable (true);
  a = new QAction (this);
  a->setSeparator (true);
  salt_mine_view_new->addAction (a);
  salt_mine_view_new->addAction (actionRefresh);
  salt_mine_view_new->setContextMenuPolicy (Qt::ActionsContextMenu);

  salt_mine_view_update->addAction (actionMarkForUpdate);
  salt_mine_view_update->addAction (actionMarkAllUpdate);
  salt_mine_view_update->addAction (actionUnmarkForUpdate);
  salt_mine_view_update->addAction (actionUnmarkAllUpdate);
  a = new QAction (this);
  a->setSeparator (true);
  salt_mine_view_update->addAction (a);
  salt_mine_view_update->addAction (actionShowMarkedOnlyUpdate);
  actionShowMarkedOnlyUpdate->setCheckable (true);
  a = new QAction (this);
  a->setSeparator (true);
  salt_mine_view_update->addAction (a);
  salt_mine_view_update->addAction (actionRefresh);
  salt_mine_view_update->setContextMenuPolicy (Qt::ActionsContextMenu);

  connect (actionCreatePackage, SIGNAL (triggered ()), this, SLOT (create_grain ()));
  connect (actionDelete, SIGNAL (triggered ()), this, SLOT (delete_grain ()));
  connect (actionUnmarkAllNew, SIGNAL (triggered ()), this, SLOT (unmark_all_new ()));
  connect (actionMarkAllNew, SIGNAL (triggered ()), this, SLOT (mark_all_new ()));
  connect (actionShowMarkedOnlyNew, SIGNAL (triggered ()), this, SLOT (show_marked_only_new ()));
  connect (actionUnmarkAllUpdate, SIGNAL (triggered ()), this, SLOT (unmark_all_update ()));
  connect (actionMarkAllUpdate, SIGNAL (triggered ()), this, SLOT (mark_all_update ()));
  connect (actionShowMarkedOnlyUpdate, SIGNAL (triggered ()), this, SLOT (show_marked_only_update ()));
  connect (actionRefresh, SIGNAL (triggered ()), this, SLOT (refresh ()));
  connect (actionMarkNew, SIGNAL (triggered ()), this, SLOT (mark_clicked ()));
  connect (actionUnmarkNew, SIGNAL (triggered ()), this, SLOT (mark_clicked ()));
  connect (actionMarkForUpdate, SIGNAL (triggered ()), this, SLOT (mark_clicked ()));
  connect (actionUnmarkForUpdate, SIGNAL (triggered ()), this, SLOT (mark_clicked ()));

  refresh ();
}

void
SaltManagerDialog::mode_changed ()
{
  //  commits edits:
  setFocus (Qt::NoFocusReason);

  QList<int> sizes;
  if (m_current_tab == 2) {
    selected_changed ();
    sizes = splitter->sizes ();
  } else if (m_current_tab == 1) {
    mine_update_selected_changed ();
    sizes = splitter_update->sizes ();
  } else if (m_current_tab == 0) {
    mine_new_selected_changed ();
    sizes = splitter_new->sizes ();
  }

  //  keeps the splitters in sync
  if (sizes.size () == 2 && sizes[1] > 0 /*visible*/) {
    splitter_new->setSizes (sizes);
    splitter_update->setSizes (sizes);
    splitter->setSizes (sizes);
  }

  actionShowMarkedOnlyNew->setChecked (false);
  actionShowMarkedOnlyUpdate->setChecked (false);

  if (mode_tab->currentIndex () < 2) {
    show_marked_only_new ();
    show_marked_only_update ();
  }

  m_current_tab = mode_tab->currentIndex ();
  update_apply_state ();
}

void
SaltManagerDialog::show_marked_only_new ()
{
  bool show_marked_only = actionShowMarkedOnlyNew->isChecked ();

  search_new_edit->clear ();

  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view_new->model ());
  if (! model) {
    return;
  }

  salt_mine_view_new->clearSelection ();

  for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
    --i;
    SaltGrain *g = model->grain_from_index (model->index (i, 0, QModelIndex ()));
    salt_mine_view_new->setRowHidden (i, show_marked_only && !(g && model->is_marked (g->name ())));
    mine_new_selected_changed ();
  }
}

void
SaltManagerDialog::show_marked_only_update ()
{
  bool show_marked_only = actionShowMarkedOnlyUpdate->isChecked ();

  search_update_edit->clear ();

  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view_update->model ());
  if (! model) {
    return;
  }

  salt_mine_view_new->clearSelection ();

  for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
    --i;
    SaltGrain *g = model->grain_from_index (model->index (i, 0, QModelIndex ()));
    salt_mine_view_update->setRowHidden (i, show_marked_only && !(g && model->is_marked (g->name ())));
    mine_update_selected_changed ();
  }
}

void
SaltManagerDialog::unmark_all_new ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view_new->model ());
  if (model) {
    model->clear_marked ();
    actionShowMarkedOnlyNew->setChecked (false);
    show_marked_only_new ();
    update_apply_state ();
  }
}

void
SaltManagerDialog::mark_all_new ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view_new->model ());
  if (model) {
    model->mark_all ();
    actionShowMarkedOnlyNew->setChecked (false);
    show_marked_only_new ();
    update_apply_state ();
  }
}

void
SaltManagerDialog::unmark_all_update ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view_update->model ());
  if (model) {
    model->clear_marked ();
    actionShowMarkedOnlyUpdate->setChecked (false);
    show_marked_only_update ();
    update_apply_state ();
  }
}

void
SaltManagerDialog::mark_all_update ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view_update->model ());
  if (model) {
    model->mark_all ();
    actionShowMarkedOnlyUpdate->setChecked (false);
    show_marked_only_update ();
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
    view = salt_mine_view_new;
  } else if (sender () == search_update_edit) {
    view = salt_mine_view_update;
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
  QListView *view;
  if (sender () == salt_mine_view_new || sender () == mark_new_button || sender () == actionMarkNew || sender () == actionUnmarkNew) {
    view = salt_mine_view_new;
  } else {
    view = salt_mine_view_update;
  }

  bool toggle = (sender () != actionMarkNew && sender () != actionUnmarkNew && sender () != actionMarkForUpdate && sender () != actionUnmarkForUpdate);
  bool set = (sender () == actionMarkNew || sender () == actionMarkForUpdate);

  SaltModel *model = dynamic_cast <SaltModel *> (view->model ());
  if (! model) {
    return;
  }

  QModelIndexList indexes = view->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = indexes.begin (); i != indexes.end (); ++i) {
    SaltGrain *g = model->grain_from_index (*i);
    if (g) {
      model->set_marked (g->name (), toggle ? ! model->is_marked (g->name ()) : set);
    }
  }

  update_apply_state ();
}

void
SaltManagerDialog::update_apply_state ()
{
  SaltModel *model;

  model = dynamic_cast <SaltModel *> (salt_mine_view_new->model ());
  if (model) {

    int marked = 0;

    for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
      --i;
      QModelIndex index = model->index (i, 0, QModelIndex ());
      SaltGrain *g = model->grain_from_index (index);
      if (g && model->is_marked (g->name ())) {
        marked += 1;
      }
    }

    apply_new_button->setEnabled (marked > 0);
    if (marked == 0) {
      apply_label_new->setText (tr ("Select at least one package for installation (check button)"));
    } else if (marked == 1) {
      apply_label_new->setText (tr ("One package selected"));
    } else if (marked > 1) {
      apply_label_new->setText (tr ("%1 packages selected").arg (marked));
    }

  }

  model = dynamic_cast <SaltModel *> (salt_mine_view_update->model ());
  if (model) {

    int marked = 0;

    for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
      --i;
      QModelIndex index = model->index (i, 0, QModelIndex ());
      SaltGrain *g = model->grain_from_index (index);
      if (g && model->is_marked (g->name ())) {
        marked += 1;
      }
    }

    apply_update_button->setEnabled (marked > 0);
    if (marked == 0) {
      apply_label_update->setText (QString ());
    } else if (marked == 1) {
      apply_label_update->setText (tr ("One package selected"));
    } else if (marked > 1) {
      apply_label_update->setText (tr ("%1 packages selected").arg (marked));
    }

  }
}

void
SaltManagerDialog::apply ()
{
BEGIN_PROTECTED

  bool update = (sender () == apply_update_button);

  lay::SaltDownloadManager manager;

  bool any = false;

  //  fetch all marked grains and register for download
  SaltModel *model;
  if (update) {
    model = dynamic_cast <SaltModel *> (salt_mine_view_update->model ());
  } else {
    model = dynamic_cast <SaltModel *> (salt_mine_view_new->model ());
  }

  if (model) {
    for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
      --i;
      QModelIndex index = model->index (i, 0, QModelIndex ());
      SaltGrain *g = model->grain_from_index (index);
      //  NOTE: checking for valid_name prevents bad entries inside the download list
      if (g && model->is_marked (g->name ()) && SaltGrain::valid_name (g->name ())) {
        manager.register_download (g->name (), g->token (), g->url (), g->version ());
        any = true;
      }
    }
  }

  if (! any) {
    if (update) {
      throw tl::Exception (tl::to_string (tr ("No packages marked for update")));
    } else {
      throw tl::Exception (tl::to_string (tr ("No packages marked for installation")));
    }
  }

  manager.compute_dependencies (*mp_salt, m_salt_mine);
  if (manager.execute (this, *mp_salt)) {
    if (update) {
      unmark_all_update ();
    } else {
      unmark_all_new ();
    }
  }

END_PROTECTED
}

void
SaltManagerDialog::edit_properties ()
{
  SaltGrain *g = current_grain ();
  if (g) {
    if (g->is_readonly ()) {
      QMessageBox::critical (this, tr ("Package is not Editable"),
                                   tr ("This package cannot be edited.\n\nEither you don't have write permissions on the directory or the package was installed from a repository."));
    } else if (mp_properties_dialog->exec_dialog (g, mp_salt)) {
      selected_changed ();
    }
  }
}

void
SaltManagerDialog::set_current_grain_by_name (const std::string &current)
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());
  if (!model) {
    return;
  }

  for (int i = model->rowCount (QModelIndex ()); i > 0; ) {
    --i;
    QModelIndex index = model->index (i, 0, QModelIndex ());
    SaltGrain *g = model->grain_from_index (index);
    if (g && g->name () == current) {
      salt_view->clearSelection ();
      salt_view->setCurrentIndex (index);
      break;
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

        //  NOTE: this is basically redundant (because it happens in the background later
        //  through dm_update_models). But we need this now to establish the selection.
        model->update();

        set_current_grain_by_name (target.name ());

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

  std::vector<SaltGrain *> gg = current_grains ();
  if (gg.empty ()) {
    throw tl::Exception (tl::to_string (tr ("No package selected to delete")));
  }

  std::vector <std::string> failed;

  if (gg.size () == 1) {
    SaltGrain *g = gg.front ();
    if (QMessageBox::question (this, tr ("Delete Package"), tr ("Are you sure to delete package '%1'?").arg (tl::to_qstring (g->name ())), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
      if (! mp_salt->remove_grain (*g)) {
        failed.push_back (g->name ());
      }
    }
  } else {
    if (QMessageBox::question (this, tr ("Delete Packages"), tr ("Are you sure to delete the selected %1 packages?").arg (int (gg.size ())), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
      for (std::vector<SaltGrain *>::const_iterator i = gg.begin (); i != gg.end (); ++i) {
        if (! mp_salt->remove_grain (**i)) {
          failed.push_back ((*i)->name ());
        }
      }
    }
  }

  if (failed.size () == 1) {
    throw tl::Exception (tl::to_string (tr ("Failed to remove package %1 (no write permissions on directory?)").arg (tl::to_qstring (failed.front ()))));
  } else if (failed.size () > 1) {
    throw tl::Exception (tl::to_string (tr ("Failed to remove the following packages:\n  %1").arg (tl::to_qstring (tl::join (failed, "\n  ")))));
  }

END_PROTECTED
}

void
SaltManagerDialog::salt_about_to_change ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());
  tl_assert (model != 0);
  model->begin_update ();
}

void
SaltManagerDialog::salt_changed ()
{
  dm_update_models ();
}

void
SaltManagerDialog::salt_mine_about_to_change ()
{
  SaltModel *model;

  model = dynamic_cast <SaltModel *> (salt_mine_view_new->model ());
  tl_assert (model != 0);
  model->begin_update ();

  model = dynamic_cast <SaltModel *> (salt_mine_view_update->model ());
  tl_assert (model != 0);
  model->begin_update ();
}

void
SaltManagerDialog::refresh ()
{
  m_salt_grain_cache.clear ();

  if (! m_salt_mine_url.empty ()) {

    tl::log << tl::to_string (tr ("Downloading package repository from %1").arg (tl::to_qstring (m_salt_mine_url)));

    m_salt_mine_reader.reset (new tl::InputStream (m_salt_mine_url));
    salt_mine_download_started ();

    tl::InputHttpStream *http = dynamic_cast<tl::InputHttpStream *> (m_salt_mine_reader->base ());
    if (http) {
      //  async reading on HTTP
      http->ready ().add (this, &SaltManagerDialog::salt_mine_data_ready);
      http->send ();
    } else {
      salt_mine_data_ready ();
    }

  }
}

void
SaltManagerDialog::salt_mine_download_started ()
{
  QApplication::setOverrideCursor (Qt::WaitCursor);
}

void
SaltManagerDialog::salt_mine_download_finished ()
{
  QApplication::restoreOverrideCursor ();
  if (m_salt_mine_reader.get ()) {
    //  NOTE: don't delete the reader in the slot it triggered
    m_salt_mine_reader->close ();
  }
}

void
SaltManagerDialog::salt_mine_data_ready ()
{
BEGIN_PROTECTED

  try {

    if (m_salt_mine_reader.get ()) {

      lay::Salt new_mine;
      new_mine.load (m_salt_mine_url, *m_salt_mine_reader);
      m_salt_mine = new_mine;

    }

    salt_mine_download_finished ();

  } catch (...) {
    salt_mine_download_finished ();
    throw;
  }

  salt_mine_changed ();

END_PROTECTED
}

void
SaltManagerDialog::salt_mine_changed ()
{
  dm_update_models ();
}

void
SaltManagerDialog::update_models ()
{
  actionShowMarkedOnlyNew->setChecked (false);
  actionShowMarkedOnlyUpdate->setChecked (false);

  SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());
  tl_assert (model != 0);

  model->clear_messages ();

  //  Maintain the current index while updating
  std::string current;
  if (salt_view->currentIndex ().isValid ()) {
    const lay::SaltGrain *g = model->grain_from_index (salt_view->currentIndex ());
    if (g) {
      current = g->name ();
    }
  }

  //  Establish a message saying that an update is available
  for (Salt::flat_iterator g = mp_salt->begin_flat (); g != mp_salt->end_flat (); ++g) {
    SaltGrain *gm = m_salt_mine.grain_by_name ((*g)->name ());
    if (gm && SaltGrain::compare_versions (gm->version (), (*g)->version ()) > 0) {
      model->set_message ((*g)->name (), SaltModel::Warning, tl::to_string (tr ("An update to version %1 is available").arg (tl::to_qstring (gm->version ()))));
    }
  }

  model->update ();

  if (! current.empty ()) {
    set_current_grain_by_name (current);
  }

  if (mp_salt->is_empty ()) {

    list_stack->setCurrentIndex (1);
    details_frame->hide ();

  } else {

    list_stack->setCurrentIndex (0);
    details_frame->show ();

    //  select the first grain if required
    if (! salt_view->currentIndex ().isValid () && model->rowCount (QModelIndex ()) > 0) {
      salt_view->setCurrentIndex (model->index (0, 0, QModelIndex ()));
    }

  }

  SaltAPIVersionCheck svc;
  SaltModel *mine_model;

  mine_model = dynamic_cast <SaltModel *> (salt_mine_view_update->model ());
  tl_assert (mine_model != 0);

  mine_model->clear_order ();
  mine_model->clear_messages ();
  mine_model->enable_all ();

  bool has_warning = false;

  //  Establish a message saying that an update is available
  for (Salt::flat_iterator g = mp_salt->begin_flat (); g != mp_salt->end_flat (); ++g) {
    SaltGrain *gm = m_salt_mine.grain_by_name ((*g)->name ());
    if (gm && SaltGrain::compare_versions (gm->version (), (*g)->version ()) > 0) {
      has_warning = true;
      mine_model->set_message ((*g)->name (), SaltModel::Warning, tl::to_string (tr ("The installed version is outdated (%1)").arg (tl::to_qstring ((*g)->version ()))));
      mine_model->set_order ((*g)->name (), -1);
    } else if (gm) {
      mine_model->set_message ((*g)->name (), SaltModel::None, tl::to_string (tr ("This package is up to date")));
      mine_model->set_order ((*g)->name (), 1);
      mine_model->set_enabled ((*g)->name (), false);
    }
  }

  //  Establish a message indicating whether the API version does not match
  for (Salt::flat_iterator g = m_salt_mine.begin_flat (); g != m_salt_mine.end_flat (); ++g) {
    if (! svc.check ((*g)->api_version ())) {
      mine_model->set_message ((*g)->name (), SaltModel::Warning, svc.message ());
      mine_model->set_enabled ((*g)->name (), false);
    }
  }

  if (has_warning) {
    mode_tab->setTabIcon (1, QIcon (":/warn_16px.png"));
  } else {
    mode_tab->setTabIcon (1, QIcon ());
  }

  mine_model->update ();

  //  select the first grain
  if (mine_model->rowCount (QModelIndex ()) > 0) {
    salt_mine_view_update->selectionModel ()->blockSignals (true);
    salt_mine_view_update->clearSelection ();
    salt_mine_view_update->setCurrentIndex (mine_model->index (0, 0, QModelIndex ()));
    salt_mine_view_update->selectionModel ()->blockSignals (false);
  }

  mine_model = dynamic_cast <SaltModel *> (salt_mine_view_new->model ());
  tl_assert (mine_model != 0);

  mine_model->clear_order ();
  mine_model->clear_messages ();
  mine_model->enable_all ();

  //  Establish a message indicating whether the API version does not match
  for (Salt::flat_iterator g = m_salt_mine.begin_flat (); g != m_salt_mine.end_flat (); ++g) {
    if (! svc.check ((*g)->api_version ())) {
      mine_model->set_message ((*g)->name (), SaltModel::Warning, svc.message ());
      mine_model->set_enabled ((*g)->name (), false);
    }
  }

  mine_model->update ();

  //  select the first grain
  if (mine_model->rowCount (QModelIndex ()) > 0) {
    salt_mine_view_new->selectionModel ()->blockSignals (true);
    salt_mine_view_new->clearSelection ();
    salt_mine_view_new->setCurrentIndex (mine_model->index (0, 0, QModelIndex ()));
    salt_mine_view_new->selectionModel ()->blockSignals (false);
  }

  mode_changed ();
}

void
SaltManagerDialog::selected_changed ()
{
  SaltGrain *g = current_grain ();
  details_text->set_grain (g);
  if (!g) {
    details_frame->setEnabled (false);
  } else {
    details_frame->setEnabled (true);
    edit_button->setEnabled (! g->is_readonly ());
  }

  delete_button->setEnabled (! current_grains ().empty ());
}

lay::SaltGrain *
SaltManagerDialog::current_grain ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());

  QModelIndexList indexes = salt_view->selectionModel ()->selectedIndexes ();
  if (indexes.size () == 1 && model) {
    return model->grain_from_index (indexes.front ());
  } else {
    return 0;
  }
}

std::vector<lay::SaltGrain *>
SaltManagerDialog::current_grains ()
{
  std::vector<lay::SaltGrain *> res;

  SaltModel *model = dynamic_cast <SaltModel *> (salt_view->model ());
  if (model) {

    QModelIndexList indexes = salt_view->selectionModel ()->selectedIndexes ();
    for (QModelIndexList::const_iterator i = indexes.begin (); i != indexes.end (); ++i) {
      lay::SaltGrain *g = model->grain_from_index (*i);
      if (g) {
        res.push_back (g);
      }
    }

  }

  return res;
}

void
SaltManagerDialog::mine_update_selected_changed ()
{
  dm_mine_update_selected_changed ();
}

void
SaltManagerDialog::do_mine_update_selected_changed ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view_update->model ());
  tl_assert (model != 0);

  SaltGrain *g = 0;
  QModelIndexList indexes = salt_mine_view_update->selectionModel ()->selectedIndexes();
  if (indexes.size () == 1) {
    g = model->grain_from_index (indexes.front ());
  }

  details_update_frame->setEnabled (g != 0);

  get_remote_grain_info (g, details_update_text);
}

void
SaltManagerDialog::mine_new_selected_changed ()
{
  dm_mine_new_selected_changed ();
}

void
SaltManagerDialog::do_mine_new_selected_changed ()
{
  SaltModel *model = dynamic_cast <SaltModel *> (salt_mine_view_new->model ());
  tl_assert (model != 0);

  SaltGrain *g = 0;
  QModelIndexList indexes = salt_mine_view_new->selectionModel ()->selectedIndexes();
  if (indexes.size () == 1) {
    g = model->grain_from_index (indexes.front ());
  }

  details_new_frame->setEnabled (g != 0);

  get_remote_grain_info (g, details_new_text);
}

namespace
{

/**
 * @brief A callback to keep the UI alive (mainly used for Git grain retrieval)
 */
class ProcessEventCallback
  : public tl::InputHttpStreamCallback
{
public:
  virtual void wait_for_input ()
  {
    QApplication::processEvents (QEventLoop::ExcludeUserInputEvents);
  }
};

class FetchGrainInfoProgressAdaptor
  : public tl::ProgressAdaptor
{
public:
  FetchGrainInfoProgressAdaptor (SaltGrainDetailsTextWidget *details, const std::string &name, const QString &html)
    : mp_details (details), m_name (name), m_html (html)
  {
    mp_details->setHtml (m_html.arg (QString ()));
    m_counter = 0;
  }

  virtual void yield (tl::Progress *progress)
  {
    QCoreApplication::processEvents (QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents, 100);

    ++m_counter;
    std::string all_dots = "..........";
    m_counter = m_counter % all_dots.size ();
    std::string dots = std::string (all_dots, 0, m_counter);
    mp_details->setHtml (m_html.arg (tl::to_qstring (tl::sprintf (tl::to_string (tr ("Downloading %.0f%% %s")), progress->value (), dots))));
  }

  virtual void trigger (tl::Progress * /*progress*/)
  {
    //  .. nothing yet ..
  }

  void error ()
  {
    mp_details->setHtml (m_html.arg (QString ()));
  }

  void success ()
  {
    mp_details->setHtml (m_html.arg (QString ()));
  }

  bool is_aborted () const
  {
    return false;
  }

private:
  lay::SaltGrainDetailsTextWidget *mp_details;
  std::string m_name;
  QString m_html;
  unsigned int m_counter;
};

}

void
SaltManagerDialog::get_remote_grain_info (lay::SaltGrain *g, SaltGrainDetailsTextWidget *details)
{
  //  NOTE: we don't want to interfere with download here, so refuse to do update
  //  the info while a package is downloaded.
  if (! g || m_downloaded_grain.get ()) {
    details->setHtml (QString ());
    return;
  }

  m_downloaded_grain.reset (0);

  if (m_downloaded_grain_reader.get ()) {
    m_downloaded_grain_reader->close ();
  }
  m_downloaded_grain_reader.reset (0);

  mp_downloaded_target = details;
  m_salt_mine_grain.reset (new lay::SaltGrain (*g));

  if (m_salt_mine.download_package_information () && m_salt_mine.grain_by_name (g->name ())) {

    //  Download actual grain definition file
    try {

      if (g->url ().empty ()) {
        throw tl::Exception (tl::to_string (tr ("No download link available")));
      }

      QString html = tr (
        "<html>"
          "<body>"
            "<font color=\"#c0c0c0\">"
              "<h2>Fetching Package Definition ...</h2>"
              "<p><b>URL</b>: %1</p>"
              "<p>%2</p>"
            "</font>"
          "</body>"
        "</html>"
      )
      .arg (tl::to_qstring (g->url ()));

      details->setHtml (html.arg (QString ()));

      FetchGrainInfoProgressAdaptor pa (details, g->name (), html);

      std::string url = g->url ();

      auto sg = m_salt_grain_cache.find (url);
      if (sg == m_salt_grain_cache.end ()) {

        m_downloaded_grain.reset (new SaltGrain ());
        m_downloaded_grain->set_url (url);

        //  NOTE: stream_from_url may modify the URL, hence we set it again
        ProcessEventCallback callback;
        m_downloaded_grain_reader.reset (SaltGrain::stream_from_url (url, 60.0, &callback));
        m_downloaded_grain->set_url (url);

        tl::InputHttpStream *http = dynamic_cast<tl::InputHttpStream *> (m_downloaded_grain_reader->base ());
        if (http) {
          //  async reading on HTTP
          http->ready ().add (this, &SaltManagerDialog::data_ready);
          http->send ();
        } else {
          data_ready ();
        }

      } else {

        m_downloaded_grain.reset (new SaltGrain (sg->second));
        data_ready ();

      }

    } catch (tl::Exception &ex) {
      show_error (ex);
    }

  } else {

    //  Download denied - take information from index
    m_downloaded_grain.reset (new SaltGrain (*g));
    data_ready ();

  }
}

void
SaltManagerDialog::data_ready ()
{
  if (! m_salt_mine_grain.get () || ! m_downloaded_grain.get () || ! mp_downloaded_target) {
    return;
  }

  //  Load the grain file (save URL as it is overwritten by the grain.xml content)
  std::string url = m_downloaded_grain->url ();
  if (m_downloaded_grain_reader.get ()) {
    m_downloaded_grain->load (*m_downloaded_grain_reader);
    m_downloaded_grain->set_url (url);
  }

  //  commit to cache
  if (m_salt_grain_cache.find (url) == m_salt_grain_cache.end ()) {
    m_salt_grain_cache [url] = *m_downloaded_grain;
  }

  try {

    if (m_salt_mine_grain->name () != m_downloaded_grain->name ()) {
      throw tl::Exception (tl::to_string (tr ("Name mismatch between repository and actual package (repository: %1, package: %2)").arg (tl::to_qstring (m_salt_mine_grain->name ())).arg (tl::to_qstring (m_downloaded_grain->name ()))));
    }
    if (SaltGrain::compare_versions (m_salt_mine_grain->version (), m_downloaded_grain->version ()) != 0) {
      throw tl::Exception (tl::to_string (tr ("Version mismatch between repository and actual package (repository: %1, package: %2)").arg (tl::to_qstring (m_salt_mine_grain->version ())).arg (tl::to_qstring (m_downloaded_grain->version ()))));
    }

    mp_downloaded_target->set_grain (m_downloaded_grain.get ());

    m_downloaded_grain.reset (0);
    if (m_downloaded_grain_reader.get ()) {
      //  NOTE: don't delete the reader in the slot it triggered
      m_downloaded_grain_reader->close ();
    }
    m_salt_mine_grain.reset (0);

  } catch (tl::Exception &ex) {
    m_downloaded_grain.reset (0);
    show_error (ex);
  }
}

void
SaltManagerDialog::show_error (tl::Exception &ex)
{
  QString html = tr (
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
  .arg (tl::to_qstring (m_downloaded_grain.get () ? m_downloaded_grain->url () : ""))
  .arg (tl::to_qstring (tl::escaped_to_html (ex.msg ())));
  mp_downloaded_target->setHtml (html);

  m_downloaded_grain.reset (0);
  if (m_downloaded_grain_reader.get ()) {
    //  NOTE: don't delete the reader in the slot it triggered
    m_downloaded_grain_reader->close();
  }
  m_salt_mine_grain.reset (0);
}

}
