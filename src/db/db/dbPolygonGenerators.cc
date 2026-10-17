
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



#include "dbPolygonGenerators.h"
#include "tlTimer.h"

#include <vector>
#include <deque>
#include <memory>

//  for sanity checks:
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbRegionProcessors.h"
#include "dbEdgesUtils.h"
#include "tlIntervalMap.h"

//  enables verbose debug output
//  #define DEBUG_POLYGON_GENERATOR

//  enables the enhanced stitching algorithm that avoids producing too many
//  overlapping edges during the stitch line generation
#define ENHANCED_STITCHING

//  enables sanity checks after generation of polygons
//  #define SANITY_CHECK

namespace db
{

// -------------------------------------------------------------------------------
//  Some test functions

template <class Poly>
static bool check_polygon_for_intersecting_edges (const Poly &p)
{
  db::Region rp;
  rp.set_merged_semantics (false);
  rp.insert (p);
  db::Edges ec = rp.processed (db::CornersAsDots (-180.0, true, 180.0, true, false, false));

  db::Edges e (p);
  e.set_merged_semantics (false);
  db::EdgeOrientationFilter h (0.0, false, false);
  db::EdgeOrientationFilter nh (0.0, true, false);
  db::Edges ei = e.filtered (h).intersections (e.filtered (nh));

  db::Edges crossings = ei - ec;
  if (! crossings.empty ()) {
    tl::info << "Found intersecting horizontal lines in polygon at points:";
    tl::info << "  " << crossings.to_string ();
    return false;
  } else {
    return true;
  }
}

namespace {
  struct AddOverlapCount
  {
    void operator () (int &a, int b) const
    {
      a += b;
    }
  };
}

template <class Poly>
static int max_edge_overlap_at_y (const Poly &p, db::Coord y)
{
  tl::interval_map<db::Coord, int> im;

  for (auto e = p.begin_edge (); ! e.at_end (); ++e) {
    if ((*e).p1 ().y () == y && (*e).p2 ().y () == y) {
      AddOverlapCount aoc;
      im.add (std::min ((*e).p1 ().x (), (*e).p2 ().x ()), std::max ((*e).p1 ().x (), (*e).p2 ().x ()), 1, aoc);
    }
  }

  int max_count = 0;
  for (auto i = im.begin (); i != im.end (); ++i) {
    if (i->second > 2) {
      tl::info << "Edge overlap count at y=" << y << " x=" << i->first.first << ".." << i->first.second << " is " << i->second;
    }
    if (i->second > max_count) {
      max_count = i->second;
    }
  }

  return max_count;
}

template <class Poly>
static int max_horizontal_edge_overlap (const Poly &poly)
{
  std::set<db::Coord> hy;
  for (auto e = poly.begin_edge (); ! e.at_end (); ++e) {
    if ((*e).dy () == 0) {
      hy.insert ((*e).p1 ().y ());
    }
  }

  int mc = 0;
  for (auto y = hy.begin (); y != hy.end (); ++y) {
    mc = std::max (mc, max_edge_overlap_at_y (poly, *y));
  }
  return mc;
}

// -------------------------------------------------------------------------------
//  PGPoint class

struct PGPoint
{
  PGPoint () : point (), contour (0), hook (false), first (false) { }
  PGPoint (const db::Point &p, size_t i, bool f) : point (p), contour (i), hook (false), first (f) { }

  bool operator== (const PGPoint &other) const
  {
    return point == other.point && contour == other.contour && hook == other.hook;
  }

  db::Point point;
  size_t contour;
  bool hook;
  bool first;
};

// -------------------------------------------------------------------------------
//  PGPolyContour class

class PGPolyContour
{
public:
  typedef std::list <db::Point> contour_type;
  typedef contour_type::const_iterator iterator;
  typedef std::list <PGPoint> open_map_type;
  typedef open_map_type::iterator open_map_iterator_type;

  struct IteratorXCompare
  {
    bool operator() (iterator a, iterator b) const
    {
      return a->x () < b->x ();
    }
  };

  PGPolyContour ()
    : m_is_hole (false),
      m_next (-1), m_prev (-1),
      m_size (0),
      mp_open (0),
      m_contour_index (-1),
      m_y_cached (std::numeric_limits<db::Coord>::min ())
  {
    //  .. nothing yet ..
  }

  std::string to_string (iterator sp = iterator ())
  {
    std::string res = "c";
    res += tl::to_string (contour_index ());
    if (is_hole ()) {
      res += "H";
    }
    res += ": ";
    for (auto i = begin (); i != end (); ++i) {
      res += i->to_string ();
      if (i == sp) {
        res += "#";
      }
      res += " ";
    }
    res += " first->" + tl::to_string (first_pt ()->contour) + ":" + first_pt ()->point.to_string ();
    res += " last->" + tl::to_string (last_pt ()->contour) + ":" + last_pt ()->point.to_string ();
    return res;
  }

  void set_map (open_map_type *map)
  {
    if (map == mp_open) {
      return;
    }

    if (mp_open) {
      remove_all_pointers ();
    }
    mp_open = map;
  }

  void set_contour_index (int ci) { m_contour_index = ci; }
  int contour_index () const { return m_contour_index; }

  /**
   *  @brief Returns a good stitch point inserting a hole position x (left coordinate)
   *
   *  @param x The right fence - only points left or equal to that value are reported
   *  @param y The scan line y
   *  @return An iterator pointing past a suitable stitch point or end() if no such point can be found
   *
   *  Suitable stitch points are ones for which
   *  - the x position where the incoming edge crosses the y line is left (or equal to) the x input
   *  - this x position is closest to the given x value (x value is maximal)
   *  - the incoming edge is pointing vertically up (is not horizontal)
   *
   *  Note that only the first and last point of a contour may be >y.
   */
  iterator stitch_point (db::Coord x, db::Coord y)
  {
    iterator s1 = stitch_point_cached (x, y);
#if defined(SANITY_CHECK)
    iterator s2 = stitch_point_uncached (x, y);
    if (s1 != s2) {
      tl::info << "Cache consistency problem on x=" << x << ", y=" << y << " - details follow.";
      tl::info << "  Problem contour: " << contour_index ();
      tl::info << "  Contour dump:" << tl::endl << "  " << to_string ();
      tl::info << "  Cached position: " << std::distance (begin (), s1);
      tl::info << "  Uncached position: " << std::distance (begin (), s2);
      check_cache_values (true);
    }
    tl_assert (s1 == s2);
#endif
    return s1;
  }

