
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

#include "dbNetlistSpiceReader.h"
#include "dbNetlistSpiceReaderDelegate.h"
#include "dbNetlistSpiceReaderExpressionParser.h"
#include "dbNetlist.h"
#include "dbNetlistDeviceClasses.h"

#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

TEST(1_BasicReader)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader1.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ('1'='1','2'='2','4'='4','7'='7');\n"
    "  device RES $1 (A='6',B='1') (R=7650,L=0,W=0,A=0,P=0);\n"
    "  device RES $2 (A='3',B='1') (R=7650,L=0,W=0,A=0,P=0);\n"
    "  device RES $3 (A='3',B='2') (R=2670,L=0,W=0,A=0,P=0);\n"
    "  device MHVPMOS $4 (S='6',G='4',D='7',B='7') (L=0.25,W=1.5,AS=0.63,AD=0.63,PS=3.84,PD=3.84);\n"
    "end;\n"
  );
}

TEST(1b_BasicReader_ResistorModels)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader1b.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ('1'='1','2'='2','4'='4','7'='7');\n"
    "  device M1 $1 (A='6',B='1') (R=7650,L=0,W=0,A=0,P=0);\n"
    "  device M2 $2 (A='3',B='1') (R=7650,L=0,W=0,A=0,P=0);\n"
    "  device M3 $3 (A='3',B='2') (R=2670,L=0,W=0,A=0,P=0);\n"
    "  device MHVPMOS $4 (S='6',G='4',D='7',B='7') (L=0.25,W=1.5,AS=0.63,AD=0.63,PS=3.84,PD=3.84);\n"
    "end;\n"
  );
}

TEST(2_ReaderWithSubcircuits)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader2.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit RINGO ('11'='11','12'='12','13'='13','14'='14','15'='15');\n"
    "  subcircuit ND2X1 $1 ('1'='12','2'='1','3'='15','4'='12','5'='11','6'='14','7'='15');\n"
    "  subcircuit INVX1 $2 ('1'='12','2'='2','3'='15','4'='12','5'='1','6'='15');\n"
    "  subcircuit INVX1 $3 ('1'='12','2'='3','3'='15','4'='12','5'='2','6'='15');\n"
    "  subcircuit INVX1 $4 ('1'='12','2'='4','3'='15','4'='12','5'='3','6'='15');\n"
    "  subcircuit INVX1 $5 ('1'='12','2'='5','3'='15','4'='12','5'='4','6'='15');\n"
    "  subcircuit INVX1 $6 ('1'='12','2'='6','3'='15','4'='12','5'='5','6'='15');\n"
    "  subcircuit INVX1 $7 ('1'='12','2'='7','3'='15','4'='12','5'='6','6'='15');\n"
    "  subcircuit INVX1 $8 ('1'='12','2'='8','3'='15','4'='12','5'='7','6'='15');\n"
    "  subcircuit INVX1 $9 ('1'='12','2'='9','3'='15','4'='12','5'='8','6'='15');\n"
    "  subcircuit INVX1 $10 ('1'='12','2'='10','3'='15','4'='12','5'='9','6'='15');\n"
    "  subcircuit INVX1 $11 ('1'='12','2'='11','3'='15','4'='12','5'='10','6'='15');\n"
    "  subcircuit INVX1 $12 ('1'='12','2'='13','3'='15','4'='12','5'='11','6'='15');\n"
    "end;\n"
    "circuit ND2X1 ('1'='1','2'='2','3'='3','4'='4','5'='5','6'='6','7'='7');\n"
    "  device MLVPMOS $1 (S='2',G='6',D='1',B='4') (L=0.25,W=1.5,AS=0.6375,AD=0.3375,PS=3.85,PD=1.95);\n"
    "  device MLVPMOS $2 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0.3375,AD=0.6375,PS=1.95,PD=3.85);\n"
    "  device MLVNMOS $3 (S='3',G='6',D='8',B='7') (L=0.25,W=0.95,AS=0.40375,AD=0.21375,PS=2.75,PD=1.4);\n"
    "  device MLVNMOS $4 (S='8',G='5',D='2',B='7') (L=0.25,W=0.95,AS=0.21375,AD=0.40375,PS=1.4,PD=2.75);\n"
    "end;\n"
    "circuit INVX1 ('1'='1','2'='2','3'='3','4'='4','5'='5','6'='6');\n"
    "  device MLVPMOS $1 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0.6375,AD=0.6375,PS=3.85,PD=3.85);\n"
    "  device MLVNMOS $2 (S='3',G='5',D='2',B='6') (L=0.25,W=0.95,AS=0.40375,AD=0.40375,PS=2.75,PD=2.75);\n"
    "end;\n"
  );
}

TEST(3_ReaderWithSubcircuitsAltOrder)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader3.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit INVX1 ('1'='1','2'='2','3'='3','4'='4','5'='5','6'='6');\n"
    "  device MLVPMOS $1 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $2 (S='3',G='5',D='2',B='6') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit ND2X1 ('1'='1','2'='2','3'='3','4'='4','5'='5','6'='6','7'='7');\n"
    "  device MLVPMOS $1 (S='2',G='6',D='1',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVPMOS $2 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $3 (S='3',G='6',D='8',B='7') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $4 (S='8',G='5',D='2',B='7') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit RINGO ('11'='11','12'='12','13'='13','14'='14','15'='15');\n"
    "  subcircuit ND2X1 $1 ('1'='12','2'='1','3'='15','4'='12','5'='11','6'='14','7'='15');\n"
    "  subcircuit INVX1 $2 ('1'='12','2'='2','3'='15','4'='12','5'='1','6'='15');\n"
    "  subcircuit INVX1 $3 ('1'='12','2'='3','3'='15','4'='12','5'='2','6'='15');\n"
    "  subcircuit INVX1 $4 ('1'='12','2'='4','3'='15','4'='12','5'='3','6'='15');\n"
    "  subcircuit INVX1 $5 ('1'='12','2'='5','3'='15','4'='12','5'='4','6'='15');\n"
    "  subcircuit INVX1 $6 ('1'='12','2'='6','3'='15','4'='12','5'='5','6'='15');\n"
    "  subcircuit INVX1 $7 ('1'='12','2'='7','3'='15','4'='12','5'='6','6'='15');\n"
    "  subcircuit INVX1 $8 ('1'='12','2'='8','3'='15','4'='12','5'='7','6'='15');\n"
    "  subcircuit INVX1 $9 ('1'='12','2'='9','3'='15','4'='12','5'='8','6'='15');\n"
    "  subcircuit INVX1 $10 ('1'='12','2'='10','3'='15','4'='12','5'='9','6'='15');\n"
    "  subcircuit INVX1 $11 ('1'='12','2'='11','3'='15','4'='12','5'='10','6'='15');\n"
    "  subcircuit INVX1 $12 ('1'='12','2'='13','3'='15','4'='12','5'='11','6'='15');\n"
    "end;\n"
  );
}

