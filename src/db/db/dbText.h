
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


#ifndef HDR_dbText
#define HDR_dbText

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbMemStatistics.h"
#include "dbPoint.h"
#include "dbBox.h"
#include "dbTrans.h"
#include "dbShapeRepository.h"
#include "dbObjectTag.h"
#include "dbHersheyFont.h"
#include "tlString.h"

#include <string>
#include <string.h>

namespace db {

template <class Coord> class generic_repository;
class ArrayRepository;
class StringRepository;

/**
 *  @brief A text reference
 *  
 *  Text references are used in texts to refer to text strings through
 *  a proxy. Text references can change their strings without changing
 *  the text object's ordering. 
 *  The main use is to provide late text binding as required for the OASIS
 *  reader in some cases.
 *  String references are reference counted and remove themselves. 
 */
class DB_PUBLIC StringRef
{
public:
  /**
   *  @brief Increment the reference counter
   */
  void add_ref () 
  {
     ++m_ref_count;
  }
 
  /**
   *  @brief Decrement the reference counter and remove the object when it reaches 0
   */
  void remove_ref ()
  {
     --m_ref_count;
     if (m_ref_count == 0) {
       delete this;
     }
  }

  /**
   *  @brief Assignment of a std::string object
   */
  StringRef &operator= (const std::string &s)
  {
    m_value = s;
    return *this;
  }

  /**
   *  @brief Get the actual string
   */
  const std::string &value () const
  {
    return m_value;
  }

  /**
   *  @brief Get the actual string (non-const)
   */
  std::string &value ()
  {
    return m_value;
  }

  /**
   *  @brief Access to the repository the strings are in
   */
  const StringRepository *rep () const
  {
    return mp_rep;
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, m_value, true, (void *) this);
  }

private:
  friend class StringRepository;
  StringRepository *mp_rep;
  std::string m_value;
  size_t m_ref_count;

  /**
   *  @brief Hidden constructor attaching the reference to a repository
   */
  StringRef (StringRepository *rep)
    : mp_rep (rep), m_ref_count (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Hidden destructor
   */
  ~StringRef ();

  StringRef (const StringRef &d);
  StringRef operator= (const StringRef &d);
};

/**
 *  @brief Collect memory usage
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const StringRef &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A string repository class 
 *
 *  A string repository holds StringRef objects.
 *  It acts as a factory for StringRef objects and allows one to rename strings.
 */
class DB_PUBLIC StringRepository
{
public:
  typedef std::set<StringRef *> string_refs_type;
  typedef string_refs_type::const_iterator iterator;

  /**
   *  @brief Constructor
   */
  StringRepository ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Destructor
   */
  ~StringRepository ()
  {
    std::set<StringRef *> st;
    m_string_refs.swap (st);
    for (std::set<StringRef *>::const_iterator s = st.begin (); s != st.end (); ++s) {
      delete *s;
    }
  }

  /** 
   *  @brief Create a string reference object.
   * 
   *  String references are intended for keeping "static" strings and 
   *  referring to them by text objects. String references are unique - 
   *  even if the strings are the same, they are sematically different. 
   *  The text objects will compare pointers rather than the content of
   *  the string references. The string reference content can be changed 
   *  therefore. The main use case for these objects is the
   *  OASIS reader, where forward references of text strings requires a
   *  late binding of the text.
   *  A string reference's text can be set by using the change_string_ref
   *  method.
   */
  const StringRef *create_string_ref ()
  {
    StringRef *ref = new StringRef (this);
    m_string_refs.insert (ref);
    return ref;
  }

  /**
   *  @brief Change the string associated with a StringRef
   */
  void change_string_ref (const StringRef *ref, const std::string &s)
  {
    *(const_cast<StringRef *> (ref)) = s;
  }

  /**
   *  @brief For debugging purposes: get the number of entries
   */
  size_t size () const
  {
    return m_string_refs.size ();
  }

  /**
   *  @brief Iterates over the string refs (begin)
   */
  iterator begin () const
  {
    return m_string_refs.begin ();
  }

  /**
   *  @brief Iterates over the string refs (end)
   */
  iterator end () const
  {
    return m_string_refs.end ();
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, &m_string_refs, true, (void *) this);
    for (std::set<StringRef *>::const_iterator r = m_string_refs.begin (); r != m_string_refs.end (); ++r) {
      db::mem_stat (stat, purpose, cat, **r, true, parent);
    }
  }

private:
  friend class StringRef;

  std::set<StringRef *> m_string_refs;

