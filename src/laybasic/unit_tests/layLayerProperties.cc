
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



#include "layLayerProperties.h"
#include "layLayoutViewBase.h"
#include "tlXMLParser.h"
#include "tlUnitTest.h"
#include "dbLayout.h"

#include <iostream>
#include <sstream>

size_t size (const lay::LayerPropertiesList &list)
{
  size_t n = 0;
  for (lay::LayerPropertiesConstIterator i = list.begin_const_recursive (); !i.at_end (); ++i) {
    ++n;
  }
  return n;
}

bool compare_real (const lay::LayerPropertiesList &a, const lay::LayerPropertiesList &b)
{
  bool debug = true;
  lay::LayerPropertiesConstIterator i = a.begin_const_recursive ();
  lay::LayerPropertiesConstIterator j = b.begin_const_recursive ();
  unsigned int n = 0;
  while (true) {
    while (! i.at_end () && i->has_children ()) {
      ++i;
    }
    while (! j.at_end () && j->has_children ()) {
      ++j;
    }
    if (! i.at_end () && ! j.at_end ()) {
      if (i->frame_color (true) != j->frame_color (true)) {
        if (debug) printf ("Difference in frame color at element %d\n", n);
        return false;
      }
      if (i->fill_color (true) != j->fill_color (true)) {
        if (debug) printf ("Difference in fill color at element %d\n", n);
        return false;
      }
      if (i->frame_brightness (true) != j->frame_brightness (true)) {
        if (debug) printf ("Difference in frame brightness at element %d\n", n);
        return false;
      }
      if (i->fill_brightness (true) != j->fill_brightness (true)) {
        if (debug) printf ("Difference in fill brightness at element %d\n", n);
        return false;
      }
      if (i->dither_pattern (true) != j->dither_pattern (true)) {
        if (debug) printf ("Difference in dither pattern at element %d\n", n);
        return false;
      }
      if (i->visible (true) != j->visible (true)) {
        if (debug) printf ("Difference in visibility at element %d\n", n);
        return false;
      }
      if (i->transparent (true) != j->transparent (true)) {
        if (debug) printf ("Difference in transparency at element %d\n", n);
        return false;
      }
      if (i->width (true) != j->width (true)) {
        if (debug) printf ("Difference in transparency at element %d\n", n);
        return false;
      }
      if (i->marked (true) != j->marked (true)) {
        if (debug) printf ("Difference in marked state at element %d\n", n);
        return false;
      }
      if (i->animation (true) != j->animation (true)) {
        if (debug) printf ("Difference in animation mode at element %d\n", n);
        return false;
      }
      if (i->source (true) != j->source (true)) {
        if (debug) printf ("Difference in source at element %d\n", n);
        return false;
      }
    } else if (i.at_end () && j.at_end ()) {
      return true;
    } else {
      if (debug) printf ("Length differs\n");
      return false;
    }
    ++i;
    ++j;
    ++n;
  }
}

TEST (1)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  list = lay::LayerPropertiesList ();

  tl::XMLStringSource s2 (os.string ());
  list.load (s2);

  const char *res =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <expanded>false</expanded>\n"
    "  <frame-color/>\n"
    "  <fill-color/>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern/>\n"
    "  <line-style/>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <xfill>false</xfill>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>*/*@*</source>\n"
    " </properties>\n"
    " <name/>\n"
    "</layer-properties>\n";

  tl::OutputStringStream os2;
  tl::OutputStream oss2 (os2);
  list.save (oss2);
  std::string os2_str (os2.string ());

  EXPECT_EQ (os2_str, res);
}

TEST (2a)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <frame-color>#aabbcc</frame-color>\n"
    "    <group-members>\n"
    "      <frame-color>#010203</frame-color>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <frame-color></frame-color>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <frame-color>#020304</frame-color>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <frame-color>#030405</frame-color>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <frame-color>#102030</frame-color>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  list = lay::LayerPropertiesList ();

  std::string s2_str (os.string ());
  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#aabbcc</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#aabbcc</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#020304</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#030405</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#102030</frame-color>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), true);
}

