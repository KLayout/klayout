
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

#include "tlStream.h"
#include "tlLog.h"

#include <sstream>
#include <cctype>

namespace db
{

static const char *allowed_name_chars = "_.:,!+$/&\\#[]|";

NetlistSpiceReader::NetlistSpiceReader ()
  : mp_netlist (0), mp_stream (0)
{
  //  .. nothing yet ..
}

NetlistSpiceReader::~NetlistSpiceReader ()
{
  //  .. nothing yet ..
}

void NetlistSpiceReader::read (tl::InputStream &stream, db::Netlist &netlist)
{
  mp_stream.reset (new tl::TextInputStream (stream));
  mp_netlist = &netlist;
  mp_circuit = 0;
  mp_nets_by_name.reset (0);

  try {

    while (! at_end ()) {
      read_element ();
    }

    finish ();

  } catch (tl::Exception &ex) {

    //  NOTE: because we do a peek to capture the "+" line continuation character, we're
    //  one line ahead.
    std::string fmt_msg = tl::sprintf ("%s in %s, line %d", ex.msg (), mp_stream->source (), mp_stream->line_number () - 1);
    finish ();
    throw tl::Exception (fmt_msg);

  } catch (...) {

    finish ();
    throw;

  }
}

void NetlistSpiceReader::finish ()
{
  while (! m_streams.empty ()) {
    pop_stream ();
  }

  mp_stream.reset (0);
  mp_netlist = 0;
  mp_circuit = 0;
  mp_nets_by_name.reset (0);
}

void NetlistSpiceReader::push_stream (const std::string &path)
{
  tl::InputStream *istream = new tl::InputStream (path);
  m_streams.push_back (std::make_pair (istream, mp_stream.release ()));
  mp_stream.reset (new tl::TextInputStream (*istream));
}

void NetlistSpiceReader::pop_stream ()
{
  if (! m_streams.empty ()) {

    mp_stream.reset (m_streams.back ().second);
    delete m_streams.back ().first;

    m_streams.pop_back ();

  }
}

bool NetlistSpiceReader::at_end ()
{
  return mp_stream->at_end () && m_streams.empty ();
}

std::string NetlistSpiceReader::get_line ()
{
  if (! m_stored_line.empty ()) {
    std::string l;
    l.swap (m_stored_line);
    return l;
  }

  std::string l;

  do {

    while (mp_stream->at_end ()) {
      if (m_streams.empty ()) {
        return std::string ();
      }
      pop_stream ();
    }

    l = mp_stream->get_line ();
    while (! mp_stream->at_end () && mp_stream->peek_char () == '+') {
      mp_stream->get_char ();
      l += mp_stream->get_line ();
    }

    tl::Extractor ex (l.c_str ());
    if (ex.test_without_case (".include")) {

      std::string path;
      ex.read_word_or_quoted (path, allowed_name_chars);

      push_stream (path);

      l.clear ();

    } else if (ex.at_end () || ex.test ("*")) {
      l.clear ();
    }

  } while (l.empty ());

  return l;
}

void NetlistSpiceReader::unget_line (const std::string &l)
{
  m_stored_line = l;
}

bool NetlistSpiceReader::read_element ()
{
  std::string l = get_line ();
  if (l.empty ()) {
    return false;
  }

  tl::Extractor ex (l.c_str ());

  const char *res_device_class_name = "RES";
  const char *cap_device_class_name = "CAP";
  const char *ind_device_class_name = "IND";

  if (ex.test_without_case (".")) {

    //  control statement
    if (ex.test_without_case ("model")) {

      //  ignore model statements

    } else if (ex.test_without_case ("subckt")) {

      read_circuit (ex);

    } else if (ex.test_without_case ("ends")) {

      return true;

    } else if (ex.test_without_case ("end")) {

      //  ignore end statements

    } else {

      std::string s;
      ex.read_word (s);
      s = tl::to_lower_case (s);
      warn (tl::to_string (tr ("Control statement ignored: ")) + s);

    }

  } else if (ex.test_without_case ("r")) {

    db::DeviceClass *dev_cls = mp_netlist->device_class_by_name (res_device_class_name);
    if (! dev_cls) {
      dev_cls = new db::DeviceClassResistor ();
      dev_cls->set_name (res_device_class_name);
      mp_netlist->add_device_class (dev_cls);
    }

    ensure_circuit ();
    read_device (dev_cls, db::DeviceClassResistor::param_id_R, ex);

  } else if (ex.test_without_case ("c")) {

    db::DeviceClass *dev_cls = mp_netlist->device_class_by_name (cap_device_class_name);
    if (! dev_cls) {
      dev_cls = new db::DeviceClassCapacitor ();
      dev_cls->set_name (cap_device_class_name);
      mp_netlist->add_device_class (dev_cls);
    }

    ensure_circuit ();
    read_device (dev_cls, db::DeviceClassCapacitor::param_id_C, ex);

  } else if (ex.test_without_case ("l")) {

    db::DeviceClass *dev_cls = mp_netlist->device_class_by_name (ind_device_class_name);
    if (! dev_cls) {
      dev_cls = new db::DeviceClassInductor ();
      dev_cls->set_name (ind_device_class_name);
      mp_netlist->add_device_class (dev_cls);
    }

    ensure_circuit ();
    read_device (dev_cls, db::DeviceClassInductor::param_id_L, ex);

  } else if (ex.test_without_case ("m")) {

    ensure_circuit ();
    read_mos4_device (ex);

  } else if (ex.test_without_case ("x")) {

    ensure_circuit ();
    read_subcircuit (ex);

  } else {

    char c = *ex.skip ();
    if (c) {
      warn (tl::sprintf (tl::to_string (tr ("Element type '%c' ignored")), c));
    }

  }

  return false;
}

void NetlistSpiceReader::error (const std::string &msg)
{
  throw tl::Exception (msg);
}

void NetlistSpiceReader::warn (const std::string &msg)
{
  std::string fmt_msg = tl::sprintf ("%s in %s, line %d", msg, mp_stream->source (), mp_stream->line_number ());
  tl::warn << fmt_msg;
}

double NetlistSpiceReader::read_atomic_value (tl::Extractor &ex)
{
  if (ex.test ("(")) {

    double v = read_dot_expr (ex);
    ex.expect (")");
    return v;

  } else {

    double v = 0.0;
    ex.read (v);

    double f = 1.0;
    if (*ex == 't' || *ex == 'T') {
      f = 1e12;
    } else if (*ex == 'g' || *ex == 'G') {
      f = 1e9;
    } else if (*ex == 'k' || *ex == 'K') {
      f = 1e3;
    } else if (*ex == 'm' || *ex == 'M') {
      f = 1e-3;
      if (ex.test_without_case ("meg")) {
        f = 1e6;
      }
    } else if (*ex == 'u' || *ex == 'U') {
      f = 1e-6;
    } else if (*ex == 'n' || *ex == 'N') {
      f = 1e-9;
    } else if (*ex == 'p' || *ex == 'P') {
      f = 1e-12;
    } else if (*ex == 'f' || *ex == 'F') {
      f = 1e-15;
    } else if (*ex == 'a' || *ex == 'A') {
      f = 1e-18;
    }
    while (*ex && isalpha (*ex)) {
      ++ex;
    }

    v *= f;
    return v;

  }
}

double NetlistSpiceReader::read_bar_expr (tl::Extractor &ex)
{
  double v = read_atomic_value (ex);
  while (true) {
    if (ex.test ("+")) {
      double vv = read_atomic_value (ex);
      v += vv;
    } else if (ex.test ("+")) {
      double vv = read_atomic_value (ex);
      v -= vv;
    } else {
      break;
    }
  }
  return v;
}

double NetlistSpiceReader::read_dot_expr (tl::Extractor &ex)
{
  double v = read_atomic_value (ex);
  while (true) {
    if (ex.test ("*")) {
      double vv = read_atomic_value (ex);
      v *= vv;
    } else if (ex.test ("/")) {
      double vv = read_atomic_value (ex);
      v /= vv;
    } else {
      break;
    }
  }
  return v;
}

double NetlistSpiceReader::read_value (tl::Extractor &ex)
{
  return read_dot_expr (ex);
}

void NetlistSpiceReader::ensure_circuit ()
{
  if (! mp_circuit) {

    mp_circuit = new db::Circuit ();
    //  TODO: make top name configurable
    mp_circuit->set_name (".TOP");
    mp_netlist->add_circuit (mp_circuit);

  }
}

db::Net *NetlistSpiceReader::make_net (const std::string &name)
{
  if (! mp_nets_by_name.get ()) {
    mp_nets_by_name.reset (new std::map<std::string, db::Net *> ());
  }

  std::map<std::string, db::Net *>::const_iterator n2n = mp_nets_by_name->find (name);

  db::Net *net = 0;
  if (n2n == mp_nets_by_name->end ()) {

    net = new db::Net ();
    net->set_name (name);
    mp_circuit->add_net (net);

    mp_nets_by_name->insert (std::make_pair (name, net));

  } else {
    net = n2n->second;
  }

  return net;
}

void NetlistSpiceReader::read_subcircuit (tl::Extractor &ex)
{
  std::string sc_name;
  ex.read_word_or_quoted (sc_name, allowed_name_chars);

  std::vector<std::string> nn;
  std::map<std::string, double> pv;

  while (! ex.at_end ()) {

    std::string n;
    ex.read_word_or_quoted (n, allowed_name_chars);

    if (ex.test ("=")) {
      //  a parameter
      pv.insert (std::make_pair (tl::to_upper_case (n), read_value (ex)));
    } else {
      nn.push_back (n);
    }

  }

  if (nn.empty ()) {
    error (tl::to_string (tr ("No circuit name given for subcircuit call")));
  }
  if (! pv.empty ()) {
    warn (tl::to_string (tr ("Circuit parameters are not allowed currently")));
  }

  std::string nc = nn.back ();
  nn.pop_back ();

  if (nn.empty ()) {
    error (tl::to_string (tr ("A circuit call needs at least one net")));
  }

  db::Circuit *cc = mp_netlist->circuit_by_name (nc);
  if (! cc) {
    cc = new db::Circuit ();
    mp_netlist->add_circuit (cc);
    cc->set_name (nc);
    for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
      cc->add_pin (std::string ());
    }
  } else {
    if (cc->pin_count () != nn.size ()) {
      error (tl::sprintf (tl::to_string (tr ("Pin count mismatch between circuit definition and circuit call: %d expected, got %d")), int (cc->pin_count ()), int (nn.size ())));
    }
  }

