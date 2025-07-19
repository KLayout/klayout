
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



#ifndef HDR_dbReader
#define HDR_dbReader

#include "dbCommon.h"

#include "tlException.h"
#include "tlInternational.h"
#include "tlString.h"

#include "tlStream.h"
#include "dbLoadLayoutOptions.h"

#include <vector>

namespace db
{

class Layout;
class ReaderBase;

/**
 *  @brief Generic base class of reader exceptions
 */
class DB_PUBLIC ReaderException
  : public tl::Exception 
{
public:
  ReaderException (const std::string &msg)
    : tl::Exception (msg)
  { }
};

/**
 *  @brief A class representing the "unknown format" reader error
 *
 *  The purpose of this class is supply the header bytes of the
 *  data stream for analysis.
 */
class DB_PUBLIC ReaderUnknownFormatException
  : public ReaderException
{
public:
  ReaderUnknownFormatException (const std::string &msg, const std::string &data, bool has_more)
    : ReaderException (msg), m_data (data), m_has_more (has_more)
  { }

  const std::string &data () const
  {
    return m_data;
  }

  bool has_more () const
  {
    return m_has_more;
  }

private:
  std::string m_data;
  bool m_has_more;
};

/**
 *  @brief Joins layer names into a single, combined layer
 *  @param s The first layer name and output
 *  @param n The name to add
 */
DB_PUBLIC void
join_layer_names (std::string &s, const std::string &n);

/**
 *  @brief The generic reader base class
 */
class DB_PUBLIC ReaderBase
{
public:
  ReaderBase ();
  virtual ~ReaderBase ();

  virtual const db::LayerMap &read (db::Layout &layout, const db::LoadLayoutOptions &options) = 0;
  virtual const db::LayerMap &read (db::Layout &layout) = 0;
  virtual const char *format () const = 0;

  /**
   *  @brief Sets a flag indicating that warnings shall be treated as errors
   *  If this flag is set, warnings will be handled as errors.
   */
  void set_warnings_as_errors (bool f);

  /**
   *  @brief Gets a flag indicating that warnings shall be treated as errors
   */
  bool warnings_as_errors () const
  {
    return m_warnings_as_errors;
  }

  /**
   *  @brief Gets the warning level
   */
  int warn_level () const
  {
    return m_warn_level;
  }

  /**
   *  @brief Returns true (once) if this is the first warning
   */
  bool first_warning ();

  /**
   *  @brief Returns a value indicating whether to compress the given warning
   *
   *  The return value is either -1 (do not skip), 0 (first warning not to be shown), 1 (warning not shown(.
   */
  int compress_warning (const std::string &msg);

  /**
   *  @brief Sets the expected database unit
   *
   *  With this value set, the reader can check if the present database unit is
   *  compatible with the expected one and either take actions to scale the layouts
   *  or to reject the file.
   *
   *  Setting the value to 0 resets the expected DBU and will disable all checks
   *  or scaling.
   */
  void set_expected_dbu (double dbu);

  /**
   *  @brief Gets the expected database unit
   */
  double expected_dbu () const
  {
    return m_expected_dbu;
  }

  /**
   *  @brief Checks the given DBU against the expected one
   *
   *  This method will raise an exception if the database units do not match.
   */
  void check_dbu (double dbu) const;

protected:
  virtual void init (const db::LoadLayoutOptions &options);

private:
  bool m_warnings_as_errors;
  int m_warn_level;
  std::string m_last_warning;
  int m_warn_count_for_same_message;
  bool m_first_warning;
  double m_expected_dbu;
};

/**
 *  @brief The generic stream reader 
 *
 *  This reader is supposed to fork to one of the specific readers
 *  depending on the format detected.
 */
class DB_PUBLIC Reader
{
public: 
  /**
   *  @brief Construct a reader object
   *
   *  If no valid format can be detected, the constructor will throw 
   *  an exception. The stream must be opened already in order to allow
   *  format detection.
   *
   *  @param s The stream object from which to read stream data from
   */
  Reader (tl::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~Reader ();

  /** 
   *  @brief The basic read method 
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  It can be passed options with a layer map which tells which
   *  OASIS layer(s) to read on which logical layers.
   *  In addition, a flag can be passed that tells whether to create 
   *  new layers. The returned map will contain all layers, the passed
   *  ones and the newly created ones.
   *
   *  @param layout The layout object to write to
   *  @param options The LayerMap object
   */
  const db::LayerMap &read (db::Layout &layout, const db::LoadLayoutOptions &options);

  /** 
   *  @brief The basic read method (without mapping)
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  This version will read all input layers and return a map
   *  which tells which OASIS layer has been read into which logical
   *  layer.
   *
   *  @param layout The layout object to write to
   *  @return The LayerMap object
   */
  const db::LayerMap &read (db::Layout &layout);

  /**
   *  @brief Returns a format describing the file format found
   */
  const char *format () const
  {
    return mp_actual_reader->format ();
  }

  /**
   *  @brief Sets a flag indicating that warnings shall be treated as errors
   *  If this flag is set, warnings will be handled as errors.
   */
  void set_warnings_as_errors (bool f)
  {
    mp_actual_reader->set_warnings_as_errors (f);
  }

  /**
   *  @brief Gets a flag indicating that warnings shall be treated as errors
   */
  bool warnings_as_errors () const
  {
    return mp_actual_reader->warnings_as_errors ();
  }

  /**
   *  @brief Sets the expected database unit (see ReaderBase)
   */
  void set_expected_dbu (double dbu)
  {
    return mp_actual_reader->set_expected_dbu (dbu);
  }

  /**
   *  @brief Gets the expected database unit
   */
  double expected_dbu () const
  {
    return mp_actual_reader->expected_dbu ();
  }

private:
  ReaderBase *mp_actual_reader;
  tl::InputStream &m_stream;
};

}

#endif

