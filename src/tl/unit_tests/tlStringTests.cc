
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


#include "tlString.h"
#include "tlTimer.h"
#include "tlUnitTest.h"

#define _USE_MATH_DEFINES // for MSVC
#include <math.h>
#include <clocale>

using namespace tl;

static std::string norm_exp (const std::string &s)
{
  return tl::replaced (tl::replaced (s, "e+006", "e+06"), "E+006", "E+06");
}

TEST(1)
{
  EXPECT_EQ (to_string (12.5), "12.5");
  EXPECT_EQ (tl::sprintf ("%.2f", 12.5), "12.50");
  EXPECT_EQ (to_string (int (12)), "12");
  EXPECT_EQ (to_string (long (12)), "12");
  EXPECT_EQ (to_string ((unsigned int)12), "12");
  EXPECT_EQ (to_string ((unsigned long)12), "12");
  EXPECT_EQ (to_string ((char *)" 12"), " 12");
  EXPECT_EQ (to_string (" 12", 2), " 1");
  EXPECT_EQ (to_string ((unsigned char *)" 12"), " 12");
  EXPECT_EQ (to_string (std::string (" 12")), " 12");

  EXPECT_EQ (norm_exp (tl::sprintf("%g %e %f",M_PI,M_PI*1e6,M_PI*0.001)), "3.14159 3.141593e+06 0.003142");
  EXPECT_EQ (norm_exp (tl::sprintf("%G %E %F",M_PI*1e6,M_PI*1e6,M_PI*1e6)), "3.14159E+06 3.141593E+06 3141592.653590");
  EXPECT_EQ (norm_exp (tl::sprintf("%-15g %015.8e %15.12f %g",M_PI,M_PI*1e6,M_PI*0.001,M_PI)), "3.14159         03.14159265e+06  0.003141592654 3.14159");
  EXPECT_EQ (norm_exp (tl::sprintf("%-15g %015.8E %15.12f %g",M_PI,M_PI*1e6,M_PI*0.001,M_PI)), "3.14159         03.14159265E+06  0.003141592654 3.14159");
  EXPECT_EQ (tl::sprintf("%-5s %5s %x %u %d (%s)","a","b",1234,2345,3456), "a         b 4d2 2345 3456 ()");
  EXPECT_EQ (tl::sprintf("%lu %llu %02x", 1, 2, 167), "1 2 a7");
  EXPECT_EQ (tl::sprintf("%lu %llu %02X", 1, 2, 761), "1 2 2F9");
  EXPECT_EQ (tl::sprintf("%c%c", 'a', 'X'), "aX");
}

TEST(1a)
{
  double d;

  from_string("-.10500", d);
  EXPECT_EQ (to_string(d), "-0.105");

  from_string("000.105", d);
  EXPECT_EQ (to_string(d), "0.105");

  from_string("10", d);
  EXPECT_EQ (to_string(d), "10");

  from_string("-0010", d);
  EXPECT_EQ (to_string(d), "-10");

  from_string("-15.", d);
  EXPECT_EQ (to_string(d), "-15");

  from_string("-15.000e-1", d);
  EXPECT_EQ (to_string(d), "-1.5");

  from_string("-15.000E+1", d);
  EXPECT_EQ (to_string(d), "-150");

  from_string_ext("-15.000E+1", d);
  EXPECT_EQ (to_string(d), "-150");

  from_string_ext("25400/25.4", d);
  EXPECT_EQ (to_string(d), "1000");

  from_string_ext("25400/(25+0.4)", d);
  EXPECT_EQ (to_string(d), "1000");

  from_string_ext(" 25400   / (25  + 0.4 )   ", d);
  EXPECT_EQ (to_string(d), "1000");

  from_string("1E+03", d);
  EXPECT_EQ (to_string(d), "1000");

  from_string("11E3", d);
  EXPECT_EQ (to_string(d), "11000");

  from_string("0.0515e+003", d);
  EXPECT_EQ (to_string(d), "51.5");

  tl::Extractor ex ("    -15.000e-1x");
  double x;
  ex.read (x);
  EXPECT_EQ (to_string(x), "-1.5");
  ex.expect ("x");
}

