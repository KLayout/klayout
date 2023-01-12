
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


#ifndef HDR_dbGerberImporter
#define HDR_dbGerberImporter

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "dbTrans.h"
#include "dbEdgeProcessor.h"
#include "dbRegion.h"
#include "tlStream.h"
#include "tlProgress.h"

#include <iostream>

namespace db
{
  class Manager;
}

namespace lay
{
  class LayoutView;
}

namespace db
{

/**
 *  @brief A structure holding the meta data for a Gerber (X2) file
 */
struct GerberMetaData
{
  /**
   *  @brief Identifies the function of the layer
   */
  enum Function
  {
    NoFunction = 0,
    Copper,
    Hole,
    PlatedHole,
    NonPlatedHole,
    Profile,
    SolderMask,
    Legend,
    Other
  };

  /**
   *  @brief Identifies the position of the layer
   */
  enum Position
  {
    NoPosition = 0,
    Bottom,
    Inner,
    Top
  };

  /**
   *  @brief Constructor
   */
  GerberMetaData ()
    : function (NoFunction),
      cu_layer_number (0),
      from_cu (0),
      to_cu (0),
      position (NoPosition)
  { }

  /**
   *  @brief The project name or an empty string if none is given
   */
  std::string project_id;

  /**
   *  @brief The creation date or an empty string if none is given
   */
  std::string creation_date;

  /**
   *  @brief The generation software or an empty string if none is given
   */
  std::string generation_software;

  /**
   *  @brief The function of the layer
   */
  Function function;

  /**
   *  @brief The copper layer number
   *
   *  This is a number identifying the layer in the copper stack.
   *  The topmost layer is 1, the bottom layer 2 or larger.
   *  This value is 0 if no layer is specified.
   */
  int cu_layer_number;

  /**
   *  @brief The drill hole start copper layer
   *
   *  This is number of the copper layer that the drill hole connects (upper layer).
   *  This number is applicable only if the function is PlatedHole or NonPlatedHole.
   *  It is a value > 0.
   */
  int from_cu;

  /**
   *  @brief The drill hole end copper layer
   *
   *  This is number of the copper layer that the drill hole connects (lower layer).
   *  This number is applicable only if the function is PlatedHole or NonPlatedHole.
   *  It is a value > 0.
   */
  int to_cu;

  /**
   *  @brief This is the position of the layer in the stack
   *
   *  This value is applicable for Copper, SolderMask and Legend.
   */
  Position position;
};

/**
 *  @brief A class holding the graphics state of the reader
 */
struct GraphicsState
{
  GraphicsState ()
    : inverse (false),
      m_rot (0.0), m_s (1.0), m_ox (false), m_oy (false), m_mx (false), m_my (false),
      m_orot (0.0), m_os (1.0), m_omx (false), m_omy (false)
  {
    displacements.push_back (db::DVector ());
  }

