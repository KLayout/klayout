
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "imgStream.h"
#include "tlUnitTest.h"

#include <memory>

TEST(1_FloatMono)
{
  img::Object image (12, 8, db::DCplxTrans (1.5, 90.0, true, db::DVector (17, -42)), false, false);

  image.set_min_value (-0.25);
  image.set_max_value (0.75);

  std::vector<db::DPoint> lm;
  lm.push_back (db::DPoint (1, 2));
  lm.push_back (db::DPoint (-101, 102));
  image.set_landmarks (lm);

  img::DataMapping dm;
  dm.blue_gain = 0.5;
  dm.green_gain = 0.75;
  dm.red_gain = 0.25;
  dm.contrast = -0.5;
  dm.gamma = 1.5;
  dm.brightness = 1.25;
  dm.false_color_nodes.clear ();
  dm.false_color_nodes.push_back (std::make_pair (0.0, std::make_pair (tl::Color (0, 0, 0), tl::Color (0, 0, 0))));
  dm.false_color_nodes.push_back (std::make_pair (0.5, std::make_pair (tl::Color (255, 0, 0), tl::Color (0, 255, 0))));
  dm.false_color_nodes.push_back (std::make_pair (1.0, std::make_pair (tl::Color (255, 255, 255), tl::Color (255, 255, 255))));
  image.set_data_mapping (dm);

  image.set_pixel (0, 0, 0.25);
  image.set_pixel (2, 5, 0.25);
  image.set_pixel (7, 1, 0.125);

  std::string path = tmp_file ("tmp.lyimg");
  {
    tl::OutputFile file (path);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, image);
  }

  std::unique_ptr<img::Object> read;

  {
    tl::InputFile file (path);
    tl::InputStream stream (file);
    read.reset (img::ImageStreamer::read (stream));
  }

  EXPECT_EQ (image.to_string (), read->to_string ());
}

TEST(2_FloatMonoWithMask)
{
  img::Object image (12, 8, db::DCplxTrans (1.5, 90.0, true, db::DVector (17, -42)), false, false);

  image.set_min_value (-0.25);
  image.set_max_value (0.75);

  image.set_pixel (0, 0, 0.25);
  image.set_pixel (2, 5, 0.25);
  image.set_pixel (7, 1, 0.125);

  image.set_mask (1, 0, 1);
  image.set_mask (1, 2, 1);
  image.set_mask (1, 3, 0);

  std::string path = tmp_file ("tmp.lyimg");
  {
    tl::OutputFile file (path);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, image);
  }

  std::unique_ptr<img::Object> read;

  {
    tl::InputFile file (path);
    tl::InputStream stream (file);
    read.reset (img::ImageStreamer::read (stream));
  }

  EXPECT_EQ (image.to_string (), read->to_string ());
}

TEST(3_ByteMono)
{
  img::Object image (12, 8, db::Matrix3d (db::DCplxTrans (1.5, 90.0, true, db::DVector (17, -42))), false, true);

  image.set_min_value (10);
  image.set_max_value (240);

  image.set_pixel (0, 0, 50);
  image.set_pixel (2, 5, 70);
  image.set_pixel (7, 1, 120);

  std::string path = tmp_file ("tmp.lyimg");
  {
    tl::OutputFile file (path);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, image);
  }

  std::unique_ptr<img::Object> read;

  {
    tl::InputFile file (path);
    tl::InputStream stream (file);
    read.reset (img::ImageStreamer::read (stream));
  }

  EXPECT_EQ (image.to_string (), read->to_string ());
}

TEST(4_ByteMonoWithMask)
{
  img::Object image (12, 8, db::DCplxTrans (1.5, 90.0, true, db::DVector (17, -42)), false, true);

  image.set_min_value (10);
  image.set_max_value (240);

  image.set_pixel (0, 0, 50);
  image.set_pixel (2, 5, 70);
  image.set_pixel (7, 1, 120);

  image.set_mask (1, 0, 1);
  image.set_mask (1, 2, 1);
  image.set_mask (1, 3, 0);

  std::string path = tmp_file ("tmp.lyimg");
  {
    tl::OutputFile file (path);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, image);
  }

  std::unique_ptr<img::Object> read;

  {
    tl::InputFile file (path);
    tl::InputStream stream (file);
    read.reset (img::ImageStreamer::read (stream));
  }

  EXPECT_EQ (image.to_string (), read->to_string ());
}

