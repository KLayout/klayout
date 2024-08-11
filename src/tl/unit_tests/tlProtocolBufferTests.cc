
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

#include "tlProtocolBufferStruct.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"

#include <sstream>
#include <cmath>

// @@@
// Missing: all kind of variants (uint8_t, ...), float

//  Basic tests of reader and writer
TEST (1_BasicTypes)
{
  tl::OutputMemoryStream out;

  {
    tl::OutputStream os (out);
    tl::ProtocolBufferWriter writer (os);
    writer.write (1, std::string ("xyz_abc"));
    writer.write (2, float (1.5));
    writer.write (3, double (2.5));
    writer.write (4, true);
    writer.write (5, int32_t (-100000));
    writer.write (6, int32_t (100000));
    writer.write (7, uint32_t (200000));
    writer.write (8, int64_t (-10000000000));
    writer.write (9, int64_t (10000000000));
    writer.write (10, uint64_t (20000000000));
    writer.write (11, int32_t (-100000), true);
    writer.write (12, int32_t (100000), true);
    writer.write (13, uint32_t (200000), true);
    writer.write (14, int64_t (-10000000000), true);
    writer.write (15, int64_t (10000000000), true);
    writer.write (16, uint64_t (20000000000), true);
  }

  {
    tl::InputMemoryStream s2 (out.data (), out.size ());
    tl::InputStream is (s2);
    tl::ProtocolBufferReader reader (is);

    std::string s;
    bool b;
    float f;
    double d;
    uint32_t ui32;
    int32_t i32;
    uint64_t ui64;
    int64_t i64;

    EXPECT_EQ (reader.read_tag (), 1);
    s = "";
    reader.read (s);
    EXPECT_EQ (s, "xyz_abc");
    EXPECT_EQ (reader.read_tag (), 2);
    f = 0;
    reader.read (f);
    EXPECT_EQ (f, 1.5);
    EXPECT_EQ (reader.read_tag (), 3);
    d = 0;
    reader.read (d);
    EXPECT_EQ (d, 2.5);
    EXPECT_EQ (reader.read_tag (), 4);
    b = false;
    reader.read (b);
    EXPECT_EQ (b, true);
    EXPECT_EQ (reader.read_tag (), 5);
    i32 = 0;
    reader.read (i32);
    EXPECT_EQ (i32, -100000);
    EXPECT_EQ (reader.read_tag (), 6);
    i32 = 0;
    reader.read (i32);
    EXPECT_EQ (i32, 100000);
    EXPECT_EQ (reader.read_tag (), 7);
    ui32 = 0;
    reader.read (ui32);
    EXPECT_EQ (ui32, 200000u);
    EXPECT_EQ (reader.read_tag (), 8);
    i64 = 0;
    reader.read (i64);
    EXPECT_EQ (i64, -10000000000);
    EXPECT_EQ (reader.read_tag (), 9);
    i64 = 0;
    reader.read (i64);
    EXPECT_EQ (i64, 10000000000);
    EXPECT_EQ (reader.read_tag (), 10);
    ui64 = 0;
    reader.read (ui64);
    EXPECT_EQ (ui64, 20000000000u);
    EXPECT_EQ (reader.read_tag (), 11);
    i32 = 0;
    reader.read (i32);
    EXPECT_EQ (i32, -100000);
    EXPECT_EQ (reader.read_tag (), 12);
    i32 = 0;
    reader.read (i32);
    EXPECT_EQ (i32, 100000);
    EXPECT_EQ (reader.read_tag (), 13);
    ui32 = 0;
    reader.read (ui32);
    EXPECT_EQ (ui32, 200000u);
    EXPECT_EQ (reader.read_tag (), 14);
    i64 = 0;
    reader.read (i64);
    EXPECT_EQ (i64, -10000000000);
    EXPECT_EQ (reader.read_tag (), 15);
    i64 = 0;
    reader.read (i64);
    EXPECT_EQ (i64, 10000000000);
    EXPECT_EQ (reader.read_tag (), 16);
    ui64 = 0;
    reader.read (ui64);
    EXPECT_EQ (ui64, 20000000000u);
    EXPECT_EQ (reader.at_end (), true);
  }
}