//  Testing correctness of compare_real implementation (hence compare vs. false)
TEST (2b)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <frame-color>#aabbcc</frame-color>\n"
    "    <group-members>\n"
    "      <frame-color>#010203</frame-color>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <frame-color></frame-color>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <frame-color>#020304</frame-color>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <frame-color>#030405</frame-color>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <frame-color>#102030</frame-color>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  list = lay::LayerPropertiesList ();

  std::string s2_str (os.string ());
  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#aabbcc</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#aabbcc</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#020304</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#030405</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#102031</frame-color>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), false);
}

//  Testing correctness of compare_real implementation (hence compare vs. false)
TEST (2c)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <frame-color>#aabbcc</frame-color>\n"
    "    <group-members>\n"
    "      <frame-color>#010203</frame-color>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <frame-color></frame-color>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <frame-color>#020304</frame-color>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <frame-color>#030405</frame-color>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <frame-color>#102030</frame-color>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  list = lay::LayerPropertiesList ();

  std::string s2_str (os.string ());
  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#aabbcc</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#aabbcc</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#020304</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#030405</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#102030</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#102030</frame-color>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), false);
}

//  Testing correctness of compare_real implementation (hence compare vs. false)
TEST (2d)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <frame-color>#aabbcc</frame-color>\n"
    "    <group-members>\n"
    "      <frame-color>#010203</frame-color>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <frame-color></frame-color>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <frame-color>#020304</frame-color>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <frame-color>#030405</frame-color>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <frame-color>#102030</frame-color>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  list = lay::LayerPropertiesList ();

  std::string s2_str (os.string ());
  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#aabbcc</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#aabbcc</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#020304</frame-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#030405</frame-color>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), false);
}

TEST (3)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <fill-color>#aabbcc</fill-color>\n"
    "    <group-members>\n"
    "      <fill-color>#010203</fill-color>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <fill-color></fill-color>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <fill-color>#020304</fill-color>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <fill-color>#030405</fill-color>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <fill-color>#102030</fill-color>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);
  std::string s2_str (os.string ());

  list = lay::LayerPropertiesList ();

  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <fill-color>#aabbcc</fill-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <fill-color>#aabbcc</fill-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <fill-color>#020304</fill-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <fill-color>#030405</fill-color>\n"
    " </properties>\n"
    " <properties>\n"
    "  <fill-color>#102030</fill-color>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), true);
}

TEST (4)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <dither-pattern>1</dither-pattern>\n"
    "    <group-members>\n"
    "      <dither-pattern>2</dither-pattern>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <dither-pattern></dither-pattern>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <dither-pattern></dither-pattern>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <dither-pattern>12</dither-pattern>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <dither-pattern>14</dither-pattern>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);
  std::string s2_str (os.string ());

  list = lay::LayerPropertiesList ();

  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <dither-pattern>1</dither-pattern>\n"
    " </properties>\n"
    " <properties>\n"
    "  <dither-pattern>1</dither-pattern>\n"
    " </properties>\n"
    " <properties>\n"
    "  <dither-pattern></dither-pattern>\n"
    " </properties>\n"
    " <properties>\n"
    "  <dither-pattern>12</dither-pattern>\n"
    " </properties>\n"
    " <properties>\n"
    "  <dither-pattern>14</dither-pattern>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), true);
}

TEST (5)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <visible>1</visible>\n"
    "    <group-members>\n"
    "      <visible>1</visible>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <visible>0</visible>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <visible>0</visible>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <visible>1</visible>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <visible>0</visible>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);
  std::string s2_str (os.string ());

  list = lay::LayerPropertiesList ();

  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <visible>1</visible>\n"
    " </properties>\n"
    " <properties>\n"
    "  <visible>1</visible>\n"
    " </properties>\n"
    " <properties>\n"
    "  <visible>0</visible>\n"
    " </properties>\n"
    " <properties>\n"
    "  <visible>0</visible>\n"
    " </properties>\n"
    " <properties>\n"
    "  <visible>0</visible>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), true);
}