  iterator stitch_point_cached (db::Coord x, db::Coord y)
  {
    if (m_y_cached != y) {
      invalidate_cache ();
    }
    if (m_y_cached == std::numeric_limits<db::Coord>::min ()) {
      m_y_cached = y;
      update_cache (begin (), end ());
    }

    contour_type tmp;
    tmp.push_back (db::Point (x == std::numeric_limits<db::Coord>::max () ? x : x + 1, y));

    auto sp = end ();
    db::Coord xmax = std::numeric_limits<db::Coord>::min ();

    //  find in cache
    auto ic = m_sp_cache.lower_bound (tmp.begin ());
    if (ic != m_sp_cache.begin ()) {
      sp = *--ic;
      xmax = sp->x ();
      //  the stitch point if defined one-after
      ++sp;
    }

    //  test end edge: the end edge is special because it may be above
    //  the scanline and needs x interpolation. Als, it may be part of
    //  a direct hole/hull connection, hence we need to consider it when
    //  last.x(maybe interpolated) == x too.
    iterator pm1 = --end ();
    iterator p = pm1--;

    db::Coord xpos_last;
    if (p->y () >= y) {
      db::Edge e = db::Edge (*pm1, *p);
      xpos_last = db::coord_traits<db::Coord>::rounded (edge_xaty (e, y));
      if (xpos_last <= x && xpos_last >= xmax) {
        sp = p;
        //  the stitch point if defined one-after
        ++sp;
      }
    }

    return sp;
  }

#if defined(SANITY_CHECK)
  iterator stitch_point_uncached (db::Coord x, db::Coord y)
  {
    db::Coord xmax = std::numeric_limits<db::Coord>::min ();
    iterator sp = --end ();

    //  Test end edge
    iterator pm1 = --end ();
    iterator p = pm1--;

    db::Coord xpos;
    if (p->y () > y) {
      db::Edge e = db::Edge (*pm1, *p);
      xpos = db::coord_traits<db::Coord>::rounded (edge_xaty (e, y));
    } else {
      xpos = p->x ();
    }
    if (xpos <= x) {
      xmax = xpos;
    }

    iterator from = ++begin ();
    iterator to = --end ();

    pm1 = from;
    --pm1;
    for (p = from; p != to; ++p, ++pm1) {
      if (pm1->y () < y && p->y () == y && p->x () <= x && p->x () > xmax) {
        sp = p;
        xmax = p->x ();
      }
    }

    //  the stitch point if defined one-after
    return ++sp;
  }

  std::set<iterator, IteratorXCompare> cache_values (db::Coord y) const
  {
    std::set<iterator, IteratorXCompare> cache;

    iterator from = ++begin ();
    iterator to = end ();

    auto pm1 = from;
    --pm1;
    for (auto p = from; p != to; ++p, ++pm1) {
      if (pm1->y () < y && p->y () == y) {
        cache.insert (p);
      }
    }

    return cache;
  }
#endif

  iterator begin () const { return m_contour.begin (); }
  iterator end () const { return m_contour.end (); }
  const db::Point &front () const { return m_contour.front (); }
  db::Point &front () { return m_contour.front (); }
  const db::Point &back () const { return m_contour.back (); }
  db::Point &back () { return m_contour.back (); }

  void push_back (const db::Point &p)
  {
    m_contour.push_back (p);
    ++m_size;

    auto at = end ();
    update_cache (--at, end ());
  }

  void push_front (const db::Point &p)
  {
    m_contour.push_front (p);
    ++m_size;

    auto at = begin ();
    update_cache (begin (), ++at);
  }

  void pop_back ()
  {
    invalidate_cache ();

    m_contour.pop_back ();
    --m_size;
  }

  void pop_front ()
  {
    invalidate_cache ();

    m_contour.pop_front ();
    --m_size;
  }

  iterator erase (iterator i)
  {
    invalidate_cache ();

    --m_size;
    return m_contour.erase (i);
  }

  iterator insert (iterator i, const db::Point &p)
  {
    ++m_size;
    auto from = m_contour.insert (i, p);

    update_cache (from, i);
    return from;
  }

  template <class I>
  iterator insert (iterator at, I from, I to)
  {
    //  NOTE: in some STL m_contour.insert already returns the new iterator
    //  For CentOS7 we compute the result explicitly
    m_size += std::distance (from, to);
    if (at == m_contour.begin ()) {

      auto ins = m_contour.insert (at, from, to);
      update_cache (ins, at);

      return m_contour.begin ();

    } else {

      iterator prev = at;
      --prev;

      auto ins = m_contour.insert (at, from, to);
      update_cache (ins, at);

      return ++prev;

    }
  }

  void replace_point (iterator at, const db::Point &pt)
  {
    db::Point &dest = const_cast<db::Point &> (*at);
    dest = pt;

    iterator n = at;
    ++n;
    update_cache (at, n);
  }

  bool is_closed () const
  {
    return ! empty () && *begin () == *--end ();
  }

  bool empty () const { return m_contour.empty (); }
  size_t size () const { return m_size; }

  void prev (long n)
  {
    m_prev = n;
  }

  long prev () const
  {
    return m_prev;
  }

  void next (long n) 
  {
    m_next = n;
  }

  long next () const
  {
    return m_next;
  }

  bool is_hole () const
  {
    return m_is_hole;
  }

  void is_hole (bool hole)
  {
    m_is_hole = hole;
  }

  void clear ()
  { 
    m_size = 0;
    m_next = -1;
    m_prev = -1;
    m_contour.clear ();
    invalidate_cache ();
  }

  void erase (iterator from, iterator to)
  {
    m_size -= std::distance (from, to);
    m_contour.erase (from, to);

    invalidate_cache ();
  }

  void splice (iterator before, PGPolyContour &contour)
  {
    auto from = contour.begin ();

    m_size += contour.size ();
    contour.m_size = 0;
    m_contour.splice (before, contour.m_contour);

    update_cache (from, before);
  }

  void splice (iterator before, PGPolyContour &&contour)
  {
    auto from = contour.begin ();

    m_size += contour.size ();
    contour.m_size = 0;
    m_contour.splice (before, std::move (contour.m_contour));

    update_cache (from, before);
  }

  void splice (iterator before, PGPolyContour &contour, iterator from, iterator to)
  {
    size_t n = std::distance (from, to);
    m_size += n;
    contour.m_size -= n;
    m_contour.splice (before, contour.m_contour, from, to);

    update_cache (from, before);
  }

  void splice (iterator before, PGPolyContour &&contour, iterator from, iterator to)
  {
    size_t n = std::distance (from, to);
    m_size += n;
    contour.m_size -= n;
    m_contour.splice (before, std::move (contour.m_contour), from, to);

    update_cache (from, before);
  }

  open_map_iterator_type first_pt () const { return m_first_pt; }
  void set_first_pt (const open_map_iterator_type &i) { m_first_pt = i; }
  void reset_first_pt () { m_first_pt = mp_open->end (); }

  open_map_iterator_type last_pt () const { return m_last_pt; }
  void set_last_pt (const open_map_iterator_type &i) { m_last_pt = i; }
  void reset_last_pt () { m_last_pt = mp_open->end (); }

  open_map_iterator_type other_pt (const open_map_iterator_type &i) const
  {
    if (i == m_first_pt) {
      return m_last_pt;
    } else if (i == m_last_pt) {
      return m_first_pt;
    } else {
      tl_assert (false);
    }
  }

  void add_hook (const open_map_iterator_type &i)
  {
    m_hooks.push_back (i);
  }

