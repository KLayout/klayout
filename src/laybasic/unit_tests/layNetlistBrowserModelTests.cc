
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "layNetlistBrowserPage.h"
#include "tlUnitTest.h"

TEST (1)
{
  db::LayoutToNetlist l2n;
  l2n.load (tl::testsrc () + "/testdata/lay/l2n_browser.l2n");

  std::auto_ptr<lay::NetlistBrowserModel> model (new lay::NetlistBrowserModel (0, &l2n));

  EXPECT_EQ (model->hasChildren (QModelIndex ()), true);
  //  two circuits
  EXPECT_EQ (model->rowCount (QModelIndex ()), 2);
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, QModelIndex ()), Qt::DisplayRole).toString ()), "INV2");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, QModelIndex ()), Qt::DisplayRole).toString ()), "RINGO");
  EXPECT_EQ (model->parent (model->index (0, 0, QModelIndex ())).isValid (), false);
  EXPECT_EQ (model->parent (model->index (1, 0, QModelIndex ())).isValid (), false);

  QModelIndex ringoIndex = model->index (1, 0, QModelIndex ());
  QModelIndex inv2Index = model->index (0, 0, QModelIndex ());

  EXPECT_EQ (model->hasChildren (inv2Index), true);
  //  5 pins, 5 nets, 0 subcircuits, 4 devices
  EXPECT_EQ (model->rowCount (inv2Index), 14);
  //  Pins
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Index), Qt::DisplayRole).toString ()), "IN");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2Index), Qt::DisplayRole).toString ()), "$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, inv2Index), Qt::DisplayRole).toString ()), "OUT");
  EXPECT_EQ (tl::to_string (model->data (model->index (3, 0, inv2Index), Qt::DisplayRole).toString ()), "$3");
  EXPECT_EQ (tl::to_string (model->data (model->index (4, 0, inv2Index), Qt::DisplayRole).toString ()), "$4");
  //  Nets
  EXPECT_EQ (tl::to_string (model->data (model->index (5, 0, inv2Index), Qt::DisplayRole).toString ()), "NIN");
  EXPECT_EQ (tl::to_string (model->data (model->index (6, 0, inv2Index), Qt::DisplayRole).toString ()), "$2");
  EXPECT_EQ (tl::to_string (model->data (model->index (7, 0, inv2Index), Qt::DisplayRole).toString ()), "NOUT");
  EXPECT_EQ (tl::to_string (model->data (model->index (8, 0, inv2Index), Qt::DisplayRole).toString ()), "$4");
  EXPECT_EQ (tl::to_string (model->data (model->index (9, 0, inv2Index), Qt::DisplayRole).toString ()), "$5");
  //  No Subcircuits
  //  Devices
  EXPECT_EQ (tl::to_string (model->data (model->index (10, 0, inv2Index), Qt::DisplayRole).toString ()), "$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (11, 0, inv2Index), Qt::DisplayRole).toString ()), "$2");
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 0, inv2Index), Qt::DisplayRole).toString ()), "$3");
  EXPECT_EQ (tl::to_string (model->data (model->index (13, 0, inv2Index), Qt::DisplayRole).toString ()), "$4");

  EXPECT_EQ (model->hasChildren (ringoIndex), true);
  //  0 pins, 12 nets, 10 subcircuits, 0 devices
  EXPECT_EQ (model->rowCount (ringoIndex), 22);
  //  Pins
  //  Nets
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoIndex), Qt::DisplayRole).toString ()), "FB");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, ringoIndex), Qt::DisplayRole).toString ()), "VSS");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, ringoIndex), Qt::DisplayRole).toString ()), "VDD");
  //  Subcircuits
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 0, ringoIndex), Qt::DisplayRole).toString ()), "$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (21, 0, ringoIndex), Qt::DisplayRole).toString ()), "$10");
  //  Devices

  //  OUT pin of INV2 has a single child node which is the "OUT" net
  QModelIndex inv2PinOutIndex = model->index (2, 0, inv2Index);
  EXPECT_EQ (model->hasChildren (inv2PinOutIndex), true);
  EXPECT_EQ (model->rowCount (inv2PinOutIndex), 1);
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PinOutIndex), Qt::DisplayRole).toString ()), "NOUT");

  QModelIndex inv2PinOutIndexNet = model->index (0, 0, inv2PinOutIndex);
  EXPECT_EQ (model->hasChildren (inv2PinOutIndexNet), false);
  EXPECT_EQ (model->rowCount (inv2PinOutIndexNet), 0);

  //  NOUT net has 1 pin, 2 devices
  QModelIndex inv2NOutIndex = model->index (7, 0, inv2Index);
  EXPECT_EQ (model->hasChildren (inv2NOutIndex), true);
  EXPECT_EQ (model->rowCount (inv2NOutIndex), 3);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2NOutIndex), Qt::DisplayRole).toString ()), "D");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2NOutIndex), Qt::DisplayRole).toString ()), "D");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, inv2NOutIndex), Qt::DisplayRole).toString ()), "OUT");

  //  no children for pins on nets
  QModelIndex inv2NOutPinOutIndex = model->index (2, 0, inv2NOutIndex);
  EXPECT_EQ (model->hasChildren (inv2NOutPinOutIndex), false);
  EXPECT_EQ (model->rowCount (inv2NOutPinOutIndex), 0);

  //  a MOS3 transistor has three other terminals
  QModelIndex inv2NOutDeviceIndex = model->index (0, 0, inv2NOutIndex);
  EXPECT_EQ (model->hasChildren (inv2NOutDeviceIndex), true);
  EXPECT_EQ (model->rowCount (inv2NOutDeviceIndex), 3);



}

