
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

#include "laySaltGrainPropertiesDialog.h"
#include "laySalt.h"
#include "tlString.h"
#include "tlExceptions.h"
#include "tlHttpStream.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QTreeWidgetItem>
#include <QItemDelegate>
#include <QPainter>
#include <QCompleter>
#include <QMessageBox>

#include <memory>
#include <map>
#include <set>

namespace lay
{

// ----------------------------------------------------------------------------------------------------

/**
 *  @brief A delegate for editing a field of the dependency list
 */
class SaltGrainEditDelegate
  : public QItemDelegate
{
public:
  SaltGrainEditDelegate (QWidget *parent)
    : QItemDelegate (parent)
  {
    //  .. nothing yet ..
  }

  QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
  {
    QLineEdit *editor = new QLineEdit (parent);
    editor->setFrame (false);
    editor->setTextMargins (2, 0, 2, 0);
    return editor;
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
  {
    editor->setGeometry(option.rect);
  }

  void setEditorData (QWidget *widget, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {
      editor->setText (index.model ()->data (index, Qt::UserRole).toString ());
    }
  }

  void setModelData (QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
  {
    QLineEdit *editor = dynamic_cast<QLineEdit *> (widget);
    if (editor) {
      model->setData (index, QVariant (editor->text ()), Qt::UserRole);
    }
  }

  QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
  {
    QSize sz = option.fontMetrics.size (Qt::TextSingleLine, QString::fromUtf8 ("M"));
    sz += QSize (0, 8);
    return sz;
  }
};

/**
 *  @brief A delegate for editing a field of the dependency list
 */
class SaltGrainNameEditDelegate
  : public SaltGrainEditDelegate
{
public:
  SaltGrainNameEditDelegate (QWidget *parent, Salt *salt)
    : SaltGrainEditDelegate (parent), mp_completer (0)
  {
    QStringList names;
    for (lay::Salt::flat_iterator i = salt->begin_flat (); i != salt->end_flat (); ++i) {
      names << tl::to_qstring ((*i)->name ());
    }
    mp_completer = new QCompleter (names, this);
  }

  QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
  {
    QWidget *editor = SaltGrainEditDelegate::createEditor (parent, option, index);
    QLineEdit *line_edit = dynamic_cast<QLineEdit *> (editor);
    if (line_edit) {
      line_edit->setCompleter (mp_completer);
    }
    return editor;
  }

public:
  QCompleter *mp_completer;
};

// ----------------------------------------------------------------------------------------------------
//  SaltGrainPropertiesDialog implementation

SaltGrainPropertiesDialog::SaltGrainPropertiesDialog (QWidget *parent)
  : QDialog (parent), mp_salt (0), m_update_enabled (true)
{
  Ui::SaltGrainPropertiesDialog::setupUi (this);

  m_title = windowTitle ();
  m_open_label = open_label->text ();

  connect (icon_delete_button, SIGNAL (clicked ()), this, SLOT (reset_icon ()));
  connect (icon_config_button, SIGNAL (clicked ()), this, SLOT (set_icon ()));
  connect (screenshot_delete_button, SIGNAL (clicked ()), this, SLOT (reset_screenshot ()));
  connect (screenshot_config_button, SIGNAL (clicked ()), this, SLOT (set_screenshot ()));
  connect (doc_url, SIGNAL (textChanged (const QString &)), this, SLOT (url_changed (const QString &)));
  connect (add_dependency, SIGNAL (clicked ()), this, SLOT (add_dependency_clicked ()));
  connect (remove_dependency, SIGNAL (clicked ()), this, SLOT (remove_dependency_clicked ()));
  connect (dependencies, SIGNAL (itemChanged (QTreeWidgetItem *, int)), this, SLOT (dependency_changed (QTreeWidgetItem *, int)));

  dependencies->setItemDelegateForColumn (1, new SaltGrainEditDelegate (dependencies));
  dependencies->setItemDelegateForColumn (2, new SaltGrainEditDelegate (dependencies));

  url_changed (QString ());
}

void
SaltGrainPropertiesDialog::update_controls ()
{
  setWindowTitle (m_title + tl::to_qstring (" - " + m_grain.name ()));
  license_alert->clear ();
  version_alert->clear ();
  doc_url_alert->clear ();
  dependencies_alert->clear ();

  version->setText (tl::to_qstring (m_grain.version ()));
  api_version->setText (tl::to_qstring (m_grain.api_version ()));
  title->setText (tl::to_qstring (m_grain.title ()));
  author->setText (tl::to_qstring (m_grain.author ()));
  author_contact->setText (tl::to_qstring (m_grain.author_contact ()));
  doc->setText (tl::to_qstring (m_grain.doc ()));
  doc_url->setText (tl::to_qstring (m_grain.doc_url ()));
  license->setText (tl::to_qstring (m_grain.license ()));

  dependencies->clear ();
  for (std::vector<SaltGrainDependency>::const_iterator d = m_grain.dependencies ().begin (); d != m_grain.dependencies ().end (); ++d) {

    QTreeWidgetItem *item = new QTreeWidgetItem (dependencies);
    item->setFlags (item->flags () | Qt::ItemIsEditable);

    item->setData (0, Qt::UserRole, tl::to_qstring (d->name));
    dependency_changed (item, 0);
    item->setData (1, Qt::UserRole, tl::to_qstring (d->version));
    dependency_changed (item, 1);
    item->setData (2, Qt::UserRole, tl::to_qstring (d->url));
    dependency_changed (item, 2);

    dependencies->addTopLevelItem (item);

  }

  update_icon ();
  update_screenshot ();
}

void
SaltGrainPropertiesDialog::update_icon ()
{
  if (m_grain.icon ().isNull ()) {
    icon_config_button->setIcon (QIcon (":/salt_icon.png"));
  } else {
    QImage img = m_grain.icon ();
    if (img.width () == icon_config_button->iconSize ().width ()) {
      icon_config_button->setIcon (QIcon (QPixmap::fromImage (img)));
    } else {
      icon_config_button->setIcon (QIcon (QPixmap::fromImage (img.scaled (icon_config_button->iconSize (), Qt::KeepAspectRatio, Qt::SmoothTransformation))));
    }
  }
}

void
SaltGrainPropertiesDialog::update_screenshot ()
{
  if (m_grain.screenshot ().isNull ()) {
    screenshot_config_button->setIcon (QIcon (":/add_16px.png"));
  } else {
    QImage img = m_grain.screenshot ();
    if (img.width () == screenshot_config_button->iconSize ().width ()) {
      screenshot_config_button->setIcon (QIcon (QPixmap::fromImage (img)));
    } else {
      screenshot_config_button->setIcon (QIcon (QPixmap::fromImage (img.scaled (screenshot_config_button->iconSize (), Qt::KeepAspectRatio, Qt::SmoothTransformation))));
    }
  }
}

void
SaltGrainPropertiesDialog::update_data ()
{
  m_grain.set_version (tl::to_string (version->text ()));
  m_grain.set_api_version (tl::to_string (api_version->text ()));
  m_grain.set_title (tl::to_string (title->text ()));
  m_grain.set_author (tl::to_string (author->text ()));
  m_grain.set_author_contact (tl::to_string (author_contact->text ()));
  m_grain.set_doc (tl::to_string (doc->toPlainText ()));
  m_grain.set_doc_url (tl::to_string (doc_url->text ()));
  m_grain.set_license (tl::to_string (license->text ()));

  m_grain.dependencies ().clear ();
  for (int i = 0; i < dependencies->topLevelItemCount (); ++i) {

    QTreeWidgetItem *item = dependencies->topLevelItem (i);
    QString name = item->data (0, Qt::UserRole).toString ().simplified ();
    QString version = item->data (1, Qt::UserRole).toString ().simplified ();
    QString url = item->data (2, Qt::UserRole).toString ().simplified ();

    if (! name.isEmpty ()) {
      lay::SaltGrainDependency dep = lay::SaltGrainDependency ();
      dep.name = tl::to_string (name);
      dep.version = tl::to_string (version);
      dep.url = tl::to_string (url);
      m_grain.dependencies ().push_back (dep);
    }

  }
}

void
SaltGrainPropertiesDialog::dependency_changed (QTreeWidgetItem *item, int column)
{
  if (! m_update_enabled) {
    return;
  }
  m_update_enabled = false;

  std::string name = tl::to_string (item->data (0, Qt::UserRole).toString ().simplified ());
  SaltGrain *g = mp_salt ? mp_salt->grain_by_name (name) : 0;

  if (column == 0 && mp_salt) {

    item->setData (0, Qt::EditRole, tl::to_qstring (name));

    //  set URL and version for known grains
    if (name == m_grain.name ()) {

      item->setData (1, Qt::UserRole, QString ());
      item->setData (2, Qt::UserRole, QString ());
      //  placeholder texts:
      item->setData (1, Qt::EditRole, QString ());
      item->setData (2, Qt::EditRole, tr ("(must not depend on itself)"));

    } else {

      if (g) {
        item->setData (1, Qt::UserRole, tl::to_qstring (g->version ()));
        item->setData (2, Qt::UserRole, QString ());
        //  placeholder texts:
        item->setData (1, Qt::EditRole, tl::to_qstring (g->version ()));
        if (! g->url ().empty ()) {
          item->setData (2, Qt::EditRole, tl::to_qstring ("(" + g->url () + ")"));
        } else {
          item->setData (2, Qt::EditRole, tr ("(from repository)"));
        }
      } else {
        item->setData (1, Qt::UserRole, QString ());
        item->setData (2, Qt::UserRole, QString ());
        //  placeholder texts:
        item->setData (1, Qt::EditRole, QString ());
        item->setData (2, Qt::EditRole, tr ("(from repository)"));
      }

    }

  } else if (column == 1) {

    QString text = item->data (column, Qt::UserRole).toString ();
    if (! text.isEmpty ()) {
      item->setData (1, Qt::EditRole, text);
    } else if (g) {
      item->setData (1, Qt::EditRole, tl::to_qstring (g->version ()));
    }

  } else if (column == 2) {

    QString text = item->data (column, Qt::UserRole).toString ();
    if (! text.isEmpty ()) {
      item->setData (2, Qt::EditRole, text);
    } else if (g) {
      if (! g->url ().empty ()) {
        item->setData (2, Qt::EditRole, tl::to_qstring ("(" + g->url () + ")"));
      } else {
        item->setData (2, Qt::EditRole, tr ("(from repository)"));
      }
    }

  }

  m_update_enabled = true;
}

void
SaltGrainPropertiesDialog::url_changed (const QString &url)
{
  //  inserts the URL into the label
  m_grain.set_doc_url (tl::to_string (url));
  open_label->setText (m_open_label.arg (tl::to_qstring (m_grain.eff_doc_url ())));
  open_label->setEnabled (! url.isEmpty ());
}

void
SaltGrainPropertiesDialog::set_icon ()
{
BEGIN_PROTECTED

  const int max_dim = 256;

  QString fileName = QFileDialog::getOpenFileName (this, tr ("Pick Icon Image File"), m_image_dir, tr ("Images (*.png *.jpg);;All Files (*)"));
  if (! fileName.isNull ()) {

    bool ok = true;
    QImage img = QImage (fileName);
    if (img.width () > max_dim || img.height () > max_dim) {
      if (QMessageBox::warning (this, tr ("Image Too Big"),
                                      tr ("Icon image too big - must be %1x%2 pixels max, but is %3x%4.\n\nScale image?").arg (max_dim).arg (max_dim).arg (img.width ()).arg (img.height ()),
                                      QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
        ok = false;
      } else {
        img = img.scaled (max_dim, max_dim, Qt::KeepAspectRatio);
      }
    }

    if (ok) {
      m_grain.set_icon (img);
      m_image_dir = QFileInfo (fileName).path ();
      update_icon ();
    }

  }

END_PROTECTED
}

void
SaltGrainPropertiesDialog::reset_icon ()
{
  m_grain.set_icon (QImage ());
  update_icon ();
}

void
SaltGrainPropertiesDialog::set_screenshot ()
{
BEGIN_PROTECTED

  const int max_dim = 1024;

  QString fileName = QFileDialog::getOpenFileName (this, tr ("Pick Showcase Image File"), m_image_dir, tr ("Images (*.png *.jpg);;All Files (*)"));
  if (! fileName.isNull ()) {

    bool ok = true;
    QImage img = QImage (fileName);
    if (img.width () > max_dim || img.height () > max_dim) {
      if (QMessageBox::warning (this, tr ("Image Too Big"),
                                      tr ("Showcase image too big - must be %1x%2 pixels max, but is %3x%4.\n\nScale image?").arg (max_dim).arg (max_dim).arg (img.width ()).arg (img.height ()),
                                      QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
        ok = false;
      } else {
        img = img.scaled (max_dim, max_dim, Qt::KeepAspectRatio);
      }
    }

    if (ok) {
      m_grain.set_screenshot (img);
      m_image_dir = QFileInfo (fileName).path ();
      update_screenshot ();
    }

  }

END_PROTECTED
}

void
SaltGrainPropertiesDialog::reset_screenshot ()
{
  m_grain.set_screenshot (QImage ());
  update_screenshot ();
}

void
SaltGrainPropertiesDialog::add_dependency_clicked ()
{
  QTreeWidgetItem *item = new QTreeWidgetItem (dependencies);
  item->setFlags (item->flags () | Qt::ItemIsEditable);
  dependencies->addTopLevelItem (item);
  dependencies->setCurrentItem (dependencies->topLevelItem (dependencies->topLevelItemCount () - 1));
}

void
SaltGrainPropertiesDialog::remove_dependency_clicked ()
{
  int index = dependencies->indexOfTopLevelItem (dependencies->currentItem ());
  if (index >= 0 && index < dependencies->topLevelItemCount ()) {
    delete dependencies->topLevelItem (index);
  }
}

namespace
{

class DependencyGraph
{
public:
  DependencyGraph (Salt *salt)
  {
    for (lay::Salt::flat_iterator i = salt->begin_flat (); i != salt->end_flat (); ++i) {
      m_name_to_grain.insert (std::make_pair ((*i)->name (), *i));
    }
  }

  bool is_valid_name (const std::string &name) const
  {
    return m_name_to_grain.find (name) != m_name_to_grain.end ();
  }

  const lay::SaltGrain *grain_for_name (const std::string &name) const
  {
    std::map <std::string, const lay::SaltGrain *>::const_iterator n = m_name_to_grain.find (name);
    if (n != m_name_to_grain.end ()) {
      return n->second;
    } else {
      return 0;
    }
  }

  void check_circular (const lay::SaltGrain *current, const lay::SaltGrain *new_dep)
  {
    std::vector <const lay::SaltGrain *> path;
    path.push_back (current);
    check_circular_follow (new_dep, path);
  }

private:
  std::map <std::string, const lay::SaltGrain *> m_name_to_grain;

  void check_circular_follow (const lay::SaltGrain *current, std::vector <const lay::SaltGrain *> &path)
  {
    if (! current) {
      return;
    }

    path.push_back (current);

    for (std::vector <const lay::SaltGrain *>::const_iterator p = path.begin (); p != path.end () - 1; ++p) {
      if (*p == current) {
        circular_reference_error (path);
      }
    }

    for (std::vector<SaltGrainDependency>::const_iterator d = current->dependencies ().begin (); d != current->dependencies ().end (); ++d) {
      check_circular_follow (grain_for_name (d->name), path);
    }

    path.pop_back ();
  }

  void circular_reference_error (std::vector <const lay::SaltGrain *> &path)
  {
    std::string msg = tl::to_string (QObject::tr ("The following path forms a circular dependency: "));
    for (std::vector <const lay::SaltGrain *>::const_iterator p = path.begin (); p != path.end (); ++p) {
      if (p != path.begin ()) {
        msg += "->";
      }
      msg += (*p)->name ();
    }
    throw tl::Exception (msg);
  }
};

}

void
SaltGrainPropertiesDialog::accept ()
{
  update_data ();

  //  Perform some checks

  //  license
  license_alert->clear ();
  if (m_grain.license ().empty ()) {
    license_alert->warn () << tr ("License field is empty. Please consider specifying a license model.") << tl::endl
                           << tr ("A license model tells users whether and how to use the source code of the package.");
  }

  //  version
  version_alert->clear ();
  if (m_grain.version ().empty ()) {
    version_alert->warn () << tr ("Version field is empty. Please consider specifying a version number.") << tl::endl
                           << tr ("Versions help the system to apply upgrades if required.");
  } else if (! SaltGrain::valid_version (m_grain.version ())) {
    version_alert->error () << tr ("'%1' is not a valid version string. A version string needs to be numeric (like '1.2.3' or '4.5'').").arg (tl::to_qstring (m_grain.version ()));
  }

  //  API version
  api_version_alert->clear ();
  if (! m_grain.api_version ().empty () && ! SaltGrain::valid_api_version (m_grain.api_version ())) {
    api_version_alert->error () << tr ("'%1' is not a valid API version string. An API version string needs to be a semicolon-separated list of features with optional numeric versions (like '0.26' or 'ruby 2.0; python').").arg (tl::to_qstring (m_grain.api_version ()));
  }

  //  doc URL
  doc_url_alert->clear ();
  if (! m_grain.doc_url ().empty ()) {
    try {
      tl::InputStream stream (m_grain.eff_doc_url ());
      if (! stream.get (1)) {
        throw tl::Exception (tl::to_string (tr ("Empty document")));
      }
    } catch (tl::Exception &ex) {
      doc_url_alert->error () << tr ("Attempt to read documentation URL failed. Error details follow.") << tl::endl
                              << tr ("URL: ") << m_grain.doc_url () << tl::endl
                              << tr ("Message: ") << ex.msg ();
    }
  }

  //  dependencies
  dependencies_alert->clear ();
  DependencyGraph dep (mp_salt);
  std::set <std::string> dep_seen;
  for (std::vector<SaltGrainDependency>::const_iterator d = m_grain.dependencies ().begin (); d != m_grain.dependencies ().end (); ++d) {

    if (! SaltGrain::valid_name (d->name)) {
      dependencies_alert->error () << tr ("'%1' is not a valid package name").arg (tl::to_qstring (d->name)) << tl::endl
                                   << tr ("Valid package names are words (letters, digits, underscores).") << tl::endl
                                   << tr ("Package groups can be specified in the form 'group/package'.");
      continue;
    }

    if (dep_seen.find (d->name) != dep_seen.end ()) {
      dependencies_alert->error () << tr ("Duplicate dependency '%1'").arg (tl::to_qstring (d->name)) << tl::endl
                                   << tr ("A package cannot be dependent on the same package twice. Remove on entry.");
      continue;
    }
    dep_seen.insert (d->name);

    if (dep.is_valid_name (d->name)) {
      try {
        dep.check_circular (dep.grain_for_name (m_grain.name ()), dep.grain_for_name (d->name));
      } catch (tl::Exception &ex) {
        dependencies_alert->error () << ex.msg () << tl::endl
                                     << tr ("Circular dependency means, a package is eventually depending on itself.");
      }
    }

    if (d->version.empty ()) {
      dependencies_alert->warn () << tr ("No version specified for dependency '%1'").arg (tl::to_qstring (d->name)) << tl::endl
                                  << tr ("Please consider giving a version here. Versions help deciding whether a package needs to be updated.") << tl::endl
                                  << tr ("If the dependency package has a version itself, the version is automatically set to its current version.");
    }

    if (!d->url.empty ()) {
      SaltGrain gdep;
      try {
        gdep = SaltGrain::from_url (d->url);
        if (gdep.name () != d->name) {
          dependencies_alert->error () << tr ("Package name obtained from download URL is not the expected name.") << tl::endl
                                       << tr ("Downloaded name: ") << gdep.name () << tl::endl
                                       << tr ("Expected name: ") << d->name;
        }
      } catch (tl::Exception &ex) {
        dependencies_alert->error () << tr ("Attempt to test-download package from URL failed. Error details follow.") << tl::endl
                                     << tr ("URL: ") << d->url << tl::endl
                                     << tr ("Message: ") << ex.msg ();
      }
    }

  }

  if (!license_alert->needs_attention () &&
      !doc_url_alert->needs_attention () &&
      !dependencies_alert->needs_attention () &&
      !version_alert->needs_attention () &&
      !api_version_alert->needs_attention ()) {
    QDialog::accept ();
  } else {
    if (QMessageBox::warning (this, tr ("Issues Encountered"),
                                    tr ("Some issues have been found when inspecting the package details.\nThe respective fields are marked with warning icons.\n\nIgnore these issues and commit the package details?"),
                                    QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
      QDialog::accept ();
    }
  }
}

bool
SaltGrainPropertiesDialog::exec_dialog (lay::SaltGrain *grain, lay::Salt *salt)
{
  m_grain = *grain;
  mp_salt = salt;

  dependencies->setItemDelegateForColumn (0, new SaltGrainNameEditDelegate (dependencies, mp_salt));

  update_controls ();

  bool res = exec ();
  if (res && *grain != m_grain) {
    *grain = m_grain;
    //  save modified grain
    grain->save ();
  }

  delete dependencies->itemDelegateForColumn (0);
  dependencies->setItemDelegateForColumn (0, 0);

  mp_salt = 0;
  return res;
}

}