TEST(4_ReaderWithUnconnectedPins)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader4.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit RINGO ('1'='1','2'='2','3'='3','4'='4');\n"
    "  subcircuit INV2PAIR $1 ('1'='4','2'='3','3'='4','4'='1','5'='6','6'='2','7'='3');\n"
    "  subcircuit INV2PAIR $2 ('1'='4','2'='3','3'='4','4'='100','5'='1','6'='5','7'='3');\n"
    "  subcircuit INV2PAIR $3 ('1'='4','2'='3','3'='4','4'='101','5'='5','6'='8','7'='3');\n"
    "  subcircuit INV2PAIR $4 ('1'='4','2'='3','3'='4','4'='102','5'='8','6'='7','7'='3');\n"
    "  subcircuit INV2PAIR $5 ('1'='4','2'='3','3'='4','4'='103','5'='7','6'='6','7'='3');\n"
    "end;\n"
    "circuit INV2PAIR ('1'='1','2'='2','3'='3','4'='4','5'='5','6'='6','7'='7');\n"
    "  subcircuit INV2 $1 ('1'='7','2'='5','3'='4','4'='3','5'='2','6'='1');\n"
    "  subcircuit INV2 $2 ('1'='7','2'='4','3'='6','4'='3','5'='2','6'='1');\n"
    "end;\n"
    "circuit INV2 ('1'='1','2'='2','3'='3','4'='4','5'='5','6'='6');\n"
    "  device PMOS $1 (S='5',G='2',D='3',B='1') (L=0.25,W=3.5,AS=1.4,AD=1.4,PS=6.85,PD=6.85);\n"
    "  device NMOS $3 (S='4',G='2',D='3',B='6') (L=0.25,W=3.5,AS=1.4,AD=1.4,PS=6.85,PD=6.85);\n"
    "end;\n"
  );
}

TEST(5_CircuitParameters)
{
  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader5.cir");

  db::NetlistSpiceReader reader;
  reader.set_strict (true);

  try {
    db::Netlist nl;
    tl::InputStream is (path);
    reader.read (is, nl);
    //  strict mode makes this sample fail
    EXPECT_EQ (true, false);
  } catch (...) {
    //  ..
  }

  db::Netlist nl;
  reader.set_strict (false);
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit SUBCKT ($1=$1,'A[5]<1>'='A[5]<1>','V42(%)'='V42(%)',Z=Z,GND=GND,GND$1=GND$1);\n"
    "  subcircuit 'HVPMOS(AD=0.18,AS=0.18,L=0.2,PD=2.16,PS=2.16,W=1)' D_$1 ('1'='V42(%)','2'=$3,'3'=Z,'4'=$1);\n"
    "  subcircuit 'HVPMOS(AD=0.18,AS=0.18,L=0.2,PD=2.16,PS=2.16,W=1)' D_$2 ('1'='V42(%)','2'='A[5]<1>','3'=$3,'4'=$1);\n"
    "  subcircuit 'HVNMOS(AD=0,AS=0,L=1.13,PD=6,PS=6,W=2.12)' D_$3 ('1'=GND,'2'=$3,'3'=GND,'4'=GND$1);\n"
    "  subcircuit 'HVNMOS(AD=0.19,AS=0.19,L=0.4,PD=1.16,PS=1.16,W=0.4)' D_$4 ('1'=GND,'2'=$3,'3'=Z,'4'=GND$1);\n"
    "  subcircuit 'HVNMOS(AD=0.19,AS=0.19,L=0.4,PD=1.76,PS=1.76,W=0.4)' D_$5 ('1'=GND,'2'='A[5]<1>','3'=$3,'4'=GND$1);\n"
    "end;\n"
    "circuit 'HVPMOS(AD=0.18,AS=0.18,L=0.2,PD=2.16,PS=2.16,W=1)' ('1'='1','2'='2','3'='3','4'='4');\n"
    "end;\n"
    "circuit 'HVNMOS(AD=0,AS=0,L=1.13,PD=6,PS=6,W=2.12)' ('1'='1','2'='2','3'='3','4'='4');\n"
    "end;\n"
    "circuit 'HVNMOS(AD=0.19,AS=0.19,L=0.4,PD=1.16,PS=1.16,W=0.4)' ('1'='1','2'='2','3'='3','4'='4');\n"
    "end;\n"
    "circuit 'HVNMOS(AD=0.19,AS=0.19,L=0.4,PD=1.76,PS=1.76,W=0.4)' ('1'='1','2'='2','3'='3','4'='4');\n"
    "end;\n"
  );
}

class MyNetlistReaderDelegate
  : public db::NetlistSpiceReaderDelegate
{
public:
  MyNetlistReaderDelegate () : db::NetlistSpiceReaderDelegate () { }

  bool wants_subcircuit (const std::string &circuit_name)
  {
    return circuit_name == "HVNMOS" || circuit_name == "HVPMOS";
  }

  bool element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const std::map<std::string, tl::Variant> &params)
  {
    if (element == "X") {

      if (nets.size () != 4) {
        error (tl::sprintf ("Device subcircuit '%s' requires four nets", model));
      }

      db::DeviceClass *cls = circuit->netlist ()->device_class_by_name (model);
      if (! cls) {
        cls = new db::DeviceClassMOS4Transistor ();
        cls->set_name (model);
        circuit->netlist ()->add_device_class (cls);
      }

      db::Device *device = new db::Device (cls);
      device->set_name (name);
      circuit->add_device (device);

      device->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_S, nets [0]);
      device->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_G, nets [1]);
      device->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_D, nets [2]);
      device->connect_terminal (db::DeviceClassMOS4Transistor::terminal_id_B, nets [3]);

      const std::vector<db::DeviceParameterDefinition> &td = cls->parameter_definitions ();
      for (std::vector<db::DeviceParameterDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
        auto pi = params.find (i->name ());
        if (pi != params.end ()) {
          device->set_parameter_value (i->id (), pi->second.to_double () * 1.5);
        }
      }

      return true;

    } else {
      return db::NetlistSpiceReaderDelegate::element (circuit, element, name, model, value, nets, params);
    }

  }
};

