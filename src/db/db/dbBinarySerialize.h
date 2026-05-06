
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


#ifndef HDR_dbBinarySerialize
#define HDR_dbBinarySerialize

#include "dbCommon.h"

#include "dbPoint.h"
#include "dbVector.h"
#include "dbEdge.h"
#include "dbEdgePair.h"
#include "dbPolygon.h"
#include "dbBox.h"
#include "dbPath.h"
#include "dbTrans.h"
#include "dbText.h"
#include "dbObjectWithProperties.h"

#include "tlBinaryStream.h"
#include "tlException.h"

namespace db
{

// -----------------------------------------------------------------------------------------
//  Stream inserters

template<class C>
tl::BinaryOutputStream &write_coord (tl::BinaryOutputStream &s, C c)
{
  return s << c;
}

inline tl::BinaryOutputStream &write_coord (tl::BinaryOutputStream &s, db::Coord c)
{
  //  NOTE: for compatibility across different builds we use 64 bit coordinates always
  return s << (int64_t) c;
}

template<class C>
tl::BinaryOutputStream &write_binary (tl::BinaryOutputStream &s, const db::point<C> &pt)
{
  return write_coord (write_coord (s, pt.x ()), pt.y ());
}

template<class C>
tl::BinaryOutputStream &write_binary (tl::BinaryOutputStream &s, const db::vector<C> &v)
{
  return write_coord (write_coord (s, v.x ()), v.y ());
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::point<C> &pt)
{
  s << (uint16_t) 1; // version
  return write_binary (s, pt);
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::vector<C> &v)
{
  s << (uint16_t) 1; // version
  return write_binary (s, v);
}

template<class C>
tl::BinaryOutputStream &write_binary (tl::BinaryOutputStream &s, const db::edge<C> &e)
{
  write_binary (s, e.p1 ());
  write_binary (s, e.p2 ());
  return s;
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::edge<C> &e)
{
  s << (uint16_t) 1; // version
  return write_binary (s, e);
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::box<C> &b)
{
  s << (uint16_t) 1; // version
  write_binary (s, b.p1 ());
  write_binary (s, b.p2 ());
  return s;
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::edge_pair<C> &ep)
{
  s << (uint16_t) 1; // version
  write_binary (s, ep.first ());
  write_binary (s, ep.second ());
  return s;
}

template<class C>
tl::BinaryOutputStream &write_binary (tl::BinaryOutputStream &s, const db::polygon_contour<C> &c)
{
  s << (uint64_t) c.size ();
  for (auto i = c.begin (); i != c.end (); ++i) {
    write_binary (s, *i);
  }
  return s;
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::polygon<C> &p)
{
  s << (uint16_t) 1; // version
  s << (uint64_t) (p.holes () + 1);
  for (size_t h = 0; h < p.holes () + 1; ++h) {
    write_binary (s, p.contour (h));
  }
  return s;
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::simple_polygon<C> &p)
{
  //  NOTE: the format of polygon and simple polygon are compatible
  s << (uint16_t) 1; // version
  s << (uint64_t) 1; // number of contours
  write_binary (s, p.hull ());
  return s;
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::unit_trans<C> &)
{
  s << (uint16_t) 1; // version
  return s;
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::disp_trans<C> &t)
{
  s << (uint16_t) 1; // version
  write_binary (s, t.disp ());
  return s;
}

template<class C>
tl::BinaryOutputStream &write_binary (tl::BinaryOutputStream &s, const db::simple_trans<C> &t)
{
  s << (uint16_t) t.rot ();
  write_binary (s, t.disp ());
  return s;
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::simple_trans<C> &t)
{
  s << (uint16_t) 1; // version
  return write_binary (s, t);
}

template<class C, class D, class R>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::complex_trans<C, D, R> &t)
{
  s << (uint16_t) 1; // version
  write_binary (s, t.disp ());
  return s << (double) t.msin () << (double) t.mcos () << (double) (t.is_mirror () ? -t.mag () : t.mag ());
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::text<C> &t)
{
  s << (uint16_t) 1; // version
  s << t.string ();
  write_binary (s, t.trans ());
  write_coord (s, t.size ());
  return s << (int32_t) t.font () << (int32_t) t.halign () << (int32_t) t.valign ();
}

