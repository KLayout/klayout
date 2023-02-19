
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


#include "antObject.h"
#include "antTemplate.h"
#include "antConfig.h"
#include "tlString.h"
#include "tlExpression.h"

#include <string.h>

namespace ant
{

static void
clean_points_impl (ant::Object::point_list &points)
{
  auto wp = points.begin ();
  auto p = points.begin ();
  while (p != points.end ()) {
    auto pp = p + 1;
    while (pp != points.end () && *pp == *p) {
      ++pp;
    }
    *wp++ = *p;
    p = pp;
  }

  points.erase (wp, points.end ());
}


Object::Object ()
  : m_id (-1),
    m_fmt_x ("$X"), m_fmt_y ("$Y"), m_fmt ("$D"),
    m_style (STY_ruler), m_outline (OL_diag),
    m_snap (true), m_angle_constraint (lay::AC_Global),
    m_main_position (POS_auto),
    m_main_xalign (AL_auto), m_main_yalign (AL_auto),
    m_xlabel_xalign (AL_auto), m_xlabel_yalign (AL_auto),
    m_ylabel_xalign (AL_auto), m_ylabel_yalign (AL_auto)
{
  //  .. nothing yet ..
}

Object::Object (const db::DPoint &_p1, const db::DPoint &_p2, int id, const std::string &fmt_x, const std::string &fmt_y, const std::string &fmt, style_type style, outline_type outline, bool snap, lay::angle_constraint_type angle_constraint)
  : m_id (id),
    m_fmt_x (fmt_x), m_fmt_y (fmt_y), m_fmt (fmt),
    m_style (style), m_outline (outline),
    m_snap (snap), m_angle_constraint (angle_constraint),
    m_main_position (POS_auto),
    m_main_xalign (AL_auto), m_main_yalign (AL_auto),
    m_xlabel_xalign (AL_auto), m_xlabel_yalign (AL_auto),
    m_ylabel_xalign (AL_auto), m_ylabel_yalign (AL_auto)
{
  p1 (_p1);
  p2 (_p2);
}

Object::Object (const Object::point_list &pts, int id, const std::string &fmt_x, const std::string &fmt_y, const std::string &fmt, style_type style, outline_type outline, bool snap, lay::angle_constraint_type angle_constraint)
  : m_id (id),
    m_fmt_x (fmt_x), m_fmt_y (fmt_y), m_fmt (fmt),
    m_style (style), m_outline (outline),
    m_snap (snap), m_angle_constraint (angle_constraint),
    m_main_position (POS_auto),
    m_main_xalign (AL_auto), m_main_yalign (AL_auto),
    m_xlabel_xalign (AL_auto), m_xlabel_yalign (AL_auto),
    m_ylabel_xalign (AL_auto), m_ylabel_yalign (AL_auto)
{
  set_points (pts);
}

Object::Object (const db::DPoint &_p1, const db::DPoint &_p2, int id, const ant::Template &t)
  : m_id (id),
    m_fmt_x (t.fmt_x ()), m_fmt_y (t.fmt_y ()), m_fmt (t.fmt ()),
    m_style (t.style ()), m_outline (t.outline ()),
    m_snap (t.snap ()), m_angle_constraint (t.angle_constraint ()),
    m_category (t.category ()),
    m_main_position (t.main_position ()),
    m_main_xalign (t.main_xalign ()), m_main_yalign (t.main_yalign ()),
    m_xlabel_xalign (t.xlabel_xalign ()), m_xlabel_yalign (t.xlabel_yalign ()),
    m_ylabel_xalign (t.ylabel_xalign ()), m_ylabel_yalign (t.ylabel_yalign ())
{
  p1 (_p1);
  p2 (_p2);
}

Object::Object (const Object::point_list &pts, int id, const ant::Template &t)
  : m_points (pts), m_id (id),
    m_fmt_x (t.fmt_x ()), m_fmt_y (t.fmt_y ()), m_fmt (t.fmt ()),
    m_style (t.style ()), m_outline (t.outline ()),
    m_snap (t.snap ()), m_angle_constraint (t.angle_constraint ()),
    m_category (t.category ()),
    m_main_position (t.main_position ()),
    m_main_xalign (t.main_xalign ()), m_main_yalign (t.main_yalign ()),
    m_xlabel_xalign (t.xlabel_xalign ()), m_xlabel_yalign (t.xlabel_yalign ()),
    m_ylabel_xalign (t.ylabel_xalign ()), m_ylabel_yalign (t.ylabel_yalign ())
{
  clean_points_impl (m_points);
}

Object::Object (const ant::Object &d)
  : m_points (d.m_points), m_id (d.m_id),
    m_fmt_x (d.m_fmt_x), m_fmt_y (d.m_fmt_y), m_fmt (d.m_fmt),
    m_style (d.m_style), m_outline (d.m_outline),
    m_snap (d.m_snap), m_angle_constraint (d.m_angle_constraint),
    m_category (d.m_category),
    m_main_position (d.m_main_position),
    m_main_xalign (d.m_main_xalign), m_main_yalign (d.m_main_yalign),
    m_xlabel_xalign (d.m_xlabel_xalign), m_xlabel_yalign (d.m_xlabel_yalign),
    m_ylabel_xalign (d.m_ylabel_xalign), m_ylabel_yalign (d.m_ylabel_yalign)
{
  //  .. nothing else ..
}

Object::~Object ()
{
  //  .. nothing yet ..
}


Object &
Object::operator= (const ant::Object &d)
{
  if (this != &d) {
    m_points = d.m_points;
    m_id = d.m_id;
    m_fmt_x = d.m_fmt_x;
    m_fmt_y = d.m_fmt_y;
    m_fmt = d.m_fmt;
    m_style = d.m_style;
    m_outline = d.m_outline;
    m_snap = d.m_snap;
    m_angle_constraint = d.m_angle_constraint;
    m_category = d.m_category;
    m_main_position = d.m_main_position;
    m_main_xalign = d.m_main_xalign;
    m_main_yalign = d.m_main_yalign;
    m_xlabel_xalign = d.m_xlabel_xalign;
    m_xlabel_yalign = d.m_xlabel_yalign;
    m_ylabel_xalign = d.m_ylabel_xalign;
    m_ylabel_yalign = d.m_ylabel_yalign;
    property_changed ();
  }
  return *this;
}

bool 
Object::operator< (const ant::Object &b) const
{
  if (m_id != b.m_id) {
    return m_id < b.m_id;
  }
  if (m_points != b.m_points) {
    return m_points < b.m_points;
  }
  if (m_fmt_x != b.m_fmt_x) {
    return m_fmt_x < b.m_fmt_x;
  }
  if (m_fmt_y != b.m_fmt_y) {
    return m_fmt_y < b.m_fmt_y;
  }
  if (m_fmt != b.m_fmt) {
    return m_fmt < b.m_fmt;
  }
  if (m_style != b.m_style) {
    return m_style < b.m_style;
  }
  if (m_outline != b.m_outline) {
    return m_outline < b.m_outline;
  }
  if (m_snap != b.m_snap) {
    return m_snap < b.m_snap;
  }
  if (m_angle_constraint != b.m_angle_constraint) {
    return m_angle_constraint < b.m_angle_constraint;
  }
  if (m_category != b.m_category) {
    return m_category < b.m_category;
  }
  if (m_main_position != b.m_main_position) {
    return m_main_position < b.m_main_position;
  }
  if (m_main_xalign != b.m_main_xalign) {
    return m_main_xalign < b.m_main_xalign;
  }
  if (m_main_yalign != b.m_main_yalign) {
    return m_main_yalign < b.m_main_yalign;
  }
  if (m_xlabel_xalign != b.m_xlabel_xalign) {
    return m_xlabel_xalign < b.m_xlabel_xalign;
  }
  if (m_xlabel_yalign != b.m_xlabel_yalign) {
    return m_xlabel_yalign < b.m_xlabel_yalign;
  }
  if (m_ylabel_xalign != b.m_ylabel_xalign) {
    return m_ylabel_xalign < b.m_ylabel_xalign;
  }
  if (m_ylabel_yalign != b.m_ylabel_yalign) {
    return m_ylabel_yalign < b.m_ylabel_yalign;
  }
  return false;
}

bool 
Object::equals (const db::DUserObjectBase *d) const
{
  const ant::Object *ruler = dynamic_cast<const ant::Object *> (d);
  if (ruler) {
    return *this == *ruler;
  } else {
    return false;
  }
}

bool 
Object::operator== (const ant::Object &d) const
{
  return m_points == d.m_points && m_id == d.m_id &&
         m_fmt_x == d.m_fmt_x && m_fmt_y == d.m_fmt_y && m_fmt == d.m_fmt &&
         m_style == d.m_style && m_outline == d.m_outline && 
         m_snap == d.m_snap && m_angle_constraint == d.m_angle_constraint &&
         m_category == d.m_category &&
         m_main_position == d.m_main_position &&
         m_main_xalign == d.m_main_xalign && m_main_yalign == d.m_main_yalign &&
         m_xlabel_xalign == d.m_xlabel_xalign && m_xlabel_yalign == d.m_xlabel_yalign &&
         m_ylabel_xalign == d.m_ylabel_xalign && m_ylabel_yalign == d.m_ylabel_yalign
    ;
}

void
Object::clean_points ()
{
  auto new_points = m_points;
  clean_points_impl (new_points);
  set_points_exact (std::move (new_points));
}

void
Object::set_points (const point_list &points)
{
  auto new_points = points;
  clean_points_impl (new_points);
  set_points_exact (std::move (new_points));
}

void
Object::set_points_exact (const point_list &points)
{
  if (m_points != points) {
    m_points = points;
    property_changed ();
  }
}

void
Object::set_points_exact (point_list &&points)
{
  if (m_points != points) {
    m_points.swap (points);
    property_changed ();
  }
}

db::DPoint
Object::seg_p1 (size_t seg_index) const
{
  if (seg_index == std::numeric_limits<size_t>::max ()) {
    return p1 ();
  } else if (seg_index < m_points.size ()) {
    return m_points[seg_index];
  } else if (m_points.empty ()) {
    return db::DPoint ();
  } else {
    return m_points.back ();
  }
}

db::DPoint
Object::seg_p2 (size_t seg_index) const
{
  if (seg_index == std::numeric_limits<size_t>::max ()) {
    return p2 ();
  } else if (seg_index + 1 < m_points.size ()) {
    return m_points[seg_index + 1];
  } else if (m_points.empty ()) {
    return db::DPoint ();
  } else {
    return m_points.back ();
  }
}

void
Object::seg_p1 (size_t seg_index, const db::DPoint &p)
{
  if (seg_index == std::numeric_limits<size_t>::max ()) {
    p1 (p);
  } else if (seg_index < m_points.size ()) {
    m_points[seg_index] = p;
  } else if (! m_points.empty ()) {
    m_points.back () = p;
  }
}

void
Object::seg_p2 (size_t seg_index, const db::DPoint &p)
{
  if (seg_index == std::numeric_limits<size_t>::max ()) {
    p2 (p);
  } else if (seg_index + 1 < m_points.size ()) {
    m_points[seg_index + 1] = p;
  } else if (! m_points.empty ()) {
    m_points.back () = p;
  }
}

void
Object::p1 (const db::DPoint &p)
{
  if (m_points.size () < 1 || ! p1 ().equal (p)) {
    if (m_points.size () < 1) {
      m_points.push_back (p);
    } else {
      m_points.front () = p;
      //  makes sure there is only one point if p1 == p2
      if (m_points.size () == 2 && m_points.back () == m_points.front ()) {
        m_points.pop_back ();
      }
    }
    property_changed ();
  }
}

void
Object::p2 (const db::DPoint &p)
{
  if (m_points.size () < 2 || ! p2 ().equal (p)) {
    if (m_points.size () < 2) {
      if (m_points.empty ()) {
        m_points.push_back (db::DPoint ());
      }
      m_points.push_back (p);
    } else {
      m_points.back () = p;
    }
    //  makes sure there is only one point if p1 == p2
    if (m_points.size () == 2 && m_points.back () == m_points.front ()) {
      m_points.pop_back ();
    }
    property_changed ();
  }
}

bool 
Object::less (const db::DUserObjectBase *d) const
{
  const ant::Object *ruler = dynamic_cast<const ant::Object *> (d);
  if (ruler) {
    return *this < *ruler;
  } else {
    return class_id () < d->class_id ();
  }
}

unsigned int 
Object::class_id () const
{
  static unsigned int cid = db::get_unique_user_object_class_id ();
  return cid;
}

db::DUserObjectBase *
Object::clone () const
{
  return new ant::Object (*this);
}

db::DBox 
Object::box () const
{
  db::DBox bx;
  for (auto d = m_points.begin (); d != m_points.end (); ++d) {
    bx += *d;
  }
  return bx;
}

class AnnotationEval
  : public tl::Eval
{
public:
  AnnotationEval (const Object &obj, const db::DFTrans &t)
    : m_obj (obj), m_trans (t)
  { }

  const Object &obj () const { return m_obj; }
  const db::DFTrans &trans () const { return m_trans; }

private:
  const Object &m_obj;
  db::DFTrans m_trans;
};

class AnnotationEvalFunction
  : public tl::EvalFunction
{
public:
  AnnotationEvalFunction (char function, const AnnotationEval *eval, size_t index)
    : m_function (function), mp_eval (eval), m_index (index)
  {
    // .. nothing yet ..
  }

  void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector <tl::Variant> &vv) const
  {
    if (vv.size () != 0) {
      throw tl::EvalError (tl::to_string (tr ("Annotation function must not have arguments")), context);
    }

    const Object &obj = mp_eval->obj ();
    const db::DFTrans &trans = mp_eval->trans ();

    if (m_function == 'L') {
      out = fabs (delta_x (obj, trans)) + fabs (delta_y (obj, trans));
    } else if (m_function == 'D') {
      out = sqrt (delta_x (obj, trans) * delta_x (obj, trans) + delta_y (obj, trans) * delta_y (obj, trans));
    } else if (m_function == 'A') {
      out = delta_x (obj, trans) * delta_y (obj, trans) * 1e-6;
    } else if (m_function == 'X') {
      out = delta_x (obj, trans);
    } else if (m_function == 'Y') {
      out = delta_y (obj, trans);
    } else if (m_function == 'U') {
      out = (trans * p1 (obj)).x ();
    } else if (m_function == 'V') {
      out = (trans * p1 (obj)).y ();
    } else if (m_function == 'P') {
      out = (trans * p2 (obj)).x ();
    } else if (m_function == 'Q') {
      out = (trans * p2 (obj)).y ();
    } else if (m_function == 'G') {
      double r, a1, a2;
      db::DPoint c;
      if (obj.compute_angle_parameters (r, c, a1, a2)) {
        out = tl::Variant ((a2 - a1) * 180.0 / M_PI);
      } else {
        out = tl::Variant ();
      }
    } else {
      out = tl::Variant ();
    }
  }