TEST(6_ReaderWithDelegate)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader6.cir");

  MyNetlistReaderDelegate delegate;
  db::NetlistSpiceReader reader (&delegate);
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  subcircuit SUBCKT SUBCKT ($1=IN,A=OUT,VDD=VDD,Z=Z,GND=VSS,GND$1=VSS);\n"
    "end;\n"
    "circuit SUBCKT ($1=$1,A=A,VDD=VDD,Z=Z,GND=GND,GND$1=GND$1);\n"
    "  device HVPMOS $1 (S=VDD,G=$3,D=Z,B=$1) (L=0.3,W=1.5,AS=0.27,AD=0.27,PS=3.24,PD=3.24);\n"
    "  device HVPMOS $2 (S=VDD,G=A,D=$3,B=$1) (L=0.3,W=1.5,AS=0.27,AD=0.27,PS=3.24,PD=3.24);\n"
    "  device HVNMOS $3 (S=GND,G=$3,D=GND,B=GND$1) (L=1.695,W=3.18,AS=0,AD=0,PS=9,PD=9);\n"
    "  device HVNMOS $4 (S=GND,G=$3,D=Z,B=GND$1) (L=0.6,W=0.6,AS=0.285,AD=0.285,PS=1.74,PD=1.74);\n"
    "  device HVNMOS $5 (S=GND,G=A,D=$3,B=GND$1) (L=0.6,W=0.6,AS=0.285,AD=0.285,PS=2.64,PD=2.64);\n"
    "  device RES $1 (A=A,B=Z) (R=100000,L=0,W=0,A=0,P=0);\n"
    "end;\n"
  );
}

TEST(7_GlobalNets)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader7.cir");

  MyNetlistReaderDelegate delegate;
  db::NetlistSpiceReader reader (&delegate);
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit RINGO (FB=FB,OUT=OUT,ENABLE=ENABLE,VDD=VDD,VSS=VSS);\n"
    "  subcircuit ND2X1 $1 (OUT='1',B=FB,A=ENABLE,VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $2 (OUT='2',IN='1',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $3 (OUT='3',IN='2',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $4 (OUT='4',IN='3',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $5 (OUT='5',IN='4',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $6 (OUT='6',IN='5',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $7 (OUT='7',IN='6',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $8 (OUT='8',IN='7',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $9 (OUT='9',IN='8',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $10 (OUT='10',IN='9',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $11 (OUT=FB,IN='10',VDD=VDD,VSS=VSS);\n"
    "  subcircuit INVX1 $12 (OUT=OUT,IN=FB,VDD=VDD,VSS=VSS);\n"
    "end;\n"
    "circuit ND2X1 (OUT=OUT,B=B,A=A,VDD=VDD,VSS=VSS);\n"
    "  device MLVPMOS $1 (S=OUT,G=A,D=VDD,B=VDD) (L=0.25,W=1.5,AS=0.6375,AD=0.3375,PS=3.85,PD=1.95);\n"
    "  device MLVPMOS $2 (S=VDD,G=B,D=OUT,B=VDD) (L=0.25,W=1.5,AS=0.3375,AD=0.6375,PS=1.95,PD=3.85);\n"
    "  device MLVNMOS $3 (S=VSS,G=A,D=INT,B=VSS) (L=0.25,W=0.95,AS=0.40375,AD=0.21375,PS=2.75,PD=1.4);\n"
    "  device MLVNMOS $4 (S=INT,G=B,D=OUT,B=VSS) (L=0.25,W=0.95,AS=0.21375,AD=0.40375,PS=1.4,PD=2.75);\n"
    "end;\n"
    "circuit INVX1 (OUT=OUT,IN=IN,VDD=VDD,VSS=VSS);\n"
    "  device MLVPMOS $1 (S=VDD,G=IN,D=OUT,B=VDD) (L=0.25,W=1.5,AS=0.6375,AD=0.6375,PS=3.85,PD=3.85);\n"
    "  device MLVNMOS $2 (S=VSS,G=IN,D=OUT,B=VSS) (L=0.25,W=0.95,AS=0.40375,AD=0.40375,PS=2.75,PD=2.75);\n"
    "end;\n"
  );
}

TEST(8_Include)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader8.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit INVX1 ('1'='1','2'='2','3'='3','4'='4','5'='5','6'='6');\n"
    "  device MLVPMOS $1 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $2 (S='3',G='5',D='2',B='6') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit ND2X1 ('1'='1','2'='2','3'='3','4'='4','5'='5','6'='6','7'='7');\n"
    "  device MLVPMOS $1 (S='2',G='6',D='1',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVPMOS $2 (S='1',G='5',D='2',B='4') (L=0.25,W=1.5,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $3 (S='3',G='6',D='8',B='7') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "  device MLVNMOS $4 (S='8',G='5',D='2',B='7') (L=0.25,W=0.95,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit RINGO ('11'='11','12'='12','13'='13','14'='14','15'='15');\n"
    "  subcircuit ND2X1 $1 ('1'='12','2'='1','3'='15','4'='12','5'='11','6'='14','7'='15');\n"
    "  subcircuit INVX1 $2 ('1'='12','2'='2','3'='15','4'='12','5'='1','6'='15');\n"
    "  subcircuit INVX1 $3 ('1'='12','2'='3','3'='15','4'='12','5'='2','6'='15');\n"
    "  subcircuit INVX1 $4 ('1'='12','2'='4','3'='15','4'='12','5'='3','6'='15');\n"
    "  subcircuit INVX1 $5 ('1'='12','2'='5','3'='15','4'='12','5'='4','6'='15');\n"
    "  subcircuit INVX1 $6 ('1'='12','2'='6','3'='15','4'='12','5'='5','6'='15');\n"
    "  subcircuit INVX1 $7 ('1'='12','2'='7','3'='15','4'='12','5'='6','6'='15');\n"
    "  subcircuit INVX1 $8 ('1'='12','2'='8','3'='15','4'='12','5'='7','6'='15');\n"
    "  subcircuit INVX1 $9 ('1'='12','2'='9','3'='15','4'='12','5'='8','6'='15');\n"
    "  subcircuit INVX1 $10 ('1'='12','2'='10','3'='15','4'='12','5'='9','6'='15');\n"
    "  subcircuit INVX1 $11 ('1'='12','2'='11','3'='15','4'='12','5'='10','6'='15');\n"
    "  subcircuit INVX1 $12 ('1'='12','2'='13','3'='15','4'='12','5'='11','6'='15');\n"
    "end;\n"
  );
}

