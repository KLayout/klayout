
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

#include "dbNetlistWriter.h"
#include "dbNetlistSpiceWriter.h"
#include "dbNetlist.h"
#include "dbNetlistDeviceClasses.h"
#include "dbLayoutToNetlist.h"

#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

static void compare_netlists (tl::TestBase *_this, const std::string &path, const std::string &au_path)
{
  tl::InputStream is (path);
  tl::InputStream is_au (au_path);

  std::string netlist = is.read_all ();
  std::string netlist_au = is_au.read_all ();

  //  normalize "1.0e-005" to "1.0e-05" for compatibility
  netlist = tl::replaced (netlist, "\r\n", "\n");   //  for Windows
  netlist = tl::replaced (netlist, "e-00", "e-0");
  netlist = tl::replaced (netlist, "e-0", "e-");
  netlist_au = tl::replaced (netlist_au, "\r\n", "\n");   //  for Windows
  netlist_au = tl::replaced (netlist_au, "e-00", "e-0");
  netlist_au = tl::replaced (netlist_au, "e-0", "e-");

  if (netlist != netlist_au) {
    _this->raise (tl::sprintf ("Compare failed - see\n  actual: %s\n  golden: %s",
                               tl::absolute_file_path (path),
                               tl::absolute_file_path (au_path)));
  }
}

