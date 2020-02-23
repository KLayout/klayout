
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "dbLEFDEFImporter.h"

#include "tlExceptions.h"

#include "layLEFDEFImportDialogs.h"
#include "layLoadLayoutOptionsDialog.h"
#include "layQtTools.h"
#include "layMainWindow.h"
#include "tlString.h"

#include <QFileDialog>

namespace lay
{

// -----------------------------------------------------------------------------------------------
//  LEFDEF importer data

LEFDEFImportData::LEFDEFImportData ()
  : mode (0)
{
  //  .. nothing yet ..

}

void LEFDEFImportData::from_string (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  while (! ex.at_end ()) {

    if (ex.test ("file")) {

      ex.test ("=");
      ex.read_quoted (file);
      ex.test (";");

    } else if (ex.test ("lef-files")) {

      ex.test ("=");
      lef_files.clear ();
      while (! ex.test (";")) {
        ex.test (",");
        lef_files.push_back (std::string ());
        ex.read_quoted (lef_files.back ());
      }

    } else if (ex.test ("import-mode")) {

      ex.test ("=");
      ex.read (mode);
      ex.test (";");

    } else {
      break;
    }

  }
}

std::string  
LEFDEFImportData::to_string () const
{
  std::string s;
  s += "file=" + tl::to_quoted_string(file) + ";";
  if (! lef_files.empty ()) {
    s += "lef-files=";
    for (size_t i = 0; i < lef_files.size (); ++i) {
      if (i > 0) {
        s += ",";
      }
      s += tl::to_quoted_string(lef_files [i]);
    }
    s += ";";
  }
  s += "import-mode=" + tl::to_string(mode) + ";";
  return s;
}

// -----------------------------------------------------------------------------------------------
//  LEFDEF import options dialog

LEFDEFImportOptionsDialog::LEFDEFImportOptionsDialog (QWidget *parent, bool is_lef_dialog)
  : QDialog (parent), m_is_lef_dialog (is_lef_dialog)
{
  setupUi (this);

  lef_files_frame->setVisible (! is_lef_dialog);

  connect (browse_pb, SIGNAL (clicked ()), this, SLOT (browse_button_clicked ()));
  connect (reader_options_pb, SIGNAL (clicked ()), this, SLOT (tech_setup_button_clicked ()));
  connect (add_lef_file, SIGNAL (clicked ()), this, SLOT (add_lef_file_clicked ()));
  connect (del_lef_files, SIGNAL (clicked ()), this, SLOT (del_lef_files_clicked ()));
  connect (move_lef_files_up, SIGNAL (clicked ()), this, SLOT (move_lef_files_up_clicked ()));
  connect (move_lef_files_down, SIGNAL (clicked ()), this, SLOT (move_lef_files_down_clicked ()));

  lay::activate_help_links (help_label);

  setWindowTitle (tl::to_qstring (m_is_lef_dialog ? tl::to_string (QObject::tr ("Import LEF File")) : tl::to_string (QObject::tr ("Import DEF File"))));
}

int 
LEFDEFImportOptionsDialog::exec_dialog (LEFDEFImportData &data)
{
  file_le->setText (tl::to_qstring (data.file));

  for (std::vector <std::string>::const_iterator f = data.lef_files.begin (); f != data.lef_files.end (); ++f) {
    lef_files->addItem (tl::to_qstring (*f));
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
  
  replace_rb->setChecked (data.mode == 0);
  import_same_panel_rb->setChecked (data.mode == 2);
  import_new_panel_rb->setChecked (data.mode == 1);

  int result = QDialog::exec ();
  if (result) {

    data.file = tl::to_string (file_le->text ());

    data.lef_files.clear ();
    data.lef_files.reserve (lef_files->count ());
    for (int i = 0; i < lef_files->count (); ++i) {
      data.lef_files.push_back (tl::to_string (lef_files->item (i)->text ()));
    }
  
    data.mode = 0;
    if (import_same_panel_rb->isChecked ()) {
      data.mode = 2;
    } else if (import_new_panel_rb->isChecked ()) {
      data.mode = 1;
    }

  }

  return result;
}

void
LEFDEFImportOptionsDialog::add_lef_file_clicked ()
{
  std::string title, filters;
  title = tl::to_string (QObject::tr ("Add LEF Files"));
  filters = tl::to_string (QObject::tr ("LEF files (*.lef *.LEF *.lef.gz *.LEF.gz);;All files (*)"));
  QStringList files = QFileDialog::getOpenFileNames (this, tl::to_qstring (title), QString (), tl::to_qstring (filters));
  for (QStringList::const_iterator f = files.begin (); f != files.end (); ++f) {
    lef_files->addItem (*f);
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

void
LEFDEFImportOptionsDialog::del_lef_files_clicked ()
{
  QStringList files;
  for (int i = 0; i < lef_files->count (); ++i) {
    if (! lef_files->item (i)->isSelected ()) {
      files.push_back (lef_files->item (i)->text ());
    }
  }

  lef_files->clear ();
  for (QStringList::const_iterator f = files.begin (); f != files.end (); ++f) {
    lef_files->addItem (*f);
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

void
LEFDEFImportOptionsDialog::move_lef_files_up_clicked ()
{
  std::set<QString> selected;
  for (int i = 0; i < lef_files->count (); ++i) {
    if (lef_files->item (i)->isSelected ()) {
      selected.insert (lef_files->item (i)->text ());
    }
  }

  QStringList files;
  int j = -1;
  for (int i = 0; i < lef_files->count (); ++i) {
    if (lef_files->item (i)->isSelected ()) {
      files.push_back (lef_files->item (i)->text ());
    } else {
      if (j >= 0) {
        files.push_back (lef_files->item (j)->text ());
      }
      j = i;
    }
  }
  if (j >= 0) {
    files.push_back (lef_files->item (j)->text ());
  }

  lef_files->clear ();
  for (QStringList::const_iterator f = files.begin (); f != files.end (); ++f) {
    lef_files->addItem (*f);
    if (selected.find (*f) != selected.end ()) {
      lef_files->item (lef_files->count () - 1)->setSelected (true);
    }
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

void
LEFDEFImportOptionsDialog::move_lef_files_down_clicked ()
{
  std::set<QString> selected;
  for (int i = 0; i < lef_files->count (); ++i) {
    if (lef_files->item (i)->isSelected ()) {
      selected.insert (lef_files->item (i)->text ());
    }
  }

  QStringList files;
  int j = -1;
  for (int i = lef_files->count (); i > 0; ) {
    --i;
    if (lef_files->item (i)->isSelected ()) {
      files.push_back (lef_files->item (i)->text ());
    } else {
      if (j >= 0) {
        files.push_back (lef_files->item (j)->text ());
      }
      j = i;
    }
  }
  if (j >= 0) {
    files.push_back (lef_files->item (j)->text ());
  }

  lef_files->clear ();
  for (QStringList::const_iterator f = files.end (); f != files.begin (); ) {
    --f;
    lef_files->addItem (*f);
    if (selected.find (*f) != selected.end ()) {
      lef_files->item (lef_files->count () - 1)->setSelected (true);
    }
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

void
LEFDEFImportOptionsDialog::browse_button_clicked ()
{
  QString file = file_le->text ();
  std::string title, filters;
  if (m_is_lef_dialog) {
    title = tl::to_string (QObject::tr ("Import LEF File"));
    filters = tl::to_string (QObject::tr ("LEF files (*.lef *.LEF *.lef.gz *.LEF.gz);;All files (*)"));
  } else {
    title = tl::to_string (QObject::tr ("Import DEF File"));
    filters = tl::to_string (QObject::tr ("DEF files (*.def *.DEF *.def.gz *.DEF.gz);;All files (*)"));
  }

  file = QFileDialog::getOpenFileName (this, tl::to_qstring (title), file, tl::to_qstring (filters));
  if (! file.isNull ()) {

    file_le->setText (file);

    if (! m_is_lef_dialog) {

      //  Scan all LEF files near that DEF file
      lef_files->clear ();
      QDir dir = QFileInfo (file).absoluteDir ();
      QStringList lef_file_filters;
      lef_file_filters << QString::fromUtf8 ("*.lef") << QString::fromUtf8 ("*.LEF") << QString::fromUtf8 ("*.lef.gz") << QString::fromUtf8 ("*.LEF.gz");
      QStringList lef_file_list = dir.entryList (lef_file_filters, QDir::Readable | QDir::Files);
      for (QStringList::const_iterator f = lef_file_list.begin (); f != lef_file_list.end (); ++f) {
        lef_files->addItem (*f);
      }
      for (int i = 0; i < lef_files->count (); ++i) {
        lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
      }

    }

  }
}

void
LEFDEFImportOptionsDialog::tech_setup_button_clicked ()
{
BEGIN_PROTECTED

  std::string tech_name;
  tech_name = lay::MainWindow::instance ()->initial_technology ();
  if (! db::Technologies::instance ()->has_technology (tech_name)) {
    tech_name.clear (); // use default technology
  }

  db::Technology *tech = db::Technologies::instance ()->technology_by_name (tech_name);
  if (!tech) {
    return;
  }

  db::LoadLayoutOptions options = tech->load_layout_options ();

  //  calls the dialog and if successful, install the new options
  lay::SpecificLoadLayoutOptionsDialog dialog (this, &options, "LEFDEF");
  if (dialog.exec ()) {
    tech->set_load_layout_options (options);
  }

END_PROTECTED
}

// -----------------------------------------------------------------------------------------------
//  LEFDEF technology components editor

LEFDEFReaderOptionsEditor::LEFDEFReaderOptionsEditor (QWidget *parent)
  : lay::StreamReaderOptionsPage (parent), mp_tech (0)
{
  setupUi (this);

  connect (produce_net_names, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_inst_names, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_pin_names, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_outlines, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_placement_blockages, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_regions, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_via_geometry, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_pins, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_obstructions, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_blockages, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_routing, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (produce_labels, SIGNAL (stateChanged (int)), this, SLOT (checkbox_changed ()));
  connect (add_lef_file, SIGNAL (clicked ()), this, SLOT (add_lef_file_clicked ()));
  connect (del_lef_files, SIGNAL (clicked ()), this, SLOT (del_lef_files_clicked ()));
  connect (move_lef_files_up, SIGNAL (clicked ()), this, SLOT (move_lef_files_up_clicked ()));
  connect (move_lef_files_down, SIGNAL (clicked ()), this, SLOT (move_lef_files_down_clicked ()));

  lay::activate_help_links (help_label);
}

void 
LEFDEFReaderOptionsEditor::commit (db::FormatSpecificReaderOptions *options, const db::Technology * /*tech*/)
{
  db::LEFDEFReaderOptions *data = dynamic_cast<db::LEFDEFReaderOptions *> (options);
  if (! data) {
    return;
  }

  data->set_read_all_layers (read_all_cbx->isChecked ());
  data->set_layer_map (layer_map->get_layer_map ());
  data->set_produce_net_names (produce_net_names->isChecked ());
  data->set_produce_inst_names (produce_inst_names->isChecked ());
  data->set_produce_pin_names (produce_pin_names->isChecked ());

  double dbu_value = 0.0;
  tl::from_string (tl::to_string (dbu->text ()), dbu_value);
  if (dbu_value < 1e-7) {
    throw tl::Exception (tl::to_string (tr ("Invalid database unit value (must be non-null and positive)")));
  }
  data->set_dbu (dbu_value);

  //  parse the net property name (may throw an exception)
  {
    std::string np = tl::to_string (net_prop_name->text ());
    tl::Extractor ex (np.c_str ());
    tl::Variant v;
    ex.read (v);
    ex.expect_end ();
    data->set_net_property_name (v);
  }

  //  parse the inst property name (may throw an exception)
  {
    std::string np = tl::to_string (inst_prop_name->text ());
    tl::Extractor ex (np.c_str ());
    tl::Variant v;
    ex.read (v);
    ex.expect_end ();
    data->set_inst_property_name (v);
  }

  //  parse the pin property name (may throw an exception)
  {
    std::string np = tl::to_string (pin_prop_name->text ());
    tl::Extractor ex (np.c_str ());
    tl::Variant v;
    ex.read (v);
    ex.expect_end ();
    data->set_pin_property_name (v);
  }

  data->set_produce_cell_outlines (produce_outlines->isChecked ());
  data->set_cell_outline_layer (tl::to_string (outline_layer->text ()));
  data->set_produce_regions (produce_regions->isChecked ());
  data->set_region_layer (tl::to_string (region_layer->text ()));
  data->set_produce_placement_blockages (produce_placement_blockages->isChecked ());
  data->set_placement_blockage_layer (tl::to_string (placement_blockage_layer->text ()));
  data->set_produce_via_geometry (produce_via_geometry->isChecked ());
  data->set_via_geometry_suffix (tl::to_string (suffix_via_geometry->text ()));
  data->set_via_geometry_datatype (datatype_via_geometry->text ().toInt ());
  data->set_produce_pins (produce_pins->isChecked ());
  data->set_pins_suffix (tl::to_string (suffix_pins->text ()));
  data->set_pins_datatype (datatype_pins->text ().toInt ());
  data->set_produce_obstructions (produce_obstructions->isChecked ());
  data->set_obstructions_suffix (tl::to_string (suffix_obstructions->text ()));
  data->set_obstructions_datatype (datatype_obstructions->text ().toInt ());
  data->set_produce_blockages (produce_blockages->isChecked ());
  data->set_blockages_suffix (tl::to_string (suffix_blockages->text ()));
  data->set_blockages_datatype (datatype_blockages->text ().toInt ());
  data->set_produce_routing (produce_routing->isChecked ());
  data->set_routing_suffix (tl::to_string (suffix_routing->text ()));
  data->set_routing_datatype (datatype_routing->text ().toInt ());
  data->set_produce_labels (produce_labels->isChecked ());
  data->set_labels_suffix (tl::to_string (suffix_labels->text ()));
  data->set_labels_datatype (datatype_labels->text ().toInt ());

  data->clear_lef_files ();
  for (int i = 0; i < lef_files->count (); ++i) {
    data->push_lef_file (tl::to_string (lef_files->item (i)->text ()));
  }
}

void 
LEFDEFReaderOptionsEditor::setup (const db::FormatSpecificReaderOptions *options, const db::Technology *tech)
{
  static db::LEFDEFReaderOptions empty;
  const db::LEFDEFReaderOptions *data = dynamic_cast<const db::LEFDEFReaderOptions *> (options);
  if (! data) {
    data = &empty;
  }

  //  TODO: there should be a const weak ptr ...
  mp_tech.reset (const_cast<db::Technology *> (tech));

  dbu->setText (tl::to_qstring (tl::to_string (data->dbu ())));
  read_all_cbx->setChecked (data->read_all_layers ());
  layer_map->set_layer_map (data->layer_map ());
  produce_net_names->setChecked (data->produce_net_names ());
  net_prop_name->setText (tl::to_qstring (data->net_property_name ().to_parsable_string ()));
  produce_inst_names->setChecked (data->produce_inst_names ());
  inst_prop_name->setText (tl::to_qstring (data->inst_property_name ().to_parsable_string ()));
  produce_pin_names->setChecked (data->produce_pin_names ());
  pin_prop_name->setText (tl::to_qstring (data->pin_property_name ().to_parsable_string ()));
  produce_outlines->setChecked (data->produce_cell_outlines ());
  outline_layer->setText (tl::to_qstring (data->cell_outline_layer ()));
  produce_regions->setChecked (data->produce_regions ());
  region_layer->setText (tl::to_qstring (data->region_layer ()));
  produce_placement_blockages->setChecked (data->produce_placement_blockages ());
  placement_blockage_layer->setText (tl::to_qstring (data->placement_blockage_layer ()));
  produce_via_geometry->setChecked (data->produce_via_geometry ());
  suffix_via_geometry->setText (tl::to_qstring (data->via_geometry_suffix ()));
  datatype_via_geometry->setText (QString::number (data->via_geometry_datatype ()));
  produce_pins->setChecked (data->produce_pins ());
  suffix_pins->setText (tl::to_qstring (data->pins_suffix ()));
  datatype_pins->setText (QString::number (data->pins_datatype ()));
  produce_obstructions->setChecked (data->produce_obstructions ());
  suffix_obstructions->setText (tl::to_qstring (data->obstructions_suffix ()));
  datatype_obstructions->setText (QString::number (data->obstructions_datatype ()));
  produce_blockages->setChecked (data->produce_blockages ());
  suffix_blockages->setText (tl::to_qstring (data->blockages_suffix ()));
  datatype_blockages->setText (QString::number (data->blockages_datatype ()));
  produce_routing->setChecked (data->produce_routing ());
  suffix_routing->setText (tl::to_qstring (data->routing_suffix ()));
  datatype_routing->setText (QString::number (data->routing_datatype ()));
  produce_labels->setChecked (data->produce_labels ());
  suffix_labels->setText (tl::to_qstring (data->labels_suffix ()));
  datatype_labels->setText (QString::number (data->labels_datatype ()));

  checkbox_changed ();

  lef_files->clear ();
  for (std::vector <std::string>::const_iterator f = data->begin_lef_files (); f != data->end_lef_files (); ++f) {
    if (mp_tech) {
      lef_files->addItem (tl::to_qstring (mp_tech->correct_path (*f)));
    } else {
      lef_files->addItem (tl::to_qstring (*f));
    }
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

void  
LEFDEFReaderOptionsEditor::checkbox_changed ()
{
  net_prop_name->setEnabled (produce_net_names->isChecked ());
  inst_prop_name->setEnabled (produce_inst_names->isChecked ());
  pin_prop_name->setEnabled (produce_pin_names->isChecked ());
  outline_layer->setEnabled (produce_outlines->isChecked ());
  region_layer->setEnabled (produce_regions->isChecked ());
  placement_blockage_layer->setEnabled (produce_placement_blockages->isChecked ());
  suffix_via_geometry->setEnabled (produce_via_geometry->isChecked ());
  suffix_pins->setEnabled (produce_pins->isChecked ());
  suffix_obstructions->setEnabled (produce_obstructions->isChecked ());
  suffix_blockages->setEnabled (produce_blockages->isChecked ());
  suffix_routing->setEnabled (produce_routing->isChecked ());
  suffix_labels->setEnabled (produce_labels->isChecked ());
  datatype_via_geometry->setEnabled (produce_via_geometry->isChecked ());
  datatype_pins->setEnabled (produce_pins->isChecked ());
  datatype_obstructions->setEnabled (produce_obstructions->isChecked ());
  datatype_blockages->setEnabled (produce_blockages->isChecked ());
  datatype_routing->setEnabled (produce_routing->isChecked ());
  datatype_labels->setEnabled (produce_labels->isChecked ());
}

void
LEFDEFReaderOptionsEditor::add_lef_file_clicked ()
{
  std::string title, filters;
  title = tl::to_string (QObject::tr ("Add LEF Files"));
  filters = tl::to_string (QObject::tr ("LEF files (*.lef *.LEF *.lef.gz *.LEF.gz);;All files (*)"));
  QStringList files = QFileDialog::getOpenFileNames (this, tl::to_qstring (title), QString (), tl::to_qstring (filters));
  for (QStringList::const_iterator f = files.begin (); f != files.end (); ++f) {
    if (mp_tech) {
      lef_files->addItem (tl::to_qstring (mp_tech->correct_path (tl::to_string (*f))));
    } else {
      lef_files->addItem (*f);
    }
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

void
LEFDEFReaderOptionsEditor::del_lef_files_clicked ()
{
  QStringList files;
  for (int i = 0; i < lef_files->count (); ++i) {
    if (! lef_files->item (i)->isSelected ()) {
      files.push_back (lef_files->item (i)->text ());
    }
  }

  lef_files->clear ();
  for (QStringList::const_iterator f = files.begin (); f != files.end (); ++f) {
    lef_files->addItem (*f);
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

void
LEFDEFReaderOptionsEditor::move_lef_files_up_clicked ()
{
  std::set<QString> selected;
  for (int i = 0; i < lef_files->count (); ++i) {
    if (lef_files->item (i)->isSelected ()) {
      selected.insert (lef_files->item (i)->text ());
    }
  }

  QStringList files;
  int j = -1;
  for (int i = 0; i < lef_files->count (); ++i) {
    if (lef_files->item (i)->isSelected ()) {
      files.push_back (lef_files->item (i)->text ());
    } else {
      if (j >= 0) {
        files.push_back (lef_files->item (j)->text ());
      }
      j = i;
    }
  }
  if (j >= 0) {
    files.push_back (lef_files->item (j)->text ());
  }

  lef_files->clear ();
  for (QStringList::const_iterator f = files.begin (); f != files.end (); ++f) {
    lef_files->addItem (*f);
    if (selected.find (*f) != selected.end ()) {
      lef_files->item (lef_files->count () - 1)->setSelected (true);
    }
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

void
LEFDEFReaderOptionsEditor::move_lef_files_down_clicked ()
{
  std::set<QString> selected;
  for (int i = 0; i < lef_files->count (); ++i) {
    if (lef_files->item (i)->isSelected ()) {
      selected.insert (lef_files->item (i)->text ());
    }
  }

  QStringList files;
  int j = -1;
  for (int i = lef_files->count (); i > 0; ) {
    --i;
    if (lef_files->item (i)->isSelected ()) {
      files.push_back (lef_files->item (i)->text ());
    } else {
      if (j >= 0) {
        files.push_back (lef_files->item (j)->text ());
      }
      j = i;
    }
  }
  if (j >= 0) {
    files.push_back (lef_files->item (j)->text ());
  }

  lef_files->clear ();
  for (QStringList::const_iterator f = files.end (); f != files.begin (); ) {
    --f;
    lef_files->addItem (*f);
    if (selected.find (*f) != selected.end ()) {
      lef_files->item (lef_files->count () - 1)->setSelected (true);
    }
  }
  for (int i = 0; i < lef_files->count (); ++i) {
    lef_files->item (i)->setFlags (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
  }
}

}

