
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


#ifndef HDR_dbGerberImportData
#define HDR_dbGerberImportData

#include "dbPluginCommon.h"
#include "dbTrans.h"
#include "dbLayerProperties.h"

#include "tlStream.h"

#include <string>
#include <vector>


namespace db
{

class GerberImporter;

struct GerberArtworkFileDescriptor
{
  std::string filename;
};

struct GerberDrillFileDescriptor
{
  GerberDrillFileDescriptor () : start (-1), stop (-1) { }

  int start;
  int stop;
  std::string filename;
};

struct GerberFreeFileDescriptor
{
  std::string filename;
  std::vector<int> layout_layers;
};

struct DB_PLUGIN_PUBLIC GerberImportData
{
public:
  GerberImportData ();

  enum mode_type { ModeIntoLayout = 0, ModeSamePanel, ModeNewPanel };
  enum mounting_type { MountingTop = 0, MountingBottom };

  bool invert_negative_layers;
  double border;
  bool free_layer_mapping;
  mode_type mode;
  std::string base_dir;
  std::string current_file;
  std::vector <db::LayerProperties> layout_layers;
  mounting_type mounting;
  int num_metal_layers;
  int num_via_types;
  std::vector <GerberArtworkFileDescriptor> artwork_files;
  std::vector <GerberDrillFileDescriptor> drill_files;
  std::vector <GerberFreeFileDescriptor> free_files;
  std::vector <std::pair <db::DPoint, db::DPoint> > reference_points;
  db::DCplxTrans explicit_trans;
  std::string layer_properties_file;
  int num_circle_points;
  bool merge_flag;
  double dbu;
  std::string topcell_name;

  std::string get_layer_properties_file () const;
  void setup_importer (db::GerberImporter *importer);
  void reset ();
  void load (tl::InputStream &stream);
  void load (const std::string &file);
  void save (const std::string &file);
  void from_string (const std::string &s);
  std::string to_string () const;
};

}

#endif