  db::DPoint p1 (const Object &obj) const
  {
    return obj.seg_p1 (m_index);
  }

  db::DPoint p2 (const Object &obj) const
  {
    return obj.seg_p2 (m_index);
  }

  double
  delta_x (const Object &obj, const db::DFTrans &t) const
  {
    double dx = ((t * p2 (obj)).x () - (t * p1 (obj)).x ());

    //  avoid "almost 0" outputs
    if (fabs (dx) < 1e-5 /*micron*/) {
      dx = 0;
    }

    return dx;
  }

  double
  delta_y (const Object &obj, const db::DFTrans &t) const
  {
    double dy = ((t * p2 (obj)).y () - (t * p1 (obj)).y ());

    //  avoid "almost 0" outputs
    if (fabs (dy) < 1e-5 /*micron*/) {
      dy = 0;
    }

    return dy;
  }

private:
  char m_function;
  const AnnotationEval *mp_eval;
  size_t m_index;
};

std::string 
Object::formatted (const std::string &fmt, const db::DFTrans &t, size_t index) const
{
  AnnotationEval eval (*this, t);
  eval.define_function ("L", new AnnotationEvalFunction('L', &eval, index)); // manhattan length
  eval.define_function ("D", new AnnotationEvalFunction('D', &eval, index)); // euclidian distance
  eval.define_function ("X", new AnnotationEvalFunction('X', &eval, index)); // x delta
  eval.define_function ("Y", new AnnotationEvalFunction('Y', &eval, index)); // y delta
  eval.define_function ("U", new AnnotationEvalFunction('U', &eval, index)); // p1.x
  eval.define_function ("V", new AnnotationEvalFunction('V', &eval, index)); // p1.y
  eval.define_function ("P", new AnnotationEvalFunction('P', &eval, index)); // p2.x
  eval.define_function ("Q", new AnnotationEvalFunction('Q', &eval, index)); // p2.y
  eval.define_function ("A", new AnnotationEvalFunction('A', &eval, index)); // area mm2
  eval.define_function ("G", new AnnotationEvalFunction('G', &eval, index)); // angle (if applicable)
  return eval.interpolate (fmt);
}

