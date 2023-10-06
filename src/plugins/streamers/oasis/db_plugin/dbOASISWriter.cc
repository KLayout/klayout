
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


#include "dbOASISWriter.h"
#include "dbPolygonGenerators.h"

#include "tlDeflate.h"
#include "tlMath.h"

#include <math.h>

namespace db
{

// ---------------------------------------------------------------------------------
//  Some definitions

static const char *klayout_context_name               = "KLAYOUT_CONTEXT";

static const char *s_gds_property_name                = "S_GDS_PROPERTY";
static const char *s_cell_offset_name                 = "S_CELL_OFFSET";
static const char *s_max_signed_integer_width_name    = "S_MAX_SIGNED_INTEGER_WIDTH";
static const char *s_max_unsigned_integer_width_name  = "S_MAX_UNSIGNED_INTEGER_WIDTH";
static const char *s_top_cell_name                    = "S_TOP_CELL";
static const char *s_bounding_boxes_available_name    = "S_BOUNDING_BOXES_AVAILABLE";
static const char *s_bounding_box_name                = "S_BOUNDING_BOX";

// ---------------------------------------------------------------------------------

/**
 *  @brief Determines whether a property shall be produced as S_GDS_PROPERTY
 */
static bool 
make_gds_property (const tl::Variant &name)
{
  //  We write S_GDS_PROPERTY properties, because that is the only way to write properties
  //  with numerical keys
  return (name.is_longlong () && name.to_longlong () < 0x8000 && name.to_longlong () >= 0) ||
         (name.is_ulonglong () && name.to_ulonglong () < 0x8000) ||
         (name.is_long () && name.to_long () < 0x8000 && name.to_long () >= 0) ||
         (name.is_ulong () && name.to_ulong () < 0x8000);
}

// ---------------------------------------------------------------------------------

/**
 *  @brief Within UTF-8 advance the pointer to the next character
 */

static void 
next_utf8 (const char * &s)
{
  int skip = 0;
  if (((unsigned char) *s) < 0x80) {
    //  single-byte character
  } else if (((unsigned char) *s) < 0xe0) {
    //  two-byte character
    skip = 1;
  } else if (((unsigned char) *s) < 0xf0) {
    //  three-byte character
    skip = 2;
  } else if (((unsigned char) *s) < 0xf8) {
    //  four-byte character
    skip = 3;
  } 

  ++s;
  while (skip > 0 && ((unsigned char) *s) >= 0x80 && ((unsigned char) *s) < 0xc0) {
    ++s;
    --skip;
  }
}

// ---------------------------------------------------------------------------------

/**
 *  @brief Makes an nstring or astring from the given string 
 *  This function employs the substitution string to replace invalid characters.
 *  The substitution string must be a valid nstring itself.
 */
static std::string
make_n_or_astring (const char *s, const std::string &subst, bool make_nstring)
{
  //  Empty strings will render the substitution string when producing nstrings
  if (make_nstring && !*s) {
    return subst;
  }

  bool valid = true;
  for (const char *c = s; *c && valid; ++c) {
    if (*c == 0x20 && make_nstring) {
      valid = false;
    } else if (((unsigned char) *c) < 0x20 || ((unsigned char) *c) > 0x7e) {
      valid = false;
    }
  }

  if (valid) {

    //  No need to translate
    return std::string (s);

  } else {

    std::string nstr;
    for (const char *c = s; *c; ) {
      if (*c == 0x20 && make_nstring) {
        nstr += subst;
      } else if (((unsigned char) *c) < 0x20 || ((unsigned char) *c) > 0x7e) {
        nstr += subst;
      } else {
        nstr += *c;
      }
      next_utf8 (c);
    }

    return nstr;

  }
}

// ---------------------------------------------------------------------------------

/**
 *  @brief Determines the type of a string
 *  The return value is 0 for an a-string, 1 for a b-string and 2 for a n-string.
 *  The return value is determined in a way that the property value type can be
 *  determined by adding 10 or 13 for direct value or reference respectively.
 */
static int 
string_type (const char *s)
{
  if (! *s) {
    //  an empty string gives an a-string
    return 0;
  }

  bool is_nstring = true;

  while (*s) {
    unsigned char c = (unsigned char) *s;
    if (c == 0x20) {
      //  space -> produces a-string instead of n-string 
      is_nstring = false;
    } else if (c < 0x20 || c > 0x7e) {
      //  non-printable character: produces a b-string always
      return 1;
    }
    next_utf8 (s);
  }
      
  return is_nstring ? 2 : 0;
}

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
//  Generic delivery of shapes (specialized to compressing / non-compressing variants)

/**
 *  @brief Convert a shape (basic object) to a repetition
 */
template <class Tag> void create_repetition_by_type (const db::Shape &array_shape, db::Repetition &rep, const Tag &tag) 
{
  const typename Tag::object_type *array = array_shape.basic_ptr (tag);

  std::vector<db::Vector> pts;
  db::Vector a, b;
  unsigned long amax = 0, bmax = 0;

  if (array->is_iterated_array (&pts)) {

    // Remove the first point which is implicitly contained in the repetition
    // Note: we can do so because below we instantiate the shape at the front of the array which includes
    // the first transformation already.
    tl_assert (! pts.empty ());
    db::Vector po = pts.front ();
    std::vector<db::Vector>::iterator pw = pts.begin();
    for (std::vector<db::Vector>::iterator p = pw + 1; p != pts.end (); ++p) {
      *pw++ = *p - po;
    }
    pts.erase (pw, pts.end ());

    db::IrregularRepetition *rep_base = new db::IrregularRepetition ();
    rep_base->points ().swap (pts);
    rep.set_base (rep_base);

  } else if (array->is_regular_array (a, b, amax, bmax)) {

    db::RegularRepetition *rep_base = new db::RegularRepetition (a, b, size_t (std::max ((unsigned long) 1, amax)), size_t (std::max ((unsigned long) 1, bmax)));
    rep.set_base (rep_base);

  } else {
    tl_assert (false);
  }  
}

/**
 *  @brief Produce a repetition from an array shape
 */
void create_repetition (const db::Shape &array, db::Repetition &rep)
{
  switch (array.type ()) {
  case db::Shape::PolygonPtrArray:
    create_repetition_by_type (array, rep, db::Shape::polygon_ptr_array_type::tag ());
    break;
  case db::Shape::SimplePolygonPtrArray:
    create_repetition_by_type (array, rep, db::Shape::simple_polygon_ptr_array_type::tag ());
    break;
  case db::Shape::PathPtrArray:
    create_repetition_by_type (array, rep, db::Shape::path_ptr_array_type::tag ());
    break;
  case db::Shape::BoxArray:
    create_repetition_by_type (array, rep, db::Shape::box_array_type::tag ());
    break;
  case db::Shape::ShortBoxArray:
    create_repetition_by_type (array, rep, db::Shape::short_box_array_type::tag ());
    break;
  case db::Shape::TextPtrArray:
    create_repetition_by_type (array, rep, db::Shape::text_ptr_array_type::tag ());
    break;
  default: 
    tl_assert (false);
    break;
  }
}

/**
 *  @brief Compare operator for points, distinct x clustered (with same y)
 */
struct vector_cmp_x
{
  bool operator() (const db::Vector &a, const db::Vector &b) const
  {
    if (a.y () != b.y ()) {
      return a.y () < b.y ();
    } else {
      return a.x () < b.x ();
    }
  }
};

/**
 *  @brief Compare operator for points, distinct y clustered (with same x)
 */
struct vector_cmp_y
{
  bool operator() (const db::Vector &a, const db::Vector &b) const
  {
    if (a.x () != b.x ()) {
      return a.x () < b.x ();
    } else {
      return a.y () < b.y ();
    }
  }
};

/**
 *  @brief Compare operator for points/abstract repetition pair with configurable point compare operator
 */
template <class PC>
struct rep_vector_cmp
{
  bool operator () (const std::pair <db::Vector, std::pair <db::Coord, int> > &a, const std::pair <db::Vector, std::pair <db::Coord, int> > &b)
  {
    if (a.second != b.second) {
      return a.second < b.second;
    }
    PC pc;
    return pc (a.first, b.first);
  }
};

/**
 *  @brief Return the cost value of a coordinate difference (or coordinate)
 *  The cost is used to estimate the size cost of a coordinate difference 
 *  in the OASIS output.
 *  The cost is roughly the number of bytes required to represent the 
 *  number. It does not consider gdelta compression, actual byte count or similar.
 */
inline double cost_of (double d)
{
  int exp = 0;
  frexp (d, &exp);
  return double ((exp + 7) / 8);
}


template <class Obj>
void 
Compressor<Obj>::flush (db::OASISWriter *writer) 
{
  static const db::Repetition rep_single;

  //  produce the repetitions
  
  disp_vector displacements;
  typedef std::vector <std::pair <db::Vector, std::pair <db::Coord, int> > > tmp_rep_vector;
  tmp_rep_vector repetitions;
  std::vector<std::pair<db::Vector, db::Repetition> > rep_vector;

  for (typename std::unordered_map <Obj, disp_vector>::iterator n = m_normalized.begin (); n != m_normalized.end (); ++n) {

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
          for (tmp_rep_vector::const_iterator r = repetitions.begin (); r != repetitions.end (); ) {

            tmp_rep_vector::const_iterator rr = r;
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
                writer->write (obj, db::Repetition (new RegularRepetition (a, dxy2, r->second.second, nxy2)));
              } else {
                db::Vector a (xrep ? r->second.first : 0, xrep ? 0 : r->second.first);
                rep_vector.push_back (std::make_pair (r->first, db::Repetition (new RegularRepetition (a, dxy2, r->second.second, nxy2))));
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
          for (std::vector<db::Vector>::const_iterator d = n->second.begin () + 1; d != n->second.end (); ++d) {
            array_cost += std::max(1.0, cost_of (d->x () - d[-1].x ()) + cost_of (d->y () - d[-1].y ())); 
          }
        }

        bool array_set = false;
        db::Vector a_ref, b_ref;
        size_t in_ref = 0, im_ref = 0;
        bool ref_set = false;
        db::Coord x_ref = 0, y_ref = 0;

        for (std::vector<std::pair<db::Vector, db::Repetition> >::const_iterator r = rep_vector.begin (); r != rep_vector.end (); ++r) {

          db::Vector a, b;
          size_t in = 0, im = 0;
          tl_assert (r->second.is_regular (a, b, in, im));

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
          for (std::vector<std::pair<db::Vector, db::Repetition> >::const_iterator r = rep_vector.begin (); r != rep_vector.end (); ++r) {
            for (db::RepetitionIterator i = r->second.begin (); ! i.at_end (); ++i) {
              n->second.push_back (r->first + *i);
            }
          }
          rep_vector.clear ();
          std::sort (n->second.begin (), n->second.end (), vector_cmp_x ());
        }

      }

    }

    for (std::vector<std::pair<db::Vector, db::Repetition> >::const_iterator r = rep_vector.begin (); r != rep_vector.end (); ++r) {
      Obj obj = n->first;
      obj.move (r->first);
      writer->write (obj, r->second);
    }

    if (n->second.size () > 1) {

      //  need to normalize?
      db::Vector p0 = n->second.front ();
      std::vector<db::Vector>::iterator pw = n->second.begin();
      for (std::vector<db::Vector>::iterator p = pw + 1; p != n->second.end (); ++p) {
        *pw++ = *p - p0;
      }
      n->second.erase (pw, n->second.end ());
        
      IrregularRepetition *iterated_rep = new IrregularRepetition ();
      iterated_rep->points ().swap (n->second);

      Obj obj = n->first;
      obj.move (p0);
      writer->write (obj, Repetition (iterated_rep));

    } else if (! n->second.empty ()) {

      Obj obj = n->first;
      obj.move (n->second.front ());
      writer->write (obj, rep_single);

    }

  }
}

