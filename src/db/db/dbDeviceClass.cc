
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

#include "dbDeviceClass.h"
#include "dbDevice.h"
#include "tlClassRegistry.h"

namespace db
{

// --------------------------------------------------------------------------------
//  EqualDeviceParameters implementation

static int compare_parameters (double pa, double pb, double absolute, double relative)
{
  double pa_min = pa - absolute;
  double pa_max = pa + absolute;

  double mean = 0.5 * (fabs (pa) + fabs (pb));
  pa_min -= mean * relative;
  pa_max += mean * relative;

  //  NOTE: parameter values may be small (e.g. pF for caps) -> no epsilon

  if (pa_max < pb) {
    return -1;
  } else if (pa_min > pb) {
    return 1;
  } else {
    return 0;
  }
}

EqualDeviceParameters::EqualDeviceParameters ()
{
  //  .. nothing yet ..
}

EqualDeviceParameters::EqualDeviceParameters (size_t parameter_id)
{
  m_compare_set.push_back (std::make_pair (parameter_id, std::make_pair (0.0, 0.0)));
}

EqualDeviceParameters::EqualDeviceParameters (size_t parameter_id, double relative, double absolute)
{
  m_compare_set.push_back (std::make_pair (parameter_id, std::make_pair (relative, absolute)));
}

bool EqualDeviceParameters::less (const db::Device &a, const db::Device &b) const
{
  for (std::vector<std::pair<size_t, std::pair<double, double> > >::const_iterator c = m_compare_set.begin (); c != m_compare_set.end (); ++c) {
    int cmp = compare_parameters (a.parameter_value (c->first), b.parameter_value (c->first), c->second.first, c->second.second);
    if (cmp != 0) {
      return cmp < 0;
    }
  }

  return false;
}

bool EqualDeviceParameters::equal (const db::Device &a, const db::Device &b) const
{
  for (std::vector<std::pair<size_t, std::pair<double, double> > >::const_iterator c = m_compare_set.begin (); c != m_compare_set.end (); ++c) {
    int cmp = compare_parameters (a.parameter_value (c->first), b.parameter_value (c->first), c->second.first, c->second.second);
    if (cmp != 0) {
      return false;
    }
  }

  return true;
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

bool AllDeviceParametersAreEqual::equal (const db::Device &a, const db::Device &b) const
{
  const std::vector<db::DeviceParameterDefinition> &parameters = a.device_class ()->parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator c = parameters.begin (); c != parameters.end (); ++c) {
    int cmp = compare_parameters (a.parameter_value (c->id ()), b.parameter_value (c->id ()), 0.0, m_relative);
    if (cmp != 0) {
      return false;
    }
  }

  return true;
}

// --------------------------------------------------------------------------------
//  DeviceClass class implementation

DeviceClass::DeviceClass ()
  : m_strict (false), mp_netlist (0)
{
  // .. nothing yet ..
}

DeviceClass::DeviceClass (const DeviceClass &other)
  : gsi::ObjectBase (other), tl::Object (other), tl::UniqueId (other), m_strict (false), mp_netlist (0)
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

//  NOTE: to allow rounding errors for parameter comparison, we use
//  a default relative tolerance.
const double relative_tolerance = 1e-6;

bool DeviceClass::less (const db::Device &a, const db::Device &b)
{
  tl_assert (a.device_class () != 0);
  tl_assert (b.device_class () != 0);

  const db::DeviceParameterCompareDelegate *pcd = a.device_class ()->mp_pc_delegate.get ();
  if (! pcd) {
    pcd = b.device_class ()->mp_pc_delegate.get ();
  }

  if (pcd != 0) {
    return pcd->less (a, b);
  } else {

    const std::vector<db::DeviceParameterDefinition> &pd = a.device_class ()->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
      if (! p->is_primary ()) {
        continue;
      }
      int cmp = compare_parameters (a.parameter_value (p->id ()), b.parameter_value (p->id ()), 0.0, relative_tolerance);
      if (cmp != 0) {
        return cmp < 0;
      }
    }

    return false;

  }
}

bool DeviceClass::equal (const db::Device &a, const db::Device &b)
{
  tl_assert (a.device_class () != 0);
  tl_assert (b.device_class () != 0);

  const db::DeviceParameterCompareDelegate *pcd = a.device_class ()->mp_pc_delegate.get ();
  if (! pcd) {
    pcd = b.device_class ()->mp_pc_delegate.get ();
  }

  if (pcd != 0) {
    return pcd->equal (a, b);
  } else {

    const std::vector<db::DeviceParameterDefinition> &pd = a.device_class ()->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
      if (! p->is_primary ()) {
        continue;
      }
      int cmp = compare_parameters (a.parameter_value (p->id ()), b.parameter_value (p->id ()), 0.0, relative_tolerance);
      if (cmp != 0) {
        return false;
      }
    }

    return true;

  }
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