const char *
Object::class_name () const
{
  return "ant::Object";
}

void 
Object::from_string (const char *s, const char * /*base_dir*/)
{
  m_points.clear ();
  point_list new_points;

  tl::Extractor ex (s);
  while (! ex.at_end ()) {

    if (ex.test ("id=")) {

      int i = 0;
      ex.read (i);
      id (i);

    } else if (ex.test ("category=")) {

      std::string s;
      ex.read_word_or_quoted (s);
      set_category (s);

    } else if (ex.test ("fmt=")) {

      std::string s;
      ex.read_word_or_quoted (s);
      fmt (s);

    } else if (ex.test ("fmt_x=")) {

      std::string s;
      ex.read_word_or_quoted (s);
      fmt_x (s);

    } else if (ex.test ("fmt_y=")) {

      std::string s;
      ex.read_word_or_quoted (s);
      fmt_y (s);

    } else if (ex.test ("x1=")) {

      double q = 0;
      ex.read (q);
      db::DPoint p (p1 ());
      p.set_x (q);
      p1 (p);

    } else if (ex.test ("y1=")) {

      double q = 0;
      ex.read (q);
      db::DPoint p (p1 ());
      p.set_y (q);
      p1 (p);

    } else if (ex.test ("x2=")) {

      double q = 0;
      ex.read (q);
      db::DPoint p (p2 ());
      p.set_x (q);
      p2 (p);

    } else if (ex.test ("y2=")) {

      double q = 0;
      ex.read (q);
      db::DPoint p (p2 ());
      p.set_y (q);
      p2 (p);

    } else if (ex.test ("pt=")) {

      double x = 0.0, y = 0.0;
      ex.read (x);
      ex.expect (":");
      ex.read (y);
      new_points.push_back (db::DPoint (x, y));

    } else if (ex.test ("position=")) {

      std::string s;
      ex.read_word (s);
      ant::PositionConverter pc;
      ant::Object::position_type pos;
      pc.from_string (s, pos);
      set_main_position (pos);
      ex.test (",");

    } else if (ex.test ("xalign=")) {

      std::string s;
      ex.read_word (s);
      ant::AlignmentConverter ac;
      ant::Object::alignment_type a;
      ac.from_string (s, a);
      set_main_xalign (a);
      ex.test (",");

    } else if (ex.test ("yalign=")) {

      std::string s;
      ex.read_word (s);
      ant::AlignmentConverter ac;
      ant::Object::alignment_type a;
      ac.from_string (s, a);
      set_main_yalign (a);
      ex.test (",");

    } else if (ex.test ("xlabel_xalign=")) {

      std::string s;
      ex.read_word (s);
      ant::AlignmentConverter ac;
      ant::Object::alignment_type a;
      ac.from_string (s, a);
      set_xlabel_xalign (a);
      ex.test (",");

    } else if (ex.test ("xlabel_yalign=")) {

      std::string s;
      ex.read_word (s);
      ant::AlignmentConverter ac;
      ant::Object::alignment_type a;
      ac.from_string (s, a);
      set_xlabel_yalign (a);
      ex.test (",");

    } else if (ex.test ("ylabel_xalign=")) {

      std::string s;
      ex.read_word (s);
      ant::AlignmentConverter ac;
      ant::Object::alignment_type a;
      ac.from_string (s, a);
      set_ylabel_xalign (a);
      ex.test (",");

    } else if (ex.test ("ylabel_yalign=")) {

      std::string s;
      ex.read_word (s);
      ant::AlignmentConverter ac;
      ant::Object::alignment_type a;
      ac.from_string (s, a);
      set_ylabel_yalign (a);
      ex.test (",");

    } else if (ex.test ("style=")) {

      std::string s;
      ex.read_word (s);
      ant::StyleConverter sc;
      ant::Object::style_type st;
      sc.from_string (s, st);
      style (st);

    } else if (ex.test ("outline=")) {

      std::string s;
      ex.read_word (s);
      ant::OutlineConverter oc;
      ant::Object::outline_type ot;
      oc.from_string (s, ot);
      outline (ot);

    } else if (ex.test ("snap=")) {

      bool f = false;
      ex.read (f);
      snap (f);

    } else if (ex.test ("angle_constraint=")) {

      std::string s;
      ex.read_word (s);
      ant::ACConverter sc;
      lay::angle_constraint_type sm;
      sc.from_string (s, sm);
      angle_constraint (sm);

    } else {
      break;
    }

    ex.test (",");

  }

  if (! new_points.empty ()) {
    set_points (new_points);
  }
}

