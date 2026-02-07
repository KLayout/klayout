
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


#include "layParsedLayerSource.h"
#include "dbLayout.h"
#include "dbCell.h"
#include "tlUnitTest.h"

namespace {

/**
 *  @brief Installs a temporary repository instance for testing
 *
 *  By using a temp instance, we do not disturb other tests.
 */
class TempPropertiesRepository
{
public:
  TempPropertiesRepository ()
  {
    db::PropertiesRepository::replace_instance_temporarily (&m_temp);
  }

  ~TempPropertiesRepository ()
  {
    db::PropertiesRepository::replace_instance_temporarily (0);
  }

private:
  db::PropertiesRepository m_temp;
};

}

TEST (1)
{
  lay::ParsedLayerSource ps1 (1, 2, -1);
  EXPECT_EQ (ps1.to_string (), "1/2@*");

  lay::ParsedLayerSource ps2 (5, 0, 0);
  EXPECT_EQ (ps2.to_string (), "5/0@1");
  EXPECT_EQ (ps2.has_name (), false);

  lay::ParsedLayerSource ps3 ("aname", 1);
  EXPECT_EQ (ps3.to_string (), "aname@2");
  EXPECT_EQ (ps3.has_name (), true);

  lay::ParsedLayerSource ps4 ("bname", -1);
  EXPECT_EQ (ps4.to_string (), "bname@*");

  EXPECT_EQ (ps1 < ps2, true);
  EXPECT_EQ (ps2 < ps2, false);
  EXPECT_EQ (ps2 < ps1, false);
  EXPECT_EQ (ps2 < ps3, true);
  EXPECT_EQ (ps3 < ps4, false);
  EXPECT_EQ (ps4 < ps3, true);

  EXPECT_EQ (ps1 == ps2, false);
  EXPECT_EQ (ps2 == ps3, false);
  EXPECT_EQ (ps3 == ps4, false);
  EXPECT_EQ (ps4 == lay::ParsedLayerSource ("x", -1), false);
  EXPECT_EQ (ps4 == lay::ParsedLayerSource ("bname", -1), true);
  EXPECT_EQ (ps3 == lay::ParsedLayerSource ("aname", 1), true);
  EXPECT_EQ (ps2 == lay::ParsedLayerSource (5, 0, 0), true);

  EXPECT_EQ (ps4 != lay::ParsedLayerSource ("x", -1), true);
  EXPECT_EQ (ps4 != lay::ParsedLayerSource ("bname", -1), false);

  EXPECT_EQ (ps1 == lay::ParsedLayerSource (ps1.to_string ()), true);
  EXPECT_EQ (ps2 == lay::ParsedLayerSource (ps2.to_string ()), true);
  EXPECT_EQ (ps3 == lay::ParsedLayerSource (ps3.to_string ()), true);
  EXPECT_EQ (ps4 == lay::ParsedLayerSource (ps4.to_string ()), true);

  ps1 = ps2;
  lay::ParsedLayerSource psc = ps2;
  EXPECT_EQ (ps1 == ps2, true);
  EXPECT_EQ (ps2 == psc, true);

  EXPECT_EQ (lay::ParsedLayerSource ("4/0@*") == lay::ParsedLayerSource (4, 0, -1), true);
}

TEST (2)
{
  lay::ParsedLayerSource ps1 ("@2");
  EXPECT_EQ (ps1.to_string (), "*/*@2");

  lay::ParsedLayerSource ps2 ("5");
  EXPECT_EQ (ps2.to_string (), "5/0@1");

  lay::ParsedLayerSource ps3 ("/5");
  EXPECT_EQ (ps3.to_string (), "*/5@1");

  lay::ParsedLayerSource ps4 ("name@5");
  EXPECT_EQ (ps4.to_string (), "name@5");

  lay::ParsedLayerSource ps5 ("name");
  EXPECT_EQ (ps5.to_string (), "name@1");

  lay::ParsedLayerSource ps6 ("%5");
  EXPECT_EQ (ps6.to_string (), "%5@1");

  lay::ParsedLayerSource ps7 ("1/5%4@7");
  EXPECT_EQ (ps7.to_string (), "%4@7");
}

