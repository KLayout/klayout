
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

#include "dbNetlistSpiceReaderDelegate.h"
#include "dbNetlistSpiceReader.h"
#include "dbNetlistSpiceReaderExpressionParser.h"
#include "dbNetlist.h"
#include "dbCircuit.h"
#include "dbNetlistDeviceClasses.h"
#include "tlLog.h"
#include "tlExpression.h"
#include "tlClassRegistry.h"

namespace db
{

// ------------------------------------------------------------------------------------------------------

NetlistSpiceReaderOptions::NetlistSpiceReaderOptions ()
{
  //  ngspice defaults:
  default_values["M"]["AD"] = 0.0;
  default_values["M"]["AS"] = 0.0;
  default_values["M"]["W"] = 100e-6;
  default_values["M"]["L"] = 100e-6;

  scale = 1.0;
}

// ------------------------------------------------------------------------------------------------------

NetlistSpiceReaderDelegate::NetlistSpiceReaderDelegate ()
  : mp_netlist (0), m_options (), m_profile (), m_read_all_parameters (false)
{
  //  .. nothing yet ..
}

NetlistSpiceReaderDelegate::~NetlistSpiceReaderDelegate ()
{
  //  .. nothing yet ..
}

bool
NetlistSpiceReaderDelegate::read_all_parameters () const
{
  return m_read_all_parameters;
}

void
NetlistSpiceReaderDelegate::set_read_all_parameters (bool f)
{
  m_read_all_parameters = f;
}

bool
NetlistSpiceReaderDelegate::legacy_mode () const
{
  return m_legacy_mode;
}

void
NetlistSpiceReaderDelegate::set_legacy_mode (bool f)
{
  m_legacy_mode = f;
}

void NetlistSpiceReaderDelegate::set_netlist (db::Netlist *netlist, const std::string &profile)
{
  m_options = NetlistSpiceReaderOptions ();
  mp_netlist = netlist;
  m_profile = profile;
}

void NetlistSpiceReaderDelegate::do_start ()
{
  //  build the element/model to class map
  m_spice_profiles.clear ();

  for (auto dc = mp_netlist->begin_device_classes (); dc != mp_netlist->end_device_classes (); ++dc) {

    const db::DeviceClass *dcc = dc.operator-> ();
    if (dcc->has_spice_profile (m_profile) || dcc->has_spice_profile ("*")) {

      const db::DeviceClass::SpiceProfile &pf = dcc->spice_profile (m_profile);
      if (! pf.element.empty ()) {
        if (m_spice_profiles.find (std::make_pair (pf.element, dcc->name ())) != m_spice_profiles.end ()) {
          tl::warn << tl::sprintf (tl::to_string (tr ("Duplicate model name %s bound to element %s in profile %s")), dcc->name (), pf.element, m_profile);
        }
        m_spice_profiles.insert (std::make_pair (std::make_pair (pf.element, dcc->name ()), dcc));
      }

    }

  }

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

bool NetlistSpiceReaderDelegate::wants_subcircuit (const std::string &circuit_name)
{
  return m_spice_profiles.find (std::make_pair ("X", circuit_name)) != m_spice_profiles.end ();
}

std::string NetlistSpiceReaderDelegate::translate_net_name (const std::string &nn)
{
  return NetlistSpiceReader::unescape_name (nn);
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

void NetlistSpiceReaderDelegate::parse_element_components (const std::string &s, std::vector<std::string> &strings, std::map<std::string, tl::Variant> &pv, const std::map<std::string, tl::Variant> &variables)
{
  tl::Extractor ex (s.c_str ());
  bool in_params = false;

  while (! NetlistSpiceReader::at_eol (ex)) {

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

        std::string comp_name = NetlistSpiceReader::parse_component (ex);
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
  auto d = m_options.default_values.find (element);
  if (d != m_options.default_values.end ()) {
    for (auto i = d->second.begin (); i != d->second.end (); ++i) {
      pv.insert (std::make_pair (i->first, i->second));
    }
  }
}

void NetlistSpiceReaderDelegate::parse_element (const std::string &s, std::string &element, std::string &model, double &value, std::vector<std::string> &nn, std::map<std::string, tl::Variant> &pv, const std::map<std::string, tl::Variant> &variables)
{
  def_values_per_element (element, pv);
  parse_element_components (s, nn, pv, variables);

  if (! m_legacy_mode && element == "X") {

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

namespace {

class SPICEParameterEval
  : public tl::Eval
{
public:
  SPICEParameterEval (const std::string &name, const std::map<std::string, tl::Variant> &params, const std::map<std::string, const db::DeviceParameterDefinition *> &all_params, double value)
    : m_name (name), m_params (params), m_all_params (all_params), m_value (value)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void resolve_name (const std::string &name, const tl::EvalFunction *& /*function*/, const tl::Variant *&value, tl::Variant *& /*var*/)
  {
    if (name == "$") {
      value = &m_value;
      return;
    }

    auto p = m_params.find (name == "_" ? m_name : name);
    if (p != m_params.end ()) {
      value = &p->second;
      return;
    }

    auto pp = m_all_params.find (name == "_" ? m_name : name);
    if (pp != m_all_params.end ()) {
      if (pp->second) {
        value = &pp->second->default_value ();
        return;
      }
    }

    static tl::Variant nil;
    value = &nil;
  }

private:
  const std::string m_name;
  const std::map<std::string, tl::Variant> &m_params;
  const std::map<std::string, const db::DeviceParameterDefinition *> &m_all_params;
  tl::Variant m_value;
};

}

static tl::Variant
eval_parameter_expression (const std::string &name, const std::string &expr, const std::map<std::string, tl::Variant> &params, const std::map<std::string, const db::DeviceParameterDefinition *> &all_params, double value)
{
  //  shortcuts
  if (expr.empty ()) {

    return tl::Variant ();

  } else if (expr == "_") {

    auto p = params.find (name);
    return p != params.end () ? p->second : tl::Variant ();

  } else if (expr == "$") {

    return tl::Variant (value);

  } else {

    //  real evaluation
    SPICEParameterEval eval (name, params, all_params, value);
    return eval.eval (expr);

  }
}

static tl::Variant
default_from_value (const tl::Variant &v)
{
  if (v.is_long () || v.is_ulong ()) {
    return tl::Variant (long (0));
  } else if (v.is_double ()) {
    return tl::Variant (0.0);
  } else if (v.is_a_string ()) {
    return tl::Variant ("");
  } else {
    return tl::Variant ();
  }
}

static std::string default_model_name (const std::string &element, size_t nets)
{
  for (tl::Registrar<db::DeviceClassTemplateBase>::iterator i = tl::Registrar<db::DeviceClassTemplateBase>::begin (); i != tl::Registrar<db::DeviceClassTemplateBase>::end (); ++i) {
    if (i->spice_element () == element && i->spice_num_nets () == nets) {
      return i->name ();
    }
  }

  //  TODO: raise an error maybe?
  return "UNK";
}

static db::DeviceClass *bootstrap_device_class (db::Netlist *netlist, const std::string &name, const std::string &element, size_t nets)
{
  for (tl::Registrar<db::DeviceClassTemplateBase>::iterator i = tl::Registrar<db::DeviceClassTemplateBase>::begin (); i != tl::Registrar<db::DeviceClassTemplateBase>::end (); ++i) {

    if (i->spice_element () == element && i->spice_num_nets () == nets) {

      db::DeviceClass *cls = netlist->device_class_by_name (name);
      if (! cls) {
        cls = i->create ();
        cls->set_name (name);
        netlist->add_device_class (cls);
      }

      return cls;

    }

  }

  return 0;
}

const db::DeviceClass *NetlistSpiceReaderDelegate::device_class (const std::string &element, const std::string &model) const
{
  auto dp = m_spice_profiles.find (std::make_pair (element, model));
  if (dp != m_spice_profiles.end ()) {
    return dp->second;
  } else {
    return 0;
  }
}

bool NetlistSpiceReaderDelegate::element (db::Circuit *circuit, const std::string &element, const std::string &name, const std::string &model, double value, const std::vector<db::Net *> &nets, const std::map<std::string, tl::Variant> &params)
{
  std::string cn = model;

  //  use a default model name if none is given
  if (cn.empty ()) {
    cn = default_model_name (element, nets.size ());
  }

  db::DeviceClass *cls = circuit->netlist ()->device_class_by_name (cn);

  if (element.empty ()) {

    //  obtained through SPICE profile.
    tl_assert (cls != 0);

  } else if (! cls) {

    //  create a device class from the device templates
    cls = bootstrap_device_class (circuit->netlist (), cn, element, nets.size ());

    if (! cls) {

      std::vector<std::string> candidates;
      for (tl::Registrar<db::DeviceClassTemplateBase>::iterator i = tl::Registrar<db::DeviceClassTemplateBase>::begin (); i != tl::Registrar<db::DeviceClassTemplateBase>::end (); ++i) {
        if (i->spice_element () == element) {
          candidates.push_back (tl::sprintf (tl::to_string (tr ("%s (%d terminals)")), i->name (), int (nets.size ())));
        }
      }

      if (! candidates.empty ()) {
        error (tl::sprintf (tl::to_string (tr ("No matching device found on element '%s' with '%d' terminals. Candidates are: ")), element, int (nets.size ())) + tl::join (candidates, ", "));
      } else {
        error (tl::sprintf (tl::to_string (tr ("Element '%s' is not understood by SPICE parser")), element, int (nets.size ())));
      }

    }

  }

  const db::DeviceClass::SpiceProfile &sp = cls->spice_profile (m_profile);
  const std::vector<db::DeviceTerminalDefinition> &td = cls->terminal_definitions ();

  if (td.size () != nets.size ()) {
    error (tl::sprintf (tl::to_string (tr ("Wrong number of terminals: class '%s' expects %d, but %d are given")), cn, int (td.size ()), int (nets.size ())));
  }

  //  derive the terminal order from the SPICE profile

  std::vector<size_t> terminal_order;
  terminal_order.reserve (sp.terminal_order.size ());

  for (auto to = sp.terminal_order.begin (); to != sp.terminal_order.end (); ++to) {
    if (! cls->has_terminal_with_name (*to)) {
      error (tl::sprintf (tl::to_string (tr ("Device class '%s' and SPICE profile '%s': inconsistent terminal order - '%s' is not a valid terminal name")),
                          cls->name (), m_profile, *to));
    }
    terminal_order.push_back (cls->terminal_id_for_name (*to));
  }

  if (terminal_order.size () != td.size ()) {
    error (tl::sprintf (tl::to_string (tr ("Device class '%s' and SPICE profile '%s': inconsistent terminal order - '%d' terminals are defined, '%d' given in terminal order list")),
                        cls->name (), m_profile, int (td.size ()), int (terminal_order.size ())));
  }

  //  create the device

  db::Device *device = new db::Device (cls, name);
  circuit->add_device (device);

  //  make the device connections

  for (auto t = terminal_order.begin (); t != terminal_order.end (); ++t) {
    device->connect_terminal (*t, nets [t - terminal_order.begin ()]);
  }

  //  transfer parameters into device

  const std::map<std::string, std::string> &dict = sp.incoming_parameters;

  std::map<std::string, const db::DeviceParameterDefinition *> all_params;

  for (auto p = params.begin (); p != params.end (); ++p) {
    if (cls->has_parameter_with_name (p->first)) {
      all_params.insert (std::make_pair (p->first, cls->parameter_definition (cls->parameter_id_for_name (p->first))));
    } else {
      all_params.insert (std::make_pair (p->first, (const db::DeviceParameterDefinition *) 0));
    }
  }

  for (auto p = all_params.begin (); p != all_params.end (); ++p) {

    std::map<std::string, std::string>::const_iterator id;
    if ((id = dict.find (p->first)) != dict.end ()) {

      tl::Variant pv = eval_parameter_expression (p->first, id->second, params, all_params, value);
      if (! pv.is_nil ()) {
        if (p->second) {
          device->set_parameter_value (p->second->id (), pv);
        } else {
          device->set_parameter_value_create (p->first, pv, false, default_from_value (pv));
        }
      }

    } else if (p->second && p->second->is_primary () && (id = dict.find ("*!")) != dict.end ()) {

      tl::Variant pv = eval_parameter_expression (p->first, id->second, params, all_params, value);
      if (! pv.is_nil ()) {
        device->set_parameter_value (p->second->id (), pv);
      }

    } else if (p->second && ! p->second->is_primary () && (id = dict.find ("*?")) != dict.end ()) {

      tl::Variant pv = eval_parameter_expression (p->first, id->second, params, all_params, value);
      if (! pv.is_nil ()) {
        device->set_parameter_value (p->second->id (), pv);
      }

    } else if (p->second && (id = dict.find ("**")) != dict.end ()) {

      tl::Variant pv = eval_parameter_expression (p->first, id->second, params, all_params, value);
      if (! pv.is_nil ()) {
        device->set_parameter_value (p->second->id (), pv);
      }

    } else if ((id = dict.find ("*")) != dict.end ()) {

      tl::Variant pv = eval_parameter_expression (p->first, id->second, params, all_params, value);
      if (! pv.is_nil ()) {
        if (p->second) {
          device->set_parameter_value (p->second->id (), pv);
        } else {
          device->set_parameter_value_create (p->first, pv, false, default_from_value (pv));
        }
      }

    } else if (m_read_all_parameters) {

      auto pp = params.find (p->first);
      tl_assert (pp != params.end ());

      if (p->second) {
        device->set_parameter_value (p->second->id (), pp->second);
      } else {
        device->set_parameter_value_create (p->first, pp->second, false, default_from_value (pp->second));
      }

    }

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
    const tl::Variant &pv = device->parameter_value (i->id ());
    if (pv.is_double ()) {
      device->set_parameter_value (i->id (), pv.to_double () / i->si_scaling () * pow (m_options.scale, i->geo_scaling_exponent ()));
    }
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
