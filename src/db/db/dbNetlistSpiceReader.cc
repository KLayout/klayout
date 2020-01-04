
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

#include "tlStream.h"
#include "tlLog.h"
#include "tlString.h"
#include "tlFileUtils.h"
#include "tlUri.h"

#include <sstream>
#include <cctype>

namespace db
{

// ------------------------------------------------------------------------------------------------------

NetlistSpiceReaderDelegate::NetlistSpiceReaderDelegate ()
{
  //  .. nothing yet ..
}

NetlistSpiceReaderDelegate::~NetlistSpiceReaderDelegate ()
{
  //  .. nothing yet ..
}

void NetlistSpiceReaderDelegate::start (db::Netlist * /*netlist*/)
{
  //  .. nothing yet ..
}

void NetlistSpiceReaderDelegate::finish (db::Netlist * /*netlist*/)
{
  //  .. nothing yet ..
}

bool NetlistSpiceReaderDelegate::wants_subcircuit (const std::string & /*circuit_name*/)
{
  return false;
}

void NetlistSpiceReaderDelegate::error (const std::string &msg)
{
  throw tl::Exception (msg);
}

template <class Cls>
static db::DeviceClass *make_device_class (db::Circuit *circuit, const std::string &name)
{
  if (! circuit || ! circuit->netlist ()) {
    return 0;
  }

  db::DeviceClass *cls = circuit->netlist ()->device_class_by_name (name);
  if (! cls) {
    cls = new Cls ();
    cls->set_name (name);
    circuit->netlist ()->add_device_class (cls);
  }

  return cls;
}

bool NetlistSpiceReaderDelegate::element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const std::map<std::string, double> &pv)
{
  std::map<std::string, double> params = pv;

  double mult = 1.0;
  std::map<std::string, double>::const_iterator mp = params.find ("M");
  if (mp != params.end ()) {
    mult = mp->second;
  }

  if (mult < 1e-10) {
    error (tl::sprintf (tl::to_string (tr ("Invalid multiplier value (M=%.12g) - must not be zero or negative")), mult));
  }

  std::string cn = model;
  db::DeviceClass *cls = circuit->netlist ()->device_class_by_name (cn);

  if (cls) {

    //  use given class

  } else if (element == "R") {

    if (cn.empty ()) {
      cn = "RES";
    }
    cls = make_device_class<db::DeviceClassResistor> (circuit, cn);

    //  Apply multiplier
    value /= mult;

  } else if (element == "L") {

    if (cn.empty ()) {
      cn = "IND";
    }
    cls = make_device_class<db::DeviceClassInductor> (circuit, cn);

    //  Apply multiplier
    value /= mult;

  } else if (element == "C") {

    if (cn.empty ()) {
      cn = "CAP";
    }
    cls = make_device_class<db::DeviceClassCapacitor> (circuit, cn);

    //  Apply multiplier
    value *= mult;

  } else if (element == "D") {

    if (cn.empty ()) {
      cn = "DIODE";
    }
    cls = make_device_class<db::DeviceClassDiode> (circuit, cn);

    //  Apply multiplier to "A"
    std::map<std::string, double>::iterator p;
    p = params.find ("A");
    if (p != params.end ()) {
      p->second *= mult;
    }

  } else if (element == "Q") {

    if (nets.size () == 3) {
      if (cn.empty ()) {
        cn = "BJT3";
      }
      cls = make_device_class<db::DeviceClassBJT3Transistor> (circuit, cn);
    } else if (nets.size () == 4) {
      if (cn.empty ()) {
        cn = "BJT4";
      }
      cls = make_device_class<db::DeviceClassBJT4Transistor> (circuit, cn);
    } else {
      error (tl::to_string (tr ("'Q' element needs to have 3 or 4 terminals")));
    }

    //  Apply multiplier to "AE"
    std::map<std::string, double>::iterator p;
    p = params.find ("AE");
    if (p != params.end ()) {
      p->second *= mult;
    }

  } else if (element == "M") {

    if (nets.size () == 4) {
      if (cn.empty ()) {
        cn = "MOS4";
      }
      cls = make_device_class<db::DeviceClassMOS4Transistor> (circuit, cn);

      //  Apply multiplier to "W"
      std::map<std::string, double>::iterator p;
      p = params.find ("W");
      if (p != params.end ()) {
        p->second *= mult;
      }

    } else {
      error (tl::to_string (tr ("'M' element needs to have 4 terminals")));
    }
  } else {
    error (tl::sprintf (tl::to_string (tr ("Not a known element type: '%s'")), element));
  }

  const std::vector<db::DeviceTerminalDefinition> &td = cls->terminal_definitions ();
  if (td.size () != nets.size ()) {
    error (tl::sprintf (tl::to_string (tr ("Wrong number of terminals: class '%s' expects %d, but %d are given")), cn, int (td.size ()), int (nets.size ())));
  }

  db::Device *device = new db::Device (cls, name);
  circuit->add_device (device);

  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
    device->connect_terminal (t->id (), nets [t - td.begin ()]);
  }

  size_t defp = std::numeric_limits<size_t>::max ();
  if (dynamic_cast<db::DeviceClassCapacitor *> (cls)) {
    defp = db::DeviceClassCapacitor::param_id_C;
  } else if (dynamic_cast<db::DeviceClassResistor *> (cls)) {
    defp = db::DeviceClassResistor::param_id_R;
  } else if (dynamic_cast<db::DeviceClassInductor *> (cls)) {
    defp = db::DeviceClassInductor::param_id_L;
  }

  const std::vector<db::DeviceParameterDefinition> &pd = cls->parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    std::map<std::string, double>::const_iterator v = params.find (i->name ());
    if (v != params.end ()) {
      device->set_parameter_value (i->id (), v->second / i->si_scaling ());
    } else if (i->id () == defp) {
      device->set_parameter_value (i->id (), value / i->si_scaling ());
    }
  }

  return true;
}