std::string 
Object::to_string () const
{
  std::string r;

  r += "id=";
  r += tl::to_string (id ());
  r += ",";

  if (m_points.size () > 2) {
    for (auto p = m_points.begin (); p != m_points.end (); ++p) {
      r += "pt=";
      r += tl::to_string (p->x ());
      r += ":";
      r += tl::to_string (p->y ());
      r += ",";
    }
  } else {
    r += "x1=";
    r += tl::to_string (p1 ().x ());
    r += ",";
    r += "y1=";
    r += tl::to_string (p1 ().y ());
    r += ",";
    r += "x2=";
    r += tl::to_string (p2 ().x ());
    r += ",";
    r += "y2=";
    r += tl::to_string (p2 ().y ());
    r += ",";
  }

  r += "category=";
  r += tl::to_word_or_quoted_string (category ());
  r += ",";

  r += "fmt=";
  r += tl::to_word_or_quoted_string (fmt ());
  r += ",";
  r += "fmt_x=";
  r += tl::to_word_or_quoted_string (fmt_x ());
  r += ",";
  r += "fmt_y=";
  r += tl::to_word_or_quoted_string (fmt_y ());
  r += ",";
  
  r += "position=";
  ant::PositionConverter pc;
  r += pc.to_string (main_position ());
  r += ",";

  ant::AlignmentConverter ac;
  r += "xalign=";
  r += ac.to_string (main_xalign ());
  r += ",";
  r += "yalign=";
  r += ac.to_string (main_yalign ());
  r += ",";
  r += "xlabel_xalign=";
  r += ac.to_string (xlabel_xalign ());
  r += ",";
  r += "xlabel_yalign=";
  r += ac.to_string (xlabel_yalign ());
  r += ",";
  r += "ylabel_xalign=";
  r += ac.to_string (ylabel_xalign ());
  r += ",";
  r += "ylabel_yalign=";
  r += ac.to_string (ylabel_yalign ());
  r += ",";

  r += "style=";
  ant::StyleConverter sc;
  r += sc.to_string (style ());
  r += ",";
  
  r += "outline=";
  ant::OutlineConverter oc;
  r += oc.to_string (outline ());
  r += ",";
  
  r += "snap=";
  r += tl::to_string (snap ());
  r += ",";

  r += "angle_constraint=";
  ant::ACConverter acc;
  r += acc.to_string (angle_constraint ());

  return r;
}

