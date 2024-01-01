
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



#include "imgObject.h"
#include "tlUnitTest.h"

static img::Object from_s (const std::string &s)
{
  img::Object img;
  img.from_string (s.c_str ());
  return img;
}

TEST(1) 
{
  img::Object image (12, 8, db::DCplxTrans (), false, false);

  EXPECT_EQ (image.is_color (), false);
  EXPECT_EQ (image.is_byte_data (), false);

  EXPECT_EQ (image.float_data ()[0], 0.0);
  EXPECT_EQ (image.float_data ()[1], 0.0);
  EXPECT_EQ (image.float_data ()[12*8-1], 0.0);
  EXPECT_EQ (image.data_length(), size_t (12 * 8));
  
  EXPECT_EQ (db::DCplxTrans (image.matrix ()).to_string (), "r0 *1 0,0");

  img::Object copy1 (image);
  EXPECT_EQ (copy1.equals (&image), true);

  image.set_matrix (db::Matrix3d (db::DCplxTrans (2.5, 90, false, db::DVector (1, -2))));
  EXPECT_EQ (db::DCplxTrans (image.matrix ()).to_string (), "r90 *2.5 1,-2");
  EXPECT_EQ (copy1.equals (&image), false);

  copy1 = image;
  EXPECT_EQ (copy1.equals (&image), true);
  EXPECT_EQ (copy1.float_data () == image.float_data (), true);

  std::vector<double> d;
  for (unsigned int i = 0; i < image.data_length (); ++i) {
    d.push_back (image.float_data ()[i]);
  }
  copy1.set_data (12, 8, d);
  EXPECT_EQ (copy1.equals (&image), true);
  EXPECT_EQ (copy1.float_data () == image.float_data (), false);
  EXPECT_EQ (copy1.width (), size_t (12));
  EXPECT_EQ (copy1.height (), size_t (8));

  d[0] = 12.5;
  d[5] = -12.5;
  copy1.set_data (12, 8, d);
  EXPECT_EQ (copy1.float_data () == image.float_data (), false);
  EXPECT_EQ (copy1.float_data ()[0], 12.5);
  EXPECT_EQ (copy1.float_data ()[1], 0.0);
  EXPECT_EQ (copy1.float_data ()[5], -12.5);
  EXPECT_EQ (copy1.float_data ()[6], 0.0);
  EXPECT_EQ (image.float_data ()[0], 0.0);
  EXPECT_EQ (image.float_data ()[1], 0.0);
  EXPECT_EQ (image.float_data ()[5], 0.0);
  EXPECT_EQ (image.float_data ()[6], 0.0);
  EXPECT_EQ (copy1.equals (&image), false);

  image.set_matrix (db::Matrix3d (db::DCplxTrans (2.5, 90, false, db::DVector (1, -2))) * db::Matrix3d::mag (2.5, 1.0));
  EXPECT_EQ (image.matrix ().mag_x (), 2.5 * 2.5);
  image.set_matrix (db::Matrix3d (db::DCplxTrans (2.5, 90, false, db::DVector (1, -2))) * db::Matrix3d::mag (2.5, 1.25));
  EXPECT_EQ (image.matrix ().mag_y (), 1.25 * 2.5);

  image.set_min_value (0.5);
  EXPECT_EQ (image.min_value (), 0.5);
  image.set_max_value (25.0);
  EXPECT_EQ (image.max_value (), 25);

  copy1 = image;
  EXPECT_EQ (copy1.equals (&image), true);

  img::DataMapping dm (image.data_mapping ());
  dm.brightness = 0.5;
  dm.contrast = 0.25;
  dm.gamma = 1.5;
  dm.red_gain = 1.25;
  dm.green_gain = 0.75;
  dm.blue_gain = 2.5;
  tl::Color c (128, 255, 64);
  tl::Color c2 (64, 32, 192);
  dm.false_color_nodes.insert (dm.false_color_nodes.begin () + 1, std::make_pair (0.5, std::make_pair (c, c)));
  image.set_data_mapping (dm);
  EXPECT_EQ (copy1.equals (&image), false);
  EXPECT_EQ (from_s (image.to_string ()).equals (&image), true);
  copy1 = image;
  EXPECT_EQ (copy1.equals (&image), true);

  dm.false_color_nodes.insert (dm.false_color_nodes.begin () + 1, std::make_pair (0.75, std::make_pair (c, c2)));
  image.set_data_mapping (dm);
  EXPECT_EQ (copy1.equals (&image), false);
  EXPECT_EQ (from_s (image.to_string ()).equals (&image), true);
  copy1 = image;
  EXPECT_EQ (copy1.equals (&image), true);

  EXPECT_EQ (copy1.data_mapping ().brightness, 0.5);
  EXPECT_EQ (copy1.data_mapping ().red_gain, 1.25);
  EXPECT_EQ (copy1.data_mapping ().false_color_nodes.size (), size_t (4));

  img::Object copy2;
  copy2.from_string (image.to_string ().c_str ());
  EXPECT_EQ (copy2.equals (&image), true);

  EXPECT_EQ (copy2.data_mapping ().brightness, 0.5);
  EXPECT_EQ (tl::to_string (copy2.data_mapping ().red_gain), "1.25");
  EXPECT_EQ (copy2.data_mapping ().false_color_nodes.size (), size_t (4));
  EXPECT_EQ (copy2.equals (&image), true);

  img::Object copy3, empty;
  copy3.swap (copy2);
  EXPECT_EQ (copy3.equals (&image), true);
  EXPECT_EQ (copy2.equals (&empty), true);
  copy3.swap (copy2);
  EXPECT_EQ (copy2.equals (&image), true);
  EXPECT_EQ (copy3.equals (&empty), true);

  EXPECT_EQ (image.to_string (), copy2.to_string ());

  EXPECT_EQ (image.mask (1, 2), true);
  image.set_mask (1, 2, false);
  EXPECT_EQ (image.mask (1, 2), false);
  image.set_mask (1, 2, true);
  EXPECT_EQ (image.mask (1, 2), true);
  image.set_mask (1, 2, false);
  copy2.from_string (image.to_string ().c_str ());
  EXPECT_EQ (image.mask (1, 2), false);
  EXPECT_EQ (copy2.mask (1, 2), false);
  EXPECT_EQ (image.to_string (), copy2.to_string ());
  image.set_mask (1, 2, true);
  image.from_string (copy2.to_string ().c_str ());
  EXPECT_EQ (image.mask (1, 2), false);
}

