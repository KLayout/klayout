
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include <QCoreApplication>

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

TEST(1) 
{
  g_na = g_nb = 0;

  QCoreApplication::instance ()->processEvents ();

  X *x = new X ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 0);
  EXPECT_EQ (g_nb, 0);

  QCoreApplication::instance ()->processEvents ();

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

  QCoreApplication::instance ()->processEvents ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 0);
  EXPECT_EQ (g_nb, 0);

  tl::DeferredMethodScheduler::enable (true);

  x->db ();
  x->db ();

  QCoreApplication::instance ()->processEvents ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 0);
  EXPECT_EQ (g_nb, 0);

  tl::DeferredMethodScheduler::enable (true);

  QCoreApplication::instance ()->processEvents ();

  EXPECT_EQ (x->na, 1);
  EXPECT_EQ (x->nb, 2);
  EXPECT_EQ (g_na, 1);
  EXPECT_EQ (g_nb, 2);
  
  QCoreApplication::instance ()->processEvents ();

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
  
  QCoreApplication::instance ()->processEvents ();

  EXPECT_EQ (x->na, 2);
  EXPECT_EQ (x->nb, 4);
  EXPECT_EQ (g_na, 2);
  EXPECT_EQ (g_nb, 4);

  QCoreApplication::instance ()->processEvents ();

  EXPECT_EQ (x->na, 2);
  EXPECT_EQ (x->nb, 4);
  EXPECT_EQ (g_na, 2);
  EXPECT_EQ (g_nb, 4);
  
  delete x;

  QCoreApplication::instance ()->processEvents ();

  x = new X ();

  EXPECT_EQ (x->na, 0);
  EXPECT_EQ (x->nb, 0);
  EXPECT_EQ (g_na, 2);
  EXPECT_EQ (g_nb, 4);

  QCoreApplication::instance ()->processEvents ();

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

  QCoreApplication::instance ()->processEvents ();

  EXPECT_EQ (x->na, 1);
  EXPECT_EQ (x->nb, 2);
  EXPECT_EQ (g_na, 3);
  EXPECT_EQ (g_nb, 6);
  
  QCoreApplication::instance ()->processEvents ();

  EXPECT_EQ (x->na, 1);
  EXPECT_EQ (x->nb, 2);
  EXPECT_EQ (g_na, 3);
  EXPECT_EQ (g_nb, 6);
  
  x->da ();
  x->da ();
  x->db ();
  x->db ();

  delete x;

  QCoreApplication::instance ()->processEvents ();

  EXPECT_EQ (g_na, 3);
  EXPECT_EQ (g_nb, 6);

}

