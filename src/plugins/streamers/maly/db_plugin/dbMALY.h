
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_dbMALY
#define HDR_dbMALY

#include "dbPoint.h"
#include "dbTrans.h"
#include "dbBox.h"
#include "dbPluginCommon.h"

#include "tlException.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlAssert.h"

#include <string>
#include <vector>

namespace db
{

/**
 *  @brief The diagnostics interface for reporting problems in the reader or writer
 */
class MALYDiagnostics
{
public:
  virtual ~MALYDiagnostics ();

  /**
   *  @brief Issue an error with positional information
   */
  virtual void error (const std::string &txt) = 0;

  /**
   *  @brief Issue a warning with positional information
   */
  virtual void warn (const std::string &txt, int warn_level) = 0;
};

/**
 *  @brief A class representing a title field on a mask
 */
class DB_PLUGIN_PUBLIC MALYTitle
{
public:
  /**
   *  @brief Default constructor
   */
  MALYTitle ()
    : width (0.0), height (0.0), pitch (0.0), type (String), font (Standard)
  { }

  /**
   *  @brief The type of the title
   */
  enum Type
  {
    String = 0,     //  A user-defined string
    Date = 1,       //  The date
    Serial = 2      //  A serial number
  };

  /**
   *  @brief The font to be used
   */
  enum Font
  {
    FontNotSet = 0, //  Undef
    Standard = 1,   //  Standard font
    Native = 2      //  Native tool font
  };

  /**
   *  @brief The string for "String" type
   */
  std::string string;

  /**
   *  @brief The transformation of the title
   *
   *  The origin of the title is supposed to be in the center of
   *  the title field.
   */
  db::DTrans transformation;

  /**
   *  @brief Optional font parameters: character width
   */
  double width;

  /**
   *  @brief Optional font parameters: character height
   */
  double height;

  /**
   *  @brief Optional font parameters: character pitch
   */
  double pitch;

  /**
   *  @brief The type of the title
   */
  Type type;

  /**
   *  @brief The font to be used
   */
  Font font;

  /**
   *  @brief Returns a string representing the structure
   */
  std::string to_string () const;
};

/**
 *  @brief A class representing a structure (pattern) on a mask
 */
class DB_PLUGIN_PUBLIC MALYStructure
{
public:
  /**
   *  @brief Default constructor
   */
  MALYStructure ()
    : nx (1), ny (1), dx (0.0), dy (0.0), layer (-1)
  { }

  /**
   *  @brief The (expanded) path of the pattern file
   */
  std::string path;

  /**
   *  @brief The name of the top cell
   *  If empty, the topcell is determined automatically
   */
  std::string topcell;

  /**
   *  @brief The pattern window in the original file
   */
  db::DBox size;

  /**
   *  @brief The transformation needed to place the original file
   */
  db::DCplxTrans transformation;

  /**
   *  @brief The number of placements in x direction
   */
  int nx;

  /**
   *  @brief The number of placements in y direction
   */
  int ny;

  /**
   *  @brief The placement pitch in x direction (if nx > 1)
   */
  double dx;

  /**
   *  @brief The placement pitch in y direction (if ny > 1)
   */
  double dy;

  /**
   *  @brief The design name
   */
  std::string dname;

  /**
   *  @brief The name for the mask process
   */
  std::string mname;

  /**
   *  @brief The name for the mask tool
   */
  std::string ename;

  /**
   *  @brief The layer used from the OASIS file
   *
   *  A value of -1 means "all".
   */
  int layer;

  /**
   *  @brief Returns a string representing the structure
   */
  std::string to_string () const;
};

/**
 *  @brief A class representing one mask
 */
class DB_PLUGIN_PUBLIC MALYMask
{
public:
  /**
   *  @brief Default constructor
   */
  MALYMask ()
    : size_um (0.0)
  { }

  /**
   *  @brief Size of the mask in micrometers
   */
  double size_um;

  /**
   *  @brief Name of the mask
   *
   *  This is also the name of the layer generated
   */
  std::string name;

  /**
   *  @brief The list of structures
   */
  std::list<MALYStructure> structures;

  /**
   *  @brief The list of titles
   */
  std::list<MALYTitle> titles;

  /**
   *  @brief Returns a string representing the mask
   */
  std::string to_string () const;
};

/**
 *  @brief A class representing the MALY file
 */
class DB_PLUGIN_PUBLIC MALYData
{
public:
  /**
   *  @brief Default constructor
   */
  MALYData ()
  { }

  /**
   *  @brief The masks defined by the file
   */
  std::list<MALYMask> masks;

  /**
   *  @brief Returns a string representing the data set
   */
  std::string to_string () const;
};

}

#endif

