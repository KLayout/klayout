
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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader1b.cir");

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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader2.cir");

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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader3.cir");

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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader4.cir");

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
    "  device PMOS $1 (S='3',G='2',D='5',B='1') (L=0.25,W=3.5,AS=1.4,AD=1.4,PS=6.85,PD=6.85);\n"
    "  device NMOS $3 (S='3',G='2',D='4',B='6') (L=0.25,W=3.5,AS=1.4,AD=1.4,PS=6.85,PD=6.85);\n"
    "end;\n"
  );
}

TEST(5_CircuitParameters)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader5.cir");

  db::NetlistSpiceReader reader;
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit SUBCKT ($1=$1,'A[5]<1>'='A[5]<1>','V42(%)'='V42(%)',Z=Z,GND=GND,GND$1=GND$1);\n"
    "  subcircuit HVPMOS D_$1 ($1='V42(%)',$2=$3,$3=Z,$4=$1);\n"
    "  subcircuit HVPMOS D_$2 ($1='V42(%)',$2='A[5]<1>',$3=$3,$4=$1);\n"
    "  subcircuit HVNMOS D_$3 ($1=GND,$2=$3,$3=GND,$4=GND$1);\n"
    "  subcircuit HVNMOS D_$4 ($1=GND,$2=$3,$3=Z,$4=GND$1);\n"
    "  subcircuit HVNMOS D_$5 ($1=GND,$2='A[5]<1>',$3=$3,$4=GND$1);\n"
    "end;\n"
    "circuit HVPMOS ($1=(null),$2=(null),$3=(null),$4=(null));\n"
    "end;\n"
    "circuit HVNMOS ($1=(null),$2=(null),$3=(null),$4=(null));\n"
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

  bool element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const std::map<std::string, double> &params)
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
        std::map<std::string, double>::const_iterator pi = params.find (i->name ());
        if (pi != params.end ()) {
          device->set_parameter_value (i->id (), pi->second * 1.5);
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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader6.cir");

  MyNetlistReaderDelegate delegate;
  db::NetlistSpiceReader reader (&delegate);
  tl::InputStream is (path);
  reader.read (is, nl);

  EXPECT_EQ (nl.to_string (),
    "circuit SUBCKT ($1=$1,A=A,VDD=VDD,Z=Z,GND=GND,GND$1=GND$1);\n"
    "  device HVPMOS $1 (S=VDD,G=$3,D=Z,B=$1) (L=0.3,W=1.5,AS=0.27,AD=0.27,PS=3.24,PD=3.24);\n"
    "  device HVPMOS $2 (S=VDD,G=A,D=$3,B=$1) (L=0.3,W=1.5,AS=0.27,AD=0.27,PS=3.24,PD=3.24);\n"
    "  device HVNMOS $3 (S=GND,G=$3,D=GND,B=GND$1) (L=1.695,W=3.18,AS=0,AD=0,PS=9,PD=9);\n"
    "  device HVNMOS $4 (S=GND,G=$3,D=Z,B=GND$1) (L=0.6,W=0.6,AS=0.285,AD=0.285,PS=1.74,PD=1.74);\n"
    "  device HVNMOS $5 (S=GND,G=A,D=$3,B=GND$1) (L=0.6,W=0.6,AS=0.285,AD=0.285,PS=2.64,PD=2.64);\n"
    "  device RES $1 (A=A,B=Z) (R=100000,L=0,W=0,A=0,P=0);\n"
    "end;\n"
    "circuit .TOP ();\n"
    "  subcircuit SUBCKT SUBCKT ($1=IN,A=OUT,VDD=VDD,Z=Z,GND=VSS,GND$1=VSS);\n"
    "end;\n"
  );
}

TEST(7_GlobalNets)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader7.cir");

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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader8.cir");

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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader9.cir");

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
      "  device NMOS $1 (S='1',G='2',D='3',B='4') (L=7,W=4,AS=0,AD=0,PS=0,PD=0);\n"
      "  device PMOS $2 (S='1',G='2',D='3',B='4') (L=7,W=2,AS=0,AD=0,PS=0,PD=0);\n"
      "  device CAP $1 (A='1',B='2') (C=2e-09,A=0,P=0);\n"
      "  device CAP $2 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL $3 (A='1',B='2') (C=2e-09,A=0,P=0);\n"
      "  device CMODEL $4 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
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
      "  device NMOS $1 (S='1',G='2',D='3',B='4') (L=7,W=4,AS=0,AD=0,PS=0,PD=0);\n"
      "  device PMOS $2 (S='1',G='2',D='3',B='4') (L=7,W=2,AS=0,AD=0,PS=0,PD=0);\n"
      "  device CAP $1 (A='1',B='2') (C=2e-09,A=0,P=0);\n"
      "  device CAP $2 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
      "  device CMODEL $3 (A='1',B='2') (C=2e-09,A=0,P=0);\n"
      "  device CMODEL $4 (A='3',B='4') (C=1e-09,A=0,P=0);\n"
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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader10.cir");

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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader11.cir");

  std::string msg;
  try {
    db::NetlistSpiceReader reader;
    tl::InputStream is (path);
    reader.read (is, nl);
  } catch (tl::Exception &ex) {
    msg = ex.msg ();
  }

  EXPECT_EQ (tl::replaced (msg, path, "?"), "Redefinition of circuit SUBCKT in ?, line 20");
}

TEST(12_IgnoreDuplicateGlobals)
{
  db::Netlist nl;

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader12.cir");

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

  std::string path = tl::combine_path (tl::combine_path (tl::combine_path (tl::testsrc (), "testdata"), "algo"), "nreader13.cir");

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
    "circuit FILLER_CAP (VDD=VDD,GND=GND);\n"
    "  device NMOS '1' (S=GND,G=VDD,D=GND,B=GND) (L=10,W=10,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit DUMMY ();\n"
    "  device NMOS '1' (S=A,G=A,D=A,B=B) (L=1,W=1,AS=0,AD=0,PS=0,PD=0);\n"
    "end;\n"
    "circuit C1 (VDD=VDD,GND=GND);\n"
    "  subcircuit FILLER_CAP '1' (VDD=VDD,GND=GND);\n"
    "  subcircuit DUMMY '2' ();\n"
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

