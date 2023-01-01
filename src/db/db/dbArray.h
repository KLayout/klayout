
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



#ifndef HDR_dbArray
#define HDR_dbArray

#include "dbCommon.h"

#include <algorithm> 
#include <limits> 
#include <vector> 

#include "dbTypes.h"
#include "dbMemStatistics.h"
#include "dbPoint.h"
#include "dbBox.h"
#include "dbBoxTree.h"
#include "dbTrans.h"
#include "dbShapeRepository.h"
#include "tlException.h"

namespace db
{

template <class Obj, class Trans> struct array_iterator;
template <class C, bool AllowEmpty = true> struct box_convert;

/**
 *  @brief The array iterator base class (internal)
 */

template <class Coord>
struct basic_array_iterator 
{
  typedef Coord coord_type;
  typedef db::point<coord_type> point_type;
  typedef db::vector<coord_type> vector_type;
  typedef db::disp_trans<coord_type> disp_type;
  typedef db::box<coord_type> box_type;

  virtual ~basic_array_iterator ()
  { 
    // .. nothing yet ..
  }

  virtual void inc () = 0;

  virtual bool at_end () const = 0;

  virtual long index_a () const { return -1; }
  virtual long index_b () const { return -1; }

  virtual size_t quad_id () const { return 0; }
  virtual box_type quad_box () const { return box_type::world (); }
  virtual void skip_quad () { }

  virtual disp_type get () const = 0;
  
  virtual basic_array_iterator<Coord> *clone () const = 0;
};

/**
 *  @brief A generic base class for all array incarnations
 */
struct ArrayBase
{
  ArrayBase ()
    : in_repository (false)
  {
    //  .. nothing yet ..
  }

  ArrayBase (const ArrayBase &)
    : in_repository (false)
  {
    //  .. nothing yet ..
  }

  ArrayBase &operator= (const ArrayBase &)
  {
    return *this;
  }

  virtual ~ArrayBase ()
  {
    //  .. nothing yet ..
  }

  virtual const ArrayBase *cast (const ArrayBase *other) const = 0;

  virtual ArrayBase *basic_clone () const = 0;

  virtual unsigned int type () const = 0;

  virtual bool equal (const ArrayBase *) const = 0;

  virtual bool fuzzy_equal (const ArrayBase *) const = 0;

  virtual bool less (const ArrayBase *) const = 0;

  virtual bool fuzzy_less (const ArrayBase *) const = 0;

  virtual void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const = 0;

  bool in_repository;
};

/**
 *  @brief Memory statistics for ArrayBase
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const ArrayBase &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief The array base class (internal)
 */

template <class Coord>
struct basic_array
  : public ArrayBase
{
  typedef Coord coord_type;
  typedef db::box <coord_type> box_type;
  typedef db::point <coord_type> point_type;
  typedef db::vector <coord_type> vector_type;
  typedef db::complex_trans <coord_type, coord_type> complex_trans_type;
  typedef db::simple_trans <coord_type> simple_trans_type;
  typedef db::unit_trans <coord_type> unit_trans_type;
  typedef db::disp_trans <coord_type> disp_trans_type;
  typedef db::disp_trans<coord_type> disp_type;

  basic_array ()
  {
    //  .. nothing yet ..
  }

  virtual ~basic_array ()
  {
    //  .. nothing yet ..
  }

  virtual std::pair<basic_array_iterator<Coord> *, bool> begin_touching (const box_type &b) const = 0;
  
  virtual std::pair<basic_array_iterator<Coord> *, bool> begin () const = 0;
  
  virtual std::pair<basic_array_iterator<Coord> *, bool> begin_regular (long /*a*/, long /*b*/) const { return begin (); }
  
  virtual ArrayBase *basic_clone () const 
  {
    return clone ();
  }

  virtual basic_array <Coord> *clone () const = 0;

  virtual box_type bbox (const box_type &obox) const = 0;

  virtual size_t size () const = 0;

  virtual void invert (simple_trans_type &t) = 0;

  virtual bool is_regular_array (vector_type & /*a*/, vector_type & /*b*/, unsigned long & /*amax*/, unsigned long & /*bmax*/) const
  {
    return false;
  }

  virtual bool is_iterated_array (std::vector<vector_type> * /*v*/)
  {
    return false;
  }

  virtual bool is_complex () const 
  { 
    return false; 
  }

  virtual complex_trans_type complex_trans (const simple_trans_type &s) const 
  { 
    return complex_trans_type (s); 
  }

  virtual unsigned int type () const
  {
    return 0;
  }

  virtual const ArrayBase *cast (const ArrayBase *other) const
  {
    return dynamic_cast <const basic_array<Coord> *> (other);
  }

  void transform (const unit_trans_type & /*st*/) { }

  void transform (const disp_trans_type & /*st*/) { }

  virtual void transform (const simple_trans_type & /*st*/) { }

  virtual void transform (const complex_trans_type & /*ct*/) { }
};

/**
 *  @brief A helper function to compare base class objects
 */
struct array_base_ptr_cmp_f
{
  bool operator() (const ArrayBase *p1, const ArrayBase *p2) const
  {
    if (p1->type () != p2->type ()) {
      return p1->type () < p2->type ();
    }
    return p1->less (p2);
  }
};

/**
 *  @brief The array repository
 *
 *  This repository may be used to hold the base objects 
 *  for compacter memory representation. Multiple array
 *  objects may share the same base object. Base objects 
 *  stored herein have the "in_repository" flag set and
 *  are managed by the repository.
 */

class DB_PUBLIC ArrayRepository
{
public:
  typedef std::set<ArrayBase *, array_base_ptr_cmp_f> basic_repository;
  typedef std::vector<basic_repository> repositories;

  ArrayRepository ();

  ArrayRepository (const ArrayRepository &d);

  ~ArrayRepository ();

  ArrayRepository &operator= (const ArrayRepository &d);

