
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include "dbNetlistSpiceReaderDelegate.h"
#include "dbNetlistSpiceReader.h"
#include "dbNetlistSpiceReaderExpressionParser.h"
#include "dbNetlist.h"
#include "dbCircuit.h"
#include "dbNetlistDeviceClasses.h"

namespace db
{

// ------------------------------------------------------------------------------------------------------

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

NetlistSpiceReaderOptions::NetlistSpiceReaderOptions ()
{
  scale = 1.0;
  defad = 0.0;
  defas = 0.0;
  //  ngspice defaults:
  defw = 100e-6;
  defl = 100e-6;
}

// ------------------------------------------------------------------------------------------------------

NetlistSpiceReaderDelegate::NetlistSpiceReaderDelegate ()
  : mp_netlist (0), m_options ()
{
  //  .. nothing yet ..
}

NetlistSpiceReaderDelegate::~NetlistSpiceReaderDelegate ()
{
  //  .. nothing yet ..
}

void NetlistSpiceReaderDelegate::set_netlist (db::Netlist *netlist)
{
  m_options = NetlistSpiceReaderOptions ();
  mp_netlist = netlist;
}

void NetlistSpiceReaderDelegate::do_start ()
{
  start (mp_netlist);
}

void NetlistSpiceReaderDelegate::do_finish ()
{
  finish (mp_netlist);
}

void NetlistSpiceReaderDelegate::start (db::Netlist * /*netlist*/)
{
  //  .. nothing yet ..
}

void NetlistSpiceReaderDelegate::finish (db::Netlist * /*netlist*/)
{
  //  .. nothing yet ..
}

bool NetlistSpiceReaderDelegate::control_statement (const std::string & /*line*/)
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

void NetlistSpiceReaderDelegate::parse_element_components (const std::string &s, std::vector<std::string> &strings, std::map<std::string, tl::Variant> &pv, const std::map<std::string, tl::Variant> &variables)
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

        //  a parameter
        std::string pn = mp_netlist ? mp_netlist->normalize_name (n) : tl::to_upper_case (n);
        pv [pn] = read_value (ex, variables, pv);

      } else {

        //  a net/model component
        ex = ex0;
        if (in_params) {
          ex.error (tl::to_string (tr ("Invalid syntax for parameter assignment - needs keyword followed by '='")));
        }

        std::string comp_name = parse_component (ex);
        comp_name = mp_netlist ? mp_netlist->normalize_name (comp_name) : tl::to_upper_case (comp_name);

        //  resolve variables if string type
        auto v = variables.find (comp_name);
        if (v != variables.end ()) {
          if (v->second.is_a_string ()) {
            strings.push_back (v->second.to_string ());
          } else if (v->second.can_convert_to_double ()) {
            //  NOTE: this allows using a variable name "x" instead of "x=x"
            pv [comp_name] = v->second;
          } else {
            strings.push_back (comp_name);
          }
        } else {
          strings.push_back (comp_name);
        }

      }

    }

  }
}

void NetlistSpiceReaderDelegate::def_values_per_element (const std::string &element, std::map<std::string, tl::Variant> &pv)
{
  if (element == "M") {

    pv.insert (std::make_pair ("W", m_options.defw));
    pv.insert (std::make_pair ("L", m_options.defl));
    pv.insert (std::make_pair ("AD", m_options.defad));
    pv.insert (std::make_pair ("AS", m_options.defas));

  }
}

