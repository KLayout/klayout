
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "dbGDS2.h"
#include "dbGDS2Format.h"
#include "dbGDS2Writer.h"
#include "dbSaveLayoutOptions.h"
#include "layCellView.h"
#include "layGDS2WriterPlugin.h"
#include "ui_GDS2WriterOptionPage.h"

#include <QFrame>

namespace lay
{

// ---------------------------------------------------------------
//  GDS2WriterOptionPage definition and implementation

GDS2WriterOptionPage::GDS2WriterOptionPage (QWidget *parent)
  : StreamWriterOptionsPage (parent)
{
  mp_ui = new Ui::GDS2WriterOptionPage ();
  mp_ui->setupUi (this);

  connect (mp_ui->multi_xy_cbx, SIGNAL (clicked ()), this, SLOT (multi_xy_clicked ()));
}

GDS2WriterOptionPage::~GDS2WriterOptionPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
GDS2WriterOptionPage::setup (const db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/)
{
  const db::GDS2WriterOptions *options = dynamic_cast<const db::GDS2WriterOptions *> (o);
  if (options) {
    mp_ui->write_timestamps->setChecked (options->write_timestamps);
    mp_ui->write_cell_properties->setChecked (options->write_cell_properties);
    mp_ui->write_file_properties->setChecked (options->write_file_properties);
    mp_ui->no_zero_length_paths->setChecked (options->no_zero_length_paths);
    mp_ui->multi_xy_cbx->setChecked (options->multi_xy_records);
    mp_ui->resolve_skew_arrays_cbx->setChecked (options->resolve_skew_arrays);
    mp_ui->max_vertex_le->setEnabled (! options->multi_xy_records);
    mp_ui->max_vertex_le->setText (tl::to_qstring (tl::to_string (options->max_vertex_count)));
    mp_ui->cell_name_length_le->setText (tl::to_qstring (tl::to_string (options->max_cellname_length)));
    mp_ui->default_text_size_le->setText (tl::to_qstring (options->default_text_size >= 0.0 ? tl::to_string (options->default_text_size) : std::string ()));
    mp_ui->libname_le->setText (tl::to_qstring (tl::to_string (options->libname)));
  }
}

void 
GDS2WriterOptionPage::commit (db::FormatSpecificWriterOptions *o, const db::Technology * /*tech*/, bool /*gzip*/)
{
  db::GDS2WriterOptions *options = dynamic_cast<db::GDS2WriterOptions *> (o);
  if (options) {

    unsigned int n;
    options->multi_xy_records = mp_ui->multi_xy_cbx->isChecked ();
    options->resolve_skew_arrays = mp_ui->resolve_skew_arrays_cbx->isChecked ();
    options->write_timestamps = mp_ui->write_timestamps->isChecked ();
    options->write_cell_properties = mp_ui->write_cell_properties->isChecked ();
    options->write_file_properties = mp_ui->write_file_properties->isChecked ();
    options->no_zero_length_paths = mp_ui->no_zero_length_paths->isChecked ();

    std::string ts_str = tl::to_string (mp_ui->default_text_size_le->text ());
    {
      double ts = -1;
      tl::Extractor ex (ts_str.c_str ());
      if (! ex.at_end ()) {
        tl::from_string_ext (ts_str, ts);
        if (ts < 0.0) {
          throw tl::Exception (tl::to_string (QObject::tr ("Default text size cannot be negative")));
        }
      }
      options->default_text_size = ts;
    }

    tl::from_string_ext (tl::to_string (mp_ui->max_vertex_le->text ()), n);
    if (! options->multi_xy_records) {
      if (n > 8191) {
        throw tl::Exception (tl::to_string (QObject::tr ("Maximum number of vertices must not exceed 8191")));
      }
      if (n < 4) {
        throw tl::Exception (tl::to_string (QObject::tr ("Maximum number of vertices must be 4 at least")));
      }
    }
    options->max_vertex_count = n;

    n = 32000;
    tl::from_string_ext (tl::to_string (mp_ui->cell_name_length_le->text ()), n);
    if (n > 32000) {
      throw tl::Exception (tl::to_string (QObject::tr ("Maximum cell name length must not exceed 32000")));
    }
    if (n < 8) {
      throw tl::Exception (tl::to_string (QObject::tr ("Maximum cell name length must be 8 at least")));
    }
    options->max_cellname_length = n;

    options->libname = tl::to_string (mp_ui->libname_le->text ());

  }
}

void 
GDS2WriterOptionPage::multi_xy_clicked ()
{
  mp_ui->max_vertex_le->setEnabled (! mp_ui->multi_xy_cbx->isChecked ());
}

// ---------------------------------------------------------------
//  GDS2WriterPluginDeclaration definition and implementation

namespace {

class GDS2WriterPluginDeclarationBase
  : public StreamWriterPluginDeclaration
{
public:
  GDS2WriterPluginDeclarationBase (const std::string &name)
    : StreamWriterPluginDeclaration (name)
  {
    // .. nothing yet ..
  }

  StreamWriterOptionsPage *format_specific_options_page (QWidget *parent) const
  {
    return new GDS2WriterOptionPage (parent);
  }

  db::FormatSpecificWriterOptions *create_specific_options () const
  {
    return new db::GDS2WriterOptions ();
  }

  void initialize_options_from_layout_handle (db::FormatSpecificWriterOptions *o, const lay::LayoutHandle &lh) const
  {
    //  Initialize the libname property from meta data with key "libname".
    db::GDS2WriterOptions *options = dynamic_cast<db::GDS2WriterOptions *> (o);
    if (options) {
      db::Layout::meta_info_name_id_type libname_name_id = lh.layout().meta_info_name_id ("libname");
      for (db::Layout::meta_info_iterator meta = lh.layout().begin_meta (); meta != lh.layout().end_meta (); ++meta) {
        if (meta->first == libname_name_id && !meta->second.value.is_nil ()) {
          options->libname = meta->second.value.to_string ();
        }
      }
    }
  }
};

}

/**
 *  @brief A plugin for GDS2
 */
class GDS2WriterPluginDeclaration
  : public GDS2WriterPluginDeclarationBase
{
public:
  GDS2WriterPluginDeclaration ()
    : GDS2WriterPluginDeclarationBase (db::GDS2WriterOptions ().format_name ())
  {
    // .. nothing yet ..
  }
};

/**
 *  @brief A plugin for GDS2Text
 *
 *  GDS2Text shares the options with GDS2, although some limitations do not exist.
 *  There is not specific option set for GDS2Text. The writer will take the options from GDS2.
 */
class GDS2TextWriterPluginDeclaration
  : public GDS2WriterPluginDeclarationBase
{
public:
  GDS2TextWriterPluginDeclaration ()
    : GDS2WriterPluginDeclarationBase ("GDS2Text")
  {
    // .. nothing yet ..
  }

  virtual const char *options_alias () const
  {
    return db::GDS2WriterOptions ().format_name ().c_str ();
  }

  StreamWriterOptionsPage *format_specific_options_page (QWidget *) const
  {
    return 0;
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl1 (new lay::GDS2WriterPluginDeclaration (), 10000, "GDS2Writer");
static tl::RegisteredClass<lay::PluginDeclaration> plugin_decl2 (new lay::GDS2TextWriterPluginDeclaration (), 10001, "GDS2TextWriter");

}
