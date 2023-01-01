
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


#include "tlXMLParser.h"
#include "tlString.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "tlProgress.h"

#include <cstring>
#include <memory>

#if defined(HAVE_EXPAT)

#include <expat.h>

namespace tl
{

// --------------------------------------------------------------------
//  SourcePrivateData implementation

class XMLSourcePrivateData
{
public:
  XMLSourcePrivateData (tl::InputStream *stream)
    : mp_stream_holder (stream),
      m_has_error (false)
  {
    mp_stream = stream;
  }

  XMLSourcePrivateData (tl::InputStream *stream, const std::string &progress_message)
    : mp_stream_holder (stream),
      mp_progress (new AbsoluteProgress (progress_message, 100)),
      m_has_error (false)
  {
    mp_stream = stream;
    mp_progress->set_format (tl::to_string (tr ("%.0f MB")));
    mp_progress->set_unit (1024 * 1024);
  }

  XMLSourcePrivateData (tl::InputStream &stream)
    : m_has_error (false)
  {
    mp_stream = &stream;
  }

  XMLSourcePrivateData (tl::InputStream &stream, const std::string &progress_message)
    : mp_progress (new AbsoluteProgress (progress_message, 100)),
      m_has_error (false)
  {
    mp_stream = &stream;
    mp_progress->set_format (tl::to_string (tr ("%.0f MB")));
    mp_progress->set_unit (1024 * 1024);
  }

  size_t read (char *data, size_t n)
  {
    try {

      if (mp_progress.get ()) {
        mp_progress->set (mp_stream->pos ());
      }

      size_t n0 = n;
      for (const char *rd = 0; n > 0 && (rd = mp_stream->get (1)) != 0; --n) {
        *data++ = *rd;
      }

      if (n0 == n) {
        return -1;
      } else {
        return n0 - n;
      }

    } catch (tl::Exception &ex) {
      m_error = ex.msg ();
      m_has_error = true;
      return -1;
    }
  }

  bool has_error () const
  {
    return m_has_error;
  }

  const std::string &error_msg () const
  {
    return m_error;
  }

  void reset ()
  {
    mp_stream->reset ();
  }

private:
  std::unique_ptr<tl::InputStream> mp_stream_holder;
  tl::InputStream *mp_stream;
  std::unique_ptr<tl::AbsoluteProgress> mp_progress;
  bool m_has_error;
  std::string m_error;
};

// --------------------------------------------------------------------
//  XMLSource implementation

XMLSource::XMLSource ()
  : mp_source (0)
{
  //  .. nothing yet ..
}

XMLSource::~XMLSource ()
{
  delete mp_source;
  mp_source = 0;
}

void
XMLSource::reset ()
{
  mp_source->reset ();
}

// --------------------------------------------------------------------
//  XMLStringSource implementation

XMLStringSource::XMLStringSource (const std::string &string)
  : m_copy (string)
{
  set_source (new XMLSourcePrivateData (new tl::InputStream (new tl::InputMemoryStream (m_copy.c_str (), string.size ()))));
}

XMLStringSource::XMLStringSource (const char *cp)
{
  set_source (new XMLSourcePrivateData (new tl::InputStream (new tl::InputMemoryStream (cp, strlen (cp)))));
}

XMLStringSource::XMLStringSource (const char *cp, size_t len)
{
  set_source (new XMLSourcePrivateData (new tl::InputStream (new tl::InputMemoryStream (cp, len))));
}

XMLStringSource::~XMLStringSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLFileSource implementation

XMLFileSource::XMLFileSource (const std::string &path, const std::string &progress_message)
{
  set_source (new XMLSourcePrivateData (new tl::InputStream (path), progress_message));
}

XMLFileSource::XMLFileSource (const std::string &path)
{
  set_source (new XMLSourcePrivateData (new tl::InputStream (path)));
}

XMLFileSource::~XMLFileSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLStreamSource implementation

XMLStreamSource::XMLStreamSource (tl::InputStream &s, const std::string &progress_message)
{
  set_source (new XMLSourcePrivateData (s, progress_message));
}

XMLStreamSource::XMLStreamSource (tl::InputStream &s)
{
  set_source (new XMLSourcePrivateData (s));
}

XMLStreamSource::~XMLStreamSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLParser implementation

void XMLCALL start_element_handler (void *user_data, const XML_Char *name, const XML_Char **atts);
void XMLCALL end_element_handler (void *user_data, const XML_Char *name);
void XMLCALL cdata_handler (void *user_data, const XML_Char *s, int len);

static std::string get_lname (const std::string &name)
{
  size_t colon = name.find (':');
  if (colon != std::string::npos) {
    return std::string (name, colon + 1, name.size () - colon - 1);
  } else {
    return name;
  }
}

class XMLParserPrivateData
{
public:
  XMLParserPrivateData ()
    : mp_struct_handler (0)
  {
    mp_parser = XML_ParserCreate ("UTF-8");
    tl_assert (mp_parser != NULL);
  }