void NetlistSpiceReaderDelegate::parse_element (const std::string &s, const std::string &element, std::string &model, double &value, std::vector<std::string> &nn, std::map<std::string, tl::Variant> &pv, const std::map<std::string, tl::Variant> &variables)
{
  def_values_per_element (element, pv);
  parse_element_components (s, nn, pv, variables);

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
      error (tl::to_string (tr ("Not enough specs (nodes, value, model) for a R, C or L device")));
    } else if (nn.size () > 5) {
      error (tl::to_string (tr ("Too many specs (nodes, value, model) for a R, C or L device")));
    }

    //  Variations are (here for "C" element):
    //  (1) Cname n1 n2 C=value [other params]
    //  (2) Cname n1 n2 value [params]
    //  (3) Cname n1 n2 model C=value [other params]
    //      Cname n1 n2 n3 C=value [other params] -> not supported, cannot tell from (3) without further analysis
    //  (4) Cname n1 n2 model value [params]
    //      Cname n1 n2 n3 value [params] -> not supported, cannot tell from (4) without further analysis
    //  (5) Cname n1 n2 n3 model C=value [other params]
    //  (6) Cname n1 n2 value model [params]
    //  (7) Cname n1 n2 n3 model value [params]
    //  (8) Cname n1 n2 n3 value model [params]

    auto rv = pv.find (element);

    bool has_value = false;
    if (nn.size () == 2) {
      if (rv != pv.end ()) {
        value = rv->second.to_double ();   //  (1)
        has_value = true;
      }
    } else if (nn.size () == 3) {
      if (try_read_value (nn.back (), value, variables)) {
        has_value = true;     //  (2)
        nn.pop_back ();
      } else {
        model = nn.back ();   //  (3)
        nn.pop_back ();
        if (rv != pv.end ()) {
          value = rv->second.to_double ();
          has_value = true;
        }
      }
    } else if (nn.size () == 4) {
      if (try_read_value (nn.back (), value, variables)) {
        has_value = true;     //  (4)
        nn.pop_back ();
      } else if (rv != pv.end ()) {
        value = rv->second.to_double ();   //  (5)
        has_value = true;
        model = nn.back ();
        nn.pop_back ();
      } else if (try_read_value (nn[2], value, variables)) {
        has_value = true;     //  (6)
        model = nn.back ();
        nn.pop_back ();
        nn.pop_back ();
      } else {
        model = nn.back ();   //  fall back to (5)
        nn.pop_back ();
      }
    } else {
      if (try_read_value (nn.back (), value, variables)) {
        has_value = true;     //  (7)
        nn.pop_back ();
        model = nn.back ();
        nn.pop_back ();
      } else if (try_read_value (nn[3], value, variables)) {
        has_value = true;     //  (8)
        model = nn.back ();
        nn.pop_back ();
        nn.pop_back ();
      }
    }

    if (rv != pv.end ()) {
      pv.erase (rv);
    }

    if (! has_value) {
      error (tl::to_string (tr ("Can't find a value for a R, C or L device")));
    }

    //  store the value under the element name always
    pv[element] = tl::Variant (value);

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