  template <class Coord>
  basic_array<Coord> *insert (const basic_array<Coord> &base)
  {
    repositories::iterator r;
    for (r = m_reps.begin (); r != m_reps.end (); ++r) {
      if ((*r->begin ())->cast (&base)) {
        break;
      }
    }

    if (r == m_reps.end ()) {
      m_reps.push_back (basic_repository ());
      r = m_reps.end () - 1;
    }

    basic_repository::iterator f = r->find ((ArrayBase *) &base);
    if (f != r->end ()) {
      return dynamic_cast <basic_array<Coord> *> (*f);
    } else {
      basic_array<Coord> *bb = base.clone ();
      bb->in_repository = true;
      r->insert (bb);
      return bb;
    }
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const;

private:
  repositories m_reps;

  void clear ();
};

/**
 *  @brief Collect memory statistics
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const ArrayRepository &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}


/**
 *  @brief The array iterator specialization for the regular case (internal)
 *
 *  The regular case is that the displacements are described as
 *    P=a*A+b*B (a=[amin..amax),b=[bmin..bmax)).
 */

template <class Coord>
struct regular_array_iterator
  : public basic_array_iterator <Coord>
{
  typedef typename basic_array_iterator<Coord>::coord_type coord_type;
  typedef typename basic_array_iterator<Coord>::point_type point_type;
  typedef typename basic_array_iterator<Coord>::vector_type vector_type;
  typedef typename basic_array_iterator<Coord>::box_type box_type;
  typedef typename basic_array_iterator<Coord>::disp_type disp_type;

  regular_array_iterator (const vector_type &a, const vector_type &b, unsigned long amin, unsigned long amax, unsigned long bmin, unsigned long bmax)
    : m_a (a), m_b (b),
      m_amin (amin), m_amax (amax),
      m_bmin (bmin), m_bmax (bmax),
      m_ai (amin), m_bi (bmin)
  { 
    //  if there is no way to iterate for ai, just finish:
    if (m_amin >= m_amax) {
      m_bi = m_bmax;
    }
  }

  virtual ~regular_array_iterator ()
  {
    //  .. nothing yet .. 
  }

  virtual disp_type get () const 
  {
    return disp_type (vector_type (m_ai * m_a.x () + m_bi * m_b.x (), m_ai * m_a.y () + m_bi * m_b.y ()));
  }
  
  virtual void inc () 
  {
    ++m_ai;
    if (m_ai >= m_amax) {
      m_ai = m_amin;
      ++m_bi;
    }
  }

  virtual bool at_end () const 
  {
    return m_bi >= m_bmax;
  }

  virtual basic_array_iterator<Coord> *clone () const 
  {
    return new regular_array_iterator <Coord> (*this);
  }

  virtual long index_a () const 
  { 
    return long (m_ai); 
  }

  virtual long index_b () const 
  { 
    return long (m_bi); 
  }
  
private:
  vector_type m_a, m_b;
  unsigned long m_amin, m_amax, m_bmin, m_bmax;
  unsigned long m_ai, m_bi;
};

/**
 *  @brief The array specialization for the regular case (internal)
 *
 *  The regular case is that the displacements are described as
 *    P=O+a*A+b*B (a=[amin..amax),b=[bmin..bmax)).
 */

template <class Coord>
struct regular_array 
  : public basic_array <Coord>
{
  typedef typename basic_array<Coord>::point_type point_type;
  typedef typename basic_array<Coord>::vector_type vector_type;
  typedef typename basic_array<Coord>::box_type box_type;
  typedef typename basic_array<Coord>::coord_type coord_type;
  typedef typename basic_array<Coord>::disp_type disp_type;
  typedef typename basic_array<Coord>::simple_trans_type simple_trans_type;
  typedef typename basic_array<Coord>::complex_trans_type complex_trans_type;

  regular_array (const vector_type &a, const vector_type &b, unsigned long amax, unsigned long bmax)
    : m_a (a), m_b (b), m_amax (amax), m_bmax (bmax)
  { 
    compute_det ();
  }

  virtual ~regular_array ()
  {
    //  .. nothing yet .. 
  }

  virtual std::pair<basic_array_iterator<Coord> *, bool>
  begin_touching (const box_type &b) const
  {
    if (b.empty ()) {

      return std::make_pair (new regular_array_iterator <Coord> (m_a, m_b, 0, 0, 0, 0), false);

    } else if (fabs (m_det) < 0.5) {

      return begin ();

    } else {

      std::pair <double, double> ab [4] = {
        ab_coord (b.p1 ()),
        ab_coord (point_type (b.left (), b.top ())),
        ab_coord (point_type (b.right (), b.bottom ())),
        ab_coord (b.p2 ())
      };

      //  NOTE: we need to take some care we don't overiterate in case of vanishing row
      //  or column vectors. Hence eff_amax and eff_bmax which are 1 in this case.

      unsigned long eff_amax = m_amax;
      if (m_a.equal (vector_type ())) {
        eff_amax = 1;
      }

      unsigned long eff_bmax = m_bmax;
      if (m_b.equal (vector_type ())) {
        eff_bmax = 1;
      }

      double amin = ab [0].first;
      double amax = ab [0].first;
      double bmin = ab [0].second;
      double bmax = ab [0].second;

      for (int i = 1; i < 4; ++i) {
        amin = std::min (amin, ab [i].first);
        amax = std::max (amax, ab [i].first);
        bmin = std::min (bmin, ab [i].second);
        bmax = std::max (bmax, ab [i].second);
      }

      unsigned long amini = 0;
      if (amin >= epsilon) {
        if (amin > double (std::numeric_limits <unsigned long>::max () - 1)) {
          amini = std::numeric_limits <unsigned long>::max () - 1;
        } else {
          amini = (unsigned long) (amin + 1.0 - epsilon);
        }
        if (amini > eff_amax) {
          amini = eff_amax;
        }
      }
         
      unsigned long amaxi = 0;
      if (amax >= -epsilon) {
        if (amax > double (std::numeric_limits <unsigned long>::max () - 1)) {
          amaxi = std::numeric_limits <unsigned long>::max () - 1;
        } else {
          amaxi = (unsigned long) (amax + epsilon) + 1;
        }
        if (amaxi > eff_amax) {
          amaxi = eff_amax;
        }
      }

      unsigned long bmini = 0;
      if (bmin >= epsilon) {
        if (bmin > double (std::numeric_limits <unsigned long>::max () - 1)) {
          bmini = std::numeric_limits <unsigned long>::max () - 1;
        } else {
          bmini = (unsigned long) (bmin + 1.0 - epsilon);
        }
        if (bmini > eff_bmax) {
          bmini = eff_bmax;
        }
      }
         
      unsigned long bmaxi = 0;
      if (bmax >= -epsilon) {
        if (bmax > double (std::numeric_limits <unsigned long>::max () - 1)) {
          bmaxi = std::numeric_limits <unsigned long>::max () - 1;
        } else {
          bmaxi = (unsigned long) (bmax + epsilon) + 1;
        }
        if (bmaxi > eff_bmax) {
          bmaxi = eff_bmax;
        }
      }
      
      return std::make_pair (new regular_array_iterator <Coord> (m_a, m_b, amini, amaxi, bmini, bmaxi), false);
    }
  }
  
  virtual std::pair <basic_array_iterator <Coord> *, bool>
  begin_regular (long a, long b) const
  {
    return std::make_pair (new regular_array_iterator <Coord> (m_a, m_b, (unsigned long) std::max (long (0), a), m_amax, (unsigned long) std::max (long (0), b), m_bmax), false);
  }

  virtual std::pair <basic_array_iterator <Coord> *, bool>
  begin () const
  {
    return std::make_pair (new regular_array_iterator <Coord> (m_a, m_b, 0, m_amax, 0, m_bmax), false);
  }

  virtual basic_array <Coord> *clone () const 
  {
    return new regular_array <Coord> (*this);
  }

  virtual box_type bbox (const box_type &obox) const
  {
    if (obox.empty ()) {
      return obox;
    } else {

      vector_type ma (m_a.x () * (m_amax - 1), m_a.y () * (m_amax - 1));
      vector_type mb (m_b.x () * (m_bmax - 1), m_b.y () * (m_bmax - 1));

      box_type b;

      if (m_amax > 0 && m_bmax > 0) {
        b += point_type ();
        b += point_type () + ma;
        b += point_type () + mb;
        b += point_type () + ma + mb;
      }

      return box_type (obox.p1 () + (b.p1 () - point_type ()), obox.p2 () + (b.p2 () - point_type ()));

    }
  }

  virtual size_t size () const 
  {
    return m_amax * m_bmax;
  }

  virtual void invert (simple_trans_type &t) 
  {
    t.invert ();
    db::fixpoint_trans<coord_type> f (t.rot ());
    m_a = -f (m_a);
    m_b = -f (m_b);
    compute_det ();
  }

  virtual bool equal (const ArrayBase *b) const
  {
    const regular_array<Coord> *d = static_cast<const regular_array<Coord> *> (b);
    return (m_a == d->m_a && m_b == d->m_b && m_amax == d->m_amax && m_bmax == d->m_bmax);
  }

  virtual bool fuzzy_equal (const ArrayBase *b) const
  {
    const regular_array<Coord> *d = static_cast<const regular_array<Coord> *> (b);
    return (m_a.equal (d->m_a) && m_b.equal (d->m_b) && m_amax == d->m_amax && m_bmax == d->m_bmax);
  }

  virtual bool less (const ArrayBase *b) const
  {
    const regular_array<Coord> *d = static_cast<const regular_array<Coord> *> (b);
    return m_a < d->m_a || (m_a == d->m_a && (
           m_b < d->m_b || (m_b == d->m_b && (
           m_amax < d->m_amax || (m_amax == d->m_amax && m_bmax < d->m_bmax)))));
  }

  virtual bool fuzzy_less (const ArrayBase *b) const
  {
    const regular_array<Coord> *d = static_cast<const regular_array<Coord> *> (b);
    return m_a.less (d->m_a) || (m_a.equal (d->m_a) && (
           m_b.less (d->m_b) || (m_b.equal (d->m_b) && (
           m_amax < d->m_amax || (m_amax == d->m_amax && m_bmax < d->m_bmax)))));
  }

  virtual bool is_regular_array (vector_type &a, vector_type &b, unsigned long &amax, unsigned long &bmax) const
  {
    a = m_a;
    b = m_b;
    amax = m_amax;
    bmax = m_bmax;
    return true;
  }

  virtual void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
  }

  virtual unsigned int type () const
  {
    return 1;
  }

  virtual void transform (const simple_trans_type &st) 
  { 
    m_a.transform (st.fp_trans ());
    m_b.transform (st.fp_trans ());
    compute_det ();
  }

  virtual void transform (const complex_trans_type &ct) 
  { 
    //  transform with the matrix, do not displace, since a and b are displacements already
    m_a = vector_type (ct * m_a);
    m_b = vector_type (ct * m_b);
    compute_det ();
  }

protected:
  vector_type m_a, m_b;
  unsigned long m_amax, m_bmax;
  double m_det;

  inline vector_type eff_a () const
  {
    if (m_a.equal (vector_type ())) {
      if (m_b.equal (vector_type ())) {
        return vector_type (1, 0);
      } else {
        return vector_type (m_b.y (), -m_b.x ());
      }
    } else {
      return m_a;
    }
  }

  inline vector_type eff_b () const
  {
    if (m_b.equal (vector_type ())) {
      if (m_a.equal (vector_type ())) {
        return vector_type (0, 1);
      } else {
        return vector_type (-m_a.y (), m_a.x ());
      }
    } else {
      return m_b;
    }
  }

  std::pair <double, double> ab_coord (const point_type &p) const
  {
    vector_type a = eff_a (), b = eff_b ();
    double ia = (double (p.x ()) * double (b.y ()) - double (p.y ()) * double (b.x ())) / m_det;
    double ib = (double (a.x ()) * double (p.y ()) - double (a.y ()) * double (p.x ())) / m_det;
    return std::make_pair (ia, ib);
  }

