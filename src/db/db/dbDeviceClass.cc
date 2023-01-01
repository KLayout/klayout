
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

#include "dbDeviceClass.h"
#include "dbDevice.h"
#include "dbNetlist.h"
#include "tlClassRegistry.h"

namespace db
{

// --------------------------------------------------------------------------------

/**
 *  @brief Returns the primary device class for both given devices
 *  One of the devices lives in a primary netlist. This one is taken for the device class.
 */
static const db::DeviceClass *primary_device_class (const db::Device &a, const db::Device &b)
{
  tl_assert (a.device_class () != 0);
  tl_assert (b.device_class () != 0);

  const db::DeviceClass *dca = a.device_class ()->primary_class () ? a.device_class ()->primary_class () : a.device_class ();
  const db::DeviceClass *dcb = b.device_class ()->primary_class () ? b.device_class ()->primary_class () : b.device_class ();

  if (dca != dcb) {
    //  different devices, same category while sorting devices - take the one with the "lower" name
    return dca->name () < dcb->name () ? dca : dcb;
  } else {
    return dca;
  }
}

// --------------------------------------------------------------------------------
//  EqualDeviceParameters implementation

//  NOTE: to allow rounding errors for parameter comparison, we use
//  a default relative tolerance.
const double default_relative_tolerance = 1e-6;

const double default_absolute_tolerance = 0.0;

static int compare_parameters (double pa, double pb, double absolute = default_absolute_tolerance, double relative = default_relative_tolerance)
{
  //  absolute value < 0 means: ignore this parameter (= always match)
  if (absolute < 0.0) {
    return 0;
  }

  double pa_min = pa - absolute;
  double pa_max = pa + absolute;

  double mean = 0.5 * (fabs (pa) + fabs (pb));
  pa_min -= mean * relative;
  pa_max += mean * relative;

  //  NOTE: parameter values may be small (e.g. pF for caps) -> no fixed epsilon

  double eps = (fabs (pa_max) + fabs(pa_min)) * 0.5e-10;

  if (pa_max < pb - eps) {
    return -1;
  } else if (pa_min > pb + eps) {
    return 1;
  } else {
    return 0;
  }
}

EqualDeviceParameters::EqualDeviceParameters ()
{
  //  .. nothing yet ..
}

EqualDeviceParameters::EqualDeviceParameters (size_t parameter_id, bool ignore)
{
  m_compare_set.push_back (std::make_pair (parameter_id, std::make_pair (ignore ? -1.0 : 0.0, 0.0)));
}

EqualDeviceParameters::EqualDeviceParameters (size_t parameter_id, double absolute, double relative)
{
  m_compare_set.push_back (std::make_pair (parameter_id, std::make_pair (std::max (0.0, absolute), std::max (0.0, relative))));
}

std::string EqualDeviceParameters::to_string () const
{
  std::string res;
  for (std::vector<std::pair<size_t, std::pair<double, double> > >::const_iterator c = m_compare_set.begin (); c != m_compare_set.end (); ++c) {
    if (!res.empty ()) {
      res += ";";
    }
    res += "#" + tl::to_string (c->first) + ":";
    if (c->second.first < 0.0) {
      res += "ignore";
    } else {
      res += "A" + tl::to_string (c->second.first) + "/R" + tl::to_string (c->second.second);
    }
  }
  return res;
}

bool EqualDeviceParameters::less (const db::Device &a, const db::Device &b) const
{
  for (std::vector<std::pair<size_t, std::pair<double, double> > >::const_iterator c = m_compare_set.begin (); c != m_compare_set.end (); ++c) {
    int cmp = compare_parameters (a.parameter_value (c->first), b.parameter_value (c->first), c->second.first, c->second.second);
    if (cmp != 0) {
      return cmp < 0;
    }
  }

  //  compare the remaining parameters with a default precision

  std::set<size_t> seen;
  for (std::vector<std::pair<size_t, std::pair<double, double> > >::const_iterator c = m_compare_set.begin (); c != m_compare_set.end (); ++c) {
    seen.insert (c->first);
  }

  const std::vector<db::DeviceParameterDefinition> &pd = primary_device_class (a, b)->parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (p->is_primary () && seen.find (p->id ()) == seen.end ()) {
      int cmp = compare_parameters (a.parameter_value (p->id ()), b.parameter_value (p->id ()));
      if (cmp != 0) {
        return cmp < 0;
      }
    }
  }

  return false;
}

EqualDeviceParameters &EqualDeviceParameters::operator+= (const EqualDeviceParameters &other)
{
  for (std::vector<std::pair<size_t, std::pair<double, double> > >::const_iterator c = other.m_compare_set.begin (); c != other.m_compare_set.end (); ++c) {
    m_compare_set.push_back (*c);
  }
  return *this;
}

// --------------------------------------------------------------------------------
//  AllDeviceParametersAreEqual class implementation

AllDeviceParametersAreEqual::AllDeviceParametersAreEqual (double relative)
  : m_relative (relative)
{
  //  .. nothing yet ..
}

bool AllDeviceParametersAreEqual::less (const db::Device &a, const db::Device &b) const
{
  const std::vector<db::DeviceParameterDefinition> &parameters = a.device_class ()->parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator c = parameters.begin (); c != parameters.end (); ++c) {
    int cmp = compare_parameters (a.parameter_value (c->id ()), b.parameter_value (c->id ()), 0.0, m_relative);
    if (cmp != 0) {
      return cmp < 0;
    }
  }

  return false;
}

// --------------------------------------------------------------------------------
//  DeviceClass class implementation

DeviceClass::DeviceClass ()
  : m_strict (false), mp_netlist (0), m_supports_parallel_combination (false), m_supports_serial_combination (false), mp_primary_class (0)
{
  // .. nothing yet ..
}