  bool inverse;
  db::DCplxTrans global_trans;
  double m_rot;
  double m_s;
  double m_ox, m_oy;
  bool m_mx, m_my;
  double m_orot;
  double m_os;
  bool m_omx, m_omy;
  std::vector<db::Path> lines;
  std::vector<db::Polygon> polygons;
  std::vector<db::Polygon> clear_polygons;
  std::vector<db::DVector> displacements;
  std::string token;
};

/**
 *  @brief The base class for all readers
 */
class GerberFileReader
  : public tl::Object
{
public:
  /**
   *  @brief Constructor
   */
  GerberFileReader (int warn_level);

  /**
   *  @brief Destructor
   */
  virtual ~GerberFileReader () { }

  /**
   *  @brief Detect the file format
   *
   *  This method returns true, if the given file is accepted by this reader.
   */
  bool accepts (tl::TextInputStream &stream);

  /**
   *  @brief Read the file from the given stream into the set of target containers
   *
   *  The reader reads the file and produces the same polygons in all layers of topcell 
   *  provided by the "targets" parameter.
   */
  void read (tl::TextInputStream &stream, db::Layout &layout, db::Cell &cell, const std::vector <unsigned int> &targets);

  /**
   *  @brief Scans the stream and extracts the metadata
   */
  GerberMetaData scan (tl::TextInputStream &stream);

  /**
   *  @brief Sets the number of points for a circle interpolation
   *
   *  The value must be larger or equal than 4.
   */
  void set_circle_points (int c) 
  {
    m_circle_points = (c >= 4 ? c : 64);
  }

  /**
   *  @brief Sets the number of points for a circle interpolation
   */
  int get_circle_points () const
  {
    return m_circle_points;
  }

  /**
   *  @brief Sets the merge flag
   *
   *  If the merge flag is set, all shapes will be merged after reading.
   */
  void set_merge (bool m) 
  {
    m_merge = m;
  }

  /**
   *  @brief Get the merge flag
   */
  bool merge () const
  {
    return m_merge;
  }

  /**
   *  @brief Set the database unit
   */
  void set_dbu (double dbu) 
  {
    m_dbu = dbu;
  }

  /** 
   *  @brief Gets the current database unit 
   */
  double dbu () const
  {
    return m_dbu;
  }

  /**
   *  @brief Set the global transformation
   *
   *  The global transformation is applied after all other transformations have been applied
   */
  void set_global_trans (const db::DCplxTrans &trans)
  {
    m_global_trans = trans;
  }

  /**
   *  @brief Get the global transformation
   */
  const db::DCplxTrans &global_trans () const
  {
    return m_global_trans;
  }

  /**
   *  @brief Set the unit 
   *
   *  The unit is given in micron, i.e. mm=1000, inch=25400
   */
  void set_unit (double u)
  {
    m_unit = u;
  }

  /**
   *  @brief Get the unit
   */
  double unit () const
  {
    return m_unit;
  }

  /**
   *  @brief Set the format string
   */
  void set_format_string (const std::string &format);

  /**
   *  @brief Get the format string
   */
  std::string format_string () const;

  /**
   *  @brief Return true, if the format is specified
   */
  bool has_format () const
  {
    return (m_omit_leading_zeroes && m_digits_after >= 0) || (! m_omit_leading_zeroes && m_digits_before >= 0);
  }

  /**
   *  @brief Set the format 
   *
   *  @param before Number of digits before the decimal point or -1 for unspecified
   *  @param after Number of digits after the decimal point or -1 for unspecified
   *  @param omit_leading_zeroes True, if leading zeroes should be omitted
   */
  void set_format (int before, int after, bool omit_leading_zeroes)
  {
    m_digits_before = before;
    m_digits_after = after;
    m_omit_leading_zeroes = omit_leading_zeroes;
  }

  /**
   *  @brief Set the format (only leading zeroes flag)
   *
   *  @param omit_leading_zeroes True, if leading zeroes should be omitted
   */
  void set_format (bool omit_leading_zeroes)
  {
    m_omit_leading_zeroes = omit_leading_zeroes;
  }

  /**
   *  @brief Set the format (without leading zeroes flag)
   *
   *  @param before Number of digits before the decimal point or -1 for unspecified
   *  @param after Number of digits after the decimal point or -1 for unspecified
   */
  void set_format (int before, int after)
  {
    m_digits_before = before;
    m_digits_after = after;
  }

  /**
   *  @brief Produce the given line on the output
   *
   *  This method will produce a line, represented by a zero-width path.
   *  Lines are special objects created for zero-diameter apertures. 
   *  Clear lines are ignored currently and lines don't participate in the
   *  boolean operations.
   *
   *  @param p The zero-width path in micron units.
   *  @param clear Currently, the path is ignored when clear is true.
   */
  void produce_line (const db::DPath &p, bool clear);

  /**
   *  @brief Produce the given polygon on the output
   *
   *  @param p The polygon in micron units.
   *  @param clear True, if the polygon must be cleared from the output area
   */
  void produce_polygon (const db::DPolygon &p, bool clear);

  /**
   *  @brief Returns true, if the inverse layer flag was set during read
   */
  bool is_inverse () const
  {
    return m_inverse;
  }

protected:
  /**
   *  @brief Read the file from the given stream into the set of target containers
   *
   *  The reader reads the file and produces the same polygons in all containers
   *  provided by the "targets" parameter.
   *  The implementation must use "produce_polygons" to produce the output.
   */
  virtual void do_read () = 0;

  /**
   *  @brief Scans the stream and returns the metadata
   */
  virtual GerberMetaData do_scan () = 0;

  /**
   *  @brief Returns true, if the reader accepts the stream
   */
  virtual bool does_accept () = 0;

  /**
   *  @brief Issue a warning
   */
  void warn (const std::string &warning, int warn_level = 1);

  /**
   *  @brief Issue a non-fatal error
   */
  void error (const std::string &error);

  /**
   *  @brief Issue a fatal error
   *
   *  This method does not return.
   */
  void fatal (const std::string &error);

  /**
   *  @brief Gets the local transformation
   */
  db::DCplxTrans local_trans () const;

  /**
   *  @brief Gets the local transformation
   */
  db::DCplxTrans object_trans () const;

  /**
   *  @brief Read a coordinate from the extractor using the format and unit
   *
   *  The coordinate is returned in micron.
   */
  double read_coord (tl::Extractor &ex);

  /**
   *  @brief Returns the accuracy of the coordinates (the resolution of the format)
   */
  double accuracy () const;

  /**
   *  @brief Flush the stored data to the output
   *  This method is similar to collect(), but writes the data to the layout.
   */
  void flush (const std::string &net_name = std::string ());

  /**
   *  @brief Collects the data taken so far into the given region
   *  This method is similar to flush(), but will return a Region object.
   */
  void collect (db::Region &region);

  /**
   *  @brief Pushes the graphics state
   */
  void push_state (const std::string &token);

  /**
   *  @brief Pops the graphics state
   *  Returns the token given in "push_state"
   */
  std::string pop_state ();

  /**
   *  @brief Returns true if the graphics stack is empty
   */
  bool graphics_stack_empty () const;

  /**
   *  @brief Access to the edge processor
   */
  db::EdgeProcessor &ep () 
  {
    return m_ep;
  }

  /**
   *  @brief Enter a repeated sequence
   *
   *  The shapes produced after this method has been called are put into 
   *  a subcell which is placed with the given displacements
   */
  void step_and_repeat (const std::vector <db::DVector> &displacements);

  /**
   *  @brief Leaves step-and-repeat mode
   */
  void reset_step_and_repeat ();

  /**
   *  @brief Sets or resets the inverse layer flag
   */
  void set_inverse (bool inverse)
  {
    m_inverse = inverse;
  }
  
  /**
   *  @brief This method updates the progress counter
   *
   *  This method should be called regularly
   */
  void progress_checkpoint ();

  /**
   *  @brief Gets the stream object
   */
  tl::TextInputStream &stream () 
  {
    return *mp_stream;
  }

  /**
   *  @brief Updates the local mirror flags
   */
  void update_local_mirror (bool mx, bool my)
  {
    m_mx = mx; m_my = my;
  }

  /**
   *  @brief Updates the local orientation
   */
  void update_local_angle (double rot)
  {
    m_rot = rot;
  }

  /**
   *  @brief Updates the local scale factor
   */
  void update_local_scale (double s)
  {
    m_s = s;
  }

  /**
   *  @brief Updates the local offset
   */
  void update_local_offset (double x, double y)
  {
    m_ox = x; m_oy = y;
  }

  /**
   *  @brief Updates the object mirror flags
   */
  void update_object_mirror (bool mx, bool my)
  {
    m_omx = mx; m_omy = my;
  }

  /**
   *  @brief Updates the object orientation
   */
  void update_object_angle (double rot)
  {
    m_orot = rot;
  }

  /**
   *  @brief Updates the object scale factor
   */
  void update_object_scale (double s)
  {
    m_os = s;
  }

private:
  int m_circle_points;
  int m_digits_before;
  int m_digits_after;
  bool m_omit_leading_zeroes;
  bool m_merge;
  bool m_inverse;
  double m_dbu;
  double m_unit;
  db::DCplxTrans m_global_trans;
  double m_rot;
  double m_s;
  double m_ox, m_oy;
  bool m_mx, m_my;
  double m_orot;
  double m_os;
  bool m_omx, m_omy;
  std::vector<db::Path> m_lines;
  std::vector<db::Polygon> m_polygons;
  std::vector<db::Polygon> m_clear_polygons;
  db::EdgeProcessor m_ep;
  std::vector<unsigned int> m_target_layers;
  std::vector<db::DVector> m_displacements;
  db::Layout *mp_layout;
  db::Cell *mp_top_cell;
  tl::TextInputStream *mp_stream;
  tl::AbsoluteProgress m_progress;
  std::list<GraphicsState> m_graphics_stack;
  int m_warn_level;

  void process_clear_polygons ();
  void swap_graphics_state (GraphicsState &state);
};

/**
 *  @brief Represents one file in a Gerber stack.
 */
class DB_PLUGIN_PUBLIC GerberFile
{
public:
  GerberFile ();

