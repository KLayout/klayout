
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

#include "gsiDecl.h"
#include "dbHierNetworkProcessor.h"

namespace gsi
{

static std::string
l2s (db::Connectivity::layer_iterator b, db::Connectivity::layer_iterator e)
{
  std::string s;
  for (db::Connectivity::layer_iterator i = b; i != e; ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (i->first);
    if (i->second < 0) {
      s += "-S";
    } else if (i->second > 0) {
      s += "+S";
    }
  }
  return s;
}

static std::string
gn2s (db::Connectivity::global_nets_iterator b, db::Connectivity::global_nets_iterator e)
{
  std::string s;
  for (db::Connectivity::global_nets_iterator i = b; i != e; ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (i->first);
    if (i->second < 0) {
      s += "-S";
    } else if (i->second > 0) {
      s += "+S";
    }
  }
  return s;
}

static std::string
connectivity_to_string (const db::Connectivity *conn)
{
  std::string res;

  for (auto l = conn->begin_layers (); l != conn->end_layers (); ++l) {
    if (conn->begin_connected (*l) != conn->end_connected (*l)) {
      if (! res.empty ()) {
        res += "\n";
      }
      res += tl::to_string (*l) + ":" + l2s (conn->begin_connected (*l), conn->end_connected (*l));
    }
    if (conn->begin_global_connections (*l) != conn->end_global_connections (*l)) {
      if (! res.empty ()) {
        res += "\n";
      }
      res += "G" + tl::to_string (*l) + ":" + gn2s (conn->begin_global_connections (*l), conn->end_global_connections (*l));
    }
  }

  return res;
}

Class<db::Connectivity> decl_dbConnectivity ("db", "Connectivity",
  gsi::method ("connect", (void (db::Connectivity::*) (unsigned int)) &db::Connectivity::connect, gsi::arg ("layer"),
    "@brief Specifies intra-layer connectivity.\n"
    "This method specifies a hard connection between shapes on the given layer. "
    "Without specifying such a connection, shapes on that layer do not form connection regions."
  ) +
  gsi::method ("connect", (void (db::Connectivity::*) (unsigned int, unsigned int)) &db::Connectivity::connect, gsi::arg ("layer_a"), gsi::arg ("layer_b"),
    "@brief Specifies inter-layer connectivity.\n"
    "This method specifies a hard connection between shapes on layer_a and layer_b."
  ) +
  gsi::method ("soft_connect", (void (db::Connectivity::*) (unsigned int, unsigned int)) &db::Connectivity::soft_connect, gsi::arg ("layer_a"), gsi::arg ("layer_b"),
    "@brief Specifies a soft connection between layer_a and layer_b.\n"
    "@param layer_a The 'upper' layer\n"
    "@param layer_b The 'lower' layer\n"
    "Soft connections are made between a lower and an upper layer. The lower layer conceptually is a high-ohmic "
    "(i.e. substrate, diffusion) region that is not intended for signal wiring. The netlist extraction will check "
    "that no routing happens over such regions.\n"
    "\n"
    "Soft connections have in introduced in version 0.29."
  ) +
  gsi::method ("connect_global", (size_t (db::Connectivity::*) (unsigned int, const std::string &)) &db::Connectivity::connect_global, gsi::arg ("layer"), gsi::arg ("global_net_name"),
    "@brief Connects the given layer to the global net given by name.\n"
    "Returns the ID of the global net."
  ) +
  gsi::method ("soft_connect_global", (size_t (db::Connectivity::*) (unsigned int, const std::string &)) &db::Connectivity::soft_connect_global, gsi::arg ("layer"), gsi::arg ("global_net_name"),
    "@brief Soft-connects the given layer to the global net given by name.\n"
    "Returns the ID of the global net.\n"
    "See \\soft_connect for a description of the soft connection feature. The global net is always the "
    "'lower' (i.e. high-ohmic, substrate) part of the soft connection.\n"
    "\n"
    "Soft connections have in introduced in version 0.29."
  ) +
  gsi::method ("global_net_name", &db::Connectivity::global_net_name, gsi::arg ("global_net_id"),
    "@brief Gets the name for a given global net ID.\n"
  ) +
  gsi::method ("global_net_id", &db::Connectivity::global_net_id, gsi::arg ("global_net_name"),
    "@brief Gets the ID for a given global net name.\n"
  ) +
  //  provided for testing purposes mainly.
  gsi::method_ext ("to_s", &connectivity_to_string,
    "@hide\n"
  ),
  "@brief This class specifies connections between different layers.\n"
  "Connections are build using \\connect. There are basically two flavours of connections: intra-layer and inter-layer.\n"
  "\n"
  "Intra-layer connections make nets begin propagated along different shapes on the same net. Without the "
  "intra-layer connections, nets are not propagated over shape boundaries. As this is usually intended, intra-layer connections "
  "should always be specified for each layer.\n"
  "\n"
  "Inter-layer connections connect shapes on different layers. Shapes which touch across layers will be connected if "
  "their layers are specified as being connected through inter-layer \\connect.\n"
  "\n"
  "All layers are specified in terms of layer indexes. Layer indexes are layout layer indexes (see \\Layout class).\n"
  "\n"
  "The connectivity object also manages the global nets. Global nets are substrate for example "
  "and they are propagated automatically from subcircuits to circuits. "
  "Global nets are defined by name and are managed through IDs. To get the name for a given ID, use "
  "\\global_net_name."
  "\n"
  "Starting with version 0.29, soft connections are supported. Soft connections attach to high-ohmic substrate or diffusion "
  "layers (the 'lower' layer) are upon netlist extraction it will be checked that no wiring is routed over such connections. "
  "See \\soft_connect and \\soft_global_connect for details.\n"
  "\n"
  "This class has been introduced in version 0.26.\n"
);

}
