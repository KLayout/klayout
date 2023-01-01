
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

#ifndef HDR_dbDXFReader
#define HDR_dbDXFReader

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "dbNamedLayerReader.h"
#include "dbDXF.h"
#include "dbDXFFormat.h"
#include "dbStreamLayers.h"
#include "dbPropertiesRepository.h"

#include "tlException.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlString.h"
#include "tlStream.h"

#include <map>
#include <set>

namespace db
{

template <class C> class matrix_3d;
typedef matrix_3d<db::DCoord> Matrix3d;

/**
 *  @brief Generic base class of DXF reader exceptions
 */
class DB_PLUGIN_PUBLIC DXFReaderException
  : public ReaderException
{
public:
  DXFReaderException (const std::string &msg, size_t p, const std::string &cell)
    : ReaderException (tl::sprintf (tl::to_string (tr ("%s (position=%ld, cell=%s)")), msg.c_str (), p, cell))
  { }

  DXFReaderException (const std::string &msg, int line, const std::string &cell)
    : ReaderException (tl::sprintf (tl::to_string (tr ("%s (line=%d, cell=%s)")), msg.c_str (), line, cell))
  { }
};

/**
 *  @brief The DXF format stream reader
 */
class DB_PLUGIN_PUBLIC DXFReader
  : public NamedLayerReader,
    public DXFDiagnostics
{
public: 
  typedef std::vector<tl::Variant> property_value_list;

  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  DXFReader (tl::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~DXFReader ();

  /** 
   *  @brief The basic read method 
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  A set of options can be specified with the LoadLayoutOptions
   *  object.
   *  The returned map will contain all layers, the passed
   *  ones and the newly created ones.
   *
   *  @param layout The layout object to write to
   *  @param map The LayerMap object
   *  @param create true, if new layers should be created
   *  @return The LayerMap object that tells where which layer was loaded
   */
  virtual const LayerMap &read (db::Layout &layout, const LoadLayoutOptions &options);

  /** 
   *  @brief The basic read method (without mapping)
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  This version will read all input layers and return a map
   *  which tells which DXF layer has been read into which logical
   *  layer.
   *
   *  @param layout The layout object to write to
   *  @return The LayerMap object
   */
  virtual const LayerMap &read (db::Layout &layout);

  /**
   *  @brief Format
   */
  virtual const char *format () const { return "DXF"; }

  /**
   *  @brief Issue an error with positional information
   *
   *  Reimplements DXFDiagnostics
   */
  virtual void error (const std::string &txt);

  /**
   *  @brief Issue a warning with positional information
   *
   *  Reimplements DXFDiagnostics
   */
  virtual void warn (const std::string &txt, int warn_level = 1);

private:
  struct VariantKey
  {
    db::cell_index_type cell_index;
    unsigned int layer;
    double sx, sy;

    VariantKey (db::cell_index_type ci, unsigned int l, double x, double y)
      : cell_index (ci), layer (l), sx (x), sy (y)
    { }

    bool operator== (const VariantKey &other) const
    {
      return cell_index == other.cell_index && layer == other.layer && fabs (sx - other.sx) < 1e-6 && fabs (sy - other.sy) < 1e-6;
    }

    bool operator< (const VariantKey &other) const
    {
      if (cell_index != other.cell_index) {
        return cell_index < other.cell_index;
      }
      if (layer != other.layer) {
        return layer < other.layer;
      }
      if (fabs (sx - other.sx) >= 1e-6) {
        return sx < other.sx;
      }
      if (fabs (sy - other.sy) >= 1e-6) {
        return sy < other.sy;
      }
      return false;
    }
  };

  tl::InputStream &m_stream;
  tl::AbsoluteProgress m_progress;
  double m_dbu;
  double m_unit;
  double m_text_scaling;
  int m_polyline_mode;
  int m_circle_points;
  double m_circle_accuracy;
  double m_contour_accuracy;
  std::string m_cellname;
  std::string m_line;
  bool m_ascii;
  bool m_initial;
  bool m_render_texts_as_polygons;
  bool m_keep_other_cells;
  int m_line_number; 
  unsigned int m_zero_layer;
  std::map <db::cell_index_type, std::string> m_template_cells;
  std::set <db::cell_index_type> m_used_template_cells;
  std::map <std::string, db::cell_index_type> m_block_per_name;
  std::map <VariantKey, db::cell_index_type> m_block_to_variant;

  void do_read (db::Layout &layout, db::cell_index_type top);

  db::cell_index_type make_layer_variant (db::Layout &layout, const std::string &cellname, db::cell_index_type template_cell, unsigned int layer, double sx, double sy);
  void cleanup (db::Layout &layout, db::cell_index_type top);

  int read_int16 ();
  int read_int32 ();
  long long read_int64 ();
  int read_group_code ();
  double read_double ();
  const std::string &read_string (bool ignore_empty_lines);
  bool prepare_read (bool ignore_empty_lines);
  void skip_value (int group_code);
  void read_cell (db::Layout &layout);
  void read_entities (db::Layout &layout, db::Cell &cell, const db::DVector &offet);
  void fill_layer_variant_cell (db::Layout &layout, const std::string &cellname, db::cell_index_type template_cell, db::cell_index_type var_cell, unsigned int layer, double sx, double sy);
  db::DCplxTrans global_trans (const db::DVector &offset, double ex, double ey, double ez);
  int determine_polyline_mode ();
  void parse_entity (const std::string &entity_code, size_t &nsolids, size_t &closed_polylines);
  void add_bulge_segment (std::vector<db::DPoint> &points, const db::DPoint &p, double b);
  std::list<db::DPoint> spline_interpolation (std::vector<std::pair<db::DPoint, double> > &points, int n, const std::vector<double> &knots);
  void arc_interpolation (std::vector<db::DPoint> &points, const std::vector<double> &rad, const std::vector<double> &start, const std::vector<double> &end, const std::vector<int> &ccw);
  void elliptic_interpolation (std::vector<db::DPoint> &points, const std::vector<double> &rmin, const std::vector<db::DPoint> &vmaj, const std::vector<double> &start, const std::vector<double> &end, const std::vector<int> &ccw);
  void deliver_points_to_edges (std::vector<db::DPoint> &points, const std::vector<db::DPoint> &points2, const db::DCplxTrans &tt, int edge_type, int value94, const std::vector<double> &value40, const std::vector<double> &value50, const std::vector<double> &value51, const std::vector<int> &value73, std::vector<db::Edge> &iedges);
  void deliver_text (db::Shapes &shapes, const std::string &s, const db::DCplxTrans &text_trans, double h, double ls, int halign, int valign, double w = -1.0);
  void check_coord (double x);
  void check_point (const db::DPoint &p);
  void check_vector (const db::DVector &p);
  db::Polygon safe_from_double (const db::DPolygon &p);
  db::SimplePolygon safe_from_double (const db::DSimplePolygon &p);
  db::Path safe_from_double (const db::DPath &p);
  db::Point safe_from_double (const db::DPoint &p);
  db::Vector safe_from_double (const db::DVector &p);
  db::Edge safe_from_double (const db::DEdge &p);
  db::Box safe_from_double (const db::DBox &p);
  db::Text safe_from_double (const db::DText &p);
  void insert_scaled (db::Shapes &target, const db::Shape &src, const db::Matrix3d &m);
  int ncircle_for_radius (double r) const;
};

}

#endif

