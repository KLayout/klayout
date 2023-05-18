
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



#include "dbPolygonGenerators.h"

#include <vector>
#include <deque>
#include <memory>

#if 0
#define DEBUG_POLYGON_GENERATOR
#endif

namespace db
{

// -------------------------------------------------------------------------------
//  PolygonGenerator implementation

struct PGPoint
{
  PGPoint () : point (), contour (0), first (false) { }
  PGPoint (const db::Point &p, size_t i, bool f) : point (p), contour (i), first (f) { }

  db::Point point;
  size_t contour;
  bool first;
};

class PGPolyContour
{
public:
  typedef std::deque <db::Point> contour_type;
  typedef contour_type::const_iterator const_iterator;
  typedef contour_type::iterator iterator;

  PGPolyContour ()
    : m_is_hole (false), m_next (-1), m_last (-1)
  {
    // ...
  }

  PGPolyContour (const PGPolyContour &d)
    : m_contour (d.m_contour), m_is_hole (d.m_is_hole), m_next (d.m_next), m_last (d.m_last)
  {
    // ...
  }

  PGPolyContour &operator= (const PGPolyContour &d)
  {
    if (this != &d) {
      m_contour = d.m_contour;
      m_is_hole = d.m_is_hole;
      m_next = d.m_next;
      m_last = d.m_last;
    }
    return *this;
  }

  const_iterator begin () const { return m_contour.begin (); }
  const_iterator end () const { return m_contour.end (); }
  iterator begin () { return m_contour.begin (); }
  iterator end () { return m_contour.end (); }
  const db::Point &front () const { return m_contour.front (); }
  db::Point &front () { return m_contour.front (); }
  const db::Point &back () const { return m_contour.back (); }
  db::Point &back () { return m_contour.back (); }
  void push_back (const db::Point &p) { m_contour.push_back (p); }
  void push_front (const db::Point &p) { m_contour.push_front (p); }
  void pop_back () { m_contour.pop_back (); }
  void pop_front () { m_contour.pop_front (); }
  iterator erase (iterator i) { return m_contour.erase (i); }
  iterator insert (iterator i, const db::Point &p) { return m_contour.insert (i, p); }
  bool empty () const { return m_contour.empty (); }
  size_t size () const { return m_contour.size (); }

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
    m_next = -1;
    m_last = -1;
    m_contour.clear ();
  }

  void erase (iterator from, iterator to) 
  {
    m_contour.erase (from, to);
  }

  template <class I>
  iterator insert (iterator at, I from, I to)
  {
    //  NOTE: in some STL m_contour.insert already returns the new iterator
    size_t index_at = at - m_contour.begin ();
    m_contour.insert (at, from, to);
    return m_contour.begin () + index_at;
  }

private:
  contour_type m_contour;
  bool m_is_hole;
  long m_next;
  long m_last;
};


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
#ifdef DEBUG_POLYGON_GENERATOR
  for (open_map_iterator_type i = m_open.begin (); i != m_open.end (); ++i) { 
    printf ("%ld:%s%c%c ", i->contour, i->point.to_string().c_str(), i->first ? '!' : ' ', i == m_open_pos ? '*' : ' ');
  } 
  printf ("\n");
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
#ifdef DEBUG_POLYGON_GENERATOR
  printf ("begin y=%d\n", y);
#endif
  m_open_pos = m_open.begin ();
  m_y = y;

#ifdef DEBUG_POLYGON_GENERATOR
  printf ("m_open=");
  for (open_map_type::const_iterator o = m_open.begin (); o != m_open.end (); ++o) {
    printf ("%ld:%s ", o->contour, o->point.to_string().c_str());
  } 
  printf ("\n");
  printf ("contours:\n"); 
  for (size_t j = 0; j < mp_contours->size (); ++j) { 
    printf ("c%ld%s: ", j, (*mp_contours)[j].is_hole () ? "H" : "");
    for (size_t i = 0; i < (*mp_contours)[j].size (); ++i) { 
      printf ("%s ", ((*mp_contours)[j].begin () + i)->to_string().c_str ()); 
    } 
    printf ("\n"); 
  }
#endif
}

void 
PolygonGenerator::end_scanline (db::Coord /*y*/)
{
  join_contours (std::numeric_limits<db::Coord>::max ());
}

void 
#ifdef DEBUG_POLYGON_GENERATOR
PolygonGenerator::crossing_edge (const db::Edge &e)
#else
PolygonGenerator::crossing_edge (const db::Edge & /*e*/)
#endif
{
#ifdef DEBUG_POLYGON_GENERATOR
  printf ("xing(%s)\n", e.to_string().c_str());
#endif
  join_contours (std::numeric_limits<db::Coord>::max ());
  ++m_open_pos;
}

