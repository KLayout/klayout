
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_dbHierProcessorUtils
#define HDR_dbHierProcessorUtils

#include "dbCommon.h"
#include "dbLayout.h"
#include "tlThreads.h"

#include <unordered_map>

namespace db
{

// ---------------------------------------------------------------------------------------------
//  determines the default boolean core flag per result type

template <class TR>
struct default_boolean_core
{
  bool operator() () const { return false; }
};

template <>
struct default_boolean_core<db::PolygonRef>
{
  bool operator() () const { return true; }
};

// ---------------------------------------------------------------------------------------------
//  Shape reference translator

template <class Ref> class shape_reference_translator;
template <class Ref, class Trans> class shape_reference_translator_with_trans;

template <class Ref>
class shape_reference_translator
{
public:
  typedef typename Ref::shape_type shape_type;
  typedef typename Ref::trans_type ref_trans_type;

  shape_reference_translator (db::Layout *target_layout)
    : mp_layout (target_layout)
  {
    //  .. nothing yet ..
  }

  Ref operator() (const Ref &ref) const
  {
    typename std::unordered_map<const shape_type *, const shape_type *>::const_iterator m = m_cache.find (ref.ptr ());
    if (m != m_cache.end ()) {

      return Ref (m->second, ref.trans ());

    } else {

      const shape_type *ptr;
      {
        tl::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (ref.obj ());
      }

      m_cache[ref.ptr ()] = ptr;
      return Ref (ptr, ref.trans ());

    }
  }

  template <class Trans>
  Ref operator() (const Ref &ref, const Trans &tr) const
  {
    shape_type sh = ref.obj ().transformed (tr * Trans (ref.trans ()));
    ref_trans_type red_trans;
    sh.reduce (red_trans);

    typename std::unordered_map<shape_type, const shape_type *>::const_iterator m = m_cache_by_shape.find (sh);
    if (m != m_cache_by_shape.end ()) {

      return Ref (m->second, red_trans);

    } else {

      const shape_type *ptr;
      {
        tl::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (sh);
      }

      m_cache_by_shape[sh] = ptr;
      return Ref (ptr, red_trans);

    }
  }

private:
  db::Layout *mp_layout;
  mutable std::unordered_map<const shape_type *, const shape_type *> m_cache;
  mutable std::unordered_map<shape_type, const shape_type *> m_cache_by_shape;
};

template <class Shape>
class simple_shape_reference_translator
{
public:
  typedef Shape shape_type;

  simple_shape_reference_translator ()
  {
    //  .. nothing yet ..
  }

  const shape_type &operator() (const shape_type &s) const
  {
    return s;
  }

  template <class Trans>
  shape_type operator() (const shape_type &s, const Trans &tr) const
  {
    return s.transformed (tr);
  }
};

template <>
class shape_reference_translator<db::Edge>
  : public simple_shape_reference_translator<db::Edge>
{
public:
  shape_reference_translator (db::Layout * /*target_layout*/) { }
};

template <>
class shape_reference_translator<db::Polygon>
  : public simple_shape_reference_translator<db::Polygon>
{
public:
  shape_reference_translator (db::Layout * /*target_layout*/) { }
};

template <>
class shape_reference_translator<db::Text>
  : public simple_shape_reference_translator<db::Text>
{
public:
  shape_reference_translator (db::Layout * /*target_layout*/) { }
};

template<class Basic>
class shape_reference_translator<db::object_with_properties<Basic> >
  : public shape_reference_translator<Basic>
{
public:
  typedef db::object_with_properties<Basic> shape_type;

  shape_reference_translator (db::Layout *target_layout)
    : shape_reference_translator<Basic> (target_layout)
  {
    //  .. nothing yet ..
  }

  shape_type operator() (const shape_type &s) const
  {
    return shape_type (shape_reference_translator<Basic>::operator () (s), s.properties_id ());
  }

  template <class Trans>
  shape_type operator() (const shape_type &s, const Trans &tr) const
  {
    return shape_type (shape_reference_translator<Basic>::operator () (s, tr), s.properties_id ());
  }
};

template <class Ref, class Trans>
class shape_reference_translator_with_trans_from_shape_ref
{
public:
  typedef typename Ref::shape_type shape_type;
  typedef typename Ref::trans_type ref_trans_type;

