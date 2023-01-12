
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


#ifndef HDR_dbRS274XReader
#define HDR_dbRS274XReader

#include "tlStream.h"
#include "tlString.h"
#include "dbShapes.h"
#include "dbTrans.h"
#include "dbPolygon.h"
#include "dbEdgeProcessor.h"
#include "dbGerberImporter.h"

#include <string>
#include <map>
#include <vector>

namespace db
{

// ---------------------------------------------------------------------------------
//  RS274XReader definition

class RS274XApertureBase;

class RS274XReader 
  : public GerberFileReader
{
public:
  RS274XReader (int warn_level);
  ~RS274XReader ();

  double um (double u)
  {
    return u * unit ();
  }

  virtual bool does_accept ();

protected:
  virtual void do_read ();
  GerberMetaData do_scan ();

private:
  bool m_clear;
  bool m_guess_polarity;
  bool m_neg_polarity;
  bool m_360deg_circular;
  double m_x, m_y;
  bool m_relative;
  std::string m_buffer;
  int m_current_gcode;
  int m_current_dcode;
  bool m_polygon_mode;
  std::vector<db::DPoint> m_polygon_points;
  std::vector<db::Polygon> m_polygons;
  std::vector<db::Polygon> m_clear_polygons;
  std::vector<RS274XApertureBase *> m_apertures;
  std::map<std::string, std::string> m_aperture_macros;
  enum { ab_xy, ab_yx } m_axis_mapping;
  RS274XApertureBase *m_current_aperture;
  std::string m_net_name;

  void read_as_parameter (const std::string &block);
  void read_fs_parameter (const std::string &block);
  void read_mi_parameter (const std::string &block);
  void read_mo_parameter (const std::string &block);
  void read_of_parameter (const std::string &block);
  void read_sf_parameter (const std::string &block);
  void read_ij_parameter (const std::string &block);
  void read_in_parameter (const std::string &block);
  void read_io_parameter (const std::string &block);
  void read_ip_parameter (const std::string &block);
  void read_ir_parameter (const std::string &block);
  void read_pf_parameter (const std::string &block);
  void read_ad_parameter (const std::string &block);
  void read_am_parameter (const std::string &block);
  void read_ko_parameter (const std::string &block);
  void read_lm_parameter (const std::string &block);
  void read_lr_parameter (const std::string &block);
  void read_ls_parameter (const std::string &block);
  void read_ln_parameter (const std::string &block);
  void read_lp_parameter (const std::string &block);
  void read_sr_parameter (const std::string &block);
  void read_if_parameter (const std::string &block);
  bool read_net_name (const std::string &block, std::string &net_name) const;
  void install_block_aperture (const std::string &dcode, const db::Region &region);
  void process_mcode (int mcode);
  const std::string &get_block ();
  bool is_clear_polarity ();
  void init ();
};

}

#endif

