
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#include "tlExpression.h"
#include "tlVariantUserClasses.h"
#include "tlUnitTest.h"
#include "tlEnv.h"

#define _USE_MATH_DEFINES // for MSVC
#include <math.h>

// basics
TEST(1) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("1+2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("1.2e3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1200"));
  v = e.parse ("-0.25e-2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-0.0025"));
  v = e.parse ("0xffff").execute ();
  EXPECT_EQ (v.to_string (), std::string ("65535"));
  v = e.parse ("0x1001").execute ();
  EXPECT_EQ (v.to_string (), std::string ("4097"));
  v = e.parse ("0x1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("1-2+3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("1-4*2+3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-4"));
  v = e.parse ("(1-4)*2+3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-3"));
  v = e.parse ("(4-1)*2%4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("7%4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("2+3/2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.5"));

  v = e.parse ("to_i(1)*to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_i(1)*to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_i(1)*to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_i(1)*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_i(1)*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_i(1)*2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.4"));
  v = e.parse ("to_i(1)*'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ui(1)*to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ui(1)*to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ui(1)*to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ui(1)*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ui(1)*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ui(1)*2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.4"));
  v = e.parse ("to_ui(1)*'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_l(1)*to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_l(1)*to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_l(1)*to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_l(1)*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_l(1)*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_l(1)*2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.4"));
  v = e.parse ("to_l(1)*'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ul(1)*to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ul(1)*to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ul(1)*to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ul(1)*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ul(1)*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("to_ul(1)*2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.4"));
  v = e.parse ("to_ul(1)*'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("1.4*to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.8"));
  v = e.parse ("1.4*to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.8"));
  v = e.parse ("1.4*to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.8"));
  v = e.parse ("1.4*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.8"));
  v = e.parse ("1.4*to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.8"));
  v = e.parse ("1.2*2.0").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2.4"));
  v = e.parse ("'1'*2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("11"));
  v = e.parse ("'3'*'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("33"));

  v = e.parse ("to_i(1)+to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_i(1)+to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_i(1)+to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_i(1)+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_i(1)+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_i(1)+2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("to_i(1)+'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("12"));
  v = e.parse ("to_ui(1)+to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ui(1)+to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ui(1)+to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ui(1)+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ui(1)+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ui(1)+2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("to_ui(1)+'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("12"));
  v = e.parse ("to_l(1)+to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_l(1)+to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_l(1)+to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_l(1)+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_l(1)+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_l(1)+2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("to_l(1)+'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("12"));
  v = e.parse ("to_ul(1)+to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ul(1)+to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ul(1)+to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ul(1)+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ul(1)+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("to_ul(1)+2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("to_ul(1)+'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("12"));
  v = e.parse ("1.4+to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("1.4+to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("1.4+to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("1.4+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("1.4+to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.4"));
  v = e.parse ("1.4+2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3.8"));
  v = e.parse ("'1'+2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("12.4"));
  v = e.parse ("'3'+'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("32"));

  v = e.parse ("to_i(1)-to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("to_i(1)-to_ui(2)").execute ();
  if (sizeof(long) == 8) {
    EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  } else {
    EXPECT_EQ (v.to_string (), std::string ("4294967295"));
  }
  v = e.parse ("to_i(1)-to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("to_i(1)-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_i(1)-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_i(1)-2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1.4"));
  v = e.parse ("to_i(1)-'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("to_ui(1)-to_i(2)").execute ();
  if (sizeof(long) == 8) {
    EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  } else {
    EXPECT_EQ (v.to_string (), std::string ("4294967295"));
  }
  v = e.parse ("to_ui(1)-to_ui(2)").execute ();
  if (sizeof(long) == 8) {
    EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  } else {
    EXPECT_EQ (v.to_string (), std::string ("4294967295"));
  }
  v = e.parse ("to_ui(1)-to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("to_ui(1)-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_ui(1)-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_ui(1)-2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1.4"));
  v = e.parse ("to_ui(1)-'2'").execute ();
  if (sizeof(long) == 8) {
    EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  } else {
    EXPECT_EQ (v.to_string (), std::string ("4294967295"));
  }
  v = e.parse ("to_l(1)-to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("to_l(1)-to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("to_l(1)-to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("to_l(1)-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_l(1)-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_l(1)-2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1.4"));
  v = e.parse ("to_l(1)-'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("to_ul(1)-to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_ul(1)-to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_ul(1)-to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_ul(1)-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_ul(1)-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("to_ul(1)-2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1.4"));
  v = e.parse ("to_ul(1)-'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18446744073709551615"));
  v = e.parse ("1.4-to_i(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-0.6"));
  v = e.parse ("1.4-to_ui(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-0.6"));
  v = e.parse ("1.4-to_l(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-0.6"));
  v = e.parse ("1.4-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-0.6"));
  v = e.parse ("1.4-to_ul(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-0.6"));
  v = e.parse ("1.4-2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("'1'-2.4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1.4"));
  v = e.parse ("'3'-'2'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));

  v = e.parse ("[1,2,3]").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1,2,3"));

  v = e.parse ("1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("false?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("nil?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("true?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("'a'+'x'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("ax"));
  v = e.parse ("'a'*4").execute ();
  EXPECT_EQ (v.to_string (), std::string ("aaaa"));
}

class Point
{
public:
  Point () : m_x (0), m_y (0) { }

  Point (int x, int y) : m_x (x), m_y (y) { }

  int x () const { return m_x; }
  int y () const { return m_y; }

  std::string
  to_string () const
  {
    return tl::db_to_string (m_x) + "," + tl::db_to_string (m_y);
  }

private:
  int m_x, m_y;
};

class Box
{
public:
  Box (int x1, int y1, int x2, int y2)
    : m_p1 (x1 < x2 ? x1 : x2, y1 < y2 ? y1 : y2),
      m_p2 (x2 > x1 ? x2 : x1, y2 > y1 ? y2 : y1)
  {
    //  .. nothing else ..
  }

  Box ()
    : m_p1 (1, 1), m_p2 (-1, -1)
  {
    //  .. nothing else ..
  }

  inline Box &operator&= (const Box &b)
  {
    if (b.empty ()) {
      *this = Box ();
    } else if (! empty ()) {
      Point p1 (m_p1.x () > b.m_p1.x () ? m_p1.x () : b.m_p1.x (),
                m_p1.y () > b.m_p1.y () ? m_p1.y () : b.m_p1.y ());
      Point p2 (m_p2.x () < b.m_p2.x () ? m_p2.x () : b.m_p2.x (),
                m_p2.y () < b.m_p2.y () ? m_p2.y () : b.m_p2.y ());
      m_p1 = p1;
      m_p2 = p2;
    }
    return *this;
  }

  Box operator& (const Box &b) const
  {
    Box r (*this);
    r &= b;
    return r;
  }

  int width () const
  {
    return m_p2.x () - m_p1.x ();
  }

  int height () const
  {
    return m_p2.y () - m_p1.y ();
  }

  bool empty () const
  {
    return m_p1.x () > m_p2.x ();
  }

  std::string to_string () const
  {
    if (empty ()) {
      return "()";
    } else {
      return "(" + m_p1.to_string () + ";" + m_p2.to_string () + ")";
    }
  }

private:
  Point m_p1, m_p2;
};

class Edge
{
public:
  Edge ()
    : m_p1 (0, 0), m_p2 (0, 0)
  {
    //  .. nothing else ..
  }

  Edge (int x1, int y1, int x2, int y2)
    : m_p1 (x1, y1), m_p2 (x2, y2)
  {
    //  .. nothing else ..
  }

  Edge operator* (double s) const
  {
    return Edge (int (floor (m_p1.x () * s + 0.5)),
                 int (floor (m_p1.y () * s + 0.5)),
                 int (floor (m_p2.x () * s + 0.5)),
                 int (floor (m_p2.y () * s + 0.5)));
  }

  int dx () const
  {
    return m_p2.x () - m_p1.x ();
  }

  int dy () const
  {
    return m_p2.y () - m_p1.y ();
  }

  std::string to_string () const
  {
    return "(" + m_p1.to_string () + ";" + m_p2.to_string () + ")";
  }

private:
  Point m_p1, m_p2;
};

class BoxClassClass : public tl::VariantUserClassBase, private tl::EvalClass
{
public:
  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const;

  virtual void *create () const { tl_assert (false); }
  virtual void destroy (void *) const { tl_assert (false); }
  virtual bool equal (const void *, const void *) const { tl_assert (false); }
  virtual bool less (const void *, const void *) const { tl_assert (false); }
  virtual void *clone (const void *) const { tl_assert (false); }
  virtual void assign (void *, const void *) const { tl_assert (false); }
  virtual std::string to_string (const void *) const { tl_assert (false); }
  virtual int to_int (const void *) const { tl_assert (false); }
  virtual double to_double (const void *) const { tl_assert (false); }
  virtual void to_variant (const void *, tl::Variant &) const { tl_assert (false); }
  virtual void read (void *, tl::Extractor &) const { }
  virtual const char *name () const { return "Box"; }
  virtual unsigned int type_code () const { return 0; }
  virtual const tl::EvalClass *eval_cls () const { return this; }
  virtual bool is_const () const { return false; }
  virtual bool is_ref () const { return false; }
  virtual void *deref_proxy (tl::Object *) const { return 0; }
  virtual const gsi::ClassBase*gsi_cls() const { return 0; }
  static BoxClassClass instance;
};

BoxClassClass BoxClassClass::instance;

class BoxClass : public tl::VariantUserClassImpl<Box>, private tl::EvalClass
{
public:
  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const;

  virtual const tl::EvalClass *eval_cls () const { return this; }
  static BoxClass instance;
};

BoxClass BoxClass::instance;

void BoxClass::execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const 
{
  if (method == "width") {
    out = object.to_user<Box> ().width ();
  } else if (method == "height") {
    out = object.to_user<Box> ().height ();
  } else if (method == "&") {
    tl_assert (args.size () == 1);
    out = tl::Variant (new Box (object.to_user<Box> () & args[0].to_user<Box> ()), &BoxClass::instance, true);
  } else if (method == "to_s") {
    out = object.to_user<Box> ().to_string ();
  } else if (method == "is_box") {
    out = true;
  } else if (method == "is_edge") {
    out = false;
  } else {
    throw tl::NoMethodError("Box", method, context);
  }
}

void BoxClassClass::execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant & /*object*/, const std::string &method, const std::vector<tl::Variant> &args) const 
{
  if (method == "new") {
    out = tl::Variant (new Box (args[0].to_long(), args[1].to_long(), args[2].to_long(), args[3].to_long()), &BoxClass::instance, true);
  } else {
    throw tl::NoMethodError("Box", method, context);
  }
}

class EdgeClassClass : public tl::VariantUserClassBase, private tl::EvalClass
{
public:
  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const;

  virtual void *create () const { tl_assert (false); }
  virtual void destroy (void *) const { tl_assert (false); }
  virtual bool equal (const void *, const void *) const { tl_assert (false); }
  virtual bool less (const void *, const void *) const { tl_assert (false); }
  virtual void *clone (const void *) const { tl_assert (false); }
  virtual void assign (void *, const void *) const { tl_assert (false); }
  virtual std::string to_string (const void *) const { tl_assert (false); }
  virtual int to_int (const void *) const { tl_assert (false); }
  virtual double to_double (const void *) const { tl_assert (false); }
  virtual void to_variant (const void *, tl::Variant &) const { tl_assert (false); }
  virtual void read (void *, tl::Extractor &) const { }
  virtual const char *name () const { return "Edge"; }
  virtual unsigned int type_code () const { return 0; }
  virtual const tl::EvalClass *eval_cls () const { return this; }
  virtual bool is_const () const { return false; }
  virtual bool is_ref () const { return false; }
  virtual void *deref_proxy (tl::Object *) const { return 0; }
  virtual const gsi::ClassBase*gsi_cls() const { return 0; }
  static EdgeClassClass instance;
};

EdgeClassClass EdgeClassClass::instance;

class EdgeClass : public tl::VariantUserClassImpl<Edge>,  private tl::EvalClass
{
public:
  virtual void execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant &object, const std::string &method, const std::vector<tl::Variant> &args) const 
  {
    if (method == "dx") {
      out = object.to_user<Edge> ().dx ();
    } else if (method == "dy") {
      out = object.to_user<Edge> ().dy ();
    } else if (method == "to_s") {
      out = object.to_user<Edge> ().to_string ();
    } else if (method == "is_box") {
      out = false;
    } else if (method == "is_edge") {
      out = true;
    } else if (method == "*") {
      out.set_user (new Edge (object.to_user<Edge> () * args [0].to_double ()), object.user_cls (), true);
    } else {
      throw tl::NoMethodError("Edge", method, context);
    }
  }
  virtual const tl::EvalClass *eval_cls () const { return this; }
  static EdgeClass instance;
};

EdgeClass EdgeClass::instance;

void 
EdgeClassClass::execute (const tl::ExpressionParserContext &context, tl::Variant &out, tl::Variant & /*object*/, const std::string &method, const std::vector<tl::Variant> &args) const 
{
  if (method == "new") {
    out = tl::Variant (new Edge (args[0].to_long(), args[1].to_long(), args[2].to_long(), args[3].to_long()), &EdgeClass::instance, true);
  } else {
    throw tl::NoMethodError("Edge", method, context);
  }
}

// basics: custom objects
TEST(1b) 
{
  tl::Eval e;
  tl::Variant v;

  e.set_var ("XBox", tl::Variant ((Box *) 0, &BoxClassClass::instance, false));
  e.set_var ("XEdge", tl::Variant ((Edge *) 0, &EdgeClassClass::instance, false));
  e.set_var ("b", tl::Variant (new Box (0, 10, 20, 40), &BoxClass::instance, true));
  e.set_var ("e", tl::Variant (new Edge (0, 10, 20, 40), &EdgeClass::instance, true));

  v = e.parse ("b.width").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20"));
  v = e.parse ("b.width()").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20"));
  v = e.parse ("b.height").execute ();
  EXPECT_EQ (v.to_string (), std::string ("30"));
  v = e.parse ("e.dx").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20"));
  v = e.parse ("e.dy").execute ();
  EXPECT_EQ (v.to_string (), std::string ("30"));
  v = e.parse ("e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("(0,10;20,40)"));
  v = e.parse ("(e*5).to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("(0,50;100,200)"));
  v = e.parse ("(e.*(5)).to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("(0,50;100,200)"));
  v = e.parse ("b.is_box").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("b.is_edge").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("XBox.new(1,2,3,4).is_box").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("XBox.new(1,2,3,4).is_edge").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("XBox.new(0, 0, 100, 200) & XBox.new(10, 10, 110, 210)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("(10,10;100,200)"));
  v = e.parse ("XBox.new(0, 0, 100, 200) & XBox.new(1000, 1000, 1010, 1010)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("()"));
  v = e.parse ("e.is_edge").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("e.is_box").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("XBox.new(1,2,3,4).width").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("XBox.new(1,2,3,4).width==2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("XBox.new(1,2,3,4).width==3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
}

//  to_bool
TEST(2)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("false?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("nil?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("[]?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("[1]?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("'1'?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("''?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
}

//  to_double
TEST(3)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("[1,2,3]/2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1.5"));
}

//  functions
TEST(5)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("pow(sin(M_PI/4),2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.5"));
  v = e.parse ("sinh(log(2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.75"));
  v = e.parse ("cos(0.0)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("cos(M_PI/3)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.5"));
  v = e.parse ("cosh(log(2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1.25"));
  v = e.parse ("tan(M_PI/4)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("tanh(log(2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.6"));
  v = e.parse ("log(M_E)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("exp(log(1.5))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1.5"));
  v = e.parse ("log10(0.1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("floor(0.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("floor(-0.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("floor(1.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("round(1.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("round(1.6)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("round(1.4)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("ceil(0.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("ceil(-0.5)").execute ();
  EXPECT_EQ (v.to_string () == std::string ("-0") || v.to_string () == std::string ("0"), true);
  v = e.parse ("ceil(1.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("sqrt(4)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("abs('-2')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("abs(-1234567)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1234567"));
  v = e.parse ("abs(-0.2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.2"));
  v = e.parse ("acos(0)/M_PI").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.5"));
  // v = e.parse ("acosh(cosh(1.0))").execute ();
  // EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("asin(1)/M_PI").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.5"));
  // v = e.parse ("asinh(sinh(1.0))").execute ();
  // EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("atan(1)/M_PI").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.25"));
  // v = e.parse ("atanh(tanh(1))").execute ();
  // EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("min(1,6)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("min(2,0,5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("max(1,6)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("6"));
  v = e.parse ("max(2,0,5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("5"));
  v = e.parse ("atan2(2,2)/M_PI").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.25"));
  v = e.parse ("to_i(6)/to_i(4)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("to_i('6')/to_i('4')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("to_f('6')/to_f('4')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1.5"));
  v = e.parse ("is_string('6')?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("is_string(6)?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("is_numeric('6')?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("is_numeric('a')?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("is_numeric(6)?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("is_array('6')?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("is_array(6)?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("is_array([])?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("is_nil([])?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("is_nil(nil)?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
}

//  string functions
TEST(6)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("gsub('bcabc','b','xx')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("xxcaxxc"));
  v = e.parse ("gsub('abcabc','b','xx')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("axxcaxxc"));
  v = e.parse ("sub('abcabc','b','xx')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("axxcabc"));
  v = e.parse ("sub('bcabc','b','xx')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("xxcabc"));
  v = e.parse ("find('abcabc','b')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("rfind('abcabc','b')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("find('abcabc','x')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("rfind('abcabc','c')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("rfind('abcabc','x')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("len('abcabc')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("6"));
  v = e.parse ("len([])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("len([1,2,3])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("substr('abcabc',2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("cabc"));
  v = e.parse ("substr('abcabc',2,1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("c"));
  v = e.parse ("substr('abcabc',8,1)").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("substr('abcabc',3,-1)").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("substr('abcabc',3,8)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("abc"));
  v = e.parse ("substr('abcabc',6,8)").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("substr('abcabc',7,8)").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("join([],':')").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("item([1,2],-1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("item([1,2],0)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("item([1,2],1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("item([1,2],2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("split('',':')").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("split('1:2',':')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1,2"));
  v = e.parse ("env('HJASK')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("env('PATH')").execute ();
  EXPECT_EQ (v.to_string (), tl::get_env ("PATH"));
  v = e.parse ("absolute_path('./x.gds')").execute ();
  // EXPECT_EQ (v.to_string (), std::string ()); // not universal
  v = e.parse ("absolute_file_path('./x.gds')").execute ();
  // EXPECT_EQ (v.to_string (), std::string ()); // not universal
  v = e.parse ("path('../irgendwas/file.tar.gz')").execute ();
#if defined(_WIN32)
  EXPECT_EQ (v.to_string (), std::string ("..\\irgendwas"));
#else
  EXPECT_EQ (v.to_string (), std::string ("../irgendwas"));
#endif
  v = e.parse ("basename('../irgendwas/file.tar.gz')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("file"));
  v = e.parse ("extension('../irgendwas/file.tar.gz')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("tar.gz"));
  v = e.parse ("file_exists('x.gds')?1:0").execute ();
  // EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("is_dir('x.gds')?1:0").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("combine('.', 'x.gds')").execute ();
  // EXPECT_EQ (v.to_string (), std::string ("./x.gds")); // not universal
  v = e.parse ("is_dir('.')?1:0").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("sprintf('%g %e %f',M_PI,M_PI*1e6,M_PI*0.001)").execute ();
  EXPECT_EQ (v.to_string (), tl::sprintf("%g %e %f", M_PI, M_PI*1e6, M_PI*0.001));
  v = e.parse ("sprintf('%g %e %f',M_PI*1e6,M_PI*1e6,M_PI*1e6)").execute ();
  EXPECT_EQ (v.to_string (), tl::sprintf("%g %e %f", M_PI*1e6, M_PI*1e6, M_PI*1e6));
  v = e.parse ("sprintf('%-15g %015.8e %15.12f %g',M_PI,M_PI*1e6,M_PI*0.001,M_PI)").execute ();
  EXPECT_EQ (v.to_string (), tl::sprintf("%-15g %015.8e %15.12f %g", M_PI, M_PI*1e6, M_PI*0.001,M_PI));
  v = e.parse ("sprintf('%-5s %5s %x %u %d (%s)','a','b',1234,2345,3456)").execute ();
  EXPECT_EQ (v.to_string (), tl::sprintf("%-5s %5s %x %u %d ()", "a", "b", 1234, 2345, 3456));
  std::string msg;
  try {
    v = e.parse ("error('My error')").execute ();
  } catch (tl::Exception &ex) {
    msg = ex.msg();
  }
  EXPECT_EQ (msg, std::string ("My error"));
}

//  compare ops
TEST(7)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("1==2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("1==1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("1!=1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("1!=2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("1<1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("1<2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("2<1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("1<=1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("1<=2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("2<=1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("1>1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("1>2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("2>1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("1>=1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("1>=2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("2>=1?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("1+1==2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("2*3-4==2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("2*3-3==2?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
}

//  boolean ops
TEST(8)
{
  tl::Eval e;
  tl::Variant v;
  bool t;

  v = e.parse ("1==2?log('a'):log(2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.69314718056"));
  t = false;
  try {
    v = e.parse ("1==1?log('a'):log(2)").execute ();
    EXPECT_EQ (v.to_string (), std::string ("abc"));
  } catch (tl::EvalError &) {
    t = true;
  }
  EXPECT_EQ (t, true);
  t = false;
  try {
    v = e.parse ("1==2||log('a')").execute ();
    EXPECT_EQ (v.to_string (), std::string ("1"));
  } catch (tl::EvalError &) {
    t = true;
  }
  EXPECT_EQ (t, true);
  v = e.parse ("1==1||log('a')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("1==2||1==1||log('a')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  t = false;
  try {
    v = e.parse ("1==1&&log('a')").execute ();
    EXPECT_EQ (v.to_string (), std::string ("false"));
  } catch (tl::EvalError &) {
    t = true;
  }
  EXPECT_EQ (t, true);
  v = e.parse ("1==2&&log('a')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("1==1&&1==2&&log('a')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
}

//  shift ops
TEST(9)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("1<<2+3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("32"));
  v = e.parse ("8*8>>2+3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
}

//  bitwise ops
TEST(10)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("(1<<2)|(1<<4)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20"));
  v = e.parse ("31&63").execute ();
  EXPECT_EQ (v.to_string (), std::string ("31"));
  v = e.parse ("31^63").execute ();
  EXPECT_EQ (v.to_string (), std::string ("32"));
}

//  unary ops
TEST(11)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("!(1==2)?2:3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("~1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-2"));
  v = e.parse ("-1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("--1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
}

class F0
  : public tl::EvalFunction
{
public:
  void execute (const tl::ExpressionParserContext &, tl::Variant &out, const std::vector <tl::Variant> &) const
  {
    out = tl::Variant (17);
  }
};

class F1
  : public tl::EvalFunction
{
public:
  void execute (const tl::ExpressionParserContext &, tl::Variant &out, const std::vector <tl::Variant> &vv) const
  {
    out = tl::Variant (vv[0].to_long() + 1);
  }
};

class F2
  : public tl::EvalFunction
{
public:
  void execute (const tl::ExpressionParserContext &, tl::Variant &out, const std::vector <tl::Variant> &vv) const
  {
    out = tl::Variant (vv[0].to_long() + 2);
  }
};

class F3
  : public tl::EvalFunction
{
public:
  void execute (const tl::ExpressionParserContext &, tl::Variant &out, const std::vector <tl::Variant> &vv) const
  {
    out = tl::Variant (vv[0].to_long() + 3);
  }
};


//  variables and functions
TEST(12)
{
  tl::Eval e, ee;
  e.set_global_var ("GV", tl::Variant("gg"));
  e.set_var ("L", tl::Variant((long) 89));
  ee.set_var ("L", tl::Variant((long) 123));
  e.define_global_function ("f0", new F0());
  e.define_global_function ("fg", new F1());
  e.define_function ("fl", new F2());
  ee.define_function ("fl", new F3());

  tl::Variant v;

  v = e.parse ("f0").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  v = e.parse ("f0()").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  v = e.parse ("GV+(L+1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("gg90"));
  v = ee.parse ("GV+(L+1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("gg124"));
  v = e.parse ("to_s(fg(17))+fl(L)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1891"));
  e.define_function ("fl", new F3());
  v = e.parse ("to_s(fg(17))+fl(L)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1892"));
  v = ee.parse ("to_s(fg(17))+fl(L)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("18126"));
}

//  interpolation
TEST(13)
{
  tl::Eval e, ee;
  e.set_var ("L", tl::Variant((long) 89));
  ee.set_var ("L", tl::Variant((long) 123));

  EXPECT_EQ (e.interpolate("A$L B$(L+100)C"), std::string ("A89 B189C"));
  EXPECT_EQ (ee.interpolate("123*11=$(L*11)."), std::string ("123*11=1353."));
}

// assignment
TEST(14) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var x=1; x=x+1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("x=x*2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("4"));
  v = e.parse ("x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("4"));
  v = e.parse ("var y=x==4; y").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
}

// index
TEST(15) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var x=[1,2,3]; x[1]").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("var x=[1,2,3]; x.size").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("var x=[1,2,3]; x[6]").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var x=[1,2,3]; x[1]=5; x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1,5,3"));
  v = e.parse ("var x=[1,2,3]; x.push('A'); x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1,2,3,A"));
  v = e.parse ("var x={1=>'A','B'=>5}; x[1]").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A"));
  v = e.parse ("var x={1=>'A','B'=>5}; x.keys").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1,B"));
  v = e.parse ("var x={1=>'A','B'=>5}; x.values").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A,5"));
  v = e.parse ("var x={1=>'A','B'=>5}; x['B']").execute ();
  EXPECT_EQ (v.to_string (), std::string ("5"));
  v = e.parse ("var x={1=>'A','B'=>5}; x[0]").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("{1=>'A','B'=>5}['B']").execute ();
  EXPECT_EQ (v.to_string (), std::string ("5"));
  v = e.parse ("var x={1=>'A','B'=>5}; x[1]=5; x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1=>5,B=>5"));
  v = e.parse ("var x={1=>'A','B'=>5}; x.size").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("var x={1=>'A','B'=>5}; x.insert(17, 3); x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1=>A,17=>3,B=>5"));
}

// match/nomatch
TEST(16) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("'abc' ~ '*a*'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("'abc' ~ '(*)a(*)'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("$1+'.'+$2+'.'+$3").execute ();
  EXPECT_EQ (v.to_string (), std::string (".bc.nil"));
  v = e.parse ("'abc' ~ 'b*'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("'abc' !~ '*a*'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("'abc' !~ 'b*'").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
}

// polymorphic functions
TEST(18)
{
  tl::Eval e;
  tl::Variant v;

  e.parse ("var tr=Trans.new(1,false,Vector.new(10,20))").execute ();
  e.parse ("var a=Point.new(1,2)").execute ();
  e.parse ("var b=Point.new(11,22)").execute ();
  v = e.parse ("var i=CellInstArray.new(17,tr,a,b,100,200); i.to_s()").execute ();
  EXPECT_EQ (v.to_string (), std::string ("#17 r90 10,20 [1,2*100;11,22*200]"));
  v = e.parse ("var i=CellInstArray.new(17,tr,a,b,100,200); i.is_complex()").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
}

// comments
TEST(19)
{
  tl::Eval e;
  e.parse ("var tr=Trans.new(1,false,Vector.new(10,20))").execute ();
  e.parse ("var a=Point.new(1,2)").execute ();
  e.parse ("var b=Point.new(11,22)").execute ();

  tl::Variant v;
  v = e.parse ("# A comment\nvar i=CellInstArray.new(17,tr,a,b,100,200); i.to_s(); # A final comment").execute ();
  EXPECT_EQ (v.to_string (), std::string ("#17 r90 10,20 [1,2*100;11,22*200]"));
}

// issue-787
TEST(20)
{
  tl::Eval e;
  e.parse ("var ly=Layout.new(true)").execute ();
  e.parse ("var top=ly.create_cell('TOP')").execute ();
  e.parse ("var cell=ly.create_cell('CHILD')").execute ();
  e.parse ("var i1 = top.insert(CellInstArray.new(cell.cell_index,Trans.new(Vector.new(100,200))))").execute ();
  e.parse ("var i2 = top.insert(CellInstArray.new(cell.cell_index,Trans.new(Vector.new(-100,300))))").execute ();

  tl::Variant v;
  v = e.parse ("i1.dtrans.disp.x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.1"));
  v = e.parse ("i1.dtrans.disp.y").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.2"));
  v = e.parse ("i2.dtrans.disp.x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-0.1"));
  v = e.parse ("i2.dtrans.disp.y").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.3"));
}