  void remove_hooks ()
  {
    if (mp_open) {
      for (auto h = m_hooks.begin (); h != m_hooks.end (); ++h) {
        mp_open->erase (*h);
      }
    }
    m_hooks.clear ();
  }

  void remove_all_pointers ()
  {
    if (mp_open) {
      if (m_first_pt != mp_open->end ()) {
        mp_open->erase (m_first_pt);
      }
      if (m_last_pt != mp_open->end ()) {
        mp_open->erase (m_last_pt);
      }
      m_first_pt = mp_open->end ();
      m_last_pt = mp_open->end ();
    }
    remove_hooks ();
  }

  void merge_with (PGPolyContour &other, open_map_iterator_type p1, open_map_iterator_type p2, bool keep_hook)
  {
    tl_assert (contour_index () != other.contour_index ());

    if ((int) p1->contour != contour_index ()) {
      std::swap (p1, p2);
    }
    tl_assert ((int) p1->contour == contour_index ());
    tl_assert ((int) p2->contour == other.contour_index ());

    if (p1 == m_last_pt) {
      m_last_pt = other.other_pt (p2);
      m_last_pt->contour = contour_index();
    } else {
      tl_assert (p1 == m_first_pt);
      m_first_pt = other.other_pt (p2);
      m_first_pt->contour = contour_index ();
    }

    if (keep_hook) {
      m_hooks.push_back (p1);
      m_hooks.push_back (p2);
      p1->hook = true;
      p2->contour = contour_index ();
      p2->hook = true;
    } else {
      mp_open->erase (p1);
      mp_open->erase (p2);
    }

    other.reset_last_pt ();
    other.reset_first_pt ();

    while (! other.m_hooks.empty ()) {
      auto h = other.m_hooks.back ();
      other.m_hooks.pop_back ();
      h->contour = contour_index ();
      m_hooks.push_back (h);
    }
  }

  void integrate (PGPolyContour &other)
  {
    while (! other.m_hooks.empty ()) {
      auto h = other.m_hooks.back ();
      other.m_hooks.pop_back ();
      h->contour = contour_index ();
      m_hooks.push_back (h);
    }

    other.remove_all_pointers ();
  }

#if defined(SANITY_CHECK)
  int check_cache_values (bool always_print = false) const
  {
    if (m_y_cached == std::numeric_limits<db::Coord>::min ()) {
      return -1;
    }

    auto cc = cache_values (m_y_cached);

    //  TODO: care about actual insert position? Does it matter where we stitch into
    //  exactly on the contour?
    auto i = cc.begin ();
    auto j = m_sp_cache.begin ();
    bool values_equal = true;
    while (i != cc.end () && j != m_sp_cache.end () && values_equal) {
      values_equal = (**i++ == **j++);
    }
    if (i != cc.end () || j != m_sp_cache.end ()) {
      values_equal = false;
    }

    if (! values_equal || always_print) {
      tl::info << "m_y_cached=" << m_y_cached;
      tl::info << "cc: " << tl::noendl;
      for (auto i = cc.begin (); i != cc.end (); ++i) {
        tl::info << (*i)->to_string () << " " << tl::noendl;
      }
      tl::info << "";
      tl::info << "m_sp_cache: " << tl::noendl;
      for (auto i = m_sp_cache.begin (); i != m_sp_cache.end (); ++i) {
        tl::info << (*i)->to_string () << " " << tl::noendl;
      }
      tl::info << "";
      if (! values_equal) {
        tl::info << "Cache value check failed.";
        return 0;
      } else {
        tl::info << "Cache value check successful.";
        return 1;
      }
    } else {
      return 1;
    }
  }
#endif

private:
  contour_type m_contour;
  bool m_is_hole;
  long m_next;
  long m_prev;
  size_t m_size;
  open_map_type *mp_open;
  open_map_iterator_type m_first_pt, m_last_pt;
  std::vector<open_map_iterator_type> m_hooks;
  int m_contour_index;
  db::Coord m_y_cached;
  std::set<iterator, IteratorXCompare> m_sp_cache;

  void invalidate_cache ()
  {
    m_y_cached = std::numeric_limits<db::Coord>::min ();
    m_sp_cache.clear ();
  }

  void update_cache (iterator from, iterator to)
  {
    if (m_y_cached == std::numeric_limits<db::Coord>::min ()) {
      return;
    }
    if (from == to) {
      return;
    }

    //  include last element, because we may turn it into a stitch point
    //  while inserting
    if (to != end ()) {
      ++to;
    }

    //  the last element is past our inserted sequence - we may turn
    //  it into a non-stitching point by inserting. So we first remove it.
    auto last = to;
    --last;
    auto lc = m_sp_cache.find (last);
    if (lc != m_sp_cache.end () && *lc == last) {
      m_sp_cache.erase (lc);
    }

    if (from == begin ()) {
      ++from;
    }

    auto ii = from;
    --ii;
    for (auto i = from; i != to; ++i, ++ii) {
      if (i->y () == m_y_cached && ii->y () < m_y_cached) {
        m_sp_cache.insert (i);
      }
    }

#if defined(SANITY_CHECK)
    tl_assert (check_cache_values () != 0);
#endif
  }

  //  no copying
  PGPolyContour (const PGPolyContour &d);
  PGPolyContour &operator= (const PGPolyContour &d);
};

static inline
PGPolyContour::iterator operator+ (PGPolyContour::iterator i, int n)
{
  while (n-- > 0) {
    ++i;
  }
  return i;
}

static inline
PGPolyContour::iterator operator- (PGPolyContour::iterator i, int n)
{
  while (n-- > 0) {
    --i;
  }
  return i;
}

// -------------------------------------------------------------------------------
//  PGContourList class

class PGContourList
{
public:
  PGContourList () 
    : m_free_contours (-1)
  { }

  ~PGContourList ()
  {
    clear ();
  }

  PGPolyContour &operator[] (size_t n) 
  {
    return *m_contours [n];
  }
  
  const PGPolyContour &operator[] (size_t n) const
  {
    return *m_contours [n];
  }
  
  size_t size () const
  {
    return m_contours.size ();
  }

  size_t allocate (PGPolyContour::open_map_type *open_map)
  {
    size_t index;

    if (m_free_contours >= 0) {
      index = m_free_contours;
      m_free_contours = m_contours [m_free_contours]->next ();
      m_contours [index]->next (-1);
    } else {
      index = m_contours.size ();
      m_contours.push_back (new PGPolyContour ());
    }

    m_contours [index]->set_map (open_map);
    m_contours [index]->set_contour_index (index);

    return index;
  }

  void join (size_t n1, size_t n2)
  {
    PGPolyContour &c1 = *m_contours [n1];
    PGPolyContour &c2 = *m_contours [n2];

    if (c1.next () < 0) {
      c1.next (c2.next ());
      c1.prev (c2.prev ());
    } else if (c2.next () >= 0) {
      m_contours [c1.prev ()]->next (c2.next ());
      c1.prev (c2.prev ());
    }

    c2.clear ();
    c2.set_map (0);
    c2.set_contour_index (-1);
    c2.next (m_free_contours);

    m_free_contours = long (n2);
  }

