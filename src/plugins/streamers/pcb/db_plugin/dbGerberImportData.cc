
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

#include "dbGerberImportData.h"
#include "dbGerberImporter.h"
#include "dbPoint.h"
#include "dbConverters.h"

#include "tlFileUtils.h"
#include "tlXMLParser.h"

namespace db
{

// -----------------------------------------------------------------------------------------
//  GerberImportData implementation

GerberImportData::GerberImportData ()
  : invert_negative_layers (false), border (5000),
    free_layer_mapping (false), mode (ModeSamePanel), mounting (MountingTop),
    num_metal_layers (0), num_via_types (0), num_circle_points (-1),
    merge_flag (false), dbu (0.001), topcell_name ("PCB")
{
  // .. nothing yet ..
}

void
GerberImportData::reset ()
{
  double dbu_saved = dbu;
  std::string base_dir_saved = base_dir;
  bool free_layer_mapping_saved = free_layer_mapping;
  mode_type mode_saved = mode;

  *this = GerberImportData ();

  dbu = dbu_saved;
  base_dir = base_dir_saved;
  free_layer_mapping = free_layer_mapping_saved;
  mode = mode_saved;
}

std::string
GerberImportData::get_layer_properties_file () const
{
  std::string lyp_file = layer_properties_file;
  if (! lyp_file.empty ()) {
    if (! base_dir.empty () && ! tl::is_absolute (lyp_file)) {
      lyp_file = tl::absolute_file_path (tl::combine_path (base_dir, lyp_file));
    }
  }

  return lyp_file;
}

void
GerberImportData::setup_importer (db::GerberImporter *importer)
{
  if (num_circle_points >= 4) {
    importer->set_circle_points (num_circle_points);
  }

  importer->set_dbu (dbu);
  importer->set_cell_name (topcell_name);
  importer->set_dir (base_dir);
  importer->set_global_trans (explicit_trans);
  importer->set_reference_points (reference_points);
  importer->set_merge (merge_flag);
  importer->set_invert_negative_layers (invert_negative_layers);
  importer->set_border (border);

  if (free_layer_mapping) {

    for (std::vector<GerberFreeFileDescriptor>::iterator file = free_files.begin (); file != free_files.end (); ++file) {

      if (! file->filename.empty ()) {

        db::GerberFile file_spec;
        file_spec.set_filename (file->filename);

        for (std::vector <int>::const_iterator i = file->layout_layers.begin (); i != file->layout_layers.end (); ++i) {
          if (*i >= 0 && *i < int (layout_layers.size ())) {
            file_spec.add_layer_spec (layout_layers [*i]);
          }
        }

        importer->add_file (file_spec);

      }

    }

  } else {

    for (std::vector<GerberArtworkFileDescriptor>::iterator file = artwork_files.begin (); file != artwork_files.end (); ++file) {

      if (! file->filename.empty ()) {

        size_t n;
        if (mounting == MountingTop) {
          n = std::distance (artwork_files.begin (), file);
        } else {
          n = std::distance (file, artwork_files.end ()) - 1;
        }

        if (n * 2 < layout_layers.size ()) {
          db::GerberFile file_spec;
          file_spec.set_filename (file->filename);
          file_spec.add_layer_spec (layout_layers [n * 2]);
          importer->add_file (file_spec);
        }

      }

    }

    for (std::vector<GerberDrillFileDescriptor>::iterator file = drill_files.begin (); file != drill_files.end (); ++file) {

      if (! file->filename.empty ()) {

        size_t nstart, nstop;
        if (mounting == MountingTop) {
          nstart = std::distance (artwork_files.begin (), artwork_files.begin () + file->start);
          nstop = std::distance (artwork_files.begin (), artwork_files.begin () + file->stop);
        } else {
          nstop = std::distance (artwork_files.begin () + file->start, artwork_files.end ()) - 1;
          nstart = std::distance (artwork_files.begin () + file->stop, artwork_files.end ()) - 1;
        }

        db::GerberFile file_spec;
        file_spec.set_filename (file->filename);

        for (size_t n = nstart; n < nstop; ++n) {
          if (n * 2 + 1 < layout_layers.size ()) {
            file_spec.add_layer_spec (layout_layers [n * 2 + 1]);
          }
        }

        importer->add_file (file_spec);

      }

    }

  }
}

struct MountingConverter
{
  std::string to_string (GerberImportData::mounting_type m) const
  {
    return m == GerberImportData::MountingTop ? "top" : "bottom";
  }

