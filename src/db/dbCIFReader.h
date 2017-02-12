
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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



#ifndef HDR_dbCIFReader
#define HDR_dbCIFReader

#include "tlException.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlString.h"

#include "dbLayout.h"
#include "dbReader.h"
#include "dbCIF.h"
#include "tlStream.h"
#include "dbStreamLayers.h"
#include "dbPropertiesRepository.h"

#include <map>
#include <set>

namespace db
{

/**
 *  @brief Structure that holds the CIF specific options for the reader
 */
class DB_PUBLIC CIFReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  CIFReaderOptions ()
    : wire_mode (0),
      dbu (0.001),
      create_other_layers (true)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief How to read 'W' objects
   *
   *  This property specifies how to read 'W' (wire) objects. 
   *  Allowed values are 0 (as square ended paths), 1 (as flush ended paths), 2 (as round paths)
   */
  unsigned int wire_mode; 

  /**
   *  @brief Specify the database unit to produce 
   *
   *  Specify the database unit which the resulting layout will receive.
   */
  double dbu;

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
    return new CIFReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("CIF");
    return n;
  }
};

/**
 *  @brief Generic base class of CIF reader exceptions
 */
class DB_PUBLIC CIFReaderException
  : public ReaderException
{
public:
  CIFReaderException (const std::string &msg, size_t l, const std::string &cell)
    : ReaderException (tl::sprintf (tl::to_string (QObject::tr ("%s (line=%ld, cell=%s)")), msg, l, cell))
  { }
};

/**
 *  @brief The CIF format stream reader
 */
class DB_PUBLIC CIFReader
  : public ReaderBase, 
    public CIFDiagnostics
{
public: 
  typedef std::vector<tl::Variant> property_value_list;

  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  CIFReader (tl::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~CIFReader ();

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
   *  which tells which CIF layer has been read into which logical
   *  layer.
   *
   *  @param layout The layout object to write to
   *  @return The LayerMap object
   */
  virtual const LayerMap &read (db::Layout &layout);

  /**
   *  @brief Format
   */
  virtual const char *format () const { return "CIF"; }

  /**
   *  @brief Issue an error with positional informations
   *
   *  Reimplements CIFDiagnostics
   */
  virtual void error (const std::string &txt);

  /**
   *  @brief Issue a warning with positional informations
   *
   *  Reimplements CIFDiagnostics
   */
  virtual void warn (const std::string &txt);

private:
  tl::TextInputStream m_stream;
  bool m_create_layers;
  LayerMap m_layer_map;
  tl::AbsoluteProgress m_progress;
  double m_dbu;
  unsigned int m_wire_mode;
  std::string m_cellname;
  std::string m_cmd_buffer;
  std::map <unsigned int, db::cell_index_type> m_cells_by_id;
  unsigned int m_next_layer_index;
  std::map <std::string, unsigned int> m_new_layers;

  void do_read (db::Layout &layout);

  const char *fetch_command ();
  bool read_cell (db::Layout &layout, db::Cell &cell, double sf, int level);
  void skip_blanks();
  void skip_sep ();
  void skip_comment ();
  char get_char ();
  bool test_semi ();
  int read_integer_digits ();
  int read_integer ();
  int read_sinteger ();
  const std::string &read_string ();
  const std::string &read_name ();
  double read_double ();
  void expect_semi ();
  void skip_to_end ();
};

}

#endif

