
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


#ifndef HDR_layStream_h
#define HDR_layStream_h

#include "laybasicCommon.h"

#include "layPlugin.h"
#include "tlXMLParser.h"
#include "tlXMLWriter.h"
#include "dbLoadLayoutOptions.h"

namespace db
{
  class StreamFormatDeclaration;
  class FormatSpecificWriterOptions;
  class FormatSpecificReaderOptions;
  class LoadLayoutOptions;
  class SaveLayoutOptions;
}

namespace lay
{

class PluginRoot;
class LayoutHandle;
class Technology;

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
  virtual void setup (const db::FormatSpecificWriterOptions * /*options*/, const lay::Technology * /*tech*/)
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
  virtual void commit (db::FormatSpecificWriterOptions * /*options*/, const lay::Technology * /*tech*/, bool /*gzip*/)
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
  virtual void setup (const db::FormatSpecificReaderOptions * /*options*/, const lay::Technology * /*tech*/)
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
  virtual void commit (db::FormatSpecificReaderOptions * /*options*/, const lay::Technology * /*tech*/)
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

  //  don't allow to override - use a special configuration page for that purpose.
  virtual ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    return 0;
  }
};
  
/**
 *  @brief A specialisation of Plugin declaration for stream reader plugins
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

  /**
   *  @brief Delivers the XMLElement object that represents this component within a technology XML tree
   *
   *  This method is supposed to return an instance ReaderOptionsXMLElement<RO> where RO is the
   *  specific reader options type. The return value can be 0 to indicate there is no specific reader
   *  option.
   *
   *  The returned XMLElement is destroyed by the caller and needs to be a new object.
   */
  virtual tl::XMLElementBase *xml_element () const
  {
    return 0;
  }
};

/**
 *  @brief Returns the XMLElement list that can represent a db::LoadLayoutOptions object
 */
LAYBASIC_PUBLIC tl::XMLElementList load_options_xml_element_list ();

/**
 *  @brief Returns the XMLElement list that can represent a db::SaveLayoutOptions object
 */
LAYBASIC_PUBLIC tl::XMLElementList save_options_xml_element_list ();

/**
 *  @brief A specialisation of Plugin declaration for stream reader plugins
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

  /**
   *  @brief Delivers the XMLElement object that represents this component within a technology XML tree
   *
   *  This method is supposed to return an instance WriterOptionsXMLElement<WO> where WO is the
   *  specific writer options type. The return value can be 0 to indicate there is no specific writer
   *  option.
   *
   *  The returned XMLElement is destroyed by the caller and needs to be a new object.
   */
  virtual tl::XMLElementBase *xml_element () const
  {
    return 0;
  }
};

/**
 *  @brief A helper class for the XML serialization of the stream options (custom read adaptor)
 *
 *  OPT is a reader or writer options class and HOST is the host class. For example, OPT
 *  can be db::GDS2ReaderOptions and HOST then is db::LoadLayoutOptions.
 */
template <class OPT, class HOST>
class StreamOptionsReadAdaptor
{
public:
  typedef tl::pass_by_ref_tag tag;

  StreamOptionsReadAdaptor ()
    : mp_options (0), m_done (false)
  {
    // .. nothing yet ..
  }

  const OPT &operator () () const
  {
    return mp_options->template get_options<OPT> ();
  }

  bool at_end () const
  {
    return m_done;
  }

  void start (const HOST &options)
  {
    mp_options = &options;
    m_done = false;
  }

  void next ()
  {
    mp_options = 0;
    m_done = true;
  }

private:
  const HOST *mp_options;
  bool m_done;
};

/**
 *  @brief A helper class for the XML serialization of the stream option (custom write adaptor)
 *
 *  See StreamOptionsReadAdaptor for details.
 */
template <class OPT, class HOST>
class StreamOptionsWriteAdaptor
{
public:
  StreamOptionsWriteAdaptor ()
  {
    // .. nothing yet ..
  }

  void operator () (HOST &options, tl::XMLReaderState &reader) const
  {
    std::auto_ptr<OPT> opt (new OPT ());

    tl::XMLObjTag<OPT> tag;
    *opt = *reader.back (tag);

    options.set_options (opt.release ());
  }
};

/**
 *  @brief A XMLElement specialization for stream options
 */
template <class OPT, class HOST>
class StreamOptionsXMLElement
  : public tl::XMLElement<OPT, HOST, StreamOptionsReadAdaptor<OPT, HOST>, StreamOptionsWriteAdaptor<OPT, HOST> >
{
public:
  StreamOptionsXMLElement (const std::string &element_name, const tl::XMLElementList &children)
    : tl::XMLElement<OPT, HOST, StreamOptionsReadAdaptor<OPT, HOST>, StreamOptionsWriteAdaptor<OPT, HOST> > (StreamOptionsReadAdaptor<OPT, HOST> (), StreamOptionsWriteAdaptor<OPT, HOST> (), element_name, children)
  {
    //  .. nothing yet ..
  }

  StreamOptionsXMLElement (const StreamOptionsXMLElement &d)
    : tl::XMLElement<OPT, HOST, StreamOptionsReadAdaptor<OPT, HOST>, StreamOptionsWriteAdaptor<OPT, HOST> > (d)
  {
    //  .. nothing yet ..
  }
};

/**
 *  @brief A custom XMLElement for the serialization of reader options
 *
 *  StreamReaderPluginDeclaration::xml_element can return such an element to
 *  insert a custom XML element into the XML tree which represents the
 *  reader options.
 */
template <class OPT>
class ReaderOptionsXMLElement
  : public StreamOptionsXMLElement<OPT, db::LoadLayoutOptions>
{
public:
  ReaderOptionsXMLElement (const std::string &element_name, const tl::XMLElementList &children)
    : StreamOptionsXMLElement<OPT, db::LoadLayoutOptions> (element_name, children)
  {
    //  .. nothing yet ..
  }

  ReaderOptionsXMLElement (const ReaderOptionsXMLElement &d)
    : StreamOptionsXMLElement<OPT, db::LoadLayoutOptions> (d)
  {
    //  .. nothing yet ..
  }

  virtual tl::XMLElementBase *clone () const
  {
    return new ReaderOptionsXMLElement (*this);
  }
};

/**
 *  @brief A custom XMLElement for the serialization of writer options
 *
 *  StreamWriterPluginDeclaration::xml_element can return such an element to
 *  insert a custom XML element into the XML tree which represents the
 *  writer options.
 */
template <class OPT>
class WriterOptionsXMLElement
  : public StreamOptionsXMLElement<OPT, db::SaveLayoutOptions>
{
public:
  WriterOptionsXMLElement (const std::string &element_name, const tl::XMLElementList &children)
    : StreamOptionsXMLElement<OPT, db::SaveLayoutOptions> (element_name, children)
  {
    //  .. nothing yet ..
  }

  WriterOptionsXMLElement (const WriterOptionsXMLElement &d)
    : StreamOptionsXMLElement<OPT, db::SaveLayoutOptions> (d)
  {
    //  .. nothing yet ..
  }

  virtual tl::XMLElementBase *clone () const
  {
    return new WriterOptionsXMLElement (*this);
  }
};

}

#endif