template<class C>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::path<C> &p)
{
  s << (uint16_t) 1; // version
  s << (uint64_t) p.points ();
  for (auto i = p.begin (); i != p.end (); ++i) {
    write_binary (s, *i);
  }
  write_coord (s, p.width ());
  write_coord (s, p.bgn_ext ());
  write_coord (s, p.end_ext ());
  s << p.round ();
  return s;
}

template<class T>
tl::BinaryOutputStream &operator<< (tl::BinaryOutputStream &s, const db::object_with_properties<T> &o)
{
  s << (uint16_t) 1; // version

  s << (const T &) o;

  const auto &ps = db::properties (o.properties_id ());

  s << (uint64_t) ps.size ();
  for (auto i = ps.begin (); i != ps.end (); ++i) {
    s << db::property_name (i->first) << db::property_value (i->second);
  }

  return s;
}

// -----------------------------------------------------------------------------------------
//  Stream extractors

class FutureBinarySerializationFormatException
  : public tl::Exception
{
public:
  FutureBinarySerializationFormatException ()
    : tl::Exception (tl::to_string (tr ("Binary serialization format version is too new for this build")))
  { }
};

template<class C>
tl::BinaryInputStream &read_coord (tl::BinaryInputStream &s, C &c)
{
  return s >> c;
}

inline tl::BinaryInputStream &read_coord (tl::BinaryInputStream &s, db::Coord &c)
{
  //  NOTE: for compatibility across different builds we use 64 bit coordinates always
  int64_t cc = 0;
  s >> cc;
  c = cc;
  return s;
}

template<class C>
tl::BinaryInputStream &read_binary (tl::BinaryInputStream &s, db::point<C> &pt)
{
  C x = 0, y = 0;
  read_coord (s, x);
  read_coord (s, y);
  pt = db::point<C> (x, y);
  return s;
}

template<class C>
tl::BinaryInputStream &read_binary (tl::BinaryInputStream &s, db::vector<C> &v)
{
  C x = 0, y = 0;
  read_coord (s, x);
  read_coord (s, y);
  v = db::vector<C> (x, y);
  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::point<C> &pt)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }
  return read_binary (s, pt);
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::vector<C> &v)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }
  return read_binary (s, v);
}

template<class C>
tl::BinaryInputStream &read_binary (tl::BinaryInputStream &s, db::edge<C> &v)
{
  db::point<C> p1, p2;
  read_binary (s, p1);
  read_binary (s, p2);
  v = db::edge<C> (p1, p2);
  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::edge<C> &e)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }
  return read_binary (s, e);
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::box<C> &b)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  db::point<C> p1, p2;
  read_binary (s, p1);
  read_binary (s, p2);
  b = db::box<C> (p1, p2);
  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::edge_pair<C> &ep)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  db::edge<C> e1, e2;
  read_binary (s, e1);
  read_binary (s, e2);
  ep = db::edge_pair<C> (e1, e2);
  return s;
}

