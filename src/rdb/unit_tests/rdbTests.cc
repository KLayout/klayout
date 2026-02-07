
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

#include "rdb.h"
#include "tlUnitTest.h"
#include "dbBox.h"
#include "dbEdge.h"
#include "tlXMLParser.h"

TEST(1) 
{
  rdb::Database db;

  db.set_filename ("filename");
  db.set_name ("name");
  db.set_generator ("generator");
  db.set_description ("descriptions");

  EXPECT_EQ (db.filename (), "filename");
  EXPECT_EQ (db.name(), "name");
  EXPECT_EQ (db.generator(), "generator");
  EXPECT_EQ (db.description(), "descriptions");
}

TEST(2) 
{
  rdb::Database db;

  rdb::Category *cath = db.create_category ("cath_name");
  rdb::Category *cath2 = db.create_category ("cath2");

  EXPECT_EQ (db.category_by_id (1) == cath, true);
  EXPECT_EQ (db.category_by_id (0) == 0, true);
  EXPECT_EQ (db.category_by_name ("x") == 0, true);
  EXPECT_EQ (db.category_by_name ("cath_name") == cath, true);

  rdb::Cell *c1 = db.create_cell ("c1");
  rdb::Cell *c2 = db.create_cell ("c2");

  rdb::Database::const_cell_iterator c = db.cells ().begin ();
  EXPECT_EQ (c == db.cells ().end (), false);
  EXPECT_EQ (c->id (), c1->id ());
  EXPECT_EQ (c->name (), c1->name ());
  ++c;
  EXPECT_EQ (c == db.cells ().end (), false);
  EXPECT_EQ (c->id (), c2->id ());
  EXPECT_EQ (c->name (), c2->name ());
  ++c;
  EXPECT_EQ (c == db.cells ().end (), true);

  EXPECT_EQ (db.cell_by_qname ("c1") == c1, true);
  EXPECT_EQ (db.cell_by_qname ("c2") == c2, true);
  EXPECT_EQ (db.cell_by_qname ("cx") == 0, true);
  EXPECT_EQ (db.cell_by_id (c1->id ()) == c1, true);
  EXPECT_EQ (db.cell_by_id (c2->id ()) == c2, true);
  EXPECT_EQ (db.cell_by_id (0) == 0, true);
  
  db.create_item (c1->id (), cath->id ());
  db.create_item (c2->id (), cath2->id ());
  db.create_item (c1->id (), cath2->id ());

  std::pair <rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> be;

  be = db.items_by_cell (c2->id ());
  EXPECT_EQ (be.first != be.second, true);
  EXPECT_EQ ((*be.first)->cell_id (), c2->id ());
  EXPECT_EQ ((*be.first)->category_id (), cath2->id ());
  ++be.first;
  EXPECT_EQ (be.first == be.second, true);

  be = db.items_by_cell (c1->id ());
  EXPECT_EQ (be.first != be.second, true);
  EXPECT_EQ ((*be.first)->cell_id (), c1->id ());
  EXPECT_EQ ((*be.first)->category_id (), cath->id ());
  ++be.first;
  EXPECT_EQ ((*be.first)->cell_id (), c1->id ());
  EXPECT_EQ ((*be.first)->category_id (), cath2->id ());
  ++be.first;
  EXPECT_EQ (be.first == be.second, true);

  be = db.items_by_category (cath->id ());
  EXPECT_EQ (be.first != be.second, true);
  EXPECT_EQ ((*be.first)->cell_id (), c1->id ());
  EXPECT_EQ ((*be.first)->category_id (), cath->id ());
  ++be.first;
  EXPECT_EQ (be.first == be.second, true);

  be = db.items_by_cell_and_category (c1->id (), cath2->id ());
  EXPECT_EQ (be.first != be.second, true);
  EXPECT_EQ ((*be.first)->cell_id (), c1->id ());
  EXPECT_EQ ((*be.first)->category_id (), cath2->id ());
  ++be.first;
  EXPECT_EQ (be.first == be.second, true);


}