  void free (size_t n)
  {
    PGPolyContour &c = *m_contours [n];

    c.clear ();
    c.set_map (0);
    c.set_contour_index (-1);
    c.next (m_free_contours);
    m_free_contours = long (n);
  }

  void free_all (size_t n)
  {
    long i = long (n);
    do {
      long ii = m_contours [i]->next ();
      free (i);
      i = ii;
    } while (i >= 0);
  }

  void clear ()
  {
    m_free_contours = -1;

    for (auto c = m_contours.begin (); c != m_contours.end (); ++c) {
      delete *c;
    }
    m_contours.clear ();
  }

  void append (size_t what, size_t to)
  {
    m_contours [m_contours [to]->next () < 0 ? long (to) : m_contours [to]->prev ()]->next (long (what));

    long l = m_contours [what]->prev ();
    m_contours [to]->prev (l < 0 ? long (what) : l);
  }

private:
  long m_free_contours;
  std::vector <PGPolyContour *> m_contours;

  //  no copying
  PGContourList (const PGContourList &);
  PGContourList &operator= (const PGContourList &);
};

// -------------------------------------------------------------------------------
//  PolygonGenerator implementation

#if defined(DEBUG_POLYGON_GENERATOR)
static std::string open_map_to_string (PolygonGenerator::open_map_type &map, PolygonGenerator::open_map_iterator_type pos = PolygonGenerator::open_map_iterator_type (), PolygonGenerator::open_map_iterator_type n = PolygonGenerator::open_map_iterator_type (), PolygonGenerator::open_map_iterator_type nn = PolygonGenerator::open_map_iterator_type ())
{
  std::string res;
  for (PolygonGenerator::open_map_iterator_type i = map.begin (); i != map.end (); ++i) {
    res += tl::to_string (i->contour);
    res += ":";
    res += i->point.to_string ();
    if (i->hook) {
      res += "~";
    }
    if (i->first) {
      res += "!";
    }
    if (i == pos) {
      res += "*";
    }
    if (i == n) {
      res += "+";
    }
    if (i == nn) {
      res += "#";
    }
    res += " ";
  }
  return res;
}
#endif

PolygonGenerator::PolygonGenerator (PolygonSink &psink, bool resolve_holes, bool min_coherence) 
  : EdgeSink (), 
    mp_contours (new PGContourList ()),
    m_y (std::numeric_limits<db::Coord>::min ()),
    m_open_pos (m_open.end ()), 
    mp_psink (&psink),
    mp_spsink (0),
    m_resolve_holes (resolve_holes),
    m_open_contours (false),
    m_min_coherence (min_coherence),
    m_compress (true)
{ 
  //  .. nothing yet ..
}

PolygonGenerator::PolygonGenerator (SimplePolygonSink &spsink, bool min_coherence) 
  : EdgeSink (), 
    mp_contours (new PGContourList ()),
    m_y (std::numeric_limits<db::Coord>::min ()),
    m_open_pos (m_open.end ()), 
    mp_psink (0),
    mp_spsink (&spsink),
    m_resolve_holes (true),
    m_open_contours (false),
    m_min_coherence (min_coherence),
    m_compress (true)
{ 
  //  .. nothing yet ..
}

PolygonGenerator::~PolygonGenerator ()
{
  delete mp_contours;
  mp_contours = 0;
}

void
PolygonGenerator::start ()
{
#if defined(DEBUG_POLYGON_GENERATOR)
  tl::info << "start ()";
#endif
  if (mp_psink) {
    mp_psink->start ();
  }
  if (mp_spsink) {
    mp_spsink->start ();
  }
}

void 
PolygonGenerator::flush ()
{
#if defined(DEBUG_POLYGON_GENERATOR)
  tl::info << "flush() open_list=" << open_map_to_string (m_open, m_open_pos);
#endif
  tl_assert (m_open.empty ());

  mp_contours->clear ();
  m_open.clear ();

  if (mp_psink) {
    mp_psink->flush ();
  }
  if (mp_spsink) {
    mp_spsink->flush ();
  }
}

void 
PolygonGenerator::begin_scanline (db::Coord y)
{
#if defined(DEBUG_POLYGON_GENERATOR)
  tl::info << "begin y=" << y;
#endif

  m_open_pos = m_open.begin ();
  m_y = y;

#if defined(DEBUG_POLYGON_GENERATOR)
  tl::info << "  open_list=" << open_map_to_string (m_open);
  tl::info << "  contours:";
  for (size_t j = 0; j < mp_contours->size (); ++j) {
    auto &c = (*mp_contours)[j];
    if (! c.empty ()) {
      tl::info << "    " << c.to_string ();
    }
  }
#endif
#if defined(SANITY_CHECK)
  for (size_t j = 0; j < mp_contours->size (); ++j) {
    auto &c = (*mp_contours)[j];
    if (! c.empty ()) {
      //  do some checking
      if (c.first_pt () != m_open.end () && c.last_pt () != m_open.end ()) {
        tl_assert (c.first_pt ()->contour == j);
        tl_assert (c.first_pt ()->point == *c.begin ());
        size_t n1 = std::distance (m_open.begin (), c.first_pt ());
        tl_assert (c.last_pt ()->contour == j);
        tl_assert (c.last_pt ()->point == *--c.end ());
        size_t n2 = std::distance (m_open.begin (), c.last_pt ());
        tl_assert (c.is_hole () ? n1 < n2 : n2 < n1);
      } else {
        tl_assert (c.first_pt () == m_open.end () && c.last_pt () == m_open.end ());
      }
    }
  }
#endif
}

void 
PolygonGenerator::end_scanline (db::Coord /*y*/)
{
  join_contours (std::numeric_limits<db::Coord>::max ());

  //  eliminate contour hooks (left for stitching) - those are pairs
  //  of pointers to the same point which sit in between the start
  //  and end point of the contour and serve as hooks for potential stitch lines.
  if (m_resolve_holes) {
    for (size_t j = 0; j < mp_contours->size (); ++j) {
      auto &c = (*mp_contours)[j];
      if (! c.empty ()) {
        c.remove_hooks ();
      }
    }
  }
}

void 
#if defined(DEBUG_POLYGON_GENERATOR)
PolygonGenerator::crossing_edge (const db::Edge &e)
#else
PolygonGenerator::crossing_edge (const db::Edge & /*e*/)
#endif
{
#if defined(DEBUG_POLYGON_GENERATOR)
  tl::info << "xing(" << e.to_string () << ")";
#endif
  join_contours (std::numeric_limits<db::Coord>::max ());
  ++m_open_pos;
}

void 
PolygonGenerator::skip_n (size_t n)
{
  join_contours (std::numeric_limits<db::Coord>::max ());
#if defined(DEBUG_POLYGON_GENERATOR)
  tl::info << "skip(" << n << ")";
#endif
  while (n-- > 0) {
    ++m_open_pos;
  }
}

void 
PolygonGenerator::put (const db::Edge &e)
{
#if defined(DEBUG_POLYGON_GENERATOR)
  tl::info << "put(" << e.to_string() << ") y=" << m_y;
  tl::info << "  open_list(before)=" << open_map_to_string (m_open, m_open_pos);
#endif

  if (m_open_pos != m_open.end ()) {
    db::Coord x;
    if (e.p1 ().y () == m_y && e.p2 ().y () == m_y) {
      x = std::min (e.p1 ().x (), e.p2 ().x ());
    } else if (e.p1 ().y () == m_y) {
      x = e.p1 ().x ();
    } else {
      x = e.p2 ().x ();
    }
    join_contours (x);
  }

  if (m_open_pos != m_open.end () && e.p1 ().y () == m_y && m_open_pos->point == e.p1 () && (! m_min_coherence || e.dy () == 0)) {

    size_t ic = m_open_pos->contour;

    PGPolyContour &c = (*mp_contours) [ic];
    tl_assert (c.back () == e.p1 ());
    c.push_back (e.p2 ());
    m_open_pos->point = e.p2 ();

    if (e.p2 ().y () > m_y) {
      if (m_open_contours) {
#if defined(DEBUG_POLYGON_GENERATOR)
        tl::info << "  eliminating hole";
#endif
        eliminate_hole ();
      }
      ++m_open_pos;
    }

  } else if (m_open_pos != m_open.end () && e.p2 ().y () == m_y && m_open_pos->point == e.p2 () && (m_min_coherence || e.dy () == 0)) {

    size_t ic = m_open_pos->contour;

    PGPolyContour &c = (*mp_contours) [ic];
    tl_assert (c.front () == e.p2 ());
    c.push_front (e.p1 ());
    m_open_pos->point = e.p1 ();

    if (e.p1 ().y () > m_y) {
      if (m_open_contours) {
#if defined(DEBUG_POLYGON_GENERATOR)
        tl::info << "  eliminating hole";
#endif
        eliminate_hole ();
      }
      ++m_open_pos;
    }

  } else {

    bool hole = (e.dy () < 0);

    size_t inew = mp_contours->allocate (&m_open);
    PGPolyContour &cnew = (*mp_contours) [inew];

    cnew.is_hole (hole);
    cnew.push_back (e.p1 ());
    cnew.push_back (e.p2 ());

#if defined(DEBUG_POLYGON_GENERATOR)
    tl::info << "  create " << (hole ? "hole" : "hull") << " " << inew;
#endif

    if (hole) {
      cnew.set_first_pt (m_open.insert (m_open_pos, PGPoint (e.p1 (), inew, true)));
      cnew.set_last_pt (m_open.insert (m_open_pos, PGPoint (e.p2 (), inew, false)));
    } else {
      cnew.set_last_pt (m_open.insert (m_open_pos, PGPoint (e.p2 (), inew, true)));
      cnew.set_first_pt (m_open.insert (m_open_pos, PGPoint (e.p1 (), inew, false)));
    }

    --m_open_pos;

  }

#if defined(DEBUG_POLYGON_GENERATOR)
  tl::info << "  open_list(after)=" << open_map_to_string (m_open, m_open_pos);
  tl::info << "  output contours:";
  for (size_t j = 0; j < mp_contours->size (); ++j) {
    auto &c = (*mp_contours)[j];
    if (! c.empty ()) {
      tl::info << "    " << c.to_string ();
    }
  }
#endif
}

void PolygonGenerator::eliminate_hole ()
{
  if (m_open_pos == m_open.end ()) {
    return;
  }

  size_t ic = m_open_pos->contour;

  PGPolyContour &c = (*mp_contours) [ic];
  if (!c.is_hole () || m_open_pos->first) {
    return;
  }

  //  We found the initial edges of a new hole: connect the (partial) hole with a stitch line to the left.
  //  This way we can turn the hole into a non-hole contour.
  tl_assert (m_open_pos != m_open.begin ());
  --m_open_pos;
  tl_assert (m_open_pos != m_open.begin ());
  --m_open_pos;

  size_t iprev = m_open_pos->contour;
  PGPolyContour &cprev = (*mp_contours) [iprev];

  tl_assert (cprev.size () >= 2);

  //  Compute intersection point with next edge
  db::Edge eprev (*(cprev.end () - 2), cprev.back ());
  db::Coord xprev = db::coord_traits<db::Coord>::rounded (edge_xaty (eprev, m_y));
  db::Point pprev (xprev, m_y);

  //  Build a separate contour that continues the contours to the left of the hole
  std::vector<db::Point> cc;
  cc.push_back (c.front ());
  cc.push_back (*(c.begin () + 1));
  if (pprev != cc.back ()) {
    cc.push_back (pprev);
  }
  if (cprev.back () != cc.back ()) {
    cc.push_back (cprev.back ());
  }

  cprev.back () = pprev;
  while (cprev.size () > 2 && cprev.back ().y () == m_y && (cprev.end () - 2)->y () == m_y && cprev.back ().x () <= (cprev.end () - 2)->x ()) {
    cprev.pop_back ();
  }
  cprev.insert (cprev.end (), c.end () - 2, c.end ());

  c.is_hole (false);
  c.clear ();
  c.insert (c.end (), cc.begin (), cc.end ());

  m_open_pos->contour = ic;
  c.set_last_pt (m_open_pos);
  ++m_open_pos;

  m_open_pos->first = false;
  ++m_open_pos;

  m_open_pos->first = true;
  m_open_pos->contour = iprev;
  cprev.set_last_pt (m_open_pos);
}

void
PolygonGenerator::join_contours (db::Coord x) 
{
  while (m_open_pos != m_open.end ()) {

    open_map_iterator_type n = m_open_pos;
    ++n;
    if (n == m_open.end () || m_open_pos->point.y () != m_y || m_open_pos->point != n->point || m_open_pos->point.x () > x) {
      return;
    }
    
    open_map_iterator_type nn = n;
    open_map_iterator_type next = ++nn;

    bool reordered = false;

    //  don't join, except -/+ pair (for max. coherence) or +/- pair (for min coherence)
    bool minus0 = (m_open_pos->first == (*mp_contours) [m_open_pos->contour].is_hole ());
    bool minus1 = (n->first == (*mp_contours) [n->contour].is_hole ());
    if (! (minus0 == !m_min_coherence && minus1 == m_min_coherence)) {

      //  join a southward pair consisting of the next and second-next edge. 
      if (nn != m_open.end () && m_open_pos->point == nn->point) {
        next = m_open_pos;
        m_open_pos = n;
        n = nn;
        ++nn;
        reordered = true;
      } else if (m_open_pos->point.x () == x) {
        return;
      }

    }

    size_t i1 = m_open_pos->contour;
    size_t i2 = n->contour;
#if defined(DEBUG_POLYGON_GENERATOR)
    tl::info << "  join " << i1 << " and " << i2 << " (x=" << x << ")";
    tl::info << "    open_list(before)=" << open_map_to_string (m_open, m_open_pos, n, nn);
    tl::info << "    input contours:";
    for (size_t j = 0; j < mp_contours->size (); ++j) { 
      auto &c = (*mp_contours)[j];
      if (! c.empty ()) {
        tl::info << "      " << c.to_string ();
      }
    }
#endif

    tl_assert (i1 < mp_contours->size ());
    tl_assert (i2 < mp_contours->size ());

    PGPolyContour &c1 = (*mp_contours) [i1];
    PGPolyContour &c2 = (*mp_contours) [i2];

    if (i1 != i2) {

      tl_assert (! c2.empty ());
      tl_assert (! c1.empty ());

      bool c1_closed = c1.is_closed ();
      bool c2_closed = c2.is_closed ();
      bool c1_hole = c1.is_hole ();
      bool c2_hole = c2.is_hole ();

      //  leave the pair of points as hooks for future stitching - we will clean this up before the next
      //  scanline starts
      bool can_keep_hook = (m_resolve_holes && ! m_open_contours && ! reordered && ! c1_closed && ! c2_closed);
      
      if (m_open_contours && !c1_hole && !c2_hole) {

        //  join with next contour by creating a stitch line
        tl_assert (m_open_pos != m_open.begin ());

        open_map_iterator_type np = m_open_pos;
        --np;

        size_t iprev = np->contour;
        PGPolyContour &cprev = (*mp_contours) [iprev];

        tl_assert (cprev.size () >= 2);

        //  compute intersection point with next edge
        db::Edge eprev (*(cprev.end () - 2), cprev.back ());
        db::Coord xprev = db::coord_traits<db::Coord>::rounded (edge_xaty (eprev, m_y));
        db::Point pprev (xprev, m_y);

        tl_assert (c1.size () >= 2);
        tl_assert (c2.size () >= 2);

        cprev.back () = pprev;
        while (cprev.size () > 2 && cprev.back ().y () == m_y && (cprev.end () - 2)->y () == m_y && cprev.back ().x () <= (cprev.end () - 2)->x ()) {
          cprev.pop_back ();
        }

        if (iprev == i1) {

          if (cprev.front ().y () == m_y && (cprev.begin () + 1)->y () == m_y && cprev.front ().x () >= (cprev.begin () + 1)->x ()) {
            cprev.front () = cprev.back ();
          } else {
            cprev.push_front (cprev.back ());
          }

          produce_poly (cprev);

          cprev.reset_first_pt ();
          cprev.reset_last_pt ();

          mp_contours->free_all (iprev);

        } else {

          tl_assert (c1.first_pt () == m_open_pos);
          tl_assert (cprev.last_pt () == np);

          cprev.splice (cprev.end (), c1);
          cprev.is_hole (false);
          if (! np->first) {
            cprev.first_pt ()->first = m_open_pos->first;
          }

          cprev.set_last_pt (c1.last_pt ());
          cprev.last_pt ()->contour = iprev;
          tl_assert (c1.first_pt () == m_open_pos);

          c1.reset_first_pt ();
          c1.reset_last_pt ();

          //  will destroy c1:
          mp_contours->join (iprev, i1);

        }

        if ((c2.end () - 2)->y () == m_y) {
          c2.back () = pprev;
        } else {
          c2.push_back (pprev);
        }
        c2.push_back (np->point);
        tl_assert (c2.last_pt () == n);
        c2.set_last_pt (np);

        np->contour = i2;
        np->first = n->first;

        m_open.erase (m_open_pos);
        m_open.erase (n);

      } else if (! m_open_pos->first && ! n->first) {

        //  remove c1 from list of contours, join with c2
        if (c2_hole) {
          c1.pop_front ();
          c2.splice (c2.end (), c1);
        } else {
          c1.pop_back ();
          c2.splice (c2.begin (), c1);
        }

        c1.other_pt (m_open_pos)->first = false;

        c2.merge_with (c1, m_open_pos, n, can_keep_hook);
        //  will destroy c1:
        mp_contours->join (i2, i1);

      } else {

        //  remove c1 from list of contours, join with c2
        if (c2_hole) {  // yes! c2 is correct!
          c2.pop_front ();
          c1.splice (c1.end (), c2);
        } else {
          c2.pop_back ();
          c1.splice (c1.begin (), c2);
        }

        if (m_open_pos->first && n->first) {
          c2.other_pt (n)->first = true;
        }

        c1.merge_with (c2, m_open_pos, n, can_keep_hook);
        //  will destroy c2:
        mp_contours->join (i1, i2);

      }

    } else {

      if (! c1.is_hole ()) {

#if defined(DEBUG_POLYGON_GENERATOR)
        tl::info << "    finish " << i1 << " (hull)";
#endif

        produce_poly (c1);

        //  remove this contour plus all associated holes from the contour list
        mp_contours->free_all (i1);

      } else if (m_resolve_holes) {

        //  join with next contour by creating a stitch line
        tl_assert (m_open_pos != m_open.begin ());

        open_map_iterator_type np = m_open_pos;
        do {
          --np;
        } while (np != m_open.begin () && np->contour == m_open_pos->contour);

        size_t iprev = np->contour;
        tl_assert (iprev != m_open_pos->contour);
        PGPolyContour &cprev = (*mp_contours) [iprev];

        tl_assert (cprev.size () >= 2);
        tl_assert (c1.size () >= 2);

        PGPolyContour::iterator ins = cprev.stitch_point (c1.begin ()->x (), m_y);

#if defined(DEBUG_POLYGON_GENERATOR)
        tl::info << "    stitching hole into " << iprev << " at position " << std::distance (cprev.begin (), ins) << " ..";
#endif

        //  insert the cutline at position "ins"

        //  compute intersection point with next edge
        PGPolyContour::iterator insm2 = ins;
        --insm2;
        PGPolyContour::iterator insm1 = insm2--;

        db::Edge eprev = db::Edge (*insm2, *insm1);
        db::Coord xprev;
        if (eprev.dy () != 0) {
          xprev = db::coord_traits<db::Coord>::rounded (edge_xaty (eprev, m_y));
        } else {
          xprev = eprev.p2 ().x ();
        }
        db::Point pprev (xprev, m_y);

        cprev.replace_point (insm1, pprev);

        //  remove collinear edges along the cut line so we create the longest possible stitch line
        //  instead of a fractured one and avoid reflecting edges
        while (insm1 != cprev.begin () && insm2->y () == m_y && insm1->y () == m_y) {
          ins = cprev.erase (insm1);
          insm1 = insm2;
          --insm2;
        }

#if defined(ENHANCED_STITCHING)
        //  insert a redundant point if it is needed to support the outgoing stitch line
        //  to a future next hole to the right
        if (ins != cprev.end () && ins->y () != m_y) {
          ins = cprev.insert (ins, *insm1);
        }

        auto insert_from = c1.begin ();

        //  skip the redundant point that closes the contour
        if (*insert_from == *(c1.end () - 1)) {    // TODO Q: does this test ever fail?
          ++insert_from;
        }

        auto insert_first = c1.end ();
        for (auto i = insert_from; i != c1.end (); ++i) {
          if (i->y () == m_y && i->x () >= xprev && (insert_first == c1.end () || i->x () <= insert_first->x ())) {
            insert_first = i;
          }
        }
        tl_assert (insert_first != c1.end ());

        auto insert_last = insert_first;
        for (auto i = insert_from; i != c1.end (); ++i) {
          if (i->y () == m_y && i->x () > insert_last->x ()) {
            insert_last = i;
          }
        }

        db::Point first = *insert_first;

        //  splice the other contour into this contour
        if (insert_first == insert_from) {
          cprev.splice (ins, std::move (c1), insert_from, c1.end ());
        } else {
          cprev.splice (ins, c1, insert_first, c1.end ());
          cprev.splice (ins, c1, insert_from, c1.end ());
        }

        //  insert a redundant point if it is needed to support the incoming stitch line
        if ((ins - 1)->y () != m_y) {
          ins = cprev.insert (ins, first);
          ++ins;
        }

        //  add an endpoint for the outbound stitch line unless we have one already
        if (ins == cprev.end ()) {
          if (*(ins - 1) != pprev) {
            ins = cprev.insert (ins, pprev);
            ++ins;
          }
          if (eprev.p2 () != pprev) {
            cprev.insert (ins, eprev.p2 ());
          }
        }
#else
        auto insert_from = c1.begin ();

        if ((insert_from + 1)->y () == m_y) {
          //  skip the first point if it closes the contour
          ++insert_from;
        }

        cprev.splice (ins, std::move (c1), insert_from, c1.end ());

        //  add a endpoint for the outbound stitch line unless we have one already
        if (ins == cprev.end ()) {
          ins = cprev.insert (ins, pprev);
          ++ins;
          if (eprev.p2 () != pprev) {
            cprev.insert (ins, eprev.p2 ());
          }
        }
#endif

        cprev.integrate (c1);

        mp_contours->free (i1);

      } else {

        //  join with next contour by inserting as a hole
        tl_assert (nn != m_open.end ());

        c1.remove_all_pointers ();

        mp_contours->append (i1, nn->contour);

      }

    }

    m_open_pos = next;

    if (m_open_pos != m_open.begin () && m_open_contours) {
      --m_open_pos;
      eliminate_hole ();
      ++m_open_pos;
    }

#if defined(DEBUG_POLYGON_GENERATOR)
    tl::info << "    open_list(after)=" << open_map_to_string (m_open, m_open_pos);
    tl::info << "    output contours:";
    for (size_t j = 0; j < mp_contours->size (); ++j) {
      auto &c = (*mp_contours)[j];
      if (! c.empty ()) {
        tl::info << "      " << c.to_string ();
      }
    }
#endif

  }
}

bool
PolygonGenerator::ms_compress = true;

void 
PolygonGenerator::produce_poly (const PGPolyContour &c)
{
  size_t n = 0;
  for (long inext = c.next (); inext >= 0; inext = (*mp_contours) [inext].next ()) {
    ++n;
  }

  bool reduce = m_compress && ms_compress;

  if (mp_psink) {

    PGPolyContour::iterator p0 = c.begin ();
    PGPolyContour::iterator p1 = c.end ();
    // remove duplicate point at end
    tl_assert (p0 != p1);
    --p1;
    tl_assert (*p1 == *p0);

    if (n == 0 && m_poly.holes () == 0) {
      //  fast mode ..
      m_poly.assign_hull (p0, p1, reduce /*compress*/);
    } else {

      m_poly.clear ((unsigned int) n);
      m_poly.assign_hull (p0, p1, reduce /*compress*/);

      for (long inext = c.next (); inext >= 0; inext = (*mp_contours) [inext].next ()) {

        tl_assert ((*mp_contours) [inext].is_hole ());

        PGPolyContour::iterator p0 = (*mp_contours) [inext].begin ();
        PGPolyContour::iterator p1 = (*mp_contours) [inext].end ();
        // remove duplicate point at end
        tl_assert (p0 != p1);
        --p1;
        tl_assert (*p1 == *p0);

        m_poly.insert_hole (p0, p1, reduce /*compress*/);

      }

      m_poly.sort_holes ();

    }

#if defined(SANITY_CHECK)
    if (m_resolve_holes) {

      bool check_polygon_for_intersecting_edges_result = check_polygon_for_intersecting_edges (m_poly);
      int max_horizontal_edge_overlap_result = max_horizontal_edge_overlap (m_poly);
      if (! check_polygon_for_intersecting_edges_result ||
          max_horizontal_edge_overlap_result > 2) {
        tl::info << "Info: polygon is " << m_poly.to_string ();
        tl_assert (check_polygon_for_intersecting_edges_result == true);
        tl_assert (max_horizontal_edge_overlap_result <= 2);
      }

    }
#endif

    mp_psink->put (m_poly);

  } 

  if (mp_spsink) {

    tl_assert (n == 0);  //  there should not be holes since we forced resolve_holes to true ...
    m_spoly.assign_hull (c.begin (), c.end (), reduce /*compress*/);

#if defined(SANITY_CHECK)
    bool check_polygon_for_intersecting_edges_result = check_polygon_for_intersecting_edges (m_spoly);
    int max_horizontal_edge_overlap_result = max_horizontal_edge_overlap (m_spoly);
    if (! check_polygon_for_intersecting_edges_result ||
        max_horizontal_edge_overlap_result > 2) {
      tl::info << "Info: simple polygon is " << m_spoly.to_string ();
      tl_assert (check_polygon_for_intersecting_edges_result == true);
      tl_assert (max_horizontal_edge_overlap_result <= 2);
    }
#endif
    mp_spsink->put (m_spoly);

  } 

}

// -------------------------------------------------------------------------------
//  TrapezoidGenerator implementation

TrapezoidGenerator::TrapezoidGenerator (PolygonSink &psink)
  : EdgeSink (),
    m_y (std::numeric_limits<db::Coord>::min ()),
    mp_psink (&psink),
    mp_spsink (0)
{
  //  .. nothing yet ..
}

TrapezoidGenerator::TrapezoidGenerator (SimplePolygonSink &spsink)
  : EdgeSink (),
    m_y (std::numeric_limits<db::Coord>::min ()),
    mp_psink (0),
    mp_spsink (&spsink)
{
  //  .. nothing yet ..
}

TrapezoidGenerator::~TrapezoidGenerator ()
{
  //  .. nothing yet ..
}

void
TrapezoidGenerator::start ()
{
  if (mp_psink) {
    mp_psink->start ();
  }
  if (mp_spsink) {
    mp_spsink->start ();
  }
}

void
TrapezoidGenerator::flush ()
{
  tl_assert (m_edges.empty ());

  m_edges.clear ();

  if (mp_psink) {
    mp_psink->flush ();
  }
  if (mp_spsink) {
    mp_spsink->flush ();
  }
}

void
TrapezoidGenerator::begin_scanline (db::Coord y)
{
  m_y = y;
  m_current_edge = m_edges.begin ();
  m_new_edges.clear ();
  m_new_edge_refs.clear ();
}

void
TrapezoidGenerator::make_trap (const db::Point (&pts)[4])
{
  if (mp_psink) {
    m_poly.assign_hull (&pts [0], &pts [4], true);
    mp_psink->put (m_poly);
  } else if (mp_spsink) {
    m_spoly.assign_hull (&pts [0], &pts [4], true);
    mp_spsink->put (m_spoly);
  }
}

void
TrapezoidGenerator::end_scanline (db::Coord y)
{
  tl_assert ((m_edges.size () % 2) == 0);
  tl_assert ((m_new_edges.size () % 2) == 0);

  //  create trapezoids for the finished pairs
  std::vector<size_t>::const_iterator rr = m_new_edge_refs.begin ();
  for (edge_map_type_iterator e = m_edges.begin (); e != m_edges.end (); ) {

    edge_map_type_iterator e1 = e;
    ++e;
    tl_assert (e != m_edges.end ());
    edge_map_type_iterator e2 = e;
    ++e;

    size_t r1 = std::numeric_limits<size_t>::max (), r2 = std::numeric_limits<size_t>::max ();
    if (rr != m_new_edge_refs.end ()) {
      r1 = *rr++;
    }
    if (rr != m_new_edge_refs.end ()) {
      r2 = *rr++;
    }

    tl_assert (e1->first.dy () > 0);
    tl_assert (e2->first.dy () < 0);

    if (e1->second.p2 ().y () == y && e2->second.p1 ().y () == y) {

      db::Point pts[4];
      pts[0] = e1->second.p1 ();
      pts[1] = e1->second.p2 ();
      pts[2] = e2->second.p1 ();
      pts[3] = e2->second.p2 ();

      make_trap (pts);

    } else if ((e1->second.p2 ().y () == y && e2->second.p2 ().y () < y) || (e2->second.p1 ().y () == y && e1->second.p1 ().y () < y)) {

      //  Create trapezoid and continue
      db::Point pts[4];
      pts[0] = e1->second.p1 ();
      pts[1] = db::Point (db::coord_traits<db::Coord>::rounded (db::edge_xaty (e1->first, y)), y);
      pts[2] = db::Point (db::coord_traits<db::Coord>::rounded (db::edge_xaty (e2->first, y)), y);
      pts[3] = e2->second.p2 ();

      if (r1 != std::numeric_limits<size_t>::max ()) {
        tl_assert (r1 < m_new_edges.size ());
        m_new_edges[r1].second.set_p1 (pts[1]);
      }
      if (r2 != std::numeric_limits<size_t>::max ()) {
        tl_assert (r2 < m_new_edges.size ());
        m_new_edges[r2].second.set_p2 (pts[2]);
      }

      make_trap (pts);

    }

  }

  for (edge_map_type_iterator e = m_new_edges.begin (); e != m_new_edges.end (); ) {

    edge_map_type_iterator e1 = e;
    ++e;
    tl_assert (e != m_new_edges.end ());
    edge_map_type_iterator e2 = e;
    ++e;

    tl_assert (e1->first.dy () > 0);
    tl_assert (e2->first.dy () < 0);

    if (e1->second.p1 ().y () < y && e2->second.p2 ().y () == y) {

      //  A trapezoid that continues below a hole to the right
      edge_map_type_iterator ee = e2;
      ++ee;
      for ( ; ee != m_new_edges.end (); ++ee) {
        if (ee->second.dy () < 0 && ee->second.p2 ().y () < y) {
          break;
        }
      }

      tl_assert (ee != m_new_edges.end ());

      //  Create trapezoid and continue
      db::Point pts[4];
      pts[0] = e1->second.p1 ();
      pts[1] = db::Point (db::coord_traits<db::Coord>::rounded (db::edge_xaty (e1->first, y)), y);
      pts[2] = db::Point (db::coord_traits<db::Coord>::rounded (db::edge_xaty (ee->first, y)), y);
      pts[3] = ee->second.p2 ();

      e1->second.set_p1 (pts[1]);
      ee->second.set_p2 (pts[2]);

      make_trap (pts);

      e = ee;
      ++e;

    }

  }

  m_new_edges.swap (m_edges);
}

void
TrapezoidGenerator::crossing_edge (const db::Edge &e)
{
  //  ignore horizontal edges
  if (e.dy () == 0) {
    return;
  }

  db::Coord x = db::coord_traits<db::Coord>::rounded (db::edge_xaty (e, m_y));

  //  skip edges which have terminated
  while (m_current_edge != m_edges.end ()) {
    db::Point pc = m_current_edge->second.dy () < 0 ? m_current_edge->second.p1 () : m_current_edge->second.p2 ();
    if (pc.y () == m_y && pc.x () <= x) {
      ++m_current_edge;
      m_new_edge_refs.push_back (std::numeric_limits<size_t>::max ());
    } else {
      break;
    }
  }

  tl_assert (m_current_edge != m_edges.end ());

  m_new_edge_refs.push_back (m_new_edges.size ());
  m_new_edges.push_back (*m_current_edge);
  ++m_current_edge;
}

void
TrapezoidGenerator::skip_n (size_t n)
{
  //  skip those edges that have terminated
  while (m_current_edge != m_edges.end () && db::edge_ymax (m_current_edge->second) == m_y) {
    m_new_edge_refs.push_back (std::numeric_limits<size_t>::max ());
    ++m_current_edge;
  }

  while (n-- > 0) {

    tl_assert (m_current_edge != m_edges.end ());

    m_new_edge_refs.push_back (m_new_edges.size ());
    m_new_edges.push_back (*m_current_edge);
    ++m_current_edge;

  }
}

void
TrapezoidGenerator::put (const db::Edge &e)
{
  db::Coord x;
  if (e.dy () == 0) {
    x = std::max (e.p1 ().x (), e.p2 ().x ());
  } else if (e.dy () < 0) {
    x = e.p2 ().x ();
  } else {
    x = e.p1 ().x ();
  }

  //  skip edges which have terminated
  while (m_current_edge != m_edges.end ()) {
    db::Point pc = m_current_edge->second.dy () < 0 ? m_current_edge->second.p1 () : m_current_edge->second.p2 ();
    if (pc.y () == m_y && pc.x () <= x) {
      ++m_current_edge;
      m_new_edge_refs.push_back (std::numeric_limits<size_t>::max ());
    } else {
      break;
    }
  }

  //  ignore horizontal edges
  if (e.dy () != 0) {
    //  create a new edge entry: the first edge will be the original (used for snapping) and
    //  the second the working edge (used for trapezoid generation)
    m_new_edges.push_back (std::make_pair (e, e));
  }
}

// -------------------------------------------------------------------------------
//  SizingPolygonSink implementation

void 
SizingPolygonFilter::put (const db::Polygon &polygon)
{
  m_sizing_processor.clear ();
  m_sizing_processor.insert (polygon.sized (m_dx, m_dy, m_mode));

  //  merge the resulting polygons to get the true outer contour
  db::SimpleMerge op (1 /*wc>0*/);
  m_sizing_processor.process (*mp_output, op);
}

} // namespace db