  void unregister_ref (StringRef *ref)
  {
    if (! m_string_refs.empty ()) {
      m_string_refs.erase (ref);
    }
  }
};

/**
 *  @brief Collect memory statistics
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const StringRepository &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A text object
 *
 *  A text object has a point (location), a text, a text transformation,
 *  a text size and a font id. Text size and font id are provided to be
 *  be able to render the text correctly.
 */

template <class C>
class DB_PUBLIC text
{
public:
  typedef C coord_type;
  typedef db::coord_traits<C> coord_traits;
  typedef db::point<C> point_type;
  typedef db::vector<C> vector_type;
  typedef db::box<C> box_type;
  typedef db::simple_trans<C> trans_type;
  typedef db::object_tag< text<C> > tag;
  
  /**
   *  @brief The standard constructor without a text
   *
   *  The standard constructor is taking a transformation (with point),
   *  a text size value (which can be zero) and a font id which can 0 
   *  also.
   * 
   *  @param t The transformation of the text
   *  @param h The size
   *  @param f The font id
   */
  text (const trans_type &t, coord_type h = 0, Font f = NoFont, HAlign halign = NoHAlign, VAlign valign = NoVAlign)
    : mp_ptr (0), m_trans (t), m_size (h), m_font (f), m_halign (halign), m_valign (valign)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The standard constructor from a StringRef object
   *
   *  The standard constructor is taking a transformation (with point),
   *  a text size value (which can be zero) and a font id which can 0 
   *  also.
   *  The StringRef object must be owned by some other entity with a 
   *  lifetime that covers the text object's lifetime.
   * 
   *  @param s The StringRef object
   *  @param t The transformation of the text
   *  @param h The size
   *  @param f The font id
   */
  text (const StringRef *sref, const trans_type &t, coord_type h = 0, Font f = NoFont, HAlign halign = NoHAlign, VAlign valign = NoVAlign)
    : m_trans (t), m_size (h), m_font (f), m_halign (halign), m_valign (valign)
  {
    const_cast <StringRef *> (sref)->add_ref ();
    mp_ptr = (char *)((size_t)sref | 1);
  }

  /**
   *  @brief The standard constructor from a const char *
   *
   *  The standard constructor is taking a transformation (with point),
   *  a text size value (which can be zero) and a font id which can 0 
   *  also.
   * 
   *  @param s The text 
   *  @param t The transformation of the text
   *  @param h The size
   *  @param f The font id
   */
  text (const char *s, const trans_type &t, coord_type h = 0, Font f = NoFont, HAlign halign = NoHAlign, VAlign valign = NoVAlign)
    : m_trans (t), m_size (h), m_font (f), m_halign (halign), m_valign (valign)
  {
    set_string_internal (s);
  }

  /**
   *  @brief The standard constructor 
   *
   *  The standard constructor is taking a transformation (with point),
   *  a text size value (which can be zero) and a font id which can 0 
   *  also.
   * 
   *  @param s The text 
   *  @param t The transformation of the text
   *  @param h The size
   *  @param f The font id
   */
  text (const std::string &s, const trans_type &t, coord_type h = 0, Font f = NoFont, HAlign halign = NoHAlign, VAlign valign = NoVAlign)
    : m_trans (t), m_size (h), m_font (f), m_halign (halign), m_valign (valign)
  {
    set_string_internal (s.c_str ());
  }

  /** 
   *  @brief Default constructor
   *
   *  Creates an empty text object at (0,0) with empty text.
   */
  text ()
    : mp_ptr (0), m_trans (), m_size (0), m_font (NoFont), m_halign (NoHAlign), m_valign (NoVAlign)
  {
    // .. nothing yet ..
  }

  /** 
   *  @brief Copy constructor
   */
  text (const text &d)
    : mp_ptr (0), m_trans (), m_size (0), m_font (NoFont), m_halign (NoHAlign), m_valign (NoVAlign)
  {
    operator= (d);
  }

  /**
   *  @brief The copy constructor from a text with a different coordinate type
   */
  template <class D>
  explicit text (const text<D> &d)
    : mp_ptr (0), m_trans (), m_size (0), m_font (NoFont), m_halign (NoHAlign), m_valign (NoVAlign)
  {
    operator= (d);
  }

  /**
   *  @brief The destructor
   */
  ~text()
  {
    cleanup ();
  }