TEST(3) 
{
  rdb::Database db;

  rdb::Category *cath = db.create_category ("cath_name");
  rdb::Category *cath2 = db.create_category ("cath2");

  rdb::Cell *c1 = db.create_cell ("c1");
  rdb::Cell *c2 = db.create_cell ("c2");

  rdb::Item *i1 = db.create_item (c1->id (), cath->id ());
  rdb::Item *i2 = db.create_item (c2->id (), cath2->id ());
  rdb::Item *i3 = db.create_item (c1->id (), cath2->id ());

  EXPECT_EQ (cath2->num_items (), size_t (2));
  EXPECT_EQ (cath->num_items (), size_t (1));
  EXPECT_EQ (c1->num_items (), size_t (2));
  EXPECT_EQ (c2->num_items (), size_t (1));

  db.set_item_visited (i1, true);

  EXPECT_EQ (cath2->num_items_visited (), size_t (0));
  EXPECT_EQ (cath->num_items_visited (), size_t (1));
  EXPECT_EQ (c1->num_items_visited (), size_t (1));
  EXPECT_EQ (c2->num_items_visited (), size_t (0));
  EXPECT_EQ (db.num_items_visited (), size_t (1));

  db.set_item_visited (i2, true);

  EXPECT_EQ (cath2->num_items_visited (), size_t (1));
  EXPECT_EQ (cath->num_items_visited (), size_t (1));
  EXPECT_EQ (c1->num_items_visited (), size_t (1));
  EXPECT_EQ (c2->num_items_visited (), size_t (1));
  EXPECT_EQ (db.num_items_visited (), size_t (2));

  db.set_item_visited (i3, true);

  EXPECT_EQ (cath2->num_items_visited (), size_t (2));
  EXPECT_EQ (cath->num_items_visited (), size_t (1));
  EXPECT_EQ (c1->num_items_visited (), size_t (2));
  EXPECT_EQ (c2->num_items_visited (), size_t (1));
  EXPECT_EQ (db.num_items_visited (), size_t (3));

  db.set_item_visited (i1, false);

  EXPECT_EQ (cath2->num_items_visited (), size_t (2));
  EXPECT_EQ (cath->num_items_visited (), size_t (0));
  EXPECT_EQ (c1->num_items_visited (), size_t (1));
  EXPECT_EQ (c2->num_items_visited (), size_t (1));
  EXPECT_EQ (db.num_items_visited (), size_t (2));
}

TEST(4) 
{
  rdb::Database db;

  {
    EXPECT_EQ (db.tags ().has_tag ("aber"), false);
    const rdb::Tag &tag = db.tags ().tag ("aber");
    EXPECT_EQ (tag.name (), "aber");
    db.set_tag_description (tag.id (), "desc");
    EXPECT_EQ (db.tags ().has_tag ("aber"), true);
  }

  {
    const rdb::Tag &tag2 = db.tags ().tag ("aber");
    EXPECT_EQ (tag2.name (), "aber");
    EXPECT_EQ (tag2.description (), "desc");
  }
  
  {
    EXPECT_EQ (db.tags ().has_tag ("nix"), false);
    const rdb::Tag &tag = db.tags ().tag ("nix");
    EXPECT_EQ (tag.name (), "nix");
    EXPECT_EQ (db.tags ().has_tag ("nix"), true);
  }

  rdb::Category *cath = db.create_category ("cath_name");
  rdb::Cell *c1 = db.create_cell ("c1");
  rdb::Item *i1 = db.create_item (c1->id (), cath->id ());

  EXPECT_EQ (i1->has_tag (db.tags ().tag ("ich").id ()), false);
  i1->add_tag (db.tags ().tag ("ich").id ());
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("ich").id ()), true);
  i1->remove_tag (db.tags ().tag ("ich").id ());
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("ich").id ()), false);
  i1->add_tag (db.tags ().tag ("aber").id ());
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("aber").id ()), true);
  i1->add_tag (db.tags ().tag ("nix").id ());
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("nix").id ()), true);
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("aber").id ()), true);
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("ich").id ()), false);
  i1->remove_tags ();
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("nix").id ()), false);
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("aber").id ()), false);
  EXPECT_EQ (i1->has_tag (db.tags ().tag ("ich").id ()), false);
}