  void from_string (const std::string &s, GerberImportData::mounting_type &m) const
  {
    if (s == "top") {
      m = GerberImportData::MountingTop;
    } else if (s == "bottom") {
      m = GerberImportData::MountingBottom;
    } else {
      throw tl::Exception (tl::to_string (tr ("Invalid mounting specification: %s")), s);
    }
  }
};

//  declaration of the layer properties file XML structure
static const tl::XMLStruct <GerberImportData>
pcb_project_structure ("pcb-project",
  tl::make_member (&GerberImportData::invert_negative_layers, "invert-negative-layers") +
  tl::make_member (&GerberImportData::border, "border") +
  tl::make_member (&GerberImportData::free_layer_mapping, "free-layer-mapping") +
  tl::make_element (&GerberImportData::layout_layers, "layout-layers",
    tl::make_member<db::LayerProperties, std::vector<db::LayerProperties>::const_iterator, std::vector<db::LayerProperties> > (&std::vector<db::LayerProperties>::begin, &std::vector<db::LayerProperties>::end, &std::vector<db::LayerProperties>::push_back, "layout-layer", db::LayoutLayerConverter ())
  ) +
  tl::make_member (&GerberImportData::mounting, "mounting", MountingConverter ()) +
  tl::make_member (&GerberImportData::num_metal_layers, "num-metal-layers") +
  tl::make_member (&GerberImportData::num_via_types, "num-via-types") +
  tl::make_element (&GerberImportData::artwork_files, "artwork-files",
    tl::make_element<GerberArtworkFileDescriptor, std::vector<GerberArtworkFileDescriptor>::const_iterator, std::vector<GerberArtworkFileDescriptor> > (&std::vector<GerberArtworkFileDescriptor>::begin, &std::vector<GerberArtworkFileDescriptor>::end, &std::vector<GerberArtworkFileDescriptor>::push_back, "artwork-file",
      tl::make_member (&GerberArtworkFileDescriptor::filename, "filename")
    )
  ) +
  tl::make_element (&GerberImportData::drill_files, "drill-files",
    tl::make_element<GerberDrillFileDescriptor, std::vector<GerberDrillFileDescriptor>::const_iterator, std::vector<GerberDrillFileDescriptor> > (&std::vector<GerberDrillFileDescriptor>::begin, &std::vector<GerberDrillFileDescriptor>::end, &std::vector<GerberDrillFileDescriptor>::push_back, "drill-file",
      tl::make_member (&GerberDrillFileDescriptor::start, "start") +
      tl::make_member (&GerberDrillFileDescriptor::stop, "stop") +
      tl::make_member (&GerberDrillFileDescriptor::filename, "filename")
    )
  ) +
  tl::make_element (&GerberImportData::free_files, "free-files",
    tl::make_element<GerberFreeFileDescriptor, std::vector<GerberFreeFileDescriptor>::const_iterator, std::vector<GerberFreeFileDescriptor> > (&std::vector<GerberFreeFileDescriptor>::begin, &std::vector<GerberFreeFileDescriptor>::end, &std::vector<GerberFreeFileDescriptor>::push_back, "free-file",
      tl::make_member (&GerberFreeFileDescriptor::filename, "filename") +
      tl::make_element (&GerberFreeFileDescriptor::layout_layers, "layout-layers",
        tl::make_member<int, std::vector<int>::const_iterator, std::vector<int> > (&std::vector<int>::begin, &std::vector<int>::end, &std::vector<int>::push_back, "index")
      )
    )
  ) +
  tl::make_element (&GerberImportData::reference_points, "reference-points",
    tl::make_element<std::pair <db::DPoint, db::DPoint>, std::vector<std::pair <db::DPoint, db::DPoint> >::const_iterator, std::vector<std::pair <db::DPoint, db::DPoint> > > (&std::vector<std::pair <db::DPoint, db::DPoint> >::begin, &std::vector<std::pair <db::DPoint, db::DPoint> >::end, &std::vector<std::pair <db::DPoint, db::DPoint> >::push_back, "reference-point",
      tl::make_member (&std::pair <db::DPoint, db::DPoint>::first, "pcb", db::PointConverter<db::DPoint> ()) +
      tl::make_member (&std::pair <db::DPoint, db::DPoint>::second, "layout", db::PointConverter<db::DPoint> ())
    )
  ) +
  tl::make_member (&GerberImportData::explicit_trans, "explicit-trans", db::TransformationConverter<db::DCplxTrans> ()) +
  tl::make_member (&GerberImportData::layer_properties_file, "layer-properties-file") +
  tl::make_member (&GerberImportData::num_circle_points, "num-circle-points") +
  tl::make_member (&GerberImportData::merge_flag, "merge-flag") +
  tl::make_member (&GerberImportData::dbu, "dbu") +
  tl::make_member (&GerberImportData::topcell_name, "cell-name")
);

void
GerberImportData::load (const std::string &file)
{
  reset ();
  current_file = file;
  tl::XMLFileSource in (file);
  pcb_project_structure.parse (in, *this);
}

void
GerberImportData::load (tl::InputStream &stream)
{
  reset ();
  current_file = std::string ();
  tl::XMLStreamSource in (stream);
  pcb_project_structure.parse (in, *this);
}

void
GerberImportData::save (const std::string &file)
{
  tl::OutputStream os (file, tl::OutputStream::OM_Plain);
  pcb_project_structure.write (os, *this);
  current_file = file;
}

void
GerberImportData::from_string (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  while (! ex.at_end ()) {

    if (ex.test ("free-layer-mapping")) {

      ex.test ("=");
      ex.read (free_layer_mapping);
      ex.test (";");

    } else if (ex.test ("invert-negative-layers")) {

      ex.test ("=");
      ex.read (invert_negative_layers);
      ex.test (";");

    } else if (ex.test ("border")) {

      ex.test ("=");
      ex.read (border);
      ex.test (";");

    } else if (ex.test ("import-mode")) {

      ex.test ("=");
      int m = 0;
      ex.read (m);
      mode = (mode_type) m;
      ex.test (";");

    } else if (ex.test ("base-dir")) {

      ex.test ("=");
      ex.read_word_or_quoted (base_dir);
      ex.test (";");

    } else if (ex.test ("layout-layers")) {

      ex.test ("=");
      layout_layers.clear ();
      while (! ex.test (";") && ! ex.at_end ()) {
        layout_layers.push_back (db::LayerProperties ());
        layout_layers.back ().read (ex);
        ex.test (",");
      }
      ex.test (";");

    } else if (ex.test ("mounting")) {

      ex.test ("=");
      std::string m;
      ex.read_word (m);
      mounting = (m == "top") ? MountingTop : MountingBottom;
      ex.test (";");

    } else if (ex.test ("num-metal-layers")) {

      ex.test ("=");
      ex.read (num_metal_layers);
      ex.test (";");

    } else if (ex.test ("num-via-types")) {

      ex.test ("=");
      ex.read (num_via_types);
      ex.test (";");

    } else if (ex.test ("artwork-files")) {

      ex.test ("=");
      artwork_files.clear ();
      while (! ex.test (";") && ! ex.at_end ()) {
        ex.test ("(");
        artwork_files.push_back (db::GerberArtworkFileDescriptor ());
        ex.read_word_or_quoted (artwork_files.back ().filename);
        ex.test (")");
        ex.test (",");
      }
      ex.test (";");

    } else if (ex.test ("drill-files")) {

      ex.test ("=");
      drill_files.clear ();
      while (! ex.test (";") && ! ex.at_end ()) {
        ex.test ("(");
        drill_files.push_back (db::GerberDrillFileDescriptor ());
        ex.read (drill_files.back ().start);
        ex.test (",");
        ex.read (drill_files.back ().stop);
        ex.test (",");
        ex.read_word_or_quoted (drill_files.back ().filename);
        ex.test (")");
        ex.test (",");
      }
      ex.test (";");

    } else if (ex.test ("free-files")) {

      ex.test ("=");
      free_files.clear ();
      while (! ex.test (";") && ! ex.at_end ()) {
        ex.test ("(");
        free_files.push_back (db::GerberFreeFileDescriptor ());
        ex.read_word_or_quoted (free_files.back ().filename);
        ex.test (",");
        while (! ex.test (")") && ! ex.at_end ()) {
          int i = -1;
          ex.read (i);
          free_files.back ().layout_layers.push_back (i);
          ex.test (",");
        }
        ex.test (",");
      }
      ex.test (";");

    } else if (ex.test ("reference-points")) {

      ex.test ("=");
      reference_points.clear ();
      while (! ex.test (";") && ! ex.at_end ()) {
        double x1, y1, x2, y2;
        ex.test ("(");
        ex.test ("(");
        ex.read (x1);
        ex.test (",");
        ex.read (y1);
        ex.test (")");
        ex.test (",");
        ex.test ("(");
        ex.read (x2);
        ex.test (",");
        ex.read (y2);
        ex.test (")");
        ex.test (")");
        ex.test (",");
        reference_points.push_back (std::make_pair (db::DPoint (x1, y1), db::DPoint (x2, y2)));
      }
      ex.test (";");

    } else if (ex.test ("explicit-trans")) {

      ex.test ("=");
      ex.read (explicit_trans);
      ex.test (";");

    } else if (ex.test ("layer-properties-file")) {

      ex.test ("=");
      ex.read_word_or_quoted (layer_properties_file);
      ex.test (";");

    } else if (ex.test ("num-circle-points")) {

      ex.test ("=");
      ex.read (num_circle_points);
      ex.test (";");

    } else if (ex.test ("merge-flag")) {

      ex.test ("=");
      ex.read (merge_flag);
      ex.test (";");

    } else if (ex.test ("dbu")) {

      ex.test ("=");
      ex.read (dbu);
      ex.test (";");

    } else if (ex.test ("cell-name")) {

      ex.test ("=");
      ex.read_word_or_quoted (topcell_name);
      ex.test (";");

    } else {
      ex.expect_end ();
    }

  }
}

std::string
GerberImportData::to_string () const
{
  std::string s;
  s += "free-layer-mapping=" + tl::to_string(free_layer_mapping) + ";";
  s += "import-mode=" + tl::to_string(int (mode)) + ";";
  s += "base-dir=" + tl::to_quoted_string (base_dir) + ";";
  s += "invert-negative-layers=" + tl::to_string (invert_negative_layers) + ";";
  s += "border=" + tl::to_string (border) + ";";

  s += "layout-layers=";
  for (std::vector <db::LayerProperties>::const_iterator ll = layout_layers.begin (); ll != layout_layers.end (); ++ll) {
    if (ll != layout_layers.begin ()) {
      s += ",";
    }
    s += ll->to_string ();
  }
  s += ";";

  s += "mounting=" + std::string (mounting == MountingTop ? "top" : "bottom") + ";";
  s += "num-metal-layers=" + tl::to_string (num_metal_layers) + ";";
  s += "num-via-types=" + tl::to_string (num_via_types) + ";";

  s += "artwork-files=";
  for (std::vector <GerberArtworkFileDescriptor>::const_iterator f = artwork_files.begin (); f != artwork_files.end (); ++f) {
    if (f != artwork_files.begin ()) {
      s += ",";
    }
    s += "(" + tl::to_quoted_string (f->filename) + ")";
  }
  s += ";";

  s += "drill-files=";
  for (std::vector <GerberDrillFileDescriptor>::const_iterator f = drill_files.begin (); f != drill_files.end (); ++f) {
    if (f != drill_files.begin ()) {
      s += ",";
    }
    s += "(" + tl::to_string (f->start) + "," + tl::to_string (f->stop) + "," + tl::to_quoted_string (f->filename) + ")";
  }
  s += ";";

  s += "free-files=";
  for (std::vector <GerberFreeFileDescriptor>::const_iterator f = free_files.begin (); f != free_files.end (); ++f) {
    if (f != free_files.begin ()) {
      s += ",";
    }
    s += "(" + tl::to_quoted_string (f->filename);
    for (std::vector <int>::const_iterator i = f->layout_layers.begin (); i != f->layout_layers.end (); ++i) {
      s += "," + tl::to_string (*i);
    }
    s += ")";
  }
  s += ";";

  s += "reference-points=";
  for (std::vector <std::pair <db::DPoint, db::DPoint> >::const_iterator rp = reference_points.begin (); rp != reference_points.end (); ++rp) {
    if (rp != reference_points.begin ()) {
      s += ",";
    }
    s += "((" + tl::to_string (rp->first.x ()) + "," + tl::to_string (rp->first.y ()) + "),("
              + tl::to_string (rp->second.x ()) + "," + tl::to_string (rp->second.y ()) + "))";
  }
  s += ";";

  s += "explicit-trans=" + explicit_trans.to_string () + ";";
  s += "layer-properties-file=" + tl::to_quoted_string (layer_properties_file) + ";";
  s += "num-circle-points=" + tl::to_string (num_circle_points) + ";";
  s += "merge-flag=" + tl::to_string (merge_flag) + ";";
  s += "dbu=" + tl::to_string (dbu) + ";";
  s += "cell-name=" + tl::to_quoted_string (topcell_name) + ";";

  return s;
}

}
