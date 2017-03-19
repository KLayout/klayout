
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

#include "laySaltGrainPropertiesDialog.h"
#include "laySalt.h"
#include "tlString.h"
#include "tlExceptions.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QTreeWidgetItem>

namespace lay
{

SaltGrainPropertiesDialog::SaltGrainPropertiesDialog (QWidget *parent)
  : QDialog (parent), mp_salt (0)
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
}

void
SaltGrainPropertiesDialog::update_controls ()
{
  setWindowTitle (m_title + tl::to_qstring (" - " + m_grain.name ()));

  version->setText (tl::to_qstring (m_grain.version ()));
  title->setText (tl::to_qstring (m_grain.title ()));
  author->setText (tl::to_qstring (m_grain.author ()));
  author_contact->setText (tl::to_qstring (m_grain.author_contact ()));
  doc->setText (tl::to_qstring (m_grain.doc ()));
  doc_url->setText (tl::to_qstring (m_grain.doc_url ()));
  license->setText (tl::to_qstring (m_grain.license ()));

  dependencies->clear ();
  for (std::vector<SaltGrain::Dependency>::const_iterator d = m_grain.dependencies ().begin (); d != m_grain.dependencies ().end (); ++d) {
    QTreeWidgetItem *item = new QTreeWidgetItem (dependencies);
    item->setFlags (item->flags () | Qt::ItemIsEditable);
    item->setText (0, tl::to_qstring (d->name));
    item->setText (1, tl::to_qstring (d->version));
    item->setText (2, tl::to_qstring (d->url));
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
    screenshot_config_button->setIcon (QIcon (":/add.png"));
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
  m_grain.set_title (tl::to_string (title->text ()));
  m_grain.set_author (tl::to_string (author->text ()));
  m_grain.set_author_contact (tl::to_string (author_contact->text ()));
  m_grain.set_doc (tl::to_string (doc->toPlainText ()));
  m_grain.set_doc_url (tl::to_string (doc_url->text ()));
  m_grain.set_license (tl::to_string (license->text ()));

  m_grain.dependencies ().clear ();
  for (int i = 0; i < dependencies->topLevelItemCount (); ++i) {
    QTreeWidgetItem *item = dependencies->topLevelItem (i);
    QString name = item->text (0).simplified ();
    QString version = item->text (1).simplified ();
    QString url = item->text (2).simplified ();
    if (! name.isEmpty ()) {
      lay::SaltGrain::Dependency dep = lay::SaltGrain::Dependency ();
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
  if (column == 0 && mp_salt) {

    //  set URL and version for known grains
    std::string name = tl::to_string (item->text (0).simplified ());
    if (name == m_grain.name ()) {

      item->setText (1, QString ());
      item->setText (2, tr ("(must not depend on itself)"));

    } else {

      SaltGrain *g = 0;
      for (lay::Salt::flat_iterator i = mp_salt->begin_flat (); i != mp_salt->end_flat (); ++i) {
        if ((*i)->name () == name) {
          g = *i;
        }
      }
      if (g) {
        item->setText (1, tl::to_qstring (g->version ()));
        item->setText (2, tl::to_qstring (g->url ()));
      } else {
        item->setText (1, QString ());
        item->setText (2, tr ("(unknown packet)"));
      }

    }

  }
}

void
SaltGrainPropertiesDialog::url_changed (const QString &url)
{
  //  inserts the URL into the label
  open_label->setText (m_open_label.arg (url));
}

void
SaltGrainPropertiesDialog::set_icon ()
{
BEGIN_PROTECTED

  const int max_dim = 256;

  QString fileName = QFileDialog::getOpenFileName (this, tr ("Pick Icon Image File"), m_image_dir, tr ("Images (*.png *.jpg)"));
  if (! fileName.isNull ()) {
    QImage img = QImage (fileName);
    if (img.width () > max_dim || img.height () > max_dim) {
      throw tl::Exception (tl::to_string (tr ("Icon image too big -\nmust be %1x%2 pixels max, but is %3x%4").arg (max_dim).arg (max_dim).arg (img.width ()).arg (img.height ())));
    }
    m_grain.set_icon (img);
    m_image_dir = QFileInfo (fileName).path ();
    update_icon ();
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

  QString fileName = QFileDialog::getOpenFileName (this, tr ("Pick Showcase Image File"), m_image_dir, tr ("Images (*.png *.jpg)"));
  if (! fileName.isNull ()) {
    QImage img = QImage (fileName);
    if (img.width () > max_dim || img.height () > max_dim) {
      throw tl::Exception (tl::to_string (tr ("Showcase image too big -\nmust be %1x%2 pixels max, but is %3x%4").arg (max_dim).arg (max_dim).arg (img.width ()).arg (img.height ())));
    }
    m_grain.set_screenshot (img);
    m_image_dir = QFileInfo (fileName).path ();
    update_screenshot ();
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

bool
SaltGrainPropertiesDialog::exec_dialog (lay::SaltGrain *grain, lay::Salt *salt)
{
  m_grain = *grain;
  mp_salt = salt;
  update_controls ();

  bool res = exec ();
  if (res) {
    update_data ();
    *grain = m_grain;
  }

  mp_salt = 0;
  return res;
}

}