  void compute_det () 
  {
    vector_type a = eff_a (), b = eff_b ();
    m_det = double (a.x ()) * double (b.y ()) - double (a.y ()) * double (b.x ());
  }
};

/**
 *  @brief The array specialization for the regular, complex case (internal)
 *
 *  The regular case is that the displacements are described as
 *    P=O+a*A+b*B (a=[amin..amax),b=[bmin..bmax)).
 *  Each instance is magnified with the given magnification and subject to
 *  further rotation by a residual angle given by acos, which is actually the cosine
 *  of this angle.
 */

template <class Coord>
struct regular_complex_array 
  : public regular_array<Coord>
{
  typedef typename basic_array<Coord>::point_type point_type;
  typedef typename basic_array<Coord>::vector_type vector_type;
  typedef typename basic_array<Coord>::box_type box_type;
  typedef typename basic_array<Coord>::coord_type coord_type;
  typedef typename basic_array<Coord>::disp_type disp_type;
  typedef typename basic_array<Coord>::simple_trans_type simple_trans_type;
  typedef typename basic_array<Coord>::complex_trans_type complex_trans_type;

  regular_complex_array (double acos, double mag, const vector_type &a, const vector_type &b, unsigned long amax, unsigned long bmax)
    : regular_array<Coord> (a, b, amax, bmax), m_acos (acos), m_mag (mag)
  { 
    //  .. nothing yet ..
  }

  virtual basic_array <Coord> *clone () const 
  {
    return new regular_complex_array <Coord> (*this);
  }

  virtual void invert (simple_trans_type &t) 
  {
    //  recompute the array parameters such that every per-instance transformation
    //  is inverted. 
    complex_trans_type r = complex_trans_type (t, m_acos, m_mag).inverted ();
    m_mag = r.mag ();
    m_acos = r.rcos ();
    t = simple_trans_type (r);
    regular_array<Coord>::m_a = -r (regular_array<Coord>::m_a);
    regular_array<Coord>::m_b = -r (regular_array<Coord>::m_b);
    regular_array<Coord>::compute_det ();
  }

  virtual bool equal (const ArrayBase *b) const
  {
    const regular_complex_array<Coord> *d = static_cast<const regular_complex_array<Coord> *> (b);
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return false;
    }
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return false;
    }
    return regular_array<Coord>::equal (b);
  }

  virtual bool fuzzy_equal (const ArrayBase *b) const
  {
    const regular_complex_array<Coord> *d = static_cast<const regular_complex_array<Coord> *> (b);
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return false;
    }
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return false;
    }
    return regular_array<Coord>::fuzzy_equal (b);
  }

  virtual bool less (const ArrayBase *b) const
  {
    const regular_complex_array<Coord> *d = static_cast<const regular_complex_array<Coord> *> (b);
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return m_acos < d->m_acos;
    }
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return m_mag < d->m_mag;
    }
    return regular_array<Coord>::less (b);
  }

  virtual bool fuzzy_less (const ArrayBase *b) const
  {
    const regular_complex_array<Coord> *d = static_cast<const regular_complex_array<Coord> *> (b);
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return m_acos < d->m_acos;
    }
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return m_mag < d->m_mag;
    }
    return regular_array<Coord>::fuzzy_less (b);
  }

  virtual void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
  }

  virtual complex_trans_type complex_trans (const simple_trans_type &s) const
  {
    return complex_trans_type (s, m_acos, m_mag);
  }

  virtual bool is_complex () const
  {
    return true;
  }

  virtual unsigned int type () const
  {
    return 2;
  }

private:
  double m_acos, m_mag;
};

/**
 *  @brief The array iterator specialization for the iterated case (internal)
 *
 *  The iterated case is where each placement is described by a displacement vector
 */

template <class Coord>
struct iterated_array_iterator
  : public basic_array_iterator <Coord>
{
  typedef typename basic_array_iterator<Coord>::coord_type coord_type;
  typedef typename basic_array_iterator<Coord>::point_type point_type;
  typedef typename basic_array_iterator<Coord>::vector_type vector_type;
  typedef typename basic_array_iterator<Coord>::box_type box_type;
  typedef typename basic_array_iterator<Coord>::disp_type disp_type;
  typedef box_convert<vector_type> box_convert_type;
  typedef unstable_box_tree <box_type, vector_type, box_convert_type> box_tree_type;
  typedef typename box_tree_type::const_iterator box_tree_const_iterator;
  typedef typename box_tree_type::touching_iterator box_tree_touching_iterator;

  iterated_array_iterator (box_tree_const_iterator from, box_tree_const_iterator to)
    : m_normal (true)
  { 
    m_b = from;
    m_e = to;
  }

  iterated_array_iterator (box_tree_touching_iterator from)
    : m_normal (false)
  { 
    m_t = from;
  }

  virtual ~iterated_array_iterator ()
  {
    //  .. nothing yet .. 
  }

  virtual disp_type get () const 
  {
    return disp_type (m_normal ? *m_b : *m_t);
  }
  
  virtual void inc () 
  {
    if (m_normal) {
      ++m_b;
    } else {
      ++m_t;
    }
  }

  virtual bool at_end () const 
  {
    if (m_normal) {
      return m_b == m_e;
    } else {
      return m_t.at_end ();
    }
  }

  virtual basic_array_iterator<Coord> *clone () const 
  {
    return new iterated_array_iterator <Coord> (*this);
  }

  virtual size_t quad_id () const
  {
    return m_t.quad_id ();
  }

  virtual box_type quad_box () const
  {
    return m_t.quad_box ();
  }

  virtual void skip_quad ()
  {
    m_t.skip_quad ();
  }

private:
  box_tree_const_iterator m_b, m_e;
  box_tree_touching_iterator m_t;
  bool m_normal;
};

/**
 *  @brief The array specialization for the iterated case (internal)
 *
 *  The iterated case is where each placement is described by a displacement vector
 */

template <class Coord>
struct iterated_array 
  : public basic_array <Coord>
{
  typedef typename basic_array<Coord>::point_type point_type;
  typedef typename basic_array<Coord>::vector_type vector_type;
  typedef typename basic_array<Coord>::box_type box_type;
  typedef typename basic_array<Coord>::coord_type coord_type;
  typedef typename basic_array<Coord>::disp_type disp_type;
  typedef typename basic_array<Coord>::simple_trans_type simple_trans_type;
  typedef typename basic_array<Coord>::complex_trans_type complex_trans_type;
  typedef box_convert<vector_type> box_convert_type;
  typedef unstable_box_tree <box_type, vector_type, box_convert_type> box_tree_type;
  typedef typename box_tree_type::const_iterator const_iterator;
  typedef typename box_tree_type::iterator iterator;
  typedef typename box_tree_type::touching_iterator touching_iterator;

  iterated_array ()
  { 
    //  .. nothing yet .. 
  }

  template <class I>
  iterated_array (I from, I to)
  {
    assign (from, to);
  }

  virtual ~iterated_array ()
  {
    //  .. nothing yet .. 
  }

  void reserve (size_t n)
  {
    m_v.reserve (n);
  }

  void insert (const vector_type &p)
  {
    m_v.insert (p);
    m_box += point_type () + p;
  }

  template <class I>
  void assign (I from, I to)
  {
    m_v.clear ();
    m_v.insert (from, to);

    m_box = box_type ();
    for (I p = from; p != to; ++p) {
      m_box += point_type () + *p;
    }
  }

  template <class I>
  void insert (I from, I to)
  {
    m_v.insert (from, to);
    for (I p = from; p != to; ++p) {
      m_box += point_type () + *p;
    }
  }

  virtual bool is_iterated_array (std::vector<vector_type> *v)
  {
    if (v) {
      v->clear ();
      v->reserve (m_v.size ());
      for (const_iterator p = m_v.begin (); p != m_v.end (); ++p) {
        v->push_back (*p);
      }
    }
    return true;
  }

  void sort ()
  {
    m_v.sort (db::box_convert <vector_type> ());
  }

  virtual std::pair <basic_array_iterator <Coord> *, bool>
  begin_touching (const box_type &b) const
  {
    if (b.empty () || ! b.touches (m_box)) {
      return std::make_pair (new iterated_array_iterator <Coord> (m_v.end (), m_v.end ()), false);
    } else {
      box_convert_type bc;
      return std::make_pair (new iterated_array_iterator <Coord> (m_v.begin_touching (b, bc)), false);
    }
  }
  
  virtual std::pair <basic_array_iterator <Coord> *, bool>
  begin () const
  {
    return std::make_pair (new iterated_array_iterator <Coord> (m_v.begin (), m_v.end ()), false);
  }

  virtual basic_array <Coord> *clone () const 
  {
    return new iterated_array <Coord> (*this);
  }

  virtual box_type bbox (const box_type &obox) const
  {
    if (obox.empty ()) {
      return obox;
    } else {
      return box_type (obox.p1 () + (m_box.p1 () - point_type ()), obox.p2 () + (m_box.p2 () - point_type ()));
    }
  }

  virtual size_t size () const 
  {
    return m_v.size ();
  }

  virtual void invert (simple_trans_type &t) 
  {
    t.invert ();
    db::fixpoint_trans<coord_type> f (t.rot ());
    m_box = box_type ();
    for (iterator p = m_v.begin (); p != m_v.end (); ++p) {
      *p = -f (*p);
      m_box += point_type () + *p;
    }
    sort ();
  }

  virtual bool equal (const ArrayBase *b) const
  {
    const iterated_array<Coord> *d = static_cast<const iterated_array<Coord> *> (b);
    if (m_v.size () != d->m_v.size ()) {
      return false;
    }
    for (const_iterator p1 = m_v.begin (), p2 = d->m_v.begin (); p1 != m_v.end (); ++p1, ++p2) {
      if (*p1 != *p2) {
        return false;
      }
    }
    return true;
  }

  virtual bool fuzzy_equal (const ArrayBase *b) const
  {
    const iterated_array<Coord> *d = static_cast<const iterated_array<Coord> *> (b);
    if (m_v.size () != d->m_v.size ()) {
      return false;
    }
    for (const_iterator p1 = m_v.begin (), p2 = d->m_v.begin (); p1 != m_v.end (); ++p1, ++p2) {
      if (! p1->equal (*p2)) {
        return false;
      }
    }
    return true;
  }

  virtual bool less (const ArrayBase *b) const
  {
    const iterated_array<Coord> *d = static_cast<const iterated_array<Coord> *> (b);
    if (m_v.size () != d->m_v.size ()) {
      return (m_v.size () < d->m_v.size ());
    }
    for (const_iterator p1 = m_v.begin (), p2 = d->m_v.begin (); p1 != m_v.end (); ++p1, ++p2) {
      if (*p1 != *p2) {
        return (*p1 < *p2);
      }
    }
    return false;
  }

  virtual bool fuzzy_less (const ArrayBase *b) const
  {
    const iterated_array<Coord> *d = static_cast<const iterated_array<Coord> *> (b);
    if (m_v.size () != d->m_v.size ()) {
      return (m_v.size () < d->m_v.size ());
    }
    for (const_iterator p1 = m_v.begin (), p2 = d->m_v.begin (); p1 != m_v.end (); ++p1, ++p2) {
      if (! p1->equal (*p2)) {
        return (p1->less (*p2));
      }
    }
    return false;
  }

  virtual void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, m_v, true, (void *) this);
  }

  virtual unsigned int type () const
  {
    return 3;
  }

  virtual void transform (const complex_trans_type &ct)
  { 
    m_box = box_type ();
    for (iterator p = m_v.begin (); p != m_v.end (); ++p) {
      *p = vector_type (ct * *p);
      m_box += point_type () + *p;
    }
    sort ();
  }

  virtual void transform (const simple_trans_type &st)
  {
    for (iterator p = m_v.begin (); p != m_v.end (); ++p) {
      *p = vector_type (st * *p);
    }
    m_box.transform (st);
    sort ();
  }