TEST (6)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <transparent>1</transparent>\n"
    "    <group-members>\n"
    "      <transparent>0</transparent>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <transparent>0</transparent>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <transparent>0</transparent>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <transparent>1</transparent>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <transparent>0</transparent>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);
  std::string s2_str (os.string ());

  list = lay::LayerPropertiesList ();

  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <transparent>1</transparent>\n"
    " </properties>\n"
    " <properties>\n"
    "  <transparent>1</transparent>\n"
    " </properties>\n"
    " <properties>\n"
    "  <transparent>0</transparent>\n"
    " </properties>\n"
    " <properties>\n"
    "  <transparent>1</transparent>\n"
    " </properties>\n"
    " <properties>\n"
    "  <transparent>0</transparent>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), true);
}

TEST (7)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <width>1</width>\n"
    "    <group-members>\n"
    "      <width>0</width>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <width>0</width>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <width>0</width>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <width>1</width>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <width>0</width>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);
  std::string s2_str (os.string ());

  list = lay::LayerPropertiesList ();

  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <width>1</width>\n"
    " </properties>\n"
    " <properties>\n"
    "  <width>1</width>\n"
    " </properties>\n"
    " <properties>\n"
    "  <width>0</width>\n"
    " </properties>\n"
    " <properties>\n"
    "  <width>1</width>\n"
    " </properties>\n"
    " <properties>\n"
    "  <width>0</width>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), true);
}

TEST (8)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <animation>1</animation>\n"
    "    <group-members>\n"
    "      <animation>0</animation>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <animation>0</animation>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <animation>0</animation>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <animation>1</animation>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <animation>0</animation>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);
  std::string s2_str (os.string ());

  list = lay::LayerPropertiesList ();

  tl::XMLStringSource s2 (s2_str);
  list.load (s2);

  const char *ref =
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <animation>1</animation>\n"
    " </properties>\n"
    " <properties>\n"
    "  <animation>1</animation>\n"
    " </properties>\n"
    " <properties>\n"
    "  <animation>0</animation>\n"
    " </properties>\n"
    " <properties>\n"
    "  <animation>1</animation>\n"
    " </properties>\n"
    " <properties>\n"
    "  <animation>0</animation>\n"
    " </properties>\n"
    "</layer-properties>\n";

  lay::LayerPropertiesList flat_ref;
  tl::XMLStringSource s3 (ref);
  flat_ref.load (s3);

  EXPECT_EQ (compare_real (flat_ref, list), true);
}


TEST (9)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <animation>1</animation>\n"
    "    <group-members>\n"
    "      <animation>0</animation>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <animation>0</animation>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <animation>0</animation>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <animation>1</animation>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <animation>0</animation>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  EXPECT_EQ (size (list), size_t (8));

  size_t n, nn;
  
  lay::LayerPropertiesIterator end = list.begin_recursive ();
  while (! end.at_end ()) {
    ++end;
  }

  nn = size (list);
  n = 0;
  for (lay::LayerPropertiesIterator iter = list.begin_recursive (); ! iter.at_end (); ++iter, ++n) {
    lay::LayerPropertiesIterator iter2 = list.begin_recursive ();
    for (unsigned int i = 0; i < n; ++i) {
      EXPECT_EQ (iter2 < iter, true);
      EXPECT_EQ (iter < iter2, false);
      ++iter2;
    }
    EXPECT_EQ (*iter2 == *iter, true);
    lay::LayerPropertiesIterator iter3 = iter2;
    if (n < nn) {
      for (unsigned int i = 0; i < nn - n; ++i) {
        ++iter3;
        EXPECT_EQ (iter2 < iter3, true);
        EXPECT_EQ (iter3 < iter2, false);
      }
    }
    EXPECT_EQ (iter3 == end, true);
  }
  EXPECT_EQ (n, size_t (8));
}

