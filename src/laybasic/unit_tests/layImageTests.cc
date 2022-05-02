
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
#include "tlTimer.h"

#if defined(HAVE_QT)

#  include <QImage>
#  include <QPainter>

static bool compare_images (const QImage &qimg, const std::string &au)
{
  QImage qimg2;
  qimg2.load (tl::to_qstring (au));

  if (qimg2.width () == (int) qimg.width () && qimg2.height () == (int) qimg.height ()) {
    for (int i = 0; i < qimg.width (); ++i) {
      for (int j = 0; j < qimg.height (); ++j) {
        if (((const lay::color_t *) qimg.scanLine (j))[i] != ((const lay::color_t *) qimg2.scanLine (j))[i]) {
          return false;
        }
      }
    }
    return true;
  } else {
    return false;
  }
}

#endif

TEST(1)
{
  lay::Image img (15, 25);
  EXPECT_EQ (img.width (), 15);
  EXPECT_EQ (img.height (), 25);

  EXPECT_EQ (img.transparent (), false);
  img.set_transparent (true);
  EXPECT_EQ (img.transparent (), true);

  img.fill (0x112233);
  EXPECT_EQ (img.scan_line (5)[10], 0x112233);

  lay::Image img2;
  EXPECT_EQ (img2.transparent (), false);
  img2 = img;
  EXPECT_EQ (img2.transparent (), true);
  EXPECT_EQ (img2.width (), 15);
  EXPECT_EQ (img2.height (), 25);

  EXPECT_EQ (img.scan_line (5)[10], 0x112233);
  EXPECT_EQ (img2.scan_line (5)[10], 0x112233);

  img2.fill (0x332211);
  EXPECT_EQ (img.scan_line (5)[10], 0x112233);
  EXPECT_EQ (img2.scan_line (5)[10], 0x332211);

  img.set_transparent (false);
  img2.swap (img);
  EXPECT_EQ (img2.transparent (), false);
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

#if defined(HAVE_QT)

TEST(2)
{
  lay::Image img (227, 231);

  for (unsigned int i = 0; i < img.width (); ++i) {
    for (unsigned int j = 0; j < img.height (); ++j) {
      img.scan_line (j) [i] = 0xff000000 | (i << 16) | j;
    }
  }

  std::string tmp = tmp_file ("test.png");
  QImage qimg = img.to_image ();
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au.png";
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (qimg, au), true);

  lay::Image img_saved (img);
  img.scan_line (52) [42] = 0xff000000;

  lay::Image diff = img.diff (img_saved);
  EXPECT_EQ (compare_images (img.to_image (), au), false);
  EXPECT_EQ (compare_images (img_saved.to_image (), au), true);

  img.patch (diff);
  EXPECT_EQ (compare_images (img.to_image (), au), true);

  img.fill (0xff000000);
  img.patch (diff);

  tmp = tmp_file ("diff.png");
  qimg = img.to_image ();
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  au = tl::testsrc () + "/testdata/lay/au_diff.png";
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (qimg, au), true);
}

#endif

TEST(3)
{
  {
    tl::SelfTimer timer ("Run time - lay::Image copy, no write (should be very fast)");

    lay::Image img (1000, 1000);
    img.fill (0x112233);

    for (unsigned int i = 0; i < 5000; ++i) {
      lay::Image img2 (img);
    }
  }

#if defined(HAVE_QT)
  {
    tl::SelfTimer timer ("Run time - QImage copy, no write (should be very fast)");

    lay::Image img (1000, 1000);
    img.fill (0x112233);
    QImage qimg (img.to_image ());

    for (unsigned int i = 0; i < 5000; ++i) {
      QImage qimg2 (qimg);
    }
  }
#endif

  {
    tl::SelfTimer timer ("Run time - lay::Image copy on write");

    lay::Image img (1000, 1000);
    img.fill (0x112233);

    for (unsigned int i = 0; i < 5000; ++i) {
      lay::Image img2 (img);
      img2.scan_line (100) [7] = 0;
    }
  }

#if defined(HAVE_QT)
  {
    tl::SelfTimer timer ("Run time - QImage copy on write (should not be much less than lay::Image copy on write)");

    lay::Image img (1000, 1000);
    img.fill (0x112233);
    QImage qimg (img.to_image ());

    for (unsigned int i = 0; i < 5000; ++i) {
      QImage qimg2 (qimg);
      qimg2.scanLine (100) [7] = 0;
    }
  }

  {
    tl::SelfTimer timer ("Run time - direct QImage paint");

    lay::Image img (1000, 1000);
    img.fill (0x112233);
    QImage qimg (img.to_image ());
    QImage qrec (img.to_image ());
    qrec.fill (0);

    QPainter painter (&qrec);
    for (unsigned int i = 0; i < 1000; ++i) {
      painter.drawImage (QPoint (0, 0), qimg);
    }
  }

  {
    tl::SelfTimer timer ("Run time - lay::Image paint (should not be much more than direct QImage paint)");

    lay::Image img (1000, 1000);
    img.fill (0x112233);
    QImage qrec (img.to_image ());
    qrec.fill (0);

    QPainter painter (&qrec);
    for (unsigned int i = 0; i < 1000; ++i) {
      painter.drawImage (QPoint (0, 0), img.to_image ());
    }
  }

#endif
}