DeviceClass::DeviceClass (const DeviceClass &other)
  : gsi::ObjectBase (other), tl::Object (other), tl::UniqueId (other), m_strict (false), mp_netlist (0), m_supports_parallel_combination (false), m_supports_serial_combination (false), mp_primary_class (0)
{
  operator= (other);
}

DeviceClass &DeviceClass::operator= (const DeviceClass &other)
{
  if (this != &other) {

    m_terminal_definitions = other.m_terminal_definitions;
    m_parameter_definitions = other.m_parameter_definitions;
    m_name = other.m_name;
    m_description = other.m_description;
    m_strict = other.m_strict;
    mp_pc_delegate.reset (const_cast<DeviceParameterCompareDelegate *> (other.mp_pc_delegate.get ()));
    mp_device_combiner.reset (const_cast<DeviceCombiner *> (other.mp_device_combiner.get ()));
    m_supports_serial_combination = other.m_supports_serial_combination;
    m_supports_parallel_combination = other.m_supports_parallel_combination;
    m_equivalent_terminal_ids = other.m_equivalent_terminal_ids;

  }
  return *this;
}

const DeviceTerminalDefinition &DeviceClass::add_terminal_definition (const DeviceTerminalDefinition &pd)
{
  m_terminal_definitions.push_back (pd);
  m_terminal_definitions.back ().set_id (m_terminal_definitions.size () - 1);
  return m_terminal_definitions.back ();
}

void DeviceClass::clear_terminal_definitions ()
{
  m_terminal_definitions.clear ();
}

const DeviceTerminalDefinition *DeviceClass::terminal_definition (size_t id) const
{
  if (id < m_terminal_definitions.size ()) {
    return & m_terminal_definitions [id];
  } else {
    return 0;
  }
}

const DeviceParameterDefinition &DeviceClass::add_parameter_definition (const DeviceParameterDefinition &pd)
{
  m_parameter_definitions.push_back (pd);
  m_parameter_definitions.back ().set_id (m_parameter_definitions.size () - 1);
  return m_parameter_definitions.back ();
}

void DeviceClass::clear_parameter_definitions ()
{
  m_parameter_definitions.clear ();
}

const DeviceParameterDefinition *DeviceClass::parameter_definition (size_t id) const
{
  if (id < m_parameter_definitions.size ()) {
    return & m_parameter_definitions [id];
  } else {
    return 0;
  }
}

DeviceParameterDefinition *DeviceClass::parameter_definition_non_const (size_t id)
{
  if (id < m_parameter_definitions.size ()) {
    return & m_parameter_definitions [id];
  } else {
    return 0;
  }
}

bool DeviceClass::has_parameter_with_name (const std::string &name) const
{
  const std::vector<db::DeviceParameterDefinition> &pd = parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    if (i->name () == name) {
      return true;
    }
  }
  return false;
}

size_t DeviceClass::parameter_id_for_name (const std::string &name) const
{
  const std::vector<db::DeviceParameterDefinition> &pd = parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    if (i->name () == name) {
      return i->id ();
    }
  }
  throw tl::Exception (tl::to_string (tr ("Invalid parameter name")) + ": '" + name + "'");
}

bool DeviceClass::has_terminal_with_name (const std::string &name) const
{
  const std::vector<db::DeviceTerminalDefinition> &td = terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
    if (i->name () == name) {
      return true;
    }
  }
  return false;
}

size_t DeviceClass::terminal_id_for_name (const std::string &name) const
{
  const std::vector<db::DeviceTerminalDefinition> &td = terminal_definitions ();
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
    if (i->name () == name) {
      return i->id ();
    }
  }
  throw tl::Exception (tl::to_string (tr ("Invalid terminal name")) + ": '" + name + "'");
}

//  The default compare delegate
static EqualDeviceParameters default_compare;

bool DeviceClass::less (const db::Device &a, const db::Device &b)
{
  tl_assert (a.device_class () != 0);
  tl_assert (b.device_class () != 0);

  const db::DeviceParameterCompareDelegate *pcd = primary_device_class (a, b)->parameter_compare_delegate ();
  if (! pcd) {
    pcd = &default_compare;
  }

  return pcd->less (a, b);
}

bool DeviceClass::equal (const db::Device &a, const db::Device &b)
{
  tl_assert (a.device_class () != 0);
  tl_assert (b.device_class () != 0);

  const db::DeviceParameterCompareDelegate *pcd = primary_device_class (a, b)->parameter_compare_delegate ();
  if (! pcd) {
    pcd = &default_compare;
  }

  return ! pcd->less (a, b) && ! pcd->less (b, a);
}

// --------------------------------------------------------------------------------
//  DeviceClassTemplateBase class implementation

DeviceClassTemplateBase *
DeviceClassTemplateBase::template_by_name (const std::string &name)
{
  for (tl::Registrar<db::DeviceClassTemplateBase>::iterator i = tl::Registrar<db::DeviceClassTemplateBase>::begin (); i != tl::Registrar<db::DeviceClassTemplateBase>::end (); ++i) {
    if (i->name () == name) {
      return i.operator-> ();
    }
  }
  return 0;
}

DeviceClassTemplateBase *
DeviceClassTemplateBase::is_a (const db::DeviceClass *dc)
{
  for (tl::Registrar<db::DeviceClassTemplateBase>::iterator i = tl::Registrar<db::DeviceClassTemplateBase>::begin (); i != tl::Registrar<db::DeviceClassTemplateBase>::end (); ++i) {
    if (i->is_of (dc)) {
      return i.operator-> ();
    }
  }
  return 0;
}

}
