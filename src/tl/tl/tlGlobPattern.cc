
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "tlGlobPattern.h"
#include "tlString.h"

#include <cstring>

namespace tl
{

//  TODO: take from tlString.h
inline uint32_t utf32_from_utf8 (const char *&cp, const char *cpe = 0)
{
  uint32_t c32 = (unsigned char) *cp++;
  if (c32 >= 0xf0 && ((cpe && cp + 2 < cpe) || (! cpe && cp [0] && cp [1] && cp [2]))) {
    c32 = ((c32 & 0x7) << 18) | ((uint32_t (cp [0]) & 0x3f) << 12) | ((uint32_t (cp [1]) & 0x3f) << 6) | (uint32_t (cp [2]) & 0x3f);
    cp += 3;
  } else if (c32 >= 0xe0 && ((cpe && cp + 1 < cpe) || (! cpe && cp [0] && cp [1]))) {
    c32 = ((c32 & 0xf) << 12) | ((uint32_t (cp [0]) & 0x3f) << 6) | (uint32_t (cp [1]) & 0x3f);
    cp += 2;
  } else if (c32 >= 0xc0 && ((cpe && cp < cpe) || (! cpe && cp [0]))) {
    c32 = ((c32 & 0x1f) << 6) | (uint32_t (*cp) & 0x3f);
    ++cp;
  }

  return c32;
}

class GlobPatternOp
{
public:
  GlobPatternOp () : m_next_owned (false), mp_next (0) { }

  virtual ~GlobPatternOp ()
  {
    if (m_next_owned) {
      delete mp_next;
    }
    mp_next = 0;
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternOp *op = new GlobPatternOp ();
    if (next ()) {
      op->set_next (next ()->clone ());
    }
    return op;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    size_t n = e ? e->size () : 0;
    if (mp_next && mp_next->match (s, e)) {
      return true;
    } else if (! mp_next && ! *s) {
      return true;
    } else if (e) {
      e->erase (e->begin () + n, e->end ());
      return false;
    } else {
      return false;
    }
  }

  void set_next (GlobPatternOp *next)
  {
    m_next_owned = true;
    mp_next = next;
  }

  GlobPatternOp *next ()
  {
    return mp_next;
  }

  const GlobPatternOp *next () const
  {
    return mp_next;
  }

  void set_tail (GlobPatternOp *op)
  {
    GlobPatternOp *n = this;
    while (n->mp_next) {
      n = n->mp_next;
    }
    n->mp_next = op;
    n->m_next_owned = false;
  }

private:
  bool m_next_owned;
  GlobPatternOp *mp_next;
};

class GlobPatternString
  : public GlobPatternOp
{
public:
  GlobPatternString (const std::string &s, bool cs)
    : GlobPatternOp (), m_s (s), m_cs (cs)
  {
    //  .. nothing yet ..
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternString *op = new GlobPatternString (m_s, m_cs);
    op->set_next (next ()->clone ());
    return op;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    if (! m_cs && strncasecmp (s, m_s.c_str (), m_s.size ()) == 0) {
      return GlobPatternOp::match (s + m_s.size (), e);
    } else if (m_cs && strncmp (s, m_s.c_str (), m_s.size ()) == 0) {
      return GlobPatternOp::match (s + m_s.size (), e);
    } else {
      return false;
    }
  }

private:
  std::string m_s;
  bool m_cs;
};

class GlobPatternPass
  : public GlobPatternOp
{
public:
  GlobPatternPass ()
    : GlobPatternOp ()
  {
    //  .. nothing yet ..
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternPass *op = new GlobPatternPass ();
    if (next ()) {
      op->set_next (next ()->clone ());
    }
    return op;
  }

  virtual bool match (const char *, std::vector<std::string> *) const
  {
    return true;
  }
};

class GlobPatternAny
  : public GlobPatternOp
{
public:
  GlobPatternAny (size_t min, size_t max)
    : GlobPatternOp (), m_min (min), m_max (max)
  {
    //  .. nothing yet ..
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternAny *op = new GlobPatternAny (m_min, m_max);
    if (next ()) {
      op->set_next (next ()->clone ());
    }
    return op;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    size_t i = 0;
    while (i <= m_max) {
      if (i >= m_min && GlobPatternOp::match (s, e)) {
        return true;
      } else if (! *s) {
        return false;
      }
      utf32_from_utf8 (s);
      ++i;
    }

    return false;
  }

private:
  size_t m_min, m_max;
};

class GlobPatternBranch;

template <class T>
class GlobPatternContinuator
  : public GlobPatternOp
{
public:
  GlobPatternContinuator (T *br)
    : mp_br (br)
  {
    //  .. nothing yet ..
  }

  virtual GlobPatternOp *clone () const { return 0; }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    return mp_br->continue_match (s, e);
  }

private:
  T *mp_br;
};

