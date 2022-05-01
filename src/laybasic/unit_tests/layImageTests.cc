
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

#include "layImage.h"

#include "tlUnitTest.h"

#if defined(HAVE_QT)
#include <QImage>
#endif

TEST(1)
{
  lay::Image img (15, 25);
  EXPECT_EQ (img.width (), 15);
  EXPECT_EQ (img.height (), 25);

  img.fill (0x112233);
  EXPECT_EQ (img.scan_line (5)[10], 0x112233);

  lay::Image img2;
  img2 = img;
  EXPECT_EQ (img2.width (), 15);
  EXPECT_EQ (img2.height (), 25);

  EXPECT_EQ (img.scan_line (5)[10], 0x112233);
  EXPECT_EQ (img2.scan_line (5)[10], 0x112233);

  img2.fill (0x332211);
  EXPECT_EQ (img.scan_line (5)[10], 0x112233);
  EXPECT_EQ (img2.scan_line (5)[10], 0x332211);

  img2.swap (img);
  EXPECT_EQ (img2.scan_line (5)[10], 0x112233);
  EXPECT_EQ (img.scan_line (5)[10], 0x332211);

  img2 = img;
  EXPECT_EQ (img.scan_line (5)[10], 0x332211);
  EXPECT_EQ (img2.scan_line (5)[10], 0x332211);

  img2 = lay::Image (10, 16);
  EXPECT_EQ (img.width (), 15);
  EXPECT_EQ (img.height (), 25);
  EXPECT_EQ (img2.width (), 10);
  EXPECT_EQ (img2.height (), 16);
  img2.fill (0x010203);

  EXPECT_EQ (img.scan_line (5)[10], 0x332211);
  EXPECT_EQ (img2.scan_line (5)[8], 0x010203);

  img = std::move (img2);
  EXPECT_EQ (img.width (), 10);
  EXPECT_EQ (img.height (), 16);
  EXPECT_EQ (img.scan_line (5)[8], 0x010203);

  lay::Image img3 (img);
  EXPECT_EQ (img3.width (), 10);
  EXPECT_EQ (img3.height (), 16);
  EXPECT_EQ (img3.scan_line (5)[8], 0x010203);

  img.fill (0x102030);
  EXPECT_EQ (img3.width (), 10);
  EXPECT_EQ (img3.height (), 16);
  EXPECT_EQ (img3.scan_line (5)[8], 0x010203);
  EXPECT_EQ (img.width (), 10);
  EXPECT_EQ (img.height (), 16);
  EXPECT_EQ (img.scan_line (5)[8], 0x102030);

  lay::Image img4 (std::move (img));
  EXPECT_EQ (img4.width (), 10);
  EXPECT_EQ (img4.height (), 16);
  EXPECT_EQ (img4.scan_line (5)[8], 0x102030);
}
