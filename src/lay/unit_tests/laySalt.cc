
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


#include "laySaltGrain.h"
#include "laySaltGrains.h"
#include "laySalt.h"
#include "tlFileUtils.h"
#include "tlUnitTest.h"

#include <QDir>
#include <QSignalSpy>

static std::string grains_to_string (const lay::SaltGrains &gg, bool with_version = false)
{
  std::string res;
  res += "[";
  bool first = true;
  for (lay::SaltGrains::grain_iterator g = gg.begin_grains (); g != gg.end_grains (); ++g) {
    if (! first) {
      res += ",";
    }
    first = false;
    res += g->name ();
    if (with_version) {
      res += "(";
      res += g->version ();
      if (!g->url ().empty ()) {
        res += ":";
        res += g->url ();
      }
      res += ")";
    }
  }
  for (lay::SaltGrains::collection_iterator gc = gg.begin_collections (); gc != gg.end_collections (); ++gc) {
    if (! first) {
      res += ",";
    }
    first = false;
    res += gc->name ();
    res += grains_to_string (*gc, with_version);
  }
  res += "]";
  return res;
}

static std::string salt_to_string (lay::Salt &salt)
{
  std::string res;
  res += "[";
  bool first = true;
  for (lay::Salt::flat_iterator i = salt.begin_flat (); i != salt.end_flat (); ++i) {
    if (! first) {
      res += ",";
    }
    first = false;
    res += (*i)->name ();
  }
  res += "]";
  return res;
}

TEST (1)
{
  std::string tmp0 = tmp_file ("tmp0");

  lay::SaltGrain g;
  g.save (tmp0);
  EXPECT_EQ (g.authored_time ().isNull (), true);
  EXPECT_EQ (g.installed_time ().isNull (), true);

  lay::SaltGrain g0;
  g0.load (tmp0);
  EXPECT_EQ (g0.authored_time ().isNull (), true);
  EXPECT_EQ (g0.installed_time ().isNull (), true);
  EXPECT_EQ (g == g0, true);

  std::string tmp = tmp_file ();

  g.set_name ("abc");
  EXPECT_EQ (g.name (), "abc");
  g.set_url ("xyz");
  EXPECT_EQ (g.url (), "xyz");
  g.set_version ("1.0");
  EXPECT_EQ (g.version (), "1.0");
  g.set_path ("a/b");
  EXPECT_EQ (g.path (), "a/b");
  g.set_title ("title");
  EXPECT_EQ (g.title (), "title");
  g.set_doc ("doc");
  EXPECT_EQ (g.doc (), "doc");
  g.set_doc_url ("doc-url");
  EXPECT_EQ (g.doc_url (), "doc-url");
  g.set_author ("me");
  EXPECT_EQ (g.author (), "me");
  g.set_author_contact ("ac");
  EXPECT_EQ (g.author_contact (), "ac");
  g.set_license ("free");
  EXPECT_EQ (g.license (), "free");
  g.set_authored_time (QDateTime ());
  EXPECT_EQ (g.authored_time ().isNull (), true);
#if QT_VERSION >= 0x40700
  g.set_authored_time (QDateTime::fromMSecsSinceEpoch (1000000000));
  EXPECT_EQ (QDateTime::fromMSecsSinceEpoch (0).msecsTo (g.authored_time ()), 1000000000);
  g.set_installed_time (QDateTime ());
  EXPECT_EQ (g.installed_time ().isNull (), true);
  g.set_installed_time (QDateTime::fromMSecsSinceEpoch (2000000000));
  EXPECT_EQ (QDateTime::fromMSecsSinceEpoch (0).msecsTo (g.installed_time ()), 2000000000);
#endif

  g.add_dependency (lay::SaltGrainDependency ());
  g.dependencies ().back ().name = "depname";
  g.dependencies ().back ().url = "depurl";
  g.dependencies ().back ().version = "0.0";
  EXPECT_EQ (int (g.dependencies ().size ()), 1);

  lay::SaltGrain gg;
  EXPECT_EQ (g == gg, false);
  EXPECT_EQ (g == g, true);
  EXPECT_EQ (g != gg, true);
  EXPECT_EQ (g != g, false);

  gg = g;
  EXPECT_EQ (g == gg, true);

  gg.set_doc ("blabla");
  EXPECT_EQ (g == gg, false);

  EXPECT_EQ (g == gg, false);
  g.save (tmp);

  EXPECT_EQ (g == gg, false);

  gg = lay::SaltGrain ();
  gg.load (tmp);
  gg.set_path (g.path ());  //  path is not set by load(file)
  EXPECT_EQ (int (gg.dependencies ().size ()), 1);
  EXPECT_EQ (g == gg, true);

  gg.add_dependency (lay::SaltGrainDependency ());
  EXPECT_EQ (g == gg, false);
  gg.set_path (tl::to_string (QFileInfo (tl::to_qstring (tmp)).absolutePath ()));
  gg.save ();

  g = lay::SaltGrain::from_path (gg.path ());
  EXPECT_EQ (g == gg, true);
}