TEST (10)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <animation>1</animation>\n"
    "    <group-members>\n"
    "      <animation>2</animation>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <animation>4</animation>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <animation>5</animation>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <animation>6</animation>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <animation>7</animation>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  lay::LayerPropertiesList org_list = list;

  EXPECT_EQ (size (list), size_t (8));

  std::vector<lay::LayerPropertiesNode> nodes;
  std::vector<size_t> positions;

  while (size (list) > 0) {
    lay::LayerPropertiesIterator iter = list.begin_recursive ();
    for (unsigned int i = 0; i < size(list) - 1; ++i) {
      ++iter;
    }
    nodes.push_back (*iter);
    positions.push_back (iter.uint ());
    list.erase (iter);
  }

  EXPECT_EQ (nodes.size (), size_t (8));

  for (unsigned int i = 0; i < 8; ++i) {
    lay::LayerPropertiesIterator iter (list, positions.back ());
    list.insert (iter, nodes.back ());
    nodes.pop_back ();
    positions.pop_back ();
  }

  EXPECT_EQ (list == org_list, true);

  while (size (list) > 0) {
    lay::LayerPropertiesIterator iter = list.begin_recursive ();
    nodes.push_back (*iter);
    positions.push_back (iter.uint ());
    list.erase (iter);
  }

  EXPECT_EQ (nodes.size (), size_t (2));

  for (unsigned int i = 0; i < 2; ++i) {
    lay::LayerPropertiesIterator iter (list, positions.back ());
    list.insert (iter, nodes.back ());
    nodes.pop_back ();
    positions.pop_back ();
  }

  EXPECT_EQ (list == org_list, true);

}

void
build_list (lay::LayerPropertiesConstIterator &iter, lay::LayerPropertiesList &list, lay::LayerPropertiesNode *node)
{
  while (! iter.at_end ()) {
    lay::LayerProperties props (*iter);
    if (node) {
      node->add_child (lay::LayerPropertiesNode (props));
      if (iter->has_children ()) {
        iter.down_first_child ();
        build_list (iter, list, &node->last_child ());
        iter.up ();
      }
    } else {
      list.push_back (lay::LayerPropertiesNode (props));
      if (iter->has_children ()) {
        iter.down_first_child ();
        build_list (iter, list, &list.back ());
        iter.up ();
      }
    }
    iter.next_sibling ();
  }
}

TEST (11)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <animation>1</animation>\n"
    "    <group-members>\n"
    "      <animation>2</animation>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <animation>4</animation>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <animation>5</animation>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <animation>6</animation>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <animation>7</animation>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  lay::LayerPropertiesList new_list;

  lay::LayerPropertiesConstIterator iter (list.begin_recursive ());
  build_list (iter, new_list, 0);

  EXPECT_EQ (size (new_list), size (list));
  EXPECT_EQ (iter.at_end (), true);

  EXPECT_EQ (list == new_list, true);

}

void
test_list (tl::TestBase *_this, lay::LayerPropertiesConstIterator &iter)
{
  lay::LayerPropertiesConstIterator i0 (iter);
  size_t nc = 0;
  while (! iter.at_end ()) {
    lay::LayerProperties props (*iter);
    if (iter->has_children ()) {
      iter.down_first_child ();
      test_list (_this, iter);
      iter.up ();
    }
    EXPECT_EQ (iter.child_index (), nc);
    iter.next_sibling ();
    ++nc;
  }
  i0.next_sibling (nc);
  EXPECT_EQ (iter == i0, true);
}

TEST (12)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <animation>1</animation>\n"
    "    <group-members>\n"
    "      <animation>2</animation>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "  <properties>\n"
    "    <animation>4</animation>\n"
    "    <group-members>\n"
    "      <group-members>\n"
    "        <animation>5</animation>\n"
    "      </group-members>\n"
    "      <group-members>\n"
    "        <animation>6</animation>\n"
    "      </group-members>\n"
    "    </group-members>\n"
    "    <group-members>\n"
    "      <animation>7</animation>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  lay::LayerPropertiesConstIterator iter (list.begin_recursive ());
  test_list (_this, iter);
}