TEST(5) 
{
  if (! tl::XMLParser::is_available ()) {
    throw tl::CancelException ();
  }

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_5.lyrdb");

  {
    rdb::Database db;

    db.set_name ("db-name");
    db.set_description ("db-description");
    db.set_generator ("db-generator");

    rdb::Category *cath = db.create_category ("cath_name");
    cath->set_description ("<>&%!$\" \n+~?");
    rdb::Category *cath2 = db.create_category ("cath2");
    rdb::Category *cath2cc = db.create_category (cath2, "cc");
    cath2cc->set_description ("cath2.cc description");
    EXPECT_EQ (db.category_by_name ("cath2.cc") != 0, true);
    EXPECT_EQ (db.category_by_name ("cath2.cc")->id (), cath2cc->id ());

    rdb::Cell *c1 = db.create_cell ("c1");
    rdb::Cell *c2 = db.create_cell ("c2");
    c2->references ().insert (rdb::Reference (db::DCplxTrans (2.5), c1->id ()));
    c2->references ().insert (rdb::Reference (db::DCplxTrans (db::DTrans (db::DVector (17.5, -25))), c1->id ()));
    rdb::Cell *c3 = db.create_cell ("c3");
    c3->references ().insert (rdb::Reference (db::DCplxTrans (), c2->id ()));
    c3->references ().insert (rdb::Reference (db::DCplxTrans (1.5, 45, true, db::DVector (10.0, 20.0)), c1->id ()));

    rdb::Item *i1 = db.create_item (c1->id (), cath->id ());
    i1->values ().add (new rdb::Value<db::DBox> (db::DBox (1.0, -1.0, 10.0, 11.0)));
    i1->add_tag (db.tags ().tag ("tag1").id ());

    rdb::Item *i2 = db.create_item (c2->id (), cath2->id ());
    i2->values ().add (new rdb::Value<db::DEdge> (db::DEdge (db::DPoint (1.0, -1.0), db::DPoint (10.0, 11.0))));
    i2->values ().add (new rdb::Value<db::DBox> (db::DBox (10.0, -10.0, 100.0, 110.0)));
    i2->add_tag (db.tags ().tag ("tag1").id ());
    i2->add_tag (db.tags ().tag ("tag2").id ());
    db.set_item_visited (i2, true);

    rdb::Item *i3 = db.create_item (c1->id (), cath2cc->id ());
    db.set_item_visited (i3, true);

    db.save (tmp_file);
  }

  {
    rdb::Database db2;
    db2.load (tmp_file);

    EXPECT_EQ (db2.name (), "tmp_5.lyrdb");
    EXPECT_EQ (db2.description (), "db-description");
    EXPECT_EQ (db2.generator (), "db-generator");
    EXPECT_EQ (db2.filename (), tmp_file);

    EXPECT_EQ (db2.category_by_name ("cath_name") != 0, true);
    EXPECT_EQ (db2.category_by_name ("cath_name")->description (), "<>&%!$\" \n+~?");
    EXPECT_EQ (db2.category_by_name ("cath2") != 0, true);
    EXPECT_EQ (db2.category_by_name ("cath2.cc") != 0, true);
    EXPECT_EQ (db2.category_by_name ("cath2.cc")->description (), "cath2.cc description");

    EXPECT_EQ (db2.cell_by_qname ("c1") != 0, true);
    EXPECT_EQ (db2.cell_by_qname ("c2") != 0, true);
    EXPECT_EQ (db2.cell_by_qname ("c3") != 0, true);

    EXPECT_EQ (db2.cell_by_qname ("c1")->name (), "c1");
    EXPECT_EQ (db2.cell_by_qname ("c2")->name (), "c2");
    EXPECT_EQ (db2.cell_by_qname ("c3")->name (), "c3");

    rdb::References::const_iterator r, rend;
    r = db2.cell_by_qname ("c1")->references ().begin ();
    rend = db2.cell_by_qname ("c1")->references ().end ();
    EXPECT_EQ (r == rend, true);

    r = db2.cell_by_qname ("c2")->references ().begin ();
    rend = db2.cell_by_qname ("c2")->references ().end ();
    EXPECT_EQ (r == rend, false);
    EXPECT_EQ (r->trans ().to_string (), "r0 *2.5 0,0");
    EXPECT_EQ (r->parent_cell_id (), db2.cell_by_qname ("c1")->id ());
    ++r;
    EXPECT_EQ (r == rend, false);
    EXPECT_EQ (r->trans ().to_string (), "r0 *1 17.5,-25");
    EXPECT_EQ (r->parent_cell_id (), db2.cell_by_qname ("c1")->id ());
    ++r;
    EXPECT_EQ (r == rend, true);

    r = db2.cell_by_qname ("c3")->references ().begin ();
    rend = db2.cell_by_qname ("c3")->references ().end ();
    EXPECT_EQ (r == rend, false);
    EXPECT_EQ (r->trans ().to_string (), "r0 *1 0,0");
    EXPECT_EQ (r->parent_cell_id (), db2.cell_by_qname ("c2")->id ());
    ++r;
    EXPECT_EQ (r == rend, false);
    EXPECT_EQ (r->trans ().to_string (), "m22.5 *1.5 10,20");
    EXPECT_EQ (r->parent_cell_id (), db2.cell_by_qname ("c1")->id ());
    ++r;
    EXPECT_EQ (r == rend, true);

    std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> be;

    be = db2.items_by_cell_and_category (db2.cell_by_qname ("c1")->id (), db2.category_by_name ("cath_name")->id ()); 
    EXPECT_EQ (be.first != be.second, true);

    EXPECT_EQ ((*be.first)->visited (), false);
    EXPECT_EQ ((*be.first)->has_tag (db2.tags ().tag ("tag1").id ()), true);
    EXPECT_EQ ((*be.first)->has_tag (db2.tags ().tag ("tag2").id ()), false);

    rdb::Values::const_iterator v = (*be.first)->values ().begin ();
    EXPECT_EQ (v != (*be.first)->values ().end (), true);
    EXPECT_EQ (v->get ()->to_string (), "box: (1,-1;10,11)");
    ++v;
    EXPECT_EQ (v != (*be.first)->values ().end (), false);

    ++be.first;
    EXPECT_EQ (be.first == be.second, true);

    be = db2.items_by_cell_and_category (db2.cell_by_qname ("c2")->id (), db2.category_by_name ("cath_name")->id ()); 
    EXPECT_EQ (be.first != be.second, false);

    be = db2.items_by_cell_and_category (db2.cell_by_qname ("c2")->id (), db2.category_by_name ("cath2")->id ()); 
    EXPECT_EQ (be.first != be.second, true);

    EXPECT_EQ ((*be.first)->visited (), true);
    EXPECT_EQ ((*be.first)->has_tag (db2.tags ().tag ("tag1").id ()), true);
    EXPECT_EQ ((*be.first)->has_tag (db2.tags ().tag ("tag2").id ()), true);

    v = (*be.first)->values ().begin ();
    EXPECT_EQ (v != (*be.first)->values ().end (), true);
    EXPECT_EQ (v->get ()->to_string (), "edge: (1,-1;10,11)");
    ++v;
    EXPECT_EQ (v != (*be.first)->values ().end (), true);
    EXPECT_EQ (v->get ()->to_string (), "box: (10,-10;100,110)");
    ++v;
    EXPECT_EQ (v != (*be.first)->values ().end (), false);

    ++be.first;
    EXPECT_EQ (be.first == be.second, true);

    be = db2.items_by_cell_and_category (db2.cell_by_qname ("c1")->id (), db2.category_by_name ("cath2.cc")->id ()); 
    EXPECT_EQ (be.first != be.second, true);
    EXPECT_EQ ((*be.first)->visited (), true);

  }
}