// ---------------------------------------------------------------------------------
//  OASISWriter implementation

OASISWriter::OASISWriter ()
  : mp_stream (0),
    m_sf (1.0),
    mp_layout (0),
    mp_cell (0),
    m_layer (0), m_datatype (0),
    m_write_context_info (false),
    m_in_cblock (false),
    m_propname_id (0),
    m_propstring_id (0),
    m_textstring_id (0),
    m_proptables_written (false),
    m_progress (tl::to_string (tr ("Writing OASIS file")), 10000)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

// 1M CBLOCK buffer size
const size_t cblock_buffer_size = 1024 * 1024;

void 
OASISWriter::write_record_id (char b)
{
  if (m_in_cblock) {
    if (m_cblock_buffer.size () > cblock_buffer_size) {
      end_cblock ();
      begin_cblock ();
    } 
    m_cblock_buffer.write ((const char *) &b, 1);
  } else {
    mp_stream->put ((const char *) &b, 1);
  }
}

void 
OASISWriter::write_byte (char b)
{
  if (m_in_cblock) {
    m_cblock_buffer.write ((const char *) &b, 1);
  } else {
    mp_stream->put ((const char *) &b, 1);
  }
}

void
OASISWriter::write_bytes (const char *b, size_t n)
{
  if (m_in_cblock) {
    m_cblock_buffer.write (b, n);
  } else {
    mp_stream->put (b, n);
  }
}

void 
OASISWriter::write (long long n)
{
  if (n < 0) {
    write (((unsigned long long) (-n) << 1) | 1);
  } else {
    write ((unsigned long long) n << 1);
  }
}

void 
OASISWriter::write (unsigned long long n)
{
  char buffer [50];
  char *bptr = buffer;

  do {
    unsigned char b = n & 0x7f;
    n >>= 7;
    if (n > 0) {
      b |= 0x80;
    }
    *bptr++ = (char) b;
  } while (n > 0);

  write_bytes (buffer, bptr - buffer);
}

void 
OASISWriter::write (long n)
{
  if (n < 0) {
    write (((unsigned long) (-n) << 1) | 1);
  } else {
    write ((unsigned long) n << 1);
  }
}

void 
OASISWriter::write (unsigned long n)
{
  char buffer [50];
  char *bptr = buffer;

  do {
    unsigned char b = n & 0x7f;
    n >>= 7;
    if (n > 0) {
      b |= 0x80;
    }
    *bptr++ = (char) b;
  } while (n > 0);

  write_bytes (buffer, bptr - buffer);
}

void
OASISWriter::write (float d) 
{
  if (fabs (d) >= 0.5 && fabs (floor (d + 0.5) - d) < 1e-6 && fabs (d) < double (std::numeric_limits<long>::max ())) {

    //  whole number (negative or positive)
    if (d < 0.0) {
      write_byte (1);
      write ((unsigned long) floor (-d + 0.5));
    } else { 
      write_byte (0);
      write ((unsigned long) floor (d + 0.5));
    }

  } else {

    write_byte (6);

    //  4-Byte IEEE real
    union {
      float d;
      uint32_t i;
    } f2i;

    f2i.d = d;
    uint32_t i = f2i.i;
    char b[sizeof (f2i.i)];
    for (unsigned int n = 0; n < sizeof (f2i.i); n++) {
      b[n] = char (i & 0xff); 
      i >>= 8;
    }
    write_bytes (b, sizeof (f2i.i));

  }
}

void
OASISWriter::write (double d) 
{
  if (fabs (d) >= 0.5 && fabs (floor (d + 0.5) - d) < 1e-10 && fabs (d) < double (std::numeric_limits<long>::max ())) {

    //  whole number (negative or positive)
    if (d < 0.0) {
      write_byte (1);
      write ((unsigned long) floor (-d + 0.5));
    } else { 
      write_byte (0);
      write ((unsigned long) floor (d + 0.5));
    }

  } else {

    write_byte (7);

    //  8-Byte IEEE real
    union {
      double d;
      uint64_t i;
    } f2i;

    f2i.d = d;
    uint64_t i = f2i.i;
    char b[sizeof (f2i.i)];
    for (unsigned int n = 0; n < sizeof (f2i.i); n++) {
      b[n] = char (i & 0xff); 
      i >>= 8;
    }
    write_bytes (b, sizeof (f2i.i));

  }
}

void 
OASISWriter::write_bstring (const char *s)
{
  size_t l = strlen (s);
  write (l);
  write_bytes (s, l);
}

std::string
OASISWriter::make_astring (const char *s)
{
  if (m_options.subst_char.empty ()) {
    //  No substitution: leave text as it is
    return std::string (s);
  } else {
    return make_n_or_astring (s, m_options.subst_char, false);
  }
}

void  
OASISWriter::write_astring (const char *s)
{
  std::string nstr = make_astring (s);
  write (nstr.size ());
  write_bytes (nstr.c_str (), nstr.size ());
}

std::string
OASISWriter::make_nstring (const char *s)
{
  if (m_options.subst_char.empty ()) {
    //  No substitution: leave text as it is
    return std::string (s);
  } else {
    return make_n_or_astring (s, m_options.subst_char, true);
  }
}

void  
OASISWriter::write_nstring (const char *s)
{
  std::string nstr = make_nstring (s);
  write (nstr.size ());
  write_bytes (nstr.c_str (), nstr.size ());
}

void
OASISWriter::write_gdelta (const db::Vector &p, double sf)
{
  db::Coord x = p.x ();
  db::Coord y = p.y ();

  if (sf != 1.0) {
    x = safe_scale (sf, x);
    y = safe_scale (sf, y);
  }

  if (x == -y || x == y || x == 0 || y == 0) {

    unsigned long long dir = 0;
    unsigned long long l = 0;

    if (x > 0) {
      l = x;
      if (y == 0) {
        dir = 0;
      } else if (y < 0) {
        dir = 7;
      } else {
        dir = 4;
      }
    } else if (x == 0) {
      if (y < 0) {
        l = -y;
        dir = 3;
      } else {
        l = y;
        dir = 1;
      }
    } else if (x < 0) {
      l = -x;
      if (y == 0) {
        dir = 2;
      } else if (y < 0) {
        dir = 6;
      } else {
        dir = 5;
      }
    }

    write ((l << 4) | (dir << 1));

  } else {

    unsigned long long d;
    if (x < 0) {
      d = ((unsigned long long) -x << 2) | 3;
    } else {
      d = ((unsigned long long) x << 2) | 1;
    }
    write (d);
    write (y);

  }
}

void
OASISWriter::write_coord (db::Coord c, double sf)
{
  if (sf == 1.0) {
    return write (c);
  } else {
    return write (safe_scale (sf, c));
  }
}

void
OASISWriter::write_ucoord (db::Coord c, double sf)
{
  // HACK: we misuse distance type as unsigned coord type here.
  typedef db::coord_traits<db::Coord>::distance_type ucoord;
  if (sf == 1.0) {
    return write ((ucoord) c);
  } else {
    return write (safe_scale (sf, (ucoord) c));
  }
}

void
OASISWriter::write_coord (db::Coord c)
{
  if (m_sf == 1.0) {
    return write (c);
  } else {
    return write (safe_scale (m_sf, c));
  }
}

void
OASISWriter::write_ucoord (db::Coord c)
{
  // HACK: we misuse distance type as unsigned coord type here.
  typedef db::coord_traits<db::Coord>::distance_type ucoord;
  if (m_sf == 1.0) {
    return write ((ucoord) c);
  } else {
    return write (safe_scale (m_sf, (ucoord) c));
  }
}

void
OASISWriter::emit_propname_def (db::properties_id_type prop_id)
{
  const db::PropertiesRepository::properties_set &props = mp_layout->properties_repository ().properties (prop_id);
  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {

    const tl::Variant &name = mp_layout->properties_repository ().prop_name (p->first);
    const char *name_str = s_gds_property_name;
    if (! make_gds_property (name)) {
      name_str = name.to_string ();
    }
    if (m_propnames.insert (std::make_pair (name_str, m_propname_id)).second) {
      write_record_id (7);
      write_nstring (name_str);
      ++m_propname_id;
    }

  }
}

void
OASISWriter::emit_propstring_def (db::properties_id_type prop_id)
{
  std::vector<tl::Variant> pv_list;

  const db::PropertiesRepository::properties_set &props = mp_layout->properties_repository ().properties (prop_id);
  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {

    pv_list.clear ();
    const std::vector<tl::Variant> *pvl = &pv_list;

    const tl::Variant &name = mp_layout->properties_repository ().prop_name (p->first);
    if (! make_gds_property (name)) {

      if (p->second.is_list ()) {
        pvl = &p->second.get_list ();
      } else if (!p->second.is_nil ()) {
        pv_list.reserve (1);
        pv_list.push_back (p->second);
      }

    } else {

      pv_list.reserve (2);
      pv_list.push_back (name.to_ulong ());
      pv_list.push_back (p->second.to_string ());

    }

    for (std::vector<tl::Variant>::const_iterator pv = pvl->begin (); pv != pvl->end (); ++pv) {
      if (!pv->is_double () && !pv->is_longlong () && !pv->is_ulonglong () && !pv->is_long () && !pv->is_ulong ()) {
        if (m_propstrings.insert (std::make_pair (pv->to_string (), m_propstring_id)).second) {
          write_record_id (9);
          write_bstring (pv->to_string ());
          ++m_propstring_id;
        }
      }
    }

  }
}

void 
OASISWriter::begin_cblock ()
{
  tl_assert (! m_in_cblock);

  m_in_cblock = true;
}

void
OASISWriter::end_cblock ()
{
  tl_assert (m_in_cblock);

  m_cblock_compressed.clear ();
  tl::OutputStream deflated_stream (m_cblock_compressed);
  tl::DeflateFilter deflate (deflated_stream);

  //  Reasoning for if(...): we don't want to access data from an empty vector through data()
  if (m_cblock_buffer.size () > 0) {
    deflate.put (m_cblock_buffer.data (), m_cblock_buffer.size ());
  }

  deflate.flush ();

  const size_t compression_overhead = 4;
  m_in_cblock = false;

  if (m_cblock_buffer.size () > m_cblock_compressed.size () + compression_overhead) {

    write_byte (34);  // CBLOCK

    //  RFC1951 compression:
    write_byte (0); 

    write (m_cblock_buffer.size ());
    write (m_cblock_compressed.size ());

    write_bytes (m_cblock_compressed.data (), m_cblock_compressed.size ());

  } else if (m_cblock_buffer.size () > 0) {  //  Reasoning for if(...): we don't want to access data from an empty vector through data()
    write_bytes (m_cblock_buffer.data (), m_cblock_buffer.size ());
  }

  m_cblock_buffer.clear ();
  m_cblock_compressed.clear ();
}

void 
OASISWriter::begin_table (size_t &pos)
{
  if (pos == 0) {
    pos = mp_stream->pos ();
    if (m_options.write_cblocks) {
      begin_cblock ();
    }
  }
}

void 
OASISWriter::end_table (size_t pos)
{
  if (pos != 0 && m_options.write_cblocks) {
    end_cblock ();
  }
}