TEST(9_DeviceMultipliers)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader9.cir");

  {
    db::NetlistSpiceReader reader;
    tl::InputStream is (path);
    reader.read (is, nl);

    std::string nl_string = nl.to_string ();
    //  normalization of exponential representation:
    nl_string = tl::replaced (nl_string, "e-009", "e-09");

    EXPECT_EQ (nl_string,
      "circuit .TOP ();\n"
      "  device RES $1 (A='1',B='2') (R=850,L=0,W=0,A=0,P=0);\n"
      "  device RES $2 (A='3',B='4') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL $3 (A='1',B='2') (R=850,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL $4 (A='3',B='4') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL $5 (A='3',B='4') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL2 $6 (A='3',B='4',W='5') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL2 $7 (A='3',B='4',W='5') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RES3 $8 (A='3',B='4',W='5') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device NMOS $1 (S='3',G='2',D='1',B='4') (L=7,W=4,AS=0,AD=0,PS=0,PD=0);\n"
      "  device PMOS $2 (S='3',G='2',D='1',B='4') (L=7,W=2,AS=0,AD=0,PS=0,PD=0);\n"
      "  device CAP $1 (A='1',B='2') (C=2e-09,A=0,P=0);\n"
      "  device CAP $2 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL $3 (A='1',B='2') (C=2e-09,A=0,P=0);\n"
      "  device CMODEL $4 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL $5 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL2 $6 (A='3',B='4',W='5') (C=1e-09,A=0,P=0);\n"
      "  device CAP3 $7 (A='3',B='4',W='5') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL2 $8 (A='3',B='4',W='5') (C=1e-09,A=0,P=0);\n"
      "  device IND $1 (A='1',B='2') (L=5e-10);\n"
      "  device IND $2 (A='3',B='4') (L=1e-09);\n"
      "  device LMODEL $3 (A='1',B='2') (L=5e-10);\n"
      "  device LMODEL $4 (A='3',B='4') (L=1e-09);\n"
      "  device DIODE $1 (A='1',C='2') (A=20,P=0);\n"
      "  device DIODE $2 (A='3',C='4') (A=10,P=0);\n"
      "  device BIP $1 (C='1',B='2',E='3',S='4') (AE=20,PE=0,AB=0,PB=0,AC=0,PC=0,NE=1);\n"
      "  device BIP $2 (C='1',B='2',E='3',S='4') (AE=10,PE=0,AB=0,PB=0,AC=0,PC=0,NE=1);\n"
      "end;\n"
    );
  }

  db::Circuit *top = nl.circuit_by_name (".TOP");
  nl.remove_circuit (top);

  //  read once again, this time with known classes (must not trigger issue-652)
  {
    db::NetlistSpiceReader reader;
    tl::InputStream is (path);
    reader.read (is, nl);

    std::string nl_string = nl.to_string ();
    //  normalization of exponential representation:
    nl_string = tl::replaced (nl_string, "e-009", "e-09");

    EXPECT_EQ (nl_string,
      "circuit .TOP ();\n"
      "  device RES $1 (A='1',B='2') (R=850,L=0,W=0,A=0,P=0);\n"
      "  device RES $2 (A='3',B='4') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL $3 (A='1',B='2') (R=850,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL $4 (A='3',B='4') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL $5 (A='3',B='4') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL2 $6 (A='3',B='4',W='5') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RMODEL2 $7 (A='3',B='4',W='5') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device RES3 $8 (A='3',B='4',W='5') (R=1700,L=0,W=0,A=0,P=0);\n"
      "  device NMOS $1 (S='3',G='2',D='1',B='4') (L=7,W=4,AS=0,AD=0,PS=0,PD=0);\n"
      "  device PMOS $2 (S='3',G='2',D='1',B='4') (L=7,W=2,AS=0,AD=0,PS=0,PD=0);\n"
      "  device CAP $1 (A='1',B='2') (C=2e-09,A=0,P=0);\n"
      "  device CAP $2 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL $3 (A='1',B='2') (C=2e-09,A=0,P=0);\n"
      "  device CMODEL $4 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL $5 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL2 $6 (A='3',B='4',W='5') (C=1e-09,A=0,P=0);\n"
      "  device CAP3 $7 (A='3',B='4',W='5') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL2 $8 (A='3',B='4',W='5') (C=1e-09,A=0,P=0);\n"
      "  device IND $1 (A='1',B='2') (L=5e-10);\n"
      "  device IND $2 (A='3',B='4') (L=1e-09);\n"
      "  device LMODEL $3 (A='1',B='2') (L=5e-10);\n"
      "  device LMODEL $4 (A='3',B='4') (L=1e-09);\n"
      "  device DIODE $1 (A='1',C='2') (A=20,P=0);\n"
      "  device DIODE $2 (A='3',C='4') (A=10,P=0);\n"
      "  device BIP $1 (C='1',B='2',E='3',S='4') (AE=20,PE=0,AB=0,PB=0,AC=0,PC=0,NE=1);\n"
      "  device BIP $2 (C='1',B='2',E='3',S='4') (AE=10,PE=0,AB=0,PB=0,AC=0,PC=0,NE=1);\n"
      "end;\n"
    );
  }
}

TEST(10_SubcircuitsNoPins)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader10.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  device RES $1 (A=VDD,B=GND) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  subcircuit FILLER_CAP '0' (VDD=VDD,GND=GND);\n"
    "end;\n"
    "circuit FILLER_CAP (VDD=VDD,GND=GND);\n"
    "  device NMOS '0' (S=GND,G=VDD,D=GND,B=GND) (L=10,W=10,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

TEST(11_ErrorOnCircuitRedefinition)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader11.cir");

  std::string msg;
  try {
    db::NetlistSpiceReader reader;
    tl::InputStream is (path);
    reader.read (is, nl);
  } catch (tl::Exception &ex) {
    msg = ex.msg ();
  }

  EXPECT_EQ (tl::replaced (msg, tl::absolute_file_path (path), "?"), "Redefinition of circuit SUBCKT in ?, line 20");
}

