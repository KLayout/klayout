
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
  std::vector<double> m_subs;
  std::vector<int> m_isubs;
  std::vector<Child> m_children;
  Child m_child;

  Root () : m(0), mi(0) { }

  bool operator== (const Root &x) const { return m == x.m &&
                                          mi == x.mi && m_subs == x.m_subs &&
                                          m_isubs == x.m_isubs && m_children == x.m_children
                                          && m_child == x.m_child; }

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

TEST (1)
{
  Root root;

  tl::PBStruct<Root> structure (
    tl::pb_make_member (&Root::begin_subs, &Root::end_subs, &Root::add_sub, 1) +
    tl::pb_make_member (&Root::begin_isubs, &Root::end_isubs, &Root::add_isub, 2) +
    tl::pb_make_element (&Root::begin_children, &Root::end_children, &Root::add_child, 3,
      tl::pb_make_member (&Child::txt, 1) +
      tl::pb_make_member (&Child::d, 2)
    ) +
    tl::pb_make_element (&Root::get_child, &Root::set_child, 4,
      tl::pb_make_member (&Child::txt, 1) +
      tl::pb_make_member (&Child::d, 2)
    ) +
    tl::pb_make_member (&Root::m, 5) +
    tl::pb_make_member (&Root::get_mi, &Root::set_mi, 6)
  );

  root.add_sub (0.5);
  root.add_sub (7.5);
  root.add_isub (42);
  root.add_isub (1700000000);

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

  std::string fn = tl::combine_path (tl::testtmp (), "pb_test.pb");

  {
    tl::OutputStream os (fn);
    tl::ProtocolBufferWriter writer (os);
    structure.write (writer, root);
  }

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
  EXPECT_EQ (root.m_subs [0], 1.0);
  EXPECT_EQ (root.m_subs [1], -2.5);
  EXPECT_EQ (root.m_isubs.size (), size_t (1));
  EXPECT_EQ (root.m_isubs [0], -100);
  EXPECT_EQ (root.m, 10);
  EXPECT_EQ (root.mi, (unsigned int) 21);
  EXPECT_EQ (root.m_children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].txt, " Text ");
  EXPECT_EQ (fabs (root.m_children [0].d - 0.001) < 1e-12, true);
  EXPECT_EQ (root.m_children [1].txt, "T2");
  EXPECT_EQ (root.m_children [1].d, -1.0);
  EXPECT_EQ (root.m_child.txt, "Single child");
  EXPECT_EQ (root.m_child.d, -1.0);

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
}
