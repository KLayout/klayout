
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

#if defined(HAVE_QT)

#ifndef HDR_layStream_h
#define HDR_layStream_h

#include "laybasicCommon.h"

#include "layPlugin.h"
#include "tlXMLParser.h"
#include "tlXMLWriter.h"
#include "dbLoadLayoutOptions.h"

#include <QFrame>

namespace db
{
  class StreamFormatDeclaration;
  class FormatSpecificWriterOptions;
  class FormatSpecificReaderOptions;
  class LoadLayoutOptions;
  class SaveLayoutOptions;
  class Technology;
}

namespace lay
{

class Dispatcher;
class LayoutHandle;

/**
 *  @brief The base class for writer configuration pages
 *
 *  This interface defines some services the configuration page
 *  must provide (i.e. setup, commit)
 */
class LAYBASIC_PUBLIC StreamWriterOptionsPage 
  : public QFrame
{
public:
  StreamWriterOptionsPage (QWidget *parent) 
    : QFrame (parent)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Load the page
   *
   *  The implementation is supposed to fetch the configuration from the
   *  Plugin object provided and load the widgets accordingly.
   *  The options object can be cast to the specific format object.
   */
  virtual void setup (const db::FormatSpecificWriterOptions * /*options*/, const db::Technology * /*tech*/)
  {
    //  the default implementation does nothing.
  }

  /**
   *  @brief Commit the page
   *
   *  The implementation is supposed to read the configuration (and 
   *  throw exceptions if the configuration something is invalid)
   *  and commit the changes through 
   *  The options object can be cast to the specific format object.
   */
  virtual void commit (db::FormatSpecificWriterOptions * /*options*/, const db::Technology * /*tech*/, bool /*gzip*/)
  {
    //  the default implementation does nothing.
  }
};

/**
 *  @brief The base class for reader configuration pages
 *
 *  This interface defines some services the configuration page
 *  must provide (i.e. setup, commit)
 */
class LAYBASIC_PUBLIC StreamReaderOptionsPage 
  : public QFrame
{
public:
  StreamReaderOptionsPage (QWidget *parent) 
    : QFrame (parent)
  {
    // .. nothing else ..
  }

  /**
   *  @brief Load the page
   *
   *  The implementation is supposed to fetch the configuration from the
   *  Plugin object provided and load the widgets accordingly.
   *  The options object can be cast to the specific format object.
   */
  virtual void setup (const db::FormatSpecificReaderOptions * /*options*/, const db::Technology * /*tech*/)
  {
    //  the default implementation does nothing.
  }

  /**
   *  @brief Commit the page
   *
   *  The implementation is supposed to read the configuration (and 
   *  throw exceptions if the configuration something is invalid)
   *  and commit the changes through 
   *  The options object can be cast to the specific format object.
   */
  virtual void commit (db::FormatSpecificReaderOptions * /*options*/, const db::Technology * /*tech*/)
  {
    //  the default implementation does nothing.
  }
};

/**
 *  This plugin specializations add the stream readers and writers to the configuration
 *  system. The plugins can provide menu entries, configuration parameters, configuration
 *  pages etc.
 */  
  
class LAYBASIC_PUBLIC StreamPluginDeclarationBase
  : public PluginDeclaration
{
public:
  StreamPluginDeclarationBase (const std::string &format_name)
    : PluginDeclaration (), m_format_name (format_name), mp_stream_fmt (0)
  { 
    //  .. nothing yet ..
  }

  db::StreamFormatDeclaration &stream_fmt ();

  const db::StreamFormatDeclaration &stream_fmt () const
  {
    //  dirty hack:
    return const_cast <StreamPluginDeclarationBase *> (this)->stream_fmt ();
  }

  const std::string &format_name () const
  {
    return m_format_name;
  }

private:
  std::string m_format_name;
  db::StreamFormatDeclaration *mp_stream_fmt;

  //  don't allow overrides - use a special configuration page for that purpose.
  virtual ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    return 0;
  }
};
  
/**
 *  @brief A specialization of Plugin declaration for stream reader plugins
 */
class LAYBASIC_PUBLIC StreamReaderPluginDeclaration
  : public StreamPluginDeclarationBase
{
public:
  /**
   *  @brief Constructor
   */
  StreamReaderPluginDeclaration (const std::string &format_name)
    : StreamPluginDeclarationBase (format_name)
  { 
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the plugin for a given format name
   */
  static const StreamReaderPluginDeclaration *plugin_for_format (const std::string &format_name);

  /**
   *  @brief Create a format specific options page 
   */
  virtual StreamReaderOptionsPage *format_specific_options_page (QWidget * /*parent*/) const 
  {
    return 0;
  }

  /**
   *  @brief Create a format specific options object from the configuration
   *
   *  This method is supposed to create a format specific options object.
   */
  virtual db::FormatSpecificReaderOptions *create_specific_options () const
  {
    return 0;
  }
};

/**
 *  @brief A specialization of Plugin declaration for stream reader plugins
 */
class LAYBASIC_PUBLIC StreamWriterPluginDeclaration
  : public StreamPluginDeclarationBase
{
public:
  StreamWriterPluginDeclaration (const std::string &format_name)
    : StreamPluginDeclarationBase (format_name)
  { 
    //  .. nothing yet ..
  }

  /**
   *  @brief Gets the plugin for a given format name
   */
  static const StreamWriterPluginDeclaration *plugin_for_format (const std::string &format_name);

  /**
   *  @brief If the options are shared with another declaration, returns this name of this declaration here
   */
  virtual const char *options_alias () const
  {
    return 0;
  }

  /**
   *  @brief Create a format specific options page 
   */
  virtual StreamWriterOptionsPage *format_specific_options_page (QWidget * /*parent*/) const 
  {
    return 0;
  }

  /**
   *  @brief Create a format specific options object from the configuration
   *
   *  This method is supposed to create a format specific options object or return 0
   *  if there is no such object.
   */
  virtual db::FormatSpecificWriterOptions *create_specific_options () const
  {
    return 0;
  }

  /**
   *  @brief Initialize the writer options from a layout handle
   *
   *  The layout handle carries information about meta data read and similar. This
   *  method gives the plugin a chance to modify the options based on the meta data
   *  of the layout.
   */
  virtual void initialize_options_from_layout_handle (db::FormatSpecificWriterOptions * /*options*/, const lay::LayoutHandle & /*lh*/) const
  {
    //  the default implementation does nothing.
  }
};

}

#endif

#endif  //  defined(HAVE_QT)