TEST(12_IgnoreDuplicateGlobals)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader12.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  device RES $1 (A=VDD,B=GND) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  subcircuit FILLER_CAP '0' (VDD=VDD,GND=GND);\n"
    "end;\n"
    "circuit FILLER_CAP (VDD=VDD,GND=GND);\n"
    "  device NMOS '0' (S=GND,G=VDD,D=GND,B=GND) (L=10,W=10,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

TEST(13_NoGlobalNetsIfNotUsed)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader13.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  device RES $1 (A=VDD,B=NC) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  subcircuit C10 '1' (VDD=VDD,GND=GND);\n"
    "end;\n"
    "circuit C10 (VDD=VDD,GND=GND);\n"
    "  subcircuit C1 '1' (VDD=VDD,GND=GND);\n"
    "  subcircuit C2 '2' ();\n"
    "  subcircuit C3 '3' (VDD=VDD,GND=GND);\n"
    "  subcircuit C4 '4' (VDD=VDD,GND=GND);\n"
    "end;\n"
    "circuit C1 (VDD=VDD,GND=GND);\n"
    "  subcircuit FILLER_CAP '1' (VDD=VDD,GND=GND);\n"
    "  subcircuit DUMMY '2' ();\n"
    "end;\n"
    "circuit FILLER_CAP (VDD=VDD,GND=GND);\n"
    "  device NMOS '1' (S=GND,G=VDD,D=GND,B=GND) (L=10,W=10,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit DUMMY ();\n"
    "  device NMOS '1' (S=A,G=A,D=A,B=B) (L=1,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit C2 ();\n"
    "  subcircuit DUMMY '1' ();\n"
    "end;\n"
    "circuit C3 (VDD=VDD,GND=GND);\n"
    "  device NMOS '1' (S=GND,G=VDD,D=GND,B=GND) (L=10,W=10,AS=0,AD=0,PS=0,PD=0);\n"
    "  subcircuit FILLER_CAP '1' (VDD=VDD,GND=GND);\n"
    "end;\n"
    "circuit C4 (VDD=VDD,GND=GND);\n"
    "  subcircuit C1 '3' (VDD=VDD,GND=GND);\n"
    "end;\n"
  );
}

TEST(14_IncludeWithError)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader14.cir");

  try {
    db::NetlistSpiceReader reader;
    tl::InputStream is (path);
    reader.read (is, nl);
    EXPECT_EQ (true, false);  //  must not happen
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "'M' element must have four nodes in " + std::string (tl::absolute_file_path (tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader14x.cir"))) + ", line 3");
  }
}

TEST(15_ContinuationWithBlanks)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader15.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit SUBCKT ($1=$1,'A[5]<1>'='A[5]<1>','V42(%)'='V42(%)',Z=Z,GND=GND,GND$1=GND$1);\n"
    "  subcircuit 'HVPMOS(AD=0.18,AS=0.18,L=0.2,PD=2.16,PS=2.16,W=1)' D_$1 ('1'='V42(%)','2'=$3,'3'=Z,'4'=$1);\n"
    "  subcircuit 'HVPMOS(AD=0.18,AS=0.18,L=0.2,PD=2.16,PS=2.16,W=1)' D_$2 ('1'='V42(%)','2'='A[5]<1>','3'=$3,'4'=$1);\n"
    "  subcircuit 'HVNMOS(AD=0,AS=0,L=1.13,PD=6,PS=6,W=2.12)' D_$3 ('1'=GND,'2'=$3,'3'=GND,'4'=GND$1);\n"
    "  subcircuit 'HVNMOS(AD=0.19,AS=0.19,L=0.4,PD=1.16,PS=1.16,W=0.4)' D_$4 ('1'=GND,'2'=$3,'3'=Z,'4'=GND$1);\n"
    "  subcircuit 'HVNMOS(AD=0.19,AS=0.19,L=0.4,PD=1.76,PS=1.76,W=0.4)' D_$5 ('1'=GND,'2'='A[5]<1>','3'=$3,'4'=GND$1);\n"
    "end;\n"
    "circuit 'HVPMOS(AD=0.18,AS=0.18,L=0.2,PD=2.16,PS=2.16,W=1)' ('1'='1','2'='2','3'='3','4'='4');\n"
    "end;\n"
    "circuit 'HVNMOS(AD=0,AS=0,L=1.13,PD=6,PS=6,W=2.12)' ('1'='1','2'='2','3'='3','4'='4');\n"
    "end;\n"
    "circuit 'HVNMOS(AD=0.19,AS=0.19,L=0.4,PD=1.16,PS=1.16,W=0.4)' ('1'='1','2'='2','3'='3','4'='4');\n"
    "end;\n"
    "circuit 'HVNMOS(AD=0.19,AS=0.19,L=0.4,PD=1.76,PS=1.76,W=0.4)' ('1'='1','2'='2','3'='3','4'='4');\n"
    "end;\n"
  );
}

TEST(16_issue898)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "issue-898.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  device RES $1 (A=VDD,B=GND) (R=1000,L=0,W=0,A=0,P=0);\n"
    "  subcircuit FILLER_CAP '0' (VDD=VDD,GND=GND);\n"
    "end;\n"
    "circuit FILLER_CAP (VDD=VDD,GND=GND);\n"
    "  device NMOS '0' (S=GND,G=VDD,D=GND,B=GND) (L=10,W=10,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );
}

