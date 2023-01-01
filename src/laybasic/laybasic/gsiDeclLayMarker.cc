
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
#include "layMarker.h"
#include "layLayoutViewBase.h"

namespace gsi
{

static 
lay::DMarker *create_marker (lay::LayoutViewBase *view)
{
  return new lay::DMarker (view);
}

static 
void reset_frame_color (lay::DMarker *marker)
{
  marker->set_frame_color (tl::Color ());
}

static 
void set_frame_color (lay::DMarker *marker, unsigned int color) 
{
  marker->set_frame_color (tl::Color (color));
}

static 
unsigned int get_frame_color (const lay::DMarker *marker) 
{
  return marker->get_frame_color ().rgb ();
}

static 
bool has_frame_color (const lay::DMarker *marker) 
{
  return marker->get_frame_color ().is_valid ();
}

static 
void reset_color (lay::DMarker *marker)
{
  marker->set_color (tl::Color ());
}

static 
void set_color (lay::DMarker *marker, unsigned int color) 
{
  marker->set_color (tl::Color (color));
}

static 
unsigned int get_color (const lay::DMarker *marker) 
{
  return marker->get_color ().rgb ();
}

static 
bool has_color (const lay::DMarker *marker) 
{
  return marker->get_color ().is_valid ();
}

Class<lay::DMarker> decl_Marker ("lay", "Marker",
  gsi::constructor ("new", &create_marker, gsi::arg ("view"),
    "@brief Creates a marker\n"
    "\n"
    "A marker is always associated with a view, in which it is shown. The "
    "view this marker is associated with must be passed to the constructor."
  ) +
  gsi::method ("set|set_box", (void (lay::DMarker::*) (const db::DBox &)) &lay::DMarker::set, gsi::arg ("box"),
    "@brief Sets the box the marker is to display\n"
    "\n"
    "Makes the marker show a box. The box must be given in micron units.\n"
    "If the box is empty, no marker is drawn.\n"
    "The set method has been added in version 0.20.\n"
  ) +
  gsi::method ("set|set_text", (void (lay::DMarker::*) (const db::DText &)) &lay::DMarker::set, gsi::arg ("text"),
    "@brief Sets the text the marker is to display\n"
    "\n"
    "Makes the marker show a text. The text must be given in micron units.\n"
    "The set method has been added in version 0.20.\n"
  ) +
  gsi::method ("set|set_edge", (void (lay::DMarker::*) (const db::DEdge &)) &lay::DMarker::set, gsi::arg ("edge"),
    "@brief Sets the edge the marker is to display\n"
    "\n"
    "Makes the marker show a edge. The edge must be given in micron units.\n"
    "The set method has been added in version 0.20.\n"
  ) +
  gsi::method ("set|set_path", (void (lay::DMarker::*) (const db::DPath &)) &lay::DMarker::set, gsi::arg ("path"),
    "@brief Sets the path the marker is to display\n"
    "\n"
    "Makes the marker show a path. The path must be given in micron units.\n"
    "The set method has been added in version 0.20.\n"
  ) +
  gsi::method ("set|set_polygon", (void (lay::DMarker::*) (const db::DPolygon &)) &lay::DMarker::set, gsi::arg ("polygon"),
    "@brief Sets the polygon the marker is to display\n"
    "\n"
    "Makes the marker show a polygon. The polygon must be given in micron units.\n"
    "The set method has been added in version 0.20.\n"
  ) + 
  gsi::method_ext ("color=", set_color, gsi::arg ("color"),
    "@brief Sets the color of the marker\n"
    "The color is a 32bit unsigned integer encoding the RGB values in the lower 3 bytes (blue in the lowest significant byte). "
    "The color can be reset with \\reset_color, in which case, the default foreground color is used."
  ) +
  gsi::method_ext ("reset_color", reset_color,
    "@brief Resets the color of the marker\n"
    "See \\set_color for a description of the color property of the marker."
  ) +
  gsi::method_ext ("color", get_color,
    "@brief Gets the color of the marker\n"
    "This value is valid only if \\has_color? is true."
  ) +
  gsi::method_ext ("has_color?", has_color,
    "@brief Returns a value indicating whether the marker has a specific color\n"
  ) +
  gsi::method_ext ("frame_color=", set_frame_color, gsi::arg ("color"),
    "@brief Sets the frame color of the marker\n"
    "The color is a 32bit unsigned integer encoding the RGB values in the lower 3 bytes (blue in the lowest significant byte). "
    "The color can be reset with \\reset_frame_color, in which case the fill color is used.\n"
    "The set method has been added in version 0.20.\n"
  ) +
  gsi::method_ext ("reset_frame_color", reset_frame_color,
    "@brief Resets the frame color of the marker\n"
    "See \\set_frame_color for a description of the frame color property of the marker."
    "The set method has been added in version 0.20.\n"
  ) +
  gsi::method_ext ("frame_color", get_frame_color,
    "@brief Gets the frame color of the marker\n"
    "This value is valid only if \\has_frame_color? is true."
    "The set method has been added in version 0.20.\n"
  ) +
  gsi::method_ext ("has_frame_color?", has_frame_color,
    "@brief Returns a value indicating whether the marker has a specific frame color\n"
    "The set method has been added in version 0.20.\n"
  ) +
  gsi::method ("dismissable=", &lay::DMarker::set_dismissable, gsi::arg ("flag"),
    "@brief Sets a value indicating whether the marker can be hidden\n"
    "Dismissable markers can be hidden setting \"View/Show Markers\" to \"off\". "
    "The default setting is \"false\" meaning the marker can't be hidden.\n"
    "\n"
    "This attribute has been introduced in version 0.25.4."
  ) +
  gsi::method ("dismissable?", &lay::DMarker::get_dismissable,
    "@brief Gets a value indicating whether the marker can be hidden\n"
    "See \\dismissable= for a description of this predicate."
  ) +
  gsi::method ("line_width=", &lay::DMarker::set_line_width, gsi::arg ("width"),
    "@brief Sets the line width of the marker\n"
    "This is the width of the line drawn for the outline of the marker."
  ) +
  gsi::method ("line_width", &lay::DMarker::get_line_width,
    "@brief Gets the line width of the marker\n"
    "See \\line_width= for a description of the line width."
  ) +
  gsi::method ("vertex_size=", &lay::DMarker::set_vertex_size, gsi::arg ("size"),
    "@brief Sets the vertex size of the marker\n"
    "This is the size of the rectangles drawn for the vertices object."
  ) +
  gsi::method ("vertex_size", &lay::DMarker::get_vertex_size,
    "@brief Gets the vertex size of the marker\n"
    "See \\vertex_size= for a description."
  ) +
  gsi::method ("halo=", &lay::DMarker::set_halo, gsi::arg ("halo"),
    "@brief Sets the halo flag\n"
    "The halo flag is either -1 (for taking the default), 0 to disable the halo or 1 to enable it. "
    "If the halo is enabled, a pixel border with the background color is drawn around the marker, the "
    "vertices and texts."
  ) +
  gsi::method ("halo", &lay::DMarker::get_halo,
    "@brief Gets the halo flag\n"
    "See \\halo= for a description of the halo flag."
  ) +
  gsi::method ("dither_pattern=", &lay::DMarker::set_dither_pattern, gsi::arg ("index"),
    "@brief Sets the stipple pattern index\n"
    "A value of -1 or less than zero indicates that the marker is not filled. Otherwise, the "
    "value indicates which pattern to use for filling the marker."
  ) +
  gsi::method ("dither_pattern", &lay::DMarker::get_dither_pattern,
    "@brief Gets the stipple pattern index\n"
    "See \\dither_pattern= for a description of the stipple pattern index."
  ) +
  gsi::method ("line_style=", &lay::DMarker::set_line_style, gsi::arg ("index"),
    "@brief Sets the line style\n"
    "The line style is given by an index. 0 is solid, 1 is dashed and so forth.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("line_style", &lay::DMarker::get_line_style,
    "@brief Get the line style\n"
    "See \\line_style= for a description of the line style index."
    "\n"
    "This method has been introduced in version 0.25."
  ),
  "@brief The floating-point coordinate marker object\n"
  "\n"
  "The marker is a visual object that \"marks\" (highlights) a \n"
  "certain area of the layout, given by a database object. "
  "This object accepts database objects with floating-point coordinates in micron values.\n"
);
  
}
