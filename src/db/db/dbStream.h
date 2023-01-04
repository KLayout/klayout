
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



#ifndef HDR_dbStream
#define HDR_dbStream

#include "dbCommon.h"

#include "dbSaveLayoutOptions.h"
#include "dbLoadLayoutOptions.h"

#include "tlClassRegistry.h"
#include "tlXMLParser.h"
#include "tlXMLWriter.h"

#include <string>
#include <vector>

namespace tl
{
  class InputStream;
}

namespace db
{

class ReaderBase;
class WriterBase;

/**
 *  @brief A stream format declaration
 */
class DB_PUBLIC StreamFormatDeclaration 
{
public:
  /**
   *  @brief Constructor
   */
  StreamFormatDeclaration () { }

  /**
   *  @brief Destructor
   */
  virtual ~StreamFormatDeclaration () { }

  /**
   *  @brief Obtain the format name
   */
  virtual std::string format_name () const = 0;

  /**
   *  @brief Obtain the format description
   */
  virtual std::string format_desc () const = 0;

  /**
   *  @brief Obtain the (long) format description
   */
  virtual std::string format_title () const = 0;

  /**
   *  @brief Obtain the file dialog format contribution
   */
  virtual std::string file_format () const = 0;

  /**
   *  @brief Auto-detect this format from the stream
   */
  virtual bool detect (tl::InputStream &s) const = 0;

  /**
   *  @brief Create the reader
   */
  virtual ReaderBase *create_reader (tl::InputStream &s) const = 0;

  /**
   *  @brief Returns true, when the format supports import (has a reader)
   */
  virtual bool can_read () const = 0;

  /**
   *  @brief Create the reader
   */
  virtual WriterBase *create_writer () const = 0;

  /**
   *  @brief Returns true, when the format supports export (has a writer)
   */
  virtual bool can_write () const = 0;

  /**
   *  @brief Delivers the XMLElement object that represents the reader options within a technology XML tree
   *
   *  This method is supposed to return an instance ReaderOptionsXMLElement<RO> where RO is the
   *  specific reader options type. The return value can be 0 to indicate there is no specific reader
   *  option.
   *
   *  The returned XMLElement is destroyed by the caller and needs to be a new object.
   */
  virtual tl::XMLElementBase *xml_reader_options_element () const
  {
    return 0;
  }

  /**
   *  @brief Delivers the XMLElement object that represents this writer options within a technology XML tree
   *
   *  This method is supposed to return an instance WriterOptionsXMLElement<WO> where WO is the
   *  specific writer options type. The return value can be 0 to indicate there is no specific writer
   *  option.
   *
   *  The returned XMLElement is destroyed by the caller and needs to be a new object.
   */
  virtual tl::XMLElementBase *xml_writer_options_element () const
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
    std::unique_ptr<OPT> opt (new OPT ());

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

/**
 *  @brief Returns the XMLElement list that can represent a db::LoadLayoutOptions object
 */
DB_PUBLIC tl::XMLElementList load_options_xml_element_list ();

/**
 *  @brief Returns the XMLElement list that can represent a db::SaveLayoutOptions object
 */
DB_PUBLIC tl::XMLElementList save_options_xml_element_list ();

}

#endif


