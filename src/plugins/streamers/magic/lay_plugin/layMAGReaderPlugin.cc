
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbMAG.h"
#include "dbMAGReader.h"
#include "dbLoadLayoutOptions.h"
#include "layMAGReaderPlugin.h"
#include "ui_MAGReaderOptionPage.h"
#include "gsiDecl.h"

#include <QFrame>
#include <QFileDialog>

namespace lay
{

// ---------------------------------------------------------------
//  List manipulation utilities
//  @@@ TODO: move this to a central place
static void
refresh_item_flags (QListWidget *list)
{
  for (int i = 0; i < list->count (); ++i) {
    list->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
  }
}

static void
add_items_to_list (QListWidget *list, const QStringList &items)
{
  for (QStringList::const_iterator f = items.begin (); f != items.end (); ++f) {
    list->addItem (*f);
  }
  refresh_item_flags (list);
}

static void
delete_selected_items_from_list (QListWidget *list)
{
  QStringList items;
  for (int i = 0; i < list->count (); ++i) {
    if (! list->item (i)->isSelected ()) {
      items.push_back (list->item (i)->text ());
    }
  }

  list->clear ();
  for (QStringList::const_iterator f = items.begin (); f != items.end (); ++f) {
    list->addItem (*f);
  }
  refresh_item_flags (list);
}

static void
move_selected_items_up (QListWidget *list)
{
  std::set<QString> selected;
  for (int i = 0; i < list->count (); ++i) {
    if (list->item (i)->isSelected ()) {
      selected.insert (list->item (i)->text ());
    }
  }

  QStringList items;
  int j = -1;
  for (int i = 0; i < list->count (); ++i) {
    if (list->item (i)->isSelected ()) {
      items.push_back (list->item (i)->text ());
    } else {
      if (j >= 0) {
        items.push_back (list->item (j)->text ());
      }
      j = i;
    }
  }
  if (j >= 0) {
    items.push_back (list->item (j)->text ());
  }

  list->clear ();
  for (QStringList::const_iterator f = items.begin (); f != items.end (); ++f) {
    list->addItem (*f);
    if (selected.find (*f) != selected.end ()) {
      list->item (list->count () - 1)->setSelected (true);
    }
  }
  refresh_item_flags (list);
}

static void
move_selected_items_down (QListWidget *list)
{
  std::set<QString> selected;
  for (int i = 0; i < list->count (); ++i) {
    if (list->item (i)->isSelected ()) {
      selected.insert (list->item (i)->text ());
    }
  }

  QStringList items;
  int j = -1;
  for (int i = list->count (); i > 0; ) {
    --i;
    if (list->item (i)->isSelected ()) {
      items.push_back (list->item (i)->text ());
    } else {
      if (j >= 0) {
        items.push_back (list->item (j)->text ());
      }
      j = i;
    }
  }
  if (j >= 0) {
    items.push_back (list->item (j)->text ());
  }

  list->clear ();
  for (QStringList::const_iterator f = items.end (); f != items.begin (); ) {
    --f;
    list->addItem (*f);
    if (selected.find (*f) != selected.end ()) {
      list->item (list->count () - 1)->setSelected (true);
    }
  }
  refresh_item_flags (list);
}

// ---------------------------------------------------------------
//  MAGReaderOptionPage definition and implementation

MAGReaderOptionPage::MAGReaderOptionPage (QWidget *parent)
  : StreamReaderOptionsPage (parent)
{
  mp_ui = new Ui::MAGReaderOptionPage ();
  mp_ui->setupUi (this);

  connect (mp_ui->add_lib_path, SIGNAL (clicked ()), this, SLOT (add_lib_path_clicked ()));
  connect (mp_ui->add_lib_path_with_choose, SIGNAL (clicked ()), this, SLOT (add_lib_path_clicked_with_choose ()));
  connect (mp_ui->del_lib_path, SIGNAL (clicked ()), this, SLOT (del_lib_paths_clicked ()));
  connect (mp_ui->move_lib_path_up, SIGNAL (clicked ()), this, SLOT (move_lib_paths_up_clicked ()));
  connect (mp_ui->move_lib_path_down, SIGNAL (clicked ()), this, SLOT (move_lib_paths_down_clicked ()));
}

MAGReaderOptionPage::~MAGReaderOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MAGReaderOptionPage::setup (const db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  static const db::MAGReaderOptions default_options;
  const db::MAGReaderOptions *options = dynamic_cast<const db::MAGReaderOptions *> (o);
  if (!options) {
    options = &default_options;
  }

  mp_ui->dbu_le->setText (tl::to_qstring (tl::to_string (options->dbu)));
  mp_ui->lambda_le->setText (tl::to_qstring (tl::to_string (options->lambda)));
  mp_ui->layer_map->set_layer_map (options->layer_map);
  mp_ui->read_all_cbx->setChecked (options->create_other_layers);
  mp_ui->keep_names_cbx->setChecked (options->keep_layer_names);

  mp_ui->lib_path->clear ();
  QStringList items;
  for (std::vector <std::string>::const_iterator f = options->lib_paths.begin (); f != options->lib_paths.end (); ++f) {
    items << tl::to_qstring (*f);
  }
  add_items_to_list (mp_ui->lib_path, items);
}

void 
MAGReaderOptionPage::commit (db::FormatSpecificReaderOptions *o, const db::Technology * /*tech*/)
{
  db::MAGReaderOptions *options = dynamic_cast<db::MAGReaderOptions *> (o);
  if (options) {

    tl::from_string (tl::to_string (mp_ui->dbu_le->text ()), options->dbu);
    if (options->dbu > 1000.0 || options->dbu < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for database unit")));
    }

    tl::from_string (tl::to_string (mp_ui->lambda_le->text ()), options->lambda);
    if (options->lambda > 10000000.0 || options->lambda < 1e-9) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid value for lambda")));
    }

    options->layer_map = mp_ui->layer_map->get_layer_map ();
    options->create_other_layers = mp_ui->read_all_cbx->isChecked ();
    options->keep_layer_names = mp_ui->keep_names_cbx->isChecked ();

    options->lib_paths.clear ();
    options->lib_paths.reserve (mp_ui->lib_path->count ());
    for (int i = 0; i < mp_ui->lib_path->count (); ++i) {
      options->lib_paths.push_back (tl::to_string (mp_ui->lib_path->item (i)->text ()));
    }

  }
}