  /**
   *  @brief Set the name (or path) of the file to load
   *
   *  This can either be a relative or absolute path. Relative paths will be
   *  interpreted relative to the importers "dir" member.
   */
  void set_filename (const std::string &filename)
  {
    m_filename = filename;
  }

  /**
   *  @brief Get the name of the file to load
   */
  const std::string &filename () const
  {
    return m_filename;
  }

  /**
   *  @brief Set the merge mode
   *
   *  Merge mode can be -1 (default), 0 (don't merge) and 1 (merge).
   */
  void set_merge_mode (int merge_mode)
  {
    m_merge_mode = merge_mode;
  }

  /**
   *  @brief Get the merge mode
   */
  int merge_mode () const
  {
    return m_merge_mode;
  }

  /**
   *  @brief Set the circle interpolation mode (number of points on full circle)
   */
  void set_circle_points (int circle_points)
  {
    m_circle_points = circle_points;
  }

  /**
   *  @brief Get the circle interpolation mode (number of points on full circle)
   */
  int circle_points () const
  {
    return m_circle_points;
  }

  /**
   *  @brief Set the format string 
   *
   *  The format string is "n:m[TL]". n, m and T or L are optional. n are the 
   *  digits before the decimal point, m are the digits after. L specifies leading zeroes (omit trailing zeroes),
   *  T specifies trailing zeroes (omit leading zeroes). "T" is the default.
   *  If "T" is specified, m must be given, otherwise n must be specified. If n or m are not specified "*" can be used instead.
   *  If "L" or "T" is not specified, a guess will be made from n or m.
   */
  void set_format_string (const std::string &format);