class GlobPatternBranch
  : public GlobPatternOp
{
public:
  GlobPatternBranch ()
    : GlobPatternOp (), m_cont (this)
  {
    //  .. nothing yet ..
  }

  ~GlobPatternBranch ()
  {
    for (std::vector<GlobPatternOp *>::const_iterator i = m_choices.begin (); i != m_choices.end (); ++i) {
      delete *i;
    }
    m_choices.clear ();
  }

  void add_choice (GlobPatternOp *op)
  {
    op->set_tail (&m_cont);
    m_choices.push_back (op);
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternBranch *br = new GlobPatternBranch ();
    if (next ()) {
      br->set_next (next ()->clone ());
    }
    for (std::vector<GlobPatternOp *>::const_iterator i = m_choices.begin (); i != m_choices.end (); ++i) {
      br->add_choice ((*i)->clone ());
    }
    return br;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    for (std::vector<GlobPatternOp *>::const_iterator i = m_choices.begin (); i != m_choices.end (); ++i) {
      if ((*i)->match (s, e)) {
        return true;
      }
    }
    return false;
  }

  virtual bool continue_match (const char *s, std::vector<std::string> *e) const
  {
    return GlobPatternOp::match (s, e);
  }

private:
  std::vector<GlobPatternOp *> m_choices;
  GlobPatternContinuator<GlobPatternBranch> m_cont;
};

class GlobPatternBracket
  : public GlobPatternOp
{
public:
  GlobPatternBracket ()
    : GlobPatternOp (), mp_inner (0), mp_s0 (0), m_cont (this)
  {
    //  .. nothing yet ..
  }

  ~GlobPatternBracket ()
  {
    delete mp_inner;
    mp_inner = 0;
  }

  void set_inner (GlobPatternOp *op)
  {
    delete mp_inner;
    op->set_tail (& m_cont);
    mp_inner = op;
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternBracket *br = new GlobPatternBracket ();
    if (next ()) {
      br->set_next (next ()->clone ());
    }
    if (mp_inner) {
      br->set_inner (mp_inner->clone ());
    }
    return br;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    if (mp_inner) {
      mp_s0 = s;
      bool res = mp_inner->match (s, e);
      mp_s0 = 0;
      return res;
    }
    return false;
  }

  virtual bool continue_match (const char *s, std::vector<std::string> *e) const
  {
    if (mp_s0 && e) {
      e->push_back (std::string (mp_s0, 0, s - mp_s0));
    }
    return GlobPatternOp::match (s, e);
  }

private:
  GlobPatternOp *mp_inner;
  //  NOTE: this isn't thread-safe unless GlobPattern objects live in different threads
  mutable const char *mp_s0;
  GlobPatternContinuator<GlobPatternBracket> m_cont;
};

class GlobPatternCharClass
  : public GlobPatternOp
{
public:
  GlobPatternCharClass (bool negate, bool cs)
    : m_negate (negate), m_cs (cs)
  {
    //  .. nothing yet ..
  }

  GlobPatternCharClass (const std::vector<std::pair<uint32_t, uint32_t> > &intervals, bool negate, bool cs)
    : m_negate (negate), m_cs (cs), m_intervals (intervals)
  {
    //  .. nothing yet ..
  }

  void add_interval (uint32_t c1, uint32_t c2)
  {
    m_intervals.push_back (std::make_pair (c1, c2));
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternCharClass *op = new GlobPatternCharClass (m_intervals, m_negate, m_cs);
    if (next ()) {
      op->set_next (next ()->clone ());
    }
    return op;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    uint32_t c = utf32_from_utf8 (s);

    for (std::vector<std::pair<uint32_t, uint32_t> >::const_iterator i = m_intervals.begin (); i != m_intervals.end (); ++i) {
      if (c >= i->first && c <= i->second) {
        if (m_negate) {
          return false;
        } else {
          return GlobPatternOp::match (s, e);
        }
      }
    }

    if (! m_negate) {
      return false;
    } else {
      return GlobPatternOp::match (s, e);
    }
  }

private:
  bool m_negate, m_cs;
  std::vector<std::pair<uint32_t, uint32_t> > m_intervals;
};

static
GlobPatternOp *compile (const char *&p, bool exact, bool cs, bool hm, bool for_brace);

void
compile_emit_op (GlobPatternOp *&op_head, GlobPatternOp *&op, GlobPatternOp *no)
{
  if (op) {
    op->set_next (no);
  } else {
    op_head = no;
  }
  op = no;
}

void
compile_emit_string (std::string &str, GlobPatternOp *&op_head, GlobPatternOp *&op, bool cs)
{
  if (! str.empty ()) {
    compile_emit_op (op_head, op, new GlobPatternString (str, cs));
    str.clear ();
  }
}

void
compile_emit_char_class (GlobPatternOp *&op_head, GlobPatternOp *&op, const char *&p, bool cs)
{
  bool negate = false;
  if (*p && *p == '^') {
    ++p;
    negate = true;
  }

  GlobPatternCharClass *cc = new GlobPatternCharClass (negate, cs);

  while (*p != ']' && *p) {

    uint32_t c1 = utf32_from_utf8 (p);
    if (c1 == '\\') {
      c1 = utf32_from_utf8 (p);
    }

    uint32_t c2 = c1;
    if (*p == '-') {
      ++p;
      c2 = utf32_from_utf8 (p);
      if (c2 == '\\') {
        c2 = utf32_from_utf8 (p);
      }
    }

    cc->add_interval (c1, c2);

  }

  compile_emit_op (op_head, op, cc);
}