void 
OASISWriter::reset_modal_variables ()
{
  //  reset modal variables
  mm_repetition.reset ();
  mm_placement_cell.reset ();
  mm_placement_x = 0;
  mm_placement_y = 0;
  mm_layer.reset ();
  mm_datatype.reset ();
  mm_textlayer.reset ();
  mm_texttype.reset ();
  mm_text_x = 0;
  mm_text_y = 0;
  mm_text_string.reset ();
  mm_geometry_x = 0;
  mm_geometry_y = 0;
  mm_geometry_w.reset ();
  mm_geometry_h.reset ();
  mm_polygon_point_list.reset ();
  mm_path_halfwidth.reset ();
  mm_path_start_extension.reset ();
  mm_path_end_extension.reset ();
  mm_path_point_list.reset ();
  mm_ctrapezoid_type.reset ();
  mm_circle_radius.reset ();
  mm_last_property_name.reset ();
  mm_last_property_is_sprop.reset ();
  mm_last_value_list.reset ();
}

void
OASISWriter::write_propname_table (size_t &propnames_table_pos, const std::vector<db::cell_index_type> &cells, const db::Layout &layout, const std::vector<std::pair<unsigned int, LayerProperties> > &layers)
{
  //  write the property names collected so far in the order of the ID's.

  std::vector<std::pair<unsigned long, std::string> > rev_pn;
  rev_pn.reserve (m_propnames.size ());
  for (auto p = m_propnames.begin (); p != m_propnames.end (); ++p) {
    rev_pn.push_back (std::make_pair (p->second, p->first));
  }
  std::sort (rev_pn.begin (), rev_pn.end ());

  for (auto p = rev_pn.begin (); p != rev_pn.end (); ++p) {
    tl_assert (p->first == (unsigned long)(p - rev_pn.begin ()));
    begin_table (propnames_table_pos);
    write_record_id (7);
    write_nstring (p->second.c_str ());
  }

  //  collect and write the future property names

  std::set <db::properties_id_type> prop_ids_done;

  for (auto cell = cells.begin (); cell != cells.end (); ++cell) {

    const db::Cell &cref (layout.cell (*cell));

    if (cref.prop_id () != 0) {
      begin_table (propnames_table_pos);
      emit_propname_def (cref.prop_id ());
    }

    for (db::Cell::const_iterator inst = cref.begin (); ! inst.at_end (); ++inst) {
      if (inst->has_prop_id () && inst->prop_id () != 0 && prop_ids_done.find (inst->prop_id ()) == prop_ids_done.end ()) {
        prop_ids_done.insert (inst->prop_id ());
        begin_table (propnames_table_pos);
        emit_propname_def (inst->prop_id ());
        m_progress.set (mp_stream->pos ());
      }
    }

    for (auto l = layers.begin (); l != layers.end (); ++l) {
      db::ShapeIterator shape (cref.shapes (l->first).begin (db::ShapeIterator::Properties | db::ShapeIterator::Boxes | db::ShapeIterator::Polygons | db::ShapeIterator::Edges | db::ShapeIterator::Paths | db::ShapeIterator::Texts));
      while (! shape.at_end ()) {
        if (shape->has_prop_id () && shape->prop_id () != 0 && prop_ids_done.find (shape->prop_id ()) == prop_ids_done.end ()) {
          prop_ids_done.insert (shape->prop_id ());
          begin_table (propnames_table_pos);
          emit_propname_def (shape->prop_id ());
          m_progress.set (mp_stream->pos ());
        }
        shape.finish_array ();
      }
    }

  }

  //  if needed, emit property name required for the PCell or meta info context information

  if (m_write_context_info && m_propnames.find (std::string (klayout_context_name)) == m_propnames.end ()) {

    bool has_context = false;
    for (auto cell = cells.begin (); cell != cells.end () && ! has_context; ++cell) {
      LayoutOrCellContextInfo ci;
      has_context = layout.has_context_info (*cell) && layout.get_context_info (*cell, ci);
    }

    if (has_context) {
      m_propnames.insert (std::make_pair (std::string (klayout_context_name), m_propname_id++));
      begin_table (propnames_table_pos);
      write_record_id (7);
      write_nstring (klayout_context_name);
    }

  }

  end_table (propnames_table_pos);
}

void
OASISWriter::write_propstring_table (size_t &propstrings_table_pos, const std::vector<db::cell_index_type> &cells, const db::Layout &layout, const std::vector<std::pair<unsigned int, LayerProperties> > &layers)
{
  //  write the property strings collected so far in the order of the ID's.

  std::vector<std::pair<unsigned long, const std::string *> > rev_ps;
  rev_ps.reserve (m_propstrings.size ());
  for (auto p = m_propstrings.begin (); p != m_propstrings.end (); ++p) {
    rev_ps.push_back (std::make_pair (p->second, &p->first));
  }
  std::sort (rev_ps.begin (), rev_ps.end ());

  tl_assert (rev_ps.size () == size_t (m_propstring_id));

  for (auto p = rev_ps.begin (); p != rev_ps.end (); ++p) {
    tl_assert (p->first == (unsigned long)(p - rev_ps.begin ()));
    begin_table (propstrings_table_pos);
    write_record_id (9);
    write_bstring (p->second->c_str ());
  }

  //  collect and write the future property strings

  std::set <db::properties_id_type> prop_ids_done;

  for (auto cell = cells.begin (); cell != cells.end (); ++cell) {

    const db::Cell &cref (layout.cell (*cell));

    if (cref.prop_id () != 0 && prop_ids_done.find (cref.prop_id ()) == prop_ids_done.end ()) {
      prop_ids_done.insert (cref.prop_id ());
      begin_table (propstrings_table_pos);
      emit_propstring_def (cref.prop_id ());
    }

    for (db::Cell::const_iterator inst = cref.begin (); ! inst.at_end (); ++inst) {
      if (inst->has_prop_id () && inst->prop_id () != 0 && prop_ids_done.find (inst->prop_id ()) == prop_ids_done.end ()) {
        prop_ids_done.insert (inst->prop_id ());
        begin_table (propstrings_table_pos);
        emit_propstring_def (inst->prop_id ());
        m_progress.set (mp_stream->pos ());
      }
    }

    for (auto l = layers.begin (); l != layers.end (); ++l) {
      db::ShapeIterator shape (cref.shapes (l->first).begin (db::ShapeIterator::Properties | db::ShapeIterator::Boxes | db::ShapeIterator::Polygons | db::ShapeIterator::Edges | db::ShapeIterator::Paths | db::ShapeIterator::Texts));
      while (! shape.at_end ()) {
        if (shape->has_prop_id () && shape->prop_id () != 0 && prop_ids_done.find (shape->prop_id ()) == prop_ids_done.end ()) {
          prop_ids_done.insert (shape->prop_id ());
          begin_table (propstrings_table_pos);
          emit_propstring_def (shape->prop_id ());
          m_progress.set (mp_stream->pos ());
        }
        shape.finish_array ();
      }
    }

  }

  if (m_write_context_info) {

    //  emit property string id's required for the PCell and meta info context information
    std::vector <std::string> context_prop_strings;

    for (auto cell = cells.begin (); cell != cells.end (); ++cell) {

      m_progress.set (mp_stream->pos ());
      context_prop_strings.clear ();

      if (layout.has_context_info (*cell) && layout.get_context_info (*cell, context_prop_strings)) {

        for (auto c = context_prop_strings.begin (); c != context_prop_strings.end (); ++c) {
          if (m_propstrings.insert (std::make_pair (*c, m_propstring_id)).second) {
            begin_table (propstrings_table_pos);
            write_record_id (9);
            write_bstring (c->c_str ());
            ++m_propstring_id;
          }
        }

      }

    }

  }

  end_table (propstrings_table_pos);
}

void
OASISWriter::write_cellname_table (size_t &cellnames_table_pos, const std::vector<db::cell_index_type> &cells_by_index, const std::map<db::cell_index_type, size_t> *cell_positions, const db::Layout &layout)
{
  bool sequential = true;
  for (auto cell = cells_by_index.begin (); cell != cells_by_index.end () && sequential; ++cell) {
    sequential = (*cell == db::cell_index_type (cell - cells_by_index.begin ()));
  }

  //  CELLNAME (implicit or explicit)
  for (auto cell = cells_by_index.begin (); cell != cells_by_index.end (); ++cell) {

    begin_table (cellnames_table_pos);

    write_record_id (sequential ? 3 : 4);
    write_nstring (layout.cell_name (*cell));
    if (! sequential) {
      write ((unsigned long) *cell);
    }

    if (m_options.write_std_properties >= 1) {

      reset_modal_variables ();

      if (m_options.write_std_properties > 1) {

        //  write S_BOUNDING_BOX entries

        std::vector<tl::Variant> values;

        //  TODO: how to set the "depends on external cells" flag?
        db::Box bbox = layout.cell (*cell).bbox ();
        if (bbox.empty ()) {
          //  empty box
          values.push_back (tl::Variant ((unsigned int) 0x2));
          bbox = db::Box (0, 0, 0, 0);
        } else {
          values.push_back (tl::Variant ((unsigned int) 0x0));
        }

        values.push_back (tl::Variant (bbox.left ()));
        values.push_back (tl::Variant (bbox.bottom ()));
        values.push_back (tl::Variant (bbox.width ()));
        values.push_back (tl::Variant (bbox.height ()));

        write_property_def (s_bounding_box_name, values, true);

      }

      //  PROPERTY record with S_CELL_OFFSET
      if (cell_positions) {
        std::map<db::cell_index_type, size_t>::const_iterator pp = cell_positions->find (*cell);
        if (pp != cell_positions->end ()) {
          write_property_def (s_cell_offset_name, tl::Variant (pp->second), true);
        } else {
          write_property_def (s_cell_offset_name, tl::Variant (size_t (0)), true);
        }
      }

    }

  }

  end_table (cellnames_table_pos);
}

void
OASISWriter::write_textstring_table (size_t &textstrings_table_pos, const std::vector<db::cell_index_type> &cells, const db::Layout &layout, const std::vector<std::pair<unsigned int, LayerProperties> > &layers)
{
  //  write present text strings

  //  collect present strings by ID
  std::vector<std::pair<unsigned long, const std::string *> > rev_ts;
  rev_ts.reserve (m_textstrings.size ());
  for (auto p = m_textstrings.begin (); p != m_textstrings.end (); ++p) {
    rev_ts.push_back (std::make_pair (p->second, &p->first));
  }
  std::sort (rev_ts.begin (), rev_ts.end ());

  tl_assert (rev_ts.size () == size_t (m_textstring_id));

  for (auto t = rev_ts.begin (); t != rev_ts.end (); ++t) {
    tl_assert (t->first == (unsigned long)(t - rev_ts.begin ()));
    begin_table (textstrings_table_pos);
    write_record_id (5);
    write_nstring (t->second->c_str ());
  }

  //  collect future test strings

  for (auto cell = cells.begin (); cell != cells.end (); ++cell) {

    const db::Cell &cref (layout.cell (*cell));
    for (auto l = layers.begin (); l != layers.end (); ++l) {
      db::ShapeIterator shape (cref.shapes (l->first).begin (db::ShapeIterator::Texts));
      while (! shape.at_end ()) {
        if (m_textstrings.insert (std::make_pair (shape->text_string (), m_textstring_id)).second) {
          begin_table (textstrings_table_pos);
          write_record_id (5);
          write_astring (shape->text_string ());
          ++m_textstring_id;
          m_progress.set (mp_stream->pos ());
        }
        ++shape;
      }
    }

  }

  end_table (textstrings_table_pos);
}

void
OASISWriter::write_layername_table (size_t &layernames_table_pos, const std::vector <std::pair <unsigned int, db::LayerProperties> > &layers)
{
  for (auto l = layers.begin (); l != layers.end (); ++l) {

    if (! l->second.name.empty ()) {

      begin_table (layernames_table_pos);

      //  write mappings to text layer and shape layers
      write_record_id (11);
      write_nstring (l->second.name.c_str ());
      write_byte (3);
      write ((unsigned long) l->second.layer);
      write_byte (3);
      write ((unsigned long) l->second.datatype);

      write_record_id (12);
      write_nstring (l->second.name.c_str ());
      write_byte (3);
      write ((unsigned long) l->second.layer);
      write_byte (3);
      write ((unsigned long) l->second.datatype);

      m_progress.set (mp_stream->pos ());

    }

  }

  end_table (layernames_table_pos);
}

