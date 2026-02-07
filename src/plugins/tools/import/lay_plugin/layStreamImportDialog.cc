
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


#include "ui_StreamImportDialog.h"
#include "layStreamImporter.h"
#include "layStreamImportDialog.h"

#include "tlExceptions.h"
#include "layFileDialog.h"
#include "layDialogs.h"
#include "layConverters.h"
#include "layLoadLayoutOptionsDialog.h"

#include <QFileDialog>

#include <fstream>

namespace {

static struct {
  const char *string;
  lay::StreamImportData::mode_type value;
} mode_strings [] = {
  { "simple", lay::StreamImportData::Simple },
  { "instantiate", lay::StreamImportData::Instantiate },
  { "extra", lay::StreamImportData::Extra },
  { "merge", lay::StreamImportData::Merge }
};

struct ModeConverter
{
  std::string to_string (lay::StreamImportData::mode_type t) const
  {
    for (unsigned int i = 0; i < sizeof (mode_strings) / sizeof (mode_strings [0]); ++i) {
      if (mode_strings [i].value == t) {
        return mode_strings [i].string;
      }
    }
    return std::string ();
  }

  void from_string (const std::string &s, lay::StreamImportData::mode_type &t) const
  {
    for (unsigned int i = 0; i < sizeof (mode_strings) / sizeof (mode_strings [0]); ++i) {
      if (s == mode_strings [i].string) {
        t = mode_strings [i].value;
        return;
      }
    }

    t = mode_strings [0].value;
  }
};

static struct {
  const char *string;
  lay::StreamImportData::layer_mode_type value;
} layer_mode_strings [] = {
  { "original", lay::StreamImportData::Original },
  { "offset", lay::StreamImportData::Offset }
};

struct LayerModeConverter
{
  std::string to_string (lay::StreamImportData::layer_mode_type t) const
  {
    for (unsigned int i = 0; i < sizeof (layer_mode_strings) / sizeof (layer_mode_strings [0]); ++i) {
      if (layer_mode_strings [i].value == t) {
        return layer_mode_strings [i].string;
      }
    }
    return std::string ();
  }

  void from_string (const std::string &s, lay::StreamImportData::layer_mode_type &t) const
  {
    for (unsigned int i = 0; i < sizeof (layer_mode_strings) / sizeof (layer_mode_strings [0]); ++i) {
      if (s == layer_mode_strings [i].string) {
        t = layer_mode_strings [i].value;
        return;
      }
    }

    t = layer_mode_strings [0].value;
  }
};

}