TEST(17_RecursiveExpansion)
{
  db::Netlist nl, nl2;

  {
    std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader17.cir");

    db::NetlistSpiceReader reader;
    tl::InputStream is (path);
    reader.read (is, nl);
  }

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  subcircuit 'SUB1(L=0.15,W=1.5)' SUB1A (N1=A,N2=B,N3=C);\n"
    "  subcircuit 'SUB1(L=0.25,W=3)' SUB1B (N1=A,N2=B,N3=C);\n"
    "end;\n"
    "circuit 'SUB1(L=0.15,W=1.5)' (N1=N1,N2=N2,N3=N3);\n"
    "  subcircuit 'SUB2(L=0.15,M=1,W=1.5)' SUB2A (N1=N1,N2=N2,N3=N3);\n"
    "  subcircuit 'SUB2(L=0.15,M=2,W=1.5)' SUB2B (N1=N1,N2=N2,N3=N3);\n"
    "end;\n"
    "circuit 'SUB2(L=0.15,M=1,W=1.5)' (N1=N1,N2=N2,N3=N3);\n"
    "  device NMOS NMOS (S=N3,G=N2,D=N1,B=N1) (L=150000,W=1500000,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit 'SUB2(L=0.15,M=2,W=1.5)' (N1=N1,N2=N2,N3=N3);\n"
    "  device NMOS NMOS (S=N3,G=N2,D=N1,B=N1) (L=150000,W=3000000,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit 'SUB1(L=0.25,W=3)' (N1=N1,N2=N2,N3=N3);\n"
    "  subcircuit 'SUB2(L=0.25,M=1,W=3)' SUB2A (N1=N1,N2=N2,N3=N3);\n"
    "  subcircuit 'SUB2(L=0.25,M=2,W=3)' SUB2B (N1=N1,N2=N2,N3=N3);\n"
    "end;\n"
    "circuit 'SUB2(L=0.25,M=1,W=3)' (N1=N1,N2=N2,N3=N3);\n"
    "  device NMOS NMOS (S=N3,G=N2,D=N1,B=N1) (L=250000,W=3000000,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit 'SUB2(L=0.25,M=2,W=3)' (N1=N1,N2=N2,N3=N3);\n"
    "  device NMOS NMOS (S=N3,G=N2,D=N1,B=N1) (L=250000,W=6000000,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
  );

  {
    std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader17b.cir");

    db::NetlistSpiceReader reader;
    tl::InputStream is (path);
    reader.read (is, nl2);
  }

  EXPECT_EQ (nl2.to_string (), nl.to_string ());
}

TEST(18_XSchemOutput)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader18.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15U,NF=4,W=1.5U)' XPMOS (D=Q,G=I,S=VDD,B=VDD);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15U,NF=4,W=1.5U)' XNMOS (D=Q,G=I,S=VSS,B=VSS);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15U,NF=2,W=1.5U)' XDUMMY0 (D=VSS,G=VSS,S=VSS,B=VSS);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15U,NF=2,W=1.5U)' XDUMMY1 (D=VSS,G=VSS,S=VSS,B=VSS);\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15U,NF=2,W=1.5U)' XDUMMY2 (D=VDD,G=VDD,S=VDD,B=VDD);\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15U,NF=2,W=1.5U)' XDUMMY3 (D=VDD,G=VDD,S=VDD,B=VDD);\n"
    "end;\n"
    "circuit 'PMOS4_STANDARD(L=0.15U,NF=4,W=1.5U)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__PFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=6,AS=1.305,AD=0.87,PS=10.74,PD=7.16);\n"
    "end;\n"
    "circuit 'NMOS4_STANDARD(L=0.15U,NF=4,W=1.5U)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__NFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=6,AS=1.305,AD=0.87,PS=10.74,PD=7.16);\n"
    "end;\n"
    "circuit 'NMOS4_STANDARD(L=0.15U,NF=2,W=1.5U)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__NFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=3,AS=0.87,AD=0.435,PS=7.16,PD=3.58);\n"
    "end;\n"
    "circuit 'PMOS4_STANDARD(L=0.15U,NF=2,W=1.5U)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__PFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=3,AS=0.87,AD=0.435,PS=7.16,PD=3.58);\n"
    "end;\n"
  );
}

TEST(19_ngspice_ref)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader19.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15,NF=4,W=1.5)' XPMOS (D=Q,G=I,S=VDD,B=VDD);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15,NF=4,W=1.5)' XNMOS (D=Q,G=I,S=VSS,B=VSS);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15,NF=2,W=1.5)' XDUMMY0 (D=VSS,G=VSS,S=VSS,B=VSS);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15,NF=2,W=1.5)' XDUMMY1 (D=VSS,G=VSS,S=VSS,B=VSS);\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15,NF=2,W=1.5)' XDUMMY2 (D=VDD,G=VDD,S=VDD,B=VDD);\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15,NF=2,W=1.5)' XDUMMY3 (D=VDD,G=VDD,S=VDD,B=VDD);\n"
    "end;\n"
    "circuit 'PMOS4_STANDARD(L=0.15,NF=4,W=1.5)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__PFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=6,AS=0.32625,AD=0.2175,PS=3.99,PD=2.66);\n"
    "end;\n"
    "circuit 'NMOS4_STANDARD(L=0.15,NF=4,W=1.5)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__NFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=6,AS=0.32625,AD=0.2175,PS=3.99,PD=2.66);\n"
    "end;\n"
    "circuit 'NMOS4_STANDARD(L=0.15,NF=2,W=1.5)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__NFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=3,AS=0.435,AD=0.2175,PS=4.16,PD=2.08);\n"
    "end;\n"
    "circuit 'PMOS4_STANDARD(L=0.15,NF=2,W=1.5)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__PFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=3,AS=0.435,AD=0.2175,PS=4.16,PD=2.08);\n"
    "end;\n"
  );
}

//  using parameters evaluated before inside formulas
TEST(19b_ngspice_ref)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader19b.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15,NF=4,V=1.5)' XPMOS (D=Q,G=I,S=VDD,B=VDD);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15,NF=4,V=1.5)' XNMOS (D=Q,G=I,S=VSS,B=VSS);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15,NF=2,V=1.5)' XDUMMY0 (D=VSS,G=VSS,S=VSS,B=VSS);\n"
    "  subcircuit 'NMOS4_STANDARD(L=0.15,NF=2,V=1.5)' XDUMMY1 (D=VSS,G=VSS,S=VSS,B=VSS);\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15,NF=2,V=1.5)' XDUMMY2 (D=VDD,G=VDD,S=VDD,B=VDD);\n"
    "  subcircuit 'PMOS4_STANDARD(L=0.15,NF=2,V=1.5)' XDUMMY3 (D=VDD,G=VDD,S=VDD,B=VDD);\n"
    "end;\n"
    "circuit 'PMOS4_STANDARD(L=0.15,NF=4,V=1.5)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__PFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=6,AS=0.32625,AD=0.2175,PS=3.99,PD=2.66);\n"
    "end;\n"
    "circuit 'NMOS4_STANDARD(L=0.15,NF=4,V=1.5)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__NFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=6,AS=0.32625,AD=0.2175,PS=3.99,PD=2.66);\n"
    "end;\n"
    "circuit 'NMOS4_STANDARD(L=0.15,NF=2,V=1.5)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__NFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=3,AS=0.435,AD=0.2175,PS=4.16,PD=2.08);\n"
    "end;\n"
    "circuit 'PMOS4_STANDARD(L=0.15,NF=2,V=1.5)' (D=D,G=G,S=S,B=B);\n"
    "  device SKY130_FD_PR__PFET_01V8 M1 (S=S,G=G,D=D,B=B) (L=0.15,W=3,AS=0.435,AD=0.2175,PS=4.16,PD=2.08);\n"
    "end;\n"
  );
}

