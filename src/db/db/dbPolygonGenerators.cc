
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
#define DEBUG_POLYGON_GENERATOR // @@@

//  enables the enhanced stitching algorithm that avoids producing too many
//  overlapping edges during the stitch line generation
#define ENHANCED_STITCHING

//  enables sanity checks after generation of polygons
//  #define SANITY_CHECK
#define SANITY_CHECK // @@@

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
//  PolygonGenerator implementation

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

class PGPolyContour
{
public:
  typedef std::list <db::Point> contour_type;
  typedef contour_type::const_iterator const_iterator;
  typedef contour_type::iterator iterator;

  PGPolyContour ()
    : m_is_hole (false), m_next (-1), m_last (-1), m_size (0)
  {
    m_stitch_point = m_contour.end ();
  }

  PGPolyContour (const PGPolyContour &d)
    : m_contour (d.m_contour), m_is_hole (d.m_is_hole), m_next (d.m_next), m_last (d.m_last), m_size (d.m_size)
  {
    if (d.m_stitch_point == d.end ()) {
      m_stitch_point = end ();
    } else {
      m_stitch_point = begin ();
      for (auto i = d.begin (); i != d.stitch_point (); ++i) {
        ++m_stitch_point;
      }
    }
  }

  PGPolyContour &operator= (const PGPolyContour &d)
  {
    if (this != &d) {

      m_contour = d.m_contour;
      m_is_hole = d.m_is_hole;
      m_next = d.m_next;
      m_last = d.m_last;
      m_size = d.m_size;

      if (d.m_stitch_point == d.end ()) {
        m_stitch_point = end ();
      } else {
        m_stitch_point = begin ();
        for (auto i = d.begin (); i != d.stitch_point (); ++i) {
          ++m_stitch_point;
        }
      }

    }
    return *this;
  }

  iterator stitch_point () const { return m_stitch_point; }
  void set_stitch_point (iterator sp) { m_stitch_point = sp; }

  /**
   *  @brief Moves the stitch point to a better position if possible
   *  A better position is one closer to the right side.
   *  The stitch point is the position before which a stitch line
   *  for holes is inserted. As holes are inserted from left to right
   *  a position further right is better for short (and non-overlapping)
   *  stitch lines.
   *
   *  The return value is the number of search steps needed for
   *  diagnostics.
   */
  unsigned int adjust_stitch_point (iterator search_from, iterator search_to, db::Coord y, db::Coord position_hint = std::numeric_limits<db::Coord>::max ())
  {
    auto sp = m_stitch_point;
    tl_assert (sp != begin ());
    --sp;

    auto xmax = sp->x ();

    //  if needed, correct the horizontal position for edges extending beyond our current y
    if (sp->y () > y) {
      tl_assert (sp != begin ());
      auto spm1 = sp;
      --spm1;
      db::Edge e = db::Edge (*spm1, *sp);
      xmax = db::coord_traits<db::Coord>::rounded (edge_xaty (e, y));
    }

    unsigned int nsteps = 0;

    auto i = search_to;
    auto ii = i--;
    do {
      ++nsteps;
      --ii;
      --i;
      if (i->y () < y && ii->y () == y) {
        if (ii->x () < position_hint && ii->x () > xmax) {
          sp = ii;
          xmax = sp->x ();
        } else if (ii->x () >= position_hint) {
          break;
        }
      }
    } while (ii != search_from && i != begin ());

    m_stitch_point = ++sp;  //  "++" as it is defined to the be position "after"

    return nsteps;
  }

  const_iterator begin () const { return m_contour.begin (); }
  const_iterator end () const { return m_contour.end (); }
  iterator begin () { return m_contour.begin (); }
  iterator end () { return m_contour.end (); }
  const db::Point &front () const { return m_contour.front (); }
  db::Point &front () { return m_contour.front (); }
  const db::Point &back () const { return m_contour.back (); }
  db::Point &back () { return m_contour.back (); }

  void push_back (const db::Point &p)
  {
    m_contour.push_back (p);
    ++m_size;
  }

  void push_front (const db::Point &p)
  {
    m_contour.push_front (p);
    ++m_size;
  }

