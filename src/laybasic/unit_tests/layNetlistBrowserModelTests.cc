
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include "layNetlistBrowserModel.h"
#include "tlUnitTest.h"

TEST (1)
{
  db::LayoutToNetlist l2n;
  l2n.load (tl::testsrc () + "/testdata/lay/l2n_browser.l2n");

  lay::NetColorizer colorizer;
  std::auto_ptr<lay::NetlistBrowserModel> model (new lay::NetlistBrowserModel (0, &l2n, &colorizer));

  EXPECT_EQ (model->hasChildren (QModelIndex ()), true);
  //  two circuits
  EXPECT_EQ (model->rowCount (QModelIndex ()), 2);
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, QModelIndex ()), Qt::UserRole).toString ()), "INV2");
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
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Index), Qt::UserRole).toString ()), "IN");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Index), Qt::DisplayRole).toString ()), "IN");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2Index), Qt::DisplayRole).toString ()), "$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, inv2Index), Qt::DisplayRole).toString ()), "OUT");
  EXPECT_EQ (tl::to_string (model->data (model->index (3, 0, inv2Index), Qt::DisplayRole).toString ()), "$3");
  EXPECT_EQ (tl::to_string (model->data (model->index (4, 0, inv2Index), Qt::DisplayRole).toString ()), "$4");
  //  Nets
  EXPECT_EQ (tl::to_string (model->data (model->index (5, 0, inv2Index), Qt::UserRole).toString ()), "NIN");
  EXPECT_EQ (tl::to_string (model->data (model->index (5, 0, inv2Index), Qt::DisplayRole).toString ()), "NIN");
  EXPECT_EQ (tl::to_string (model->data (model->index (5, 2, inv2Index), Qt::DisplayRole).toString ()), "NIN (3)");
  EXPECT_EQ (tl::to_string (model->data (model->index (6, 0, inv2Index), Qt::DisplayRole).toString ()), "NOUT");
  EXPECT_EQ (tl::to_string (model->data (model->index (6, 2, inv2Index), Qt::DisplayRole).toString ()), "NOUT (3)");
  EXPECT_EQ (tl::to_string (model->data (model->index (7, 0, inv2Index), Qt::DisplayRole).toString ()), "$2");
  EXPECT_EQ (tl::to_string (model->data (model->index (7, 2, inv2Index), Qt::DisplayRole).toString ()), "$2 (5)");
  EXPECT_EQ (tl::to_string (model->data (model->index (8, 0, inv2Index), Qt::DisplayRole).toString ()), "$4");
  EXPECT_EQ (tl::to_string (model->data (model->index (8, 2, inv2Index), Qt::DisplayRole).toString ()), "$4 (3)");
  EXPECT_EQ (tl::to_string (model->data (model->index (9, 0, inv2Index), Qt::DisplayRole).toString ()), "$5");
  EXPECT_EQ (tl::to_string (model->data (model->index (9, 2, inv2Index), Qt::DisplayRole).toString ()), "$5 (3)");
  //  No Subcircuits
  //  Devices
  EXPECT_EQ (tl::to_string (model->data (model->index (10, 0, inv2Index), Qt::UserRole).toString ()), "$1|PMOS");
  EXPECT_EQ (tl::to_string (model->data (model->index (10, 0, inv2Index), Qt::DisplayRole).toString ()), "PMOS [L=0.25, W=0.95, AS=0.49875, AD=0.26125, PS=2.95, PD=1.5]");
  EXPECT_EQ (tl::to_string (model->data (model->index (10, 2, inv2Index), Qt::DisplayRole).toString ()), "$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (11, 0, inv2Index), Qt::DisplayRole).toString ()), "PMOS [L=0.25, W=0.95, AS=0.26125, AD=0.49875, PS=1.5, PD=2.95]");
  EXPECT_EQ (tl::to_string (model->data (model->index (11, 2, inv2Index), Qt::DisplayRole).toString ()), "$2");
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 0, inv2Index), Qt::DisplayRole).toString ()), "NMOS [L=0.25, W=0.95, AS=0.49875, AD=0.26125, PS=2.95, PD=1.5]");
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 2, inv2Index), Qt::DisplayRole).toString ()), "$3");
  EXPECT_EQ (tl::to_string (model->data (model->index (13, 0, inv2Index), Qt::DisplayRole).toString ()), "NMOS [L=0.25, W=0.95, AS=0.26125, AD=0.49875, PS=1.5, PD=2.95]");
  EXPECT_EQ (tl::to_string (model->data (model->index (13, 2, inv2Index), Qt::DisplayRole).toString ()), "$4");

  EXPECT_EQ (model->hasChildren (ringoIndex), true);
  //  0 pins, 12 nets, 10 subcircuits, 0 devices
  EXPECT_EQ (model->rowCount (ringoIndex), 22);
  //  Pins
  //  Nets
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoIndex), Qt::UserRole).toString ()), "FB");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, ringoIndex), Qt::DisplayRole).toString ()), "FB (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 2, ringoIndex), Qt::DisplayRole).toString ()), "VDD (10)");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 2, ringoIndex), Qt::DisplayRole).toString ()), "VSS (10)");
  EXPECT_EQ (tl::to_string (model->data (model->index (3, 2, ringoIndex), Qt::DisplayRole).toString ()), "$4 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (4, 2, ringoIndex), Qt::DisplayRole).toString ()), "$5 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (5, 2, ringoIndex), Qt::DisplayRole).toString ()), "$6 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (6, 2, ringoIndex), Qt::DisplayRole).toString ()), "$7 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (7, 2, ringoIndex), Qt::DisplayRole).toString ()), "$8 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (8, 2, ringoIndex), Qt::DisplayRole).toString ()), "$9 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (9, 2, ringoIndex), Qt::DisplayRole).toString ()), "$10 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (10, 2, ringoIndex), Qt::DisplayRole).toString ()), "$11 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (11, 2, ringoIndex), Qt::DisplayRole).toString ()), "$12 (2)");
  //  Subcircuits
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 0, ringoIndex), Qt::UserRole).toString ()), "INV2|$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 0, ringoIndex), Qt::DisplayRole).toString ()), "<a href='int:circuit?id=0'>INV2</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 2, ringoIndex), Qt::DisplayRole).toString ()), "$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (21, 0, ringoIndex), Qt::DisplayRole).toString ()), "<a href='int:circuit?id=0'>INV2</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (21, 2, ringoIndex), Qt::DisplayRole).toString ()), "$10");
  //  Devices

  //  OUT pin of INV2 has a single child node which is the "NOUT" net
  QModelIndex inv2PinOutIndex = model->index (2, 0, inv2Index);
  EXPECT_EQ (model->parent (inv2PinOutIndex) == inv2Index, true);
  EXPECT_EQ (model->hasChildren (inv2PinOutIndex), true);
  EXPECT_EQ (model->rowCount (inv2PinOutIndex), 1);
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PinOutIndex), Qt::DisplayRole).toString ()), "NOUT");

  QModelIndex inv2PinOutIndexNet = model->index (0, 0, inv2PinOutIndex);
  EXPECT_EQ (model->parent (inv2PinOutIndexNet) == inv2PinOutIndex, true);
  EXPECT_EQ (model->hasChildren (inv2PinOutIndexNet), false);
  EXPECT_EQ (model->rowCount (inv2PinOutIndexNet), 0);

  //  NOUT net has 1 pin, 2 devices, 0 subcircuits
  QModelIndex inv2NOutIndex = model->index (6, 0, inv2Index);
  EXPECT_EQ (model->parent (inv2NOutIndex) == inv2Index, true);
  EXPECT_EQ (model->hasChildren (inv2NOutIndex), true);
  EXPECT_EQ (model->rowCount (inv2NOutIndex), 3);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2NOutIndex), Qt::UserRole).toString ()), "D|PMOS|$2");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2NOutIndex), Qt::DisplayRole).toString ()), "D / PMOS [L=0.25, W=0.95, AS=0.26125, AD=0.49875, PS=1.5, PD=2.95]");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, inv2NOutIndex), Qt::DisplayRole).toString ()), "<a href='int:device?id=24'>$2</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2NOutIndex), Qt::DisplayRole).toString ()), "D / NMOS [L=0.25, W=0.95, AS=0.26125, AD=0.49875, PS=1.5, PD=2.95]");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 2, inv2NOutIndex), Qt::DisplayRole).toString ()), "<a href='int:device?id=56'>$4</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, inv2NOutIndex), Qt::DisplayRole).toString ()), "<a href='int:pin?id=18'>OUT</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 2, inv2NOutIndex), Qt::DisplayRole).toString ()), "");

  //  no children for pins on nets
  QModelIndex inv2NOutPinOutIndex = model->index (2, 0, inv2NOutIndex);
  EXPECT_EQ (model->parent (inv2NOutPinOutIndex) == inv2NOutIndex, true);
  EXPECT_EQ (model->hasChildren (inv2NOutPinOutIndex), false);
  EXPECT_EQ (model->rowCount (inv2NOutPinOutIndex), 0);

  //  a MOS3 transistor has three other terminals
  QModelIndex inv2NOutDeviceIndex = model->index (0, 0, inv2NOutIndex);
  EXPECT_EQ (model->parent (inv2NOutDeviceIndex) == inv2NOutIndex, true);
  EXPECT_EQ (model->hasChildren (inv2NOutDeviceIndex), true);
  EXPECT_EQ (model->rowCount (inv2NOutDeviceIndex), 3);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2NOutDeviceIndex), Qt::UserRole).toString ()), "S|$5");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2NOutDeviceIndex), Qt::DisplayRole).toString ()), "S");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2NOutDeviceIndex), Qt::DisplayRole).toString ()), "G");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, inv2NOutDeviceIndex), Qt::DisplayRole).toString ()), "D");

  QModelIndex inv2NOutDeviceGateIndex = model->index (1, 0, inv2NOutDeviceIndex);
  EXPECT_EQ (model->parent (inv2NOutDeviceGateIndex) == inv2NOutDeviceIndex, true);
  EXPECT_EQ (model->hasChildren (inv2NOutDeviceGateIndex), false);
  EXPECT_EQ (model->rowCount (inv2NOutDeviceGateIndex), 0);

  //  FB net has 0 pin, 0 devices, 2 subcircuits
  QModelIndex ringoFbIndex = model->index (0, 0, ringoIndex);
  EXPECT_EQ (model->parent (ringoFbIndex) == ringoIndex, true);
  EXPECT_EQ (model->hasChildren (ringoFbIndex), true);
  EXPECT_EQ (model->rowCount (ringoFbIndex), 2);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoFbIndex), Qt::UserRole).toString ()), "IN|INV2|$2");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoFbIndex), Qt::DisplayRole).toString ()), "<a href='int:pin?id=2'>IN</a> / <a href='int:circuit?id=0'>INV2</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, ringoFbIndex), Qt::DisplayRole).toString ()), "<a href='int:subcircuit?id=23'>$2</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, ringoFbIndex), Qt::DisplayRole).toString ()), "<a href='int:pin?id=34'>$1</a> / <a href='int:circuit?id=0'>INV2</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 2, ringoFbIndex), Qt::DisplayRole).toString ()), "<a href='int:subcircuit?id=7'>$1</a>");

  QModelIndex ringoFbSubcircuit2Index = model->index (0, 0, ringoFbIndex);
  EXPECT_EQ (model->parent (ringoFbSubcircuit2Index) == ringoFbIndex, true);
  EXPECT_EQ (model->hasChildren (ringoFbSubcircuit2Index), true);
  EXPECT_EQ (model->rowCount (ringoFbSubcircuit2Index), 5);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoFbSubcircuit2Index), Qt::UserRole).toString ()), "IN|NIN");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=2'>IN</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:net?id=5'>FB</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=34'>$1</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 2, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=18'>OUT</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 2, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:net?id=53'>$4</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (3, 0, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=50'>$3</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (3, 2, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:net?id=37'>VSS</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (4, 0, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=66'>$4</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (4, 2, ringoFbSubcircuit2Index), Qt::DisplayRole).toString ()), "<a href='int:net?id=21'>VDD</a>");

  QModelIndex ringoFbSubcircuit2InPinIndex = model->index (1, 0, ringoFbSubcircuit2Index);
  EXPECT_EQ (model->parent (ringoFbSubcircuit2InPinIndex) == ringoFbSubcircuit2Index, true);
  EXPECT_EQ (model->hasChildren (ringoFbSubcircuit2InPinIndex), false);
  EXPECT_EQ (model->rowCount (ringoFbSubcircuit2InPinIndex), 0);

  //  Subcircuit 1 of RINGO has 5 pins

  QModelIndex ringoSubcircuit1Index = model->index (12, 0, ringoIndex);
  EXPECT_EQ (model->parent (ringoSubcircuit1Index) == ringoIndex, true);
  EXPECT_EQ (model->hasChildren (ringoSubcircuit1Index), true);
  EXPECT_EQ (model->rowCount (ringoSubcircuit1Index), 5);

  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, ringoSubcircuit1Index), Qt::UserRole).toString ()), "OUT");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 0, ringoSubcircuit1Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=18'>OUT</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (2, 2, ringoSubcircuit1Index), Qt::DisplayRole).toString ()), "");

  QModelIndex ringoSubcircuit1OutPinIndex = model->index (2, 0, ringoSubcircuit1Index);
  EXPECT_EQ (model->parent (ringoSubcircuit1OutPinIndex) == ringoSubcircuit1Index, true);
  EXPECT_EQ (model->hasChildren (ringoSubcircuit1OutPinIndex), false);
  EXPECT_EQ (model->rowCount (ringoSubcircuit1OutPinIndex), 0);

  //  Device 1 of INV2 has 3 pins

  QModelIndex inv2Device1Index = model->index (10, 0, inv2Index);
  EXPECT_EQ (model->parent (inv2Device1Index) == inv2Index, true);
  EXPECT_EQ (model->hasChildren (inv2Device1Index), true);
  EXPECT_EQ (model->rowCount (inv2Device1Index), 3);

  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2Device1Index), Qt::UserRole).toString ()), "G|NIN");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2Device1Index), Qt::DisplayRole).toString ()), "G");

  QModelIndex inv2Device1GateIndex = model->index (1, 0, inv2Device1Index);
  EXPECT_EQ (model->parent (inv2Device1GateIndex) == inv2Device1Index, true);
  EXPECT_EQ (model->hasChildren (inv2Device1GateIndex), false);
  EXPECT_EQ (model->rowCount (inv2Device1GateIndex), 0);
}