void
MAGReaderOptionPage::add_lib_path_clicked ()
{
  QStringList dirs;
  dirs << tr ("Enter your path here ..");
  add_items_to_list (mp_ui->lib_path, dirs);
  mp_ui->lib_path->clearSelection ();
  mp_ui->lib_path->setCurrentItem (mp_ui->lib_path->item (mp_ui->lib_path->count () - 1));
}

void
MAGReaderOptionPage::add_lib_path_clicked_with_choose ()
{
  QString dir = QFileDialog::getExistingDirectory (this, QObject::tr ("Add library path"));
  if (! dir.isNull ()) {
    QStringList dirs;
    dirs << dir;
    add_items_to_list (mp_ui->lib_path, dirs);
    mp_ui->lib_path->clearSelection ();
    mp_ui->lib_path->setCurrentItem (mp_ui->lib_path->item (mp_ui->lib_path->count () - 1));
  }
}

void
MAGReaderOptionPage::del_lib_paths_clicked ()
{
  delete_selected_items_from_list (mp_ui->lib_path);
}

void
MAGReaderOptionPage::move_lib_paths_up_clicked ()
{
  move_selected_items_up (mp_ui->lib_path);
}

void
MAGReaderOptionPage::move_lib_paths_down_clicked ()
{
  move_selected_items_down (mp_ui->lib_path);
}

// ---------------------------------------------------------------
//  MAGReaderPluginDeclaration definition and implementation

class MAGReaderPluginDeclaration
  : public StreamReaderPluginDeclaration
{
public:
  MAGReaderPluginDeclaration ()
    : StreamReaderPluginDeclaration (db::MAGReaderOptions ().format_name ())
  {
    // .. nothing yet ..
  }

  StreamReaderOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new MAGReaderOptionPage (parent);
  }

  db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return new db::MAGReaderOptions ();
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl (new lay::MAGReaderPluginDeclaration (), 10000, "MAGReader");

}





