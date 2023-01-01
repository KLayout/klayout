
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


#include "tlGlobPattern.h"
#include "tlString.h"

#include <cstring>

namespace tl
{

class GlobPatternOpBase
{
public:
  GlobPatternOpBase () { }
  virtual ~GlobPatternOpBase () { }

  virtual GlobPatternOpBase *clone () const = 0;
  virtual bool match (const char *s, std::vector<std::string> *e) const = 0;

  virtual GlobPatternOpBase *next () { return 0; }
  virtual const GlobPatternOpBase *next () const { return 0; }
  virtual void set_next (GlobPatternOpBase * /*next*/, bool /*owned*/) { tl_assert (false); }

private:
  GlobPatternOpBase (const GlobPatternOpBase &);
  GlobPatternOpBase &operator= (const GlobPatternOpBase &);
};

class GlobPatternOp
  : public GlobPatternOpBase
{
public:
  GlobPatternOp () : m_next_owned (false), mp_next (0) { }

  virtual ~GlobPatternOp ()
  {
    set_next (0, false);
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternOp *op = new GlobPatternOp ();
    init_clone (op);
    return op;
  }

  virtual bool is_const () const
  {
    return false;
  }

  virtual bool is_catchall () const
  {
    return false;
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

  virtual void set_next (GlobPatternOpBase *next, bool owned)
  {
    if (mp_next && m_next_owned) {
      delete mp_next;
    }

    m_next_owned = owned;
    mp_next = next;
  }

  GlobPatternOpBase *next ()
  {
    return mp_next;
  }

  const GlobPatternOpBase *next () const
  {
    return mp_next;
  }

  void set_tail (GlobPatternOpBase *op)
  {
    GlobPatternOpBase *n = this;
    while (n->next ()) {
      n = n->next ();
    }

    n->set_next (op, false);
  }

protected:
  void init_clone (GlobPatternOp *op) const
  {
    if (mp_next && m_next_owned) {
      op->set_next (mp_next->clone (), true);
    }
  }

private:
  bool m_next_owned;
  GlobPatternOpBase *mp_next;

  GlobPatternOp (const GlobPatternOp &);
  GlobPatternOp &operator= (const GlobPatternOp &);
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
    init_clone (op);
    return op;
  }

  virtual bool is_const () const
  {
    return next () == 0;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    if (! m_cs) {

      const char *sr = m_s.c_str ();
      while (*sr) {
        if (! *s) {
          return false;
        }
        uint32_t cr = utf32_from_utf8 (sr);
        uint32_t c = utf32_from_utf8 (s);
        if (utf32_downcase (cr) != utf32_downcase (c)) {
          return false;
        }
      }

      return GlobPatternOp::match (s, e);

    } else if (m_cs && strncmp (s, m_s.c_str (), m_s.size ()) == 0) {

      return GlobPatternOp::match (s + m_s.size (), e);

    } else {

      return false;

    }
  }

private:
  std::string m_s;
  bool m_cs;

  GlobPatternString (const GlobPatternString &);
  GlobPatternString &operator= (const GlobPatternString &);
};

class GlobPatternEmpty
  : public GlobPatternOp
{
public:
  GlobPatternEmpty ()
    : GlobPatternOp ()
  {
    //  .. nothing yet ..
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternEmpty *op = new GlobPatternEmpty ();
    init_clone (op);
    return op;
  }

  virtual bool is_const () const
  {
    return next () == 0;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    return GlobPatternOp::match (s, e);
  }

private:
  GlobPatternEmpty (const GlobPatternEmpty &);
  GlobPatternEmpty &operator= (const GlobPatternString &);
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
    init_clone (op);
    return op;
  }

  virtual bool is_catchall () const
  {
    return true;
  }

  virtual bool match (const char *, std::vector<std::string> *) const
  {
    return true;
  }

  GlobPatternPass (const GlobPatternPass &);
  GlobPatternPass &operator= (const GlobPatternPass &);
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
    init_clone (op);
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

  GlobPatternAny (const GlobPatternAny &);
  GlobPatternAny &operator= (const GlobPatternAny &);
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
    if (m_cs) {
      m_intervals.push_back (std::make_pair (c1, c2));
    } else {
      m_intervals.push_back (std::make_pair (utf32_downcase (c1), utf32_downcase (c2)));
    }
  }

  virtual GlobPatternOp *clone () const
  {
    GlobPatternCharClass *op = new GlobPatternCharClass (m_intervals, m_negate, m_cs);
    init_clone (op);
    return op;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    if (!*s) {
      return false;
    }

    uint32_t c = utf32_from_utf8 (s);
    if (! m_cs) {
      c = utf32_downcase (c);
    }

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

  GlobPatternCharClass (const GlobPatternCharClass &);
  GlobPatternCharClass &operator= (const GlobPatternCharClass &);
};