TEST(5a) 
{
  if (! tl::XMLParser::is_available ()) {
    throw tl::CancelException ();
  }

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_5a.lyrdb");

  {
    rdb::Database db;

    db.set_name ("db-name");
    db.set_description ("db-description");
    db.set_generator ("db-generator");

    rdb::Category *cath = db.create_category ("cath_name");
    cath->set_description ("<>&%!$\" \n+~?");
    rdb::Category *cath2 = db.create_category ("cath2");
    rdb::Category *cath2cc = db.create_category (cath2, "cc");
    cath2cc->set_description ("cath2.cc description");
    EXPECT_EQ (db.category_by_name ("cath2.cc") != 0, true);
    EXPECT_EQ (db.category_by_name ("cath2.cc")->id (), cath2cc->id ());

    rdb::Cell *c1 = db.create_cell ("c1");
    rdb::Cell *c2 = db.create_cell ("c1"); // variant!
    c2->references ().insert (rdb::Reference (db::DCplxTrans (2.5), c1->id ()));
    c2->references ().insert (rdb::Reference (db::DCplxTrans (db::DTrans (db::DVector (17.5, -25))), c1->id ()));
    rdb::Cell *c3 = db.create_cell ("c3");
    c3->references ().insert (rdb::Reference (db::DCplxTrans (), c2->id ()));
    c3->references ().insert (rdb::Reference (db::DCplxTrans (1.5, 45, true, db::DVector (10.0, 20.0)), c1->id ()));

    rdb::Item *i1 = db.create_item (c1->id (), cath->id ());
    i1->values ().add (new rdb::Value<db::DBox> (db::DBox (1.0, -1.0, 10.0, 11.0)));
    i1->add_tag (db.tags ().tag ("tag1").id ());

    rdb::Item *i2 = db.create_item (c2->id (), cath2->id ());
    i2->values ().add (new rdb::Value<db::DEdge> (db::DEdge (db::DPoint (1.0, -1.0), db::DPoint (10.0, 11.0))));
    i2->values ().add (new rdb::Value<db::DBox> (db::DBox (10.0, -10.0, 100.0, 110.0)));
    i2->add_tag (db.tags ().tag ("tag1").id ());
    i2->add_tag (db.tags ().tag ("tag2").id ());
    db.set_item_visited (i2, true);

    rdb::Item *i3 = db.create_item (c1->id (), cath2cc->id ());
    db.set_item_visited (i3, true);

    db.save (tmp_file);
  }

  {
    rdb::Database db2;
    db2.load (tmp_file);

    EXPECT_EQ (db2.name (), "tmp_5a.lyrdb");
    EXPECT_EQ (db2.description (), "db-description");
    EXPECT_EQ (db2.generator (), "db-generator");
    EXPECT_EQ (db2.filename (), tmp_file);

    EXPECT_EQ (db2.category_by_name ("cath_name") != 0, true);
    EXPECT_EQ (db2.category_by_name ("cath_name")->description (), "<>&%!$\" \n+~?");
    EXPECT_EQ (db2.category_by_name ("cath2") != 0, true);
    EXPECT_EQ (db2.category_by_name ("cath2.cc") != 0, true);
    EXPECT_EQ (db2.category_by_name ("cath2.cc")->description (), "cath2.cc description");

    EXPECT_EQ (db2.cell_by_qname ("c1:1") != 0, true);
    EXPECT_EQ (db2.cell_by_qname ("c1:2") != 0, true);

    EXPECT_EQ (db2.cell_by_qname ("c1:1")->name (), "c1");
    EXPECT_EQ (db2.cell_by_qname ("c1:2")->name (), "c1");

    rdb::References::const_iterator r, rend;
    r = db2.cell_by_qname ("c1:1")->references ().begin ();
    rend = db2.cell_by_qname ("c1:1")->references ().end ();
    EXPECT_EQ (r == rend, true);

    r = db2.cell_by_qname ("c1:2")->references ().begin ();
    rend = db2.cell_by_qname ("c1:2")->references ().end ();
    EXPECT_EQ (r == rend, false);
    EXPECT_EQ (r->trans ().to_string (), "r0 *2.5 0,0");
    EXPECT_EQ (r->parent_cell_id (), db2.cell_by_qname ("c1:1")->id ());
    ++r;
    EXPECT_EQ (r == rend, false);
    EXPECT_EQ (r->trans ().to_string (), "r0 *1 17.5,-25");
    EXPECT_EQ (r->parent_cell_id (), db2.cell_by_qname ("c1:1")->id ());
    ++r;
    EXPECT_EQ (r == rend, true);

    r = db2.cell_by_qname ("c3")->references ().begin ();
    rend = db2.cell_by_qname ("c3")->references ().end ();
    EXPECT_EQ (r == rend, false);
    EXPECT_EQ (r->trans ().to_string (), "r0 *1 0,0");
    EXPECT_EQ (r->parent_cell_id (), db2.cell_by_qname ("c1:2")->id ());
    ++r;
    EXPECT_EQ (r == rend, false);
    EXPECT_EQ (r->trans ().to_string (), "m22.5 *1.5 10,20");
    EXPECT_EQ (r->parent_cell_id (), db2.cell_by_qname ("c1:1")->id ());
    ++r;
    EXPECT_EQ (r == rend, true);

    std::pair<rdb::Database::const_item_ref_iterator, rdb::Database::const_item_ref_iterator> be;

    be = db2.items_by_cell_and_category (db2.cell_by_qname ("c1:1")->id (), db2.category_by_name ("cath_name")->id ()); 
    EXPECT_EQ (be.first != be.second, true);

    EXPECT_EQ ((*be.first)->visited (), false);
    EXPECT_EQ ((*be.first)->has_tag (db2.tags ().tag ("tag1").id ()), true);
    EXPECT_EQ ((*be.first)->has_tag (db2.tags ().tag ("tag2").id ()), false);

    rdb::Values::const_iterator v = (*be.first)->values ().begin ();
    EXPECT_EQ (v != (*be.first)->values ().end (), true);
    EXPECT_EQ (v->get ()->to_string (), "box: (1,-1;10,11)");
    ++v;
    EXPECT_EQ (v != (*be.first)->values ().end (), false);

    ++be.first;
    EXPECT_EQ (be.first == be.second, true);

    be = db2.items_by_cell_and_category (db2.cell_by_qname ("c1:2")->id (), db2.category_by_name ("cath_name")->id ()); 
    EXPECT_EQ (be.first != be.second, false);

    be = db2.items_by_cell_and_category (db2.cell_by_qname ("c1:2")->id (), db2.category_by_name ("cath2")->id ()); 
    EXPECT_EQ (be.first != be.second, true);

    EXPECT_EQ ((*be.first)->visited (), true);
    EXPECT_EQ ((*be.first)->has_tag (db2.tags ().tag ("tag1").id ()), true);
    EXPECT_EQ ((*be.first)->has_tag (db2.tags ().tag ("tag2").id ()), true);

    v = (*be.first)->values ().begin ();
    EXPECT_EQ (v != (*be.first)->values ().end (), true);
    EXPECT_EQ (v->get ()->to_string (), "edge: (1,-1;10,11)");
    ++v;
    EXPECT_EQ (v != (*be.first)->values ().end (), true);
    EXPECT_EQ (v->get ()->to_string (), "box: (10,-10;100,110)");
    ++v;
    EXPECT_EQ (v != (*be.first)->values ().end (), false);

    ++be.first;
    EXPECT_EQ (be.first == be.second, true);

    be = db2.items_by_cell_and_category (db2.cell_by_qname ("c1:1")->id (), db2.category_by_name ("cath2.cc")->id ()); 
    EXPECT_EQ (be.first != be.second, true);
    EXPECT_EQ ((*be.first)->visited (), true);

  }
}

