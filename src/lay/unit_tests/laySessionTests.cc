
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


#include "laySession.h"
#include "layMainWindow.h"
#include "layLayoutView.h"
#include "tlUnitTest.h"
#include "dbLayoutToNetlist.h"
#include "dbLayoutVsSchematic.h"
#include "rdb.h"
#include "antObject.h"
#include "antService.h"
#include "imgObject.h"
#include "imgService.h"
#include "tlFileUtils.h"

TEST (1)
{
  lay::MainWindow *mw = lay::MainWindow::instance ();
  tl_assert (mw != 0);

  mw->close_all ();

  mw->load_layout (tl::testdata () + "/sessions/test.gds");

  lay::LayoutView *view = mw->current_view ();

  view->set_title ("xyz");

  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  tl_assert (ant_service != 0);
  if (ant_service) {
    ant::Object ruler;
    ruler.fmt ("Hello, world!");
    ant_service->insert_ruler (ruler, false /*do not observe the ruler count limit*/);
  }

  img::Service *img_service = view->get_plugin <img::Service> ();
  tl_assert (img_service != 0);
  if (img_service) {
    img::Object img;
    img.load_data (tl::testdata () + "/sessions/test.png");
    img_service->insert_image (img);
  }

  std::unique_ptr<rdb::Database> rdb (new rdb::Database ());
  rdb->load (tl::testdata () + "/sessions/test.lyrdb");
  view->add_rdb (rdb.release ());

  std::unique_ptr<db::LayoutToNetlist> l2ndb (db::LayoutToNetlist::create_from_file (tl::testdata () + "/sessions/test.l2n"));
  view->add_l2ndb (l2ndb.release ());

  std::unique_ptr<db::LayoutToNetlist> lvsdb (db::LayoutToNetlist::create_from_file (tl::testdata () + "/sessions/test.lvsdb"));
  view->add_l2ndb (lvsdb.release ());

  std::string lys_file = tmp_file ("test1.lys");
  mw->save_session (lys_file);

  mw->close_all ();

  EXPECT_EQ (mw->views (), (unsigned int) 0);
  mw->restore_session (lys_file);

  EXPECT_EQ (mw->views (), (unsigned int) 1);

  view = mw->current_view ();
  tl_assert (view != 0);

  EXPECT_EQ (view->title (), "xyz");

  ant_service = view->get_plugin <ant::Service> ();
  tl_assert (ant_service != 0);
  if (ant_service) {
    ant::AnnotationIterator a = ant_service->begin_annotations ();
    EXPECT_EQ (a.at_end (), false);
    if (! a.at_end ()) {
      EXPECT_EQ (a->fmt (), "Hello, world!");
      ++a;
      EXPECT_EQ (a.at_end (), true);
    }
  }

  img_service = view->get_plugin <img::Service> ();
  tl_assert (img_service != 0);
  if (img_service) {
    img::ImageIterator i = img_service->begin_images ();
    EXPECT_EQ (i.at_end (), false);
    if (! i.at_end ()) {
      EXPECT_EQ (i->width (), (unsigned int) 256);
      EXPECT_EQ (i->height (), (unsigned int) 256);
      ++i;
      EXPECT_EQ (i.at_end (), true);
    }
  }

  EXPECT_EQ (view->num_l2ndbs (), (unsigned int) 2);
  EXPECT_EQ (tl::filename (view->get_l2ndb (0)->filename ()), "test.l2n");
  EXPECT_EQ (tl::filename (view->get_l2ndb (1)->filename ()), "test.lvsdb");
  EXPECT_EQ (dynamic_cast <db::LayoutVsSchematic *> (view->get_l2ndb (1)) != 0, true);

  EXPECT_EQ (view->num_rdbs (), (unsigned int) 1);
  EXPECT_EQ (tl::filename (view->get_rdb (0)->filename ()), "test.lyrdb");
}

//  issue-353 (all paths relative to .lys file)
TEST (2)
{
  lay::MainWindow *mw = lay::MainWindow::instance ();
  tl_assert (mw != 0);

  mw->close_all ();

  mw->close_all ();

  EXPECT_EQ (mw->views (), (unsigned int) 0);
  mw->restore_session (tl::testdata () + "/sessions/test_with_relative_paths.lys");

  EXPECT_EQ (mw->views (), (unsigned int) 1);

  lay::LayoutView *view = mw->current_view ();
  tl_assert (view != 0);

  ant::Service *ant_service = view->get_plugin <ant::Service> ();
  tl_assert (ant_service != 0);
  if (ant_service) {
    ant::AnnotationIterator a = ant_service->begin_annotations ();
    EXPECT_EQ (a.at_end (), false);
    if (! a.at_end ()) {
      EXPECT_EQ (a->fmt (), "Hello, world!");
      ++a;
      EXPECT_EQ (a.at_end (), true);
    }
  }

  img::Service *img_service = view->get_plugin <img::Service> ();
  tl_assert (img_service != 0);
  if (img_service) {
    img::ImageIterator i = img_service->begin_images ();
    EXPECT_EQ (i.at_end (), false);
    if (! i.at_end ()) {
      EXPECT_EQ (i->width (), (unsigned int) 256);
      EXPECT_EQ (i->height (), (unsigned int) 256);
      ++i;
      EXPECT_EQ (i.at_end (), true);
    }
  }

  EXPECT_EQ (view->num_l2ndbs (), (unsigned int) 2);
  EXPECT_EQ (tl::filename (view->get_l2ndb (0)->filename ()), "test.l2n");
  EXPECT_EQ (tl::filename (view->get_l2ndb (1)->filename ()), "test.lvsdb");
  EXPECT_EQ (dynamic_cast <db::LayoutVsSchematic *> (view->get_l2ndb (1)) != 0, true);

  EXPECT_EQ (view->num_rdbs (), (unsigned int) 1);
  EXPECT_EQ (tl::filename (view->get_rdb (0)->filename ()), "test.lyrdb");
}
