
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



#include "layRenderer.h"
#include "layBitmapRenderer.h"
#include "layBitmap.h"
#include "tlUnitTest.h"

static std::string 
to_string (const lay::Bitmap &bm)
{
  std::string r;

  for (unsigned int j = bm.height (); j > 0; --j) {
    std::string s;
    for (unsigned int k = 0; k < bm.width (); ++k) {
      const char *t = (bm.scanline (j - 1)[k / 32] & (1 << (k % 32))) != 0 ? "#" : "-";
      s += t;
    }
    r += s;
    r += "\n";
  }

  return r;
}

static std::string 
to_string (const lay::Bitmap &bm, const lay::Bitmap &bf)
{
  std::string r;

  for (unsigned int j = bm.height (); j > 0; --j) {
    std::string s;
    for (unsigned int k = 0; k < bm.width (); ++k) {
      const char *t = (bm.scanline (j - 1)[k / 32] & (1 << (k % 32))) != 0 ? "#" : "-";
      if ((bf.scanline (j - 1)[k / 32] & (1 << (k % 32))) != 0) {
        t = "*";
      }
      s += t;
    }
    r += s;
    r += "\n";
  }

  return r;
}

TEST(1) 
{
  lay::Bitmap b1 (16, 16, 1.0);
   
  lay::BitmapRenderer r (16, 16, 1.0);
  r.insert (db::DEdge (3.4, 2.1, 12.7, -2.1));
  r.insert (db::DEdge (12.7, -2.1, 3.4, 2.1));
  r.insert (db::DEdge (3.4, 2.1, 12.7, 2.1));
  r.insert (db::DEdge (12.7, 2.1, 3.4, 2.1));
  r.insert (db::DEdge (3.4, 2.1, -12.7, 2.1));
  r.insert (db::DEdge (-12.7, 2.1, 3.4, 2.1));
  r.insert (db::DEdge (3.4, 2.1, 12.7, 12.1));
  r.insert (db::DEdge (12.7, 12.1, 3.4, 2.1));
  r.render_vertices (b1, 0);

  EXPECT_EQ (to_string (b1), "----------------\n"
                             "----------------\n"
                             "----------------\n"
                             "-------------#--\n"
                             "----------------\n"
                             "----------------\n"
                             "----------------\n"
                             "----------------\n"
                             "----------------\n"
                             "----------------\n"
                             "----------------\n"
                             "----------------\n"
                             "----------------\n"
                             "---#---------#--\n"
                             "----------------\n"
                             "----------------\n");

}


TEST(2) 
{
  lay::Bitmap b1 (16, 16, 1.0);

  lay::BitmapRenderer r(16, 16, 1.0);
  r.clear ();
  r.insert (db::DEdge (3.4, 2.1, 12.7, 12.1));
  r.insert (db::DEdge (3.4, 0.1, 100.0, 22.5));
  r.insert (db::DEdge (3.4, 5.1, 12.7, 5.1));
  r.insert (db::DEdge (-3.4, 5.1, 25.7, 30.0));
  r.render_contour (b1);

  EXPECT_EQ (to_string (b1), "--------#-------\n"
                             "-------#--------\n"
                             "------#---------\n"
                             "-----#-------#--\n"
                             "---##-------#---\n"
                             "--#--------#----\n"
                             "-#--------#-----\n"
                             "#--------#------\n"
                             "--------#-------\n"
                             "-------#--------\n"
                             "---###########--\n"
                             "-----#----------\n"
                             "----#---------##\n"
                             "---#------####--\n"
                             "------####------\n"
                             "---###----------\n");

  r.clear ();
  r.insert (db::DEdge (12.7, 2.1, 3.4, 12.1));
  r.insert (db::DEdge (100.0, 0.1, 3.4, 14.5));
  r.insert (db::DEdge (12.7, 5.1, 3.4, 5.1));
  r.insert (db::DEdge (15.3, -5.1, -5.1, 5.0));
  b1 = lay::Bitmap (16, 16, 1.0);
  r.render_contour (b1);

  EXPECT_EQ (to_string (b1), "---#------------\n"
                             "----#######-----\n"
                             "-----------#####\n"
                             "---#------------\n"
                             "----#-----------\n"
                             "-----#----------\n"
                             "------#---------\n"
                             "-------#--------\n"
                             "--------#-------\n"
                             "---------#------\n"
                             "---###########--\n"
                             "-----------#----\n"
                             "------------#---\n"
                             "##-----------#--\n"
                             "--##------------\n"
                             "----###---------\n");

}