TEST (2)
{
  EXPECT_EQ (lay::SaltGrain::valid_version (""), true);
  EXPECT_EQ (lay::SaltGrain::valid_version ("1"), true);
  EXPECT_EQ (lay::SaltGrain::valid_version ("1.2"), true);
  EXPECT_EQ (lay::SaltGrain::valid_version ("\t1 . 2.\n3"), true);
  EXPECT_EQ (lay::SaltGrain::valid_version ("x"), false);
  EXPECT_EQ (lay::SaltGrain::valid_version ("1.2x"), false);
  EXPECT_EQ (lay::SaltGrain::valid_name (""), false);
  EXPECT_EQ (lay::SaltGrain::valid_name ("x"), true);
  EXPECT_EQ (lay::SaltGrain::valid_name (".x"), false);
  EXPECT_EQ (lay::SaltGrain::valid_name (".."), false);
  EXPECT_EQ (lay::SaltGrain::valid_name ("x1"), true);
  EXPECT_EQ (lay::SaltGrain::valid_name ("x1 "), false);
  EXPECT_EQ (lay::SaltGrain::valid_name ("x$1"), false);
  EXPECT_EQ (lay::SaltGrain::valid_name ("x/y"), true);
  EXPECT_EQ (lay::SaltGrain::valid_name ("x/.y"), false);
  EXPECT_EQ (lay::SaltGrain::valid_name ("x/.."), false);
  EXPECT_EQ (lay::SaltGrain::valid_name ("x_y"), true);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("", ""), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1", "2"), -1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1", ""), 1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1", "1"), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("2", "1"), 1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.0", "2.0"), -1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.0", "1.0"), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.1", "1.0"), 1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.0.1", "1.0.0"), 1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.0.1", "1.0"), 1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.0.1", "1"), 1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.0.0", "1"), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1a", "1"), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.a.1", "1.0.1"), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.1a", "1.1"), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.1a", "1.0"), 1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.1a.1", "1.0"), 1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("1.1a.1", "1.1.1"), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("990", "991"), -1);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("990", "990"), 0);
  EXPECT_EQ (lay::SaltGrain::compare_versions ("991", "990"), 1);
}