void
compile_emit_alt (GlobPatternOp *&op_head, GlobPatternOp *&op, const char *&p, bool cs)
{
  GlobPatternBranch *alt_op = new GlobPatternBranch ();
  while (*p) {
    GlobPatternOp *alt = compile (p, false, cs, false, true);
    if (alt) {
      alt_op->add_choice (alt);
    }
    if (*p == ',') {
      ++p;
    } else if (*p == '}') {
      ++p;
      break;
    }
  }

  compile_emit_op (op_head, op, alt_op);
}

static
GlobPatternOp *compile (const char *&p, bool exact, bool cs, bool hm, bool for_brace)
{
  std::string str;
  GlobPatternOp *op = 0, *op_head = 0;

  while (*p) {

    if (exact) {

      str += *++p;

    } else if (*p == '\\') {

      ++p;
      if (*p) {
        str += *++p;
      }

    } else if (*p == '?') {

      compile_emit_string (str, op_head, op, cs);
      compile_emit_op (op_head, op, new GlobPatternAny (1, 1));

      ++p;

    } else if (*p == '*') {

      compile_emit_string (str, op_head, op, cs);
      if (p[1]) {
        compile_emit_op (op_head, op, new GlobPatternAny (0, std::numeric_limits<size_t>::max ()));
      } else {
        compile_emit_op (op_head, op, new GlobPatternPass ());
      }

      ++p;

    } else if (*p == '[') {

      compile_emit_string (str, op_head, op, cs);
      ++p;
      compile_emit_char_class (op_head, op, p, cs);

    } else if (*p == '{') {

      compile_emit_string (str, op_head, op, cs);
      ++p;
      compile_emit_alt (op_head, op, p, cs);

    } else if (for_brace && (*p == ',' || *p == '}')) {

      break;

    } else {

      str += *p++;

    }

  }

  compile_emit_string (str, op_head, op, cs);

  if (hm) {
    compile_emit_op (op_head, op, new GlobPatternPass ());
  }

  return op_head;
}

GlobPattern::GlobPattern ()
  : m_case_sensitive (true), m_exact (false), m_header_match (false)
{
  mp_op = 0;
  m_needs_compile = true;
}

GlobPattern::GlobPattern (const std::string &p)
  : m_p (p), m_case_sensitive (true), m_exact (false), m_header_match (false)
{
  mp_op = 0;
  m_needs_compile = true;
}

GlobPattern::GlobPattern (const GlobPattern &other)
  : m_case_sensitive (true), m_exact (false), m_header_match (false)
{
  mp_op = 0;
  m_needs_compile = true;

  operator= (other);
}

GlobPattern &
GlobPattern::operator= (const GlobPattern &other)
{
  if (this != &other) {

    m_case_sensitive = other.m_case_sensitive;
    m_exact = other.m_exact;
    m_header_match = other.m_header_match;

    m_needs_compile = true;

  }
  return *this;
}

void
GlobPattern::do_compile ()
{
  delete mp_op;

  const char *p = m_p.c_str ();
  mp_op = compile (p, m_exact, m_case_sensitive, m_header_match, false);

  if (! mp_op) {
    mp_op = new GlobPatternOp ();
  }

  m_needs_compile = false;
}

GlobPattern &GlobPattern::operator= (const std::string &p)
{
  if (m_p != p) {
    m_p = p;
    m_needs_compile = true;
  }

  return *this;
}

void GlobPattern::set_case_sensitive (bool f)
{
  if (f != m_case_sensitive) {
    m_case_sensitive = f;
    m_needs_compile = true;
  }
}

bool GlobPattern::case_sensitive () const
{
  return m_case_sensitive;
}

void GlobPattern::set_exact (bool f)
{
  if (f != m_exact) {
    m_exact = f;
    m_needs_compile = true;
  }
}

bool GlobPattern::exact () const
{
  return m_exact;
}

void GlobPattern::set_header_match (bool f)
{
  if (f != m_header_match) {
    m_header_match = f;
    m_needs_compile = true;
  }
}

bool GlobPattern::header_match () const
{
  return m_header_match;
}

GlobPatternOp *GlobPattern::op () const
{
  if (m_needs_compile) {
    GlobPattern *non_const_this = const_cast<GlobPattern *> (this);
    non_const_this->do_compile ();
  }

  return mp_op;
}

bool GlobPattern::match (const char *s) const
{
  return op ()->match (s, 0);
}

bool GlobPattern::match (const char *s, std::vector<std::string> &e) const
{
  if (! e.empty ()) {
    e.clear ();
  }

  return op ()->match (s, &e);
}

bool GlobPattern::match (const std::string &s) const
{
  return op ()->match (s.c_str (), 0);
}

bool GlobPattern::match (const std::string &s, std::vector<std::string> &e) const
{
  if (! e.empty ()) {
    e.clear ();
  }

  return op ()->match (s.c_str (), &e);
}

}