void 
PolygonGenerator::skip_n (size_t n)
{
  join_contours (std::numeric_limits<db::Coord>::max ());
#ifdef DEBUG_POLYGON_GENERATOR
  printf ("skip(%ld)\n", n);
#endif
  while (n-- > 0) {
    ++m_open_pos;
  }
}

void 
PolygonGenerator::put (const db::Edge &e)
{
#ifdef DEBUG_POLYGON_GENERATOR
  printf ("put(%s) y=%d m_open(%ld)=", e.to_string().c_str(),m_y,std::distance (m_open.begin (), m_open_pos));
  for (open_map_iterator_type i = m_open.begin (); i != m_open.end (); ++i) { 
    printf ("%ld:%s%c%c ", i->contour, i->point.to_string().c_str(), i->first ? '!' : ' ', i == m_open_pos ? '*' : ' ');
  } 
  printf ("\n");
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

#ifdef DEBUG_POLYGON_GENERATOR
    printf ("create %s %ld\n", hole ? "hole" : "hull", inew);
#endif
    m_open.insert (m_open_pos, PGPoint (hole ? e.p1 () : e.p2 (), inew, true));
    m_open.insert (m_open_pos, PGPoint (hole ? e.p2 () : e.p1 (), inew, false));

    --m_open_pos;

  }

#ifdef DEBUG_POLYGON_GENERATOR
  for (open_map_iterator_type i = m_open.begin (); i != m_open.end (); ++i) { 
    printf ("%ld:%s%c%c ", i->contour, i->point.to_string().c_str(), i->first ? '!' : ' ', i == m_open_pos ? '*' : ' ');
  } 
  printf ("\n");
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
  db::Edge eprev (cprev.end ()[-2], cprev.back ());
  db::Coord xprev = db::coord_traits<db::Coord>::rounded (edge_xaty (eprev, m_y));
  db::Point pprev (xprev, m_y);

  //  Build a separate contour that continues the contours to the left of the hole
  PGPolyContour cc = c;
  cc.clear ();

  cc.is_hole (false);
  cc.push_back (c.front ());
  cc.push_back (c.begin ()[1]);
  if (pprev != cc.back ()) {
    cc.push_back (pprev);
  }
  if (cprev.back () != cc.back ()) {
    cc.push_back (cprev.back ());
  }

  cprev.back () = pprev;
  while (cprev.size () > 2 && cprev.back ().y () == m_y && cprev.end ()[-2].y () == m_y && cprev.back ().x () <= cprev.end ()[-2].x ()) {
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
      } else if (m_open_pos->point.x () == x) {
        return;
      }

    }

    size_t i1 = m_open_pos->contour;
    size_t i2 = n->contour;
#ifdef DEBUG_POLYGON_GENERATOR
    printf ("join %ld and %ld\n", i1, i2);
    for (open_map_iterator_type i = m_open.begin (); i != m_open.end (); ++i) { 
      printf ("%ld:%s%c%c%c%c ", i->contour, i->point.to_string().c_str(), i->first ? '!' : ' ', i == m_open_pos ? '*' : ' ', i == n ? '+' : ' ', i == nn ? '#' : ' ');
    } 
    printf ("\n");
    printf ("--> input contours:\n"); 
    for (size_t j = 0; j < mp_contours->size (); ++j) { 
      printf ("--> c%ld%s: ", j, (*mp_contours)[j].is_hole () ? "H" : "");
      for (size_t i = 0; i < (*mp_contours)[j].size (); ++i) { 
        printf ("%s ", ((*mp_contours)[j].begin () + i)->to_string().c_str ()); 
      } 
      printf ("\n"); 
    }