static bool must_write_cell (const db::Cell &cref)
{
  //  Don't write proxy cells which are not employed
  return ! cref.is_proxy () || ! cref.is_top ();
}

static bool skip_cell_body (const db::Cell &cref)
{
  //  Skip cell bodies for ghost cells unless empty (they are not longer ghost cells in this case)
  return cref.is_ghost_cell () && cref.empty ();
}


void 
OASISWriter::write (db::Layout &layout, tl::OutputStream &stream, const db::SaveLayoutOptions &options)
{
  typedef db::coord_traits<db::Coord>::distance_type coord_distance_type;

  mp_layout = &layout;
  mp_cell = 0;
  m_layer = m_datatype = 0;
  m_in_cblock = false;
  m_cblock_buffer.clear ();
  m_write_context_info = options.write_context_info ();

  m_options = options.get_options<OASISWriterOptions> ();
  mp_stream = &stream;

  if (stream.is_compressing ()) {
    std::string msg = tl::to_string (tr ("File compression is discouraged in OASIS, please use CBLOCK compression"));
    tl::warn << msg;
  }

  double dbu = (options.dbu () == 0.0) ? layout.dbu () : options.dbu ();
  m_sf = options.scale_factor () * (layout.dbu () / dbu);
  if (fabs (m_sf - 1.0) < 1e-9) {
    //  to avoid rounding problems, set to 1.0 exactly if possible.
    m_sf = 1.0;
  }

  std::vector <std::pair <unsigned int, db::LayerProperties> > layers;
  options.get_valid_layers (layout, layers, db::SaveLayoutOptions::LP_AssignNumber);

  std::set <db::cell_index_type> cell_set;
  options.get_cells (layout, cell_set, layers);

  //  create a cell index vector sorted bottom-up
  std::vector <db::cell_index_type> cells, cells_by_index;

  cells.reserve (cell_set.size ());
  cells_by_index.reserve (cell_set.size ());

  for (db::Layout::bottom_up_const_iterator cell = layout.begin_bottom_up (); cell != layout.end_bottom_up (); ++cell) {
    if (cell_set.find (*cell) != cell_set.end () && must_write_cell (layout.cell (*cell))) {
      cells.push_back (*cell);
    }
  }

  for (db::Layout::const_iterator cell = layout.begin (); cell != layout.end (); ++cell) {
    if (cell_set.find (cell->cell_index ()) != cell_set.end () && must_write_cell (layout.cell (cell->cell_index ()))) {
      cells_by_index.push_back (cell->cell_index ());
    }
  }

  //  write header

  char magic[] = "%SEMI-OASIS\015\012";
  write_bytes (magic, sizeof (magic) - 1);

  //  START record
  write_record_id (1); 
  write_bstring ("1.0");
  write (1.0 / dbu);
  write_byte (m_options.strict_mode ? 1 : 0);  //  offset-flag (strict mode: at the end, non-strict mode: at the beginning)

  size_t cellnames_table_pos = 0;
  size_t textstrings_table_pos = 0;
  size_t propnames_table_pos = 0;
  size_t propstrings_table_pos = 0;
  size_t layernames_table_pos = 0;
  std::map<db::cell_index_type, size_t> cell_positions;

  if (! m_options.strict_mode) {

    //  offset table:
    for (unsigned int i = 0; i < 12; ++i) {
      write_byte (0);
    }

  }

  //  Reset the global variables

  reset_modal_variables ();

  //  Prepare name tables
 
  m_textstrings.clear ();
  m_propnames.clear ();
  m_propstrings.clear ();

  //  We will collect the standard properties here:
  
  m_propstring_id = m_propname_id = 0;
  m_textstring_id = 0;
  m_proptables_written = false;
  std::vector<std::pair<std::string, unsigned int> > init_props;

  //  write file properties (must happen before any other PROPNAME record since formally the
  //  PROPERTY records are associated with the names rather than the file)

  //  prepare some property ID's in strict mode .. in non-strict mode we write strings to
  //  avoid forward references
  if (m_options.strict_mode) {
    m_propnames.insert (std::make_pair (s_cell_offset_name, m_propname_id++));
    m_propnames.insert (std::make_pair (s_gds_property_name, m_propname_id++));
    if (m_options.write_std_properties > 0) {
      m_propnames.insert (std::make_pair (s_max_signed_integer_width_name, m_propname_id++));
      m_propnames.insert (std::make_pair (s_max_unsigned_integer_width_name, m_propname_id++));
      m_propnames.insert (std::make_pair (s_top_cell_name, m_propname_id++));
      if (m_options.write_std_properties > 1) {
        m_propnames.insert (std::make_pair (s_bounding_boxes_available_name, m_propname_id++));
      }
    }
  }

  if (m_options.write_std_properties > 0) {

    write_property_def (s_max_signed_integer_width_name, tl::Variant (sizeof (db::Coord)), true);
    write_property_def (s_max_unsigned_integer_width_name, tl::Variant (sizeof (coord_distance_type)), true);

    for (std::vector<db::cell_index_type>::const_iterator cell = cells.begin (); cell != cells.end (); ++cell) {
      const db::Cell &c = layout.cell (*cell);
      bool is_top = true;
      for (db::Cell::parent_cell_iterator p = c.begin_parent_cells (); p != c.end_parent_cells () && is_top; ++p) {
        is_top = (cell_set.find (*p) == cell_set.end ());
      }
      if (is_top) {
        write_property_def (s_top_cell_name, tl::Variant (make_nstring (layout.cell_name (*cell))), true);
      }
    }

    if (m_options.write_std_properties > 1) {
      write_property_def (s_bounding_boxes_available_name, tl::Variant ((unsigned int) 2), true);
    }

  }

  if (m_options.write_std_properties > 1) {
    m_propnames.insert (std::make_pair (s_bounding_box_name, m_propname_id++));
  }

  if (layout.prop_id () != 0) {
    write_props (layout.prop_id ());
  }

  std::vector <std::string> context_prop_strings;

  //  write the global layout context information

  if (options.write_context_info () && layout.has_context_info () && layout.get_context_info (context_prop_strings)) {

    std::vector<tl::Variant> values;
    values.reserve (context_prop_strings.size ());
    for (auto i = context_prop_strings.begin (); i != context_prop_strings.end (); ++i) {
      values.push_back (tl::Variant (*i));
    }

    write_property_def (klayout_context_name, values, false);

    context_prop_strings.clear ();

  }

  //  write the tables

  if (! m_options.tables_at_end) {

    write_propname_table (propnames_table_pos, cells, layout, layers);
    write_propstring_table (propstrings_table_pos, cells, layout, layers);

    //  Now we cannot open new property ID's in strict mode
    m_proptables_written = true;

    //  build cell name table now in non-strict mode (in strict mode it is written at the
    //  end because then we have the cell positions fo S_CELL_OFFSET)
    if (! m_options.strict_mode) {
      write_cellname_table (cellnames_table_pos, cells_by_index, 0, layout);
    }

    write_textstring_table (textstrings_table_pos, cells, layout, layers);
    write_layername_table (layernames_table_pos, layers);

  }

  //  write cells

  for (std::vector<db::cell_index_type>::const_iterator cell = cells.begin (); cell != cells.end (); ++cell) {

    m_progress.set (mp_stream->pos ());

    //  cell body 
    const db::Cell &cref (layout.cell (*cell));
    mp_cell = &cref;

    //  skip cell body if the cell is not to be written
    if (skip_cell_body (cref)) {
      continue;
    }

    //  cell header

    cell_positions.insert (std::make_pair (*cell, mp_stream->pos ()));

    write_record_id (13);  // CELL
    write ((unsigned long) *cell);

    reset_modal_variables ();

    if (m_options.write_cblocks) {
      begin_cblock ();
    }

    //  context information as property named KLAYOUT_CONTEXT
    if (options.write_context_info () && layout.has_context_info (*cell)) {

      context_prop_strings.clear ();

      if (layout.get_context_info (*cell, context_prop_strings)) {

        write_record_id (28);
        write_byte (char (0xf6));
        unsigned long pnid = 0;
        std::map <std::string, unsigned long>::const_iterator pni = m_propnames.find (klayout_context_name);
        if (pni == m_propnames.end ()) {
          pnid = m_propname_id++;
          m_propnames.insert (std::make_pair (klayout_context_name, pnid));
        } else {
          pnid = pni->second;
        }
        write (pnid);

        write ((unsigned long) context_prop_strings.size ());

        for (std::vector <std::string>::const_iterator c = context_prop_strings.begin (); c != context_prop_strings.end (); ++c) {
          write_byte (14); // b-string by reference number
          unsigned long psid = 0;
          std::map <std::string, unsigned long>::const_iterator psi = m_propstrings.find (*c);
          if (psi == m_propstrings.end ()) {
            psid = m_propstring_id++;
            m_propstrings.insert (std::make_pair (*c, psid)).second;
          } else {
            psid = psi->second;
          }
          write (psid);
        }

        mm_last_property_name = klayout_context_name;
        mm_last_property_is_sprop = false;
        mm_last_value_list.reset ();

      }

    }

    if (cref.prop_id () != 0) {
      write_props (cref.prop_id ());
    }

    //  instances
    if (cref.cell_instances () > 0) {
      write_insts (cell_set);
    }

    //  shapes
    for (std::vector <std::pair <unsigned int, db::LayerProperties> >::const_iterator l = layers.begin (); l != layers.end (); ++l) {
      const db::Shapes &shapes = cref.shapes (l->first);
      if (! shapes.empty ()) {
        write_shapes (l->second, shapes);
        m_progress.set (mp_stream->pos ());
      }
    }

    //  end CBLOCK if required
    if (m_options.write_cblocks) {
      end_cblock ();
    }

    //  end of cell

  }

  //  write the tables if at end

  if (m_options.tables_at_end) {

    //  do not consider future items as everything has been collected
    std::vector<cell_index_type> no_cells;
    std::vector <std::pair <unsigned int, db::LayerProperties> > no_layers;

    write_propname_table (propnames_table_pos, no_cells, layout, no_layers);
    write_propstring_table (propstrings_table_pos, no_cells, layout, no_layers);

    //  Now we cannot open new property ID's in strict mode
    m_proptables_written = true;

    write_textstring_table (textstrings_table_pos, no_cells, layout, no_layers);

    //  write all layers here
    write_layername_table (layernames_table_pos, layers);

  }

  //  write cell table at the end in strict mode (in that mode we need the cell positions
  //  for the S_CELL_OFFSET properties)
  if (m_options.tables_at_end || m_options.strict_mode) {
    write_cellname_table (cellnames_table_pos, cells_by_index, &cell_positions, layout);
  }

  //  END record

  size_t end_record_pos = mp_stream->pos ();

  write_record_id (2);

  if (m_options.strict_mode) {

    //  offset table for strict mode (write it now since we have the table offsets now)

    //  cellnames
    write_byte (1); 
    write (cellnames_table_pos);

    //  textstrings
    write_byte (1); 
    write (textstrings_table_pos);

    //  propnames
    write_byte (1); 
    write (propnames_table_pos);

    //  propstrings
    write_byte (1); 
    write (propstrings_table_pos);

    //  layernames
    write_byte (1); 
    write (layernames_table_pos);

    //  xnames (not used)
    write_byte (1); 
    write (0);

  } 

  //  write a b-string to pad up to 255 bytes
  //  (this bstring consists of a "long zero" and no characters
  while (mp_stream->pos () < end_record_pos + 254) {
    write_byte (char (0x80));
  }
  write_byte (0);

  //  validation-scheme
  write_byte (0);

  m_progress.set (mp_stream->pos ());
}