TEST(1_WriterResistorDevices)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  rcls->set_name ("RCLS");

  nl.add_device_class (rcls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *rdev1 = new db::Device (rcls);
  rdev1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.7);
  db::Device *rdev2 = new db::Device (rcls);
  rdev2->set_parameter_value (db::DeviceClassResistor::param_id_R, 42e-6);
  circuit1->add_device (rdev1);
  circuit1->add_device (rdev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  rdev1->connect_terminal (rdev1->device_class ()->terminal_id_for_name ("A"), n1);
  rdev1->connect_terminal (rdev1->device_class ()->terminal_id_for_name ("B"), n3);
  rdev2->connect_terminal (rdev2->device_class ()->terminal_id_for_name ("A"), n3);
  rdev2->connect_terminal (rdev2->device_class ()->terminal_id_for_name ("B"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter1.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter1_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(1_WriterResistorDevicesWithBulk)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistorWithBulk ();
  rcls->set_name ("RCLS");

  nl.add_device_class (rcls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *rdev1 = new db::Device (rcls);
  rdev1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.7);
  db::Device *rdev2 = new db::Device (rcls);
  rdev2->set_parameter_value (db::DeviceClassResistor::param_id_R, 42e-6);
  circuit1->add_device (rdev1);
  circuit1->add_device (rdev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  rdev1->connect_terminal (rdev1->device_class ()->terminal_id_for_name ("A"), n1);
  rdev1->connect_terminal (rdev1->device_class ()->terminal_id_for_name ("B"), n3);
  rdev1->connect_terminal (rdev1->device_class ()->terminal_id_for_name ("W"), n3);
  rdev2->connect_terminal (rdev2->device_class ()->terminal_id_for_name ("A"), n3);
  rdev2->connect_terminal (rdev2->device_class ()->terminal_id_for_name ("B"), n2);
  rdev2->connect_terminal (rdev2->device_class ()->terminal_id_for_name ("W"), n3);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter1.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter1b_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(2_WriterCapacitorDevices)
{
  db::Netlist nl;

  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();

  ccls->set_name ("CCLS");

  nl.add_device_class (ccls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *cdev1 = new db::Device (ccls);
  cdev1->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 1.7e-12);
  db::Device *cdev2 = new db::Device (ccls);
  cdev2->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 42e-15);
  circuit1->add_device (cdev1);
  circuit1->add_device (cdev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("A"), n1);
  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("B"), n3);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("A"), n3);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("B"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter2.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter2_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(2_WriterCapacitorDevicesNoName)
{
  db::Netlist nl;

  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();

  nl.add_device_class (ccls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *cdev1 = new db::Device (ccls);
  cdev1->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 1.7e-12);
  db::Device *cdev2 = new db::Device (ccls);
  cdev2->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 42e-15);
  circuit1->add_device (cdev1);
  circuit1->add_device (cdev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("A"), n1);
  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("B"), n3);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("A"), n3);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("B"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter2.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter2b_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(2_WriterCapacitorDevicesWithBulk)
{
  db::Netlist nl;

  db::DeviceClass *ccls = new db::DeviceClassCapacitorWithBulk ();

  ccls->set_name ("CCLS");

  nl.add_device_class (ccls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *cdev1 = new db::Device (ccls);
  cdev1->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 1.7e-12);
  db::Device *cdev2 = new db::Device (ccls);
  cdev2->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 42e-15);
  circuit1->add_device (cdev1);
  circuit1->add_device (cdev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("A"), n1);
  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("B"), n3);
  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("W"), n3);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("A"), n3);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("B"), n2);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("W"), n3);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter2.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter2c_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(2_WriterCapacitorDevicesWithBulkNoName)
{
  db::Netlist nl;

  db::DeviceClass *ccls = new db::DeviceClassCapacitorWithBulk ();

  nl.add_device_class (ccls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *cdev1 = new db::Device (ccls);
  cdev1->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 1.7e-12);
  db::Device *cdev2 = new db::Device (ccls);
  cdev2->set_parameter_value (db::DeviceClassCapacitor::param_id_C, 42e-15);
  circuit1->add_device (cdev1);
  circuit1->add_device (cdev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("A"), n1);
  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("B"), n3);
  cdev1->connect_terminal (cdev1->device_class ()->terminal_id_for_name ("W"), n3);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("A"), n3);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("B"), n2);
  cdev2->connect_terminal (cdev2->device_class ()->terminal_id_for_name ("W"), n3);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter2.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter2d_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(3_WriterInductorDevices)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *m3cls = new db::DeviceClassMOS3Transistor ();
  db::DeviceClass *m4cls = new db::DeviceClassMOS4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  m3cls->set_name ("M3CLS");
  m4cls->set_name ("M4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (m3cls);
  nl.add_device_class (m4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *ldev1 = new db::Device (lcls);
  ldev1->set_parameter_value (db::DeviceClassInductor::param_id_L, 1.7e-10);
  db::Device *ldev2 = new db::Device (lcls);
  ldev2->set_parameter_value (db::DeviceClassInductor::param_id_L, 42e-9);
  circuit1->add_device (ldev1);
  circuit1->add_device (ldev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  ldev1->connect_terminal (ldev1->device_class ()->terminal_id_for_name ("A"), n1);
  ldev1->connect_terminal (ldev1->device_class ()->terminal_id_for_name ("B"), n3);
  ldev2->connect_terminal (ldev2->device_class ()->terminal_id_for_name ("A"), n3);
  ldev2->connect_terminal (ldev2->device_class ()->terminal_id_for_name ("B"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter3.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter3_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(4_WriterDiodeDevices)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *m3cls = new db::DeviceClassMOS3Transistor ();
  db::DeviceClass *m4cls = new db::DeviceClassMOS4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  m3cls->set_name ("M3CLS");
  m4cls->set_name ("M4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (m3cls);
  nl.add_device_class (m4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *ddev1 = new db::Device (dcls);
  ddev1->set_parameter_value (db::DeviceClassDiode::param_id_A, 1.7);
  db::Device *ddev2 = new db::Device (dcls);
  ddev2->set_parameter_value (db::DeviceClassDiode::param_id_A, 0.42);
  circuit1->add_device (ddev1);
  circuit1->add_device (ddev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("A"), n1);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("C"), n3);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("A"), n3);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("C"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter4.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter4_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(5_WriterMOS3Devices)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *m3cls = new db::DeviceClassMOS3Transistor ();
  db::DeviceClass *m4cls = new db::DeviceClassMOS4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  m3cls->set_name ("M3CLS");
  m4cls->set_name ("M4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (m3cls);
  nl.add_device_class (m4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3, *n4;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);
  n4 = new db::Net ();
  n4->set_name ("n4");
  circuit1->add_net (n4);

  db::Device *ddev1 = new db::Device (m3cls);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.25);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.18);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 1.2);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 0.75);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 2.2);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 1.75);
  db::Device *ddev2 = new db::Device (m3cls);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 1.4);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.25);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 1.3);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 0.85);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 2.3);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 1.85);
  circuit1->add_device (ddev1);
  circuit1->add_device (ddev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();
  size_t pid3 = circuit1->add_pin ("p3").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);
  circuit1->connect_pin (pid3, n4);

  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("S"), n1);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("G"), n4);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("D"), n3);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("S"), n3);
  ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("G"), n4);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("D"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter5.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter5_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(6_WriterMOS4Devices)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *m3cls = new db::DeviceClassMOS3Transistor ();
  db::DeviceClass *m4cls = new db::DeviceClassMOS4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  m3cls->set_name ("M3CLS");
  m4cls->set_name ("M4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (m3cls);
  nl.add_device_class (m4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3, *n4, *n5;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);
  n4 = new db::Net ();
  n4->set_name ("n4");
  circuit1->add_net (n4);
  n5 = new db::Net ();
  n5->set_name ("n5");
  circuit1->add_net (n5);

  db::Device *ddev1 = new db::Device (m4cls);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.25);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.18);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 1.2);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 0.75);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 2.2);
  ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 1.75);
  db::Device *ddev2 = new db::Device (m4cls);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 1.4);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.25);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 1.3);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 0.85);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 2.3);
  ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 1.85);
  circuit1->add_device (ddev1);
  circuit1->add_device (ddev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();
  size_t pid3 = circuit1->add_pin ("p3").id ();
  size_t pid4 = circuit1->add_pin ("p4").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);
  circuit1->connect_pin (pid3, n4);
  circuit1->connect_pin (pid4, n5);

  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("S"), n1);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("G"), n4);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("D"), n3);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n5);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("S"), n3);
  ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("G"), n4);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("D"), n2);
  ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n5);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter6.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter6_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(7_WriterAnyDevices)
{
  db::Netlist nl;

  db::DeviceClass *cls = new db::DeviceClass ();
  cls->add_terminal_definition (db::DeviceTerminalDefinition ("A", "a"));
  cls->add_terminal_definition (db::DeviceTerminalDefinition ("B", "b"));
  cls->add_parameter_definition (db::DeviceParameterDefinition ("U", "u"));
  cls->add_parameter_definition (db::DeviceParameterDefinition ("V", "v"));
  cls->set_name ("XCLS");

  nl.add_device_class (cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *ddev1 = new db::Device (cls);
  ddev1->set_parameter_value (0, -17);
  ddev1->set_parameter_value (1, 42);
  db::Device *ddev2 = new db::Device (cls);
  ddev2->set_parameter_value (0, 17);
  ddev2->set_parameter_value (1, -42);
  circuit1->add_device (ddev1);
  circuit1->add_device (ddev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("A"), n1);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n3);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("A"), n3);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("B"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter7.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter7_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(8_WriterSubcircuits)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *m3cls = new db::DeviceClassMOS3Transistor ();
  db::DeviceClass *m4cls = new db::DeviceClassMOS4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  m3cls->set_name ("M3CLS");
  m4cls->set_name ("M4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (m3cls);
  nl.add_device_class (m4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  {
    db::Net *n1, *n2, *n3, *n4, *n5;
    n1 = new db::Net ();
    n1->set_name ("n1");
    circuit1->add_net (n1);
    n2 = new db::Net ();
    n2->set_name ("n2");
    circuit1->add_net (n2);
    n3 = new db::Net ();
    n3->set_name ("n3");
    circuit1->add_net (n3);
    n4 = new db::Net ();
    n4->set_name ("n4");
    circuit1->add_net (n4);
    n5 = new db::Net ();
    n5->set_name ("n5");
    circuit1->add_net (n5);

    db::Device *ddev1 = new db::Device (m4cls);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.25);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.18);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 1.2);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 0.75);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 2.2);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 1.75);
    db::Device *ddev2 = new db::Device (m4cls);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 1.4);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.25);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 1.3);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 0.85);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 2.3);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 1.85);
    circuit1->add_device (ddev1);
    circuit1->add_device (ddev2);

    size_t pid1 = circuit1->add_pin ("p1").id ();
    size_t pid2 = circuit1->add_pin ("p2").id ();
    size_t pid3 = circuit1->add_pin ("p3").id ();
    size_t pid4 = circuit1->add_pin ("p4").id ();

    circuit1->connect_pin (pid1, n1);
    circuit1->connect_pin (pid2, n2);
    circuit1->connect_pin (pid3, n4);
    circuit1->connect_pin (pid4, n5);

    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("S"), n1);
    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("G"), n4);
    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("D"), n3);
    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n5);
    ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("S"), n3);
    ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("G"), n4);
    ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("D"), n2);
    ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n5);
  }

  db::Circuit *circuit2 = new db::Circuit ();
  circuit2->set_name ("C2");
  nl.add_circuit (circuit2);

  {
    db::Net *n1, *n2, *n3, *n4, *n5;
    n1 = new db::Net ();
    n1->set_name ("n1");
    circuit2->add_net (n1);
    n2 = new db::Net ();
    n2->set_name ("n2");
    circuit2->add_net (n2);
    n3 = new db::Net ();
    n3->set_name ("n3");
    circuit2->add_net (n3);
    n4 = new db::Net ();
    n4->set_name ("n4");
    circuit2->add_net (n4);
    n5 = new db::Net ();
    n5->set_name ("n5");
    circuit2->add_net (n5);

    db::SubCircuit *sc1 = new db::SubCircuit (circuit1, "SC1");
    circuit2->add_subcircuit (sc1);
    sc1->connect_pin (0, n1);
    sc1->connect_pin (1, n3);
    sc1->connect_pin (2, n4);
    sc1->connect_pin (3, n3);

    db::SubCircuit *sc2 = new db::SubCircuit (circuit1, "SC2");
    circuit2->add_subcircuit (sc2);
    sc2->connect_pin (0, n3);
    sc2->connect_pin (1, n2);
    sc2->connect_pin (2, n4);
    sc2->connect_pin (3, n3);

    size_t pid1 = circuit2->add_pin ("p1").id ();
    size_t pid2 = circuit2->add_pin ("p2").id ();
    size_t pid3 = circuit2->add_pin ("p3").id ();

    circuit2->connect_pin (pid1, n1);
    circuit2->connect_pin (pid2, n2);
    circuit2->connect_pin (pid3, n4);
  }

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter8.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter8_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(9_WriterNetNamesInsteadOfNumbers)
{
  db::Netlist nl;

  db::DeviceClass *cls = new db::DeviceClass ();
  cls->add_terminal_definition (db::DeviceTerminalDefinition ("A", "a"));
  cls->add_terminal_definition (db::DeviceTerminalDefinition ("B", "b"));
  cls->add_parameter_definition (db::DeviceParameterDefinition ("U", "u"));
  cls->add_parameter_definition (db::DeviceParameterDefinition ("V", "v"));
  cls->set_name ("XCLS");

  nl.add_device_class (cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  {
    db::Net *n1, *n2, *n3;
    n1 = new db::Net ();
    n1->set_name ("N1");
    circuit1->add_net (n1);
    n2 = new db::Net ();
    n2->set_name ("N 2");
    circuit1->add_net (n2);
    n3 = new db::Net ();
    n3->set_name ("n3");
    circuit1->add_net (n3);

    db::Device *ddev1 = new db::Device (cls);
    ddev1->set_parameter_value (0, -17);
    ddev1->set_parameter_value (1, 42);
    db::Device *ddev2 = new db::Device (cls);
    ddev2->set_parameter_value (0, 17);
    ddev2->set_parameter_value (1, -42);
    circuit1->add_device (ddev1);
    circuit1->add_device (ddev2);

    size_t pid1 = circuit1->add_pin ("p1").id ();
    size_t pid2 = circuit1->add_pin ("p2").id ();

    circuit1->connect_pin (pid1, n1);
    circuit1->connect_pin (pid2, n2);

    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("A"), n1);
    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n3);
    ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("A"), n3);
    ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("B"), n2);
  }

  db::Circuit *circuit2 = new db::Circuit ();
  circuit2->set_name ("C2");
  nl.add_circuit (circuit2);

  {
    db::Net *n1, *n2;
    n1 = new db::Net ();
    n1->set_name ("n1");
    circuit2->add_net (n1);
    n2 = new db::Net ();
    n2->set_name ("n2");
    circuit2->add_net (n2);

    db::SubCircuit *sc1 = new db::SubCircuit (circuit1, "SC1");
    circuit2->add_subcircuit (sc1);
    sc1->connect_pin (0, n1);
    sc1->connect_pin (1, n2);

    size_t pid1 = circuit2->add_pin ("p1").id ();
    size_t pid2 = circuit2->add_pin ("p2").id ();

    circuit2->connect_pin (pid1, n1);
    circuit2->connect_pin (pid2, n2);
  }

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter9.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.set_use_net_names (true);
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter9_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(10_WriterLongLines)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  nl.add_device_class (rcls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1withaverylongextensionthatgoesbeyondmultiplelinesunlessipasteeverythingtogetherwhichmakesithardtoreadbutexactlythatisthereasonwhyiwriteitthisway");
  nl.add_circuit (circuit1);

  db::Net *n0 = new db::Net ();
  n0->set_name ("n0");
  circuit1->add_net (n0);

  size_t pid0 = circuit1->add_pin ("p0").id ();
  circuit1->connect_pin (pid0, n0);

  for (int i = 0; i < 100; ++i) {

    db::Net *n = new db::Net ();
    n->set_name ("n" + tl::to_string (i + 1));
    circuit1->add_net (n);

    size_t pid = circuit1->add_pin ("p" + tl::to_string (i + 1)).id ();
    circuit1->connect_pin (pid, n);

    db::Device *ddev = new db::Device (rcls);
    circuit1->add_device (ddev);
    ddev->connect_terminal (db::DeviceClassResistor::terminal_id_A, n0);
    ddev->connect_terminal (db::DeviceClassResistor::terminal_id_B, n);

  }

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter10.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter10_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(11_WriterNonConnectedPins)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *m3cls = new db::DeviceClassMOS3Transistor ();
  db::DeviceClass *m4cls = new db::DeviceClassMOS4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  m3cls->set_name ("M3CLS");
  m4cls->set_name ("M4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (m3cls);
  nl.add_device_class (m4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  {
    db::Net *n1, *n2, *n3, *n4, *n5;
    n1 = new db::Net ();
    n1->set_name ("n1");
    circuit1->add_net (n1);
    n2 = new db::Net ();
    n2->set_name ("n2");
    circuit1->add_net (n2);
    n3 = new db::Net ();
    n3->set_name ("n3");
    circuit1->add_net (n3);
    n4 = new db::Net ();
    n4->set_name ("n4");
    circuit1->add_net (n4);
    n5 = new db::Net ();
    n5->set_name ("n5");
    circuit1->add_net (n5);

    db::Device *ddev1 = new db::Device (m4cls);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 0.25);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.18);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 1.2);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 0.75);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 2.2);
    ddev1->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 1.75);
    db::Device *ddev2 = new db::Device (m4cls);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, 1.4);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, 0.25);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AS, 1.3);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_AD, 0.85);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PS, 2.3);
    ddev2->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_PD, 1.85);
    circuit1->add_device (ddev1);
    circuit1->add_device (ddev2);

    size_t pid1 = circuit1->add_pin ("p1").id ();
    size_t pid2 = circuit1->add_pin ("p2").id ();
    size_t pid3 = circuit1->add_pin ("p3").id ();
    size_t pid4 = circuit1->add_pin ("p4").id ();

    circuit1->connect_pin (pid1, n1);
    circuit1->connect_pin (pid2, n2);
    circuit1->connect_pin (pid3, n4);
    circuit1->connect_pin (pid4, n5);

    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("S"), n1);
    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("G"), n4);
    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("D"), n3);
    ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n5);
    ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("S"), n3);
    ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("G"), n4);
    ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("D"), n2);
    ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n5);
  }

  db::Circuit *circuit2 = new db::Circuit ();
  circuit2->set_name ("C2");
  nl.add_circuit (circuit2);

  {
    db::Net *n1, *n2, *n3, *n4, *n5;
    n1 = new db::Net ();
    //  this gives a clash with the auto-generated node names with non-connected subcircuit pins
    //  and terminals - we test proper generation of such names this way
    n1->set_name ("nc_10");
    circuit2->add_net (n1);
    n2 = new db::Net ();
    n2->set_name ("n2");
    circuit2->add_net (n2);
    n3 = new db::Net ();
    n3->set_name ("n3");
    circuit2->add_net (n3);
    n4 = new db::Net ();
    n4->set_name ("n4");
    circuit2->add_net (n4);
    n5 = new db::Net ();
    n5->set_name ("n5");
    circuit2->add_net (n5);

    db::SubCircuit *sc1 = new db::SubCircuit (circuit1, "SC1");
    circuit2->add_subcircuit (sc1);
    sc1->connect_pin (0, n1);
    sc1->connect_pin (1, n3);
    //  pin 2 unconnected
    sc1->connect_pin (3, n3);

    db::SubCircuit *sc2 = new db::SubCircuit (circuit1, "SC2");
    circuit2->add_subcircuit (sc2);
    sc2->connect_pin (0, n3);
    //  pin 1 unconnected
    sc2->connect_pin (2, n4);
    sc2->connect_pin (3, n3);

    size_t pid1 = circuit2->add_pin ("p1").id ();
    size_t pid2 = circuit2->add_pin ("p2").id ();
    size_t pid3 = circuit2->add_pin ("p3").id ();

    circuit2->connect_pin (pid1, n1);
    circuit2->connect_pin (pid2, n2);
    circuit2->connect_pin (pid3, n4);
  }

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter11.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter11_au.txt");

  compare_netlists (_this, path, au_path);

  path = tmp_file ("tmp_nwriter11b.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.set_use_net_names (true);
    writer.write (stream, nl, "written by unit test");
  }

  au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter11b_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(12_UniqueNetNames)
{
  db::LayoutToNetlist l2n;
  std::string l2n_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "same_net_names.l2n");
  l2n.load (l2n_path);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter12.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, *l2n.netlist (), "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter12_au.txt");

  compare_netlists (_this, path, au_path);

  path = tmp_file ("tmp_nwriter12b.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.set_use_net_names (true);
    writer.write (stream, *l2n.netlist (), "written by unit test");
  }

  au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter12b_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(13_WriterBJT3Devices)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *b3cls = new db::DeviceClassBJT3Transistor ();
  db::DeviceClass *b4cls = new db::DeviceClassBJT4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  b3cls->set_name ("B3CLS");
  b4cls->set_name ("B4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (b3cls);
  nl.add_device_class (b4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3, *n4;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);
  n4 = new db::Net ();
  n4->set_name ("n4");
  circuit1->add_net (n4);

  db::Device *ddev1 = new db::Device (b3cls);
  ddev1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AE, 0.25);
  ddev1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PE, 0.18);
  ddev1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AB, 1.2);
  ddev1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PB, 0.75);
  ddev1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AC, 1.0);
  ddev1->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PC, 0.6);
  db::Device *ddev2 = new db::Device (b3cls);
  ddev2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AE, 1.2);
  ddev2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PE, 2.5);
  ddev2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AB, 1.4);
  ddev2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PB, 2.8);
  ddev2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AC, 1.5);
  ddev2->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PC, 3.0);
  circuit1->add_device (ddev1);
  circuit1->add_device (ddev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();
  size_t pid3 = circuit1->add_pin ("p3").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);
  circuit1->connect_pin (pid3, n4);

  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("E"), n1);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n4);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("C"), n3);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("E"), n3);
  ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n4);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("C"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter13.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter13_au.txt");

  compare_netlists (_this, path, au_path);
}