TEST (3)
{
  lay::ParsedLayerSource ps1 ("@2");
  ps1 += lay::ParsedLayerSource ("1");
  EXPECT_EQ (ps1.to_string (), "1/0@2");

  lay::ParsedLayerSource ps2 ("@2");
  ps2 += lay::ParsedLayerSource ("@3");
  EXPECT_EQ (ps2.to_string (), "*/*@2");

  lay::ParsedLayerSource ps3 ("1/5@*");
  ps3 += lay::ParsedLayerSource ("@3");
  EXPECT_EQ (ps3.to_string (), "1/5@3");

  lay::ParsedLayerSource ps4 ("namea");
  EXPECT_EQ (ps4.has_name (), true);
  ps4 += lay::ParsedLayerSource ("nameb");
  EXPECT_EQ (ps4.has_name (), true);
  EXPECT_EQ (ps4.to_string (), "namea@1");

  lay::ParsedLayerSource ps5 ("namea@5");
  ps5 += lay::ParsedLayerSource ("1/*");
  EXPECT_EQ (ps5.to_string (), "namea 1/*@5");

  lay::ParsedLayerSource ps6 ("1/5@4");
  EXPECT_EQ (ps6.has_name (), false);
  ps6 += lay::ParsedLayerSource ("nameb");
  EXPECT_EQ (ps6.has_name (), true);
  EXPECT_EQ (ps6.to_string (), "nameb 1/5@4");

  lay::ParsedLayerSource ps7 ("*/5");
  EXPECT_EQ (ps7.has_name (), false);
  ps7 += lay::ParsedLayerSource ("2/7");
  EXPECT_EQ (ps7.has_name (), false);
  EXPECT_EQ (ps7.to_string (), "2/5@1");

  lay::ParsedLayerSource ps8 ("1/*@1");
  ps8 += lay::ParsedLayerSource ("*/8@2");
  EXPECT_EQ (ps8.to_string (), "1/8@1");

  lay::ParsedLayerSource ps9;
  EXPECT_EQ (ps9.to_string (), "*/*@*");
  EXPECT_EQ (ps9.has_name (), false);
  ps9.layer (2);
  EXPECT_EQ (ps9.to_string (), "2/*@*");
  EXPECT_EQ (ps9.has_name (), false);
  ps9.datatype (3);
  EXPECT_EQ (ps9.to_string (), "2/3@*");
  EXPECT_EQ (ps9.has_name (), false);
  ps9.name ("abc");
  EXPECT_EQ (ps9.to_string (), "abc 2/3@*");
  EXPECT_EQ (ps9.has_name (), true);
  ps9.name (std::string ());
  EXPECT_EQ (ps9.to_string (), "2/3@*");
  EXPECT_EQ (ps9.has_name (), false);
}

TEST (4)
{
  lay::ParsedLayerSource ps1 ("@2");
  EXPECT_EQ (ps1.to_string (), "*/*@2");

  lay::ParsedLayerSource ps2 ("@2 (*0.5 -1.0,17.1 m45)");
  EXPECT_EQ (ps2.to_string (), "*/*@2 (m45 *0.5 -1,17.1)");
}

