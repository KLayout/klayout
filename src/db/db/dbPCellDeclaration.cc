
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



#include "dbPCellDeclaration.h"

namespace db
{

// -----------------------------------------------------------------------------------------
//  ParameterStates implementation

ParameterStates::ParameterStates ()
  : m_states ()
{
  //  .. nothing yet ..
}

ParameterStates::ParameterStates (const ParameterStates &other)
  : m_states (other.m_states)
{
  //  .. nothing yet ..
}

ParameterStates::ParameterStates (ParameterStates &&other)
  : m_states (std::move (other.m_states))
{
  //  .. nothing yet ..
}

ParameterStates &
ParameterStates::operator= (const ParameterStates &other)
{
  if (this != &other) {
    m_states = other.m_states;
  }
  return *this;
}

void
ParameterStates::set_parameter (const std::string &name, const ParameterState &ps)
{
  m_states [name] = ps;
}

ParameterState &
ParameterStates::parameter (const std::string &name)
{
  return m_states [name];
}

const ParameterState &
ParameterStates::parameter (const std::string &name) const
{
  auto i = m_states.find (name);
  if (i == m_states.end ()) {
    static ParameterState empty;
    return empty;
  } else {
    return i->second;
  }
}

bool
ParameterStates::has_parameter (const std::string &name) const
{
  return m_states.find (name) != m_states.end ();
}

bool
ParameterStates::values_are_equal (const db::ParameterStates &other) const
{
  auto i = m_states.begin (), j = other.m_states.begin ();
  while (i != m_states.end () && j != other.m_states.end () && i->first == j->first && i->second.value () == j->second.value ()) {
    ++i; ++j;
  }
  return i == m_states.end () && j == other.m_states.end ();
}


// -----------------------------------------------------------------------------------------
//  PCellDeclaration implementation

PCellDeclaration::PCellDeclaration ()
  : m_ref_count (0), m_id (0), mp_layout (0), m_has_parameter_declarations (false)
{ 
  // .. nothing yet ..
}

void 
PCellDeclaration::add_ref ()
{
  ++m_ref_count;
}

void 
PCellDeclaration::release_ref ()
{
  --m_ref_count;
  if (m_ref_count <= 0) {
    delete this;
  }
}

const std::vector<PCellParameterDeclaration> &
PCellDeclaration::parameter_declarations () const
{
  if (! m_has_parameter_declarations || ! wants_parameter_declaration_caching ()) {
    std::vector<PCellParameterDeclaration> pcp = get_parameter_declarations ();
    //  NOTE: this ensures that reallocation of the vector only happens if the parameters
    //  change. This makes the returned reference more stable and iterators over this reference
    //  don't get invalidated so easily if wants_parameter_declaration_caching is false.
    if (m_parameter_declarations != pcp) {
      m_parameter_declarations = pcp;
    }
    m_has_parameter_declarations = true;
  }
  return m_parameter_declarations;
}

const std::string &
PCellDeclaration::parameter_name (size_t index)
{
  const std::vector<db::PCellParameterDeclaration> &pcp = parameter_declarations ();
  if (index < pcp.size ()) {
    return pcp [index].get_name ();
  } else {
    static std::string empty;
    return empty;
  }
}

pcell_parameters_type
PCellDeclaration::map_parameters (const std::map<size_t, tl::Variant> &param_by_name) const
{
  db::pcell_parameters_type new_param;
  size_t i = 0;
  const std::vector<db::PCellParameterDeclaration> &pcp = parameter_declarations ();
  for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end (); ++pd, ++i) {
    std::map<size_t, tl::Variant>::const_iterator p = param_by_name.find (i);
    if (p != param_by_name.end ()) {
      new_param.push_back (p->second);
    } else {
      new_param.push_back (pd->get_default ());
    }
  }

  return new_param;
}

pcell_parameters_type
PCellDeclaration::map_parameters (const std::map<std::string, tl::Variant> &param_by_name) const
{
  db::pcell_parameters_type new_param;
  const std::vector<db::PCellParameterDeclaration> &pcp = parameter_declarations ();
  for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end (); ++pd) {
    std::map<std::string, tl::Variant>::const_iterator p = param_by_name.find (pd->get_name ());
    if (p != param_by_name.end ()) {
      new_param.push_back (p->second);
    } else {
      new_param.push_back (pd->get_default ());
    }
  }

  return new_param;
}

std::map<std::string, tl::Variant>
PCellDeclaration::named_parameters (const pcell_parameters_type &pv) const
{
  std::map<std::string, tl::Variant> np;

  const std::vector<db::PCellParameterDeclaration> &pcp = parameter_declarations ();
  for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end (); ++pd) {
    size_t index = pd - pcp.begin ();
    if (index >= pv.size ()) {
      break;
    }
    np.insert (std::make_pair (pd->get_name (), pv [index]));
  }

  return np;
}

}