template <class T>
class GlobPatternContinuator
  : public GlobPatternOpBase
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

  GlobPatternContinuator (const GlobPatternContinuator &);
  GlobPatternContinuator &operator= (const GlobPatternContinuator &);
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
    for (std::vector<GlobPatternOp *>::const_iterator i = m_choices.begin (); i != m_choices.end (); ++i) {
      br->add_choice ((*i)->clone ());
    }
    init_clone (br);
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

  GlobPatternBranch (const GlobPatternBranch &);
  GlobPatternBranch &operator= (const GlobPatternBranch &);
};

class GlobPatternBracket
  : public GlobPatternOp
{
public:
  GlobPatternBracket ()
    : GlobPatternOp (), mp_inner (0), mp_s0 (0), m_index (0), m_cont (this)
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
    if (mp_inner) {
      br->set_inner (mp_inner->clone ());
    }
    init_clone (br);
    return br;
  }

  virtual bool match (const char *s, std::vector<std::string> *e) const
  {
    if (mp_inner) {

      if (e) {
        mp_s0 = s;
        m_index = e->size ();
        e->push_back (std::string ());
      } else {
        mp_s0 = 0;
      }

      bool res = mp_inner->match (s, e);

      mp_s0 = 0;
      return res;

    }
    return false;
  }

  virtual bool continue_match (const char *s, std::vector<std::string> *e) const
  {
    if (mp_s0 && e) {
      (*e) [m_index] = std::string (mp_s0, 0, s - mp_s0);
    }
    return GlobPatternOp::match (s, e);
  }

private:
  GlobPatternOp *mp_inner;
  //  NOTE: this isn't thread-safe unless GlobPattern objects live in different threads
  mutable const char *mp_s0;
  mutable size_t m_index;
  GlobPatternContinuator<GlobPatternBracket> m_cont;

  GlobPatternBracket (const GlobPatternBracket &);
  GlobPatternBracket &operator= (const GlobPatternBracket &);
};

static
GlobPatternOp *compile (const char *&p, bool exact, bool cs, bool hm, bool for_brace);

void
compile_emit_op (GlobPatternOp *&op_head, GlobPatternOp *&op, GlobPatternOp *no)
{
  if (op) {
    op->set_next (no, true);
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

  while (*p) {

    if (*p == ']') {
      ++p;
      break;
    }

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
    } else {
      alt_op->add_choice (new GlobPatternEmpty ());
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

void
compile_emit_bracket (GlobPatternOp *&op_head, GlobPatternOp *&op, const char *&p, bool cs)
{
  GlobPatternBracket *br_op = new GlobPatternBracket ();
  GlobPatternOp *inner = compile (p, false, cs, false, true);
  if (inner) {
    br_op->set_inner (inner);
  }
  if (*p == ')') {
    ++p;
  }

  compile_emit_op (op_head, op, br_op);
}

static
GlobPatternOp *compile (const char *&p, bool exact, bool cs, bool hm, bool for_brace)
{
  std::string str;
  GlobPatternOp *op = 0, *op_head = 0;

  while (*p) {

    if (exact) {

      str += *p++;

    } else if (*p == '\\') {

      ++p;
      if (*p) {
        str += *p++;
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

    } else if (*p == '(') {

      compile_emit_string (str, op_head, op, cs);
      ++p;
      compile_emit_bracket (op_head, op, p, cs);

    } else if (for_brace && (*p == ',' || *p == '}' || *p == ')')) {

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

GlobPattern::~GlobPattern ()
{
  delete mp_op;
  mp_op = 0;
}

GlobPattern &
GlobPattern::operator= (const GlobPattern &other)
{
  if (this != &other) {

    m_case_sensitive = other.m_case_sensitive;
    m_exact = other.m_exact;
    m_header_match = other.m_header_match;
    m_p = other.m_p;
    mp_op = other.mp_op ? other.mp_op->clone () : 0;
    m_needs_compile = other.m_needs_compile;

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

void
GlobPattern::needs_compile ()
{
  if (! m_needs_compile) {

    m_needs_compile = true;

    delete mp_op;
    mp_op = 0;

  }
}

GlobPattern &GlobPattern::operator= (const std::string &p)
{
  if (m_p != p) {
    m_p = p;
    needs_compile ();
  }

  return *this;
}

void GlobPattern::set_case_sensitive (bool f)
{
  if (f != m_case_sensitive) {
    m_case_sensitive = f;
    needs_compile ();
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
    needs_compile ();
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
    needs_compile ();
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

bool GlobPattern::is_catchall () const
{
  return op ()->is_catchall ();
}

bool GlobPattern::is_const () const
{
  return op ()->is_const ();
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
