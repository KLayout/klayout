
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


#ifndef HDR_dbLoadLayoutOptions
#define HDR_dbLoadLayoutOptions

#include <string>
#include <map>

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbStreamLayers.h"

#include "gsiObject.h"
#include "gsiClass.h"
#include "tlVariant.h"

namespace db
{

/**
 *  @brief Base class for specific options for a certain format
 */
class DB_PUBLIC FormatSpecificReaderOptions
  : public gsi::ObjectBase
{
public:
  FormatSpecificReaderOptions () { }  
  virtual ~FormatSpecificReaderOptions () { }  //  to enable RTTI

  virtual FormatSpecificReaderOptions *clone () const = 0;
  virtual const std::string &format_name () const = 0;
};

/**
 *  @brief Options for loading layouts
 */
class DB_PUBLIC LoadLayoutOptions
{
public:
  /**
   *  @brief Default constructor
   */
  LoadLayoutOptions ();

  /**
   *  @brief Copy constructor
   */
  LoadLayoutOptions (const LoadLayoutOptions &d);

  /**
   *  @brief Assignment 
   */
  LoadLayoutOptions &operator= (const LoadLayoutOptions &d);

  /**
   *  @brief Destructor
   */
  ~LoadLayoutOptions ();

  /**
   *  @brief Gets the warning level
   *
   *  The warning level is a reader-specific setting which enables or disables warnings
   *  on specific levels. Level 0 is always "warnings off". The default level is 1
   *  which means "reasonable warnings emitted".
   */
  int warn_level () const
  {
    return m_warn_level;
  }

  /**
   *  @brief Sets the warning level
   */
  void set_warn_level (int w)
  {
    m_warn_level = w;
  }

  /**
   *  @brief Sets specific options for the given format
   *
   *  T is a type derived from FormatSpecificReaderOptions.
   *  In this version, the ownership over the options object is not transferred to the LoadLayoutOptions object.
   *
   *  @param options The options to use for reading the file
   *  @param format The format name for which to use these options
   */
  template <class T>
  void set_options (const T &options)
  {
    set_options (options.clone ());
  }

  /**
   *  @brief Sets specific options for the given format
   *
   *  T is a type derived from FormatSpecificReaderOptions.
   *  The ownership over the options object is transferred to the LoadLayoutOptions object.
   *
   *  @param options The options to use for reading the file
   *  @param format The format name for which to use these options
   */
  template <class T>
  void set_options (T *options)
  {
    std::map<std::string, FormatSpecificReaderOptions *>::iterator o = m_options.find (options->format_name ());
    if (o != m_options.end ()) {
      delete o->second;
      o->second = options;
    } else {
      m_options.insert (std::make_pair (options->format_name (), options));
    }
  }

  /**
   *  @brief Gets the format specific option object for the given format
   *
   *  T is a type derived from FormatSpecificReaderOptions.
   */
  template <class T>
  const T &get_options () const
  {
    static const T default_format;
    std::map <std::string, FormatSpecificReaderOptions *>::const_iterator o = m_options.find (default_format.format_name ());
    if (o != m_options.end () && dynamic_cast<const T *> (o->second)) {
      return *(dynamic_cast<const T *> (o->second));
    } else {
      return default_format;
    }
  }

  /**
   *  @brief Gets the format specific option object for the given format (non-const version)
   *
   *  @return 0, if there is no such object attached
   */
  template <class T>
  T &get_options ()
  {
    static const T default_format;
    std::map <std::string, FormatSpecificReaderOptions *>::iterator o = m_options.find (default_format.format_name ());
    if (o != m_options.end () && dynamic_cast<T *> (o->second)) {
      return *(dynamic_cast<T *> (o->second));
    } else {
      T *no = new T ();
      m_options [no->format_name ()] = no;
      return *no;
    }
  }

  /**
   *  @brief Gets the format specific options by format name
   *
   *  This version takes a generic FormatSpecificReaderOptions object and replaces or installs the
   *  options under the name delivered by the option object.
   */
  void set_options (const FormatSpecificReaderOptions &options);

  /**
   *  @brief Gets the format specific options by format name
   *
   *  This version takes a generic FormatSpecificReaderOptions object and replaces or installs the
   *  options under the name delivered by the option object.
   *
   *  The LoadLayoutOptions object will take ownership over the options object.
   */
  void set_options (FormatSpecificReaderOptions *options);

  /**
   *  @brief Gets the format specific options by format name
   *
   *  If no options are registered under the given name, 0 is returned.
   */
  const FormatSpecificReaderOptions *get_options (const std::string &name) const;

  /**
   *  @brief Gets the format specific options by format name
   *
   *  If no options are registered under the given name, 0 is returned.
   *  This is the non-const version.
   */
  FormatSpecificReaderOptions *get_options (const std::string &name);

  /**
   *  @brief Sets a layout reader option by name
   *
   *  The name is taken to be a GSI method which is called to set the
   *  option. For example, setting "gds2_unit", the method "gds2_unit=" is
   *  called with the given value.
   */
  void set_option_by_name (const std::string &name, const tl::Variant &value);

  /**
   *  @brief Gets a layout reader option by name
   *
   *  See "set_option_by_name" for details.
   */
  tl::Variant get_option_by_name (const std::string &name);

  /**
   *  @brief Sets a layout reader option by calling method
   *
   *  The name is taken to be a GSI method which is called to set the
   *  option.
   */
  void set_option_by_method (const std::string &name, const tl::Variant &value);

  /**
   *  @brief Gets a layout reader option by calling a method
   *
   *  See "set_option_by_method" for details.
   */
  tl::Variant get_option_by_method (const std::string &name);

private:
  std::map <std::string, FormatSpecificReaderOptions *> m_options;
  int m_warn_level;

  void release ();
};

}

#endif