TEST (13)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <frame-color>#112233</frame-color>\n"
    "    <fill-color>#aabbcc</fill-color>\n"
    "    <frame-brightness>-20</frame-brightness>\n"
    "    <fill-brightness>16</fill-brightness>\n"
    "    <dither-pattern>5</dither-pattern>\n"
    "    <visible>0</visible>\n"
    "    <transparent>1</transparent>\n"
    "    <width>3</width>\n"
    "    <marked>1</marked>\n"
    "    <animation>2</animation>\n"
    "    <marked>1</marked>\n"
    "    <source>3/2@1</source>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  lay::LayerPropertiesConstIterator iter (list.begin_recursive ());
  ++iter;
  EXPECT_EQ (iter->has_children (), false);
  const lay::LayerPropertiesNode &node = *iter;
  lay::LayerProperties flat = node.flat ();

  EXPECT_EQ (node.animation (false /*local*/), 0);  
  EXPECT_EQ (node.animation (true /*real*/), 2);  

  EXPECT_EQ (flat.animation (false /*local*/), 2);
  EXPECT_EQ (flat.animation (true /*real*/), 2);

  EXPECT_EQ (flat.eff_fill_color (true) == node.eff_fill_color (true), true);
  EXPECT_EQ (flat.eff_fill_color (false) == node.eff_fill_color (true), true);
  EXPECT_EQ (flat.eff_fill_color (false) == node.eff_fill_color (false), false);
  EXPECT_EQ (flat.eff_frame_color (true) == node.eff_frame_color (true), true);
  EXPECT_EQ (flat.eff_frame_color (false) == node.eff_frame_color (true), true);
  EXPECT_EQ (flat.eff_frame_color (false) == node.eff_frame_color (false), false);
  EXPECT_EQ (flat.frame_brightness (true) == node.frame_brightness (true), true);
  EXPECT_EQ (flat.frame_brightness (false) == node.frame_brightness (true), true);
  EXPECT_EQ (flat.frame_brightness (false) == node.frame_brightness (false), false);
  EXPECT_EQ (flat.fill_brightness (true) == node.fill_brightness (true), true);
  EXPECT_EQ (flat.fill_brightness (false) == node.fill_brightness (true), true);
  EXPECT_EQ (flat.fill_brightness (false) == node.fill_brightness (false), false);
  EXPECT_EQ (flat.dither_pattern (true) == node.dither_pattern (true), true);
  EXPECT_EQ (flat.dither_pattern (false) == node.dither_pattern (true), true);
  EXPECT_EQ (flat.dither_pattern (false) == node.dither_pattern (false), false);
  EXPECT_EQ (flat.visible (true) == node.visible (true), true);
  EXPECT_EQ (flat.visible (false) == node.visible (true), true);
  EXPECT_EQ (flat.visible (false) == node.visible (false), false);
  EXPECT_EQ (flat.transparent (true) == node.transparent (true), true);
  EXPECT_EQ (flat.transparent (false) == node.transparent (true), true);
  EXPECT_EQ (flat.transparent (false) == node.transparent (false), false);
  EXPECT_EQ (flat.marked (true) == node.marked (true), true);
  EXPECT_EQ (flat.marked (false) == node.marked (true), true);
  EXPECT_EQ (flat.marked (false) == node.marked (false), false);
  EXPECT_EQ (flat.width (true) == node.width (true), true);
  EXPECT_EQ (flat.width (false) == node.width (true), true);
  EXPECT_EQ (flat.width (false) == node.width (false), false);
  EXPECT_EQ (flat.animation (true) == node.animation (true), true);
  EXPECT_EQ (flat.animation (false) == node.animation (true), true);
  EXPECT_EQ (flat.animation (false) == node.animation (false), false);
  EXPECT_EQ (flat.source (true) == node.source (true), true);
  EXPECT_EQ (flat.source (false) == node.source (true), true);
  EXPECT_EQ (flat.source (false) == node.source (false), false);
}

