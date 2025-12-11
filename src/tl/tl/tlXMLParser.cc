
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


#include "tlXMLParser.h"
#include "tlString.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "tlProgress.h"
#include "tlProtocolBuffer.h"

#include <cstring>
#include <memory>

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

// -----------------------------------------------------------------
//  XMLElementList implementation

static size_t s_oid = 0;

XMLElementList::XMLElementList ()
  : m_oid (++s_oid)
{
  //  .. nothing yet ..
}

XMLElementList::XMLElementList (const XMLElementBase &e)
  : m_oid (++s_oid)
{
  m_elements.push_back (XMLElementProxy (e));
}

XMLElementList::XMLElementList (XMLElementBase *e)
  : m_oid (++s_oid)
{
  if (e) {
    m_elements.push_back (XMLElementProxy (e));
  }
}

XMLElementList::XMLElementList (const std::string &name, const XMLElementList &d)
  : m_elements (d.m_elements), m_oid (d.m_oid), m_name (name)
{
  //  .. nothing yet ..
}

XMLElementList::XMLElementList (const XMLElementList &d)
  : m_elements (d.m_elements), m_oid (d.m_oid), m_name (d.m_name)
{
  //  .. nothing yet ..
}

XMLElementList::XMLElementList (const XMLElementList &d, const XMLElementBase &e)
  : m_elements (d.m_elements), m_oid (++s_oid), m_name (d.m_name)
{
  m_elements.push_back (XMLElementProxy (e));
}

XMLElementList::XMLElementList (const XMLElementList &d, XMLElementBase *e)
  : m_elements (d.m_elements), m_oid (++s_oid), m_name (d.m_name)
{
  if (e) {
    m_elements.push_back (XMLElementProxy (e));
  }
}

void XMLElementList::append (const XMLElementBase &e)
{
  m_elements.push_back (XMLElementProxy (e));
}

void XMLElementList::append (XMLElementBase *e)
{
  if (e) {
    m_elements.push_back (XMLElementProxy (e));
  }
}

XMLElementList::iterator XMLElementList::begin () const
{
  return m_elements.begin ();
}

XMLElementList::iterator XMLElementList::end () const
{
  return m_elements.end ();
}

XMLElementList XMLElementList::empty ()
{
  return XMLElementList ();
}

// -----------------------------------------------------------------
//  XMLElementBase implementation

static std::string parse_name (const std::string &n)
{
  auto hash = n.find ("#");
  if (hash != std::string::npos) {
    return std::string (n, 0, hash);
  } else {
    return n;
  }
}

static int parse_tag (const std::string &n)
{
  auto hash = n.find ("#");
  if (hash != std::string::npos) {
    tl::Extractor ex (n.c_str () + hash + 1);
    int tag;
    if (ex.try_read (tag)) {
      return tag;
    }
  }
  return -1;
}

XMLElementBase::XMLElementBase (const std::string &name, const XMLElementList &children)
  : m_name (parse_name (name)), m_tag (parse_tag (name)), mp_children (new XMLElementList (children)), m_owns_child_list (true)
{
  // .. nothing yet ..
}

XMLElementBase::XMLElementBase (const std::string &name, const XMLElementList *children)
  : m_name (parse_name (name)), m_tag (parse_tag (name)), mp_children (children), m_owns_child_list (false)
{
  // .. nothing yet ..
}

XMLElementBase::XMLElementBase (const XMLElementBase &d)
  : m_name (d.m_name), m_tag (d.m_tag), m_owns_child_list (d.m_owns_child_list)
{
  if (m_owns_child_list) {
    mp_children = new XMLElementList (*d.mp_children);
  } else {
    mp_children = d.mp_children;
  }
}

XMLElementBase::~XMLElementBase ()
{
  if (m_owns_child_list) {
    delete const_cast <XMLElementList *> (mp_children);
    mp_children = 0;
  }
}

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

bool
XMLElementBase::check_name (const std::string & /*uri*/, const std::string &lname, const std::string & /*qname*/) const
{
  if (m_name == "*") {
    return true;
  } else {
    return m_name == lname; // no namespace currently
  }
}

