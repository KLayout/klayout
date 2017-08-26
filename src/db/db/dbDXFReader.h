
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "tlException.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlString.h"

#include "dbLayout.h"
#include "dbReader.h"
#include "dbDXF.h"
#include "tlStream.h"
#include "dbStreamLayers.h"
#include "dbPropertiesRepository.h"

#include <map>
#include <set>

namespace db
{

class Matrix3d;

/**
 *  @brief Structure that holds the DXF specific options for the reader
 */
class DB_PUBLIC DXFReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  DXFReaderOptions ()
    : dbu (0.001),
      unit (1.0),
      text_scaling (100.0),
      polyline_mode (0),
      circle_points (100),
      circle_accuracy (0.0),
      render_texts_as_polygons (false),
      keep_other_cells (false),
      create_other_layers (true)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Specify the database unit to produce 
   *
   *  Specify the database unit which the resulting layout will receive.
   */
  double dbu;

  /**
   *  @brief Specify the unit of the DXF file
   *
   *  Since DXF is unitless, this value allows to specify the units of the DXF file given as input.
   */
  double unit;

  /**
   *  @brief Text scaling factor
   *
   *  This value specifies text scaling in percent. A value of 100 roughly means that the letter 
   *  pitch of the font will be 92% of the specified text height. That value applies for ROMANS fonts.
   *  When generating GDS texts, a value of 100 generates TEXT objects with 
   *  the specified size. Smaller values generate smaller sizes.
   */
  double text_scaling;

  /**
   *  @brief POLYLINE/LWPOLYLINE mode
   *
   *  0: automatic mode
   *  1: keep lines
   *  2: create polygons from closed POLYLINE/LWPOLYLINE with width == 0
   *  3: merge all lines (width width 0)
   *  4: as 3 and auto-close contours
   */
  int polyline_mode;

  /**
   *  @brief Number of points for a full circle for arc interpolation
   *
   *  See circle_accuracy for another way of specifying the number of
   *  points per circle.
   */
  int circle_points;

  /**
   *  @brief Accuracy of circle approximation
   *
   *  This value specifies the approximation accuracy of the circle and other
   *  "round" structures. If this value is a positive number bigger than the
   *  database unit (see dbu), it will control the number of points the
   *  circle is resolved into. The number of points will be chosen such that
   *  the deviation from the ideal curve is less than this value.
   *
   *  The actual number of points used for the circle approximation is
   *  not larger than circle_points.
   *
   *  The value is given in the units of the DXF file.
   */
  double circle_accuracy;

  /**
   *  @brief If set to true, converts texts to polygons on read
   * 
   *  Converting texts avoids problems with UTF-8 character sets.
   */
  bool render_texts_as_polygons;

  /**
   *  @brief If set to true, cells other than the top cell are kept instead of removed
   */
  bool keep_other_cells;

  /**
   *  @brief Specifies a layer mapping
   *
   *  If a layer mapping is specified, only the given layers are read.
   *  Otherwise, all layers are read.
   *  Setting "create_other_layers" to true will make the reader
   *  create other layers for all layers not given in the layer map.
   *  Setting an empty layer map and create_other_layers to true effectively
   *  enables all layers for reading.
   */
  db::LayerMap layer_map;

  /**
   *  @brief A flag indicating that a new layers shall be created
   *
   *  If this flag is set to true, layers not listed in the layer map a created
   *  too.
   */
  bool create_other_layers;

  /** 
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new DXFReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("DXF");
    return n;
  }
};

/**
 *  @brief Generic base class of DXF reader exceptions
 */
class DB_PUBLIC DXFReaderException
  : public ReaderException
{
public:
  DXFReaderException (const std::string &msg, size_t p, const std::string &cell)
    : ReaderException (tl::sprintf (tl::to_string (QObject::tr ("%s (position=%ld, cell=%s)")), msg.c_str (), p, cell))
  { }

  DXFReaderException (const std::string &msg, int line, const std::string &cell)
    : ReaderException (tl::sprintf (tl::to_string (QObject::tr ("%s (line=%d, cell=%s)")), msg.c_str (), line, cell))
  { }
};

/**
 *  @brief The DXF format stream reader
 */
class DB_PUBLIC DXFReader
  : public ReaderBase, 
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
   *  @brief Issue an error with positional informations
   *
   *  Reimplements DXFDiagnostics
   */
  virtual void error (const std::string &txt);

  /**
   *  @brief Issue a warning with positional informations
   *
   *  Reimplements DXFDiagnostics
   */
  virtual void warn (const std::string &txt);

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
  bool m_create_layers;
  LayerMap m_layer_map;
  tl::AbsoluteProgress m_progress;
  double m_dbu;
  double m_unit;
  double m_text_scaling;
  int m_polyline_mode;
  int m_circle_points;
  double m_circle_accuracy;
  std::string m_cellname;
  std::string m_line;
  bool m_ascii;
  bool m_initial;
  bool m_render_texts_as_polygons;
  bool m_keep_other_cells;
  int m_line_number; 
  unsigned int m_zero_layer;
  unsigned int m_next_layer_index;
  std::map <std::string, unsigned int> m_new_layers;
  std::set <db::cell_index_type> m_template_cells;
  std::set <db::cell_index_type> m_used_template_cells;
  std::map <std::string, db::cell_index_type> m_block_per_name;
  std::map <VariantKey, db::cell_index_type> m_block_to_variant;

  void do_read (db::Layout &layout, db::cell_index_type top);

  std::pair <bool, unsigned int> open_layer (db::Layout &layout, const std::string &name);
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
  void spline_interpolation (std::vector<db::DPoint> &points, int n, const std::vector<double> &knots, bool save_first = false);
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