TEST (14)
{
  tl::XMLStringSource s (
    "<?xml version=\"1.0\"?>\n"
    "<layer-properties>\n"
    "  <properties>\n"
    "    <frame-color>#112233</frame-color>\n"
    "    <fill-color>#aabbcc</fill-color>\n"
    "    <frame-brightness>-20</frame-brightness>\n"
    "    <fill-brightness>16</fill-brightness>\n"
    "    <dither-pattern>5</dither-pattern>\n"
    "    <visible>0</visible>\n"
    "    <transparent>1</transparent>\n"
    "    <width>3</width>\n"
    "    <marked>1</marked>\n"
    "    <animation>2</animation>\n"
    "    <marked>1</marked>\n"
    "    <source>3/2@1</source>\n"
    "    <group-members>\n"
    "    </group-members>\n"
    "  </properties>\n"
    "</layer-properties>\n");

  lay::LayerPropertiesList list;
  list.load (s);

  lay::LayerPropertiesConstIterator iter (list.begin_recursive ());
  ++iter;
  EXPECT_EQ (iter->has_children (), false);

  const lay::LayerPropertiesNode &node = *iter;
  EXPECT_EQ (node.animation (false /*local*/), 0);  
  EXPECT_EQ (node.animation (true /*real*/), 2);  

  lay::LayerProperties f = node.flat ();
  lay::LayerProperties flat (f);

  EXPECT_EQ (flat.animation (false /*local*/), 2);
  EXPECT_EQ (flat.animation (true /*real*/), 2);

  EXPECT_EQ (flat.eff_fill_color (true) == node.eff_fill_color (true), true);
  EXPECT_EQ (flat.eff_fill_color (false) == node.eff_fill_color (true), true);
  EXPECT_EQ (flat.eff_fill_color (false) == node.eff_fill_color (false), false);
  EXPECT_EQ (flat.eff_frame_color (true) == node.eff_frame_color (true), true);
  EXPECT_EQ (flat.eff_frame_color (false) == node.eff_frame_color (true), true);
  EXPECT_EQ (flat.eff_frame_color (false) == node.eff_frame_color (false), false);
  EXPECT_EQ (flat.frame_brightness (true) == node.frame_brightness (true), true);
  EXPECT_EQ (flat.frame_brightness (false) == node.frame_brightness (true), true);
  EXPECT_EQ (flat.frame_brightness (false) == node.frame_brightness (false), false);
  EXPECT_EQ (flat.fill_brightness (true) == node.fill_brightness (true), true);
  EXPECT_EQ (flat.fill_brightness (false) == node.fill_brightness (true), true);
  EXPECT_EQ (flat.fill_brightness (false) == node.fill_brightness (false), false);
  EXPECT_EQ (flat.dither_pattern (true) == node.dither_pattern (true), true);
  EXPECT_EQ (flat.dither_pattern (false) == node.dither_pattern (true), true);
  EXPECT_EQ (flat.dither_pattern (false) == node.dither_pattern (false), false);
  EXPECT_EQ (flat.visible (true) == node.visible (true), true);
  EXPECT_EQ (flat.visible (false) == node.visible (true), true);
  EXPECT_EQ (flat.visible (false) == node.visible (false), false);
  EXPECT_EQ (flat.transparent (true) == node.transparent (true), true);
  EXPECT_EQ (flat.transparent (false) == node.transparent (true), true);
  EXPECT_EQ (flat.transparent (false) == node.transparent (false), false);
  EXPECT_EQ (flat.marked (true) == node.marked (true), true);
  EXPECT_EQ (flat.marked (false) == node.marked (true), true);
  EXPECT_EQ (flat.marked (false) == node.marked (false), false);
  EXPECT_EQ (flat.width (true) == node.width (true), true);
  EXPECT_EQ (flat.width (false) == node.width (true), true);
  EXPECT_EQ (flat.width (false) == node.width (false), false);
  EXPECT_EQ (flat.animation (true) == node.animation (true), true);
  EXPECT_EQ (flat.animation (false) == node.animation (true), true);
  EXPECT_EQ (flat.animation (false) == node.animation (false), false);
  EXPECT_EQ (flat.source (true) == node.source (true), true);
  EXPECT_EQ (flat.source (false) == node.source (true), true);
  EXPECT_EQ (flat.source (false) == node.source (false), false);
}

TEST (15)
{
  lay::LayerPropertiesList list;

  list.push_back (lay::LayerPropertiesNode ());
  unsigned int id = list.back ().id ();
  lay::LayerPropertiesNode n = list.back ();
  EXPECT_EQ (n.id (), id);

  lay::LayerPropertiesNode nn;
  EXPECT_EQ (nn.id () == id, false);

  list.push_back (nn);
  EXPECT_EQ (list.back ().id (), nn.id ());

  lay::LayerPropertiesNode n2;
  EXPECT_EQ (n2.id () == id, false);
  EXPECT_EQ (n2.id () == nn.id (), false);
  lay::LayerPropertiesIterator iter = list.begin_recursive ();
  ++iter;
  list.insert (iter, n2);

  EXPECT_EQ (list.begin ()[0].id (), id);
  EXPECT_EQ (list.begin ()[1].id (), n2.id ());
  EXPECT_EQ (list.begin ()[2].id (), nn.id ());
}