TEST(2) 
{
  for (unsigned int channel = 0; channel < 3; ++channel) {
    img::Object image (12, 8, db::DCplxTrans (), true, false);

    EXPECT_EQ (image.is_color (), true);

    EXPECT_EQ (image.float_data (channel)[0], 0.0);
    EXPECT_EQ (image.float_data (channel)[1], 0.0);
    EXPECT_EQ (image.float_data (channel)[12*8-1], 0.0);
    EXPECT_EQ (image.data_length(), size_t (12 * 8));
    
    EXPECT_EQ (db::DCplxTrans (image.matrix ()).to_string (), "r0 *1 0,0");

    img::Object copy1 (image);
    EXPECT_EQ (copy1.equals (&image), true);

    image.set_matrix (db::Matrix3d (db::DCplxTrans (2.5, 90, false, db::DVector (1, -2))));
    EXPECT_EQ (db::DCplxTrans (image.matrix ()).to_string (), "r90 *2.5 1,-2");
    EXPECT_EQ (copy1.equals (&image), false);

    copy1 = image;
    EXPECT_EQ (copy1.equals (&image), true);
    EXPECT_EQ (copy1.float_data (channel) == image.float_data (channel), true);

    std::vector<double> d[3];
    for (unsigned int j = 0; j < 3; ++j) {
      for (unsigned int i = 0; i < image.data_length (); ++i) {
        d[j].push_back (image.float_data (j)[i]);
      }
    }
    copy1.set_data (12, 8, d[0], d[1], d[2]);
    EXPECT_EQ (copy1.equals (&image), true);
    EXPECT_EQ (copy1.float_data (channel) == image.float_data (channel), false);
    EXPECT_EQ (copy1.width (), size_t (12));
    EXPECT_EQ (copy1.height (), size_t (8));

    d[channel][0] = 12.5;
    d[channel][5] = -12.5;
    copy1.set_data (12, 8, d[0], d[1], d[2]);
    EXPECT_EQ (copy1.float_data (channel) == image.float_data (channel), false);
    EXPECT_EQ (copy1.float_data (channel)[0], 12.5);
    EXPECT_EQ (copy1.float_data (channel)[1], 0.0);
    EXPECT_EQ (copy1.float_data (channel)[5], -12.5);
    EXPECT_EQ (copy1.float_data (channel)[6], 0.0);
    EXPECT_EQ (image.float_data (channel)[0], 0.0);
    EXPECT_EQ (image.float_data (channel)[1], 0.0);
    EXPECT_EQ (image.float_data (channel)[5], 0.0);
    EXPECT_EQ (image.float_data (channel)[6], 0.0);
    EXPECT_EQ (copy1.equals (&image), false);

    image.set_matrix (db::Matrix3d (db::DCplxTrans (2.5, 90, false, db::DVector (1, -2))) * db::Matrix3d::mag (2.5, 1.0));
    EXPECT_EQ (image.matrix ().mag_x (), 2.5 * 2.5);
    image.set_matrix (db::Matrix3d (db::DCplxTrans (2.5, 90, false, db::DVector (1, -2))) * db::Matrix3d::mag (2.5, 1.25));
    EXPECT_EQ (image.matrix ().mag_y (), 1.25 * 2.5);

    image.set_min_value (0.5);
    EXPECT_EQ (image.min_value (), 0.5);
    image.set_max_value (25.0);
    EXPECT_EQ (image.max_value (), 25);

    copy1 = image;
    EXPECT_EQ (copy1.equals (&image), true);

    img::DataMapping dm (image.data_mapping ());
    dm.brightness = 0.5;
    dm.contrast = 0.25;
    dm.gamma = 1.5;
    dm.red_gain = 1.25;
    dm.green_gain = 0.75;
    dm.blue_gain = 2.5;
    tl::Color c (128, 255, 64);
    dm.false_color_nodes.insert (dm.false_color_nodes.begin () + 1, std::make_pair (0.5, std::make_pair (c, c)));
    image.set_data_mapping (dm);
    EXPECT_EQ (copy1.equals (&image), false);

    copy1 = image;
    EXPECT_EQ (copy1.equals (&image), true);

    EXPECT_EQ (copy1.data_mapping ().brightness, 0.5);
    EXPECT_EQ (copy1.data_mapping ().red_gain, 1.25);
    EXPECT_EQ (copy1.data_mapping ().false_color_nodes.size (), size_t (3));

    img::Object copy2;
    copy2.from_string (image.to_string ().c_str ());

    EXPECT_EQ (copy2.data_mapping ().brightness, 0.5);
    EXPECT_EQ (tl::to_string (copy2.data_mapping ().red_gain), "1.25");
    EXPECT_EQ (copy2.data_mapping ().false_color_nodes.size (), size_t (3));
    EXPECT_EQ (copy2.equals (&image), true);

    EXPECT_EQ (image.to_string (), copy2.to_string ());

    EXPECT_EQ (image.mask (1, 2), true);
    image.set_mask (1, 2, false);
    EXPECT_EQ (image.mask (1, 2), false);
    image.set_mask (1, 2, true);
    EXPECT_EQ (image.mask (1, 2), true);
    image.set_mask (1, 2, false);
    copy2.from_string (image.to_string ().c_str ());
    EXPECT_EQ (image.mask (1, 2), false);
    EXPECT_EQ (copy2.mask (1, 2), false);
    EXPECT_EQ (image.to_string (), copy2.to_string ());
    image.set_mask (1, 2, true);
    image.from_string (copy2.to_string ().c_str ());
    EXPECT_EQ (image.mask (1, 2), false);
  }
}