TEST(2)
{
  long l;
  unsigned long ul;
  int i;
  unsigned int ui;
  std::string s;
  double d;
  bool error;

  from_string ("12.5", d);
  EXPECT_EQ (d, 12.5);
  from_string ("-12.5", d);
  EXPECT_EQ (d, -12.5);
  from_string ("-12.5e2", d);
  EXPECT_EQ (d, -12.5e2);
  from_string ("   -12.5e2    ", d);
  EXPECT_EQ (d, -12.5e2);
  error = false;
  try { from_string ("a", d); } catch (...) { error = true; }
  EXPECT_EQ (error, true);
  error = false;
  try { from_string ("12a", d); } catch (...) { error = true; }
  EXPECT_EQ (error, true);
  
  from_string ("   12   ", ul);
  EXPECT_EQ (ul, (unsigned int) 12);
  from_string_ext ("   12   ", ul);
  EXPECT_EQ (ul, (unsigned int) 12);
  error = false;
  try { from_string ("a", ul); } catch (...) { error = true; }
  EXPECT_EQ (error, true);
  error = false;
  try { from_string ("-12", ul); } catch (...) { error = true; }
  EXPECT_EQ (error, true);

  from_string ("   12   ", l);
  EXPECT_EQ (l, 12);
  from_string_ext ("   12   ", l);
  EXPECT_EQ (l, 12);
  error = false;
  try { from_string ("a", l); } catch (...) { error = true; }
  EXPECT_EQ (error, true);

  from_string ("   12   ", ui);
  EXPECT_EQ (ui, (unsigned int) 12);
  from_string_ext ("   12   ", ui);
  EXPECT_EQ (ui, (unsigned int) 12);
  error = false;
  try { from_string ("a", ui); } catch (...) { error = true; }
  EXPECT_EQ (error, true);
  error = false;
  try { from_string ("-12", ui); } catch (...) { error = true; }
  EXPECT_EQ (error, true);

  from_string ("   12   ", i);
  EXPECT_EQ (i, 12);
  from_string_ext ("   12   ", i);
  EXPECT_EQ (i, 12);
  error = false;
  try { from_string ("a", i); } catch (...) { error = true; }
  EXPECT_EQ (error, true);

  from_string ("  12", s);
  EXPECT_EQ (s, "  12");
}

TEST(3)
{
  EXPECT_EQ (trim ("   12   "), "12");
  EXPECT_EQ (trim ("   1 2   "), "1 2");
  EXPECT_EQ (trim ("   1 2"), "1 2");
  EXPECT_EQ (trim ("1 2"), "1 2");
}

TEST(4)
{
  tl::string s;
  EXPECT_EQ (std::string (s.c_str ()), "");
  EXPECT_EQ (s.std_str (), "");
  EXPECT_EQ (s.size (), size_t (0));
  EXPECT_EQ (s.capacity (), size_t (0));

  s = "abc";
  EXPECT_EQ (std::string (s.c_str ()), "abc");
  EXPECT_EQ (s.std_str (), "abc");
  EXPECT_EQ (s.size (), size_t (3));
  EXPECT_EQ (s.capacity (), size_t (3));

  s.assign ("abc", 1, 2);
  EXPECT_EQ (std::string (s.c_str ()), "b");
  EXPECT_EQ (s == "b", true);
  EXPECT_EQ (s != "b", false);
  EXPECT_EQ (s == "a", false);
  EXPECT_EQ (s == "", false);
  EXPECT_EQ (s > "", true);
  EXPECT_EQ (s > "a", true);
  EXPECT_EQ (s > "b", false);
  EXPECT_EQ (s < "", false);
  EXPECT_EQ (s < "b", false);
  EXPECT_EQ (s < "ba", true);
  EXPECT_EQ (s < "c", true);
  EXPECT_EQ (s.std_str (), "b");
  EXPECT_EQ (s.size (), size_t (1));
  EXPECT_EQ (s.capacity (), size_t (3));

  s = std::string ("abcdef");
  EXPECT_EQ (s.std_str (), "abcdef");
  EXPECT_EQ (s.size (), size_t (6));
  EXPECT_EQ (s.capacity (), size_t (6));

  s = std::string ();
  EXPECT_EQ (s.std_str (), "");
  EXPECT_EQ (s.size (), size_t (0));
  EXPECT_EQ (s.capacity (), size_t (6));

  s = "xyz";
  EXPECT_EQ (s.std_str (), "xyz");
  EXPECT_EQ (s.size (), size_t (3));
  EXPECT_EQ (s.capacity (), size_t (6));

  s.clear ();
  EXPECT_EQ (s.std_str (), "");
  EXPECT_EQ (s.size (), size_t (0));
  EXPECT_EQ (s.capacity (), size_t (0));

  // ...

}

