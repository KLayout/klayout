
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

#include "tlDeferredExecution.h"
#include "tlUnitTest.h"

#if defined(HAVE_QT)
#  include <QCoreApplication>
#endif

int g_na = 0;
int g_nb = 0;

class X 
{
public:
  X () : da (this, &X::a), db (this, &X::b, false), na (0), nb (0) { }

  void a () { ++na; ++g_na; }
  void b () { ++nb; ++g_nb; }
  
  tl::DeferredMethod<X> da, db;
  int na, nb;
};

void trigger_execution ()
{
#if defined(HAVE_QT)
  if (QCoreApplication::instance ()) {
    QCoreApplication::instance ()->processEvents ();
    return;
  }
#endif

  //  explicit execute if timer does not do it
  tl::DeferredMethodScheduler::execute ();
}

TEST(1) 
{
  g_na = g_nb = 0;

  trigger_execution ();

  X *x = new X ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 0);
  EXPECT_EQ (g_nb, 0);

  trigger_execution ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 0);
  EXPECT_EQ (g_nb, 0);

  x->da ();
  x->da ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 0);
  EXPECT_EQ (g_nb, 0);

  tl::DeferredMethodScheduler::enable (false);
  tl::DeferredMethodScheduler::enable (false);

  trigger_execution ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 0);
  EXPECT_EQ (g_nb, 0);

  tl::DeferredMethodScheduler::enable (true);

  x->db ();
  x->db ();

  trigger_execution ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 0);
  EXPECT_EQ (g_nb, 0);

  tl::DeferredMethodScheduler::enable (true);

  trigger_execution ();

  EXPECT_EQ (x->na, 1);
  EXPECT_EQ (x->nb, 2);
  EXPECT_EQ (g_na, 1);
  EXPECT_EQ (g_nb, 2);
  
  trigger_execution ();

  EXPECT_EQ (x->na, 1);
  EXPECT_EQ (x->nb, 2);
  EXPECT_EQ (g_na, 1);
  EXPECT_EQ (g_nb, 2);
  
  x->da ();
  x->da ();
  x->db ();
  x->db ();

  EXPECT_EQ (x->na, 1);
  EXPECT_EQ (x->nb, 2);
  EXPECT_EQ (g_na, 1);
  EXPECT_EQ (g_nb, 2);
  
  trigger_execution ();

  EXPECT_EQ (x->na, 2);
  EXPECT_EQ (x->nb, 4);
  EXPECT_EQ (g_na, 2);
  EXPECT_EQ (g_nb, 4);

  trigger_execution ();

  EXPECT_EQ (x->na, 2);
  EXPECT_EQ (x->nb, 4);
  EXPECT_EQ (g_na, 2);
  EXPECT_EQ (g_nb, 4);
  
  delete x;

  trigger_execution ();

  x = new X ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 2);
  EXPECT_EQ (g_nb, 4);

  trigger_execution ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 2);
  EXPECT_EQ (g_nb, 4);

  x->da ();
  x->db ();
  x->da ();
  x->db ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 2);
  EXPECT_EQ (g_nb, 4);

  trigger_execution ();

  EXPECT_EQ (x->na, 1);
  EXPECT_EQ (x->nb, 2);
  EXPECT_EQ (g_na, 3);
  EXPECT_EQ (g_nb, 6);
  
  trigger_execution ();

  EXPECT_EQ (x->na, 1);
  EXPECT_EQ (x->nb, 2);
  EXPECT_EQ (g_na, 3);
  EXPECT_EQ (g_nb, 6);
  
  x->da ();
  x->da ();
  x->db ();
  x->db ();

  delete x;

  trigger_execution ();

  EXPECT_EQ (g_na, 3);
  EXPECT_EQ (g_nb, 6);

}

static int y_inst = 0;

class Y
{
public:
  Y () : da (this, &Y::a), db (this, &Y::b) { ++y_inst; }
  ~Y () { --y_inst; }

  void a () { delete this; }
  void b () { tl_assert(false); }

  tl::DeferredMethod<Y> da, db;
};

TEST(2)
{
  //  execution of a deletes db which must not be executed
  y_inst = 0;
  Y *y = new Y ();
  y->da ();
  y->db ();

  trigger_execution ();

  EXPECT_EQ (y_inst, 0);
}
