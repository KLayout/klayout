
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


#include "pexRExtractorTech.h"

#include "tlString.h"

namespace pex
{

// ------------------------------------------------------------------

std::string
RExtractorTechVia::to_string () const
{
  std::string res = "Via(";
  res += tl::sprintf ("bottom=L%u, cut=L%u, top=L%u, R=%.12g \xC2\xB5m\xC2\xB2*Ohm", bottom_conductor, cut_layer, top_conductor, resistance);
  if (merge_distance > 1e-10) {
    res += tl::sprintf(", d_merge=%.12g \xC2\xB5m", merge_distance);
  }
  res += ")";
  return res;
}

// ------------------------------------------------------------------

std::string
RExtractorTechConductor::to_string () const
{
  std::string res = "Conductor(";
  res += tl::sprintf ("layer=L%u, R=%.12g Ohm/sq", layer, resistance);

  switch (algorithm) {
  case SquareCounting:
    res += ", algo=SquareCounting";
    break;
  case Tesselation:
    res += ", algo=Tesselation";
    break;
  default:
    break;
  }

  if (triangulation_min_b > 1e-10) {
    res += tl::sprintf(", tri_min_b=%.12g \xC2\xB5m", triangulation_min_b);
  }

  if (triangulation_max_area > 1e-10) {
    res += tl::sprintf(", tri_max_area=%.12g \xC2\xB5m\xC2\xB2", triangulation_max_area);
  }

  res += ")";
  return res;
}

// ------------------------------------------------------------------

RExtractorTech::RExtractorTech ()
  : skip_simplify (false)
{
  //  .. nothing yet ..
}

std::string
RExtractorTech::to_string () const
{
  std::string res;
  if (skip_simplify) {
    res += "skip_simplify=true\n";
  }
  res += tl::join (vias.begin (), vias.end (), "\n");
  res += "\n";
  res += tl::join (conductors.begin (), conductors.end (), "\n");
  return res;
}

}
