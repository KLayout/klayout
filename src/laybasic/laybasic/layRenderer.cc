
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



#include "layRenderer.h"
#include "layCanvasPlane.h"
#include "laySnap.h"

#include "tlAlgorithm.h"

#include <algorithm>

namespace lay 
{

// ----------------------------------------------------------------------------------------------
//  Renderer implementation

Renderer::Renderer (unsigned int width, unsigned int height, double resolution)
  : m_draw_texts (true),
    m_draw_properties (false),
    m_draw_description_property (false),
    m_default_text_size (16),
    m_default_text_size_dbl (16),
    m_apply_text_trans (true),
    m_precise (false),
    m_xfill (false),
    m_font (db::DefaultFont),
    m_width (width), m_height (height),
    m_resolution (resolution)
{
  // .. nothing else ..
}

void 
Renderer::draw_propstring (const db::Shape &shape, const db::PropertiesRepository *prep, lay::CanvasPlane *text, const db::CplxTrans &trans)
{
  if (! shape.has_prop_id ()) {
    return;
  }

  db::DPoint dp;

  if (shape.is_text ()) {
    dp = trans * (db::Point () + shape.text_trans ().disp ());
  } else if (shape.is_box ()) {
    dp = trans (shape.box ().p1 ());
  } else if (shape.is_point ()) {
    dp = trans (shape.point ());
  } else if (shape.is_polygon ()) {
    db::Shape::polygon_edge_iterator e = shape.begin_edge (); 
    dp = trans ((*e).p1 ());
  } else if (shape.is_edge ()) {
    dp = trans (shape.edge ().p1 ());
  } else if (shape.is_path ()) {
    dp = trans (*shape.begin_point ());
  } else {
    return;
  }

  if (shape.has_prop_id () && prep && text && (m_draw_properties || m_draw_description_property)) {
    if (m_draw_properties) {
      draw_propstring (shape.prop_id (), prep, dp, text, trans);
    }
    if (m_draw_description_property) {
      draw_description_propstring (shape.prop_id (), prep, dp, text, trans);
    }
  }
}

void 
Renderer::draw_propstring (db::properties_id_type id, 
                           const db::PropertiesRepository *prep, const db::DPoint &pref, 
                           lay::CanvasPlane *text, const db::CplxTrans &trans)
{
  db::DPoint tp1 (pref + db::DVector (2.0, -2.0));
  db::DPoint tp2 (pref + db::DVector (2.0, -2.0 - trans.ctrans (m_default_text_size)));

  std::string ptext;

  const char *sep = "";
  const db::PropertiesRepository::properties_set &props = prep->properties (id);
  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {
    ptext += sep;
    sep = "\n";
    ptext += prep->prop_name (p->first).to_string ();
    ptext += ": ";
    ptext += p->second.to_string ();
  }

  draw (db::DBox (tp1, tp2), ptext, m_font,
          db::HAlignLeft, db::VAlignTop, 
          db::DFTrans (db::DFTrans::r0), 0, 0, 0, text);
}

void 
Renderer::draw_description_propstring (db::properties_id_type id, 
                                       const db::PropertiesRepository *prep, const db::DPoint &pref, 
                                       lay::CanvasPlane *text, const db::CplxTrans &trans)
{
  db::DPoint tp1 (pref + db::DVector (5.0, -5.0));
  db::DPoint tp2 (pref + db::DVector (5.0, -5.0 - trans.ctrans (m_default_text_size)));

  const db::PropertiesRepository::properties_set &props = prep->properties (id);
  //  TODO: get rid of this const_cast hack (i.e. by a mutable definition inside the properties repository)
  db::property_names_id_type dn = (const_cast<db::PropertiesRepository *> (prep))->prop_name_id (tl::Variant ("description"));

  db::PropertiesRepository::properties_set::const_iterator dv = props.find (dn);
  if (dv != props.end ()) {

    draw (db::DBox (tp1, tp2), dv->second.to_string (), m_font,
            db::HAlignLeft, db::VAlignTop, 
            db::DFTrans (db::DFTrans::r0), 0, 0, 0, text);

  }
}

}


