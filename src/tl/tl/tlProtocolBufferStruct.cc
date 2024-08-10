
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


namespace tl
{

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
PBParser::parse (tl::ProtocolBufferReader &reader, const PBElementBase *root, PBReaderState *reader_state)
{
  mp_state = reader_state;
  parse_element (root, reader);
}

void
PBParser::parse_element (const PBElementBase *parent, tl::ProtocolBufferReader &reader)
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
    }

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
