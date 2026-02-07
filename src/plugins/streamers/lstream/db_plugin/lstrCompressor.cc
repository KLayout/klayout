
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

#include "lstrCompressor.h"

#include "dbPluginCommon.h"
#include "dbHash.h"

namespace lstr
{

// ---------------------------------------------------------------------------------
//  Utilities that prevent signed coordinate overflow

template <class R>
inline R safe_scale (double sf, R value)
{
  double i = floor (sf * value + 0.5);
  if (i < double (std::numeric_limits<R>::min ())) {
    throw tl::Exception ("Scaling failed: coordinate underflow");
  }
  if (i > double (std::numeric_limits<R>::max ())) {
    throw tl::Exception ("Scaling failed: coordinate overflow");
  }
  return R (i);
}

template <class R>
inline R safe_diff (R a, R b)
{
  R d = a - b;
  if ((a > b && d < 0) || (a < b && d > 0)) {
    throw tl::Exception ("Signed coordinate difference overflow");
  }
  return d;
}

// ---------------------------------------------------------------------------------

template <class Obj>
void 
Compressor<Obj>::flush (CompressorDelivery *writer) 
{
  //  produce the repetitions

  std::vector<db::Vector> empty_irregular;
  RegularArray empty_regular;
  
  disp_vector displacements;
  typedef std::vector <std::pair <db::Vector, std::pair <db::Coord, int> > > tmp_rep_vector;
  tmp_rep_vector repetitions;
  std::vector<std::pair<db::Vector, RegularArray> > rep_vector;

  for (auto n = m_normalized.begin (); n != m_normalized.end (); ++n) {

    rep_vector.clear ();

    //  don't compress below a threshold of 10 shapes
    if (m_level < 1 || n->second.size () < 10) {

      //  Simple compression: just sort and make irregular repetitions
      std::sort (n->second.begin (), n->second.end (), vector_cmp_x ());

    } else {
    
      disp_vector::iterator d;
      tmp_rep_vector::iterator rw;

      std::unordered_set<db::Coord> xcoords, ycoords;
      if (m_level > 1) {
        for (d = n->second.begin (); d != n->second.end (); ++d) {
          xcoords.insert (d->x ());
          ycoords.insert (d->y ());
        }
      }

      bool xfirst = xcoords.size () < ycoords.size ();

      double simple_rep_cost = 0;
      double array_cost = 0;

      //  Try single-point compression to repetitions in the x and y direction. For the first
      //  direction, use the one with more distinct values. For this, a better compression is 
      //  expected.
      for (int xypass = 0; xypass <= 1; ++xypass) {

        bool xrep = (xfirst == (xypass == 0));
      
        displacements.clear ();
        repetitions.clear ();

        displacements.swap (n->second);
        if (xrep) {
          std::sort (displacements.begin (), displacements.end (), vector_cmp_x ());
        } else {
          std::sort (displacements.begin (), displacements.end (), vector_cmp_y ());
        }

        if (xypass == 0 && m_level > 1) {
          //  Establish a baseline for the repetition cost
          simple_rep_cost += cost_of (displacements.front ().x ()) + cost_of (displacements.front ().y ()); 
          for (d = displacements.begin () + 1; d != displacements.end (); ++d) {
            simple_rep_cost += std::max (1.0, cost_of (double (d->x ()) - double (d[-1].x ())) + cost_of (double (d->y ()) - double (d[-1].y ()))); 
          }
        }

        disp_vector::iterator dwindow = displacements.begin ();
        for (d = displacements.begin (); d != displacements.end (); ) {

          if (m_level < 2) {

            disp_vector::iterator dd = d;
            ++dd;

            db::Vector dxy;
            int nxy = 1;

            if (dd != displacements.end ()) {

              dxy = xrep ? db::Vector (safe_diff (dd->x (), d->x ()), 0) : db::Vector (0, safe_diff (dd->y (), d->y ()));
              while (dd != displacements.end () && *dd == dd[-1] + dxy) {
                ++dd;
                ++nxy;
              } 

            }

            //  Note in level 1 optimization, no cost estimation is done, hence small arrays won't be removed.
            //  To compensate that, we use a minimum size of 3 items per array.
            if (nxy < 3) {

              n->second.push_back (*d++);

            } else {

              repetitions.push_back (std::make_pair (*d, std::make_pair (xrep ? dxy.x () : dxy.y (), nxy)));
              d = dd;

            }

          } else {

            //  collect the nearest neighbor distances and counts for 2..level order neighbors
            int nxy_max = 1;
            unsigned int nn_max = 0;

            //  move the window of identical x/y coordinates if necessary
            if (d == dwindow) {
              for (dwindow = d + 1; dwindow != displacements.end () && (xrep ? (dwindow->y () == d->y ()) : (dwindow->x () == d->x ())); ++dwindow) 
                ;
            }

            for (unsigned int nn = 0; nn < m_level; ++nn) {

              disp_vector::iterator dd = d + (nn + 1);
              if (dd >= dwindow) {
                break;
              }

              db::Vector dxy = xrep ? db::Vector (safe_diff (dd->x (), d->x ()), 0) : db::Vector (0, safe_diff (dd->y (), d->y ()));

              int nxy = 2;
              while (dd != dwindow) {
                disp_vector::iterator df = std::lower_bound (dd + 1, dwindow, *dd + dxy);
                if (df == dwindow || *df != *dd + dxy) {
                  break;
                }
                ++nxy;
                dd = df;
              }

              if (nxy > nxy_max) {
                nxy_max = nxy;
                nn_max = nn;
              }

            }

            if (nxy_max < 2) {

              //  no candidate found - just keep that one
              n->second.push_back (*d++);

            } else {

              //  take out the ones of this sequence from the list
              db::Vector dxy_max = xrep ? db::Vector (safe_diff ((d + nn_max + 1)->x (), d->x ()), 0) : db::Vector (0, safe_diff ((d + nn_max + 1)->y (), d->y ()));

              disp_vector::iterator ds = dwindow;
              disp_vector::iterator dt = dwindow;
              db::Vector df = *d + dxy_max * long (nxy_max - 1);

              do {
                --ds;
                if (*ds != df) {
                  *--dt = *ds;
                } else {
                  df -= dxy_max;
                }
              } while (ds != d);

              repetitions.push_back (std::make_pair (*d, std::make_pair (xrep ? dxy_max.x () : dxy_max.y (), nxy_max)));

              d = dt;

            }

          }

        }

        //  Apply some heuristic criterion that allows the algorithm to determine whether it's worth doing the compression

        //  Try to compact these repetitions further, y direction first, then x direction
        for (int xypass2 = 1; xypass2 >= 0; --xypass2) {
        
          if (xypass2) {
            std::sort (repetitions.begin (), repetitions.end (), rep_vector_cmp<vector_cmp_y> ());
          } else {
            std::sort (repetitions.begin (), repetitions.end (), rep_vector_cmp<vector_cmp_x> ());
          }

          rw = repetitions.begin ();
          for (auto r = repetitions.begin (); r != repetitions.end (); ) {

            auto rr = r;
            ++rr;

            db::Vector dxy2;
            if (rr != repetitions.end ()) {
              dxy2 = xypass2 ? db::Vector (0, safe_diff (rr->first.y (), r->first.y ())) : db::Vector (safe_diff (rr->first.x (), r->first.x ()), 0);
            }
            int nxy2 = 1;

            db::Vector dxy2n (dxy2);
            while (rr != repetitions.end () && rr->second == r->second && rr->first == r->first + dxy2n) {
              ++nxy2;
              ++rr;
              dxy2n += dxy2;
            }

            if (nxy2 < 2 && xypass2) {
              *rw++ = *r;
            } else {
              if (m_level < 2) {
                Obj obj = n->first;
                obj.move (r->first);
                db::Vector a (xrep ? r->second.first : 0, xrep ? 0 : r->second.first);
                writer->write (obj, RegularArray (a, dxy2, r->second.second, nxy2), empty_irregular);
              } else {
                db::Vector a (xrep ? r->second.first : 0, xrep ? 0 : r->second.first);
                rep_vector.push_back (std::make_pair (r->first, RegularArray (a, dxy2, r->second.second, nxy2)));
              }
            }

            r = rr;

          }

          repetitions.erase (rw, repetitions.end ());

        }

      }

      if (m_level > 1) {

        //  Compute a cost for the repetitions

        if (! n->second.empty ()) {
          //  irregular repetition contribution
          array_cost += cost_of (n->second.front ().x ()) + cost_of (n->second.front ().y ()); 
          for (auto d = n->second.begin () + 1; d != n->second.end (); ++d) {
            array_cost += std::max(1.0, cost_of (d->x () - d[-1].x ()) + cost_of (d->y () - d[-1].y ())); 
          }
        }

        bool array_set = false;
        db::Vector a_ref, b_ref;
        size_t in_ref = 0, im_ref = 0;
        bool ref_set = false;
        db::Coord x_ref = 0, y_ref = 0;

        for (auto r = rep_vector.begin (); r != rep_vector.end (); ++r) {

          db::Vector a, b;
          size_t in = 0, im = 0;

          array_cost += 2; // two bytes for the shape

          //  The cost of the first point (takes into account compression by reuse of one coordinate)
          if (!ref_set || x_ref != r->first.x ()) {
            array_cost += cost_of (r->first.x ());
          }
          if (!ref_set || y_ref != r->first.y ()) {
            array_cost += cost_of (r->first.y ());
          }
          ref_set = true;
          x_ref = r->first.x ();
          y_ref = r->first.y ();

          //  Cost of the repetition (takes into account reuse)
          if (! array_set || a != a_ref || b != b_ref || in != in_ref || im != im_ref) {
            array_set = true;
            a_ref = a;
            b_ref = b;
            in_ref = in;
            im_ref = im;
            array_cost += cost_of (a.x ()) + cost_of (b.x ()) + cost_of (a.y ()) + cost_of (b.y ()) + cost_of (in) + cost_of (im);
          } else {
            array_cost += 1; // one byte
          }

          //  Note: the pointlist is reused, hence does not contribute

        }

        //  And resolve the repetitions if it does not make sense to keep them
        if (array_cost > simple_rep_cost) {
          for (auto r = rep_vector.begin (); r != rep_vector.end (); ++r) {
            for (size_t ia = 0; ia < r->second.na (); ++ia) {
              for (size_t ib = 0; ib < r->second.nb (); ++ib) {
                n->second.push_back (r->first + r->second.a () * long (ia) + r->second.b () * long (ib));
              }
            }
          }
          rep_vector.clear ();
          std::sort (n->second.begin (), n->second.end (), vector_cmp_x ());
        }

      }

    }

    for (auto r = rep_vector.begin (); r != rep_vector.end (); ++r) {
      Obj obj = n->first;
      obj.move (r->first);
      writer->write (obj, r->second, empty_irregular);
    }

    if (n->second.size () > 1) {

      //  need to normalize?
      db::Vector p0 = n->second.front ();
      std::vector<db::Vector>::iterator pw = n->second.begin();
      for (auto p = pw + 1; p != n->second.end (); ++p) {
        *pw++ = *p - p0;
      }
      n->second.erase (pw, n->second.end ());

      Obj obj = n->first;
      obj.move (p0);
      writer->write (obj, empty_regular, n->second);

    } else if (! n->second.empty ()) {

      Obj obj = n->first;
      obj.move (n->second.front ());
      writer->write (obj, empty_regular, empty_irregular);

    }

  }
}

//  explicit instantiations
template class Compressor<db::Point>;
template class Compressor<db::PointWithProperties>;
template class Compressor<db::Box>;
template class Compressor<db::BoxWithProperties>;
template class Compressor<db::Edge>;
template class Compressor<db::EdgeWithProperties>;
template class Compressor<db::EdgePair>;
template class Compressor<db::EdgePairWithProperties>;
template class Compressor<db::Polygon>;
template class Compressor<db::PolygonWithProperties>;
template class Compressor<db::SimplePolygon>;
template class Compressor<db::SimplePolygonWithProperties>;
template class Compressor<db::Text>;
template class Compressor<db::TextWithProperties>;
template class Compressor<db::Path>;
template class Compressor<db::PathWithProperties>;
template class Compressor<db::CellInstArray>;
template class Compressor<db::CellInstArrayWithProperties>;

}
