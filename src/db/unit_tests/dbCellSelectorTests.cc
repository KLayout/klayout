
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



#include "dbLayout.h"
#include "dbCellSelector.h"
#include "tlString.h"
#include "tlUnitTest.h"

//  select or unselect single cells
TEST(1)
{
  db::CellSelector sel;

  EXPECT_EQ (sel.empty (), true);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "");
  EXPECT_EQ (sel.is_valid (), true);

  sel.select_cell (1);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+#1");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), 1);
  EXPECT_EQ (sel.in_selection (2), 0);

  sel.unselect_cell (1);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "-#1");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), -1);
  EXPECT_EQ (sel.in_selection (2), 0);

  sel.select_cell (2);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+#2 -#1");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), -1);
  EXPECT_EQ (sel.in_selection (2), 1);

  sel.reset_selection ();
  EXPECT_EQ (sel.empty (), true);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "");
  EXPECT_EQ (sel.is_valid (), true);

  sel.select_all_cells ();
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), true);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+*");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), 1);
  EXPECT_EQ (sel.in_selection (2), 1);

  sel.unselect_cell (5);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+* +-#5 -#5");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), 1);
  EXPECT_EQ (sel.in_selection (2), 1);
  EXPECT_EQ (sel.in_selection (5), -1);

  sel.select_cell (5);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), true);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+*");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), 1);
  EXPECT_EQ (sel.in_selection (2), 1);
  EXPECT_EQ (sel.in_selection (5), 1);

  sel.unselect_all_cells ();
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), true);
  EXPECT_EQ (sel.to_string (), "-*");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), -1);
  EXPECT_EQ (sel.in_selection (2), -1);
  EXPECT_EQ (sel.in_selection (5), -1);

  sel.reset_selection ();

  sel.unselect_all_cells ();
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), true);
  EXPECT_EQ (sel.to_string (), "-*");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), -1);
  EXPECT_EQ (sel.in_selection (2), -1);
  EXPECT_EQ (sel.in_selection (5), -1);

  sel.select_cell (5);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+#5 -* --#5");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), -1);
  EXPECT_EQ (sel.in_selection (2), -1);
  EXPECT_EQ (sel.in_selection (5), 1);

  sel.unselect_cell (5);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), true);
  EXPECT_EQ (sel.to_string (), "-*");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), -1);
  EXPECT_EQ (sel.in_selection (2), -1);
  EXPECT_EQ (sel.in_selection (5), -1);

  sel.select_all_cells ();
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), true);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+*");
  EXPECT_EQ (sel.is_valid (), true);

  EXPECT_EQ (sel.in_selection (1), 1);
  EXPECT_EQ (sel.in_selection (2), 1);
  EXPECT_EQ (sel.in_selection (5), 1);
}

//  select or unselect sequences of cells
TEST(2)
{
  db::CellSelector sel;
  std::set<db::cell_index_type> ci;

  EXPECT_EQ (sel.empty (), true);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "");

  ci = { 1, 2 };
  sel.select_cells (ci);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+#1 +#2");

  EXPECT_EQ (sel.in_selection (1), 1);
  EXPECT_EQ (sel.in_selection (2), 1);
  EXPECT_EQ (sel.in_selection (3), 0);
  EXPECT_EQ (sel.in_selection (4), 0);

  ci = { 2, 3 };
  sel.unselect_cells (ci);
  EXPECT_EQ (sel.empty (), false);
  EXPECT_EQ (sel.selects_all (), false);
  EXPECT_EQ (sel.unselects_all (), false);
  EXPECT_EQ (sel.to_string (), "+#1 -#2 -#3");
}

//  to_string with cell names
TEST(3)
{
  db::Layout ly;
  db::CellSelector sel;

  auto c1 = ly.add_cell ("C1");
  auto c2 = ly.add_cell ("C2");
  auto c5 = ly.add_cell ("C5");

  sel.select_cell (c1);
  EXPECT_EQ (sel.to_string (&ly), "+C1");

  sel.unselect_cell (c1);
  EXPECT_EQ (sel.to_string (&ly), "-C1");

  sel.select_cell (c2);
  EXPECT_EQ (sel.to_string (&ly), "+C2 -C1");

  sel.reset_selection ();
  EXPECT_EQ (sel.to_string (&ly), "");

  sel.select_all_cells ();
  EXPECT_EQ (sel.to_string (&ly), "+*");

  sel.unselect_cell (c5);
  EXPECT_EQ (sel.to_string (&ly), "+* +-C5 -C5");

  sel.select_cell (c5);
  EXPECT_EQ (sel.to_string (&ly), "+*");

  sel.reset_selection ();
  EXPECT_EQ (sel.to_string (&ly), "");

  sel.unselect_all_cells ();
  EXPECT_EQ (sel.to_string (&ly), "-*");

  sel.select_cell (c5);
  EXPECT_EQ (sel.to_string (&ly), "+C5 -* --C5");

  sel.unselect_all_cells ();
  EXPECT_EQ (sel.to_string (&ly), "-*");
}

