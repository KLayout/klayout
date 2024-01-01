
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


#include "dbLayout.h"
#include "dbLayerMapping.h"
#include "tlString.h"
#include "tlUnitTest.h"

std::string nl2s (const std::vector<unsigned int> &nl, const db::Layout &a)
{
  std::string res;
  for (std::vector<unsigned int>::const_iterator i = nl.begin (); i != nl.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += a.get_properties (*i).to_string ();
  }
  return res;
}

std::string m2s (const db::LayerMapping &lm, const db::Layout &a, const db::Layout &b)
{
  std::string res;
  for (db::LayerMapping::iterator i = lm.begin (); i != lm.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += a.get_properties (i->second).to_string ();
    res += "->";
    res += b.get_properties (i->first).to_string ();
  }
  return res;
}

TEST(1) 
{
  // some basic example

  db::Layout g;
  g.insert_layer (db::LayerProperties (1, 0));
  g.insert_layer (db::LayerProperties (2, 0));
  g.insert_layer (db::LayerProperties ("L1"));
  g.insert_layer (db::LayerProperties ("L2"));
  g.insert_layer (db::LayerProperties (10, 17, "L10D17"));
  g.insert_layer (db::LayerProperties (11, 17, "L11D17"));

  db::Layout h;
  h.insert_layer (db::LayerProperties (1, 0));
  h.insert_layer (db::LayerProperties ("L1"));
  h.insert_layer (db::LayerProperties ("L3"));
  h.insert_layer (db::LayerProperties (10, 17, "L10D17"));
  h.insert_layer (db::LayerProperties (10, 18, "L10D18"));

  db::LayerMapping lm;
  lm.create (g, h);
  EXPECT_EQ (m2s (lm, g, h), "1/0->1/0;L1->L1;L10D17 (10/17)->L10D17 (10/17)");
  lm.clear ();
  lm.create (h, g);
  EXPECT_EQ (m2s (lm, h, g), "1/0->1/0;L1->L1;L10D17 (10/17)->L10D17 (10/17)");

  std::vector<unsigned int> nl;

  db::Layout gg = g;
  lm.clear ();
  nl = lm.create_full (gg, h);
  EXPECT_EQ (m2s (lm, gg, h), "1/0->1/0;L1->L1;L3->L3;L10D17 (10/17)->L10D17 (10/17);L10D18 (10/18)->L10D18 (10/18)");
  EXPECT_EQ (nl2s (nl, gg), "L3;L10D18 (10/18)");

  db::Layout hh = h;
  lm.clear ();
  nl = lm.create_full (hh, g);
  EXPECT_EQ (m2s (lm, hh, g), "1/0->1/0;2/0->2/0;L1->L1;L2->L2;L10D17 (10/17)->L10D17 (10/17);L11D17 (11/17)->L11D17 (11/17)");
  EXPECT_EQ (nl2s (nl, hh), "2/0;L2;L11D17 (11/17)");

}