struct Child {
  Child () : txt (""), d(-1), live(true) { }
  ~Child () { tl_assert (live); live = false; }
  std::string txt;
  double d;
  bool live;
  bool operator== (const Child &x) const { return txt == x.txt && fabs (d - x.d) < 1e-9 && children == x.children; }
  std::vector<Child> children;
  std::vector<Child>::const_iterator begin_children() const { return children.begin (); }
  std::vector<Child>::const_iterator end_children() const { return children.end (); }
  void add_child (const Child &c) { children.push_back (c); }
  void add_child_ptr (Child *c) { children.push_back (*c); delete c; }
};

struct Root {
  long m;
  unsigned int mi;
  bool b;
  std::vector<double> m_subs;
  std::vector<int> m_isubs;
  std::vector<Child> m_children;
  Child m_child;

  Root () : m(0), mi(0), b(false) { }

  bool operator== (const Root &x) const { return m == x.m &&
                                          mi == x.mi && m_subs == x.m_subs &&
                                          m_isubs == x.m_isubs && m_children == x.m_children
                                          && m_child == x.m_child && b == x.b; }

  int get_mi () const { return mi; }
  void set_mi (int i) { mi = i; }
  void add_sub (const double &s) {
    m_subs.push_back (s);
  }
  void add_isub (const int &s) {
    m_isubs.push_back (s);
  }
  std::vector<double>::const_iterator begin_subs () const {
    return m_subs.begin ();
  }
  std::vector<double>::const_iterator end_subs () const {
    return m_subs.end ();
  }
  std::vector<int>::const_iterator begin_isubs () const {
    return m_isubs.begin ();
  }
  std::vector<int>::const_iterator end_isubs () const {
    return m_isubs.end ();
  }
  void add_child_ptr (Child *c) {
    m_children.push_back (*c);
    delete c;
  }
  void add_child (const Child &c) {
    m_children.push_back (c);
  }
  std::vector<Child>::const_iterator begin_children () const {
    return m_children.begin ();
  }
  std::vector<Child>::const_iterator end_children () const {
    return m_children.end ();
  }
  void set_child (const Child &child) { m_child = child; }
  const Child &get_child () const { return m_child; }
};