  /**
   *  @brief Assignment from a text
   */
  text &operator= (const text &d)
  {
    if (&d != this) {

      m_trans = trans_type (d.m_trans);
      m_size = d.m_size;
      m_font = d.m_font;
      m_halign = d.m_halign;
      m_valign = d.m_valign;

      cleanup ();

      size_t p = (size_t) d.mp_ptr;
      if (p & 1) {
        reinterpret_cast <StringRef *> (p - 1)->add_ref ();
        mp_ptr = d.mp_ptr;
      } else if (d.mp_ptr) {
        set_string_internal (d.mp_ptr);
      }

    }

    return *this;
  }

  /**
   *  @brief Assignment from a text with a different coordinate type
   */
  template <class D>
  text &operator= (const text<D> &d)
  {
    m_trans = trans_type (d.m_trans);
    m_size = coord_traits::rounded (d.m_size);
    m_font = d.m_font;
    m_halign = d.m_halign;
    m_valign = d.m_valign;

    cleanup ();

    size_t p = (size_t) d.mp_ptr;
    if (p & 1) {
      reinterpret_cast <StringRef *> (p - 1)->add_ref ();
      mp_ptr = d.mp_ptr;
    } else if (d.mp_ptr) {
      set_string_internal (d.mp_ptr);
    }

    return *this;
  }

  /**
   *  @brief The (dummy) translation operator
   */
  void translate (const text<C> &d, db::generic_repository<C> &, db::ArrayRepository &)
  {
    //  don't use StringRef's on translate - since those live in the source layout, we must not copy them
    m_trans = d.m_trans;
    m_size = d.m_size;
    m_font = d.m_font;
    m_halign = d.m_halign;
    m_valign = d.m_valign;

    string (d.string ());
  }

  /**
   *  @brief The (dummy) translation operator with transformation
   */
  template <class T>
  void translate (const text<C> &d, const T &t, db::generic_repository<C> &r, db::ArrayRepository &a)
  {
    translate (d, r, a);
    transform (t);
  }

  /**
   *  @brief Resolve any string reference if there is one
   *
   *  Calling this method basically disconnects the text from the string repository and
   *  should be used if a text object is about to be transferred to another layout.
   */
  void resolve_ref ()
  {
    if ((size_t (mp_ptr) & 1) != 0) {
      string (string ());
    } 
  }

  /**
   *  @brief Ordering operator
   */
  bool operator< (const text<C> &b) const
  {
    //  Compare transformation
    if (m_trans != b.m_trans) {
      return m_trans < b.m_trans;
    }
    return text_less (b);
  }

  /**
   *  @brief Equality test
   */
  bool operator== (const text<C> &b) const
  {
    //  Compare transformation
    if (m_trans != b.m_trans) {
      return false;
    }
    return text_equal (b);
  }