  ~XMLParserPrivateData ()
  {
    if (mp_parser != NULL) {
      XML_ParserFree (mp_parser);
    }
  }

  void start_element (const std::string &name)
  {
    try {
      //  TODO: Provide namespace URI?
      mp_struct_handler->start_element (std::string (), get_lname (name), name);
    } catch (tl::Exception &ex) {
      error (ex);
    }
  }

  void end_element (const std::string &name)
  {
    try {
      //  TODO: Provide namespace URI?
      mp_struct_handler->end_element (std::string (), get_lname (name), name);
    } catch (tl::Exception &ex) {
      error (ex);
    }
  }

  void cdata (const std::string &cdata)
  {
    try {
      mp_struct_handler->characters (cdata);
    } catch (tl::Exception &ex) {
      error (ex);
    }
  }

  void parse (tl::XMLSource &source, XMLStructureHandler &struct_handler)
  {
    m_has_error = false;
    mp_struct_handler = &struct_handler;

    //  Just in case we want to reuse it ...
    XML_ParserReset (mp_parser, NULL);
    XML_SetUserData (mp_parser, (void *) this);
    XML_SetElementHandler (mp_parser, start_element_handler, end_element_handler);
    XML_SetCharacterDataHandler (mp_parser, cdata_handler);

    const size_t chunk = 65536;
    char buffer [chunk];

    size_t n;

    do {

      try {

        n = source.source ()->read (buffer, chunk);

        XML_Status status = XML_Parse (mp_parser, buffer, int (n), n < chunk /*is final*/);
        if (status == XML_STATUS_ERROR) {
          m_has_error = true;
          m_error = XML_ErrorString (XML_GetErrorCode (mp_parser));
          m_error_line = XML_GetErrorLineNumber (mp_parser);
          m_error_column = XML_GetErrorColumnNumber (mp_parser);
        }

      } catch (tl::Exception &ex) {
        error (ex);
      }

    } while (n == chunk && !m_has_error);
  }

  void check_error ()
  {
    if (m_has_error) {
      throw tl::XMLLocatedException (m_error, m_error_line, m_error_column);
    }
  }

private:
  void error (tl::Exception &ex)
  {
    m_has_error = true;
    m_error_line = XML_GetCurrentLineNumber (mp_parser);
    m_error_column = XML_GetCurrentColumnNumber (mp_parser);
    m_error = ex.msg ();
  }

