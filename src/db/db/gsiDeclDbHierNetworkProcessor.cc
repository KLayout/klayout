
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

#include "gsiDecl.h"
#include "dbHierNetworkProcessor.h"

namespace gsi
{

Class<db::Connectivity> decl_dbConnectivity ("db", "Connectivity",
  gsi::method ("connect", (void (db::Connectivity::*) (unsigned int)) &db::Connectivity::connect, gsi::arg ("layer"),
    "@brief Specifies intra-layer connectivity.\n"
  ) +
  gsi::method ("connect", (void (db::Connectivity::*) (unsigned int, unsigned int)) &db::Connectivity::connect, gsi::arg ("layer_a"), gsi::arg ("layer_b"),
    "@brief Specifies inter-layer connectivity.\n"
  ) +
  gsi::method ("connect_global", (size_t (db::Connectivity::*) (unsigned int, const std::string &)) &db::Connectivity::connect_global, gsi::arg ("layer"), gsi::arg ("global_net_name"),
    "@brief Connects the given layer to the global net given by name.\n"
    "Returns the ID of the global net."
  ) +
  gsi::method ("global_net_name", &db::Connectivity::global_net_name, gsi::arg ("global_net_id"),
    "@brief Gets the name for a given global net ID.\n"
  ) +
  gsi::method ("global_net_id", &db::Connectivity::global_net_id, gsi::arg ("global_net_name"),
    "@brief Gets the ID for a given global net name.\n"
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
  "This class has been introduced in version 0.26.\n"
);

}