//  parsing cell filters
TEST(4)
{
  db::Layout ly;
  db::CellSelector sel;

  auto c1 = ly.add_cell ("C1");
  /*auto c2 =*/ ly.add_cell ("C2");
  /*auto c5 =*/ ly.add_cell ("C+5");
  /*auto cstar =*/ ly.add_cell ("C*");

  tl::Extractor ex;
  ex = tl::Extractor ("");

  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.empty (), true);
  EXPECT_EQ (sel.to_string (&ly), "");
  EXPECT_EQ (sel.original_string (), "");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("**INVALID**");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (ex.at_end (), false);
  EXPECT_EQ (sel.empty (), true);
  EXPECT_EQ (sel.to_string (&ly), "");
  EXPECT_EQ (sel.original_string (), "");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+#0 -#1");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+C1 -C2");
  EXPECT_EQ (sel.original_string (), "+#0 -#1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+C1");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+C1");
  EXPECT_EQ (sel.original_string (), "+C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+\"C1\"");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+C1");
  EXPECT_EQ (sel.original_string (), "+\"C1\"");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+\"C+5\"");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+'C+5'");
  EXPECT_EQ (sel.original_string (), "+\"C+5\"");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+'C+5'");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+'C+5'");
  EXPECT_EQ (sel.original_string (), "+'C+5'");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+C*");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+C1 +C2 +'C+5' +'C*'");
  EXPECT_EQ (sel.original_string (), "+C*");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+'C*'");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+'C*'");
  EXPECT_EQ (sel.original_string (), "+'C*'");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+\"C*\"");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+C1 +C2 +'C+5' +'C*'");
  EXPECT_EQ (sel.original_string (), "+\"C*\"");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+* -C1  ");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+* +-C1 -C1");
  EXPECT_EQ (sel.original_string (), "+* -C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+* +-C1");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+* +-C1");
  EXPECT_EQ (sel.original_string (), "+* +-C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+-C1");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "");
  EXPECT_EQ (sel.original_string (), "+-C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("-* +C1");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+C1 -* --C1");
  EXPECT_EQ (sel.original_string (), "-* +C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("-* --C1");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "-* --C1");
  EXPECT_EQ (sel.original_string (), "-* --C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("--C1");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "");
  EXPECT_EQ (sel.original_string (), "--C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+* -*");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "-*");
  EXPECT_EQ (sel.original_string (), "+* -*");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+* -@");
  EXPECT_EQ (sel.parse (ex, &ly), true);
  EXPECT_EQ (sel.to_string (&ly), "+* -@");
  EXPECT_EQ (sel.original_string (), "+* -@");
  EXPECT_EQ (sel.is_valid (), false);   //  no top cell given

  ex = tl::Extractor ("+* -@");
  EXPECT_EQ (sel.parse (ex, &ly, c1), true);
  EXPECT_EQ (sel.to_string (&ly), "+* +-C1 -C1");
  EXPECT_EQ (sel.original_string (), "+* -@");
  EXPECT_EQ (sel.is_valid (), true);
}

//  late validation
TEST(5)
{
  db::Layout ly;
  db::CellSelector sel;

  auto c1 = ly.add_cell ("C1");
  /*auto c2 =*/ ly.add_cell ("C2");
  /*auto c5 =*/ ly.add_cell ("C+5");
  /*auto cstar =*/ ly.add_cell ("C*");

  tl::Extractor ex;
  ex = tl::Extractor ("");

  EXPECT_EQ (sel.parse (ex), true);
  EXPECT_EQ (sel.empty (), true);
  EXPECT_EQ (sel.to_string (&ly), "");
  EXPECT_EQ (sel.original_string (), "");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+#0 -#1");
  EXPECT_EQ (sel.parse (ex), true);
  EXPECT_EQ (sel.to_string (&ly), "+C1 -C2");
  EXPECT_EQ (sel.original_string (), "+#0 -#1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+C1");
  EXPECT_EQ (sel.parse (ex), true);
  EXPECT_EQ (sel.to_string (&ly), "+C1");
  EXPECT_EQ (sel.original_string (), "+C1");
  EXPECT_EQ (sel.is_valid (), false);
  sel.ensure_valid (&ly, c1);
  EXPECT_EQ (sel.to_string (&ly), "+C1");
  EXPECT_EQ (sel.original_string (), "+C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+C*");
  EXPECT_EQ (sel.parse (ex), true);
  EXPECT_EQ (sel.to_string (&ly), "+C*");
  EXPECT_EQ (sel.original_string (), "+C*");
  EXPECT_EQ (sel.is_valid (), false);
  sel.ensure_valid (&ly, c1);
  EXPECT_EQ (sel.to_string (&ly), "+C1 +C2 +'C+5' +'C*'");
  EXPECT_EQ (sel.original_string (), "+C*");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+* -C1");
  EXPECT_EQ (sel.parse (ex), true);
  EXPECT_EQ (sel.to_string (&ly), "+* -C1");
  EXPECT_EQ (sel.original_string (), "+* -C1");
  EXPECT_EQ (sel.is_valid (), false);
  sel.ensure_valid (&ly, c1);
  EXPECT_EQ (sel.to_string (&ly), "+* +-C1 -C1");
  EXPECT_EQ (sel.original_string (), "+* -C1");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+* -*");
  EXPECT_EQ (sel.parse (ex), true);
  EXPECT_EQ (sel.to_string (&ly), "-*");
  EXPECT_EQ (sel.original_string (), "+* -*");
  EXPECT_EQ (sel.is_valid (), true);
  sel.ensure_valid (&ly, c1);  //  does nothing
  EXPECT_EQ (sel.to_string (&ly), "-*");
  EXPECT_EQ (sel.original_string (), "+* -*");
  EXPECT_EQ (sel.is_valid (), true);

  ex = tl::Extractor ("+* -@");
  EXPECT_EQ (sel.parse (ex), true);
  EXPECT_EQ (sel.to_string (&ly), "+* -@");
  EXPECT_EQ (sel.original_string (), "+* -@");
  EXPECT_EQ (sel.is_valid (), false);
  sel.ensure_valid (&ly, c1);
  EXPECT_EQ (sel.to_string (&ly), "+* +-C1 -C1");
  EXPECT_EQ (sel.original_string (), "+* -@");
  EXPECT_EQ (sel.is_valid (), true);
}
