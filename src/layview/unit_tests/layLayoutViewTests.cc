
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

#include "layLayoutView.h"

#include "tlUnitTest.h"
#include "tlTimer.h"
#include "tlSleep.h"

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

static bool compare_images_mono (const QImage &qimg, const std::string &au)
{
  QImage qimg2;
  qimg2.load (tl::to_qstring (au));

  if (qimg2.width () == (int) qimg.width () && qimg2.height () == (int) qimg.height ()) {
    //  NOTE: slooooow ...
    for (int j = 0; j < qimg.height (); ++j) {
      for (int i = 0; i < qimg.width (); ++i) {
        if ((qimg.scanLine (j)[i / 8] & (0x80 >> (i % 8))) != (qimg2.scanLine (j)[i / 8] & (0x80 >> (i % 8)))) {
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

static bool compare_images (const lay::PixelBuffer &img, const lay::PixelBuffer &img2)
{
  return img == img2;
}

static bool compare_images (const lay::BitmapBuffer &img, const lay::BitmapBuffer &img2)
{
  return img == img2;
}

#if defined(HAVE_QT)
TEST(1)
{
  lay::LayoutView lv (0, false, 0);
  lv.cell_box_color (lay::Color (0, 0, 0));

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  QImage qimg;
  qimg = lv.get_image_with_options (500, 500, 1, 1, 1.0, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox (), false);

  EXPECT_EQ (qimg.format () == QImage::Format_RGB32, true);

  std::string tmp = tmp_file ("test.png");
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv1.png";
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (qimg, au), true);
}

TEST(2)
{
  lay::LayoutView lv (0, false, 0);
  lv.full_hier_new_cell (true);

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  QImage qimg;
  qimg = lv.get_image_with_options (500, 500, 1, 1, 1.0, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox (), false);

  EXPECT_EQ (qimg.format () == QImage::Format_RGB32, true);

  std::string tmp = tmp_file ("test.png");
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv2.png";
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (qimg, au), true);
}

//  monochrome
TEST(3)
{
  lay::LayoutView lv (0, false, 0);
  lv.full_hier_new_cell (true);

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  QImage qimg;
  qimg = lv.get_image_with_options (500, 500, 1, 1, 1.0, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox (), true);

  EXPECT_EQ (qimg.format () == QImage::Format_MonoLSB, true);

  std::string tmp = tmp_file ("test.png");
  qimg.save (tl::to_qstring (tmp));
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv3.png";
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images_mono (qimg.convertToFormat (QImage::Format_Mono), au), true);
}
#endif

TEST(4)
{
  lay::LayoutView lv (0, false, 0);
  lv.set_drawing_workers (2);
  lv.cell_box_color (lay::Color (0, 0, 0));

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  lv.resize (42, 117);
  tl::msleep (250);

  lay::PixelBuffer img = lv.get_screenshot_pb ();
  EXPECT_EQ ((int) img.width (), 42);
  EXPECT_EQ ((int) img.height (), 117);

  lv.resize (142, 217);

  img = lv.get_screenshot_pb ();
  EXPECT_EQ ((int) img.width (), 142);
  EXPECT_EQ ((int) img.height (), 217);
}

#if defined(HAVE_PNG)
TEST(11)
{
  lay::LayoutView lv (0, false, 0);
  lv.cell_box_color (lay::Color (0, 0, 0));

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  lay::PixelBuffer img;
  img = lv.get_pixels_with_options (500, 500, 1, 1, 1.0, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox ());

  std::string tmp = tmp_file ("test.png");
  {
    tl::OutputStream stream (tmp);
    img.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv1.png";
  lay::PixelBuffer au_img;
  {
    tl::InputStream stream (au);
    au_img = lay::PixelBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (img, au_img), true);
}

TEST(12)
{
  lay::LayoutView lv (0, false, 0);
  lv.full_hier_new_cell (true);

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  lay::PixelBuffer img;
  img = lv.get_pixels_with_options (500, 500, 1, 1, 1.0, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox ());

  std::string tmp = tmp_file ("test.png");
  {
    tl::OutputStream stream (tmp);
    img.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv2.png";
  lay::PixelBuffer au_img;
  {
    tl::InputStream stream (au);
    au_img = lay::PixelBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (img, au_img), true);
}

//  monochrome
TEST(13)
{
  lay::LayoutView lv (0, false, 0);
  lv.full_hier_new_cell (true);

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  lay::BitmapBuffer img;
  img = lv.get_pixels_with_options_mono (500, 500, 1, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox ());

  std::string tmp = tmp_file ("test.png");
  {
    tl::OutputStream stream (tmp);
    img.write_png (stream);
  }
  tl::info << "PNG file written to " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv3.png";
  lay::BitmapBuffer au_img;
  {
    tl::InputStream stream (au);
    au_img = lay::BitmapBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (img, au_img), true);
}
#endif

#if defined(HAVE_PNG) && defined(HAVE_QT)
TEST(21)
{
  lay::LayoutView lv (0, false, 0);
  lv.cell_box_color (lay::Color (0, 0, 0));

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  std::string tmp = tmp_file ("test.png");
  lv.save_image_with_options (tmp, 500, 500, 1, 1, 1.0, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox (), false);

  lay::PixelBuffer img;
  {
    tl::InputStream stream (tmp);
    img = lay::PixelBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv1.png";
  lay::PixelBuffer au_img;
  {
    tl::InputStream stream (au);
    au_img = lay::PixelBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (img, au_img), true);
}

TEST(22)
{
  lay::LayoutView lv (0, false, 0);
  lv.full_hier_new_cell (true);

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  std::string tmp = tmp_file ("test.png");
  lv.save_image_with_options (tmp, 500, 500, 1, 1, 1.0, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox (), false);

  lay::PixelBuffer img;
  {
    tl::InputStream stream (tmp);
    img = lay::PixelBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv2.png";
  lay::PixelBuffer au_img;
  {
    tl::InputStream stream (au);
    au_img = lay::PixelBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (img, au_img), true);
}

//  monochrome
TEST(23)
{
  lay::LayoutView lv (0, false, 0);
  lv.full_hier_new_cell (true);

  lv.load_layout (tl::testsrc () + "/testdata/gds/t10.gds", true);

  std::string tmp = tmp_file ("test.png");
  lv.save_image_with_options (tmp, 500, 500, 1, 1, 1.0, lay::Color (255, 255, 255), lay::Color (0, 0, 0), lay::Color (128, 128, 128), db::DBox (), true);

  lay::BitmapBuffer img;
  {
    tl::InputStream stream (tmp);
    img = lay::BitmapBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << tmp;

  std::string au = tl::testsrc () + "/testdata/lay/au_lv3.png";
  lay::BitmapBuffer au_img;
  {
    tl::InputStream stream (au);
    au_img = lay::BitmapBuffer::read_png (stream);
  }
  tl::info << "PNG file read from " << au;

  EXPECT_EQ (compare_images (img, au_img), true);
}
#endif