TEST(14_WriterBJT4Devices)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *b3cls = new db::DeviceClassBJT3Transistor ();
  db::DeviceClass *b4cls = new db::DeviceClassBJT4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  b3cls->set_name ("B3CLS");
  b4cls->set_name ("B4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (b3cls);
  nl.add_device_class (b4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3, *n4, *n5;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);
  n4 = new db::Net ();
  n4->set_name ("n4");
  circuit1->add_net (n4);
  n5 = new db::Net ();
  n5->set_name ("n5");
  circuit1->add_net (n5);

  db::Device *ddev1 = new db::Device (b4cls);
  ddev1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AE, 0.25);
  ddev1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PE, 0.18);
  ddev1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AB, 1.2);
  ddev1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PB, 0.75);
  ddev1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AC, 1.0);
  ddev1->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PC, 0.6);
  db::Device *ddev2 = new db::Device (b4cls);
  ddev2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AE, 1.2);
  ddev2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PE, 2.5);
  ddev2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AB, 1.4);
  ddev2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PB, 2.8);
  ddev2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_AC, 1.5);
  ddev2->set_parameter_value (db::DeviceClassBJT4Transistor::param_id_PC, 3.0);
  circuit1->add_device (ddev1);
  circuit1->add_device (ddev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();
  size_t pid3 = circuit1->add_pin ("p3").id ();
  size_t pid4 = circuit1->add_pin ("p4").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);
  circuit1->connect_pin (pid3, n4);
  circuit1->connect_pin (pid4, n5);

  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("E"), n1);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n4);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("C"), n3);
  ddev1->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("S"), n5);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("E"), n3);
  ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("B"), n4);
  ddev2->connect_terminal (ddev2->device_class ()->terminal_id_for_name ("C"), n2);
  ddev2->connect_terminal (ddev1->device_class ()->terminal_id_for_name ("S"), n5);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter14.txt");
  {
    tl::OutputStream stream (path);
    db::NetlistSpiceWriter writer;
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter14_au.txt");

  compare_netlists (_this, path, au_path);
}