TEST(6) 
{
  rdb::Database db;

  rdb::Cell *c1 = db.create_cell ("c1");
  EXPECT_EQ (c1->qname (), "c1");

  EXPECT_EQ (db.variants ("c1").size (), size_t (0));

  rdb::Cell *c1a = db.create_cell ("c1");
  EXPECT_EQ (c1a->qname (), "c1:2");
  EXPECT_EQ (c1->qname (), "c1:1")

  EXPECT_EQ (db.variants ("c1").size (), size_t (2));
  EXPECT_EQ (db.variants ("c1")[0], c1->id ());
  EXPECT_EQ (db.variants ("c1")[1], c1a->id ());

  rdb::Cell *c1b = db.create_cell ("c1", "var", std::string ());
  EXPECT_EQ (c1b->qname (), "c1:var")
  EXPECT_EQ (db.variants ("c1").size (), size_t (3));

  rdb::Cell *c2 = db.create_cell ("c2", "1027", std::string ());
  EXPECT_EQ (c2->qname (), "c2:1027");
  EXPECT_EQ (db.variants ("c2").size (), size_t (1));

  rdb::Cell *c2a = db.create_cell ("c2");
  EXPECT_EQ (c2a->qname (), "c2:1");
  EXPECT_EQ (c2->qname (), "c2:1027")
  EXPECT_EQ (db.variants ("c2").size (), size_t (2));

  rdb::Cell *c2b = db.create_cell ("c2", "var", "c2$1");
  EXPECT_EQ (c2b->qname (), "c2:var")
  EXPECT_EQ (c2b->layout_name (), "c2$1")

  rdb::Cell *c2c = db.create_cell ("c2");
  EXPECT_EQ (c2c->qname (), "c2:2");

  rdb::Cell *c2d = db.create_cell ("c2");
  EXPECT_EQ (c2d->qname (), "c2:3");

  rdb::Cell *c2e = db.create_cell ("c2");
  EXPECT_EQ (c2e->qname (), "c2:4");

  EXPECT_EQ (db.variants ("c2").size (), size_t (6));
  EXPECT_EQ (db.variants ("c2")[0], c2->id ());
  EXPECT_EQ (db.variants ("c2")[5], c2e->id ());
}