//  issue #1319, clarification
TEST(20_precendence)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader20.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit TEST ();\n"
    "  device CAP '1' (A=A,B=B) (C=5e-12,A=0,P=0);\n"
    "  device CAP '2A' (A=A,B=B) (C=2.5e-12,A=0,P=0);\n"
    "  device CAP '2B' (A=A,B=B) (C=2.5e-12,A=0,P=0);\n"
    "  device CAP_MODEL1 '3' (A=A,B=B) (C=5e-12,A=0,P=0);\n"
    "  device CAP3 '4' (A=A,B=B,W=CAP_MODEL1) (C=2.5e-12,A=0,P=0);\n"
    "  device CAP_MODEL2 '5' (A=A,B=B,W=C) (C=5e-12,A=0,P=0);\n"
    "  device CAP_MODEL1 '6' (A=A,B=B) (C=2.5e-12,A=0,P=0);\n"
    "  device CAP_MODEL2 '7A' (A=A,B=B,W=C) (C=2.5e-12,A=0,P=0);\n"
    "  device CAP_MODEL2 '7B' (A=A,B=B,W=C) (C=2.5e-12,A=0,P=0);\n"
    "  device CAP_MODEL2 '8' (A=A,B=B,W=C) (C=2.5e-12,A=0,P=0);\n"
    "end;\n"
  );
}

//  issue #1320, .lib support
TEST(21_lib)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader21.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  device CAP '10' (A='1',B='2') (C=1e-12,A=0,P=0);\n"
    "  device CAP '1' (A='1',B='2') (C=1e-10,A=0,P=0);\n"
    "  device CAP '2A' (A='1',B='2') (C=1.01e-10,A=0,P=0);\n"
    "  device CAP '2B' (A='1',B='2') (C=1.02e-10,A=0,P=0);\n"
    "  device CAP '100' (A='1',B='2') (C=1.5e-11,A=0,P=0);\n"
    "end;\n"
  );
}

//  issue #1681, .endl with lib name
TEST(22_endl)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader22.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  device CAP '10' (A='1',B='2') (C=1e-12,A=0,P=0);\n"
    "  device CAP '1' (A='1',B='2') (C=1e-10,A=0,P=0);\n"
    "  device CAP '2A' (A='1',B='2') (C=1.01e-10,A=0,P=0);\n"
    "  device CAP '2B' (A='1',B='2') (C=1.02e-10,A=0,P=0);\n"
    "  device CAP '100' (A='1',B='2') (C=1.5e-11,A=0,P=0);\n"
    "end;\n"
  );
}

//  issue #1683
TEST(23_endl)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nreader23.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit .TOP ();\n"
    "  device PFET M1 (S=VDD,G=I,D=O,B=VDD) (L=100,W=100,AS=0,AD=0,PS=0,PD=0);\n"
    "  device NFET M2 (S=VSS,G=I,D=O,B=VSS) (L=100,W=100,AS=0,AD=0,PS=0,PD=0);\n"
    "  subcircuit DOESNOTEXIST DUMMY ('1'=O,'2'=I);\n"
    "end;\n"
    "circuit DOESNOTEXIST ('1'='1','2'='2');\n"
    "end;\n"
  );
}