  /**
   *  @brief Get the format string
   */
  std::string format_string () const;

  /**
   *  @brief Set the format 
   *
   *  @param before Number of digits before the decimal point or -1 for unspecified
   *  @param after Number of digits after the decimal point or -1 for unspecified
   *  @param omit_leading_zeroes True, if leading zeroes should be omitted
   */
  void set_format (int before, int after, bool omit_leading_zeroes)
  {
    m_digits_before = before;
    m_digits_after = after;
    m_omit_leading_zeroes = omit_leading_zeroes;
  }

  /**
   *  @brief Get the format: number of digits before the decimal point
   */
  int digits_before () const
  {
    return m_digits_before;
  }

  /**
   *  @brief Get the format: number of digits after the decimal point
   */
  int digits_after () const
  {
    return m_digits_after;
  }

  /**
   *  @brief Get the format: omit leading zeroes
   */
  bool omit_leading_zeroes () const
  {
    return m_omit_leading_zeroes;
  }

  /**
   *  @brief Return true, if the format is specified
   */
  bool has_format () const
  {
    return (m_omit_leading_zeroes && m_digits_after >= 0) || (! m_omit_leading_zeroes && m_digits_before >= 0);
  }

  /**
   *  @brief Specify a list of layout layers to which to write this layer
   *
   *  This must be a comma-separated list of db::LayerProperties string representations.
   */
  void set_layers_string (const std::string &layers);

