
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

#include "tlPixelBuffer.h"

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
    for (int j = 0; j < qimg.height (); ++j) {
      for (int i = 0; i < qimg.width (); ++i) {
        if (((const tl::color_t *) qimg.scanLine (j))[i] != ((const tl::color_t *) qimg2.scanLine (j))[i]) {
          return false;
        }
      }
    }
    return true;
  } else {
    return false;
  }
}

static bool compare_images_mono (const QImage &qimg, const std::string &au)
{
  QImage qimg2;
  qimg2.load (tl::to_qstring (au));
  qimg2 = qimg2.convertToFormat (QImage::Format_MonoLSB); // that's the format of BitmapBuffer ..

  if (qimg2.width () == (int) qimg.width () && qimg2.height () == (int) qimg.height ()) {
    //  NOTE: slooooow ...
    for (int j = 0; j < qimg.height (); ++j) {
      for (int i = 0; i < qimg.width (); ++i) {
        if ((qimg.scanLine (j)[i / 8] & (0x01 << (i % 8))) != (qimg2.scanLine (j)[i / 8] & (0x01 << (i % 8)))) {
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

static bool compare_images (const tl::PixelBuffer &img, const tl::PixelBuffer &img2)
{
  return img == img2;
}

static bool compare_images (const tl::BitmapBuffer &img, const tl::BitmapBuffer &img2)
{
  return img == img2;
}

TEST(1)
{
  tl::PixelBuffer img (15, 25);
  EXPECT_EQ (img.width (), 15u);
  EXPECT_EQ (img.height (), 25u);
  EXPECT_EQ (img.stride (), 15 * sizeof (tl::color_t));

  EXPECT_EQ (img.transparent (), false);
  img.set_transparent (true);
  EXPECT_EQ (img.transparent (), true);

  img.fill (0x112233);
  EXPECT_EQ (img.scan_line (5)[10], 0x112233u);

  tl::PixelBuffer img2;
  EXPECT_EQ (img2.transparent (), false);
  img2 = img;
  EXPECT_EQ (img2.transparent (), true);
  EXPECT_EQ (img2.width (), 15u);
  EXPECT_EQ (img2.height (), 25u);

  EXPECT_EQ (img.scan_line (5)[10], 0x112233u);
  EXPECT_EQ (img2.scan_line (5)[10], 0x112233u);

  img2.fill (0x332211);
  EXPECT_EQ (img.scan_line (5)[10], 0x112233u);
  EXPECT_EQ (img2.scan_line (5)[10], 0x332211u);

  img.set_transparent (false);
  img2.swap (img);
  EXPECT_EQ (img2.transparent (), false);
  EXPECT_EQ (img2.scan_line (5)[10], 0x112233u);
  EXPECT_EQ (img.scan_line (5)[10], 0x332211u);

  img2 = img;
  EXPECT_EQ (compare_images (img, img2), true);
  EXPECT_EQ (img.scan_line (5)[10], 0x332211u);
  EXPECT_EQ (img2.scan_line (5)[10], 0x332211u);

  img2 = tl::PixelBuffer (10, 16);
  EXPECT_EQ (img.width (), 15u);
  EXPECT_EQ (img.height (), 25u);
  EXPECT_EQ (img2.width (), 10u);
  EXPECT_EQ (img2.height (), 16u);
  img2.fill (0x010203);
  EXPECT_EQ (compare_images (img, img2), false);

  EXPECT_EQ (img.scan_line (5)[10], 0x332211u);
  EXPECT_EQ (img2.scan_line (5)[8], 0xff010203u);

  img = std::move (img2);
  EXPECT_EQ (compare_images (img, img2), false);
  EXPECT_EQ (img.width (), 10u);
  EXPECT_EQ (img.height (), 16u);
  EXPECT_EQ (img.scan_line (5)[8], 0xff010203u);

  tl::PixelBuffer img3 (img);
  EXPECT_EQ (compare_images (img, img3), true);
  EXPECT_EQ (img3.width (), 10u);
  EXPECT_EQ (img3.height (), 16u);
  EXPECT_EQ (img3.scan_line (5)[8], 0xff010203u);

  img.fill (0x102030);
  EXPECT_EQ (compare_images (img, img3), false);
  EXPECT_EQ (img3.width (), 10u);
  EXPECT_EQ (img3.height (), 16u);
  EXPECT_EQ (img3.scan_line (5)[8], 0xff010203u);
  EXPECT_EQ (img.width (), 10u);
  EXPECT_EQ (img.height (), 16u);
  EXPECT_EQ (img.scan_line (5)[8], 0xff102030u);

  tl::PixelBuffer img4 (std::move (img));
  EXPECT_EQ (img4.width (), 10u);
  EXPECT_EQ (img4.height (), 16u);
  EXPECT_EQ (img4.scan_line (5)[8], 0xff102030u);

  //  other constructors
  EXPECT_EQ (compare_images (tl::PixelBuffer (img4.width (), img4.height (), (const tl::color_t *) img4.data ()), img4), true);
  EXPECT_EQ (compare_images (tl::PixelBuffer (img4.width (), img4.height (), (const tl::color_t *) img4.data (), img4.stride ()), img4), true);

  tl::color_t *dnew = new tl::color_t [ img4.width () * img4.height () * sizeof (tl::color_t) ];
  memcpy (dnew, (const tl::color_t *) img4.data (), img4.width () * img4.height () * sizeof (tl::color_t));
  EXPECT_EQ (compare_images (tl::PixelBuffer (img4.width (), img4.height (), dnew), img4), true);
}

#if defined(HAVE_QT)

TEST(2)
{
  tl::PixelBuffer img (227, 231);

  for (unsigned int i = 0; i < img.width (); ++i) {
    for (unsigned int j = 0; j < img.height (); ++j) {
      img.scan_line (j) [i] = 0xff000000 | (i << 16) | j;
    }
  }

  EXPECT_EQ (img.transparent (), false);
  EXPECT_EQ (img.to_image ().format () == QImage::Format_RGB32, true);

  std::string tmp = tmp_file ("test.png");
  QImage qimg = img.to_image ();
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au.png";
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (qimg, au), true);

  tl::PixelBuffer img_returned = tl::PixelBuffer::from_image (qimg);
  EXPECT_EQ (compare_images (img, img_returned), true);

  tl::PixelBuffer img_saved (img);
  img.scan_line (52) [42] = 0xff000000;

  tl::PixelBuffer diff = img.diff (img_saved);
  EXPECT_EQ (diff.transparent (), true);
  EXPECT_EQ (diff.to_image ().format () == QImage::Format_ARGB32, true);
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

  qimg = img.to_image_copy ();
  img.fill (false);

  tmp = tmp_file ("test2.png");
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  EXPECT_EQ (compare_images (qimg, au), true);
}

#endif

#if defined(HAVE_PNG)

//  libpng support
TEST(3)
{
  tl::PixelBuffer img;

  std::string in = tl::testsrc () + "/testdata/lay/png1.png";  //  ARGB32
  tl::info << "PNG file read (libpng) from " << in;

  {
    tl::InputStream stream (in);
    img = tl::PixelBuffer::read_png (stream);
  }

  std::string tmp = tmp_file ("test.png");
  {
    tl::OutputStream stream (tmp);
    img.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp;

  tl::PixelBuffer img2;

  {
    tl::InputStream stream (tmp);
    img2 = tl::PixelBuffer::read_png (stream);
  }

  std::string tmp2 = tmp_file ("test2.png");
  {
    tl::OutputStream stream (tmp2);
    img2.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp2;

  EXPECT_EQ (compare_images (img, img2), true);

#if defined (HAVE_QT)
  //  Qt cross-check
  std::string au = tl::testsrc () + "/testdata/lay/au.png";
  EXPECT_EQ (compare_images (img2.to_image (), au), true);
#endif
}

TEST(4)
{
  tl::PixelBuffer img;

  std::string in = tl::testsrc () + "/testdata/lay/png2.png";  //  RGB32
  tl::info << "PNG file read (libpng) from " << in;

  {
    tl::InputStream stream (in);
    img = tl::PixelBuffer::read_png (stream);
  }

  std::string tmp = tmp_file ("test.png");
  {
    tl::OutputStream stream (tmp);
    img.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp;

  tl::PixelBuffer img2;

  {
    tl::InputStream stream (tmp);
    img2 = tl::PixelBuffer::read_png (stream);
  }

  std::string tmp2 = tmp_file ("test2.png");
  {
    tl::OutputStream stream (tmp2);
    img2.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp2;

  EXPECT_EQ (compare_images (img, img2), true);

#if defined (HAVE_QT)
  //  Qt cross-check
  std::string au = tl::testsrc () + "/testdata/lay/au.png";
  EXPECT_EQ (compare_images (img2.to_image (), au), true);
#endif
}

TEST(5)
{
  tl::PixelBuffer img;

  std::string in = tl::testsrc () + "/testdata/lay/png3.png";  //  GA
  tl::info << "PNG file read (libpng) from " << in;

  {
    tl::InputStream stream (in);
    img = tl::PixelBuffer::read_png (stream);
  }

  std::string tmp = tmp_file ("test.png");
  {
    tl::OutputStream stream (tmp);
    img.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp;

  tl::PixelBuffer img2;

  {
    tl::InputStream stream (tmp);
    img2 = tl::PixelBuffer::read_png (stream);
  }

  std::string tmp2 = tmp_file ("test2.png");
  {
    tl::OutputStream stream (tmp2);
    img2.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp2;

  EXPECT_EQ (compare_images (img, img2), true);

#if defined (HAVE_QT)
  //  Qt cross-check
  std::string au = tl::testsrc () + "/testdata/lay/au_gs.png";
  EXPECT_EQ (compare_images (img2.to_image (), au), true);
#endif
}

TEST(6)
{
  tl::PixelBuffer img;

  std::string in = tl::testsrc () + "/testdata/lay/png4.png";  //  G
  tl::info << "PNG file read (libpng) from " << in;

  {
    tl::InputStream stream (in);
    img = tl::PixelBuffer::read_png (stream);
  }

  std::string tmp = tmp_file ("test.png");
  {
    tl::OutputStream stream (tmp);
    img.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp;

  tl::PixelBuffer img2;

  {
    tl::InputStream stream (tmp);
    img2 = tl::PixelBuffer::read_png (stream);
  }

  std::string tmp2 = tmp_file ("test2.png");
  {
    tl::OutputStream stream (tmp2);
    img2.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp2;

  EXPECT_EQ (compare_images (img, img2), true);

#if defined (HAVE_QT)
  //  Qt cross-check
  std::string au = tl::testsrc () + "/testdata/lay/au_gs.png";
  EXPECT_EQ (compare_images (img2.to_image (), au), true);
#endif
}

#endif

TEST(7)
{
  {
    tl::SelfTimer timer ("Run time - tl::Image copy, no write (should be very fast)");

    tl::PixelBuffer img (1000, 1000);
    img.fill (0x112233);

    for (unsigned int i = 0; i < 5000; ++i) {
      tl::PixelBuffer img2 (img);
    }
  }

#if defined(HAVE_QT)
  {
    tl::SelfTimer timer ("Run time - QImage copy, no write (should be very fast)");

    tl::PixelBuffer img (1000, 1000);
    img.fill (0x112233);
    QImage qimg (img.to_image ());

    for (unsigned int i = 0; i < 5000; ++i) {
      QImage qimg2 (qimg);
    }
  }
#endif

  {
    tl::SelfTimer timer ("Run time - tl::Image copy on write");

    tl::PixelBuffer img (1000, 1000);
    img.fill (0x112233);

    for (unsigned int i = 0; i < 5000; ++i) {
      tl::PixelBuffer img2 (img);
      img2.scan_line (100) [7] = 0;
    }
  }

#if defined(HAVE_QT)
  {
    tl::SelfTimer timer ("Run time - QImage copy on write (should not be much less than tl::Image copy on write)");

    tl::PixelBuffer img (1000, 1000);
    img.fill (0x112233);
    QImage qimg (img.to_image ());

    for (unsigned int i = 0; i < 5000; ++i) {
      QImage qimg2 (qimg);
      qimg2.scanLine (100) [7] = 0;
    }
  }

  {
    tl::SelfTimer timer ("Run time - direct QImage paint");

    tl::PixelBuffer img (1000, 1000);
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
    tl::SelfTimer timer ("Run time - tl::Image paint (should not be much more than direct QImage paint)");

    tl::PixelBuffer img (1000, 1000);
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

//  Monochrome version

TEST(11)
{
  tl::BitmapBuffer img (15, 25);
  EXPECT_EQ (img.width (), 15u);
  EXPECT_EQ (img.height (), 25u);
  EXPECT_EQ (img.stride (), 4u);

  img.fill (true);
  EXPECT_EQ (img.scan_line (5)[1], 0xff);

  tl::BitmapBuffer img2;
  img2 = img;
  EXPECT_EQ (img2.width (), 15u);
  EXPECT_EQ (img2.height (), 25u);

  EXPECT_EQ (img.scan_line (5)[1], 0xff);
  EXPECT_EQ (img2.scan_line (5)[1], 0xff);

  img2.fill (false);
  EXPECT_EQ (img.scan_line (5)[1], 0xff);
  EXPECT_EQ (img2.scan_line (5)[1], 0);

  img2.swap (img);
  EXPECT_EQ (img2.scan_line (5)[1], 0xff);
  EXPECT_EQ (img.scan_line (5)[1], 0);

  img2 = img;
  EXPECT_EQ (compare_images (img, img2), true);
  EXPECT_EQ (img.scan_line (5)[1], 0);
  EXPECT_EQ (img2.scan_line (5)[1], 0);

  img2 = tl::BitmapBuffer (10, 16);
  EXPECT_EQ (img.width (), 15u);
  EXPECT_EQ (img.height (), 25u);
  EXPECT_EQ (img2.width (), 10u);
  EXPECT_EQ (img2.height (), 16u);
  img2.fill (true);
  EXPECT_EQ (compare_images (img, img2), false);

  EXPECT_EQ (img.scan_line (5)[1], 0);
  EXPECT_EQ (img2.scan_line (5)[0], 0xff);

  img = std::move (img2);
  EXPECT_EQ (compare_images (img, img2), false);
  EXPECT_EQ (img.width (), 10u);
  EXPECT_EQ (img.height (), 16u);
  EXPECT_EQ (img.scan_line (5)[0], 0xff);

  tl::BitmapBuffer img3 (img);
  EXPECT_EQ (compare_images (img, img3), true);
  EXPECT_EQ (img3.width (), 10u);
  EXPECT_EQ (img3.height (), 16u);
  EXPECT_EQ (img3.scan_line (5)[1], 0xff);

  img.fill (false);
  EXPECT_EQ (compare_images (img, img3), false);
  EXPECT_EQ (img3.width (), 10u);
  EXPECT_EQ (img3.height (), 16u);
  EXPECT_EQ (img3.scan_line (5)[1], 0xff);
  EXPECT_EQ (img.width (), 10u);
  EXPECT_EQ (img.height (), 16u);
  EXPECT_EQ (img.scan_line (5)[1], 0);

  tl::BitmapBuffer img4 (std::move (img));
  EXPECT_EQ (img4.width (), 10u);
  EXPECT_EQ (img4.height (), 16u);
  EXPECT_EQ (img4.scan_line (5)[1], 0);

  //  other constructors
  EXPECT_EQ (compare_images (tl::BitmapBuffer (img4.width (), img4.height (), (const uint8_t *) img4.data ()), img4), true);
  EXPECT_EQ (compare_images (tl::BitmapBuffer (img4.width (), img4.height (), (const uint8_t *) img4.data (), img4.stride ()), img4), true);

  uint8_t *dnew = new uint8_t [ img4.width () * img4.height () * sizeof (uint8_t) ];
  memcpy (dnew, (const uint8_t *) img4.data (), img4.stride () * img4.height ());
  EXPECT_EQ (compare_images (tl::BitmapBuffer (img4.width (), img4.height (), dnew), img4), true);
}

#if defined(HAVE_QT)

TEST(12)
{
  tl::BitmapBuffer img (227, 231);

  for (unsigned int i = 0; i < img.stride (); ++i) {
    for (unsigned int j = 0; j < img.height (); ++j) {
      img.scan_line (j) [i] = uint8_t (i * j);
    }
  }

  EXPECT_EQ (img.to_image ().format () == QImage::Format_MonoLSB, true);

  std::string tmp = tmp_file ("test.png");
  QImage qimg = img.to_image ();
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_mono.png";
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images_mono (qimg, au), true);

  tl::BitmapBuffer img_returned = tl::BitmapBuffer::from_image (qimg);
  EXPECT_EQ (compare_images (img, img_returned), true);

  qimg = img.to_image_copy ();
  img.fill (false);

  tmp = tmp_file ("test2.png");
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  EXPECT_EQ (compare_images_mono (qimg, au), true);
}

#endif

#if defined(HAVE_PNG)

//  libpng support
TEST(13)
{
  tl::BitmapBuffer img;

  std::string in = tl::testsrc () + "/testdata/lay/au_mono.png";
  tl::info << "PNG file read (libpng) from " << in;

  {
    tl::InputStream stream (in);
    img = tl::BitmapBuffer::read_png (stream);
  }

  std::string tmp = tmp_file ("test.png");
  {
    tl::OutputStream stream (tmp);
    img.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp;

  tl::BitmapBuffer img2;

  {
    tl::InputStream stream (tmp);
    img2 = tl::BitmapBuffer::read_png (stream);
  }

  std::string tmp2 = tmp_file ("test2.png");
  {
    tl::OutputStream stream (tmp2);
    img2.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp2;

  EXPECT_EQ (compare_images (img, img2), true);

#if defined (HAVE_QT)
  //  Qt cross-check
  std::string au = tl::testsrc () + "/testdata/lay/au_mono.png";
  EXPECT_EQ (compare_images_mono (img2.to_image (), au), true);
#endif
}

#endif