TEST (5)
{
  //  Use a temporary singleton properties repo, so we have better control
  //  over the results of property selectors.
  TempPropertiesRepository tmp_prop_repo;

  lay::ParsedLayerSource ps0 ("@2");

  lay::ParsedLayerSource ps1 ("@2 [ X   == #2 ]");
  EXPECT_EQ (ps1.to_string (), "*/*@2 ['X'==#2]");

  lay::ParsedLayerSource ps2 ("[X==#2||X==Y&&Z!=##4] @2");
  EXPECT_EQ (ps2.to_string (), "*/*@2 [('X'==#2||'X'=='Y')&&'Z'!=##4]");

  lay::ParsedLayerSource ps2a ("[!(X==#2||X==Y&&Z!=##4)] @2");
  EXPECT_EQ (ps2a.to_string (), "*/*@2 [!(('X'==#2||'X'=='Y')&&'Z'!=##4)]");

  lay::ParsedLayerSource ps2b ("[(X!=#2&&X!=Y)||Z==##4] @2");
  EXPECT_EQ (ps2b.to_string (), "*/*@2 [('X'!=#2&&'X'!='Y')||'Z'==##4]");

  lay::ParsedLayerSource ps3 ("[!(X==#2||(X==Y&&Y==X)&&!Z!=##4)] @2");
  EXPECT_EQ (ps3.to_string (), "*/*@2 [!(('X'==#2||('X'=='Y'&&'Y'=='X'))&&!('Z'!=##4))]");

  lay::ParsedLayerSource ps4 ("@2 [X==#2||X==#3||X==#20||X==#120||X==#210||X==#5||X==#15||X==#11||X==#17||X==#18]");
  EXPECT_EQ (ps4.to_string (), "*/*@2 ['X'==#2||'X'==#3||'X'==#20||'X'==#120||'X'==#210||'X'==#5||'X'==#15||'X'==#11||'X'==#17||'X'==#18]");
  EXPECT_EQ (ps4.display_string (0), "@2 ['X'==#2||'X'==#3||'X'==#20||'X'==#120||...]");

  lay::ParsedLayerSource ps4a;
  ps4a = ps4;
  EXPECT_EQ (ps4a.to_string (), ps4.to_string ());
  EXPECT_EQ (ps4a == ps4, true);

  lay::ParsedLayerSource ps4b (ps4);
  EXPECT_EQ (ps4b.to_string (), ps4.to_string ());

  lay::ParsedLayerSource ps4c ("@2 [X==#3||X==#20||X==#120||X==#210||X==#5||X==#15||X==#11||X==#17||X==#18]");
  EXPECT_EQ (ps4c == ps4, false);
  EXPECT_EQ (ps4c < ps4, true);
  EXPECT_EQ (ps4 < ps4c, false);

  lay::ParsedLayerSource ps4d ("@2 [X==#2||X==#4||X==#20||X==#120||X==#210||X==#5||X==#15||X==#11||X==#17||X==#18]");
  EXPECT_EQ (ps4d == ps4, false);
  EXPECT_EQ (ps4d < ps4, false);
  EXPECT_EQ (ps4 < ps4d, true);

  lay::ParsedLayerSource ps4e ("@2 [X==##2||X==#3||X==#20||X==#120||X==#210||X==#5||X==#15||X==#11||X==#17||X==#18]");
  EXPECT_EQ (ps4e == ps4, true);
  EXPECT_EQ (ps4e < ps4, false);
  EXPECT_EQ (ps4 < ps4e, false);

  lay::ParsedLayerSource ps4f ("@2 [X==#222||X==#3||X==#4||X==#20||X==#120||X==#210||X==#5||X==#15||X==#11||X==#17||X==#18]");
  EXPECT_EQ (ps4f == ps4, false);
  EXPECT_EQ (ps4f < ps4, false);
  EXPECT_EQ (ps4 < ps4f, true);

  lay::ParsedLayerSource ps4g ("@2 [X!=#2||X==#3||X==#20||X==#120||X==#210||X==#5||X==#15||X==#11||X==#17||X==#18]");
  EXPECT_EQ (ps4g == ps4, false);
  EXPECT_EQ (ps4g < ps4, false);
  EXPECT_EQ (ps4 < ps4g, true);

  db::PropertiesSet ps;
  std::set<db::properties_id_type> ids;
  bool inv;

  ps.insert (tl::Variant ("X"), tl::Variant (2l));
  db::properties_id_type id1 = db::properties_id (ps);
  EXPECT_EQ (ps1.property_selector ().check (id1), true);
  EXPECT_EQ (ps0.property_selector ().check (id1), true);
  ids.clear ();
  inv = ps1.property_selector ().matching (ids);
  EXPECT_EQ (inv, false);
  EXPECT_EQ (ids.size (), size_t (1));
  EXPECT_EQ (*ids.begin (), id1);
  ids.clear ();
  inv = ps0.property_selector ().matching (ids);
  EXPECT_EQ (inv, true);
  EXPECT_EQ (ids.size (), size_t (0));
  ps.clear ();

  ps.insert (tl::Variant ("X"), tl::Variant (3l));
  db::properties_id_type id2 = db::properties_id (ps);
  EXPECT_EQ (ps1.property_selector ().check (id2), false);
  EXPECT_EQ (ps0.property_selector ().check (id2), true);
  ids.clear ();
  inv = ps1.property_selector ().matching (ids);
  EXPECT_EQ (inv, false);
  EXPECT_EQ (ids.size (), size_t (1));
  EXPECT_EQ (*ids.begin () == id2, false);
  EXPECT_EQ (*ids.begin (), id1);
  ids.clear ();
  inv = ps0.property_selector ().matching (ids);
  EXPECT_EQ (inv, true);
  EXPECT_EQ (ids.size (), size_t (0));
  ps.clear ();

  ps.insert (tl::Variant ("X"), tl::Variant (2l));
  ps.insert (tl::Variant ("Z"), tl::Variant (4.0));
  db::properties_id_type id3 = db::properties_id (ps);
  EXPECT_EQ (ps2.property_selector ().check (id3), false);
  EXPECT_EQ (ps0.property_selector ().check (id3), true);
  ids.clear ();
  inv = ps2.property_selector ().matching (ids);
  EXPECT_EQ (inv, false);
  EXPECT_EQ (ids.size (), size_t (1));
  EXPECT_EQ (*ids.begin () == id3, false);
  EXPECT_EQ (*ids.begin (), id1);
  ids.clear ();
  inv = ps0.property_selector ().matching (ids);
  EXPECT_EQ (inv, true);
  EXPECT_EQ (ids.size (), size_t (0));
  ps.clear ();

  ps.insert (tl::Variant ("X"), tl::Variant (2l));
  ps.insert (tl::Variant ("Z"), tl::Variant (6l));
  db::properties_id_type id4 = db::properties_id (ps);
  EXPECT_EQ (ps2.property_selector ().check (id4), true);
  ids.clear ();
  inv = ps2.property_selector ().matching (ids);
  EXPECT_EQ (inv, false);
  EXPECT_EQ (ids.size (), size_t (2));
  EXPECT_EQ (ids.find (id1) != ids.end (), true);
  EXPECT_EQ (ids.find (id4) != ids.end (), true);
  ps.clear ();

  ps.insert (tl::Variant ("X"), tl::Variant (2l));
  ps.insert (tl::Variant ("Z"), tl::Variant (5.0));
  db::properties_id_type id5 = db::properties_id (ps);
  EXPECT_EQ (ps2.property_selector ().check (id5), true);
  ids.clear ();
  inv = ps2.property_selector ().matching (ids);
  EXPECT_EQ (inv, false);
  EXPECT_EQ (ids.size (), size_t (3));
  EXPECT_EQ (ids.find (id1) != ids.end (), true);
  EXPECT_EQ (ids.find (id4) != ids.end (), true);
  EXPECT_EQ (ids.find (id5) != ids.end (), true);

  EXPECT_EQ (ps2a.property_selector ().check (id5), false);
  ids.clear ();
  inv = ps2a.property_selector ().matching (ids);
  EXPECT_EQ (inv, true);
  EXPECT_EQ (ids.size (), size_t (3));
  EXPECT_EQ (ids.find (id1) != ids.end (), true);
  EXPECT_EQ (ids.find (id4) != ids.end (), true);
  EXPECT_EQ (ids.find (id5) != ids.end (), true);

  EXPECT_EQ (ps2b.property_selector ().check (id5), false);
  ids.clear ();
  inv = ps2b.property_selector ().matching (ids);
  EXPECT_EQ (inv, true);
  EXPECT_EQ (ids.size (), size_t (3));
  EXPECT_EQ (ids.find (id1) != ids.end (), true);
  EXPECT_EQ (ids.find (id4) != ids.end (), true);
  EXPECT_EQ (ids.find (id5) != ids.end (), true);

  ps.clear ();

  db::properties_id_type id6 = db::properties_id (ps);
  EXPECT_EQ (ps2.property_selector ().check (id6), false);
  ids.clear ();
  inv = ps2.property_selector ().matching (ids);
  EXPECT_EQ (inv, false);
  EXPECT_EQ (ids.size (), size_t (3));
  EXPECT_EQ (ids.find (id1) != ids.end (), true);
  EXPECT_EQ (ids.find (id4) != ids.end (), true);
  EXPECT_EQ (ids.find (id5) != ids.end (), true);

  EXPECT_EQ (ps0.property_selector ().check (id6), true);
  ids.clear ();
  inv = ps0.property_selector ().matching (ids);
  EXPECT_EQ (inv, true);
  EXPECT_EQ (ids.size (), size_t (0));

  ps.insert (tl::Variant ("Z"), tl::Variant (5l));
  db::properties_id_type id7 = db::properties_id (ps);
  EXPECT_EQ (ps4.property_selector ().check (id7), false);
  ids.clear ();
  inv = ps4.property_selector ().matching (ids);
  EXPECT_EQ (inv, false);
  EXPECT_EQ (ids.size (), size_t (5));
  EXPECT_EQ (ids.find (id7) == ids.end (), true);

  ps.insert (tl::Variant ("X"), tl::Variant (15l));
  db::properties_id_type id8 = db::properties_id (ps);
  EXPECT_EQ (ps4.property_selector ().check (id8), true);
  ids.clear ();
  inv = ps4.property_selector ().matching (ids);
  EXPECT_EQ (inv, false);
  EXPECT_EQ (ids.size (), size_t (6));
  EXPECT_EQ (ids.find (id8) != ids.end (), true);
}