TEST(7)
{
  rdb::Database db;
  rdb::Category *cath = db.create_category ("cath_name");
  rdb::Cell *c1 = db.create_cell ("c1");
  rdb::Item *i1 = db.create_item (c1->id (), cath->id ());

#if defined(HAVE_QT)

  {
    QImage img (16, 26, QImage::Format_RGB32);
    for (int i = 0; i < img.height (); ++i) {
      for (int j = 0; j < img.height (); ++j) {
        img.scanLine (j) [i] = (i << 16) + j;
      }
    }
    i1->set_image (img);

    QImage img2 = i1->image ();

    EXPECT_EQ (img.width (), img2.width ());
    EXPECT_EQ (img.height (), img2.height ());
    EXPECT_EQ (tl::PixelBuffer::from_image (img) == tl::PixelBuffer::from_image (img2), true);
  }

#endif

#if defined(HAVE_PNG)

  {
    tl::PixelBuffer img (16, 26);
    for (unsigned int i = 0; i < img.width (); ++i) {
      for (unsigned int j = 0; j < img.height (); ++j) {
        img.scan_line (j) [i] = (i << 16) + j;
      }
    }
    i1->set_image (img);

    tl::PixelBuffer img2 = i1->image_pixels ();

    EXPECT_EQ (img.width (), img2.width ());
    EXPECT_EQ (img.height (), img2.height ());
    EXPECT_EQ (img == img2, true);
  }

#endif
}