TEST(3) 
{
  lay::Bitmap b1 (16, 16, 1.0);
  lay::Bitmap b2 (16, 16, 1.0);
   
  lay::BitmapRenderer r(16, 16, 1.0);
  r.insert (db::DEdge (3.4, 2.1, 12.7, 14.5));
  r.insert (db::DEdge (12.7, 14.5, 10.7, 0.6));
  r.insert (db::DEdge (10.7, 0.6, 3.4, 2.1));
  r.render_fill (b1);
  r.render_contour (b2);

  EXPECT_EQ (to_string (b1, b2), "-------------*--\n"
                                 "------------*---\n"
                                 "-----------**---\n"
                                 "-----------**---\n"
                                 "----------*#*---\n"
                                 "---------*##*---\n"
                                 "--------*##*----\n"
                                 "--------*##*----\n"
                                 "-------*###*----\n"
                                 "------*####*----\n"
                                 "-----*#####*----\n"
                                 "-----*#####*----\n"
                                 "----*######*----\n"
                                 "---****####*----\n"
                                 "-------*****----\n"
                                 "----------------\n");

  r.clear ();
  r.insert (db::DEdge (3.1, 9.0, 12.7, 14.5));
  r.insert (db::DEdge (12.7, 14.5, 10.7, 0.6));
  r.insert (db::DEdge (10.7, 0.6, 3.1, 9.0));
  b1 = lay::Bitmap (16, 16, 1.0);
  b2 = lay::Bitmap (16, 16, 1.0);
  r.render_fill (b1);
  r.render_contour (b2);

  EXPECT_EQ (to_string (b1, b2), "-------------*--\n"
                                 "-----------**---\n"
                                 "----------*#*---\n"
                                 "--------**##*---\n"
                                 "------**####*---\n"
                                 "----**######*---\n"
                                 "---*#######*----\n"
                                 "----*######*----\n"
                                 "-----*#####*----\n"
                                 "------*####*----\n"
                                 "-------*###*----\n"
                                 "--------*##*----\n"
                                 "---------*#*----\n"
                                 "---------*#*----\n"
                                 "----------**----\n"
                                 "----------------\n");

  r.clear ();
  r.insert (db::DEdge (3.0, 9.0, 3.0, 14.0));
  r.insert (db::DEdge (3.0, 14.0, 12.0, 14.0));
  r.insert (db::DEdge (12.0, 14.0, 12.0, 9.0));
  r.insert (db::DEdge (12.0, 9.0, 3.0, 9.0));
  b1 = lay::Bitmap (16, 16, 1.0);
  b2 = lay::Bitmap (16, 16, 1.0);
  r.render_fill (b1);
  // r.render_contour (b2);

  EXPECT_EQ (to_string (b1, b2), "----------------\n"
                                 "---##########---\n"
                                 "---##########---\n"
                                 "---##########---\n"
                                 "---##########---\n"
                                 "---##########---\n"
                                 "----------------\n"
                                 "----------------\n"
                                 "----------------\n"
                                 "----------------\n"
                                 "----------------\n"
                                 "----------------\n"
                                 "----------------\n"
                                 "----------------\n"
                                 "----------------\n"
                                 "----------------\n");

  r.clear ();
  r.insert (db::DEdge (0.2, 9.6, 2.2, 3.8));
  r.insert (db::DEdge (2.2, 3.8, 10.7, 6.6));
  r.insert (db::DEdge (10.7, 6.6, 7.0, 11.4));
  r.insert (db::DEdge (7.0, 11.4, 14.2, 12.5));
  r.insert (db::DEdge (14.2, 12.5, 12.3, 4.9));
  r.insert (db::DEdge (12.3, 4.9, 5.9, 11.1));
  r.insert (db::DEdge (5.9, 11.1, 8.8, 6.0));
  r.insert (db::DEdge (8.8, 6.0, 4.2, 2.9));
  r.insert (db::DEdge (4.2, 2.9, 12.2, 0.4));
  r.insert (db::DEdge (12.2, 0.4, 0.2, 9.6));
  b1 = lay::Bitmap (16, 16, 1.0);
  b2 = lay::Bitmap (16, 16, 1.0);
  r.render_fill (b1);
  r.render_contour (b2);

  EXPECT_EQ (to_string (b1, b2), "----------------\n"
                                 "----------------\n"
                                 "--------------*-\n"
                                 "--------*******-\n"
                                 "------**#####*--\n"
                                 "*------**####*--\n"
                                 "-*-----***###*--\n"
                                 "-**-----***##*--\n"
                                 "-*#**---*#***---\n"
                                 "--*##*--*****---\n"
                                 "--*##****---*---\n"
                                 "--***-***-------\n"
                                 "----**###*------\n"
                                 "------***#*-----\n"
                                 "---------***----\n"
                                 "------------*---\n");
}