protected:
  box_tree_type m_v;
  box_type m_box;
};

/**
 *  @brief The array specialization for the iterated, complex case (internal)
 *
 *  The iterated case is that the displacements are described as
 *    P=O+a*A+b*B (a=[amin..amax),b=[bmin..bmax)).
 *  Each instance is magnified with the given magnification and subject to
 *  further rotation by a residual angle given by acos, which is actually the cosine
 *  of this angle.
 */

template <class Coord>
struct iterated_complex_array 
  : public iterated_array<Coord>
{
  typedef typename basic_array<Coord>::point_type point_type;
  typedef typename basic_array<Coord>::vector_type vector_type;
  typedef typename basic_array<Coord>::box_type box_type;
  typedef typename basic_array<Coord>::coord_type coord_type;
  typedef typename basic_array<Coord>::disp_type disp_type;
  typedef typename basic_array<Coord>::simple_trans_type simple_trans_type;
  typedef typename basic_array<Coord>::complex_trans_type complex_trans_type;

  iterated_complex_array (double acos, double mag)
    : iterated_array<Coord> (), m_acos (acos), m_mag (mag)
  { 
    //  .. nothing yet ..
  }

  template <class I>
  iterated_complex_array (double acos, double mag, I from, I to)
    : iterated_array<Coord> (from, to), m_acos (acos), m_mag (mag)
  { 
    //  .. nothing yet ..
  }

  virtual basic_array <Coord> *clone () const 
  {
    return new iterated_complex_array <Coord> (*this);
  }

  virtual void invert (simple_trans_type &t) 
  {
    //  recompute the array parameters such that every per-instance transformation
    //  is inverted. This code is somewhat complex to maintain the splitting between 
    //  the simple transformation part and the residual given by m_acos and m_mag.
    complex_trans_type r = complex_trans_type (t, m_acos, m_mag).inverted ();
    m_mag = r.mag ();
    m_acos = r.rcos ();

    t = simple_trans_type (r);
    this->m_box = box_type ();
    for (typename iterated_array<Coord>::iterator p = this->m_v.begin (); p != this->m_v.end (); ++p) {
      *p = -r (*p);
      this->m_box += point_type () + *p;
    }
    iterated_array<Coord>::sort ();
  }

  virtual bool equal (const ArrayBase *b) const
  {
    const iterated_complex_array<Coord> *d = static_cast<const iterated_complex_array<Coord> *> (b);
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return false;
    }
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return false;
    }
    return iterated_array<Coord>::equal (b);
  }

  virtual bool fuzzy_equal (const ArrayBase *b) const
  {
    const iterated_complex_array<Coord> *d = static_cast<const iterated_complex_array<Coord> *> (b);
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return false;
    }
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return false;
    }
    return iterated_array<Coord>::fuzzy_equal (b);
  }

  virtual bool less (const ArrayBase *b) const
  {
    const iterated_complex_array<Coord> *d = static_cast<const iterated_complex_array<Coord> *> (b);
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return m_acos < d->m_acos;
    }
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return m_mag < d->m_mag;
    }
    return iterated_array<Coord>::less (b);
  }

  virtual bool fuzzy_less (const ArrayBase *b) const
  {
    const iterated_complex_array<Coord> *d = static_cast<const iterated_complex_array<Coord> *> (b);
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return m_acos < d->m_acos;
    }
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return m_mag < d->m_mag;
    }
    return iterated_array<Coord>::fuzzy_less (b);
  }

  virtual complex_trans_type complex_trans (const simple_trans_type &s) const
  {
    return complex_trans_type (s, m_acos, m_mag);
  }

  virtual bool is_complex () const
  {
    return true;
  }

  virtual unsigned int type () const
  {
    return 4;
  }

private:
  double m_acos, m_mag;
};

/**
 *  @brief The array specialization for the single magnified case (internal)
 *
 *  In the single case the displacement is 0.
 */

template <class Coord>
struct single_complex_inst 
  : public basic_array <Coord>
{
  typedef typename basic_array<Coord>::point_type point_type;
  typedef typename basic_array<Coord>::box_type box_type;
  typedef typename basic_array<Coord>::coord_type coord_type;
  typedef typename basic_array<Coord>::disp_type disp_type;
  typedef typename basic_array<Coord>::complex_trans_type complex_trans_type;
  typedef typename basic_array<Coord>::simple_trans_type simple_trans_type;

  single_complex_inst (double acos, double mag)
    : m_acos (acos), m_mag (mag)
  { 
    //  .. nothing yet ..
  }

  virtual std::pair <basic_array_iterator <Coord> *, bool>
  begin_touching (const box_type &b) const
  {
    return std::make_pair ((basic_array_iterator <Coord> *) 0, ! b.contains (point_type (0, 0))); 
  }
  
  virtual std::pair <basic_array_iterator <Coord> *, bool>
  begin () const
  {
    return std::make_pair ((basic_array_iterator <Coord> *) 0, false);
  }

  virtual basic_array <Coord> *clone () const 
  {
    return new single_complex_inst <Coord> (*this);
  }

  virtual box_type bbox (const box_type &obox) const
  {
    return obox;
  }

  virtual size_t size () const 
  {
    return 1;
  }

  virtual void invert (simple_trans_type &t) 
  {
    complex_trans_type r = complex_trans_type (t, m_acos, m_mag).inverted ();
    m_mag = r.mag ();
    m_acos = r.rcos ();
    t = simple_trans_type (r);
  }

  virtual bool equal (const ArrayBase *b) const 
  {
    const double epsilon = 1e-10;
    const single_complex_inst<Coord> *d = static_cast<const single_complex_inst<Coord> *> (b);
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return false;
    }
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return false;
    }
    return true;
  }

  virtual bool fuzzy_equal (const ArrayBase *b) const
  {
    return equal (b);
  }

  virtual bool less (const ArrayBase *b) const
  {
    const double epsilon = 1e-10;
    const single_complex_inst<Coord> *d = static_cast<const single_complex_inst<Coord> *> (b);
    if (fabs (m_mag - d->m_mag) > epsilon) {
      return m_mag < d->m_mag;
    }
    if (fabs (m_acos - d->m_acos) > epsilon) {
      return m_acos < d->m_acos;
    }
    return false;
  }

  virtual bool fuzzy_less (const ArrayBase *b) const
  {
    return less (b);
  }

  virtual void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
  }

  virtual complex_trans_type complex_trans (const simple_trans_type &s) const
  {
    return complex_trans_type (s, m_acos, m_mag);
  }

  virtual bool is_complex () const
  {
    return true;
  }

  virtual unsigned int type () const
  {
    return 5;
  }

private:
  double m_acos, m_mag;
};

//  this template computes the result type for the array iterator given 
//  a transformation and the minimum required capabilities to represent a displacement

template <class C, class T>
struct compute_result_trans
{
  typedef T result_trans;
};

template <class C>
struct compute_result_trans<C, unit_trans<C> >
{
  typedef disp_trans<C> result_trans;
};

template <class C>
struct compute_result_trans<C, fixpoint_trans<C> >
{
  typedef disp_trans<C> result_trans;
};

/** 
 *  @brief The array iterator 
 *
 *  This facade object wraps the actual basic_array iterator
 *  object. The iterator is not end-tested through comparing
 *  with end() but rather querying the at_end() property.
 *  This iterator as well acts as a single or zero instance
 *  iterator if the base pointer is 0.
 */

template <class Coord, class Trans>
struct array_iterator
{
  typedef Coord coord_type;
  typedef db::point <coord_type> point_type;
  typedef db::vector <coord_type> vector_type;
  typedef db::complex_trans <coord_type, coord_type> complex_trans_type;
  typedef Trans trans_type;
  typedef typename compute_result_trans<coord_type, trans_type>::result_trans result_type;
  //  dummy definitions to satisfy iterator traits (without making much sense):
  typedef result_type reference;
  typedef result_type value_type;
  typedef void pointer_type;
  typedef void difference_type;
  typedef void pointer;
  typedef std::forward_iterator_tag iterator_category;

  /**
   *  @brief The default constructor
   */
  array_iterator ()
    : m_trans (), mp_base (0), m_done (true)
  { 
    // .. nothing yet ..
  }

  /**
   *  @brief The iterator constructor
   *
   *  The constructor will receive a basic_array_iterator object
   *  and take over ownership over this.
   *  This is a convenience constructor that combines the next two constructors.
   */
  array_iterator (const trans_type &trans, std::pair <db::basic_array_iterator <Coord> *, bool> base)
    : m_trans (trans), mp_base (base.first), m_done (base.second)
  { 
    // .. nothing yet ..
  }