bool
Object::compute_interpolating_circle (double &radius, db::DPoint &center, double &start_angle, double &stop_angle) const
{
  if (m_points.size () < 2) {
    return false;
  }

  double d = m_points.back ().distance (m_points.front ()) * 0.5;
  if (d < db::epsilon) {
    return false;
  }

  db::DVector n = m_points.back () - m_points.front ();
  db::DPoint m = m_points.front () + n * 0.5;
  n = db::DVector (n.y (), -n.x ()) * (0.5 / d);

  double nom = 0.0;
  double div = 0.0;

  for (size_t i = 1; i + 1 < m_points.size (); ++i) {
    db::DVector p = m_points [i] - m;
    double pn = db::sprod (p, n);
    div += pn * pn;
    nom += pn * (p.sq_double_length () - d * d);
  }

  if (div < db::epsilon) {
    return false;
  }

  double l = 0.5 * nom / div;
  radius = sqrt (l * l + d * d);
  center = m + n * l;

  double a = atan2 (-n.y (), -n.x ());
  double da = atan2 (d, l);

  if (fabs (l) < db::epsilon) {

    start_angle = 0.0;
    stop_angle = M_PI * 2.0;

  } else if (l < 0.0) {

    stop_angle = a + da;
    start_angle = stop_angle + 2.0 * (M_PI - da);

  } else {

    start_angle = a - da;
    stop_angle = a + da;

  }

  while (stop_angle < start_angle - db::epsilon) {
    stop_angle += M_PI * 2.0;
  }

  return true;
}

