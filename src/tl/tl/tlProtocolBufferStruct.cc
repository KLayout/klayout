
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

#include "tlProtocolBufferStruct.h"
#include <cctype>

namespace tl
{

static size_t s_oid = 0;

// --------------------------------------------------------------------
//  PBElementList implementation

PBElementList::PBElementList ()
  : m_oid (++s_oid)
{
  //  .. nothing yet ..
}

PBElementList::PBElementList (const PBElementBase &e)
  : m_oid (++s_oid)
{
  m_elements.push_back (PBElementProxy (e));
}

PBElementList::PBElementList (PBElementBase *e)
  : m_oid (++s_oid)
{
  if (e) {
    m_elements.push_back (PBElementProxy (e));
  }
}

PBElementList::PBElementList (const std::string &name, const PBElementList &d)
  : m_elements (d.m_elements), m_oid (d.m_oid), m_name (name)
{
  //  .. nothing yet ..
}

PBElementList::PBElementList (const PBElementList &d)
  : m_elements (d.m_elements), m_oid (d.m_oid), m_name (d.m_name)
{
  //  .. nothing yet ..
}

PBElementList::PBElementList (const PBElementList &d, const PBElementBase &e)
  : m_elements (d.m_elements), m_oid (++s_oid), m_name (d.m_name)
{
  m_elements.push_back (PBElementProxy (e));
}

PBElementList::PBElementList (const PBElementList &d, PBElementBase *e)
  : m_elements (d.m_elements), m_oid (++s_oid), m_name (d.m_name)
{
  if (e) {
    m_elements.push_back (PBElementProxy (e));
  }
}

void PBElementList::append (const PBElementBase &e)
{
  m_elements.push_back (PBElementProxy (e));
}

void PBElementList::append (PBElementBase *e)
{
  if (e) {
    m_elements.push_back (PBElementProxy (e));
  }
}

PBElementList::iterator PBElementList::begin () const
{
  return m_elements.begin ();
}

PBElementList::iterator PBElementList::end () const
{
  return m_elements.end ();
}

PBElementList PBElementList::empty ()
{
  return PBElementList ();
}

// --------------------------------------------------------------------
//  PBElementBase implementation

PBElementBase::PBElementBase (const std::string &name, int tag, const PBElementList &children)
  : m_name (name), m_tag (tag), mp_children (new PBElementList (children)), m_owns_child_list (true)
{
  // .. nothing yet ..
}

PBElementBase::PBElementBase (const std::string &name, int tag, const PBElementList *children)
  : m_name (name), m_tag (tag), mp_children (children), m_owns_child_list (false)
{
  // .. nothing yet ..
}

PBElementBase::PBElementBase (const PBElementBase &d)
  : m_name (d.m_name), m_tag (d.m_tag), m_owns_child_list (d.m_owns_child_list)
{
  if (m_owns_child_list) {
    mp_children = new PBElementList (*d.mp_children);
  } else {
    mp_children = d.mp_children;
  }
}

PBElementBase::~PBElementBase ()
{
  if (m_owns_child_list) {
    delete const_cast <PBElementList *> (mp_children);
    mp_children = 0;
  }
}

PBElementBase::Cardinality
PBElementBase::cardinality () const
{
  return Zero;
}

PBElementBase::iterator
PBElementBase::begin () const
{
  return mp_children->begin ();
}

PBElementBase::iterator
PBElementBase::end () const
{
  return mp_children->end ();
}

std::string
PBElementBase::name4code () const
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
PBElementBase::create_def (std::map<size_t, std::pair<const PBElementBase *, std::string> > &messages) const
{
  std::string res;

  auto m = messages.find (oid ());
  if (m != messages.end ()) {

    res += "message " + m->second.second + " {\n";

    for (auto i = begin (); i != end (); ++i) {
      const PBElementBase *e = i->get ();
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
PBElementBase::collect_messages (std::map<size_t, std::pair<const PBElementBase *, std::string> > &messages) const
{
  for (auto i = begin (); i != end (); ++i) {
    i->get ()->collect_messages (messages);
  }
}

std::string
PBElementBase::make_message_name () const
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
PBParser::parse (tl::ProtocolBufferReaderBase &reader, const PBElementBase *root, PBReaderState *reader_state)
{
  mp_state = reader_state;
  parse_element (root, reader);
}

void
PBParser::parse_element (const PBElementBase *parent, tl::ProtocolBufferReaderBase &reader)
{
  while (! reader.at_end ()) {

    int tag = reader.read_tag ();

    const PBElementBase *new_element = 0;
    if (parent) {
      for (PBElementBase::iterator c = parent->begin (); c != parent->end (); ++c) {
        if ((*c)->tag () == tag) {
          new_element = (*c).get ();
          break;
        }
      }
    }

    if (! new_element) {
      reader.skip ();
    } else {
      new_element->create (parent, *mp_state);
      new_element->parse (this, reader);
      new_element->finish (parent, *mp_state);
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
//  PBElementProxy implementation

PBElementProxy::PBElementProxy (const PBElementProxy &d)
  : mp_ptr (d.mp_ptr->clone ())
{
  //  .. nothing yet ..
}

PBElementProxy::PBElementProxy (const PBElementBase &d)
  : mp_ptr (d.clone ())
{
  //  .. nothing yet ..
}

PBElementProxy::PBElementProxy (PBElementBase *d)
  : mp_ptr (d)
{
  //  .. nothing yet ..
}

PBElementProxy::~PBElementProxy ()
{
  delete mp_ptr;
  mp_ptr = 0;
}

// --------------------------------------------------------------------
//  PBReaderState implementation

PBReaderState::PBReaderState ()
{
  //  .. nothing yet ..
}

PBReaderState::~PBReaderState ()
{
  for (std::vector <PBReaderProxyBase *>::const_iterator o = m_objects.begin (); o != m_objects.end (); ++o) {
    (*o)->release ();
    delete *o;
  }
  m_objects.clear ();
}

// --------------------------------------------------------------------
//  PBWriterState implementation

PBWriterState::PBWriterState ()
{
  //  .. nothing yet ..
}

}
