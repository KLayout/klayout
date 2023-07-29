
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


#include "dbPCellVariant.h"
#include "dbPCellHeader.h"

#include "tlLog.h"

namespace db
{

PCellVariant::PCellVariant (db::cell_index_type ci, db::Layout &layout, db::pcell_id_type pcell_id, const pcell_parameters_type &parameters)
  : Cell (ci, layout), m_parameters (parameters), m_pcell_id (pcell_id), m_registered (false)
{
  PCellVariant::reregister (); // actually, no "re-register", but the first registration ..
}

PCellVariant::~PCellVariant ()
{
  PCellVariant::unregister ();
}

Cell *
PCellVariant::clone (Layout &layout) const
{
  tl_assert (m_registered);  // don't clone detached PCellVariant's
  Cell *cell = new PCellVariant (cell_index (), layout, m_pcell_id, m_parameters);
  //  copy the cell content
  *cell = *this;
  return cell;
}

void  
PCellVariant::unregister ()
{
  // A PCellVariant that is saved as a member of a transaction will explicitly be unregistered ..
  if (m_registered) {
    PCellHeader *header = pcell_header ();
    if (header) {
      header->unregister_variant (this);
    }
    m_registered = false;
  }
}

void  
PCellVariant::reregister ()
{
  if (! m_registered) {
    PCellHeader *header = pcell_header ();
    if (header) {
      header->register_variant (this);
    }
    m_registered = true;
  }
}

std::string 
PCellVariant::get_basic_name () const
{
  const PCellHeader *header = pcell_header ();
  if (header) {
    return header->get_name ();
  } else {
    return Cell::get_basic_name ();
  }
}

std::string 
PCellVariant::get_display_name () const
{
  const PCellHeader *header = pcell_header ();
  if (header) {
    if (m_display_name.empty ()) {
      return header->get_name () + "*";
    } else {
      return m_display_name;
    }
  } else {
    return Cell::get_basic_name ();
  }
}

tl::Variant
PCellVariant::parameter_by_name (const std::string &name) const
{
  const PCellHeader *header = pcell_header ();
  if (header && header->declaration ()) {

    db::pcell_parameters_type::const_iterator pp = parameters ().begin ();
    const std::vector<db::PCellParameterDeclaration> &pcp = header->declaration ()->parameter_declarations ();
    for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end () && pp != parameters ().end (); ++pd, ++pp) {
      if (pd->get_name () == name) {
        return *pp;
      }
    }

  }

  return tl::Variant ();
}

std::map<std::string, tl::Variant>
PCellVariant::parameters_by_name () const
{
  return parameters_by_name_from_list (parameters ());
}

std::map<std::string, tl::Variant>
PCellVariant::parameters_by_name_from_list (const db::pcell_parameters_type &list) const
{
  std::map<std::string, tl::Variant> param_by_name;

  const PCellHeader *header = pcell_header ();
  if (header && header->declaration ()) {

    db::pcell_parameters_type::const_iterator pp = list.begin ();
    const std::vector<db::PCellParameterDeclaration> &pcp = header->declaration ()->parameter_declarations ();
    for (std::vector<PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end () && pp != parameters ().end (); ++pd, ++pp) {
      param_by_name.insert (std::make_pair (pd->get_name (), *pp));
    }

  }

  return param_by_name;
}

void 
PCellVariant::update (ImportLayerMapping *layer_mapping)
{
  tl_assert (layout () != 0);

  clear_shapes ();
  clear_insts ();

  PCellHeader *header = pcell_header ();
  if (header && header->declaration ()) {

    db::property_names_id_type pn = layout ()->properties_repository ().prop_name_id (tl::Variant ("name"));
    db::property_names_id_type dn = layout ()->properties_repository ().prop_name_id (tl::Variant ("description"));

    std::vector<unsigned int> layer_ids;
    try {

      layer_ids = header->get_layer_indices (*layout (), m_parameters, layer_mapping);

      //  call coerce prior to produce to make sure we have a validated parameter set
      //  (note that we cannot persist parameters from here)
      db::pcell_parameters_type plist = m_parameters;
      header->declaration ()->coerce_parameters (*layout (), plist);

      header->declaration ()->produce (*layout (), layer_ids, plist, *this);

      m_display_name = header->declaration ()->get_display_name (plist);

    } catch (tl::Exception &ex) {

      tl::error << ex.msg ();

      //  put error messages into layout as text objects on error layer
      shapes (layout ()->error_layer ()).insert (db::Text (ex.msg (), db::Trans ()));

    }

    //  produce the shape parameters on the guiding shape layer so they can be edited
    size_t i = 0;
    const std::vector<db::PCellParameterDeclaration> &pcp = header->declaration ()->parameter_declarations ();
    for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pcp.begin (); p != pcp.end (); ++p, ++i) {

      if (i < m_parameters.size () && p->get_type () == db::PCellParameterDeclaration::t_shape && ! p->is_hidden ()) {

        //  use property with name "name" to indicate the parameter name
        db::PropertiesRepository::properties_set props;
        props.insert (std::make_pair (pn, tl::Variant (p->get_name ())));

        if (! p->get_description ().empty ()) {
          props.insert (std::make_pair (dn, tl::Variant (p->get_description ())));
        }

        if (m_parameters[i].is_user<db::DBox> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (db::BoxWithProperties (db::Box (m_parameters[i].to_user<db::DBox> () * (1.0 / layout ()->dbu ())), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::Box> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (db::BoxWithProperties (m_parameters[i].to_user<db::Box> (), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::DEdge> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (db::EdgeWithProperties (db::Edge (m_parameters[i].to_user<db::DEdge> () * (1.0 / layout ()->dbu ())), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::Edge> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (db::EdgeWithProperties (m_parameters[i].to_user<db::Edge> (), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::DPoint> ()) {

          db::DPoint p = m_parameters[i].to_user<db::DPoint> ();
          shapes (layout ()->guiding_shape_layer ()).insert (db::PointWithProperties (db::Point (p * (1.0 / layout ()->dbu ())), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::Point> ()) {

          db::Point p = m_parameters[i].to_user<db::Point> ();
          shapes (layout ()->guiding_shape_layer ()).insert (db::PointWithProperties (p, layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::DPolygon> ()) {

          db::complex_trans<db::DCoord, db::Coord> dbu_trans (1.0 / layout ()->dbu ());
          db::Polygon poly = m_parameters[i].to_user<db::DPolygon> ().transformed (dbu_trans, false);
          //  Hint: we don't compress the polygon since we don't want to loose information
          shapes (layout ()->guiding_shape_layer ()).insert (db::PolygonWithProperties (poly, layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::Polygon> ()) {

          db::Polygon poly = m_parameters[i].to_user<db::Polygon> ();
          //  Hint: we don't compress the polygon since we don't want to loose information
          shapes (layout ()->guiding_shape_layer ()).insert (db::PolygonWithProperties (poly, layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::DPath> ()) {

          db::complex_trans<db::DCoord, db::Coord> dbu_trans (1.0 / layout ()->dbu ());
          shapes (layout ()->guiding_shape_layer ()).insert (db::PathWithProperties (dbu_trans * m_parameters[i].to_user<db::DPath> (), layout ()->properties_repository ().properties_id (props)));

        } else if (m_parameters[i].is_user<db::Path> ()) {

          shapes (layout ()->guiding_shape_layer ()).insert (db::PathWithProperties (m_parameters[i].to_user<db::Path> (), layout ()->properties_repository ().properties_id (props)));

        }

      }

    }

  }
}

}