  /**
   *  @brief Get a string specifying the list of layout layers
   */
  std::string layers_string () const;

  /**
   *  @brief Add a new layer specification
   */
  void add_layer_spec (const db::LayerProperties &lp)
  {
    m_layer_specs.push_back (lp);
  }

  /**
   *  @brief Add a new layer specification
   */
  const std::vector <db::LayerProperties> &layer_specs () 
  {
    return m_layer_specs;
  }

private:
  int m_circle_points;
  int m_merge_mode;
  int m_digits_before;
  int m_digits_after;
  bool m_omit_leading_zeroes;
  std::vector <db::LayerProperties> m_layer_specs;
  std::string m_filename;
};

/**
 *  @brief The Gerber format importer object
 *
 *  This class provides a importer for Gerber layer stacks. It can be 
 *  loaded from project files and saved to such.
 */
class DB_PLUGIN_PUBLIC GerberImporter
{
public:
  /**
   *  @brief Default constructor
   */
  GerberImporter (int warn_level = 1);

  /**
   *  @brief Scans the given file and extracts the metadata from it
   */
  static GerberMetaData scan (const std::string &fn);

  /**
   *  @brief Scans the given stream and extracts the metadata from it
   */
  static GerberMetaData scan (tl::TextInputStream &stream);

  /**
   *  @brief Load the project file from the given stream
   *
   *  This method will use the directory part of the file parameter
   *  and ignore the "dir" member.
   */
  void load_project (const std::string &file);

  /**
   *  @brief Load the project file from the given stream
   *
   *  This method will use the base directory specified in the 
   *  "dir" member for the files.
   */
  void load_project (tl::TextInputStream &stream);

  /**
   *  @brief Save the project to the given stream
   */
  void save_project (std::ostream &stream);

  /**
   *  @brief Read into an existing layout
   *
   *  This method reads the layer stack into the layout and cell provided.
   *  The database unit and cell name members are ignored.
   */
  void read (db::Layout &layout, db::cell_index_type cell_index);

  /**
   *  @brief Read into a fresh layout
   *
   *  This method reads the layer stack into the layout provided.
   *  The layout should be empty and is initialized with the given 
   *  database unit and a cell with the given name is created.
   */
  db::cell_index_type read (db::Layout &layout);

  /**
   *  @brief Sets the merge flag
   *
   *  If the merge flag is set, all shapes will be merged after reading.
   *
   *  This value will be used unless a merge mode is explicitly specified per file.
   */
  void set_merge (bool m) 
  {
    m_merge = m;
  }

  /**
   *  @brief Get the merge flag
   */
  bool merge () const
  {
    return m_merge;
  }

  /**
   *  @brief Sets the flag indicating whether to invert negative layers
   *
   *  If the merge flag is set, layers with negative contrast will be inverted.
   */
  void set_invert_negative_layers (bool i) 
  {
    m_invert_negative_layers = i;
  }

  /**
   *  @brief Gets the flag indicating whether to invert negative layers
   */
  bool invert_negative_layers () const
  {
    return m_invert_negative_layers;
  }

  /**
   *  @brief Sets the border width by which the bounding box is oversized to render the background for inversion
   *
   *  This value is given in micrometer
   */
  void set_border (double w)
  {
    m_border = w;
  }