TEST (6)
{
  lay::ParsedLayerSource ps1;

  ps1 = lay::ParsedLayerSource ("");
  EXPECT_EQ (ps1.to_string (), "*/*@1");

  ps1 = lay::ParsedLayerSource ("#1");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #0..1");

  ps1 = lay::ParsedLayerSource ("#1..4");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #1..4");
  EXPECT_EQ (ps1 == lay::ParsedLayerSource ("#1..4"), true);
  EXPECT_EQ (ps1 != lay::ParsedLayerSource ("#1..4"), false);
  EXPECT_EQ (ps1 == lay::ParsedLayerSource ("#1..5"), false);
  EXPECT_EQ (ps1 != lay::ParsedLayerSource ("#1..5"), true);

  ps1 = lay::ParsedLayerSource ("#1..2");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #1..2");

  ps1 = lay::ParsedLayerSource ("   #   ..   2");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #..2");

  ps1 = lay::ParsedLayerSource (" #   1 ..   ");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #1..");

  ps1 = lay::ParsedLayerSource ();
  ps1 += lay::ParsedLayerSource ("#..20");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #..20");

  ps1 += lay::ParsedLayerSource ("#10..11");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #10..20");

  ps1 = lay::ParsedLayerSource ();
  ps1 += lay::ParsedLayerSource ("#5..");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #5..");

  ps1 += lay::ParsedLayerSource ("#10..11");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #5..11");

  ps1 = lay::ParsedLayerSource ("#*");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #0..*");

  ps1 = lay::ParsedLayerSource ("#..*");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #..*");

  ps1 = lay::ParsedLayerSource ("#..(*)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #..*");

  ps1 = lay::ParsedLayerSource ("#1..*");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #1..*");

  ps1 = lay::ParsedLayerSource ("#1..(*)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #1..*");

  ps1 = lay::ParsedLayerSource ("#(*)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #(0)..*");

  ps1 = lay::ParsedLayerSource ("#(-1)..(5)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #(-1)..(5)");

  ps1 = lay::ParsedLayerSource ("#(2)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #(0)..(2)");

  ps1 = lay::ParsedLayerSource ("#(2)..3");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #(2)..3");

  ps1 = lay::ParsedLayerSource ("#2..(3)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #2..(3)");

  ps1 = lay::ParsedLayerSource ("#>2..(<3)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #>2..(<3)");

  ps1 = lay::ParsedLayerSource ("#>2..(<*)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 #>2..<*");
}

