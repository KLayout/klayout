
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
#include "tlTimer.h"
#include "tlLog.h"

#include <sstream>
#include <cctype>

namespace db
{

// ------------------------------------------------------------------------------------------------------

static const char *allowed_name_chars = "_.:,!+$/&\\#[]|<>";

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

static std::string unescape_name (const std::string &n)
{
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

bool NetlistSpiceReaderDelegate::control_statement(const std::string & /*line*/)
{
  return false;
}

bool NetlistSpiceReaderDelegate::wants_subcircuit (const std::string & /*circuit_name*/)
{
  return false;
}

std::string NetlistSpiceReaderDelegate::translate_net_name (const std::string &nn)
{
  return unescape_name (nn);
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

static std::string parse_component (tl::Extractor &ex)
{
  const char *cp = ex.skip ();
  const char *cp0 = cp;

  char quote = 0;
  unsigned int brackets = 0;

  while (*cp) {
    if (quote) {
      if (*cp == quote) {
        quote = 0;
      } else if (*cp == '\\' && cp[1]) {
        ++cp;
      }
    } else if ((isspace (*cp) || *cp == '=') && ! brackets) {
      break;
    } else if (*cp == '"' || *cp == '\'') {
      quote = *cp;
    } else if (*cp == '(') {
      ++brackets;
    } else if (*cp == ')') {
      if (brackets > 0) {
        --brackets;
      }
    }
    ++cp;
  }

  ex = tl::Extractor (cp);
  return std::string (cp0, cp - cp0);
}

void NetlistSpiceReaderDelegate::parse_element_components (const std::string &s, std::vector<std::string> &strings, std::map<std::string, double> &pv)
{
  tl::Extractor ex (s.c_str ());
  bool in_params = false;

  while (! ex.at_end ()) {

    if (ex.test_without_case ("params:")) {

      in_params = true;

    } else {

      tl::Extractor ex0 = ex;
      std::string n;

      if (ex.try_read_word (n) && ex.test ("=")) {
        //  a parameter. Note that parameter names are always made upper case.
        pv.insert (std::make_pair (tl::to_upper_case (n), read_value (ex)));
      } else {
        ex = ex0;
        if (in_params) {
          ex.error (tl::to_string (tr ("Invalid syntax for parameter assignment - needs keyword followed by '='")));
        }
        strings.push_back (parse_component (ex));
      }

    }

  }
}

double NetlistSpiceReaderDelegate::read_atomic_value (tl::Extractor &ex)
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

double NetlistSpiceReaderDelegate::read_bar_expr (tl::Extractor &ex)
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

double NetlistSpiceReaderDelegate::read_dot_expr (tl::Extractor &ex)
{
  double v = read_bar_expr (ex);
  while (true) {
    if (ex.test ("*")) {
      double vv = read_bar_expr (ex);
      v *= vv;
    } else if (ex.test ("/")) {
      double vv = read_bar_expr (ex);
      v /= vv;
    } else {
      break;
    }
  }
  return v;
}

double NetlistSpiceReaderDelegate::read_value (tl::Extractor &ex)
{
  return read_dot_expr (ex);
}

bool NetlistSpiceReaderDelegate::try_read_value (const std::string &s, double &value)
{
  tl::Extractor ve (s.c_str ());
  double vv = 0;
  if (ve.try_read (vv) || ve.test ("(")) {
    ve = tl::Extractor (s.c_str ());
    value = read_value (ve);
    return true;
  } else {
    return false;
  }
}

void NetlistSpiceReaderDelegate::parse_element (const std::string &s, const std::string &element, std::string &model, double &value, std::vector<std::string> &nn, std::map<std::string, double> &pv)
{
  parse_element_components (s, nn, pv);

  //  interpret the parameters according to the code
  if (element == "X") {

    //  subcircuit call:
    //  Xname n1 n2 ... nn circuit [params]

    if (nn.empty ()) {
      error (tl::to_string (tr ("No circuit name given for subcircuit call")));
    }

    model = nn.back ();
    nn.pop_back ();

  } else if (element == "R" || element == "C" || element == "L") {

    //  resistor, cap, inductor: two-terminal devices with a value
    //  Rname n1 n2 value
    //  Rname n1 n2 n3 value
    //  Rname n1 n2 value model [params]
    //  Rname n1 n2 n3 value model [params]
    //  Rname n1 n2 [params]
    //  Rname n1 n2 model [params]
    //  Rname n1 n2 n3 model [params]
    //  NOTE: there is no "Rname n1 n2 n3 [params]"!
    //  (same for C, L instead of R)

    if (nn.size () < 2) {
      error (tl::to_string (tr ("Not enough specs for a R, C or L device")));
    }

    std::map<std::string, double>::const_iterator rv = pv.find (element);
    if (rv != pv.end ()) {

      //  value given by parameter
      value = rv->second;

      if (nn.size () >= 3) {
        //  Rname n1 n2 model [params]
        //  Rname n1 n2 n3 model [params]
        model = nn.back ();
        nn.pop_back ();
      }

    } else if (nn.size () >= 3) {

      if (try_read_value (nn.back (), value)) {

        //  Rname n1 n2 value
        //  Rname n1 n2 n3 value
        nn.pop_back ();

      } else {

        //  Rname n1 n2 value model [params]
        //  Rname n1 n2 n3 value model [params]
        model = nn.back ();
        nn.pop_back ();
        if (! try_read_value (nn.back (), value)) {
          error (tl::to_string (tr ("Can't find a value for a R, C or L device")));
        } else {
          nn.pop_back ();
        }

      }

    }

  } else {

    //  others: n-terminal devices with a model (last node)

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

  if (element == "R") {

    if (nets.size () == 2) {
      if (cls) {
        if (! dynamic_cast<db::DeviceClassResistor *>(cls)) {
          error (tl::sprintf (tl::to_string (tr ("Class %s is not a resistor device class as required by 'R' element")), cn));
        }
      } else {
        if (cn.empty ()) {
          cn = "RES";
        }
        cls = make_device_class<db::DeviceClassResistor> (circuit, cn);
      }
    } else if (nets.size () == 3) {
      if (cls) {
        if (! dynamic_cast<db::DeviceClassResistorWithBulk *>(cls)) {
          error (tl::sprintf (tl::to_string (tr ("Class %s is not a three-terminal resistor device class as required by 'R' element")), cn));
        }
      } else {
        if (cn.empty ()) {
          cn = "RES3";
        }
        cls = make_device_class<db::DeviceClassResistorWithBulk> (circuit, cn);
      }
    } else {
      error (tl::to_string (tr ("A 'R' element requires two or three nets")));
    }

    //  Apply multiplier
    value /= mult;

  } else if (element == "L") {

    if (nets.size () == 2) {
      if (cls) {
        if (! dynamic_cast<db::DeviceClassInductor *>(cls)) {
          error (tl::sprintf (tl::to_string (tr ("Class %s is not a inductor device class as required by 'L' element")), cn));
        }
      } else {
        if (cn.empty ()) {
          cn = "IND";
        }
        cls = make_device_class<db::DeviceClassInductor> (circuit, cn);
      }
    } else {
      error (tl::to_string (tr ("A 'L' element requires two nets")));
    }

    //  Apply multiplier
    value /= mult;

  } else if (element == "C") {

    if (nets.size () == 2) {
      if (cls) {
        if (! dynamic_cast<db::DeviceClassCapacitor *>(cls)) {
          error (tl::sprintf (tl::to_string (tr ("Class %s is not a capacitor device class as required by 'C' element")), cn));
        }
      } else {
        if (cn.empty ()) {
          cn = "CAP";
        }
        cls = make_device_class<db::DeviceClassCapacitor> (circuit, cn);
      }
    } else if (nets.size () == 3) {
      if (cls) {
        if (! dynamic_cast<db::DeviceClassCapacitorWithBulk *>(cls)) {
          error (tl::sprintf (tl::to_string (tr ("Class %s is not a three-terminal capacitor device class as required by 'C' element")), cn));
        }
      } else {
        if (cn.empty ()) {
          cn = "CAP3";
        }
        cls = make_device_class<db::DeviceClassCapacitorWithBulk> (circuit, cn);
      }
    } else {
      error (tl::to_string (tr ("A 'C' element requires two or three nets")));
    }

    //  Apply multiplier
    value *= mult;

  } else if (element == "D") {

    if (cls) {
      if (! dynamic_cast<db::DeviceClassDiode *>(cls)) {
        error (tl::sprintf (tl::to_string (tr ("Class %s is not a diode device class as required by 'D' element")), cn));
      }
    } else {
      if (cn.empty ()) {
        cn = "DIODE";
      }
      cls = make_device_class<db::DeviceClassDiode> (circuit, cn);
    }

    //  Apply multiplier to "A"
    std::map<std::string, double>::iterator p;
    p = params.find ("A");
    if (p != params.end ()) {
      p->second *= mult;
    }

  } else if (element == "Q") {

    if (nets.size () != 3 && nets.size () != 4) {
      error (tl::to_string (tr ("'Q' element needs to have 3 or 4 terminals")));
    } else if (cls) {
      if (nets.size () == 3) {
        if (! dynamic_cast<db::DeviceClassBJT3Transistor *>(cls)) {
          error (tl::sprintf (tl::to_string (tr ("Class %s is not a 3-terminal BJT device class as required by 'Q' element")), cn));
        }
      } else {
        if (! dynamic_cast<db::DeviceClassBJT4Transistor *>(cls)) {
          error (tl::sprintf (tl::to_string (tr ("Class %s is not a 4-terminal BJT device class as required by 'Q' element")), cn));
        }
      }
    } else {
      if (nets.size () == 3) {
        if (cn.empty ()) {
          cn = "BJT3";
        }
        cls = make_device_class<db::DeviceClassBJT3Transistor> (circuit, cn);
      } else {
        if (cn.empty ()) {
          cn = "BJT4";
        }
        cls = make_device_class<db::DeviceClassBJT4Transistor> (circuit, cn);
      }
    }

    //  Apply multiplier to "AE"
    std::map<std::string, double>::iterator p;
    p = params.find ("AE");
    if (p != params.end ()) {
      p->second *= mult;
    }

  } else if (element == "M") {

    if (cls) {
      if (! dynamic_cast<db::DeviceClassMOS4Transistor *>(cls)) {
        error (tl::sprintf (tl::to_string (tr ("Class %s is not a 4-terminal MOS device class as required by 'M' element")), cn));
      }
    } else {
      if (nets.size () == 4) {
        if (cn.empty ()) {
          cn = "MOS4";
        }
        cls = make_device_class<db::DeviceClassMOS4Transistor> (circuit, cn);
      } else {
        error (tl::to_string (tr ("'M' element needs to have 4 terminals")));
      }
    }

    //  Apply multiplier to "W"
    std::map<std::string, double>::iterator p;
    p = params.find ("W");
    if (p != params.end ()) {
      p->second *= mult;
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

  std::vector<db::DeviceParameterDefinition> &pd = cls->parameter_definitions_non_const ();
  for (std::vector<db::DeviceParameterDefinition>::iterator i = pd.begin (); i != pd.end (); ++i) {
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

NetlistSpiceReader::SpiceReaderStream::SpiceReaderStream ()
  : mp_stream (0), m_owns_stream (false), mp_text_stream (0), m_line_number (0), m_stored_line (), m_has_stored_line (false)
{
  //  .. nothing yet ..
}

NetlistSpiceReader::SpiceReaderStream::~SpiceReaderStream ()
{
  close ();
}

void
NetlistSpiceReader::SpiceReaderStream::close ()
{
  delete mp_text_stream;
  mp_text_stream = 0;

  if (m_owns_stream) {
    delete mp_stream;
    mp_stream = 0;
    m_owns_stream = false;
  }
}

std::pair<std::string, bool>
NetlistSpiceReader::SpiceReaderStream::get_line ()
{
  if (at_end ()) {
    return std::make_pair (std::string (), false);
  }

  ++m_line_number;

  std::string l = m_has_stored_line ? m_stored_line : mp_text_stream->get_line ();

  m_has_stored_line = false;
  m_stored_line.clear ();

  while (! mp_text_stream->at_end ()) {

    std::string ll = mp_text_stream->get_line ();

    tl::Extractor ex (ll.c_str ());
    if (! ex.test ("+")) {
      m_stored_line = ll;
      m_has_stored_line = true;
      break;
    } else {
      ++m_line_number;
      l += " ";
      l += ex.get ();
    }

  }

  return std::make_pair (l, true);
}

int
NetlistSpiceReader::SpiceReaderStream::line_number () const
{
  return m_line_number;
}

std::string
NetlistSpiceReader::SpiceReaderStream::source () const
{
  return mp_stream->source ();
}

bool
NetlistSpiceReader::SpiceReaderStream::at_end () const
{
  return !m_has_stored_line && mp_text_stream->at_end ();
}

void
NetlistSpiceReader::SpiceReaderStream::set_stream (tl::InputStream &stream)
{
  close ();
  mp_stream = &stream;
  mp_text_stream = new tl::TextInputStream (stream);
  m_owns_stream = false;
  m_has_stored_line = false;
  m_line_number = 0;
}

void
NetlistSpiceReader::SpiceReaderStream::set_stream (tl::InputStream *stream)
{
  close ();
  mp_stream = stream;
  mp_text_stream = new tl::TextInputStream (*stream);
  m_owns_stream = true;
  m_has_stored_line = false;
  m_line_number = 0;
}

// ------------------------------------------------------------------------------------------------------

NetlistSpiceReader::NetlistSpiceReader (NetlistSpiceReaderDelegate *delegate)
  : mp_netlist (0), mp_delegate (delegate), m_stream ()
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
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Reading netlist ")) + stream.source ());

  m_stream.set_stream (stream);

  mp_netlist = &netlist;
  mp_circuit = 0;
  mp_anonymous_top_circuit = 0;
  mp_nets_by_name.reset (0);
  m_global_nets.clear ();
  m_circuits_read.clear ();

  //  SPICE netlists are case insensitive
  netlist.set_case_sensitive (false);

  try {

    mp_delegate->start (&netlist);

    while (! at_end ()) {
      read_card ();
    }

    build_global_nets ();

    mp_delegate->finish (&netlist);
    finish ();

  } catch (tl::Exception &ex) {

    //  NOTE: because we do a peek to capture the "+" line continuation character, we're
    //  one line ahead.
    std::string fmt_msg = tl::sprintf ("%s in %s, line %d", ex.msg (), m_stream.source (), m_stream.line_number ());
    finish ();
    throw tl::Exception (fmt_msg);

  } catch (...) {

    finish ();
    throw;

  }
}

void NetlistSpiceReader::build_global_nets ()
{
  for (std::vector<std::string>::const_iterator gn = m_global_nets.begin (); gn != m_global_nets.end (); ++gn) {

    for (db::Netlist::bottom_up_circuit_iterator c = mp_netlist->begin_bottom_up (); c != mp_netlist->end_bottom_up (); ++c) {

      if (c.operator-> () == mp_anonymous_top_circuit) {
        //  no pins for the anonymous top circuit
        continue;
      }

      db::Net *net = c->net_by_name (*gn);
      if (! net || net->pin_count () > 0) {
        //  only add a pin for a global net if there is a net with this name
        //  don't add a pin if it already has one
        continue;
      }

      const db::Pin &pin = c->add_pin (*gn);
      c->connect_pin (pin.id (), net);

      for (db::Circuit::refs_iterator r = c->begin_refs (); r != c->end_refs (); ++r) {

        db::SubCircuit &sc = *r;

        db::Net *pnet = sc.circuit ()->net_by_name (*gn);
        if (! pnet) {
          pnet = new db::Net ();
          pnet->set_name (*gn);
          sc.circuit ()->add_net (pnet);
        }

        sc.connect_pin (pin.id (), pnet);

      }

    }

  }
}


void NetlistSpiceReader::finish ()
{
  m_streams.clear ();
  m_stream.close ();

  mp_netlist = 0;
  mp_circuit = 0;
  mp_nets_by_name.reset (0);
}

void NetlistSpiceReader::push_stream (const std::string &path)
{
  tl::URI current_uri (m_stream.source ());
  tl::URI new_uri (path);

  tl::InputStream *istream;
  if (current_uri.scheme ().empty () && new_uri.scheme ().empty ()) {
    if (tl::is_absolute (path)) {
      istream = new tl::InputStream (path);
    } else {
      istream = new tl::InputStream (tl::combine_path (tl::dirname (m_stream.source ()), path));
    }
  } else {
    istream = new tl::InputStream (current_uri.resolved (new_uri).to_abstract_path ());
  }

  m_streams.push_back (SpiceReaderStream ());
  m_streams.back ().swap (m_stream);
  m_stream.set_stream (istream);
}

void NetlistSpiceReader::pop_stream ()
{
  if (! m_streams.empty ()) {
    m_stream.swap (m_streams.back ());
    m_streams.pop_back ();
  }
}

bool NetlistSpiceReader::at_end ()
{
  return m_stream.at_end () && m_streams.empty ();
}

std::string NetlistSpiceReader::get_line ()
{
  std::pair<std::string, bool> lp;

  while (true) {

    lp = m_stream.get_line ();
    if (! lp.second) {

      if (m_streams.empty ()) {
        break;
      } else {
        pop_stream ();
      }

    } else {

      tl::Extractor ex (lp.first.c_str ());
      if (ex.test_without_case (".include") || ex.test_without_case (".inc")) {

        std::string path;
        ex.read_word_or_quoted (path, allowed_name_chars);

        push_stream (path);

      } else if (ex.at_end () || ex.test ("*")) {

        //  skip empty and comment lines

      } else {
        break;
      }

    }

  }

  return lp.first;
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
        std::string n = mp_delegate->translate_net_name (read_name (ex));
        if (m_global_net_names.find (n) == m_global_net_names.end ()) {
          m_global_nets.push_back (n);
          m_global_net_names.insert (n);
        }
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

    } else if (! mp_delegate->control_statement (l)) {

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
  std::string fmt_msg = tl::sprintf ("%s in %s, line %d", msg, m_stream.source (), m_stream.line_number ());
  tl::warn << fmt_msg;
}

void NetlistSpiceReader::ensure_circuit ()
{
  if (! mp_circuit) {

    mp_circuit = new db::Circuit ();
    //  TODO: make top name configurable
    mp_circuit->set_name (".TOP");
    mp_anonymous_top_circuit = mp_circuit;
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

std::string NetlistSpiceReader::read_name (tl::Extractor &ex)
{
  std::string n;
  ex.read_word_or_quoted (n, allowed_name_chars);
  return mp_netlist->normalize_name (n);
}

bool NetlistSpiceReader::read_element (tl::Extractor &ex, const std::string &element, const std::string &name)
{
  //  generic parse
  std::vector<std::string> nn;
  std::map<std::string, double> pv;
  std::string model;
  double value = 0.0;

  mp_delegate->parse_element (ex.skip (), element, model, value, nn, pv);

  model = mp_netlist->normalize_name (model);

  std::vector<db::Net *> nets;
  for (std::vector<std::string>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
    nets.push_back (make_net (mp_delegate->translate_net_name (mp_netlist->normalize_name (*i))));
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

  } else {

    if (cc->pin_count () != nets.size ()) {
      error (tl::sprintf (tl::to_string (tr ("Pin count mismatch between circuit definition and circuit call: %d expected, got %d")), int (cc->pin_count ()), int (nets.size ())));
    }

  }

  db::SubCircuit *sc = new db::SubCircuit (cc, sc_name);
  mp_circuit->add_subcircuit (sc);

  for (std::vector<db::Net *>::const_iterator i = nets.begin (); i != nets.end (); ++i) {
    sc->connect_pin (i - nets.begin (), *i);
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
  mp_delegate->parse_element_components (ex.skip (), nn, pv);

  for (std::vector<std::string>::iterator i = nn.begin (); i != nn.end (); ++i) {
    *i = mp_delegate->translate_net_name (mp_netlist->normalize_name (*i));
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
      error (tl::sprintf (tl::to_string (tr ("Pin count mismatch between implicit (through call) and explicit circuit definition: %d expected, got %d in circuit %s")), int (cc->pin_count ()), int (nn.size ()), nc));
    }

  }

  if (m_circuits_read.find (cc) != m_circuits_read.end ()) {
    error (tl::sprintf (tl::to_string (tr ("Redefinition of circuit %s")), nc));
  }
  m_circuits_read.insert (cc);

  std::unique_ptr<std::map<std::string, db::Net *> > n2n (mp_nets_by_name.release ());
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

  while (! at_end ()) {
    if (read_card ()) {
      break;
    }
  }

  mp_nets_by_name.reset (n2n.release ());
  std::swap (cc, mp_circuit);
}

}
