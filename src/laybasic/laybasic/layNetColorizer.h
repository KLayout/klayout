
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


#ifndef HDR_layNetColorizer
#define HDR_layNetColorizer

#include "laybasicCommon.h"
#include "layColorPalette.h"
#include "tlColor.h"
#include "tlEvents.h"

#include <map>
#include <memory>

namespace db
{
  class Net;
}

namespace lay
{

// ----------------------------------------------------------------------------------
//  NetColorizer definition

class LAYBASIC_PUBLIC NetColorizer
  : public tl::Object
{
public:
  NetColorizer ();

  void configure (const tl::Color &marker_color, const lay::ColorPalette *auto_colors);
  bool has_color_for_net (const db::Net *net);
  void set_color_of_net (const db::Net *net, const tl::Color &color);
  void reset_color_of_net (const db::Net *net);
  void clear ();

  tl::Color color_of_net (const db::Net *net) const;

  const tl::Color &marker_color () const
  {
    return m_marker_color;
  }

  void begin_changes ();
  void end_changes ();

  tl::Event colors_changed;

private:
  tl::Color m_marker_color;
  lay::ColorPalette m_auto_colors;
  bool m_auto_colors_enabled;
  std::map<const db::Net *, tl::Color> m_custom_color;
  bool m_update_needed;
  bool m_signals_enabled;
  mutable std::map<const db::Net *, size_t> m_net_index_by_object;

  void emit_colors_changed ();
};

} // namespace lay

#endif