  shape_reference_translator_with_trans_from_shape_ref (db::Layout *target_layout)
    : mp_layout (target_layout)
  {
    //  .. nothing yet ..
  }

  void set_trans (const Trans &trans)
  {
    m_trans = trans;
    m_ref_trans = ref_trans_type (trans);
    m_bare_trans = Trans (m_ref_trans.inverted ()) * trans;
  }

  Ref operator() (const Ref &ref) const
  {
    auto m = m_cache.find (std::make_pair (ref.ptr (), m_bare_trans));
    if (m != m_cache.end ()) {

      return Ref (m->second.first, ref_trans_type (m_trans * Trans (ref.trans ())) * m->second.second);

    } else {

      shape_type sh = ref.obj ().transformed (m_bare_trans);
      ref_trans_type red_trans;
      sh.reduce (red_trans);

      const shape_type *ptr;
      {
        tl::MutexLocker locker (&mp_layout->lock ());
        ptr = mp_layout->shape_repository ().repository (typename shape_type::tag ()).insert (sh);
      }

      m_cache[std::make_pair (ref.ptr (), m_bare_trans)] = std::make_pair (ptr, red_trans);

      return Ref (ptr, ref_trans_type (m_trans * Trans (ref.trans ())) * red_trans);

    }
  }

private:
  db::Layout *mp_layout;
  Trans m_trans;
  ref_trans_type m_ref_trans;
  Trans m_bare_trans;
  mutable std::unordered_map<std::pair<const shape_type *, Trans>, std::pair<const shape_type *, ref_trans_type> > m_cache;
};

template <class Trans>
class shape_reference_translator_with_trans<db::PolygonRef, Trans>
  : public shape_reference_translator_with_trans_from_shape_ref<db::PolygonRef, Trans>
{
public:
  shape_reference_translator_with_trans (db::Layout *target_layout)
    : shape_reference_translator_with_trans_from_shape_ref<db::PolygonRef, Trans> (target_layout)
  {
    //  .. nothing yet ..
  }
};

template <class Trans>
class shape_reference_translator_with_trans<db::TextRef, Trans>
  : public shape_reference_translator_with_trans_from_shape_ref<db::TextRef, Trans>
{
public:
  shape_reference_translator_with_trans (db::Layout *target_layout)
    : shape_reference_translator_with_trans_from_shape_ref<db::TextRef, Trans> (target_layout)
  {
    //  .. nothing yet ..
  }
};

template <class Sh, class Trans>
class shape_reference_translator_with_trans
{
public:
  typedef Sh shape_type;

  shape_reference_translator_with_trans (db::Layout * /*target_layout*/)
  {
    //  .. nothing yet ..
  }

  void set_trans (const Trans &trans)
  {
    m_trans = trans;
  }

  shape_type operator() (const shape_type &s) const
  {
    return s.transformed (m_trans);
  }

private:
  Trans m_trans;
};

template <class Basic, class Trans>
class shape_reference_translator_with_trans<db::object_with_properties<Basic>, Trans>
  : public shape_reference_translator_with_trans<Basic, Trans>
{
public:
  typedef db::object_with_properties<Basic> shape_type;

  shape_reference_translator_with_trans (db::Layout *target_layout)
    : shape_reference_translator_with_trans<Basic, Trans> (target_layout)
  {
    //  .. nothing yet ..
  }

  shape_type operator() (const shape_type &s) const
  {
    //  CAUTION: no property ID translation happens here (reasoning: the main use case is fake ID for net tagging)
    return shape_type (shape_reference_translator_with_trans<Basic, Trans>::operator () (s), s.properties_id ());
  }
};

}

#endif