  void pop_back ()
  {
    m_contour.pop_back ();
    --m_size;
  }

  void pop_front ()
  {
    m_contour.pop_front ();
    --m_size;
  }

  iterator erase (iterator i)
  {
    --m_size;
    return m_contour.erase (i);
  }

  iterator insert (iterator i, const db::Point &p)
  {
    ++m_size;
    return m_contour.insert (i, p);
  }

  bool is_closed () const
  {
    return ! empty () && *begin () == *--end ();
  }

  bool empty () const { return m_contour.empty (); }
  size_t size () const { return m_size; }

  void last (long n) 
  {
    m_last = n;
  }

  long last () const
  {
    return m_last;
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
    m_last = -1;
    m_contour.clear ();
    m_stitch_point = m_contour.end ();
  }

  void erase (iterator from, iterator to)
  {
    m_size -= std::distance (from, to);
    m_contour.erase (from, to);
  }

  void splice (iterator before, PGPolyContour &contour)
  {
    m_size += contour.size ();
    contour.m_size = 0;
    m_contour.splice (before, contour.m_contour);
  }

  void splice (iterator before, PGPolyContour &&contour)
  {
    m_size += contour.size ();
    contour.m_size = 0;
    m_contour.splice (before, std::move (contour.m_contour));
  }

  void splice (iterator before, PGPolyContour &contour, iterator from, iterator to)
  {
    size_t n = std::distance (from, to);
    m_size += n;
    contour.m_size -= n;
    m_contour.splice (before, contour.m_contour, from, to);
  }

  void splice (iterator before, PGPolyContour &&contour, iterator from, iterator to)
  {
    size_t n = std::distance (from, to);
    m_size += n;
    contour.m_size -= n;
    m_contour.splice (before, std::move (contour.m_contour), from, to);
  }

  template <class I>
  iterator insert (iterator at, I from, I to)
  {
    //  NOTE: in some STL m_contour.insert already returns the new iterator
    //  For CentOS7 we compute the result explicitly
    m_size += std::distance (from, to);
    if (at == m_contour.begin ()) {
      m_contour.insert (at, from, to);
      return m_contour.begin ();
    } else {
      iterator prev = at;
      --prev;
      m_contour.insert (at, from, to);
      return ++prev;
    }
  }

private:
  contour_type m_contour;
  bool m_is_hole;
  long m_next;
  long m_last;
  size_t m_size;
  contour_type::iterator m_stitch_point;
};

static inline
PGPolyContour::const_iterator operator+ (PGPolyContour::const_iterator i, int n)
{
  while (n-- > 0) {
    ++i;
  }
  return i;
}

static inline
PGPolyContour::iterator operator+ (PGPolyContour::iterator i, int n)
{
  while (n-- > 0) {
    ++i;
  }
  return i;
}