  /** 
   *  @brief Inequality test
   */
  bool operator!= (const text<C> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief Fuzzy ordering operator
   */
  bool less (const text<C> &b) const
  {
    //  Compare transformation
    if (m_trans.not_equal (b.m_trans)) {
      return m_trans.less (b.m_trans);
    }
    return text_less (b);
  }

  /**
   *  @brief Fuzzy equality test
   */
  bool equal (const text<C> &b) const
  {
    //  Compare transformation
    if (m_trans.not_equal (b.m_trans)) {
      return false;
    }
    return text_equal (b);
  }

  /**
   *  @brief Fuzzy inequality test
   */
  bool not_equal (const text<C> &b) const
  {
    return !equal (b);
  }

  /**
   *  @brief The text string write accessor
   */
  void string (const std::string &s)
  { 
    cleanup ();
    set_string_internal (s);
  }
 
  /**
   *  @brief The text string accessor
   */
  const char *string () const
  {
    size_t p = (size_t) mp_ptr;
    if (p & 1) {
      return reinterpret_cast<StringRef *> (p - 1)->value ().c_str ();
    } else if (mp_ptr) {
      return mp_ptr;
    } else {
      return "";
    }
  }

  /**
   *  @brief Gets the StringRef object is there is one
   *
   *  If the string is a plain text kept internally, this method returns 0.
   */
  const StringRef *string_ref () const
  {
    size_t p = (size_t) mp_ptr;
    if (p & 1) {
      return reinterpret_cast<const StringRef *> (p - 1);
    } else {
      return 0;
    }
  }

  /**
   *  @brief The transformation write accessor
   */
  void trans (const trans_type &t) 
  { 
    m_trans = t;
  }
   
  /**
   *  @brief The transformation accessor
   */
  const trans_type &trans () const 
  { 
    return m_trans;
  }
   
  /**
   *  @brief The size write accessor
   */
  void size (coord_type s) 
  { 
    m_size = s;
  }
   
  /**
   *  @brief The size accessor
   */
  coord_type size () const 
  { 
    return m_size;
  }
   
  /**
   *  @brief The font id write accessor
   */
  void font (Font f)
  { 
    m_font = f;
  }
   
  /**
   *  @brief The font id accessor
   */
  Font font () const 
  { 
    return m_font;
  }
   
  /**
   *  @brief The horizontal alignment flags write accessor
   */
  void halign (HAlign a)
  { 
    m_halign = a;
  }
   
  /**
   *  @brief The horizontal alignment flags 
   */
  HAlign halign () const 
  { 
    return m_halign;
  }
   
  /**
   *  @brief The vertical alignment flags write accessor
   */
  void valign (VAlign a)
  { 
    m_valign = a;
  }
   
  /**
   *  @brief The vertical alignment flags 
   */
  VAlign valign () const 
  { 
    return m_valign;
  }
   
  /**
   *  @brief Transform the text.
   *
   *  Transforms the text with the given transformation.
   *  Modifies the text with the transformed text.
   *  The transformation does not transform text size and alignment flags.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed text.
   */
  template <class Tr>
  text<C> &transform (const Tr &t)
  {
    typedef typename Tr::target_coord_type target_coord_type;
    fixpoint_trans<coord_type> fp (t);
    m_trans = simple_trans<target_coord_type> ((fp * m_trans.fp_trans ()).rot (), t (point_type () + m_trans.disp ()) - point<target_coord_type> ());
    m_size = t.ctrans (m_size);
    return *this;
  }

  /**
   *  @brief Transform the text.
   *
   *  Transforms the text with the given transformation.
   *  Does not modify the text but returns the transformed text.
   *  The transformation does not transform text size and alignment flags.
   *  
   *  @param t The transformation to apply.
   *
   *  @return The transformed text.
   */
  template <class Tr>
  text<typename Tr::target_coord_type> transformed (const Tr &t) const
  {
    typedef typename Tr::target_coord_type target_coord_type;
    fixpoint_trans<coord_type> fp (t);
    size_t p = (size_t) mp_ptr;
    if (p & 1) {
      return text<target_coord_type> (reinterpret_cast<StringRef *> (p - 1), simple_trans<target_coord_type> ((fp * m_trans.fp_trans ()).rot (), t (point_type () + m_trans.disp ()) - point<target_coord_type> ()), t.ctrans (m_size), m_font, m_halign, m_valign);
    } else if (mp_ptr) {
      return text<target_coord_type> (mp_ptr, simple_trans<target_coord_type> ((fp * m_trans.fp_trans ()).rot (), t (point_type () + m_trans.disp ()) - point<target_coord_type> ()), t.ctrans (m_size), m_font, m_halign, m_valign);
    } else {
      return text<target_coord_type> (simple_trans<target_coord_type> ((fp * m_trans.fp_trans ()).rot (), t (point_type () + m_trans.disp ()) - point<target_coord_type> ()), t.ctrans (m_size), m_font, m_halign, m_valign);
    }
  }

  /**
   *  @brief Return the moved text
   *
   *  @param p The distance to move the text.
   * 
   *  @return The moved text.
   */
  text moved (const vector_type &p) const
  { 
    text d (*this);
    d.move (p);
    return d;
  }
   
  /**
   *  @brief Move by a distance
   * 
   *  @param p The distance to move the path.
   */
  void move (const vector_type &p)
  { 
    m_trans = trans_type (p) * m_trans;
  }
   
  /**
   *  @brief Return the bounding box
   *
   *  The bounding box is a box consisting of a single point
   */
  box_type box () const
  {
    return box_type (point_type () + m_trans.disp (), point_type () + m_trans.disp ());
  }

  /**
   *  @brief String conversion
   */
  std::string to_string (double dbu = 0.0) const;

  /**
   *  @brief Reduce the text
   *
   *  Reduction of a text normalizes the text by extracting
   *  a suitable transformation and placing the text in a unique
   *  way.
   *
   *  @return The transformation that must be applied to render the original text
   */
  void reduce (simple_trans<coord_type> &tr)
  {
    tr = m_trans;
    m_trans = trans_type ();
  }

  /**
   *  @brief Reduce the text
   *
   *  Reduction of a text normalizes the text by extracting
   *  a suitable transformation and placing the text in a unique
   *  way.
   *
   *  @return The transformation that must be applied to render the original text
   */
  void reduce (disp_trans<coord_type> &tr)
  {
    tr = disp_trans<coord_type> (m_trans.disp ());
    m_trans = trans_type (m_trans.rot ());
  }

  /**
   *  @brief Reduce the text for unit transformation references
   *
   *  Does not do any reduction since no transformation can be provided.
   *
   *  @return A unit transformation
   */
  void reduce (unit_trans<C> &)
  {
    //  .. nothing ..
  }

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (! no_self) {
      stat->add (typeid (text<C>), (void *) this, sizeof (text<C>), sizeof (text<C>), parent, purpose, cat);
    }
    size_t p = (size_t) mp_ptr;
    if (! (p & 1) && mp_ptr != 0) {
      stat->add (typeid (char *), (void *) mp_ptr, strlen (mp_ptr) + 1, strlen (mp_ptr) + 1, (void *) this, purpose, cat);
    }
  }

private:
  template <class D> friend class text;