void 
OASISWriter::write (const Repetition &rep)
{
  if (mm_repetition == rep) {
    write_byte (0);  // reuse
  } else {

    mm_repetition = rep;

    db::Vector a, b;
    size_t amax, bmax;

    bool is_reg = rep.is_regular (a, b, amax, bmax);
    const std::vector<db::Vector> *iterated = rep.is_iterated ();

    if (iterated) {

      tl_assert (! iterated->empty ());

      //  extract common grid
      db::Coord g = 0;
      for (std::vector<db::Vector>::const_iterator p = iterated->begin (); p != iterated->end (); ++p) {

        db::Coord x = safe_scale (m_sf, p->x ());
        if (x < 0) {
          x = -x;
        }
        if (x != 0) {
          g = (g == 0) ? x : tl::gcd (g, x);
        }

        db::Coord y = safe_scale (m_sf, p->y ());
        if (y < 0) {
          y = -y;
        }
        if (y != 0) {
          g = (g == 0) ? y : tl::gcd (g, y);
        }

      }

      if (g <= 1) {
        write_byte (10);
        write (iterated->size () - 1);
        g = 1;
      } else {
        write_byte (11);
        write (iterated->size () - 1);
        write_ucoord (g, 1.0);
      }

      db::Vector last_point;
      for (std::vector<db::Vector>::const_iterator p = iterated->begin (); p != iterated->end (); ++p) {
        db::Vector s (safe_scale (m_sf, p->x ()), safe_scale (m_sf, p->y ()));
        db::Vector delta = s - last_point;
        last_point = s;
        write_gdelta (db::Vector (delta.x () / g, delta.y () / g), 1.0);
      }

    } else {

      tl_assert (is_reg);

      //  currently there are only regular repetitions
      //  TODO: optimize for orthogonal cases 
      tl_assert (is_reg);
      tl_assert (amax >= 2 || bmax >= 2);

      if (amax == 1 || bmax == 1) {

        if (bmax == 1) {
          b = a;
          bmax = amax;
        }

        if (b.x () == 0 && b.y () >= 0) {
          write_byte (3);
          write (bmax - 2);
          write_ucoord (b.y ());
        } else if (b.y () == 0 && b.x () >= 0) {
          write_byte (2);
          write (bmax - 2);
          write_ucoord (b.x ());
        } else {
          write_byte (9);
          write (bmax - 2);
          write_gdelta (b);
        }

      } else if (b.x () == 0 && b.y () >= 0 && a.y () == 0 && a.x () >= 0) {

        write_byte (1);
        write (amax - 2);
        write (bmax - 2);
        write_ucoord (a.x ());
        write_ucoord (b.y ());

      } else if (b.y () == 0 && b.x () >= 0 && a.x () == 0 && a.y () >= 0) {

        write_byte (1);
        write (bmax - 2);
        write (amax - 2);
        write_ucoord (b.x ());
        write_ucoord (a.y ());

      } else {

        write_byte (8);
        write (amax - 2);
        write (bmax - 2);
        write_gdelta (a);
        write_gdelta (b);

      }

    }

  }
}

void 
OASISWriter::write_inst_with_rep (const db::CellInstArray &inst, db::properties_id_type prop_id, const db::Vector &disp, const db::Repetition &rep)
{
  db::Vector tr = inst.front ().disp () + disp;

  unsigned char info = 0x40;  // by reference number
  if (mm_placement_cell != inst.object ().cell_index ()) {
    info |= 0x80;
  }
  if (mm_placement_x != tr.x ()) {
    info |= 0x20;
  }
  if (mm_placement_y != tr.y ()) {
    info |= 0x10;
  }
  if (rep != Repetition ()) {
    info |= 0x08;
  }

  if (inst.front ().is_mirror ()) {
    info |= 0x01;
  }

  if (inst.is_complex ()) {
    write_record_id (18);
    write_byte (info | 0x06);
  } else {
    write_record_id (17);
    write_byte (info | ((inst.front ().rot () & 0x03) << 1));
  }

  if (info & 0x80) {
    mm_placement_cell = inst.object ().cell_index ();
    write ((unsigned long) mm_placement_cell.get ());
  }

  if (inst.is_complex ()) {
    write (inst.complex_trans ().mag ());
    write (inst.complex_trans ().angle ());
  }
  
  if (info & 0x20) {
    mm_placement_x = tr.x ();
    write_coord (mm_placement_x.get ());
  }
  if (info & 0x10) {
    mm_placement_y = tr.y ();
    write_coord (mm_placement_y.get ());
  }

  if (info & 0x08) {
    write (rep);
  }

  if (prop_id != 0) {
    write_props (prop_id);
  }
}

void
OASISWriter::write (const db::CellInstArray &inst, db::properties_id_type prop_id, const db::Repetition &rep)
{
  m_progress.set (mp_stream->pos ());

  std::vector<db::Vector> pts;
  db::Vector a, b;
  unsigned long amax, bmax;

  if (inst.is_iterated_array (&pts) && pts.size () > 1) {

    // Remove the first point which is implicitly contained in the repetition
    // Note: we can do so because below we instantiate the shape at the front of the array which includes
    // the first transformation already.
    db::Vector po = pts.front ();
    std::vector<db::Vector>::iterator pw = pts.begin();
    for (std::vector<db::Vector>::iterator p = pw + 1; p != pts.end (); ++p) {
      *pw++ = *p - po;
    }
    pts.erase (pw, pts.end ());

    db::IrregularRepetition *rep_base = new db::IrregularRepetition ();
    rep_base->points ().swap (pts);
    db::Repetition array_rep (rep_base);

    if (rep != db::Repetition ()) {
      for (db::RepetitionIterator r = rep.begin (); ! r.at_end (); ++r) {
        write_inst_with_rep (inst, prop_id, *r + po, array_rep);
      }
    } else {
      write_inst_with_rep (inst, prop_id, po, array_rep);
    }

  } else if (inst.is_regular_array (a, b, amax, bmax) && (amax > 1 || bmax > 1)) {

    //  we cannot use the repetition - instead we write every single instance and use the repetition 
    //  for the array information
    
    db::Repetition array_rep (new db::RegularRepetition (a, b, amax, bmax));

    if (rep != db::Repetition ()) {
      for (db::RepetitionIterator r = rep.begin (); ! r.at_end (); ++r) {
        write_inst_with_rep (inst, prop_id, *r, array_rep);
      }
    } else {
      write_inst_with_rep (inst, prop_id, db::Vector (), array_rep);
    }

  } else {
    write_inst_with_rep (inst, prop_id, db::Vector (), rep);
  }
}

void 
OASISWriter::write_insts (const std::set <db::cell_index_type> &cell_set)
{
  int level = m_options.compression_level;

  //  use compression 0 for the instances - this preserves the arrays and does not create new ones, the
  //  remaining ones are compressed into irregular arrays
  Compressor<db::CellInstArray> inst_compressor (0);
  Compressor<db::CellInstArrayWithProperties> inst_with_properties_compressor (0);

  db::Repetition single_rep;

  //  Collect all instances 
  for (db::Cell::const_iterator inst_iterator = mp_cell->begin (); ! inst_iterator.at_end (); ++inst_iterator) {

    if (cell_set.find (inst_iterator->cell_index ()) != cell_set.end ()) {

      db::properties_id_type prop_id = inst_iterator->prop_id ();

      if (level <= 0) {

        //  no compression -> just write
        write (inst_iterator->cell_inst (), prop_id, single_rep);

      } else {

        //  reduce by displacement
        db::CellInstArray inst_array = inst_iterator->cell_inst ();
        db::Vector disp = inst_array.front ().disp ();
        inst_array.transform (db::Trans (-disp));

        if (prop_id != 0) {
          inst_with_properties_compressor.add (db::CellInstArrayWithProperties (inst_array, prop_id), disp);
        } else {
          inst_compressor.add (inst_array, disp);
        }

      }

    }

  }

  inst_compressor.flush (this);
  inst_with_properties_compressor.flush (this);
}

void
OASISWriter::write_props (db::properties_id_type prop_id)
{
  std::vector<tl::Variant> pv_list; 

  const db::PropertiesRepository::properties_set &props = mp_layout->properties_repository ().properties (prop_id);

  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {

    m_progress.set (mp_stream->pos ());

    const tl::Variant &name = mp_layout->properties_repository ().prop_name (p->first);

    const char *name_str = s_gds_property_name;
    bool sflag = true;

    pv_list.clear ();
    const std::vector<tl::Variant> *pvl = &pv_list;

    if (! make_gds_property (name)) {

      name_str = name.to_string ();
      sflag = false;

      if (p->second.is_list ()) {
        pvl = &p->second.get_list ();
      } else if (!p->second.is_nil ()) {
        pv_list.reserve (1);
        pv_list.push_back (p->second);
      }

    } else {

      pv_list.reserve (2);
      pv_list.push_back (name.to_ulong ());
      pv_list.push_back (p->second.to_string ());

    }

    write_property_def (name_str, *pvl, sflag);

  }
}

void
OASISWriter::write_property_def (const char *name_str, const tl::Variant &pv, bool sflag)
{ 
  std::vector<tl::Variant> pvl;
  pvl.reserve (1);
  pvl.push_back (pv);
  write_property_def (name_str, pvl, sflag);
}

void
OASISWriter::write_property_def (const char *name_str, const std::vector<tl::Variant> &pvl, bool sflag)
{ 
  bool same_name = (mm_last_property_name == name_str);
  bool same_value = (mm_last_value_list == pvl);
  bool same_sflag = (mm_last_property_is_sprop == sflag);

  if (same_name && same_value && same_sflag) {
    write_record_id (29); // repeat property 
  } else {

    write_record_id (28);

    unsigned char info = sflag ? 1 : 0;
    if (same_value) {
      info |= 0x08;
    } else {
      if (pvl.size () >= 15) {
        info |= 0xf0;
      } else {
        info |= ((unsigned char)pvl.size ()) << 4;
      }
    }

    if (! same_name) {

      std::map <std::string, unsigned long>::const_iterator pni = m_propnames.find (name_str);

      //  In strict mode always write property ID's: before we have issued the table we can 
      //  create new ID's.
      if (pni == m_propnames.end () && m_options.strict_mode) {
        tl_assert (! m_proptables_written);
        pni = m_propnames.insert (std::make_pair (name_str, m_propname_id++)).first;
      }

      if (pni == m_propnames.end ()) {
        //  write the name itself, if not found in the property repository
        write_byte (info | 0x04);
        write_nstring (name_str);
      } else {
        //  write the property ID
        write_byte (info | 0x06);
        write (pni->second);
      }

      mm_last_property_name = name_str;

    } else {
      write_byte (info);
    }

    if (! same_value) {

      if (pvl.size () >= 15) {
        write ((unsigned long) pvl.size ());
      }

      //  write property values
      for (unsigned long i = 0; i < pvl.size (); ++i) {

        const tl::Variant &v = pvl[i];

        if (v.is_double ()) {

          write (v.to_double ());

        } else if (v.is_longlong ()) {

          write_byte (9);
          write (v.to_longlong ());

        } else if (v.is_ulonglong ()) {

          write_byte (8);
          write (v.to_ulonglong ());

        } else if (v.is_long ()) {

          write_byte (9);
          write (v.to_long ());

        } else if (v.is_ulong ()) {

          write_byte (8);
          write (v.to_ulong ());

        } else {

          const char *pvs = v.to_string ();
          std::map <std::string, unsigned long>::const_iterator pvi = m_propstrings.find (pvs);

          //  In strict mode always write property string ID's: before we have issued the table we can 
          //  create new ID's.
          if (pvi == m_propstrings.end () && m_options.strict_mode) {
            tl_assert (! m_proptables_written);
            pvi = m_propstrings.insert (std::make_pair (pvs, m_propstring_id++)).first;
          }

          if (pvi != m_propstrings.end ()) {
            write_byte (13 + string_type (pvs));
            write (pvi->second);
          } else {
            write_byte (10 + string_type (pvs));
            write_bstring (pvs);
          }

        }

      }

      mm_last_value_list = pvl;

    }

    mm_last_property_is_sprop = sflag;

  }
}