static inline
PGPolyContour::const_iterator operator- (PGPolyContour::const_iterator i, int n)
{
  while (n-- > 0) {
    --i;
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

class PGContourList
{
public:
  PGContourList () 
    : m_free_contours (-1)
  { }

  PGPolyContour &operator[] (size_t n) 
  {
    return m_contours [n];
  }
  
  const PGPolyContour &operator[] (size_t n) const
  {
    return m_contours [n];
  }
  
  size_t size () const
  {
    return m_contours.size ();
  }

  size_t allocate ()
  {
    size_t index;

    if (m_free_contours >= 0) {
      index = m_free_contours;
      m_free_contours = m_contours [m_free_contours].next ();
      m_contours [index].next (-1);
    } else {
      index = m_contours.size ();
      m_contours.push_back (PGPolyContour ());
    }

    return index;
  }

  void join (size_t n1, size_t n2)
  {
    PGPolyContour &c1 = m_contours [n1];
    PGPolyContour &c2 = m_contours [n2];

    if (c1.next () < 0) {
      c1.next (c2.next ());
      c1.last (c2.last ());
    } else if (c2.next () >= 0) {
      m_contours [c1.last ()].next (c2.next ());
      c1.last (c2.last ());
    }

    c2.clear ();
    c2.next (m_free_contours);
    m_free_contours = long (n2);
  }

  void free (size_t n)
  {
    PGPolyContour &c = m_contours [n];

    c.clear ();
    c.next (m_free_contours);
    m_free_contours = long (n);
  }

  void free_all (size_t n)
  {
    long i = long (n);
    do {
      long ii = m_contours [i].next ();
      free (i);
      i = ii;
    } while (i >= 0);
  }

  void clear ()
  {
    m_free_contours = -1;
    m_contours.clear ();
  }

  void append (size_t what, size_t to)
  {
    m_contours [m_contours [to].next () < 0 ? long (to) : m_contours [to].last ()].next (long (what));

    long l = m_contours [what].last ();
    m_contours [to].last (l < 0 ? long (what) : l);
  }

private:
  long m_free_contours;
  std::vector <PGPolyContour> m_contours;
};


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
  for (open_map_iterator_type i = m_open.begin (); i != m_open.end (); ++i) { 
    printf ("%ld:%s%s%s ", i->contour, i->point.to_string().c_str(), i->first ? "!" : "", i == m_open_pos ? "*" : "");
  } 
  printf ("\n");
  fflush (stdout);
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
  printf ("begin y=%d\n", y);
  fflush (stdout);
#endif

  m_open_pos = m_open.begin ();
  m_y = y;

  //  reset the stitch point to contour.end() of all contours
  for (size_t j = 0; j < mp_contours->size (); ++j) {
    auto &c = (*mp_contours)[j];
    c.set_stitch_point (c.end ());
  }

#if defined(DEBUG_POLYGON_GENERATOR)
  printf ("  m_open=");
  for (open_map_type::const_iterator o = m_open.begin (); o != m_open.end (); ++o) {
    printf ("%ld:%s%s ", o->contour, o->point.to_string().c_str(), o->hook ? "~" : "");
  } 
  printf ("\n");
  printf ("  contours:\n");
  for (size_t j = 0; j < mp_contours->size (); ++j) { 
    if (! (*mp_contours)[j].empty ()) {
      printf ("    c%ld%s: ", j, (*mp_contours)[j].is_hole () ? "H" : "");
      for (size_t i = 0; i < (*mp_contours)[j].size (); ++i) {
        printf ("%s ", ((*mp_contours)[j].begin () + i)->to_string().c_str ());
      }
      printf ("\n");
    }
  }
  fflush (stdout);
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

    for (auto p = m_open.begin (); p != m_open.end (); ) {
      auto pp = p;
      ++pp;
      if (p->hook && pp != m_open.end () && *p == *pp) {
        m_open.erase (p);
        p = pp;
        ++p;
        m_open.erase (pp);
      } else {
        p = pp;
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
  printf ("xing(%s)\n", e.to_string().c_str());
  fflush (stdout);
#endif
  join_contours (std::numeric_limits<db::Coord>::max ());
  ++m_open_pos;
}

void 
PolygonGenerator::skip_n (size_t n)
{
  join_contours (std::numeric_limits<db::Coord>::max ());
#if defined(DEBUG_POLYGON_GENERATOR)
  printf ("skip(%ld)\n", n);
  fflush (stdout);
#endif
  while (n-- > 0) {
    ++m_open_pos;
  }
}

void 
PolygonGenerator::put (const db::Edge &e)
{
#if defined(DEBUG_POLYGON_GENERATOR)
  printf ("put(%s) y=%d\n  (before) m_open(%ld)=", e.to_string().c_str(), m_y, std::distance (m_open.begin (), m_open_pos));
  for (open_map_iterator_type i = m_open.begin (); i != m_open.end (); ++i) { 
    printf ("%ld:%s%s%s%s ", i->contour, i->point.to_string().c_str(), i->hook ? "~" : "", i->first ? "!" : "", i == m_open_pos ? "*" : "");
  } 
  printf ("\n");
  fflush (stdout);
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
        eliminate_hole ();
      }
      ++m_open_pos;
    }

  } else {

    bool hole = (e.dy () < 0);

    size_t inew = mp_contours->allocate ();
    PGPolyContour &cnew = (*mp_contours) [inew];

    cnew.is_hole (hole);
    cnew.push_back (e.p1 ());
    cnew.push_back (e.p2 ());

#if defined(DEBUG_POLYGON_GENERATOR)
    printf ("  create %s %ld\n", hole ? "hole" : "hull", inew);
    fflush (stdout);
#endif
    m_open.insert (m_open_pos, PGPoint (hole ? e.p1 () : e.p2 (), inew, true));
    m_open.insert (m_open_pos, PGPoint (hole ? e.p2 () : e.p1 (), inew, false));

    --m_open_pos;

  }

#if defined(DEBUG_POLYGON_GENERATOR)
  printf ("  (after) m_open=");
  for (open_map_iterator_type i = m_open.begin (); i != m_open.end (); ++i) { 
    printf ("%ld:%s%s%s ", i->contour, i->point.to_string().c_str(), i->first ? "!" : "", i == m_open_pos ? "*" : "");
  } 
  printf ("\n");
  fflush (stdout);
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
  PGPolyContour cc = c;
  cc.clear ();

  cc.is_hole (false);
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

  c = cc;

  m_open_pos->contour = ic;
  ++m_open_pos;

  m_open_pos->first = false;
  ++m_open_pos;

  m_open_pos->first = true;
  m_open_pos->contour = iprev;
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
    printf ("  join %ld and %ld\n", i1, i2);
    for (open_map_iterator_type i = m_open.begin (); i != m_open.end (); ++i) { 
      printf ("    %ld:%s%s%s%s%s%s ", i->contour, i->point.to_string().c_str(), i->hook ? "~" : "", i->first ? "!" : "", i == m_open_pos ? "*" : "", i == n ? "+" : "", i == nn ? "#" : "");
    } 
    printf ("\n");
    printf ("    input contours:\n");
    for (size_t j = 0; j < mp_contours->size (); ++j) { 
      const auto &c = (*mp_contours)[j];
      if (! c.empty ()) {
        printf ("      c%ld%s: ", j, c.is_hole () ? "H" : "");
        for (auto i = c.begin (); i != c.end (); ++i) {
          printf ("%s%s ", i->to_string().c_str (), i == c.stitch_point () ? "#" : "");
        }
        printf ("\n");
      }
    }
    fflush (stdout);
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

        } else {

          cprev.splice (cprev.end (), c1);
          cprev.is_hole (false);

          mp_contours->join (iprev, i1);

        }

        if ((c2.end () - 2)->y () == m_y) {
          c2.back () = pprev;
        } else {
          c2.push_back (pprev);
        }
        c2.push_back (np->point);

        if (!np->first) {

          open_map_iterator_type o = np;
          while (o != m_open.begin ()) {
            --o;
            if (o->contour == iprev) {
              break;
            }
          }
          tl_assert (o->contour == iprev);
          o->first = m_open_pos->first;

          o = np;
          while (o != m_open.begin ()) {
            --o;
            if (o->contour == i1) {
              break;
            }
          }
          tl_assert (o->contour == i1);
          o->contour = iprev;

        }

        np->contour = i2;
        np->first = n->first;

      } else if (! m_open_pos->first && ! n->first) {

        PGPolyContour::iterator new_from, new_to;

        //  remove c1 from list of contours, join with c2
        if (c2_hole) {
          c1.pop_front ();
          new_from = c1.begin ();
          new_to = c2.end ();
          c2.splice (c2.end (), c1);
          --new_from;
        } else {
          c1.pop_back ();
          new_from = c1.begin ();
          new_to = c2.begin ();
          c2.splice (c2.begin (), c1);
          ++new_to;
        }

        mp_contours->join (i2, i1);

        open_map_iterator_type o = m_open_pos;
        o->contour = i2;
        while (o != m_open.begin () && (o->contour != i1 || o->hook)) {
          if (o->contour == i1) {
            o->contour = i2;
          }
          --o;
        }
        o->contour = i2;
        o->first = false;

#if defined(ENHANCED_STITCHING)
#if defined(DEBUG_POLYGON_GENERATOR)
        unsigned int nsteps = c2.adjust_stitch_point (new_from, new_to, m_y);
        if (nsteps > 0) {
          printf ("    adjusted stitch position to %ld in %u steps\n", std::distance (c2.begin (), c2.stitch_point ()), nsteps);
          fflush (stdout);
        }
#else
        c2.adjust_stitch_point (new_from, new_to, m_y);
#endif
#endif

      } else {

        PGPolyContour::iterator new_from, new_to;

        //  remove c1 from list of contours, join with c2
        if (c2_hole) {  // yes! c2 is correct!
          c2.pop_front ();
          new_from = c2.begin ();
          new_to = c1.end ();
          c1.splice (c1.end (), c2);
          --new_from;
        } else {
          c2.pop_back ();
          new_from = c2.begin ();
          new_to = c1.begin ();
          c1.splice (c1.begin (), c2);
          ++new_to;
        }

        mp_contours->join (i1, i2);

        open_map_iterator_type o = n;
        tl_assert (o != m_open.end ());
        o->contour = i1;
        while (o != m_open.end () && (o->contour != i2 || o->hook)) {
          if (o->contour == i2) {
            o->contour = i1;
          }
          ++o;
        }
        tl_assert (o != m_open.end ());
        o->contour = i1;

        if (m_open_pos->first && n->first) {
          o->first = true;
        }

#if defined(ENHANCED_STITCHING)
#if defined(DEBUG_POLYGON_GENERATOR)
        unsigned int nsteps = c1.adjust_stitch_point (new_from, new_to, m_y);
        if (nsteps > 0) {
          printf ("    adjusted stitch position to %ld in %u steps\n", std::distance (c1.begin (), c1.stitch_point ()), nsteps);
          fflush (stdout);
        }
#else
        c1.adjust_stitch_point (new_from, new_to, m_y);
#endif
#endif

      }

      if (m_resolve_holes && ! reordered && ! c1_hole && ! c2_hole && ! c1_closed && ! c2_closed) {
        //  leave the pair of points as hooks for future stitching - we will clean this up before the next
        //  scanline starts
        m_open_pos->hook = true;
        n->hook = true;
      } else {
        m_open.erase (m_open_pos);
        m_open.erase (n);
      }

    } else {

      if (! c1.is_hole ()) {

#if defined(DEBUG_POLYGON_GENERATOR)
        printf ("    finish %ld (hull)\n", i1);
        fflush (stdout);
#endif

        produce_poly (c1);

        //  remove this contour plus all associated holes from the contour list
        mp_contours->free_all (i1);

      } else if (m_resolve_holes) {

        //  join with next contour by creating a stitch line
        tl_assert (m_open_pos != m_open.begin ());

        open_map_iterator_type np = m_open_pos;
        --np;

        size_t iprev = np->contour;
        PGPolyContour &cprev = (*mp_contours) [iprev];

#if defined(DEBUG_POLYGON_GENERATOR)
        printf ("    stitching hole into %ld ..\n", iprev);
        fflush (stdout);
#endif

        tl_assert (cprev.size () >= 2);
        tl_assert (c1.size () >= 2);

#if 0 // @@@ not needed.
        if (cprev.stitch_point () == cprev.end ()) {
          unsigned int nsteps = cprev.adjust_stitch_point (cprev.begin (), cprev.end (), m_y, x);
#if defined(DEBUG_POLYGON_GENERATOR)
          if (nsteps > 0) {
            printf ("    adjusted stitch position to %ld in %u steps\n", std::distance (cprev.begin (), cprev.stitch_point ()), nsteps);
            fflush (stdout);
          }
#endif
        }
#endif

        PGPolyContour::iterator ins = cprev.stitch_point ();

        //  insert the cutline at the end of the sequence of the previous contour -
        //  this is the left end of that contour.

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

        *insm1 = pprev;

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

#if 0 // @@@
printf("@@@cx1 ");
for (auto i = cprev.begin (); i != cprev.end (); ++i) {
  printf ("%s%s ", i->to_string().c_str (), i == ins ? "#" : "");
}
printf ("\n");
fflush(stdout);
#endif // @@@

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

        //  compute the offset of the new stitch position relative to the end
        //  of the contour piece formed by the cyclic interval [insert_from, insert_from)
        size_t n1 = std::distance (insert_from, insert_first);
        size_t n2 = std::distance (insert_from, insert_last);

        size_t n_after_stitch = 0;
        if (n1 > n2) {
          n_after_stitch = n1 - n2 - 1;
        } else if (n2 > n1) {
          n_after_stitch = c1.size () - std::distance (c1.begin (), insert_from) - n2 + n1 - 1;
        }

        db::Point first = *insert_first;

        //  splice the other contour into this contour
        if (insert_first == insert_from) {
          cprev.splice (ins, std::move (c1), insert_from, c1.end ());
        } else {
          cprev.splice (ins, c1, insert_first, c1.end ());
          cprev.splice (ins, c1, insert_from, c1.end ());
        }

        //  locate the new stitch point where we place stitch lines to next holes
        auto new_stitch_point = ins - n_after_stitch;
#if 0 // @@@
printf("@@@c0 ");
for (auto i = cprev.begin (); i != cprev.end (); ++i) {
  printf ("%s%s ", i->to_string().c_str (), i == new_stitch_point ? "#" : "");
}
printf ("\n");
fflush(stdout);
#endif // @@@

        //  insert a redundant point if it is needed to support the incoming stitch line
        if ((ins - 1)->y () != m_y) {
          ins = cprev.insert (ins, first);
          ++ins;
        }
#if 0 // @@@
printf("@@@c1 ");
for (auto i = cprev.begin (); i != cprev.end (); ++i) {
  printf ("%s%s ", i->to_string().c_str (), i == new_stitch_point ? "#" : "");
}
printf ("\n");
fflush(stdout);
#endif // @@@

        //  add an endpoint for the outbound stitch line unless we have one already
        if (cprev.stitch_point () == cprev.end ()) {
          if (*(ins - 1) != pprev) {
            bool at_sp = ins == new_stitch_point;
            ins = cprev.insert (ins, pprev);
            if (at_sp) {
              new_stitch_point = ins;
            }
            ++ins;
          }
          if (eprev.p2 () != pprev) {
            cprev.insert (ins, eprev.p2 ());
          }
        }
#if 0 // @@@
printf("@@@c3 ");
for (auto i = cprev.begin (); i != cprev.end (); ++i) {
  printf ("%s%s ", i->to_string().c_str (), i == new_stitch_point ? "#" : "");
}
printf ("\n");
fflush(stdout);
#endif // @@@

        cprev.set_stitch_point (new_stitch_point);
#else
        auto insert_from = c1.begin ();

        if ((insert_from + 1)->y () == m_y) {
          //  skip the first point if it closes the contour
          ++insert_from;
        }

        cprev.splice (ins, std::move (c1), insert_from, c1.end ());

        //  add a endpoint for the outbound stitch line unless we have one already
        if (cprev.stitch_point () == cprev.end ()) {
          ins = cprev.insert (ins, pprev);
          cprev.set_stitch_point (ins);
          ++ins;
          if (eprev.p2 () != pprev) {
            cprev.insert (ins, eprev.p2 ());
          }
        }
#endif

        mp_contours->free (i1);

      } else {

        //  join with next contour by inserting as a hole
        tl_assert (nn != m_open.end ());

        mp_contours->append (i1, nn->contour);

      }

      m_open.erase (m_open_pos);
      m_open.erase (n);

    }

    m_open_pos = next;

    if (m_open_pos != m_open.begin () && m_open_contours) {
      --m_open_pos;
      eliminate_hole ();
      ++m_open_pos;
    }

#if defined(DEBUG_POLYGON_GENERATOR)
    printf ("    output contours:\n");
    for (size_t j = 0; j < mp_contours->size (); ++j) {
      const auto &c = (*mp_contours)[j];
      if (! c.empty ()) {
        printf ("      c%ld%s: ", j, c.is_hole () ? "H" : "");
        for (auto i = c.begin (); i != c.end (); ++i) {
          printf ("%s%s ", i->to_string().c_str (), i == c.stitch_point () ? "#" : "");
        }
        printf ("\n");
      }
    }
    fflush (stdout);
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

    PGPolyContour::const_iterator p0 = c.begin ();
    PGPolyContour::const_iterator p1 = c.end ();
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

        PGPolyContour::const_iterator p0 = (*mp_contours) [inext].begin ();
        PGPolyContour::const_iterator p1 = (*mp_contours) [inext].end ();
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

