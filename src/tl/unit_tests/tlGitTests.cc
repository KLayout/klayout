
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#if defined(HAVE_GIT2)

#include "tlGit.h"
#include "tlUnitTest.h"

// @@@
static std::string test_url ("https://github.com/klayoutmatthias/xsection.git");

TEST(1)
{
  std::string path = tl::TestBase::tmp_file ("repo");
  tl::GitObject repo (path);
  repo.read (test_url, std::string (), std::string ());

  printf("@@@ done: %s\n", path.c_str ());
}

TEST(2)
{
  std::string path = tl::TestBase::tmp_file ("repo");
  tl::GitObject repo (path);
  repo.read (test_url, std::string ("LICENSE"), std::string ());

  printf("@@@ done: %s\n", path.c_str ());
}

TEST(3)
{
  std::string path = tl::TestBase::tmp_file ("repo");
  tl::GitObject repo (path);
  repo.read (test_url, std::string (), std::string ("brxxx"));

  printf("@@@ done: %s\n", path.c_str ());
}

#endif