TEST (16)
{
  lay::LayerPropertiesList list;

  db::Manager mgr (true);
  lay::LayoutViewBase view (&mgr, is_editable (), 0);
  list.attach_view (&view, 0);

  int cv1 = view.create_layout ("", true, false);
  db::Layout &ly1 = view.cellview (cv1)->layout ();
  EXPECT_EQ (ly1.is_editable (), is_editable ());
  ly1.insert_layer (db::LayerProperties (1, 0));
  ly1.insert_layer (db::LayerProperties (2, 0));

  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("1/0@1");

  std::map<int, int> cvmap;
  cvmap.insert (std::make_pair (cv1, cv1));
  list.expand (cvmap, false);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  tl::XMLStringSource s (
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#ff80a8</frame-color>\n"
    "  <fill-color>#ff80a8</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I9</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>1/0@1</source>\n"
    " </properties>\n"
    " <name/>\n"
    "</layer-properties>\n"
  );

  lay::LayerPropertiesList ref;
  ref.load (s);

  EXPECT_EQ (compare_real (ref, list), true);
}

TEST (17)
{
  lay::LayerPropertiesList list;

  db::Manager mgr (true);
  lay::LayoutViewBase view (&mgr, is_editable (), 0);
  list.attach_view (&view, 0);

  int cv1 = view.create_layout ("", true, false);
  db::Layout &ly1 = view.cellview (cv1)->layout ();
  ly1.insert_layer (db::LayerProperties (1, 0));
  ly1.insert_layer (db::LayerProperties (2, 0));

  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("1/0@1");

  std::map<int, int> cvmap;
  cvmap.insert (std::make_pair (cv1, cv1));
  list.expand (cvmap, true);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  tl::XMLStringSource s (
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#ff80a8</frame-color>\n"
    "  <fill-color>#ff80a8</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I9</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>1/0@1</source>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#c080ff</frame-color>\n"
    "  <fill-color>#c080ff</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I5</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>2/0@1</source>\n"
    " </properties>\n"
    " <name/>\n"
    "</layer-properties>\n"
  );

  lay::LayerPropertiesList ref;
  ref.load (s);

  EXPECT_EQ (compare_real (ref, list), true);
}

TEST (18)
{
  lay::LayerPropertiesList list;

  db::Manager mgr (true);
  lay::LayoutViewBase view (&mgr, is_editable (), 0);
  list.attach_view (&view, 0);

  int cv1 = view.create_layout ("", true, false);
  db::Layout &ly1 = view.cellview (cv1)->layout ();
  ly1.insert_layer (db::LayerProperties (1, 0));
  ly1.insert_layer (db::LayerProperties (2, 0));

  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("1/0@1");
  list.push_back (lay::LayerPropertiesNode ());

  std::map<int, int> cvmap;
  cvmap.insert (std::make_pair (cv1, cv1));
  list.expand (cvmap, false);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  tl::XMLStringSource s (
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#ff80a8</frame-color>\n"
    "  <fill-color>#ff80a8</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I9</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>1/0@1</source>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#c080ff</frame-color>\n"
    "  <fill-color>#c080ff</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I5</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>2/0@1</source>\n"
    " </properties>\n"
    " <name/>\n"
    "</layer-properties>\n"
  );

  lay::LayerPropertiesList ref;
  ref.load (s);

  EXPECT_EQ (compare_real (ref, list), true);
}

