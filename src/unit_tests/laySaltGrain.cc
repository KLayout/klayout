
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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
#include "utHead.h"

#include <QDir>

TEST (1)
{
  lay::SaltGrain g;

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

  g.add_dependency (lay::SaltGrain::Dependency ());
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

  std::string tmp = tmp_file ();

  EXPECT_EQ (g == gg, false);
  g.save (tmp);

  EXPECT_EQ (g == gg, false);

  gg = lay::SaltGrain ();
  gg.load (tmp);
  gg.set_path (g.path ());  //  path is not set by load(file)
  EXPECT_EQ (int (gg.dependencies ().size ()), 1);
  EXPECT_EQ (g == gg, true);

  gg.add_dependency (lay::SaltGrain::Dependency ());
  EXPECT_EQ (g == gg, false);
  gg.set_path (tl::to_string (QFileInfo (tl::to_qstring (tmp)).absolutePath ()));
  gg.save ();

  g = lay::SaltGrain::from_path (gg.path ());
  EXPECT_EQ (g == gg, true);
}

TEST (2)
{
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
