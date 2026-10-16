
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




#include "layBitmap.h"
#include "layFixedFont.h"
#include "tlUnitTest.h"

static std::string 
to_string (const lay::Bitmap &bm)
{
  std::string r;

  for (unsigned int j = bm.height (); j > 0; --j) {
    std::string s;
    for (unsigned int k = 0; k < bm.width (); ++k) {
      s += (bm.scanline (j - 1)[k / 32] & (1 << (k % 32))) != 0 ? "#" : "-";
    }
    r += s;
    r += "\n";
  }

  return r;
}

TEST(1) 
{
  lay::Bitmap b1 (8, 8, 1.0, 1.0);
  EXPECT_EQ (to_string (b1), "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n");

  b1.fill (4, 3, 6);
  EXPECT_EQ (to_string (b1), "--------\n"
                             "--------\n"
                             "--------\n"
                             "---###--\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n");

  lay::Bitmap b2;
  b2 = b1;
  EXPECT_EQ (to_string (b1), to_string (b2));

  b1 = lay::Bitmap ();
  EXPECT_EQ (to_string (b1), "");

  b1 = b2;
  EXPECT_EQ (to_string (b1), to_string (b2));
}

TEST(2) 
{
  lay::Bitmap b1 (8, 8, 1.0, 1.0);
  EXPECT_EQ (to_string (b1), "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n");

  lay::Bitmap b2 (3, 2, 1.0, 1.0);
  b2.fill (0, 0, 3);
  b2.fill (1, 1, 3);
  EXPECT_EQ (to_string (b2), "-##\n###\n");

  b1.merge (&b2, 0, 0);
  EXPECT_EQ (to_string (b1), "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "-##-----\n"
                             "###-----\n");

  b1.merge (&b2, 1, 1);
  EXPECT_EQ (to_string (b1), "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--##----\n"
                             "-###----\n"
                             "###-----\n");

  b1.clear ();
  b1.merge (&b2, -1, -1);
  EXPECT_EQ (to_string (b1), "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "##------\n");

  b1.clear ();
  b1.merge (&b2, 6, 2);
  EXPECT_EQ (to_string (b1), "--------\n"
                             "--------\n"
                             "--------\n"
                             "--------\n"
                             "-------#\n"
                             "------##\n"
                             "--------\n"
                             "--------\n");

  b1 = lay::Bitmap (40, 8, 1.0, 1.0);
  EXPECT_EQ (to_string (b1), "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n");

  b2 = lay::Bitmap (40, 8, 1.0, 1.0);
  b2.fill (1, 4, 36);
  b2.fill (2, 5, 15);
  b2.fill (2, 34, 35);
  EXPECT_EQ (to_string (b2), "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "-----##########-------------------#-----\n"
                             "----################################----\n"
                             "----------------------------------------\n");

  b1.merge (&b2, -1, -1);
  EXPECT_EQ (to_string (b1), "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----##########-------------------#------\n"
                             "---################################-----\n");

  b1.clear ();
  b1.merge (&b2, -32, -1);
  EXPECT_EQ (to_string (b1), "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "--#-------------------------------------\n"
                             "####------------------------------------\n");

  b1.clear ();
  b1.merge (&b2, -33, -1);
  EXPECT_EQ (to_string (b1), "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "-#--------------------------------------\n"
                             "###-------------------------------------\n");

  b1.clear ();
  b1.merge (&b2, 1, -1);
  EXPECT_EQ (to_string (b1), "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "----------------------------------------\n"
                             "------##########-------------------#----\n"
                             "-----################################---\n");

}


//  Glyph rendering of random texts placed partly outside bitmaps of several widths, at two font
//  resolutions and all alignments; the hash covers every pixel inside the bitmaps
TEST(3)
{
  unsigned int s = 4711;
  const unsigned int widths [] = { 20, 45, 100, 333 };
  const double resolutions [] = { 1.0, 2.0 };
  const db::HAlign haligns [] = { db::HAlignLeft, db::HAlignCenter, db::HAlignRight };
  const db::VAlign valigns [] = { db::VAlignBottom, db::VAlignCenter, db::VAlignTop };

  uint64_t hash = 14695981039346656037ull;
  unsigned int set_pixels = 0;

  for (unsigned int ri = 0; ri < 2; ++ri) {
    for (unsigned int wi = 0; wi < sizeof (widths) / sizeof (widths [0]); ++wi) {

      const unsigned int w = widths [wi], h = 61;
      lay::Bitmap bitmap (w, h, 1.0, resolutions [ri]);

      for (unsigned int t = 0; t < 40; ++t) {

        lay::RenderText text;
        std::string str;
        s = s * 1103515245u + 12345u;
        unsigned int len = 1 + (s >> 8) % 9;
        for (unsigned int k = 0; k < len; ++k) {
          s = s * 1103515245u + 12345u;
          unsigned int c = (s >> 8) % 100;
          str += (c == 0 ? '\n' : char (' ' + c % 95));
        }
        text.text = str;
        text.font = db::DefaultFont;
        s = s * 1103515245u + 12345u;
        text.halign = haligns [(s >> 8) % 3];
        s = s * 1103515245u + 12345u;
        text.valign = valigns [(s >> 8) % 3];
        s = s * 1103515245u + 12345u;
        double x = double (int ((s >> 8) % (w + 160)) - 120) + 0.25;
        s = s * 1103515245u + 12345u;
        double y = double (int ((s >> 8) % (h + 60)) - 30) + 0.5;
        text.b = db::DBox (x, y, x, y);

        bitmap.render_text (text);

      }

      for (unsigned int py = 0; py < h; ++py) {
        const uint32_t *sl = bitmap.scanline (py);
        for (unsigned int px = 0; px < w; ++px) {
          unsigned int bit = (sl [px / 32] >> (px % 32)) & 1;
          set_pixels += bit;
          hash = (hash ^ (bit + 2 * px + 1000 * py)) * 1099511628211ull;
        }
      }

    }
  }

  EXPECT_EQ (tl::to_string (set_pixels) + " " + tl::to_string (hash), "5139 16480986320884342220");
}

TEST(4)
{
  for (unsigned int ri = 0; ri < 2; ++ri) {

    //  ff25 has two words per font row, ff1 has one
    const double fr = (ri == 0 ? 0.2 : 1.0);
    const lay::FixedFont &ff = lay::FixedFont::get_font (fr);
    if (ri == 0) {
      EXPECT (ff.stride () > 1);
    }

    const unsigned int n = ff.height ();
    const unsigned int y0 = n + 25;
    const unsigned int clipped_rows [] = { 1, 10, n - 1, n };

    lay::Bitmap full (100, y0 + 1, 1.0, fr);

    lay::RenderText text;
    text.text = "AB";
    text.font = db::DefaultFont;
    text.halign = db::HAlignLeft;
    text.valign = db::VAlignTop;
    text.b = db::DBox (2.0, double (y0), 2.0, double (y0));
    full.render_text (text);

    for (unsigned int ci = 0; ci < sizeof (clipped_rows) / sizeof (clipped_rows [0]); ++ci) {

      //  the same text with the first k rows pushed out above the top edge
      const unsigned int k = clipped_rows [ci];
      const unsigned int hc = y0 - k + 1;

      lay::Bitmap clipped (100, hc, 1.0, fr);
      clipped.render_text (text);

      //  below the first drawn row the clipped bitmap stays empty, further up
      //  it shows the same rows as the fully drawn glyph
      const unsigned int first = (k < n ? hc - (n - k) : hc);
      for (unsigned int py = 0; py < hc; ++py) {
        const uint32_t *sl = clipped.scanline (py);
        const uint32_t *slf = full.scanline (py);
        for (unsigned int px = 0; px < 100; ++px) {
          int bit = (sl [px / 32] >> (px % 32)) & 1;
          if (py >= first) {
            EXPECT_EQ (bit, int ((slf [px / 32] >> (px % 32)) & 1));
          } else {
            EXPECT_EQ (bit, 0);
          }
        }
      }

    }

  }
}