  /**
   *  @brief Gets the border width
   */
  double border () const
  {
    return m_border;
  }

  /**
   *  @brief Set the circle interpolation mode (number of points on full circle)
   *
   *  This value will be used unless a number of points is explicitly specified per file.
   */
  void set_circle_points (int circle_points)
  {
    m_circle_points = circle_points;
  }

  /**
   *  @brief Get the circle interpolation mode (number of points on full circle)
   */
  int circle_points () const
  {
    return m_circle_points;
  }

  /**
   *  @brief Specifies the layer styles to use
   *
   *  This allows one to specify a layer style file (path) which will be loaded 
   *  after the Gerber files have been imported.
   *
   *  Specify an empty string to disable that feature.
   */
  void set_layer_styles (const std::string ls)
  {
    m_layer_styles = ls;
  }

  /**
   *  @brief Gets the layer styles to use
   */
  const std::string &layer_styles () const
  {
    return m_layer_styles;
  }

  /**
   *  @brief Specifies the global transformation
   *
   *  This specifies the global transformation to apply (in micron units).
   */
  void set_global_trans (const db::DCplxTrans &trans)
  {
    m_global_trans = trans;
  }

  /**
   *  @brief Gets the global transformation
   */
  const db::DCplxTrans &global_trans () const
  {
    return m_global_trans;
  }

  /**
   *  @brief Set the reference points
   */
  void set_reference_points (const std::vector<std::pair <db::DPoint, db::DPoint> > &pts)
  {
    m_reference_points = pts;
  }

  /**
   *  @brief Set the directory where the files are read from (base directory)
   *
   *  All files specified in the components are looked up relative to this path.
   *  This member is set with the directory part of the file name when "load_from_file"
   *  is used with a file name argument.
   */
  void set_dir (const std::string &dir)
  {
    m_dir = dir;
  }

  /**
   *  @brief Get the directory where the files are read from (base directory)
   */
  const std::string &dir () const
  {
    return m_dir;
  }

  /**
   *  @brief Set the cell name to which the geometries are written
   *
   *  This is the name of the cell created when the read method is called without a 
   *  cell argument.
   */
  void set_cell_name (const std::string &cell_name)
  {
    m_cell_name = cell_name;
  }

  /**
   *  @brief Get the cell name to which the geometries are written
   */
  const std::string &cell_name () const
  {
    return m_cell_name;
  }

  /**
   *  @brief Set the data base unit of the layout to generate
   *
   *  This is the database unit used when a new cell is created. The layout will be
   *  given this database unit.
   */
  void set_dbu (double dbu)
  {
    m_dbu = dbu;
  }

  /**
   *  @brief Get the data base unit of the layout to generate
   */
  double dbu () const
  {
    return m_dbu;
  }

  /**
   *  @brief Clear the file specifications
   */
  void clear_files ()
  {
    m_files.clear ();
  }

  /**
   *  @brief Add a file specification
   */
  void add_file (const db::GerberFile &file)
  {
    m_files.push_back (file);
  }

  /**
   *  @brief Get the files: begin iterator
   */
  std::vector<db::GerberFile>::const_iterator begin_files () const
  {
    return m_files.begin ();
  }

  /**
   *  @brief Get the files: end iterator
   */
  std::vector<db::GerberFile>::const_iterator end_files () const
  {
    return m_files.end ();
  }

private:
  std::string m_cell_name;
  double m_dbu;
  bool m_merge;
  bool m_invert_negative_layers;
  double m_border;
  int m_circle_points;
  int m_warn_level;
  std::string m_format_string;
  std::string m_layer_styles;
  std::string m_dir;
  db::DCplxTrans m_global_trans;
  std::vector <std::pair <db::DPoint, db::DPoint> > m_reference_points;
  std::vector <db::GerberFile> m_files;

  void do_read (db::Layout &layout, db::cell_index_type cell_index);
  void do_load_project (tl::TextInputStream &stream);
};

}

#endif