TEST(5)
{
  Extractor x ("\t5   :  -6 oder-1.5e001");
  Extractor xx ("\t   ");

  EXPECT_EQ (x.at_end (), false);
  EXPECT_EQ (xx.at_end (), true);

  unsigned int ui = 0;
  long l = 0;
  std::string s;
  double d = 0;

  x.read (ui);
  EXPECT_EQ (x.test (":"), true);
  x.read (l);
  x.read (s, "-");
  x.read (d);

  EXPECT_EQ (ui, (unsigned int) 5);
  EXPECT_EQ (l, -6);
  EXPECT_EQ (s, "oder");
  EXPECT_EQ (d, -15.0);
}

TEST(6)
{
  Extractor x ("\t5:  -6 oder");

  EXPECT_EQ (x.at_end (), false);

  unsigned long ul = 0;
  int i = 0;
  std::string s;

  EXPECT_EQ (x.try_read (ul), true);
  EXPECT_EQ (x.try_read (ul), false);
  EXPECT_EQ (ul, (unsigned long) 5);
  EXPECT_EQ (x.test (";"), false);
  x.expect (":");
  EXPECT_EQ (x.try_read (i), true);
  x.skip ();
  EXPECT_EQ (*x, 'o');
  ++x;
  EXPECT_EQ (x.try_read (s, "-"), true);
  EXPECT_EQ (s, "der");

  Extractor x1 ("\t aber:");
  x1.read (s, ":");
  EXPECT_EQ (s, "aber");

  Extractor x2 ("\t aber  :");
  x2.read (s);
  EXPECT_EQ (s, "aber");
  x2.expect (":");

  Extractor x3 ("\t aber\t:");
  x3.read (s);
  EXPECT_EQ (s, "aber");
  EXPECT_EQ (x3.test (";"), false);
  EXPECT_EQ (x3.test (":"), true);
}

TEST(7)
{
  EXPECT_EQ (tl::to_quoted_string ("a_word!"), "'a_word!'");
  EXPECT_EQ (tl::to_quoted_string ("a_word'!"), "'a_word\\'!'");
  EXPECT_EQ (tl::to_word_or_quoted_string ("a_word!"), "'a_word!'");
  EXPECT_EQ (tl::to_word_or_quoted_string ("a_word!", "_!"), "a_word!");
}

TEST(8)
{
  std::string s;
  Extractor x;

  x = Extractor ("a_word!");
  x.read_word (s);
  EXPECT_EQ (s, "a_word");

  x = Extractor ("a_word!");
  s.clear ();
  x.read_name (s);
  EXPECT_EQ (s, "a_word");
  EXPECT_EQ (x.test ("!"), true);

  x = Extractor ("0_word!");
  EXPECT_EQ (x.try_read_word (s), true);

  x = Extractor ("0_word!");
  EXPECT_EQ (x.try_read_name (s), false);

  x = Extractor ("a_word!");
  EXPECT_EQ (x.try_read_word (s), true);
  EXPECT_EQ (s, "a_word");
  EXPECT_EQ (x.test ("!"), true);

  x = Extractor ("a_word!");
  EXPECT_EQ (x.try_read_name (s), true);
  EXPECT_EQ (s, "a_word");
  EXPECT_EQ (x.test ("!"), true);

  x = Extractor ("a_word!");
  x.read_word (s, "_!");
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("a_word!");
  x.read_name (s, "_!");
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("a_word!");
  EXPECT_EQ (x.try_read_word (s, "_!"), true);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("a_word!");
  EXPECT_EQ (x.try_read_name (s, "_!"), true);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("a_word!");
  x.read_word_or_quoted (s);
  EXPECT_EQ (s, "a_word");
  EXPECT_EQ (x.test ("!"), true);

  x = Extractor ("a_word!");
  EXPECT_EQ (x.try_read_word_or_quoted (s), true);
  EXPECT_EQ (s, "a_word");
  EXPECT_EQ (x.test ("!"), true);

  x = Extractor ("a_word!");
  x.read_word_or_quoted (s, "_!");
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("a_word!");
  EXPECT_EQ (x.try_read_word_or_quoted (s, "_!"), true);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("'a_word!'");
  x.read_word_or_quoted (s);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("'a_word!'");
  EXPECT_EQ (x.try_read_word_or_quoted (s), true);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("'a_word!'x");
  EXPECT_EQ (x.try_read_word_or_quoted (s), true);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.test ("x"), true);

  x = Extractor ("'a_word\\'!'");
  EXPECT_EQ (x.try_read_word_or_quoted (s), true);
  EXPECT_EQ (s, "a_word\'!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("'a_word!'");
  x.read_quoted (s);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("'a_word!'");
  EXPECT_EQ (x.try_read_quoted (s), true);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor ("'a_word!'x");
  EXPECT_EQ (x.try_read_quoted (s), true);
  EXPECT_EQ (s, "a_word!");
  EXPECT_EQ (x.test ("x"), true);

  x = Extractor ("'a_word\\'!'");
  EXPECT_EQ (x.try_read_quoted (s), true);
  EXPECT_EQ (s, "a_word\'!");
  EXPECT_EQ (x.at_end (), true);

  x = Extractor (" foobar");
  EXPECT_EQ (x.test ("foo"), true);
  EXPECT_EQ (x.test ("bar"), true);

  x = Extractor (" foo bar");
  EXPECT_EQ (x.test ("foo"), true);
  EXPECT_EQ (x.test ("bar"), true);

  x = Extractor (" FOObar");
  EXPECT_EQ (x.test ("foo"), false);
  EXPECT_EQ (x.test ("BAR"), false);

  x = Extractor (" FOObar");
  EXPECT_EQ (x.test_without_case ("foo"), true);
  EXPECT_EQ (x.test_without_case ("BAR"), true);

  x = Extractor (" µm");
  EXPECT_EQ (x.test ("µm"), true);

  x = Extractor (" µM");
  EXPECT_EQ (x.test ("µm"), false);
  EXPECT_EQ (x.test_without_case ("µm"), true);

  x = Extractor (" µm");
  EXPECT_EQ (x.test ("µM"), false);
  EXPECT_EQ (x.test_without_case ("µM"), true);
}