// ------------------------------------------------------------------------------------------------------

static const char *allowed_name_chars = "_.:,!+$/&\\#[]|<>";

NetlistSpiceReader::NetlistSpiceReader (NetlistSpiceReaderDelegate *delegate)
  : mp_netlist (0), mp_stream (0), mp_delegate (delegate)
{
  static NetlistSpiceReaderDelegate std_delegate;
  if (! delegate) {
    mp_delegate.reset (&std_delegate);
  }
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
  m_global_nets.clear ();
  m_circuits_read.clear ();

  try {

    mp_delegate->start (&netlist);

    while (! at_end ()) {
      read_card ();
    }

    mp_delegate->finish (&netlist);
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
  tl::URI current_uri (mp_stream->source ());
  tl::URI new_uri (path);

  tl::InputStream *istream;
  if (current_uri.scheme ().empty () && new_uri.scheme ().empty ()) {
    if (tl::is_absolute (path)) {
      istream = new tl::InputStream (path);
    } else {
      istream = new tl::InputStream (tl::combine_path (tl::dirname (mp_stream->source ()), path));
    }
  } else {
    istream = new tl::InputStream (current_uri.resolved (new_uri).to_string ());
  }

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
    if (ex.test_without_case (".include") || ex.test_without_case (".inc")) {

      std::string path = read_name_with_case (ex);

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

bool NetlistSpiceReader::subcircuit_captured (const std::string &nc_name)
{
  std::map<std::string, bool>::const_iterator c = m_captured.find (nc_name);
  if (c != m_captured.end ()) {
    return c->second;
  } else {
    bool cap = mp_delegate->wants_subcircuit (nc_name);
    m_captured.insert (std::make_pair (nc_name, cap));
    return cap;
  }
}

bool NetlistSpiceReader::read_card ()
{
  std::string l = get_line ();
  if (l.empty ()) {
    return false;
  }

  tl::Extractor ex (l.c_str ());

  ex.skip ();
  char next_char = toupper (*ex);

  if (ex.test_without_case (".")) {

    //  control statement
    if (ex.test_without_case ("model")) {

      //  ignore model statements

    } else if (ex.test_without_case ("global")) {

      while (! ex.at_end ()) {
        std::string n = read_name (ex);
        m_global_nets.push_back (n);
      }

    } else if (ex.test_without_case ("subckt")) {

      std::string nc = read_name (ex);
      if (subcircuit_captured (nc)) {
        skip_circuit (ex);
      } else {
        read_circuit (ex, nc);
      }

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

  } else if (isalpha (next_char)) {

    ++ex;

    std::string name = read_name (ex);
    ensure_circuit ();

    std::string es;
    es.push_back (next_char);

    if (! read_element (ex, es, name)) {
      warn (tl::sprintf (tl::to_string (tr ("Element type '%c' ignored")), next_char));
    }

    ex.expect_end ();

  } else {
    warn (tl::to_string (tr ("Line ignored")));
  }

  return false;
}

void NetlistSpiceReader::error (const std::string &msg)
{
  throw tl::Exception (msg);
}

void NetlistSpiceReader::warn (const std::string &msg)
{
  std::string fmt_msg = tl::sprintf ("%s in %s, line %d", msg, mp_stream->source (), mp_stream->line_number () - 1);
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

    for (std::vector<std::string>::const_iterator gn = m_global_nets.begin (); gn != m_global_nets.end (); ++gn) {
      make_net (*gn);
    }

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

void NetlistSpiceReader::read_pin_and_parameters (tl::Extractor &ex, std::vector<std::string> &nn, std::map<std::string, double> &pv)
{
  bool in_params = false;

  while (! ex.at_end ()) {

    if (ex.test_without_case ("params:")) {

      in_params = true;

    } else {

      std::string n = read_name (ex);

      if (ex.test ("=")) {
        //  a parameter
        pv.insert (std::make_pair (n, read_value (ex)));
      } else {
        if (in_params) {
          error (tl::to_string (tr ("Missing '=' in parameter assignment")));
        }
        nn.push_back (n);
      }

    }

  }
}

inline static int hex_num (char c)
{
  if (c >= '0' && c <= '9') {
    return (int (c - '0'));
  } else if (c >= 'a' && c <= 'f') {
    return (int (c - 'f') + 10);
  } else {
    return -1;
  }
}

std::string NetlistSpiceReader::read_name_with_case (tl::Extractor &ex)
{
  std::string n;
  ex.read_word_or_quoted (n, allowed_name_chars);

  std::string nn;
  nn.reserve (n.size ());
  const char *cp = n.c_str ();
  while (*cp) {

    if (*cp == '\\' && cp[1]) {

      if (tolower (cp[1]) == 'x') {

        cp += 2;

        char c = 0;
        for (int i = 0; i < 2 && *cp; ++i) {
          int n = hex_num (*cp);
          if (n >= 0) {
            ++cp;
            c = c * 16 + char (n);
          } else {
            break;
          }
        }

        nn += c;

      } else {
        ++cp;
        nn += *cp++;
      }

    } else {
      nn += *cp++;
    }

  }

  return nn;
}

std::string NetlistSpiceReader::read_name (tl::Extractor &ex)
{
  //  TODO: allow configuring Spice reader as case sensitive?
  //  this is easy to do: just avoid to_upper here:
#if 1
  return tl::to_upper_case (read_name_with_case (ex));
#else
  return read_name_with_case (ex);
#endif
}

bool NetlistSpiceReader::read_element (tl::Extractor &ex, const std::string &element, const std::string &name)
{
  //  generic parse
  std::vector<std::string> nn;
  std::map<std::string, double> pv;

  std::string model;
  double value = 0.0;

  //  interpret the parameters according to the code
  if (element == "X") {

    //  subcircuit call:
    //  Xname n1 n2 ... nn circuit [params]

    read_pin_and_parameters (ex, nn, pv);

    if (nn.empty ()) {
      error (tl::to_string (tr ("No circuit name given for subcircuit call")));
    }

    model = nn.back ();
    nn.pop_back ();

  } else if (element == "R" || element == "C" || element == "L") {

    //  resistor, cap, inductor: two-terminal devices with a value
    //  Rname n1 n2 value
    //  Rname n1 n2 value model [params]
    //  Rname n1 n2 model [params]
    //  (same for C, L instead of R)

    while (! ex.at_end () && nn.size () < 2) {
      nn.push_back (read_name (ex));
    }

    if (nn.size () != 2) {
      error (tl::to_string (tr ("Two-terminal device needs two nets")));
    }

    tl::Extractor ve (ex);
    double vv = 0.0;
    if (ve.try_read (vv) || ve.test ("(")) {
      value = read_value (ex);
    }

    while (! ex.at_end ()) {
      std::string n = read_name (ex);
      if (ex.test ("=")) {
        pv [n] = read_value (ex);
      } else if (! model.empty ()) {
        error (tl::sprintf (tl::to_string (tr ("Too many arguments for two-terminal device (additional argumen is '%s')")), n));
      } else {
        model = n;
      }
    }

  } else {

    //  others: n-terminal devices with a model (last node)

    while (! ex.at_end ()) {
      std::string n = read_name (ex);
      if (ex.test ("=")) {
        pv [n] = read_value (ex);
      } else {
        nn.push_back (n);
      }
    }

    if (nn.empty ()) {
      error (tl::sprintf (tl::to_string (tr ("No model name given for element '%s'")), element));
    }

    model = nn.back ();
    nn.pop_back ();

    if (element == "M") {
      if (nn.size () != 4) {
        error (tl::to_string (tr ("'M' element must have four nodes")));
      }
    } else if (element == "Q") {
      if (nn.size () != 3 && nn.size () != 4) {
        error (tl::to_string (tr ("'Q' element must have three or four nodes")));
      }
    } else if (element == "D") {
      if (nn.size () != 2) {
        error (tl::to_string (tr ("'D' element must have two nodes")));
      }
    }

    //  TODO: other devices?

  }

  std::vector<db::Net *> nets;
  for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
    nets.push_back (make_net (*i));
  }

  if (element == "X" && ! subcircuit_captured (model)) {
    if (! pv.empty ()) {
      warn (tl::to_string (tr ("Circuit parameters are not allowed currently")));
    }
    read_subcircuit (name, model, nets);
    return true;
  } else {
    return mp_delegate->element (mp_circuit, element, name, model, value, nets, pv);
  }
}

void NetlistSpiceReader::read_subcircuit (const std::string &sc_name, const std::string &nc_name, const std::vector<db::Net *> &nets)
{
  db::Circuit *cc = mp_netlist->circuit_by_name (nc_name);
  if (! cc) {

    cc = new db::Circuit ();
    mp_netlist->add_circuit (cc);
    cc->set_name (nc_name);

    //  we'll make the names later ...
    for (std::vector<db::Net *>::const_iterator i = nets.begin (); i != nets.end (); ++i) {
      cc->add_pin (std::string ());
    }
    for (std::vector<std::string>::const_iterator gn = m_global_nets.begin (); gn != m_global_nets.end (); ++gn) {
      cc->add_pin (std::string ());
    }

  } else {

    if (cc->pin_count () != nets.size () + m_global_nets.size ()) {
      error (tl::sprintf (tl::to_string (tr ("Pin count mismatch between circuit definition and circuit call: %d expected, got %d")), int (cc->pin_count ()), int (nets.size ())));
    }

  }

  db::SubCircuit *sc = new db::SubCircuit (cc, sc_name);
  mp_circuit->add_subcircuit (sc);

  for (std::vector<db::Net *>::const_iterator i = nets.begin (); i != nets.end (); ++i) {
    sc->connect_pin (i - nets.begin (), *i);
  }

  for (std::vector<std::string>::const_iterator gn = m_global_nets.begin (); gn != m_global_nets.end (); ++gn) {
    db::Net *net = make_net (*gn);
    sc->connect_pin (gn - m_global_nets.begin () + nets.size (), net);
  }
}

void NetlistSpiceReader::skip_circuit (tl::Extractor & /*ex*/)
{
  while (! at_end ()) {

    std::string l = get_line ();
    tl::Extractor ex (l.c_str ());
    if (ex.test_without_case (".")) {

      //  control statement
      if (ex.test_without_case ("subckt")) {
        skip_circuit (ex);
      } else if (ex.test_without_case ("ends")) {
        break;
      }

    }

  }
}

void NetlistSpiceReader::read_circuit (tl::Extractor &ex, const std::string &nc)
{
  std::vector<std::string> nn;
  std::map<std::string, double> pv;
  read_pin_and_parameters (ex, nn, pv);

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
    for (std::vector<std::string>::const_iterator gn = m_global_nets.begin (); gn != m_global_nets.end (); ++gn) {
      cc->add_pin (std::string ());
    }

  } else {

    if (cc->pin_count () != nn.size () + m_global_nets.size ()) {
      error (tl::sprintf (tl::to_string (tr ("Pin count mismatch between implicit (through call) and explicit circuit definition: %d expected, got %d in circuit %s")), int (cc->pin_count ()), int (nn.size ()), nc));
    }

  }

  if (m_circuits_read.find (cc) != m_circuits_read.end ()) {
    error (tl::sprintf (tl::to_string (tr ("Redefinition of circuit %s")), nc));
  }
  m_circuits_read.insert (cc);

  std::auto_ptr<std::map<std::string, db::Net *> > n2n (mp_nets_by_name.release ());
  mp_nets_by_name.reset (0);

  std::swap (cc, mp_circuit);

  //  produce the explicit pins
  for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
    db::Net *net = make_net (*i);
    //  use the net name to name the pin (otherwise SPICE pins are always unnamed)
    size_t pin_id = i - nn.begin ();
    if (! i->empty ()) {
      mp_circuit->rename_pin (pin_id, net->name ());
    }
    mp_circuit->connect_pin (pin_id, net);
  }

  //  produce pins for the global nets
  for (std::vector<std::string>::const_iterator gn = m_global_nets.begin (); gn != m_global_nets.end (); ++gn) {
    db::Net *net = make_net (*gn);
    size_t pin_id = gn - m_global_nets.begin () + nn.size ();
    mp_circuit->rename_pin (pin_id, net->name ());
    mp_circuit->connect_pin (pin_id, net);
  }

  while (! at_end ()) {
    if (read_card ()) {
      break;
    }
  }

  mp_nets_by_name.reset (n2n.release ());
  std::swap (cc, mp_circuit);

  ex.expect_end ();
}

}