TEST (19)
{
  lay::LayerPropertiesList list;

  db::Manager mgr (true);
  lay::LayoutViewBase view (&mgr, is_editable (), 0);
  list.attach_view (&view, 0);

  int cv1 = view.create_layout ("", true, false);
  db::Layout &ly1 = view.cellview (cv1)->layout ();
  ly1.insert_layer (db::LayerProperties (1, 0, "L01"));
  ly1.insert_layer (db::LayerProperties (2, 0, "L02"));

  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("1/0@1");
  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("*/*@* (r90)");

  std::map<int, int> cvmap;
  cvmap.insert (std::make_pair (cv1, cv1));
  list.expand (cvmap, false);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  tl::XMLStringSource s (
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#ff80a8</frame-color>\n"
    "  <fill-color>#ff80a8</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I9</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>1/0@1</source>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#c080ff</frame-color>\n"
    "  <fill-color>#c080ff</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I5</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>L02 2/0@1 (r90 *1 0,0)</source>\n"
    " </properties>\n"
    " <name/>\n"
    "</layer-properties>\n"
  );

  lay::LayerPropertiesList ref;
  ref.load (s);

  EXPECT_EQ (compare_real (ref, list), true);
}

TEST (20)
{
  lay::LayerPropertiesList list;

  db::Manager mgr (true);
  lay::LayoutViewBase view (&mgr, is_editable (), 0);
  list.attach_view (&view, 0);

  int cv1 = view.create_layout ("", true, false);
  db::Layout &ly1 = view.cellview (cv1)->layout ();
  ly1.insert_layer (db::LayerProperties (1, 0, "L01"));
  ly1.insert_layer (db::LayerProperties (2, 0, "L02"));

  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("1/0@1");
  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("*/*@* (r90)");
  list.back ().set_frame_color (0x123456);
  list.back ().set_fill_color (0x654321);
  list.back ().set_visible (false);

  std::map<int, int> cvmap;
  cvmap.insert (std::make_pair (cv1, cv1));
  list.expand (cvmap, false);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  tl::XMLStringSource s (
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#ff80a8</frame-color>\n"
    "  <fill-color>#ff80a8</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I9</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>1/0@1</source>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#123456</frame-color>\n"
    "  <fill-color>#654321</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I5</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>false</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>L02 2/0@1 (r90 *1 0,0)</source>\n"
    " </properties>\n"
    " <name/>\n"
    "</layer-properties>\n"
  );

  lay::LayerPropertiesList ref;
  ref.load (s);

  EXPECT_EQ (compare_real (ref, list), true);
}

TEST (21)
{
  lay::LayerPropertiesList list;

  db::Manager mgr (true);
  lay::LayoutViewBase view (&mgr, is_editable (), 0);
  list.attach_view (&view, 0);

  int cv1 = view.create_layout ("", true, false);
  db::Layout &ly1 = view.cellview (cv1)->layout ();
  ly1.insert_layer (db::LayerProperties (2, 0, "L02"));

  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("1/0@1");
  list.push_back (lay::LayerPropertiesNode ());
  list.back ().set_source ("*/*@* (r90)");
  list.back ().set_frame_color (0x123456);
  list.back ().set_fill_color (0x654321);
  list.back ().set_visible (false);

  std::map<int, int> cvmap;
  cvmap.insert (std::make_pair (cv1, cv1));
  list.expand (cvmap, false);

  tl::OutputStringStream os;
  tl::OutputStream oss (os);
  list.save (oss);

  tl::XMLStringSource s (
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<layer-properties>\n"
    " <properties>\n"
    "  <frame-color>#ff80a8</frame-color>\n"
    "  <fill-color>#ff80a8</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I9</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>true</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>1/0@1</source>\n"
    " </properties>\n"
    " <properties>\n"
    "  <frame-color>#123456</frame-color>\n"
    "  <fill-color>#654321</fill-color>\n"
    "  <frame-brightness>0</frame-brightness>\n"
    "  <fill-brightness>0</fill-brightness>\n"
    "  <dither-pattern>I5</dither-pattern>\n"
    "  <valid>true</valid>\n"
    "  <visible>false</visible>\n"
    "  <transparent>false</transparent>\n"
    "  <width/>\n"
    "  <marked>false</marked>\n"
    "  <animation>0</animation>\n"
    "  <name/>\n"
    "  <source>L02 2/0@1 (r90 *1 0,0)</source>\n"
    " </properties>\n"
    " <name/>\n"
    "</layer-properties>\n"
  );

  lay::LayerPropertiesList ref;
  ref.load (s);

  EXPECT_EQ (compare_real (ref, list), true);
}