void
OASISWriter::write_pointlist (const std::vector<db::Vector> &pointlist, bool for_polygons)
{
  tl_assert ((for_polygons && pointlist.size () > 1) || (! for_polygons && pointlist.size () > 0));

  //  determine type: 0 (manhattan, horizontal first), 1 (manhattan, vert. first), -1 other
  db::Vector plast (0, 0);
  int type = -1;
  int hvlast = -1;
  for (std::vector<db::Vector>::const_iterator p = pointlist.begin (); p != pointlist.end (); ++p) {
    int hv = -1;
    if (p->x () == plast.x ()) {
      hv = 1;
    } else if (p->y () == plast.y ()) {
      hv = 0;
    } else {
      type = -1;
      break;
    }
    if (type == -1) {
      type = hv;
    } else if (hv != !hvlast) {
      type = -1;
      break;
    }
    hvlast = hv;
    plast = *p;
  }

  //  test last displacement for polygons
  if (for_polygons && type >= 0) {
    if (hvlast != type) {
      type = -1;
    } else if (plast.x () == 0) {
      if (hvlast != 0) {
        type = -1;
      }
    } else if (plast.y () == 0) {
      if (hvlast != 1) {
        type = -1;
      }
    } else {
      type = -1;
    }
  }

  if (type == 0 || type == 1) {

    //  manhattan pointlist
    write_byte (type);
    size_t implicit = for_polygons ? 1 : 0;
    write ((unsigned long) (pointlist.size () - implicit));

    db::Vector plast (0, 0);
    for (std::vector<db::Vector>::const_iterator p = pointlist.begin (); p != pointlist.end () - implicit; ++p) {
      db::Coord x = (m_sf == 1.0 ? p->x () : safe_scale (m_sf, p->x ()));
      db::Coord y = (m_sf == 1.0 ? p->y () : safe_scale (m_sf, p->y ()));
      db::Coord d = x - plast.x ();
      if (d == 0) {
        d = y - plast.y ();
      }
      write (d);
      plast = db::Vector (x, y);
    }

  } else {

    //  generic pointlist
    write_byte (4);
    write ((unsigned long) pointlist.size ());
    db::Vector plast (0, 0);
    if (m_sf == 1.0) {
      for (std::vector<db::Vector>::const_iterator p = pointlist.begin (); p != pointlist.end (); ++p) {
        write_gdelta (*p - plast, 1.0);
        plast = *p;
      }
    } else {
      for (std::vector<db::Vector>::const_iterator p = pointlist.begin (); p != pointlist.end (); ++p) {
        db::Vector ps (safe_scale (m_sf, p->x ()), safe_scale (m_sf, p->y ()));
        write_gdelta (ps - plast, 1.0);
        plast = ps;
      }
    }

  }
}

void 
OASISWriter::write (const db::Text &text, db::properties_id_type prop_id, const db::Repetition &rep)
{
  m_progress.set (mp_stream->pos ());

  db::Trans trans = text.trans ();

  unsigned long text_id = 0;
  std::map <std::string, unsigned long>::const_iterator ts = m_textstrings.find (text.string ());
  if (ts == m_textstrings.end ()) {
    text_id = m_textstring_id++;
    m_textstrings.insert (std::make_pair (text.string (), text_id));
  } else {
    text_id = ts->second;
  }

  unsigned char info = 0x20;

  if (mm_text_string != text.string ()) {
    info |= 0x40;
  }
  if (mm_textlayer != m_layer) {
    info |= 0x01;
  }
  if (mm_texttype != m_datatype) {
    info |= 0x02;
  }
  if (mm_text_x != trans.disp ().x ()) {
    info |= 0x10;
  }
  if (mm_text_y != trans.disp ().y ()) {
    info |= 0x08;
  }
  if (! rep.is_singular ()) {
    info |= 0x04;
  }

  write_record_id (19);
  write_byte (info);
  if (info & 0x40) {
    mm_text_string = text.string ();
    write ((unsigned long) text_id);
  }
  if (info & 0x01) {
    mm_textlayer = m_layer;
    write ((unsigned long) m_layer);
  }
  if (info & 0x02) {
    mm_texttype = m_datatype;
    write ((unsigned long) m_datatype);
  }
  if (info & 0x10) {
    mm_text_x = trans.disp ().x ();
    write_coord (mm_text_x.get ());
  }
  if (info & 0x08) {
    mm_text_y = trans.disp ().y ();
    write_coord (mm_text_y.get ());
  }

  if (info & 0x04) {
    write (rep);
  }

  if (prop_id != 0) {
    write_props (prop_id);
  }
}

void 
OASISWriter::write (const db::SimplePolygon &polygon, db::properties_id_type prop_id, const db::Repetition &rep)
{
  m_progress.set (mp_stream->pos ());

  //  TODO: how to deal with max vertex count?
 
  db::Polygon::polygon_contour_iterator e = polygon.begin_hull ();

  //  don't write empty polygons
  if (e == polygon.end_hull ()) {
    return;
  }

  db::Point start = *e;
  m_pointlist.clear ();
  while (++e != polygon.end_hull ()) {
    m_pointlist.push_back (*e - start);
  }

  if (m_pointlist.size () < 2) {
    std::string msg = tl::to_string (tr ("Polygons with less than three points cannot be written to OASIS files (cell ")) + mp_layout->cell_name (mp_cell->cell_index ()) + tl::to_string (tr (", position ")) + tl::to_string (start.x ()) + ", " + tl::to_string (start.y ()) + " DBU)";
    if (m_options.permissive) {
      tl::warn << msg;
      return;
    } else {
      throw tl::Exception (msg);
    }
  }

  unsigned char info = 0x00;

  if (mm_layer != m_layer) {
    info |= 0x01;
  }
  if (mm_datatype != m_datatype) {
    info |= 0x02;
  }
  if (mm_geometry_x != start.x ()) {
    info |= 0x10;
  }
  if (mm_geometry_y != start.y ()) {
    info |= 0x08;
  }
  if (mm_polygon_point_list != m_pointlist) {
    info |= 0x20;
  }
  if (! rep.is_singular ()) {
    info |= 0x04;
  }

  write_record_id (21);
  write_byte (info);

  if (info & 0x01) {
    mm_layer = m_layer;
    write ((unsigned long) m_layer);
  }
  if (info & 0x02) {
    mm_datatype = m_datatype;
    write ((unsigned long) m_datatype);
  }
  if (info & 0x20) {
    mm_polygon_point_list.swap (m_pointlist);
    write_pointlist (mm_polygon_point_list.get (), true /*for polygons*/);
  }
  if (info & 0x10) {
    mm_geometry_x = start.x ();
    write_coord (start.x ());
  }
  if (info & 0x08) {
    mm_geometry_y = start.y ();
    write_coord (start.y ());
  }
  if (info & 0x04) {
    write (rep);
  }

  if (prop_id != 0) {
    write_props (prop_id);
  }
}

void 
OASISWriter::write (const db::Polygon &polygon, db::properties_id_type prop_id, const db::Repetition &rep)
{
  if (polygon.holes () > 0) {

    //  resolve holes ...
    std::vector<db::Polygon> polygons;

    db::EdgeProcessor ep;
    ep.insert_sequence (polygon.begin_edge ());
    db::PolygonContainer pc (polygons);
    db::PolygonGenerator out (pc, true /*resolve holes*/, false /*max coherence*/);
    db::SimpleMerge op;
    ep.process (out, op);

    for (std::vector<db::Polygon>::const_iterator p = polygons.begin (); p != polygons.end (); ++p) {
      write (*p, prop_id, rep);
    }
  
  } else {

    m_progress.set (mp_stream->pos ());

    //  TODO: how to deal with max vertex count?
   
    db::Polygon::polygon_contour_iterator e = polygon.begin_hull ();

    //  don't write empty polygons
    if (e == polygon.end_hull ()) {
      return;
    }

    db::Point start = *e;
    m_pointlist.clear ();
    while (++e != polygon.end_hull ()) {
      m_pointlist.push_back (*e - start);
    }

    if (m_pointlist.size () < 2) {
      std::string msg = tl::to_string (tr ("Polygons with less than three points cannot be written to OASIS files (cell ")) + mp_layout->cell_name (mp_cell->cell_index ()) + tl::to_string (tr (", position ")) + tl::to_string (start.x ()) + ", " + tl::to_string (start.y ()) + " DBU)";
      if (m_options.permissive) {
        tl::warn << msg;
        return;
      } else {
        throw tl::Exception (msg);
      }
    }

    unsigned char info = 0x00;

    if (mm_layer != m_layer) {
      info |= 0x01;
    }
    if (mm_datatype != m_datatype) {
      info |= 0x02;
    }
    if (mm_geometry_x != start.x ()) {
      info |= 0x10;
    }
    if (mm_geometry_y != start.y ()) {
      info |= 0x08;
    }
    if (mm_polygon_point_list != m_pointlist) {
      info |= 0x20;
    }
    if (! rep.is_singular ()) {
      info |= 0x04;
    }

    write_record_id (21);
    write_byte (info);

    if (info & 0x01) {
      mm_layer = m_layer;
      write ((unsigned long) m_layer);
    }
    if (info & 0x02) {
      mm_datatype = m_datatype;
      write ((unsigned long) m_datatype);
    }
    if (info & 0x20) {
      mm_polygon_point_list.swap (m_pointlist);
      write_pointlist (mm_polygon_point_list.get (), true /*for polygons*/);
    }
    if (info & 0x10) {
      mm_geometry_x = start.x ();
      write_coord (start.x ());
    }
    if (info & 0x08) {
      mm_geometry_y = start.y ();
      write_coord (start.y ());
    }
    if (info & 0x04) {
      write (rep);
    }

    if (prop_id != 0) {
      write_props (prop_id);
    }

  }
}

void 
OASISWriter::write (const db::Box &box, db::properties_id_type prop_id, const db::Repetition &rep)
{
  m_progress.set (mp_stream->pos ());

  unsigned char info = 0x00;
  
  if (mm_layer != m_layer) {
    info |= 0x01;
  }
  if (mm_datatype != m_datatype) {
    info |= 0x02;
  }

  if (box.width () == box.height ()) {
    info |= 0x80;  // square
  } else {
    if (mm_geometry_h != box.height ()) {
      info |= 0x20;
    }
  }
  if (mm_geometry_w != box.width ()) {
    info |= 0x40;
  }

  if (mm_geometry_x != box.left ()) {
    info |= 0x10;
  }
  if (mm_geometry_y != box.bottom ()) {
    info |= 0x08;
  }
  
  if (! rep.is_singular ()) {
    info |= 0x04;
  }

  write_record_id (20);
  write_byte (info);

  if (info & 0x01) {
    mm_layer = m_layer;
    write ((unsigned long) m_layer);
  }
  if (info & 0x02) {
    mm_datatype = m_datatype;
    write ((unsigned long) m_datatype);
  }

  mm_geometry_w = box.width ();
  mm_geometry_h = box.height ();

  if (info & 0x40) {
    write_ucoord (mm_geometry_w.get ());
  }
  if (info & 0x20) {
    write_ucoord (mm_geometry_h.get ());
  }

  if (info & 0x10) {
    mm_geometry_x = box.left ();
    write_coord (mm_geometry_x.get ());
  }
  if (info & 0x08) {
    mm_geometry_y = box.bottom ();
    write_coord (mm_geometry_y.get ());
  }

  if (info & 0x04) {
    write (rep);
  }

  if (prop_id != 0) {
    write_props (prop_id);
  }
}