  /**
   *  @brief The iterator constructor
   *
   *  The constructor will receive a basic_array_iterator object
   *  and take over ownership over this.
   */
  array_iterator (const trans_type &trans, db::basic_array_iterator <Coord> *base)
    : m_trans (trans), mp_base (base), m_done (false)
  { 
    // .. nothing yet ..
  }

  /**
   *  @brief The single iterator constructor
   *
   *  The constructor takes a flag indicating whether to represent
   *  zero (true) or one (false) instance.
   */
  array_iterator (const trans_type &trans, bool done)
    : m_trans (trans), mp_base (0), m_done (done)
  { 
    // .. nothing yet ..
  }

  /**
   *  @brief The copy constructor
   */
  array_iterator (const array_iterator &d)
    : m_trans (d.m_trans), mp_base (0), m_done (d.m_done)
  {
    if (mp_base) {
      delete mp_base;
    }
    mp_base = d.mp_base ? d.mp_base->clone () : 0;
  }

  /**
   *  @brief The assignment operator
   */
  array_iterator &operator= (const array_iterator &d)
  {
    if (&d != this) {
      m_trans = d.m_trans;
      m_done = d.m_done;
      if (mp_base) {
        delete mp_base;
      }
      mp_base = d.mp_base ? d.mp_base->clone () : 0;
    }
    return *this;
  }

  /**
   *  @brief The destructor
   *
   *  This will delete the basic_array_iterator object passed
   *  in the constructor
   */
  ~array_iterator ()
  {
    if (mp_base) {
      delete mp_base;
    }
    mp_base = 0;
  }

  /**
   *  @brief The access operator
   *
   *  This delivers a displacement vector that is associated with
   *  the iterator's "position".
   */
  result_type operator* () const 
  {
    if (mp_base) {
      return result_type (mp_base->get ()) * result_type (m_trans);
    } else {
      return result_type (m_trans);
    }
  }
  
  /**
   *  @brief Increment operator
   */
  array_iterator &operator++ () 
  {
    if (mp_base) {
      mp_base->inc ();
    } else {
      m_done = true;
    }
    return *this;
  }

  /**
   *  @brief End test
   *
   *  @return true, if the iterator is at the end.
   */
  bool at_end () const 
  {
    return mp_base ? mp_base->at_end () : m_done;
  }

  /**
   *  @brief If the iterator is a regular iterator, get the "a" index
   *
   *  If the iterator is not a regular one, returns -1.
   */
  long index_a () const
  {
    return mp_base ? mp_base->index_a () : -1;
  }

  /**
   *  @brief If the iterator is a regular iterator, get the "b" index
   *
   *  If the iterator is not a regular one, returns -1.
   */
  long index_b () const
  {
    return mp_base ? mp_base->index_b () : -1;
  }

  /**
   *  @brief For iterators supporting quads (iterated arrays), this method will return the quad ID
   */
  size_t quad_id () const
  {
    return mp_base ? mp_base->quad_id () : 0;
  }

  /**
   *  @brief For iterators supporting quads (iterated arrays), this method will return the quad bounding box
   *
   *  Note that this method will only return a valid quad box is the quad_id is non-null.
   *
   *  This method will return the bounding box of all array offsets in the quad.
   */
  db::box<Coord> quad_box () const
  {
    return mp_base ? mp_base->quad_box () : db::box<Coord>::world ();
  }

  /**
   *  @brief For iterators supporting quads (iterated arrays), this method will skip the current quad
   */
  void skip_quad ()
  {
    if (mp_base) {
      mp_base->skip_quad ();
    }
  }

private:
  trans_type m_trans;
  basic_array_iterator <Coord> *mp_base;
  bool m_done;
};

/**
 *  @brief The array container
 *
 *  This is the facade object that wraps the array implementation
 *  which is either a regular_array or another object derived
 *  from a basic_array object.
 *  Access to the object instances is through the iterators which
 *  deliver the displacement vectors of the various instances, 
 *  possibly filtered by a box region.
 *
 *  Arrays are currently used for creating shape and instance arrays.
 *  There is a fundamental difference between shape and instance arrays:
 *  while shape arrays can transform the object and must not make use 
 *  of complex transformation type arrays, instance arrays cannot transform
 *  the object itself and must store every aspect of the transformation
 *  in the array. Instance arrays thus make use of the complex type
 *  feature of the array.
 */

template <class Obj, class Trans>
struct array
{
  typedef typename Trans::coord_type coord_type;
  typedef db::box <coord_type> box_type;
  typedef db::point <coord_type> point_type;
  typedef db::vector <coord_type> vector_type;
  typedef db::complex_trans <coord_type, coord_type> complex_trans_type;
  typedef db::simple_trans <coord_type> simple_trans_type;
  typedef db::disp_trans <coord_type> disp_trans_type;
  typedef db::unit_trans <coord_type> unit_trans_type;
  typedef Trans trans_type;
  typedef db::array_iterator <coord_type, Trans> iterator;
  typedef Obj object_type;
  typedef db::object_tag< array<Obj, Trans> > tag;
  typedef db::iterated_array<coord_type> iterated_array_type;
  typedef db::iterated_complex_array<coord_type> iterated_complex_array_type;

