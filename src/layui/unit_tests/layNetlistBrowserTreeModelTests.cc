
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

#include "layNetlistBrowserTreeModel.h"
#include "tlUnitTest.h"

TEST (1)
{
  db::LayoutToNetlist l2n;
  l2n.load (tl::testdata () + "/lay/l2n_browser.l2n");

  std::unique_ptr<lay::NetlistBrowserTreeModel> model (new lay::NetlistBrowserTreeModel (0, &l2n));

  EXPECT_EQ (model->hasChildren (QModelIndex ()), true);
  //  two circuits
  EXPECT_EQ (model->rowCount (QModelIndex ()), 1);
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, QModelIndex ()), Qt::UserRole).toString ()), "RINGO");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, QModelIndex ()), Qt::DisplayRole).toString ()), "RINGO");
  EXPECT_EQ (model->parent (model->index (0, 0, QModelIndex ())).isValid (), false);

  QModelIndex ringoIndex = model->index (0, 0, QModelIndex ());

  EXPECT_EQ (model->hasChildren (ringoIndex), true);
  EXPECT_EQ (model->rowCount (ringoIndex), 1);
  EXPECT_EQ (model->parent (ringoIndex).isValid (), false);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoIndex), Qt::UserRole).toString ()), "INV2");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoIndex), Qt::DisplayRole).toString ()), "INV2");
  EXPECT_EQ (model->parent (model->index (0, 0, ringoIndex)).isValid (), true);
  EXPECT_EQ (model->parent (model->index (0, 0, ringoIndex)).internalId () == ringoIndex.internalId (), true);

  QModelIndex inv2Index = model->index (0, 0, ringoIndex);

  EXPECT_EQ (model->hasChildren (inv2Index), false);
  EXPECT_EQ (model->rowCount (inv2Index), 0);
  EXPECT_EQ (model->parent (inv2Index) == ringoIndex, true);
}

TEST (2)
{
  db::LayoutVsSchematic lvs;
  lvs.load (tl::testdata () + "/lay/lvsdb_browser.lvsdb");

  std::unique_ptr<lay::NetlistBrowserTreeModel> model (new lay::NetlistBrowserTreeModel (0, &lvs));

  EXPECT_EQ (model->hasChildren (QModelIndex ()), true);
  //  two top circuits
  EXPECT_EQ (model->rowCount (QModelIndex ()), 2);
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, QModelIndex ()), Qt::UserRole).toString ()), "INV2PAIRX");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, QModelIndex ()), Qt::DisplayRole).toString ()), "- \u21D4 INV2PAIRX");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, QModelIndex ()), Qt::UserRole).toString ()), "RINGO|RINGO");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, QModelIndex ()), Qt::DisplayRole).toString ()), "RINGO");
  EXPECT_EQ (model->parent (model->index (0, 0, QModelIndex ())).isValid (), false);
  EXPECT_EQ (model->parent (model->index (1, 0, QModelIndex ())).isValid (), false);

  EXPECT_EQ (model->hasChildren (model->index (0, 0, QModelIndex ())), false);
  EXPECT_EQ (model->rowCount (model->index (0, 0, QModelIndex ())), 0);

  QModelIndex ringoIndex = model->index (1, 0, QModelIndex ());
  EXPECT_EQ (model->hasChildren (model->index (1, 0, QModelIndex ())), true);
  EXPECT_EQ (model->rowCount (model->index (1, 0, QModelIndex ())), 1);
  EXPECT_EQ (model->parent (ringoIndex).isValid (), false);

  EXPECT_EQ (model->parent (model->index (0, 0, ringoIndex)).isValid (), true);
  EXPECT_EQ (model->parent (model->index (0, 0, ringoIndex)).internalId () == ringoIndex.internalId (), true);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoIndex), Qt::UserRole).toString ()), "INV2PAIR|INV2PAIR");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoIndex), Qt::DisplayRole).toString ()), "INV2PAIR");

  QModelIndex inv2PairIndex = model->index (0, 0, ringoIndex);
  EXPECT_EQ (model->hasChildren (model->index (0, 0, ringoIndex)), true);
  EXPECT_EQ (model->rowCount (model->index (0, 0, ringoIndex)), 2);
  EXPECT_EQ (model->parent (inv2PairIndex) == ringoIndex, true);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairIndex), Qt::UserRole).toString ()), "INV2");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairIndex), Qt::DisplayRole).toString ()), "- \u21D4 INV2");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2PairIndex), Qt::UserRole).toString ()), "INV2");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2PairIndex), Qt::DisplayRole).toString ()), "INV2 \u21D4 -");

  EXPECT_EQ (model->hasChildren (model->index (0, 0, inv2PairIndex)), false);
  EXPECT_EQ (model->rowCount (model->index (0, 0, inv2PairIndex)), 0);
  EXPECT_EQ (model->parent (model->index (0, 0, inv2PairIndex)).isValid (), true);
  EXPECT_EQ (model->parent (model->index (0, 0, inv2PairIndex)).internalId () == inv2PairIndex.internalId (), true);
  EXPECT_EQ (model->parent (model->index (1, 0, inv2PairIndex)).isValid (), true);
  EXPECT_EQ (model->parent (model->index (1, 0, inv2PairIndex)).internalId () == inv2PairIndex.internalId (), true);
}