void 
OASISWriter::write (const db::Path &path, db::properties_id_type prop_id, const db::Repetition &rep)
{
  typedef db::coord_traits<db::Coord>::distance_type ucoord;

  //  don't write empty paths
  if (path.begin () == path.end ()) {
    return;
  }

  m_progress.set (mp_stream->pos ());

  std::pair<db::Path::coord_type, db::Path::coord_type> ext (0, 0);
  //  for round paths, circles are placed to mimic the extensions
  if (! path.round ()) {
    ext = path.extensions ();
    //  Because we scale the width already, we also need to scale the extensions for comparing them
    ext.first = safe_scale (m_sf, ext.first);
    ext.second = safe_scale (m_sf, ext.second);
  }

  db::Path::iterator e = path.begin ();
  db::Point start = *e;

  m_pointlist.clear ();
  while (++e != path.end ()) {
    m_pointlist.push_back (*e - start);
  }

  if (m_pointlist.empty ()) {

    if (path.round ()) {

      db::Coord w = safe_scale (m_sf, path.width ());
      db::Coord hw = w / 2;
      if (hw * 2 != w) {
        std::string msg = tl::to_string (tr ("Circles with odd diameter cannot be written to OASIS files (cell ")) + mp_layout->cell_name (mp_cell->cell_index ()) + tl::to_string (tr (", position ")) + tl::to_string (start.x ()) + ", " + tl::to_string (start.y ()) + " DBU)";
        if (m_options.permissive) {
          tl::warn << msg << " - " << tl::to_string (tr ("circle diameter is rounded"));
        } else {
          throw tl::Exception (msg);
        }
      }

      unsigned char info = 0;
      if (mm_layer != m_layer) {
        info |= 0x01;
      }
      if (mm_datatype != m_datatype) {
        info |= 0x02;
      }
      if (mm_circle_radius != hw) {
        info |= 0x20;
      }
      if (mm_geometry_x != start.x ()) {
        info |= 0x10;
      }
      if (mm_geometry_y != start.y ()) {
        info |= 0x08;
      }

      if (! rep.is_singular ()) {
        info |= 0x04;
      }

      write_record_id (27);
      write_byte (info);

      if (info & 0x01) {
        mm_layer = m_layer;
        write ((unsigned long) m_layer);
      }
      if (info & 0x02) {
        mm_datatype = m_datatype;
        write ((unsigned long) m_datatype);
      }
      if (info & 0x20) {
        mm_circle_radius = hw;
        //  NOTE: the radius has already been scaled, so we don't use write_ucoord here
        write ((ucoord) hw);
      }
      if (info & 0x10) {
        mm_geometry_x = start.x ();
        write_coord (start.x ());
      }
      if (info & 0x08) {
        mm_geometry_y = start.y ();
        write_coord (start.y ());
      }

      if (info & 0x04) {
        write (rep);
      }

      if (prop_id != 0) {
        write_props (prop_id);
      }

    } else {
      //  Single-point paths are translated into polygons
      write (path.polygon (), prop_id, rep);
    }

  } else {

    db::Coord w = safe_scale (m_sf, path.width ());
    db::Coord hw = w / 2;
    if (hw * 2 != w) {
      std::string msg = tl::to_string (tr ("Paths with odd width cannot be written to OASIS files (cell ")) + mp_layout->cell_name (mp_cell->cell_index ()) + tl::to_string (tr (", position ")) + tl::to_string (start.x ()) + ", " + tl::to_string (start.y ()) + " DBU)";
      if (m_options.permissive) {
        tl::warn << msg << " - " << tl::sprintf (tl::to_string (tr ("path width is rounded from %d to %d DBU")), w, hw * 2);
      } else {
        throw tl::Exception (msg);
      }
    }

    db::Point end = start + m_pointlist.back ();

    unsigned char info = 0x00;

    if (mm_layer != m_layer) {
      info |= 0x01;
    }
    if (mm_datatype != m_datatype) {
      info |= 0x02;
    }
    if (mm_geometry_x != start.x ()) {
      info |= 0x10;
    }
    if (mm_geometry_y != start.y ()) {
      info |= 0x08;
    }
    if (mm_path_point_list != m_pointlist) {
      info |= 0x20;
    }
    if (mm_path_start_extension != ext.first || mm_path_end_extension != ext.second) {
      info |= 0x80;
    }
    if (mm_path_halfwidth != hw) {
      info |= 0x40;
    }

    if (! rep.is_singular ()) {
      info |= 0x04;
    }

    write_record_id (22);
    write_byte (info);

    if (info & 0x01) {
      mm_layer = m_layer;
      write ((unsigned long) m_layer);
    }
    if (info & 0x02) {
      mm_datatype = m_datatype;
      write ((unsigned long) m_datatype);
    }
    if (info & 0x40) {
      mm_path_halfwidth = hw;
      //  NOTE: the half-width has already been scaled, so we don't use write_ucoord here
      write ((ucoord) hw);
    }

    if (info & 0x80) {

      unsigned char ext_scheme = 0;
      if (mm_path_start_extension == ext.first) {
        //  00
      } else if (ext.first == 0) {
        ext_scheme |= 0x04;
      } else if (ext.first == hw) {
        ext_scheme |= 0x08;
      } else {
        ext_scheme |= 0x0c;
      }
      if (mm_path_end_extension == ext.second) {
        //  00
      } else if (ext.second == 0) {
        ext_scheme |= 0x01;
      } else if (ext.second == hw) {
        ext_scheme |= 0x02;
      } else {
        ext_scheme |= 0x03;
      }

      write_byte (ext_scheme);

      if ((ext_scheme & 0x0c) == 0x0c) {
        //  NOTE: ext.first is already scaled, so we don't use write_coord
        write (ext.first);
      }
      if ((ext_scheme & 0x03) == 0x03) {
        //  NOTE: ext.second is already scaled, so we don't use write_coord
        write (ext.second);
      }

      mm_path_start_extension = ext.first;
      mm_path_end_extension = ext.second;

    }

    if (info & 0x20) {
      mm_path_point_list.swap (m_pointlist);
      write_pointlist (mm_path_point_list.get (), false /*=for paths*/);
    }
    if (info & 0x10) {
      mm_geometry_x = start.x ();
      write_coord (start.x ());
    }
    if (info & 0x08) {
      mm_geometry_y = start.y ();
      write_coord (start.y ());
    }

    if (info & 0x04) {
      write (rep);
    }

    if (prop_id != 0) {
      write_props (prop_id);
    }

    if (path.round ()) {

      //  write two circles at the path ends to mimic the round path ends.

      unsigned char info = 0;
      if (mm_circle_radius != hw) {
        info |= 0x20;
      }
      if (mm_geometry_x != start.x ()) {
        info |= 0x10;
      }
      if (mm_geometry_y != start.y ()) {
        info |= 0x08;
      }

      if (! rep.is_singular ()) {
        info |= 0x04;
      }

      write_byte (27);
      write_byte (info);

      if (info & 0x20) {
        mm_circle_radius = hw;
        //  NOTE: the half-width has already been scaled, so we don't use write_ucoord here
        write ((ucoord) hw);
      }
      if (info & 0x10) {
        mm_geometry_x = start.x ();
        write_coord (start.x ());
      }
      if (info & 0x08) {
        mm_geometry_y = start.y ();
        write_coord (start.y ());
      }

      if (info & 0x04) {
        write (rep);
      }

      if (prop_id != 0) {
        write_props (prop_id);
      }

      info = 0;
      if (mm_geometry_x != end.x ()) {
        info |= 0x10;
      }
      if (mm_geometry_y != end.y ()) {
        info |= 0x08;
      }

      if (! rep.is_singular ()) {
        info |= 0x04;
      }

      write_byte (27);
      write_byte (info);

      if (info & 0x10) {
        mm_geometry_x = end.x ();
        write_coord (end.x ());
      }
      if (info & 0x08) {
        mm_geometry_y = end.y ();
        write_coord (end.y ());
      }

      if (info & 0x04) {
        write (rep);
      }

      if (prop_id != 0) {
        write_props (prop_id);
      }

    }

  }
}

void 
OASISWriter::write (const db::Edge &edge, db::properties_id_type prop_id, const db::Repetition &rep)
{
  m_progress.set (mp_stream->pos ());

  m_pointlist.reserve (1);
  m_pointlist.erase (m_pointlist.begin (), m_pointlist.end ());
  m_pointlist.push_back (edge.p2 () - edge.p1 ());

  unsigned char info = 0x00;

  if (mm_layer != m_layer) {
    info |= 0x01;
  }
  if (mm_datatype != m_datatype) {
    info |= 0x02;
  }
  if (! rep.is_singular ()) {
    info |= 0x04;
  }
  if (mm_geometry_x != edge.p1 ().x ()) {
    info |= 0x10;
  }
  if (mm_geometry_y != edge.p1 ().y ()) {
    info |= 0x08;
  }
  if (mm_path_point_list != m_pointlist) {
    info |= 0x20;
  }
  if (mm_path_start_extension != 0 || mm_path_end_extension != 0) {
    info |= 0x80;
  }
  if (mm_path_halfwidth != 0) {
    info |= 0x40;
  }

  write_record_id (22);
  write_byte (info);

  if (info & 0x01) {
    mm_layer = m_layer;
    write ((unsigned long) m_layer);
  }
  if (info & 0x02) {
    mm_datatype = m_datatype;
    write ((unsigned long) m_datatype);
  }
  if (info & 0x40) {
    mm_path_halfwidth = 0;
    write (0);
  }

  if (info & 0x80) {
    write_byte (0x05);  // flush
    mm_path_start_extension = 0;
    mm_path_end_extension = 0;
  }

  if (info & 0x20) {
    mm_path_point_list = m_pointlist;
    write_pointlist (m_pointlist, false /*=for paths*/);
  }
  if (info & 0x10) {
    mm_geometry_x = edge.p1 ().x ();
    write_coord (edge.p1 ().x ());
  }
  if (info & 0x08) {
    mm_geometry_y = edge.p1 ().y ();
    write_coord (edge.p1 ().y ());
  }
  if (info & 0x04) {
    write (rep);
  }

  if (prop_id != 0) {
    write_props (prop_id);
  }
}