TEST (100_BasicStruct)
{
  Root root;

  tl::PBElementList child_struct =
      tl::pb_make_member (&Child::txt, 1) +
      tl::pb_make_member (&Child::d, 2) +
      tl::pb_make_element (&Child::begin_children, &Child::end_children, &Child::add_child, 3, &child_struct);

  tl::PBStruct<Root> structure ("pbtest-struct", 88888888,
    tl::pb_make_member (&Root::begin_subs, &Root::end_subs, &Root::add_sub, 1) +
    tl::pb_make_member (&Root::begin_isubs, &Root::end_isubs, &Root::add_isub, 2) +
    tl::pb_make_element (&Root::begin_children, &Root::end_children, &Root::add_child, 3, &child_struct) +
    tl::pb_make_element (&Root::get_child, &Root::set_child, 4,
      tl::pb_make_member (&Child::txt, 1) +
      tl::pb_make_member (&Child::d, 2)
    ) +
    tl::pb_make_member (&Root::m, 5) +
    tl::pb_make_member (&Root::get_mi, &Root::set_mi, 6) +
    tl::pb_make_member (&Root::b, 7)
  );

  root.add_sub (0.5);
  root.add_sub (7.5);
  root.add_isub (420000000);
  root.m = -1700000;
  root.set_mi (21);
  root.b = true;

  Child c1;
  c1.txt = "c1";
  c1.d = 1.0;
  root.add_child (c1);

  Child c2;
  c2.txt = "c2";
  c2.d = 2.0;

  Child c21;
  c21.txt = "c21";
  c21.d = 2.1;
  c2.add_child (c21);

  Child c22;
  c22.txt = "c22";
  c22.d = 2.2;
  c2.add_child (c22);

  Child c23;
  c23.txt = "c23";
  c23.d = 2.3;
  c2.add_child (c23);

  root.add_child (c2);

  Child sc;
  sc.txt = "single";
  sc.d = 4.2e6;
  root.set_child (sc);

  std::string fn = tl::combine_path (tl::testtmp (), "pb_test.pb");

  {
    tl::OutputStream os (fn);
    tl::ProtocolBufferWriter writer (os);
    structure.write (writer, root);
    /*
    for debugging:
    tl::ProtocolBufferDumper dumper;
    structure.write (dumper, root);
    */
  }

  root = Root ();

  std::string error;
  try {
    tl::InputStream is (fn);
    tl::ProtocolBufferReader reader (is);
    structure.parse (reader, root);
  } catch (tl::Exception &ex) {
    error = ex.msg ();
  }

  //  TODO: adjust
  EXPECT_EQ (error, "");
  EXPECT_EQ (root.m_subs.size (), size_t (2));
  EXPECT_EQ (root.m_subs [0], 0.5);
  EXPECT_EQ (root.m_subs [1], 7.5);
  EXPECT_EQ (root.m_isubs.size (), size_t (1));
  EXPECT_EQ (root.m_isubs [0], 420000000);
  EXPECT_EQ (root.m, -1700000);
  EXPECT_EQ (root.b, true);
  EXPECT_EQ (root.mi, (unsigned int) 21);
  EXPECT_EQ (root.m_children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].txt, "c1");
  EXPECT_EQ (root.m_children [0].d, 1.0);
  EXPECT_EQ (root.m_children [1].txt, "c2");
  EXPECT_EQ (root.m_children [1].d, 2.0);
  EXPECT_EQ (root.m_children [1].end_children () - root.m_children [1].begin_children (), 3);
  EXPECT_EQ (root.m_child.txt, "single");
  EXPECT_EQ (root.m_child.d, 4.2e6);

  //  write ..
  tl::OutputMemoryStream out;

  {
    tl::OutputStream os (out);
    tl::ProtocolBufferWriter writer (os);
    structure.write (writer, root);
  }

  //  and read again.
  Root rsave (root);

  try {
    error.clear ();
    tl::InputMemoryStream s2 (out.data (), out.size ());
    tl::InputStream is (s2);
    root = Root ();
    tl::ProtocolBufferReader reader (is);
    structure.parse (reader, root);
  } catch (tl::Exception &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (root == rsave, true);

  //  write empty object
  out.clear ();
  root = Root ();

  {
    tl::OutputStream os (out);
    tl::ProtocolBufferWriter writer (os);
    structure.write (writer, root);
  }

  //  and read again.
  try {
    error.clear ();
    tl::InputMemoryStream s2 (out.data (), out.size ());
    tl::InputStream is (s2);
    root = Root ();
    tl::ProtocolBufferReader reader (is);
    structure.parse (reader, root);
  } catch (tl::Exception &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (root == Root (), true);
}

struct TestClass
{
  enum enum_type { A, B, C };

  TestClass () : e (A) { }
  enum_type e;
};

struct TestClassEnumConverter
{
  typedef uint32_t pb_type;

  pb_type pb_encode (TestClass::enum_type e) const
  {
    switch (e) {
    case TestClass::A:
      return 17;
    case TestClass::B:
      return 18;
    case TestClass::C:
      return 19;
    default:
      return 0;
    }
  }

  void pb_decode (uint32_t value, TestClass::enum_type &e) const
  {
    switch (value) {
    case 17:
      e = TestClass::A;
      break;
    case 18:
      e = TestClass::B;
      break;
    case 19:
      e = TestClass::C;
      break;
    default:
      e = TestClass::enum_type (0);
      break;
    }
  }
};

TEST (101_Converter)
{
  TestClass tc;

  tl::PBStruct<TestClass> structure ("pbtest-tc", 1,
    tl::pb_make_member (&TestClass::e, 2, TestClassEnumConverter ())
  );

  tc.e = TestClass::A;
  std::string fn = tl::combine_path (tl::testtmp (), "pb_101a.pb");

  {
    tl::OutputStream os (fn);
    tl::ProtocolBufferWriter writer (os);
    structure.write (writer, tc);
  }

  tc = TestClass ();

  std::string error;
  try {
    tl::InputStream is (fn);
    tl::ProtocolBufferReader reader (is);
    structure.parse (reader, tc);
  } catch (tl::Exception &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (tc.e, TestClass::A);

  tc.e = TestClass::B;

  fn = tl::combine_path (tl::testtmp (), "pb_101b.pb");

  {
    tl::OutputStream os (fn);
    tl::ProtocolBufferWriter writer (os);
    structure.write (writer, tc);
  }

  tc = TestClass ();

  error.clear ();
  try {
    tl::InputStream is (fn);
    tl::ProtocolBufferReader reader (is);
    structure.parse (reader, tc);
  } catch (tl::Exception &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (tc.e, TestClass::B);
}