  char *mp_ptr;
  trans_type m_trans;
  coord_type m_size;
  Font m_font : 26;
  HAlign m_halign : 3;
  VAlign m_valign : 3;

  void cleanup ()
  {
    if (mp_ptr != 0) {
      if (((size_t) mp_ptr & 1) == 0) {
        delete [] mp_ptr;
      } else {
        reinterpret_cast<StringRef *>((size_t) mp_ptr - 1)->remove_ref ();
      }
    }

    mp_ptr = 0;
  }

  void set_string_internal (const std::string &s)
  {
    mp_ptr = new char[s.size() + 1];
    strncpy (mp_ptr, s.c_str (), s.size () + 1);
  }

  bool text_less (const text<C> &b) const
  {
    //  Compare strings or StringRef's by pointer (that is
    //  the intention of StringRef's: if the text changes, the sort
    //  order must not!)
    if (((size_t) mp_ptr & 1) == 0 || ((size_t) b.mp_ptr & 1) == 0) {
      int c = strcmp (string (), b.string ());
      if (c != 0) {
        return c < 0;
      }
    } else {
      if (mp_ptr != b.mp_ptr) {
        //  if references are present, use their pointers rather than the strings
        //  if they belong to the same collection
        const StringRef *r1 = reinterpret_cast<const StringRef *> (mp_ptr - 1);
        const StringRef *r2 = reinterpret_cast<const StringRef *> (b.mp_ptr - 1);
        if (r1->rep () != r2->rep ()) {
          int c = strcmp (r1->value ().c_str (), r2->value ().c_str ());
          if (c != 0) {
            return c < 0;
          }
        } else {
          return mp_ptr < b.mp_ptr;
        }
      }
    }

#if 1
    //  Compare size and presentation flags - without that, the text repository does not work properly.
    if (m_size != b.m_size) {
      return m_size < b.m_size;
    }
    if (m_font != b.m_font) {
      return m_font < b.m_font;
    }
    if (m_halign != b.m_halign) {
      return m_halign < b.m_halign;
    }
    if (m_valign != b.m_valign) {
      return m_valign < b.m_valign;
    }
#endif

    return false;
  }

  bool text_equal (const text<C> &b) const
  {
    //  Compare strings or StringRef's by pointer (that is
    //  the intention of StringRef's: if the text changes, the sort
    //  order must not!)
    if (((size_t) mp_ptr & 1) == 0 || ((size_t) b.mp_ptr & 1) == 0) {
      int c = strcmp (string (), b.string ());
      if (c != 0) {
        return false;
      }
    } else {
      if (mp_ptr != b.mp_ptr) {
        //  if references are present, use their pointers rather than the strings
        //  if they belong to the same collection
        const StringRef *r1 = reinterpret_cast<const StringRef *> (mp_ptr - 1);
        const StringRef *r2 = reinterpret_cast<const StringRef *> (b.mp_ptr - 1);
        if (r1->rep () != r2->rep ()) {
          int c = strcmp (r1->value ().c_str (), r2->value ().c_str ());
          if (c != 0) {
            return false;
          }
        } else {
          return false;
        }
      }
    }

#if 1
    //  Compare size and presentation flags - without that, the text repository does not work properly.
    if (m_size != b.m_size) {
      return false;
    }
    return m_font == b.m_font && m_halign == b.m_halign && m_valign == b.m_valign;
#else
    //  Don't compare size, font and alignment
    return true;
#endif
  }
};

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the text with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param s The text to transform
 *  @return t * s
 */
