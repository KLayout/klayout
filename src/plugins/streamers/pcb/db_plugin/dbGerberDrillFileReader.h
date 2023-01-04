
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


#ifndef HDR_dbGerberDrillFileReader
#define HDR_dbGerberDrillFileReader

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

struct DrillHoleDescriptor
{
  DrillHoleDescriptor (double _x, double _y, double _r, double _ex, double _ey)
    : x(_x), y(_y), r(_r), ex(_ex), ey(_ey)
  { }

  double x, y, r, ex, ey;
};

// ---------------------------------------------------------------------------------
//  GerberDrillFileReader definition

class GerberDrillFileApertureBase;

class GerberDrillFileReader 
  : public GerberFileReader
{
public:
  GerberDrillFileReader (int warn_level);
  ~GerberDrillFileReader ();

  double um (double u)
  {
    return u * unit ();
  }

  virtual bool does_accept ();

protected:
  virtual void do_read ();
  GerberMetaData do_scan ();

private:
  std::string m_buffer;
  bool m_relative;
  bool m_format_set;
  double m_x, m_y;
  double m_xoff, m_yoff;
  double m_current_diameter;
  int m_current_qty;
  std::list <std::pair <long, double> > m_qty;
  int m_current_tool;
  std::map <int, double> m_tools;
  std::vector <DrillHoleDescriptor> m_holes;
  std::vector <DrillHoleDescriptor> m_pattern;
  size_t m_end_block;
  bool m_recording;
  bool m_record_pattern;
  bool m_in_header;
  double m_m02_xoffset, m_m02_yoffset;
  bool m_routing;
  bool m_plunged;
  bool m_linear_interpolation;

  const std::string &get_block ();
  void read_line (std::string &b);
  void produce_circle (double cx, double cy, double r) { produce_circle (cx, cy, r, cx, cy); }
  void produce_circle (double cx, double cy, double r, double ex, double ey);
  void produce_circle_raw (double cx, double cy, double r, double ex, double ey);
  void start_step_and_repeat ();
  void stop_step_and_repeat ();
  void end_block ();
  void begin_pattern ();
  void end_pattern ();
  void repeat_block (double dx, double dy, double fx = 1.0, double fy = 1.0, bool swapxy = false);
  void repeat_pattern (double dx, double dy);
  void process_line (const std::string &s);
  void next_hole ();
  void init ();
};

}

#endif

