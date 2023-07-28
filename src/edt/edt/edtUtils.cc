
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


#include "dbBox.h"
#include "dbLayout.h"
#include "dbLibrary.h"

#include "edtUtils.h"
#include "edtService.h"

#include "layCellView.h"
#include "layLayoutViewBase.h"
#include "layEditable.h"
#include "tlException.h"

namespace edt {

// -------------------------------------------------------------

std::string pcell_parameters_to_string (const std::map<std::string, tl::Variant> &parameters)
{
  std::string param;

  param = "!";  //  flags PCells
  for (std::map<std::string, tl::Variant>::const_iterator p = parameters.begin (); p != parameters.end (); ++p) {
    param += tl::to_word_or_quoted_string (p->first);
    param += ":";
    param += p->second.to_parsable_string ();
    param += ";";
  }

  return param;
}

std::map<std::string, tl::Variant> pcell_parameters_from_string (const std::string &s)
{
  tl::Extractor ex (s.c_str ());
  std::map<std::string, tl::Variant> pm;

  ex.test ("!");

  try {
    while (! ex.at_end ()) {
      std::string n;
      ex.read_word_or_quoted (n);
      ex.test (":");
      ex.read (pm.insert (std::make_pair (n, tl::Variant ())).first->second);
      ex.test (";");
    }
  } catch (...) {
    //  ignore errors
  }

  return pm;
}

// -------------------------------------------------------------
//  SelectionIterator implementation

SelectionIterator::SelectionIterator (lay::LayoutViewBase *view, bool including_transient)
  : m_transient_mode (false)
{
  mp_edt_services = view->get_plugins <edt::Service> ();

  m_current_service = mp_edt_services.begin ();
  if (m_current_service != mp_edt_services.end ()) {
    m_current_object = (*m_current_service)->selection ().begin ();
  }

  next ();

  if (at_end () && including_transient) {

    m_transient_mode = true;

    m_current_service = mp_edt_services.begin ();
    if (m_current_service != mp_edt_services.end ()) {
      m_current_object = (*m_current_service)->transient_selection ().begin ();
    }

    next ();

  }
}

bool
SelectionIterator::at_end () const
{
  return m_current_service == mp_edt_services.end ();
}

void
SelectionIterator::inc ()
{
  tl_assert (! at_end ());
  ++m_current_object;
}

void
SelectionIterator::next ()
{
  if (at_end ()) {
    return;
  }

  const edt::Service::objects *sel = m_transient_mode ? &(*m_current_service)->transient_selection () : &(*m_current_service)->selection ();

  while (m_current_object == sel->end ()) {

    ++m_current_service;

    if (m_current_service != mp_edt_services.end ()) {

      sel = m_transient_mode ? &(*m_current_service)->transient_selection () : &(*m_current_service)->selection ();
      m_current_object = sel->begin ();

    } else {
      break;
    }

  }
}

// -------------------------------------------------------------
//  TransformationsVariants implementation
//  for a lay::LayoutView

TransformationVariants::TransformationVariants (const lay::LayoutViewBase *view, bool per_cv_and_layer, bool per_cv)
{
  //  build the transformation variants cache

  for (lay::LayerPropertiesConstIterator l = view->begin_layers (); !l.at_end (); ++l) {

    if (! l->has_children ()) {

      unsigned int cvi = (l->cellview_index () >= 0) ? (unsigned int) l->cellview_index () : 0;
      if (view->cellview (cvi).is_valid ()) {

        if (per_cv) {
          std::vector<db::DCplxTrans> &tv = m_per_cv_tv.insert (std::make_pair (cvi, std::vector<db::DCplxTrans> ())).first->second;
          tv.insert (tv.end (), l->trans ().begin (), l->trans ().end ());
        }

        if (l->layer_index () >= 0 && per_cv_and_layer) {
          std::vector<db::DCplxTrans> &tv = m_per_cv_and_layer_tv.insert (std::make_pair (std::make_pair (cvi, (unsigned int) l->layer_index ()), std::vector<db::DCplxTrans> ())).first->second;
          tv.insert (tv.end (), l->trans ().begin (), l->trans ().end ());
        }

        if (per_cv_and_layer) {
          std::vector<db::DCplxTrans> &tv = m_per_cv_and_layer_tv.insert (std::make_pair (std::make_pair (cvi, view->cellview (cvi)->layout ().guiding_shape_layer ()), std::vector<db::DCplxTrans> ())).first->second;
          tv.insert (tv.end (), l->trans ().begin (), l->trans ().end ());
        }

      }

    }

  }

  //  remove duplicates of the list of transformations (not related to layers)
  for (std::map <unsigned int, std::vector<db::DCplxTrans> >::iterator t = m_per_cv_tv.begin (); t != m_per_cv_tv.end (); ++t) {
    std::sort (t->second.begin (), t->second.end ());
    std::vector<db::DCplxTrans>::iterator new_last = std::unique (t->second.begin (), t->second.end ());
    t->second.erase (new_last, t->second.end ());
  }

  //  remove duplicates of the list of transformations (related to layers)
  for (std::map < std::pair<unsigned int, unsigned int>, std::vector<db::DCplxTrans> >::iterator t = m_per_cv_and_layer_tv.begin (); t != m_per_cv_and_layer_tv.end (); ++t) {
    std::sort (t->second.begin (), t->second.end ());
    std::vector<db::DCplxTrans>::iterator new_last = std::unique (t->second.begin (), t->second.end ());
    t->second.erase (new_last, t->second.end ());
  }
}

const std::vector<db::DCplxTrans> *
TransformationVariants::per_cv_and_layer (unsigned int cv, unsigned int layer) const
{
  std::map <std::pair<unsigned int, unsigned int>, std::vector<db::DCplxTrans> >::const_iterator t = m_per_cv_and_layer_tv.find (std::make_pair (cv, layer));
  if (t != m_per_cv_and_layer_tv.end ()) {
    return &t->second;
  } else {
    return 0;
  }
}

const std::vector<db::DCplxTrans> *
TransformationVariants::per_cv (unsigned int cv) const
{
  std::map <unsigned int, std::vector<db::DCplxTrans> >::const_iterator t = m_per_cv_tv.find (cv);
  if (t != m_per_cv_tv.end ()) {
    return &t->second;
  } else {
    return 0;
  }
}

// -------------------------------------------------------------

bool
get_parameters_from_pcell_and_guiding_shapes (db::Layout *layout, db::cell_index_type cell_index, db::pcell_parameters_type &parameters_for_pcell)
{
  //  extract parameters from the guiding shapes
  std::pair<db::Library *, db::cell_index_type> lc = layout->defining_library (cell_index);
  const db::Layout *def_layout = layout;
  if (lc.first) {
    def_layout = &lc.first->layout ();
  }

  std::pair<bool, db::pcell_id_type> lpc = def_layout->is_pcell_instance (lc.second);
  if (! lpc.first) {
    return false;
  }

  //  convert the guiding shapes to parameters
  parameters_for_pcell = def_layout->get_pcell_parameters (lc.second);
  const db::PCellDeclaration *pcell_decl = def_layout->pcell_declaration (lpc.second);
  const std::vector<db::PCellParameterDeclaration> &pcp = pcell_decl->parameter_declarations ();

  db::pcell_parameters_type org_parameters = parameters_for_pcell;

  std::map <std::string, size_t> pname_map;
  for (size_t i = 0; i < pcp.size () && i < parameters_for_pcell.size (); ++i) {
    pname_map.insert (std::make_pair (pcp [i].get_name (), i));
  }

  db::property_names_id_type pn = layout->properties_repository ().prop_name_id ("name");
  db::property_names_id_type dn = layout->properties_repository ().prop_name_id (tl::Variant ("description"));

  db::Shapes &guiding_shapes = layout->cell (cell_index).shapes (layout->guiding_shape_layer ());

  db::Shapes::shape_iterator sh = guiding_shapes.begin (db::ShapeIterator::All);
  while (! sh.at_end ()) {

    if (sh->has_prop_id ()) {

      const db::PropertiesRepository::properties_set &props = layout->properties_repository ().properties (sh->prop_id ());
      db::PropertiesRepository::properties_set::const_iterator pv = props.find (pn);
      if (pv != props.end ()) {

        std::map <std::string, size_t>::const_iterator pnm = pname_map.find (pv->second.to_string ());
        if (pnm != pname_map.end ()) {

          db::CplxTrans dbu_trans (layout->dbu ());

          if (sh->is_box ()) {
            parameters_for_pcell [pnm->second] = tl::Variant (dbu_trans * sh->box ());
          } else if (sh->is_edge ()) {
            parameters_for_pcell [pnm->second] = tl::Variant (dbu_trans * sh->edge ());
          } else if (sh->is_point ()) {
            parameters_for_pcell [pnm->second] = tl::Variant (dbu_trans * sh->point ());
          } else if (sh->is_polygon ()) {
            //  Hint: we don't compress since we don't want to loose information
            parameters_for_pcell [pnm->second] = tl::Variant (sh->polygon ().transformed (dbu_trans, false));
          } else if (sh->is_path ()) {
            parameters_for_pcell [pnm->second] = tl::Variant (dbu_trans * sh->path ());
          }

        }
        
      }

    }

    ++sh;

  }

  //  Note that we have modified the pcell representative's guiding shapes. That is not a good idea: this
  //  will modify other instances which reuse that representative (or library proxy) as well. Before we
  //  create a variant in the calling code we have to revert the shapes back to their initial state which
  //  is consistent with the parameters.
  guiding_shapes.clear ();
  for (size_t i = 0; i < pcp.size () && i < org_parameters.size (); ++i) {

    const db::PCellParameterDeclaration &pd = pcp [i];

    if (pd.get_type () == db::PCellParameterDeclaration::t_shape && ! pd.is_hidden ()) {

      //  use property with name "name" to indicate the parameter name
      db::PropertiesRepository::properties_set props;
      props.insert (std::make_pair (pn, tl::Variant (pd.get_name ())));

      if (! pd.get_description ().empty ()) {
        props.insert (std::make_pair (dn, tl::Variant (pd.get_description ())));
      }

      if (org_parameters[i].is_user<db::DBox> ()) {

        guiding_shapes.insert (db::BoxWithProperties(db::Box (org_parameters[i].to_user<db::DBox> () * (1.0 / layout->dbu ())), layout->properties_repository ().properties_id (props)));

      } else if (org_parameters[i].is_user<db::DEdge> ()) {

        guiding_shapes.insert (db::EdgeWithProperties(db::Edge (org_parameters[i].to_user<db::DEdge> () * (1.0 / layout->dbu ())), layout->properties_repository ().properties_id (props)));

      } else if (org_parameters[i].is_user<db::DPoint> ()) {

        db::DPoint p = org_parameters[i].to_user<db::DPoint> ();
        guiding_shapes.insert (db::PointWithProperties(db::Point (p * (1.0 / layout->dbu ())), layout->properties_repository ().properties_id (props)));

      } else if (org_parameters[i].is_user<db::DPolygon> ()) {

        db::complex_trans<db::DCoord, db::Coord> dbu_trans (1.0 / layout->dbu ());
        //  Hint: we don't compress the polygon since we don't want to loose information
        db::Polygon poly = org_parameters[i].to_user<db::DPolygon> ().transformed (dbu_trans, false);
        guiding_shapes.insert (db::PolygonWithProperties(poly, layout->properties_repository ().properties_id (props)));

      } else if (org_parameters[i].is_user<db::DPath> ()) {

        db::complex_trans<db::DCoord, db::Coord> dbu_trans (1.0 / layout->dbu ());
        guiding_shapes.insert (db::PathWithProperties(dbu_trans * org_parameters[i].to_user<db::DPath> (), layout->properties_repository ().properties_id (props)));

      }

    }

  }

  pcell_decl->coerce_parameters (*layout, parameters_for_pcell);

  return true;
}

}