template <class C, class Tr>
inline text<typename Tr::target_coord_type> 
operator* (const Tr &t, const text<C> &s)
{
  return s.transformed (t);
}

/**
 *  @brief Output stream insertion operator
 */
template <class C>
inline std::ostream &
operator<< (std::ostream &os, const text<C> &s)
{
  return (os << s.to_string ());
}

/**
 *  @brief The standard text typedef
 */
typedef text<db::Coord> Text;

/**
 *  @brief The double coordinate text typedef
 */
typedef text<db::DCoord> DText;

/** 
 *  @brief A text reference
 *
 *  A text reference is basically a proxy to a text and
 *  is used to implement text references with a repository.
 */

template <class Text, class Trans>
struct text_ref
  : public shape_ref<Text, Trans>
{
  typedef typename Text::coord_type coord_type;
  typedef typename Text::box_type box_type;
  typedef typename Text::point_type point_type;
  typedef Trans trans_type;
  typedef Text text_type;
  typedef db::generic_repository<coord_type> repository_type;
  typedef db::object_tag< text_ref<Text, Trans> > tag;

  /**
   *  @brief The default constructor.
   *  
   *  The default constructor creates a invalid text reference
   */
  text_ref ()
    : shape_ref<Text, Trans> ()
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor creating a reference from an actual text
   */
  text_ref (const text_type &p, repository_type &rep)
    : shape_ref<Text, Trans> (p, rep)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The constructor creating a reference from an text pointer and transformation
   *
   *  The text pointer passed is assumed to reside in a proper repository.
   */
  text_ref (const text_type *p, const trans_type &t)
    : shape_ref<Text, Trans> (p, t)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The translation constructor.
   *  
   *  This constructor allows one to copy a text reference from one
   *  repository to another
   */
  text_ref (const text_ref &ref, repository_type &rep)
    : shape_ref<Text, Trans> (ref, rep)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief The transformation translation constructor
   *  
   *  This constructor allows one to copy a text reference with a certain transformation
   *  to one with another transformation
   */
  template <class TransIn>
  text_ref (const text_ref<Text, TransIn> &ref)
    : shape_ref<Text, Trans> (ref.ptr (), Trans (ref.trans ()))
  {
    // .. nothing yet ..
  }

  /** 
   *  @brief Return the transformed object
   * 
   *  This version does not change the object and is const.
   */
  template <class TargetTrans>
  text_ref<Text, TargetTrans> transformed (const TargetTrans &t) const
  {
    text_ref<Text, TargetTrans> tref (*this);
    tref.transform (t);
    return tref;
  }
};

/**
 *  @brief Binary * operator (transformation)
 *
 *  Transforms the text reference with the given transformation and 
 *  returns the result.
 *
 *  @param t The transformation to apply
 *  @param p The text reference to transform
 *  @return t * p
 */
template <class Text, class Tr, class TargetTr>
inline text_ref<Text, TargetTr>
operator* (const TargetTr &t, const text_ref<Text, Tr> &p)
{
  return p.transformed (t);
}

/**
 *  @brief Binary * operator (scaling)
 *
 *  @param p The text to scale.
 *  @param s The scaling factor
 *
 *  @return The scaled text
 */
template <class C>
inline text<double>
operator* (const text<C> &t, double s)
{
  db::complex_trans<C, double> ct (s);
  return ct * t;
}

/**
 *  @brief The text reference typedef
 */
typedef text_ref<Text, Disp> TextRef;

/**
 *  @brief The text reference typedef for double coordinates
 */
typedef text_ref<DText, DDisp> DTextRef;

/**
 *  @brief The text reference (without transformation) typedef
 */
typedef text_ref<Text, UnitTrans> TextPtr;

/**
 *  @brief The text reference (without transformation) typedef for double coordinates
 */
typedef text_ref<DText, DUnitTrans> DTextPtr;

/**
 *  @brief Collect memory usage
 */
template <class X>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const text<X> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

} // namespace db

namespace tl 
{
  template<> void DB_PUBLIC extractor_impl (tl::Extractor &ex, db::Text &p);
  template<> void DB_PUBLIC extractor_impl (tl::Extractor &ex, db::DText &p);

  template<> bool DB_PUBLIC test_extractor_impl (tl::Extractor &ex, db::Text &p);
  template<> bool DB_PUBLIC test_extractor_impl (tl::Extractor &ex, db::DText &p);

} // namespace tl

#endif