XMLElementBase::Cardinality
XMLElementBase::cardinality () const
{
  return Zero;
}

XMLElementBase::iterator
XMLElementBase::begin () const
{
  return mp_children->begin ();
}

XMLElementBase::iterator
XMLElementBase::end () const
{
  return mp_children->end ();
}

std::string
XMLElementBase::name4code () const
{
  std::string res;
  const char *n = name ().c_str ();

  if (! isalpha (*n) && *n != '_') {
    res += '_';
  }

  while (*n) {
    if (*n == '-') {
      res += '_';
    } else if (isalnum (*n) || *n == '_') {
      res += *n;
    }
    ++n;
  }

  return res;
}

std::string
XMLElementBase::create_def (std::map<size_t, std::pair<const XMLElementBase *, std::string> > &messages) const
{
  std::string res;

  auto m = messages.find (oid ());
  if (m != messages.end ()) {

    res += "message " + m->second.second + " {\n";

    for (auto i = begin (); i != end (); ++i) {
      const XMLElementBase *e = i->get ();
      Cardinality c = e->cardinality ();
      std::string entry = e->create_def_entry (messages);
      if (! entry.empty ()) {
        res += "  ";
        if (c != Zero) {
          if (c == Many) {
            res += "repeated ";
          } else {
            res += "optional ";
          }
          res += entry + "\n";
        }
      }
    }

    res += "}";

  }

  return res;
}

void
XMLElementBase::collect_messages (std::map<size_t, std::pair<const XMLElementBase *, std::string> > &messages) const
{
  for (auto i = begin (); i != end (); ++i) {
    i->get ()->collect_messages (messages);
  }
}

std::string
XMLElementBase::make_message_name () const
{
  std::string res = mp_children->name ();
  if (! res.empty ()) {
    return res;
  }

  //  Capitalize names
  std::string n4c = name4code ();

  bool upcase = true;

  const char *n = n4c.c_str ();
  while (*n) {
    if (*n == '_') {
      upcase = true;
    } else if (upcase) {
      res += toupper (*n);
      upcase = false;
    } else {
      res += *n;
    }
    ++n;
  }

  return res;
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

// --------------------------------------------------------------------
//  PBParser implementation

PBParser::PBParser ()
{
  //  .. nothing yet ..
}

PBParser::~PBParser ()
{
  //  .. nothing yet ..
}

void
PBParser::parse (tl::ProtocolBufferReaderBase &reader, const XMLElementBase *root, XMLReaderState *reader_state)
{
  mp_state = reader_state;
  parse_element (root, reader);
}

void
PBParser::parse_element (const XMLElementBase *parent, tl::ProtocolBufferReaderBase &reader)
{
  while (! reader.at_end ()) {

    int tag = reader.read_tag ();

    const XMLElementBase *new_element = 0;
    if (parent) {
      for (XMLElementBase::iterator c = parent->begin (); c != parent->end (); ++c) {
        if ((*c)->tag () == tag) {
          new_element = (*c).get ();
          break;
        }
      }
    }

    if (! new_element) {
      reader.skip ();
    } else {
      new_element->pb_create (parent, *mp_state);
      new_element->pb_parse (this, reader);
      new_element->pb_finish (parent, *mp_state);
    }

  }
}

void
PBParser::expect_header (tl::ProtocolBufferReaderBase &reader, int name_tag, const std::string &name)
{
  int tag = reader.read_tag ();
  if (tag != name_tag) {
    reader.error (tl::sprintf (tl::to_string (tr ("Expected header field with ID %d (got %d)")), name_tag, tag));
  }

  std::string n;
  reader.read (n);
  if (n != name) {
    reader.error (tl::sprintf (tl::to_string (tr ("Expected header field with string '%s' (got '%s')")), name, n));
  }
}

// --------------------------------------------------------------------
//  PBWriterState implementation

PBWriterState::PBWriterState ()
{
  //  .. nothing yet ..
}

}