TEST(8_ApplyBasicEmptyValue)
{
  rdb::Database db1;
  rdb::Category *cat1 = db1.create_category ("cat_name");
  rdb::Cell *c1 = db1.create_cell ("cell");
  rdb::Item *i1 = db1.create_item (c1->id (), cat1->id ());

  rdb::Database db2;
  db2.create_category ("dummy_cat");
  rdb::Category *cat2 = db2.create_category ("cat_name");
  db2.create_cell ("dummy_cell");
  rdb::Cell *c2 = db2.create_cell ("cell");
  rdb::Item *i2 = db2.create_item (c2->id (), cat2->id ());

  rdb::id_type tag2 = db2.tags ().tag ("tag2").id ();
  i2->add_tag (tag2);

  EXPECT_EQ (i2->tag_str (), "tag2");
  EXPECT_EQ (i1->tag_str (), "");

  //  empty value apply
  db1.apply (db2);

  EXPECT_EQ (i1->tag_str (), "tag2");
}

TEST(9_ApplyBasicSomeValue)
{
  rdb::Database db1;
  rdb::Category *cat1 = db1.create_category ("cat_name");
  rdb::Cell *c1 = db1.create_cell ("cell");
  rdb::Item *i1 = db1.create_item (c1->id (), cat1->id ());
  i1->add_value (std::string ("abc"));

  rdb::Database db2;
  db2.create_category ("dummy_cat");
  rdb::Category *cat2 = db2.create_category ("cat_name");
  db2.create_cell ("dummy_cell");
  rdb::Cell *c2 = db2.create_cell ("cell");
  rdb::Item *i2 = db2.create_item (c2->id (), cat2->id ());

  db2.tags ().tag ("dummy_tag");
  rdb::id_type tag2 = db2.tags ().tag ("tag2").id ();
  i2->add_tag (tag2);

  EXPECT_EQ (i2->tag_str (), "tag2");
  EXPECT_EQ (i1->tag_str (), "");

  //  empty value apply
  db1.apply (db2);

  //  not applied (different value)
  EXPECT_EQ (i1->tag_str (), "");

  //  incorrect value
  i2->add_value (17);

  db1.apply (db2);

  //  still not applied
  EXPECT_EQ (i1->tag_str (), "");

  //  correct value
  i2->values ().clear ();
  i2->add_value (std::string ("abc"));

  db1.apply (db2);

  //  now, the tag is applied
  EXPECT_EQ (i1->tag_str (), "tag2");

  //  too many values
  i1->remove_tags ();
  i2->add_value (17);

  db1.apply (db2);

  // not applied
  EXPECT_EQ (i1->tag_str (), "");
}

TEST(10_ApplyTaggedValue)
{
  rdb::Database db1;
  rdb::Category *cat1 = db1.create_category ("cat_name");
  rdb::Cell *c1 = db1.create_cell ("cell");
  rdb::Item *i1 = db1.create_item (c1->id (), cat1->id ());
  rdb::id_type vtag11 = db1.tags ().tag ("vtag1").id ();
  rdb::id_type vtag12 = db1.tags ().tag ("vtag2").id ();
  i1->add_value (std::string ("abc"));

  rdb::Database db2;
  db2.create_category ("dummy_cat");
  rdb::Category *cat2 = db2.create_category ("cat_name");
  db2.create_cell ("dummy_cell");
  rdb::Cell *c2 = db2.create_cell ("cell");
  rdb::Item *i2 = db2.create_item (c2->id (), cat2->id ());
  db2.tags ().tag ("dummy_tag");

  rdb::id_type tag2 = db2.tags ().tag ("tag2").id ();
  rdb::id_type vtag21 = db2.tags ().tag ("vtag1").id ();
  i2->add_tag (tag2);
  i2->add_value (std::string ("abc"), vtag21);

  //  empty tag vs. vtag1
  db1.apply (db2);

  //  not applied (empty tag vs. tagged)
  EXPECT_EQ (i1->tag_str (), "");

  //  vtag2 vs. vtag1
  i1->values ().clear ();
  i1->add_value (std::string ("abc"), vtag12);

  db1.apply (db2);

  //  not applied (different tags)
  EXPECT_EQ (i1->tag_str (), "");

  //  vtag1 vs. vtag1
  i1->values ().clear ();
  i1->add_value (std::string ("abc"), vtag11);

  db1.apply (db2);

  //  this time it is applied (same tag)
  EXPECT_EQ (i1->tag_str (), "tag2");
}