TEST(5_FloatColor)
{
  img::Object image (12, 8, db::DCplxTrans (1.5, 90.0, true, db::DVector (17, -42)), true, false);

  image.set_min_value (-0.25);
  image.set_max_value (0.75);

  image.set_pixel (0, 0, 0.25, -0.25, -0.125);
  image.set_pixel (2, 5, 0.25, 0.125, 0.625);
  image.set_pixel (7, 1, 0.125, 0.25, 0.75);

  std::string path = tmp_file ("tmp.lyimg");
  {
    tl::OutputFile file (path);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, image);
  }

  std::unique_ptr<img::Object> read;

  {
    tl::InputFile file (path);
    tl::InputStream stream (file);
    read.reset (img::ImageStreamer::read (stream));
  }

  EXPECT_EQ (image.to_string (), read->to_string ());
}

TEST(6_FloatColorWithMask)
{
  img::Object image (12, 8, db::DCplxTrans (1.5, 90.0, true, db::DVector (17, -42)), true, false);

  image.set_min_value (-0.25);
  image.set_max_value (0.75);

  image.set_pixel (0, 0, 0.25, -0.25, -0.125);
  image.set_pixel (2, 5, 0.25, 0.125, 0.625);
  image.set_pixel (7, 1, 0.125, 0.25, 0.75);

  image.set_mask (1, 0, 1);
  image.set_mask (1, 2, 1);
  image.set_mask (1, 3, 0);

  std::string path = tmp_file ("tmp.lyimg");
  {
    tl::OutputFile file (path);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, image);
  }

  std::unique_ptr<img::Object> read;

  {
    tl::InputFile file (path);
    tl::InputStream stream (file);
    read.reset (img::ImageStreamer::read (stream));
  }

  EXPECT_EQ (image.to_string (), read->to_string ());
}

TEST(7_ByteColor)
{
  img::Object image (12, 8, db::DCplxTrans (1.5, 90.0, true, db::DVector (17, -42)), true, true);

  image.set_min_value (10);
  image.set_max_value (240);

  image.set_pixel (0, 0, 10, 20, 30);
  image.set_pixel (2, 5, 11, 21, 31);
  image.set_pixel (7, 1, 12, 22, 32);

  std::string path = tmp_file ("tmp.lyimg");
  {
    tl::OutputFile file (path);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, image);
  }

  std::unique_ptr<img::Object> read;

  {
    tl::InputFile file (path);
    tl::InputStream stream (file);
    read.reset (img::ImageStreamer::read (stream));
  }

  EXPECT_EQ (image.to_string (), read->to_string ());
}

TEST(8_ByteColorWithMask)
{
  img::Object image (12, 8, db::DCplxTrans (1.5, 90.0, true, db::DVector (17, -42)), true, true);

  image.set_min_value (10);
  image.set_max_value (240);

  image.set_pixel (0, 0, 10, 20, 30);
  image.set_pixel (2, 5, 11, 21, 31);
  image.set_pixel (7, 1, 12, 22, 32);

  image.set_mask (1, 0, 1);
  image.set_mask (1, 2, 1);
  image.set_mask (1, 3, 0);

  std::string path = tmp_file ("tmp.lyimg");
  {
    tl::OutputFile file (path);
    tl::OutputStream stream (file);
    img::ImageStreamer::write (stream, image);
  }

  std::unique_ptr<img::Object> read;

  {
    tl::InputFile file (path);
    tl::InputStream stream (file);
    read.reset (img::ImageStreamer::read (stream));
  }

  EXPECT_EQ (image.to_string (), read->to_string ());
}