  XML_Parser mp_parser;
  XMLStructureHandler *mp_struct_handler;
  bool m_has_error;
  std::string m_error;
  int m_error_line, m_error_column;
};

void start_element_handler (void *user_data, const XML_Char *name, const XML_Char ** /*atts*/)
{
  XMLParserPrivateData *d = reinterpret_cast<XMLParserPrivateData *> (user_data);
  d->start_element (std::string (name));
}

void end_element_handler (void *user_data, const XML_Char *name)
{
  XMLParserPrivateData *d = reinterpret_cast<XMLParserPrivateData *> (user_data);
  d->end_element (std::string (name));
}

void cdata_handler (void *user_data, const XML_Char *s, int len)
{
  XMLParserPrivateData *d = reinterpret_cast<XMLParserPrivateData *> (user_data);
  d->cdata (std::string (s, 0, size_t (len)));
}


XMLParser::XMLParser ()
  : mp_data (new XMLParserPrivateData ())
{
  //  .. nothing yet ..
}

XMLParser::~XMLParser ()
{
  delete mp_data;
  mp_data = 0;
}

void
XMLParser::parse (XMLSource &source, XMLStructureHandler &struct_handler)
{
  mp_data->parse (source, struct_handler);

  //  throws an exception if there is an error
  mp_data->check_error ();
}

bool
XMLParser::is_available ()
{
  return true;
}

}

#elif defined(HAVE_QT)

#include <QFile>
#include <QIODevice>
#include <QXmlContentHandler>