TEST (3)
{
  const QString grain_spec_file = QString::fromUtf8 ("grain.xml");

  lay::SaltGrain g;
  g.set_name ("x");

  QDir tmp_dir (QFileInfo (tl::to_qstring (tmp_file ())).absolutePath ());
  QDir dir_a (tmp_dir.filePath (QString::fromUtf8 ("a")));
  QDir dir_b (tmp_dir.filePath (QString::fromUtf8 ("b")));
  QDir dir_c (tmp_dir.filePath (QString::fromUtf8 ("c")));
  QDir dir_cu (dir_c.filePath (QString::fromUtf8 ("u")));
  QDir dir_cc (dir_c.filePath (QString::fromUtf8 ("c")));
  QDir dir_ccv (dir_cc.filePath (QString::fromUtf8 ("v")));

  lay::SaltGrains gg;
  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (gg.is_empty (), true);
  EXPECT_EQ (grains_to_string (gg), "[]");

  tmp_dir.mkdir (dir_a.dirName ());
  tmp_dir.mkdir (dir_b.dirName ());
  tmp_dir.mkdir (dir_c.dirName ());
  dir_c.mkdir (dir_cu.dirName ());
  dir_c.mkdir (dir_cc.dirName ());
  dir_cc.mkdir (dir_ccv.dirName ());

  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (gg.is_empty (), true);
  EXPECT_EQ (grains_to_string (gg), "[]");
  EXPECT_EQ (gg.path (), tl::to_string (tmp_dir.path ()));

  g.save (tl::to_string (dir_a.absoluteFilePath (grain_spec_file)));

  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (gg.is_empty (), false);
  EXPECT_EQ (grains_to_string (gg), "[a]");
  EXPECT_EQ (gg.begin_grains ()->path (), tl::to_string (dir_a.absolutePath ()));

  g.save (tl::to_string (dir_b.absoluteFilePath (grain_spec_file)));
  g.save (tl::to_string (dir_cu.absoluteFilePath (grain_spec_file)));
  g.save (tl::to_string (dir_ccv.absoluteFilePath (grain_spec_file)));

  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (gg.is_empty (), false);
  EXPECT_EQ (grains_to_string (gg), "[a,b,c[c/u,c/c[c/c/v]]]");
  EXPECT_EQ (gg.begin_collections ()->path (), tl::to_string (dir_c.absolutePath ()));

  std::string gg_path = tmp_file ("gg.tmp");
  gg.save (gg_path);

  lay::SaltGrains ggg;
  ggg.load (gg_path);
  EXPECT_EQ (grains_to_string (ggg), "[a,b,c[c/u,c/c[c/c/v]]]");
  //  NOTE: The path is not set, so this will fail:
  //  EXPECT_EQ (gg == ggg, true);

  gg.remove_grain (gg.begin_grains (), false);
  EXPECT_EQ (grains_to_string (gg), "[b,c[c/u,c/c[c/c/v]]]");

  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (grains_to_string (gg), "[a,b,c[c/u,c/c[c/c/v]]]");
  gg.remove_grain (gg.begin_grains (), true);

  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (grains_to_string (gg), "[b,c[c/u,c/c[c/c/v]]]");

  gg.remove_collection (gg.begin_collections (), false);
  EXPECT_EQ (grains_to_string (gg), "[b]");

  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (grains_to_string (gg), "[b,c[c/u,c/c[c/c/v]]]");

  gg.remove_collection (gg.begin_collections (), true);
  EXPECT_EQ (grains_to_string (gg), "[b]");
  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (grains_to_string (gg), "[b]");
}

