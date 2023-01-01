
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

#include "layNetColorizer.h"
#include "dbNet.h"
#include "dbCircuit.h"

namespace lay
{

// ----------------------------------------------------------------------------------
//  NetColorizer implementation

NetColorizer::NetColorizer ()
{
  m_auto_colors_enabled = false;
  m_update_needed = false;
  m_signals_enabled = true;
}

void
NetColorizer::configure (const tl::Color &marker_color, const lay::ColorPalette *auto_colors)
{
  m_marker_color = marker_color;
  if (auto_colors) {
    m_auto_colors = *auto_colors;
    m_auto_colors_enabled = true;
  } else {
    m_auto_colors_enabled = false;
  }

  emit_colors_changed ();
}

bool
NetColorizer::has_color_for_net (const db::Net *net)
{
  return net != 0 && (m_auto_colors_enabled || m_custom_color.find (net) != m_custom_color.end ());
}

void
NetColorizer::set_color_of_net (const db::Net *net, const tl::Color &color)
{
  m_custom_color[net] = color;
  emit_colors_changed ();
}

void
NetColorizer::reset_color_of_net (const db::Net *net)
{
  m_custom_color.erase (net);
  emit_colors_changed ();
}

void
NetColorizer::clear ()
{
  m_net_index_by_object.clear ();
  m_custom_color.clear ();
  emit_colors_changed ();
}

void
NetColorizer::begin_changes ()
{
  if (m_signals_enabled) {
    m_update_needed = false;
    m_signals_enabled = false;
  }
}

void
NetColorizer::end_changes ()
{
  if (! m_signals_enabled) {
    m_signals_enabled = true;
    if (m_update_needed) {
      colors_changed ();
    }
    m_update_needed = false;
  }
}

void
NetColorizer::emit_colors_changed ()
{
  if (! m_signals_enabled) {
    m_update_needed = true;
  } else {
    colors_changed ();
  }
}

tl::Color
NetColorizer::color_of_net (const db::Net *net) const
{
  if (! net) {
    return tl::Color ();
  }

  std::map<const db::Net *, tl::Color>::const_iterator c = m_custom_color.find (net);
  if (c != m_custom_color.end ()) {
    return c->second;
  }

  if (m_auto_colors_enabled) {

    const db::Circuit *circuit = net->circuit ();

    size_t index = 0;

    std::map<const db::Net *, size_t>::iterator cc = m_net_index_by_object.find (net);
    if (cc == m_net_index_by_object.end ()) {

      size_t i = 0;
      for (db::Circuit::const_net_iterator n = circuit->begin_nets (); n != circuit->end_nets (); ++n, ++i) {
        m_net_index_by_object.insert (std::make_pair (n.operator-> (), i));
        if (n.operator-> () == net) {
          index = i;
        }
      }

    } else {
      index = cc->second;
    }

    return m_auto_colors.color_by_index ((unsigned int) index);

  } else {
    return tl::Color ();
  }
}

}