#endif

    tl_assert (i1 < mp_contours->size ());
    tl_assert (i2 < mp_contours->size ());

    PGPolyContour &c1 = (*mp_contours) [i1];
    PGPolyContour &c2 = (*mp_contours) [i2];

    if (i1 != i2) {

      tl_assert (! c2.empty ());
      tl_assert (! c1.empty ());
      
      if (m_open_contours && !c1.is_hole () && !c2.is_hole ()) {

        //  join with next contour by creating a stitch line
        tl_assert (m_open_pos != m_open.begin ());

        open_map_iterator_type np = m_open_pos;
        --np;

        size_t iprev = np->contour;
        PGPolyContour &cprev = (*mp_contours) [iprev];

        tl_assert (cprev.size () >= 2);

        //  compute intersection point with next edge
        db::Edge eprev (cprev.end ()[-2], cprev.back ());
        db::Coord xprev = db::coord_traits<db::Coord>::rounded (edge_xaty (eprev, m_y));
        db::Point pprev (xprev, m_y);

        tl_assert (c1.size () >= 2);
        tl_assert (c2.size () >= 2);

        cprev.back () = pprev;
        while (cprev.size () > 2 && cprev.back ().y () == m_y && cprev.end ()[-2].y () == m_y && cprev.back ().x () <= cprev.end ()[-2].x ()) {
          cprev.pop_back ();
        }

        if (iprev == i1) {

          if (cprev.begin ()->y () == m_y && cprev.begin ()[1].y () == m_y && cprev.front ().x () >= cprev.begin ()[1].x ()) {
            cprev.front () = cprev.back ();
          } else {
            cprev.push_front (cprev.back ());
          }

          produce_poly (cprev);

        } else {

          cprev.insert (cprev.end (), c1.begin (), c1.end ());
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

        //  remove c1 from list of contours, join with c2
        if (c2.is_hole ()) {
          c2.insert (c2.end (), c1.begin () + 1, c1.end ());
        } else {
          c2.insert (c2.begin (), c1.begin (), c1.end () - 1);
        }

        mp_contours->join (i2, i1);

        open_map_iterator_type o = m_open_pos;
        do {
          --o;
        } while (o != m_open.begin () && o->contour != i1);
        tl_assert (o != m_open.begin ());
        o->contour = i2;
        o->first = false;

      } else {

        //  remove c1 from list of contours, join with c2
        if (c2.is_hole ()) {  // yes! c2 is correct!
          c1.insert (c1.end (), c2.begin () + 1, c2.end ());
        } else {
          c1.insert (c1.begin (), c2.begin (), c2.end () - 1);
        }

        mp_contours->join (i1, i2);

        open_map_iterator_type o = n;
        do {
          ++o;
        } while (o != m_open.end () && o->contour != i2);
        tl_assert (o != m_open.end ());
        o->contour = i1;

        if (m_open_pos->first && n->first) {
          o->first = true;
        }

      }

    } else {

      if (! c1.is_hole ()) {

#ifdef DEBUG_POLYGON_GENERATOR
        printf ("finish %ld (hull)\n", i1);
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

        tl_assert (cprev.size () >= 2);
        tl_assert (c1.size () >= 2);

        PGPolyContour::iterator ins = cprev.end ();
        db::Coord xprev = 0;
        db::Edge eprev;

#if 1
        //  shallow analysis: insert the cutline at the end of the sequence - this may
        //  cut lines collinear with contour edges

        eprev = db::Edge (ins[-2], ins[-1]);
        xprev = db::coord_traits<db::Coord>::rounded (edge_xaty (db::Edge (ins[-2], ins[-1]), m_y));
#else
        //  deep analysis: determine insertion point: pick the one where the cutline is shortest

        for (PGPolyContour::iterator i = ins; i > cprev.begin () + 1; --i) {

          db::Edge ecut (i[-2], i[-1]);
          db::Coord xcut = db::coord_traits<db::Coord>::rounded (edge_xaty (db::Edge (i[-2], i[-1]), m_y));

          if (ins == i || (i[-1].y () >= m_y && i[-2].y () < m_y && xcut < c1.back ().x () && xcut > xprev)) {
            xprev = xcut;
            eprev = ecut;
            ins = i;
          }

        }
#endif

        //  compute intersection point with next edge
        db::Point pprev (xprev, m_y);

        //  remove collinear edges along the cut line
        ins[-1] = pprev;
        while (ins - cprev.begin () > 1 && ins[-2].y () == m_y && ins[-1].y () == m_y) {
          ins = cprev.erase (ins - 1);
        }

        if ((c1.begin () + 1)->y () == m_y) {
          ins = cprev.insert (ins, c1.begin () + 1, c1.end ());
          ins += c1.size () - 1;
        } else {
          ins = cprev.insert (ins, c1.begin (), c1.end ());
          ins += c1.size ();
        }

        ins = cprev.insert (ins, pprev);
        ++ins;
        if (eprev.p2 () != pprev) {
          cprev.insert (ins, eprev.p2 ());
        }

        mp_contours->free (i1);

      } else {

        //  join with next contour by inserting as a hole
        tl_assert (nn != m_open.end ());

        mp_contours->append (i1, nn->contour);

      }

    }

    m_open.erase (m_open_pos);
    m_open.erase (n);

    m_open_pos = next;

    if (m_open_pos != m_open.begin () && m_open_contours) {
      --m_open_pos;
      eliminate_hole ();
      ++m_open_pos;
    }

#ifdef DEBUG_POLYGON_GENERATOR
    printf ("--> output contours:\n"); 
    for (size_t j = 0; j < mp_contours->size (); ++j) { 
      printf ("--> c%ld%s: ", j, (*mp_contours)[j].is_hole () ? "H" : "");
      for (size_t i = 0; i < (*mp_contours)[j].size (); ++i) { 
        printf ("%s ", ((*mp_contours)[j].begin () + i)->to_string().c_str ()); 
      } 
      printf ("\n"); 
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

    mp_psink->put (m_poly);

  } 

  if (mp_spsink) {

    tl_assert (n == 0);  //  there should not be holes since we forced resolve_holes to true ...
    m_spoly.assign_hull (c.begin (), c.end (), reduce /*compress*/);

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