  db::SubCircuit *sc = new db::SubCircuit (cc, sc_name);
  mp_circuit->add_subcircuit (sc);

  for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
    db::Net *net = make_net (*i);
    sc->connect_pin (i - nn.begin (), net);
  }

  ex.expect_end ();
}

void NetlistSpiceReader::read_circuit (tl::Extractor &ex)
{
  std::string nc;
  ex.read_word_or_quoted (nc, allowed_name_chars);

  std::vector<std::string> nn;
  std::map<std::string, double> pv;

  while (! ex.at_end ()) {

    std::string n;
    ex.read_word_or_quoted (n, allowed_name_chars);

    if (ex.test ("=")) {
      //  a parameter
      pv.insert (std::make_pair (tl::to_upper_case (n), read_value (ex)));
    } else {
      nn.push_back (n);
    }

  }

  if (! pv.empty ()) {
    warn (tl::to_string (tr ("Circuit parameters are not allowed currently")));
  }

  db::Circuit *cc = mp_netlist->circuit_by_name (nc);
  if (! cc) {
    cc = new db::Circuit ();
    mp_netlist->add_circuit (cc);
    cc->set_name (nc);
    for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
      cc->add_pin (std::string ());
    }
  } else {
    if (cc->pin_count () != nn.size ()) {
      error (tl::sprintf (tl::to_string (tr ("Pin count mismatch between implicit (through call) and explicit circuit definition: %d expected, got %d")), int (cc->pin_count ()), int (nn.size ())));
    }
  }

  std::auto_ptr<std::map<std::string, db::Net *> > n2n (mp_nets_by_name.release ());
  mp_nets_by_name.reset (0);

  std::swap (cc, mp_circuit);

  for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
    db::Net *net = make_net (*i);
    mp_circuit->connect_pin (i - nn.begin (), net);
  }

  while (! at_end ()) {
    if (read_element ()) {
      break;
    }
  }

  mp_nets_by_name.reset (n2n.release ());
  std::swap (cc, mp_circuit);

  ex.expect_end ();
}