TEST(100_ExpressionParser)
{
  std::map<std::string, tl::Variant> vars;
  vars["A"] = 17.5;
  vars["B"] = 42;
  vars["S"] = "string";

  tl::Variant v;

  db::NetlistSpiceReaderExpressionParser parser (&vars);

  EXPECT_EQ (parser.read ("1.75").to_string (), "1.75");
  EXPECT_EQ (parser.read ("-1.75").to_string (), "-1.75");
  EXPECT_EQ (parser.read ("-a*0.1").to_string (), "-1.75");
  EXPECT_EQ (parser.read ("-A*0.1").to_string (), "-1.75");
  EXPECT_EQ (parser.read ("b/6").to_string (), "7");
  EXPECT_EQ (parser.read ("B/6").to_string (), "7");
  EXPECT_EQ (parser.read ("s").to_string (), "string");
  EXPECT_EQ (parser.read ("S").to_string (), "string");
  EXPECT_EQ (parser.read ("!0").to_string (), "true");
  EXPECT_EQ (parser.read ("!1").to_string (), "false");
  EXPECT_EQ (parser.read ("4*2+1").to_string (), "9");
  EXPECT_EQ (parser.read ("4*2-1").to_string (), "7");
  EXPECT_EQ (parser.read ("4/2-1").to_string (), "1");
  EXPECT_EQ (parser.read ("4%2-1").to_string (), "-1");
  EXPECT_EQ (parser.read ("5%2-1").to_string (), "0");
  EXPECT_EQ (parser.read ("2**2*2+1").to_string (), "9");
  EXPECT_EQ (parser.read ("2**2*(2+1)").to_string (), "12");
  EXPECT_EQ (parser.read ("pow(2,2)*(2+1)").to_string (), "12");
  EXPECT_EQ (parser.read ("POW(2,2)*(2+1)").to_string (), "12");
  EXPECT_EQ (parser.read ("pwr(2,2)*(2+1)").to_string (), "12");
  EXPECT_EQ (parser.read ("PWR(2,2)*(2+1)").to_string (), "12");
  EXPECT_EQ (parser.read ("3==2+1").to_string (), "true");
  EXPECT_EQ (parser.read ("4==2+1").to_string (), "false");
  EXPECT_EQ (parser.read ("3!=2+1").to_string (), "false");
  EXPECT_EQ (parser.read ("4!=2+1").to_string (), "true");
  EXPECT_EQ (parser.read ("2<2+1").to_string (), "true");
  EXPECT_EQ (parser.read ("3<2+1").to_string (), "false");
  EXPECT_EQ (parser.read ("4<2+1").to_string (), "false");
  EXPECT_EQ (parser.read ("2<=2+1").to_string (), "true");
  EXPECT_EQ (parser.read ("3<=2+1").to_string (), "true");
  EXPECT_EQ (parser.read ("4<=2+1").to_string (), "false");
  EXPECT_EQ (parser.read ("2>2+1").to_string (), "false");
  EXPECT_EQ (parser.read ("3>2+1").to_string (), "false");
  EXPECT_EQ (parser.read ("4>2+1").to_string (), "true");
  EXPECT_EQ (parser.read ("2>=2+1").to_string (), "false");
  EXPECT_EQ (parser.read ("3>=2+1").to_string (), "true");
  EXPECT_EQ (parser.read ("4>=2+1").to_string (), "true");
  EXPECT_EQ (parser.read ("1==2||2==2").to_string (), "true");
  EXPECT_EQ (parser.read ("1==2||3==2").to_string (), "false");
  EXPECT_EQ (parser.read ("1==2&&2==2").to_string (), "false");
  EXPECT_EQ (parser.read ("1==1&&2==2").to_string (), "true");
  EXPECT_EQ (parser.read ("1==2?2:3").to_string (), "3");
  EXPECT_EQ (parser.read ("ternery_fcn(1==2,2,3)").to_string (), "3");
  EXPECT_EQ (parser.read ("1==1?2:3").to_string (), "2");
  EXPECT_EQ (parser.read ("ternery_fcn(1==1,2,3)").to_string (), "2");

  EXPECT_EQ (parser.read ("sin(0)").to_string (), "0");
  EXPECT_EQ (parser.read ("sin(atan(1.0)*2)").to_string (), "1");
  EXPECT_EQ (parser.read ("cos(0)").to_string (), "1");
  EXPECT_EQ (parser.read ("cos(atan(1.0)*2)").to_string (), "0");
  EXPECT_EQ (parser.read ("tan(0)").to_string (), "0");
  EXPECT_EQ (parser.read ("tan(atan(1.0))").to_string (), "1");
  EXPECT_EQ (parser.read ("sin(asin(0.5))").to_string (), "0.5");
  EXPECT_EQ (parser.read ("cos(acos(0.5))").to_string (), "0.5");
  EXPECT_EQ (parser.read ("ln(exp(0.5))").to_string (), "0.5");
  EXPECT_EQ (parser.read ("exp(0.0)").to_string (), "1");
  EXPECT_EQ (parser.read ("log(10**0.5)").to_string (), "0.5");
  EXPECT_EQ (parser.read ("int(-0.5)").to_string (), "0");
  EXPECT_EQ (parser.read ("int(-1.5)").to_string (), "-1");
  EXPECT_EQ (parser.read ("int(0.5)").to_string (), "0");
  EXPECT_EQ (parser.read ("int(1.5)").to_string (), "1");
  EXPECT_EQ (parser.read ("floor(-0.5)").to_string (), "-1");
  EXPECT_EQ (parser.read ("floor(-1.5)").to_string (), "-2");
  EXPECT_EQ (parser.read ("floor(0.5)").to_string (), "0");
  EXPECT_EQ (parser.read ("floor(1.5)").to_string (), "1");
  EXPECT_EQ (parser.read ("ceil(-0.5)").to_string (), "0");
  EXPECT_EQ (parser.read ("ceil(-1.5)").to_string (), "-1");
  EXPECT_EQ (parser.read ("ceil(0.5)").to_string (), "1");
  EXPECT_EQ (parser.read ("ceil(1.5)").to_string (), "2");
  EXPECT_EQ (parser.read ("nint(-0.5)").to_string (), "0");
  EXPECT_EQ (parser.read ("nint(-1.5)").to_string (), "-2");
  EXPECT_EQ (parser.read ("nint(0.5)").to_string (), "0");
  EXPECT_EQ (parser.read ("nint(1.5)").to_string (), "2");
  EXPECT_EQ (parser.read ("min(4,1,3)").to_string (), "1");
  EXPECT_EQ (parser.read ("min(4,3)").to_string (), "3");
  EXPECT_EQ (parser.read ("min(4)").to_string (), "4");
  EXPECT_EQ (parser.read ("max(1,4,3)").to_string (), "4");
  EXPECT_EQ (parser.read ("max(4,3)").to_string (), "4");
  EXPECT_EQ (parser.read ("max(4)").to_string (), "4");
  EXPECT_EQ (parser.read ("max(a,b)").to_string (), "42");

  EXPECT_EQ (parser.try_read ("a syntax error", v), false);
  v = tl::Variant ();
  EXPECT_EQ (parser.try_read ("1+2*(2+1)-1", v), true);
  EXPECT_EQ (v.to_string (), "6");
  EXPECT_EQ (parser.try_read ("{1+2*(2+1)-1)", v), false);
  EXPECT_EQ (parser.try_read ("'1+2*(2+1)-1)", v), false);
  EXPECT_EQ (parser.try_read ("\"1+2*(2+1)-1)", v), false);
  EXPECT_EQ (parser.try_read ("\"1+2*(2+1)-1'", v), false);
  v = tl::Variant ();
  EXPECT_EQ (parser.try_read ("{1+2*(2+1)-1}", v), true);
  EXPECT_EQ (v.to_string (), "6");
  v = tl::Variant ();
  EXPECT_EQ (parser.try_read ("'1+2*(2+1)-1'", v), true);
  EXPECT_EQ (v.to_string (), "6");
  v = tl::Variant ();
  EXPECT_EQ (parser.try_read ("\"1+2*(2+1)-1\"", v), true);
  EXPECT_EQ (v.to_string (), "6");
}

TEST(101_ExpressionParserWithDefScale)
{
  std::map<std::string, tl::Variant> vars;
  vars["A"] = 17.5;

  tl::Variant v;

  db::NetlistSpiceReaderExpressionParser parser (&vars, 1e-3);

  EXPECT_EQ (parser.read ("1.75").to_string (), "0.00175");
  EXPECT_EQ (parser.read ("-1.75u").to_string (), "-1.75e-06");
  EXPECT_EQ (parser.read ("1.75k").to_string (), "1750");
  EXPECT_EQ (parser.read ("2*A").to_string (), "0.035");
}