TEST(9)
{
  EXPECT_EQ (tl::edit_distance ("", ""), 0);
  EXPECT_EQ (tl::edit_distance ("aber", "aber"), 0);
  EXPECT_EQ (tl::edit_distance ("ober", "aber"), 1);
  EXPECT_EQ (tl::edit_distance ("obe", "aber"), 2);
  EXPECT_EQ (tl::edit_distance ("abe", "aber"), 1);
  EXPECT_EQ (tl::edit_distance ("axbe", "aber"), 2);
  EXPECT_EQ (tl::edit_distance ("axbep", "aber"), 2);
  EXPECT_EQ (tl::edit_distance ("Tor", "Tier"), 2);
  EXPECT_EQ (tl::edit_distance ("kitten", "sitting"), 3);
  EXPECT_EQ (tl::edit_distance ("matthias", "koefferlein"), 11);
}

TEST(10)
{
  std::string s;
  s = to_quoted_string ("'a\n\003");
  EXPECT_EQ (s, "'\\'a\\n\\003'");
  std::string t;
  tl::Extractor ex (s.c_str ());
  ex.read_word_or_quoted (t);

  s = to_quoted_string ("hallo\303\t\r\"");
  EXPECT_EQ (s, "'hallo\\303\\t\\r\"'");
  t.clear ();
  ex = tl::Extractor (s.c_str ());
  ex.read_word_or_quoted (t);
  EXPECT_EQ (t, "hallo\303\t\r\"");

  EXPECT_EQ (escape_string ("'a\n\003"), "'a\\n\\003");
  EXPECT_EQ (escape_string ("'a\n\003"), "'a\\n\\003");
  EXPECT_EQ (unescape_string (escape_string ("'a\n\003")), "'a\n\003");
}

TEST(11)
{
  std::string s;
  tl::escape_to_html (s, "x");
  EXPECT_EQ (s, "x");
  tl::escape_to_html (s, "<&>");
  EXPECT_EQ (s, "x&lt;&amp;&gt;");
  s = std::string ();
  tl::escape_to_html (s, "a\nb");
  EXPECT_EQ (s, "a<br/>b");
  s = std::string ();
  tl::escape_to_html (s, "a\nb", false);
  EXPECT_EQ (s, "a\nb");
  EXPECT_EQ (tl::escaped_to_html ("x<&>\""), "x&lt;&amp;&gt;&quot;");
  EXPECT_EQ (tl::escaped_to_html ("a\nb"), "a<br/>b");
  EXPECT_EQ (tl::escaped_to_html ("a\nb", false), "a\nb");
}