  /**
   *  @brief The default constructor
   */
  array ()
    : m_obj (), m_trans (), mp_base (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The array constructor
   *
   *  The array object takes over the ownership over the
   *  basic_array object.
   *
   *  @param obj The object to put into an array
   *  @param base The basic_array object that describes the array further
   */
  array (const Obj &obj, const trans_type &trans, basic_array <coord_type> *base)
    : m_obj (obj), m_trans (trans), mp_base (base)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The regular array constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object.
   */
  array (const Obj &obj, const trans_type &trans, const vector_type &a, const vector_type &b, unsigned long amax, unsigned long bmax)
    : m_obj (obj), m_trans (trans), mp_base (new regular_array <coord_type> (a, b, amax, bmax))
  {
    //  .. nothing yet ..
  }
  
  /**
   *  @brief The regular array constructor using an array repository
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object. 
   *  It uses the array repository to store the base object rather than providing 
   *  it's own storage.
   */
  array (const Obj &obj, const trans_type &trans, ArrayRepository &rep, const vector_type &a, const vector_type &b, unsigned long amax, unsigned long bmax)
    : m_obj (obj), m_trans (trans), mp_base (rep.insert (regular_array <coord_type> (a, b, amax, bmax)))
  {
    //  .. nothing yet ..
  }
  
  /**
   *  @brief The singular array constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object.
   *  mp_base == 0 is implicitly a single-object array.
   */
  explicit 
  array (const Obj &obj, const trans_type &trans)
    : m_obj (obj), m_trans (trans), mp_base (0)
  {
    //  .. nothing yet ..
  }
  
  /**
   *  @brief The regular complex array constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object.
   */
  array (const Obj &obj, const trans_type &trans, double acos, double mag, const vector_type &a, const vector_type &b, unsigned long amax, unsigned long bmax)
    : m_obj (obj), m_trans (trans), mp_base (new regular_complex_array <coord_type> (acos, mag, a, b, amax, bmax))
  {
    //  .. nothing yet ..
  }
  
  /**
   *  @brief The regular complex array constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object using a complex transformation.
   */
  array (const Obj &obj, const complex_trans_type &ct, const vector_type &a, const vector_type &b, unsigned long amax, unsigned long bmax)
    : m_obj (obj), m_trans (ct), mp_base (ct.is_complex () ? new regular_complex_array <coord_type> (ct.rcos (), ct.mag (), a, b, amax, bmax) : new regular_array <coord_type> (a, b, amax, bmax))
  {
    //  .. nothing yet ..
  }
  
  /**
   *  @brief The regular complex array constructor using an array repository
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object.
   *  It uses the array repository to store the base object rather than providing 
   *  it's own storage.
   */
  array (const Obj &obj, const trans_type &trans, ArrayRepository &rep, double acos, double mag, const vector_type &a, const vector_type &b, unsigned long amax, unsigned long bmax)
    : m_obj (obj), m_trans (trans), mp_base (rep.insert (regular_complex_array <coord_type> (acos, mag, a, b, amax, bmax)))
  {
    //  .. nothing yet ..
  }
  
  /**
   *  @brief The regular complex array constructor using an array repository
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object using a complex transformation.
   *  It uses the array repository to store the base object rather than providing 
   *  it's own storage.
   */
  array (const Obj &obj, const complex_trans_type &ct, ArrayRepository &rep, const vector_type &a, const vector_type &b, unsigned long amax, unsigned long bmax)
    : m_obj (obj), m_trans (ct), mp_base (ct.is_complex () ? rep.insert (regular_complex_array <coord_type> (ct.rcos (), ct.mag (), a, b, amax, bmax)) : rep.insert (regular_array <coord_type> (a, b, amax, bmax)))
  {
    //  .. nothing yet ..
  }
  
  /**
   *  @brief The singular complex instance constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object.
   */
  explicit 
  array (const Obj &obj, const trans_type &trans, double acos, double mag)
    : m_obj (obj), m_trans (trans), mp_base (new single_complex_inst <coord_type> (acos, mag))
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The iterated array constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object.
   */
  template <class Iter>
  array (const Obj &obj, const trans_type &trans, Iter from, Iter to)
    : m_obj (obj), m_trans (trans), mp_base (new iterated_array <coord_type> (from, to))
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The complex iterated array constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object.
   */
  template <class Iter>
  array (const Obj &obj, const complex_trans_type &ct, Iter from, Iter to)
    : m_obj (obj), m_trans (ct), mp_base (ct.is_complex () ? new iterated_complex_array <coord_type> (ct.rcos (), ct.mag (), from, to) : new iterated_array <coord_type> (from, to))
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The singular complex instance constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object using a complex transformation.
   */
  explicit 
  array (const Obj &obj, const complex_trans_type &ct)
    : m_obj (obj), m_trans (ct), mp_base (ct.is_complex () ? new single_complex_inst <coord_type> (ct.rcos (), ct.mag ()) : 0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The singular complex instance constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object.
   *  It uses the array repository to store the base object rather than providing 
   *  it's own storage.
   */
  explicit 
  array (const Obj &obj, const trans_type &trans, ArrayRepository &rep, double acos, double mag)
    : m_obj (obj), m_trans (trans), mp_base (rep.insert (single_complex_inst <coord_type> (acos, mag)))
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The singular complex instance constructor
   *
   *  This is basically a convenience function that creates
   *  an appropriate basic_array object using a complex transformation.
   *  It uses the array repository to store the base object rather than providing 
   *  it's own storage.
   */
  explicit 
  array (const Obj &obj, const complex_trans_type &ct, ArrayRepository &rep)
    : m_obj (obj), m_trans (ct), mp_base (ct.is_complex () ? rep.insert (single_complex_inst <coord_type> (ct.rcos (), ct.mag ())) : 0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The destructor
   *
   *  This will delete the basic_array object that was passed
   *  to the constructor.
   */
  ~array ()
  {
    if (mp_base && ! mp_base->in_repository) {
      delete mp_base;
    }
    mp_base = 0;
  }

  /**
   *  @brief The copy constructor
   */
  array (const array &d)
    : m_obj (d.m_obj), m_trans (d.m_trans), mp_base (0)
  {
    if (d.mp_base) {
      mp_base = d.mp_base->in_repository ? d.mp_base : d.mp_base->clone ();
    }
  }

  /**
   *  @brief The copy constructor with array repository translation
   *
   *  If the array repository pointer is 0, the d array will be detached from an repository.
   *  Otherwise it will be translated to the given repository.
   */
  array (const array &d, ArrayRepository *rep)
    : m_obj (d.m_obj), m_trans (d.m_trans), mp_base (0)
  {
    if (d.mp_base) {
      if (rep) {
        mp_base = rep->insert (*d.mp_base);
      } else {
        mp_base = d.mp_base->clone ();
      }
    }
  }

  /**
   *  @brief The assignment operator
   */
  array &operator= (const array &d)
  {
    if (&d != this) {
      m_trans = d.m_trans;
      m_obj = d.m_obj;
      if (mp_base && ! mp_base->in_repository) {
        delete mp_base;
      }
      if (d.mp_base) {
        mp_base = d.mp_base->in_repository ? d.mp_base : d.mp_base->clone ();
      } else {
        mp_base = 0;
      }
    }
    return *this;
  }

  /**
   *  @brief The region-selective iterator
   *
   *  There is no end() method. Rather use the at_end() method
   *  of the iterator to determine the end of the sequence.
   *  The iterator returned will report all object displacement 
   *  vectors of the objects that touch the given box and 
   *  possibly some more.
   */
  template <class BoxConv>
  array_iterator <coord_type, Trans> begin_touching (const box_type &b, const BoxConv &bc) const 
  {
    if (b.empty ()) {
      return array_iterator <coord_type, Trans> (m_trans, true);
    } else if (b == box_type::world ()) {
      return begin ();
    } else if (mp_base) {
      box_type ob (bc (m_obj));
      if (ob.empty ()) {
        return array_iterator <coord_type, Trans> (m_trans, true);
      } else {
        if (mp_base->is_complex ()) {
          complex_trans_type ct = mp_base->complex_trans (simple_trans_type (m_trans));
          ct.disp (typename complex_trans_type::displacement_type ());
          ob = box_type (ct * ob);
        } else {
          ob.transform (db::fixpoint_trans<coord_type> (m_trans.rot ()));
        }
        vector_type d = m_trans.disp ();
        return array_iterator <coord_type, Trans> (m_trans, mp_base->begin_touching (box_type (point_type () + (b.p1 () - (ob.p2 () + d)), point_type () + (b.p2 () - (ob.p1 () + d)))));
      }
    } else {
      box_type ob (bc (m_obj));
      if (ob.empty ()) {
        return array_iterator <coord_type, Trans> (m_trans, true); 
      } else {
        vector_type d = m_trans.disp ();
        ob.transform (db::fixpoint_trans<coord_type> (m_trans.rot ()));
        return array_iterator <coord_type, Trans> (m_trans, ! box_type (point_type () + (b.p1 () - (ob.p2 () + d)), point_type () + (b.p2 () - (ob.p1 () + d))).contains (point_type ()));
      }
    }
  }
  
  /**
   *  @brief The regular array member iterator
   *
   *  This method delivers an array iterator pointing to the member with the given indexes,
   *  provided the array is a regular one. If the array is not regular this method
   *  will deliver the same iterator than begin ().
   *  There is no end() method. Rather use the at_end() method
   *  of the iterator to determine the end of the sequence.
   */
  array_iterator <coord_type, Trans> begin (long a, long b) const 
  {
    if (mp_base) {
      return array_iterator <coord_type, Trans> (m_trans, mp_base->begin_regular (a, b));
    } else {
      return array_iterator <coord_type, Trans> (m_trans, false);
    }
  }

  /**
   *  @brief The non-selective iterator
   *
   *  There is no end() method. Rather use the at_end() method
   *  of the iterator to determine the end of the sequence.
   */
  array_iterator <coord_type, Trans> begin () const 
  {
    if (mp_base) {
      return array_iterator <coord_type, Trans> (m_trans, mp_base->begin ());
    } else {
      return array_iterator <coord_type, Trans> (m_trans, false);
    }
  }

  /**
   *  @brief The bbox of the array
   */
  template <class BoxConv>
  box_type bbox (const BoxConv &bc) const
  {
    if (mp_base) {
      if (mp_base->is_complex ()) {
        return mp_base->bbox (box_type (mp_base->complex_trans (simple_trans_type (m_trans)) * bc (m_obj)));
      } else {
        return mp_base->bbox (m_trans * bc (m_obj));
      }
    } else {
      return m_trans * bc (m_obj);
    }
  }

  /**
   *  @brief The raw bounding box of the array
   *
   *  The raw bounding box is the box that encloses the array instantiation
   *  points, ignoring the extensions of the object.
   *  The raw bounding boxes can be accumulated for all arrays having the same
   *  orientation, magnification and object. 
   *  bbox_from_raw_bbox can be used to compute the total bbox from such an
   *  accumulated raw bounding box. This is for example exploited in the 
   *  update_bbox method of db::cell.
   */
  box_type raw_bbox () const
  {
    point_type d = m_trans (point_type ());
    if (mp_base) {
      return mp_base->bbox (box_type (d, d));
    } else {
      return box_type (d, d);
    }
  }

  /**
   *  @brief The bbox of the array computed from the raw bounding box
   *
   *  See "raw_bbox" for a description.
   */
  template <class BoxConv>
  box_type bbox_from_raw_bbox (const box_type &rb, const BoxConv &bc) const
  {
    if (mp_base) {
      if (mp_base->is_complex ()) {
        return rb * (box_type (mp_base->complex_trans (simple_trans_type (m_trans.fp_trans ())) * bc (m_obj)));
      } else {
        return rb * (m_trans.fp_trans () * bc (m_obj));
      }
    } else {
      return rb * (m_trans.fp_trans () * bc (m_obj));
    }
  }

  /**
   *  @brief Gets the bounding box from the iterator's current quad
   *
   *  The bounding box is that of all objects in the current quad and
   *  is confined to the array's total bounding box.
   */
  template <class Iter, class BoxConv>
  box_type quad_box (const Iter &iter, const BoxConv &bc) const
  {
    box_type bb;
    if (mp_base) {
      bb = mp_base->bbox (box_type (0, 0, 0, 0));
    }
    bb &= iter.quad_box ();

    if (mp_base) {
      if (mp_base->is_complex ()) {
        return bb * box_type (mp_base->complex_trans (simple_trans_type (m_trans)) * bc (m_obj));
      } else {
        return bb * (m_trans * bc (m_obj));
      }
    } else {
      return bb * (m_trans * bc (m_obj));
    }
  }

  /**
   *  @brief The number of single instances in the array
   */
  size_t size () const
  {
    if (mp_base) {
      return mp_base->size ();
    } else {
      return 1;
    }
  }

  /**
   *  @brief Gets the simple component of the transformation
   *
   *  This component does not include additional transformations due to complex
   *  components and array-item displacements.
   */
  const trans_type &front () const
  {
    return m_trans;
  }

  /**
   *  @brief Access to the object in the instance
   */
  Obj &object ()
  {
    return m_obj;
  }

  /**
   *  @brief Access to the object in the instance (const version)
   */
  const Obj &object () const
  {
    return m_obj;
  }

  /**
   *  @brief Invert an array reference
   */
  void invert ()
  {
    if (mp_base) {
      //  Hint: detach from repository so we can invert
      if (mp_base->in_repository) {
        mp_base = mp_base->clone ();
      }
      //  Hint: this assumes that the delegate does not carry a "superior" transformation over trans_type - 
      //  i.e. no complex trans inside arrays stating a unit_trans type for example. The assertion checks that.
      simple_trans_type t (m_trans);
      mp_base->invert (t);
      m_trans = trans_type (t);
      tl_assert (simple_trans_type (m_trans) == t);
    } else {
      m_trans.invert ();
    }
  }

  /**
   *  @brief Compare operator for inequality
   */
  bool operator!= (const array<Obj, Trans> &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Compare operator for equality
   */
  bool operator== (const array<Obj, Trans> &d) const
  {
    if (! mp_base) {
      return (m_trans == d.m_trans && m_obj == d.m_obj && ! d.mp_base);
    } else {
      if (m_trans != d.m_trans || ! (m_obj == d.m_obj) || type () != d.type ()) {
        return false;
      }
      return mp_base && mp_base->equal (d.mp_base);
    }
  }

  /**
   *  @brief Compare operator for equality (fuzzy version)
   */
  bool equal (const array<Obj, Trans> &d) const
  {
    if (! mp_base) {
      return (m_trans.equal (d.m_trans) && m_obj == d.m_obj && ! d.mp_base);
    } else {
      if (! m_trans.equal (d.m_trans) || ! (m_obj == d.m_obj) || type () != d.type ()) {
        return false;
      }
      return mp_base && mp_base->fuzzy_equal (d.mp_base);
    }
  }

  /**
   *  @brief A sorting order criterion
   */
  bool operator< (const array<Obj, Trans> &d) const
  {
    if (! (m_obj == d.m_obj)) {
      return (m_obj < d.m_obj);
    }
    if (m_trans != d.m_trans) {
      return (m_trans < d.m_trans);
    }
    if (type () != d.type ()) {
      return (type () < d.type ());
    }
    if (mp_base == d.mp_base) {
      return false;
    } else if (! mp_base) {
      return true;
    } else if (! d.mp_base) {
      return false;
    } else {
      return mp_base->less (d.mp_base);
    }
  }

  /**
   *  @brief A fuzzy sorting order criterion
   */
  bool less (const array<Obj, Trans> &d) const
  {
    if (! (m_obj == d.m_obj)) {
      return (m_obj < d.m_obj);
    }
    if (! m_trans.equal (d.m_trans)) {
      return m_trans.less (d.m_trans);
    }
    if (type () != d.type ()) {
      return (type () < d.type ());
    }
    if (mp_base == d.mp_base) {
      return false;
    } else if (! mp_base) {
      return true;
    } else if (! d.mp_base) {
      return false;
    } else {
      return mp_base->fuzzy_less (d.mp_base);
    }
  }

  /**
   *  @brief Compare operator
   *
   *  Implementation of a comparison operator that just
   *  compares the object and the matrix part of the transformation.
   *  Needed for sorting of cell instances for example.
   */
  bool raw_less (const array<Obj, Trans> &d) const
  {
    if (! (m_obj == d.m_obj)) {
      return (m_obj < d.m_obj);
    }
    if (m_trans.rot () != d.m_trans.rot ()) {
      return (m_trans.rot () < d.m_trans.rot ());
    }
    if (is_complex () != d.is_complex ()) {
      return is_complex () < d.is_complex ();
    }
    if (is_complex ()) {
      complex_trans_type ct1 (complex_trans ());
      complex_trans_type ct2 (d.complex_trans ());
      if (ct1.mcos () != ct2.mcos ()) {
        return ct1.mcos () < ct2.mcos ();
      }
      if (ct1.msin () != ct2.msin ()) {
        return ct1.msin () < ct2.msin ();
      }
      if (ct1.mag () != ct2.mag ()) {
        return ct1.mag () < ct2.mag ();
      }
    }
    return false;
  }

  /**
   *  @brief Compare operator (equal)
   *
   *  Implementation of a comparison operator that just
   *  compares the object and the matrix part of the transformation.
   *  Needed for sorting of cell instances for example.
   */
  bool raw_equal (const array<Obj, Trans> &d) const
  {
    if (! (m_obj == d.m_obj)) {
      return false;
    }
    if (m_trans.rot () != d.m_trans.rot ()) {
      return false;
    }
    if (is_complex () != d.is_complex ()) {
      return false;
    }
    if (is_complex ()) {
      complex_trans_type ct1 (complex_trans ());
      complex_trans_type ct2 (d.complex_trans ());
      if (ct1.mcos () != ct2.mcos ()) {
        return false;
      }
      if (ct1.msin () != ct2.msin ()) {
        return false;
      }
      if (ct1.mag () != ct2.mag ()) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Returns true, if the array lives in an array repository
   */
  bool in_repository () const
  {
    return mp_base && mp_base->in_repository;
  }

  /**
   *  @brief Test, if the array is a complex array
   *
   *  Returns true if the array represents complex instances (that is, with magnification and 
   *  arbitrary rotation angles).
   */
  bool is_complex () const
  {
    return mp_base && mp_base->is_complex ();
  }

  /**
   *  @brief Return the type code 
   *
   *  Returns the type code of the array. Mainly used internally.
   */
  int type () const
  {
    return mp_base ? mp_base->type () : 0;
  }

  /**
   *  @brief Return the complex_transformation
   *
   *  Returns the complex transformation that incorporates magnification and arbitrary angle rotation.
   *  This method returns the complex transformation for the first element in the array. To obtain the
   *  complex transformation for a specific element, use 'complex_trans (*iter)' instead.
   */
  complex_trans_type complex_trans () const
  {
    return mp_base ? mp_base->complex_trans (simple_trans_type (m_trans)) : complex_trans_type (m_trans);
  }

  /**
   *  @brief Return the complex_transformation
   *
   *  Returns the complex transformation that incorporates magnification and arbitrary angle rotation
   *  for a given base transformation. This method is intended to be used with the base transformation
   *  delivered by an iterator.
   */
  complex_trans_type complex_trans (const trans_type &trans) const
  {
    return mp_base ? mp_base->complex_trans (simple_trans_type (trans)) : complex_trans_type (trans);
  }

  /**
   *  @brief Test, if the array is a regular one
   *
   *  Returns true and the array parameters if the array is a regular
   *  one. 
   */
  bool is_regular_array (vector_type &a, vector_type &b, unsigned long &amax, unsigned long &bmax) const
  {
    return mp_base && mp_base->is_regular_array (a, b, amax, bmax);
  }

  /**
   *  @brief Test, if the array is an iterated one
   *
   *  Returns true and the array parameters if the array is a iterated
   *  one. If v is 0, no vector of points is returned
   */
  bool is_iterated_array (std::vector<vector_type> *v = 0) const
  {
    return mp_base && mp_base->is_iterated_array (v);
  }

  /**
   *  @brief The translation operator
   *
   *  This operator is required since shape reference arrays are put into db::Layer objects
   *
   *  Hint this method is supposed to be used for shape arrays. Instance arrays to not support the translate method currently.
   *  TODO: this functionality should be put into the Shapes translate method rather than into this class ...
   */
  void translate (const array<Obj, Trans> &d, db::generic_repository<coord_type> &rep, db::ArrayRepository &array_rep) 
  {
    m_obj.translate (d.m_obj, rep, array_rep);
    m_trans = d.m_trans;
    if (mp_base && ! mp_base->in_repository) {
      delete mp_base;
    }
    if (d.mp_base) {
      mp_base = d.mp_base->in_repository ? array_rep.insert (*d.mp_base) : d.mp_base->clone ();
    } else {
      mp_base = 0;
    }
  }

  /**
   *  @brief The translation operator with transformation 
   *
   *  Hint this method is supposed to be used for shape arrays. Instance arrays to not support the translate method currently.
   *  TODO: this functionality should be put into the Shapes translate method rather than into this class ...
   */
  template <class T>
  void translate (const array<Obj, Trans> &d, const T &t, db::generic_repository<coord_type> &rep, db::ArrayRepository &array_rep) 
  {
    //  Translate the array into the new system and if necessary, transform and translate the object
    translate_from (t, d, rep, array_rep);

    //  remove the current delegate
    if (mp_base && ! mp_base->in_repository) {
      delete mp_base;
      mp_base = 0;
    }

    //  transform the delegate 
    if (d.mp_base) {
      basic_array <coord_type> *new_base = d.mp_base->clone ();
      new_base->transform (t);
      mp_base = array_rep.insert (*new_base);
      delete new_base;
    }
  }

  /**
   *  @brief Swap with another object
   *
   *  This implementation does not require to reallocate the base pointer
   *  which might speed up somewhat the sorting of cell instances.
   */
  void
  swap (db::array<Obj, Trans> &b)
  {
    std::swap (m_trans, b.m_trans);
    std::swap (m_obj, b.m_obj);
    std::swap (mp_base, b.mp_base);
  }

  /**
   *  @brief For test purposes: return the delegate
   */
  const ArrayBase *delegate () const
  {
    return mp_base;
  }

  /**
   *  @brief In-place transformation of an array
   *
   *  This method will transform the array specification according to the given transformation.
   *
   *  @param tr The transformation to apply
   *  @param rep The repository where to enter the new array or 0 if no repository should be used
   *
   *  Hint: this method is supposed to be used for arrays with simple_transformation for the transformation type mainly. 
   *  The reason is that this method will create complex arrays from complex transformations. 
   *  Arrays based on reduced transformations (unit_trans, disp_trans) must not use complex arrays because that 
   *  is not supported currently. In other words, this method will not try to transform the object itself and rather
   *  put all transformation aspects into the array, which does not work on reduced transformation types, except
   *  if the applied transformation "tr" is also a reduced type.
   *  If required, the remaining component can be computed from the previous complex transformation and the new one
   *  using this formula: trem = t_new.inverted() * tr * t_old where t_new is complex_trans() or front() after this
   *  method has been called and t_old is complex_trans() or front() before this method was called.
   *
   *  Here is a table of compatibility:
   *     Trans     T                     Compatible
   *     unit      unit                  yes
   *     unit      disp,simple,complex   in general not
   *     disp      unit,disp             yes
   *     disp      simple,complex        in general not
   *     simple    all                   yes
   *
   *  For the non-compatible combinations, %translate is provided. This however requires that the object
   *  can be transformed ("translated" in the more general sense). That specifically applies to 
   *  shape arrays and references which are based on unit or disp transformations.
   */
  template <class T>
  void transform (const T &tr, db::ArrayRepository *array_rep = 0) 
  {
    transform_from (tr, *this);
    if (mp_base) {
      //  transform the delegate
      transform_delegate (tr, array_rep);
    }
  }

  /**
   *  @brief Moves the arrays
   *
   *  This is equivalent to transforming with a displacement
   */
  void move (const vector_type &pt, db::ArrayRepository *array_rep = 0)
  {
    transform (disp_trans_type (pt), array_rep);
  }

  /**
   *  @brief Transformation into a new system 
   *
   *  Arrays are transformed into a new system by applying a formal transformation
   *  A' = T A Ti (Ti is the inverse of T). Hence T A = A' T. In other words: the transformation
   *  T is propagated over the array A. This transformation is useful for transformation 
   *  of hierarchies, since the transformation T can be propagated further down in the hierarchy 
   *  therefore.
   *
   *  See the remarks on "transform" regarding compatibility of the transformation argument and 
   *  the Trans type of this array.
   *
   *  @param tr The transformation to apply
   *  @param rep The repository where to enter the new array or 0 if no repository should be used
   */
  template <class T>
  void transform_into (const T &tr, db::ArrayRepository *array_rep = 0) 
  {
    transform_into_from (tr, *this);
    if (mp_base) {
      //  transform the delegate
      transform_delegate (tr, array_rep);
    }
  }

  /**
   *  @brief Transformation of an array
   */
  template <class T>
  array transformed (const T &tr, db::ArrayRepository *array_rep = 0) const 
  {
    array a (*this);
    a.transform (tr, array_rep);
    return a;
  }

  /**
   *  @brief Transformation of an array into a new system
   *
   *  See the "transform_into" method for details about this transformation.
   */
  template <class T>
  array transformed_into (const T &tr, db::ArrayRepository *array_rep = 0) const 
  {
    array a (*this);
    a.transform_into (tr, array_rep);
    return a;
  }

  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (! no_self) {
      stat->add (typeid (array<Obj, Trans>), (void *) this, sizeof (array<Obj, Trans>), sizeof (array<Obj, Trans>), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, m_obj, true, (void *) this);
    if (mp_base) {
      mp_base->mem_stat (stat, purpose, cat, false, (void *) this);
    }
  }

private:
  Obj m_obj;
  trans_type m_trans;
  basic_array <coord_type> *mp_base;

  void transform_into_from (const unit_trans_type & /*tr*/, const array<Obj, Trans> & /*d*/)
  {
    //  .. nothing to do ..
  }

  void transform_into_from (const disp_trans_type &tr, const array<Obj, Trans> &d)
  {
    if (is_complex ()) {
      complex_trans_type t = d.complex_trans ().transform_into (tr);
      m_trans = Trans (t);
      set_complex (t.mag (), t.rcos (), d);
    } else {
      simple_trans_type t = simple_trans_type (tr) * d.front () * simple_trans_type (tr.inverted ());
      m_trans = Trans (t);
    }
  }

  void transform_into_from (const simple_trans_type &tr, const array<Obj, Trans> &d)
  {
    if (is_complex ()) {
      complex_trans_type t = d.complex_trans ().transform_into (tr);
      m_trans = trans_type (t);
      set_complex (t.mag (), t.rcos (), d);
    } else {
      simple_trans_type t = tr * d.front () * tr.inverted ();
      m_trans = Trans (t);
    }
  }

  void transform_into_from (const complex_trans_type &tr, const array<Obj, Trans> &d)
  {
    complex_trans_type t = d.complex_trans ().transform_into (tr);
    m_trans = trans_type (t);
    set_complex (t.mag (), t.rcos (), d);
  }

  void translate_from (const unit_trans_type & /*tr*/, const array<Obj, Trans> & /*d*/, db::generic_repository<coord_type> & /*rep*/, db::ArrayRepository & /*array_rep*/)
  {
    //  .. nothing to do ..
  }

  void translate_from (const disp_trans_type &tr, const array<Obj, Trans> &d, db::generic_repository<coord_type> &rep, db::ArrayRepository &array_rep)
  {
    if (is_complex ()) {
      complex_trans_type t = complex_trans_type (tr) * d.complex_trans ();
      m_trans = trans_type (t);
      set_complex (t.mag (), t.rcos (), d);
      m_obj.translate (d.m_obj, complex_trans ().inverted () * complex_trans_type (t), rep, array_rep);
    } else {
      simple_trans_type t = simple_trans_type (tr) * simple_trans_type (d.front ());
      m_trans = trans_type (t);
      m_obj.translate (d.m_obj, simple_trans_type (m_trans.inverted ()) * t, rep, array_rep);
    }
  }

  void translate_from (const simple_trans_type &tr, const array<Obj, Trans> &d, db::generic_repository<coord_type> &rep, db::ArrayRepository &array_rep)
  {
    if (is_complex ()) {
      complex_trans_type t = complex_trans_type (tr) * d.complex_trans ();
      m_trans = trans_type (t);
      set_complex (t.mag (), t.rcos (), d);
      m_obj.translate (d.m_obj, complex_trans ().inverted () * complex_trans_type (t), rep, array_rep);
    } else {
      simple_trans_type t = tr * simple_trans_type (d.front ());
      m_trans = trans_type (t);
      m_obj.translate (d.m_obj, simple_trans_type (m_trans.inverted ()) * t, rep, array_rep);
    }
  }

  void translate_from (const complex_trans_type &tr, const array<Obj, Trans> &d, db::generic_repository<coord_type> &rep, db::ArrayRepository &array_rep)
  {
    if (is_complex ()) {
      complex_trans_type t = tr * d.complex_trans ();
      m_trans = trans_type (t);
      set_complex (t.mag (), t.rcos (), d);
      m_obj.translate (d.m_obj, complex_trans ().inverted () * complex_trans_type (t), rep, array_rep);
    } else {
      //  don't create a complex array - delegate the transformation to the translate method
      complex_trans_type t = tr * complex_trans_type (d.front ());
      m_trans = trans_type (t);
      m_obj.translate (d.m_obj, complex_trans_type (m_trans.inverted ()) * t, rep, array_rep);
    }
  }

  void transform_from (const unit_trans_type & /*tr*/, const array<Obj, Trans> & /*d*/)
  {
    //  .. nothing to do ..
  }

  void transform_from (const disp_trans_type &tr, const array<Obj, Trans> &d)
  {
    m_trans = trans_type (simple_trans_type (tr) * simple_trans_type (d.front ()));
  }

  void transform_from (const simple_trans_type &tr, const array<Obj, Trans> &d)
  {
    m_trans = trans_type (tr * simple_trans_type (d.front ()));
  }

  void transform_from (const complex_trans_type &tr, const array<Obj, Trans> &d)
  {
    complex_trans_type t = tr * d.complex_trans ();
    //  TODO: this only works properly if Trans is a simple_trans_type!
    m_trans = trans_type (t);
    set_complex (t.mag (), t.rcos (), d);
  }

  void set_complex (double mag, double rcos, const array<Obj, Trans> &d)
  {
    basic_array <coord_type> *new_base = 0;

    //  if we finally have a complex transformation, set a new base object
    if (fabs (mag - 1.0) > epsilon || fabs (rcos - 1.0) > epsilon) {

      //  create a new base object with the transformed base vectors
      vector_type a, b;
      unsigned long amax, bmax;
      bool is_reg = d.is_regular_array (a, b, amax, bmax);

      std::vector<vector_type> v;
      bool is_iterated = d.is_iterated_array (&v);

      if (is_reg) {
        new_base = new regular_complex_array <coord_type> (rcos, mag, a, b, amax, bmax);
      } else if (is_iterated) {
        new_base = new iterated_complex_array <coord_type> (rcos, mag, v.begin (), v.end ());
      } else {
        new_base = new single_complex_inst <coord_type> (rcos, mag);
      }

    } else if (d.is_complex ()) {

      //  create a new base object with the transformed base vectors
      vector_type a, b;
      unsigned long amax, bmax;
      bool is_reg = d.is_regular_array (a, b, amax, bmax);

      std::vector<vector_type> v;
      bool is_iterated = d.is_iterated_array (&v);

      if (is_reg) {
        new_base = new regular_array <coord_type> (a, b, amax, bmax);
      } else if (is_iterated) {
        new_base = new iterated_array <coord_type> (v.begin (), v.end ());
      } else {
        //  a complex transformation delegate is no longer required
        if (mp_base && ! mp_base->in_repository) {
          delete mp_base;
        }
        mp_base = 0;
      }

    }

    if (new_base) {
      if (mp_base && ! mp_base->in_repository) {
        delete mp_base;
      }
      mp_base = new_base;
    }
  }

  template <class T>
  void transform_delegate (const T &tr, db::ArrayRepository *array_rep) 
  {
    //  transform the delegate
    if (! array_rep && ! mp_base->in_repository) {

      //  special case: allow in-place transformation
      mp_base->transform (tr);

    } else {

      basic_array <coord_type> *new_base = mp_base->clone ();
      new_base->transform (tr);

      if (! mp_base->in_repository) {
        delete mp_base;
      }

      if (array_rep) {
        mp_base = array_rep->insert (*new_base);
        delete new_base;
      } else {
        mp_base = new_base;
      }

    }
  }
};

/**
 *  @brief Collect memory statistics
 */
template <class Obj, class Trans>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const array<Obj, Trans> &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

namespace std
{

//  injecting a global std::swap for arrays into the 
//  std namespace
template <class Obj, class Trans>
void swap (db::array<Obj, Trans> &a, db::array<Obj, Trans> &b)
{
  a.swap (b);
}

} // namespace std

#endif