bool NetlistSpiceReaderDelegate::element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const std::map<std::string, tl::Variant> &pv)
{
  std::map<std::string, tl::Variant> params = pv;
  std::vector<size_t> terminal_order;

  double mult = 1.0;
  auto mp = params.find ("M");
  if (mp != params.end ()) {
    mult = mp->second.to_double ();
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

    //  Apply multiplier (divider, according to ngspice manual)
    value /= mult;
    params["R"] = tl::Variant (value);

    //  Apply multiplier to other parameters
    static const char *scale_params[] = { "A", "P", "W" };
    for (size_t i = 0; i < sizeof (scale_params) / sizeof (scale_params[0]); ++i) {
      auto p = params.find (scale_params [i]);
      if (p != params.end ()) {
        p->second = tl::Variant (p->second.to_double () * mult);
      }
    }

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

    //  Apply multiplier (divider, according to ngspice manual)
    value /= mult;
    params["L"] = tl::Variant (value);

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
    params["C"] = tl::Variant (value);

    //  Apply multiplier to other parameters
    static const char *scale_params[] = { "A", "P" };
    for (size_t i = 0; i < sizeof (scale_params) / sizeof (scale_params[0]); ++i) {
      auto p = params.find (scale_params [i]);
      if (p != params.end ()) {
        p->second = tl::Variant (p->second.to_double () * mult);
      }
    }

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

    //  Apply multiplier
    static const char *scale_params[] = { "A", "P" };
    for (size_t i = 0; i < sizeof (scale_params) / sizeof (scale_params[0]); ++i) {
      auto p = params.find (scale_params [i]);
      if (p != params.end ()) {
        p->second = tl::Variant (p->second.to_double () * mult);
      }
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

    //  Apply multiplier
    static const char *scale_params[] = { "AE", "PE", "AB", "PB", "AC", "PC" };
    for (size_t i = 0; i < sizeof (scale_params) / sizeof (scale_params[0]); ++i) {
      auto p = params.find (scale_params [i]);
      if (p != params.end ()) {
        p->second = tl::Variant (p->second.to_double () * mult);
      }
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

    //  Apply multiplier
    static const char *scale_params[] = { "W", "AD", "AS", "PD", "PS" };
    for (size_t i = 0; i < sizeof (scale_params) / sizeof (scale_params[0]); ++i) {
      auto p = params.find (scale_params [i]);
      if (p != params.end ()) {
        p->second = tl::Variant (p->second.to_double () * mult);
      }
    }

    //  issue #1304
    terminal_order.push_back (DeviceClassMOS4Transistor::terminal_id_D);
    terminal_order.push_back (DeviceClassMOS4Transistor::terminal_id_G);
    terminal_order.push_back (DeviceClassMOS4Transistor::terminal_id_S);
    terminal_order.push_back (DeviceClassMOS4Transistor::terminal_id_B);

  } else {
    error (tl::sprintf (tl::to_string (tr ("Not a known element type: '%s'")), element));
  }

  const std::vector<db::DeviceTerminalDefinition> &td = cls->terminal_definitions ();
  if (td.size () != nets.size ()) {
    error (tl::sprintf (tl::to_string (tr ("Wrong number of terminals: class '%s' expects %d, but %d are given")), cn, int (td.size ()), int (nets.size ())));
  }

  db::Device *device = new db::Device (cls, name);
  circuit->add_device (device);

  if (terminal_order.empty ()) {
    for (auto t = td.begin (); t != td.end (); ++t) {
      device->connect_terminal (t->id (), nets [t - td.begin ()]);
    }
  } else {
    for (auto t = terminal_order.begin (); t != terminal_order.end (); ++t) {
      device->connect_terminal (*t, nets [t - terminal_order.begin ()]);
    }
  }


  std::vector<db::DeviceParameterDefinition> &pd = cls->parameter_definitions_non_const ();
  for (std::vector<db::DeviceParameterDefinition>::iterator i = pd.begin (); i != pd.end (); ++i) {
    auto v = params.find (i->name ());
    double pv = 0.0;
    if (v != params.end ()) {
      pv = v->second.to_double ();
    } else {
      continue;
    }
    device->set_parameter_value (i->id (), pv);
  }

  apply_parameter_scaling (device);
  return true;
}

void
NetlistSpiceReaderDelegate::apply_parameter_scaling (db::Device *device) const
{
  if (! device || ! device->device_class ()) {
    return;
  }

  const std::vector<db::DeviceParameterDefinition> &pd = device->device_class ()->parameter_definitions ();
  for (auto i = pd.begin (); i != pd.end (); ++i) {
    double pv = device->parameter_value (i->id ());
    device->set_parameter_value (i->id (), pv / i->si_scaling () * pow (m_options.scale, i->geo_scaling_exponent ()));
  }
}

tl::Variant
NetlistSpiceReaderDelegate::read_value (tl::Extractor &ex, const std::map<std::string, tl::Variant> &variables)
{
  NetlistSpiceReaderExpressionParser parser (&variables);
  return parser.read (ex);
}

tl::Variant
NetlistSpiceReaderDelegate::read_value (tl::Extractor &ex, const std::map<std::string, tl::Variant> &variables1, const std::map<std::string, tl::Variant> &variables2)
{
  NetlistSpiceReaderExpressionParser parser (&variables1, &variables2);
  return parser.read (ex);
}

bool
NetlistSpiceReaderDelegate::try_read_value (const std::string &s, double &v, const std::map<std::string, tl::Variant> &variables)
{
  NetlistSpiceReaderExpressionParser parser (&variables);

  tl::Variant vv;
  tl::Extractor ex (s.c_str ());
  bool res = parser.try_read (ex, vv);

  if (res && ! vv.can_convert_to_double ()) {
    res = false;
  }
  if (res) {
    v = vv.to_double ();
  }

  return res;
}

}