TEST (7)
{
  lay::ParsedLayerSource ps1;
  ps1 = lay::ParsedLayerSource ("(*2)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 (r0 *2 0,0)");
  ps1 = lay::ParsedLayerSource ("(*2) (*1.5)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 (r0 *2 0,0) (r0 *1.5 0,0)");

  lay::ParsedLayerSource ps2 (ps1);
  ps1 += lay::ParsedLayerSource ("(*2)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 (r0 *4 0,0) (r0 *3 0,0)");

  ps1 = ps2;
  ps1 += lay::ParsedLayerSource ("(*2) (*3)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 (r0 *4 0,0) (r0 *6 0,0) (r0 *3 0,0) (r0 *4.5 0,0)");
}

TEST (8)
{
  lay::ParsedLayerSource ps1;
  ps1 = lay::ParsedLayerSource ("(*2) {-* +HALLO}");
  EXPECT_EQ (ps1.to_string (), "*/*@1 {-* +HALLO} (r0 *2 0,0)");
  ps1 = lay::ParsedLayerSource ("{-HALLO} (*2) (*1.5)");
  EXPECT_EQ (ps1.to_string (), "*/*@1 {-HALLO} (r0 *2 0,0) (r0 *1.5 0,0)");
}

TEST (10)
{
  lay::CellSelector sel;

  tl::Extractor ex;

  ex = tl::Extractor ("+HALLO * -H*");
  sel.parse (ex);
  EXPECT_EQ (sel.to_string (), "+HALLO +* -H*");

  ex = tl::Extractor ("+HALLO * -H*}ignored");
  sel.parse (ex);
  EXPECT_EQ (sel.to_string (), "+HALLO +* -H*");
  EXPECT_EQ (ex.test ("}"), true);

  ex = tl::Extractor ("+HALLO (* -H*)");
  sel.parse (ex);
  EXPECT_EQ (sel.to_string (), "+HALLO (+* -H*)");

  ex = tl::Extractor ("( +HALLO -H* ) ( *HA 'WITH BLANK' )");
  sel.parse (ex);
  EXPECT_EQ (sel.to_string (), "(+HALLO -H*) (+*HA +'WITH BLANK')");

  std::string c = sel.to_string ();
  ex = tl::Extractor (c.c_str ());
  sel = lay::CellSelector ();
  sel.parse (ex);
  EXPECT_EQ (sel.to_string (), "(+HALLO -H*) (+*HA +'WITH BLANK')");

  lay::CellSelector sel2;
  EXPECT_EQ (sel2.to_string (), "");
  EXPECT_EQ (sel2.is_empty (), true);
  EXPECT_EQ (sel == sel2, false);
  EXPECT_EQ (sel != sel2, true);
  EXPECT_EQ (sel < sel2, false);
  EXPECT_EQ (sel2 < sel, true);

  sel2 = sel;
  EXPECT_EQ (sel2.to_string (), "(+HALLO -H*) (+*HA +'WITH BLANK')");
  EXPECT_EQ (sel2.is_empty (), false);
  EXPECT_EQ (sel == sel2, true);
  EXPECT_EQ (sel != sel2, false);
  EXPECT_EQ (sel < sel2, false);
  EXPECT_EQ (sel2 < sel, false);

  lay::CellSelector sel2a (sel2);
  EXPECT_EQ (sel2a.to_string (), "(+HALLO -H*) (+*HA +'WITH BLANK')");
}