TEST (2)
{
  db::LayoutVsSchematic lvs;
  lvs.load (tl::testsrc () + "/testdata/lay/lvsdb_browser.lvsdb");

  lay::NetColorizer colorizer;
  std::auto_ptr<lay::NetlistBrowserModel> model (new lay::NetlistBrowserModel (0, &lvs, &colorizer));

  EXPECT_EQ (model->hasChildren (QModelIndex ()), true);
  //  two circuits
  EXPECT_EQ (model->rowCount (QModelIndex ()), 4);
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, QModelIndex ()), Qt::UserRole).toString ()), "INV2PAIRX");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, QModelIndex ()), Qt::DisplayRole).toString ()), "- ⇔ INV2PAIRX");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, QModelIndex ()), Qt::DisplayRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 3, QModelIndex ()), Qt::DisplayRole).toString ()), "INV2PAIRX");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, QModelIndex ()), Qt::DisplayRole).toString ()), "INV2");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 2, QModelIndex ()), Qt::DisplayRole).toString ()), "INV2");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 3, QModelIndex ()), Qt::DisplayRole).toString ()), "INV2");
  EXPECT_EQ (model->parent (model->index (0, 0, QModelIndex ())).isValid (), false);
  EXPECT_EQ (model->parent (model->index (1, 0, QModelIndex ())).isValid (), false);

  EXPECT_EQ (model->hasChildren (model->index (0, 0, QModelIndex ())), false);
  EXPECT_EQ (model->rowCount (model->index (0, 0, QModelIndex ())), 0);

  QModelIndex inv2Index = model->index (1, 0, QModelIndex ());

  //  INV2 circuit node
  EXPECT_EQ (model->hasChildren (inv2Index), true);
  EXPECT_EQ (model->rowCount (inv2Index), 14);
  EXPECT_EQ (model->parent (inv2Index).isValid (), false);

  //  first of pins in INV2 circuit
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Index), Qt::UserRole).toString ()), "$0|$0");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Index), Qt::DisplayRole).toString ()), "$0");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, inv2Index), Qt::DisplayRole).toString ()), "$0");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 3, inv2Index), Qt::DisplayRole).toString ()), "$0");

  //  INV2, pin 0 node
  QModelIndex inv2Pin0Index = model->index (0, 0, inv2Index);
  EXPECT_EQ (model->hasChildren (inv2Pin0Index), true);
  EXPECT_EQ (model->rowCount (inv2Pin0Index), 1);
  EXPECT_EQ (model->parent (inv2Pin0Index) == inv2Index, true);

  //  INV2, pin 0 has one net node
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Pin0Index), Qt::UserRole).toString ()), "$1|1");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Pin0Index), Qt::DisplayRole).toString ()), "$1 ⇔ 1");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, inv2Pin0Index), Qt::DisplayRole).toString ()), "<a href='int:net?id=9'>$1</a>");
  std::pair<const db::Net *, const db::Net *> nets = model->net_from_index (model->index_from_id ((void *) 9, 0));
  EXPECT_EQ (nets.first != 0, true);
  if (nets.first != 0) {
    EXPECT_EQ (nets.first->expanded_name (), "$1");
  }
  EXPECT_EQ (nets.second != 0, true);
  if (nets.second != 0) {
    EXPECT_EQ (nets.second->expanded_name (), "1");
  }
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 3, inv2Pin0Index), Qt::DisplayRole).toString ()), "<a href='int:net?id=9'>1</a>");

  //  first of nets in INV2 circuit
  EXPECT_EQ (tl::to_string (model->data (model->index (6, 0, inv2Index), Qt::UserRole).toString ()), "$1|1");
  EXPECT_EQ (tl::to_string (model->data (model->index (6, 0, inv2Index), Qt::DisplayRole).toString ()), "$1 ⇔ 1");
  EXPECT_EQ (tl::to_string (model->data (model->index (6, 2, inv2Index), Qt::DisplayRole).toString ()), "$1 (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (6, 3, inv2Index), Qt::DisplayRole).toString ()), "1 (2)");

  //  INV2, net 1 node
  QModelIndex inv2Net0Index = model->index (6, 0, inv2Index);
  EXPECT_EQ (model->hasChildren (inv2Net0Index), true);
  EXPECT_EQ (model->rowCount (inv2Net0Index), 2);
  EXPECT_EQ (model->parent (inv2Net0Index) == inv2Index, true);

  //  INV2, net 1 has one pin and one terminal at BULK
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Net0Index), Qt::UserRole).toString ()), "B|B|PMOS|PMOS|$1|$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2Net0Index), Qt::DisplayRole).toString ()), "B / PMOS [L=0.25, W=3.5]");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, inv2Net0Index), Qt::DisplayRole).toString ()), "<a href='int:device?id=17'>$1</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 3, inv2Net0Index), Qt::DisplayRole).toString ()), "<a href='int:device?id=17'>$1</a>");

  //  This terminal connects to a device with four other terminals ..
  QModelIndex inv2Net0TerminalIndex = model->index (0, 0, inv2Net0Index);
  EXPECT_EQ (model->hasChildren (inv2Net0TerminalIndex), true);
  EXPECT_EQ (model->rowCount (inv2Net0TerminalIndex), 4);
  EXPECT_EQ (model->parent (inv2Net0TerminalIndex) == inv2Net0Index, true);
  //  .. whose second terminal is gate
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2Net0TerminalIndex), Qt::UserRole).toString ()), "G|G|IN|2");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2Net0TerminalIndex), Qt::DisplayRole).toString ()), "G");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 2, inv2Net0TerminalIndex), Qt::DisplayRole).toString ()), "<a href='int:net?id=73'>IN</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 3, inv2Net0TerminalIndex), Qt::DisplayRole).toString ()), "<a href='int:net?id=73'>2</a>");

  //  The Pin
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2Net0Index), Qt::UserRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2Net0Index), Qt::DisplayRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 2, inv2Net0Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=5'>$0</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 3, inv2Net0Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=5'>$0</a>");

  //  This pin does not have children
  QModelIndex inv2Net0PinIndex = model->index (1, 0, inv2Net0Index);
  EXPECT_EQ (model->hasChildren (inv2Net0PinIndex), false);
  EXPECT_EQ (model->rowCount (inv2Net0PinIndex), 0);
  EXPECT_EQ (model->parent (inv2Net0PinIndex) == inv2Net0Index, true);

  //  second of nets in INV2 circuit
  EXPECT_EQ (tl::to_string (model->data (model->index (7, 0, inv2Index), Qt::UserRole).toString ()), "BULK|6");
  EXPECT_EQ (tl::to_string (model->data (model->index (7, 0, inv2Index), Qt::DisplayRole).toString ()), "BULK ⇔ 6");
  EXPECT_EQ (tl::to_string (model->data (model->index (7, 2, inv2Index), Qt::DisplayRole).toString ()), "BULK (2)");
  EXPECT_EQ (tl::to_string (model->data (model->index (7, 3, inv2Index), Qt::DisplayRole).toString ()), "6 (2)");

  //  first of devices in INV2 circuit
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 0, inv2Index), Qt::UserRole).toString ()), "$1|$1|PMOS|PMOS");
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 0, inv2Index), Qt::DisplayRole).toString ()), "PMOS");
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 2, inv2Index), Qt::DisplayRole).toString ()), "$1 / PMOS [L=0.25, W=3.5]");
  EXPECT_EQ (tl::to_string (model->data (model->index (12, 3, inv2Index), Qt::DisplayRole).toString ()), "$1 / PMOS [L=0.25, W=3.5]");

  QModelIndex inv2PairIndex = model->index (2, 0, QModelIndex ());
  EXPECT_EQ (model->parent (inv2PairIndex).isValid (), false);

  //  INV2PAIR circuit node
  EXPECT_EQ (model->hasChildren (inv2PairIndex), true);
  EXPECT_EQ (model->rowCount (inv2PairIndex), 18);

  //  first of pins in INV2 circuit
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairIndex), Qt::UserRole).toString ()), "$4");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairIndex), Qt::DisplayRole).toString ()), "- ⇔ $4");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, inv2PairIndex), Qt::DisplayRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 3, inv2PairIndex), Qt::DisplayRole).toString ()), "$4");

  //  INV2, pin 0 node
  QModelIndex inv2PairPin0Index = model->index (0, 0, inv2PairIndex);
  EXPECT_EQ (model->hasChildren (inv2PairPin0Index), true);
  EXPECT_EQ (model->rowCount (inv2PairPin0Index), 1);
  EXPECT_EQ (model->parent (inv2PairPin0Index) == inv2PairIndex, true);

  //  INV2, pin 0 has one net node
  //  The pin isnt't connected to any net, left side because there is no match, right side because the pin isn't connected
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairPin0Index), Qt::UserRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairPin0Index), Qt::DisplayRole).toString ()), "-");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, inv2PairPin0Index), Qt::DisplayRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 3, inv2PairPin0Index), Qt::DisplayRole).toString ()), "");

  //  first of nets in INV2 circuit
  EXPECT_EQ (tl::to_string (model->data (model->index (8, 0, inv2PairIndex), Qt::UserRole).toString ()), "$4");
  EXPECT_EQ (tl::to_string (model->data (model->index (8, 0, inv2PairIndex), Qt::DisplayRole).toString ()), "$4 ⇔ -");
  EXPECT_EQ (tl::to_string (model->data (model->index (8, 2, inv2PairIndex), Qt::DisplayRole).toString ()), "$4 (3)");
  EXPECT_EQ (tl::to_string (model->data (model->index (8, 3, inv2PairIndex), Qt::DisplayRole).toString ()), "");

  //  This net has only left side which has one pin and two subcircuits
  QModelIndex inv2PairNet0Index = model->index (8, 0, inv2PairIndex);
  EXPECT_EQ (model->hasChildren (inv2PairNet0Index), true);
  EXPECT_EQ (model->rowCount (inv2PairNet0Index), 3);
  EXPECT_EQ (model->parent (inv2PairNet0Index) == inv2PairIndex, true);

  //  The pin
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairNet0Index), Qt::UserRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairNet0Index), Qt::DisplayRole).toString ()), "");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, inv2PairNet0Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=38'>$3</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 3, inv2PairNet0Index), Qt::DisplayRole).toString ()), "");

  //  This pin does not have children
  QModelIndex inv2PairNet0Pin0Index = model->index (0, 0, inv2PairNet0Index);
  EXPECT_EQ (model->hasChildren (inv2PairNet0Pin0Index), false);
  EXPECT_EQ (model->rowCount (inv2PairNet0Pin0Index), 0);
  EXPECT_EQ (model->parent (inv2PairNet0Pin0Index) == inv2PairNet0Index, true);

  //  The first subcircuit
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2PairNet0Index), Qt::UserRole).toString ()), "OUT|INV2|$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 0, inv2PairNet0Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=101'>OUT ⇔ -</a> / <a href='int:circuit?id=1'>INV2 ⇔ -</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 2, inv2PairNet0Index), Qt::DisplayRole).toString ()), "<a href='int:subcircuit?id=46'>$1</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (1, 3, inv2PairNet0Index), Qt::DisplayRole).toString ()), "");

  //  This subcircuit has 6 other pins
  QModelIndex inv2PairNet0SubCircuit0Index = model->index (1, 0, inv2PairNet0Index);
  EXPECT_EQ (model->hasChildren (inv2PairNet0SubCircuit0Index), true);
  EXPECT_EQ (model->rowCount (inv2PairNet0SubCircuit0Index), 6);
  EXPECT_EQ (model->parent (inv2PairNet0SubCircuit0Index) == inv2PairNet0Index, true);

  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairNet0SubCircuit0Index), Qt::UserRole).toString ()), "$1");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 0, inv2PairNet0SubCircuit0Index), Qt::DisplayRole).toString ()), "<a href='int:pin?id=5'>$0</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 2, inv2PairNet0SubCircuit0Index), Qt::DisplayRole).toString ()), "<a href='int:net?id=170'>$7</a>");
  EXPECT_EQ (tl::to_string (model->data (model->index (0, 3, inv2PairNet0SubCircuit0Index), Qt::DisplayRole).toString ()), "");
}