TEST(3) 
{
  unsigned char d[] =  { 
    11, 12, 13, 14, 15, 16, 17, 18, 19, 11, 11, 11, 
    21, 22, 23, 24, 25, 26, 27, 28, 29, 21, 21, 21, 
    31, 32, 33, 34, 35, 36, 37, 38, 39, 31, 31, 31, 
    41, 42, 43, 44, 45, 46, 47, 48, 49, 41, 41, 41, 
    51, 52, 53, 54, 55, 56, 57, 58, 59, 51, 51, 51, 
    61, 62, 63, 64, 65, 66, 67, 68, 69, 61, 61, 61,
    71, 72, 73, 74, 75, 76, 77, 78, 79, 71, 71, 71, 
    81, 82, 83, 84, 85, 86, 87, 88, 89, 81, 81, 81, 
  };

  unsigned char *data = new unsigned char [12*8];
  memcpy (data, d, 12*8);
  img::Object image (12, 8, db::DCplxTrans (), data);

  EXPECT_EQ (image.is_color (), false);
  EXPECT_EQ (image.is_byte_data (), true);

  EXPECT_EQ ((int)image.byte_data ()[0], 11);
  EXPECT_EQ ((int)image.byte_data ()[1], 12);
  EXPECT_EQ ((int)image.byte_data ()[12*8-1], 81);
  EXPECT_EQ (image.data_length (), size_t (12 * 8));
  
  img::Object copy1 (image);
  EXPECT_EQ (copy1.equals (&image), true);

  image.set_matrix (db::Matrix3d (db::DCplxTrans (2.5, 90, false, db::DVector (1, -2))));
  EXPECT_EQ (db::DCplxTrans (image.matrix ()).to_string (), "r90 *2.5 1,-2");
  EXPECT_EQ (copy1.equals (&image), false);

  copy1 = image;
  EXPECT_EQ (copy1.equals (&image), true);
  EXPECT_EQ (copy1.is_byte_data (), true);
  EXPECT_EQ (copy1.byte_data () == image.byte_data (), true);

  unsigned char *data2 = new unsigned char[8*12];
  unsigned char *d2 = data2;
  for (unsigned int i = 0; i < image.data_length (); ++i) {
    *d2++ = image.byte_data ()[i];
  }
  copy1.set_data (12, 8, data2);
  EXPECT_EQ (copy1.is_byte_data (), true);
  EXPECT_EQ (copy1.equals (&image), true);
  EXPECT_EQ (copy1.byte_data () == image.byte_data (), false);
  EXPECT_EQ (copy1.width (), size_t (12));
  EXPECT_EQ (copy1.height (), size_t (8));

  EXPECT_EQ (image.mask (1, 2), true);
  image.set_mask (1, 2, false);
  EXPECT_EQ (image.mask (1, 2), false);
  image.set_mask (1, 2, true);
  EXPECT_EQ (image.mask (1, 2), true);
  image.set_mask (1, 2, false);
  copy1.from_string (image.to_string ().c_str ());
  EXPECT_EQ (image.mask (1, 2), false);
  EXPECT_EQ (copy1.mask (1, 2), false);
  EXPECT_EQ (image.to_string (), copy1.to_string ());
  image.set_mask (1, 2, true);
  image.from_string (copy1.to_string ().c_str ());
  EXPECT_EQ (image.mask (1, 2), false);
}

