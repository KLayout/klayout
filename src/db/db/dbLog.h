
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

#ifndef _HDR_dbLog
#define _HDR_dbLog

#include "dbCommon.h"
#include "dbPolygon.h"

#include <string>

namespace db
{

/**
 *  @brief An enum describing the severity for a log entry
 */
enum Severity {
  NoSeverity = 0,   //  unspecific
  Info = 1,         //  information only
  Warning = 2,      //  a warning
  Error = 3         //  an error
};

/**
 *  @brief A generic log entry
 *
 *  This object can be used for collecting errors or warnings.
 *  It features a message and a severity level and optionally
 *  a polygon (for geometry marker), a category name and a category description.
 */
class DB_PUBLIC LogEntryData
{
public:
  typedef size_t string_id_type;

  /**
   *  @brief Creates a log entry
   */
  LogEntryData ();

  /**
   *  @brief Creates a log entry with the severity and a message
   */
  LogEntryData (Severity s, const std::string &msg);

  /**
   *  @brief Creates an error with the severity, a cell name and a message
   */
  LogEntryData (Severity s, const std::string &cell_name, const std::string &msg);

  /**
   *  @brief Equality
   */
  bool operator== (const LogEntryData &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const LogEntryData &other) const
  {
    return ! operator== (other);
  }

  /**
   *  @brief Sets the severity
   */
  void set_severity (Severity severity)
  {
    m_severity = severity;
  }

  /**
   *  @brief Gets the severity
   */
  Severity severity () const
  {
    return m_severity;
  }

  /**
   *  @brief The category name of the error
   *  Specifying the category name is optional. If a category is given, it will be used for
   *  the report.
   */
  const std::string &category_name () const;

  /**
   *  @brief Sets the category name
   */
  void set_category_name (const std::string &s);

  /**
   *  @brief The category description of the error
   *  Specifying the category description is optional. If a category is given, this attribute will
   *  be used for the category description.
   */
  const std::string &category_description () const;

  /**
   *  @brief Sets the category description
   */
  void set_category_description (const std::string &s);

  /**
   *  @brief Gets the geometry for this error
   *  Not all errors may specify a geometry. In this case, the polygon is empty.
   */
  const db::DPolygon &geometry () const
  {
    return m_geometry;
  }

  /**
   *  @brief Sets the geometry
   */
  void set_geometry (const db::DPolygon &g)
  {
    m_geometry = g;
  }

  /**
   *  @brief Gets the message for this error
   */
  const std::string &message () const;

  /**
   *  @brief Sets the message
   */
  void set_message (const std::string &n);

  /**
   *  @brief Gets the cell name the error occurred in
   */
  const std::string &cell_name () const;

  /**
   *  @brief Sets the cell name
   */
  void set_cell_name (const std::string &n);

  /**
   *  @brief Formats this message for printing
   */
  std::string to_string (bool with_geometry = true) const;

private:
  Severity m_severity;
  string_id_type m_cell_name;
  string_id_type m_message;
  db::DPolygon m_geometry;
  string_id_type m_category_name, m_category_description;
};



}

#endif