TEST (4)
{
  const QString grain_spec_file = QString::fromUtf8 ("grain.xml");

  //  That's just preparation ...

  lay::SaltGrain g;
  g.set_name ("x");

  QDir tmp_dir (QFileInfo (tl::to_qstring (tmp_file ())).absolutePath ());
  QDir dir_a (tmp_dir.filePath (QString::fromUtf8 ("a")));
  QDir dir_b (tmp_dir.filePath (QString::fromUtf8 ("b")));
  QDir dir_c (tmp_dir.filePath (QString::fromUtf8 ("c")));
  QDir dir_cu (dir_c.filePath (QString::fromUtf8 ("u")));
  QDir dir_cc (dir_c.filePath (QString::fromUtf8 ("c")));
  QDir dir_ccv (dir_cc.filePath (QString::fromUtf8 ("v")));

  lay::SaltGrains gg;
  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (gg.is_empty (), true);
  EXPECT_EQ (grains_to_string (gg), "[]");

  tmp_dir.mkdir (dir_a.dirName ());
  tmp_dir.mkdir (dir_b.dirName ());
  tmp_dir.mkdir (dir_c.dirName ());
  dir_c.mkdir (dir_cu.dirName ());
  dir_c.mkdir (dir_cc.dirName ());
  dir_cc.mkdir (dir_ccv.dirName ());

  gg = lay::SaltGrains::from_path (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (gg.is_empty (), true);
  EXPECT_EQ (grains_to_string (gg), "[]");
  EXPECT_EQ (gg.path (), tl::to_string (tmp_dir.path ()));

  g.save (tl::to_string (dir_a.absoluteFilePath (grain_spec_file)));
  g.save (tl::to_string (dir_b.absoluteFilePath (grain_spec_file)));
  g.save (tl::to_string (dir_cu.absoluteFilePath (grain_spec_file)));
  g.save (tl::to_string (dir_ccv.absoluteFilePath (grain_spec_file)));

  //  That's the main test part

  lay::Salt salt;
  EXPECT_EQ (salt.is_empty (), true);

  QSignalSpy spy (&salt, SIGNAL (collections_changed ()));
  EXPECT_EQ (salt_to_string (salt), "[]");

  spy.clear ();
  salt.add_location (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (salt.is_empty (), false);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (salt_to_string (salt), "[a,b,c/c/v,c/u]");

  spy.clear ();
  salt.add_location (tl::to_string (tmp_dir.path ()));
  EXPECT_EQ (spy.count (), 0);
  EXPECT_EQ (salt_to_string (salt), "[a,b,c/c/v,c/u]");

  spy.clear ();
  salt.add_location (tl::to_string (dir_c.path ()));
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (salt_to_string (salt), "[a,b,c/c/v,c/u,c/v,u]");

  lay::Salt salt_copy = salt;
  (const_cast<lay::SaltGrains &> (*salt_copy.begin ())).remove_grain (salt_copy.begin ()->begin_grains (), true);

  spy.clear ();
  salt.refresh ();
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (salt_to_string (salt), "[b,c/c/v,c/u,c/v,u]");

  spy.clear ();
  salt.remove_location (tl::to_string (dir_c.path ()));
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (salt_to_string (salt), "[b,c/c/v,c/u]");

  spy.clear ();
  //  location already removed
  salt.remove_location (tl::to_string (dir_c.path ()));
  EXPECT_EQ (spy.count (), 0);
  EXPECT_EQ (salt_to_string (salt), "[b,c/c/v,c/u]");

  EXPECT_EQ (salt.grain_by_name ("x"), 0);
  EXPECT_EQ (salt.grain_by_name ("b")->name (), "b");
  EXPECT_EQ (salt.grain_by_name ("c/c/v")->name (), "c/c/v");
}

TEST (5)
{
  lay::SaltGrains grains;

  lay::SaltGrain g1;
  g1.set_name ("g1");
  lay::SaltGrainDependency dep;
  dep.name = "g2";
  g1.dependencies ().push_back (dep);
  dep.name = "g3";
  g1.dependencies ().push_back (dep);
  grains.add_grain (g1);

  lay::SaltGrains g34;

  lay::SaltGrain g3;
  g3.set_name ("g3");
  g34.add_grain (g3);

  lay::SaltGrain g4;
  g4.set_name ("g4");
  g34.add_grain (g4);

  grains.add_collection (g34);

  lay::SaltGrain g2;
  g2.set_name ("g2");
  dep.name = "g3";
  g2.dependencies ().push_back (dep);
  grains.add_grain (g2);

  lay::Salt salt;
  salt.root ().add_collection (grains);

  std::vector<std::string> names;
  for (lay::Salt::flat_iterator i = salt.begin_flat (); i != salt.end_flat (); ++i) {
    names.push_back ((*i)->name ());
  }

  EXPECT_EQ (tl::join (names, ","), "g3,g2,g1,g4");
}

TEST (6)
{
  lay::SaltGrains gg1;
  lay::SaltGrains gg2;

  lay::SaltGrain ga1;
  ga1.set_name ("a");
  ga1.set_url ("url1");
  ga1.set_version ("1.0");

  lay::SaltGrain ga2;
  ga2.set_name ("a");
  ga2.set_url ("url2");
  ga2.set_version ("1.1");

  lay::SaltGrain gb;
  gb.set_name ("b");

  lay::SaltGrain gc;
  gc.set_name ("c");

  gg1.add_grain (ga1);
  gg1.add_grain (gb);

  gg2.add_grain (gc);
  gg2.add_grain (ga2);

  //  higher version wins
  gg1.merge_with (gg2);
  EXPECT_EQ (grains_to_string (gg1, true), "[b(),c(),a(1.1:url2)]");

  gg1 = lay::SaltGrains ();
  gg2 = lay::SaltGrains ();

  gg2.add_grain (gc);
  gg1.add_grain (ga2);
  gg1.add_grain (gb);

  gg2.add_grain (ga1);

  //  higher version wins - also in different order
  gg1.merge_with (gg2);
  EXPECT_EQ (grains_to_string (gg1, true), "[a(1.1:url2),b(),c()]");

  gg1 = lay::SaltGrains ();
  gg2 = lay::SaltGrains ();

  gg2.add_grain (gc);
  ga2.set_version ("1.0");
  gg1.add_grain (ga2);
  gg1.add_grain (gb);

  gg2.add_grain (ga1);

  //  first one wins on same version
  gg1.merge_with (gg2);
  EXPECT_EQ (grains_to_string (gg1, true), "[a(1.0:url2),b(),c()]");

  gg1 = lay::SaltGrains ();

  gg1.add_grain (gc);
  gg1.add_grain (ga2);
  gg1.add_grain (ga1);
  gg1.add_grain (gb);

  //  consolidate does the same on one list
  gg1.consolidate ();
  EXPECT_EQ (grains_to_string (gg1, true), "[c(),a(1.0:url2),b()]");

  gg1 = lay::SaltGrains ();

  gg1.add_grain (ga1);
  gg1.add_grain (ga2);
  gg1.add_grain (gb);
  gg1.add_grain (gc);

  //  consolidate does the same on one list
  gg1.consolidate ();
  EXPECT_EQ (grains_to_string (gg1, true), "[a(1.0:url1),b(),c()]");

  gg1 = lay::SaltGrains ();

  ga1.set_version ("1.1");
  gg1.add_grain (ga1);
  gg1.add_grain (ga2);
  gg1.add_grain (gb);

  //  consolidate does the same on one list
  gg1.consolidate ();
  EXPECT_EQ (grains_to_string (gg1, true), "[a(1.1:url1),b()]");


  //  merging of sub-collections

  gg1 = lay::SaltGrains ();
  gg2 = lay::SaltGrains ();

  lay::SaltGrains gga1;
  gga1.set_name ("a");

  {
    lay::SaltGrain g;
    g.set_name ("a");
    g.set_version ("1.0");
    g.set_url ("url1");
    gga1.add_grain (g);
  }

  {
    lay::SaltGrain g;
    g.set_name ("b");
    gga1.add_grain (g);
  }

  lay::SaltGrains ggb;
  ggb.set_name ("b");

  {
    lay::SaltGrain g;
    g.set_name ("x");
    ggb.add_grain (g);
  }

  gg1.add_collection (gga1);
  gg1.add_collection (ggb);

  lay::SaltGrains gga2;
  gga2.set_name ("a");

  {
    lay::SaltGrain g;
    g.set_name ("a");
    g.set_version ("1.1");
    g.set_url ("url2");
    gga2.add_grain (g);
  }

  {
    lay::SaltGrain g;
    g.set_name ("c");
    gga2.add_grain (g);
  }

  lay::SaltGrains ggc;
  ggc.set_name ("c");

  {
    lay::SaltGrain g;
    g.set_name ("y");
    ggc.add_grain (g);
  }

  gg2.add_collection (gga2);
  gg2.add_collection (ggc);

  //  gg2:a collection is merged into gg1:a, gg2:c is copied.
  gg1.merge_with (gg2);
  EXPECT_EQ (grains_to_string (gg1, true), "[a[b(),a(1.1:url2),c()],b[x()],c[y()]]");
}