bool
Object::compute_angle_parameters (double &radius, db::DPoint &center, double &start_angle, double &stop_angle) const
{
  if (m_points.size () < 3) {
    return false;
  }

  db::DPoint p1 = m_points.front (), p2 = m_points.back ();

  db::DVector pc;
  for (size_t i = 1; i + 1 < m_points.size (); ++i) {
    pc += m_points[i] - db::DPoint ();
  }
  center = db::DPoint () + pc * (1.0 / double (m_points.size () - 2));

  db::DVector v1 (p1 - center);
  if (v1.double_length () < db::epsilon) {
    return false;
  }

  db::DVector v2 (p2 - center);
  if (v2.double_length () < db::epsilon) {
    return false;
  }

  radius = std::min (v1.double_length (), v2.double_length ());

  v1 *= 1.0 / v1.double_length ();
  v2 *= 1.0 / v2.double_length ();

  if (db::vprod_sign (v1, v2) == 0) {
    return false;
  }

  start_angle = 0.0;
  stop_angle = 0.0;
  start_angle = atan2 (v1.y (), v1.x ());
  stop_angle = atan2 (v2.y (), v2.x ());

  if (db::vprod_sign (v1, v2) < 0) {
    std::swap (stop_angle, start_angle);
  }

  while (stop_angle < start_angle - db::epsilon) {
    stop_angle += M_PI * 2.0;
  }

  return true;
}

void
Object::property_changed ()
{
  //  .. nothing yet ..
}

/**
 *  @brief Registration of the ant::Object class in the DUserObject space
 */
static db::DUserObjectDeclaration class_registrar (new db::user_object_factory_impl<ant::Object, db::DCoord> ("ant::Object"));

} // namespace ant