TEST(12)
{
  EXPECT_EQ (replaced ("abc", "b", "xy"), "axyc");
  EXPECT_EQ (replaced ("ab", "b", "xy"), "axy");
  EXPECT_EQ (replaced ("bc", "b", "xy"), "xyc");
  EXPECT_EQ (replaced ("b", "b", "xy"), "xy");
  EXPECT_EQ (replaced ("bbbb", "b", "xy"), "xyxyxyxy");
  EXPECT_EQ (replaced ("", "b", "xy"), "");
  EXPECT_EQ (replaced ("ac", "b", "xy"), "ac");
  EXPECT_EQ (replaced ("abc", "b", ""), "ac");
  EXPECT_EQ (replaced ("bb", "b", ""), "");
  EXPECT_EQ (replaced ("bb", "bbb", ""), "bb");
  EXPECT_EQ (replaced ("abbbc", "bbb", "xy"), "axyc");
  EXPECT_EQ (replaced ("abbbbbbc", "bbb", "xy"), "axyxyc");
  EXPECT_EQ (replaced ("abbbbbbbc", "bbb", "xy"), "axyxybc");
}

TEST(13)
{
  EXPECT_EQ (replicate ("abc", 0), "");
  EXPECT_EQ (replicate ("abc", 1), "abc");
  EXPECT_EQ (replicate ("abc", 2), "abcabc");
  EXPECT_EQ (replicate ("", 2), "");
}

TEST(14)
{
  EXPECT_EQ (pad_string_right (0, "abc"), "abc");
  EXPECT_EQ (pad_string_right (2, "abc"), "abc");
  EXPECT_EQ (pad_string_right (4, "abc"), "abc ");
  EXPECT_EQ (pad_string_right (6, "abc"), "abc   ");
  EXPECT_EQ (pad_string_right (4, ""), "    ");
  EXPECT_EQ (pad_string_left (0, "abc"), "abc");
  EXPECT_EQ (pad_string_left (2, "abc"), "abc");
  EXPECT_EQ (pad_string_left (4, "abc"), " abc");
  EXPECT_EQ (pad_string_left (6, "abc"), "   abc");
  EXPECT_EQ (pad_string_left (4, ""), "    ");
}

//  UTF-8 to wchar_t and local conversion
TEST(15)
{
  std::string locale = setlocale (LC_ALL, NULL);
  const char *lc = setlocale (LC_ALL, "en_US.UTF-8");
  if (! lc || std::string (lc) != "en_US.UTF-8") {
    //  use C.UTF-8 as fallback
    setlocale (LC_ALL, "C.UTF-8");
  }

  try {
    EXPECT_EQ (tl::to_string_from_local (tl::to_local ("H\xc3\xa4llo\tW\xc3\xb6rld!").c_str ()), "H\xc3\xa4llo\tW\xc3\xb6rld!");
    setlocale (LC_ALL, locale.c_str ());
  } catch (...) {
    setlocale (LC_ALL, locale.c_str ());
    throw;
  }

  EXPECT_EQ (std::string ("\xc3\x84").size (), size_t (2));
  EXPECT_EQ (tl::to_string (std::wstring (L"Ä")), "\xc3\x84");
  EXPECT_EQ (tl::to_wstring (std::string ("\xc3\x84")).size (), size_t (1));
  EXPECT_EQ (tl::to_string (tl::to_wstring ("Utf8 supports emoticons: \xF0\x9F\x98\x81\nand Umlauts: \xc3\xa4\xc3\xbc\xc3\xb6")).c_str (), "Utf8 supports emoticons: \xF0\x9F\x98\x81\nand Umlauts: \xc3\xa4\xc3\xbc\xc3\xb6");

  EXPECT_EQ (tl::to_upper_case ("nOrMaliI(\xc3\xa4\xc3\x84\xc3\xbc\xc3\x9c\xc3\xb6\xc3\x96\xc3\x9f-42\xc2\xb0+6\xe2\x82\xac)"), "NORMALII(\xc3\x84\xc3\x84\xc3\x9c\xc3\x9c\xc3\x96\xc3\x96\xc3\x9f-42\xc2\xb0+6\xe2\x82\xac)");
  EXPECT_EQ (tl::to_lower_case ("nOrMaliI(\xc3\xa4\xc3\x84\xc3\xbc\xc3\x9c\xc3\xb6\xc3\x96\xc3\x9f-42\xc2\xb0+6\xe2\x82\xac)"), "normalii(\xc3\xa4\xc3\xa4\xc3\xbc\xc3\xbc\xc3\xb6\xc3\xb6\xc3\x9f-42\xc2\xb0+6\xe2\x82\xac)");
}
