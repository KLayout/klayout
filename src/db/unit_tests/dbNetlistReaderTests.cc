
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

#include "dbNetlistSpiceReader.h"
#include "dbNetlist.h"
#include "dbNetlistDeviceClasses.h"

#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

TEST(1_BasicReader)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader1.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device RES $1 (A='6',B='1') (R=7650);\n"
    "  device RES $2 (A='3',B='1') (R=7650);\n"
    "  device RES $3 (A='3',B='2') (R=2670);\n"
    "  device MHVPMOS $4 (S='6',G='4',D='7',B='7') (L=0.25,W=1.5,AS=0.63,AD=0.63,PS=3.84,PD=3.84);\n"
    "end;\n"
  );
}

TEST(2_ReaderWithSubcircuits)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader2.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit RINGO ($1='11',$2='12',$3='13',$4='14',$5='15');\n"
    "  subcircuit ND2X1 $1 ($1='12',$2='1',$3='15',$4='12',$5='11',$6='14',$7='15');\n"
    "  subcircuit INVX1 $2 ($1='12',$2='2',$3='15',$4='12',$5='1',$6='15');\n"
    "  subcircuit INVX1 $3 ($1='12',$2='3',$3='15',$4='12',$5='2',$6='15');\n"
    "  subcircuit INVX1 $4 ($1='12',$2='4',$3='15',$4='12',$5='3',$6='15');\n"
    "  subcircuit INVX1 $5 ($1='12',$2='5',$3='15',$4='12',$5='4',$6='15');\n"
    "  subcircuit INVX1 $6 ($1='12',$2='6',$3='15',$4='12',$5='5',$6='15');\n"
    "  subcircuit INVX1 $7 ($1='12',$2='7',$3='15',$4='12',$5='6',$6='15');\n"
    "  subcircuit INVX1 $8 ($1='12',$2='8',$3='15',$4='12',$5='7',$6='15');\n"
    "  subcircuit INVX1 $9 ($1='12',$2='9',$3='15',$4='12',$5='8',$6='15');\n"
    "  subcircuit INVX1 $10 ($1='12',$2='10',$3='15',$4='12',$5='9',$6='15');\n"
    "  subcircuit INVX1 $11 ($1='12',$2='11',$3='15',$4='12',$5='10',$6='15');\n"
    "  subcircuit INVX1 $12 ($1='12',$2='13',$3='15',$4='12',$5='11',$6='15');\n"
    "end;\n"
    "circuit ND2X1 ($1='1',$2='2',$3='3',$4='4',$5='5',$6='6',$7='7');\n"
    "  device MLVPMOS $1 (S='2',G='6',D='1',B='4') (L=0.25,W=1.5,AS=0.6375,AD=0.3375,PS=3.85,PD=1.95);\n"
    "  device MLVPMOS $2 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0.3375,AD=0.6375,PS=1.95,PD=3.85);\n"
    "  device MLVNMOS $3 (S='3',G='6',D='8',B='7') (L=0.25,W=0.95,AS=0.40375,AD=0.21375,PS=2.75,PD=1.4);\n"
    "  device MLVNMOS $4 (S='8',G='5',D='2',B='7') (L=0.25,W=0.95,AS=0.21375,AD=0.40375,PS=1.4,PD=2.75);\n"
    "end;\n"
    "circuit INVX1 ($1='1',$2='2',$3='3',$4='4',$5='5',$6='6');\n"
    "  device MLVPMOS $1 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0.6375,AD=0.6375,PS=3.85,PD=3.85);\n"
    "  device MLVNMOS $2 (S='3',G='5',D='2',B='6') (L=0.25,W=0.95,AS=0.40375,AD=0.40375,PS=2.75,PD=2.75);\n"
    "end;\n"
  );
}

TEST(3_ReaderWithSubcircuitsAltOrder)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader3.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit INVX1 ($1='1',$2='2',$3='3',$4='4',$5='5',$6='6');\n"
    "  device MLVPMOS $1 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $2 (S='3',G='5',D='2',B='6') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit ND2X1 ($1='1',$2='2',$3='3',$4='4',$5='5',$6='6',$7='7');\n"
    "  device MLVPMOS $1 (S='2',G='6',D='1',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVPMOS $2 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $3 (S='3',G='6',D='8',B='7') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $4 (S='8',G='5',D='2',B='7') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit RINGO ($1='11',$2='12',$3='13',$4='14',$5='15');\n"
    "  subcircuit ND2X1 $1 ($1='12',$2='1',$3='15',$4='12',$5='11',$6='14',$7='15');\n"
    "  subcircuit INVX1 $2 ($1='12',$2='2',$3='15',$4='12',$5='1',$6='15');\n"
    "  subcircuit INVX1 $3 ($1='12',$2='3',$3='15',$4='12',$5='2',$6='15');\n"
    "  subcircuit INVX1 $4 ($1='12',$2='4',$3='15',$4='12',$5='3',$6='15');\n"
    "  subcircuit INVX1 $5 ($1='12',$2='5',$3='15',$4='12',$5='4',$6='15');\n"
    "  subcircuit INVX1 $6 ($1='12',$2='6',$3='15',$4='12',$5='5',$6='15');\n"
    "  subcircuit INVX1 $7 ($1='12',$2='7',$3='15',$4='12',$5='6',$6='15');\n"
    "  subcircuit INVX1 $8 ($1='12',$2='8',$3='15',$4='12',$5='7',$6='15');\n"
    "  subcircuit INVX1 $9 ($1='12',$2='9',$3='15',$4='12',$5='8',$6='15');\n"
    "  subcircuit INVX1 $10 ($1='12',$2='10',$3='15',$4='12',$5='9',$6='15');\n"
    "  subcircuit INVX1 $11 ($1='12',$2='11',$3='15',$4='12',$5='10',$6='15');\n"
    "  subcircuit INVX1 $12 ($1='12',$2='13',$3='15',$4='12',$5='11',$6='15');\n"
    "end;\n"
  );
}
