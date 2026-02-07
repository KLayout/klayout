
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

#include "tlXMLParser.h"
#include "tlUnitTest.h"

#if defined(HAVE_QT) || defined(HAVE_EXPAT)

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
  tl::XMLStringSource s ("<?xml version=\"1.0\"?>\n"
                         "<root>"
                         "  <member>"
                         "    10"
                         "  </member>"
                         "  <imember>21</imember>"
                         "  <sub>1.0</sub>"
                         "  <isub>-100</isub>"
                         "  <sub>-2.5</sub>"
                         "  <child><t> Text </t><d>2.5</d><d>1e-3</d></child>"
                         "  <child><t>T2</t></child>"
                         "  <c><t>Single child</t></c>"
                         "</root>");

  Root root;

  tl::XMLStruct<Root> structure ("root",
    tl::make_member (&Root::begin_subs, &Root::end_subs, &Root::add_sub, "sub") +
    tl::make_member (&Root::begin_isubs, &Root::end_isubs, &Root::add_isub, "isub") +
    tl::make_element (&Root::begin_children, &Root::end_children, &Root::add_child, "child", 
      tl::make_member (&Child::txt, "t") +
      tl::make_member (&Child::d, "d")
    ) +
    tl::make_element (&Root::get_child, &Root::set_child, "c",
      tl::make_member (&Child::txt, "t") +
      tl::make_member (&Child::d, "d")
    ) +
    tl::make_member (&Root::m, "member") +
    tl::make_member (&Root::get_mi, &Root::set_mi, "imember")
  );

  std::string error;
  try {
    structure.parse (s, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

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
  tl::OutputStringStream out;
  tl::OutputStream os (out);
  structure.write (os, root);

  //  and read again.
  tl::XMLStringSource s2 (out.string ());

  Root rsave (root);
  root = Root ();

  try {
    structure.parse (s2, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (root == rsave, true);
}

TEST (5)
{
  tl::XMLStringSource s ("<?xml version=\"1.0\"?>\n"
                         "<root>"
                         "  <member>1</member>"
                         "</ruut>");

  Root root;

  tl::XMLStruct<Root> structure ("root",
    tl::make_member (&Root::m, "member")
  );

  std::string error;
  try {
    structure.parse (s, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

#if !defined (HAVE_EXPAT)
  EXPECT_EQ (error, "XML parser error: tag mismatch in line 2, column 33");
#else
  EXPECT_EQ (error, "XML parser error: mismatched tag in line 2, column 28");
#endif
}

TEST (6)
{
  tl::XMLStringSource s ("<?xml version=\"1.0\"?>\n"
                         "<root>"
                         "  <member>1a</member>"
                         "</root>");

  Root root;

  tl::XMLStruct<Root> structure ("root",
    tl::make_member (&Root::m, "member")
  );

  std::string error;
  try {
    structure.parse (s, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

#if !defined (HAVE_EXPAT)
  EXPECT_EQ (error, "XML parser error: Unexpected text after numeric value: '...a' in line 2, column 27");
#else
  //  expat delivers cdata at beginning of closing tag
  EXPECT_EQ (error, "XML parser error: Unexpected text after numeric value: '...a' in line 2, column 18");
#endif
}

TEST (7)
{
  std::string tmp_file = tl::TestBase::tmp_file ("tmp_tlXMLParser_7.xml");

  FILE *f = fopen (tmp_file.c_str (), "w");
  tl_assert (f != NULL);

  fprintf (f, 
    "<?xml version=\"1.0\"?>\n"
    "<root>\n"
    "  <member>\n"
    "    10\n"
    "  </member>\n"
    "  <sub>1.0</sub>\n"
    "  <isub>-100</isub>\n"
    "  <sub>-2.5</sub>\n"
    "  <child><t> Text </t><d>2.5</d><d>1e-3</d></child>\n"
    "  <child><t>T2</t></child>\n"
    "</root>\n");
  fclose (f);

  tl::XMLFileSource s (tmp_file.c_str ());

  Root root;

  tl::XMLStruct<Root> structure ("root",
    tl::make_member (&Root::add_sub, "sub") +
    tl::make_member (&Root::add_isub, "isub") +
    tl::make_element (&Root::add_child, "child", 
      tl::make_member (&Child::txt, "t") +
      tl::make_member (&Child::d, "d")
    ) +
    tl::make_member (&Root::m, "member")
  );

  std::string error;
  try {
    structure.parse (s, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (root.m_subs.size (), size_t (2));
  EXPECT_EQ (root.m_subs [0], 1.0);
  EXPECT_EQ (root.m_subs [1], -2.5);
  EXPECT_EQ (root.m_isubs.size (), size_t (1));
  EXPECT_EQ (root.m_isubs [0], -100);
  EXPECT_EQ (root.m, 10);
  EXPECT_EQ (root.m_children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].txt, " Text ");
  EXPECT_EQ (fabs (root.m_children [0].d - 0.001) < 1e-12, true);
  EXPECT_EQ (root.m_children [1].txt, "T2");
  EXPECT_EQ (root.m_children [1].d, -1.0);
}

TEST (7a)
{
  std::string tmp_file = tl::TestBase::tmp_file ("tmp_tlXMLParser_7a.xml");

  FILE *f = fopen (tmp_file.c_str (), "w");
  tl_assert (f != NULL);

  fprintf (f, 
    "<?xml version=\"1.0\"?>\n"
    "<root>\n"
    "  <member>\n"
    "    10\n"
    "  </member>\n"
    "  <sub>1.0</sub>\n"
    "  <isub>-100</isub>\n"
    "  <sub>-2.5</sub>\n"
    "  <child><t> Text </t><d>2.5</d><d>1e-3</d></child>\n"
    "  <child><t>T2</t></child>\n"
    "</root>\n");
  fclose (f);

  tl::XMLFileSource s (tmp_file.c_str ());

  Root root;

  tl::XMLStruct<Root> structure ("root",
    tl::make_member (&Root::add_sub, "sub") +
    tl::make_member (&Root::add_isub, "isub") +
    tl::make_element (&Root::add_child_ptr, "child", 
      tl::make_member (&Child::txt, "t") +
      tl::make_member (&Child::d, "d")
    ) +
    tl::make_member (&Root::m, "member")
  );

  std::string error;
  try {
    structure.parse (s, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (root.m_subs.size (), size_t (2));
  EXPECT_EQ (root.m_subs [0], 1.0);
  EXPECT_EQ (root.m_subs [1], -2.5);
  EXPECT_EQ (root.m_isubs.size (), size_t (1));
  EXPECT_EQ (root.m_isubs [0], -100);
  EXPECT_EQ (root.m, 10);
  EXPECT_EQ (root.m_children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].txt, " Text ");
  EXPECT_EQ (fabs (root.m_children [0].d - 0.001) < 1e-12, true);
  EXPECT_EQ (root.m_children [1].txt, "T2");
  EXPECT_EQ (root.m_children [1].d, -1.0);
}

TEST (8)
{
  std::string tmp_file = tl::TestBase::tmp_file ("tmp_tlXMLParser_8.xml");

  FILE *f = fopen (tmp_file.c_str (), "w");
  tl_assert (f != NULL);

  fprintf (f, 
    "<?xml version=\"1.0\"?>\n"
    "<root>\n"
    "  <member>\n"
    "    10\n"
    "  </member>\n"
    "  <child><t> Text </t>\n"
    "    <child><t>C1</t></child>\n"
    "    <child><t>c2</t><child><t>d2</t><d>-1.25</d></child></child>\n"
    "    <d>2.5</d><d>125e-3</d></child>\n"
    "  <child><t>T2</t></child>\n"
    "</root>\n");
  fclose (f);

  tl::XMLFileSource s (tmp_file.c_str ());

  Root root;

  tl::XMLElementList child_struct = 
      tl::make_member (&Child::txt, "t") +
      tl::make_member (&Child::d, "d") +
      tl::make_element (&Child::begin_children, &Child::end_children, &Child::add_child, "child", &child_struct);

  tl::XMLStruct<Root> structure ("root",
    tl::make_element (&Root::begin_children, &Root::end_children, &Root::add_child, "child", &child_struct) +
    tl::make_member (&Root::m, "member")
  );

  std::string error;
  try {
    structure.parse (s, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  //  write ..
  tl::OutputStringStream out;
  tl::OutputStream os (out);
  structure.write (os, root);
  std::string xml = out.string ();

  EXPECT_EQ (error, "");
  EXPECT_EQ (root.m, 10);
  EXPECT_EQ (root.m_children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].txt, " Text ");
  EXPECT_EQ (fabs (root.m_children [0].d - 0.125) < 1e-12, true);
  EXPECT_EQ (root.m_children [0].children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].children [0].txt, "C1");
  EXPECT_EQ (root.m_children [0].children [1].txt, "c2");
  EXPECT_EQ (root.m_children [0].children [1].children.size (), size_t (1));
  EXPECT_EQ (root.m_children [0].children [1].children [0].txt, "d2");
  EXPECT_EQ (root.m_children [1].txt, "T2");
  EXPECT_EQ (root.m_children [1].d, -1.0);

  tl::XMLStringSource xml_src (xml);
  Root root2;
  try {
    structure.parse (xml_src, root2);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (root == root2, true);

}

TEST (8a)
{
  std::string tmp_file = tl::TestBase::tmp_file ("tmp_tlXMLParser_8a.xml");

  FILE *f = fopen (tmp_file.c_str (), "w");
  tl_assert (f != NULL);

  fprintf (f, 
    "<?xml version=\"1.0\"?>\n"
    "<root>\n"
    "  <member>\n"
    "    10\n"
    "  </member>\n"
    "  <child><t> Text </t>\n"
    "    <child><t>C1</t></child>\n"
    "    <child><t>c2</t><child><t>d2</t><d>-1.25</d></child></child>\n"
    "    <d>2.5</d><d>125e-3</d></child>\n"
    "  <child><t>T2</t></child>\n"
    "</root>\n");
  fclose (f);

  tl::XMLFileSource s (tmp_file.c_str ());

  Root root;

  tl::XMLElementList child_struct = 
      tl::make_member (&Child::txt, "t") +
      tl::make_member (&Child::d, "d") +
      tl::make_element (&Child::begin_children, &Child::end_children, &Child::add_child_ptr, "child", &child_struct);

  tl::XMLStruct<Root> structure ("root",
    tl::make_element (&Root::begin_children, &Root::end_children, &Root::add_child_ptr, "child", &child_struct) +
    tl::make_member (&Root::m, "member")
  );

  std::string error;
  try {
    structure.parse (s, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  //  write ..
  tl::OutputStringStream out;
  tl::OutputStream os (out);
  structure.write (os, root);
  std::string xml = out.string ();

  EXPECT_EQ (error, "");
  EXPECT_EQ (root.m, 10);
  EXPECT_EQ (root.m_children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].txt, " Text ");
  EXPECT_EQ (fabs (root.m_children [0].d - 0.125) < 1e-12, true);
  EXPECT_EQ (root.m_children [0].children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].children [0].txt, "C1");
  EXPECT_EQ (root.m_children [0].children [1].txt, "c2");
  EXPECT_EQ (root.m_children [0].children [1].children.size (), size_t (1));
  EXPECT_EQ (root.m_children [0].children [1].children [0].txt, "d2");
  EXPECT_EQ (root.m_children [1].txt, "T2");
  EXPECT_EQ (root.m_children [1].d, -1.0);

  tl::XMLStringSource xml_src (xml);
  Root root2;
  try {
    structure.parse (xml_src, root2);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (root == root2, true);

}

TEST (9)
{
  //  unknown elements shall be ignored.
  std::string x =
    "<?xml version=\"1.0\"?>\n"
    "<root>\n"
    "  <member>\n"
    "    10\n"
    "  </member>\n"
    "  <unknown-member>15</unknown-member>\n"
    "  <unknown-child><t>blabla</t>\n"
    "    <child><t>C1</t></child>\n"
    "    <child><t>c2</t><child><t>d2</t><d>-1.25</d><unknown-data>blabla</unknown-data></child></child>\n"
    "    <d>2.5</d><d>125e-3</d>\n"
    "    <unknown-data>2.5</unknown-data>\n"
    "  </unknown-child>\n"
    "  <child><t> Text </t>\n"
    "    <child><t>C1</t></child>\n"
    "    <child><t>c2</t><child><t>d2</t><d>-1.25</d><unknown-data>blabal</unknown-data></child></child>\n"
    "    <d>2.5</d><d>125e-3</d>\n"
    "    <unknown-data>2.5</unknown-data>\n"
    "  </child>\n"
    "  <unknown-child><t>TT</t></unknown-child>\n"
    "  <child><t>T2</t></child>\n"
    "</root>\n";

  tl::XMLStringSource s (x);

  Root root;

  tl::XMLElementList child_struct = 
      tl::make_member (&Child::txt, "t") +
      tl::make_member (&Child::d, "d") +
      tl::make_element (&Child::begin_children, &Child::end_children, &Child::add_child_ptr, "child", &child_struct);

  tl::XMLStruct<Root> structure ("root",
    tl::make_element (&Root::begin_children, &Root::end_children, &Root::add_child_ptr, "child", &child_struct) +
    tl::make_member (&Root::m, "member")
  );

  std::string error;
  try {
    structure.parse (s, root);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (root.m, 10);
  EXPECT_EQ (root.m_children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].txt, " Text ");
  EXPECT_EQ (fabs (root.m_children [0].d - 0.125) < 1e-12, true);
  EXPECT_EQ (root.m_children [0].children.size (), size_t (2));
  EXPECT_EQ (root.m_children [0].children [0].txt, "C1");
  EXPECT_EQ (root.m_children [0].children [1].txt, "c2");
  EXPECT_EQ (root.m_children [0].children [1].children.size (), size_t (1));
  EXPECT_EQ (root.m_children [0].children [1].children [0].txt, "d2");
  EXPECT_EQ (root.m_children [1].txt, "T2");
  EXPECT_EQ (root.m_children [1].d, -1.0);
}

TEST (10)
{
  //  UTF8 encoding
  std::string x =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<child><t>H\xc3\xa4llo</t>\n"
    "</child>\n";

  tl::XMLStringSource s (x);

  Child child;

  tl::XMLStruct<Child> child_struct ("child", tl::make_member (&Child::txt, "t"));

  std::string error;
  try {
    child_struct.parse (s, child);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (child.txt, "H\xc3\xa4llo");
}

#if defined(HAVE_EXPAT) || QT_VERSION < 0x60000   // #QTBUG-98656 (XML reader does not read encoding properly)
TEST (11)
{
  //  iso8859-1 encoding
  std::string x =
    "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
    "<child><t>H\xe4llo</t>\n"
    "</child>\n";

  tl::XMLStringSource s (x);

  Child child;

  tl::XMLStruct<Child> child_struct ("child", tl::make_member (&Child::txt, "t"));

  std::string error;
  try {
    child_struct.parse (s, child);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (child.txt, "H\xc3\xa4llo");
}
#endif

TEST (12)
{
  //  UTF8 encoding
  std::string x =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<child><t>H\xc3\xa4llo</t>\n"
    "</child>\n";

  tl::InputMemoryStream ms (x.c_str (), x.size ());
  tl::InputStream is (ms);
  tl::XMLStreamSource s (is);

  Child child;

  tl::XMLStruct<Child> child_struct ("child", tl::make_member (&Child::txt, "t"));

  std::string error;
  try {
    child_struct.parse (s, child);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (child.txt, "H\xc3\xa4llo");
}

#if defined(HAVE_EXPAT) || QT_VERSION < 0x60000   // #QTBUG-98656 (XML reader does not read encoding properly)
TEST (13)
{
  //  iso8859 encoding
  std::string x =
    "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
    "<child><t>H\xe4llo</t>\n"
    "</child>\n";

  tl::InputMemoryStream ms (x.c_str (), x.size ());
  tl::InputStream is (ms);
  tl::XMLStreamSource s (is);

  Child child;

  tl::XMLStruct<Child> child_struct ("child", tl::make_member (&Child::txt, "t"));

  std::string error;
  try {
    child_struct.parse (s, child);
  } catch (tl::XMLException &ex) {
    error = ex.msg ();
  }

  EXPECT_EQ (error, "");
  EXPECT_EQ (child.txt, "H\xc3\xa4llo");
}
#endif

#endif