TEST(11_ApplyWrongCat)
{
  rdb::Database db1;
  rdb::Category *cat1 = db1.create_category ("cat_name");
  rdb::Cell *c1 = db1.create_cell ("cell");
  rdb::Item *i1 = db1.create_item (c1->id (), cat1->id ());

  rdb::Database db2;
  db2.create_category ("dummy_cat");
  rdb::Category *cat2 = db2.create_category ("xcat_name");
  db2.create_cell ("dummy_cell");
  rdb::Cell *c2 = db2.create_cell ("cell");
  rdb::Item *i2 = db2.create_item (c2->id (), cat2->id ());

  rdb::id_type tag2 = db2.tags ().tag ("tag2").id ();
  i2->add_tag (tag2);

  EXPECT_EQ (i2->tag_str (), "tag2");
  EXPECT_EQ (i1->tag_str (), "");

  //  empty value apply
  db1.apply (db2);

  EXPECT_EQ (i1->tag_str (), "");
}

TEST(12_ApplyWrongCell)
{
  rdb::Database db1;
  rdb::Category *cat1 = db1.create_category ("cat_name");
  rdb::Cell *c1 = db1.create_cell ("cell");
  rdb::Item *i1 = db1.create_item (c1->id (), cat1->id ());

  rdb::Database db2;
  db2.create_category ("dummy_cat");
  rdb::Category *cat2 = db2.create_category ("cat_name");
  db2.create_cell ("dummy_cell");
  rdb::Cell *c2 = db2.create_cell ("xcell");
  rdb::Item *i2 = db2.create_item (c2->id (), cat2->id ());

  rdb::id_type tag2 = db2.tags ().tag ("tag2").id ();
  i2->add_tag (tag2);

  EXPECT_EQ (i2->tag_str (), "tag2");
  EXPECT_EQ (i1->tag_str (), "");

  //  empty value apply
  db1.apply (db2);

  EXPECT_EQ (i1->tag_str (), "");
}

TEST(13_ApplyIgnoreUnknownTag)
{
  rdb::Database db1;
  rdb::Category *cat1 = db1.create_category ("cat_name");
  rdb::Cell *c1 = db1.create_cell ("cell");
  rdb::Item *i1 = db1.create_item (c1->id (), cat1->id ());
  rdb::id_type vtag11 = db1.tags ().tag ("vtag1").id ();
  i1->add_value (std::string ("abc"), vtag11);

  rdb::Database db2;
  db2.create_category ("dummy_cat");
  rdb::Category *cat2 = db2.create_category ("cat_name");
  db2.create_cell ("dummy_cell");
  rdb::Cell *c2 = db2.create_cell ("cell");
  rdb::Item *i2 = db2.create_item (c2->id (), cat2->id ());
  db2.tags ().tag ("dummy_tag");

  rdb::id_type tag2 = db2.tags ().tag ("tag2").id ();
  rdb::id_type vtag21 = db2.tags ().tag ("vtag1").id ();
  rdb::id_type vtag22 = db2.tags ().tag ("vtag2").id ();
  i2->add_tag (tag2);

  //  same tags, different values
  i2->add_value (std::string ("xyz"), vtag21);

  db1.apply (db2);

  //  not applied
  EXPECT_EQ (i1->tag_str (), "");

  //  different tags without mapping
  i2->values ().clear ();
  i2->add_value (std::string ("xyz"), vtag22);

  //  values with incompatible tags are ignored -> tag2 is applied
  db1.apply (db2);

  EXPECT_EQ (i1->tag_str (), "tag2");
}

