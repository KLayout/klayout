
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


#include "dbOASIS.h"
#include "dbOASISReader.h"
#include "dbOASISWriter.h"
#include "dbStream.h"

#include "tlClassRegistry.h"

namespace db
{

// ---------------------------------------------------------------
//  OASISDiagnostics implementation

OASISDiagnostics::~OASISDiagnostics ()
{
  //  .. nothing yet ..
}

// ---------------------------------------------------------------
//  RepetitionBase and RepetitionIteratorBase specializations


//  Regular repetitions

RegularRepetitionIterator::RegularRepetitionIterator (const RegularRepetition *rep, size_t i, size_t j)
  : mp_rep (rep), m_i (i), m_j (j)
{
  // .. nothing yet ..
}

RepetitionIteratorBase *
RegularRepetitionIterator::clone () const
{
  return new RegularRepetitionIterator (mp_rep, m_i, m_j);
}

void 
RegularRepetitionIterator::inc ()
{
  ++m_i;
  if (m_i == mp_rep->m_n) {
    m_i = 0;
    ++m_j;
  }
}

db::Vector
RegularRepetitionIterator::get () const
{
  return db::Vector (mp_rep->m_a.x () * db::Coord (m_i) + mp_rep->m_b.x () * db::Coord (m_j),
                    mp_rep->m_a.y () * db::Coord (m_i) + mp_rep->m_b.y () * db::Coord (m_j));
}

unsigned int 
RegularRepetitionIterator::type () const
{
  return 0;
}

bool 
RegularRepetitionIterator::equals (const RepetitionIteratorBase *b) const
{
  const RegularRepetitionIterator *r = dynamic_cast <const RegularRepetitionIterator *> (b);
  return r && mp_rep == r->mp_rep && m_i == r->m_i && m_j == r->m_j;
}

bool 
RegularRepetitionIterator::at_end () const
{
  return m_j == mp_rep->m_m;
}


RegularRepetition::RegularRepetition (const db::Vector &a, const db::Vector &b, size_t n, size_t m)
  : m_a (a), m_b (b), m_n (n), m_m (m)
{
  // .. nothing yet ..
}

RepetitionBase *
RegularRepetition::clone () const
{
  return new RegularRepetition (m_a, m_b, m_n, m_m);
}

RepetitionIteratorBase *
RegularRepetition::begin () const
{
  return new RegularRepetitionIterator (this, 0, 0);
}

unsigned int 
RegularRepetition::type () const
{
  return 0;
}

bool 
RegularRepetition::equals (const RepetitionBase *b) const
{
  const RegularRepetition *r = dynamic_cast <const RegularRepetition *> (b);
  tl_assert (r != 0);
  return m_a == r->m_a && m_b == r->m_b && m_n == r->m_n && m_m == r->m_m;
}

bool 
RegularRepetition::less (const RepetitionBase *b) const
{
  const RegularRepetition *r = dynamic_cast <const RegularRepetition *> (b);
  tl_assert (r != 0);
  if (m_a != r->m_a) {
    return m_a < r->m_a;
  }
  if (m_b != r->m_b) {
    return m_b < r->m_b;
  }
  if (m_n != r->m_n) {
    return m_n < r->m_n;
  }
  return m_m < r->m_m;
}

bool 
RegularRepetition::is_regular (db::Vector &a, db::Vector &b, size_t &n, size_t &m) const
{
  a = m_a;
  b = m_b;
  n = m_n;
  m = m_m;
  return true;
}

const std::vector<db::Vector> *
RegularRepetition::is_iterated () const
{
  return 0;
}

//  Irregular repetitions

IrregularRepetitionIterator::IrregularRepetitionIterator (const IrregularRepetition *rep, size_t i)
  : mp_rep (rep), m_i (i)
{
  // .. nothing yet ..
}

RepetitionIteratorBase *
IrregularRepetitionIterator::clone () const
{
  return new IrregularRepetitionIterator (mp_rep, m_i);
}

void 
IrregularRepetitionIterator::inc ()
{
  ++m_i;
}

db::Vector
IrregularRepetitionIterator::get () const
{
  if (m_i == 0) {
    return db::Vector (0, 0);
  } else {
    return mp_rep->m_points [m_i - 1];
  }
}

unsigned int 
IrregularRepetitionIterator::type () const
{
  return 1;
}

bool 
IrregularRepetitionIterator::equals (const RepetitionIteratorBase *b) const
{
  const IrregularRepetitionIterator *r = dynamic_cast <const IrregularRepetitionIterator *> (b);
  return r && mp_rep == r->mp_rep && m_i == r->m_i;
}

bool 
IrregularRepetitionIterator::at_end () const
{
  return m_i == mp_rep->m_points.size () + 1;
}


IrregularRepetition::IrregularRepetition ()
  : m_points ()
{
  // .. nothing yet ..
}

RepetitionBase *
IrregularRepetition::clone () const
{
  IrregularRepetition *r =  new IrregularRepetition ();
  r->m_points = m_points;
  return r;
}

RepetitionIteratorBase *
IrregularRepetition::begin () const
{
  return new IrregularRepetitionIterator (this, 0);
}

unsigned int 
IrregularRepetition::type () const
{
  return 1;
}

bool 
IrregularRepetition::equals (const RepetitionBase *b) const
{
  const IrregularRepetition *r = dynamic_cast <const IrregularRepetition *> (b);
  tl_assert (r != 0);
  return m_points == r->m_points;
}

bool 
IrregularRepetition::less (const RepetitionBase *b) const
{
  const IrregularRepetition *r = dynamic_cast <const IrregularRepetition *> (b);
  tl_assert (r != 0);
  return m_points < r->m_points;
}

bool 
IrregularRepetition::is_regular (db::Vector & /*a*/, db::Vector & /*b*/, size_t & /*n*/, size_t & /*m*/) const
{
  return false;
}

const std::vector<db::Vector> *
IrregularRepetition::is_iterated () const
{
  return &m_points;
}


//  Repetition iterator

RepetitionIterator::RepetitionIterator (RepetitionIteratorBase *base)
  : mp_base (base)
{
  //  .. nothing yet ..
}

RepetitionIterator::~RepetitionIterator ()
{
  delete mp_base;
  mp_base = 0;
}

RepetitionIterator::RepetitionIterator (const RepetitionIterator &d)
{
  mp_base = d.mp_base->clone ();
}

RepetitionIterator &
RepetitionIterator::operator= (const RepetitionIterator &d)
{
  if (this != &d) {
    delete mp_base;
    mp_base = d.mp_base->clone ();
  }
  return *this;
}

bool 
RepetitionIterator::operator== (const RepetitionIterator &d) const
{
  if (mp_base->type () != d.mp_base->type ()) {
    return false;
  }
  return mp_base->equals (d.mp_base);
}

bool 
RepetitionIterator::at_end () const
{
  return mp_base->at_end ();
}

RepetitionIterator &
RepetitionIterator::operator++ ()
{
  mp_base->inc ();
  return *this;
}

db::Vector
RepetitionIterator::operator* () const
{
  return mp_base->get ();
}


//  Repetition 

Repetition::Repetition (RepetitionBase *base)
  : mp_base (base)
{
  // .. nothing yet ..
}

Repetition::~Repetition ()
{
  set_base (0);
}

Repetition::Repetition (const Repetition &d)
{
  if (d.mp_base) {
    mp_base = d.mp_base->clone ();
  } else {
    mp_base = 0;
  }
}

Repetition &
Repetition::operator= (const Repetition &d)
{
  if (this != &d) {
    set_base (d.mp_base ? d.mp_base->clone () : 0);
  }
  return *this;
}

Repetition &
Repetition::operator= (RepetitionBase *base)
{
  set_base (base);
  return *this;
}

bool 
Repetition::operator== (const Repetition &d) const
{
  if (mp_base == 0 && d.mp_base == 0) { 
    return true;
  }
  if (! (mp_base != 0 && d.mp_base != 0)) {
    return false;
  }
  if (mp_base->type () != d.mp_base->type ()) {
    return false;
  }
  return mp_base->equals (d.mp_base);
}

bool 
Repetition::operator< (const Repetition &d) const
{
  if (mp_base == 0 || d.mp_base == 0) { 
    return (mp_base == 0) < (d.mp_base == 0);
  }
  if (mp_base->type () != d.mp_base->type ()) {
    return mp_base->type () < d.mp_base->type ();
  }
  return mp_base->less (d.mp_base);
}

void
Repetition::set_base (RepetitionBase *base)
{
  if (mp_base) {
    delete mp_base;
  }
  mp_base = base;
}

size_t
Repetition::size () const
{
  return mp_base ? mp_base->size () : 1;
}

bool 
Repetition::is_regular (db::Vector &a, db::Vector &b, size_t &n, size_t &m) const
{
  return mp_base && mp_base->is_regular (a, b, n, m);
}

const std::vector<db::Vector> *
Repetition::is_iterated () const
{
  return mp_base != 0 ? mp_base->is_iterated () : 0;
}

RepetitionIterator 
Repetition::begin () const
{
  tl_assert (mp_base != 0);
  return RepetitionIterator (mp_base->begin ());
}

// ---------------------------------------------------------------
//  OASIS format declaration

class OASISFormatDeclaration
  : public db::StreamFormatDeclaration
{
public:
  OASISFormatDeclaration ()
  {
    //  .. nothing yet ..
  }

  virtual std::string format_name () const { return "OASIS"; }
  virtual std::string format_desc () const { return "OASIS"; }
  virtual std::string format_title () const { return "OASIS"; }
  virtual std::string file_format () const { return "OASIS files (*.oas *.OAS *.oas.gz *.OAS.gz)"; }

  virtual bool detect (tl::InputStream &stream) const 
  {
    const char *hdr = stream.get (4);
    return (hdr && hdr[0] == 0x25 && hdr[1] == 0x53 && hdr[2] == 0x45 && hdr[3] == 0x4d);
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new db::OASISReader (s);
  }

  virtual WriterBase *create_writer () const
  {
    return new db::OASISWriter ();
  }

  virtual bool can_read () const
  {
    return true;
  }

  virtual bool can_write () const
  {
    return true;
  }

  virtual tl::XMLElementBase *xml_writer_options_element () const
  {
    return new db::WriterOptionsXMLElement<db::OASISWriterOptions> ("oasis",
      tl::make_member (&db::OASISWriterOptions::compression_level, "compression-level") +
      tl::make_member (&db::OASISWriterOptions::write_cblocks, "write-cblocks") +
      tl::make_member (&db::OASISWriterOptions::strict_mode, "strict-mode") +
      tl::make_member (&db::OASISWriterOptions::write_std_properties, "write-std-properties") +
      tl::make_member (&db::OASISWriterOptions::subst_char, "subst-char") +
      tl::make_member (&db::OASISWriterOptions::permissive, "permissive")
    );
  }
};

static tl::RegisteredClass<db::StreamFormatDeclaration> reader_decl (new OASISFormatDeclaration (), 10, "OASIS");

//  provide a symbol to force linking against
int force_link_OASIS = 0;

}