namespace tl
{

// --------------------------------------------------------------------
//  A SAX handler for the Qt implementation

class SAXHandler
  : public QXmlDefaultHandler
{
public:
  SAXHandler (XMLStructureHandler *sh);

  bool characters (const QString &ch);
  bool endElement (const QString &namespaceURI, const QString &localName, const QString &qName);
  bool startElement (const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts);
  bool error (const QXmlParseException &exception);
  bool fatalError (const QXmlParseException &exception);
  bool warning (const QXmlParseException &exception);

  void setDocumentLocator (QXmlLocator *locator);

private:
  QXmlLocator *mp_locator;
  XMLStructureHandler *mp_struct_handler;
};

// --------------------------------------------------------------------------------------------------------
//  trureHandler implementation

SAXHandler::SAXHandler (XMLStructureHandler *sh)
  : QXmlDefaultHandler (), mp_locator (0), mp_struct_handler (sh)
{
  // .. nothing yet ..
}

void
SAXHandler::setDocumentLocator (QXmlLocator *locator)
{
  mp_locator = locator;
}

bool
SAXHandler::startElement (const QString &qs_uri, const QString &qs_lname, const QString &qs_qname, const QXmlAttributes & /*atts*/)
{
  std::string uri (tl::to_string (qs_uri));
  std::string lname (tl::to_string (qs_lname));
  std::string qname (tl::to_string (qs_qname));

  try {
    mp_struct_handler->start_element (uri, lname, qname);
  } catch (tl::XMLException &ex) {
    throw tl::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (tl::Exception &ex) {
    throw tl::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }

  //  successful
  return true;
}

bool
SAXHandler::endElement (const QString &qs_uri, const QString &qs_lname, const QString &qs_qname)
{
  std::string uri (tl::to_string (qs_uri));
  std::string lname (tl::to_string (qs_lname));
  std::string qname (tl::to_string (qs_qname));

  try {
    mp_struct_handler->end_element (uri, lname, qname);
  } catch (tl::XMLException &ex) {
    throw tl::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (tl::Exception &ex) {
    throw tl::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }

  //  successful
  return true;
}

bool
SAXHandler::characters (const QString &t)
{
  try {
    mp_struct_handler->characters (tl::to_string (t));
  } catch (tl::XMLException &ex) {
    throw tl::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (tl::Exception &ex) {
    throw tl::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }

  //  successful
  return true;
}

bool
SAXHandler::error (const QXmlParseException &ex)
{
  throw tl::XMLLocatedException (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
}

bool
SAXHandler::fatalError (const QXmlParseException &ex)
{
  throw tl::XMLLocatedException (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
}

bool
SAXHandler::warning (const QXmlParseException &ex)
{
  tl::XMLLocatedException lex (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
  tl::warn << lex.msg ();
  //  continue
  return true;
}

// --------------------------------------------------------------------
//  SourcePrivateData implementation

class XMLSourcePrivateData
  : public QXmlInputSource
{
public:
  XMLSourcePrivateData ()
    : QXmlInputSource ()
  {
    //  .. nothing yet ..
  }

  XMLSourcePrivateData (QIODevice *dev)
    : QXmlInputSource (dev)
  {
    //  .. nothing yet ..
  }
};

// --------------------------------------------------------------------
//  XMLSource implementation

XMLSource::XMLSource ()
  : mp_source (0)
{
  //  .. nothing yet ..
}

XMLSource::~XMLSource ()
{
  delete mp_source;
  mp_source = 0;
}

void
XMLSource::reset ()
{
  mp_source->reset ();
}

// --------------------------------------------------------------------
//  XMLStringSource implementation

XMLStringSource::XMLStringSource (const std::string &string)
{
  XMLSourcePrivateData *source = new XMLSourcePrivateData ();
  source->setData (QByteArray (string.c_str ()));
  set_source (source);
}

XMLStringSource::XMLStringSource (const char *cp)
{
  XMLSourcePrivateData *source = new XMLSourcePrivateData ();
  source->setData (QByteArray (cp));
  set_source (source);
}

XMLStringSource::XMLStringSource (const char *cp, size_t len)
{
  XMLSourcePrivateData *source = new XMLSourcePrivateData ();
  source->setData (QByteArray (cp, int (len)));
  set_source (source);
}

XMLStringSource::~XMLStringSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  StreamIODevice definition and implementation

class StreamIODevice
  : public QIODevice
{
public:
  StreamIODevice (tl::InputStream &stream)
    : mp_stream (&stream),
      mp_progress (0),
      m_has_error (false)
  {
    open (QIODevice::ReadOnly);
  }

  StreamIODevice (tl::InputStream &stream, const std::string &progress_message)
    : mp_stream (&stream),
      mp_progress (new AbsoluteProgress (progress_message, 100)),
      m_has_error (false)
  {
    mp_progress->set_format (tl::to_string (tr ("%.0f MB")));
    mp_progress->set_unit (1024 * 1024);
    open (QIODevice::ReadOnly);
  }

  StreamIODevice (const std::string &path)
    : mp_stream_holder (new tl::InputStream (path)),
      mp_progress (0),
      m_has_error (false)
  {
    mp_stream = mp_stream_holder.get ();
    open (QIODevice::ReadOnly);
  }

  StreamIODevice (const std::string &path, const std::string &progress_message)
    : mp_stream_holder (new tl::InputStream (path)),
      mp_progress (new AbsoluteProgress (progress_message, 100)),
      m_has_error (false)
  {
    mp_stream = mp_stream_holder.get ();
    mp_progress->set_format (tl::to_string (tr ("%.0f MB")));
    mp_progress->set_unit (1024 * 1024);
    open (QIODevice::ReadOnly);
  }

  ~StreamIODevice ()
  {
    if (mp_progress) {
      delete mp_progress;
      mp_progress = 0;
    }
  }

  virtual bool isSequential () const
  {
    return true;
  }

  qint64 writeData (const char *, qint64) 
  {
    tl_assert (false);
  }

  qint64 readData (char *data, qint64 n)
  {
    try {

      if (mp_progress) {
        mp_progress->set (mp_stream->pos ());
      }

      qint64 n0 = n;
      for (const char *rd = 0; n > 0 && (rd = mp_stream->get (1)) != 0; ) {
        //  NOTE: we skip CR to compensate for Windows CRLF line terminators (issue #419).
        if (*rd != '\r') {
          *data++ = *rd;
          --n;
        }
      }

      if (n0 == n) {
        return -1;
      } else {
        return n0 - n;
      }

    } catch (tl::Exception &ex) {
      setErrorString (tl::to_qstring (ex.msg ()));
      m_has_error = true;
      return -1;
    }
  }

  bool has_error () const
  {
    return m_has_error;
  }

private:
  tl::InputStream *mp_stream;
  std::unique_ptr<tl::InputStream> mp_stream_holder;
  tl::AbsoluteProgress *mp_progress;
  bool m_has_error;
};

// --------------------------------------------------------------------
//  XMLFileSource implementation

class XMLStreamSourcePrivateData
  : public XMLSourcePrivateData
{
public:
  XMLStreamSourcePrivateData (StreamIODevice *io)
    : XMLSourcePrivateData (io), mp_io (io)
  {
    //  .. nothing yet ..
  }

  virtual void fetchData ()
  {
    QXmlInputSource::fetchData ();

    //  This feature is actually missing in the original implementation: throw an exception on error
    if (mp_io->has_error ()) {
      throw tl::Exception (tl::to_string (mp_io->errorString ()));
    }
  }

private:
  std::unique_ptr<StreamIODevice> mp_io;
};

// --------------------------------------------------------------------
//  XMLFileSource implementation

XMLFileSource::XMLFileSource (const std::string &path, const std::string &progress_message)
{
  set_source (new XMLStreamSourcePrivateData (new StreamIODevice (path, progress_message)));
}

XMLFileSource::XMLFileSource (const std::string &path)
{
  set_source (new XMLStreamSourcePrivateData (new StreamIODevice (path)));
}

XMLFileSource::~XMLFileSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLStreamSource implementation

XMLStreamSource::XMLStreamSource (tl::InputStream &s, const std::string &progress_message)
{
  set_source (new XMLStreamSourcePrivateData (new StreamIODevice (s, progress_message)));
}

XMLStreamSource::XMLStreamSource (tl::InputStream &s)
{
  set_source (new XMLStreamSourcePrivateData (new StreamIODevice (s)));
}

XMLStreamSource::~XMLStreamSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLParser implementation

class XMLParserPrivateData
  : public QXmlSimpleReader
{
public:
  XMLParserPrivateData () : QXmlSimpleReader () { }
};

XMLParser::XMLParser ()
  : mp_data (new XMLParserPrivateData ())
{
  //  .. nothing yet ..
}

XMLParser::~XMLParser ()
{
  delete mp_data;
  mp_data = 0;
}

void 
XMLParser::parse (XMLSource &source, XMLStructureHandler &struct_handler)
{
  SAXHandler handler (&struct_handler);

  mp_data->setContentHandler (&handler);
  mp_data->setErrorHandler (&handler);

  mp_data->parse (source.source (), false /*=not incremental*/);
}

bool
XMLParser::is_available ()
{
  return true;
}

}

#else

namespace tl
{

// --------------------------------------------------------------------
//  XMLSource implementation

XMLSource::XMLSource ()
  : mp_source (0)
{
  //  .. nothing yet ..
}

XMLSource::~XMLSource ()
{
  //  .. nothing yet ..
}

void
XMLSource::reset ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLStringSource implementation

XMLStringSource::XMLStringSource (const std::string &)
{
  tl_assert (false);
}

XMLStringSource::XMLStringSource (const char *)
{
  tl_assert (false);
}

XMLStringSource::XMLStringSource (const char *, size_t)
{
  tl_assert (false);
}

XMLStringSource::~XMLStringSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLFileSource implementation

XMLFileSource::XMLFileSource (const std::string &, const std::string &)
{
  tl_assert (false);
}

XMLFileSource::XMLFileSource (const std::string &)
{
  tl_assert (false);
}

XMLFileSource::~XMLFileSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLStreamSource implementation

XMLStreamSource::XMLStreamSource (tl::InputStream &, const std::string &)
{
  tl_assert (false);
}

XMLStreamSource::XMLStreamSource (tl::InputStream &)
{
  tl_assert (false);
}

XMLStreamSource::~XMLStreamSource ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------
//  XMLParser implementation

XMLParser::XMLParser ()
  : mp_data (0)
{
  //  .. nothing yet ..
}

XMLParser::~XMLParser ()
{
  //  .. nothing yet ..
}

void
XMLParser::parse (XMLSource &, XMLStructureHandler &)
{
  tl_assert (false);
}

bool
XMLParser::is_available ()
{
  return false;
}

}

#endif

namespace tl {

// -----------------------------------------------------------------
//  The C++ structure definition interface (for use cases see tlXMLParser.ut)

//  XMLElementProxy implementation

XMLElementProxy::XMLElementProxy (const XMLElementProxy &d)
  : mp_ptr (d.mp_ptr->clone ())
{
  //  .. nothing yet ..
}

XMLElementProxy::XMLElementProxy (const XMLElementBase &d)
  : mp_ptr (d.clone ())
{
  //  .. nothing yet ..
}

XMLElementProxy::XMLElementProxy (XMLElementBase *d)
  : mp_ptr (d)
{
  //  .. nothing yet ..
}

XMLElementProxy::~XMLElementProxy ()
{
  delete mp_ptr;
  mp_ptr = 0;
}

//  XMLElementBase implementation

void 
XMLElementBase::write_indent (tl::OutputStream &os, int indent)
{
  for (int i = 0; i < indent; ++i) {
    os << " ";
  }
}

void 
XMLElementBase::write_string (tl::OutputStream &os, const std::string &s)
{
  for (const char *cp = s.c_str (); *cp; ++cp) {
    unsigned char c = (unsigned char) *cp;
    if (c == '&') {
      os << "&amp;";
    } else if (c == '<') {
      os << "&lt;";
    } else if (c == '>') {
      os << "&gt;";
    } else if (c == '\r') {
      //  ignore CR characters (#13)
    } else if (c == '\t' || c == '\n') {
      os << c;
    } else if (c < ' ') {
      os << "&#" << int (c) << ";";
    } else {
      os << c;
    }
  }
}

// --------------------------------------------------------------------------------------------------------
//  trureHandler implementation

XMLStructureHandler::XMLStructureHandler (const XMLElementBase *root, XMLReaderState *reader_state)
  : mp_root (root), mp_state (reader_state)
{ 
  // .. nothing yet ..
}

void
XMLStructureHandler::start_element (const std::string &uri, const std::string &lname, const std::string &qname)
{
  const XMLElementBase *new_element = 0;
  const XMLElementBase *parent = 0;

  if (m_stack.size () == 0) {
    if (! mp_root->check_name (uri, lname, qname)) {
      throw tl::XMLException (tl::to_string (tr ("Root element must be ")) + mp_root->name ());
    }
    new_element = mp_root;
  } else {
    parent = m_stack.back ();
    if (parent) {
      for (XMLElementBase::iterator c = parent->begin (); c != parent->end (); ++c) {
        if ((*c)->check_name (uri, lname, qname)) {
          new_element = (*c).get ();
          break;
        }
      }
    }
  }

  if (new_element) {
    new_element->create (parent, *mp_state, uri, lname, qname);
  }

  m_stack.push_back (new_element);
}

void
XMLStructureHandler::end_element (const std::string &uri, const std::string &lname, const std::string &qname)
{
  if (m_stack.empty ()) {
    return;
  }

  const XMLElementBase *element = m_stack.back ();
  m_stack.pop_back ();

  if (! element) {
    //  inside unknown element
  } else if (m_stack.size () == 0) {
    element->finish (0, *mp_state, uri, lname, qname);
  } else {
    element->finish (m_stack.back (), *mp_state, uri, lname, qname);
  }
}

void
XMLStructureHandler::characters (const std::string &t)
{
  if (! m_stack.empty () && m_stack.back ()) {
    m_stack.back ()->cdata (t, *mp_state);
  }
}

// --------------------------------------------------------------------
//  XMLReaderState implementation

XMLReaderState::XMLReaderState ()
{
  //  .. nothing yet ..
}

XMLReaderState::~XMLReaderState ()
{
  for (std::vector <XMLReaderProxyBase *>::const_iterator o = m_objects.begin (); o != m_objects.end (); ++o) {
    (*o)->release ();
    delete *o;
  }
  m_objects.clear ();
}

// --------------------------------------------------------------------
//  XMLWriterState implementation

XMLWriterState::XMLWriterState ()
{
  //  .. nothing yet ..
}

}

