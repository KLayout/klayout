
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


#include "tlUnitTest.h"
#include "dbLoadLayoutOptions.h"
#include "gsiDecl.h"

class MyReaderOptions
  : public db::FormatSpecificReaderOptions
{
public:
  MyReaderOptions ()
    : db::FormatSpecificReaderOptions ()
  {
  }

  virtual FormatSpecificReaderOptions *clone () const
  {
    return new MyReaderOptions (*this);
  }

  virtual const std::string &format_name () const
  {
    static std::string fmt ("myformat");
    return fmt;
  }

  std::string value;
  db::LayerMap lm;
};

static std::string get_myreader_value (const db::LoadLayoutOptions *options)
{
  return options->get_options<MyReaderOptions> ().value;
}

static void set_myreader_value (db::LoadLayoutOptions *options, const std::string &v)
{
  options->get_options<MyReaderOptions> ().value = v;
}

static db::LayerMap get_myreader_lm (const db::LoadLayoutOptions *options)
{
  return options->get_options<MyReaderOptions> ().lm;
}

static void set_myreader_lm (db::LoadLayoutOptions *options, const db::LayerMap &lm)
{
  options->get_options<MyReaderOptions> ().lm = lm;
}


static
gsi::ClassExt<db::LoadLayoutOptions> myreaderoptions_cls (
  gsi::method_ext ("myreader_value", &get_myreader_value) +
  gsi::method_ext ("myreader_value=", &set_myreader_value) +
  gsi::method_ext ("myreader_lm", &get_myreader_lm) +
  gsi::method_ext ("myreader_lm=", &set_myreader_lm),
  "@hide");


TEST(1)
{
  db::LoadLayoutOptions opt;
  MyReaderOptions myopt;
  myopt.value = "42";
  opt.set_options (myopt);

  EXPECT_EQ (opt.get_options<MyReaderOptions> ().value, "42");
  EXPECT_EQ (opt.get_option_by_name ("myreader_value").to_string (), "42");
  opt.set_option_by_name ("myreader_value", tl::Variant ("abc"));
  EXPECT_EQ (opt.get_option_by_name ("myreader_value").to_string (), "abc");

  db::LayerMap lm = db::LayerMap::from_string_file_format ("1/0:2\n10/0");
  EXPECT_EQ (lm.to_string (), "layer_map('1/0 : 2/0';'10/0')");
  opt.set_option_by_name ("myreader_lm", tl::Variant::make_variant (lm));
  EXPECT_EQ (opt.get_option_by_name ("myreader_lm").to_user<db::LayerMap> ().to_string (), "layer_map('1/0 : 2/0';'10/0')");

  myopt.value = "17";
  opt.set_options (myopt);
  EXPECT_EQ (opt.get_options<MyReaderOptions> ().value, "17");
  EXPECT_EQ (opt.get_option_by_name ("myreader_value").to_string (), "17");
}

