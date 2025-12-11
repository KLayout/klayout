
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#ifndef HDR_tlXMLReader
#define HDR_tlXMLReader

#include "tlCommon.h"

#include <list>
#include <vector>

#include "tlAssert.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlStream.h"

class QIODevice;

namespace tl
{

/**
 *  NOTE: This XML parser package also supports a ProtocolBuffer flavor.
 *  This allows binding the same scheme to efficient binary PB format.
 */

class ProtocolBufferReaderBase;
class ProtocolBufferWriterBase;

/**
 *  @brief A basic XML parser error exception class
 */

class TL_PUBLIC XMLException : public tl::Exception
{
public:
  XMLException (const char *msg)
    : Exception (tl::to_string (tr ("XML parser error: %s")).c_str ()),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

  XMLException (const std::string &msg)
    : Exception (fmt (-1, -1).c_str (), msg.c_str ()),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Raw (unprefixed) message of the XML parser
   */
  const std::string &
  raw_msg () const
  {
    return m_msg;
  }

protected:
  XMLException (const std::string &msg, int line, int column)
    : Exception (fmt (line, column).c_str (), msg.c_str (), line, column),
      m_msg (msg)
  {
    //  .. nothing yet ..
  }

private:
  std::string m_msg;

  static std::string fmt (int line, int /*column*/)
  {
    if (line < 0) {
      return tl::to_string (tr ("XML parser error: %s")).c_str ();
    } else {
      return tl::to_string (tr ("XML parser error: %s in line %d, column %d")).c_str ();
    }
  }
};

/**
 *  @brief A XML parser error exception class that additionally provides line and column information
 */

class TL_PUBLIC XMLLocatedException : public XMLException
{
public:
  XMLLocatedException (const std::string &msg, int line, int column)
    : XMLException (msg, line, column),
      m_line (line), m_column (column)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Line number information of the exception
   */
  int line () const
  {
    return m_line;
  }

  /**
   *  @brief Column number information of the exception
   */
  int column () const
  {
    return m_column;
  }

private:
  int m_line;
  int m_column;
};

//  The opaque source type
class XMLSourcePrivateData;

/**
 *  @brief A generic XML text source class
 *
 *  This class is the base class providing input for
 *  the Qt XML parser and basically maps to a QXmlInputSource object
 *  for compatibility with the "libparsifal" branch.
 */

class TL_PUBLIC XMLSource
{
public:
  XMLSource ();
  ~XMLSource ();

  XMLSourcePrivateData *source ()
  {
    return mp_source;
  }

  void reset ();

protected:
  void set_source (XMLSourcePrivateData *source)
  {
    mp_source = source;
  }

private:
  XMLSourcePrivateData *mp_source;
};

/**
 *  @brief A specialization of XMLSource to receive a string
 */

class TL_PUBLIC XMLStringSource : public XMLSource
{
public:
  XMLStringSource (const std::string &string);
  XMLStringSource (const char *cp);
  XMLStringSource (const char *cp, size_t len);
  ~XMLStringSource ();

private:
  std::string m_copy;
};

/**
 *  @brief A specialization of XMLSource to receive from a file
 */

class TL_PUBLIC XMLFileSource : public XMLSource
{
public:
  XMLFileSource (const std::string &path);
  XMLFileSource (const std::string &path, const std::string &progress_message);
  ~XMLFileSource ();
};

/**
 *  @brief A generic stream source class
 *
 *  This class implements a XML parser source from a tl::InputStream
 */

class TL_PUBLIC XMLStreamSource : public XMLSource
{
public:
  XMLStreamSource (tl::InputStream &stream);
  XMLStreamSource (tl::InputStream &stream, const std::string &progress_message);
  ~XMLStreamSource ();
};

} // namespace tl

#endif