namespace {

class MyDelegate
  : public db::NetlistSpiceWriterDelegate
{
public:
  MyDelegate ()
    : db::NetlistSpiceWriterDelegate ()
  { }

  void write_header () const
  {
    emit_line ("*** My special header");
  }

  void write_device_intro (const db::DeviceClass &cls) const
  {
    emit_line ("*** My intro for class " + cls.name ());
  }

  void write_device (const db::Device &dev) const
  {
    emit_line ("*** Before device " + dev.expanded_name ());
    db::NetlistSpiceWriterDelegate::write_device (dev);
    emit_line ("*** After device " + dev.expanded_name ());
  }
};

}

TEST(20_Delegate)
{
  db::Netlist nl;

  db::DeviceClass *rcls = new db::DeviceClassResistor ();
  db::DeviceClass *ccls = new db::DeviceClassCapacitor ();
  db::DeviceClass *lcls = new db::DeviceClassInductor ();
  db::DeviceClass *dcls = new db::DeviceClassDiode ();
  db::DeviceClass *m3cls = new db::DeviceClassMOS3Transistor ();
  db::DeviceClass *m4cls = new db::DeviceClassMOS4Transistor ();

  rcls->set_name ("RCLS");
  lcls->set_name ("LCLS");
  ccls->set_name ("CCLS");
  dcls->set_name ("DCLS");
  m3cls->set_name ("M3CLS");
  m4cls->set_name ("M4CLS");

  nl.add_device_class (rcls);
  nl.add_device_class (lcls);
  nl.add_device_class (ccls);
  nl.add_device_class (dcls);
  nl.add_device_class (m3cls);
  nl.add_device_class (m4cls);

  db::Circuit *circuit1 = new db::Circuit ();
  circuit1->set_name ("C1");
  nl.add_circuit (circuit1);

  db::Net *n1, *n2, *n3;
  n1 = new db::Net ();
  n1->set_name ("n1");
  circuit1->add_net (n1);
  n2 = new db::Net ();
  n2->set_name ("n2");
  circuit1->add_net (n2);
  n3 = new db::Net ();
  n3->set_name ("n3");
  circuit1->add_net (n3);

  db::Device *rdev1 = new db::Device (rcls);
  rdev1->set_parameter_value (db::DeviceClassResistor::param_id_R, 1.7);
  db::Device *rdev2 = new db::Device (rcls);
  rdev2->set_parameter_value (db::DeviceClassResistor::param_id_R, 42e-6);
  circuit1->add_device (rdev1);
  circuit1->add_device (rdev2);

  size_t pid1 = circuit1->add_pin ("p1").id ();
  size_t pid2 = circuit1->add_pin ("p2").id ();

  circuit1->connect_pin (pid1, n1);
  circuit1->connect_pin (pid2, n2);

  rdev1->connect_terminal (rdev1->device_class ()->terminal_id_for_name ("A"), n1);
  rdev1->connect_terminal (rdev1->device_class ()->terminal_id_for_name ("B"), n3);
  rdev2->connect_terminal (rdev2->device_class ()->terminal_id_for_name ("A"), n3);
  rdev2->connect_terminal (rdev2->device_class ()->terminal_id_for_name ("B"), n2);

  //  verify against the input

  std::string path = tmp_file ("tmp_nwriter20.txt");
  {
    tl::OutputStream stream (path);
    MyDelegate delegate;
    db::NetlistSpiceWriter writer (&delegate);
    writer.write (stream, nl, "written by unit test");
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nwriter20_au.txt");

  compare_netlists (_this, path, au_path);
}
