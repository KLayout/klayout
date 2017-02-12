
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


#include "tlXMLParser.h"
#include "tlString.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "tlProgress.h"

#include <QFile>
#include <QIODevice>

#include <cstring>

namespace tl
{

// --------------------------------------------------------------------
//  XMLStringSource implementation

XMLStringSource::XMLStringSource (const std::string &string)
{
  mp_source = new QXmlInputSource ();
  mp_source->setData (QByteArray (string.c_str ()));
}

XMLStringSource::~XMLStringSource ()
{
  delete mp_source;
  mp_source = 0;
}

// --------------------------------------------------------------------
//  StreamIODevice definition and implementation

class StreamIODevice
  : public QIODevice
{
public:
  StreamIODevice (tl::InputStream &stream)
    : m_stream (stream),
      mp_progress (0)
  {
    open (QIODevice::ReadOnly);
  }

  StreamIODevice (tl::InputStream &stream, const std::string &progress_message)
    : m_stream (stream),
      mp_progress (new AbsoluteProgress (progress_message, 100))
  {
    mp_progress->set_format (tl::to_string (QObject::tr ("%.0f MB")));
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
        mp_progress->set (m_stream.pos ());
      }

      qint64 n0 = n;
      for (const char *rd = 0; n > 0 && (rd = m_stream.get (1)) != 0; --n) {
        *data++ = *rd;
      }

      return n0 - n;

    } catch (tl::Exception &) {
      //  TODO: is there another way of reporting errors? This reports an "unexpected EOF" when the "Cancel" button is pressed.
      //  However, throwing a simple tl::Exception does not pass through the Qt library.
      return 0;
    }
  }

private:
  tl::InputStream &m_stream;
  tl::AbsoluteProgress *mp_progress;
};

// --------------------------------------------------------------------
//  XMLFileSource implementation

XMLFileSource::XMLFileSource (const std::string &path, const std::string &progress_message)
  : mp_source (0), mp_io (0), m_stream (path)
{
  mp_io = new StreamIODevice (m_stream, progress_message);
  mp_source = new QXmlInputSource (mp_io);
}

XMLFileSource::XMLFileSource (const std::string &path)
  : mp_source (0), mp_io (0), m_stream (path)
{
  mp_io = new StreamIODevice (m_stream);
  mp_source = new QXmlInputSource (mp_io);
}

XMLFileSource::~XMLFileSource ()
{
  delete mp_source;
  mp_source = 0;
  delete mp_io;
  mp_io = 0;
}

// --------------------------------------------------------------------
//  XMLStreamSource implementation

XMLStreamSource::XMLStreamSource (tl::InputStream &s, const std::string &progress_message)
{
  mp_io = new StreamIODevice (s, progress_message);
  mp_source = new QXmlInputSource (mp_io);
}

XMLStreamSource::XMLStreamSource (tl::InputStream &s)
{
  mp_io = new StreamIODevice (s);
  mp_source = new QXmlInputSource (mp_io);
}

XMLStreamSource::~XMLStreamSource ()
{
  delete mp_source;
  mp_source = 0;
  delete mp_io;
  mp_io = 0;
}

// --------------------------------------------------------------------
//  XMLParser implementation

XMLParser::XMLParser ()
{
  mp_reader = new QXmlSimpleReader ();
}

XMLParser::~XMLParser ()
{
  delete mp_reader;
  mp_reader = 0;
}

void 
XMLParser::parse (XMLSource &source, QXmlDefaultHandler &handler) 
{
  mp_reader->setContentHandler (&handler);
  mp_reader->setErrorHandler (&handler);

  mp_reader->parse (source.source (), false /*=not incremental*/);
}

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

//  XMLStructureHandler implementation

XMLStructureHandler::XMLStructureHandler (const XMLElementBase *root, XMLReaderState *reader_state) 
  : QXmlDefaultHandler (), mp_root (root), mp_locator (0), mp_state (reader_state)
{ 
  // .. nothing yet ..
}

void
XMLStructureHandler::setDocumentLocator (QXmlLocator *locator)
{
  mp_locator = locator;
}

bool 
XMLStructureHandler::startElement (const QString &qs_uri, const QString &qs_lname, const QString &qs_qname, const QXmlAttributes & /*atts*/)
{
  const XMLElementBase *new_element = 0;
  const XMLElementBase *parent = 0;

  std::string uri (tl::to_string (qs_uri));
  std::string lname (tl::to_string (qs_lname));
  std::string qname (tl::to_string (qs_qname));

  try {

    if (m_stack.size () == 0) {
      if (! mp_root->check_name (uri, lname, qname)) {
        throw tl::XMLException (tl::to_string (QObject::tr ("Root element must be ")) + mp_root->name ());
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

  } catch (tl::XMLException &ex) {
    throw tl::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (tl::Exception &ex) {
    throw tl::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }
  m_stack.push_back (new_element);

  //  successful
  return true;
}

bool 
XMLStructureHandler::endElement (const QString &qs_uri, const QString &qs_lname, const QString &qs_qname)
{
  const XMLElementBase *element = m_stack.back ();
  m_stack.pop_back ();

  std::string uri (tl::to_string (qs_uri));
  std::string lname (tl::to_string (qs_lname));
  std::string qname (tl::to_string (qs_qname));

  try {
    if (! element) {
      //  inside unknown element
    } else if (m_stack.size () == 0) {
      element->finish (0, *mp_state, uri, lname, qname);
    } else {
      element->finish (m_stack.back (), *mp_state, uri, lname, qname);
    }
  } catch (tl::XMLException &ex) {
    throw tl::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (tl::Exception &ex) {
    throw tl::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }

  //  successful
  return true;
}

bool 
XMLStructureHandler::characters (const QString &t)
{
  try {
    if (m_stack.back ()) {
      m_stack.back ()->cdata (tl::to_string (t), *mp_state);
    }
  } catch (tl::XMLException &ex) {
    throw tl::XMLLocatedException (ex.raw_msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  } catch (tl::Exception &ex) {
    throw tl::XMLLocatedException (ex.msg (), mp_locator->lineNumber (), mp_locator->columnNumber ());
  }

  //  successful
  return true;
}

bool  
XMLStructureHandler::error (const QXmlParseException &ex)
{
  throw tl::XMLLocatedException (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
}

bool  
XMLStructureHandler::fatalError (const QXmlParseException &ex)
{
  throw tl::XMLLocatedException (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
}

bool  
XMLStructureHandler::warning (const QXmlParseException &ex)
{
  tl::XMLLocatedException lex (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
  tl::warn << lex.msg ();
  //  continue
  return true;
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

