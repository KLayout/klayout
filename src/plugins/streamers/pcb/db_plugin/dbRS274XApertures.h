
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


#ifndef HDR_dbRS274XApertures
#define HDR_dbRS274XApertures

#include "dbPoint.h"
#include "dbPath.h"
#include "dbPolygon.h"
#include "dbTrans.h"
#include "dbRegion.h"
#include "tlStream.h"

#include <vector>

namespace db
{
  class EdgeProcessor;
}
 
namespace db
{

class RS274XReader;

class RS274XApertureBase
{
public:
  RS274XApertureBase ();
  virtual ~RS274XApertureBase () { }

  void produce_flash (const db::DCplxTrans &d, RS274XReader &reader, db::EdgeProcessor &ep, bool clear);
  void produce_linear (const db::DCplxTrans &d, const db::DVector &dist, RS274XReader &reader, db::EdgeProcessor &ep, bool clear);

protected:
  void clear_points ();
  void add_point (double x, double y);
  void add_point (const db::DPoint &d);
  void add_point (const db::Point &d);
  void produce_circle (double cx, double cy, double r, bool clear);
  void produce_polygon (bool clear);
  void produce_line ();

  RS274XReader &reader () 
  {
    return *mp_reader;
  }

  virtual void do_produce_flash () = 0;
  virtual bool do_produce_linear (const db::DPoint &from, const db::DPoint &to) = 0;

private:
  std::vector<db::Point> m_points;
  std::vector<db::Polygon> m_polygons;
  std::vector<db::Polygon> m_clear_polygons;
  std::vector<db::Path> m_lines;
  db::EdgeProcessor *mp_ep;
  RS274XReader *mp_reader;
  bool m_needs_update;
};


class RS274XCircleAperture
  : public RS274XApertureBase
{
public:
  RS274XCircleAperture (RS274XReader &reader, tl::Extractor &ex);

  virtual void do_produce_flash ();
  virtual bool do_produce_linear (const db::DPoint &from, const db::DPoint &to);

private:
  double m_d;
  double m_dx, m_dy;
};

class RS274XRectAperture
  : public RS274XApertureBase
{
public:
  RS274XRectAperture (RS274XReader &reader, tl::Extractor &ex);

  virtual void do_produce_flash ();
  virtual bool do_produce_linear (const db::DPoint &from, const db::DPoint &to);

private:
  double m_dx, m_dy;
  double m_hx, m_hy;
};

class RS274XOvalAperture
  : public RS274XApertureBase
{
public:
  RS274XOvalAperture (RS274XReader &reader, tl::Extractor &ex);

  virtual void do_produce_flash ();
  virtual bool do_produce_linear (const db::DPoint &from, const db::DPoint &to);

private:
  double m_dx, m_dy;
  double m_hx, m_hy;
};

class RS274XRegularAperture
  : public RS274XApertureBase
{
public:
  RS274XRegularAperture (RS274XReader &reader, tl::Extractor &ex);

  virtual void do_produce_flash ();
  virtual bool do_produce_linear (const db::DPoint &from, const db::DPoint &to);

private:
  double m_d, m_a;
  int m_nsides;
  double m_hx, m_hy;
};

class RS274XRegionAperture
  : public RS274XApertureBase
{
public:
  RS274XRegionAperture (const db::Region &region);

  virtual void do_produce_flash ();
  virtual bool do_produce_linear (const db::DPoint &from, const db::DPoint &to);

private:
  db::Region m_region;
};

class RS274XMacroAperture
  : public RS274XApertureBase
{
public:
  RS274XMacroAperture (RS274XReader &reader, const std::string &name, const std::string &def, tl::Extractor &ex);

  virtual void do_produce_flash ();
  virtual bool do_produce_linear (const db::DPoint &from, const db::DPoint &to);

private:
  std::string m_name;
  std::string m_def;
  double m_unit;
  std::vector<double> m_parameters;

  double read_atom (tl::Extractor &ex);
  double read_dot_expr (tl::Extractor &ex);
  double read_expr (tl::Extractor &ex, bool length = false);
  void read_exposure (tl::Extractor &ex, bool &clear, bool &clear_set);
  void do_produce_flash_internal ();
};

}

#endif