void 
OASISWriter::write_shapes (const db::LayerProperties &lprops, const db::Shapes &shapes)
{
  int level = m_options.compression_level;
  bool recompress = m_options.recompress;

  m_layer = lprops.layer;
  m_datatype = lprops.datatype;

  Compressor<db::Path> path_compressor (level);
  Compressor<db::SimplePolygon> simple_polygon_compressor (level);
  Compressor<db::Polygon> polygon_compressor (level);
  Compressor<db::Edge> edge_compressor (level);
  Compressor<db::Box> box_compressor (level);
  Compressor<db::Text> text_compressor (level);

  Compressor<db::PathWithProperties> path_with_properties_compressor (level);
  Compressor<db::SimplePolygonWithProperties> simple_polygon_with_properties_compressor (level);
  Compressor<db::PolygonWithProperties> polygon_with_properties_compressor (level);
  Compressor<db::EdgeWithProperties> edge_with_properties_compressor (level);
  Compressor<db::BoxWithProperties> box_with_properties_compressor (level);
  Compressor<db::TextWithProperties> text_with_properties_compressor (level);

  db::Repetition single_rep;

  for (db::ShapeIterator shape = shapes.begin (db::ShapeIterator::All); ! shape.at_end (); ) {

    if (level <= 0) {

      if (shape->is_simple_polygon ()) {

        db::SimplePolygon sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), single_rep);

      } else if (shape->is_polygon ()) {

        db::Polygon sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), single_rep);

      } else if (shape->is_path ()) {

        db::Path sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), single_rep);

      } else if (shape->is_text ()) {

        db::Text sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), single_rep);

      } else if (shape->is_edge ()) {

        db::Edge sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), single_rep);

      } else if (shape->is_box ()) {

        db::Box sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), single_rep);

      } else if (shape->is_user_object ()) {
        // ignore
      } else {
        tl_assert (false); // unknown shape type
      }

      ++shape;

    } else if (! recompress && shape.in_array ()) {

      db::Repetition rep;
      create_repetition (shape.array (), rep);

      if (shape->is_simple_polygon ()) {

        db::SimplePolygon sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), rep);

      } else if (shape->is_polygon ()) {

        db::Polygon sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), rep);

      } else if (shape->is_path ()) {

        db::Path sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), rep);

      } else if (shape->is_text ()) {

        db::Text sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), rep);

      } else if (shape->is_edge ()) {

        db::Edge sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), rep);

      } else if (shape->is_box ()) {

        db::Box sh;
        shape->instantiate (sh);
        write (sh, shape->prop_id (), rep);

      } else if (shape->is_user_object ()) {
        // ignore
      } else {
        tl_assert (false); // unknown shape type
      }

      shape.finish_array ();

    } else {

      switch (shape->type ()) {
      case db::Shape::Polygon:

        if (shape->has_prop_id ()) {
          db::PolygonWithProperties polygon = *shape->basic_ptr (db::PolygonWithProperties::tag ());
          db::Disp tr;
          polygon.reduce (tr);
          polygon_with_properties_compressor.add (polygon, tr.disp ());
        } else {
          db::Polygon polygon = *shape->basic_ptr (db::Polygon::tag ());
          db::Disp tr;
          polygon.reduce (tr);
          polygon_compressor.add (polygon, tr.disp ());
        }

        break;

      case db::Shape::PolygonRef:

        if (shape->has_prop_id ()) {
          db::object_with_properties<db::PolygonRef> polygon_ref = *shape->basic_ptr (db::object_with_properties<db::PolygonRef>::tag ());
          db::PolygonWithProperties polygon (polygon_ref.obj (), polygon_ref.properties_id ());
          polygon_with_properties_compressor.add (polygon, polygon_ref.trans ().disp ());
        } else {
          const db::PolygonRef &polygon_ref = *shape->basic_ptr (db::PolygonRef::tag ());
          polygon_compressor.add (polygon_ref.obj (), polygon_ref.trans ().disp ());
        }

        break;

      case db::Shape::PolygonPtrArrayMember:

        if (shape->has_prop_id ()) {
          db::object_with_properties<db::Shape::polygon_ptr_array_type> polygon_ref = *shape->basic_ptr (db::object_with_properties<db::Shape::polygon_ptr_array_type>::tag ());
          db::PolygonWithProperties polygon (polygon_ref.object ().obj (), polygon_ref.properties_id ());
          polygon_with_properties_compressor.add (polygon, shape->array_trans ().disp ());
        } else {
          const db::Shape::polygon_ptr_array_type &polygon_ref = *shape->basic_ptr (db::Shape::polygon_ptr_array_type::tag ());
          polygon_compressor.add (polygon_ref.object ().obj (), shape->array_trans ().disp ());
        }

        break;

      case db::Shape::SimplePolygon:

        if (shape->has_prop_id ()) {
          db::SimplePolygonWithProperties simple_polygon = *shape->basic_ptr (db::SimplePolygonWithProperties::tag ());
          db::Disp tr;
          simple_polygon.reduce (tr);
          simple_polygon_with_properties_compressor.add (simple_polygon, tr.disp ());
        } else {
          db::SimplePolygon simple_polygon = *shape->basic_ptr (db::SimplePolygon::tag ());
          db::Disp tr;
          simple_polygon.reduce (tr);
          simple_polygon_compressor.add (simple_polygon, tr.disp ());
        }

        break;

      case db::Shape::SimplePolygonRef:

        if (shape->has_prop_id ()) {
          db::object_with_properties<db::SimplePolygonRef> polygon_ref = *shape->basic_ptr (db::object_with_properties<db::SimplePolygonRef>::tag ());
          db::SimplePolygonWithProperties polygon (polygon_ref.obj (), polygon_ref.properties_id ());
          simple_polygon_with_properties_compressor.add (polygon, polygon_ref.trans ().disp ());
        } else {
          const db::SimplePolygonRef &polygon_ref = *shape->basic_ptr (db::SimplePolygonRef::tag ());
          simple_polygon_compressor.add (polygon_ref.obj (), polygon_ref.trans ().disp ());
        }

        break;

      case db::Shape::SimplePolygonPtrArrayMember:
           
        if (shape->has_prop_id ()) {
          db::object_with_properties<db::Shape::simple_polygon_ptr_array_type> simple_polygon_ref = *shape->basic_ptr (db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>::tag ());
          db::SimplePolygonWithProperties simple_polygon (simple_polygon_ref.object ().obj (), simple_polygon_ref.properties_id ());
          simple_polygon_with_properties_compressor.add (simple_polygon, shape->array_trans ().disp ());
        } else {
          const db::Shape::simple_polygon_ptr_array_type &simple_polygon_ref = *shape->basic_ptr (db::Shape::simple_polygon_ptr_array_type::tag ());
          simple_polygon_compressor.add (simple_polygon_ref.object ().obj (), shape->array_trans ().disp ());
        }

        break;

      case db::Shape::Edge:

        if (shape->has_prop_id ()) {
          db::EdgeWithProperties edge = *shape->basic_ptr (db::EdgeWithProperties::tag ());
          db::Disp tr;
          edge.reduce (tr);
          edge_with_properties_compressor.add (edge, tr.disp ());
        } else {
          db::Edge edge = *shape->basic_ptr (db::Edge::tag ());
          db::Disp tr;
          edge.reduce (tr);
          edge_compressor.add (edge, tr.disp ());
        }

        break;

      case db::Shape::Path:

        if (shape->has_prop_id ()) {
          db::PathWithProperties path = *shape->basic_ptr (db::PathWithProperties::tag ());
          db::Disp tr;
          path.reduce (tr);
          path_with_properties_compressor.add (path, tr.disp ());
        } else {
          db::Path path = *shape->basic_ptr (db::Path::tag ());
          db::Disp tr;
          path.reduce (tr);
          path_compressor.add (path, tr.disp ());
        }

        break;

      case db::Shape::PathRef:

        if (shape->has_prop_id ()) {
          db::object_with_properties<db::PathRef> path_ref = *shape->basic_ptr (db::object_with_properties<db::PathRef>::tag ());
          db::PathWithProperties path (path_ref.obj (), path_ref.properties_id ());
          path_with_properties_compressor.add (path, path_ref.trans ().disp ());
        } else {
          const db::PathRef &path_ref = *shape->basic_ptr (db::PathRef::tag ());
          path_compressor.add (path_ref.obj (), path_ref.trans ().disp ());
        }

        break;

      case db::Shape::PathPtrArrayMember:

        if (shape->has_prop_id ()) {
          db::object_with_properties<db::Shape::path_ptr_array_type> path_ref = *shape->basic_ptr (db::object_with_properties<db::Shape::path_ptr_array_type>::tag ());
          db::PathWithProperties path (path_ref.object ().obj (), path_ref.properties_id ());
          path_with_properties_compressor.add (path, shape->array_trans ().disp ());
        } else {
          const db::Shape::path_ptr_array_type &path_ref = *shape->basic_ptr (db::Shape::path_ptr_array_type::tag ());
          path_compressor.add (path_ref.object ().obj (), shape->array_trans ().disp ());
        }

        break;

      case db::Shape::Box:

        if (shape->has_prop_id ()) {
          db::BoxWithProperties box = *shape->basic_ptr (db::BoxWithProperties::tag ());
          db::Disp tr;
          box.reduce (tr);
          box_with_properties_compressor.add (box, tr.disp ());
        } else {
          db::Box box = *shape->basic_ptr (db::Box::tag ());
          db::Disp tr;
          box.reduce (tr);
          box_compressor.add (box, tr.disp ());
        }

        break;

      case db::Shape::BoxArray:
      case db::Shape::BoxArrayMember:
      case db::Shape::ShortBox:
      case db::Shape::ShortBoxArrayMember:

        if (shape->has_prop_id ()) {
          db::BoxWithProperties box;
          shape->instantiate (box);
          box.properties_id (shape->prop_id ());
          db::Disp tr;
          box.reduce (tr);
          box_with_properties_compressor.add (box, tr.disp ());
        } else {
          db::Box box;
          shape->instantiate (box);
          db::Disp tr;
          box.reduce (tr);
          box_compressor.add (box, tr.disp ());
        }

        break;

      case db::Shape::Text:

        if (shape->has_prop_id ()) {
          db::TextWithProperties text = *shape->basic_ptr (db::TextWithProperties::tag ());
          db::Disp tr;
          text.reduce (tr);
          text_with_properties_compressor.add (text, tr.disp ());
        } else {
          db::Text text = *shape->basic_ptr (db::Text::tag ());
          db::Disp tr;
          text.reduce (tr);
          text_compressor.add (text, tr.disp ());
        }

        break;

      case db::Shape::TextRef:

        if (shape->has_prop_id ()) {
          db::object_with_properties<db::TextRef> text_ref = *shape->basic_ptr (db::object_with_properties<db::TextRef>::tag ());
          db::TextWithProperties text (text_ref.obj (), text_ref.properties_id ());
          text_with_properties_compressor.add (text, text_ref.trans ().disp ());
        } else {
          const db::TextRef &text_ref = *shape->basic_ptr (db::TextRef::tag ());
          text_compressor.add (text_ref.obj (), text_ref.trans ().disp ());
        }

        break;

      case db::Shape::TextPtrArrayMember:

        if (shape->has_prop_id ()) {
          db::object_with_properties<db::Shape::text_ptr_array_type> text_ref = *shape->basic_ptr (db::object_with_properties<db::Shape::text_ptr_array_type>::tag ());
          db::TextWithProperties text (text_ref.object ().obj (), text_ref.properties_id ());
          text_with_properties_compressor.add (text, shape->array_trans ().disp ());
        } else {
          const db::Shape::text_ptr_array_type &text_ref = *shape->basic_ptr (db::Shape::text_ptr_array_type::tag ());
          text_compressor.add (text_ref.object ().obj (), shape->array_trans ().disp ());
        }

        break;

      case db::Shape::UserObject:
        //  ignore.
        break;

      default:
        tl_assert (false);

      }

      ++shape;

    }

  }

  path_compressor.flush (this);
  simple_polygon_compressor.flush (this);
  polygon_compressor.flush (this);
  edge_compressor.flush (this);
  box_compressor.flush (this);
  text_compressor.flush (this);

  path_with_properties_compressor.flush (this);
  simple_polygon_with_properties_compressor.flush (this);
  polygon_with_properties_compressor.flush (this);
  edge_with_properties_compressor.flush (this);
  box_with_properties_compressor.flush (this);
  text_with_properties_compressor.flush (this);
}

}

