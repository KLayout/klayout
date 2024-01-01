
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

#include "laySaltParsedURL.h"
#include "tlUnitTest.h"

TEST (1_Basic)
{
  lay::SaltParsedURL purl ("https://server.com/repo/trunk");
  EXPECT_EQ (purl.protocol () == lay::DefaultProtocol, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo/trunk");
  EXPECT_EQ (purl.branch (), "");
  EXPECT_EQ (purl.subfolder (), "");
}

TEST (2_SVN)
{
  lay::SaltParsedURL purl ("svn+https://server.com/repo/trunk");
  EXPECT_EQ (purl.protocol () == lay::WebDAV, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo/trunk");
  EXPECT_EQ (purl.branch (), "");
  EXPECT_EQ (purl.subfolder (), "");
}

TEST (10_GitBasic)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "");
  EXPECT_EQ (purl.subfolder (), "");
}

TEST (11_GitSubFolder)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git/sub/folder");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "");
  EXPECT_EQ (purl.subfolder (), "sub/folder");
}

TEST (12_GitExplicitBranch)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git[v1.0]");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "v1.0");
  EXPECT_EQ (purl.subfolder (), "");
}

TEST (13_GitExplicitBranchAndSubFolder)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git/sub/folder[refs/tags/1.0]");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "refs/tags/1.0");
  EXPECT_EQ (purl.subfolder (), "sub/folder");
}

TEST (14_GitExplicitBranchAndExplicitSubFolder)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo+sub/folder[refs/tags/1.0]");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo");
  EXPECT_EQ (purl.branch (), "refs/tags/1.0");
  EXPECT_EQ (purl.subfolder (), "sub/folder");
}

TEST (15_GitSVNEmulationTrunk)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git/trunk");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "HEAD");
  EXPECT_EQ (purl.subfolder (), "");
}

TEST (16_GitSVNEmulationTrunkWithSubFolder)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git/trunk/sub/folder");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "HEAD");
  EXPECT_EQ (purl.subfolder (), "sub/folder");
}

TEST (17_GitSVNEmulationBranch)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git/branches/xyz");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "refs/heads/xyz");
  EXPECT_EQ (purl.subfolder (), "");
}

TEST (18_GitSVNEmulationTag)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git/tags/1.9");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "refs/tags/1.9");
  EXPECT_EQ (purl.subfolder (), "");
}

TEST (19_GitSVNEmulationTagWithSubFolder)
{
  lay::SaltParsedURL purl ("git+https://server.com/repo.git/tags/1.9/sub/folder");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://server.com/repo.git");
  EXPECT_EQ (purl.branch (), "refs/tags/1.9");
  EXPECT_EQ (purl.subfolder (), "sub/folder");
}

TEST (20_Example1)
{
  lay::SaltParsedURL purl ("git+https://github.com/my-user/test-core[refs/tags/v1.1.0]");
  EXPECT_EQ (purl.protocol () == lay::Git, true);
  EXPECT_EQ (purl.url (), "https://github.com/my-user/test-core");
  EXPECT_EQ (purl.branch (), "refs/tags/v1.1.0");
  EXPECT_EQ (purl.subfolder (), "");
}