void NetlistSpiceReader::read_device (db::DeviceClass *dev_cls, size_t param_id, tl::Extractor &ex)
{
  std::string dn;
  ex.read_word_or_quoted (dn, allowed_name_chars);

  std::vector<std::string> nn;

  while (! ex.at_end () && nn.size () < 2) {
    nn.push_back (std::string ());
    ex.read_word_or_quoted (nn.back (), allowed_name_chars);
  }

  if (nn.size () != 2) {
    error (tl::to_string (tr ("Two-terminal device needs two nets")));
  }

  double v = read_value (ex);

  db::Device *dev = new db::Device (dev_cls, dn);
  mp_circuit->add_device (dev);

  for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
    db::Net *net = make_net (*i);
    dev->connect_terminal (i - nn.begin (), net);
  }

  dev->set_parameter_value (param_id, v);

  ex.expect_end ();
}

void NetlistSpiceReader::read_mos4_device (tl::Extractor &ex)
{
  std::string dn;
  ex.read_word_or_quoted (dn, allowed_name_chars);

  std::vector<std::string> nn;
  std::map<std::string, double> pv;

  while (! ex.at_end ()) {

    std::string n;
    ex.read_word_or_quoted (n, allowed_name_chars);

    if (ex.test ("=")) {
      //  a parameter
      pv.insert (std::make_pair (tl::to_upper_case (n), read_value (ex)));
    } else {
      nn.push_back (n);
    }

  }

  if (nn.empty ()) {
    error (tl::to_string (tr ("No model name given for MOS transistor element")));
  }

  std::string mn = nn.back ();
  nn.pop_back ();

  if (nn.size () != 4) {
    error (tl::to_string (tr ("A MOS transistor needs four nets")));
  }

  db::DeviceClass *dev_cls = mp_netlist->device_class_by_name (mn);
  if (! dev_cls) {
    dev_cls = new db::DeviceClassMOS4Transistor ();
    dev_cls->set_name (mn);
    mp_netlist->add_device_class (dev_cls);
  }

  db::Device *dev = new db::Device (dev_cls, dn);
  mp_circuit->add_device (dev);

  for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
    db::Net *net = make_net (*i);
    dev->connect_terminal (i - nn.begin (), net);
  }

  const std::vector<db::DeviceParameterDefinition> &pd = dev_cls->parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    std::map<std::string, double>::const_iterator v = pv.find (i->name ());
    if (v != pv.end ()) {
      //  by conventions, dimensions are in micrometer
      if (i->id () == db::DeviceClassMOS4Transistor::param_id_AD || i->id () == db::DeviceClassMOS4Transistor::param_id_AS) {
        dev->set_parameter_value (i->id (), v->second * 1e12);
      } else if (i->id () == db::DeviceClassMOS4Transistor::param_id_W
                 || i->id () == db::DeviceClassMOS4Transistor::param_id_L
                 || i->id () == db::DeviceClassMOS4Transistor::param_id_PD
                 || i->id () == db::DeviceClassMOS4Transistor::param_id_PS) {
        dev->set_parameter_value (i->id (), v->second * 1e6);
      }
    }
  }

  ex.expect_end ();
}

}