template<class C>
tl::BinaryInputStream &read_binary (tl::BinaryInputStream &s, db::polygon_contour<C> &c, bool hole)
{
  uint64_t n = 0;
  s >> n;

  std::vector<db::point<C> > pts;
  pts.reserve (n);
  while (n-- > 0) {
    pts.push_back (db::point<C> ());
    read_binary (s, pts.back ());
  }

  //  NOTE: not normalization or modification is applied
  c.assign (pts.begin (), pts.end (), hole, false, false, false);

  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::polygon<C> &p)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  uint64_t n = 0;
  s >> n;

  p = db::polygon<C> ();
  if (n > 0) {
    p.reserve_holes (n - 1);
  }

  db::polygon_contour<C> ctr;
  for (uint64_t h = 0; h < n; ++h) {
    read_binary (s, ctr, h > 0);
    if (h == 0) {
      p.assign_hull (ctr);
    } else {
      p.insert_hole (ctr);
    }
  }

  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::simple_polygon<C> &p)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  uint64_t n = 0;
  s >> n;
  tl_assert (n == (uint64_t) 1);

  db::polygon_contour<C> ctr;
  read_binary (s, ctr, false);
  p.assign_hull (ctr);

  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::unit_trans<C> &)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::disp_trans<C> &t)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  db::vector<C> d;
  read_binary (s, d);
  t = db::disp_trans<C> (d);

  return s;
}

template<class C>
tl::BinaryInputStream &read_binary (tl::BinaryInputStream &s, db::simple_trans<C> &t)
{
  uint16_t r = 0;
  s >> r;

  db::vector<C> d;
  read_binary (s, d);

  t = db::simple_trans<C> (r, d);
  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::simple_trans<C> &t)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  return read_binary (s, t);
}

template<class C, class D, class R>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::complex_trans<C, D, R> &t)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  db::vector<R> d;
  read_binary (s, d);

  double asin, acos, mag;
  s >> asin >> acos >> mag;

  t = db::complex_trans<C, D, R> (d, asin, acos, mag);

  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::text<C> &t)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  std::string txt;
  s >> txt;

  db::simple_trans<C> tr;
  read_binary (s, tr);

  C size;
  read_coord (s, size);

  int32_t font, halign, valign;
  s >> font >> halign >> valign;

  t = db::text<C> (txt, tr, size, db::Font (font), db::HAlign (halign), db::VAlign (valign));
  return s;
}

template<class C>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::path<C> &p)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  uint64_t n = 0;
  s >> n;

  std::vector<db::point<C> > pts;
  pts.reserve (n);
  while (n-- > 0) {
    pts.push_back (db::point<C> ());
    read_binary (s, pts.back ());
  }

  C width, bgn_ext, end_ext;
  read_coord (s, width);
  read_coord (s, bgn_ext);
  read_coord (s, end_ext);

  bool round;
  s >> round;

  p = db::path<C> (pts.begin (), pts.end (), width, bgn_ext, end_ext, round);
  return s;
}

template<class T>
tl::BinaryInputStream &operator>> (tl::BinaryInputStream &s, db::object_with_properties<T> &o)
{
  uint16_t fmt = 0;
  s >> fmt;
  if (fmt > 1) {
    throw FutureBinarySerializationFormatException ();
  }

  s >> (T &) o;

  uint64_t n = 0;
  s >> n;

  db::PropertiesSet ps;

  while (n-- > 0) {
    tl::Variant name, value;
    s >> name >> value;
    ps.insert (name, value);
  }

  o.properties_id (db::properties_id (ps));

  return s;
}

// -----------------------------------------------------------------------------------------
//  Converters of objects to a binary string

template<class T>
std::vector<char> to_bytes (const T &t)
{
  tl::OutputMemoryStream osm;
  {
    tl::BinaryOutputStream os (osm);
    os << t;
  }
  return std::vector<char> (osm.data (), osm.data () + osm.size ());
}

template<class T>
std::string to_bytes_str (const T &t)
{
  tl::OutputMemoryStream osm;
  {
    tl::BinaryOutputStream os (osm);
    os << t;
  }
  return std::string (osm.data (), osm.size ());
}

template<class T>
void from_bytes (const char *data, size_t n, T &t)
{
  tl::InputMemoryStream ism (data, n);
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);
  bis >> t;
}

template<class T>
void from_bytes (const std::vector<char> &s, T &t)
{
  tl::InputMemoryStream ism (s.begin ().operator-> (), s.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);
  bis >> t;
}

}

#endif
