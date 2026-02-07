
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


#include "dbObject.h"
#include "tlUnitTest.h"

namespace {

struct AO : public db::Op
{
  AO (int dd) : d (dd) { ++ao_inst; }
  ~AO () { --ao_inst; }
  int d;
  static int inst_count () { return ao_inst; }
  static int ao_inst;
};

int AO::ao_inst = 0;

struct A : public db::Object 
{
  A (db::Manager *m) : db::Object (m), x (0) { }

  void add (int d)
  {
    if (transacting ()) {
      manager ()->queue (this, new AO (d));
    }
    x += d;
  }
  
  void undo (db::Op *op)
  {
    AO *aop = dynamic_cast<AO *> (op);
    x -= aop->d;
  }

  void redo (db::Op *op)
  {
    AO *aop = dynamic_cast<AO *> (op);
    tl_assert (aop != 0);
    x += aop->d;
  }

  int x;
};

}

TEST(1) 
{
  db::Manager *man = new db::Manager (true);
  {
    EXPECT_EQ (man->available_undo ().first, false);
    EXPECT_EQ (man->available_redo ().first, false);

    A a (man);
    man->transaction ("add 1");
    a.add (1);
    man->commit ();

    EXPECT_EQ (a.x, 1);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_undo ().second, "add 1");

    man->undo ();
    EXPECT_EQ (a.x, 0);
    EXPECT_EQ (man->available_undo ().first, false);

    EXPECT_EQ (man->available_redo ().first, true);
    EXPECT_EQ (man->available_redo ().second, "add 1");
    man->redo ();
    EXPECT_EQ (man->available_redo ().first, false);
    EXPECT_EQ (a.x, 1);

    man->undo ();
    EXPECT_EQ (a.x, 0);
    EXPECT_EQ (man->available_undo ().first, false);

    man->transaction ("add 1,2");
    a.add (1);
    a.add (2);
    man->commit ();
    EXPECT_EQ (a.x, 3);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_redo ().first, false);

    man->transaction ("add 3");
    a.add (3);
    man->commit ();
    EXPECT_EQ (a.x, 6);

    man->undo ();
    EXPECT_EQ (a.x, 3);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_redo ().first, true);
    
    man->undo ();
    EXPECT_EQ (a.x, 0);
    EXPECT_EQ (man->available_undo ().first, false);
    EXPECT_EQ (man->available_redo ().first, true);
  }

  delete man;
  EXPECT_EQ (AO::inst_count (), 0);
}


//  The same than above but with implicitly declared operations through
//  redo.

namespace
{

struct BO : public db::Op
{
  BO (int dd) : db::Op (false), d (dd) { ++bo_inst; }
  ~BO () { --bo_inst; }
  int d;
  static int inst_count () { return bo_inst; }
  static int bo_inst;
};

int BO::bo_inst = 0;

struct B : public db::Object 
{
  B (db::Manager *m) : db::Object (m), x (0) { }

  void add (int d)
  {
    if (transacting ()) {
      manager ()->queue (this, new BO (d));
    } else {
       x += d;
    }
  }
  
  void undo (db::Op *op)
  {
    BO *bop = dynamic_cast<BO *> (op);
    x -= bop->d;
  }

  void redo (db::Op *op)
  {
    BO *bop = dynamic_cast<BO *> (op);
    tl_assert (bop != 0);
    x += bop->d;
  }

  int x;
};

}

TEST(2) 
{
  db::Manager *man = new db::Manager (true);
  {
    EXPECT_EQ (man->available_undo ().first, false);
    EXPECT_EQ (man->available_redo ().first, false);

    B b (man);
    man->transaction ("add 1");
    b.add (1);
    man->commit ();

    EXPECT_EQ (b.x, 1);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_undo ().second, "add 1");

    man->undo ();
    EXPECT_EQ (b.x, 0);
    EXPECT_EQ (man->available_undo ().first, false);

    EXPECT_EQ (man->available_redo ().first, true);
    EXPECT_EQ (man->available_redo ().second, "add 1");
    man->redo ();
    EXPECT_EQ (man->available_redo ().first, false);
    EXPECT_EQ (b.x, 1);

    man->undo ();
    EXPECT_EQ (b.x, 0);
    EXPECT_EQ (man->available_undo ().first, false);

    man->transaction ("add 1,2");
    b.add (1);
    b.add (2);
    man->commit ();
    EXPECT_EQ (b.x, 3);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_redo ().first, false);

    man->transaction ("add 3");
    b.add (3);
    man->commit ();
    EXPECT_EQ (b.x, 6);

    man->undo ();
    EXPECT_EQ (b.x, 3);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_redo ().first, true);
    
    man->undo ();
    EXPECT_EQ (b.x, 0);
    EXPECT_EQ (man->available_undo ().first, false);
    EXPECT_EQ (man->available_redo ().first, true);
  }

  delete man;
  EXPECT_EQ (BO::inst_count (), 0);
}

TEST(3) 
{
  db::Manager *man = new db::Manager (true);
  {
    EXPECT_EQ (man->available_undo ().first, false);
    EXPECT_EQ (man->available_redo ().first, false);

    B b (man);
    man->transaction ("add 1");
    b.add (1);
    man->commit ();

    EXPECT_EQ (b.x, 1);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_undo ().second, "add 1");

    man->transaction ("add 1,2");
    b.add (1);
    b.add (2);
    man->cancel ();
    EXPECT_EQ (b.x, 1);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_redo ().first, false);

    man->undo ();
    EXPECT_EQ (b.x, 0);
    EXPECT_EQ (man->available_undo ().first, false);
    EXPECT_EQ (man->available_redo ().first, true);
  }

  delete man;
  EXPECT_EQ (BO::inst_count (), 0);
}

TEST(4)
{
  db::Manager *man = new db::Manager (true);
  {
    EXPECT_EQ (man->available_undo ().first, false);
    EXPECT_EQ (man->available_redo ().first, false);

    B b (man);
    {
      db::Transaction t (man, "add 1");
      b.add (1);
    }

    EXPECT_EQ (b.x, 1);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_undo ().second, "add 1");

    {
      db::Transaction t (man, "add 1,2");
      b.add (1);
      EXPECT_EQ (b.x, 2);
      EXPECT_EQ (man->transacting (), true);
      t.close ();
      EXPECT_EQ (man->transacting (), false);
      b.add (1);  //  after close -> not undone!
      EXPECT_EQ (b.x, 3);
      t.open ();
      EXPECT_EQ (man->transacting (), true);
      b.add (2);
      EXPECT_EQ (b.x, 5);
      t.cancel ();
    }

    EXPECT_EQ (b.x, 2);
    EXPECT_EQ (man->available_undo ().first, true);
    EXPECT_EQ (man->available_redo ().first, false);

    man->undo ();
    EXPECT_EQ (b.x, 1);
    EXPECT_EQ (man->available_undo ().first, false);
    EXPECT_EQ (man->available_redo ().first, true);
  }

  delete man;
  EXPECT_EQ (BO::inst_count (), 0);
}

