
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



#ifndef HDR_dbMALYReader
#define HDR_dbMALYReader

#include "dbPluginCommon.h"
#include "dbNamedLayerReader.h"
#include "dbLayout.h"
#include "dbMALY.h"
#include "dbMALYFormat.h"
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

/**
 *  @brief Generic base class of MALY reader exceptions
 */
class DB_PLUGIN_PUBLIC MALYReaderException
  : public ReaderException
{
public:
  MALYReaderException (const std::string &msg, size_t l, const std::string &file)
    : ReaderException (tl::sprintf (tl::to_string (tr ("%s (line=%ld, file=%s)")), msg, l, file))
  { }
};

/**
 *  @brief The MALY format stream reader
 */
class DB_PLUGIN_PUBLIC MALYReader
  : public NamedLayerReader,
    public MALYDiagnostics
{
public: 
  typedef std::vector<tl::Variant> property_value_list;

  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  MALYReader (tl::InputStream &s);

  /**
   *  @brief Destructor
   */
  ~MALYReader ();

  /**
   *  @brief Tests, if the stream is a valid MALY file
   *
   *  This method can be used for the format detection
   */
  bool test ();

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
   *  which tells which MALY layer has been read into which logical
   *  layer.
   *
   *  @param layout The layout object to write to
   *  @return The LayerMap object
   */
  virtual const LayerMap &read (db::Layout &layout);

  /**
   *  @brief Format
   */
  virtual const char *format () const { return "MALY"; }

  /**
   *  @brief Issue an error with positional information
   *
   *  Reimplements MALYDiagnostics
   */
  virtual void error (const std::string &txt);

  /**
   *  @brief Issue a warning with positional information
   *
   *  Reimplements MALYDiagnostics
   */
  virtual void warn (const std::string &txt, int wl = 1);

  /**
   *  @brief Reads the MALY file into a MALYData structure
   *
   *  This method is provided for test purposes mainly.
   */
  MALYData read_maly_file ();

private:
  struct MALYReaderTitleSpec
  {
    MALYReaderTitleSpec ()
      : given (false), enabled (false), width (1.0), height (1.0), pitch (1.0)
    { }

    bool given;
    bool enabled;
    db::DTrans trans;
    double width, height, pitch;
  };

  struct MALYReaderTitleData
  {
    MALYReaderTitleData ()
    { }

    MALYReaderTitleSpec date_spec;
    MALYReaderTitleSpec serial_spec;
    std::list<std::pair<std::string, MALYReaderTitleSpec> > string_titles;
  };

  struct MALYReaderParametersData
  {
    MALYReaderParametersData ()
      : base (BaseNotSet), array_base (BaseNotSet), masksize (0.0), maskmirror (false), font (MALYTitle::FontNotSet)
    { }

    enum Base
    {
      BaseNotSet,
      Origin,
      Center,
      LowerLeft
    };

    Base base;
    Base array_base;
    double masksize;
    bool maskmirror;
    MALYTitle::Font font;
    std::list<std::pair<std::string, std::string> > roots;
  };

  struct MALYReaderStrRefData
  {
    MALYReaderStrRefData ()
      : layer (-1), scale (1.0), nx (1), ny (1), dx (0.0), dy (0.0)
    { }

    std::string file;
    std::string name;
    std::string dname, ename, mname;
    int layer;
    db::DVector org;
    db::DBox size;
    double scale;
    int nx, ny;
    double dx, dy;
  };

  struct MALYReaderStrGroupData
  {
    std::string name;
    std::list<MALYReaderStrRefData> refs;
  };

  struct MALYReaderMaskData
  {
    std::string name;
    MALYReaderParametersData parameters;
    MALYReaderTitleData title;
    std::list<MALYReaderStrGroupData> strgroups;
  };

  tl::TextInputStream m_stream;
  tl::AbsoluteProgress m_progress;
  double m_dbu;
  unsigned int m_last_record_line;
  std::string m_record;
  std::string m_record_returned;
  std::list<std::string> m_sections;

  void import_data (db::Layout &layout, const MALYData &data);
  void create_metadata (db::Layout &layout, const MALYData &data);
  tl::Extractor read_record ();
  void unget_record ();
  std::string read_record_internal ();
  void do_read_maly_file (MALYData &data);
  bool read_maskset (MALYData &data);
  void read_mask (MALYReaderMaskData &mask);
  void read_title (MALYReaderTitleData &mask);
  void read_parameter (MALYReaderParametersData &mask);
  void read_strgroup (MALYReaderStrGroupData &mask);
  db::DTrans extract_title_trans (tl::Extractor &ex);
  void extract_title_trans (tl::Extractor &ex, MALYReaderTitleSpec &spec);
  bool begin_section (tl::Extractor &ex, const std::string &name = std::string ());
  bool end_section (tl::Extractor &ex);
  void skip_section ();
  MALYTitle create_title (MALYTitle::Type type, const MALYReaderTitleSpec &data, MALYTitle::Font font, bool maskmirror, const std::string &string);
  void create_masks (const MALYReaderMaskData &cmask, const std::list<MALYReaderMaskData> &masks, MALYData &data);
  MALYStructure create_structure (const MALYReaderParametersData &mparam, const MALYReaderParametersData &cparam, const MALYReaderStrRefData &data, const std::string &strgroup_name, MALYReaderParametersData::Base base, MALYReaderParametersData::Base array_base);
  std::string resolve_path (const MALYReaderParametersData &param, const std::string &path);
  static MALYReaderParametersData::Base string_to_base (const std::string &string);
};

}

#endif