lay::CellSelector selector_from_string (const char *s)
{
  tl::Extractor ex (s);
  lay::CellSelector sel;
  sel.parse (ex);
  return sel;
}

std::string tspath (const db::Layout &l, db::cell_index_type c, lay::PartialTreeSelector &pt)
{
  std::string r;
  if (pt.is_selected ()) {
    r += "+";
  } else {
    r += "-";
  }
  r += l.cell_name (c);

  bool any = false;

  for (db::Cell::child_cell_iterator cc = l.cell (c).begin_child_cells (); ! cc.at_end (); ++cc) {
    int cs = pt.is_child_selected (*cc);
    if (cs) {
      if (! any) {
        r += "(";
        any = true;
      }
      pt.descend (*cc);
      r += tspath (l, *cc, pt);
      pt.ascend ();
    }
  }

  if (any) {
    r += ")";
  }

  return r;
}

TEST (11)
{
  db::Layout layout;
  db::Cell &c1 = layout.cell (layout.add_cell ("C1"));
  db::Cell &c2 = layout.cell (layout.add_cell ("C2"));
  db::Cell &c3 = layout.cell (layout.add_cell ("C3"));
  db::Cell &c4 = layout.cell (layout.add_cell ("C4"));
  db::Cell &c5 = layout.cell (layout.add_cell ("C5"));
  db::Cell &cc1 = layout.cell (layout.add_cell ("CC1"));
  db::Cell &cc2 = layout.cell (layout.add_cell ("CC2"));
  db::Cell &cc3 = layout.cell (layout.add_cell ("CC3"));
  db::Cell &cc4 = layout.cell (layout.add_cell ("CC4"));
  
  c1.insert (db::CellInstArray (c2.cell_index (), db::Trans ()));
  c2.insert (db::CellInstArray (c3.cell_index (), db::Trans ()));
  c2.insert (db::CellInstArray (c4.cell_index (), db::Trans ()));
  c2.insert (db::CellInstArray (cc2.cell_index (), db::Trans ()));
  c4.insert (db::CellInstArray (cc4.cell_index (), db::Trans ()));
  c3.insert (db::CellInstArray (cc3.cell_index (), db::Trans ()));
  c3.insert (db::CellInstArray (c5.cell_index (), db::Trans ()));
  c1.insert (db::CellInstArray (cc1.cell_index (), db::Trans ()));

  lay::PartialTreeSelector pt (selector_from_string("").create_tree_selector (layout, c1.cell_index ()));
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "+C1(+C2(+C3(+C5+CC3)+C4(+CC4)+CC2)+CC1)");

  pt = selector_from_string("+C1").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "+C1(+C2(+C3(+C5+CC3)+C4(+CC4)+CC2)+CC1)");

  pt = selector_from_string("-C1").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "-C1");

  pt = selector_from_string("-C2").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "+C1(+CC1)");

  pt = selector_from_string("+C1 -C2").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "+C1(+CC1)");

  pt = selector_from_string("+C1 ( -C* +CC* )").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "+C1(+CC1)");

  pt = selector_from_string("-C2 +C3").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "+C1(-C2(+C3(+C5+CC3)-C4(-CC4)-CC2)+CC1)");

  pt = selector_from_string("-C2 +CC*").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "+C1(-C2(-C3(-C5+CC3)-C4(+CC4)+CC2)+CC1)");

  pt = selector_from_string("+CC*").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "-C1(-C2(-C3(-C5+CC3)-C4(+CC4)+CC2)+CC1)");

  pt = selector_from_string("-* +CC*").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "-C1(-C2(-C3(-C5+CC3)-C4(+CC4)+CC2)+CC1)");

  pt = selector_from_string("-* ( -* +CC* )").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "-C1(+CC1)");

  pt = selector_from_string("-C3 +CC*").create_tree_selector (layout, c1.cell_index ());
  EXPECT_EQ (tspath (layout, c1.cell_index (), pt), "+C1(+C2(-C3(-C5+CC3)+C4(+CC4)+CC2)+CC1)");
}

