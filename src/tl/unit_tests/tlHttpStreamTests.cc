
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


#include "tlHttpStream.h"
#include "tlUnitTest.h"
#include "tlTimer.h"
#include "tlStream.h"
#include "tlEnv.h"

static std::string test_url1 ("http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text");
static std::string test_url1_gz ("http://www.klayout.org/svn-public/klayout-resources/trunk/testdata2/text.gz");
static std::string test_url2 ("http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/dir1");

TEST(1)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::InputHttpStream stream (test_url1);

  char b[100];
  size_t n = stream.read (b, sizeof (b));
  std::string res (b, n);
  EXPECT_EQ (res, "hello, world.\n");
}

TEST(2)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::InputHttpStream stream (test_url2);
  stream.add_header ("User-Agent", "SVN");
  stream.add_header ("Depth", "1");
  stream.set_request ("PROPFIND");
  stream.set_data ("<?xml version=\"1.0\" encoding=\"utf-8\"?><propfind xmlns=\"DAV:\"><prop><resourcetype xmlns=\"DAV:\"/></prop></propfind>");

  char b[10000];
  size_t n = stream.read (b, sizeof (b));
  std::string res (b, n);

  EXPECT_EQ (res,
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<D:multistatus xmlns:D=\"DAV:\" xmlns:ns0=\"DAV:\">\n"
    "<D:response xmlns:lp1=\"DAV:\">\n"
    "<D:href>/svn-public/klayout-resources/trunk/testdata/dir1/</D:href>\n"
    "<D:propstat>\n"
    "<D:prop>\n"
    "<lp1:resourcetype><D:collection/></lp1:resourcetype>\n"
    "</D:prop>\n"
    "<D:status>HTTP/1.1 200 OK</D:status>\n"
    "</D:propstat>\n"
    "</D:response>\n"
    "<D:response xmlns:lp1=\"DAV:\">\n"
    "<D:href>/svn-public/klayout-resources/trunk/testdata/dir1/text</D:href>\n"
    "<D:propstat>\n"
    "<D:prop>\n"
    "<lp1:resourcetype/>\n"
    "</D:prop>\n"
    "<D:status>HTTP/1.1 200 OK</D:status>\n"
    "</D:propstat>\n"
    "</D:response>\n"
    "</D:multistatus>\n"
  );
}

namespace
{

class Receiver : public tl::Object
{
public:
  Receiver () : flag (false) { }
  void handle () { flag = true; }
  bool flag;
};

}

//  async mode
TEST(3)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::InputHttpStream stream (test_url1);

  Receiver r;
  stream.ready ().add (&r, &Receiver::handle);

  EXPECT_EQ (stream.data_available (), false);
  stream.send ();
  EXPECT_EQ (stream.data_available (), false);

  tl::Clock start = tl::Clock::current ();
  while (! r.flag && (tl::Clock::current () - start).seconds () < 10) {
    stream.tick ();
  }
  EXPECT_EQ (r.flag, true);
  EXPECT_EQ (stream.data_available (), true);

  char b[100];
  size_t n = stream.read (b, sizeof (b));
  std::string res (b, n);
  EXPECT_EQ (res, "hello, world.\n");
}

//  tl::Stream embedding
TEST(4)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::InputStream stream (test_url1);

  std::string res = stream.read_all ();
  EXPECT_EQ (res, "hello, world.\n");
}

//  tl::Stream embedding with automatic unzip
TEST(5)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::InputStream stream (test_url1_gz);

  std::string res = stream.read_all ();
  EXPECT_EQ (res, "hello, world.\n");
}

//  tl::InputHttpStream timeout
TEST(6)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  {
    tl::set_env ("KLAYOUT_HTTP_TIMEOUT", "");
    tl::InputHttpStream stream (test_url1);
    stream.set_timeout (0.001); //  probably too fast :)

    try {
      char b[100];
      stream.read (b, sizeof (b));
      EXPECT_EQ (true, false);
    } catch (tl::HttpErrorException &ex) {
      tl::info << "Got exception (expected): " << ex.msg ();
    }
  }

  {
    tl::set_env ("KLAYOUT_HTTP_TIMEOUT", "0.001");
    tl::InputHttpStream stream (test_url1);

    try {
      char b[100];
      stream.read (b, sizeof (b));
      EXPECT_EQ (true, false);
    } catch (tl::HttpErrorException &ex) {
      tl::info << "Got exception (expected): " << ex.msg ();
    }
  }

  tl::unset_env ("KLAYOUT_HTTP_TIMEOUT");
}