namespace lay
{

// -----------------------------------------------------------------------------------------
//  StreamImportData implementation

StreamImportData::StreamImportData ()
  : mode (Simple), layer_mode (Original)
{
  // .. nothing yet ..
}

void 
StreamImportData::setup_importer (StreamImporter *importer)
{
  importer->set_global_trans (explicit_trans);
  importer->set_reference_points (reference_points);
  importer->set_cell_mapping (mode);
  importer->set_layer_mapping (layer_mode);
  importer->set_files (files);
  importer->set_topcell (topcell);
  importer->set_layer_offset (layer_offset);
  importer->set_reader_options (options);
}

static tl::XMLElementList xml_elements ()
{
  typedef std::pair <db::DPoint, db::DPoint> ref_point;
  typedef std::vector<ref_point> ref_point_v;
  typedef std::vector<std::string> string_v;

  return
    tl::make_element (&StreamImportData::files, "files",
      tl::make_member<std::string, string_v::const_iterator, string_v> (&string_v::begin, &string_v::end, &string_v::push_back, "file")
    ) +
    tl::make_member (&StreamImportData::topcell, "cell-name") +
    tl::make_member (&StreamImportData::layer_offset, "layer-offset") +
    tl::make_member (&StreamImportData::layer_mode, "layer-mode", LayerModeConverter ()) +
    tl::make_member (&StreamImportData::mode, "import-mode", ModeConverter ()) +
    tl::make_element (&StreamImportData::reference_points, "reference-points",
      tl::make_element<ref_point, ref_point_v::const_iterator, ref_point_v> (&ref_point_v::begin, &ref_point_v::end, &ref_point_v::push_back, "reference-point",
        tl::make_member (&ref_point::first, "p1") +
        tl::make_member (&ref_point::second, "p2")
      )
    ) +
    tl::make_member (&StreamImportData::explicit_trans, "explicit-trans") +
    tl::make_element (&StreamImportData::options, "options",
      db::load_options_xml_element_list ()
    );
}

void  
StreamImportData::from_string (const std::string &s)
{
  *this = StreamImportData ();

  tl::XMLStringSource source (s);
  tl::XMLStruct<StreamImportData> xml_struct ("stream-import-data", xml_elements ());
  xml_struct.parse (source, *this);
}

std::string
StreamImportData::to_string () const
{
  tl::OutputStringStream os;
  tl::XMLStruct<StreamImportData> xml_struct ("stream-import-data", xml_elements ());
  tl::OutputStream oss (os);
  xml_struct.write (oss, *this);
  return os.string ();
}

// -----------------------------------------------------------------------------------------
//  StreamImportDialog implementation

StreamImportDialog::StreamImportDialog (QWidget *parent, StreamImportData *data)
  : QDialog (parent), mp_data (data)
{
  mp_ui = new Ui::StreamImportDialog ();
  mp_ui->setupUi (this);

  connect (mp_ui->last_pb, SIGNAL (clicked ()), this, SLOT (last_page ()));
  connect (mp_ui->next_pb, SIGNAL (clicked ()), this, SLOT (next_page ()));
  connect (mp_ui->file_pb, SIGNAL (clicked ()), this, SLOT (browse_filename ()));
  connect (mp_ui->edit_options_pb, SIGNAL (clicked ()), this, SLOT (edit_options ()));
  connect (mp_ui->reset_options_pb, SIGNAL (clicked ()), this, SLOT (reset_options ()));
  connect (mp_ui->reset_pb, SIGNAL (clicked ()), this, SLOT (reset ()));
  connect (mp_ui->offset_rb, SIGNAL (clicked ()), this, SLOT (mapping_changed ()));
  connect (mp_ui->no_mapping_rb, SIGNAL (clicked ()), this, SLOT (mapping_changed ()));
}

StreamImportDialog::~StreamImportDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
StreamImportDialog::edit_options ()
{
  lay::LoadLayoutOptionsDialog dialog (this, tl::to_string (QObject::tr ("Import Layout Options")));

  dialog.get_options (mp_data->options);
}

void
StreamImportDialog::reset_options ()
{
  mp_data->options = db::LoadLayoutOptions ();
}

void  
StreamImportDialog::browse_filename ()
{
  QStringList files = mp_ui->files_te->toPlainText ().split (QString::fromUtf8 ("\n"));
  QString file;
  if (! files.isEmpty ()) {
    file = files.front ();
  }
  files = QFileDialog::getOpenFileNames (this, QObject::tr ("Select Files To Import"), file, QObject::tr ("All files (*)"));
  if (! files.isEmpty ()) {
    mp_ui->files_te->setPlainText (files.join (QString::fromUtf8 ("\n")));
  }
}

void 
StreamImportDialog::reject ()
{
  try {
    commit_page ();
  } catch (...) {
    // .. nothing yet ..
  }

  QDialog::reject ();
}

void 
StreamImportDialog::accept ()
{
  BEGIN_PROTECTED

  commit_page ();
  QDialog::accept ();

  END_PROTECTED
}

int 
StreamImportDialog::exec ()
{
  mp_ui->central_stack->setCurrentIndex (0);
  update ();

  return QDialog::exec ();
}

/* 
 * 0 - General
 * 1 - Layers 
 * 2 - Reference points
 */
static int next_pages[] = {  1, 2, -1 };
static int prev_pages[] = { -1, 0, 1  };

void
StreamImportDialog::next_page ()
{
  BEGIN_PROTECTED 
  commit_page ();

  int index = mp_ui->central_stack->currentIndex ();
  if (index >= 0 && index < int (sizeof (next_pages) / sizeof (next_pages [0]))) {
    index = next_pages [index];
    if (index >= 0) {
      mp_ui->central_stack->setCurrentIndex (index);
      enter_page ();
    }
  } 

  update ();
  END_PROTECTED
}

void 
StreamImportDialog::last_page ()
{
  // "safe" commit
  try {
    commit_page ();
  } catch (...) { }

  BEGIN_PROTECTED 

  int index = mp_ui->central_stack->currentIndex ();
  if (index >= 0 && index < int (sizeof (next_pages) / sizeof (next_pages [0]))) {
    index = prev_pages [index];
    if (index >= 0) {
      mp_ui->central_stack->setCurrentIndex (index);
    }
  } 

  update ();
  END_PROTECTED
}

void
StreamImportDialog::enter_page ()
{
  // .. nothing yet ..
}

void 
StreamImportDialog::commit_page ()
{
  int page = mp_ui->central_stack->currentIndex ();

  if (page == 0) {

    //  --- General page
    mp_data->files = tl::split (tl::to_string (mp_ui->files_te->toPlainText ()), "\n");
    mp_data->topcell = tl::to_string (mp_ui->topcell_le->text ());
    if (mp_ui->import_simple_rb->isChecked ()) {
      mp_data->mode = StreamImportData::Simple;
    } else if (mp_ui->import_instantiate_rb->isChecked ()) {
      mp_data->mode = StreamImportData::Instantiate;
    } else if (mp_ui->import_extra_rb->isChecked ()) {
      mp_data->mode = StreamImportData::Extra;
    } else if (mp_ui->import_merge_rb->isChecked ()) {
      mp_data->mode = StreamImportData::Merge;
    }

  } else if (page == 1) {

    //  --- Layer mapping
    if (mp_ui->no_mapping_rb->isChecked ()) {
      mp_data->layer_mode = StreamImportData::Original;
    } else if (mp_ui->offset_rb->isChecked ()) {
      mp_data->layer_mode = StreamImportData::Offset;
    }

    mp_data->layer_offset = db::LayerOffset ();
    std::string offset = tl::to_string (mp_ui->offset_le->text ());
    tl::Extractor ex (offset.c_str ());
    mp_data->layer_offset.read (ex);

  } else if (page == 2) {

    //  --- Coordinate Mapping page
    QLineEdit *(coord_editors[][4]) = {
      { mp_ui->pcb_x1_le, mp_ui->pcb_y1_le, mp_ui->layout_x1_le, mp_ui->layout_y1_le },
      { mp_ui->pcb_x2_le, mp_ui->pcb_y2_le, mp_ui->layout_x2_le, mp_ui->layout_y2_le },
      { mp_ui->pcb_x3_le, mp_ui->pcb_y3_le, mp_ui->layout_x3_le, mp_ui->layout_y3_le }
    };

    mp_data->reference_points.clear ();
    for (unsigned int i = 0; i < sizeof (coord_editors) / sizeof (coord_editors [0]); ++i) {

      std::string t_pcb_x (tl::to_string (coord_editors [i][0]->text ()));
      tl::Extractor pcb_x (t_pcb_x.c_str ());

      std::string t_pcb_y (tl::to_string (coord_editors [i][1]->text ()));
      tl::Extractor pcb_y (t_pcb_y.c_str ());

      std::string t_layout_x (tl::to_string (coord_editors [i][2]->text ()));
      tl::Extractor layout_x (t_layout_x.c_str ());

      std::string t_layout_y (tl::to_string (coord_editors [i][3]->text ()));
      tl::Extractor layout_y (t_layout_y.c_str ());

      if (pcb_x.at_end () || pcb_y.at_end () || layout_x.at_end () || layout_y.at_end ()) {

        if (! pcb_x.at_end () || ! pcb_y.at_end () || ! layout_x.at_end () || ! layout_y.at_end ()) {
          throw tl::Exception (tl::to_string (QObject::tr ("All coordinates (imported and existing layout) must be specified for a reference point")));
        }

      } else {

        double x, y;

        pcb_x.read (x);
        pcb_x.expect_end ();
        pcb_y.read (y);
        pcb_y.expect_end ();
        db::DPoint pcb (x, y);

        layout_x.read (x);
        layout_x.expect_end ();
        layout_y.read (y);
        layout_y.expect_end ();
        db::DPoint layout (x, y);

        mp_data->reference_points.push_back (std::make_pair (pcb, layout));

      }
    }

    std::string t (tl::to_string (mp_ui->explicit_trans_le->text ()));
    tl::Extractor ex (t.c_str ());
    mp_data->explicit_trans = db::DCplxTrans ();
    if (! ex.at_end ()) {
      ex.read (mp_data->explicit_trans);
      ex.expect_end ();
    }

  } 
}

void 
StreamImportDialog::update ()
{
  std::string section_headers[] = {
    tl::to_string (QObject::tr ("General")),
    tl::to_string (QObject::tr ("Layers")),
    tl::to_string (QObject::tr ("Coordinate Mapping")),
  };

  int page = mp_ui->central_stack->currentIndex ();
  if (page < 0 || page >= int (sizeof (section_headers) / sizeof (section_headers [0]))) {
    return;
  }

  mp_ui->last_pb->setEnabled (page > 0);
  mp_ui->next_pb->setEnabled (page < int (sizeof (section_headers) / sizeof (section_headers [0]) - 1));
  mp_ui->section_header_lbl->setText (tl::to_qstring (section_headers [page]));

  //  --- General page
  mp_ui->files_te->setPlainText (tl::to_qstring (tl::join (mp_data->files, "\n")));
  mp_ui->topcell_le->setText (tl::to_qstring (mp_data->topcell));
  mp_ui->import_simple_rb->setChecked (mp_data->mode == StreamImportData::Simple);
  mp_ui->import_extra_rb->setChecked (mp_data->mode == StreamImportData::Extra);
  mp_ui->import_instantiate_rb->setChecked (mp_data->mode == StreamImportData::Instantiate);
  mp_ui->import_merge_rb->setChecked (mp_data->mode == StreamImportData::Merge);

  //  --- Layers page
  mp_ui->no_mapping_rb->setChecked (mp_data->layer_mode == StreamImportData::Original);
  mp_ui->offset_rb->setChecked (mp_data->layer_mode == StreamImportData::Offset);
  mp_ui->offset_le->setText (tl::to_qstring (mp_data->layer_offset.to_string ()));
  mapping_changed ();

  //  --- Coordinate Mapping page
  QLineEdit *(coord_editors[][4]) = {
    { mp_ui->pcb_x1_le, mp_ui->pcb_y1_le, mp_ui->layout_x1_le, mp_ui->layout_y1_le },
    { mp_ui->pcb_x2_le, mp_ui->pcb_y2_le, mp_ui->layout_x2_le, mp_ui->layout_y2_le },
    { mp_ui->pcb_x3_le, mp_ui->pcb_y3_le, mp_ui->layout_x3_le, mp_ui->layout_y3_le }
  };

  for (unsigned int i = 0; i < sizeof (coord_editors) / sizeof (coord_editors [0]); ++i) {
    if (mp_data->reference_points.size () > i) {
      coord_editors[i][0]->setText (tl::to_qstring (tl::to_string (mp_data->reference_points [i].first.x ())));
      coord_editors[i][1]->setText (tl::to_qstring (tl::to_string (mp_data->reference_points [i].first.y ())));
      coord_editors[i][2]->setText (tl::to_qstring (tl::to_string (mp_data->reference_points [i].second.x ())));
      coord_editors[i][3]->setText (tl::to_qstring (tl::to_string (mp_data->reference_points [i].second.y ())));
    } else {
      for (unsigned int j = 0; j < 4; ++j) {
        coord_editors[i][j]->setText (QString ());
      }
    }
  }

  if (mp_data->explicit_trans == db::DCplxTrans ()) {
    mp_ui->explicit_trans_le->setText (QString ());
  } else {
    mp_ui->explicit_trans_le->setText (tl::to_qstring (mp_data->explicit_trans.to_string ()));
  }
}

void 
StreamImportDialog::mapping_changed ()
{
  mp_ui->offset_le->setEnabled (mp_ui->offset_rb->isChecked ());
}

void 
StreamImportDialog::reset ()
{
  //  Commit everything that is not loaded
  try {
    commit_page ();
  } catch (...) {
    // ..
  }

  *mp_data = StreamImportData ();
  mp_ui->central_stack->setCurrentIndex (0);
  update ();
}

}

